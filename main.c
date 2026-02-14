#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static void lsh_loop(void);
static char *lsh_read_line(void);
static char **lsh_split_line(char *line);
static int lsh_execute(char **args);
static int lsh_launch(char **args);

/* builtins */
static int lsh_cd(char **args);
static int lsh_help(char **args);
static int lsh_exit(char **args);

static char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

static int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

static int lsh_num_builtins(void) {
  return (int)(sizeof(builtin_str) / sizeof(builtin_str[0]));
}

int main(void) {
  lsh_loop();
  return EXIT_SUCCESS;
}

static void lsh_loop(void) {
  char *line = NULL;
  char **args = NULL;
  int status = 1;

  while (status) {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
    line = NULL;
    args = NULL;
  }
}

static char *lsh_read_line(void) {
  char *line = NULL;
  size_t bufsize = 0;

  ssize_t nread = getline(&line, &bufsize, stdin);
  if (nread == -1) {
    if (feof(stdin)) {
      free(line);
      exit(EXIT_SUCCESS);  // Ctrl-D
    } else {
      free(line);
      perror("lsh: getline");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

static char **lsh_split_line(char *line) {
  int bufsize = LSH_TOK_BUFSIZE;
  int position = 0;

  char **tokens = malloc((size_t)bufsize * sizeof(char *));
  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  char *token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position++] = token;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      char **new_tokens = realloc(tokens, (size_t)bufsize * sizeof(char *));
      if (!new_tokens) {
        free(tokens);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
      tokens = new_tokens;
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }

  tokens[position] = NULL;
  return tokens;
}

static int lsh_launch(char **args) {
  pid_t pid = fork();
  int status = 0;

  if (pid == 0) {
    // child
    execvp(args[0], args);
    perror("lsh");
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // fork error
    perror("lsh");
  } else {
    // parent
    do {
      if (waitpid(pid, &status, WUNTRACED) == -1) {
        perror("lsh: waitpid");
        break;
      }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

static int lsh_execute(char **args) {
  if (args[0] == NULL) {
    return 1; // empty command
  }

  for (int i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

static int lsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

static int lsh_help(char **args) {
  (void)args; // suppress unused parameter warning

  printf("Simple Shell (lsh)\n");
  printf("Type program names and arguments, then hit enter.\n");
  printf("Built-in commands:\n");

  for (int i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

static int lsh_exit(char **args) {
  (void)args; // suppress unused parameter warning
  return 0;
}