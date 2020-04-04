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
InitReturnType initialize(const Graph& G, Weight lb)
{
	VertexOrdering O0;	O0.reserve(G.size());
	Clique C0;
	Graph Gp(G);//Gp = G' (G prime)
	//Calcule le degré de chaque sommets. Contient aussi U qui est l'ensemble des sommets
	VertexDegreePairs degrees(G.computeDegrees());
	std::sort(degrees.begin(),degrees.end(),
		[](const VertexDegreePair& a, const VertexDegreePair& b) { return a.d > b.d; });

	for (size_t i = 0; i < G.size(); ++i)
	{
		const VertexDegreePair& vi = degrees.back();

		if (vi.d == degrees.size() - 1)
		{
			//Order U arbitrarily as vi, v_(i+1), v_(i+2)...
			std::sort(degrees.begin(), degrees.end(),
				[](const VertexDegreePair& a, const VertexDegreePair& b) { return a.v->num() < b.v->num(); });

			for (const auto& vdp : degrees)
			{
				//On ajoute tous les sommets dans O0
				O0.emplace_back(vdp.v->vertex);
				//Les sommets restant font partis de la clique initial C0
				C0.emplace_back(vdp.v->vertex);
			}
			break;
		}

		//U <- U\{vi} fait plus tard car on a besoin de vi ensuite

		//For each neighbors v of vi: deg(v) -= 1
		const std::vector<VertexStructPtr>& neighbors = vi.v->neighbors;

		for (const Vertex& neighbor: neighbors)
		{
			for (size_t i = 0; i < degrees.size(); ++i)
			{
				if (degrees[i].v->vertex == neighbor.vertex)
				{
					--degrees[i].d;

					for (size_t j = i; (j < degrees.size()-2) && (degrees[j+1].d > degrees[j].d); ++j)
						std::swap(degrees[j+1],degrees[j]);
					
					break;
				}
			}
		}

		//O0 est l'ensemble des sommets dans l'ordre avec lequel ils sont trouvés par cette fonction
		O0.emplace_back(vi.v->vertex);
		//U <- U\{vi} Cette ligne arrive à la fin car on a besoin de vi avant
		degrees.pop_back();
	}

	if (C0.weight() > lb)
		lb = C0.weight();

	for (const Vertex& v : G.getVertices())
	{
		Weight w_s = Vertices(v.neighbors).weight() + v.weight();

		if (w_s <= lb)
			Gp.removeVertex(v);
	}
	return { C0, O0, Gp };
}

//TODO: Eliminer les copies innutiles ?
Vertices getBranches(const Graph& G, const Weight t, const VertexOrdering& O)
{
	Vertices B;
	std::vector<Vertices> PI;
	Vertices V = G.getVertices();
	V.orderWith(O);
	for (size_t i = V.size() - 1; i < V.size(); --i)
	{
		const Vertex& v = V[i];

		const auto found = std::find_if(PI.begin(), PI.end(),
			[&](Vertices& d)
			{
				/** test l'intersection entre les sommets déjà dans d et les voisins de v **/
				for (const VertexStruct* vertex : v.neighbors)
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
			Weight sum = 0;
			found->emplace_back(v);
			std::for_each(PI.begin(), PI.end(), [&sum](const Vertices& vs) { sum += vs.getMaxWeight(); });

			if (sum > t)
			{
				found->pop_back();
				B.emplace_back(v);
			}
		}
		else
		{
			Weight sum = v.weight();
			std::for_each(PI.begin(), PI.end(), [&sum](const Vertices& vs) { sum += vs.getMaxWeight(); });

			if (sum <= t)
				PI.emplace_back(Vertices({v}));
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

	Vertices B = getBranches(G, Cmax.weight() - C.weight(), O);
	if (B.empty())
		return Cmax;

	Vertices A = G.getVertices();
	A.remove(B);
	B.orderWith(O);

	for (size_t i = B.size() - 1; i < B.size(); --i)
	{
		const Vertices& BSubset = B.subSet(i + 1, B.size() - 1);
		const Vertices& unionWithA = Vertices::unionBetween(A, BSubset);
		const Vertices& neighbors = B[i].neighbors;
		Vertices P(Vertices::intersectionBetween(neighbors, unionWithA));

		if (Vertices::unionBetween(C, B[i]).weight() + P.weight() > Cmax.weight())
		{
			Clique Cp = searchMaxWClique(G[P], Cmax, Vertices::unionBetween(C, B[i]), O);

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
	Vertices Vp = i.Gp.getVertices();
	Vp.orderWith(i.O0);

	for (size_t j = Vp.size() - 1; j < Vp.size(); --j)
	{
		const Vertex& vi = Vp[j];
		Vertices P = Vertices::intersectionBetween(vi.neighbors, Vp.subSet(j + 1, Vp.size() - 1));

		if ((P.weight() + vi.weight()) > Cmax.weight())
		{
			bool cliqueReplaced = false;
			InitReturnType ip = initialize(G[P], Cmax.weight() - vi.weight());

			if ((ip.C0.weight() + vi.weight()) > Cmax.weight())
			{
				Cmax = Vertices::unionBetween(ip.C0, vi);
				cliqueReplaced = true;
			}

			Clique Cp = searchMaxWClique(ip.Gp, Cmax, { {vi} }, ip.O0);

			if (Cp.weight() > Cmax.weight())
			{
				Cmax = Cp;
				cliqueReplaced = true;
			}
		}
	}
	const auto end = std::chrono::steady_clock::now();

	std::cout << "WLMC took: " << std::chrono::duration_cast<std::chrono::seconds>(end-start).count() << "s" << std::endl;
	return Cmax;
}

static bool isClique (const Clique& c, const Edges& edges)
{
	for (size_t i = 0; i < c.size(); ++i)
	{
		for (size_t j = i+1; j < c.size(); ++j)
		{
			bool found = false;

			for (const Edge& e: edges)
			{
				if (((e.first == c[i]) && (e.second == c[j])) || 
					((e.first == c[j]) && (e.second == c[i])))
				{
					found = true;
					break;
				}
			}

			if (!found) return false;
		}
	}

	return true;
}

static void signalHandler(const int sigNum)
{
	std::cerr << "Exiting with signal: " << sigNum << std::endl;
	exit(EXIT_FAILURE);
}

static void setup (void)
{
	//_MSC_VER est une macro contenant la version du compilateur msvc utilisé par windows. On utilise cette
	//macro pour savoir si on compile sous windows ou pas car signal n'existe pas sous windows
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
	VertexStructContainer container;

	GraphFileReader reader (argv[1]);
	std::pair<Vertices, Edges> pair = reader.readFile(container);

	Clique Cmax = WLMC(Graph(pair.first,pair.second));
	std::sort(Cmax.begin(),Cmax.end(),[](const Vertex& a, const Vertex& b) { return a.num() < b.num(); });
	std::cout << Cmax << " weight: " << Cmax.weight() << std::endl;
	std::cout << "is clique..." << std::boolalpha << isClique(Cmax,pair.second) << std::endl;

	return EXIT_SUCCESS;
}