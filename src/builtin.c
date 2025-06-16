#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>

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

  if (shell_builtin_lookup(context->argv[1]) != NULL) {
    printf("%s is a shell builtin\n", context->argv[1]);
    return 0;
  }

  char *executable_path;
  if ((executable_path = shell_path_executable_lookup(context, context->argv[1])) != NULL) {
    printf("%s is %s\n", context->argv[1], executable_path);
    free(executable_path);
    return 0;
  }

  fprintf(stderr, "%s: not found\n", context->argv[1]);
  return 1;
}

int builtin_env(context_t* context) {
  for (size_t i = 0; context->envp[i] != NULL; i++)
    printf("%s\n", context->envp[i]);
  return 0;
}

int builtin_pwd(context_t* context) {
  char cwd[PATH_MAX];
  printf("%s\n", getcwd(cwd, sizeof(cwd)));
  return 0;
}

int builtin_cd(context_t* context) {
  if (context->argc < 2) {
    environment_variable_t* home_var = shell_get_env_var("HOME");

    if (home_var == NULL) {
      fprintf(stderr, "cd: Unable to get $HOME variable.\n");
      return 1;
    }

    if (chdir(home_var->value) == -1) {
      perror(home_var->value);
      free(home_var);
      return 1;
    }

    return 0;
  }

  if (chdir(context->argv[1]) == -1) {
    perror(context->argv[1]);
    return 1;
  }

  return 0;
}
