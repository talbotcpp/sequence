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
		if (cap < capacity) return capacity;
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

	size_t front_gap(size_t cap, size_t size) const
	{
		switch (location)
		{
		default:
		case sequence_location_lits::FRONT:		return 0;
		case sequence_location_lits::BACK:		return cap - size;
		case sequence_location_lits::MIDDLE:	return (cap - size) / 2;
		}
	}
	size_t front_gap(size_t size = 0) const
	{
		return front_gap(capacity, size);
	}
};


// ==============================================================================================================
// Utility functions

// The add functions implement the algorithms for adding an element at the front
// or back. These algorithms are used for both fixed and dynamic storage.

template<typename T, std::regular_invocable FUNC, typename... ARGS>
T* back_add_at(T* dst, T* pos, FUNC adjust, ARGS&&... args)
{
	T temp(std::forward<ARGS>(args)...);
	new(dst) T(std::move(*(dst - 1)));
	adjust();
	for (auto src = --dst - 1; dst != pos;)
		*dst-- = std::move(*src--);
	*pos = std::move(temp);
	return pos;
}

template<typename T, std::regular_invocable FUNC, typename... ARGS>
T* front_add_at(T* dst, T* pos, FUNC adjust, ARGS&&... args)
{
	T temp(std::forward<ARGS>(args)...);
	new(dst - 1) T(std::move(*dst));
	adjust();
	--pos;
	for (auto src = dst + 1; dst != pos;)
		*dst++ = std::move(*src++);
	*pos = std::move(temp);
	return pos;
}

// The destroy_data function encapsulates calling the element destructors. It is called
// in the sequence destructor and elsewhere when elements are either going away or have
// been moved somewhere else.

template<typename T>
void destroy_data(T* data_begin, T* data_end)
{
	for (auto&& element : span<T>(data_begin, data_end))
		element.~T();
}

// The erase functions implement the erase element and erase range algorithms for front
// and back erasure. These algorithms are used for both fixed and dynamic storage.

template<typename T, std::regular_invocable<size_t> FUNC>
void front_erase(T* data_begin, T* data_end, T* erase_begin, T* erase_end, FUNC adjust)
{
	assert(erase_begin >= data_begin);
	assert(erase_end <= data_end);
	assert(erase_end >= erase_begin);

	if (erase_begin != erase_end)
	{
		auto beg = data_begin - 1;
		auto dst = erase_end - 1;
		auto src = erase_begin - 1;
		while (src != beg)
			*dst-- = std::move(*src--);
		adjust(erase_end - erase_begin);
		destroy_data(data_begin, ++dst);
	}
}

template<typename T, std::regular_invocable FUNC>
void front_erase(T* data_begin, T* data_end, T* dst, FUNC adjust)
{
	assert(dst >= data_begin && dst < data_end);

	auto src = dst - 1;
	while (dst != data_begin)
		*dst-- = std::move(*src--);
	adjust();
	dst->~T();
}

template<typename T, std::regular_invocable<size_t> FUNC>
void back_erase(T* data_begin, T* data_end, T* erase_begin, T* erase_end, FUNC adjust)
{
	assert(erase_begin >= data_begin);
	assert(erase_end <= data_end);
	assert(erase_end >= erase_begin);

	if (erase_begin != erase_end)
	{
		auto dst = erase_begin;
		auto src = erase_end;
		while (src != data_end)
			*dst++ = std::move(*src++);
		adjust(erase_end - erase_begin);
		destroy_data(dst, data_end);
	}
}

template<typename T, std::regular_invocable FUNC>
void back_erase(T* data_begin, T* data_end, T* dst, FUNC adjust)
{
	assert(dst >= data_begin && dst < data_end);

	auto src = dst + 1;
	while (src != data_end)
		*dst++ = std::move(*src++);
	adjust();
	dst->~T();
}

// The recenter function shifts the elements in a MIDDLE location capacity to prepare for size growth.
// If the remaining space is odd, then the extra space will be at the front if we are making space at
// the front, otherwise it will be at the back. It returns the new front and back gaps.

template<typename CAPACITY, typename T>
std::pair<size_t, size_t> recenter(T* capacity_begin, T* capacity_end, T* data_begin, T* data_end)
{
	assert(data_begin == capacity_begin || data_end == capacity_end);
	assert(data_begin != capacity_begin || data_end != capacity_end);

	auto capacity = capacity_end - capacity_begin;
	auto size = data_end - data_begin;

	CAPACITY temp(size);
	std::uninitialized_move(data_begin, data_end, temp.capacity_begin());
	destroy_data(data_begin, data_end);

	auto fg = sequence_traits{.location = sequence_location_lits::MIDDLE}.front_gap(capacity, size);
	auto bg = capacity - (fg + size);
	if (data_begin == capacity_begin) std::swap(fg, bg);
	std::uninitialized_move(temp.capacity_begin(), temp.capacity_begin() + size, capacity_begin + fg);

	return {fg, bg};
}


