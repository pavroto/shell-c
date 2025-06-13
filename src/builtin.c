#include <stdio.h>

#include "shell.h"

int builtin_exit(context_t* context) {
  int return_code = (context->argc < 2) ? 0 : atoi(context->argv[1]);
  shell_destroy_context(context);
  exit(return_code);
}

int builtin_echo(context_t* context) {
  for (size_t i = 1; i < context->argc; i++)
  {
    if (i < context->argc - 1) {
      printf("%s ", context->argv[i]);
      continue;
    }
    printf("%s", context->argv[i]);
  }

  printf("\n");
  return 0;
}

int builtin_type(context_t* context) {
  if (context->argc < 2)
    return 0;

  int (*builtin_function)(context_t* context) = shell_builtin_lookup(context->argv[1]);

  if (builtin_function != NULL) {
    printf("%s is a shell builtin\n", context->argv[1]);
    return 0;
  }

  fprintf(stderr, "%s: not found\n", context->argv[1]);
  return 1;
}
