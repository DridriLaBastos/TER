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

Vertices getBranches(const Graph& G, const Weight t, const VertexOrdering& O)
{
	Vertices B;
	VerticesSet PI;
	Vertices V = G.getVertices();
	V.orderWith(O);

	for (size_t i = V.size() - 1; i < V.size(); --i)
	{
		const Vertex& v = V[i];

		Vertices* found = nullptr;
		bool tryCreateNewIS = true;
		bool vertexShouldBeAddedToBranche = true;

		//Première partie de la condition: si il existe un ensemble D de PI qui n'a pas de voisin de v dedans
		for (Vertices& s: PI.set)
		{
			bool neighborFound = false;

			for (const Vertex& n: v.neighbors)
			{
				for (const Vertex& testVertex: s)
				{
					if (testVertex == n)
					{
						neighborFound = true;
						break;
					}
				}

				if (neighborFound)
					break;
			}

			if (!neighborFound)
			{
				found = &s;
				break;
			}
		}

		//Si la première partie de la condition est vraie, on passe à la deuxième partie qui vérifie que
		//la somme des poids max n'est pas supérieures à t une fois que l'on a ajouté v dans l'ensemble que
		//l'on a trouvé juste avant
		if (found != nullptr)
		{
			std::cout << "first\n";
			found->emplace_back(v);
			Weights sumMaxWeights;

			for (const Vertices& vs : PI.set)
				sumMaxWeights += vs.getMaxWeights();

			//S'il exite un poids qui domine t, alors la deuxième partie de la conition est fausse,
			//on passe donc au bloc else if qui va essayer de créer un nouvel ensemble indépendant avec
			//ce sommet
			if (!(sumMaxWeights <= t))
				found->pop_back();
			else
			{
				//Sinon on arrive à la fin de la boucle, alors on peut ajouter v dans l'ensemble indépendant trouvé
				//dans found, mais comme cette action a été faite au début du bloc, l'algorithme ce termine ici pour ce sommet
				tryCreateNewIS = false;
				vertexShouldBeAddedToBranche = false;
			}
		}

		if (tryCreateNewIS)
		{
			Weights sumMaxWeights = { v.weight() };
			bool shouldCreateNewIS = true;

			for (const Vertices& vs : PI.set)
				sumMaxWeights += vs.getMaxWeights();

			//S'il existe un des poids max pour lequel on ne peut pas dire qu'il est <= t, alors on ne peut pas créer un nouvel ensemble indépendant,
			//il faudra ajouter ce sommet à l'ensemble des sommets de branchements
			if (!(sumMaxWeights <= t))
				shouldCreateNewIS = false;

			if (shouldCreateNewIS)
			{
				std::cout << "second\n";
				vertexShouldBeAddedToBranche = false;
				PI.set.emplace_back(Vertices({ v }));
			}
		}

		if (vertexShouldBeAddedToBranche)
		{
			std::cout << "B" << std::endl;
			B.emplace_back(v);
		}
	}
	
	B.orderWith(O);
	return B;
}

Cliques searchMaxWCliques(const Graph& G, const Clique& Cmax, const Clique& C, const VertexOrdering& O)
{
	if (G.empty())
		return {{ C }};

	Vertices B = getBranches(G, Cmax.weight() - C.weight(), O);
	if (B.empty())
		return {{ Cmax }};

	Vertices A = G.getVertices();
	A.remove(B);
	B.orderWith(O);

	Cliques results {{ Cmax }};

	for (size_t i = B.size() - 1; i < B.size(); --i)
	{
		const Vertices& BSubset = B.subSet(i + 1, B.size() - 1);
		const Vertices& unionWithA = Vertices::unionBetween(A, BSubset);
		const Vertices& neighbors = B[i].neighbors;
		Vertices P(Vertices::intersectionBetween(neighbors, unionWithA));

		if (!(Vertices::unionBetween(C, B[i]).weight() + P.weight() <= Cmax.weight()))
		{
			Cliques Cp = searchMaxWCliques(G[P], Cmax, Vertices::unionBetween(C, B[i]), O);

			for (const Clique& c: Cp.set)
				results.tryInsertAndRemoveDominated(c);
		}
	}

	return results;
}