// ==============================================================================================================
// Concepts

template<typename T>
concept sequence_storage = requires(T& t)
{
    t.capacity_begin();
    t.capacity_end();
    t.data_begin();
    t.data_end();
    t.size();
};


// ==============================================================================================================
// Fixed capacity storage classes.

// fixed_capacity - This is the base class for fixed_sequence_storage instantiations. It handles the capacity.

template<typename T, size_t CAP> requires (CAP != 0)
class fixed_capacity
{
	using value_type = T;

public:

	fixed_capacity() {}
	fixed_capacity(size_t cap) {}
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


// fixed_sequence_storage - This class provides the 3 different element management strategies
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
	template<sequence_storage SEQ>
	fixed_sequence_storage(SEQ&& rhs)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_size = static_cast<size_type>(rhs.size());
	}
	~fixed_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	fixed_sequence_storage& operator=(const fixed_sequence_storage& rhs)
	{
		clear();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_size = rhs.m_size;
		return *this;
	}
	fixed_sequence_storage& operator=(fixed_sequence_storage&& rhs)
	{
		clear();
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_size = rhs.m_size;
		return *this;
	}

	value_type* data_begin() { return capacity_begin(); }
	value_type* data_end() { return capacity_begin() + m_size; }
	const value_type* data_begin() const { return capacity_begin(); }
	const value_type* data_end() const { return capacity_begin() + m_size; }
	size_t size() const { return m_size; }

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (size() == 0 || pos == data_end())
			add_back(std::forward<ARGS>(args)...);
		else
			pos = back_add_at(data_end(), pos, [this](){ ++m_size; }, std::forward<ARGS>(args)...);
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
	template<typename... ARGS>
	void add(size_t count, ARGS&&... args)
	{
		while (count--)
			add_back(std::forward<ARGS>(args)...);
	}
	template<std::ranges::sized_range R>
	void fill(const R& range)
	{
		assert(range.size() <= capacity());
		assert(size() == 0);

		std::uninitialized_copy(range.begin(), range.end(), capacity_begin());
		m_size = static_cast<size_type>(range.size());
	}

	void clear()
	{
		auto end = data_end();
		m_size = 0;
		destroy_data(data_begin(), end);
	}
	void erase(value_type* erase_begin, value_type* erase_end)
	{
		back_erase(data_begin(), data_end(), erase_begin, erase_end,
				   [this](size_t count){ m_size -= static_cast<size_type>(count); });
	}
	void erase(value_type* element)
	{
		back_erase(data_begin(), data_end(), element, [this](){ --m_size; });
	}
	void pop_front()
	{
		assert(size());

		erase(data_begin());
	}
	void pop_back()
	{
		assert(size());

		--m_size;
		data_end()->~value_type();
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
	template<sequence_storage SEQ>
	fixed_sequence_storage(SEQ&& rhs)
	{
		m_size = static_cast<size_type>(rhs.size());
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_end() - m_size);
	}
	~fixed_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	fixed_sequence_storage& operator=(const fixed_sequence_storage& rhs)
	{
		clear();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_end() - rhs.m_size);
		m_size = rhs.m_size;
		return *this;
	}
	fixed_sequence_storage& operator=(fixed_sequence_storage&& rhs)
	{
		clear();
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_end() - rhs.m_size);
		m_size = rhs.m_size;
		return *this;
	}

	value_type* data_begin() { return capacity_end() - m_size; }
	value_type* data_end() { return capacity_end(); }
	const value_type* data_begin() const { return capacity_end() - m_size; }
	const value_type* data_end() const { return capacity_end(); }
	size_t size() const { return m_size; }

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (m_size == 0 || pos == data_begin())
			add_front(std::forward<ARGS>(args)...);
		else
			pos = front_add_at(data_begin(), pos, [this](){ ++m_size; }, std::forward<ARGS>(args)...);
		return pos;
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		assert(size() < capacity());

		new(data_begin() - 1) value_type(std::forward<ARGS>(args)...);
		++m_size;
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		add_at(data_end(), std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add(size_t count, ARGS&&... args)
	{
		while (count--)
			add_front(std::forward<ARGS>(args)...);
	}
	template<std::ranges::sized_range R>
	void fill(const R& range)
	{
		assert(range.size() <= capacity());
		assert(size() == 0);

		std::uninitialized_copy(range.begin(), range.end(), capacity_end() - range.size());
		m_size = static_cast<size_type>(range.size());
	}

	void clear()
	{
		auto begin = data_begin();
		m_size = 0;
		destroy_data(begin, data_end());
	}
	void erase(value_type* erase_begin, value_type* erase_end)
	{
		front_erase(data_begin(), data_end(), erase_begin, erase_end,
					[this](size_t count){ m_size -= static_cast<size_type>(count); });
	}
	void erase(value_type* element)
	{
		front_erase(data_begin(), data_end(), element, [this](){ --m_size; });
	}
	void pop_front()
	{
		assert(size());

		auto dst = data_begin();
		--m_size;
		dst->~value_type();
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
	template<sequence_storage SEQ>
	fixed_sequence_storage(SEQ&& rhs)
	{
		auto size = rhs.size();
		auto offset = TRAITS.front_gap(size);
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin() + offset);
		m_front_gap = static_cast<size_type>(offset);
		m_back_gap = static_cast<size_type>(TRAITS.capacity - (m_front_gap + size));
	}
	~fixed_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	fixed_sequence_storage& operator=(const fixed_sequence_storage& rhs)
	{
		clear();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin() + rhs.m_front_gap);
		m_front_gap = rhs.m_front_gap;
		m_back_gap = rhs.m_back_gap;
		return *this;
	}
	fixed_sequence_storage& operator=(fixed_sequence_storage&& rhs)
	{
		clear();
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin() + rhs.m_front_gap);
		m_front_gap = rhs.m_front_gap;
		m_back_gap = rhs.m_back_gap;
		return *this;
	}

	value_type* data_begin() { return capacity_begin() + m_front_gap; }
	value_type* data_end() { return capacity_end() - m_back_gap; }
	const value_type* data_begin() const { return capacity_begin() + m_front_gap; }
	const value_type* data_end() const { return capacity_end() - m_back_gap; }
	size_t size() const { return capacity() - (m_front_gap + m_back_gap); }

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (size() == 0 || pos == data_end())
			add_back(std::forward<ARGS>(args)...);
		else if (pos == data_begin())
			add_front(std::forward<ARGS>(args)...);
		else if (pos - data_begin() >= data_end() - pos)			// Inserting closer to the end--add at back.
		{
			if (m_back_gap == 0)
			{
				recenter();
				pos -= m_back_gap;
			}
			pos = back_add_at(data_end(), pos, [this](){ --m_back_gap; }, std::forward<ARGS>(args)...);
		}
		else										// Inserting closer to the beginning--add at front.
		{
			if (m_front_gap == 0)
			{
				recenter();
				pos += m_front_gap;
			}
			pos = front_add_at(data_begin(), pos, [this](){ --m_front_gap; }, std::forward<ARGS>(args)...);
		}
		return pos;
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_front_gap || m_back_gap);

		if (m_front_gap == 0)
			recenter();
		new(data_begin() - 1) value_type(std::forward<ARGS>(args)...);
		--m_front_gap;
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_front_gap || m_back_gap);

		if (m_back_gap == 0)
			recenter();
		new(data_end()) value_type(std::forward<ARGS>(args)...);
		--m_back_gap;
	}
	template<typename... ARGS>
	void add(size_t count, ARGS&&... args)
	{
		// This is certainly not as optimized as it could be. The question is,
		// how useful is resize for a middle location container?
		while (count--)
			add_back(std::forward<ARGS>(args)...);
	}
	template<std::ranges::sized_range R>
	void fill(const R& range)
	{
		assert(range.size() <= capacity());
		assert(size() == 0);

		auto offset = TRAITS.front_gap(range.size());
		std::uninitialized_copy(range.begin(), range.end(), capacity_begin() + offset);
		m_front_gap = static_cast<size_type>(offset);
		m_back_gap = static_cast<size_type>(TRAITS.capacity - (m_front_gap + range.size()));
	}

	void clear()
	{
		auto begin = data_begin();
		auto end = data_end();
		m_front_gap = static_cast<size_type>(TRAITS.front_gap());
		m_back_gap = static_cast<size_type>(TRAITS.capacity - m_front_gap);
		destroy_data(begin, end);
	}
	void erase(value_type* erase_begin, value_type* erase_end)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (erase_begin - data_begin() >= data_end() - erase_end)
			back_erase(data_begin(), data_end(), erase_begin, erase_end,
						[this](size_t count){ m_back_gap += static_cast<size_type>(count); });
		else
			front_erase(data_begin(), data_end(), erase_begin, erase_end,
						[this](size_t count){ m_front_gap += static_cast<size_type>(count); });
	}
	void erase(value_type* element)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (element - data_begin() >= data_end() - element)
			back_erase(data_begin(), data_end(), element, [this](){ ++m_back_gap; });
		else
			front_erase(data_begin(), data_end(), element, [this](){ ++m_front_gap; });
	}
	void pop_front()
	{
		assert(size());

		auto dst = data_begin();
		++m_front_gap;
		dst->~value_type();
	}
	void pop_back()
	{
		assert(size());

		++m_back_gap;
		data_end()->~value_type();
	}

