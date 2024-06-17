
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>

#include "Command.hh"
#include "SimpleCommand.hh"
#include "IfCommand.hh"

IfCommand::IfCommand() {
    _condition = NULL;
    _listCommands =  NULL;
}


// Run condition with command "test" and return the exit value.
int
IfCommand::runTest(SimpleCommand * condition) {
    std::vector<char*> args;
    char *test = (char *)"test";
    args.push_back(test);
    for (auto &arg : condition->_arguments) {
        args.push_back(const_cast<char*>(arg->c_str()));
    }
    args.push_back(nullptr);
    pid_t ret = fork();
    if (ret == -1) {
        perror("bash fork");
        return -1;
    } else if (ret == 0) {
        execvp(args[0], args.data());
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(ret, &status, 0);
        return WEXITSTATUS(status);
    }

    args.clear();
    condition->print();
    return 0;
}

void 
IfCommand::insertCondition( SimpleCommand * condition ) {
    _condition = condition;
}

void 
IfCommand::insertListCommands( ListCommands * listCommands) {
    _listCommands = listCommands;
}

void 
IfCommand::clear() {
}

void 
IfCommand::print() {
    printf("IF [ \n"); 
    this->_condition->print();
    printf("   ]; then\n");
    this->_listCommands->print();
}
  
void 
IfCommand::execute() {
    // Run command if test is 0
    if (runTest(this->_condition) == 0) {
	_listCommands->execute();
    }
}

