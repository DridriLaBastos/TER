#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <vector>
#include <utility>
#include <iostream>
#include <algorithm>

struct Vertex
{
	unsigned int n;//Vertex number
	int w;//Vertex weight

	Vertex(const unsigned int n, const int w = 1) { this->n = n;   this->w = w; }
	Vertex(const Vertex& v) { n = v.n;   w = v.w; std::cout << "Vertex copy!\n"; }
	Vertex* operator-> (void) { return this; }
	const Vertex* operator->(void) const { return this; }
};
using Vertices = std::vector<Vertex>;
using VertexOrdering = Vertices;

using Edge = std::pair<Vertex, Vertex>;
using Edges = std::vector<Edge>;

Vertex makeVertex(const unsigned int n, const int w = 1) { return Vertex(n, w); }

class VertexSet;
using Clique = VertexSet;

struct VertexDegreePair
{
	Vertex v;
	unsigned int d;
	//v: Vertex   d: degree
	VertexDegreePair(const Vertex& ver, const unsigned int d) : v(ver) { this->d = d; }
};

using VertexDegreePairs = std::vector<VertexDegreePair>;

bool operator== (const Vertex& a, const Vertex& b) { return (a.n == b.n); }

//TODO: réécrire ces classes sans faire de copie des Vertex. L'ensemble des sommets sera référencé via 
//un std::vector<std::shared_ptr<Vertex>>
class VertexSet
{
public:
	VertexSet(void) {}
	VertexSet(const Vertices& vertices) : m_vertices(vertices) {}

public:
	void emplace_back(const Vertex& v) { m_vertices.emplace_back(v); }
	void pop_back(void) { m_vertices.pop_back(); }
	void reserve(const size_t n) { m_vertices.reserve(n); }

	const Vertices getVertices(void) const { return m_vertices; }

	Vertices::iterator begin(void) { return m_vertices.begin(); }
	Vertices::const_iterator begin(void) const { return m_vertices.begin(); }

	Vertices::iterator end(void) { return m_vertices.end(); }
	Vertices::const_iterator end(void) const { return m_vertices.end(); }

	int weight(void) const
	{
		int totalWeight = 0;

		for (const auto& v : m_vertices)
			totalWeight += v.w;

		return totalWeight;
	}

	int getMaxWeight(void) const
	{
		return std::max_element(m_vertices.begin(), m_vertices.end(),
			[](const Vertex& a, const Vertex& b) { return a.w <= b.w; })->w;
	}

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
		std::for_each(V.begin(), V.end(),
			[&](const Vertex& v)
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
			});
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
		std::for_each(V2.begin(), V2.end(),
			[&](const Vertex& v) { finalUnion = unionBetween(finalUnion, v); });

		return finalUnion;
	}

	static VertexSet intersectionBetween(const VertexSet& V1, const VertexSet& V2)
	{
		VertexSet finalIntersection;

		std::for_each(V1.begin(), V1.end(),
			[&](const Vertex& v)
			{
				for (size_t i = 0; i < V2.size(); ++i)
				{
					if (V2[i] == v)
						finalIntersection.emplace_back(v);
				}
			});

		return finalIntersection;
	}

private:
	Vertices m_vertices;
};
using VertexSets = std::vector<VertexSet>;

//TODO: storing vertex
class Graph
{
public:
	Graph(const Edges& edges = {}) : m_edges(edges) { rebuildVerticesSet(); }

public:
	Graph operator[](const VertexSet& V) const
	{
		Graph ret;
		ret.m_vertices = std::move(V.getVertices());
		Edges E;

		std::for_each(m_edges.begin(), m_edges.end(),
			[&](const Edge& e)
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
			});

		return ret;
	}

	VertexDegreePairs computeDegrees(void) const
	{
		return computeDegreesForVertices(m_vertices);
	}

	VertexDegreePairs computeDegreesForVertices(const Vertices& vertices) const
	{
		VertexDegreePairs ret;

		for (const Vertex v : vertices)
			ret.emplace_back(v, computeDegreeForVertex(v));

		return ret;
	}

	VertexSet getNeighborsOf(const Vertex& v) const
	{
		VertexSet neighbors;

		for (const Edge& e : m_edges)
		{
			if (e.first == v)
				neighbors.emplace_back(e.second);
			else if (e.second == v)
				neighbors.emplace_back(e.first);
		}

		return neighbors;
	}

	void removeVertex(const Vertex v)
	{
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
		m_vertices.clear();
		rebuildVerticesSet();
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
	stream << "(" << v.n << ", " << v.w << ")";
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