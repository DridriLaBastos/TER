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

		std::pair<Vertices,Edges> readFile (VertexContainer& container)
		{
			std::pair<Vertices,Edges> ret;
			ret.first.reserve(1000000);   ret.second.reserve(1000000);

			std::cout << "Begin reading... ";
			auto begin = std::chrono::system_clock::now();
			do
			{
				if ((char)m_stream.peek() == '%')
					passComment();
				else
					parseLine(ret,container);
				
			} while (!m_stream.eof());
			auto end = std::chrono::system_clock::now();

			std::cout << "took: " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "s" << std::endl;

			ret.first.shrink_to_fit();
			ret.second.shrink_to_fit();
			return ret;
		}
	
	private:
		void passComment(void)
		{ m_stream.ignore(std::numeric_limits<std::streamsize>::max(),'\n'); };

		Vertex findVertexAndEmplaceIfNot(const unsigned int vertexNumber, Vertices& vertices, VertexContainer& container) const
		{
			auto findResult = std::find_if(container.begin(), container.end(),
				[vertexNumber](const std::unique_ptr<VertexStruct>& vs) { return vs->n == vertexNumber; });

			const bool found = (findResult != container.end());

			if (!found)
			{
				container.emplace_back(new VertexStruct(vertexNumber));
				vertices.emplace_back(container.back().get());
			}
			
			return found ? findResult->get() : vertices.back();
		}

		void findEdgeFromVerticesAndEmplaceIfNot(const Vertex& v1, const Vertex&v2, Edges& edges) const
		{
			auto found = std::find_if(edges.begin(), edges.end(),
				[&v1,&v2](const Edge& e)
				{ return ((e.first == v1) && (e.second == v2)) || ((e.first == v2) && (e.second == v1)); });
			
			if (found == edges.end())
				edges.emplace_back(makeEdge(v1,v2));
		}

		void parseLine(std::pair<Vertices,Edges>& pair, VertexContainer& container)
		{
			unsigned int n1 = 0;
			unsigned int n2 = 0;

			//A priori le fichier n'est pas trié, il n'y a donc aucun garantie que le noeud que l'on lit n'est pas
			//déjà été trouvé, il faut donc le rechercher et le créer s'il n'existe pas
			m_stream >> n1 >> n2;

			const Vertex v1 = findVertexAndEmplaceIfNot(n1,pair.first,container);
			const Vertex v2 = findVertexAndEmplaceIfNot(n2,pair.first,container);

			//Une fois que l'on a trouvé les vertex correspondant aux valeurs que l'on a lu du fichier,
			//il faut vérifier que l'arrête qu'ils forment n'existe pas déjà car rien ne garantit
			//qu'une arrête ne soit pas mise deux fois (par exemple (1,0) et (0,1) sont la même arrête et on
			//ne veut pas la mettre deux fois)
			findEdgeFromVerticesAndEmplaceIfNot(v1,v2,pair.second);
		}

	private:
		std::ifstream m_stream;
};

#endif
