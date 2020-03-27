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
		const VertexSet& neighbors = vi->v->neighbors;

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
		Weight w_s = VertexSet(v->neighbors).weight() + v->w;

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

		const auto found = std::find_if(PI.set.begin(), PI.set.end(),
			[&](VertexSet& d)
			{
				/** test l'intersection entre les sommets déjà dans d et les voisins de v **/
				for (const Vertex& vertex : v->neighbors)
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
		if (found != PI.set.end())
		{
			Weights sum;
			found->emplace_back(v);
			std::for_each(PI.set.begin(), PI.set.end(), [&sum](const VertexSet& vs) { tryInsertAndRemoveDominated(vs.weight(),sum); });

			if (t <= sum)
			{
				found->pop_back();
				B.emplace_back(v);
			}
		}
		else
		{
			Weights sum = { v->w };
			std::for_each(PI.set.begin(), PI.set.end(), [&sum](const VertexSet& vs) { tryInsertAndRemoveDominated(vs.weight(),sum); });

			if (t <= sum)
				B.emplace_back(v);
			else
				PI.set.emplace_back(Vertices{ v });
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
		const VertexSet& neighbors = B[i]->neighbors;
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
		//on la sauvegarde pour l'utiliser. Il y a un bug ici si Cmax est vide mais ce n'est jamais le cas
		//tel qu'est écris l'algorithme
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
			InitReturnType ip = initialize(G[P], cliqueToImprove.weight() - vi->w);

			if ((ip.C0.weight() + vi->w) > cliqueToImprove.weight())
				cliqueToImprove = VertexSet::unionBetween(ip.C0, vi);

			Clique Cp = searchMaxWClique(ip.Gp, cliqueToImprove, { {vi} }, ip.O0);

			if (Cp.weight() > cliqueToImprove.weight())
				cliqueToImprove = Cp;
			
			//Maintenant que l'on a remplacé l'ancienne clique par une meilleure, on regarde si cette nouvelle clique
			//en domine d'autres pour les supprimer de l'ensemble des cliques max
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
	container.emplace_back(new VertexStruct(1,{10, 1}));
	container.emplace_back(new VertexStruct(2,{10, 1}));
	container.emplace_back(new VertexStruct(3,{1, 10}));
	container.emplace_back(new VertexStruct(4,{1, 15}));

	std::pair<Vertices, Edges> pair;
	pair.first.resize(container.size());

	for (size_t i = 0; i < container.size(); ++i)
		pair.first[i] = container[i].get();
	
	pair.second.emplace_back(makeEdge(pair.first[2],pair.first[0]));
	pair.second.emplace_back(makeEdge(pair.first[2],pair.first[1]));
	pair.second.emplace_back(makeEdge(pair.first[2],pair.first[3]));
	pair.second.emplace_back(makeEdge(pair.first[0],pair.first[1]));

	/*GraphFileReader reader (argv[1]);
	std::pair<Vertices, Edges> pair = reader.readFile(container);*/

	Cliques Cmax = WLMC(Graph(pair.first,pair.second));

	for (Clique& c: Cmax.set)
		std::sort(c.begin(),c.end(),[](const Vertex& a, const Vertex& b) { return a->n < b->n; });
	std::cout << Cmax << std::endl;
	//std::cout << "is clique..." << std::boolalpha << isClique(Cmax,pair.second) << std::endl;

	return EXIT_SUCCESS;
}