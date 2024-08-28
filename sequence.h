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

template<std::unsigned_integral SIZE = size_t>
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
	// 
	// STATIC	The capacity is embedded in the sequence object (like std::inplace_vector or boost::static_vector).
	//			The capacity cannot change size or move.
	//
	// FIXED	The capacity is dynamically allocated. The capacity cannot change size. Clearing the sequence
	//			deallocates the capacity. Erasing the sequence does not deallocate the capacity. This is like
	//			a std::vector which has been reserved and not allowed to reallocate, except that the in-class
	//			storage is only one pointer. The size(s) are in the dynamic allocation and can be sized.
	//
	// VARIABLE	The capacity is dynamically allocated (like std::vector). The capacity can change and move.
	//			Neither clearing nor erasing the sequence deallocates the capacity (like std::vector).
	//
	// BUFFERED	The capacity may be embedded (buffered) or dynamically allocated (like boost::small_vector).
	//			The capacity can change and move. Clearing the sequence deallocates the capacity if it was
	//			dynamically allocated. Erasing the sequence does not deallocate the capacity. Reserving a
	//			capacity less than or equal to the fixed capacity size has no effect. Reserving a capacity
	//			greater than the capacity size causes the capacity to be dynamically (re)allocated. Calling
	//			shrink_to_fit when the capacity is buffered has no effect. Calling it when the capacity is
	//			dynamically allocated and the size is greater than the fixed capacity size has the expected
	//			effect (as with std::vector). Calling it when the capacity is dynamically allocated and the
	//			size is less than or equal to the fixed capacity size causes the capacity to be rebuffered
	//			and the dynamic capacity to be deallocated.

	sequence_storage_lits storage = sequence_storage_lits::VARIABLE;

	// 'location' specifies how the data are managed within the capacity:
	// 
	//	FRONT	Data starts at the lowest memory location.
	//			This makes push_back most efficient (like std::vector).
	// 
	//	BACK	Data ends at the highest memory location.
	//			This makes push_front most efficient.
	// 
	//	MIDDLE	Data floats in the middle of the capacity.
	//			This makes both push_back and push_front efficient (similar to std::deque).

	sequence_location_lits location = sequence_location_lits::FRONT;

	// 'growth' indicates how the capacity grows when growth is necessary:
	// 
	//	LINEAR		Capacity grows by a fixed amount specified by 'increment'.
	// 
	//	EXPONENTIAL	Capacity grows exponentially by a factor specified by 'factor'.
	// 
	//	VECTOR		Capacity grows in the same way as std::vector. This behavior is implementation dependent.
	//				It is provided so that sequence can be used as an implementation of std::vector and/or a
	//				drop-in replacement for std::vector with no changes in behavior, even if the std::vector
	//				growth behavior cannot be otherwise modeled with LINEAR or EXPONENTIAL growth modes.

	sequence_growth_lits growth = sequence_growth_lits::VECTOR;

	// 'capacity' is the size of the fixed capacity for STATIC and FIXED storages.
	// For VARIABLE storage 'capacity' is the initial capacity when allocation first occurs.
	// (Initially-empty containers have no capacity.) For BUFFERED storage 'capacity' is the size
	// of the small object optimization buffer (SBO). 'capacity' must be greater than 0.

	size_t capacity = 1;

	// 'increment' specifies the linear capacity growth in elements. This must be greater than 0.

	size_t increment = 1;

	// 'factor' specifies the exponential capacity growth factor. This must be greater than 1.0.
	// The minimum growth will be one element, regardless of how close to 1.0 this is set.

	float factor = 1.5;

	// 'grow' returns a new (larger) capacity given the current capacity. The calculation is based
	// on the sequence_traits members which control capacity.

	size_t grow(size_t cap) const
	{
		if (cap == 0) return capacity;
		switch (growth)
		{
			case sequence_growth_lits::LINEAR:
				return cap + increment;
			case sequence_growth_lits::EXPONENTIAL:
				return cap + std::max(size_t(cap * (factor - 1.f)), increment);
			default:
			case sequence_growth_lits::VECTOR:
				return cap + std::max<size_t>(cap / 2, 1u);
		}
	};

	// 'front_gap' returns the location of the start of the data given a capacity and size.
	// The formula is based on the 'location' value.

	size_t front_gap(size_t capacity, size_t size) const
	{
		switch (location)
		{
		default:
		case sequence_location_lits::FRONT:		return 0;
		case sequence_location_lits::BACK:		return capacity - size;
		case sequence_location_lits::MIDDLE:	return (capacity - size) / 2;
		}
	}
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
		new(--dest) T(std::move(*--end));
		end->~T();
	}
}