Cliques WLMC(const Graph& G, long long& duration)
{
	const auto start = std::chrono::steady_clock::now();
	InitReturnType i = initialize(G, {});
	Cliques Cmax;   Cmax.set = { i.C0 };
	Vertices Vp = i.Gp.getVertices();
	Vp.orderWith(i.O0);

	for (size_t j = Vp.size() - 1; j < Vp.size(); --j)
	{
		const Vertex& vi = Vp[j];
		Vertices P = Vertices::intersectionBetween(vi.neighbors, Vp.subSet(j + 1, Vp.size() - 1));

		bool cliqueToImproveFound = false;
		Clique cliqueToImprove;

		//Si le poid estimé d'une clique est dominé par une clique de l'ensemble de pareto, on passe directement
		//au sommet suivant. Dans le cas où cette condition n'arrive pas, on garde en mémoire la première clique
		//que l'on trouve potentiellement améliorable
		for (size_t i = 0; i < Cmax.set.size(); ++i)
		{
			//Le poids potentiel est dominé
			if (Cmax.set[i].weight() > (P.weight() + vi.weight()))
			{
				cliqueToImproveFound = false;
				break;
			}

			//On enregistre la première clique améliorable
			if (!((P.weight() + vi.weight()) <= Cmax.set[i].weight()))
			{
				if (!cliqueToImproveFound)
				{
					cliqueToImprove = Cmax.set[i];
					cliqueToImproveFound = true;
				}
			}
		}

		//On entre ici s'il existe une clique à améliorer
		if (cliqueToImproveFound)
		{
			const Weight cliqueToImproveWeight = cliqueToImprove.weight();
			const Weight viWeight = vi.weight();

			InitReturnType ip = initialize(G[P], cliqueToImproveWeight - viWeight);

			if (!((ip.C0.weight() + vi.weight()) <= cliqueToImprove.weight()))
				cliqueToImprove = Vertices::unionBetween(ip.C0, vi);

			Cliques Cp = searchMaxWCliques(ip.Gp, cliqueToImprove, { {vi} }, ip.O0);

			for (const Clique& c: Cp.set)
			{
				if (!(c.weight() <= cliqueToImprove.weight()))
					cliqueToImprove = c;
				
				//Maintenant que l'on a remplacé l'ancienne clique par une meilleure, on vérifie que cette clique n'est dominée par aucune autres cliques de l'ensemble. Si c'est le cas
				//(ou si la clique est de même poids), on l'oublie
				bool keepClique = true;
				for (const Clique& c : Cmax.set)
				{
					if (cliqueToImprove.weight() <= c.weight())
					{
						keepClique = false;
						break;
					}
				}

				//Sinon, on supprime toutes les cliques dominées par cette nouvelle clique, puis on l'ajoute à l'ensemble des solutions
				if (keepClique)
				{
					for (size_t i = 0; i < Cmax.set.size(); ++i)
					{
						if (Cmax.set[i].weight() <= cliqueToImprove.weight())
						{
							std::swap(Cmax.set[i],Cmax.set.back());
							Cmax.set.pop_back();
							--i;
						}
					}

					Cmax.set.emplace_back(cliqueToImprove);
				}
			}
		}
	}
	const auto end = std::chrono::steady_clock::now();

	duration = std::chrono::duration_cast<std::chrono::seconds>(end-start).count();
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
	signal(SIGTERM, signalHandler);
	signal(SIGSTOP, signalHandler);
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

	long long WLMCDuration = 0;
	VertexStructContainer container;
	GraphFileReader reader (argv[1]);
	std::pair<Vertices, Edges> pair = reader.readFile(container);

	Cliques Cmax = WLMC(Graph(pair.first,pair.second),WLMCDuration);

	for (Clique& c: Cmax.set)
		std::sort(c.begin(),c.end(),[](const Vertex& a, const Vertex& b) { return a.num() < b.num(); });
	std::cout << Cmax << std::endl;

	std::cout << "found: " << Cmax.set.size() << " cliques   took: " << WLMCDuration << "s" << std::endl;
	return EXIT_SUCCESS;
}