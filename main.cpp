#include "graphCopy.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <signal.h>

struct InitReturnType
{
	Clique C0;			//Initial clique
	VertexOrdering O0;	//Initial ordering
	Graph Gp;			//Reduced graph G'
};

/** TESTEE ET FONCTIONNE CORRECTEMENT **/
InitReturnType initialize(const Graph& G, int lb)
{
	PROFILE_FUNC();
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
	PROFILE_FUNC();
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
				/** test l'intersection entre les sommets d�j� dans d et les voisins de v **/
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
	PROFILE_FUNC();
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
	PROFILE_FUNC();
	const time_t begin = time(NULL);
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
	const time_t end = time(NULL);

	std::cout << "WLMC took: " << difftime(end, begin) << "s" << std::endl;

	return Cmax;
}

/**
 * \brief Lit le fichier fournit par path et construit l'ensemble de tous les sommets et des arr�tes
 *
 *
 * \param [in]	path Le chemin du fichier
 * \param [out]	Une r�f�rence sur une paire contenant un VertexContainer dans lequel tous les sommets du graph seront stock�s,
 * 				et un Edges dans lequel toutes les arr�tes du graph seront stock�es
 */
std::pair<Vertices, Edges> createEdgesFromEdgesFile(const std::string& path, VertexContainer& vertexContainer)
{
	PROFILE_FUNC();
	std::ifstream is(path);

	if (!is.is_open())
		throw std::logic_error("Unable to open \"" + path + "\"");

	VertexStruct nullVertex(0, 0);
	std::pair<Vertices, Edges> ret;

	unsigned int firstVertexNumber;		//< Le premier num�ro de sommet de la ligne lu du fichier
	unsigned int secondVertexNumber;	//< Le second num�ro de sommet de la ligne lu du fichier
	Vertex secondVertex = &nullVertex;

	time_t begin = time(NULL);
	do
	{
		is >> firstVertexNumber >> secondVertexNumber;
		//Le premier numéro de noeud lu dans un fichier '.edges' peut être n'importe quel numéro de noeud. Il faut donc repacourir
		//l'ensemble des noeuds déjà lu et voir s'il a déjà été trouvé.
		auto findResult = std::find_if(vertexContainer.begin(), vertexContainer.end(),
			[firstVertexNumber](const std::unique_ptr<VertexStruct>& vs) { return vs->n == firstVertexNumber; });

		bool found = (findResult != vertexContainer.end());

		if (!found)
		{
			vertexContainer.emplace_back(new VertexStruct(firstVertexNumber));
			ret.first.emplace_back(vertexContainer.back().get());
		}

		const Vertex firstVertex = found ? findResult->get() : ret.first.back();

		//Le deuxième noeud peut être plusieurs fois le même, on vérifie que c'est le même que le noeud précédent.
		//Si ce n'est pas le même, on vérifie qu'il n'a pas déjà été ajouté et dans le cas contraire, on l'ajoute
		if (secondVertexNumber != secondVertex->n)
		{
			findResult = std::find_if(vertexContainer.begin(), vertexContainer.end(),
				[secondVertexNumber](const std::unique_ptr<VertexStruct>& vs) { return vs->n == secondVertexNumber; });
			
			bool found = (findResult != vertexContainer.end());

			if (!found)
			{
				vertexContainer.emplace_back(new VertexStruct(secondVertexNumber));
				ret.first.emplace_back(vertexContainer.back().get());
			}

			secondVertex = found ? findResult->get() : ret.first.back();
		}

		//Il n'y a pas d'arrêtes duppliquées dans un fichier '.edge'. On peut donc directement la créer
		ret.second.emplace_back(makeEdge(firstVertex, secondVertex));

	} while (!is.eof());

	time_t end = time(NULL);

	std::cout << "Creating vertex set took: " << difftime(end, begin) << "s" << std::endl;
	return ret;
}

static void signalHandler(const int sigNum)
{
	PROFILE_FUNC();
	std::cerr << "Exiting with signal: " << sigNum << std::endl;
	END_SESSION();
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


void temp1()
{
	PROFILE_FUNC();
	std::cout << "Entering " << FUNC_NAME << std::endl;
	for (size_t i = 0; i < 10000000000; i += (i % 2 == 0) ? 2 : 5);
}

void temp2()
{
	PROFILE_FUNC();
	std::cout << "Entering " << FUNC_NAME << std::endl;
	for (size_t i = 0; i < 10000000000; i += (i % 2 == 0) ? 1 : 2);
}

void temp(void)
{
	PROFILE_FUNC();
	temp1();
	temp2();
}

int main(int argc, const char** argv)
{
	BEGIN_SESSION("WLMC Profile");
	if (argc != 2)
	{
		std::cerr << "arguments are <file path>\n";
		return EXIT_FAILURE;
	}

	setup();
	VertexContainer container;
	try
	{
		std::pair<Vertices, Edges> pair = createEdgesFromEdgesFile(argv[1], container);
		std::cout << WLMC(Graph(pair.first, pair.second)) << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	END_SESSION();
	return EXIT_SUCCESS;
}
//graphs/bio-dmela.clq.edges