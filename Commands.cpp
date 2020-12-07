#include "Commands.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
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
SmallShell::SmallShell() : prompt_name("smash"), old_pwd(""), curr_fg_command(""), curr_fg_pid(-1), forked(false) {
    this->job_list = new JobsList();
}

SmallShell::~SmallShell() {
    // TODO: add your implementation
}

Command* SmallShell::CreateCommand(const char* cmd_line) {
    if (!cmd_line) {
        return nullptr;
    }
    string cmd_string = cmd_line;
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
    int num_of_args = args.size();
    int pipe_idx = cmd_string.find("|");
    int redir_idx = cmd_string.find(">");
    if (pipe_idx != cmd_string.npos) {
        return new PipeCommand(cmd_line);
    }
    if (redir_idx != cmd_string.npos) {
        return new RedirectionCommand(cmd_line);
    }
    if (strcmp(args[0].c_str(), "chprompt") == 0) {
        if (num_of_args <= 1) {
            this->changePromptName("smash> ");
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
        strcpy(arg1, args[0].c_str());
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

    //cout << "got ext command" << endl;
    return new ExternalCommand(cmd_line);
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
void JobsList::addJob(string cmd_line, int pid, bool is_stopped) {
    removeFinishedJobs();
    int tmp_job_id = 1;
    for (auto it = jobs.cbegin(), end = jobs.cend();  //to find first free job id
         it != end && tmp_job_id == it->first; ++it, ++tmp_job_id) {
    }
    JobEntry* new_job = new JobEntry(tmp_job_id, pid, false, is_stopped, cmd_line);
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
        if (!sm.forked) {
            setpgrp();
        }
        if (execv("/bin/bash", execv_args) == -1) {
            perror("smash error: execv failed");
            exit(0);
        }
    } else {
        if (is_bg_command) {
            sm.getJoblist()->addJob(this->cmd_line, pid, false);
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

int c_to_int(char* num) {  // aux function
    if (num == nullptr) {
        std::cout << "ERROR CONVERTING. NULL_ARG" << endl;
        return 0;
    }
    int res;
    sscanf(num, "%d", &res);
    return res;
}

void KillCommand::execute() {
    int job_id;
    int signal_num;
    char** args = new char*;
    int num_of_args = _parseCommandLine(this->cmd_line.c_str(), args);
    if (num_of_args != 3) {
        cout << "smash error : kill : invalid arguments " << endl;
        return;
    }
    try {
        signal_num = c_to_int(args[1]);
        job_id = c_to_int(args[2]);
    } catch (invalid_argument) {
        cout << "smash error: kill: invalid arguments" << endl;
        delete args;
        return;
    }
    if (args[1][0] != '-') {
        cout << "smash error: kill: invalid arguments" << endl;
        delete args;
        return;
    }

    signal_num = abs(signal_num);
    JobsList::JobEntry* job_entry = this->jobs->getJobById(job_id);
    if (job_entry == nullptr) {
        cout << "smash error: kill: job-id " << job_id << " does not exist" << endl;
        delete args;
        return;
    }

    pid_t job_pid = job_entry->pid;
    int res = killpg(job_pid, signal_num);
    if (res == -1) {
        perror("smash error: kill failed");
        delete args;
        return;
    }
    cout << "signal number " << signal_num << " was sent to pid " << job_pid << endl;
    delete args;
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
PipeCommand::PipeCommand(const char* cmd_line) {
    string full_cmd = string(cmd_line);
    auto pipe_sign_idx = full_cmd.find("|");
    this->left_cmd = full_cmd.substr(0, pipe_sign_idx);
    if (full_cmd.substr(pipe_sign_idx + 1, 1) == "&") {
        this->right_cmd = full_cmd.substr(pipe_sign_idx + 2);
        this->pipe_type = 1;
    } else {
        this->right_cmd = full_cmd.substr(pipe_sign_idx + 1);
        this->pipe_type = 0;
    }
    cout << "cmd1: " << left_cmd << " cmd2: " << right_cmd << ", type: " << pipe_type << endl;
}

void PipeCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    bool is_bg_command = _isBackgroundComamnd(this->left_cmd.c_str());
    sm.forked = true;
    pid_t top_pid = fork();
    if (top_pid < 0) {
        perror("smash error: fork failed");
        return;
    }
    if (top_pid == 0) {  //main child
        setpgrp();
        int tmp_pipe[2];
        if (pipe(tmp_pipe) < 0) {
            perror("smash error: pipe failed");
            return;
        }
        pid_t left_cmd_pid = fork();
        if (left_cmd_pid < 0) {
            perror("smash error: fork failed");
            return;
        }
        if (left_cmd_pid == 0) {  //left command process
            setpgrp();
            if (this->pipe_type == 0) {
                if (dup2(tmp_pipe[1], STDOUT_FILENO) < 0) {
                    perror("smash error: dup failed");
                    return;
                }
            } else {
                if (dup2(tmp_pipe[1], STDERR_FILENO) < 0) {
                    perror("smash error: dup failed");
                    return;
                }
            }
            if (close(tmp_pipe[0]) < 0 || close(tmp_pipe[1]) < 0) {
                perror("smash error: close failed");
                return;
            }
            sm.executeCommand(this->left_cmd.c_str());
            exit(0);
        }
        pid_t right_cmd_pid = fork();
        if (right_cmd_pid < 0) {
            perror("smash error: fork failed");
            return;
        }
        if (right_cmd_pid == 0) {  //right command process
            setpgrp();
            if (dup2(tmp_pipe[0], STDIN_FILENO) < 0) {
                perror("smash error: dup failed");
                return;
            }
            if (close(tmp_pipe[0] < 0) || close(tmp_pipe[1]) < 0) {
                perror("smash error: close failed");
                return;
            }
            sm.executeCommand(this->right_cmd.c_str());
            exit(0);
        }
        if (close(tmp_pipe[0]) < 0 || close(tmp_pipe[1]) < 0) {
            perror("smash error: close failed");
            return;
        }
        if (waitpid(left_cmd_pid, nullptr, WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
            return;
        }
        if (waitpid(right_cmd_pid, nullptr, WUNTRACED) == -1) {
            perror("smash error: waitpid failed");
            return;
        }
        exit(0);
    }
    if (top_pid > 0) {
        if (is_bg_command) {
            sm.getJoblist()->addJob(cmd_line, top_pid, false);
        } else {
            sm.setFgCommand(cmd_line);
            sm.setFgPid(top_pid);
            if (waitpid(top_pid, nullptr, WUNTRACED) == -1) {
                perror("smash error: waitpid failed");
                return;
            }
            sm.setFgCommand("");
            sm.setFgPid(-1);
            sm.forked = false;
        }
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
    cout << "cmd1: " << cmd << " cmd2: " << dest << ", type: " << redirection_type << endl;
}

void RedirectionCommand::execute() {
    SmallShell& sm = SmallShell::getInstance();
    int fd, old_stdout;
    old_stdout = dup(1);
    if (close(1) < 0) {
        perror("smash error: close failed");
        return;
    }
    if (!redirection_type) {
        fd = open(dest.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
    } else {
        fd = open(dest.c_str(), O_APPEND | O_WRONLY | O_CREAT, 0666);
    }
    if (fd < 0) {
        perror("smash error: open failed");
        return;
    }

    if (is_bg_command) {
        if (!_isBackgroundComamnd(cmd.c_str())) {
            cmd.append(" &");
        }
    }
    sm.executeCommand(this->cmd.c_str());
    close(1);
    dup2(old_stdout, 1);
    close(old_stdout);
}
