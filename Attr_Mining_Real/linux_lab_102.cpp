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
#include "timer.h"

using namespace std;

map<string, int> seen;

vector<vector<double>> matrix;
vector<vector<Pattern>> storage;

vector<Pattern> singles;
vector<Pattern> maximal;
vector<Pattern> true_max;

string partial_path = "/home/schumach/Desktop/";

pthread_mutex_t seen_writer;
pthread_mutex_t storage_writer;
ifstream myFile;

int NUM_THREADS = 0;
int min_count = 70;
int num_nodes = 0;
int big = 0;

double min_dif = .30;
double error = .0001;

double start_localmax;
double start_globalmax;
double stop_localmax;
double stop_globalmax;
double time_local;
double time_global;

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
		storage.push_back({});
		counter++;
	}

	num_nodes = singles.size();
	ifs.close();
}

bool isMaximal(Pattern &patt, vector<Pattern> &local)
{
	int size_patt = patt.Size();
	int size_local = local.size();

	for (int i = 0; i < size_local; i++)
	{
		if (local[i].Size() > size_patt)
		{
			if (patt.isSubset(local[i].getCode()))
			{
				return false;
			}
		}
	}

	return true;
}

Pattern makeNew(Pattern &patt, Pattern &ext)
{
	Pattern next = Pattern(patt, ext);

	vector<int> nodes = next.getNodes();
	vector<int> indexes = patt.getIndexes();
	vector<double> row = matrix[ext.Anchor()];

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

		if (make){ next.addIndex(i); }
	}

	return next;
}

void Mine(Pattern &patt, vector<Pattern> &localMax)
{
	Pattern next;
	string code;
	int edge;

	vector<int> valid;
	vector<int> edges = patt.getEdges();

	vector<string> resses;
	map<string, int>::iterator end = seen.end();

	for (int edg : edges)
	{
		resses.push_back(patt.hash(edg));
	}

	pthread_mutex_lock(&seen_writer);

	for (int i = 0; i < resses.size(); i++)
	{
		code = resses[i];

		if (seen.find(code) == end)
		{
			valid.push_back(i);
			seen[code] = 1;
		}
	}

	pthread_mutex_unlock(&seen_writer);

	for (int index : valid)
	{
		edge = edges[index];
		code = resses[index];

		next = makeNew(patt, singles[edge]);
		next.setCode(code);

		if (next.Count() >= min_count)
		{
			if (isMaximal(next, localMax))
			{
				localMax.push_back(next);
			}

			Mine(next, localMax);
		}
	}
}

void output()
{
	int cnt = 0;
	int total = 0;

	for (Pattern p : true_max)
	{
		if (p.Size() > 3)
		{
			cnt++;
			total++;
		}
	}

	big = total;
	cout << "Maximal Patterns Found : " << true_max.size() << endl;
	cout << "Large patterns found: " << total << endl;
}

void *Thread_Mine(void *threadid)
{
	long rank = (long)threadid;
	int index = rank;
	int sz = 0;

	while (index <= num_nodes)
	{
		vector<Pattern> local;

		Mine(singles[index], local);

		pthread_mutex_lock(&storage_writer);

		for (Pattern next : local)
		{
			for (int i : next.getNodes())
			{
				storage[i].push_back(next);
			}

			maximal.push_back(next);
		}

		pthread_mutex_unlock(&storage_writer);

		index += NUM_THREADS;
	}

	return NULL;
}

void *Thread_Maximal(void *thr)
{
	long rank = (long)thr;
	int size = maximal.size();
	int index = rank;

	Pattern curr;

	while (index < size)
	{
		curr = maximal[index];

		if (isMaximal(curr,storage[curr.lastNode()]))
		{
			pthread_mutex_lock(&storage_writer);
			true_max.push_back(curr);
			pthread_mutex_unlock(&storage_writer);
		}

		index += NUM_THREADS;
	}

	return NULL;
}

void WriteToFile(double local, double global)
{
	ofstream myfile;

	double total = local + global;
	double perc_local = local / total;
	double perc_global = global / total;

	myfile.open("Threading_Results_1_0.txt", std::ios::app);
	myfile << NUM_THREADS << "\t" << min_count << "\t" << time_local << "\t" << time_global << "\t" << total << "\t" << true_max.size() << "\t" << big << endl;
	myfile.close();
}

int main(int argc, char *argv[])
{
	NUM_THREADS = atoi(argv[6]);

	min_count = atoi(argv[4]);
	min_dif = atof(argv[5]) + error;

	string net = argv[2];
	string attr = argv[3];

	string network_path = partial_path + net.substr(1, net.size());
	string attrs_path = partial_path + attr.substr(1, attr.size());

	cout << endl << "Min_Count = " << min_count << ", Min_Dif = " << min_dif << endl;
	cout << endl << "Network = " << network_path;
	cout << endl << "Attributes = " << attrs_path << endl << endl;
	
	gen_SinglePatterns(attrs_path);
	gen_Graph(network_path);

	cout << "Graph Generated" << endl;
	cout << "Nodes found = " << singles.size() << endl << endl;

	pthread_t threads[NUM_THREADS];
	pthread_mutex_init(&storage_writer, NULL);
	pthread_mutex_init(&seen_writer, NULL);


	cout << "Threads Generated = " << NUM_THREADS << endl << endl;
	cout << "Begin Mining Local Maximals..." << endl;

	GET_TIME(start_localmax);

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&threads[i], NULL, Thread_Mine, (void *)i);
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}

	GET_TIME(stop_localmax);

	time_local = stop_localmax - start_localmax;

	cout << maximal.size() << " Local Maximals Generated ( " << time_local << " sec )" << endl << endl;
	cout << "Begin Computing Global Maximals..." << endl;

	GET_TIME(start_globalmax);

	pthread_t second_threads[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&second_threads[i], NULL, Thread_Maximal, (void *)i);
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(second_threads[i], NULL);
	}

	GET_TIME(stop_globalmax);

	time_global = stop_globalmax - start_globalmax;

	cout << "Global Maximals Generated ( " << time_global << " sec )" << endl << endl << endl;

	output();
	WriteToFile(time_local, time_global);
	
	double sum = time_local + time_global;
	double local_percentage = time_local / sum;
	double global_percentage = time_global / sum;

	cout << endl << "Local Percentage: " << local_percentage * 100 << "%" << endl;
	cout << "Global Percentage: " << global_percentage * 100 << "%" << endl << endl;

	cout << time_local << "\t" << time_global << "\t" << sum << endl;
	cout << local_percentage << "\t" << global_percentage;

	cout << endl << endl << "Total time: " << sum << " sec" << endl << endl;

	cout << NUM_THREADS << "\t" << min_count << "\t" << time_local << "\t" << time_global << "\t" << (time_local + time_global) << "\t" << true_max.size() << "\t" << big << endl;

	cout << "Done." << endl;

	pthread_mutex_destroy(&seen_writer);
	pthread_mutex_destroy(&storage_writer);

	exit(EXIT_SUCCESS);

	return 0;
}