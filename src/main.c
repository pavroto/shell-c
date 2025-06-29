#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "shell.h"

// TODO: Handle interruption

extern context_t* shell_context;

int main(int argc, char** argv, char** envp) {
  setbuf(stdout, NULL);

  if (shell_initiate_context(envp) == NULL) {
    fprintf(stderr, "CONTEXT INITIATION FAILED. EXITING.");
    return 1;
  }

  while (1)
  {
    shell_get_args(shell_context);
    shell_command_router(shell_context);
    shell_clean_args(shell_context);
  }

  return 0;
}
