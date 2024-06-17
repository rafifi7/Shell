#ifndef whilecommand_hh
#define whilecommand_hh

#include "Command.hh"
#include "SimpleCommand.hh"
#include "ListCommands.hh"

// While Command Data Structure

class WhileCommand : public Command {
public:
  SimpleCommand * _condition;
  ListCommands * _listCommands; 

  WhileCommand();
  void insertCondition( SimpleCommand * condition );
  void insertListCommands( ListCommands * listCommands);
  int runTest(SimpleCommand * condition);

  std::string envExpansion(std::string& command);
  std::string subShell(std::string& command);

  void clear();
  void print();
  void execute();

};

#endif