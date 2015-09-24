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
#include <unordered_set>

using namespace std;

unordered_set<string> seen;

vector<vector<double>> matrix;
vector<vector<Pattern>> storage;
vector<vector<Pattern>> threader;

vector<Pattern> singles, maximal, true_max;

pthread_mutex_t seen_writer, storage_writer;

string partial_path = "/home/schumach/Desktop/";

int NUM_THREADS, min_count, num_nodes, big;
double min_dif, error = .0001;
double start_localmax, start_globalmax, stop_localmax, stop_globalmax, time_local, time_global;

vector<string> split(string st)
{
	string buf;
	stringstream ss(st);

	vector<string> tokens;

	while (ss >> buf){ tokens.push_back(buf); }

	return tokens;
}

void gen_Graph(string file)
{
	string line;
	int l, r;
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
	string res;
	int edge;

	vector<int> edges = patt.getEdges();

	for (int edg : edges)
	{
		res = patt.hash(edg);

		pthread_mutex_lock(&seen_writer);

		bool sn = seen.find(res) == seen.end();

		if (sn)
		{
			seen.insert(res);

			pthread_mutex_unlock(&seen_writer);

			next = makeNew(patt, singles[edg]);
			next.setCode(res);

			if (next.Count() >= min_count)
			{
				if (isMaximal(next, localMax))
				{
					localMax.push_back(next);
				}

				Mine(next, localMax);
			}
		}
		else{ pthread_mutex_unlock(&seen_writer); }
	}
}

void *Thread_Mine(void *threadid)
{
	long rank = (long)threadid;
	int index = rank, sz = 0;

	vector<Pattern> total;

	while (index <= num_nodes)
	{
		vector<Pattern> local;

		Mine(singles[index], local);

		for (Pattern next : local)
		{
			total.push_back(next);
		}

		index += NUM_THREADS;
	}

	threader[rank] = total;

	return NULL;
}

void *Thread_Maximal(void *thr)
{
	long rank = (long)thr;
	int size = maximal.size(), index = rank;

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

void CombineResults()
{
	Pattern curr;

	for (int i = 0; i < threader.size(); i++)
	{
		for (int j = 0; j < threader[i].size(); j++)
		{
			curr = threader[i][j];

			for (int x : curr.getNodes())
			{
				storage[x].push_back(curr);
			}

			maximal.push_back(curr);
		}
	}
}

void output()
{
	int cnt = 0, total = 0;

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
		threader.push_back({});
		pthread_create(&threads[i], NULL, Thread_Mine, (void *)i);
	}

	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}

	GET_TIME(stop_localmax);

	time_local = stop_localmax - start_localmax;

	CombineResults();

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