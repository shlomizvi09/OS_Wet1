#include "signals.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

#include "Commands.h"
using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim2(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim2(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim2(const std::string &s) {
    return _rtrim2(_ltrim2(s));
}

void ctrlZHandler(int sig_num) {
    SmallShell &sm = SmallShell::getInstance();
    auto tmp_pid = sm.getFgPid();
    if (tmp_pid < 1) {
        return;
    }
    cout << " got ctrl-Z" << endl;
    killpg(tmp_pid, SIGSTOP);
    sm.getJoblist()->addJob(sm.getFgCommand().c_str(), tmp_pid, true);
    cout << "process " << tmp_pid << " was stopped" << endl;
}

void ctrlCHandler(int sig_num) {
    SmallShell &sm = SmallShell::getInstance();
    auto tmp_pid = sm.getFgPid();
    if (tmp_pid < 1) {
        return;
    }
    cout << " got ctrl-C" << endl;
    killpg(tmp_pid, SIGILL);
    cout << "process " << tmp_pid << " was killed" << endl;
}

void alarmHandler(int sig_num) {
    SmallShell &sm = SmallShell::getInstance();
    cout << "smash: got an alarm" << endl;
    for (auto iterator = sm.timed_out_list->timed_dict->begin(); iterator != sm.timed_out_list->timed_dict->end();) {
        if (sm.timed_out_list->timed_dict->empty()) {
            break;
        }
        int pid = iterator->first;
        if (difftime(time(nullptr), iterator->second->timestamp + iterator->second->duration) >= 0) {
            int status;
            int res_pid = waitpid(pid, &status, WNOHANG);
            bool is_exit = WIFEXITED(status);
            if (res_pid != -1) {
                kill(iterator->first, SIGKILL);
                cout << "smash: timeout " << sm.timeout_duration << " " << _trim2(iterator->second->command) << " timed out!" << endl;
                sm.job_list->removeJobById(iterator->first);
                iterator = sm.timed_out_list->timed_dict->erase(iterator);
                if (sm.timed_out_list->timed_dict->empty()) {
                    break;
                }
                if (iterator != sm.timed_out_list->timed_dict->begin()) {
                    iterator--;
                }
            }
        }
        iterator++;
    }
    sm.list_of_alarms->list_of_alarms->erase(sm.list_of_alarms->list_of_alarms->begin());
    if (!sm.timed_out_list->timed_dict->empty()) {
        Alarm *next_alarm = sm.list_of_alarms->list_of_alarms->at(0);
        if (next_alarm != nullptr) {
            int delta_time = next_alarm->when_to_fire - time(nullptr);
            if (delta_time > 0) {
                int alarm_res = alarm(delta_time);
                if (alarm_res == -1) {
                    perror("smash error: alarm failed");
                }
            }
        }
    }
}
