#ifndef GRAPH_FILE_READER_HPP
#define GRAPH_FILE_READER_HPP

#include <cctype>
#include <chrono>
#include <string>
#include <cstring>
#include <fstream>

#include "graph.hpp"

constexpr unsigned int LINE_BUFFER_SIZE = 127;

class GraphFileReader
{
	public:
		GraphFileReader(const std::string& path): m_stream(path), m_path(path)
		{
			if (!m_stream.is_open())
				throw std::logic_error("ERROR: cannot open '" + path + "'");
			
			std::memset((void*)m_lineBuffer,'\0',LINE_BUFFER_SIZE);
		}

		//TODO: faire une petite gestion des erreurs dans le parser
		std::pair<Vertices,Edges> readFile (VertexStructContainer& container)
		{
			std::pair<Vertices,Edges> ret;
			std::vector<Vertex*> vertexStructToVertexPtr;
			container.clear();
			ret.first.reserve(1000000);   ret.second.reserve(1000000);
			container.resize(1000000);
			vertexStructToVertexPtr.resize(container.size());

			std::cout << "Begin reading... ";
			auto begin = std::chrono::system_clock::now();

			passComment();
			readLine();

			if (isVertexInfoLine())
			{
				unsigned int vertexCount = 0;
				unsigned int weightCount = 1;
				extractVertexCountAndWeightCount(vertexCount,weightCount);
				parseVertices(ret.first,container,vertexCount,vertexStructToVertexPtr);
			}
			else
				parseEdge(ret,container,vertexStructToVertexPtr);

			do
			{
				readLine();
				if (*m_lineBufferPtr == '\0')
					break;

				if (!isCommentLine())
					parseEdge(ret,container,vertexStructToVertexPtr);
				
			} while (!m_stream.eof());
			auto end = std::chrono::system_clock::now();

			std::cout << "took: " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "s" << std::endl;

			ret.first.shrink_to_fit();
			ret.second.shrink_to_fit();
			container.shrink_to_fit();
			return ret;
		}
	
	private:
		bool isVertexInfoLine(void) const { return *m_lineBuffer == 'i'; }
		//Certains fichier utilise un 'n' pour enregistrer un sommet. Pour l'instant on ne lit pas les sommets de cette façon, mais directement depuis les arrêtes
		bool isCommentLine(void) const { return (*m_lineBuffer == '%') || (*m_lineBuffer == 'c') || (*m_lineBuffer == 'n'); }

		//retourne true si elle la fin du buffer n'a pas été rencontré
		bool passWhites(void)
		{
			while (!std::isdigit(*m_lineBufferPtr))
			{
				if ((*m_lineBufferPtr == '\0') || (m_lineBufferPtr - m_lineBuffer) > LINE_BUFFER_SIZE)
					return false;
				++m_lineBufferPtr;
			}

			return true;
		}

		void readLine(void)
		{
			m_stream.getline(m_lineBuffer,LINE_BUFFER_SIZE);
			m_lineBufferPtr = m_lineBuffer;
		}

		void passComment(void)
		{
			while (m_stream.peek() == '%')
				m_stream.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
		};

		void parseVertices (Vertices& vertices, VertexStructContainer& container, const unsigned int vertexCount, std::vector<Vertex*>& vertexStructToVertexPtr)
		{
			for (size_t i = 0; i < vertexCount; ++i)
			{
				readLine();
				findVertexAndEmplaceIfNot(i+1,vertices,container,vertexStructToVertexPtr, extractUInt());
			}
		}

		void extractVertexCountAndWeightCount(unsigned int& vertexCount, unsigned int& weightCount)
		{
			weightCount = 1;

			passWhites();
			vertexCount = extractUInt();
			return;

			if (passWhites())
				weightCount = extractUInt();
		}

		Vertex& findVertexAndEmplaceIfNot(const unsigned int vertexNumber, Vertices& vertices, VertexStructContainer& container, std::vector<Vertex*>& vertexStructToVertexPtr, Weight w = 1)
		{
			//On utilise le numéro du sommet pour trouver sa place dans le graphe, du coup il faut être sûr que le container est assez grand pour contenir tous les sommets
			if (vertexNumber >= container.size())
			{
				container.resize(container.size() + 100000);
				vertexStructToVertexPtr.resize(container.size());
			}
			
			if (container[vertexNumber].get() == nullptr)
			{
				container[vertexNumber] = std::unique_ptr<VertexStruct>(new VertexStruct(vertexNumber,w));
				vertices.emplace_back(container[vertexNumber].get());
				vertexStructToVertexPtr[vertexNumber] = &vertices.back();
			}

			return *vertexStructToVertexPtr[vertexNumber];
		}

		void findEdgeFromVerticesAndEmplaceIfNot(Vertex& v1, Vertex&v2, Edges& edges)
		{
			//Si une arête existe entre ces sommets, alors chacun des sommets à l'autre dans ses voisins. Pour vérifier
			//si une arrête existe ou pas, il suffit donc de chercher un des sommets dans la liste des voisins de l'autre.
			//Il n'y a pas besoin de tester les deux listes de voisins que si un sommet est dans les voisins d'un autre
			//les deux sont forcément voisin l'un de l'autre (voir makeEdge)
			//Wouah *_*
			auto found = std::find_if(v1.neighbors.begin(), v1.neighbors.end(),
				[&v2](const VertexStructPtr vs)
				{ return vs == v2; });
			
			if (found == v1.neighbors.end())
				edges.emplace_back(makeEdge(v1,v2));
		}

		void parseEdge(std::pair<Vertices,Edges>& pair, VertexStructContainer& container, std::vector<Vertex*>& vertexStructToVertexPtr)
		{
			//A priori le fichier n'est pas trié, il n'y a donc aucune garantie que le noeud lu n'ait pas
			//déjà été trouvé, il faut donc le rechercher et le créer s'il n'existe pas

			passWhites();
			const unsigned int n1 = extractUInt();

			passWhites();
			const unsigned int n2 = extractUInt();
			
			Vertex& v1 = findVertexAndEmplaceIfNot(n1,pair.first,container,vertexStructToVertexPtr);
			Vertex& v2 = findVertexAndEmplaceIfNot(n2,pair.first,container,vertexStructToVertexPtr);

			//Une fois que l'on a trouvé les vertex correspondant aux valeurs que l'on a lu du fichier,
			//il faut vérifier que l'arrête qu'ils forment n'existe pas déjà car rien ne garantit
			//qu'une arrête ne soit pas mise deux fois (par exemple (1,0) et (0,1) sont la même arrête et on
			//ne veut pas la mettre deux fois)
			findEdgeFromVerticesAndEmplaceIfNot(v1,v2,pair.second);
		}

		unsigned int extractUInt (void)
		{
			unsigned int ret = 0;

			while (std::isdigit(*m_lineBufferPtr))
			{
				ret *= 10;
				ret += *m_lineBufferPtr - '0';
				++m_lineBufferPtr;
			}

			return ret;
		}

	private:
		std::ifstream m_stream;
		std::string m_path;
		char m_lineBuffer [LINE_BUFFER_SIZE];
		const char* m_lineBufferPtr;
};

#endif