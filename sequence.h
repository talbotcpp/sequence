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

enum class sequence_storage_lits { STATIC, FIXED, VARIABLE, BUFFERED };	// See sequence_traits::storage.
enum class sequence_location_lits { FRONT, BACK, MIDDLE };				// See sequence_traits::location.
enum class sequence_growth_lits { LINEAR, EXPONENTIAL, VECTOR };		// See sequence_traits::growth.

// sequence_traits - Structure used to supply the sequence traits. The default values have been chosen
// to exactly model std::vector so that sequence can be used as a drop-in replacement with no adjustments.

template<std::unsigned_integral SIZE = size_t, size_t CAP = 1>
struct sequence_traits
{
	// 'size_type' is the type of the size field for fixed storage. This allows small sequences of
	// small types to be made significantly more space efficient by representing the size with a
	// smaller type than size_t. This size reduction may (or may not) also result in faster run times.
	// (Note that this type is not used in this structure. Using it for 'capacity' complicates
	// sequence_storage without offering any real benefits, and it's not correct for 'increment'
	// because the SBO (see below) may be small but the dynamic size large.)

	using size_type = SIZE;

	// 'storage' specifies how the capacity is allocated:
	//		STATIC:		The capacity is embedded in the sequence object (like std::inplace_vector or boost::static_vector). The capacity cannot change.
	//		FIXED:		The capacity is dynamically allocated. The capacity cannot change.
	//		VARIABLE:	The capacity is dynamically allocated (like std::vector). The capacity can change.
	//		BUFFERED:	The capacity may be embedded (buffered) or dynamically allocated (like boost::small_vector). The capacity can change.

	sequence_storage_lits storage = sequence_storage_lits::VARIABLE;

	// 'location' specifies how the data are managed within the capacity:
	//		FRONT:	Data starts at the lowest memory location. This makes push_back most efficient (like std::vector).
	//		BACK:	Data ends at the highest memory location. This makes push_front most efficient.
	//		MIDDLE:	Data floats in the middle of the capacity. This makes both push_back and push_front efficient (similar to std::deque).

	sequence_location_lits location = sequence_location_lits::FRONT;

	// 'growth' indicates how the capacity grows when growth is necessary:
	//		LINEAR:			Capacity grows by a fixed amount specified by 'increment'.
	//		EXPONENTIAL:	Capacity grows exponentially by a factor specified by 'factor'.
	//		VECTOR:			Capacity grows in the same way as std::vector. This behavior is implementation dependent.
	//						It is provided so that sequence can be used as an implementation of std::vector and/or a
	//						drop-in replacement for std::vector with no changes in behavior, even if the std::vector
	//						growth behavoir cannot be otherwise modeled with LINEAR or EXPONENTIAL growth modes.

	sequence_growth_lits growth = sequence_growth_lits::VECTOR;

	// 'capacity' is the size of the fixed capacity for STATIC and FIXED storages.
	// For VARIABLE storage 'capacity' is the initial capacity when allocation first occurs. (Initially empty containers have no capacity.)
	// For BUFFERED storage 'capacity' is the size of the small object optimization buffer (SBO).
	// 'capacity' must be greater than 0.

	size_t capacity = CAP;

	// 'increment' specifies the linear capacity growth in elements. This must be greater than 0.

	size_t increment = 1;

	// 'factor' specifies the exponential capacity growth factor. This must be greater than 1.0.
	// The minimum growth will be one element, regardless of how close to 1.0 this is set.

	float factor = 1.5;

	// 'grow' is a utility function which returns a new (larger) capacity given the current capacity.

	size_t grow(size_t cap) const
	{
		if (cap == 0) return capacity;
		switch (growth)
		{
			case sequence_growth_lits::LINEAR:
				return cap + increment;
			case sequence_growth_lits::EXPONENTIAL:
				return cap + std::max<size_t>(size_t(cap * (factor - 1.f)), 1u);
			default:
			case sequence_growth_lits::VECTOR:
				return cap + std::max<size_t>(cap / 2, 1u);
		}
	};
};


// ==============================================================================================================
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
		begin->~T();
	}
}


// The destroy_data function encapsulates calling the element destructors. It is called
// in the sequence destructor and elsewhere when elements are either going away or have
// been moved somewhere else.

