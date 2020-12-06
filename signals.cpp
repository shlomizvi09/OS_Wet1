#include "signals.h"

#include <signal.h>

#include <iostream>

#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  SmallShell &sm = SmallShell::getInstance();
  auto tmp_pid = sm.getFgPid();
  if (tmp_pid < 1){
    return;
  }
  cout<<" got ctrl-Z"<<endl;
  killpg(tmp_pid,SIGSTOP);
  sm.getJoblist()->addJob(sm.getFgCommand().c_str(),tmp_pid,true);
  cout<<"process "<<tmp_pid<<" was stopped"<<endl;
}

void ctrlCHandler(int sig_num) {
    SmallShell &sm = SmallShell::getInstance();
    auto tmp_pid = sm.getFgPid();
    if (tmp_pid < 1){
      return;
    }
    cout<<" got ctrl-C"<<endl;
    killpg(tmp_pid, SIGILL);
    cout<<"process "<<tmp_pid<<" was killed"<<endl;
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}
