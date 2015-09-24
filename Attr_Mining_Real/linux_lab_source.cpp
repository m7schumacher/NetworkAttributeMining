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
#include "timer.h"

using namespace std;

map<int, vector<int>> vertices;
vector<Pattern> singles;
map<string, int> seen;

vector<vector<double>> matrix;
vector<vector<Pattern>> patts;

vector<Pattern> maximal;
vector<Pattern> local_maximal;
vector<Pattern> local;

string partial_path = "/home/schumach/Desktop/";

ifstream myFile;

int min_count = 70;
double min_dif = .30;
double error = .0001;
int num_attr = 0;
int num_nodes = 0;

double start_globalmax;
double stop_globalmax;

double start_localmax;
double stop_localmax;

double time_global;
double time_local;

int max_size;

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

	for (int i = 0; i < singles.size(); i++)
	{
		patts.push_back({});
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
//	int size_patt = patt.Size();
//	int size_maximal = maximal.size();
//
//	if (size_patt == 1){ return false; }
//
//	for (int i = 0; i < size_maximal; i++)
//	{
//		if (maximal[i].Size() > size_patt)
//		{
//			if (isSubset(maximal[i], patt))
//			{
//				return false;
//			}
//		}
//	}
//
//	return true;
//}

bool isMaximal(Pattern &patt, vector<Pattern> &local)
{
	int size_patt = patt.Size();
	int size_local = local.size();

	if (size_patt == 1){ return false; }

	for (int i = 0; i < size_local; i++)
	{
		if (local[i].Size() > size_patt)
		{
			if (isSubset(local[i], patt))
			{
				return false;
			}
		}
	}

	return true;
}

bool isGlobalMaximal(Pattern &patt, int size)
{
	for (int i = size + 1; i <= max_size; i++)
	{
		for (int j = 0; j < patts[i].size(); j++)
		{
			if (isSubset(patts[i][j], patt))
			{
				return false;
			}
		}
	}

	return true;
}

//bool isGlobalMaximal(Pattern &patt, int &size)
//{
//	int size_patt = patt.Size();
//	if (size_patt == 1){ return false; }
//
//	for (int i = 0; i < size; i++)
//	{
//		if (local[i].Size() > size_patt)
//		{
//			if (isSubset(local[i], patt))
//			{
//				return false;
//			}
//		}
//	}
//
//	return true;
//}

void addMaximal(Pattern &max, vector<Pattern> &patts)
{
	vector<Pattern> copy_maximals;

	for (Pattern p : patts)
	{
		if (max.Size() >= p.Size())
		{
			if (!isSubset(max, p))
			{
				copy_maximals.push_back(p);
			}
		}
		else
		{
			copy_maximals.push_back(p);
		}
	}

	copy_maximals.push_back(max);
	patts = copy_maximals;
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

void Mine(Pattern &patt, vector<Pattern> &local)
{
	Pattern next;
	string res;
	int sz;

	for (int edg : patt.getEdges())
	{
		res = patt.hash(edg);

		if (!(seen.find(res) != seen.end()))
		{
			seen[res] = 1;

			next = makeNew(patt, singles[edg]);
			next.setCode(res);

			if (next.Count() >= min_count)
			{
				if (isMaximal(next, local))
				{
					addMaximal(next, local);
				}
				
				Mine(next,local);
			}
		}
	}
}

void output()
{
	int cnt = 0;
	int total = 0;

	int two_count = 0;
	int three_count = 0;
	int other_count = 0;

	int sz = 0;

	for (Pattern p : maximal)
	{
		sz = p.Size();

		if (p.Size() >= 4)
		{
			cout << "[" << p.toString() << "]\t";
			cnt++;
			total++;
		}

		if (cnt == 5)
		{
			cout << endl;
			cnt = 0;
		}

		//if (sz == 2)
		//{
		//	two_count++;
		//}
		//else if (sz == 3)
		//{
		//	three_count++;
		//}
		//else
		//{
		//	other_count++;
		//}
	}

	//cout << "Twos: " << two_count << endl;
	//cout << "Thre: " << three_count << endl;
	//cout << "Othe: " << other_count << endl;

	cout << endl << endl << "Maximal Patterns Found : " << maximal.size() << endl;
	cout << "Large patterns found: " << total << endl;
}

int main(int argc, char *argv[])
{
	istringstream(argv[4]) >> min_count;
	min_dif = atof(argv[5]) + error;

	string attr = argv[3];
	attr = attr.substr(1, attr.size());

	string net = argv[2];
	net = net.substr(1, net.size());

	string network_path = partial_path + net;
	string attrs_path = partial_path + attr;
	
	cout << "Min_Count = " << min_count << ", Min_Dif = " << min_dif << endl;
	cout << "Network = " << network_path << endl;
	cout << "Attributes = " << attrs_path << endl << endl;

	gen_SinglePatterns(attrs_path);
	gen_Graph(network_path);

	cout << "Graph Generated" << endl;
	cout << "Nodes found = " << singles.size() << endl;
	cout << "Begin Mining..." << endl << endl;

	GET_TIME(start_localmax);
	int size;

	for (Pattern p : singles)
	{
		vector<Pattern> local;

		Mine(p,local);

		for (Pattern p : local)
		{
			size = p.Size();

			if (size > max_size){ max_size = size; }

			patts[size].push_back(p);
			maximal.push_back(p);
		}
	}

	GET_TIME(stop_localmax);

	GET_TIME(start_globalmax);

	Pattern patt;
	int sz;
	int count = 0;
	int cnt = 0;

	for (int i = 0; i <= max_size; i++)
	{
		for (int j = 0; j < patts[i].size(); j++)
		{
			patt = patts[i][j];
			sz = patt.Size();

			if (isGlobalMaximal(patt, sz))
			{
				maximal.push_back(patt);
			}

			count++;
		}
	}

	cout << endl << "Local max found: " << count << endl;

	/*GET_TIME(stop_globalmax);

	output();

	time_global = stop_globalmax - start_globalmax;
	time_local = stop_localmax - start_localmax;

	double sum = time_local + time_global;

	ofstream myfile;
	myfile.open("serial_results.txt", std::ios::app);
	myfile << min_count << " - " << sum << endl;
	myfile.close();

	cout << endl << "time local: " << time_local << endl;
	cout << "time global: " << time_global << endl << endl;

	double local_percentage = time_local / sum;
	double global_percentage = time_global / sum;

	cout << "Local Percentage: " << local_percentage * 100 << "%" << endl;
	cout << "Global Percentage: " << global_percentage * 100 << "%" << endl;

	cout << endl << "Total Time: " << sum << " seconds" << endl;
	cout << max_size << endl;*/
}
