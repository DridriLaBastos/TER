#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <array>
#include <vector>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

#include "weight.hpp"

struct VertexStruct
{
	unsigned int n;//Vertex number
	Weight w;//Vertex weights

	std::vector<VertexStruct*> neighbors;

	VertexStruct(const unsigned int n, const Weight w) { this->n = n;   this->w = w; }
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

static bool tryInsertAndRemoveDominated(const Weight& w, Weights& weights)
{
	weights.emplace_back(w);
	
	for (size_t i = 0; i < weights.size()-1; ++i)
	{
		//Si le poids ajouté est plus petit qu'un autre poids, on supprime l'insertion et
		//l'algorithme s'arrête là
		if (w <= weights[i])
		{
			weights.pop_back();
			return false;
		}
		//Si le poids ajouté domine un autre poids, on supprime ce poids dominé en gardant le poids que l'on
		//vient d'ajouter à la fin du tableau
		else if (w > weights[i])
		{
			std::swap(weights.back(), weights[i]);
			weights.pop_back();
			std::swap(weights.back(), weights[i]);
			--i;
		}
	}

	//Si addVertexWeight vaut true, ça veut dire que l'on a pas ajouté le sommet la fonction renvoie donc
	//false, et inversement si addVertexWeight vaut true
	return  true;
}

using VertexDegreePairs = std::vector<VertexDegreePair>;

Weight& operator+= (Weight& w1, const Weight& w2)
{
	for (size_t i = 0; i < WEIGHTS_SIZE; ++i)
		w1[i] += w2[i];
	
	return w1;
}

Weight operator+ (const Weight& w1, const Weight& w2)
{
	Weight w (w1);
	w += w2;
	return w;
}

Weight& operator-= (Weight& w1, const Weight& w2)
{
	for (size_t i = 0; i < WEIGHTS_SIZE; ++i)
		w1[i] -= w2[i];
	
	return w1;
}

Weight operator- (const Weight& w1, const Weight& w2)
{
	Weight w (w1);
	w -= w2;
	return w;
}

Weights& operator+= (Weights& weights, const Weights& toAdd)
{
	if (weights.empty())
		weights = toAdd;
	else
	{
		Weights newWeights;
		newWeights.reserve(weights.size() * toAdd.size());
		std::swap(weights,newWeights);

		for (const Weight& w1: toAdd)
		{
			for (const Weight& w2: newWeights)
				weights.emplace_back(w1 + w2);
		}
	}
	return weights;
}

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

	//Retourne le poids total de l'ensemble de sommet
	Weight weight(void) const
	{
		Weight totalWeight;

		for (size_t i = 0; i < WEIGHTS_SIZE; ++i)
			totalWeight[i] = 0;

		for (const Vertex v : m_vertices)
			totalWeight += v->w;

		return totalWeight;
	}

	//Retourne le poids max
	Weights getMaxWeights(void) const
	{
		Weights WMax;

		for (const Vertex v : m_vertices)
			tryInsertAndRemoveDominated(v->w, WMax);
		
		return WMax;
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

struct VertexSets
{
	std::vector<VertexSet> set;

	Weights getWeights(void) const
	{
		Weights ret;
		
		for (const VertexSet& s: set)
			ret.emplace_back(s.weight());

		return ret;
	}

	void tryInsertAndRemoveDominated (const VertexSet& vs)
	{
		set.emplace_back(vs);

		for (size_t i = 0; i < set.size() - 1; ++i)
		{
			//Si le poids ajouté est plus petit qu'un autre poids, on supprime l'insertion et
			//l'algorithme s'arrête là
			if (vs.weight() <= set[i].weight())
			{
				set.pop_back();
				break;
			}
			//Si le poids ajouté domine un autre poids, on supprime ce poids dominé en gardant le poids que l'on
			//vient d'ajouter à la fin du tableau
			else if (vs.weight() > set[i].weight())
			{
				std::swap(set.back(), set[i]);
				set.pop_back();
				std::swap(set.back(), set[i]);
				--i;
			}
		}
	}
};

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

	VertexSet getNeighborsOf(const Vertex& vi) const
	{
		VertexSet neighbors;

		for (const Edge& e : m_edges)
		{
			if (e.first == vi)
				neighbors.emplace_back(e.second);
			else if (e.second == vi)
				neighbors.emplace_back(e.first);
		}

		return neighbors;
	}

	VertexDegreePairs computeDegrees(void) const
	{
		VertexDegreePairs ret;	ret.reserve(m_vertices.size());

		for (const Vertex& v: m_vertices)
			ret.emplace_back(v,computeDegreeForVertex(v));

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

			//TODO: regarder comment préserver l'ordre
			//TODO: est-ce vraiment nécessaire ? Le profiler windows n'affichait même pas l'utilisation de orderWith
			//On présèrve l'ordre des vertex
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
	stream << "(" << v->n << " " << v->w << ")";
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
	std::cout << "}   weight: " << vs.weight();

	return stream;
}

std::ostream& operator<< (std::ostream& stream, const VertexSets& vs)
{
	for (size_t i = 0; i < vs.set.size() - 1; ++i)
		stream << vs.set[i] << std::endl;

	stream << vs.set[vs.set.size() - 1];

	return stream;
}

std::ostream& operator<< (std::ostream& stream, const Graph& g)
{
	for (const Edge& e : g.getEdges())
		std::cout << e << std::endl;

	return stream;
}

#endif