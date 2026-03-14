#include "interface.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct raw_kv {
  char key[MAX_KEY_SIZE];
  char value[MAX_VALUE_SIZE];
};

struct kv_buffer {
  struct raw_kv *data;
  size_t count;
  size_t capacity;
  pthread_mutex_t mutex;
};

struct grouped_kv {
  char key[MAX_KEY_SIZE];
  char (*values)[MAX_VALUE_SIZE];
  size_t count;
};

struct map_task {
  const struct mr_input *input;
  void (*map)(const struct mr_in_kv *);
  size_t start;
  size_t end;
};

struct reduce_task {
  const struct grouped_kv *groups;
  void (*reduce)(const struct mr_out_kv *);
  size_t start;
  size_t end;
};

static struct kv_buffer g_intermediate = {0};
static struct kv_buffer g_final = {0};

static int kv_buffer_init(struct kv_buffer *buf) {
  buf->count = 0;
  buf->capacity = 16;
  buf->data = malloc(buf->capacity * sizeof(struct raw_kv));
  if (buf->data == NULL) {
    return -1;
  }
  if (pthread_mutex_init(&buf->mutex, NULL) != 0) {
    free(buf->data);
    buf->data = NULL;
    return -1;
  }
  return 0;
}

static void kv_buffer_destroy(struct kv_buffer *buf) {
  if (buf->data != NULL) {
    free(buf->data);
    buf->data = NULL;
  }
  buf->count = 0;
  buf->capacity = 0;
  pthread_mutex_destroy(&buf->mutex);
}

static int kv_buffer_push(struct kv_buffer *buf, const char *key,
                          const char *value) {
  if (key == NULL || value == NULL) {
    return -1;
  }

  if (strlen(key) >= MAX_KEY_SIZE || strlen(value) >= MAX_VALUE_SIZE) {
    return -1;
  }

  if (pthread_mutex_lock(&buf->mutex) != 0) {
    return -1;
  }

  if (buf->count == buf->capacity) {
    size_t new_capacity = buf->capacity * 2;
    struct raw_kv *new_data =
        realloc(buf->data, new_capacity * sizeof(struct raw_kv));
    if (new_data == NULL) {
      pthread_mutex_unlock(&buf->mutex);
      return -1;
    }
    buf->data = new_data;
    buf->capacity = new_capacity;
  }

  strcpy(buf->data[buf->count].key, key);
  strcpy(buf->data[buf->count].value, value);
  buf->count++;

  pthread_mutex_unlock(&buf->mutex);
  return 0;
}

int mr_emit_i(const char *key, const char *value) {
  return kv_buffer_push(&g_intermediate, key, value);
}

int mr_emit_f(const char *key, const char *value) {
  return kv_buffer_push(&g_final, key, value);
}

static int compare_raw_kv(const void *a, const void *b) {
  const struct raw_kv *ka = a;
  const struct raw_kv *kb = b;
  return strcmp(ka->key, kb->key);
}

static void compute_chunk(size_t total, size_t parts, size_t index,
                          size_t *start, size_t *end) {
  size_t base = total / parts;
  size_t rem = total % parts;

  size_t my_size = base + (index < rem ? 1 : 0);
  size_t offset = index * base + (index < rem ? index : rem);

  *start = offset;
  *end = offset + my_size;
}

static void *map_worker(void *arg) {
  struct map_task *task = arg;

  for (size_t i = task->start; i < task->end; i++) {
    task->map(&task->input->kv_lst[i]);
  }

  return NULL;
}

static void *reduce_worker(void *arg) {
  struct reduce_task *task = arg;

  for (size_t i = task->start; i < task->end; i++) {
    struct mr_out_kv out;
    strcpy(out.key, task->groups[i].key);
    out.value = task->groups[i].values;
    out.count = task->groups[i].count;
    task->reduce(&out);
  }

  return NULL;
}

static int group_sorted_raw(const struct raw_kv *raw, size_t raw_count,
                            struct grouped_kv **groups_out,
                            size_t *group_count_out) {
  *groups_out = NULL;
  *group_count_out = 0;

  if (raw_count == 0) {
    return 0;
  }

  size_t unique_count = 1;
  for (size_t i = 1; i < raw_count; i++) {
    if (strcmp(raw[i - 1].key, raw[i].key) != 0) {
      unique_count++;
    }
  }

  struct grouped_kv *groups = calloc(unique_count, sizeof(struct grouped_kv));
  if (groups == NULL) {
    return -1;
  }

  size_t g = 0;
  size_t i = 0;
  while (i < raw_count) {
    size_t j = i + 1;
    while (j < raw_count && strcmp(raw[i].key, raw[j].key) == 0) {
      j++;
    }

    strcpy(groups[g].key, raw[i].key);
    groups[g].count = j - i;
    groups[g].values = malloc(groups[g].count * sizeof(*groups[g].values));
    if (groups[g].values == NULL) {
      for (size_t k = 0; k < g; k++) {
        free(groups[k].values);
      }
      free(groups);
      return -1;
    }

    for (size_t k = i; k < j; k++) {
      strcpy(groups[g].values[k - i], raw[k].value);
    }

    g++;
    i = j;
  }

  *groups_out = groups;
  *group_count_out = unique_count;
  return 0;
}

static void free_grouped(struct grouped_kv *groups, size_t count) {
  if (groups == NULL) {
    return;
  }

  for (size_t i = 0; i < count; i++) {
    free(groups[i].values);
  }
  free(groups);
}

