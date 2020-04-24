#ifndef WEIGHT_HPP
#define WEIGHT_HPP

#include <array>
#include <vector>
#include <iostream>

constexpr size_t WEIGHTS_SIZE = 3;

//TODO: peut être passer ça en std::vector ?
using Weight = std::array<float, WEIGHTS_SIZE>;
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
		if (w <= W)
			return true;
	}

	return false;
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

Weight& operator+= (Weight& w1, const Weight& w2)
{
	for (size_t i = 0; i < WEIGHTS_SIZE; ++i)
		w1[i] += w2[i];
	
	return w1;
}

Weight operator+ (const Weight& w1, const Weight& w2)
{
	Weight w (w1);
	w += w2;
	return w;
}

Weight& operator-= (Weight& w1, const Weight& w2)
{
	for (size_t i = 0; i < WEIGHTS_SIZE; ++i)
		w1[i] -= w2[i];
	
	return w1;
}

Weight operator- (const Weight& w1, const Weight& w2)
{
	Weight w (w1);
	w -= w2;
	return w;
}

Weights& operator+= (Weights& weights, const Weights& toAdd)
{
	if (weights.empty())
		weights = toAdd;
	else
	{
		Weights newWeights;
		newWeights.reserve(weights.size() * toAdd.size());
		std::swap(weights,newWeights);

		for (const Weight& w1: toAdd)
		{
			for (const Weight& w2: newWeights)
				weights.emplace_back(w1 + w2);
		}
	}
	return weights;
}

Weights operator- (Weights W, const Weight& w)
{
	for (Weight& wp: W)
		wp -= w;
	
	return W;
}

std::ostream& operator<< (std::ostream& stream, const Weight& w)
{
	stream << "{" << w[0];

	for (size_t i = 1; i < WEIGHTS_SIZE; ++i)
		stream << ", " << w[i];
	
	stream << "}";
	return stream;
}

#endif