#include <array>
#include <chrono>
#include <fstream>
#include <signal.h>

#include "graph.hpp"
//#include "graphFileReader.hpp"

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
		const VertexDegreePair* vi = &degrees.back();

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
		const VertexSet& neighbors = G.getNeighborsOf(vi->v);

		for (const Vertex& neighbor: neighbors)
		{
			for (size_t i = 0; i < degrees.size(); ++i)
			{
				if (degrees[i].v == neighbor)
				{
					--degrees[i].d;

					for (size_t j = i; (j < degrees.size()-2) && (degrees[j+1].d > degrees[j].d); ++j)
						std::swap(degrees[j+1],degrees[j]);
					
					break;
				}
			}
		}

		//O0 est l'ensemble des sommets dans l'ordre avec lequel ils sont trouv�s par cette fonction
		O0.emplace_back(vi->v);
		//U <- U\{vi} Cette ligne arrive à la fin car on a besoin de vi avant
		degrees.pop_back();
	}

	if (C0.weight() > lb)
		lb = C0.weight();

	for (const Vertex& v : G.getVertices())
	{
		Weight w_s = VertexSet(G.getNeighborsOf(v)).weight() + v->w;

		if (w_s <= lb)
			Gp.removeVertex(v);
	}
	return { C0, O0, Gp };
}

VertexSet getBranches(const Graph& G, const Weight t, const VertexOrdering& O)
{
	VertexSet B;
	VertexSets PI;
	VertexSet V = G.getVertexSet();
	V.orderWith(O);
	for (size_t i = V.size() - 1; i < V.size(); --i)
	{
		const Vertex& v = V[i];

		VertexSet* found = nullptr;
		bool tryCreateNewIS = false;

		//Première partie de la condition: si il existe D dans PI qui n'a pas de voisin de v dedans
		for (VertexSet& s: PI.set)
		{
			found = &s;
			for (const Vertex n: G.getNeighborsOf(v))
			{
				for (const Vertex& testVertex: s.getVertices())
				{
					if (testVertex == n)
						found = nullptr;
				}

				if (found != nullptr)
					break;
			}

			if (found != nullptr)
				break;
		}

		//Si la première partie de la condition est vraie, on passe à la deuxième partie quiu vérifie que
		//la somme des poids max n'est pas supérieures à t une fois que l'on a ajouté v dans l'ensemble que
		//l'on a trouvé juste avant
		if (found != nullptr)
		{
			found->emplace_back(v);
			Weights sumMaxWeights;

			for (const VertexSet& vs : PI.set)
			{
				sumMaxWeights += vs.getMaxWeights();
				
				//S'il exite un poids qui domine t, alors la deuxième partie de la conition est fausse,
				//on passe donc au bloc else if qui va essayer de créer un nouvel ensemble indépendant avec
				//ce sommet
				if (!(t <= sumMaxWeights))
				{
					found->pop_back();
					tryCreateNewIS = true;
					break;
				}
			}

			//Si on arrive à la fin de la boucle, alors on peut ajouter v dans l'ensemble indépendant trouvé
			//dans found, mais comme cette action a été faite au début du bloc, l'algorithme ce termine ici pour ce sommet
		}

		if (tryCreateNewIS)
		{
			Weights sumMaxWeights = {v->w};
			bool shouldCreateNewIS = true;

			for (const VertexSet& vs: PI.set)
			{
				sumMaxWeights += vs.getMaxWeights();

				//S'il existe un poids qui domine t, alors on ne peut pas créer un nouvel ensemble indépendant,
				//on ajoute donc t à l'ensemble des sommets de branchements et l'algorithme se finit ici pour v
				if (!(t <= sumMaxWeights))
				{
					B.emplace_back(v);
					tryCreateNewIS = false;
					break;
				}
			}

			if (shouldCreateNewIS)
				PI.set.emplace_back(Vertices{v});

		}
	}

	B.orderWith(O);
	return B;
}

Cliques searchMaxWCliques(const Graph& G, const Clique Cmax, const Clique& C, const VertexOrdering& O)
{
	if (G.empty())
		return {{ C }};

	VertexSet B = getBranches(G, Cmax.weight() - C.weight(), O);
	if (B.empty())
		return {{ Cmax }};

	VertexSet A = G.getVertexSet();
	A.remove(B);
	B.orderWith(O);

	Cliques results {{ Cmax }};

	for (size_t i = B.size() - 1; i < B.size(); --i)
	{
		const VertexSet& BSubset = B.subSet(i + 1, B.size() - 1);
		const VertexSet& unionWithA = VertexSet::unionBetween(A, BSubset);
		const VertexSet& neighbors = G.getNeighborsOf(B[i]);
		VertexSet P(VertexSet::intersectionBetween(neighbors, unionWithA));

		if (!(VertexSet::unionBetween(C, B[i]).weight() + P.weight() <= Cmax.weight()))
		{
			Cliques Cp = searchMaxWCliques(G[P], Cmax, VertexSet::unionBetween(C, B[i]), O);

			for (const Clique& c: Cp.set)
				results.tryInsertAndRemoveDominated(c);
		}
	}

	return results;
}

