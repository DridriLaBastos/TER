#ifndef GRAPH_HPP
#define GRAPH_HPP

//#define END

#include <vector>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>

#include "weight.hpp"

struct VertexStruct
{
	unsigned int n;//Vertex number
	Weight w;//Vertex weight

	VertexStruct(const unsigned int n, const Weight w) { this->n = n;   this->w = w; }
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
		Weight totalWeight;

		for (size_t i = 0; i < WEIGHTS_SIZE; ++i)
			totalWeight[i] = 0;

		for (const Vertex& v : m_vertices)
			totalWeight += v.weight();

		return totalWeight;
	}

	Weights getMaxWeights(void) const
	{
		Weights WMax;

		for (const Vertex v : m_vertices)
			tryInsertAndRemoveDominated(v.weight(), WMax);
		
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

	Vertices subSet(const size_t begin, const size_t end)
	{
		Vertices subset;
		if (begin <= end)
		{
			subset.reserve(end - begin + 1);//+1: index depuis 0

			for (size_t i = begin; i <= end; ++i)
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
	VertexVector m_vertices;
};
using VerticesVector = std::vector<Vertices>;

struct VerticesSet
{
	VerticesVector set;

	Weights getWeights(void) const
	{
		Weights ret;
		
		for (const Vertices& s: set)
			ret.emplace_back(s.weight());

		return ret;
	}

	void tryInsertAndRemoveDominated (const Vertices& vs)
	{
		bool insert = true;

		for (size_t i = 0; i < set.size() - 1; ++i)
		{
			//Si le poids ajouté est plus petit qu'un autre poids, on supprime l'insertion et
			//l'algorithme s'arrête là
			if (vs.weight() <= set[i].weight())
			{
				insert = false;
				break;
			}
			//Si le poids ajouté domine un autre poids, on supprime ce poids dominé en gardant le poids que l'on
			//vient d'ajouter à la fin du tableau
			else if (vs.weight() > set[i].weight())
			{
				std::swap(set.back(), set[i]);
				set.pop_back();
				--i;
			}
		}

		if (insert)
			set.emplace_back(vs);
	}
};

using Clique = Vertices;
using Cliques = VerticesSet;

class Graph
{
public:
	Graph(const Vertices& vertices = {}, const Edges& edges = {}) : m_vertices(vertices), m_edges(edges) {}

public:
	Graph operator[](const Vertices& V) const
	{
		Graph ret(V);
		ret.m_edges.reserve((V.size() * (V.size()-1))/2);

		//Dans le nouveau graphe, les sommets n'ont encore aucun voisin car on a pas ajouté les arrêtes. Il faut donc les supprimer
		for (Vertex& v : ret.m_vertices)
			v.neighbors.clear();

		for(const Edge& e: m_edges)
		{
			bool findFirst = false;
			bool findSecond = false;

			size_t posFirst = 0;
			size_t posSecond = 0;

			for (size_t i = 0; i < ret.m_vertices.size(); ++i)
			{
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
		ret.m_edges.shrink_to_fit();
		return ret;
	}

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

			//Puis on supprime du graphe les arrêtes qui avaient ce sommet
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
	{
		std::cout << vs[i];
		if (i < vs.size()-1)
			std::cout << " | ";
	}
	std::cout << "}   weight: " << vs.weight();

	return stream;
}

std::ostream& operator<< (std::ostream& stream, const VerticesSet& vs)
{
	for (size_t i = 0; i < vs.set.size() - 1; ++i)
		stream << vs.set[i] << std::endl;

	stream << vs.set[vs.set.size() - 1];

	return stream;
}

#endif