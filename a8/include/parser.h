#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

typedef struct {
  char **argv;
  int argc;
  bool background;
} ParsedCommand;

void parsed_command_init(ParsedCommand *pc);
void parsed_command_destroy(ParsedCommand *pc);
int parse_command_line(const char *line, ParsedCommand *out);

#endif
