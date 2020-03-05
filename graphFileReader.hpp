#ifndef GRAPH_FILE_READER_HPP
#define GRAPH_FILE_READER_HPP

#include <string>
#include <fstream>

class GraphFileReader
{
	public:
		GraphFileReader(const std::string& path) : m_stream(path)
		{
			if (!m_stream.is_open())
				throw std::logic_error("ERROR: unable to open '" + path + "'");
		}

		void buildGraph(void)
		{
			// récupérer le format du fichier
			// parser le fichier
			// lire les données
			// retourner le Graph construit
		}

	private:
		std::ifstream m_stream;
};

#endif
