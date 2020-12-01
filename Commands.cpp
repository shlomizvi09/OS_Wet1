#include "Commands.h"

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

SmallShell::~SmallShell() {
    // TODO: add your implementation
}

Command* SmallShell::CreateCommand(const char* cmd_line) {
    char** args = new char*;
    int num_of_args = _parseCommandLine(cmd_line, args);
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
    }else if (strcmp(args[0], "showpid") == 0){
        return new ShowPidCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char* cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    if (cmd != nullptr) {
        cmd->execute();
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
        if (chdir(this->next_dir)==-1){
                perror("smash error: chdir failed");
                return;
            }
        else {
            ss.changeOldPwd(string(buff));
            return;
        }
    }
}

// Command //
Command::Command(const char* cmd_line) {
    this->cmd_line = cmd_line;
}

Command::Command() {
    this->cmd_line = " ";
}

// ShowPidCommand //
void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << endl;
}