template<typename T>
void shift_reverse(T* begin, T* end, size_t distance)
{
	assert(distance != 0);

	for (auto dest = begin - distance; begin < end; ++dest, ++begin)
	{
		new(dest) T(std::move(*begin));
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


// ==============================================================================================================
// fixed_capacity

template<typename T, size_t CAP> requires (CAP != 0)
class fixed_capacity
{
	using value_type = T;

public:

	fixed_capacity()
	{

	}
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
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reference = value_type&;
	using size_type = typename decltype(TRAITS)::size_type;
	using inherited = fixed_capacity<T, TRAITS.capacity>;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	fixed_sequence_storage() = default;
	fixed_sequence_storage(const fixed_sequence_storage& rhs)
	{
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_size = rhs.m_size;
	}
	fixed_sequence_storage(fixed_sequence_storage&& rhs)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_size = rhs.m_size;
	}
	fixed_sequence_storage& operator=(const fixed_sequence_storage& rhs)
	{
		destroy_data(data_begin(), data_end());
		m_size = 0;
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_size = rhs.m_size;
		return *this;
	}
	fixed_sequence_storage& operator=(fixed_sequence_storage&& rhs)
	{
		destroy_data(data_begin(), data_end());
		m_size = 0;
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_size = rhs.m_size;
		return *this;
	}

	size_t size() const { return m_size; }
	void set_size(size_t current_size) { m_size = static_cast<size_type>(current_size); }

	value_type* data_begin() { return capacity_begin(); }
	value_type* data_end() { return capacity_begin() + m_size; }
	const value_type* data_begin() const { return capacity_begin(); }
	const value_type* data_end() const { return capacity_begin() + m_size; }

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(data_end() < capacity_end());
		assert(pos >= data_begin() && pos <= data_end());

		shift_forward(pos, data_end(), 1);
		new(pos) value_type(std::forward<ARGS>(args)...);
		++m_size;
		return pos;
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		add_at(data_begin(), std::forward<ARGS>(args)...);
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
	void erase(value_type* begin, value_type* end)
	{
		assert(begin >= data_begin());
		assert(end <= data_end());

		size_t count = end - begin;
		destroy_data(begin, end);
		shift_reverse(end, data_end(), count);
		m_size -= count;
	}
	void erase(value_type* element)
	{
		assert(element >= data_begin());
		assert(element < data_end());
		assert(size());

		element->~value_type();
		shift_reverse(element + 1, data_end(), 1);
		--m_size;
	}
	void pop_front()
	{
		assert(size());

		erase(data_begin());
	}
	void pop_back()
	{
		assert(size());

		(data_end() - 1)->~value_type();
		--m_size;
	}

private:

	size_type m_size = 0;
};

template<typename T, sequence_traits TRAITS>
class fixed_sequence_storage<sequence_location_lits::BACK, T, TRAITS> : fixed_capacity<T, TRAITS.capacity>
{
	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;
	using inherited = fixed_capacity<T, TRAITS.capacity>;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	fixed_sequence_storage() = default;
	fixed_sequence_storage(const fixed_sequence_storage& rhs)
	{
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_end() - rhs.m_size);
		m_size = rhs.m_size;
	}
	fixed_sequence_storage(fixed_sequence_storage&& rhs)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_end() - rhs.m_size);
		m_size = rhs.m_size;
	}

	size_t size() const { return m_size; }
	void set_size(size_t current_size) { m_size = static_cast<size_type>(current_size); }

	value_type* data_begin() { return capacity_end() - m_size; }
	value_type* data_end() { return capacity_end(); }
	const value_type* data_begin() const { return capacity_end() - m_size; }
	const value_type* data_end() const { return capacity_end(); }

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(data_begin() > capacity_begin());
		assert(pos >= data_begin() && pos <= data_end());

		shift_reverse(data_begin(), pos, 1);
		++m_size;
		new(--pos) value_type(std::forward<ARGS>(args)...);
		return pos;
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		assert(size() < capacity());

		++m_size;
		new(data_begin()) value_type(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		add_at(data_end(), std::forward<ARGS>(args)...);
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_size = 0;
	}
	void erase(value_type* begin, value_type* end)
	{
		assert(begin >= data_begin());
		assert(end <= data_end());

		size_t count = end - begin;
		destroy_data(begin, end);
		shift_forward(data_begin(), begin, count);
		m_size -= count;
	}
	void erase(value_type* element)
	{
		assert(element >= data_begin());
		assert(element < data_end());
		assert(size());

		element->~value_type();
		shift_forward(data_begin(), element, 1);
		--m_size;
	}
	void pop_front()
	{
		assert(size());

		data_begin()->~value_type();
		--m_size;
	}
	void pop_back()
	{
		assert(size());

		erase(data_end() - 1);
	}

