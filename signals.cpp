#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    std::cout<<"smash: got ctrl-C"<<std::endl;
    pid_t fpid = SmallShell::getInstance().getForegroundPid();
    if(fpid == -1){
        return;
    }
    if(fpid !=-1){
        if(kill(fpid,SIGKILL) == -1){  
            perror("smash error: kill failed");
            return;
        }
        std::cout<<"smash: process "<< fpid <<" was killed"<<std::endl;
        SmallShell::getInstance().setForegroundPid(-1);
        SmallShell::getInstance().m_curr_foregrount_cmd = "";
    }
}
