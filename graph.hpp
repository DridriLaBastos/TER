#ifndef GRAPH_HPP
#define GRAPH_HPP

//#define END

#include <vector>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

using Weight = int;

struct VertexStruct
{
	unsigned int n;//Vertex number
	Weight w;//Vertex weight

	VertexStruct(const unsigned int n, const Weight w = 1) { this->n = n;   this->w = w; }
	VertexStruct(const VertexStruct& vs) { n = vs.n; w = vs.w; std::cout << "VertexStruct copy!\n"; }
};

using VertexStructContainer = std::vector<std::unique_ptr<VertexStruct>>;
using VertexStructPtr = const VertexStruct*;
using VerticesStruct = std::vector<VertexStructPtr>;
using VertexOrdering = VerticesStruct;

struct Vertex
{
	VertexStructPtr vertex;
	VerticesStruct neighbors;
	Weight weight(void) const  { return vertex->w; }
	unsigned int num(void) const { return vertex->n; }
	Vertex(const VertexStructPtr v = nullptr, const VerticesStruct& n = {}): vertex(v), neighbors(n) {}
};

bool operator==(const Vertex& v1, const VertexStructPtr& v2) { return v1.vertex == v2; }
bool operator==(const Vertex& v1, const Vertex& v2) { return v1.vertex == v2.vertex; }

using VertexVector = std::vector<Vertex>;

//On veut le unique_ptr pour ne pas avoir de copy de VertexStruct si jamais VertexContainer.emplace_back doit allouer
//un nouveau tableau
//using Vertex = VertexStruct*;
//using Vertices = std::vector<Vertex>;

using Edge = std::pair<VertexStructPtr, VertexStructPtr>;
using Edges = std::vector<Edge>;

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
	//La référence ne peut pas être constante sinon l'appelle a std::swap ne marche pas dans Initialize
	const Vertex* v;
	unsigned int d;
	//v: Vertex   d: degree
	VertexDegreePair(const Vertex& ver, const unsigned int d) : v(&ver) { this->d = d; }
};

using VertexDegreePairs = std::vector<VertexDegreePair>;

class Vertices
{
public:
	Vertices(const VertexVector& vertices = {}) : m_vertices(vertices) {}
	Vertices(const VerticesStruct& vertices)
	{
		m_vertices.reserve(vertices.size());
		for (const VertexStruct* v: vertices)
			m_vertices.emplace_back(v);
	}

public:
	VertexVector::iterator begin(void) { return m_vertices.begin(); }
	VertexVector::const_iterator begin(void) const { return m_vertices.begin(); }

	VertexVector::iterator end(void) { return m_vertices.end(); }
	VertexVector::const_iterator end(void) const { return m_vertices.end(); }

	VertexVector::reference front(void) { return m_vertices.front(); }
	VertexVector::const_reference front(void) const { return m_vertices.front(); }

	VertexVector::reference back(void) { return m_vertices.back(); }
	VertexVector::const_reference back(void) const { return m_vertices.back(); }

	VertexVector::reference operator[](VertexVector::size_type pos) { return m_vertices[pos]; }
	VertexVector::const_reference operator[](VertexVector::size_type pos) const { return m_vertices[pos]; }

	VertexVector::size_type size(void) const noexcept { return m_vertices.size(); }

	bool empty(void) const noexcept { return m_vertices.empty(); }

	template<class... Args>
	void emplace_back(Args&&... args) { m_vertices.emplace_back(std::forward<Args>(args)...); }
	void reserve(VertexVector::size_type new_cap) { m_vertices.reserve(new_cap); }
	void pop_back(void) { m_vertices.pop_back(); }
	void shrink_to_fit(void) { m_vertices.shrink_to_fit(); }

public:
	Weight weight(void) const
	{
		Weight totalWeight = 0;

		for (const Vertex& v : m_vertices)
			totalWeight += v.weight();

		return totalWeight;
	}

	Weight getMaxWeight(void) const
	{
		Weight max = 0;

		for (const Vertex& v : m_vertices)
			max = (v.weight() > max) ? v.weight() : max;
		return max;
	}

	//TODO: améliorer le O(n^2)
	void orderWith(const VertexOrdering& O)
	{
		unsigned int OPos = 0;
		unsigned int VPos = 0;

		while (VPos < m_vertices.size() && OPos < O.size())
		{
			bool found = false;

			for (size_t j = VPos; j < m_vertices.size(); ++j)
			{
				if (m_vertices[j] == O[OPos])
				{
					std::swap(m_vertices[VPos++], m_vertices[j]);
					++OPos;
					found = true;
				}
			}

			//Si le sommet que l'on veut trier n'est pas présent dans le graphe, on passe au sommet suivant à trier
			if (!found) ++OPos;
		}
	}

