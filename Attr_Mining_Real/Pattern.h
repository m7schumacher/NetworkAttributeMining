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
#include <functional>
#include <unordered_set>

using namespace std;

class Pattern
{
	vector<int> nodes;
	vector<int> edges;
	vector<double> attributes;
	vector<int> indexes;

	int size;
	int count;
	int anchor;
	string code;

	bool inNodes(int i)
	{
		return find(nodes.begin(), nodes.end(), i) != nodes.end();
	}

	bool inEdges(int i)
	{
		return find(edges.begin(), edges.end(), i) != edges.end();
	}

public:
	Pattern(int n);
	Pattern(Pattern f, Pattern s);
	Pattern();
	
	vector<int> getNodes(){ return nodes; }
	vector<int> getEdges(){ return edges; }
	vector<double> getAttributes(){ return attributes; }
	vector<int> getIndexes(){ return indexes; }

	int Size(){ return size; }
	int Count(){ return count; }
	int Anchor(){ return anchor; }

	void setSize(int s){ size = s; }
	void addCount(){ count++; }
	void setAnchor(int a){ anchor = a; }

	void setCode(string ha)
	{
		code = ha;
	}

	string getCode()
	{
		return code;
	}

	void setEdges(vector<int> &edg)
	{
		edges = edg;
	}

	void setNodes(vector<int> &nds)
	{
		nodes = nds;
	}

	void setIndexes(vector<int> ind)
	{
		indexes = ind;
	}

	int firstNode()
	{
		return nodes[0];
	}

	int lastNode()
	{
		return nodes[size - 1];
	}

	void addEdge(int i)
	{
		edges.push_back(i);
	}

	double Convert()
	{
		string res = "";
		std::string::size_type sz;

		for (int i : nodes)
		{
			res += to_string(i);
		}

		double st = stod(res, &sz);

		return st;
	}

	void addIndex(int i)
	{
		indexes.push_back(i);
		count++;
	}

	void setAttributes(vector<double> &att)
	{
		attributes = att;
		for (int i = 0; i < att.size(); i++)
		{
			indexes.push_back(i);
		}
	}

	void setAttribute(int index, double value)
	{
		attributes.push_back(value);
		indexes.push_back(index);
	}

	void combineNodes(vector<int>& one, vector<int>& two)
	{
		set_union(one.begin(), one.end(), two.begin(), two.end(), back_inserter(nodes));
	}

	void combineEdges(vector<int>& one, vector<int>& two)
	{
		vector<int> storage;

		set_union(one.begin(), one.end(), two.begin(), two.end(), back_inserter(storage));
		set_difference(storage.begin(), storage.end(), nodes.begin(), nodes.end(), back_inserter(edges));
	}

	bool isSubset(string code)
	{
		for (int i : nodes)
		{
			if (code.find(" " + to_string(i) + " ") == std::string::npos)
			{
				return false;
			}
		}

		return true;
	}

	string hash(int ext)
	{
		string results = " ";
		vector<int> copy_nds = nodes;

		nodes.push_back(ext);
		sort(nodes.begin(), nodes.end());

		for (int nd : nodes)
		{
			results += to_string(nd) + " ";
		}

		nodes = copy_nds;

		return results;
	}

	string hash()
	{
		string results = "";
		sort(nodes.begin(), nodes.end());

		for (int nd : nodes)
		{
			results += to_string(nd);
		}

		return results;
	}

	//toString method return string representation of nodes in pattern
	string toString()
	{
		string results;

		for (int nd : nodes)
		{
			results += to_string(nd) + " ";
		}

		return results;
	}

	bool equals(Pattern p)
	{
		vector<int> other_nds = p.getNodes();

		if (p.size != size)
		{
			return false;
		}
		else
		{
			for (int i = 0; i < size; i++)
			{
				if (nodes[i] != other_nds[i])
				{
					return false;
				}
			}
		}

		return true;
	}
};

Pattern::Pattern(int n)
{
	nodes.push_back(n);
	size = 1;
	anchor = n;
}

Pattern::Pattern()
{

}

Pattern::Pattern(Pattern f, Pattern s)
{
	nodes = f.getNodes();
	nodes.push_back(s.firstNode());
	sort(nodes.begin(), nodes.end());

	combineEdges(f.edges, s.edges);

	count = 0;
	size = f.size + 1;
}