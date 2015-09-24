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

using namespace std;

map<int, vector<int>> vertices;
vector<Pattern> singles;
map<string, int> seen;

vector<vector<double>> matrix;

vector<Pattern> maximal;

string partial_path = "/home/schumach/desktop/";

ifstream myFile;

int min_count = 70;
double min_dif = .30;
double error = .0001;
int num_attr = 0;
int num_nodes = 0;

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
	vector<double> attributes;
	int counter = 0;
	int i;
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

bool isMaximal(Pattern &patt)
{
	if (patt.Size() == 1){ return false; }
	return find_if(maximal.begin(), maximal.end(), [patt](Pattern pt) { return isSubset(pt, patt); }) == maximal.end();
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

Pattern makeNew(Pattern &patt,Pattern &ext)
{
	Pattern next = Pattern(patt, ext);

	vector<int> nds = patt.getNodes();
	vector<int> ind = patt.getIndexes();
	vector<int> next_ind;
	vector<double> row = matrix[ext.getNodes()[0]];
	bool make;

	for (int i : ind)
	{
		make = true;

		for (int j : nds)
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

void Mine(Pattern &patt)
{
	Pattern next;

	for (int edg : patt.getEdges())
	{
		next = makeNew(patt,singles[edg]);

		if (!(seen.find(next.hash()) != seen.end()))
		{
			seen[next.hash()] = 1;

			if (next.Count() >= min_count)
			{
				if (isMaximal(next))
				{
					addMaximal(next);
				}

				Mine(next);
			}
		}
	}
}

void output()
{
	int cnt = 0;

	for (Pattern p : maximal)
	{
		if (p.getNodes().size() > 3)
		{
			cout << "[" << p.toString() << "]\t";
			cnt++;
		}
		
		if (cnt == 2)
		{
			cout << endl;
			cnt = 0;
		}
	}

	cout << endl << "Maximal Patterns Found : " << maximal.size() << endl;
}

int main(int argc, char *argv[])
{
	min_count = stoi(argv[4]);
	min_dif = stod(argv[5]) + error;

	string attrs_path = partial_path + argv[3];
	string network_path = partial_path + argv[2];

	cout << "Min_Count = " << min_count << ", Min_Dif = " << min_dif << endl << endl;

	gen_SinglePatterns(attrs_path);
	gen_Graph(network_path);

	cout << "Graph Generated" << endl;
	cout << "Nodes found = " << singles.size() << endl;
	cout << "Begin Mining..." << endl << endl;

	int start_s = clock();

	int level = singles.size() / 10;
	int percent = 0;
	int cnt = 0;

	for (Pattern p : singles)
	{
		Mine(p);
		cnt++;

		if (cnt == level)
		{
			percent += 10;
			cout << percent << "%   ";
			cnt = 0;
		}
	}

	int stop_s = clock();
	cout << endl << endl;

	output();
	cout << endl << "time: " << (stop_s - start_s) / double(CLOCKS_PER_SEC) << endl;

	system("pause");
}