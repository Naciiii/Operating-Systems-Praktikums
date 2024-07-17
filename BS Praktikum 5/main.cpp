#include <iostream>
#include <vector>
#include <stdio.h>
#include <sched.h>
#include <sys/resource.h>


using namespace std;

int main()
{

	
    //speicher einnehmen
    cout << "using memory" << endl;
    long long arraygroeße = 2000000000;
    int* array  = new int [arraygroeße];
    for (int i = 0; i< arraygroeße; ++i){
        array[i] = i;
    }

    //CPU auslasten
    cout << "using CPU" << endl;
    long long sum = 0;

    for(int i = 0; i< 10000000; ++i){
        sum += i;
    }
    
	
	
    return 0;
}