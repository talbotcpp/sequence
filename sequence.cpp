import std;

#include "Sequence.h"

using namespace std;

template<typename T>
using my_vector = sequence<T,
	sequence_traits<> {
		.dynamic = true,
		.variable = true,
		.growth = sequence_lits::VECTOR,
	}>;

template<typename T, size_t S>
using my_inplace_vector = sequence<T,
	sequence_traits<size_t, S> {
		.dynamic = false,
		.variable = false,
	}>;

template<typename T, size_t S>
using my_small_vector = sequence<T,
	sequence_traits<size_t, S> {
		.dynamic = true,
		.variable = true,
	}>;

template<bool B>
struct thing {
};

struct base1 {
	void id() const
	{
		std::println("base1");
	}
};

struct base2 {
	void id() const
	{
		std::println("base2");
	}
};

template<>
struct thing<true> : base1 {
	void id() const
	{
		std::println("thing<true>");
		base1::id();
	}
};
template<>
struct thing<false> : base2 {
	void id() const
	{
		std::println("thing<false>");
		base2::id();
	}
};

int main()
{
	println("---- default --------------------------------");
	sequence<int> s;
	show(s);

	println("---- vector ---------------------------------");
	my_vector<int> s5;
	show(s5);

	println("---- inplace_vector -------------------------");
	my_inplace_vector<int, 10> s6;
	show(s6);

	println("---- small_vector ---------------------------");
	my_small_vector<int, 15> s7;
	show(s7);

	println("---- test -----------------------------------");
	sequence<int,
		sequence_traits<> {
			.dynamic = true,
			.variable = false,
			.capacity = 16,
			.location = sequence_lits::BACK,
		}> s3;
	show(s3);

	println("---------------------------------------------");

	thing<true> tt;
	tt.id();
	thing<false> tf;
	tf.id();
}




/*
	struct foo {
		char c;
	};
	struct bar {
		unsigned char size;
		foo data[15];
	};

	std::println("Size of foo:\t{}", sizeof(foo));
	std::println("Size of bar:\t{}", sizeof(bar));

	constexpr sequence_traits<unsigned char> st{
		.dynamic = false,
		.variable = false,
		.capacity = 128,
		.location = sequence_lits::MIDDLE,
	};

	println("---- default --------------------------------");
	sequence<int, st> s2;
	show(s2);
	println("--------------------------------");

	sequence<int,
		sequence_traits<unsigned short> {
			.dynamic = false,
			.variable = false,
			.capacity = 16,
			.location = sequence_lits::BACK,
		}> s3;
	show(s3);
	println("--------------------------------");

	sequence<int,
		sequence_traits<> {
			.location = sequence_lits::MIDDLE,
			.growth = sequence_lits::LINEAR,
			.increment = 256,
		}> s4;
	show(s4);
	println("--------------------------------");
*/