private:

	size_type m_size = 0;
};

template<typename T, sequence_traits TRAITS>
class fixed_sequence_storage<sequence_location_lits::MIDDLE, T, TRAITS> : fixed_capacity<T, TRAITS.capacity>
{
	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;
	using inherited = fixed_capacity<T, TRAITS.capacity>;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	fixed_sequence_storage() = default;
	fixed_sequence_storage(const fixed_sequence_storage& rhs)
	{
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin() + rhs.m_front_gap);
		m_front_gap = rhs.m_front_gap;
		m_back_gap = rhs.m_back_gap;
	}
	fixed_sequence_storage(fixed_sequence_storage&& rhs)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin() + rhs.m_front_gap);
		m_front_gap = rhs.m_front_gap;
		m_back_gap = rhs.m_back_gap;
	}

	size_t size() const { return capacity() - (m_front_gap + m_back_gap); }
	void set_size(size_t current_size)
	{
		m_front_gap = static_cast<size_type>(TRAITS.front_gap(capacity(), current_size));
		m_back_gap = static_cast<size_type>(capacity() - (m_front_gap + current_size));
	}

	value_type* data_begin() { return capacity_begin() + m_front_gap; }
	value_type* data_end() { return capacity_end() - m_back_gap; }
	const value_type* data_begin() const { return capacity_begin() + m_front_gap; }
	const value_type* data_end() const { return capacity_end() - m_back_gap; }

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_front_gap || m_back_gap);
		assert(pos >= data_begin() && pos <= data_end());

		if (pos - data_begin() >= data_end() - pos)	// Inserting closer to the end--add at back.
		{
			if (m_back_gap == 0)
			{
				recenter_reverse();
				pos -= m_back_gap;
			}
			shift_forward(pos, data_end(), 1);

			new(pos) value_type(std::forward<ARGS>(args)...);
			--m_back_gap;
		}
		else										// Inserting closer to the beginning--add at front.
		{
			if (m_front_gap == 0)
			{
				recenter_forward();
				pos += m_front_gap;
			}
			shift_reverse(data_begin(), pos, 1);

			new(--pos) value_type(std::forward<ARGS>(args)...);
			--m_front_gap;
		}
		return pos;
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_front_gap || m_back_gap);

		if (m_front_gap == 0)
			recenter_forward();
		--m_front_gap;
		new(data_begin()) value_type(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_front_gap || m_back_gap);

		if (m_back_gap == 0)
			recenter_reverse();
		new(data_end()) value_type(std::forward<ARGS>(args)...);
		--m_back_gap;
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_front_gap = TRAITS.capacity / 2;
		m_back_gap = TRAITS.capacity - TRAITS.capacity / 2;
	}
	void erase(value_type* begin, value_type* end)
	{
		assert(begin >= data_begin());
		assert(end <= data_end());

		size_t count = end - begin;
		destroy_data(begin, end);

		if (begin - data_begin() > data_end() - end)
		{
			shift_reverse(end, data_end(), count);
			m_back_gap += count;
		}
		else
		{
			shift_forward(data_begin(), begin, count);
			m_front_gap += count;
		}
	}
	void erase(value_type* element)
	{
		assert(element >= data_begin());
		assert(element < data_end());

		element->~value_type();
		if (element - data_begin() > data_end() - element)
		{
			shift_reverse(element + 1, data_end(), 1);
			++m_back_gap;
		}
		else
		{
			shift_forward(data_begin(), element, 1);
			++m_front_gap;
		}
	}
	void pop_front()
	{
		assert(size());

		data_begin()->~value_type();
		++m_front_gap;
	}
	void pop_back()
	{
		assert(size());

		(data_end() - 1)->~value_type();
		++m_back_gap;
	}

private:

	void recenter_forward()
	{
		assert(m_front_gap == 0);

		auto bg = m_back_gap / 2;						// We have to cache these because data_begin/end depend on the gaps.
		auto fg = m_back_gap - bg;						// Leave more room at the front if gap is odd.
		shift_forward(data_begin(), data_end(), fg);
		m_front_gap = fg;
		m_back_gap = bg;
	}

	void recenter_reverse()
	{
		assert(m_back_gap == 0);

		auto fg = m_front_gap / 2;						// We have to cache these because data_begin/end depend on the gaps.
		auto bg = m_front_gap - fg;						// Leave more room at the back if gap is odd.
		shift_reverse(data_begin(), data_end(), bg);
		m_front_gap = fg;
		m_back_gap = bg;
	}

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
	const value_type* capacity_begin() const { return m_capacity_begin; }
	const value_type* capacity_end() const { return m_capacity_end; }

