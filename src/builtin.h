#ifndef _BUILTIN
#define _BUILTIN

#include "shell.h"

int builtin_exit(context_t* context);
int builtin_echo(context_t* context);
int builtin_type(context_t* context);
int builtin_env(context_t* context);
int builtin_pwd(context_t* context);
int builtin_cd(context_t* context);

#endif
