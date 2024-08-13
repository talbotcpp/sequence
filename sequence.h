/*	Sequence.h
* 
*	Header file for general-purpose sequence container.
* 
*	Alan Talbot
*/

#include <assert.h>

// sequence_lits - Literals used to specify the values of various sequence traits.
// These are hoisted out of the class template to avoid template dependencies.
// See sequence_traits below for a detailed discussion of these values.

enum class sequence_lits {
	LOCAL, FIXED, VARIABLE,			// See sequence_traits::storage.
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

	// 'storage' specifies how the capacity is allocated:
	//		LOCAL:		The capacity is embedded in the sequence object (like std::inplace_vector). The capacity cannot change.
	//		FIXED:		The capacity is always dynamically allocated (on the heap). The capacity cannot change.
	//		VARIABLE:	The capacity may be embedded or dynamically allocated (on the heap), and the capacity can change (like std::vector).

	sequence_lits storage = sequence_lits::VARIABLE;

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

	// 'capacity' is the size of the fixed or local (SBO) capacity. For storages LOCAL and FIXED, it must be non-zero.
	// For storage VARIABLE: if 'capacity' is zero then the capacity is always dynamically allocated; if 'capacity' is
	// non-zero then it is the size of the small object optimization buffer (SBO).

	size_t capacity = CAP;

	// 'increment' specifies the linear capacity growth in elements. This must be greater than 0.

	size_t increment = 1;

	// 'factor' specifies the exponential capacity growth factor. This must be greater than 1.0.
	// The minimum growth will be one element, regardless of how close to 1.0 this is set.

	float factor = 1.5;
};

/*
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
// The CAP parameter is ignored. The SIZE parameter is used only
// for the offset value when the management mode is MIDDLE.

template<typename T, typename SIZE>
class sequence_storage<true, true, T, SIZE, size_t(0)>
{
	using value_type = T;
	using storage_type = sequence_storage_type<T, 1>;

public:

	void resize(size_t new_size)
	{}
	void reserve(size_t new_capacity)
	{}
	void shrink_to_fit()
	{}

	constexpr static size_t capacity() { return 0; }
	size_t size() { return 0; }

protected:

	value_type* capacity_start() { return m_cap_begin; }
	value_type* capacity_end() { return m_cap_end; }
	const value_type* capacity_start() const { return m_cap_begin; }
	const value_type* capacity_end() const { return m_cap_end; }

	void add(value_type* p, const value_type& e)
	{
		assert(size() < capacity());
		new(p) value_type(e);
///		++m_size;
	}
	void reallocate()
	{
		throw std::bad_alloc();
	}

private:

	storage_type* m_cap_begin = nullptr;	// The start of the capacity.
	storage_type* m_data_edge = nullptr;	// The start of the data for BACK, otherwise the end+1 of the data.
	storage_type* m_cap_end = nullptr;		// The end+1 of the capacity.
};

// Dynamic, fixed memory allocation. This is like std::vector if you pre-reserve memory,
// and never allow it to grow past the reserve, but it supports immovable objects.

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
	size_t size() const { return m_size; }

protected:

	value_type* capacity_start() { return m_storage.elements; }
	value_type* capacity_end() { return m_storage.elements + CAP; }
	const value_type* capacity_start() const { return m_storage.elements; }
	const value_type* capacity_end() const { return m_storage.elements + CAP; }

	void add(value_type* p, const value_type& e)
	{
		assert(size() < capacity());
		new(p) value_type(e);
		++m_size;
	}
	void shift(value_type* beg, value_type* end, ptrdiff_t dist)
	{
		if (dist > 0)
		{
			assert(beg >= capacity_start());
			assert(end + dist <= capacity_end());
			for (auto dst = end + dist; end > beg; --dst)
			{
				new(dst) value_type(*--end);
///				*end = 99999;	// !!!
				end->~value_type();
			}
			return;
		}
		if (dist < 0)
		{
			assert(beg - dist >= capacity_start());
			assert(end <= capacity_end());
			for (auto dst = beg + dist; beg < end; ++dst, ++beg)
			{
				new(dst) value_type(*beg);
///				*beg = 99999;	// !!!
				beg->~value_type();
			}
		}
	}
	void reallocate()
	{
		throw std::bad_alloc();
	}

private:

	sequence_storage_type<T, CAP> m_storage;
	size_type m_size = 0;
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

*/

