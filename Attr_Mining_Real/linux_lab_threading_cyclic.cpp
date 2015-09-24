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
#include "Pattern.h"
#include <ctime>
#include <pthread.h>
#include <time.h>

using namespace std;

map<int, vector<int>> vertices;
vector<Pattern> singles;
map<string, int> seen;

vector<vector<double>> matrix;
map<int, vector<int>> partitions;

vector<Pattern> maximal;
vector<Pattern> true_max;

string partial_path = "/home/schumach/Desktop/";

ifstream myFile;

int min_count = 70;
double min_dif = .30;
double error = .0001;
int num_attr = 0;
int num_nodes = 0;

double chunk_size = 0;
double chunk = 0;
int NUM_THREADS;

int current = 0;
pthread_mutex_t mutex;

vector<string> split(string st)
{
	string buf;
	stringstream ss(st);

	vector<string> tokens;

	while (ss >> buf)
		tokens.push_back(buf);

	return tokens;
}

void gen_Graph(string file)
{
	string line;
	int l;
	int r;
	vector<string> splitter;

	ifstream ifs(file, ifstream::in);

	while (getline(ifs, line))
	{
		splitter = split(line);

		istringstream(splitter[0]) >> l;
		istringstream(splitter[1]) >> r;

		singles[l].addEdge(r);
		singles[r].addEdge(l);
	}

	ifs.close();
}

void gen_SinglePatterns(string file)
{
	string line;
	vector<string> splitter;
	int counter = 0;
	double curr;

	ifstream ifs(file, ifstream::in);

	while (getline(ifs, line))
	{
		Pattern patt = Pattern(counter);
		matrix.push_back({});

		splitter = split(line);

		for (int i = 0; i < splitter.size(); i++)
		{
			curr = stod(splitter[i]);

			matrix[counter].push_back(curr);
			patt.setAttribute(i, curr);
		}

		singles.push_back(patt);
		counter++;
	}

	ifs.close();
}

//next is a subset of patt
bool isSubset(Pattern patt, Pattern next)
{
	vector<int> first = patt.getNodes();
	vector<int> sec = next.getNodes();

	return includes(first.begin(), first.end(), sec.begin(), sec.end());
}

//bool isMaximal(Pattern &patt)
//{
//	if (patt.Size() == 1){ return false; }
//	return find_if(maximal.begin(), maximal.end(), [patt](Pattern pt) { return isSubset(pt, patt); }) == maximal.end();
//}

bool isMaximal(Pattern &patt, vector<Pattern> &local)
{
	if (patt.Size() == 1){ return false; }
	int count = count_if(local.begin(), local.end(), [patt](Pattern pt) { return isSubset(pt, patt); });

	if (count > 1){ return false; }
	else{ return true; }
}

void addMaximal(Pattern &max)
{
	vector<Pattern> copy_maximals;

	for (Pattern p : maximal)
	{
		if (!isSubset(max, p))
		{
			copy_maximals.push_back(p);
		}
	}

	copy_maximals.push_back(max);
	maximal = copy_maximals;
}

Pattern makeNew(Pattern &patt, Pattern &ext)
{
	Pattern next = Pattern(patt, ext);

	vector<int> nodes = next.getNodes();
	vector<int> indexes = patt.getIndexes();

	vector<int> next_ind;
	vector<double> row = matrix[ext.getNodes()[0]];
	bool make;

	for (int i : indexes)
	{
		make = true;

		for (int j : nodes)
		{
			if (abs(matrix[j][i] - row[i]) >= min_dif)
			{
				make = false;
				break;
			}
		}

		if (make){ next_ind.push_back(i); }
	}

	next.setIndexes(next_ind);

	return next;
}

void Mine(Pattern &patt, vector<Pattern> &localMax)
{
	Pattern next;

	for (int edg : patt.getEdges())
	{
		next = makeNew(patt, singles[edg]);

		if (!(seen.find(next.hash()) != seen.end()))
		{
			pthread_mutex_lock(&mutex);
			seen[next.hash()] = 1;
			pthread_mutex_unlock(&mutex);

			if (next.Count() >= min_count)
			{
				if (isMaximal(next,localMax))
				{
					localMax.push_back(next);
				}

			/*	if (isMaximal(next))
				{
					mtx.lock();
					addMaximal(next);
					mtx.unlock();
				}*/

				Mine(next,localMax);
			}
		}
	}
}

