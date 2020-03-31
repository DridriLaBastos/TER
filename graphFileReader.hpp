#ifndef GRAPH_FILE_READER_HPP
#define GRAPH_FILE_READER_HPP

#include <cctype>
#include <chrono>
#include <string>
#include <cstring>
#include <fstream>

#include "graph.hpp"

class GraphFileReader
{
	public:
		GraphFileReader(const std::string& path): m_stream(path)
		{
			if (!m_stream.is_open())
				throw std::logic_error("ERROR: cannot open '" + path + "'");
		}

		void readFile (std::pair<GraphVertices, Edges>& pair, VertexContainer& container)
		{
			pair.first.clear();   pair.second.clear();   container.clear();   m_vertexToGraphVertexPtr.clear();
			pair.first.reserve(1000000);   pair.second.reserve(1000000);
			container.resize(1000000);   m_vertexToGraphVertexPtr.resize(1000000);

			std::cout << "Begin reading... ";
			auto begin = std::chrono::system_clock::now();
			do
			{
				if ((char)m_stream.peek() == '%')
					passComment();
				else
					parseLine(pair,container);
				
			} while (!m_stream.eof());
			auto end = std::chrono::system_clock::now();

			std::cout << "took: " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "s" << std::endl;

//			pair.first.shrink_to_fit();
//			pair.second.shrink_to_fit();
			container.shrink_to_fit();
		}
	
	private:
		void passComment(void)
		{ m_stream.ignore(std::numeric_limits<std::streamsize>::max(),'\n'); };

		GraphVertex& findVertexAndEmplaceIfNot(const unsigned int vertexNumber, GraphVertices& vertices, VertexContainer& container)
		{
			//On utilise le numéro du sommet pour trouver sa place dans le graphe, du coup il faut être sûr que le container est assez grand pour contenir tous les sommets
			if (vertexNumber >= container.size())
			{
				container.resize(container.size() * 2);
				m_vertexToGraphVertexPtr.resize(container.size());
			}
			
			if (container[vertexNumber].get() == nullptr)
			{
				container[vertexNumber] = std::unique_ptr<VertexStruct>(new VertexStruct(vertexNumber));
				vertices.emplace_back(container[vertexNumber].get());
				m_vertexToGraphVertexPtr[vertexNumber] = &vertices.back();
			}

			return *m_vertexToGraphVertexPtr[vertexNumber];
		}

		void findEdgeFromVerticesAndEmplaceIfNot(GraphVertex& v1, GraphVertex& v2, Edges& edges) const
		{
			//Si une arête existe entre ces sommets, alors chacun des sommets à l'autre dans ses voisins. Pour vérifier
			//si une arrête existe ou pas, il suffit donc de chercher un des sommets dans la liste des voisins de l'autre.
			//Il n'y a pas besoin de tester les deux listes de voisins que si un sommet est dans les voisins d'un autre
			//les deux sont forcément voisin l'un de l'autre (voir makeEdge)
			//Wouah *_*
			auto found = std::find_if(v1.neighbors.begin(), v1.neighbors.end(),
				[&v2](const GraphVertex* vs)
				{ return vs->vertex == v2.vertex; });
			
			if (found == v1.neighbors.end())
				edges.emplace_back(makeEdge(v1,v2));
		}

		void parseLine(std::pair<GraphVertices,Edges>& pair, VertexContainer& container)
		{
			unsigned int n1 = 0;
			unsigned int n2 = 0;
			char line [127];

			//A priori le fichier n'est pas trié, il n'y a donc aucune garantie que le noeud lu n'ait pas
			//déjà été trouvé, il faut donc le rechercher et le créer s'il n'existe pas
			m_stream.getline(line,127);
			extractUInt(line,n1,n2);
			
			GraphVertex& v1 = findVertexAndEmplaceIfNot(n1,pair.first,container);
			GraphVertex& v2 = findVertexAndEmplaceIfNot(n2,pair.first,container);

			//Une fois que l'on a trouvé les vertex correspondant aux valeurs que l'on a lu du fichier,
			//il faut vérifier que l'arrête qu'ils forment n'existe pas déjà car rien ne garantit
			//qu'une arrête ne soit pas mise deux fois (par exemple (1,0) et (0,1) sont la même arrête et on
			//ne veut pas la mettre deux fois)
			findEdgeFromVerticesAndEmplaceIfNot(v1,v2,pair.second);
		}

	private:
		static void extractUInt(const char* str, unsigned int& n1, unsigned int& n2)
		{
			//exctraction du premier nombre
			while (std::isdigit(*str))
			{
				n1 *= 10;
				n1 += *str - '0';
				++str;
			}

			//On va jusqu'au deuxième
			while (!std::isdigit(*str))
				++str;
			
			//Extraction du deuxième nombre
			while (std::isdigit(*str))
			{
				n2 *= 10;
				n2 += *str - '0';
				++str;
			}
		}

	private:
		std::ifstream m_stream;
		std::vector<GraphVertex*> m_vertexToGraphVertexPtr;
};

#endif
