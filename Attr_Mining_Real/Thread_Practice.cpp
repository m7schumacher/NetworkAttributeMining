#include <iostream>
#include <fstream>
#include <ostream>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <stdio.h>      
#include <stdlib.h>
#include <pthread.h>

int thread_count;

void* Hello(void* rank);

int main(int argc, char *argv[])
{
	long thread;
	pthread_t *thread_handles;

	thread_count = strtol(argv[1], NULL, 10);
	thread_handles = malloc(thread_count * sizeof(pthread_t));

	for (thread = 0; thread < thread_count; thread++)
	{
		pthread_create(&thread_handles[thread], NULL, Hello, (void*)thread);
	}

	std::cout << "Hello from the main thread" << std::endl;

	for (thread = 0; thread < thread_count; thread++)
	{
		pthread_join(thread_handles[thread], NULL);
	}

	free(thread_handles);
	return 0;
}

void *Hello(void* rank)
{
	long my_rank = (long)rank;

	std::cout << "Hello from thread " << my_rank << " of " << thread_count << std::endl;

	return NULL;
}