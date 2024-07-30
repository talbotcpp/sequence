/*	Sequence.h
* 
*	Header file for general-purpose sequence container.
* 
*	Alan Talbot
*/

// sequence_lits - Literals used to specify the values of various sequence traits.
// These are hoisted out of the class template to avoid template dependencies.
// See sequence_traits below for a detailed discussion of these values.

enum class sequence_lits {
	FRONT, MIDDLE, BACK,			// See sequence_traits::location.
	LINEAR, EXPONENTIAL, VECTOR,	// See sequence_traits::growth.
};

// sequence_traits - Structure used to supply the sequence traits. The default values have been chosen
// to exactly model std::vector so that sequence can be used as a drop-in replacement with no adjustments.

template<std::unsigned_integral SIZE = size_t, size_t CAP = 0>
struct sequence_traits
{
	// 'size_type' is the type of the size field for local storage. This allows small sequences of
	// small types to be made meaningfully more space efficient by representing the size with a
	// smaller type than size_t. This size reduction may (or may not) also result in faster run times.
	// (Note that this type is not used in this structure. Using it for 'capacity' complicates
	// sequence_storage without offering any real benefits, and it's not correct for 'increment'
	// because the SBO (see below) may be small but the dynamic size large.)

	using size_type = SIZE;

	// 'dynamic' is true to allow storage to be dynamically allocated; false if storage must always be local.

	bool dynamic = true;

	// 'variable' is true if the capacity is permitted to grow; false if the capacity must remain fixed.
	// Local storage ('dynamic' == false) implies a fixed capacity, so in this case 'variable' must be false.
	// (Allowing it to be true would be harmless since the field would be ignored, but this would always be
	// a programming error or a misunderstanding).

	bool variable = true;

	// 'capacity' is size of a fixed capacity. If this field is non-zero for dynamic storage, then it is the
	// size of the small object optimization buffer (SBO). If it is zero, no small object optimization is used.

	size_t capacity = CAP;		

	// 'location' specifies how the data are managed within the capacity:
	//		FRONT:	Data starts at the lowest memory location. This makes push_back most efficient (like std::vector).
	//		MIDDLE:	Data floats in the middle of the capacity. This makes both push_back and push_front efficient (similar to std::deque).
	//		BACK:	Data ends at the highest memory location. This makes push_front most efficient.

	sequence_lits location = sequence_lits::FRONT;

	// 'growth' indicates how the capacity grows when growth is necessary:
	//		LINEAR:			Capacity grows by a fixed amount specified by 'increment'.
	//		EXPONENTIAL:	Capacity grows exponentially by a factor specified by 'factor'.
	//		VECTOR:			Capacity grows in the same way as std::vector. This behavior is implementation dependent.
	//						It is provided so that sequence can be used as an implementation of std::vector and/or a
	//						drop-in replacement for std::vector with no changes in behavior, even if the std::vector
	//						growth behavoir cannot be otherwise modeled with LINEAR or EXPONENTIAL growth modes.

	sequence_lits growth = sequence_lits::VECTOR;

	// 'increment' specifies the linear capacity growth in elements. This must be greater than 0.

	size_t increment = 1;

	// 'factor' specifies the exponential capacity growth factor. This must be greater than 1.0.
	// The minimum growth will be one element, regardless of how close to 1.0 this is set.

	float factor = 1.5;
};

// sequence_storage - Base class for sequence which provides the different memory allocation strategies.
// The DYN and VAR boolean parameters are sequence_traits::dynamic and sequence_traits::variable respectively.
// The SIZE type parameter is sequence_traits::size_type. The CAP unsigned parameter is sequence_traits::capacity.

// The primary template. This is never instantiated.

template<bool DYN, bool VAR, typename T, typename SIZE, size_t CAP>
class sequence_storage
{
public:
	constexpr static size_t capacity() { return 0; }
	size_t size() { return 0; }
};

// Dynamic, variable memory allocation with small object optimization. This is like boost::small_vector.

template<typename T, typename SIZE, size_t CAP>
class sequence_storage<true, true, T, SIZE, CAP>
{
public:
	constexpr static size_t capacity() { return 0; }
	size_t size() { return 0; }
};

// Dynamic, variable memory allocation. This is like std::vector.
// The SIZE and CAP parameters are ignored.

template<typename T, typename SIZE>
class sequence_storage<true, true, T, SIZE, size_t(0)>
{
public:
	constexpr static size_t capacity() { return 0; }
	size_t size() { return 0; }
};