	Vertices subSet(const size_t begin, const size_t end)
	{
		Vertices subset;

		if (begin <= end)
		{
			subset.reserve(end - begin + 1);//+1: index depuis 0

			for (size_t i = begin; i <= std::min(end,m_vertices.size()-1); ++i)
				subset.emplace_back(m_vertices[i]);
		}
		return subset;
	}

	void remove(const Vertices& V)
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

	static Vertices unionBetween(const Vertices& V1, const Vertex& v)
	{
		Vertices finalUnion(V1);

		auto found = std::find(finalUnion.begin(), finalUnion.end(), v);

		if (found == finalUnion.end())
			finalUnion.emplace_back(v);
		return finalUnion;
	}

	static Vertices unionBetween(const Vertices& V1, const Vertices& V2)
	{
		Vertices finalUnion(V1);
		for (const Vertex& v: V2)
			finalUnion = unionBetween(finalUnion, v);
		return finalUnion;
	}

	static Vertices intersectionBetween(const Vertices& V1, const Vertices& V2)
	{
		Vertices finalIntersection;
		for (const Vertex& v1: V1)
		{
			for (const Vertex& v2: V2)
			{
				if (v1 == v2)
					finalIntersection.emplace_back(v2);
			}
		}
		return finalIntersection;
	}

private:
	VertexVector m_vertices;
};

using VerticesVector = std::vector<Vertices>;
using Clique = Vertices;
using Cliques = std::vector<Clique>;
class Graph
{
public:
	Graph(const Vertices& vertices = {}, const Edges& edges = {}) : m_vertices(vertices), m_edges(edges) {}

public:
	Graph operator[](const Vertices& V) const
	{
		Graph ret;
		ret.m_vertices = V;

		for(const Edge& e: m_edges)
		{
			bool findFirst = false;
			bool findSecond = false;

			for (size_t i = 0; i < ret.m_vertices.size(); ++i)
			{
				size_t posFirst = 0;
				size_t posSecond = 0;

				if (ret.m_vertices[i] == e.first)
				{
					findFirst = true;
					posFirst = i;
				}
				else if (ret.m_vertices[i] == e.second)
				{
					posSecond = i;
					findSecond = true;
				}
			
				if (findFirst && findSecond)
				{
					ret.m_edges.emplace_back(makeEdge(ret.m_vertices[posFirst],ret.m_vertices[posSecond]));
					break;
				}
			}
		}

		return ret;
	}

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
			ret.emplace_back(v, v.neighbors.size());

		return ret;
	}

	void removeVertex(const Vertex& v)
	{
		//D'abord on cherche le sommet dans le graphe
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
			Vertex& vertexToRemove = m_vertices[vPosInVertices];
			Vertex saveOfLastVertex = m_vertices.back();

			//Puis on le supprime de tous ses voisins
			for (const VertexStructPtr& n: vertexToRemove.neighbors)
			{
				//On recherche le voisin dans l'ensemble des sommets
				//TODO: voir si cette version à besoin d'^etre optimisé
				for (Vertex& v: m_vertices)
				{
					bool removeHappened = false;
					if (v == n)
					{
						for (size_t i = 0; i < v.neighbors.size(); ++i)
						{
							if (v.neighbors[i] == vertexToRemove)
							{
								std::swap(v.neighbors[i],v.neighbors.back());
								v.neighbors.pop_back();
								removeHappened = true;
								break;
							}
						}
					}
					if (removeHappened)
						break;
				}
			}

			//Enfin on le supprime du graphe
			std::swap(m_vertices[vPosInVertices], m_vertices.back());
			m_vertices.pop_back();

			//Puis on supprime du graphe les arr^etes qui avaient ce sommet
			for (size_t i = 0; i < m_edges.size(); ++i)
			{
				Edge& currentEdge = m_edges[i];

				if ((currentEdge.first == v) || (currentEdge.second == v))
				{
					std::swap(m_edges[i], m_edges.back());
					m_edges.pop_back();
					--i;
				}
			}
		}
	}

	size_t size(void) const { return m_vertices.size(); }
	bool empty(void) const { return m_vertices.empty(); }

	const Vertices& getVertices(void) const { return m_vertices; }
	const Edges& getEdges(void) const { return m_edges; }

#ifdef END
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

#endif
private:
	Edges		m_edges;//Les arrêtes
	Vertices	m_vertices;//Les sommets
};

std::ostream& operator<< (std::ostream& stream, const Vertex& v)
{
	stream << "(" << v.num() << ", " << v.weight() << ")";
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

std::ostream& operator<< (std::ostream& stream, const std::vector<Vertices>& VS)
{
	for (const Vertices& vs : VS)
		stream << vs << std::endl;

	return stream;
}

#ifdef END
std::ostream& operator<< (std::ostream& stream, const Graph& g)
{
	for (const Edge& e : g.getEdges())
		std::cout << e << std::endl;

	return stream;
}
#endif

#endif