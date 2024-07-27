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
	using size_type = SIZE;		// The type of the size field.

	bool dynamic = true;		// True if storage could be dynamically allocated.
	bool variable = true;		// True if the capacity can grow.
	size_type capacity = CAP;	// The size of the fixed capacity (or the SBO).

	sequence_lits location = sequence_lits::FRONT;
	sequence_lits growth = sequence_lits::EXPONENTIAL;

	float factor = 1.5;			// The exponential growth factor (> 1.0).
	size_t increment = 0;		// The linear growth in elements.
								// Note: size_t (not size_type) since SBO may be small but the max size large.
};

template<typename T, sequence_traits TRAITS = sequence_traits<size_t>()>
class sequence
{
public:

	using traits_type = decltype(TRAITS);
	static constexpr traits_type traits = TRAITS;
};

template<typename SEQ>
void show(const SEQ& seq)
{
	using traits_type = SEQ::traits_type;

	std::println("Size Type:\t{}", typeid(traits_type::size_type).name());

	std::println("Dynamic:\t{}", seq.traits.dynamic ? "yes" : "no");
	std::println("Variable:\t{}", seq.traits.variable ? "yes" : "no");
	std::println("Capacity:\t{}", seq.traits.capacity);

	std::print("Location:\t");
	switch (seq.traits.location)
	{
		case sequence_lits::FRONT:			std::println("FRONT");	break;
		case sequence_lits::MIDDLE:			std::println("MIDDLE");	break;
		case sequence_lits::BACK:			std::println("BACK");	break;
	}
	std::print("Growth:\t\t");
	switch (seq.traits.growth)
	{
		case sequence_lits::LINEAR:			std::println("LINEAR");			break;
		case sequence_lits::EXPONENTIAL:	std::println("EXPONENTIAL");	break;
		case sequence_lits::VECTOR:			std::println("VECTOR");			break;
	}

	std::println("Increment:\t{}", seq.traits.increment);
	std::println("Factor:\t\t{}", seq.traits.factor);
}
