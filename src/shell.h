#ifndef _SHELL
#define _SHELL

#include <stdlib.h>

#define SHELL_ENV_DEFAULT_PROMPT "$ "
#define SHELL_PROMPT_DELIMITERS " \n\t"
#define SHELL_PROMPT_SIZE_MAX 100
#define SHELL_PROMPT_ARGS_COUNT_MAX (SHELL_PROMPT_SIZE_MAX / 2)

typedef struct environment_variable_struct {
  char* key;
  char* value;

  struct environment_variable_struct* next;
  struct environment_variable_struct* previous;
} environment_variable_t;

typedef struct {
  environment_variable_t* head;
  size_t size;
} environment_t;

typedef struct {
  environment_t* environment;
  char** argv;
  int argc;
} context_t;

typedef struct {
  char* key; 
  int (* function)(context_t* context);
} shell_command_map_entry_t;

context_t* shell_initiate_context();
int shell_destroy_context(context_t* context);

int shell_get_args(context_t* context);
void shell_destroy_args(context_t* context);
void shell_clean_args(context_t* context);

int (*shell_builtin_lookup(char* key))(context_t* context);
int shell_command_router(context_t* context);

#endif
