#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "shell.h"
#include "builtin.h"

int shell_exit_status;
context_t* shell_context;

int shell_destroy_context(context_t* context) {
  environment_variable_t* previous_var;
  environment_variable_t* current_var = context->environment->head;

  while (current_var != NULL) {
    free (current_var->key);
    free (current_var->value);

    if (current_var->next != NULL) {
      previous_var = current_var;
      current_var = current_var->next;
      free(previous_var);
    }
    else
    {
      free(current_var);
      break;
    }
  }

  free(context->environment);
  free(context);

  return 0;
}

void shell_clean_args(context_t* context) {
  for (int i = 0; i < context->argc; i++)
    free(context->argv[i]);
  context->argc = 0;
}

void shell_destroy_args(context_t* context) {
  shell_clean_args(context);
  free(context->argv);
  free(context);
}

environment_variable_t* shell_get_environment_variable(context_t* context, char* key) {
  if (context == NULL)
    return NULL;

  if (context->environment == NULL)
    return NULL;
  
  environment_variable_t* current_var = context->environment->head;
  while (current_var != NULL) {
    if (strcmp(current_var->key, key) == 0)
      return current_var;
    current_var = current_var->next;
  }
  return NULL;
}

int shell_delete_environment_variable(context_t* context, char* key) {
  if (context == NULL)
    return 1;

  if (context->environment == NULL)
    return 1;

  environment_variable_t* current_var = context->environment->head;

  while (current_var != NULL) {
    if (strcmp(current_var->key, key) == 0) {
      if (context->environment->head == current_var) {
        context->environment->head = current_var->next;
        if (current_var->next != NULL)
          current_var->next->previous = NULL;
      }
      else {
        if (current_var->previous != NULL)
          current_var->previous->next = current_var->next;
        if (current_var->next != NULL)
          current_var->next->previous = current_var->previous;
      }

      free(current_var->key);
      free(current_var->value);
      free(current_var);
      
      context->environment->size--;
      return 0;
    }
  }

  return 0;
}

environment_variable_t* shell_set_environment_variable(context_t* context, char* key, char* value) {
  environment_variable_t* current_var = context->environment->head;

  if (value[0] == '\0') {
    fprintf(stderr, "Trying to delete %s variable\n", key);
    shell_delete_environment_variable(context, key);
    return NULL;
  }

  while (current_var != NULL) {
    if (strcmp(current_var->key, key) == 0) {
      free(current_var->value);
      size_t value_size = strlen(value) + 1;
      current_var->value = (char*)malloc(value_size);
      strncpy(current_var->value, value, value_size);
      return current_var;
    }

    if (current_var->next == NULL)
      break;

    current_var = current_var->next;
  } 

  environment_variable_t* new_variable = (environment_variable_t*)malloc(sizeof(environment_variable_t));

  size_t key_size = strlen(key) + 1;
  size_t value_size = strlen(value) + 1;

  new_variable->key = (char*)malloc(key_size);
  new_variable->value = (char*)malloc(value_size);

  strncpy(new_variable->key, key, key_size);
  strncpy(new_variable->value, value, value_size);

  new_variable->key[key_size-1] = '\0';
  new_variable->value[value_size-1] = '\0';

  new_variable->next = NULL;

  if (current_var != NULL) {
    new_variable->previous = current_var;
    current_var->next = new_variable; 
  }
  else {
    new_variable->previous = NULL;
    context->environment->head = new_variable;
  }

  context->environment->size += 1;

  return new_variable;
}

void shell_clean_environment(context_t* context) {
  environment_variable_t* cur = context->environment->head;
  environment_variable_t* previous = NULL;
  while (cur != NULL) {
    free(cur->key);
    free(cur->value);

    previous = cur;
    cur = cur->next;

    if (previous != NULL)
      free(previous);
  }

  context->environment->head = NULL;
  context->environment->size = 0;
}

