#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>

#define HISTORY_CAP 10

typedef struct {
  long number;
  char *cmd;
} HistoryEntry;

typedef struct {
  HistoryEntry entries[HISTORY_CAP];
  int count;
  int start;
  long next_number;
} History;

void history_init(History *h);
void history_destroy(History *h);
void history_add(History *h, const char *cmdline);
void history_print(const History *h);
bool history_get_by_number(const History *h, long n, const char **out_cmd);
bool history_get_last(const History *h, const char **out_cmd);

#endif
