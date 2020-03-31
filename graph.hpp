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

	VertexStruct(const unsigned int n, const Weight w = 1) { this->n = n;   this->w = w; }
	VertexStruct(const VertexStruct& vs) { n = vs.n; w = vs.w; std::cout << "VertexStruct copy!\n"; }
};

using VertexContainer = std::vector<std::unique_ptr<VertexStruct>>;
using VertexStructPtr = const VertexStruct*;
using VerticesStruct = std::vector<VertexStructPtr>;
using VertexOrdering = VerticesStruct;

struct Vertex
{
	VertexStructPtr vertex;
	VerticesStruct neighbors;

	Vertex(VertexStructPtr v = nullptr, const VerticesStruct& n = {}): vertex(v), neighbors(n) {}
};

using VertexSet = std::vector<Vertex>;

//TODO: comme chaque GraphVertex stock ses voisins, l'information edge est redondante et je devrais peut être
//l'enlever
using Edge = std::pair<const VertexStruct*, const VertexStruct*>;
using Edges = std::vector<Edge>;

bool operator==(const Vertex& v1, const VertexStruct& v2) { return v1.vertex == v2; }
bool operator==(const Vertex& v1, const Vertex& v2) { return v1.vertex == v2.vertex; }

static void connect(Vertex& a, Vertex& b)
{
	auto found = std::find(a.neighbors.begin(), a.neighbors.end(), b.vertex);

	//On connect deux sommets que s'ils ne sont pas déjà connectés
	if (found == a.neighbors.end())
	{
		a.neighbors.emplace_back(b.vertex);
		b.neighbors.emplace_back(a.vertex);
	}
}

static Edge makeEdge(Vertex& a, Vertex& b)
{
	connect(a, b);
	return std::make_pair(a.vertex, b.vertex);
}

struct VertexDegreePair
{
	const Vertex* v;
	unsigned int d;
	//v: Vertex   d: degree
	VertexDegreePair(const Vertex* ver, const unsigned int d) : v(ver) { this->d = d; }
};

using VertexDegreePairs = std::vector<VertexDegreePair>;

class Vertices
{
	public:
		Vertices(const VertexSet& s = {}) : m_set(s) {}
		Vertices(const Vertex& v) { m_set.emplace_back(v); }
		Vertices(const VerticesStruct& vs)
		{
			m_set.reserve(vs.size());
			for (const VertexStruct* const v: vs)
				m_set.emplace_back(v);
		}
	
	public:
		/** Redéfinition des opérations de base pour les vector utiles pour les ensembles **/
		VertexSet::iterator begin(void) { return m_set.begin(); }
		VertexSet::const_iterator begin(void) const { return m_set.begin(); }

		VertexSet::iterator end(void) { return m_set.end(); }
		VertexSet::const_iterator end(void) const { return m_set.end(); }

		VertexSet::reference operator[](VertexSet::size_type pos) { return m_set[pos]; }
		VertexSet::const_reference operator[](VertexSet::size_type pos) const { return m_set[pos]; }

		VertexSet::reference front(void) { return m_set.front(); }
		VertexSet::const_reference front(void) const { return m_set.front(); }

		VertexSet::reference back(void) { return m_set.back(); }
		VertexSet::const_reference back(void) const { return m_set.back(); }

		VertexSet::size_type size (void) const noexcept { return m_set.size(); }

		template <class... Args>
		void emplace_back(Args&&... args) { m_set.emplace_back(std::forward<Args>(args)...); }
		void reserve (VertexSet::size_type newCapacity) { m_set.reserve(newCapacity); }
		void pop_back(void) { m_set.pop_back(); }
		void shrink_to_fit(void) { m_set.shrink_to_fit(); }

		bool empty(void) const noexcept { return m_set.empty(); }

		Weight weight(void) const
		{
			Weight totalWeight = 0;

			for (const Vertex& v : m_set)
				totalWeight += v.vertex->w;

			return totalWeight;
		}

	Weight getMaxWeight(void) const
	{
		Weight max = 0;

		for (const Vertex& v : m_set)
			max = (v.vertex->w > max) ? v.vertex->w : max;
		return max;
	}

	//TODO: améliorer le O(n^2)
	void orderWith(const VertexOrdering& O)
	{
		unsigned int vPos = 0;

		for (size_t i = 0; (i < O.size()) && (vPos < m_set.size()); ++i)
		{
			for (size_t j = vPos; j < m_set.size(); ++j)
			{
				if (m_set[j] == O[i])
					std::swap(m_set[vPos++], m_set[j]);
			}
		}
	}

	Vertices subSet(const size_t begin, const size_t end)
	{
		Vertices subset;
		if (begin <= end)
		{
			subset.m_set.reserve(end - begin + 1);//+1: index depuis 0

			for (size_t i = begin; i <= end; ++i)
				subset.m_set.emplace_back(m_set[i]);
		}
		return subset;
	}

	void remove(const Vertices& V)
	{
		for (const Vertex& v: V.m_set)
		{
			for (size_t i = 0; i < m_set.size(); ++i)
			{
				if (m_set[i] == v)
				{
					std::swap(m_set[i], m_set.back());
					m_set.pop_back();
					break;
				}
			}
		}
	}

