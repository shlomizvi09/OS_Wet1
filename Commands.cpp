#include "Commands.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
#define MAX_BUFFER_SIZE 4096

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
    string temp_str(cmd_line);
    temp_str = _trim(temp_str);
    strcpy(cmd_line, temp_str.c_str());
}

// TODO: Add your implementation for classes in Commands.h

// SmallShell //
SmallShell::SmallShell() : prompt_name("smash"), old_pwd(""), curr_fg_command(""), curr_fg_pid(-1), forked(false) {
    this->timeout = false;
    this->timeout_duration = 0;
    this->job_list = new JobsList();
    this->list_of_alarms = new ListOfAlarms();
    this->timed_out_list = new TimedoutList();
    this->smash_pid = getpid();
}

SmallShell::~SmallShell() {
    delete this->job_list;
    delete this->list_of_alarms;
    delete timed_out_list;
}

Command* SmallShell::CreateCommand(const char* cmd_line) {
    if (!cmd_line) {
        return nullptr;
    }
    string cmd_string = cmd_line;
    cmd_string = _trim(cmd_string);
    istringstream iss(_trim(cmd_string));
    vector<string> args;
    while (iss) {
        string arg;
        iss >> arg;
        if (arg != "")
            args.push_back(arg);
    }
    if (args.empty()) {
        return nullptr;
    }
    // if (!this->timeout) {
    //    CheckTimeout(args);
    // }
    int num_of_args = args.size();
    int pipe_idx = cmd_string.find("|");
    int redir_idx = cmd_string.find(">");
    auto kill_when_quit = find(args.begin(), args.end(), string("kill"));
    if (pipe_idx != cmd_string.npos) {
        return new PipeCommand(cmd_line);
    }
    if (strcmp(args[0].c_str(), "timeout") == 0) {
        CheckTimeout(args);
        return new TimeoutCommand(cmd_line);
    }
    if (redir_idx != cmd_string.npos) {
        return new RedirectionCommand(cmd_line);
    }
    if (strcmp(args[0].c_str(), "quit") == 0) {
        if (kill_when_quit != args.end()) {
            return new QuitCommand(cmd_line, true);
        }
        return new QuitCommand(cmd_line, false);
    }
    if (strcmp(args[0].c_str(), "chprompt") == 0) {
        if (num_of_args <= 1) {
            this->changePromptName("smash");
        } else {
            this->changePromptName(args[1]);
        }
        return nullptr;
    }
    if (strcmp(args[0].c_str(), "pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    if (strcmp(args[0].c_str(), "cd") == 0) {
        if (num_of_args > 2) {
            std::cout << "smash error: cd: too many arguments" << endl;
            return nullptr;
        }
        if (num_of_args == 1) {
            return nullptr;
        }
        char arg1[COMMAND_ARGS_MAX_LENGTH];
        strcpy(arg1, args[1].c_str());
        return new ChangeDirCommand(cmd_line, arg1);
    }
    if (strcmp(args[0].c_str(), "showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    if (strcmp(args[0].c_str(), "ls") == 0) {
        return new LsCommand(cmd_line);
    }
    if (strcmp(args[0].c_str(), "jobs") == 0) {
        return new JobsCommand(cmd_line);
    }
    if (strcmp(args[0].c_str(), "fg") == 0) {
        if (this->job_list->jobs.empty()) {
            cout << "smash error: fg: jobs list is empty" << endl;
            return nullptr;
        }
        if (num_of_args == 1) {
            int i;
            this->job_list->getLastJob(&i);
            return new ForegroundCommand(cmd_line, i);
        }
        if (num_of_args > 2) {
            cout << "smash error: bg: invalid arguments" << endl;
            return nullptr;
        }
        try {
            int tmp_job_id = stoi(string(args[1]));
            if (tmp_job_id < 1) {
                cout << "smash error: fg: invalid arguments" << endl;
                return nullptr;
            }
            return new ForegroundCommand(cmd_line, tmp_job_id);
        } catch (invalid_argument& e) {
            cout << "smash error: fg: invalid arguments" << endl;
            return nullptr;
        }
    }
    if (strcmp(args[0].c_str(), "bg") == 0) {
        if (this->job_list->jobs.empty()) {
            cout << "smash error: bg: there is no stopped jobs to resume" << endl;
            return nullptr;
        }
        if (num_of_args == 1) {
            int i;
            this->job_list->getLastStoppedJob(&i);
            if (i == -1) {
                cout << "smash error: bg: there is no stopped jobs to resume" << endl;
                return nullptr;
            }
            if (i > 0) {
                return new BackgroundCommand(cmd_line, i);
            }
        }
        if (num_of_args == 2) {
            try {
                int tmp_job_id = stoi(string(args[1]));
                if (tmp_job_id < 1) {
                    cout << "smash error: bg: invalid arguments" << endl;
                    return nullptr;
                }
                return new BackgroundCommand(cmd_line, tmp_job_id);
            } catch (invalid_argument& e) {
                cout << "smash error: bg: invalid arguments" << endl;
                return nullptr;
            }

        } else {
            cout << "smash error: bg: invalid arguments" << endl;
            return nullptr;
        }
    }
    if (strcmp(args[0].c_str(), "kill") == 0) {
        return new KillCommand(cmd_line, this->job_list);
    }
    if (strcmp(args[0].c_str(), "cp") == 0) {
        return new CpCommand(cmd_line);
    }

    return new ExternalCommand(cmd_line);
}

void SmallShell::executeCommand(const char* cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    if (cmd != nullptr) {
        cmd->execute();
    }
    delete cmd;
    this->forked = false;

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

void SmallShell::setFgPid(int pid) {
    this->curr_fg_pid = pid;
}

int SmallShell::getFgPid() {
    return this->curr_fg_pid;
}

void SmallShell::setFgCommand(string cmd_line) {
    this->curr_fg_command = cmd_line;
}

string SmallShell::getFgCommand() {
    return this->curr_fg_command;
}

pid_t SmallShell::getSmashPid() {
    return this->smash_pid;
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
    SmallShell& sm = SmallShell::getInstance();
    char buff[COMMAND_ARGS_MAX_LENGTH];
    if (getcwd(buff, sizeof(buff)) == nullptr) {
        perror("smash error: getcwd failed");
        return;
    } else if (strcmp(this->next_dir, "-") == 0) {
        if (sm.getOldPwd().empty()) {
            cout << "smash error: cd: OLDPWD not set" << endl;
            return;
        } else {
            chdir(sm.getOldPwd().c_str());
            sm.changeOldPwd(string(buff));
            return;
        }
    } else {
        if (chdir(this->next_dir) == -1) {
            perror("smash error: chdir failed");
            return;
        } else {
            sm.changeOldPwd(string(buff));
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
    SmallShell& sm = SmallShell::getInstance();
    std::cout << "smash pid is " << sm.getSmashPid() << endl;
}

// JobsList //
void JobsList::addJob(string cmd_line, int pid, bool is_stopped) {
    removeFinishedJobs();
    int tmp_job_id = 1;
    for (auto it = jobs.cbegin(), end = jobs.cend();  //to find first free job id
         it != end && tmp_job_id == it->first; ++it, ++tmp_job_id) {
    }
    JobEntry* new_job = new JobEntry(tmp_job_id, pid, false, is_stopped, cmd_line);
    jobs.insert(pair<int, JobEntry*>(tmp_job_id, new_job));
    //cout << "added job: " << jobs.find(tmp_job_id)->second->job_id << " with pid: " << jobs.find(tmp_job_id)->second->pid << endl;
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
    this->removeFinishedJobs();
    cout << "smash: sending SIGKILL signal to " << jobs.size() << " jobs:" << endl;  
    auto it = jobs.cbegin();
    while (it != jobs.cend()) {
        auto job_to_kill = it;
        ++it;
        cout << job_to_kill->second->pid << ": " << job_to_kill->second->cmd_line;
        if (job_to_kill->second->is_stopped) {
            cout << " (stopped)";
        }
        cout << endl;
        kill(job_to_kill->second->pid, SIGKILL);
        delete job_to_kill->second;
        jobs.erase(job_to_kill);
    }
}

void JobsList::removeFinishedJobs() {
    auto it = jobs.cbegin();
    while (it != jobs.cend()) {
        if (waitpid(it->second->pid, nullptr, WNOHANG)) {
            auto job_to_remove = it;
            ++it;
            // cout << "removing: [" << job_to_remove->second->cmd_line << "] with pid: " << job_to_remove->second->pid << endl;
            delete job_to_remove->second;
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

JobsList::JobEntry* JobsList::getLastStoppedJob(int* jobId) {
    if (jobs.empty()) {
        *jobId = -1;
        return nullptr;
    }
    for (auto it = jobs.rbegin(); it != jobs.rend(); ++it) {
        if (it->second->is_stopped) {
            *jobId = it->second->job_id;
            return it->second;
        }
    }
    *jobId = -1;
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
    string original_cmd_string = this->cmd_line;
    original_cmd_string = _trim(original_cmd_string);
    istringstream iss(original_cmd_string);
    vector<string> args;
    while (iss) {
        string arg;
        iss >> arg;
        if (arg != "")
            args.push_back(arg);
    }
    int num_of_args = args.size();
    bool is_bg_command = _isBackgroundComamnd(this->cmd_line.c_str());
    pid_t pid;
    int status;
    char curr_cmd_line[COMMAND_ARGS_MAX_LENGTH];
    strcpy(curr_cmd_line, this->cmd_line.c_str());
    _removeBackgroundSign(curr_cmd_line);
    char* execv_args[] = {"/bin/bash", "-c", curr_cmd_line, nullptr};
    int duration = sm.timeout_duration;
    pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
    } else if (pid == 0) {
        if (!sm.forked) {
            setpgrp();
        }
        if (execv("/bin/bash", execv_args) == -1) {
            perror("smash error: execv failed");
            exit(0);
        }
    } else {
        if (sm.timeout) {
            sm.list_of_alarms->addAlarm(time(nullptr), duration);
            if (!sm.list_of_alarms->list_of_alarms->empty()) {
                sm.timed_out_list->addTimedout(original_cmd_string, pid, time(nullptr), duration);
            }
        }
        if (is_bg_command) {
            sm.getJoblist()->addJob(original_cmd_string, pid, false);
        } else {
            sm.setFgCommand(string(cmd_line));
            sm.setFgPid(pid);
            if (waitpid(pid, nullptr, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            sm.setFgPid(-1);
            sm.setFgCommand("");
        }
    }
}

// JobsCommand //

void JobsCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    sm.getJoblist()->printJobsList();
}

// KillCommand //

void KillCommand::execute() {
    int job_id = 0;
    int signal_num = 0;
    string cmd_string = cmd_line;
    istringstream iss(_trim(cmd_string));
    vector<string> args;
    while (iss) {
        string arg;
        iss >> arg;
        if (arg != "")
            args.push_back(arg);
    }
    int num_of_args = args.size();
    if (num_of_args != 3) {
        cout << "smash error: kill: invalid arguments" << endl;
        return;
    }

    signal_num = c_to_int(args[1].c_str());
    job_id = c_to_int(args[2].c_str());
    if(job_id<=0 || signal_num >=0){
        cout << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if (args[1][0] != '-') {
        cout << "smash error: kill: invalid arguments" << endl;
        return;
    }

    signal_num = abs(signal_num);
    JobsList::JobEntry* job_entry = this->jobs->getJobById(job_id);
    if (job_entry == nullptr) {
        cout << "smash error: kill: job-id " << job_id << " does not exist" << endl;
        return;
    }

    pid_t job_pid = job_entry->pid;
    int res = killpg(job_pid, signal_num);
    if (res == -1) {
        perror("smash error: kill failed");
        return;
    }
    cout << "signal number " << signal_num << " was sent to pid " << job_pid << endl;
    return;
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
            sm.setFgCommand(tmp_job->cmd_line);
            sm.setFgPid(tmp_job->pid);
            if (waitpid(tmp_job->pid, nullptr, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
            sm.getJoblist()->removeJobById(this->job_id);
            sm.setFgCommand("");
            sm.setFgPid(-1);
        }
    }
}

// BackgroundCommand //
void BackgroundCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    JobsList::JobEntry* tmp_job = sm.getJoblist()->getJobById(this->job_id);
    if (!tmp_job) {
        cout << "smash error: bg job-id " << this->job_id << " does not exist" << endl;
    } else {
        if (!tmp_job->is_stopped) {
            cout << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
            return;
        }
        if (kill(tmp_job->pid, SIGCONT) == -1) {
            perror("smash error: kill failed");
        } else {
            tmp_job->is_stopped = false;
            cout << tmp_job->cmd_line << " : " << tmp_job->pid << endl;
        }
    }
}

// PipeCommand //
PipeCommand::PipeCommand(const char* cmd_line) : Command(cmd_line) {
    string full_cmd = string(cmd_line);
    auto pipe_sign_idx = full_cmd.find("|");
    this->left_cmd = full_cmd.substr(0, pipe_sign_idx);
    this->left_cmd = _trim(this->left_cmd);
    if (full_cmd.substr(pipe_sign_idx + 1, 1) == "&") {
        this->right_cmd = full_cmd.substr(pipe_sign_idx + 2);
        this->right_cmd = _trim(this->right_cmd);
        this->pipe_type = 1;
    } else {
        this->right_cmd = full_cmd.substr(pipe_sign_idx + 1);
        this->right_cmd = _trim(this->right_cmd);
        this->pipe_type = 0;
    }
    //cout << "cmd1: " << left_cmd << " cmd2: " << right_cmd << ", type: " << pipe_type << endl;
}

void PipeCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    bool is_bg_command = _isBackgroundComamnd(this->right_cmd.c_str());
    Command* left_cmd = sm.CreateCommand(this->left_cmd.c_str());
    Command* right_cmd = sm.CreateCommand(this->right_cmd.c_str());
    int left_space_idx = this->left_cmd.find(" ");
    string left_cmd_string = this->left_cmd.substr(0,left_space_idx);
    int right_space_idx = this->right_cmd.find(" ");
    string right_cmd_string = this->right_cmd.substr(0,right_space_idx);
    if (left_cmd_string == "quit"){
        left_cmd->execute();
        return;
    }
    if (right_cmd_string == "quit"){
        right_cmd->execute();
        return;
    }

    sm.forked = true;
    int pipe_pid = fork();
    if (pipe_pid == -1) {
        perror("smash error: fork failed");
        exit(0);
    }
    if (pipe_pid == 0) {
        setpgrp();
        int fd[2];
        if (pipe(fd) == -1) {
            perror("smash error: pipe failed");
            exit(1);
        }
        int left_cmd_pid = fork();
        if (left_cmd_pid == -1) {
            perror("smash error: fork failed");
            exit(0);
        }
        if (left_cmd_pid == 0) {
            if (this->pipe_type == 0) {
                if (dup2(fd[1], STDOUT_FILENO) == -1) {
                    perror("smash error: dup2 failed");
                    exit(0);
                }
                if (close(fd[0]) == -1) {
                    perror("smash error: closer failed");
                    exit(0);
                }
                if (close(fd[1]) == -1) {
                    perror("smash error: close failed");
                    exit(0);
                }
            } else {
                if (dup2(fd[1], STDERR_FILENO) == -1) {
                    perror("smash error: dup2 failed");
                    exit(0);
                }
                if (close(fd[0]) == -1) {
                    perror("smash error: close failed");
                    exit(0);
                }
                if (close(fd[1]) == -1) {
                    perror("smash error: close failed");
                    exit(0);
                }
            }
            left_cmd->execute();
            exit(0);
        }

        int right_cmd_pid = fork();
        if (right_cmd_pid == -1) {
            perror("smash error: fork failed");
            exit(0);
        }
        if (right_cmd_pid == 0) {
            if (dup2(fd[0], STDIN_FILENO) == -1) {
                perror("smash error: dup2 failed");
                exit(0);
            }
            if (close(fd[0]) == -1) {
                perror("smash error: close failed");
                exit(0);
            }
            if (close(fd[1]) == -1) {
                perror("smash error: close failed");
                exit(0);
            }
            right_cmd->execute();
            exit(0);
        }

        if (close(fd[0]) == -1) {
            perror("smash error: close failed");
            exit(0);
        }
        if (close(fd[1]) == -1) {
            perror("smash error: close failed");
            exit(0);
        }

        if (waitpid(left_cmd_pid, nullptr, WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
            exit(0);
        }
        if (waitpid(right_cmd_pid, nullptr, WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
            exit(0);
        }

        exit(0);
    }
    if (!is_bg_command) {
        if (waitpid(pipe_pid, nullptr, WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
            exit(0);
        }
    } else {
        //picout<< "adding pipe job: "<<this->cmd_line<<" with pid: "<<pipe_pid<<endl;
        sm.getJoblist()->addJob(this->cmd_line, pipe_pid, false);
    }
}

// RedirectionCommand //
RedirectionCommand::RedirectionCommand(const char* cmd_line) {
    string full_cmd = string(cmd_line);
    full_cmd = _trim(full_cmd);
    auto redir_sign_idx = full_cmd.find(">");
    this->cmd = _trim(full_cmd.substr(0, redir_sign_idx));
    if (full_cmd.substr(redir_sign_idx + 1, 1) == ">") {
        this->dest = _trim(full_cmd.substr(redir_sign_idx + 2));
        this->redirection_type = 1;
    } else {
        this->dest = _trim(full_cmd.substr(redir_sign_idx + 1));
        this->redirection_type = 0;
    }
    is_bg_command = (_isBackgroundComamnd(full_cmd.c_str()) || _isBackgroundComamnd(cmd.c_str()));
    if (dest.substr(dest.size() - 1, 1) == "&") {
        dest.erase(dest.size() - 1);
    }
    //cout << "cmd1: " << cmd << " cmd2: " << dest << ", type: " << this->redirection_type << endl;
}

void RedirectionCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    int fd, old_stdout;

    if (!this->redirection_type) {
        fd = open(dest.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
    } else {
        fd = open(dest.c_str(), O_APPEND | O_WRONLY | O_CREAT, 0666);
    }
    if (fd < 0) {
        perror("smash error: open failed");
        return;
    }
    old_stdout = dup(STDOUT_FILENO);
    if (old_stdout < 0) {
        perror("smash error: dup failed");
        return;
    }
    if (dup2(fd, STDOUT_FILENO) < 0) {
        perror("smash error: dup2 failed");
        return;
    }
    if (close(fd) < 0) {
        perror("smash error: close failed");
        return;
    }
    sm.executeCommand(this->cmd.c_str());
    if (close(STDOUT_FILENO) < 0) {
        perror("smash error: close failed");
        return;
    }
    if (dup2(old_stdout, STDOUT_FILENO) < 0) {
        perror("smash error: dup2 failed");
        return;
    }
    if (close(old_stdout) < 0) {
        perror("smash error: close failed");
        return;
    }
}

// TimeoutCommand //

Alarm::Alarm(time_t creation_time, int duration) {
    this->creation_time = creation_time;
    this->duration = duration;
    this->when_to_fire = time(nullptr) + duration;
}

void ListOfAlarms::addAlarm(time_t time_created, int duration) {
    //cout << duration << endl;
    Alarm* new_alarm = new Alarm(time_created, duration);
    this->list_of_alarms->push_back(new_alarm);
    sort(this->list_of_alarms->begin(), this->list_of_alarms->end(), compareAlarms);
    fireAlarm();
}

void ListOfAlarms::fireAlarm() {
    Alarm* curr_alarm = this->list_of_alarms->at(0);
    int duration = curr_alarm->when_to_fire - time(nullptr);
    int res = alarm(duration);
    if (res == -1) {
        perror("smash error: alarm failed");
        return;
    }
}

void TimeoutCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    string cmd_string = this->cmd_line;
    cmd_string = _trim(cmd_string);
    istringstream iss(cmd_string);
    vector<string> args;
    while (iss) {
        string arg;
        iss >> arg;
        if (arg != "")
            args.push_back(arg);
    }
    int num_of_args = args.size();
    if (num_of_args <= 2) {
        cout << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    sm.timeout_duration = c_to_int(args[1].c_str());
    if (sm.timeout_duration < 0) {
        cout << "smash error: timeout: invalid arguments" << endl;
        return;
    }

    string temp_str = "";
    for (int i = 2; i < num_of_args; i++) {
        temp_str = temp_str + args[i] + " ";
    }
    temp_str = _trim(temp_str);
    const char* new_cmd_line = temp_str.c_str();
    Command* cmd = sm.CreateCommand(new_cmd_line);
    if (isBuiltIn(args)) {
        sm.list_of_alarms->addAlarm(time(nullptr), c_to_int(args[1].c_str()));
    }
    cmd->execute();
    sm.timeout = false;
    sm.timeout_duration = 0;
}

// CpCommand //

CpCommand::CpCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
    string temp_dest;
    this->cmd_line = cmd_line;
    string cmd_string = this->cmd_line;
    cmd_string = _trim(cmd_string);
    istringstream iss(cmd_string);
    vector<string> args;
    while (iss) {
        string arg;
        iss >> arg;
        if (arg != "")
            args.push_back(arg);
    }
    if (_isBackgroundComamnd(cmd_line)) {
        char dest_arg[COMMAND_ARGS_MAX_LENGTH];
        strcpy(dest_arg, args[2].c_str());
        _removeBackgroundSign(dest_arg);
        temp_dest = dest_arg;
    } else {
        temp_dest = args[2];
    }
    this->are_the_same = false;
    this->source = args[1];
    this->destination = temp_dest;
    try {
        string full_path_source = realpath(this->source.c_str(), NULL);
        string full_path_dest = realpath(this->destination.c_str(), NULL);
        if (full_path_source == full_path_dest) {
            this->are_the_same = true;
        }
    } catch (exception) {
    }
}

void CpCommand::execute() {
    int w_flag, r_flag, content, wrote;
    if (this->are_the_same == true) {
        cout << "smash: " << this->source << " was copied to " << this->destination << endl;
        return;
    }
    bool is_backgroung = _isBackgroundComamnd(this->cmd_line.c_str());
    pid_t res_pid = fork();
    if (res_pid < 0) {
        perror("smash error: fork failed");
        return;
    }
    if (res_pid == 0) {
        setpgrp();
        int fd_read = open(this->source.c_str(), O_RDONLY);
        int fd_write = open(this->destination.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd_read == -1 || fd_write == -1) {
            perror("smash error: open failed");
            exit(0);
        }
        char buffer[MAX_BUFFER_SIZE];
        while ((content = read(fd_read, buffer, MAX_BUFFER_SIZE)) > 0) {
            r_flag = content;
            if ((wrote = write(fd_write, buffer, content)) != content) {
                w_flag = wrote;
                perror("smash error: write failed");
                break;
            }
            memset(buffer, 0, MAX_BUFFER_SIZE);
        }
        if (r_flag == -1) {
            perror("smash error: read failed");
        }
        if (r_flag != -1 && w_flag != -1) {
            cout << "smash: " << this->source << " was copied to " << this->destination << endl;
        }
        if (close(fd_read) == -1) {
            perror("smash error: close failed");
        }
        if (close(fd_write) == -1) {
            perror("smash error: close failed");
        }
        exit(0);
    } else {
        if (is_backgroung) {
            SmallShell::getInstance().job_list->addJob(this->cmd_line.c_str(), res_pid, false);
        } else {
            if (waitpid(res_pid, NULL, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
            }
        }
    }
}

// Auxiliary Functions //

int c_to_int(const char* num) {
    if (num == nullptr) {
        std::cout << "ERROR CONVERTING. NULL_ARG" << endl;
        return 0;
    }
    int res;
    sscanf(num, "%d", &res);
    return res;
}

bool isBuiltIn(std::vector<std::string> args) {
    string builtins[] = {"chprompt", "ls", "showpid", "pwd", "cd", "jobs", "kill", "fg", "bg", "quit"};
    string command_name = args[2];
    for (int i = 0; i < 10; i++) {
        if (builtins[i] == command_name) {
            return true;
        }
    }
    return false;
}

bool compareAlarms(Alarm* alarm_1, Alarm* alarm_2) {
    return alarm_1->when_to_fire < alarm_2->when_to_fire;
}

void CheckTimeout(std::vector<std::string> args) {
    SmallShell& sm = SmallShell::getInstance();
    sm.timeout = false;
    if (args[0] == "timeout") {
        sm.timeout = true;
    }
}
// QuitCommand //
void QuitCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    if (this->got_kill) {
        sm.getJoblist()->killAllJobs();
    }
    exit(0);
}
