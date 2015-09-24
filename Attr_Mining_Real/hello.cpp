#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <ostream>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <stdio.h>      
#include <stdlib.h>
#include <map>

using namespace std;

void *PrintHello(void *threadid)
{
	long tid;
	tid = (long)threadid;
	cout << "Hello World! Thread ID, " << tid << endl;
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int NUM_THREADS = atoi(argv[2]);
	pthread_t threads[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; i++)
	{
		cout << "main() : creating thread, " << i << endl;
		
		pthread_create(&threads[i], NULL, PrintHello, (void *)1);
	}

	pthread_exit(NULL);
}