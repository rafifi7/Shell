
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <iostream>
#include <regex>
#include <limits.h>

#include "Command.hh"
#include "SimpleCommand.hh"
#include "WhileCommand.hh"

extern char * path;
int last_code = 0;
pid_t last_pid = 0;

WhileCommand::WhileCommand() {
    _condition = NULL;
    _listCommands =  NULL;
}

std::string WhileCommand::envExpansion(std::string& command) {
  //if command is of type env var then expands and return
  std::string argo = command.substr(2); //get rid of ${
  argo.resize(argo.size() - 1); //get rid of }

  if (argo == "$") {
    argo = std::to_string(getpid());
  } else if (argo == "?") {
    argo = std::to_string(last_code);
  } else if (argo == "!") {
    argo = std::to_string(last_pid);
  } else if (argo == "_") {
    argo = getenv("_lastarg");
  } else if (argo == "SHELL") {
    char realPath[PATH_MAX];
    if (realpath(path, realPath) != NULL) {
      argo = std::string(realPath);
    } else {
      argo = "error";
    }
  } else {
    argo = getenv(argo.c_str());
  }
  return argo;
}


std::string WhileCommand::subShell(std::string& command) {
  //if command is of type subshell and meets regex requirements then create subshell
  std::string argo;

  if (command.find("$(") != std::string::npos) {
    argo = command.substr(2); //get rid of $(
    argo.resize(argo.size() - 1); //get rid of )
  } else {
    argo = command.substr(1); //get rid of first backtick
    argo.resize(argo.size() - 1); //get rid of second backtick
  }

  int tempin = dup(0);
  int tempout = dup(1);

  int pin[2];
  pipe(pin);
  
  int pout[2];
  pipe(pout);

  write(pin[1], argo.c_str(), argo.length());
  write(pin[1], "\n", 1);
  close(pin[1]);

  char * buffer = (char *) malloc(4096);
  strncpy(buffer, (char *)"", 10);

  int ret = fork();
  if (ret == 0) {
    close(pin[1]);
    close(pout[0]);

    dup2(pin[0], 0);
    dup2(pout[1], 1);

    char *args[2] = {const_cast<char*>("/proc/self/exe"), NULL};

    execvp(args[0], args);
    //execvp closes pin[1] and pout[0]
    _exit(1);
  } else if (ret < 0) {
    perror("fork");
    exit(1);
  } else {
    //parent
    waitpid(ret, NULL, 0);

    close(pin[1]);
    close(pout[1]);
    close(pin[0]);

    char c; 
    int i = 0;

    while(read(pout[0], &c, 1)) {
      if (c == '\n' || c == ' ') {
        buffer[i++] = ' ';
      } else {
        buffer[i++] = c;
      }
    }
    buffer[i] = '\0';
    
    dup2(tempin, 0);
    dup2(tempout, 1);

    close(tempin);
    close(tempout);

    
  }
  waitpid(ret, NULL, 0);
  
  std::string output(buffer);
  free(buffer);
  return output;
}

// Run condition with command "test" and return the exit value.
int WhileCommand::runTest(SimpleCommand * condition) {
    // std::vector<std::string> arg_list;
    // std::string test = "test";
    // arg_list.push_back(test);

    // std::regex envVar(R"(\$\{[^}]+\})");
    // std::regex sub(R"(\$\(([^)]+)\))");
    // std::regex sub2(R"('([^`]*)`)");


    // if (strcmp(condition->_arguments[0]->c_str(), "setenv") == 0) {
    //     if (setenv(condition->_arguments[1]->c_str(), condition->_arguments[2]->c_str(), 1) < 0) {
    //         perror("setenv");
    //     }
    //     return 0;
    //     //exit
    // }
    // //do environment var expansion on each arg in condition
    // //do subshell on each arg in condition
    // for (unsigned long j = 0; j < condition->_arguments.size(); j++) {
    //     std::string argo = condition->_arguments[j]->c_str();

    //     if (std::regex_search(argo, envVar)) {
    //         std::string com = argo;
    //         std::string full_arg = "";
    //         while (com != "") {
    //             size_t dollar = com.find('$');
    //             size_t closer = com.find('}');
    //             if (dollar == std::string::npos || closer == std::string::npos) {
    //                 full_arg += com;
    //                 com = "";
    //             } else {
    //                 full_arg += com.substr(0, dollar);
    //                 com = com.substr(dollar, com.length() - dollar);
    //                 closer = com.find('}');            
    //                 std::string temp = com.substr(0, closer + 1);
    //                 full_arg += envExpansion(temp);
    //                 com = com.substr(closer + 1, com.length() - closer);
    //             }
    //         }
    //         argo = full_arg;
    //     }
    //     delete condition->_arguments[j];
    //     condition->_arguments[j] = new std::string(argo);
    // }
    // for (unsigned long j = 0; j < condition->_arguments.size(); j++) {
    //     //do subshell
    //     std::string argo = condition->_arguments[j]->c_str();
    //     if (std::regex_search(argo, sub) || std::regex_search(argo, sub2)) {
    //         std::string output = subShell(argo);
    //         std::istringstream iss(output);
    //         std::string token;
    //         while (iss >> token) {
    //             arg_list.push_back(token); 
    //             //put each arg of the subshell output into the freelist
    //         }
    //     } else {
    //         //if its not subshell so its just a single arg
    //         arg_list.push_back(argo);
    //     }
    // }
    

    // //now create std::vector <char *> args and add copy everything back in
    // std::vector <char *> args;
    // for (auto &arg : arg_list) {
    //     args.push_back(const_cast<char*>(arg.c_str()));
    // }

    // args.push_back(nullptr);
    // pid_t ret = fork();
    // if (ret == -1) {
    //     perror("bash fork");
    //     return -1;
    // } else if (ret == 0) {
    //     execvp(args[0], args.data());
    //     perror("execvp");
    //     exit(EXIT_FAILURE);
    // } else {
    //     int status;
    //     waitpid(ret, &status, 0);
    //     if (WIFEXITED(status)) {
    //         last_code = WEXITSTATUS(status);
    //     } else {
    //         last_pid = ret;
    //     }
    // }
    // args.clear();
    condition->print();
    return 0;
}

void 
WhileCommand::insertCondition( SimpleCommand * condition ) {
    _condition = condition;
}

void 
WhileCommand::insertListCommands( ListCommands * listCommands) {
    _listCommands = listCommands;
}

void 
WhileCommand::clear() {
}

void 
WhileCommand::print() {
    printf("while [ \n"); 
    this->_condition->print();
    printf("   ]; do\n");
    this->_listCommands->print();
}
  
void 
WhileCommand::execute() {
    // Run command if test is 0
    if (runTest(this->_condition) == 0) {
	    _listCommands->execute();
    }
}

