#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "shell.h"
#include "builtin.h"

int shell_exit_status;
context_t* shell_context;

void shell_clean_args(context_t* context) {
  for (int i = 0; i < context->argc; i++)
    free(context->argv[i]);
  context->argc = 0;
}

void shell_destroy_args(context_t* context) {
  shell_clean_args(context);
  free(context->argv);
}

int shell_destroy_context(context_t* context) {
  shell_destroy_args(context);
  free(context);
  return 0;
}

context_t* shell_initiate_context(char** envp) {
  if ((shell_context = (context_t*)malloc(sizeof(context_t))) == NULL) {
    fprintf(stderr, "Context memory allocation failed\n");
    return NULL;
  }

  if ((shell_context->argv = (char**)calloc(SHELL_PROMPT_ARGS_COUNT_MAX, sizeof(char*))) == NULL) {
    free(shell_context);
    fprintf(stderr, "Context Arguments memory allocation failed\n");
    return NULL;
  }

  shell_context->argc = 0;
  shell_context->envp = envp;

  return shell_context;
}

inline uint8_t get_exit_status() {
  // TODO: Get and set this value in environment variables
  return shell_exit_status;
}

inline int shell_unset_env_var(char* key) {
  if (unsetenv(key) == -1) {
    perror("unsetenv");
    return 1;
  }
  return 0;
}

inline int shell_set_env_var(char* key, char* value, int overwrite) {
  if (setenv(key, value, overwrite) == -1) {
    perror("setenv");
    return 1;
  }
  return 0;
}

environment_variable_t* shell_get_env_var(char* key) {
  char* var_value = getenv(key);

  if (var_value == NULL)
    return NULL;

  environment_variable_t* var_struct = (environment_variable_t*)malloc(sizeof(environment_variable_t));

  size_t var_key_size = strlen(key) + 1;
  size_t var_value_size = strlen(var_value) + 1;

  var_struct->key = (char*)malloc(var_key_size);
  var_struct->value = (char*)malloc(var_value_size);

  strncpy(var_struct->key, key, var_key_size);
  strncpy(var_struct->value, var_value, var_value_size);

  return var_struct;
}

int shell_get_args(context_t* context) {

  if (context == NULL) {
    fprintf(stderr, "Context not initiated\n");
    return 1;
  }

  printf("%s", SHELL_ENV_DEFAULT_PROMPT);

  // Wait for user input
  char user_input[SHELL_PROMPT_SIZE_MAX];
  fgets(user_input, SHELL_PROMPT_SIZE_MAX, stdin);
  
  char* token = strtok(user_input, SHELL_PROMPT_DELIMITERS);

  if (token == NULL) 
    return 0; // empty input;

  size_t i = 0;
  do
  {
    // WARNING: What to do with \" character?
    if (token[0] == '$') {

      environment_variable_t* var = shell_get_env_var(token+1);

      if (var == NULL)
        continue;

      size_t value_size = strlen(var->value) + 1;
      context->argv[context->argc] = (char*)malloc(value_size);
      strncpy(context->argv[context->argc], var->value, value_size);
      context->argc += 1;

      continue;
    }

    size_t token_size = strlen(token) + 1;
    context->argv[context->argc] = (char*)malloc(token_size);
    strncpy(context->argv[context->argc], token, token_size);
    context->argc += 1;

  } while ((token = strtok(NULL, SHELL_PROMPT_DELIMITERS)) != NULL && i++ < SHELL_PROMPT_ARGS_COUNT_MAX - 1);

  context->argv[i+1] = NULL; // Ensure that argv is NULL terminated

  return 0;
}

int (*shell_builtin_lookup(char* key))(context_t* context) {
  static shell_command_map_entry_t command_map[] = {
    { "exit", builtin_exit },
    { "echo", builtin_echo },
    { "type", builtin_type },
    { "env",  builtin_env  },
    { "pwd",  builtin_pwd  },
    { "cd",   builtin_cd   },
  };

  static size_t command_map_size = sizeof(command_map) / sizeof(shell_command_map_entry_t);
  
  for (size_t i = 0; i < command_map_size; i++) {
    if (strcmp(command_map[i].key, key) == 0) 
      return command_map[i].function;
  }
  return NULL; 
}

char* shell_path_executable_lookup(context_t* context, char* key) {
  DIR* current_dir;
  struct dirent* ep;

  environment_variable_t* path_variable = shell_get_env_var("PATH");
  char* path_buf = strdup(path_variable->value);

  char* current_dir_path = strtok(path_buf, ":");

  do {
    if ((current_dir = opendir(current_dir_path)) != NULL) {

      while ((ep = readdir(current_dir)) != NULL) 
        if (strcmp(ep->d_name, key) == 0) {
          size_t executable_absolute_path_size = strlen(current_dir_path) + strlen(ep->d_name) + 2; // '/' and '\0'
          char* executable_absolute_path = (char*)malloc(executable_absolute_path_size);

          int desired_size = snprintf(executable_absolute_path, executable_absolute_path_size, "%s/%s", current_dir_path, ep->d_name);
          if (desired_size < 0 || (unsigned) desired_size >= executable_absolute_path_size){
            fprintf(stderr, "%s: snprintf error\n", __func__);
            free(path_buf);
            free(executable_absolute_path);
            return NULL;
          }
          free (path_buf);
          return executable_absolute_path;
        }

    }
  } while ((current_dir_path = strtok(NULL, ":")) != NULL);

  free(path_buf);
  return NULL;
}

int shell_command_router(context_t* context) {
  char* executable_path;
  int last_exit_status;
  int (*builtin_function)(context_t* context) = shell_builtin_lookup(context->argv[0]);

  if (builtin_function != NULL)
    return builtin_function(context);

  if ((executable_path = shell_path_executable_lookup(context, context->argv[0])) != NULL) {
    pid_t pid = fork();
    int return_code = 0;

    if (pid == 0) { // If a child process
      if (execv(executable_path, context->argv) == -1) {
        perror("execv");
        free(executable_path);
        exit(127);
      }
    } else if (pid > 0) { // If a parent process

      waitpid(pid, &return_code, 0);

      if ( WIFEXITED(return_code) ) {
        // TODO: add support for environment variable
        shell_exit_status = WEXITSTATUS(return_code);
      }

    } else {
      perror("Fork failed");
      free(executable_path);
      return 2;
    }

    free(executable_path);
    return 0;
  }

  fprintf(stderr, "%s: command not found\n", context->argv[0]);
  return 1;
}
