#include <stdio.h>
#include <dirent.h>

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
  // TODO: Find 
  if (context->argc < 2)
    return 0;

  int (*builtin_function)(context_t* context) = shell_builtin_lookup(context->argv[1]);

  if (builtin_function != NULL) {
    printf("%s is a shell builtin\n", context->argv[1]);
    return 0;
  }

  char* executable_path;
  if ((executable_path = shell_path_executable_lookup(context, context->argv[1])) != NULL) {
    printf("%s is %s\n", context->argv[1], executable_path);
    free(executable_path);
    return 0;
  }

  fprintf(stderr, "%s: not found\n", context->argv[1]);
  return 1;
}

int builtin_env(context_t* context) {
  if (context == NULL)
    return 1;

  if (context->environment == NULL)
    return 1;

  environment_variable_t* current_var = context->environment->head;

  while (current_var != NULL) {
    printf("%s=%s\n", current_var->key, current_var->value);
    current_var = current_var->next;
  }

  return 0;
}

int builtin_pwd(context_t* context) {

  environment_variable_t* pwd_var;
  if ((pwd_var = shell_get_environment_variable(context, "PWD")) != NULL) {
    printf("%s\n", pwd_var->value);
    return 0;
  }

  if ((pwd_var = shell_set_environment_variable(context, "PWD", "/")) != NULL) {
    printf("%s\n", pwd_var->value);
    return 0;
  }

  return 1;
}
