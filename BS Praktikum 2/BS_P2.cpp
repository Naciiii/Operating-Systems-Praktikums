#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>
#include <string>
#include <sstream>

using namespace std;

// Orphan process : if child terminates before parent
// Zombi process : if parent terminates before child

bool running = true;
vector<pid_t> background_processes;

int main()
{
    string eingabe;
    
    while (running) {
        cout << "Kleinshell> ";
        getline(cin, eingabe);

        if (eingabe == "exit") {
            cout << " Shell beenden? (y/n): ";
            char antwort;
            cin >> antwort;
            if (antwort == 'y') {
                break;
            }
            else
            continue;
        }
                    // Tokenisierung der Eingabe
        vector<string> args;    
        istringstream instringstream(eingabe);
        string token;
        while (instringstream >> token) {
            args.push_back(token);
        }

        if (args.empty())  // Überprüfen ob Eingabe leer ist
            continue;  
        

        bool run_in_background = false;  // mit "&" wird der Program im Hintergrund weiter laufen
        if (args.back() == "&") {
            run_in_background = true;
            args.pop_back();
        }

        //Umwandlung der Argumente für execvp
        char** argv = new char*[args.size() + 1];
        for (size_t i = 0; i < args.size(); i++) {
            argv[i] = new char[args[i].length() + 1];
            strcpy(argv[i], args[i].c_str());
        }
        argv[args.size()] = NULL;

        pid_t child_pid = fork(); // Kindprozess erzeugen

        if (child_pid == 0) {
            execvp(argv[0], argv);
            cerr << "Fehler beim Starten des Prozesses!" << endl;
            exit(1);
        } else if (child_pid < 0) {
            cerr << "Fehler beim Forken des Kindprozesses!" << endl;
        } else {
            cout << "Prozess mit PID " << child_pid << " gestartet." << endl;

            if (run_in_background) {
                background_processes.push_back(child_pid);
            } else {
                int status;
                waitpid(child_pid, &status, WUNTRACED);
                cout << "Prozess mit PID " << child_pid << " beendet." << endl;
            }
        }

        // Die Ressourcen für die Argumente freiräumen
        for (size_t i = 0; i < args.size(); i++) {
            delete[] argv[i];
        }
        delete[] argv;

        // Background Prozesse ausgeben
        if (!background_processes.empty()) {
            cout << "Hintergrundprozesse:";
            for (pid_t pid : background_processes) {
                cout << " " << pid;
            }
            cout << endl;
        }
    }

    // Background Prozesse beenden.
    for (pid_t pid : background_processes) {
        int status;
        waitpid(pid, &status, 0);
        cout<<"Hintergrundprozess: "<< pid <<" beendet."<< endl;
    }

    return 0;
}