template<typename T>
void destroy_data(T* begin, T* end)
{
	for (auto&& element : span<T>(begin, end))
		element.~T();
}


// The move_data function encapsulates the process of moving elements from one storage space
// to another. It executes the move, then destroys the moved-from element. Note: this could
// be optimized to use only one loop, but since std::uninitialized_move is probably optimized
// to handle simple types, that might not help in many cases unless it's done carefully.

template<typename T>
void move_data(T* data_begin, T* data_end, T* destination)
{
	std::uninitialized_move(data_begin, data_end, destination);
	destroy_data(data_begin, data_end);
}


// The front_gap function encapsulates the calculation of the location of the start of the data
// so that it can be easily selected by the sequence_traits::location value. This could probably
// also be a simple function, but making it a function template matches the rest of the code.

template<sequence_location_lits LOC>
size_t front_gap(size_t capacity, size_t size)
{
	static_assert(false, "An unimplemented specialization of front_gap was instantiated.");
}

template<>
size_t front_gap<sequence_location_lits::FRONT>(size_t capacity, size_t size)
{
	return 0;
}

template<>
size_t front_gap<sequence_location_lits::BACK>(size_t capacity, size_t size)
{
	return capacity - size;
}

template<>
size_t front_gap<sequence_location_lits::MIDDLE>(size_t capacity, size_t size)
{
	return (capacity - size) / 2;
}


// ==============================================================================================================
// fixed_capacity

template<typename T, size_t CAP> requires (CAP != 0)
class fixed_capacity
{
	using value_type = T;

public:

	fixed_capacity() {}
	fixed_capacity(fixed_capacity&&) {}
	~fixed_capacity() {}

	constexpr static size_t capacity() { return CAP; }

	value_type* capacity_begin() { return elements; }
	value_type* capacity_end() { return elements + CAP; }
	const value_type* capacity_begin() const { return elements; }
	const value_type* capacity_end() const { return elements + CAP; }

private:

	union
	{
		value_type elements[CAP];
		unsigned char unused;
	};
};


// fixed_sequence_storage - Helper class for sequence which provides the 3 different element management strategies
// for fixed capacity sequences (local or dynamically allocated).

template<sequence_location_lits LOC, typename T, sequence_traits TRAITS>
class fixed_sequence_storage
{
	static_assert(false, "An unimplemented specialization of fixed_sequence_storage was instantiated.");
};

template<typename T, sequence_traits TRAITS>
class fixed_sequence_storage<sequence_location_lits::FRONT, T, TRAITS> : fixed_capacity<T, TRAITS.capacity>
{
	using value_type = T;
	using size_type = typename decltype(TRAITS)::size_type;
	using inherited = fixed_capacity<T, TRAITS.capacity>;
	using inherited::capacity_begin;
	using inherited::capacity_end;

public:

	using inherited::capacity;
	size_t size() const { return m_size; }
	void set_size(size_t current_size) { m_size = static_cast<size_type>(current_size); }

	value_type* data_begin() { return capacity_begin(); }
	value_type* data_end() { return capacity_begin() + m_size; }
	const value_type* data_begin() const { return capacity_begin(); }
	const value_type* data_end() const { return capacity_begin() + m_size; }

	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(data_end() < capacity_end());

		shift_forward(data_begin(), data_end(), 1);
		new(data_begin()) value_type(std::forward<ARGS>(args)...);
		++m_size;
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		assert(size() < capacity());

		new(data_end()) value_type(std::forward<ARGS>(args)...);
		++m_size;
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_size = 0;
	}

private:

	size_type m_size = 0;
};

template<typename T, sequence_traits TRAITS>
class fixed_sequence_storage<sequence_location_lits::BACK, T, TRAITS> : fixed_capacity<T, TRAITS.capacity>
{
	using value_type = T;
	using size_type = typename decltype(TRAITS)::size_type;
	using inherited = fixed_capacity<T, TRAITS.capacity>;
	using inherited::capacity_begin;
	using inherited::capacity_end;

public:

	using inherited::capacity;
	size_t size() const { return m_size; }
	void set_size(size_t current_size) { m_size = static_cast<size_type>(current_size); }

