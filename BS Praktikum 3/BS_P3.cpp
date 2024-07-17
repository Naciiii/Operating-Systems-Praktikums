#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <algorithm>

using namespace std;
// Orphan process : if child terminates before parent
// Zombi process : if parent terminates before child

vector<pid_t> bg_pids; //hintergrundprozesse
vector<pid_t> angehalten_vg; //angehaltene vordergrundprozesse
vector<pid_t> angehalten_bg; //angehaltene hintergrundprozesse
pid_t vordergrundProzess = 0; //aktuelle vordergrundprozess

void handle_SIGTSTP(int signum){
    pid_t pid = getpid();
    if (vordergrundProzess != 0 && vordergrundProzess != pid){
        kill(vordergrundProzess, SIGTSTP);
    }
    for (pid_t bg_pid : bg_pids) {
        kill(bg_pid, SIGTSTP);
        angehalten_bg.push_back(bg_pid);
    }
    bg_pids.clear();
    cout << "This process is stopped with SIGTSTP: " << vordergrundProzess<< endl;
    angehalten_vg.push_back(vordergrundProzess);
    cin.clear();
}
void handle_SIGCHLD(int signum){
    pid_t pid;
    int status;
    //WNOHANG returns 0 if there is nothing to wait for
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {

        //WIFEXITED = true -> child terminated normally
        if (WIFEXITED(status))
            cout << "Child process " << pid << " terminated normally with status: " << WEXITSTATUS(status) << endl;
        //WIFSIGNALED = true -> child terminated by a signal
        else if (WIFSIGNALED(status))
            cout << "Child process " << pid << " terminated by signal: " << WTERMSIG(status) << endl;

        // Remove the terminated process from the background processes list
        auto it = find(bg_pids.begin(), bg_pids.end(), pid);
        if (it != bg_pids.end())
            bg_pids.erase(it);
        auto it2 = find(angehalten_bg.begin(), angehalten_bg.end(), pid);
        if (it2 != angehalten_bg.end())
            angehalten_bg.erase(it2);
        auto it3 = find(angehalten_vg.begin(), angehalten_vg.end(), pid);
        if (it3 != angehalten_vg.end())
            angehalten_vg.erase(it3);    
    }
}


void handle_SIGINT(int signum){
    pid_t pid = getpid();
    if (vordergrundProzess != 0 && vordergrundProzess != pid){
        kill(vordergrundProzess, SIGINT);
    }
    cin.clear();
}


void printBgProcesses(){
    if (!bg_pids.empty()) {
            cout << "Background processes:";
            for (auto pid : bg_pids) {
                cout << " " << pid;
            }
            cout << endl;
    }
}
void stopAllBgProcess(){
    for(auto it = bg_pids.begin(); it != bg_pids.end(); it++){
        kill(*it, SIGTERM);     
    }
    for(auto it = angehalten_bg.begin(); it != angehalten_bg.end(); it++){
        kill(*it, SIGTERM);     
    }
    for(auto it = angehalten_vg.begin(); it != angehalten_vg.end(); it++){
        kill(*it, SIGTERM);     
    }
    cout << "All background processes closed" << endl;
}
void stopProcess(vector<pid_t> ids, pid_t pid, int index){
    //check if the PID is of a hintergrundprozess
    kill(pid, SIGTSTP);
    ids.erase(ids.begin()+index);
}
void continueProcess(vector<pid_t> &ids, pid_t pid, int index){
    kill(pid, SIGCONT);
    ids.erase(ids.begin()+index);
}

int get_pid_index(vector<pid_t> *ids, int id){
    for(pid_t i=0; i<ids->size(); i++){
        if((*ids)[i] == id)
            return i;
    }
    return -1;
}

