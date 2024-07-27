/*	Sequence.h
* 
*	Header file for general-purpose sequence container.
* 
*	Alan Talbot
*/

enum class sequence_lits {
	FRONT, MIDDLE, BACK,			// Data location: i.e. vector, deque, stack.
	LINEAR, EXPONENTIAL, VECTOR,	// Growth strategy (see below).
};

template<std::unsigned_integral SIZE = size_t, SIZE CAP = 0>
struct sequence_traits
{
	using size_type = SIZE;	// The type of the size field.

	bool dynamic = true;	// True if storage could be dynamically allocated.
	bool variable = true;	// True if the capacity can grow.
	size_type capacity = CAP;	// The size of the fixed capacity (or the SBO).

	sequence_lits location = sequence_lits::FRONT;
	sequence_lits growth = sequence_lits::EXPONENTIAL;

	size_t increment = 0;	// The linear growth in elements (size_t, not size_type)
	float factor = 1.5;		// The exponential growth factor (> 1.0).
};

template<typename T, sequence_traits TRAITS = sequence_traits<size_t>()>
class sequence
{
	using traits_type = decltype(TRAITS);
public:
	void show()
	{
		std::println("Size Type:\t{}", typeid(traits_type::size_type).name());
		std::println("Dynamic:\t{}", TRAITS.dynamic ? "yes" : "no");
		std::println("Variable:\t{}", TRAITS.variable ? "yes" : "no");
		std::println("Capacity:\t{}", TRAITS.capacity);

		std::print("Location:\t");
		switch (TRAITS.location)
		{
			case sequence_lits::FRONT:			std::println("FRONT");	break;
			case sequence_lits::MIDDLE:			std::println("MIDDLE");	break;
			case sequence_lits::BACK:			std::println("BACK");	break;
		}
		std::print("Growth:\t\t");
		switch (TRAITS.growth)
		{
			case sequence_lits::LINEAR:			std::println("LINEAR");			break;
			case sequence_lits::EXPONENTIAL:	std::println("EXPONENTIAL");	break;
			case sequence_lits::VECTOR:			std::println("VECTOR");			break;
		}
		std::println("Increment:\t{}", TRAITS.increment);
		std::println("Factor:\t\t{}", TRAITS.factor);
	}
};