	static Vertices unionBetween(const Vertices& V1, const Vertex& v)
	{
		Vertices finalUnion(V1);

		auto found = std::find(finalUnion.m_set.begin(), finalUnion.m_set.end(), v);

		if (found == finalUnion.m_set.end())
			finalUnion.m_set.emplace_back(v);
		return finalUnion;
	}

	static Vertices unionBetween(const Vertices& V1, const Vertices& V2)
	{
		Vertices finalUnion(V1);
		for (const Vertex& v: V2.m_set)
			finalUnion = unionBetween(finalUnion, v);
		return finalUnion;
	}

	static Vertices intersectionBetween(const Vertices& V1, const Vertices& V2)
	{
		VertexSet finalIntersection;
		for (const Vertex& v: V1.m_set)
		{
			for (size_t i = 0; i < V2.m_set.size(); ++i)
			{
				if (V2.m_set[i] == v)
					finalIntersection.emplace_back(v);
			}
		}
		return finalIntersection;
	}

	private:
		VertexSet m_set;
};

using VerticesSet = std::vector<Vertices>;
using Clique = Vertices;
using Cliques = VerticesSet;

class Graph
{
public:
	Graph(void) {}
	Graph(const Vertices& vertices, const Edges& edges) : m_vertices(vertices), m_edges(edges) {}

public:
	VertexSet::iterator begin(void) { return m_vertices.begin(); }
	VertexSet::const_iterator begin(void) const { return m_vertices.begin(); }

	VertexSet::iterator end(void) { return m_vertices.end(); }
	VertexSet::const_iterator end(void) const { return m_vertices.end(); }

	bool empty(void) const { return m_vertices.empty(); }

public:
	Graph operator[](const Vertices& V) const
	{
		Graph ret;   ret.m_vertices.reserve(V.size());

		for (const Vertex& v: V)
			ret.m_vertices.emplace_back(v);

		for(const Edge& e: m_edges)
		{
			bool findFirst = false;
			bool findSecond = false;

			for (size_t i = 0; i < V.size(); ++i)
			{
				size_t posFirst = 0;
				size_t posSecond = 0;

				if (V[i] == e.first)
				{
					posFirst = i;
					findFirst = true;
				}
			
				if (V[i] == e.second)
				{
					posSecond = i;
					findSecond = true;
				}
			
				if (findFirst && findSecond)
				{
					ret.m_edges.emplace_back(makeEdge(ret.m_vertices[posFirst], ret.m_vertices[posSecond]));
					break;
				}
			}
		}

		return ret;
	}

	size_t size(void) const { return m_vertices.size(); }

	//VertexSet getNeighborsOf(const Vertex& vi) const
	//{
	//	VertexSet neighbors;
//
	//	for (const Edge& e : m_edges)
	//	{
	//		if (e.first == vi)
	//			neighbors.emplace_back(e.second);
	//		else if (e.second == vi)
	//			neighbors.emplace_back(e.first);
	//	}
//
	//	return neighbors;
	//}

	VertexDegreePairs computeDegrees(void) const
	{
		VertexDegreePairs ret;	ret.reserve(m_vertices.size());
	
		for (const Vertex& v : m_vertices)
			ret.emplace_back(&v, v.neighbors.size());
	
		return ret;
	}

	//TODO: ne plus utiliser l'ensemble des arrêtes mais uniquement les voisins
	void removeVertex(const Vertex& v)
	{
		size_t vPosInVertices = 0;

		for (size_t i = 0; i < m_vertices.size(); ++i)
		{
			if (m_vertices[i].vertex == v)
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
			//for (size_t i = vPosInVertices; i < m_vertices.size() - 1; ++i)
			//{ m_vertices[i] = m_vertices[i+1]; }

			//m_vertices[m_vertices.size() - 1] = saveOfLastVertex;

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

	//size_t size(void) const { return m_vertices.size(); }
	//bool empty(void) const { return m_vertices.empty(); }

	//const Vertices& getVertices(void) const { return m_vertices; }
	const Vertices getVertices(void) const { return m_vertices; }
	const Edges& getEdges(void) const { return m_edges; }

private:
	//unsigned int computeDegreeForVertex(const Vertex v) const
	//{
	//	unsigned int degreeOfV = 0;
	//	for (const Edge& e : m_edges)
	//	{
	//		if ((e.first == v) || (e.second == v))
	//			++degreeOfV;
	//	}
	//	return degreeOfV;
	//}

private:
	Edges		m_edges;//Les arrêtes
	Vertices	m_vertices;//Les sommets
};

std::ostream& operator<< (std::ostream& stream, const Vertex& v)
{
	stream << "(" << v.vertex->n << ", " << v.vertex->w << ")";
	return stream;
}

std::ostream& operator<< (std::ostream& stream, const Edge& edge)
{
	stream << edge.first << " -- " << edge.second;
	return stream;
}

std::ostream& operator<< (std::ostream& stream, const Vertices& vs)
{
	std::cout << "{ ";
	for (size_t i = 0; i < vs.size(); ++i)
		std::cout << vs[i] << " ";
	std::cout << "}";

	return stream;
}

std::ostream& operator<< (std::ostream& stream, const VerticesSet& VS)
{
	for (const Vertices& vs : VS)
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