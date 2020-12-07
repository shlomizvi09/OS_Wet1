#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <string.h>
#include <time.h>

#include <iterator>
#include <map>
#include <string>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)
#define MAX_SIMULTANEOUS_PROSSESES (101)

class JobsList;

class Command {
    // TODO: Add your data members
   protected:
    std::string cmd_line;

   public:
    Command(const char* cmd_line) : cmd_line(cmd_line) {}
    Command();
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
   public:
    BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}
    BuiltInCommand();
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
   public:
    ExternalCommand(const char* cmd_line) : Command(cmd_line) {}
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {

   public:
   std::string left_cmd;
   std::string right_cmd;
   int pipe_type; // 0->stdout , 1->stderr
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    std::string cmd;
    std::string dest;
    bool redirection_type; //0 == >  , 1==>>
   public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {  // Arik - DONE
   private:
    char next_dir[COMMAND_ARGS_MAX_LENGTH];

   public:
    ChangeDirCommand(const char* cmd_line, char* new_dir);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {  //Arik -- DONE
   public:
    GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {  //Shlomi -- DONE
   public:
    ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
    virtual ~ShowPidCommand() {}
    void execute() override;
};

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
       public:
        int job_id;
        pid_t pid;
        bool is_bg_command;
        bool is_stopped;
        time_t create_time;
        std::string cmd_line;
        JobEntry(int job_id, int pid, bool is_bg_command,bool is_stopped, std::string cmd_line) : job_id(job_id), pid(pid), is_bg_command(is_bg_command), is_stopped(is_stopped), create_time(time(nullptr)), cmd_line(cmd_line) {}
        ~JobEntry() {}
    };

   public:
    std::map<int, JobEntry*> jobs;
    JobsList() : jobs(*(new std::map<int, JobEntry*>())) {}
    ~JobsList() {}
    void addJob(std::string cmd_line, int pid,bool is_stopped);
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
    JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;

   public:
    KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    int job_id;
   public:
   
    ForegroundCommand(const char* cmd_line, int job_id) : BuiltInCommand(cmd_line), job_id(job_id){}
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    int job_id;
   public:
    BackgroundCommand(const char* cmd_line, int job_id) : BuiltInCommand(cmd_line), job_id(job_id) {}
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
    LsCommand(const char* cmd_line) : BuiltInCommand(cmd_line){};
    virtual ~LsCommand() {}
    void execute() override;
};

class SmallShell {
   private:
    // TODO: Add your data members
    std::string prompt_name;
    std::string old_pwd;
    JobsList* job_list;
    std::string curr_fg_command;
    pid_t curr_fg_pid;

   public:
    bool forked;
    SmallShell();
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
    JobsList* getJoblist();
    void setFgPid (int pid);
    int getFgPid();
    void setFgCommand(std::string cmd_line);
    std::string getFgCommand();
};

#endif  //SMASH_COMMAND_H_
