#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <string.h>

#include <string>
#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

class Command {
    // TODO: Add your data members
   protected:
    std::string cmd_line;

   public:
    Command(const char* cmd_line);
    Command();
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
   public:
    BuiltInCommand(const char* cmd_line): Command(cmd_line){}
    BuiltInCommand();
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
   public:
    ExternalCommand(const char* cmd_line);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
   public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
   public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {  // Arik
                                                  // TODO: Add your data members public:
   private:
    char next_dir[COMMAND_ARGS_MAX_LENGTH];

   public:
    ChangeDirCommand(const char* cmd_line, char* new_dir);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {  //Arik -- DONE
   public:
    GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {  //Shlomi
   public:
    ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
    // TODO: Add your data members public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};
/*                           Arkadi said we dont need this shit
class CommandsHistory {
 protected:
  class CommandHistoryEntry {
	  // TODO: Add your data members
  };
 // TODO: Add your data members
 public:
  CommandsHistory();
  ~CommandsHistory() {}
  void addRecord(const char* cmd_line);
  void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  HistoryCommand(const char* cmd_line, CommandsHistory* history);
  virtual ~HistoryCommand() {}
  void execute() override;
};
*/

class JobsList {
   public:
    class JobEntry {
        // TODO: Add your data members
    };
    // TODO: Add your data members
   public:
    JobsList();
    ~JobsList();
    void addJob(Command* cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry* getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry* getLastJob(int* lastJobId);
    JobEntry* getLastStoppedJob(int* jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

// TODO: add more classes if needed
// maybe ls, timeout ?
/*
class ChangeChpromptCommand : public BuiltInCommand {
   public:
    ChangeChpromptCommand(const char* cmd_line);
    virtual ~ChangeChpromptCommand() {}
    void execute() override;
};
*/

class LsCommand : public BuiltInCommand {  //Shlomi
   public:
    LsCommand(const char* cmd_line);
    virtual ~LsCommand() {}
    void execute() override;
};

class SmallShell {
   private:
    // TODO: Add your data members
    std::string prompt_name;
    std::string old_pwd;
    SmallShell() : prompt_name("smash"), old_pwd("") {}
   public:
    Command* CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&) = delete;      // disable copy ctor
    void operator=(SmallShell const&) = delete;  // disable = operator
    static SmallShell& getInstance()             // make SmallShell singleton
    {
        static SmallShell instance;  // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
    void changePromptName(std::string new_name);
    std::string getPromptName();
    std::string getOldPwd();
    void changeOldPwd(std::string path);
};

#endif  //SMASH_COMMAND_H_
