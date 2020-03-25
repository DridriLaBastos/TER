#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <vector>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

using Weight = float;

struct VertexStruct
{
	unsigned int n;//Vertex number
	Weight w;//Vertex weight

	std::vector<VertexStruct*> neighbors;

	VertexStruct(const unsigned int n, const Weight w = 1) { this->n = n;   this->w = w; }
	VertexStruct(const VertexStruct& vs) { n = vs.n; w = vs.w; std::cout << "VertexStruct copy!\n"; }
};

//On veut le unique_ptr pour ne pas avoir de copy de VertexStruct si jamais VertexContainer.emplace_back doit allouer
//un nouveau tableau
using VertexContainer = std::vector<std::unique_ptr<VertexStruct>>;
using Vertex = VertexStruct*;
using Vertices = std::vector<Vertex>;
using VertexOrdering = Vertices;

using Edge = std::pair<Vertex, Vertex>;
using Edges = std::vector<Edge>;

static void connect(Vertex a, Vertex b)
{
	auto found = std::find(a->neighbors.begin(), a->neighbors.end(), b);

	//On connect deux sommets que s'ils ne sont pas déjà connectés
	if (found == a->neighbors.end())
	{
		a->neighbors.emplace_back(b);
		b->neighbors.emplace_back(a);
	}
}

static Edge makeEdge(const Vertex& a, const Vertex& b)
{
	connect(a, b);
	return std::make_pair(a, b);
}

struct VertexDegreePair
{
	Vertex v;
	unsigned int d;
	//v: Vertex   d: degree
	VertexDegreePair(const Vertex& ver, const unsigned int d) : v(ver) { this->d = d; }
};

using VertexDegreePairs = std::vector<VertexDegreePair>;

class VertexSet
{
public:
	VertexSet(void) {}
	VertexSet(const Vertices& vertices) : m_vertices(vertices) {}

public:
	void emplace_back(const Vertex& v) { m_vertices.emplace_back(v); }
	void pop_back(void) { m_vertices.pop_back(); }
	void reserve(const size_t n) { m_vertices.reserve(n); }

	const Vertices& getVertices(void) const { return m_vertices; }

	Vertices::iterator begin(void) { return m_vertices.begin(); }
	Vertices::const_iterator begin(void) const { return m_vertices.begin(); }

	Vertices::iterator end(void) { return m_vertices.end(); }
	Vertices::const_iterator end(void) const { return m_vertices.end(); }

	Weight weight(void) const
	{
		Weight totalWeight = 0;

		for (const Vertex v : m_vertices)
			totalWeight += v->w;

		return totalWeight;
	}

	Weight getMaxWeight(void) const
	{
		Weight max = 0;

		for (const Vertex v : m_vertices)
			max = (v->w > max) ? v->w : max;
		return max;
	}

	//TODO: améliorer le O(n^2)
	void orderWith(const VertexOrdering& O)
	{
		unsigned int vPos = 0;

		for (size_t i = 0; (i < O.size()) && (vPos < m_vertices.size()); ++i)
		{
			for (size_t j = vPos; j < m_vertices.size(); ++j)
			{
				if (m_vertices[j] == O[i])
					std::swap(m_vertices[vPos++], m_vertices[j]);
			}
		}
	}

	VertexSet subSet(const size_t begin, const size_t end)
	{
		VertexSet subset;
		if (begin <= end)
		{
			subset.reserve(end - begin + 1);//+1: index depuis 0

			for (size_t i = begin; i <= end; ++i)
				subset.emplace_back(m_vertices[i]);
		}
		return subset;
	}

	void remove(const VertexSet& V)
	{
		for (const Vertex& v: V)
		{
			for (size_t i = 0; i < m_vertices.size(); ++i)
			{
				if (m_vertices[i] == v)
				{
					std::swap(m_vertices[i], m_vertices.back());
					m_vertices.pop_back();
					break;
				}
			}
		}
	}

	size_t size(void) const { return m_vertices.size(); }
	bool empty(void) const { return m_vertices.empty(); }
	const Vertex& operator[] (const size_t pos) const { return m_vertices[pos]; }

	static VertexSet unionBetween(const VertexSet& V1, const Vertex& v)
	{
		VertexSet finalUnion(V1);

		auto found = std::find(finalUnion.begin(), finalUnion.end(), v);

		if (found == finalUnion.end())
			finalUnion.emplace_back(v);
		return finalUnion;
	}

	static VertexSet unionBetween(const VertexSet& V1, const VertexSet& V2)
	{
		VertexSet finalUnion(V1);
		for (const Vertex& v: V2)
			finalUnion = unionBetween(finalUnion, v);
		return finalUnion;
	}

	static VertexSet intersectionBetween(const VertexSet& V1, const VertexSet& V2)
	{
		VertexSet finalIntersection;
		for (const Vertex& v: V1)
		{
			for (size_t i = 0; i < V2.size(); ++i)
			{
				if (V2[i] == v)
					finalIntersection.emplace_back(v);
			}
		}
		return finalIntersection;
	}

private:
	Vertices m_vertices;
};

