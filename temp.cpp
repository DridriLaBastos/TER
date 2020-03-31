#include "graphFileReader.hpp"

int main(int argc, const char** argv)
{
	if (argc != 2)
	{
		std::cerr << "arguments are <file path>\n";
		return EXIT_FAILURE;
	}

	VertexContainer container;
	GraphFileReader reader (argv[1]);
	std::pair<Vertices,Edges> pair = reader.readFile(container);

	return EXIT_SUCCESS;
}