// The shift_... functions move sequential elements in memory by the requested amount.
// Forward is toward the end (increasing memory). Reverse is toward the beginning (decreasing memory).
// The preconditions are:
//		end is not prior to begin
//		distance is non-zero (this algorithm is destructive for zero distance)
//		distance does not result in overflowing the array

template<typename T>
void shift_forward(T* begin, T* end, size_t distance)
{
	assert(distance != 0);

	for (auto dest = end + distance; end > begin;)
	{
		new(--dest) T(*--end);
		*end = 99999;	// !!!
		end->~T();
	}
}

template<typename T>
void shift_reverse(T* begin, T* end, size_t distance)
{
	assert(distance != 0);

	for (auto dest = begin - distance; begin < end; ++dest, ++begin)
	{
		new(dest) T(*begin);
		*begin = 99999;	// !!!
		begin->~T();
	}
}


// sequence_storage_type - This union template provides uninitialized storage for any capacity.
// The specialization for capacity == 1 supports dynamically allocated arrays for variable capacity.
// Note that it does not provide storage for the container size--that is the responsibility of
// the sequence_storage template specializations.

template<typename T, size_t CAP> requires (CAP != 0)
union sequence_storage_type {
	sequence_storage_type() {}
	T elements[CAP];
	unsigned char unused;
};

template<typename T>
union sequence_storage_type<T, 1> {
	sequence_storage_type() {}
	T element;
	unsigned char unused;
};


// sequence_implementation - This is the main class template.

template<sequence_lits STO, sequence_lits LOC, typename T, typename SIZE, size_t CAP>
class sequence_implementation
{
	static_assert(false, "An unimplemented specialization of sequence_implementation was instantiated.");
};

// LOCAL storage specializations. These are like std::inplace_vector (or boost::static_vector).

template<typename T, typename SIZE, size_t CAP>
class sequence_implementation<sequence_lits::LOCAL, sequence_lits::FRONT, T, SIZE, CAP>
{
public:

	using value_type = T;
	using size_type = SIZE;

	constexpr static size_t capacity() { return CAP; }
	size_t size() const { return m_size; }

protected:

	value_type* capacity_begin() { return m_storage.elements; }
	value_type* capacity_end() { return m_storage.elements + CAP; }
	const value_type* capacity_begin() const { return m_storage.elements; }
	const value_type* capacity_end() const { return m_storage.elements + CAP; }
	value_type* data_begin() { return capacity_begin(); }
	value_type* data_end() { return capacity_begin() + m_size; }
	const value_type* data_begin() const { return capacity_begin(); }
	const value_type* data_end() const { return capacity_begin() + m_size; }

	void add_front(const value_type& e)
	{
		assert(size() < capacity());
		assert(data_begin() >= capacity_begin());
		assert(data_end() < capacity_end());

		shift_forward(data_begin(), data_end(), 1);
		new(data_begin()) value_type(e);
		++m_size;
	}
	void add_back(const value_type& e)
	{
		assert(size() < capacity());

		new(data_end()) value_type(e);
		++m_size;
	}
	void reallocate()
	{
		throw std::bad_alloc();
	}

private:

	sequence_storage_type<T, CAP> m_storage;
	size_type m_size = 0;
};

template<typename T, typename SIZE, size_t CAP>
class sequence_implementation<sequence_lits::LOCAL, sequence_lits::MIDDLE, T, SIZE, CAP>
{
public:

	using value_type = T;
	using size_type = SIZE;

	constexpr static size_t capacity() { return CAP; }
	size_t size() const { return capacity() - (m_front_gap + m_back_gap); }

protected:

