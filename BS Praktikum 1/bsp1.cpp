#include <iostream>
#include <sys/resource.h>
#include <cstring>
#include <exception>

using namespace std;

const int MAX_RECURSION_DEPTH = 10000;

void funcMem(int limit, int sample) {
    int* arr = new int[limit];
    for (int i = 0; i < limit; i++) {
        arr[i] = i;
        if ((i + 1) % sample == 0) {
            rusage usage;
            if (getrusage(RUSAGE_SELF, &usage) == 0) {
                cout << "Benutzer-CPU-Zeit: " << usage.ru_utime.tv_sec << "s " << usage.ru_utime.tv_usec << "μs\n";
                cout << "System-CPU-Zeit: " << usage.ru_stime.tv_sec << "s " << usage.ru_stime.tv_usec << "μs\n";
                cout << "Speichernutzung: " << usage.ru_maxrss << " Bytes\n";
            }
        }
    }
    delete[] arr;
}

void funcRec(int counter, int sample, int limit) {
    int frame_size = 4 * sizeof(int) + 8;
    counter++;
    if (counter % sample == 0) {
        rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            cout << "Rahmengröße: " << counter * frame_size << " Bytes\n";
            cout << "Benutzer-CPU-Zeit: " << usage.ru_utime.tv_sec << "s " << usage.ru_utime.tv_usec << "μs\n";
            cout << "System-CPU-Zeit: " << usage.ru_stime.tv_sec << "s " << usage.ru_stime.tv_usec << "μs\n";
        }
    }
    if (counter >= MAX_RECURSION_DEPTH || counter >= limit)
        return;
    funcRec(counter, sample, limit);
}

unsigned getCurMem() {
    rusage usage;
    return getrusage(RUSAGE_SELF, &usage) == 0 ? usage.ru_maxrss : -1;
}

void checkResults() {
    cout << "Statistiken:" << endl;
    cout << "Maximale Speichernutzung: " << getCurMem() << " KB\n";
    rlimit stack_limit;
    if (getrlimit(RLIMIT_STACK, &stack_limit) == 0) {
        cout << "Maximale Stack-Größe Soft Limit: " << stack_limit.rlim_cur / 1024 << " KB\n";
        cout << "Maximale Stack-Größe Hard Limit: " << stack_limit.rlim_max / 1024 << " KB\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        if (strcmp(argv[1], "speicher") == 0) {
            try {
                while (true) {
                    int* a = new int[40000000];
                }
            } catch (exception &e) {
                cout << "Ausnahme: " << e.what() << "\nSpeicherlimit erreicht. Beende das Programm..." << endl;
                return 0;
            }
        } else if (strcmp(argv[1], "stack") == 0) {
            int x = 0;
            while (true) {
                x++;
            }
        }
    }
    cout << "---------------- FuncRec ----------------" << endl;
    funcRec(0, 1000, 10000);
    cout << "##########################################" << endl;
    cout << "---------------- FuncMem ----------------" << endl;
    funcMem(10000, 1000);
    cout << "##########################################" << endl;
    checkResults();
    return 0;
}
