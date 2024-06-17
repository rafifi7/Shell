#ifndef shell_hh
#define shell_hh

#include "ListCommands.hh"
#include "PipeCommand.hh"
#include "IfCommand.hh"
#include "WhileCommand.hh"

extern char * path;

class Shell {


public:
  int _level; // Only outer level executes.
  bool _enablePrompt;
  ListCommands * _listCommands; 
  SimpleCommand *_simpleCommand;
  PipeCommand * _pipeCommand;
  IfCommand * _ifCommand;
  WhileCommand * _whileCommand;
  Command * _currentCommand;
  static Shell * TheShell;
  

  Shell();
  void set();
  void execute();
  void print();
  void clear();
  void prompt();

};

#endif
