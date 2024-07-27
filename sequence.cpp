import std;

#include "Sequence.h"

using namespace std;

template<typename T>
using my_vector = sequence<T,
	sequence_traits<> {
		.dynamic = true,
		.variable = true,
		.location = sequence_lits::FRONT,
		.growth = sequence_lits::VECTOR,
	}>;

template<typename T, size_t S>
using my_inplace_vector = sequence<T,
	sequence_traits<size_t, S> {
		.dynamic = false,
		.variable = false,
	}>;


int main()
{
	sequence<int> s;
	show(s);
	println("");

	constexpr sequence_traits<unsigned char> st{
		.dynamic = false,
		.capacity = 128,
		.location = sequence_lits::MIDDLE,
	};

	sequence<int, st> s2;
	show(s2);
	println("");

	sequence<int,
		sequence_traits<unsigned short> {
			.dynamic = false,
			.variable = false,
			.capacity = 16,
			.location = sequence_lits::BACK,
		}> s3;
	show(s3);
	println("");

	sequence<int,
		sequence_traits<> {
			.location = sequence_lits::MIDDLE,
			.growth = sequence_lits::LINEAR,
			.increment = 256,
		}> s4;
	show(s4);
	println("");

	my_vector<int> s5;
	show(s5);
	println("");

	my_inplace_vector<int, 10> s6;
	show(s6);
	println("");
}
