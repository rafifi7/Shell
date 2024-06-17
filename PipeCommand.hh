#ifndef pipecommand_hh
#define pipecommand_hh

#include "Command.hh"
#include "SimpleCommand.hh"

// Command Data Structure

class PipeCommand : public Command {
public:
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  bool _append;
  bool _background;

  PipeCommand();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();

  // Expands environment vars and wildcards of a SimpleCommand and
  // returns the arguments to pass to execvp.
  std::string subShell(std::string &command);
  std::string envExpansion(std::string &command);
  std::string tildeExpansion(std::string &command);
  std::vector<std::string> wildCards(std::string &command);
  void expandWildcards(std::string &prefix, std::string &suffix, std::vector <std::string> &matched_files, bool multilevel);
  bool needsWildcardExpansion(std::string &command);
  std::string regConvert(std::string s);

  char ** expandEnvVarsAndWildcards(SimpleCommand * simpleCommandNumber);

};

#endif