	value_type* data_begin() { return capacity_end() - m_size; }
	value_type* data_end() { return capacity_end(); }
	const value_type* data_begin() const { return capacity_end() - m_size; }
	const value_type* data_end() const { return capacity_end(); }

	void add_front(const value_type& e)
	{
		assert(size() < capacity());

		++m_size;
		new(data_begin()) value_type(e);
	}
	void add_back(const value_type& e)
	{
		assert(size() < capacity());
		assert(data_begin() > capacity_begin());

		shift_reverse(data_begin(), data_end(), 1);
		++m_size;
		new(data_end() - 1) value_type(e);
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_size = 0;
	}

private:

	size_type m_size = 0;
};

template<typename T, sequence_traits TRAITS>
class fixed_sequence_storage<sequence_location_lits::MIDDLE, T, TRAITS> : fixed_capacity<T, TRAITS.capacity>
{
	using value_type = T;
	using size_type = typename decltype(TRAITS)::size_type;
	using inherited = fixed_capacity<T, TRAITS.capacity>;
	using inherited::capacity_begin;
	using inherited::capacity_end;

public:

	using inherited::capacity;
	size_t size() const { return capacity() - (m_front_gap + m_back_gap); }
	void set_size(size_t current_size)
	{
		m_front_gap = static_cast<size_type>(front_gap<TRAITS.location>(capacity(), current_size));
		m_back_gap = static_cast<size_type>(capacity() - (m_front_gap + current_size));
	}

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
			auto bg = m_back_gap / 2;
			auto fg = m_back_gap - bg;
			shift_reverse(data_begin(), data_end(), fg);
			m_back_gap = bg;
			m_front_gap = fg - 1;
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
			auto fg = m_front_gap / 2;
			auto bg = m_front_gap - fg;
			shift_reverse(data_begin(), data_end(), bg);
			m_front_gap = fg;
			m_back_gap = bg - 1;
		}
		else --m_back_gap;
		new(data_end() - 1) value_type(e);
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_front_gap = TRAITS.capacity / 2;
		m_back_gap = TRAITS.capacity - TRAITS.capacity / 2;
	}

private:

	size_type m_front_gap = static_cast<size_type>(TRAITS.capacity / 2);
	size_type m_back_gap = static_cast<size_type>(TRAITS.capacity - TRAITS.capacity / 2);
};


// ==============================================================================================================
// dynamic_capacity

template<typename T, sequence_traits TRAITS>
class dynamic_capacity
{
	using value_type = T;

public:

	dynamic_capacity() = default;
	dynamic_capacity(dynamic_capacity&& rhs)
	{
		m_capacity_begin = rhs.m_capacity_begin;
		rhs.m_capacity_begin = nullptr;
		m_capacity_end = rhs.m_capacity_end;
	}
	dynamic_capacity& operator=(dynamic_capacity&& rhs)
	{
		m_capacity_begin = rhs.m_capacity_begin;
		rhs.m_capacity_begin = nullptr;
		m_capacity_end = rhs.m_capacity_end;
		return *this;
	}
	~dynamic_capacity()
	{
		if (m_capacity_begin)
			delete static_cast<void*>(m_capacity_begin);
	}

	size_t capacity() const { return m_capacity_end - m_capacity_begin; }

protected:

	void make_new_capacity(size_t new_capacity, size_t offset, value_type* data_begin, value_type* data_end)
	{
std::println("    reallocate({}, {})", new_capacity, offset);
		auto new_store = static_cast<value_type*>(operator new(sizeof(value_type) * new_capacity));

		if (data_begin)
			move_data(data_begin, data_end, new_store + offset);

		if (m_capacity_begin)
			delete static_cast<void*>(m_capacity_begin);

		m_capacity_begin = new_store;
		m_capacity_end = m_capacity_begin + new_capacity;

for (auto p = m_capacity_begin; p != m_capacity_end; ++p)
	std::print("{}\t", p->i);
std::println();
	}

	value_type* m_capacity_begin = nullptr;	// Owning pointer using new/delete.
	value_type* m_capacity_end = nullptr;
};


// dynamic_sequence_storage - Helper class for sequence which provides the 3 different element management strategies
// for dynamically allocated variable capacity sequences.

