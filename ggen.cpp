#include <random>
#include <vector>
#include <fstream>
#include <iostream>

static unsigned int vertexNumber;
static unsigned int edgeNumber;
static unsigned int weightNumber;

struct EdgeVertex
{
	unsigned int vertexNumber;
	unsigned int edgesCount;
};

std::vector<EdgeVertex> edgeVertex;

void genWeights(std::ofstream& stream, const unsigned int vertexID)
{
	std::random_device device;
	std::uniform_int_distribution<unsigned int> distr (vertexID%1000, 10000);

	for (size_t i = 0; i < weightNumber; ++i)
	{
		stream << distr(device);
		if (1 != weightNumber - 1)
			stream << " ";
	}
	
	stream << "\n";
}

void genVertices (std::ofstream& stream)
{
	for (size_t i = 0; i < vertexNumber; ++i)
		genWeights(stream, i+1);
}

unsigned int getEdgeableVertex (unsigned int vertexNumber)
{
	while (true)
	{
		if(vertexNumber == edgeVertex.size())
			vertexNumber = 0;
		
		if (edgeVertex[vertexNumber].edgesCount != 0)
		{
			edgeVertex[vertexNumber].edgesCount -= 1;
			break;
		}
		++vertexNumber;
	}

	return edgeVertex[vertexNumber].vertexNumber;
}

void genEdges (std::ofstream& stream)
{
	std::random_device device;
	std::uniform_int_distribution<unsigned int> distr (0, vertexNumber-1);

	for (size_t i = 0; i < edgeNumber; ++i)
	{
		//distr gen vertex number between 0 and |V|-1
		unsigned int firstVertex = getEdgeableVertex(distr(device));
		unsigned int secondVertex = getEdgeableVertex(distr(device));

		while (secondVertex == firstVertex)
			secondVertex = getEdgeableVertex(distr(device));
			
		stream << firstVertex << " " << secondVertex << "\n";
	}
}

float computeGraphDensity(void)
{
	const float floatVertexNumber = vertexNumber;
	const float floatEdgeNumber = edgeNumber;
	//Simply applying the formula
	return (2*floatVertexNumber) / (floatVertexNumber * floatVertexNumber - floatEdgeNumber);
}

void genFile (std::ofstream& stream)
{
	stream << "%edges: " << edgeNumber << " density: " << computeGraphDensity() << "\n";
	stream << "i " << vertexNumber << " w " << weightNumber << "\n";
	genVertices(stream);
	genEdges(stream);
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

//Format: ggen <file path> <weight per vertex> <n vertices> <n edges>
int main (int argc, const char** argv)
{
	if (argc != 5)
	{
		std::cerr << "ERROR: arguments are <file parth> <weight per vertex> <n vertices> <n edges>" << std::endl;
		return EXIT_FAILURE;
	}

	std::ofstream file (argv[1], std::ios::out);

	weightNumber = getUInt(argv[2]);
	vertexNumber = getUInt(argv[3]);
	edgeNumber = getUInt(argv[4]);


	const unsigned int maximumNumberOfEdges = (vertexNumber * (vertexNumber-1)) >> 1;

	if (weightNumber >= 5)
		std::cout << "WARNING: too many weights may lead to the program taking more time to complete" << std::endl;


	if (edgeNumber > maximumNumberOfEdges)
	{
		std::cout << "INFO: edge number too high, set back to " << maximumNumberOfEdges << std::endl;
		edgeNumber = maximumNumberOfEdges;
	}

	std::cout << "INFO: w=" << weightNumber << "   |V|=" << vertexNumber << "   |E|=" << edgeNumber << std::endl;
	edgeVertex.resize(vertexNumber);

	for (size_t i = 0; i < vertexNumber; ++i)
	{
		edgeVertex[i].vertexNumber = i+1;
		edgeVertex[i].edgesCount = vertexNumber - 1;
	}

	genFile(file);

	return EXIT_SUCCESS;
}