void* Thread_Partition(void *threadid)
{
	long rank = (long)threadid;

	vector<int> rows;

	for (int j = 0; j < num_nodes; j++)
	{
		if (j % NUM_THREADS == rank)
		{
			rows.push_back(j);
		}
	}

	pthread_mutex_lock(&mutex);
	partitions[rank] = rows;
	pthread_mutex_unlock(&mutex);
}

void *Thread_Mine(void *threadid)
{
	long rank = (long)threadid;
	vector<Pattern> local;

	for (int i : partitions[rank])
	{
		Mine(singles[i], local);
	}

	for (Pattern p : local)
	{
		pthread_mutex_lock(&mutex);
		maximal.push_back(p);
		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}

void *Thread_Maximal(void *thr)
{
	long rank = (long)thr;
	int begin;
	int end;

	if (rank == NUM_THREADS - 1)
	{
		begin = chunk * rank;
		end = maximal.size();
	}
	else
	{
		begin = chunk * rank;
		end = begin + chunk - 1;
	}

	vector<Pattern> copy_max;

	for (int i = begin; i <= end; i++)
	{
		if (isMaximal(maximal[i],maximal))
		{
			pthread_mutex_lock(&mutex);
			true_max.push_back(maximal[i]);
			pthread_mutex_unlock(&mutex);
		}
	}

	return NULL;
}

void output()
{
	int cnt = 0;
	int total = 0;

	for (Pattern p : true_max)
	{
		if (p.Size() > 3)
		{
			cout << "[" << p.toString() << "]\t";
			cnt++;
			total++;
		}

		if (cnt == 2)
		{
			cout << endl;
			cnt = 0;
		}
	}

	cout << endl << endl << "Maximal Patterns Found : " << true_max.size() << endl;
	cout << "Large patterns found: " << total << endl;
}

int main(int argc, char *argv[])
{
	time_t start;
	time_t stop;

	istringstream(argv[4]) >> min_count;
	min_dif = atof(argv[5]) + error;

	string attr = argv[3];
	string net = argv[2];

	attr = attr.substr(1, attr.size());
	net = net.substr(1, net.size());

	NUM_THREADS = atoi(argv[6]);

	string network_path = partial_path + net;
	string attrs_path = partial_path + attr;

	cout << endl << endl;
	cout << "Network = " << network_path << endl;
	cout << "Attributes = " << attrs_path << endl;
	cout << "Min_Count = " << min_count << ", Min_Dif = " << min_dif << endl << endl;
	
	gen_SinglePatterns(attrs_path);
	gen_Graph(network_path);

	cout << "Graph Generated" << endl;
	cout << "Nodes found = " << singles.size() << endl << endl;
	cout << "Generating Threads..." << endl;

	chunk_size = (float)singles.size() / NUM_THREADS;
	num_nodes = singles.size();

	pthread_t* threads = (pthread_t*)malloc(NUM_THREADS* sizeof(pthread_t));
	pthread_mutex_init(&mutex, NULL);

	cout << "Threads Generated: " << NUM_THREADS << endl << endl;

	cout << "Begin mining..." << endl << endl;

	time(&start);

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&threads[i], NULL, Thread_Partition, (void *)i);
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}

	cout << "Partitions formed" << endl;

	for (int i = 0; i < NUM_THREADS; i++)
	{
		cout << i << ": " << partitions[i].size() << endl;
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&threads[i], NULL, Thread_Mine, (void *)i);
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}

	cout << "Local Maximals gathered" << endl;
	cout << "Computing global maximals..." << endl;

	chunk = maximal.size() / NUM_THREADS;
	pthread_t second_threads[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&second_threads[i], NULL, Thread_Maximal, (void *)i);
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(second_threads[i], NULL);
	}

	time(&stop);

	double second = difftime(start, stop);
	second = 0 - second;
	
	output();
	cout << endl << "time: " << second << " seconds" << endl;
	//cout << endl << "time: " << (stop_s - start_s) / double(CLOCKS_PER_SEC) << endl;
}