using VertexSets = std::vector<VertexSet>;
using Clique = VertexSet;
using Cliques = VertexSets;

class Graph
{
public:
	Graph(const Edges& edges = {}) : m_edges(edges) { rebuildVerticesSet(); }
	Graph(const Vertices& vertices, const Edges& edges) : m_vertices(vertices), m_edges(edges) {}

public:
	Graph operator[](const VertexSet& V) const
	{
		Graph ret;
		ret.m_vertices = V.getVertices();
		Edges E;

		for(const Edge& e: m_edges)
		{
			bool findFirst = false;
			bool findSecond = false;

			for (size_t i = 0; i < V.size(); ++i)
			{
				if (V[i] == e.first)
					findFirst = true;
			
				if (V[i] == e.second)
					findSecond = true;
			
				if (findFirst && findSecond)
				{
					ret.m_edges.emplace_back(e);
					break;
				}
			}
		}

		return ret;
	}

	VertexDegreePairs computeDegrees(void) const
	{
		VertexDegreePairs ret;	ret.reserve(m_vertices.size());

		for (const Vertex v : m_vertices)
			ret.emplace_back(v, v->neighbors.size());

		return ret;
	}

	void removeVertex(const Vertex v)
	{
		size_t vPosInVertices = 0;

		for (size_t i = 0; i < m_vertices.size(); ++i)
		{
			if (m_vertices[i] == v)
			{
				vPosInVertices = i;
				break;
			}
		}

		if (vPosInVertices < m_vertices.size())
		{
			Vertex saveOfLastVertex = m_vertices.back();

			std::swap(m_vertices[vPosInVertices], m_vertices.back());
			m_vertices.pop_back();

			//On présèrve l'ordre des vertex
			//TODO: est-ce vraiment nécessaire ? Le profiler windows n'affichait même pas l'utilisation de orderWith
			//comme importante pour le CPU ?
			for (size_t i = vPosInVertices; i < m_vertices.size() - 1; ++i)
			{ m_vertices[i] = m_vertices[i+1]; }

			m_vertices[m_vertices.size() - 1] = saveOfLastVertex;

			size_t graphSize = m_edges.size();
			for (size_t i = 0; i < graphSize; ++i)
			{
				Edge& currentEdge = m_edges[i];

				if ((currentEdge.first == v) || (currentEdge.second == v))
				{
					std::swap(m_edges[i], m_edges.back());
					m_edges.pop_back();
					--graphSize;
					--i;
				}
			}
		}
	}

	size_t size(void) const { return m_vertices.size(); }
	bool empty(void) const { return m_vertices.empty(); }

	const Vertices& getVertices(void) const { return m_vertices; }
	const VertexSet getVertexSet(void) const { return m_vertices; }
	const Edges& getEdges(void) const { return m_edges; }

private:
	void rebuildVerticesSet(void)
	{
		Vertices temp = std::move(m_vertices);

		for (const Edge& e : m_edges)
		{
			bool foundFirst = false;
			bool foundSecond = false;

			for (const Vertex v : m_vertices)
			{
				if (e.first == v)
					foundFirst = true;

				if (e.second == v)
					foundSecond = true;

				if (foundFirst && foundSecond)
					break;
			}

			if (!foundFirst)
				m_vertices.emplace_back(e.first);

			if (!foundSecond)
				m_vertices.emplace_back(e.second);
		}

		//Ajout des sommets qui ne sont reliés a aucune arrêtes
		for (const Vertex& v : temp)
		{
			bool canAdd = true;

			for (const Edge& e : m_edges)
			{
				if ((e.first == v) || (e.second == v))
				{
					canAdd = false;
					break;
				}
			}

			if (canAdd)
				m_vertices.emplace_back(v);
		}
	}

	unsigned int computeDegreeForVertex(const Vertex v) const
	{
		unsigned int degreeOfV = 0;
		for (const Edge& e : m_edges)
		{
			if ((e.first == v) || (e.second == v))
				++degreeOfV;
		}
		return degreeOfV;
	}

private:
	Edges		m_edges;//Les arrêtes
	Vertices	m_vertices;//Les sommets
};

std::ostream& operator<< (std::ostream& stream, const Vertex& v)
{
	stream << "(" << v->n << ", " << v->w << ")";
	return stream;
}

std::ostream& operator<< (std::ostream& stream, const Edge& edge)
{
	stream << edge.first << " -- " << edge.second;
	return stream;
}

std::ostream& operator<< (std::ostream& stream, const VertexSet& vs)
{
	std::cout << "{ ";
	for (size_t i = 0; i < vs.size(); ++i)
		std::cout << vs[i] << " ";
	std::cout << "}";

	return stream;
}

std::ostream& operator<< (std::ostream& stream, const VertexSets& VS)
{
	for (const VertexSet& vs : VS)
		stream << vs << std::endl;

	return stream;
}

std::ostream& operator<< (std::ostream& stream, const Graph& g)
{
	for (const Edge& e : g.getEdges())
		std::cout << e << std::endl;

	return stream;
}

#endif