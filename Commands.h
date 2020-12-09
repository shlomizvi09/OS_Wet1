#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <iterator>
#include <map>
#include <stack>
#include <string>
#include <vector>

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
    int pipe_type;  // 0->stdout , 1->stderr
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    std::string cmd;
    std::string dest;
    bool redirection_type;  //0 == >  , 1==>>
    bool is_bg_command;

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
        JobEntry(int job_id, int pid, bool is_bg_command, bool is_stopped, std::string cmd_line) : job_id(job_id), pid(pid), is_bg_command(is_bg_command), is_stopped(is_stopped), create_time(time(nullptr)), cmd_line(cmd_line) {}
        ~JobEntry() {}
    };

   public:
    std::map<int, JobEntry*> jobs;
    JobsList() : jobs(*(new std::map<int, JobEntry*>())) {}
    ~JobsList() {}
    void addJob(std::string cmd_line, int pid, bool is_stopped);
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
    ForegroundCommand(const char* cmd_line, int job_id) : BuiltInCommand(cmd_line), job_id(job_id) {}
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

class Alarm {
   public:
    Alarm(time_t creation_time, int duration);
    ~Alarm();
    time_t creation_time;
    int duration;
    int when_to_fire;
    bool operator==(Alarm& alarm) {
        if (this->creation_time == alarm.creation_time && this->duration == alarm.duration)
            return true;
        return false;
    }
    bool operator<(Alarm& alarm) {
        return this->when_to_fire < alarm.when_to_fire;
    }
    bool operator!=(Alarm& alarm) {
        if (*this == alarm)
            return false;
        else
            return true;
    }
    void operator=(Alarm* alarm) {
        this->duration = alarm->duration;
        this->creation_time = alarm->creation_time;
    }
};

class ListOfAlarms {  // we need to hanle multiple commands with a timeout, in a row
   public:
    std::vector<Alarm*>* list_of_alarms;
    std::string cmd_line;
    ListOfAlarms() {
        list_of_alarms = new std::vector<Alarm*>();
    };
    ~ListOfAlarms() {
        this->list_of_alarms->clear();
        delete this->list_of_alarms;
    };
    void fireAlarm();
    void addAlarm(time_t time_created, int duration);
};

class TimeoutCommand : public Command {
   public:
    TimeoutCommand(const char* cmd_line) : Command(cmd_line){};
    virtual ~TimeoutCommand() {}
    void execute() override;
};

class TimedoutList {
   public:
    class TimedoutEntry {
       public:
        time_t timestamp;
        int duration;
        std::string command;
        pid_t process_id;
        bool operator==(TimedoutEntry& timed) {
            if (this->timestamp = timed.timestamp && this->duration == timed.duration && this->command == timed.command &&
                                  this->process_id == timed.process_id)
                return true;
            return false;
        }
        bool operator!=(TimedoutEntry& timed) {
            if (*this == timed)
                return false;
            else
                return true;
        }
        void operator=(TimedoutEntry* timed) {
            this->duration = timed->duration;
            this->command = timed->command;
            this->timestamp = timed->timestamp;
            this->process_id = timed->process_id;
        }
    };
    std::map<int, TimedoutEntry*>* timed_dict;
    TimedoutList() {
        timed_dict = new std::map<int, TimedoutEntry*>();
    }
    ~TimedoutList() {
        this->timed_dict->clear();
        delete this->timed_dict;
    }
    void addTimedout(std::string cmd, pid_t processId, time_t timestap, int duration) {
        TimedoutEntry* new_entry = new TimedoutEntry();
        new_entry->command = cmd;
        new_entry->timestamp = time(nullptr);
        new_entry->process_id = processId;
        new_entry->duration = duration;
        std::pair<int, TimedoutEntry*> _pair = std::make_pair(processId, new_entry);
        this->timed_dict->insert(_pair);
    }
    void removeTimedout(pid_t processId) {
        this->timed_dict->erase(processId);
    }
};

class CpCommand : public BuiltInCommand {
    bool are_the_same;

   public:
    std::string source;
    std::string destination;
    CpCommand(const char* cmd_line);
    virtual ~CpCommand(){};
    void execute() override;
};

class SmallShell {
   public:
    // TODO: Add your data members
    std::string prompt_name;
    std::string old_pwd;
    JobsList* job_list;
    std::string curr_fg_command;
    pid_t curr_fg_pid;
    ListOfAlarms* list_of_alarms;
    TimedoutList* timed_out_list;

   public:
    bool forked;
    bool timeout;
    int timeout_duration;
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
    void setFgPid(int pid);
    int getFgPid();
    void setFgCommand(std::string cmd_line);
    std::string getFgCommand();
};

// Auxiliary Functions //

int c_to_int(const char* num);
bool isBuiltIn(std::vector<std::string> args);
bool compareAlarms(Alarm* alarm_1, Alarm* alarm_2);
void CheckTimeout(std::vector<std::string> args);

#endif  //SMASH_COMMAND_H_