protected:

	void make_new_capacity(size_t new_capacity, size_t offset, value_type* data_begin, value_type* data_end)
	{
		auto new_store = static_cast<value_type*>(operator new(sizeof(value_type) * new_capacity));

		if (data_begin)
		{
			std::uninitialized_move(data_begin, data_end, new_store + offset);
			destroy_data(data_begin, data_end);
		}

		if (m_capacity_begin)
			delete static_cast<void*>(m_capacity_begin);

		m_capacity_begin = new_store;
		m_capacity_end = m_capacity_begin + new_capacity;
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
	using iterator = value_type*;
	using inherited = dynamic_capacity<T, TRAITS>;
	using inherited::make_new_capacity;

public:

	using inherited::capacity;
	using inherited::m_capacity_begin;
	using inherited::m_capacity_end;

	dynamic_sequence_storage() = default;
	dynamic_sequence_storage(const dynamic_sequence_storage& rhs)
	{
		make_new_capacity(rhs.size(), 0, nullptr, nullptr);
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), m_capacity_begin);
		m_data_end = m_capacity_end;
	}
	dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		make_new_capacity(rhs.size(), 0, rhs.data_begin(), rhs.data_end());
		m_data_end = m_capacity_end;
	}

	size_t size() const { return m_data_end - m_capacity_begin; }

	value_type* data_begin() { return m_capacity_begin; }
	value_type* data_end() { return m_data_end; }
	const value_type* data_begin() const { return m_capacity_begin; }
	const value_type* data_end() const { return m_data_end; }

	void reallocate(size_t new_capacity, size_t current_size, size_t new_size, value_type* d_begin, value_type* d_end)
	{
		assert(current_size <= new_capacity);

		make_new_capacity(new_capacity, TRAITS.front_gap(new_capacity, new_size), d_begin, d_end);
		m_data_end = m_capacity_begin + current_size;
	}

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_data_end < m_capacity_end);
		assert(pos >= data_begin() && pos <= data_end());

		shift_forward(pos, data_end(), 1);
		new(pos) value_type(std::forward<ARGS>(args)...);
		++m_data_end;
		return pos;
}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		add_at(data_begin(), std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		assert(size() < capacity());

		new(data_end()) value_type(std::forward<ARGS>(args)...);
		++m_data_end;
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_data_end = m_capacity_begin;
	}
	void erase(value_type* begin, value_type* end)
	{
		assert(begin >= data_begin());
		assert(end <= data_end());

		size_t count = end - begin;
		destroy_data(begin, end);
		shift_reverse(end, data_end(), count);
		m_data_end -= count;
	}
	void erase(value_type* element)
	{
		assert(element >= data_begin());
		assert(element < data_end());
		assert(size());

		element->~value_type();
		shift_reverse(element + 1, data_end(), 1);
		--m_data_end;
	}
	void pop_front()
	{
		assert(size());

		erase(data_begin());
	}
	void pop_back()
	{
		assert(size());

		(data_end() - 1)->~value_type();
		--m_data_end;
	}

private:

	value_type* m_data_end = nullptr;
};

template<typename T, sequence_traits TRAITS>
class dynamic_sequence_storage<sequence_location_lits::BACK, T, TRAITS> : public dynamic_capacity<T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using inherited = dynamic_capacity<T, TRAITS>;
	using inherited::make_new_capacity;

public:

	using inherited::capacity;
	using inherited::m_capacity_begin;
	using inherited::m_capacity_end;

	dynamic_sequence_storage() = default;
	dynamic_sequence_storage(const dynamic_sequence_storage& rhs)
	{
		make_new_capacity(rhs.size(), 0, nullptr, nullptr);
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), m_capacity_begin);
		m_data_begin = m_capacity_begin;
	}
	dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		make_new_capacity(rhs.size(), 0, rhs.data_begin(), rhs.data_end());
		m_data_begin = m_capacity_begin;
	}

	size_t size() const { return m_capacity_end - m_data_begin; }

	value_type* data_begin() { return m_data_begin; }
	value_type* data_end() { return m_capacity_end; }
	const value_type* data_begin() const { return m_data_begin; }
	const value_type* data_end() const { return m_capacity_end; }

	void reallocate(size_t new_capacity, size_t current_size, size_t new_size, value_type* d_begin, value_type* d_end)
	{
		assert(current_size <= new_capacity);

		make_new_capacity(new_capacity, TRAITS.front_gap(new_capacity, new_size), d_begin, d_end);
		m_data_begin = m_capacity_end - current_size;
	}

	template<typename... ARGS>
	iterator add_at(value_type* pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(data_begin() > m_capacity_begin);
		assert(pos >= data_begin() && pos <= data_end());

		shift_reverse(data_begin(), pos, 1);
		--m_data_begin;
		new(--pos) value_type(std::forward<ARGS>(args)...);
		return pos;
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		assert(size() < capacity());

		--m_data_begin;
		new(data_begin()) value_type(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		add_at(data_end(), std::forward<ARGS>(args)...);
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_data_begin = m_capacity_end;
	}
	void erase(value_type* begin, value_type* end)
	{
		assert(begin >= data_begin());
		assert(end <= data_end());

		size_t count = end - begin;
		destroy_data(begin, end);
		shift_forward(data_begin(), begin, count);
		m_data_begin += count;
	}
	void erase(value_type* element)
	{
		assert(element >= data_begin());
		assert(element < data_end());
		assert(size());

		element->~value_type();
		shift_forward(data_begin(), element, 1);
		++m_data_begin;
	}
	void pop_front()
	{
		assert(size());

		data_begin()->~value_type();
		++m_data_begin;
	}
	void pop_back()
	{
		assert(size());

		erase(data_end() - 1);
	}

private:

	value_type* m_data_begin = nullptr;
};

