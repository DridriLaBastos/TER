#include <random>
#include <vector>
#include <fstream>
#include <iostream>

struct EdgeVertex
{
	unsigned int vertexNumber;
	unsigned int edgesCount;
};

std::vector<EdgeVertex> edgeVertex;

unsigned int getEdgeableVertex (unsigned int vertexNumber)
{
	if (edgeVertex[vertexNumber].edgesCount == 0)
	{
		while (true)
		{
			if(++vertexNumber == edgeVertex.size())
				vertexNumber = 0;
			
			if (edgeVertex[vertexNumber].edgesCount != 0)
			{
				edgeVertex[vertexNumber].edgesCount -= 1;
				break;
			}
		}
	}

	return edgeVertex[vertexNumber].vertexNumber;
}

void genVertices (std::ofstream& stream, const unsigned int vertexNumber)
{
	for (size_t i = 0; i < vertexNumber; ++i)
		stream << 1 + (i%200) << std::endl;
}

void genEdges (std::ofstream& stream, const unsigned int vertexNumber, const unsigned int edgeNumber)
{
	std::random_device device;
	std::uniform_int_distribution<unsigned int> distr (0, vertexNumber-1);

	for (size_t i = 0; i < edgeNumber; ++i)
	{
		const unsigned int firstVertex = getEdgeableVertex(distr(device));
		const unsigned int secondVertex = getEdgeableVertex(distr(device));
		stream << firstVertex << " " << secondVertex << "\n";
	}
}

void genFile (std::ofstream& stream, const unsigned int vertexNumber, const unsigned int edgeNumber)
{
	stream << "%edges: " << edgeNumber << "\n";
	stream << "i " << vertexNumber << " w 1\n";
	genVertices(stream,vertexNumber);
	genEdges(stream,vertexNumber,edgeNumber);
}

unsigned int getUInt (const char* str)
{
	unsigned int result = 0;

	while (std::isdigit(*str))
	{
		result *= 10;
		result += *str - '0';
		++str;
	}

	if (*str != '\0')
		std::cerr << "WARNING: '" << *str << "' is not a digit, the rest of the line is not parsed" << std::endl;

	return result;
}

//Format: ggen <file path> <n vertices> <n edges>
int main (int argc, const char** argv)
{
	if (argc != 4)
	{
		std::cerr << "ERROR: arguments are <file parth> <n vertices> <n edges>" << std::endl;
		return EXIT_FAILURE;
	}

	std::ofstream file (argv[1], std::ios::out);

	const unsigned int vertexNumber = getUInt(argv[2]);
	unsigned int edgesNumber = getUInt(argv[3]);

	if (edgesNumber > vertexNumber*vertexNumber)
	{
		std::cout << "INFO: edge number too high, set back to " << (vertexNumber*vertexNumber) << std::endl;
		edgesNumber = vertexNumber*vertexNumber;
	}

	edgeVertex.resize(vertexNumber);

	for (size_t i = 0; i < vertexNumber; ++i)
	{
		edgeVertex[i].vertexNumber = i+1;
		edgeVertex[i].edgesCount = vertexNumber - 1;
	}

	genFile(file,vertexNumber,edgesNumber);

	return EXIT_SUCCESS;
}