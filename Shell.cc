
#include <unistd.h>
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>


#include "Command.hh"
#include "Shell.hh"

int yyparse(void);
extern int yylex_destroy();

char * path;

Shell * Shell::TheShell;

Shell::Shell() {
    this->_level = 0;
    this->_enablePrompt = true;
    this->_listCommands = new ListCommands(); 
    this->_simpleCommand = new SimpleCommand();
    this->_pipeCommand = new PipeCommand();
    this->_currentCommand = this->_pipeCommand;
    if ( !isatty(0)) {
	this->_enablePrompt = false;
    }
}

void Shell::prompt() {
    if (_enablePrompt) {
	printf("myshell>");
	fflush(stdout);
    }
}

void Shell::print() {
    printf("\n--------------- Command Table ---------------\n");
    this->_listCommands->print();
}

void Shell::clear() {
    this->_listCommands->clear();
    this->_simpleCommand->clear();
    this->_pipeCommand->clear();
    this->_currentCommand->clear();
    this->_level = 0;
}

void Shell::execute() {
  if (this->_level == 0 ) {
    //this->print();
    this->_listCommands->execute();
    this->_listCommands->clear();
    this->prompt();
  }
}

void set() {
  setenv("#", "4", 1);
  setenv("0", "./test_script_1.sh", 1);
  setenv("1", "a", 1);
  setenv("2", "b", 1);
  setenv("3", "c", 1);
  setenv("4", "d" ,1);
  setenv("*", "a b c d", 1);
}

extern "C" void disp(int sig) {
  fprintf(stderr, "\nsig:%d     Ouch!\n", sig);
  Shell::TheShell->prompt();
}
//signal child handler
extern "C" void zomb(int sig) {
  int ret;
  int status;
  while((ret = waitpid(-1, &status, WNOHANG)) != -1  ) {
    if (isatty(0)) {
      printf("[PID] %d exited.\n", ret);
    }
  }
}

void yyset_in (FILE *  in_str );

int main(int argc, char **argv) {
  Shell::TheShell = new Shell();
  path = argv[0];
  set();
  struct sigaction sa;
  sa.sa_handler=disp;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  int error = sigaction(SIGINT, &sa, NULL);

  if (error) {
    perror("sigaction");
    exit(-1);
  }
  if (Shell::TheShell->_pipeCommand->_background == false) {
    
    struct sigaction sz;
    sz.sa_handler=zomb;
    sigemptyset(&sz.sa_mask);
    sz.sa_flags = SA_RESTART;
    int err = sigaction(SIGCHLD, &sz, NULL);
    if (err) {
      perror("sigaction");
      exit(2);
    }
  }

  char * input_file = NULL;
  if ( argc > 1 ) {
    input_file = argv[1];
    FILE * f = fopen(input_file, "r");
    if (f==NULL) {
	fprintf(stderr, "Cannot open file %s\n", input_file);
        perror("fopen");
        exit(1);
    }
    yyset_in(f);
  }



  

  if (input_file != NULL) {
    // No prompt if running a script
    Shell::TheShell->_enablePrompt = false;
  }
  else {
    Shell::TheShell->prompt();
  }
  yyparse();
  Shell::TheShell->clear();
  yylex_destroy();
  delete Shell::TheShell;
}