template<typename T, sequence_traits TRAITS>
class dynamic_sequence_storage<sequence_location_lits::MIDDLE, T, TRAITS> : public dynamic_capacity<T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using inherited = dynamic_capacity<T, TRAITS>;
	using inherited::make_new_capacity;

public:

	using inherited::capacity;
	using inherited::m_capacity_begin;
	using inherited::m_capacity_end;

	dynamic_sequence_storage() = default;
	dynamic_sequence_storage(const dynamic_sequence_storage& rhs)
	{
		make_new_capacity(rhs.size(), 0, nullptr, nullptr);
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), m_capacity_begin);
		m_data_begin = m_capacity_begin;
		m_data_end = m_capacity_end;
	}
	dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		make_new_capacity(rhs.size(), 0, rhs.data_begin(), rhs.data_end());
		m_data_begin = m_capacity_begin;
		m_data_end = m_capacity_end;
	}

	size_t size() const { return m_data_end - m_data_begin; }

	value_type* data_begin() { return m_data_begin; }
	value_type* data_end() { return m_data_end; }
	const value_type* data_begin() const { return m_data_begin; }
	const value_type* data_end() const { return m_data_end; }

	void reallocate(size_t new_capacity, size_t current_size, size_t new_size, value_type* d_begin, value_type* d_end)
	{
		assert(current_size <= new_capacity);

		size_t gap = TRAITS.front_gap(new_capacity, new_size);
		make_new_capacity(new_capacity, gap, d_begin, d_end);
		m_data_begin = m_capacity_begin + gap;
		m_data_end = m_data_begin + current_size;
	}

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_data_begin > m_capacity_begin || m_data_end < m_capacity_end);
		assert(pos >= data_begin() && pos <= data_end());

		if (pos - data_begin() >= data_end() - pos)	// Inserting closer to the end--add at back.
		{
			if (m_data_end == m_capacity_end)
			{
				recenter_reverse();
				pos -= m_capacity_end - m_data_end;
			}
			shift_forward(pos, m_data_end, 1);

			new(pos) value_type(std::forward<ARGS>(args)...);
			++m_data_end;
		}
		else										// Inserting closer to the beginning--add at front.
		{
			if (m_data_begin == m_capacity_begin)
			{
				recenter_forward();
				pos += m_data_begin - m_capacity_begin;
			}
			shift_reverse(m_data_begin, pos, 1);

			new(--pos) value_type(std::forward<ARGS>(args)...);
			--m_data_begin;
		}
		return pos;
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_data_begin > m_capacity_begin || m_data_end < m_capacity_end);

		if (m_data_begin == m_capacity_begin)
			recenter_forward();
		--m_data_begin;
		new(data_begin()) value_type(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_data_begin > m_capacity_begin || m_data_end < m_capacity_end);

		if (m_data_end == m_capacity_end)
			recenter_reverse();
		new(data_end()) value_type(std::forward<ARGS>(args)...);
		++m_data_end;
	}

	void clear()
	{
		destroy_data(data_begin(), data_end());
		m_data_begin = m_capacity_begin + capacity() / 2;
		m_data_end = m_data_begin;
	}
	void erase(value_type* begin, value_type* end)
	{
		assert(begin >= data_begin());
		assert(end <= data_end());

		size_t count = end - begin;
		destroy_data(begin, end);

		if (begin - data_begin() > data_end() - end)
		{
			shift_reverse(end, data_end(), count);
			m_data_end -= count;
		}
		else
		{
			shift_forward(data_begin(), begin, count);
			m_data_begin += count;
		}
	}
	void erase(value_type* element)
	{
		assert(element >= data_begin());
		assert(element < data_end());
		assert(size());

		element->~value_type();
		if (element - data_begin() > data_end() - element)
		{
			shift_reverse(element + 1, data_end(), 1);
			--m_data_end;
		}
		else
		{
			shift_forward(data_begin(), element, 1);
			++m_data_begin;
		}
	}
	void pop_front()
	{
		assert(size());

		data_begin()->~value_type();
		++m_data_begin;
	}
	void pop_back()
	{
		assert(size());

		(data_end() - 1)->~value_type();
		--m_data_end;
	}

