#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    if (signal(SIGALRM, alarmHandler) == SIG_ERR) {
        perror("smash error: failed to set alarm handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    while (true) {
        std::cout << smash.getPromptName() << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        if (!cmd_line.empty()) {
            smash.executeCommand(cmd_line.c_str());
        }
    }

    return 0;
}