void updateProcesses(vector<pid_t> &processes){
    int status;
    auto begin = processes.begin();
    for(int i=0; i<processes.size(); i++){
        waitpid(processes[i], &status, WNOHANG);
        if(WIFEXITED(status)){
            processes.erase(begin + i);
        }
    }
}
void print_vg_angehalten(){
    if(!angehalten_vg.empty()){
        cout << "Angehaltene VG PIDs: ";
        for(auto a : angehalten_vg){
            cout << a << " ";
        }
        cout << endl;
    }
}
void print_bg_angehalten(){
    if(!angehalten_bg.empty()){
        cout << "Angehaltene BG PIDs: ";
        for(auto b : angehalten_bg){
            cout << b << " ";
        }
        cout << endl;
    }
}
bool running = true;
int main() {
    signal(SIGTSTP, handle_SIGTSTP);
    signal(SIGCHLD, handle_SIGCHLD);
    signal(SIGINT, handle_SIGINT);
    cout << getpid() << endl;
    while (running) {
        cout << "Kleinshell>";
        //read command from user
        string input;
        getline(cin, input);

        //check for exit cmd 
        if(input == "exit"){
            if(bg_pids.empty()&&angehalten_bg.empty()&&angehalten_vg.empty()){
                cout << "Do you really want to exit? Y/n ";
                char a;
                cin >> a;
                if(a=='y'||a=='Y'){
                    cout << "Exiting shell..." << endl;
                    break;
                }
                else if(a=='n'||a=='N'){
                    cout << "Okay, then continue " << endl;
                    continue;
                }
            }
            else{
                cout << "There are still background processes running, cannot exit shell." << endl;
                cout << "Do you want to close all background processes? (y/n) ";
                char a;
                cin >> a;
                if(a=='y'||a=='Y') {
                    stopAllBgProcess();
                    cout << "Exiting shell" << endl;
                    break;
                }
                else if(a=='n'||a=='N'){
                    cout << "Okay, then continue " << endl;
                    continue;
                }
            }
        }
        //check for stop cmd
        else if(input.find("stop ") == 0){
            string stopCmd = input.substr(5); //extract process id from the command
            pid_t pid = stoi(stopCmd);
            int index;
            if((index = get_pid_index(&bg_pids, pid)) != -1){
                kill(pid, SIGTSTP);
                bg_pids.erase(bg_pids.begin()+index);
                angehalten_bg.push_back(pid);
            }else{
                cout << "Prozess nicht gefunden!" << endl;                
            }
            continue;
        }
        //check for cont cmd
        else if (input.find("cont ") == 0) {
            string contCommand = input.substr(5);
            pid_t pid = stoi(contCommand);
            int index;
            if((index = get_pid_index(&angehalten_bg, pid)) != -1){
                continueProcess(angehalten_bg, pid, index);
            }else if((index = get_pid_index(&angehalten_vg, pid)) != -1){
                continueProcess(angehalten_vg, pid, index);
                vordergrundProzess = pid;
                waitpid(pid, nullptr , WUNTRACED);
                continue;
            }else{
                cout << "PID nicht gefunden!" << endl;
                continue;
            }
        }

        //check if process should run in background
        bool bg = false;
        if (!input.empty() && input.back() == '&') {
            bg = true;
            input.pop_back();
        }

        //split cmd into arguments
        vector<string> args;
        string arg;
        for (char i : input) {
            if (i == ' ') {
                if (!arg.empty()) {
                    args.push_back(arg);
                    arg = "";
                }
            } else {
                arg += i;
            }
        }
        if (!arg.empty()) {
            args.push_back(arg);
        }

        //convert arguments to char array
        char* argv[args.size() + 1];
        for (int i = 0; i < args.size(); i++) {
            argv[i] = (char*) args[i].c_str();
        }
        argv[args.size()] = nullptr;


        //create child process to execute command
        pid_t pid = fork();
        if (pid == -1) {
            cout << "Failed to create new process" << endl;
        } else if (pid == 0) {
            //child process
            if(*argv != "exit") {
                execvp(argv[0], argv);
                cout << "Failed to execute command" << endl;
                exit(1);
            }
        } else {
            //parent process
            setpgid(pid,pid); //set kindprozess's group ID to its own process ID
            if(bg){
                bg_pids.push_back(pid);
                cout << "Background process started with PID: " << pid << endl;
            }
            else{
                setpgid(pid,pid);
                vordergrundProzess = pid; //update the vgp variable
                int status;
                cout << "Current process ID: " << pid << endl;
                //WUNTRACED allow parent process to be returned from waitpid when child is interrupted -> don't have to wait
                waitpid(pid, &status, WUNTRACED);
                if(WIFSTOPPED(status)){
                    cout << "Process stopped. PID: " << pid << endl;
                }
            }
        }
        //print alle laufenden / angehaltenen Prozesse
        printBgProcesses();
        print_vg_angehalten();
        print_bg_angehalten();
    }
    return 0;
}