template<sequence_location_lits LOC, typename T, sequence_traits TRAITS>
class dynamic_sequence_storage
{
	static_assert(false, "An unimplemented specialization of variable_sequence_storage was instantiated.");
};

template<typename T, sequence_traits TRAITS>
class dynamic_sequence_storage<sequence_location_lits::FRONT, T, TRAITS> : public dynamic_capacity<T, TRAITS>
{
	using value_type = T;
	using inherited = dynamic_capacity<T, TRAITS>;
	using inherited::make_new_capacity;
	using inherited::m_capacity_begin;
	using inherited::m_capacity_end;

public:

	using inherited::capacity;
	size_t size() const { return m_data_end - m_capacity_begin; }

	value_type* data_begin() { return m_capacity_begin; }
	value_type* data_end() { return m_data_end; }
	const value_type* data_begin() const { return m_capacity_begin; }
	const value_type* data_end() const { return m_data_end; }

	void reallocate(size_t new_capacity, size_t current_size, value_type* d_begin, value_type* d_end)
	{
		assert(current_size <= new_capacity);

		make_new_capacity(new_capacity, front_gap<TRAITS.location>(new_capacity, current_size), d_begin, d_end);
		m_data_end = m_capacity_begin + current_size;
	}
	void reallocate(size_t new_capacity)
	{
		reallocate(new_capacity, size(), data_begin(), data_end());
	}

	void add_front(const value_type& e)
	{
		assert(size() < capacity());
		assert(m_data_end < m_capacity_end);

		shift_forward(data_begin(), data_end(), 1);
		new(data_begin()) value_type(e);
		++m_data_end;
	}
	void add_back(const value_type& e)
	{
		assert(size() < capacity());
		assert(m_data_end < m_capacity_end);

		new(data_end()) value_type(e);
		++m_data_end;
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_data_end = m_capacity_begin;
	}

private:

	value_type* m_data_end = nullptr;
};

template<typename T, sequence_traits TRAITS>
class dynamic_sequence_storage<sequence_location_lits::BACK, T, TRAITS> : public dynamic_capacity<T, TRAITS>
{
	using value_type = T;
	using inherited = dynamic_capacity<T, TRAITS>;
	using inherited::make_new_capacity;
	using inherited::m_capacity_begin;
	using inherited::m_capacity_end;

public:

	using inherited::capacity;
	size_t size() const { return m_capacity_end - m_data_begin; }

	value_type* data_begin() { return m_data_begin; }
	value_type* data_end() { return m_capacity_end; }
	const value_type* data_begin() const { return m_data_begin; }
	const value_type* data_end() const { return m_capacity_end; }

	void reallocate(size_t new_capacity, size_t current_size, value_type* d_begin, value_type* d_end)
	{
		assert(current_size <= new_capacity);

		make_new_capacity(new_capacity, front_gap<TRAITS.location>(new_capacity, current_size), d_begin, d_end);
		m_data_begin = m_capacity_end - current_size;
	}
	void reallocate(size_t new_capacity)
	{
		reallocate(new_capacity, size(), data_begin(), data_end());
	}

	void add_front(const value_type& e)
	{
		assert(size() < capacity());
		assert(m_data_begin > m_capacity_begin);

		--m_data_begin;
		new(data_begin()) value_type(e);
	}
	void add_back(const value_type& e)
	{
		assert(size() < capacity());
		assert(m_data_begin > m_capacity_begin);

		shift_reverse(data_begin(), data_end(), 1);
		--m_data_begin;
		new(data_end() - 1) value_type(e);
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_data_begin = m_capacity_end;
	}

private:

	value_type* m_data_begin = nullptr;
};

template<typename T, sequence_traits TRAITS>
class dynamic_sequence_storage<sequence_location_lits::MIDDLE, T, TRAITS> : public dynamic_capacity<T, TRAITS>
{
	using value_type = T;
	using inherited = dynamic_capacity<T, TRAITS>;
	using inherited::make_new_capacity;
	using inherited::m_capacity_begin;
	using inherited::m_capacity_end;

public:

	using inherited::capacity;
	size_t size() const { return m_data_end - m_data_begin; }

