#include <stdio.h>

#include "shell.h"

int builtin_exit(context_t* context) {
  int return_code = (context->argc < 2) ? 0 : atoi(context->argv[1]);
  shell_destroy_context(context);
  exit(return_code);
}

int builtin_echo(context_t* context) {
  for (size_t i = 1; i < context->argc; i++)
    printf("%s ", context->argv[i]);

  printf("\n");
  return 0;
}
