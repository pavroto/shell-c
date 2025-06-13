#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "shell.h"
#include "builtin.h"

uint8_t shell_exit_status;
context_t* shell_context;

context_t* shell_initiate_context() {
  shell_context = (context_t*)malloc(sizeof(context_t));
  shell_context->environment = (environment_t*)malloc(sizeof(environment_t));
  shell_context->environment->size = 0;

  shell_context->argv = (char**)calloc(SHELL_PROMPT_ARGS_COUNT_MAX, sizeof(char*));
  shell_context->argc = 0;

  return shell_context;
}

int shell_destroy_context(context_t* context) {

  // Free environment variables
  for (size_t i = 0; i < context->environment->size; i++) {
    free(context->environment->head->key);
    free(context->environment->head->value);
    free(context->environment->head);
  }

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
  else 
    new_variable->previous = NULL;

  return new_variable;
}

inline uint8_t get_exit_status() {
  // TODO: Get and set this value in environment variables
  return shell_exit_status;
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

  do
  {
    size_t token_size = strlen(token) + 1;
    context->argv[context->argc] = (char*)malloc(token_size);
    strncpy(context->argv[context->argc], token, token_size);
    context->argc += 1;
  } while ((token = strtok(NULL, SHELL_PROMPT_DELIMITERS)) != NULL);

  return 0;
}

int (*shell_builtin_lookup(char* key))(context_t* context) {
  static shell_command_map_entry_t command_map[] = {
    { "exit", builtin_exit },
    { "echo", builtin_echo },
    { "type", builtin_type },
  };

  static size_t command_map_size = sizeof(command_map) / sizeof(shell_command_map_entry_t);
  
  for (size_t i = 0; i < command_map_size; i++) {
    if (strcmp(command_map[i].key, key) == 0) 
      return command_map[i].function;
  }
  return NULL; 
}

int shell_command_router(context_t* context) {
  int (*builtin_function)(context_t* context) = shell_builtin_lookup(context->argv[0]);

  if (builtin_function != NULL)
    return builtin_function(context);

  fprintf(stderr, "%s: command not found\n", context->argv[0]);
  return 1;
}