	value_type* capacity_begin() { return m_storage.elements; }
	value_type* capacity_end() { return m_storage.elements + CAP; }
	const value_type* capacity_begin() const { return m_storage.elements; }
	const value_type* capacity_end() const { return m_storage.elements + CAP; }
	value_type* data_begin() { return capacity_begin() + m_front_gap; }
	value_type* data_end() { return capacity_end() - m_back_gap; }
	const value_type* data_begin() const { return capacity_begin() + m_front_gap; }
	const value_type* data_end() const { return capacity_end() - m_back_gap; }

	void add_front(const value_type& e)
	{
		assert(size() < capacity());
		assert(m_front_gap != 0 || m_back_gap != 0);

		if (m_front_gap == 0)
		{
			auto offset = m_back_gap;
			m_back_gap /= 2u;
			offset -= m_back_gap;
			shift_forward(data_begin(), data_end(), offset);
			m_front_gap = offset - 1;
		}
		else --m_front_gap;
		new(data_begin()) value_type(e);
	}
	void add_back(const value_type& e)
	{
		assert(size() < capacity());
		assert(m_front_gap != 0 || m_back_gap != 0);

		if (m_back_gap == 0)
		{
			auto offset = m_front_gap;
			m_front_gap /= 2u;
			offset -= m_front_gap;
			shift_reverse(data_begin(), data_end(), offset);
			m_back_gap = offset - 1;
		}
		else --m_back_gap;
		new(data_end() - 1) value_type(e);
	}
	void reallocate()
	{
		throw std::bad_alloc();
	}

private:

	sequence_storage_type<T, CAP> m_storage;
	size_type m_front_gap = CAP / 2;
	size_type m_back_gap = CAP - CAP / 2;
};

template<typename T, typename SIZE, size_t CAP>
class sequence_implementation<sequence_lits::LOCAL, sequence_lits::BACK, T, SIZE, CAP>
{
public:

	using value_type = T;
	using size_type = SIZE;

	constexpr static size_t capacity() { return CAP; }
	size_t size() const { return m_size; }

protected:

	value_type* capacity_begin() { return m_storage.elements; }
	value_type* capacity_end() { return m_storage.elements + CAP; }
	const value_type* capacity_begin() const { return m_storage.elements; }
	const value_type* capacity_end() const { return m_storage.elements + CAP; }
	value_type* data_begin() { return capacity_end() - m_size; }
	value_type* data_end() { return capacity_end(); }
	const value_type* data_begin() const { return capacity_end() - m_size; }
	const value_type* data_end() const { return capacity_end(); }

	void add_front(const value_type& e)
	{
		assert(size() < capacity());

		new(data_begin() - 1) value_type(e);
		++m_size;
	}
	void add_back(const value_type& e)
	{
		assert(size() < capacity());
		assert(data_begin() > capacity_begin());
		assert(data_end() <= capacity_end());

		shift_reverse(data_begin(), data_end(), 1);
		new(data_end() - 1) value_type(e);
		++m_size;
	}
	void reallocate()
	{
		throw std::bad_alloc();
	}

private:

	sequence_storage_type<T, CAP> m_storage;
	size_type m_size = 0;
};

// FIXED storage specializations.

template<typename T, typename SIZE, size_t CAP>
class sequence_implementation<sequence_lits::FIXED, sequence_lits::FRONT, T, SIZE, CAP>
{
};

template<typename T, typename SIZE, size_t CAP>
class sequence_implementation<sequence_lits::FIXED, sequence_lits::MIDDLE, T, SIZE, CAP>
{
};

template<typename T, typename SIZE, size_t CAP>
class sequence_implementation<sequence_lits::FIXED, sequence_lits::BACK, T, SIZE, CAP>
{
};

// VARIABLE storage specializations supporting a small object buffer optimization.

template<typename T, typename SIZE, size_t CAP>
class sequence_implementation<sequence_lits::VARIABLE, sequence_lits::FRONT, T, SIZE, CAP>
{
};

template<typename T, typename SIZE, size_t CAP>
class sequence_implementation<sequence_lits::VARIABLE, sequence_lits::MIDDLE, T, SIZE, CAP>
{
};

