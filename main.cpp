#include <array>
#include <chrono>
#include <fstream>
#include <signal.h>

#include "graph.hpp"
#include "graphFileReader.hpp"

struct InitReturnType
{
	Clique C0;			//Initial clique
	VertexOrdering O0;	//Initial ordering
	Graph Gp;			//Reduced graph G'
};

/** TESTEE ET FONCTIONNE CORRECTEMENT **/
InitReturnType initialize(const Graph& G, int lb)
{
	VertexOrdering O0;	O0.reserve(G.size());
	Clique C0;
	Graph Gp(G);//Gp = G' (G prime)
	//Calcule le degré de chaque sommets. Contient aussi U qui est l'ensemble des sommets
	VertexDegreePairs degrees(G.computeDegrees());

	for (size_t i = 0; i < G.size(); ++i)
	{
		auto vi = std::min_element(degrees.begin(), degrees.end(),
			[](const VertexDegreePair& a, const VertexDegreePair& b) { return a.d < b.d; });

		if (vi->d == degrees.size() - 1)
		{
			//Order U arbitrarily as vi, v_(i+1), v_(i+2)...
			std::sort(degrees.begin(), degrees.end(),
				[](const VertexDegreePair& a, const VertexDegreePair& b) { return a.v->n < b.v->n; });

			for (const auto& vdp : degrees)
			{
				//On ajoute tous les sommets dans O0
				O0.emplace_back(vdp.v);
				//Les sommets restant font partis de la clique initial C0
				C0.emplace_back(vdp.v);
			}
			break;
		}

		//U <- U\{vi} fait plus tard car on a besoin de vi ensuite

		//For each neighbors v of vi: deg(v) -= 1
		const VertexSet& neighbors = vi->v->neightbors;

		std::for_each(neighbors.begin(), neighbors.end(),
			[&](const Vertex& neighbor)
			{
				for (size_t i = 0; i < degrees.size(); ++i)
					degrees[i].d -= (degrees[i].v == neighbor) ? 1 : 0;
			});

		//O0 est l'ensemble des sommets dans l'ordre avec lequel ils sont trouv�s par cette fonction
		O0.emplace_back(vi->v);
		//U <- U\{vi} Cette ligne arrive � la fin car on a besoin de vi avant
		std::swap(*vi, degrees.back());
		degrees.pop_back();
	}

	if (C0.weight() > lb)
		lb = C0.weight();

	for (const Vertex& v : G.getVertices())
	{
		int w_s = VertexSet(v->neightbors).weight() + v->w;

		if (w_s <= lb)
			Gp.removeVertex(v);
	}
	return { C0, O0, Gp };
}

//TODO: Eliminer les copies innutiles ?
VertexSet getBranches(const Graph& G, const int t, const VertexOrdering& O)
{
	VertexSet B;
	VertexSets PI;
	VertexSet V = G.getVertexSet();
	V.orderWith(O);
	for (size_t i = V.size() - 1; i < V.size(); --i)
	{
		const Vertex& v = V[i];

		const auto found = std::find_if(PI.begin(), PI.end(),
			[&](VertexSet& d)
			{
				/** test l'intersection entre les sommets déjà dans d et les voisins de v **/
				for (const Vertex& vertex : v->neightbors)
				{
					for (size_t i = 0; i < d.size(); ++i)
					{
						if (d[i] == vertex)
							return false;
					}
				}

				return true;
			});

		/** Si l'intersection est vide on test le poids **/
		if (found != PI.end())
		{
			int sum = 0;
			found->emplace_back(v);
			std::for_each(PI.begin(), PI.end(), [&sum](const VertexSet& vs) { sum += vs.getMaxWeight(); });

			if (sum > t)
			{
				found->pop_back();
				B.emplace_back(v);
			}
		}
		else
		{
			int sum = v->w;
			std::for_each(PI.begin(), PI.end(), [&sum](const VertexSet& vs) { sum += vs.getMaxWeight(); });

			if (sum <= t)
				PI.emplace_back(Vertices{ v });
			else
				B.emplace_back(v);
		}
	}

	B.orderWith(O);
	return B;
}

