#ifndef WEIGHT_HPP
#define WEIGHT_HPP

#include <array>
#include <vector>

constexpr size_t WEIGHTS_SIZE = 1;

using Weight = std::array<int, WEIGHTS_SIZE>;
using Weights = std::vector<Weight>;

bool operator>(const Weight& w1, const Weight& w2)
{
	for (size_t i = 0; i < WEIGHTS_SIZE; ++i)
	{
		if (w1[i] <= w2[i])
			return false;
	}

	return true;
}

bool operator<= (const Weight& w1, const Weight& w2)
{
	for (size_t i = 0; i < WEIGHTS_SIZE; ++i)
	{
		if (w1[i] > w2[i])
			return false;
	}

	return true;
}

bool operator<= (const Weight& w, const Weights& weights)
{
	for (const Weight& W : weights)
	{
		if (!(w <= W))
			return false;
	}

	return true;
}

bool operator<= (const Weights& weights, const Weight& W)
{
	for (const Weight& w : weights)
	{
		if (!(w <= W))
			return false;
	}
	return true;
}

std::ostream& operator<< (std::ostream& stream, const Weight& w)
{
	stream << "{" << w[0];

	for (size_t i = 1; i < WEIGHTS_SIZE; ++i)
		stream << " " << w[i];

	stream << "}";
	return stream;
}

#endif