private:

	void recenter_forward()
	{
		assert(m_data_begin == m_capacity_begin);
		assert(m_data_end != m_capacity_end);

		size_t offset = m_capacity_end - m_data_end;
		offset -= offset / 2;
		shift_forward(data_begin(), data_end(), offset);
		m_data_begin += offset;
		m_data_end += offset;
	}

	void recenter_reverse()
	{
		assert(m_data_begin != m_capacity_begin);
		assert(m_data_end == m_capacity_end);

		size_t offset = m_data_begin - m_capacity_begin;
		offset -= offset / 2;
		shift_reverse(data_begin(), data_end(), offset);
		m_data_begin -= offset;
		m_data_end -= offset;
	}

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
	using storage_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;

public:

	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using size_type = typename decltype(TRAITS)::size_type;

	constexpr static size_t capacity() { return TRAITS.capacity; }
	size_t size() const { return m_storage.size(); }
	size_t max_size() const { return std::numeric_limits<size_type>::max(); }

	void clear() { m_storage.clear(); }
	void erase(value_type* begin, value_type* end) { m_storage.erase(begin, end); }
	void erase(value_type* element) { m_storage.erase(element); }
	void pop_front() { m_storage.pop_front(); }
	void pop_back() { m_storage.pop_back(); }

	void swap(sequence_implementation& other)
	{
		std::swap(m_storage, other.m_storage);
	}

protected:

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		return m_storage.add_at(pos, std::forward<ARGS>(args)...);
	}
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
	auto capacity_begin() const { return m_storage.capacity_begin(); }
	auto capacity_end() const { return m_storage.capacity_end(); }

	void reallocate(size_t new_capacity, size_t new_size)
	{
		throw std::bad_alloc();
	}

private:

	storage_type m_storage;
};

// FIXED storage.

template<typename T, sequence_traits TRAITS>
class sequence_implementation<sequence_storage_lits::FIXED, T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using storage_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;
	using size_type = typename decltype(TRAITS)::size_type;

public:

	constexpr static size_t capacity() { return TRAITS.capacity; }
	size_t size() const { return m_storage ? m_storage->size() : 0; }
	size_t max_size() const { return std::numeric_limits<size_type>::max(); }

	void clear()
	{
		m_storage->clear();
		m_storage.reset();
	}
	void erase(value_type* begin, value_type* end) { m_storage->erase(begin, end); }
	void erase(value_type* element) { m_storage->erase(element); }
	void pop_front() { m_storage->pop_front(); }
	void pop_back() { m_storage->pop_back(); }

	void swap(sequence_implementation& other)
	{
		std::swap(m_storage, other.m_storage);
	}

protected:

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		return m_storage->add_at(pos, std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		m_storage->add_front(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		m_storage->add_back(std::forward<ARGS>(args)...);
	}
	auto data_begin() { return m_storage ? m_storage->data_begin() : nullptr; }
	auto data_end() { return m_storage ? m_storage->data_end() : nullptr; }
	auto data_begin() const { return m_storage ? m_storage->data_begin() : nullptr; }
	auto data_end() const { return m_storage ? m_storage->data_end() : nullptr; }
	auto capacity_begin() const { return m_storage ? m_storage->capacity_begin() : nullptr; }
	auto capacity_end() const { return m_storage ? m_storage->capacity_end() : nullptr; }

	void reallocate(size_t new_capacity, size_t new_size)
	{
		throw std::bad_alloc();
	}

private:

	std::unique_ptr<storage_type> m_storage;
};

// VARIABLE storage.

template<typename T, sequence_traits TRAITS>
class sequence_implementation<sequence_storage_lits::VARIABLE, T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;

public:

	size_t capacity() const { return m_storage.capacity(); }
	size_t size() const { return m_storage.size(); }
	size_t max_size() const { return std::numeric_limits<size_t>::max(); }

	void clear() { m_storage.clear(); }
	void erase(value_type* begin, value_type* end) { m_storage.erase(begin, end); }
	void erase(value_type* element) { m_storage.erase(element); }
	void pop_front() { m_storage.pop_front(); }
	void pop_back() { m_storage.pop_back(); }

