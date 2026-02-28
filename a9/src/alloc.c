#include "alloc.h"

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

static struct header *free_head = NULL;
static enum algs cur_alg = FIRST_FIT;
static int heap_limit = 0;
static int heap_grown = 0;
static void *initial_brk = NULL;

static inline void *hdr_to_payload(struct header *h) {
  return (void *)((char *)h + (intptr_t)sizeof(struct header));
}

static inline struct header *payload_to_hdr(void *p) {
  return (struct header *)((char *)p - (intptr_t)sizeof(struct header));
}

static inline char *block_end(struct header *h) {
  return (char *)h + (intptr_t)h->size;
}

static inline void remove_node(struct header *prev, struct header *cur) {
  if (prev == NULL)
    free_head = cur->next;
  else
    prev->next = cur->next;
  cur->next = NULL;
}

static inline void push_head(struct header *h) {
  h->next = free_head;
  free_head = h;
}

static void coalesce_all(void) {
  int changed = 1;
  while (changed) {
    changed = 0;
    struct header *a_prev = NULL;
    for (struct header *a = free_head; a != NULL; a_prev = a, a = a->next) {
      struct header *b_prev = NULL;
      for (struct header *b = free_head; b != NULL; b_prev = b, b = b->next) {
        if (a == b)
          continue;

        if (block_end(a) == (char *)b) {
          remove_node(b_prev, b);
          a->size += b->size;
          changed = 1;
          break;
        }

        if (block_end(b) == (char *)a) {
          remove_node(a_prev, a);
          b->size += a->size;
          changed = 1;
          break;
        }
      }
      if (changed)
        break;
    }
  }
}

static int grow_heap_once(void) {
  if (heap_limit >= 0 && (heap_grown + INCREMENT) > heap_limit)
    return 0;

  void *p = sbrk(INCREMENT);
  if (p == (void *)-1)
    return 0;

  heap_grown += INCREMENT;

  struct header *h = (struct header *)p;
  h->size = (uint64_t)INCREMENT; // total block size (header + payload)
  h->next = NULL;
  push_head(h);

  coalesce_all();
  return 1;
}

struct found {
  struct header *prev;
  struct header *cur;
};

static struct found find_first_fit(uint64_t need_total) {
  struct header *prev = NULL;
  for (struct header *cur = free_head; cur != NULL;
       prev = cur, cur = cur->next) {
    if (cur->size >= need_total)
      return (struct found){prev, cur};
  }
  return (struct found){NULL, NULL};
}

static struct found find_best_fit(uint64_t need_total) {
  struct header *best = NULL, *best_prev = NULL;
  uint64_t best_rem = 0;

  struct header *prev = NULL;
  for (struct header *cur = free_head; cur != NULL;
       prev = cur, cur = cur->next) {
    if (cur->size < need_total)
      continue;
    uint64_t rem = cur->size - need_total;
    if (best == NULL || rem < best_rem) {
      best = cur;
      best_prev = prev;
      best_rem = rem;
    }
  }
  return (struct found){best_prev, best};
}

static struct found find_worst_fit(uint64_t need_total) {
  struct header *worst = NULL, *worst_prev = NULL;
  uint64_t worst_rem = 0;

  struct header *prev = NULL;
  for (struct header *cur = free_head; cur != NULL;
       prev = cur, cur = cur->next) {
    if (cur->size < need_total)
      continue;
    uint64_t rem = cur->size - need_total;
    if (worst == NULL || rem > worst_rem) {
      worst = cur;
      worst_prev = prev;
      worst_rem = rem;
    }
  }
  return (struct found){worst_prev, worst};
}

static struct found find_block(uint64_t need_total) {
  switch (cur_alg) {
  case FIRST_FIT:
    return find_first_fit(need_total);
  case BEST_FIT:
    return find_best_fit(need_total);
  case WORST_FIT:
    return find_worst_fit(need_total);
  default:
    return find_first_fit(need_total);
  }
}

void allocopt(enum algs alg, int limit) {
  if (initial_brk == NULL)
    initial_brk = sbrk(0);

  void *cur = sbrk(0);
  intptr_t diff = (char *)initial_brk - (char *)cur;
  if (diff != 0)
    sbrk(diff);

  free_head = NULL;
  heap_grown = 0;

  cur_alg = alg;
  heap_limit = limit;
}

void *alloc(int size) {
  if (size <= 0)
    return NULL;

  uint64_t need_total = (uint64_t)size + (uint64_t)sizeof(struct header);

  if (initial_brk == NULL)
    initial_brk = sbrk(0);

  if (free_head == NULL) {
    if (!grow_heap_once())
      return NULL;
  }

  while (1) {
    struct found f = find_block(need_total);
    if (f.cur == NULL) {
      if (!grow_heap_once())
        return NULL;
      continue;
    }

    struct header *h = f.cur;
    remove_node(f.prev, h);

    uint64_t old_total = h->size;
    uint64_t rem_total = old_total - need_total;

    if (old_total >= need_total &&
        rem_total > (uint64_t)sizeof(struct header)) {
      h->size = need_total;

      struct header *r = (struct header *)((char *)h + (intptr_t)need_total);
      r->size = rem_total;
      r->next = NULL;
      push_head(r);
    } else {
      h->size = old_total;
    }

    return hdr_to_payload(h);
  }
}

void dealloc(void *p) {
  if (p == NULL)
    return;

  struct header *h = payload_to_hdr(p);
  push_head(h);
  coalesce_all();
}

struct allocinfo allocinfo(void) {
  struct allocinfo info;
  info.free_size = 0;
  info.free_chunks = 0;
  info.largest_free_chunk_size = 0;

  uint64_t smallest = 0;

  for (struct header *cur = free_head; cur != NULL; cur = cur->next) {
    uint64_t payload = (cur->size >= (uint64_t)sizeof(struct header))
                           ? (cur->size - (uint64_t)sizeof(struct header))
                           : 0;

    info.free_chunks++;
    info.free_size += payload;

    if (payload > info.largest_free_chunk_size)
      info.largest_free_chunk_size = payload;
    if (smallest == 0 || payload < smallest)
      smallest = payload;
  }

  info.smallest_free_chunk_size = (info.free_chunks == 0) ? 0 : smallest;
  return info;
}