static int build_output_from_final(struct mr_output *output) {
  output->kv_lst = NULL;
  output->count = 0;

  if (g_final.count == 0) {
    return 0;
  }

  qsort(g_final.data, g_final.count, sizeof(struct raw_kv), compare_raw_kv);

  struct grouped_kv *groups = NULL;
  size_t group_count = 0;
  if (group_sorted_raw(g_final.data, g_final.count, &groups, &group_count) !=
      0) {
    return -1;
  }

  output->kv_lst = calloc(group_count, sizeof(struct mr_out_kv));
  if (output->kv_lst == NULL) {
    free_grouped(groups, group_count);
    return -1;
  }

  output->count = group_count;

  for (size_t i = 0; i < group_count; i++) {
    strcpy(output->kv_lst[i].key, groups[i].key);
    output->kv_lst[i].count = groups[i].count;
    output->kv_lst[i].value =
        malloc(groups[i].count * sizeof(*output->kv_lst[i].value));
    if (output->kv_lst[i].value == NULL) {
      for (size_t j = 0; j < i; j++) {
        free(output->kv_lst[j].value);
      }
      free(output->kv_lst);
      output->kv_lst = NULL;
      output->count = 0;
      free_grouped(groups, group_count);
      return -1;
    }

    for (size_t j = 0; j < groups[i].count; j++) {
      strcpy(output->kv_lst[i].value[j], groups[i].values[j]);
    }
  }

  free_grouped(groups, group_count);
  return 0;
}

int mr_exec(const struct mr_input *input, void (*map)(const struct mr_in_kv *),
            size_t mapper_count, void (*reduce)(const struct mr_out_kv *),
            size_t reducer_count, struct mr_output *output) {
  if (input == NULL || map == NULL || reduce == NULL || output == NULL ||
      mapper_count == 0 || reducer_count == 0) {
    return -1;
  }

  output->kv_lst = NULL;
  output->count = 0;

  if (kv_buffer_init(&g_intermediate) != 0) {
    return -1;
  }
  if (kv_buffer_init(&g_final) != 0) {
    kv_buffer_destroy(&g_intermediate);
    return -1;
  }

  pthread_t *map_threads = malloc(mapper_count * sizeof(pthread_t));
  struct map_task *map_tasks = malloc(mapper_count * sizeof(struct map_task));
  if (map_threads == NULL || map_tasks == NULL) {
    free(map_threads);
    free(map_tasks);
    kv_buffer_destroy(&g_intermediate);
    kv_buffer_destroy(&g_final);
    return -1;
  }

  for (size_t i = 0; i < mapper_count; i++) {
    compute_chunk(input->count, mapper_count, i, &map_tasks[i].start,
                  &map_tasks[i].end);
    map_tasks[i].input = input;
    map_tasks[i].map = map;

    if (pthread_create(&map_threads[i], NULL, map_worker, &map_tasks[i]) != 0) {
      for (size_t j = 0; j < i; j++) {
        pthread_join(map_threads[j], NULL);
      }
      free(map_threads);
      free(map_tasks);
      kv_buffer_destroy(&g_intermediate);
      kv_buffer_destroy(&g_final);
      return -1;
    }
  }

  for (size_t i = 0; i < mapper_count; i++) {
    pthread_join(map_threads[i], NULL);
  }

  free(map_threads);
  free(map_tasks);

  qsort(g_intermediate.data, g_intermediate.count, sizeof(struct raw_kv),
        compare_raw_kv);

  struct grouped_kv *intermediate_groups = NULL;
  size_t intermediate_group_count = 0;
  if (group_sorted_raw(g_intermediate.data, g_intermediate.count,
                       &intermediate_groups, &intermediate_group_count) != 0) {
    kv_buffer_destroy(&g_intermediate);
    kv_buffer_destroy(&g_final);
    return -1;
  }

  pthread_t *reduce_threads = malloc(reducer_count * sizeof(pthread_t));
  struct reduce_task *reduce_tasks =
      malloc(reducer_count * sizeof(struct reduce_task));
  if (reduce_threads == NULL || reduce_tasks == NULL) {
    free(reduce_threads);
    free(reduce_tasks);
    free_grouped(intermediate_groups, intermediate_group_count);
    kv_buffer_destroy(&g_intermediate);
    kv_buffer_destroy(&g_final);
    return -1;
  }

  for (size_t i = 0; i < reducer_count; i++) {
    compute_chunk(intermediate_group_count, reducer_count, i,
                  &reduce_tasks[i].start, &reduce_tasks[i].end);
    reduce_tasks[i].groups = intermediate_groups;
    reduce_tasks[i].reduce = reduce;

    if (pthread_create(&reduce_threads[i], NULL, reduce_worker,
                       &reduce_tasks[i]) != 0) {
      for (size_t j = 0; j < i; j++) {
        pthread_join(reduce_threads[j], NULL);
      }
      free(reduce_threads);
      free(reduce_tasks);
      free_grouped(intermediate_groups, intermediate_group_count);
      kv_buffer_destroy(&g_intermediate);
      kv_buffer_destroy(&g_final);
      return -1;
    }
  }

  for (size_t i = 0; i < reducer_count; i++) {
    pthread_join(reduce_threads[i], NULL);
  }

  free(reduce_threads);
  free(reduce_tasks);
  free_grouped(intermediate_groups, intermediate_group_count);

  int rc = build_output_from_final(output);

  kv_buffer_destroy(&g_intermediate);
  kv_buffer_destroy(&g_final);

  return rc == 0 ? 0 : -1;
}
