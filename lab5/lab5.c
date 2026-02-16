#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct header {
  uint64_t size;
  struct header *next;
  int id;
};

void initialize_block(struct header *block, uint64_t size, struct header *next,
                      int id) {
  block->size = size;
  block->next = next;
  block->id = id;
}

int find_first_fit(struct header *free_list_ptr, uint64_t size) {
  struct header *cur = free_list_ptr;

  while (cur != NULL) {
    if (cur->size >= size) {
      return cur->id;
    }
    cur = cur->next;
  }

  return -1;
}

int find_best_fit(struct header *free_list_ptr, uint64_t size) {
  struct header *cur = free_list_ptr;
  int best_id = -1;
  uint64_t best_size = 0;

  while (cur != NULL) {
    if (cur->size >= size) {
      if (best_id == -1 || cur->size < best_size) {
        best_id = cur->id;
        best_size = cur->size;
      }
    }
    cur = cur->next;
  }

  return best_id;
}

int find_worst_fit(struct header *free_list_ptr, uint64_t size) {
  struct header *cur = free_list_ptr;
  int worst_id = -1;
  uint64_t worst_size = 0;

  while (cur != NULL) {
    if (cur->size >= size) {
      if (worst_id == -1 || cur->size > worst_size) {
        worst_id = cur->id;
        worst_size = cur->size;
      }
    }
    cur = cur->next;
  }

  return worst_id;
}

int main(void) {
  struct header *free_block1 = malloc(sizeof(struct header));
  struct header *free_block2 = malloc(sizeof(struct header));
  struct header *free_block3 = malloc(sizeof(struct header));
  struct header *free_block4 = malloc(sizeof(struct header));
  struct header *free_block5 = malloc(sizeof(struct header));

  initialize_block(free_block1, 6, free_block2, 1);
  initialize_block(free_block2, 12, free_block3, 2);
  initialize_block(free_block3, 24, free_block4, 3);
  initialize_block(free_block4, 8, free_block5, 4);
  initialize_block(free_block5, 4, NULL, 5);

  struct header *free_list_ptr = free_block1;

  int first_fit_id = find_first_fit(free_list_ptr, 7);
  int best_fit_id = find_best_fit(free_list_ptr, 7);
  int worst_fit_id = find_worst_fit(free_list_ptr, 7);

  printf("The ID for First-Fit algorithm is: %d\n", first_fit_id);
  printf("The ID for Best-Fit algorithm is: %d\n", best_fit_id);
  printf("The ID for Worst-Fit algorithm is: %d\n", worst_fit_id);

  return 0;
}

/*
Part 2:

coalesce_insert(free_list_head, new):

    if free_list_head == NULL:
        new->next = NULL
        return new

    prev = NULL
    cur = free_list_head

    while cur != NULL and address(cur) < address(new):
        prev = cur
        cur = cur->next

    new->next = cur

    if prev == NULL:
        free_list_head = new
    else:
        prev->next = new

    if prev != NULL and end(prev) == address(new):
        prev->size = prev->size + new->size
        prev->next = new->next
        new = prev

    next = new->next

    if next != NULL and end(new) == address(next):
        new->size = new->size + next->size
        new->next = next->next

    return free_list_head
*/