protected:

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args) { return m_storage.add_at(pos, std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	void add_front(ARGS&&... args) { m_storage.add_front(std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	void add_back(ARGS&&... args) { m_storage.add_back(std::forward<ARGS>(args)...); }

	auto data_begin() { return m_storage.data_begin(); }
	auto data_end() { return m_storage.data_end(); }
	auto data_begin() const { return m_storage.data_begin(); }
	auto data_end() const { return m_storage.data_end(); }
	auto capacity_begin() const { return m_storage.capacity_begin(); }
	auto capacity_end() const { return m_storage.capacity_end(); }

	void reallocate(size_t new_capacity, size_t new_size)
	{
		m_storage.reallocate(new_capacity, size(), new_size, data_begin(), data_end());
	}

private:

	dynamic_sequence_storage<TRAITS.location, T, TRAITS> m_storage;
};

// BUFFERED storage supporting a small object buffer optimization (like boost::small_vector).

template<typename T, sequence_traits TRAITS>
class sequence_implementation<sequence_storage_lits::BUFFERED, T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using fixed_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;		// STC
	using dynamic_type = dynamic_sequence_storage<TRAITS.location, T, TRAITS>;	// DYN
	enum { STC, DYN };

public:

	size_t capacity() const { return m_storage.index() == STC ? get<STC>(m_storage).capacity() : get<DYN>(m_storage).capacity(); }
	size_t size() const { return m_storage.index() == STC ? get<STC>(m_storage).size() : get<DYN>(m_storage).size(); }
	size_t max_size() const { return std::numeric_limits<size_t>::max(); }

	void clear()
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).clear();
		else
		{
			destroy_data(get<DYN>(m_storage).data_begin(), get<DYN>(m_storage).data_end());
			m_storage.emplace<STC>();
		}
	}
	void erase(value_type* begin, value_type* end)
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).erase(begin, end);
		else
			get<DYN>(m_storage).erase(begin, end);
	}
	void erase(value_type* element)
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).erase(element);
		else
			get<DYN>(m_storage).erase(element);
	}
	auto pop_front() { return m_storage.index() == STC ? get<STC>(m_storage).pop_front() : get<DYN>(m_storage).pop_front(); }
	auto pop_back() { return m_storage.index() == STC ? get<STC>(m_storage).pop_back() : get<DYN>(m_storage).pop_back(); }