private:

	// This function recenters the elements to prepare for size growth. If the remaining space is odd, then the
	// extra space will be at the front if we are making space at the front, otherwise it will be at the back.
	
	void recenter()
	{
		auto [front_gap, back_gap] = ::recenter<inherited>(capacity_begin(), capacity_end(), data_begin(), data_end());
		m_front_gap = static_cast<size_type>(front_gap);
		m_back_gap = static_cast<size_type>(back_gap);
	}

	// Empty sequences with odd capacity will have the extra space at the back.

	size_type m_front_gap = static_cast<size_type>(TRAITS.front_gap());
	size_type m_back_gap = static_cast<size_type>(TRAITS.capacity - TRAITS.front_gap());
};


// ==============================================================================================================
// dynamic_capacity

template<typename T, sequence_traits TRAITS>
class dynamic_capacity
{
	using value_type = T;
	using pointer = value_type*;
	using const_pointer = const value_type*;

public:

	dynamic_capacity() : m_capacity_end(nullptr) {}
	dynamic_capacity(size_t cap) :
		m_capacity_begin(operator new(sizeof(value_type) * cap))
	{
		m_capacity_end = capacity_begin() + cap;
	}
	dynamic_capacity(const dynamic_capacity&) = delete;
	dynamic_capacity(dynamic_capacity&& rhs) = default;
	dynamic_capacity& operator=(const dynamic_capacity&) = delete;
	dynamic_capacity& operator=(dynamic_capacity&& rhs) = default;

