#ifndef _SHELL
#define _SHELL

#include <stdlib.h>

#define SHELL_ENV_DEFAULT_PROMPT "$ "
#define SHELL_PROMPT_DELIMITERS " \n\t"
#define SHELL_PROMPT_SIZE_MAX 100
#define SHELL_PROMPT_ARGS_COUNT_MAX (SHELL_PROMPT_SIZE_MAX / 2)

typedef struct {
  char* key;
  char* value;
} environment_variable_t;

typedef struct {
  char** envp;
  char** argv;
  size_t argc;
} context_t;

typedef struct {
  char* key; 
  int (* function)(context_t* context);
} shell_command_map_entry_t;

context_t* shell_initiate_context(char** envp);
int shell_destroy_context(context_t* context);

int shell_get_args(context_t* context);
void shell_destroy_args(context_t* context);
void shell_clean_args(context_t* context);

int (*shell_builtin_lookup(char* key))(context_t* context);
char* shell_path_executable_lookup(context_t* context, char* key);
int shell_command_router(context_t* context);

int shell_unset_env_var(char* key);
int shell_set_env_var(char* key, char* value, int overwrite);
environment_variable_t* shell_get_env_var(char* key);

#endif