protected:

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		if (m_storage.index() == STC)
			return get<STC>(m_storage).add_at(pos, std::forward<ARGS>(args)...);
		else
			return get<DYN>(m_storage).add_at(pos, std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).add_front(std::forward<ARGS>(args)...);
		else
			get<DYN>(m_storage).add_front(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).add_back(std::forward<ARGS>(args)...);
		else
			get<DYN>(m_storage).add_back(std::forward<ARGS>(args)...);
	}

	auto data_begin() { return m_storage.index() == STC ? get<STC>(m_storage).data_begin() : get<DYN>(m_storage).data_begin(); }
	auto data_end() { return m_storage.index() == STC ? get<STC>(m_storage).data_end() : get<DYN>(m_storage).data_end(); }
	auto data_begin() const { return m_storage.index() == STC ? get<STC>(m_storage).data_begin() : get<DYN>(m_storage).data_begin(); }
	auto data_end() const { return m_storage.index() == STC ? get<STC>(m_storage).data_end() : get<DYN>(m_storage).data_end(); }
	auto capacity_begin() const { return m_storage.index() == STC ? get<STC>(m_storage).capacity_begin() : get<DYN>(m_storage).capacity_begin(); }
	auto capacity_end() const { return m_storage.index() == STC ? get<STC>(m_storage).capacity_end() : get<DYN>(m_storage).capacity_end(); }

	void reallocate(size_t new_capacity, size_t new_size)
	{
		// The new capacity will not fit in the buffer.
		if (new_capacity > TRAITS.capacity)
		{
			// We're moving out of the buffer: switch to dynamic storage.
			if (m_storage.index() == STC)
			{
				dynamic_type new_storage;
				new_storage.reallocate(new_capacity, get<STC>(m_storage).size(), new_size,
									   get<STC>(m_storage).data_begin(), get<STC>(m_storage).data_end());
				m_storage = std::move(new_storage);
			}
			// We're already out of the buffer: adjust the dynamic capacity.
			else						
				get<DYN>(m_storage).reallocate(new_capacity, get<DYN>(m_storage).size(), new_size,
											   get<DYN>(m_storage).data_begin(), get<DYN>(m_storage).data_end());
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
				std::uninitialized_move(temp_storage.data_begin(), temp_storage.data_end(), get<STC>(m_storage).data_begin());
				destroy_data(temp_storage.data_begin(), temp_storage.data_end());
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
	using inherited::add_at;
	using inherited::add_front;
	using inherited::add_back;

public:

	using value_type = T;
	using reference = value_type&;
	using const_reference = const value_type&;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	using inherited::size;
	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;
	using inherited::erase;

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

	// A fixed capacity of any kind requires that the size type can represent a count up to the fixed capacity size.
	static_assert(traits.storage == sequence_storage_lits::VARIABLE ||
				  traits.capacity <= std::numeric_limits<size_type>::max(),
				  "Size type is insufficient to hold requested capacity.");

	sequence() = default;
	sequence(const sequence&) = default;
	sequence(sequence&&) = default;
	sequence(std::initializer_list<value_type> il) : sequence{}
	{
		reserve(il.size());
		for (auto&& i : il)
			add_back(i);
	}
	~sequence()
	{
		destroy_data(data_begin(), data_end());
	}

	sequence& operator=(const sequence&) = default;
	sequence& operator=(sequence&&) = default;

	iterator				begin() { return data_begin(); }
	const_iterator			begin() const { return data_begin(); }
	iterator				end() { return data_end(); }
	const_iterator			end() const { return data_end(); }
	reverse_iterator		rbegin() { return reverse_iterator(data_end()); }
	const_reverse_iterator	rbegin() const { return const_reverse_iterator(data_end()); }
	reverse_iterator		rend() { return reverse_iterator(data_begin()); }
	const_reverse_iterator	rend() const { return const_reverse_iterator(data_begin()); }

	const_iterator			cbegin() const { return data_begin(); }
	const_iterator			cend() const { return data_end(); }
	const_reverse_iterator	crbegin() const { return data_begin(); }
	const_reverse_iterator	crend() const { return data_end(); }

	value_type*				data() { return data_begin(); }
	const value_type*		data() const { return data_begin(); }

	value_type& front() { return *data_begin(); }
	value_type& back() { return *(data_end() - 1); }
	const value_type& front() const { return *data_begin(); }
	const value_type& back() const { return *(data_end() - 1); }

	value_type& at(size_t index)
	{
		if (index >= size()) throw std::out_of_range(std::format(OUT_OF_RANGE_ERROR, index));
		return *(data_begin() + index);
	}
	const value_type& at(size_t index) const
	{
		if (index >= size()) throw std::out_of_range(std::format(OUT_OF_RANGE_ERROR, index));
		return *(data_begin() + index);
	}
	value_type& operator[](size_t index) { return *(data_begin() + index); }
	const value_type& operator[](size_t index) const { return *(data_begin() + index); }

	bool empty() const { return size() == 0; }

	void reserve(size_t new_capacity)
	{
		if (new_capacity > capacity())
			reallocate(new_capacity, size());
	}
	void shrink_to_fit()
	{
		if (auto current_size = size(); current_size < capacity())
			reallocate(current_size, current_size);
	}

	template< class... ARGS >
	iterator emplace(const_iterator cpos, ARGS&&... args)
	{
		if (auto old_capacity = capacity(), current_size = size(); current_size == old_capacity)
		{
			size_t index = cpos - data_begin();
			reallocate(traits.grow(old_capacity), current_size);
			cpos = data_begin() + index;
		}
		return add_at(const_cast<iterator>(cpos), std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void emplace_front(ARGS&&... args)
	{
		if (auto old_capacity = capacity(), current_size = size(); current_size == old_capacity)
			reallocate(traits.grow(old_capacity), current_size);
		add_front(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void emplace_back(ARGS&&... args)
	{
		if (auto old_capacity = capacity(), current_size = size(); current_size == old_capacity)
			reallocate(traits.grow(old_capacity), current_size);
		add_back(std::forward<ARGS>(args)...);
	}

	iterator insert(const_iterator cpos, const_reference e) { return emplace(cpos, e); }
	void push_front(const_reference e) { emplace_front(e); }
	void push_back(const_reference e) { emplace_back(e); }

	template<typename... ARGS>
	void resize(size_t new_size, ARGS&&... args)
	{
		auto old_size = size();

		if (new_size < old_size)
			erase(data_end() - (old_size - new_size), data_end());
		else if (new_size > old_size)
		{
			if (new_size > capacity())
				reallocate(new_size, new_size);
			while (new_size-- > old_size)
				add_back(std::forward<ARGS>(args)...);
		}
	}

private:

	constexpr static auto OUT_OF_RANGE_ERROR = "invalid sequence index {}";
};
