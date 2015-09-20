#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "io.h"
#include "parse.h"
#include "process.h"
#include "shell.h"

#include <fcntl.h> // for open
#include <unistd.h> // for close

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_quit(tok_t arg[]);
int cmd_help(tok_t arg[]);
int cmd_pwd(tok_t arg[]);
int cmd_cd(tok_t arg[]);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(tok_t args[]);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_quit, "quit", "quit the command shell"},
  {cmd_pwd, "pwd", "prints current working directory"},
  {cmd_cd, "cd", "change to a new working directory"}
};

/** 
  * Prints current working directory
  */
int cmd_pwd(tok_t arg[]) {
  char cwdBuffer[1024];
  if (getcwd(cwdBuffer, sizeof(cwdBuffer)) != NULL ) {
    fprintf(stdout, "%s\n", cwdBuffer);
  }
  return 1;
}

/**
  * Change to new working directory
  */
int cmd_cd(tok_t arg[]) {
  // fprintf(stdout, "Trying to cd into: %s\n", arg[0]);
  if (chdir(arg[0]) == -1) {
    fprintf(stderr, "cd: %s: No such file or directory\n", arg[0]);
  }
  return 1;
}

/**
 * Prints a helpful description for the given command
 */
int cmd_help(tok_t arg[]) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++) {
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  }
  return 1;
}

/**
 * Quits this shell
 */
int cmd_quit(tok_t arg[]) {
  exit(0);
  return 1;
}

/**
 * Looks up the built-in command, if it exists.
 */
int lookup(char cmd[]) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++) {
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0)) return i;
  }
  return -1;
}

/**
 * Intialization procedures for this shell
 */
void init_shell() {
  /* Check if we are running interactively */
  shell_terminal = STDIN_FILENO;
  shell_is_interactive = isatty(shell_terminal);

  if(shell_is_interactive){
    /* Force the shell into foreground */
    while(tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);

    // Ignore certain errors
    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);

  }
}

int shell(int argc, char *argv[]) {
  char *input_bytes;
  tok_t *tokens;
  int line_num = 0;
  int fundex = -1;

  init_shell();

  if (shell_is_interactive)
    /* Please only print shell prompts when standard input is not a tty */
    fprintf(stdout, "%d: ", line_num);

  while ((input_bytes = freadln(stdin))) {
    tokens = get_toks(input_bytes);
    fundex = lookup(tokens[0]);
    if (fundex >= 0) {
      cmd_table[fundex].fun(&tokens[1]);
    } else {
      /* REPLACE this to run commands as programs. */

      // fork child process
      pid_t pid = fork();

      // check if process should be background
      int tokenLen = 0;
      bool inBackground = false;
      while (tokens[tokenLen] != NULL ) {
        tokenLen += 1;
      }
      if (tokenLen >= 2) {
        if (strcmp("&",tokens[tokenLen -1]) == 0) {
          inBackground = true;
          // tokens[tokenLen - 1] = NULL;
        }
      }

      //check if child process
      if (pid == 0) {

        if (inBackground) {
          put_process_in_background(pid, true);
        } else {
          wait(&pid);
        }
        // put child process on foreground
        // tcsetpgrp(shell_terminal , pid);
        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);


        // handle stdin/stdout redirect; </>
        // tokenLen = 0;
        // while (tokens[tokenLen] != NULL ) {
        //   tokenLen += 1;
        // }
        if (tokenLen > 2) {
          if (strcmp(">",tokens[tokenLen - 2]) == 0) {
            // do redirect
            int openInt = open(tokens[tokenLen-1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if (openInt >= 0) {
              dup2(openInt, 1);
              close(openInt);
              tokens[tokenLen -2] = NULL;
            }
          } else if (strcmp("<",tokens[tokenLen - 2]) == 0) {
            // do redirect
            int openInt = open(tokens[tokenLen-1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if (openInt >= 0) {
              dup2(openInt, 0);
              close(openInt);
              tokens[tokenLen -2] = NULL;
            }
          } 
        }

        // try to execute command without path
        if (execv(tokens[0],tokens) == -1) {

          // search through path string for command resolution
          char * pathString = getenv("PATH");
          char * tokenPath = strtok(pathString, ":");
          char path[1024];
          while (tokenPath != NULL) {
            strcpy(path,tokenPath);
            strcat(path,"/");
            strcat(path,tokens[0]);
            if (execv(path,tokens) == -1) {
              tokenPath = strtok(NULL, ":");
            } else {
              break;
            }
          }
        }

        // end of child process
        return 1;

      } else {
        // parent process wait for child to finish
        int exitInfo;
        waitpid(pid, &exitInfo , 0);
        tcsetpgrp(shell_terminal, getpid());
      }

      // fprintf(stdout, "This shell doesn't know how to run programs.\n");
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);
  }

  return 0;
}






