	value_type* data_begin() { return m_data_begin; }
	value_type* data_end() { return m_data_end; }
	const value_type* data_begin() const { return m_data_begin; }
	const value_type* data_end() const { return m_data_end; }

	void reallocate(size_t new_capacity, size_t current_size, value_type* d_begin, value_type* d_end)
	{
		assert(current_size <= new_capacity);

		size_t gap = front_gap<TRAITS.location>(new_capacity, current_size);
		make_new_capacity(new_capacity, gap, d_begin, d_end);
		m_data_begin = m_capacity_begin + gap;
		m_data_end = m_data_begin + current_size;
	}
	void reallocate(size_t new_capacity)
	{
		reallocate(new_capacity, size(), data_begin(), data_end());
	}

	void add_front(const value_type& e)
	{
		assert(size() < capacity());
		assert(m_data_begin > m_capacity_begin || m_data_end < m_capacity_end);

		if (m_data_begin == m_capacity_begin)
		{
			size_t back_gap = m_capacity_end - m_data_end;
			size_t offset = back_gap - back_gap / 2;
			assert(offset > 0);

			shift_forward(data_begin(), data_end(), offset);
			m_data_begin += offset;
			m_data_end += offset;
		}
		--m_data_begin;
		new(data_begin()) value_type(e);
	}
	void add_back(const value_type& e)
	{
		assert(size() < capacity());
		assert(m_data_begin > m_capacity_begin || m_data_end < m_capacity_end);

		if (m_data_end == m_capacity_end)
		{
			size_t front_gap = m_data_begin - m_capacity_begin;
			size_t offset = front_gap - front_gap / 2;
			assert(offset > 0);

			shift_reverse(data_begin(), data_end(), offset);
			m_data_begin -= offset;
			m_data_end -= offset;
		}
		new(data_end()) value_type(e);
		++m_data_end;
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_data_begin = m_capacity_begin + capacity() / 2;
		m_data_end = m_data_begin;
	}

private:

	value_type* m_data_begin = nullptr;
	value_type* m_data_end = nullptr;
};


// ==============================================================================================================
// sequence_implementation - Base class for sequence which provides the 4 different memory allocation strategies.

template<sequence_storage_lits STO, typename T, sequence_traits TRAITS>
class sequence_implementation
{
	static_assert(false, "An unimplemented specialization of sequence_implementation was instantiated.");
};

// STATIC storage (like std::inplace_vector or boost::static_vector).

template<typename T, sequence_traits TRAITS>
class sequence_implementation<sequence_storage_lits::STATIC, T, TRAITS>
{
public:

	using value_type = T;

	constexpr static size_t capacity() { return TRAITS.capacity; }
	size_t size() const { return m_storage.size(); }

	void clear() { return m_storage.clear(); }

protected:

	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		m_storage.add_front(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		m_storage.add_back(std::forward<ARGS>(args)...);
	}

	auto data_begin() { return m_storage.data_begin(); }
	auto data_end() { return m_storage.data_end(); }
	auto data_begin() const { return m_storage.data_begin(); }
	auto data_end() const { return m_storage.data_end(); }

	void reallocate(size_t new_capacity)
	{
		throw std::bad_alloc();
	}

private:

	fixed_sequence_storage<TRAITS.location, T, TRAITS> m_storage;
};

// FIXED storage. This is kind of like a std::vector which has been reserved and not allowed to reallocate.

template<typename T, sequence_traits TRAITS>
class sequence_implementation<sequence_storage_lits::FIXED, T, TRAITS>
{
	using value_type = T;
	using storage_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;

public:

	constexpr static size_t capacity() { return TRAITS.capacity; }
	size_t size() const { return m_storage ? m_storage->size() : 0; }

protected:

	void add_front(const value_type& e)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		m_storage->add_front(e);
	}
	void add_back(const value_type& e)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		m_storage->add_back(e);
	}
	auto data_begin() { return m_storage ? m_storage->data_begin() : nullptr; }
	auto data_end() { return m_storage ? m_storage->data_end() : nullptr; }
	auto data_begin() const { return m_storage ? m_storage->data_begin() : nullptr; }
	auto data_end() const { return m_storage ? m_storage->data_end() : nullptr; }

	void reallocate(size_t new_capacity)
	{
		throw std::bad_alloc();
	}

