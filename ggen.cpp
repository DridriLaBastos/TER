#include <cctype>
#include <random>
#include <vector>
#include <fstream>
#include <iostream>

static unsigned int vertexNumber;
static unsigned int edgeNumber;
static unsigned int weightNumber;

std::vector<std::pair<unsigned int, std::vector<unsigned int>>> possibleEdges;

void genWeights(std::ofstream& stream, const unsigned int vertexID)
{
	std::random_device device;
	std::uniform_int_distribution<unsigned int> distr (vertexID%1000, 10000);

	//for (size_t i = 0; i < weightNumber; ++i)
	//{
	//	stream << distr(device);
	//	if (1 != weightNumber - 1)
	//		stream << " ";
	//}
	
	stream << 1 << "\n";
}

void genVertices (std::ofstream& stream)
{
	for (size_t i = 0; i < vertexNumber; ++i)
		genWeights(stream, i+1);
}

//unsigned int getEdgeableVertex (unsigned int vertexNumber)
//{
//	while (true)
//	{
//		if(vertexNumber == edgeVertex.size())
//			vertexNumber = 0;
//		
//		if (edgeVertex[vertexNumber].edgesCount != 0)
//		{
//			edgeVertex[vertexNumber].edgesCount -= 1;
//			break;
//		}
//		++vertexNumber;
//	}
//
//	return edgeVertex[vertexNumber].vertexNumber;
//}

void genEdges (std::ofstream& stream)
{
	std::random_device device;

	for (size_t i = 0; i < edgeNumber; ++i)
	{
		std::uniform_int_distribution<unsigned int> distr1 (0, possibleEdges.size()-1);
		const unsigned int firstVertex = distr1(device);

		std::uniform_int_distribution<unsigned int> distr2 (0, possibleEdges[firstVertex].second.size()-1);
		const unsigned int secondVertex = distr2(device);

		stream << (possibleEdges[firstVertex].first + 1) << " " << (possibleEdges[firstVertex].second[secondVertex] + 1) << std::endl;

		std::swap(possibleEdges[firstVertex].second[secondVertex],possibleEdges[firstVertex].second.back());
		possibleEdges[firstVertex].second.pop_back();

		if (possibleEdges[firstVertex].second.empty())
		{
			std::swap(possibleEdges[firstVertex], possibleEdges.back());
			possibleEdges.pop_back();
		}
	}
}

float computeGraphDensity(void)
{
	const float floatVertexNumber = vertexNumber;
	const float floatEdgeNumber = edgeNumber;
	//Simply applying the formula
	return (2.f*floatEdgeNumber) / (floatVertexNumber * floatVertexNumber - floatVertexNumber);
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
		std::cout << "WARNING: too many weights leads to the program taking more time to find the best cliques" << std::endl;

	if (edgeNumber > maximumNumberOfEdges)
	{
		std::cout << "INFO: edge number too high, set back to " << maximumNumberOfEdges << std::endl;
		edgeNumber = maximumNumberOfEdges;
	}

	std::cout << "INFO: w=" << weightNumber << "   |V|=" << vertexNumber << "   |E|=" << edgeNumber << std::endl;

	possibleEdges.resize(vertexNumber-1);

	for (size_t i = 0; i < vertexNumber-1; ++i)
	{
		possibleEdges[i].first = i;
		possibleEdges[i].second.resize(vertexNumber - (i + 1));
		
		for (size_t j = 0; j < possibleEdges[i].second.size(); ++j)
			possibleEdges[i].second[j] = j+i+1;
	}

	genFile(file);

	return EXIT_SUCCESS;
}