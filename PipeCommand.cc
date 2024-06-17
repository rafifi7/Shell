/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */



#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <limits.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <regex>
#include <regex.h>
#include <unistd.h>


#include "PipeCommand.hh"
#include "Shell.hh"

int last_return_code = 0;
pid_t last_background_pid = 0;
std::string last_argument;
std::string shell_path;
extern char * path;


PipeCommand::PipeCommand() {
    // Initialize a new vector of Simple PipeCommands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _append = false;
    _background = false;
}

void PipeCommand::insertSimpleCommand( SimpleCommand * simplePipeCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simplePipeCommand);
}

void PipeCommand::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simplePipeCommand : _simpleCommands) {
        delete simplePipeCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile ) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;
    _append = false;

    _background = false;
}

void PipeCommand::print() {
    printf("\n\n");
    //printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple PipeCommands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simplePipeCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simplePipeCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

void PipeCommand::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::TheShell->prompt();
        return;
    }
    // Print contents of PipeCommand data structure
    //print();
    //save stdin/stdout
    int tempin = dup(0);
    int tempout = dup(1);
    int temperr = dup(2);
    //create pipe
    int fdin = 0;
    if (_inFile) {
      //read infile
      fdin = open(_inFile->c_str(), O_RDONLY);
    } else {
      //use default input
      fdin = dup(tempin);
    }
    int fderr = 0;
    if (_errFile) {
      if (_append) {
        //read append to error file if bool append
        fderr = open(_errFile->c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
      } else {
        //write over error file
        fderr = open(_errFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
      }
    } else {
      fderr = dup(temperr);
    }
    dup2(fderr, 2);
    close(fderr);

    int ret = 0;
    int fdout = 0;
    unsigned long size = _simpleCommands.size();
    int temp = 0;

    for (unsigned long i = 0; i < size; i++) {
      std::vector<std::string*> arguments;
      if (_simpleCommands[i]->_arguments.empty() == false) {
        arguments = _simpleCommands[i]->_arguments;
      }

      //redirect input
      dup2(fdin, 0);
      close(fdin);
      //setup output
      int fdpipe[2];
      if (i == size - 1) {
        //last simple command
        
        if (_outFile) {
          if (_append) {
           fdout = open(_outFile->c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
          } else {
            fdout = open(_outFile->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
          }
        } else {
          //use default output
          fdout=dup(tempout);
        }
      } else {
        // not last simple commmand
        // create pipe
        pipe(fdpipe);
        fdin = fdpipe[0];
        fdout=fdpipe[1];
        //dup2(temp, fdin);
      }
      char ** args = this->expandEnvVarsAndWildcards(_simpleCommands[i]);
      if (args == NULL) {
        clear();
        return;
      }

      setenv("_lastarg", arguments.back()->c_str(), 1);
      //redirect
      dup2(fdout, 1);
      close(fdout);
      //create child process

      ret = fork();

      if (ret == -1) {
        perror("fork\n");
        exit(2);
      }
    
      if (ret == 0) {
        if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv")) {
          char **p = environ;
          
          while (*p != NULL) {
            if (!strcmp(*p, "_lastarg")) {
              continue;
            } else {
              printf("%s\n", *p);
              p++;
            }
          }
          exit(0);
        }
        execvp(args[0], args);
        for (unsigned long i = 0; args[i] != nullptr; i++) {
          delete[] args[i];
        }
        delete[] args;
        perror("execvp");
        exit(1);
      }
      if (i < size - 1) {
        close(fdpipe[1]);
      }
      dup2(fdpipe[0], temp);
    }

    dup2(tempin, 0);
    dup2(tempout, 1);
    dup2(temperr, 2);
    close(tempin);
    close(tempout);
    close(temperr);
   

    if (_background == false) {
      int status;
      waitpid(ret, &status, 0);
      if (WIFEXITED(status)) {
        last_return_code = WEXITSTATUS(status);
      } else {
        last_background_pid = ret;
      }
    }

    // For every simple command fork a new process
    // Setup i/o redirection
    // and call exec
    // Clear to prepare for next command
    clear();
    // Print new prompt
    //Shell::TheShell->prompt();
}

std::string PipeCommand::subShell(std::string& command) {
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


std::string PipeCommand::envExpansion(std::string& command) {
  //if command is of type env var then expands and return
  std::string argo = command.substr(2); //get rid of ${
  argo.resize(argo.size() - 1); //get rid of }

  if (argo == "$") {
    argo = std::to_string(getpid());
  } else if (argo == "?") {
    argo = std::to_string(last_return_code);
  } else if (argo == "!") {
    argo = std::to_string(last_background_pid);
  } else if (argo == "_") {
    //not working    
    argo = getenv("_lastarg");
    //std::cout << "Hello" <<std::endl;
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

std::string PipeCommand::tildeExpansion(std::string& command) {
  while (true) {
    std::string::iterator found = std::find(command.begin(), command.end(), '~'); 
    if (found == command.end()) {
      break;
    }
    if (command.length() == 1 || *(found + 1) == '/') {
      struct passwd* pwd = getpwnam(getenv("USER"));
      if (pwd) {
        command.replace(found, found + 1, pwd->pw_dir);
      } else {
        break;
      }
    } else {
      std::string::iterator slash = std::find(found, command.end(), '/');
      std::string user(found + 1, slash);
      struct passwd* pwd = getpwnam(user.c_str());
      if (pwd) {
        command.replace(found, slash, pwd->pw_dir);
      } else {
        break;
      }
    }
  }
  return command;
}

std::vector<std::string> PipeCommand::wildCards(std::string& command) {
  std::string regex = regConvert(command);
  std::regex pattern(regex);
  std::vector<std::string> matched_files;

  DIR *dir;
  struct dirent* ent;
  if ((dir = opendir(".")) != nullptr) {
    while ((ent = readdir(dir)) != nullptr) {
      std::string file = ent->d_name;
      if (std::regex_match(file, pattern)) {
        if (file[0] == '.') {
          if (command[0] == '.') {
            matched_files.push_back(file);        
          }
        } else {
          matched_files.push_back(file);
        }
      }
    }
    closedir(dir);
  } else {
    perror("oopendir");
    return matched_files;
  }

  if (matched_files.empty()) {
    matched_files.push_back(regex);
  }

  return matched_files;
}

void PipeCommand::expandWildcards(std::string& prefix, std::string& suffix, std::vector <std::string> &matched_files, bool multilevel) {
  if (suffix == "") {
    size_t lastSlash = prefix.find_last_of('/');
    if (prefix[lastSlash + 1] != '.') {
      matched_files.push_back(prefix);
    }
    return;
  }

  if (multilevel == true) {
    size_t star = suffix.find('*');
    // find first star
    size_t firstSlash = suffix.find_last_of('/', star);
    // find slash before star
    size_t secondSlash = suffix.find_first_of('/', firstSlash + 1);
    // find slash after star
    std::string fakeSuffix; //segment the has the first regex 
    // /etc/r*/*s* -> fakeSuffix would be r*
    std::string nextSuffix; //segment the rest of the suffix 
    // /etc/r*/*s* -> nextSuffix would be *s*
    if (secondSlash != std::string::npos) {
      //if secondSlash exists
      fakeSuffix = suffix.substr(firstSlash + 1, secondSlash);
      nextSuffix = suffix.substr(secondSlash + 1);
    } else {
      //if secondSlash doesnt exist
      multilevel = false; // becomes false because there is no more levels
      fakeSuffix = suffix.substr(firstSlash + 1);
      nextSuffix = "";
    }
    DIR * dir;
    struct dirent *ent;

    //create regex of fakeSuffix -> this will be what we are matching 
    std::string regex = regConvert(fakeSuffix);
    std::regex pattern(regex);

    if ((dir = opendir(prefix.c_str())) != nullptr) {
      while ((ent = readdir(dir)) != nullptr) {
        if (regex_match(ent->d_name, pattern)) {
          std::string full_arg = prefix + "/" + ent->d_name;
          if (ent->d_name[0] == '.') {
            if (fakeSuffix[0] == '.') {
              matched_files.push_back(full_arg);
            }
          }
          expandWildcards(full_arg, nextSuffix, matched_files, multilevel);
        }
      }
    }
  }
  return;
}

std::string PipeCommand::regConvert(std::string s) {
  std::string regex = "^";
  for (char c: s) {
    switch (c) {
      case '*': 
        regex+= ".*"; 
        break;
      case '?':
        regex+= ".";
        break;
      case '.':
        regex+="\\.";
        break;
      default:
        regex+=c;
        break;
    }
  }
  regex += "$";
  return regex;
}

bool PipeCommand::needsWildcardExpansion(std::string& command) {
    return command.find('*') != std::string::npos || command.find('?') != std::string::npos;
}

// Expands environment vars and wildcards of a SimpleCommand and
// returns the arguments to pass to execvp.
char **
PipeCommand::expandEnvVarsAndWildcards(SimpleCommand * simpleCommandNumber)
{
  //print();
  //simpleCommandNumber->print();

  std::regex envVar(R"(\$\{[^}]+\})");
  std::regex sub(R"(\$\(([^)]+)\))");
  std::regex sub2(R"(`([^`]*)`)");
  std::regex tilde(R"(^~[^ |>\t\n]*)");
  
  std::vector <std::string> arg_list;
  
  if (strcmp(simpleCommandNumber->_arguments[0]->c_str(), "setenv") == 0) {
    //setenv
    if (setenv(simpleCommandNumber->_arguments[1]->c_str(), simpleCommandNumber->_arguments[2]->c_str(), 1) < 0) {
      perror("setenv");
    }
    return NULL;
  } else if (strcmp(simpleCommandNumber->_arguments[0]->c_str(), "unsetenv") == 0) {
    //unsetenv
    if (unsetenv(simpleCommandNumber->_arguments[1]->c_str()) < 0) {
      perror("unsetenv");
    }
    return NULL;
  } else if (strcmp(simpleCommandNumber->_arguments[0]->c_str(), "cd") == 0) {
    //cd
    int error;
    if (simpleCommandNumber->_arguments.size() == 1) {
      error = chdir(getenv("HOME"));
      if (error < 0) {
        std::string a = getenv("HOME");
        std::string b = "cd: can't cd to ";
        std::string c = b + a;
        perror(c.c_str());
      }
    } else {
       if (std::regex_search(simpleCommandNumber->_arguments[1]->c_str(), envVar)) {
        //expand before cd ing
        std::string com = simpleCommandNumber->_arguments[1]->c_str();
        std::string full_arg = "";
        while (com != "") {
          size_t dollar = com.find('$');
          size_t closer = com.find('}');
          if (dollar == std::string::npos || closer == std::string::npos) {
              full_arg += com;
              com = "";
          } else {
            full_arg += com.substr(0, dollar);
            com = com.substr(dollar, com.length() - dollar);
            closer = com.find('}');            
            std::string temp = com.substr(0, closer + 1);
            full_arg += envExpansion(temp);
            com = com.substr(closer + 1, com.length() - closer);
          }
        }
        delete simpleCommandNumber->_arguments[1];
        simpleCommandNumber->_arguments[1] = new std::string(full_arg);
      }
      error = chdir(simpleCommandNumber->_arguments[1]->c_str());
      if (error < 0) {
        std::string a = simpleCommandNumber->_arguments[1]->c_str();
        std::string b = "cd: can't cd to ";
        std::string c = b + a;
        perror(c.c_str());
      }
    }
    return NULL;
  } else {
      for (unsigned long j = 0; j < simpleCommandNumber->_arguments.size(); j++) {
      std::string argo = simpleCommandNumber->_arguments[j]->c_str();
      if (std::regex_search(argo, envVar)) {
        //envExpansion
        std::string com = argo;
        std::string full_arg = "";
        while (com != "") {
          size_t dollar = com.find('$');
          size_t closer = com.find('}');
          if (dollar == std::string::npos || closer == std::string::npos) {
              full_arg += com;
              com = "";
          } else {
            full_arg += com.substr(0, dollar);
            com = com.substr(dollar, com.length() - dollar);
            closer = com.find('}');            
            std::string temp = com.substr(0, closer + 1);
            full_arg += envExpansion(temp);
            com = com.substr(closer + 1, com.length() - closer);
          }
        }
        argo = full_arg;
        arg_list.push_back(argo);
      } else if (std::regex_search(argo, sub) || std::regex_search(argo, sub2)) {

        //subshell

        std::string output = subShell(argo);
        argo = subShell(argo);
        //split the single argument into a vector of strings on ' '
        //since subshell already gets rid of all the \n's
        std::istringstream iss(output);
        std::string token;
        while (iss >> token) {
          arg_list.push_back(token);
        }
      } else if (std::regex_search(argo, tilde)) {
        //tilde
        argo = tildeExpansion(argo);
        arg_list.push_back(argo);

      } else {


        //check if wildcards needed
        if (needsWildcardExpansion(argo) && argo.find("??") == std::string::npos) {
          std::vector<std::string> matched_files = wildCards(argo);
          //use own arg list since i am looping through simpleCommand's args
          if (matched_files.size() == 1 && needsWildcardExpansion(matched_files[0])) {
            //if wildCards function did not change the expression 
            //every test except for first
            bool multilevel = false;
            //for multi level regex expressions
            size_t star = argo.find('*');
            size_t slash = argo.find_last_of('/', star); 
            // finds the first slash between 0 and star position
            size_t starAfterSlash = argo.find('*', slash + 1);
            //finds the first star after slash position
            if (starAfterSlash != std::string::npos) {
              multilevel = true;
            }
            std::string pre = argo.substr(0, slash);
            std::string suf = argo.substr(slash + 1);
            matched_files.clear();
            expandWildcards(pre, suf, matched_files, multilevel);
          }
          std::sort(matched_files.begin(), matched_files.end());
          for (unsigned long k = 0; k < matched_files.size(); k++) {
            arg_list.push_back(matched_files[k]);
          }
        } else { //matched_files empty 
          arg_list.push_back(argo);
        }
      }
    }

    char ** args = new char * [arg_list.size() + 1];
    for (unsigned long i = 0; i < arg_list.size(); i++) {
      args[i] = new char[arg_list[i].size() + 1];
      std::strcpy(args[i], arg_list[i].c_str());
    }

    args[arg_list.size()] = NULL;
    arg_list.clear();
    return args;
  }
  return NULL;
} 