private:

	std::unique_ptr<storage_type> m_storage;
};

// VARIABLE storage (like std::vector).

template<typename T, sequence_traits TRAITS>
class sequence_implementation<sequence_storage_lits::VARIABLE, T, TRAITS>
{
	using value_type = T;

public:

	size_t capacity() const { return m_storage.capacity(); }
	size_t size() const { return m_storage.size(); }

protected:

	void add_front(const value_type& e) { m_storage.add_front(e); }
	void add_back(const value_type& e) { m_storage.add_back(e); }
	auto data_begin() { return m_storage.data_begin(); }
	auto data_end() { return m_storage.data_end(); }
	auto data_begin() const { return m_storage.data_begin(); }
	auto data_end() const { return m_storage.data_end(); }
	void reallocate(size_t new_capacity) { m_storage.reallocate(new_capacity); }

private:

	dynamic_sequence_storage<TRAITS.location, T, TRAITS> m_storage;
};

// BUFFERED storage supporting a small object buffer optimization (like boost::small_vector).

template<typename T, sequence_traits TRAITS>
class sequence_implementation<sequence_storage_lits::BUFFERED, T, TRAITS>
{
	using value_type = T;
	using fixed_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;		// STC
	using dynamic_type = dynamic_sequence_storage<TRAITS.location, T, TRAITS>;	// DYN
	enum { STC, DYN };

public:

	size_t capacity() const { return m_storage.index() == STC ? get<STC>(m_storage).capacity() : get<DYN>(m_storage).capacity(); }
	size_t size() const { return m_storage.index() == STC ? get<STC>(m_storage).size() : get<DYN>(m_storage).size(); }

protected:

	void add_front(const value_type& e) { m_storage.index() == STC ? get<STC>(m_storage).add_front(e) : get<DYN>(m_storage).add_front(e); }
	void add_back(const value_type& e) { m_storage.index() == STC ? get<STC>(m_storage).add_back(e) : get<DYN>(m_storage).add_back(e); }

	auto data_begin() { return m_storage.index() == STC ? get<STC>(m_storage).data_begin() : get<DYN>(m_storage).data_begin(); }
	auto data_end() { return m_storage.index() == STC ? get<STC>(m_storage).data_end() : get<DYN>(m_storage).data_end(); }
	auto data_begin() const { return m_storage.index() == STC ? get<STC>(m_storage).data_begin() : get<DYN>(m_storage).data_begin(); }
	auto data_end() const { return m_storage.index() == STC ? get<STC>(m_storage).data_end() : get<DYN>(m_storage).data_end(); }

	void reallocate(size_t new_capacity)
	{
		// The new capacity will not fit in the buffer.
		if (new_capacity > TRAITS.capacity)
		{
			// We're moving out of the buffer: switch to dynamic storage.
			if (m_storage.index() == STC)
			{
				dynamic_type new_storage;
				new_storage.reallocate(new_capacity, get<STC>(m_storage).size(),
									   get<STC>(m_storage).data_begin(), get<STC>(m_storage).data_end());
				m_storage = std::move(new_storage);
			}
			// We're already out of the buffer: adjust the dynamic capacity.
			else						
				get<DYN>(m_storage).reallocate(new_capacity);
		}

		// The new capacity will fit in the buffer.
		else
			// We're moving into the buffer: switch to buffer storage.
			if (m_storage.index() == DYN)
			{
				assert(get<DYN>(m_storage).size() <= TRAITS.capacity);

				dynamic_type temp_storage(std::move(get<DYN>(m_storage)));
				m_storage.emplace<STC>();
				get<STC>(m_storage).set_size(temp_storage.size());
				move_data(temp_storage.data_begin(), temp_storage.data_end(), get<STC>(m_storage).data_begin());
			}
			// We're already in the buffer: do nothing (the buffer capacity cannot change).
	}

private:

	std::variant<fixed_type, dynamic_type> m_storage;
};


// ==============================================================================================================
// sequence - This is the main class template.