template<typename T, typename SIZE, size_t CAP>
class sequence_implementation<sequence_lits::VARIABLE, sequence_lits::BACK, T, SIZE, CAP>
{
};

// VARIABLE storage specializations with no small object buffer optimization (like std::vector).

template<typename T, typename SIZE>
class sequence_implementation<sequence_lits::VARIABLE, sequence_lits::FRONT, T, SIZE, 0>
{
};

template<typename T, typename SIZE>
class sequence_implementation<sequence_lits::VARIABLE, sequence_lits::MIDDLE, T, SIZE, 0>
{
};

template<typename T, typename SIZE>
class sequence_implementation<sequence_lits::VARIABLE, sequence_lits::BACK, T, SIZE, 0>
{
};

// sequence - This is the main class template.

template<typename T, sequence_traits TRAITS = sequence_traits<size_t>()>
class sequence : public sequence_implementation<TRAITS.storage, TRAITS.location, T, typename decltype(TRAITS)::size_type, TRAITS.capacity>
{
	using inherited = sequence_implementation<TRAITS.storage, TRAITS.location, T, typename decltype(TRAITS)::size_type, TRAITS.capacity>;
	using inherited::capacity_begin;
	using inherited::capacity_end;
	using inherited::data_begin;
	using inherited::data_end;
	using inherited::add_front;
	using inherited::add_back;

public:

	using value_type = T;
	using inherited::size;
	using inherited::capacity;
	using inherited::reallocate;

	~sequence()
	{
		for (auto next(data_begin()), end(data_end()); next != end; ++next)
		{
			*next = 99999;	// !!!
			next->~value_type();
		}
	}

	using traits_type = decltype(TRAITS);
	static constexpr traits_type traits = TRAITS;

	// Variable capacity means that the capacity must grow, and this growth must actually make progress.
	static_assert(traits.increment > 0,
				  "Linear capacity growth must be greater than 0.");
	static_assert(traits.factor > 1.0f,
				  "Exponential capacity growth must be greater than 1.0.");

	// Maintaining elements in the middle of the capacity is more or less useless without the ability to shift.
	static_assert(traits.location != sequence_lits::MIDDLE || std::move_constructible<T>,
				  "Middle element location requires move-constructible types.");

	const value_type* begin() const { return data_begin(); }
	const value_type* end() const { return data_end(); }

	void push_front(const value_type& e)
	{
		if (size() == capacity())
			reallocate();
		add_front(e);
	}
	void push_back(const value_type& e)
	{
		if (size() == capacity())
			reallocate();
		add_back(e);
	}

private:


};

template<typename SEQ>
void show(const SEQ& seq)
{
	using traits_type = SEQ::traits_type;

	std::println("Size Type:\t{}", typeid(traits_type::size_type).name());

	std::print("Storage:\t");
	switch (seq.traits.storage)
	{
		case sequence_lits::LOCAL:			std::println("LOCAL");		break;
		case sequence_lits::FIXED:			std::println("FIXED");		break;
		case sequence_lits::VARIABLE:		std::println("VARIABLE");	break;
	}
	std::print("Location:\t");
	switch (seq.traits.location)
	{
		case sequence_lits::FRONT:			std::println("FRONT");		break;
		case sequence_lits::MIDDLE:			std::println("MIDDLE");		break;
		case sequence_lits::BACK:			std::println("BACK");		break;
	}
	std::print("Growth:\t\t");
	switch (seq.traits.growth)
	{
		case sequence_lits::LINEAR:			std::println("LINEAR");			break;
		case sequence_lits::EXPONENTIAL:	std::println("EXPONENTIAL");	break;
		case sequence_lits::VECTOR:			std::println("VECTOR");			break;
	}

	std::println("Capacity:\t{}", seq.traits.capacity);
	std::println("Increment:\t{}", seq.traits.increment);
	std::println("Factor:\t\t{}", seq.traits.factor);
	std::println("Size:\t\t{}", sizeof(seq));
}