// Dynamic, fixed memory allocation. This is like std::vector if you pre-reserve memory,
// but supports immovable objects. The SIZE parameter is ignored.

template<typename T, typename SIZE, size_t CAP>
class sequence_storage<true, false, T, SIZE, CAP>
{
	using value_type = T;
	using size_type = SIZE;

public:

	constexpr static size_t capacity() { return CAP; }
	size_t size() { return m_size; }

private:

	size_type m_size = 0;
};

// Local, fixed memory allocation. This is like std::inplace_vector (or boost::static_vector).

template<typename T, typename SIZE, size_t CAP>
class sequence_storage<false, false, T, SIZE, CAP>
{
	using value_type = T;
	using size_type = SIZE;

public:

	constexpr static size_t capacity() { return CAP; }
	size_t size() { return m_size; }

protected:

	value_type* capacity_start() const { return m_elements; }
	value_type* capacity_end() const { return m_elements + CAP; }

	void add_back(const value_type& e)
	{
		assert(size() < capacity());
		new(m_elements + m_size++) value_type;
	}
	void reallocate()
	{
		throw std::bad_alloc;
	}

private:

	size_type m_size = 0;
	union {
		value_type m_elements[CAP];
		unsigned char unused;
	};
};

// sequence_management - Base class for sequence which provides the different element management strategies.
// The LOC sequence_lits parameter specifies which element management strategy to use. The remaining parameters
// are passed on the sequence_storage base class.

// The primary template. This is never instantiated.

template<sequence_lits LOC, bool DYN, bool VAR, typename T, typename SIZE, size_t CAP>
class sequence_management
{
public:
	void id() const
	{
		std::println("sequence_management: primary template");
	}
};

// FRONT element location.

template<bool DYN, bool VAR, typename T, typename SIZE, size_t CAP>
class sequence_management<sequence_lits::FRONT, DYN, VAR, T, SIZE, CAP> : public sequence_storage<DYN, VAR, T, SIZE, CAP>
{
	using value_type = T;

public:
	void id() const
	{
		std::println("sequence_management: FRONT");
	}

	void push_front(const value_type& e)
	{

	}
	void push_back(const value_type& e)
	{
		if (size() == capacity());
		//	reallocate();
		add_back(e);
	}

protected:

	//value_type* data_start() const { return capacity_start(); }
	//value_type* data_end() const { return capacity_start() + size(); }
};

// MIDDLE element location.

template<bool DYN, bool VAR, typename T, typename SIZE, size_t CAP>
class sequence_management<sequence_lits::MIDDLE, DYN, VAR, T, SIZE, CAP> : public sequence_storage<DYN, VAR, T, SIZE, CAP>
{
public:
	void id() const
	{
		std::println("sequence_management: MIDDLE");
	}
};

// BACK element location.

template<bool DYN, bool VAR, typename T, typename SIZE, size_t CAP>
class sequence_management<sequence_lits::BACK, DYN, VAR, T, SIZE, CAP> : public sequence_storage<DYN, VAR, T, SIZE, CAP>
{
public:
	void id() const
	{
		std::println("sequence_management: BACK");
	}
};

// sequence - This is the main class template.

template<typename T, sequence_traits TRAITS = sequence_traits<size_t>()>
class sequence : public sequence_management<TRAITS.location, TRAITS.dynamic, TRAITS.variable,
											T, typename decltype(TRAITS)::size_type, TRAITS.capacity>
{
public:

	using value_type = T;

	~sequence()
	{
		//for (auto next(data_start()), end(data_end()); next != end; ++next)
		//	next->~value_type();
	}

	using traits_type = decltype(TRAITS);
	static constexpr traits_type traits = TRAITS;

	// This combination is meaningless. See comment in sequence_traits.
	static_assert(!(traits.dynamic == false && traits.variable == true),
				  "A sequence with local storage must have a fixed capacity.");

	// Variable capacity means that the capacity must grow, and this growth must actually make progress.
	static_assert(traits.increment > 0,
				  "Linear capacity growth must be greater than 0.");
	static_assert(traits.factor > 1.0f,
				  "Exponential capacity growth must be greater than 1.0.");

	// Maintaining elements in the middle of the capacity is more or less useless without the ability to shift.
	static_assert(traits.location != sequence_lits::MIDDLE || std::move_constructible<T>,
				  "Middle element location requires move-constructible types.");


private:


};

template<typename SEQ>
void show(const SEQ& seq)
{
	using traits_type = SEQ::traits_type;

	seq.id();

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