	size_t capacity() const { return capacity_end() - capacity_begin(); }
	pointer capacity_begin() { return static_cast<pointer>(m_capacity_begin.get()); }
	pointer capacity_end() { return m_capacity_end; }
	const_pointer capacity_begin() const { return static_cast<const_pointer>(m_capacity_begin.get()); }
	const_pointer capacity_end() const { return m_capacity_end; }

protected:

	void reallocate(size_t new_cap, size_t offset, pointer data_begin, pointer data_end)
	{
		dynamic_capacity<T, TRAITS> new_capacity(new_cap);
		if (data_begin != data_end)	// This is redundant. Is it actually an optimization?
		{
			std::uninitialized_move(data_begin, data_end, new_capacity.capacity_begin() + offset);
			destroy_data(data_begin, data_end);
		}
		this->swap(new_capacity);
	}

	template<std::ranges::sized_range R>
	void fill(const R& range)
	{
		dynamic_capacity<T, TRAITS> new_capacity(range.size());
		std::uninitialized_copy(range.begin(), range.end(), new_capacity.capacity_begin());
		this->swap(new_capacity);
	}

	void swap(dynamic_capacity& rhs)
	{
		std::swap(m_capacity_begin, rhs.m_capacity_begin);
		std::swap(m_capacity_end, rhs.m_capacity_end);
	}