Clique searchMaxWClique(const Graph& G, Clique Cmax, const Clique& C, const VertexOrdering& O)
{
	if (G.empty())
		return C;

	VertexSet B = getBranches(G, Cmax.weight() - C.weight(), O);
	if (B.empty())
		return Cmax;

	VertexSet A = G.getVertexSet();
	A.remove(B);
	B.orderWith(O);

	for (size_t i = B.size() - 1; i < B.size(); --i)
	{
		const VertexSet& BSubset = B.subSet(i + 1, B.size() - 1);
		const VertexSet& unionWithA = VertexSet::unionBetween(A, BSubset);
		const VertexSet& neighbors = B[i]->neightbors;
		VertexSet P(VertexSet::intersectionBetween(neighbors, unionWithA));

		if (VertexSet::unionBetween(C, B[i]).weight() + P.weight() > Cmax.weight())
		{
			Clique Cp = searchMaxWClique(G[P], Cmax, VertexSet::unionBetween(C, B[i]), O);

			if (Cp.weight() > Cmax.weight())
				Cmax = Cp;
		}
	}
	return Cmax;
}

Clique WLMC(const Graph& G)
{
	const auto start = std::chrono::steady_clock::now();
	InitReturnType i = initialize(G, 0);
	Clique Cmax = i.C0;
	VertexSet Vp = i.Gp.getVertexSet();
	Vp.orderWith(i.O0);

	for (size_t j = Vp.size() - 1; j < Vp.size(); --j)
	{
		const Vertex& vi = Vp[j];
		VertexSet P = VertexSet::intersectionBetween(vi->neightbors, Vp.subSet(j + 1, Vp.size() - 1));

		if ((P.weight() + vi->w) > Cmax.weight())
		{
			InitReturnType ip = initialize(G[P], Cmax.weight() - vi->w);

			if ((ip.C0.weight() + vi->w) > Cmax.weight())
				Cmax = VertexSet::unionBetween(ip.C0, vi);

			Clique Cp = searchMaxWClique(ip.Gp, Cmax, { {vi} }, ip.O0);

			if (Cp.weight() > Cmax.weight())
				Cmax = Cp;
		}
	}
	const auto end = std::chrono::steady_clock::now();

	std::cout << "WLMC took: " << std::chrono::duration_cast<std::chrono::seconds>(end-start).count() << "s" << std::endl;
	return Cmax;
}

static void signalHandler(const int sigNum)
{
	std::cerr << "Exiting with signal: " << sigNum << std::endl;
	exit(EXIT_FAILURE);
}

static void setup (void)
{
#if !defined(_MSC_VER)
	signal(SIGKILL, signalHandler);
	signal(SIGQUIT, signalHandler);
	signal(SIGINT, signalHandler);
#endif
}

int main(int argc, const char** argv)
{
	if (argc != 2)
	{
		std::cerr << "arguments are <file path>\n";
		return EXIT_FAILURE;
	}

	setup();
	VertexContainer container;

	GraphFileReader reader (argv[1]);
	//reader.readFile();

	std::pair<Vertices, Edges> pair = reader.readFile(container);
	/*VertexStruct v1 (1);
	VertexStruct v2 (2);
	VertexStruct v3 (3);
	VertexStruct v4 (4);
	VertexStruct v5 (5,1000);
	VertexStruct v6 (6);
	VertexStruct v7 (4);
	VertexStruct v8 (8);
	VertexStruct v9 (9,100);

	Edges e ({makeEdge(&v1,&v2),   makeEdge(&v1,&v3),
			  makeEdge(&v2,&v3),   makeEdge(&v2,&v4),   makeEdge(&v2,&v6),   makeEdge(&v2,&v7),
			  makeEdge(&v3,&v4),   makeEdge(&v3,&v6),
			  makeEdge(&v4,&v5),   makeEdge(&v4,&v6),
			  makeEdge(&v5,&v6),   makeEdge(&v5,&v9),
			  makeEdge(&v6,&v7),   makeEdge(&v7,&v8),   makeEdge(&v8,&v9)});*/

	Clique Cmax = WLMC(Graph(pair.first,pair.second));
	std::sort(Cmax.begin(),Cmax.end(),[](const Vertex& a, const Vertex& b) { return a->n < b->n; });
	std::cout << Cmax << std::endl;

	return EXIT_SUCCESS;
}