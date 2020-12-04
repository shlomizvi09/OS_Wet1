#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    /*      @@@@@ self testing @@@@@
    JobsList list;
    for (size_t j = 0; j < 50; j++)
    {
        list.addJob("cd",rand()%100 + 1);
    }
    
    
    for (size_t i = 0; i < MAX_SIMULTANEOUS_PROSSESES+1; i++)
    {
        if ((list).jobs[i]){
            std::cout<<list.jobs.find(i)->second->pid<<std::endl;
            std::cout<<list.jobs.find(i)->second->job_id<<std::endl;
            std::cout<<list.jobs.find(i)->second->cmd_line<<std::endl;
            std::cout<<std::endl;
        }  
    }
    */
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.getPromptName()<< "> "; // TODO: change this (why?)
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    

    return 0;
}