Cliques WLMC(const Graph& G)
{
	const auto start = std::chrono::steady_clock::now();
	InitReturnType i = initialize(G, {});
	Cliques Cmax;   Cmax.set = { i.C0 };
	VertexSet Vp = i.Gp.getVertexSet();
	Vp.orderWith(i.O0);

	for (size_t j = Vp.size() - 1; j < Vp.size(); --j)
	{
		const Vertex& vi = Vp[j];
		VertexSet P = VertexSet::intersectionBetween(vi->neighbors, Vp.subSet(j + 1, Vp.size() - 1));

		bool cliqueToImproveFound = false;
		Clique cliqueToImprove;

		//On cherche une clique qui pourrait potentiellement être moins bonne, si on la trouve
		//on la sauvegarde pour l'utiliser.
		for (size_t i = 0; i < Cmax.set.size(); ++i)
		{
			if (!((P.weight() + vi->w) <= Cmax.set[i].weight()))
			{
				cliqueToImproveFound = true;
				cliqueToImprove = Cmax.set[i];
				break;
			}
		}

		//On entre ici s'il existe une clique à améliorer
		if (cliqueToImproveFound)
		{
			const Weight cliqueToImproveWeight = cliqueToImprove.weight();
			const Weight viWeight = vi->w;

			InitReturnType ip = initialize(G[P], cliqueToImproveWeight - viWeight);

			if (!((ip.C0.weight() + vi->w) <= cliqueToImprove.weight()))
				cliqueToImprove = VertexSet::unionBetween(ip.C0, vi);

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
						if (cliqueToImprove.weight() > Cmax.set[i].weight())
						{
							std::swap(Cmax.set[i],Cmax.set.back());
							Cmax.set.pop_back();
						}
					}

					Cmax.set.emplace_back(cliqueToImprove);
				}
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
	/*if (argc != 2)
	{
		std::cerr << "arguments are <file path>\n";
		return EXIT_FAILURE;
	}*/

	setup();
	VertexContainer container;
	std::unique_ptr<VertexStruct> v1 (new VertexStruct(1,{10,1}));
	std::unique_ptr<VertexStruct> v2 (new VertexStruct(2,{20,5}));
	std::unique_ptr<VertexStruct> v3 (new VertexStruct(3,{20,4}));
	std::unique_ptr<VertexStruct> v4 (new VertexStruct(4,{0,31}));
	std::unique_ptr<VertexStruct> v5 (new VertexStruct(5,{5,40}));

	std::unique_ptr<VertexStruct> v6 (new VertexStruct(6,{5,30}));
	std::unique_ptr<VertexStruct> v7 (new VertexStruct(7,{5,9}));
	std::unique_ptr<VertexStruct> v8 (new VertexStruct(8,{20,100}));
	std::unique_ptr<VertexStruct> v9 (new VertexStruct(9,{35,100}));
	std::unique_ptr<VertexStruct> v10 (new VertexStruct(10,{40,100}));

	std::pair<Vertices, Edges> pair;
	pair.first.emplace_back(v1.get());
	pair.first.emplace_back(v2.get());
	pair.first.emplace_back(v3.get());
	pair.first.emplace_back(v4.get());
	pair.first.emplace_back(v5.get());
	pair.first.emplace_back(v6.get());
	pair.first.emplace_back(v7.get());
	//pair.first.emplace_back(v8.get());
	//pair.first.emplace_back(v9.get());
	//pair.first.emplace_back(v10.get());
	
	pair.second.emplace_back(makeEdge(v1.get(),v2.get()));
	pair.second.emplace_back(makeEdge(v1.get(),v3.get()));
	pair.second.emplace_back(makeEdge(v1.get(),v5.get()));
	pair.second.emplace_back(makeEdge(v1.get(),v7.get()));
	pair.second.emplace_back(makeEdge(v2.get(),v3.get()));
	pair.second.emplace_back(makeEdge(v2.get(),v4.get()));
	pair.second.emplace_back(makeEdge(v3.get(),v4.get()));
	pair.second.emplace_back(makeEdge(v3.get(),v6.get()));
	pair.second.emplace_back(makeEdge(v5.get(),v6.get()));
	pair.second.emplace_back(makeEdge(v5.get(),v7.get()));
	//pair.second.emplace_back(makeEdge(v6.get(),v7.get()));
	//pair.second.emplace_back(makeEdge(v7.get(),v10.get()));
	//pair.second.emplace_back(makeEdge(v7.get(),v8.get()));
	//pair.second.emplace_back(makeEdge(v7.get(),v9.get()));
	//pair.second.emplace_back(makeEdge(v8.get(),v9.get()));
	//pair.second.emplace_back(makeEdge(v8.get(),v10.get()));
	//pair.second.emplace_back(makeEdge(v9.get(),v10.get()));

	/*GraphFileReader reader (argv[1]);
	std::pair<Vertices, Edges> pair = reader.readFile(container);*/

	Cliques Cmax = WLMC(Graph(pair.first,pair.second));

	for (Clique& c: Cmax.set)
		std::sort(c.begin(),c.end(),[](const Vertex& a, const Vertex& b) { return a->n < b->n; });
	std::cout << Cmax << std::endl;
	//std::cout << "is clique..." << std::boolalpha << isClique(Cmax,pair.second) << std::endl;

	return EXIT_SUCCESS;
}