	using deleter_type = decltype([](void* p){ delete p; });
	std::unique_ptr<void, deleter_type> m_capacity_begin;
	pointer m_capacity_end;
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

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	dynamic_sequence_storage() = default;
	dynamic_sequence_storage(const dynamic_sequence_storage& rhs) : inherited(rhs.size())
	{
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_data_end = capacity_end();
	}
	dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		swap(rhs);
	}
	template<sequence_storage SEQ>
	dynamic_sequence_storage(size_t cap, SEQ&& rhs) :  inherited(cap)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_data_end = capacity_begin() + rhs.size();
	}
	~dynamic_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	dynamic_sequence_storage& operator=(const dynamic_sequence_storage& rhs)
	{
		clear();
		if (rhs.capacity() > capacity())
			inherited::reallocate(rhs.capacity(), 0, nullptr, nullptr);
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_data_end = capacity_begin() + rhs.size();
		return *this;
	}
	dynamic_sequence_storage& operator=(dynamic_sequence_storage&& rhs)
	{
		clear();
		swap(rhs);
		return *this;
	}

	value_type* data_begin() { return capacity_begin(); }
	value_type* data_end() { return m_data_end; }
	const value_type* data_begin() const { return capacity_begin(); }
	const value_type* data_end() const { return m_data_end; }
	size_t size() const { return data_end() - data_begin(); }

	void swap(dynamic_sequence_storage& rhs)
	{
		inherited::swap(rhs);
		std::swap(m_data_end, rhs.m_data_end);
	}

	void reallocate(size_t new_cap)
	{
		assert(size() <= new_cap);

		auto current_size = size();
		inherited::reallocate(new_cap, 0, data_begin(), data_end());
		m_data_end = capacity_begin() + current_size;
	}

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (size() == 0 || pos == data_end())
			add_back(std::forward<ARGS>(args)...);
		else
			pos = back_add_at(data_end(), pos, [this](){ ++m_data_end; }, std::forward<ARGS>(args)...);
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
	template<typename... ARGS>
	void add(size_t count, ARGS&&... args)
	{
		while (count--)
			add_back(std::forward<ARGS>(args)...);
	}
	template<std::ranges::range R>
	void fill(const R& range)
	{
		inherited::fill(range);
		m_data_end = capacity_end();
	}

	void clear()
	{
		auto end = data_end();
		m_data_end = capacity_begin();
		destroy_data(data_begin(), end);
	}
	void erase(value_type* erase_begin, value_type* erase_end)
	{
		back_erase(data_begin(), data_end(), erase_begin, erase_end,
				   [this](size_t count){ m_data_end -= count; });
	}
	void erase(value_type* element)
	{
		back_erase(data_begin(), data_end(), element, [this](){ --m_data_end; });
	}
	void pop_front()
	{
		assert(size());

		erase(data_begin());
	}
	void pop_back()
	{
		assert(size());

		--m_data_end;
		data_end()->~value_type();
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

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	dynamic_sequence_storage() = default;
	dynamic_sequence_storage(const dynamic_sequence_storage& rhs) : inherited(rhs.size())
	{
		m_data_begin = capacity_begin();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), m_data_begin);
	}
	dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		swap(rhs);
	}
	template<sequence_storage SEQ>
	dynamic_sequence_storage(size_t cap, SEQ&& rhs) :  inherited(cap)
	{
		m_data_begin = capacity_end() - rhs.size();
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), m_data_begin);
	}
	~dynamic_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	dynamic_sequence_storage& operator=(const dynamic_sequence_storage& rhs)
	{
		clear();
		if (rhs.capacity() > capacity())
			inherited::reallocate(rhs.capacity(), 0, nullptr, nullptr);
		auto begin = capacity_end() - rhs.size();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), begin);
		m_data_begin = begin;
		return *this;
	}
	dynamic_sequence_storage& operator=(dynamic_sequence_storage&& rhs)
	{
		clear();
		swap(rhs);
		return *this;
	}

	value_type* data_begin() { return m_data_begin; }
	value_type* data_end() { return capacity_end(); }
	const value_type* data_begin() const { return m_data_begin; }
	const value_type* data_end() const { return capacity_end(); }
	size_t size() const { return data_end() - data_begin(); }

	void swap(dynamic_sequence_storage& rhs)
	{
		inherited::swap(rhs);
		std::swap(m_data_begin, rhs.m_data_begin);
	}

	void reallocate(size_t new_cap)
	{
		assert(size() <= new_cap);

		auto current_size = size();
		inherited::reallocate(new_cap, TRAITS.front_gap(new_cap, current_size), data_begin(), data_end());
		m_data_begin = capacity_end() - current_size;
	}

	template<typename... ARGS>
	iterator add_at(value_type* pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (size() == 0 || pos == data_begin())
			add_front(std::forward<ARGS>(args)...);
		else
			pos = front_add_at(data_begin(), pos, [this](){ --m_data_begin; }, std::forward<ARGS>(args)...);
		return pos;
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		assert(size() < capacity());

		new(data_begin() - 1) value_type(std::forward<ARGS>(args)...);
		--m_data_begin;
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		add_at(data_end(), std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	void add(size_t count, ARGS&&... args)
	{
		while (count--)
			add_front(std::forward<ARGS>(args)...);
	}
	template<std::ranges::range R>
	void fill(const R& range)
	{
		inherited::fill(range);
		m_data_begin = capacity_begin();
	}

	void clear()
	{
		auto begin = data_begin();
		m_data_begin = capacity_end();
		destroy_data(begin, data_end());
	}
	void erase(value_type* erase_begin, value_type* erase_end)
	{
		front_erase(data_begin(), data_end(), erase_begin, erase_end,
					[this](size_t count){ m_data_begin += count; });
	}
	void erase(value_type* element)
	{
		front_erase(data_begin(), data_end(), element, [this](){ ++m_data_begin; });
	}
	void pop_front()
	{
		assert(size());

		auto dst = data_begin();
		++m_data_begin;
		dst->~value_type();
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

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	dynamic_sequence_storage() = default;
	dynamic_sequence_storage(const dynamic_sequence_storage& rhs) : inherited(rhs.size())
	{
		m_data_begin = capacity_begin();
		m_data_end = capacity_end();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), m_data_begin);
	}
	dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		swap(rhs);
	}
	template<sequence_storage SEQ>
	dynamic_sequence_storage(size_t cap, SEQ&& rhs) :  inherited(cap)
	{
		auto size = rhs.size();
		m_data_begin = capacity_begin() + TRAITS.front_gap(capacity(), size);
		m_data_end = m_data_begin + size;
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), m_data_begin);
	}
	~dynamic_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	dynamic_sequence_storage& operator=(const dynamic_sequence_storage& rhs)
	{
		clear();
		if (rhs.capacity() > capacity())
			inherited::reallocate(rhs.capacity(), 0, nullptr, nullptr);
		auto begin = capacity_begin() + TRAITS.front_gap(capacity(), rhs.size());
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), begin);
		m_data_begin = begin;
		m_data_end = m_data_begin + rhs.size();
		return *this;
	}
	dynamic_sequence_storage& operator=(dynamic_sequence_storage&& rhs)
	{
		clear();
		swap(rhs);
		return *this;
	}

	value_type* data_begin() { return m_data_begin; }
	value_type* data_end() { return m_data_end; }
	const value_type* data_begin() const { return m_data_begin; }
	const value_type* data_end() const { return m_data_end; }
	size_t size() const { return m_data_end - m_data_begin; }

	void swap(dynamic_sequence_storage& rhs)
	{
		inherited::swap(rhs);
		std::swap(m_data_begin, rhs.m_data_begin);
		std::swap(m_data_end, rhs.m_data_end);
	}

	void reallocate(size_t new_cap)
	{
		assert(size() <= new_cap);

		auto current_size = size();
		auto offset = TRAITS.front_gap(new_cap, current_size);
		inherited::reallocate(new_cap, offset, data_begin(), data_end());
		m_data_begin = capacity_begin() + offset;
		m_data_end = m_data_begin + current_size;
	}

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		auto dbeg = data_begin();
		auto dend = data_end();

		if (size() == 0 || pos == dend)
			add_back(std::forward<ARGS>(args)...);
		else if (pos == dbeg)
			add_front(std::forward<ARGS>(args)...);

		else if (pos - dbeg >= dend - pos)			// Inserting closer to the end--add at back.
		{
			if (m_data_end == capacity_end())
			{
				recenter();
				pos -= capacity_end() - m_data_end;
			}
			pos = back_add_at(data_end(), pos, [this](){ ++m_data_end; }, std::forward<ARGS>(args)...);
		}
		else										// Inserting closer to the beginning--add at front.
		{
			if (m_data_begin == capacity_begin())
			{
				recenter();
				pos += m_data_begin - capacity_begin();
			}
			pos = front_add_at(data_begin(), pos, [this](){ --m_data_begin; }, std::forward<ARGS>(args)...);
		}
		return pos;
	}
	template<typename... ARGS>
	void add_front(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_data_begin > capacity_begin() || m_data_end < capacity_end());

		if (m_data_begin == capacity_begin())
			recenter();
		new(m_data_begin - 1) value_type(std::forward<ARGS>(args)...);
		--m_data_begin;
	}
	template<typename... ARGS>
	void add_back(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_data_begin > capacity_begin() || m_data_end < capacity_end());

		if (m_data_end == capacity_end())
			recenter();
		new(m_data_end) value_type(std::forward<ARGS>(args)...);
		++m_data_end;
	}
	template<typename... ARGS>
	void add(size_t count, ARGS&&... args)
	{
		// This is certainly not as optimized as it could be. The question is,
		// how useful is resize for a middle location container?
		while (count--)
			add_back(std::forward<ARGS>(args)...);
	}
	template<std::ranges::sized_range R>
	void fill(R range)
	{
		inherited::fill(range);
		m_data_begin = capacity_begin();
		m_data_end = capacity_end();
	}

	void clear()
	{
		auto begin = data_begin();
		auto end = data_end();
		m_data_begin = capacity_begin() + TRAITS.front_gap(capacity(), 0);
		m_data_end = m_data_begin;
		destroy_data(begin, end);
	}
	void erase(value_type* erase_begin, value_type* erase_end)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (erase_begin - data_begin() >= data_end() - erase_end)
			back_erase(data_begin(), data_end(), erase_begin, erase_end,
						[this](size_t count){ m_data_end -= count; });
		else
			front_erase(data_begin(), data_end(), erase_begin, erase_end,
						[this](size_t count){ m_data_begin += count; });
	}
	void erase(value_type* element)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (element - data_begin() >= data_end() - element)
			back_erase(data_begin(), data_end(), element, [this](){ --m_data_end; });
		else
			front_erase(data_begin(), data_end(), element, [this](){ ++m_data_begin; });
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

	// This function recenters the elements to prepare for size growth. If the remaining space is odd, then the
	// extra space will be at the front if we are making space at the front, otherwise it will be at the back.
	
	void recenter()
	{
		auto [front_gap, back_gap] = ::recenter<inherited>(capacity_begin(), capacity_end(), data_begin(), data_end());
		m_data_begin = capacity_begin() + front_gap;
		m_data_end = capacity_end() - back_gap;
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
	bool is_dynamic() const { return false; }

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
	template<typename... ARGS>
	void add(size_t new_size, ARGS&&... args)
	{
		m_storage.add(new_size, std::forward<ARGS>(args)...);
	}
	void fill(std::initializer_list<value_type> il)
	{
		m_storage.fill(il);
	}

	auto data_begin() { return m_storage.data_begin(); }
	auto data_end() { return m_storage.data_end(); }
	auto data_begin() const { return m_storage.data_begin(); }
	auto data_end() const { return m_storage.data_end(); }
	auto capacity_begin() const { return m_storage.capacity_begin(); }
	auto capacity_end() const { return m_storage.capacity_end(); }

	void reallocate(size_t new_capacity)
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
	bool is_dynamic() const { return true; }

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
	template<typename... ARGS>
	void add(size_t new_size, ARGS&&... args)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		m_storage->add(new_size, std::forward<ARGS>(args)...);
	}
	void fill(std::initializer_list<value_type> il)
	{
		assert(!m_storage);

		m_storage.reset(new storage_type);
		m_storage->fill(il);
	}

	auto data_begin() { return m_storage ? m_storage->data_begin() : nullptr; }
	auto data_end() { return m_storage ? m_storage->data_end() : nullptr; }
	auto data_begin() const { return m_storage ? m_storage->data_begin() : nullptr; }
	auto data_end() const { return m_storage ? m_storage->data_end() : nullptr; }
	auto capacity_begin() const { return m_storage ? m_storage->capacity_begin() : nullptr; }
	auto capacity_end() const { return m_storage ? m_storage->capacity_end() : nullptr; }

	void reallocate(size_t new_capacity)
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
	bool is_dynamic() const { return true; }

	void clear() { m_storage.clear(); }
	void erase(value_type* begin, value_type* end) { m_storage.erase(begin, end); }
	void erase(value_type* element) { m_storage.erase(element); }
	void pop_front() { m_storage.pop_front(); }
	void pop_back() { m_storage.pop_back(); }

	void swap(sequence_implementation& other)
	{
		m_storage.swap(other.m_storage);
	}

