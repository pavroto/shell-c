#include <stdio.h>
#include <stdlib.h>

#define PROMPT_MAX 100

int remove_trailing_newline(char* input) {
  for (size_t i = 0; input[i] != '\0'; i++)
    if (input[i] == '\n' && input[i+1] == '\0')
    {
      input[i] = '\0';
      return 0;
    }
  return 0;
}

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  while (1)
  {
    // Uncomment this block to pass the first stage
    printf("$ ");

    // Wait for user input
    char input[PROMPT_MAX];
    fgets(input, PROMPT_MAX, stdin);
  
    remove_trailing_newline(input);
    printf("%s: command not found\n", input);
  }

  return 0;
}
