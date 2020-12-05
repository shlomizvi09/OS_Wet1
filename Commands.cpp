#include "Commands.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY() \
    cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
    cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(buff, arg) \
    execvp((buff), (arg));

string _ltrim(const std::string& s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char*)malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

// SmallShell //
SmallShell::SmallShell() : prompt_name("smash"), old_pwd("") {
    this->job_list = new JobsList();
}

SmallShell::~SmallShell() {
    // TODO: add your implementation
}

Command* SmallShell::CreateCommand(const char* cmd_line) {
    char** args = new char*;
    int num_of_args = _parseCommandLine(cmd_line, args);
    cout<<"the command is: "<< args[0]<<endl;
    cout<<"num of args: "<< num_of_args<<endl;
    if (strcmp(args[0], "chprompt") == 0) {
        if (num_of_args <= 1) {
            this->changePromptName("smash> ");
        } else {
            this->changePromptName(args[1]);
        }
        return nullptr;
    } else if (strcmp(args[0], "pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    } else if (strcmp(args[0], "cd") == 0) {
        if (num_of_args > 2) {
            std::cout << "smash error: cd: too many arguments" << endl;
            return nullptr;
        } else if (num_of_args == 1) {
            return nullptr;
        } else {
            return new ChangeDirCommand(cmd_line, args[1]);
        }
    } else if (strcmp(args[0], "showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    } else if (strcmp(args[0], "ls") == 0) {
        return new LsCommand(cmd_line);
    } else if (strcmp(args[0], "jobs") == 0) {
        return new JobsCommand(cmd_line);
    } else if (strcmp(args[0], "fg") == 0) {
        if (this->job_list->jobs.empty()) {
            cout << "smash error: fg: jobs list is empty" << endl;
            return nullptr;
        }
        else if (num_of_args == 1) {
            int i;
            this->job_list->getLastJob(&i);
            return new ForegroundCommand(cmd_line, i);
        }
        else if (num_of_args > 2) {
            cout << "smash error: bg: invalid arguments" << endl;
            return nullptr;
        } else {
            int tmp_job_id = stoi(string(args[1]));
            if (tmp_job_id < 1) {
                cout << "smash error: bg: invalid arguments" << endl;
                return nullptr;
            } else {
                return new ForegroundCommand(cmd_line, tmp_job_id);
            }
        }
    } else {
        cout << "got ext command" << endl;
        return new ExternalCommand(cmd_line);
    }
    delete args;

    return nullptr;
}

void SmallShell::executeCommand(const char* cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    if (cmd != nullptr) {
        cmd->execute();
        delete cmd;
    }

    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

string SmallShell::getOldPwd() {
    return old_pwd;
}

void SmallShell::changeOldPwd(string path) {
    this->old_pwd = path;
}

void SmallShell::changePromptName(string new_name) {
    this->prompt_name = new_name;
}

string SmallShell::getPromptName() {
    return this->prompt_name;
}

JobsList* SmallShell::getJoblist() {
    return this->job_list;
}
// GetCurrDirCommand //
void GetCurrDirCommand::execute() {
    char buff[COMMAND_ARGS_MAX_LENGTH];
    if (getcwd(buff, sizeof(buff)) != nullptr) {
        std::cout << buff << endl;
    }
}

// ChangeDirCommand //
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char* new_dir) : BuiltInCommand(cmd_line) {
    strcpy(next_dir, new_dir);
}

void ChangeDirCommand::execute() {
    SmallShell& ss = SmallShell::getInstance();
    char buff[COMMAND_ARGS_MAX_LENGTH];
    if (getcwd(buff, sizeof(buff)) == nullptr) {
        perror("smash error: getcwd failed");
        return;
    } else if (strcmp(this->next_dir, "-") == 0) {
        if (ss.getOldPwd().empty()) {
            cout << "smash error: cd: OLDPWD not set" << endl;
            return;
        } else {
            chdir(ss.getOldPwd().c_str());
            ss.changeOldPwd(string(buff));
            return;
        }
    } else {
        if (chdir(this->next_dir) == -1) {
            perror("smash error: chdir failed");
            return;
        } else {
            ss.changeOldPwd(string(buff));
            return;
        }
    }
}

// Command //

Command::Command() {
    this->cmd_line = " ";
}

// ShowPidCommand //
void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << endl;
}

// JobsList //
void JobsList::addJob(string cmd_line, int pid) {
    removeFinishedJobs();
    int tmp_job_id = 1;
    for (auto it = jobs.cbegin(), end = jobs.cend();  //to find first free job id
         it != end && tmp_job_id == it->first; ++it, ++tmp_job_id) {
    }
    JobEntry* new_job = new JobEntry(tmp_job_id, pid, false, cmd_line);
    jobs.insert(pair<int, JobEntry*>(tmp_job_id, new_job));
    cout << "added job: " << jobs.find(tmp_job_id)->second->job_id << " with pid: " << jobs.find(tmp_job_id)->second->pid << endl;
}

void JobsList::printJobsList() {
    this->removeFinishedJobs();
    for (auto it = jobs.cbegin(); it != jobs.cend(); ++it) {
        time_t time_diff = difftime(time(nullptr), it->second->create_time);
        cout << "[" << it->second->job_id << "] " << it->second->cmd_line << " : " << it->second->pid << " " << time_diff << " secs";
        if (it->second->is_stopped) {
            cout << " (stopped)";
        }
        cout << endl;
    }
}

void JobsList::killAllJobs() {
    for (auto it = jobs.cbegin(); it != jobs.cend(); ++it) {
        kill(it->second->pid, SIGKILL);
        delete it->second;
        jobs.erase(it);
    }
}

void JobsList::removeFinishedJobs() {
    auto it = jobs.cbegin();
    while (it != jobs.cend()) {
        if (waitpid(it->second->pid, nullptr, WNOHANG)) {
            auto job_to_remove = it;
            ++it;
            cout << "removing: [" << job_to_remove->second->cmd_line << "] with pid: " << job_to_remove->second->pid << endl;
            jobs.erase(job_to_remove);
        } else {
            ++it;
        }
    }
}

JobsList::JobEntry* JobsList::getJobById(int job_id) {
    removeFinishedJobs();
    auto tmp_job = jobs.find(job_id);
    return (tmp_job->second);
}

void JobsList::removeJobById(int jobId) {
    auto tmp_job = jobs.find(jobId);
    if (tmp_job != jobs.cend()) {
        kill(tmp_job->second->pid, SIGKILL);
        jobs.erase(tmp_job);
    }
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {
    if (!(jobs.empty())) {
        auto tmp_job = jobs.rbegin()->second;
        *lastJobId = tmp_job->job_id;
        return tmp_job;
    }
    return nullptr;
}
// LsCommand //
void LsCommand::execute() {
    struct dirent** entries;
    int res = scandir(".", &entries, NULL, alphasort);
    if (res == -1) {
        perror("smash error: scandir failed");
    }
    for (int i = 0; i < res; i++) {
        char* curr = entries[i]->d_name;
        if (strcmp(curr, ".") && strcmp(curr, "..")) {
            cout << curr << endl;
        }
        free(entries[i]);
    }
    free(entries);
}

// ExternalCommand //
void ExternalCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    bool is_bg_command = _isBackgroundComamnd(this->cmd_line.c_str());
    pid_t pid;
    int status;
    char curr_cmd_line[COMMAND_ARGS_MAX_LENGTH];
    strcpy(curr_cmd_line, this->cmd_line.c_str());
    _removeBackgroundSign(curr_cmd_line);
    char* execv_args[] = {"/bin/bash", "-c", curr_cmd_line, nullptr};
    pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
    } else if (pid == 0) {
        setpgrp();
        if (execv("/bin/bash", execv_args) == -1) {
            perror("smash error: execv failed");
            exit(0);
        }
    } else {
        if (is_bg_command) {
            sm.getJoblist()->addJob(this->cmd_line, pid);
        } else if (waitpid(pid, nullptr, WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
        }
    }
}

// JobsCommand //
void JobsCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    sm.getJoblist()->printJobsList();
}

// ForegroundCommand //

void ForegroundCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();

    JobsList::JobEntry* tmp_job = sm.getJoblist()->getJobById(this->job_id);
    if (!tmp_job) {
        cout << "smash error: fg job-id " << this->job_id << " does not exist" << endl;
    } else {
        if (kill(tmp_job->pid, SIGCONT) == -1) {
            perror("smash error: kill failed");
        } else {
            cout << tmp_job->cmd_line << " : " << tmp_job->pid << endl;
            if (waitpid(tmp_job->pid, nullptr, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            sm.getJoblist()->removeJobById(this->job_id);
        }
    }
}