context_t* shell_initiate_context(char** envp) {
  if ((shell_context = (context_t*)malloc(sizeof(context_t))) == NULL) {
    fprintf(stderr, "Context memory allocation failed\n");
    return NULL;
  }

  if ((shell_context->environment = (environment_t*)malloc(sizeof(environment_t))) == NULL) {
    free(shell_context);
    fprintf(stderr, "Context Environment memory allocation failed\n");
    return NULL;
  }

  shell_context->environment->size = 0;

  if ((shell_context->argv = (char**)calloc(SHELL_PROMPT_ARGS_COUNT_MAX, sizeof(char*))) == NULL) {
    free(shell_context->environment);
    free(shell_context);
    fprintf(stderr, "Context Arguments memory allocation failed\n");
    return NULL;
  }

  shell_context->argc = 0;

  for (char** env = envp; *env != 0; env++) {
    char* variable;
    if ((variable = strdup(*env)) == NULL) {
      shell_clean_environment(shell_context);
      free(shell_context->environment);
      free(shell_context);
      return NULL;
    }

    char* key = strtok(variable, "=");
    char* value = strtok(NULL, "=");
    
    if (shell_set_environment_variable(shell_context, key, value) == NULL) {
      fprintf(stderr, "WARNING: Unable to set environment variable %s\n", key);
    }
    free(variable);
  }

  return shell_context;
}

inline uint8_t get_exit_status() {
  // TODO: Get and set this value in environment variables
  return shell_exit_status;
}

char* shell_path_executable_lookup(context_t* context, char* key) {
  DIR* current_dir;
  struct dirent* ep;

  environment_variable_t* path_variable = shell_get_environment_variable(context, "PATH");
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

char** shell_envp_compose(context_t* context) {
  char** envp = (char**)malloc((context->environment->size + 1) * sizeof(char*));
  environment_variable_t* current_var = context->environment->head;

  size_t i;
  for (i = 0; i < context->environment->size && current_var != NULL; i++) {
    size_t var_size = strlen(current_var->key) + strlen(current_var->value) + 2;
    envp[i] = (char*)malloc(var_size);
    int desired_size = snprintf(envp[i], var_size, "%s=%s", current_var->key, current_var->value);

    if (desired_size < 0 || (unsigned) desired_size >= var_size){
      fprintf(stderr, "%s: snprintf error\n", __func__);
      for (size_t k = 0; k <= i; k++) 
        free(envp[k]);
      free(envp);
      return NULL;
    }

    current_var = current_var->next;
  }

  envp[i+1] = NULL;

  return envp;
}

int shell_get_args(context_t* context) {

  if (context == NULL) {
    fprintf(stderr, "Context not initiated\n");
    return 1;
  }

  environment_variable_t* env_var_prompt = shell_get_environment_variable(context, "PROMPT");

  if (env_var_prompt == NULL)
    env_var_prompt = shell_set_environment_variable(context, "PROMPT", SHELL_ENV_DEFAULT_PROMPT);

  if (env_var_prompt != NULL)
    printf("%s", env_var_prompt->value);
  else 
    printf(SHELL_ENV_DEFAULT_PROMPT);

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
      environment_variable_t* var;

      if ((var = shell_get_environment_variable(context, token+1)) == NULL) 
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
  };

  static size_t command_map_size = sizeof(command_map) / sizeof(shell_command_map_entry_t);
  
  for (size_t i = 0; i < command_map_size; i++) {
    if (strcmp(command_map[i].key, key) == 0) 
      return command_map[i].function;
  }
  return NULL; 
}

int shell_command_router(context_t* context) {
  char** envp; 
  char* executable_path;
  int last_exit_status;
  int (*builtin_function)(context_t* context) = shell_builtin_lookup(context->argv[0]);

  if (builtin_function != NULL)
    return builtin_function(context);

  if ((executable_path = shell_path_executable_lookup(context, context->argv[0])) != NULL) {
    pid_t pid = fork();
    int return_code = 0;

    if (pid == 0) { // If a child process
      if ((envp = shell_envp_compose(context)) == NULL) {
        free(executable_path);
        exit(127);
      }

      if (execve(executable_path, context->argv, envp) == -1) {
        perror("execve failed");
        free(executable_path);
        free(envp);
        exit(127);
      }

    } else if (pid > 0) { // If a parent process

      waitpid(pid, &return_code, 0);

      if ( WIFEXITED(return_code) ) {
        // TODO: add support for environment variable
        shell_exit_status = WEXITSTATUS(return_code);
      }

      free(envp);

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