template<typename T, sequence_traits TRAITS = sequence_traits<size_t>()>
class sequence : public sequence_implementation<TRAITS.storage, T, TRAITS>
{
	using inherited = sequence_implementation<TRAITS.storage, T, TRAITS>;
	using inherited::data_begin;
	using inherited::data_end;
	using inherited::reallocate;
	using inherited::add_front;
	using inherited::add_back;

public:

	using value_type = T;

	using inherited::size;
	using inherited::capacity;

	using traits_type = decltype(TRAITS);
	static constexpr traits_type traits = TRAITS;
	using size_type = typename traits_type::size_type;

	// Variable capacity means that the capacity must grow, and this growth must actually make progress.
	// Zero capacity is not permitted (although this could be changed if it poses problems in generic contexts).
	static_assert(traits.capacity > 0,
				  "Fixed, initial, and buffer capacity must be greater than 0.");
	static_assert(traits.increment > 0,
				  "Linear capacity growth must be greater than 0.");
	static_assert(traits.factor > 1.0f,
				  "Exponential capacity growth must be greater than 1.0.");

	// Maintaining elements in the middle of the capacity is more or less useless without the ability to shift.
	static_assert(traits.location != sequence_location_lits::MIDDLE || std::move_constructible<T>,
				  "Middle element location requires move-constructible types.");

	// A fixed capacity of any kind requires that the size type represent a count up to the requested capacity.
	static_assert(traits.storage == sequence_storage_lits::VARIABLE ||
				  traits.capacity <= std::numeric_limits<size_type>::max(),
				  "Size type is insufficient to hold requested capacity.");


	~sequence()
	{
		destroy_data(data_begin(), data_end());
	}

	const value_type* data() const { return data_begin(); }
	const value_type* begin() const { return data_begin(); }
	const value_type* end() const { return data_end(); }
	bool empty() const { return size() == 0; }

	void reserve(size_t new_capacity)
	{
		if (new_capacity > capacity())
			reallocate(new_capacity);
	}
	void shrink_to_fit()
	{
		if (auto current_size = size(); current_size < capacity())
			reallocate(current_size);
	}

	template<typename... ARGS>
	void emplace_front(ARGS&&... args)
	{
		if (auto old_capacity = capacity(); size() == old_capacity)
			reallocate(traits.grow(old_capacity));
		add_front(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void emplace_back(ARGS&&... args)
	{
		if (auto old_capacity = capacity(); size() == old_capacity)
			reallocate(traits.grow(old_capacity));
		add_back(std::forward<ARGS>(args)...);
	}

	void push_front(const value_type& e) { emplace_front(e); }
	void push_back(const value_type& e) { emplace_back(e); }

	void resize(size_t new_size)
	{}

private:


};


// show - Debugging display for sequence traits.

template<typename SEQ>
void show(const SEQ& seq)
{
	using traits_type = SEQ::traits_type;

	std::println("Size Type:\t{}", typeid(traits_type::size_type).name());

	std::print("Storage:\t");
	switch (seq.traits.storage)
	{
		case sequence_storage_lits::STATIC:		std::println("STATIC");		break;
		case sequence_storage_lits::FIXED:		std::println("FIXED");		break;
		case sequence_storage_lits::VARIABLE:	std::println("VARIABLE");	break;
		case sequence_storage_lits::BUFFERED:	std::println("BUFFERED");	break;
	}
	std::print("Location:\t");
	switch (seq.traits.location)
	{
		case sequence_location_lits::FRONT:		std::println("FRONT");		break;
		case sequence_location_lits::MIDDLE:	std::println("MIDDLE");		break;
		case sequence_location_lits::BACK:		std::println("BACK");		break;
	}
	std::print("Growth:\t\t");
	switch (seq.traits.growth)
	{
		case sequence_growth_lits::LINEAR:		std::println("LINEAR");			break;
		case sequence_growth_lits::EXPONENTIAL:	std::println("EXPONENTIAL");	break;
		case sequence_growth_lits::VECTOR:		std::println("VECTOR");			break;
	}

	std::println("Capacity:\t{}", seq.traits.capacity);
	std::println("Increment:\t{}", seq.traits.increment);
	std::println("Factor:\t\t{}", seq.traits.factor);
	std::println("Size:\t\t{}", sizeof(seq));
}