protected:

	template<typename... ARGS>
	iterator add_at(iterator pos, ARGS&&... args) { return m_storage.add_at(pos, std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	void add_front(ARGS&&... args) { m_storage.add_front(std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	void add_back(ARGS&&... args) { m_storage.add_back(std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	void add(size_t new_size, ARGS&&... args) { m_storage.add(new_size, std::forward<ARGS>(args)...); }
	void fill(std::initializer_list<value_type> il) { m_storage.fill(il); }

	auto data_begin() { return m_storage.data_begin(); }
	auto data_end() { return m_storage.data_end(); }
	auto data_begin() const { return m_storage.data_begin(); }
	auto data_end() const { return m_storage.data_end(); }
	auto capacity_begin() const { return m_storage.capacity_begin(); }
	auto capacity_end() const { return m_storage.capacity_end(); }

	void reallocate(size_t new_capacity)
	{
		m_storage.reallocate(new_capacity);
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
	bool is_dynamic() const { return m_storage.index() == DYN; }

	void clear()
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).clear();
		else
			m_storage.emplace<STC>();
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
	void pop_front()
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).pop_front();
		else
			get<DYN>(m_storage).pop_front();
	}
	void pop_back()
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).pop_back();
		else
			get<DYN>(m_storage).pop_back();
	}

	void swap(sequence_implementation& other)
	{
		auto swap_mixed = [](sequence_implementation& stc, sequence_implementation& dyn)
		{
			assert(stc.m_storage.index() == STC);
			assert(dyn.m_storage.index() == DYN);

			dynamic_type temp = std::move(get<DYN>(dyn.m_storage));
			dyn.m_storage.emplace<STC>(std::move(get<STC>(stc.m_storage)));
			stc.m_storage.emplace<DYN>(std::move(temp));
		};

		if (m_storage.index() == STC)
		{
			if (other.m_storage.index() == STC)
				std::swap(get<STC>(m_storage), get<STC>(other.m_storage));
			else
				swap_mixed(*this, other);
		}
		else
		{
			if (other.m_storage.index() == STC)
				swap_mixed(other, *this);
			else
				get<DYN>(m_storage).swap(get<DYN>(other.m_storage));
		}
	}

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
	template<typename... ARGS>
	void add(size_t new_size, ARGS&&... args)
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).add(new_size, std::forward<ARGS>(args)...);
		else
			get<DYN>(m_storage).add(new_size, std::forward<ARGS>(args)...);
	}
	void fill(std::initializer_list<value_type> il)
	{
		assert(m_storage.index() == STC);
		assert(get<STC>(m_storage).size() == 0);

		if (il.size() <= TRAITS.capacity)
			get<STC>(m_storage).fill(il);
		else
		{
			m_storage.emplace<DYN>();
			get<DYN>(m_storage).fill(il);
		}
	}

	auto data_begin() { return m_storage.index() == STC ? get<STC>(m_storage).data_begin() : get<DYN>(m_storage).data_begin(); }
	auto data_end() { return m_storage.index() == STC ? get<STC>(m_storage).data_end() : get<DYN>(m_storage).data_end(); }
	auto data_begin() const { return m_storage.index() == STC ? get<STC>(m_storage).data_begin() : get<DYN>(m_storage).data_begin(); }
	auto data_end() const { return m_storage.index() == STC ? get<STC>(m_storage).data_end() : get<DYN>(m_storage).data_end(); }
	auto capacity_begin() const { return m_storage.index() == STC ? get<STC>(m_storage).capacity_begin() : get<DYN>(m_storage).capacity_begin(); }
	auto capacity_end() const { return m_storage.index() == STC ? get<STC>(m_storage).capacity_end() : get<DYN>(m_storage).capacity_end(); }

	void reallocate(size_t new_capacity)
	{
		// The new capacity will not fit in the buffer.
		if (new_capacity > TRAITS.capacity)
		{
			if (m_storage.index() == STC)		// We're moving out of the buffer: switch to dynamic storage.
				m_storage = dynamic_type(new_capacity, get<STC>(m_storage));
			else								// We're already out of the buffer: adjust the dynamic capacity.		
				get<DYN>(m_storage).reallocate(new_capacity);
		}

		// The new capacity will fit in the buffer.
		else if (m_storage.index() == DYN)		// We're moving into the buffer: switch to buffer storage.
				m_storage.emplace<STC>(dynamic_type(std::move(get<DYN>(m_storage))));
		// If we're already in the buffer: do nothing (the buffer capacity cannot change).
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
	using inherited::add;
	using inherited::fill;

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
				  "Capacity must be greater than 0.");
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
		fill(il);
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
	value_type& operator[](size_t index) & { return *(data_begin() + index); }
	const value_type& operator[](size_t index) const && { return *(data_begin() + index); }

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

	template< class... ARGS >
	iterator emplace(const_iterator cpos, ARGS&&... args)
	{
		if (auto old_capacity = capacity(); size() == old_capacity)
		{
			size_t index = cpos - data_begin();
			reallocate(traits.grow(old_capacity));
			cpos = data_begin() + index;
		}
		return add_at(const_cast<iterator>(cpos), std::forward<ARGS>(args)...);
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
				reallocate(std::max(new_size, traits.capacity));
			add(new_size - old_size, std::forward<ARGS>(args)...);
		}
	}

private:

	constexpr static auto OUT_OF_RANGE_ERROR = "invalid sequence index {}";
};
