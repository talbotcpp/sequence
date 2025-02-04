/*	Sequence.ixx
* 
*	Primary module interface file for general-purpose sequence container.
* 
*	Alan Talbot
*/

export module sequence;

//import std;
import <assert.h>;
import <concepts>;
import <utility>;
import <span>;
import <memory>;
import <variant>;
import <stdexcept>;
import <format>;

// ==============================================================================================================
// Traits - Control structure template parameter.

// sequence_lits - Literals used to specify the values of various sequence traits.
// These are hoisted out of the class template to avoid template dependencies.
// See sequence_traits below for a detailed discussion of these values.

export enum class sequence_storage_lits { STATIC, FIXED, VARIABLE, BUFFERED };	// See sequence_traits::storage.
export enum class sequence_location_lits { FRONT, BACK, MIDDLE };				// See sequence_traits::location.
export enum class sequence_growth_lits { LINEAR, EXPONENTIAL, VECTOR };			// See sequence_traits::growth.

// sequence_traits - Structure used to supply the sequence traits. This is fully documented in the README.md file.

export template<std::unsigned_integral SIZE = size_t>
struct sequence_traits
{
	using size_type = SIZE;

	sequence_storage_lits storage = sequence_storage_lits::VARIABLE;
	sequence_location_lits location = sequence_location_lits::FRONT;
	sequence_growth_lits growth = sequence_growth_lits::VECTOR;

	size_t capacity = 1;
	size_t increment = 1;
	float factor = 1.5;

	constexpr size_t grow(size_t cap) const
	{
		if (cap < capacity) return capacity;
		else switch (growth)
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

	// 'is_variable' returns true iff the capacity can change size
	// (i.e. storage is VARIABLE or BUFFERED).

	constexpr bool is_variable() const { return storage >= sequence_storage_lits::VARIABLE; }

	// 'front_gap' returns the location of the start of the data given a capacity and size.
	// The formula is based on the 'location' value.

	constexpr size_t front_gap(size_t cap, size_t size) const
	{
		switch (location)
		{
		default:
		case sequence_location_lits::FRONT:		return 0;
		case sequence_location_lits::BACK:		return cap - size;
		case sequence_location_lits::MIDDLE:	return (cap - size) / 2;
		}
	}
	constexpr size_t front_gap(size_t size = 0) const
	{
		return front_gap(capacity, size);
	}
};

// ==============================================================================================================
// Utility functions - Not publically available.

namespace {

// This provides the needed additional uninitialized memory function to handle moving of types.

template<typename InpIt, typename FwdIt>
auto uninitialized_move_if_noexcept(InpIt src, InpIt end, FwdIt dst)
{
	using value_type = std::iterator_traits<InpIt>::value_type;
	if constexpr (!std::is_nothrow_move_constructible_v<value_type> && std::is_copy_constructible_v<value_type>)
		return std::uninitialized_copy(src, end, dst);
	else
		return std::uninitialized_move(src, end, dst);
}


// The add functions implement the algorithms for adding an element at the front
// or back. These algorithms are used for both fixed and dynamic storage.

template<typename T, std::regular_invocable FUNC, typename... ARGS>
inline T* back_add_at(T* dst, T* pos, FUNC adjust, ARGS&&... args)
{
	T temp(std::forward<ARGS>(args)...);	// Do this first in case the T ctor throws.
	new(dst) T(std::move(*(dst - 1)));
	adjust();
	for (auto src = --dst - 1; dst != pos;)
		*dst-- = std::move(*src--);
	*pos = std::move(temp);
	return pos;
}

template<typename T, std::regular_invocable FUNC, typename... ARGS>
inline T* front_add_at(T* dst, T* pos, FUNC adjust, ARGS&&... args)
{
	T temp(std::forward<ARGS>(args)...);	// Do this first in case the T ctor throws.
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
inline void destroy_data(T* data_begin, T* data_end)
{
	for (auto&& element : std::span<T>(data_begin, data_end))
		element.~T();
}

// The erase functions implement the erase element and erase range algorithms for front
// and back erasure. These algorithms are used for both fixed and dynamic storage.

template<typename T, std::regular_invocable<size_t> FUNC>
inline void front_erase(T* data_begin, T* data_end, T* erase_begin, T* erase_end, FUNC adjust)
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
inline void front_erase(T* data_begin, T* data_end, T* dst, FUNC adjust)
{
	assert(dst >= data_begin && dst < data_end);

	auto src = dst - 1;
	while (dst != data_begin)
		*dst-- = std::move(*src--);
	adjust();
	dst->~T();
}

template<typename T, std::regular_invocable<size_t> FUNC>
inline void back_erase(T* data_begin, T* data_end, T* erase_begin, T* erase_end, FUNC adjust)
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
inline void back_erase(T* data_begin, T* data_end, T* dst, FUNC adjust)
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
inline std::pair<size_t, size_t> recenter(T* capacity_begin, T* capacity_end, T* data_begin, T* data_end)
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

}

// ==============================================================================================================
// fixed_capacity - This is the base class for fixed_storage instantiations. It handles the raw capacity.

template<typename T, size_t CAP> requires (CAP != 0)
class fixed_capacity
{
	using value_type = T;

public:

	fixed_capacity() {}
	fixed_capacity(size_t cap) {}
	~fixed_capacity() {}

	constexpr static size_t capacity() { return CAP; }

	inline value_type* capacity_begin() { return elements; }
	inline value_type* capacity_end() { return elements + CAP; }
	inline const value_type* capacity_begin() const { return elements; }
	inline const value_type* capacity_end() const { return elements + CAP; }

private:

	union
	{
		value_type elements[CAP];
		unsigned char unused;
	};
};


// ==============================================================================================================
// fixed_storage - This is the location-adjustable base class for the fixed_sequence_storage class.
// It handles the element storage.

template<typename T, sequence_traits TRAITS, sequence_location_lits LOC = TRAITS.location>
class fixed_storage
{
	static_assert(false, "An unimplemented specialization of fixed_storage was instantiated.");
};


template<typename T, sequence_traits TRAITS>
class fixed_storage<T, TRAITS, sequence_location_lits::FRONT> : public fixed_capacity<T, TRAITS.capacity>
{
	using inherited = fixed_capacity<T, TRAITS.capacity>;
	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	inline value_type* data_begin() { return capacity_begin(); }
	inline value_type* data_end() { return capacity_begin() + m_size; }
	inline const value_type* data_begin() const { return capacity_begin(); }
	inline const value_type* data_end() const { return capacity_begin() + m_size; }
	inline size_t size() const { return m_size; }
	inline bool empty() const { return m_size == 0; }

protected:

	auto new_data_start() { return capacity_begin(); }
	auto data_area() const { return m_size; }
	void set_data_area(size_type size) { m_size = size; }
	void clear_data_area() { m_size = 0; }
	void set_size(size_type size) { m_size = size; }

private:

	size_type m_size = 0;
};

// Forward declaration so that the sequence storage type can refer to each other.

template<sequence_location_lits LOC, typename T, sequence_traits TRAITS>
class dynamic_sequence_storage;


// ==============================================================================================================
// fixed_sequence_storage - This class provides the 3 different element management strategies
// for fixed capacity sequences (static or dynamically allocated).

template<sequence_location_lits LOC, typename T, sequence_traits TRAITS>
class fixed_sequence_storage
{
	static_assert(false, "An unimplemented specialization of fixed_sequence_storage was instantiated.");
};

template<typename T, sequence_traits TRAITS>
class fixed_sequence_storage<sequence_location_lits::FRONT, T, TRAITS> : fixed_storage<T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reference = value_type&;
	using size_type = typename decltype(TRAITS)::size_type;
	using inherited = fixed_storage<T, TRAITS>;

	using inherited::new_data_start;
	using inherited::data_area;
	using inherited::set_data_area;
	using inherited::clear_data_area;
	using inherited::set_size;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;
	using inherited::data_begin;
	using inherited::data_end;
	using inherited::size;
	using inherited::empty;

	inline fixed_sequence_storage() = default;
	inline fixed_sequence_storage(std::initializer_list<value_type> il)
	{
		if (il.size() > capacity())
			throw std::bad_alloc();

		std::uninitialized_copy(il.begin(), il.end(), new_data_start());
		set_size(static_cast<size_type>(il.size()));				// This must come last in case of a copy exception.
	}
	fixed_sequence_storage(dynamic_sequence_storage<TRAITS.location, T, TRAITS>&&);

	inline fixed_sequence_storage(const fixed_sequence_storage& rhs)
	{
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), new_data_start());
		set_data_area(rhs.data_area());								// This must come last in case of a copy exception.
	}
	inline fixed_sequence_storage(fixed_sequence_storage&& rhs)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start());
		set_data_area(rhs.data_area());								// This must come last in case of a move exception.
	}

	inline fixed_sequence_storage& operator=(const fixed_sequence_storage& rhs)
	{
		auto dst = data_begin();
		auto src = rhs.data_begin();

		while (src != rhs.data_end() && dst != data_end())
			*dst++ = *src++;
		destroy_data(dst, data_end());

		std::uninitialized_copy(src, rhs.data_end(), dst);
		set_data_area(rhs.data_area());

		return *this;
	}
	inline fixed_sequence_storage& operator=(fixed_sequence_storage&& rhs)
	{
		auto dst = data_begin();
		auto src = rhs.data_begin();

		while (src != rhs.data_end() && dst != data_end())
			*dst++ = std::move(*src++);
		destroy_data(dst, data_end());

		std::uninitialized_move(src, rhs.data_end(), dst);
		set_data_area(rhs.data_area());

		return *this;
	}

	inline ~fixed_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}


	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
	{
		if (size() >= capacity())
			throw std::bad_alloc();

		assert(pos >= data_begin() && pos <= data_end());

		if (size() == 0 || pos == data_end())
			add_back(std::forward<ARGS>(args)...);
		else
			pos = back_add_at(data_end(), pos, [this](){ set_data_area(size() + 1); }, std::forward<ARGS>(args)...);
		return pos;
	}
	template<typename... ARGS>
	inline void add_front(ARGS&&... args)
	{
		add_at(data_begin(), std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		if (size() >= capacity())
			throw std::bad_alloc();

		new(data_end()) value_type(std::forward<ARGS>(args)...);
		set_data_area(size() + 1);
	}
	template<typename... ARGS>
	inline void add(size_t count, ARGS&&... args)
	{
		while (count--)
			add_back(std::forward<ARGS>(args)...);
	}

	inline void clear()
	{
		if (!empty())
		{
			destroy_data(data_begin(), data_end());
			clear_data_area();
		}
	}
	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		back_erase(data_begin(), data_end(), erase_begin, erase_end,
				   [this](size_t count){ set_data_area(size() - static_cast<size_type>(count)); });
	}
	inline void erase(value_type* element)
	{
		back_erase(data_begin(), data_end(), element, [this](){ set_data_area(size() - 1); });
	}
	inline void pop_front()
	{
		assert(size());

		erase(data_begin());
	}
	inline void pop_back()
	{
		assert(size());

		set_data_area(size() - 1);
		data_end()->~value_type();
	}
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

	inline fixed_sequence_storage() = default;
	inline fixed_sequence_storage(std::initializer_list<value_type> il)
	{
		assert(il.size() <= capacity());

		std::uninitialized_copy(il.begin(), il.end(), capacity_end() - il.size());
		m_size = static_cast<size_type>(il.size());
	}

	inline fixed_sequence_storage(const fixed_sequence_storage& rhs)
	{
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_end() - rhs.m_size);
		m_size = rhs.m_size;
	}
	inline fixed_sequence_storage(fixed_sequence_storage&& rhs)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_end() - rhs.m_size);
		m_size = rhs.m_size;
	}

	inline fixed_sequence_storage& operator=(const fixed_sequence_storage& rhs)
	{
		clear();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_end() - rhs.m_size);
		m_size = rhs.m_size;
		return *this;
	}
	inline fixed_sequence_storage& operator=(fixed_sequence_storage&& rhs)
	{
		clear();
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_end() - rhs.m_size);
		m_size = rhs.m_size;
		return *this;
	}

	inline ~fixed_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	inline value_type* data_begin() { return capacity_end() - m_size; }
	inline value_type* data_end() { return capacity_end(); }
	inline const value_type* data_begin() const { return capacity_end() - m_size; }
	inline const value_type* data_end() const { return capacity_end(); }
	inline size_t size() const { return m_size; }
	inline bool empty() const { return m_size == 0; }

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
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
	inline void add_front(ARGS&&... args)
	{
		assert(size() < capacity());

		new(data_begin() - 1) value_type(std::forward<ARGS>(args)...);
		++m_size;
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		add_at(data_end(), std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add(size_t count, ARGS&&... args)
	{
		while (count--)
			add_front(std::forward<ARGS>(args)...);
	}

	inline void clear()
	{
		if (!empty())
		{
			destroy_data(data_begin(), data_end());
			m_size = 0;
		}
	}
	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		front_erase(data_begin(), data_end(), erase_begin, erase_end,
					[this](size_t count){ m_size -= static_cast<size_type>(count); });
	}
	inline void erase(value_type* element)
	{
		front_erase(data_begin(), data_end(), element, [this](){ --m_size; });
	}
	inline void pop_front()
	{
		assert(size());

		auto dst = data_begin();
		--m_size;
		dst->~value_type();
	}
	inline void pop_back()
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

	inline fixed_sequence_storage() = default;
	inline fixed_sequence_storage(std::initializer_list<value_type> il)
	{
		assert(il.size() <= capacity());

		auto offset = TRAITS.front_gap(il.size());
		std::uninitialized_copy(il.begin(), il.end(), capacity_begin() + offset);
		m_front_gap = static_cast<size_type>(offset);
		m_back_gap = static_cast<size_type>(TRAITS.capacity - (m_front_gap + il.size()));
	}

	inline fixed_sequence_storage(const fixed_sequence_storage& rhs)
	{
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin() + rhs.m_front_gap);
		m_front_gap = rhs.m_front_gap;
		m_back_gap = rhs.m_back_gap;
	}
	inline fixed_sequence_storage(fixed_sequence_storage&& rhs)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin() + rhs.m_front_gap);
		m_front_gap = rhs.m_front_gap;
		m_back_gap = rhs.m_back_gap;
	}

	inline fixed_sequence_storage& operator=(const fixed_sequence_storage& rhs)
	{
		clear();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin() + rhs.m_front_gap);
		m_front_gap = rhs.m_front_gap;
		m_back_gap = rhs.m_back_gap;
		return *this;
	}
	inline fixed_sequence_storage& operator=(fixed_sequence_storage&& rhs)
	{
		clear();
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin() + rhs.m_front_gap);
		m_front_gap = rhs.m_front_gap;
		m_back_gap = rhs.m_back_gap;
		return *this;
	}

	inline ~fixed_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	inline value_type* data_begin() { return capacity_begin() + m_front_gap; }
	inline value_type* data_end() { return capacity_end() - m_back_gap; }
	inline const value_type* data_begin() const { return capacity_begin() + m_front_gap; }
	inline const value_type* data_end() const { return capacity_end() - m_back_gap; }
	inline size_t size() const { return capacity() - (m_front_gap + m_back_gap); }
	inline bool empty() const { return m_front_gap + m_back_gap == capacity(); }

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
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
	inline void add_front(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_front_gap || m_back_gap);

		if (m_front_gap == 0)
			recenter();
		new(data_begin() - 1) value_type(std::forward<ARGS>(args)...);
		--m_front_gap;
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_front_gap || m_back_gap);

		if (m_back_gap == 0)
			recenter();
		new(data_end()) value_type(std::forward<ARGS>(args)...);
		--m_back_gap;
	}
	template<typename... ARGS>
	inline void add(size_t count, ARGS&&... args)
	{
		// This is certainly not as optimized as it could be. The question is,
		// how useful is resize for a middle location container?
		while (count--)
			add_back(std::forward<ARGS>(args)...);
	}

	inline void clear()
	{
		if (!empty())
		{
			destroy_data(data_begin(), data_end());
			m_front_gap = static_cast<size_type>(TRAITS.front_gap());
			m_back_gap = static_cast<size_type>(TRAITS.capacity - m_front_gap);
		}
	}
	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (erase_begin - data_begin() >= data_end() - erase_end)
			back_erase(data_begin(), data_end(), erase_begin, erase_end,
						[this](size_t count){ m_back_gap += static_cast<size_type>(count); });
		else
			front_erase(data_begin(), data_end(), erase_begin, erase_end,
						[this](size_t count){ m_front_gap += static_cast<size_type>(count); });
	}
	inline void erase(value_type* element)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (element - data_begin() >= data_end() - element)
			back_erase(data_begin(), data_end(), element, [this](){ ++m_back_gap; });
		else
			front_erase(data_begin(), data_end(), element, [this](){ ++m_front_gap; });
	}
	inline void pop_front()
	{
		assert(size());

		auto dst = data_begin();
		++m_front_gap;
		dst->~value_type();
	}
	inline void pop_back()
	{
		assert(size());

		++m_back_gap;
		data_end()->~value_type();
	}

private:

	// This function recenters the elements to prepare for size growth. If the remaining space is odd, then the
	// extra space will be at the front if we are making space at the front, otherwise it will be at the back.
	
	inline void recenter()
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

	inline dynamic_capacity() : m_capacity_end(nullptr) {}
	inline dynamic_capacity(size_t cap) :
		m_capacity_begin(operator new(sizeof(value_type) * cap))
	{
		m_capacity_end = capacity_begin() + cap;
	}
	dynamic_capacity(const dynamic_capacity&) = delete;
	inline dynamic_capacity(dynamic_capacity&& rhs) = default;
	dynamic_capacity& operator=(const dynamic_capacity&) = delete;
	inline dynamic_capacity& operator=(dynamic_capacity&& rhs) = default;

	inline size_t capacity() const { return capacity_end() - capacity_begin(); }
	inline pointer capacity_begin() { return static_cast<pointer>(m_capacity_begin.get()); }
	inline pointer capacity_end() { return m_capacity_end; }
	inline const_pointer capacity_begin() const { return static_cast<const_pointer>(m_capacity_begin.get()); }
	inline const_pointer capacity_end() const { return m_capacity_end; }

protected:

	inline void swap(dynamic_capacity& rhs)
	{
		std::swap(m_capacity_begin, rhs.m_capacity_begin);
		std::swap(m_capacity_end, rhs.m_capacity_end);
	}
	inline void swap(dynamic_capacity&& rhs) { swap(rhs); }
	inline void free()
	{
		m_capacity_begin.reset();
		m_capacity_end = nullptr;
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

	inline dynamic_sequence_storage() = default;
	inline dynamic_sequence_storage(std::initializer_list<value_type> il) :
		inherited(il.size()),
		m_data_end(std::uninitialized_copy(il.begin(), il.end(), capacity_begin()))
	{}
	inline dynamic_sequence_storage(size_t cap, fixed_sequence_storage<TRAITS.location, T, TRAITS>&& rhs) :
		inherited(cap),
		m_data_end(uninitialized_move_if_noexcept(rhs.data_begin(), rhs.data_end(), capacity_begin()))
	{}

	inline dynamic_sequence_storage(const dynamic_sequence_storage& rhs) :
		inherited(rhs.size()),
		m_data_end(std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), capacity_begin()))
	{}
	inline dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		swap(rhs);
	}

	inline dynamic_sequence_storage& operator=(const dynamic_sequence_storage& rhs)
	{
		auto dst = data_begin();
		auto src = rhs.data_begin();

		if (rhs.size() > capacity())
		{
			destroy_data(dst, data_end());
			inherited::swap(inherited(rhs.size()));
			dst = data_begin();
		}
		else
		{
			while (src != rhs.data_end() && dst != data_end())
				*dst++ = *src++;
			destroy_data(dst, data_end());
		}
		m_data_end = std::uninitialized_copy(src, rhs.data_end(), dst);

		return *this;
	}
	inline dynamic_sequence_storage& operator=(dynamic_sequence_storage&& rhs)
	{
		swap(rhs);
		return *this;
	}

	inline ~dynamic_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	inline value_type* data_begin() { return capacity_begin(); }
	inline value_type* data_end() { return m_data_end; }
	inline const value_type* data_begin() const { return capacity_begin(); }
	inline const value_type* data_end() const { return m_data_end; }
	inline size_t size() const { return data_end() - data_begin(); }
	inline bool empty() const { return m_data_end == capacity_begin(); }

	inline void swap(dynamic_sequence_storage& rhs)
	{
		inherited::swap(rhs);
		std::swap(m_data_end, rhs.m_data_end);
	}

	inline void reallocate(size_t new_cap_size)
	{
		assert(size() <= new_cap_size);

		auto old_begin = data_begin();
		auto old_end = data_end();

		inherited new_capacity(new_cap_size);
		m_data_end = uninitialized_move_if_noexcept(old_begin, old_end, new_capacity.capacity_begin());
		destroy_data(old_begin, old_end);
		inherited::swap(new_capacity);
	}

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
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
	inline void add_front(ARGS&&... args)
	{
		add_at(data_begin(), std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		assert(size() < capacity());

		new(data_end()) value_type(std::forward<ARGS>(args)...);
		++m_data_end;
	}
	template<typename... ARGS>
	inline void add(size_t count, ARGS&&... args)
	{
		while (count--)
			add_back(std::forward<ARGS>(args)...);
	}

	inline void pop_front()
	{
		assert(size());

		erase(data_begin());
	}
	inline void pop_back()
	{
		assert(size());

		--m_data_end;
		data_end()->~value_type();
	}
	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		back_erase(data_begin(), data_end(), erase_begin, erase_end,
				   [this](size_t count){ m_data_end -= count; });
	}
	inline void erase(value_type* element)
	{
		back_erase(data_begin(), data_end(), element, [this](){ --m_data_end; });
	}
	inline void clear()
	{
		if (!empty())
		{
			destroy_data(data_begin(), data_end());
			m_data_end = capacity_begin();
		}
	}
	inline void free()
	{
		if (!empty())
			destroy_data(data_begin(), data_end());
		inherited::free();
		m_data_end = nullptr;
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

	inline dynamic_sequence_storage() = default;
	inline dynamic_sequence_storage(std::initializer_list<value_type> il) : inherited(il.size())
	{
		std::uninitialized_copy(il.begin(), il.end(), capacity_begin());
		m_data_begin = capacity_begin();
	}

	inline dynamic_sequence_storage(const dynamic_sequence_storage& rhs) : inherited(rhs.size())
	{
		m_data_begin = capacity_begin();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), m_data_begin);
	}
	inline dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		swap(rhs);
	}

	inline dynamic_sequence_storage& operator=(const dynamic_sequence_storage& rhs)
	{
		clear();
		if (rhs.size() > capacity())
			inherited::swap(inherited(rhs.size()));
		auto begin = capacity_end() - rhs.size();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), begin);
		m_data_begin = begin;
		return *this;
	}
	inline dynamic_sequence_storage& operator=(dynamic_sequence_storage&& rhs)
	{
		clear();
		swap(rhs);
		return *this;
	}

	inline ~dynamic_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	inline value_type* data_begin() { return m_data_begin; }
	inline value_type* data_end() { return capacity_end(); }
	inline const value_type* data_begin() const { return m_data_begin; }
	inline const value_type* data_end() const { return capacity_end(); }
	inline size_t size() const { return data_end() - data_begin(); }
	inline bool empty() const { return m_data_begin == capacity_end(); }

	inline void swap(dynamic_sequence_storage& rhs)
	{
		inherited::swap(rhs);
		std::swap(m_data_begin, rhs.m_data_begin);
	}

	inline void reallocate(size_t new_cap)
	{
		assert(size() <= new_cap);

		auto current_size = size();

		inherited new_capacity(new_cap);
		std::uninitialized_move(data_begin(), data_end(), new_capacity.capacity_end() - current_size);
		destroy_data(data_begin(), data_end());
		inherited::swap(new_capacity);

		m_data_begin = capacity_end() - current_size;
	}

	template<typename... ARGS>
	inline iterator add_at(value_type* pos, ARGS&&... args)
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
	inline void add_front(ARGS&&... args)
	{
		assert(size() < capacity());

		new(data_begin() - 1) value_type(std::forward<ARGS>(args)...);
		--m_data_begin;
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		add_at(data_end(), std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add(size_t count, ARGS&&... args)
	{
		while (count--)
			add_front(std::forward<ARGS>(args)...);
	}

	inline void pop_front()
	{
		assert(size());

		auto dst = data_begin();
		++m_data_begin;
		dst->~value_type();
	}
	inline void pop_back()
	{
		assert(size());

		erase(data_end() - 1);
	}
	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		front_erase(data_begin(), data_end(), erase_begin, erase_end,
					[this](size_t count){ m_data_begin += count; });
	}
	inline void erase(value_type* element)
	{
		front_erase(data_begin(), data_end(), element, [this](){ ++m_data_begin; });
	}
	inline void clear()
	{
		if (!empty())
		{
			destroy_data(data_begin(), data_end());
			m_data_begin = capacity_end();
		}
	}
	inline void free()
	{
		if (!empty())
			destroy_data(data_begin(), data_end());
		inherited::free();
		m_data_begin = nullptr;
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

	inline dynamic_sequence_storage() = default;
	inline dynamic_sequence_storage(std::initializer_list<value_type> il) : inherited(il.size())
	{
		std::uninitialized_copy(il.begin(), il.end(), capacity_begin());
		m_data_begin = capacity_begin();
		m_data_end = capacity_end();
	}

	inline dynamic_sequence_storage(const dynamic_sequence_storage& rhs) : inherited(rhs.size())
	{
		m_data_begin = capacity_begin();
		m_data_end = capacity_end();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), m_data_begin);
	}
	inline dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		swap(rhs);
	}

	inline dynamic_sequence_storage& operator=(const dynamic_sequence_storage& rhs)
	{
		clear();
		if (rhs.size() > capacity())
			inherited::swap(inherited(rhs.size()));
		auto begin = capacity_begin() + TRAITS.front_gap(capacity(), rhs.size());
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), begin);
		m_data_begin = begin;
		m_data_end = m_data_begin + rhs.size();
		return *this;
	}
	inline dynamic_sequence_storage& operator=(dynamic_sequence_storage&& rhs)
	{
		clear();
		swap(rhs);
		return *this;
	}

	inline ~dynamic_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	inline value_type* data_begin() { return m_data_begin; }
	inline value_type* data_end() { return m_data_end; }
	inline const value_type* data_begin() const { return m_data_begin; }
	inline const value_type* data_end() const { return m_data_end; }
	inline size_t size() const { return m_data_end - m_data_begin; }
	inline bool empty() const { return m_data_end == m_data_begin; }

	inline void swap(dynamic_sequence_storage& rhs)
	{
		inherited::swap(rhs);
		std::swap(m_data_begin, rhs.m_data_begin);
		std::swap(m_data_end, rhs.m_data_end);
	}

	inline void reallocate(size_t new_cap)
	{
		assert(size() <= new_cap);

		auto current_size = size();
		auto offset = TRAITS.front_gap(new_cap, current_size);

		inherited new_capacity(new_cap);
		std::uninitialized_move(data_begin(), data_end(), new_capacity.capacity_begin() + offset);
		destroy_data(data_begin(), data_end());
		inherited::swap(new_capacity);

		m_data_begin = capacity_begin() + offset;
		m_data_end = m_data_begin + current_size;
	}

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
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
	inline void add_front(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_data_begin > capacity_begin() || m_data_end < capacity_end());

		if (m_data_begin == capacity_begin())
			recenter();
		new(m_data_begin - 1) value_type(std::forward<ARGS>(args)...);
		--m_data_begin;
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		assert(size() < capacity());
		assert(m_data_begin > capacity_begin() || m_data_end < capacity_end());

		if (m_data_end == capacity_end())
			recenter();
		new(m_data_end) value_type(std::forward<ARGS>(args)...);
		++m_data_end;
	}
	template<typename... ARGS>
	inline void add(size_t count, ARGS&&... args)
	{
		// This is certainly not as optimized as it could be. The question is,
		// how useful is resize for a middle location container?
		while (count--)
			add_back(std::forward<ARGS>(args)...);
	}

	inline void pop_front()
	{
		assert(size());

		data_begin()->~value_type();
		++m_data_begin;
	}
	inline void pop_back()
	{
		assert(size());

		(data_end() - 1)->~value_type();
		--m_data_end;
	}
	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (erase_begin - data_begin() >= data_end() - erase_end)
			back_erase(data_begin(), data_end(), erase_begin, erase_end,
						[this](size_t count){ m_data_end -= count; });
		else
			front_erase(data_begin(), data_end(), erase_begin, erase_end,
						[this](size_t count){ m_data_begin += count; });
	}
	inline void erase(value_type* element)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (element - data_begin() >= data_end() - element)
			back_erase(data_begin(), data_end(), element, [this](){ --m_data_end; });
		else
			front_erase(data_begin(), data_end(), element, [this](){ ++m_data_begin; });
	}
	inline void clear()
	{
		if (!empty())
		{
			destroy_data(data_begin(), data_end());
			m_data_begin = capacity_begin() + TRAITS.front_gap(capacity(), 0);
			m_data_end = m_data_begin;
		}
	}
	inline void free()
	{
		if (!empty())
			destroy_data(data_begin(), data_end());
		inherited::free();
		m_data_begin = nullptr;
		m_data_end = nullptr;
	}

private:

	// This function recenters the elements to prepare for size growth. If the remaining space is odd, then the
	// extra space will be at the front if we are making space at the front, otherwise it will be at the back.
	
	inline void recenter()
	{
		auto [front_gap, back_gap] = ::recenter<inherited>(capacity_begin(), capacity_end(), data_begin(), data_end());
		m_data_begin = capacity_begin() + front_gap;
		m_data_end = capacity_end() - back_gap;
	}

	value_type* m_data_begin = nullptr;
	value_type* m_data_end = nullptr;
};


// ==============================================================================================================
// fixed_sequence_storage - These member functions have to be here so they can see dynamic_sequence_storage.

template<typename T, sequence_traits TRAITS>
inline fixed_sequence_storage<sequence_location_lits::FRONT, T, TRAITS>::fixed_sequence_storage(dynamic_sequence_storage<TRAITS.location, T, TRAITS>&& rhs)
{
	std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start());
	set_data_area(0, rhs.size());		// This must come last in case of a move exception.
}


// ==============================================================================================================
// sequence_storage - Base class for sequence which provides the 4 different memory allocation strategies.

template<sequence_storage_lits STO, typename T, sequence_traits TRAITS>
class sequence_storage
{
	static_assert(false, "An unimplemented specialization of sequence_storage was instantiated.");
};

// STATIC storage (like std::inplace_vector or boost::static_vector).

template<typename T, sequence_traits TRAITS>
class sequence_storage<sequence_storage_lits::STATIC, T, TRAITS>
{

	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;
	using storage_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;

public:

	inline sequence_storage() = default;
	inline sequence_storage(std::initializer_list<value_type> il) : m_storage(il) {}

	static constexpr size_t max_size() { return std::numeric_limits<size_type>::max(); }
	static constexpr size_t capacity() { return TRAITS.capacity; }
	static constexpr bool is_dynamic() { return false; }
	inline size_t size() const { return m_storage.size(); }
	inline bool empty() const { return m_storage.empty(); }

	inline void pop_front() { m_storage.pop_front(); }
	inline void pop_back() { m_storage.pop_back(); }
	inline void erase(value_type* begin, value_type* end) { m_storage.erase(begin, end); }
	inline void erase(value_type* element) { m_storage.erase(element); }
	inline void clear() { m_storage.clear(); }
	inline void free() { m_storage.clear(); }

	inline void swap(sequence_storage& other)
	{
		std::swap(m_storage, other.m_storage);
	}

protected:

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
	{
		return m_storage.add_at(pos, std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add_front(ARGS&&... args)
	{
		m_storage.add_front(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		m_storage.add_back(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add(size_t new_size, ARGS&&... args)
	{
		m_storage.add(new_size, std::forward<ARGS>(args)...);
	}

	inline auto data_begin() { return m_storage.data_begin(); }
	inline auto data_end() { return m_storage.data_end(); }
	inline auto data_begin() const { return m_storage.data_begin(); }
	inline auto data_end() const { return m_storage.data_end(); }
	inline auto capacity_begin() const { return m_storage.capacity_begin(); }
	inline auto capacity_end() const { return m_storage.capacity_end(); }

	inline void reallocate(size_t new_capacity)
	{
		if (new_capacity > capacity())
			throw std::bad_alloc();
	}

private:

	storage_type m_storage;
};

// FIXED storage.

template<typename T, sequence_traits TRAITS>
class sequence_storage<sequence_storage_lits::FIXED, T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;
	using storage_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;

public:

	inline sequence_storage() = default;
	inline sequence_storage(std::initializer_list<value_type> il) : m_storage(new storage_type(il)) {}

	inline sequence_storage(const sequence_storage& rhs)
	{
		if (rhs.m_storage)
		{
			m_storage.reset(new storage_type);
			*m_storage = *rhs.m_storage;
		}
	}
	inline sequence_storage(sequence_storage&& rhs) :
		m_storage(std::move(rhs.m_storage))
	{}

	inline sequence_storage& operator=(const sequence_storage& rhs)
	{
		if (rhs.m_storage)
		{
			if (!m_storage)
				m_storage.reset(new storage_type);
			*m_storage = *rhs.m_storage;
		}
		else clear();
		return *this;
	}
	inline sequence_storage& operator=(sequence_storage&& rhs)
	{
		m_storage = std::move(rhs.m_storage);
		return *this;
	}

	static constexpr size_t max_size() { return std::numeric_limits<size_type>::max(); }
	inline size_t capacity() const { return m_storage ? TRAITS.capacity : 0; }
	static constexpr bool is_dynamic() { return true; }
	inline size_t size() const { return m_storage ? m_storage->size() : 0; }
	inline bool empty() const { return m_storage ? m_storage->empty() : true; }

	inline void pop_front() { m_storage->pop_front(); }
	inline void pop_back() { m_storage->pop_back(); }
	inline void erase(value_type* begin, value_type* end) { m_storage->erase(begin, end); }
	inline void erase(value_type* element) { m_storage->erase(element); }
	inline void clear() { if (m_storage) m_storage->clear(); }
	inline void free()
	{
		clear();
		m_storage.reset();
	}

	inline void swap(sequence_storage& other)
	{
		std::swap(m_storage, other.m_storage);
	}

protected:

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		return m_storage->add_at(pos, std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add_front(ARGS&&... args)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		m_storage->add_front(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		m_storage->add_back(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add(size_t new_size, ARGS&&... args)
	{
		if (!m_storage)
			m_storage.reset(new storage_type);
		m_storage->add(new_size, std::forward<ARGS>(args)...);
	}

	inline auto data_begin() { return m_storage ? m_storage->data_begin() : nullptr; }
	inline auto data_end() { return m_storage ? m_storage->data_end() : nullptr; }
	inline auto data_begin() const { return m_storage ? m_storage->data_begin() : nullptr; }
	inline auto data_end() const { return m_storage ? m_storage->data_end() : nullptr; }
	inline auto capacity_begin() const { return m_storage ? m_storage->capacity_begin() : nullptr; }
	inline auto capacity_end() const { return m_storage ? m_storage->capacity_end() : nullptr; }

	inline void reallocate(size_t new_capacity)
	{
		if (new_capacity > TRAITS.capacity)
			throw std::bad_alloc();
		if (!m_storage)
			m_storage.reset(new storage_type);
	}

private:

	std::unique_ptr<storage_type> m_storage;
};

// VARIABLE storage.

template<typename T, sequence_traits TRAITS>
class sequence_storage<sequence_storage_lits::VARIABLE, T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;

public:

	inline sequence_storage() = default;
	inline sequence_storage(std::initializer_list<value_type> il) : m_storage(il) {}

	static constexpr size_t max_size() { return std::numeric_limits<size_t>::max(); }
	inline size_t capacity() const { return m_storage.capacity(); }
	static constexpr bool is_dynamic() { return true; }
	inline size_t size() const { return m_storage.size(); }
	inline bool empty() const { return m_storage.empty(); }

	inline void pop_front() { m_storage.pop_front(); }
	inline void pop_back() { m_storage.pop_back(); }
	inline void erase(value_type* begin, value_type* end) { m_storage.erase(begin, end); }
	inline void erase(value_type* element) { m_storage.erase(element); }
	inline void clear() { m_storage.clear(); }
	inline void free() { m_storage.free(); }

	inline void swap(sequence_storage& other)
	{
		m_storage.swap(other.m_storage);
	}

protected:

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args) { return m_storage.add_at(pos, std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	inline void add_front(ARGS&&... args) { m_storage.add_front(std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	inline void add_back(ARGS&&... args) { m_storage.add_back(std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	inline void add(size_t new_size, ARGS&&... args) { m_storage.add(new_size, std::forward<ARGS>(args)...); }

	inline auto data_begin() { return m_storage.data_begin(); }
	inline auto data_end() { return m_storage.data_end(); }
	inline auto data_begin() const { return m_storage.data_begin(); }
	inline auto data_end() const { return m_storage.data_end(); }
	inline auto capacity_begin() const { return m_storage.capacity_begin(); }
	inline auto capacity_end() const { return m_storage.capacity_end(); }

	inline void reallocate(size_t new_capacity)
	{
		if (new_capacity > capacity())
			m_storage.reallocate(new_capacity);
	}

private:

	dynamic_sequence_storage<TRAITS.location, T, TRAITS> m_storage;
};

// BUFFERED storage supporting a small object buffer optimization (like boost::small_vector).

template<typename T, sequence_traits TRAITS>
class sequence_storage<sequence_storage_lits::BUFFERED, T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;

	enum { STC, DYN };
	using fixed_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;		// STC
	using dynamic_type = dynamic_sequence_storage<TRAITS.location, T, TRAITS>;	// DYN

public:

	inline sequence_storage() = default;
	inline sequence_storage(std::initializer_list<value_type> il)
	{
		if (il.size() <= TRAITS.capacity)
			m_storage.emplace<STC>(il);
		else
			m_storage.emplace<DYN>(il);
	}

	//inline sequence_storage(const sequence_storage& rhs)
	//{
	//	if (rhs.m_storage)
	//	{
	//		m_storage.reset(new storage_type);
	//		*m_storage = *rhs.m_storage;
	//	}
	//}
	//inline sequence_storage(sequence_storage&& rhs) :
	//	m_storage(std::move(rhs.m_storage))
	//{
	//}

	//inline sequence_storage& operator=(const sequence_storage& rhs)
	//{
	//	if (rhs.m_storage.index() == STC)
	//	if (rhs.m_storage)
	//	{
	//		if (!m_storage)
	//			m_storage.reset(new storage_type);
	//		*m_storage = *rhs.m_storage;
	//	}
	//	else clear();
	//	return *this;
	//}
	//inline sequence_storage& operator=(sequence_storage&& rhs)
	//{
	//	m_storage = std::move(rhs.m_storage);
	//	return *this;
	//}

	static constexpr size_t max_size() { return std::numeric_limits<size_type>::max(); }
	inline size_t capacity() const { return execute([](auto&& storage){ return storage.capacity(); }); }
	inline bool is_dynamic() const { return m_storage.index() == DYN; }
	inline size_t size() const { return execute([](auto&& storage){ return storage.size(); }); }
	inline bool empty() const { return execute([](auto&& storage){ return storage.empty(); }); }

	inline void pop_front() { execute([](auto&& storage){ storage.pop_front(); }); }
	inline void pop_back() { execute([](auto&& storage){ storage.pop_back(); }); }
	inline void erase(value_type* begin, value_type* end) { execute([=](auto&& storage){ storage.erase(begin, end); }); }
	inline void erase(value_type* element) { execute([=](auto&& storage){ storage.erase(element); }); }
	inline void clear() { execute([=](auto&& storage){ storage.clear(); }); }
	inline void free()
	{
		if (m_storage.index() == STC)
			get<STC>(m_storage).clear();
		else
			m_storage.emplace<STC>();
	}

	inline void swap(sequence_storage& other)
	{
		auto swap_mixed = [](sequence_storage& stc, sequence_storage& dyn)
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
	inline iterator add_at(iterator pos, ARGS&&... args)
	{
		return execute([=, &...args = std::forward<ARGS>(args)](auto&& storage){ return storage.add_at(pos, std::forward<ARGS>(args)...); });
	}
	template<typename... ARGS>
	inline void add_front(ARGS&&... args)
	{
		return execute([&...args = std::forward<ARGS>(args)](auto&& storage){ return storage.add_front(std::forward<ARGS>(args)...); });
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		return execute([&...args = std::forward<ARGS>(args)](auto&& storage){ return storage.add_back(std::forward<ARGS>(args)...); });
	}
	template<typename... ARGS>
	inline void add(size_t new_size, ARGS&&... args)
	{
		return execute([=, &...args = std::forward<ARGS>(args)](auto&& storage){ return storage.add(new_size, std::forward<ARGS>(args)...); });
	}

	inline auto data_begin()			{ return execute([](auto&& storage){ return storage.data_begin(); }); }
	inline auto data_begin() const		{ return execute([](auto&& storage){ return storage.data_begin(); }); }
	inline auto data_end()				{ return execute([](auto&& storage){ return storage.data_end(); }); }
	inline auto data_end() const		{ return execute([](auto&& storage){ return storage.data_end(); }); }
	inline auto capacity_begin() const	{ return execute([](auto&& storage){ return storage.capacity_begin(); }); }
	inline auto capacity_end() const	{ return execute([](auto&& storage){ return storage.capacity_end(); }); }

	inline void reallocate(size_t new_capacity)
	{
		// The new capacity will not fit in the buffer.
		if (new_capacity > TRAITS.capacity)
		{
			if (m_storage.index() == STC)		// We're moving out of the buffer: switch to dynamic storage.
				m_storage = dynamic_type(new_capacity, std::move(get<STC>(m_storage)));
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

	template<typename FUNC>
	inline auto execute(FUNC f)
	{ return m_storage.index() == STC ? f(get<STC>(m_storage)) : f(get<DYN>(m_storage)); }
	template<typename FUNC>
	inline auto execute(FUNC f) const
	{ return m_storage.index() == STC ? f(get<STC>(m_storage)) : f(get<DYN>(m_storage)); }

};


// ==============================================================================================================
// sequence - This is the main class template.

export template<typename T, sequence_traits TRAITS = sequence_traits<size_t>()>
class sequence : public sequence_storage<TRAITS.storage, T, TRAITS>
{
	using inherited = sequence_storage<TRAITS.storage, T, TRAITS>;
	using inherited::data_begin;
	using inherited::data_end;
	using inherited::reallocate;
	using inherited::add_at;
	using inherited::add_front;
	using inherited::add_back;
	using inherited::add;

public:

	using value_type = T;
	using reference = value_type&;
	using const_reference = const value_type&;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	using inherited::size;
	using inherited::empty;
	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;
	using inherited::erase;
	using inherited::clear;
	using inherited::free;

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

	inline sequence() = default;
	inline sequence(const sequence&) = default;
	inline sequence(sequence&&) = default;
	inline sequence(std::initializer_list<value_type> il) : inherited(il) {}
	template<typename... ARGS>
	inline sequence(size_type n, ARGS&&... args)
	{
		if constexpr (traits.is_variable())
			if (n)
				reallocate(std::max<size_t>(n, traits.capacity));
		add(n, std::forward<ARGS>(args)...);
	}

	inline sequence& operator=(const sequence&) = default;
	inline sequence& operator=(sequence&&) = default;
	inline sequence& operator=(std::initializer_list<value_type> il) { assign(il); return *this; }

	template<typename... ARGS>
	inline void assign(size_type n, ARGS&&... args)
	{
		clear();
		if constexpr (traits.is_variable())
			if (n > capacity())
				reallocate(std::max<size_t>(n, traits.capacity));
		add(n, std::forward<ARGS>(args)...);
	}

	// This is not sufficient. It is pessimized for BACK and MIDDLE sequences.
	template<typename IT>
	inline void assign(IT first, IT last)
	{
		clear();
		for (; first != last; ++first)
			emplace_back(*first);
	}

	inline void assign(std::initializer_list<value_type> il) { assign(il.begin(), il.end()); }

	inline iterator					begin() { return data_begin(); }
	inline const_iterator			begin() const { return data_begin(); }
	inline iterator					end() { return data_end(); }
	inline const_iterator			end() const { return data_end(); }
	inline reverse_iterator			rbegin() { return reverse_iterator(data_end()); }
	inline const_reverse_iterator	rbegin() const { return const_reverse_iterator(data_end()); }
	inline reverse_iterator			rend() { return reverse_iterator(data_begin()); }
	inline const_reverse_iterator	rend() const { return const_reverse_iterator(data_begin()); }

	inline const_iterator			cbegin() const { return data_begin(); }
	inline const_iterator			cend() const { return data_end(); }
	inline const_reverse_iterator	crbegin() const { return const_reverse_iterator(data_end()); }
	inline const_reverse_iterator	crend() const { return const_reverse_iterator(data_begin()); }

	inline value_type*				data() { return data_begin(); }
	inline const value_type*		data() const { return data_begin(); }

	inline value_type&				front() { return *data_begin(); }
	inline const value_type&		front() const { return *data_begin(); }
	inline value_type&				back() { return *(data_end() - 1); }
	inline const value_type&		back() const { return *(data_end() - 1); }

	inline value_type& at(size_t index)
	{
		if (index >= size()) throw std::out_of_range(std::format(OUT_OF_RANGE_ERROR, index));
		return *(data_begin() + index);
	}
	inline const value_type& at(size_t index) const
	{
		if (index >= size()) throw std::out_of_range(std::format(OUT_OF_RANGE_ERROR, index));
		return *(data_begin() + index);
	}
	inline value_type& operator[](size_t index) & { return *(data_begin() + index); }
	inline const value_type& operator[](size_t index) const && { return *(data_begin() + index); }

	inline void reserve(size_t new_capacity)
	{
		if (!traits.is_variable() || new_capacity > capacity())
			reallocate(new_capacity);
	}
	inline void shrink_to_fit()
	{
		if (auto current_size = size(); current_size == 0)
			free();
		else if (traits.is_variable() && current_size < capacity())
			reallocate(current_size);
	}
	template<typename... ARGS>
	inline void resize(size_type new_size, ARGS&&... args)
	{
		auto old_size = size();

		if (new_size < old_size)
			erase(data_end() - (old_size - new_size), data_end());
		else if (new_size > old_size)
		{
			if (new_size > capacity())
				reallocate(std::max<size_t>(new_size, traits.capacity));
			add(new_size - old_size, std::forward<ARGS>(args)...);
		}
	}

	template< class... ARGS >
	inline iterator emplace(const_iterator cpos, ARGS&&... args)
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
	inline void emplace_front(ARGS&&... args)
	{
		if (auto old_capacity = capacity(); size() == old_capacity)
			reallocate(traits.grow(old_capacity));
		add_front(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void emplace_back(ARGS&&... args)
	{
		if (auto old_capacity = capacity(); size() == old_capacity)
			reallocate(traits.grow(old_capacity));
		add_back(std::forward<ARGS>(args)...);
	}

	inline iterator insert(const_iterator cpos, const_reference e) { return emplace(cpos, e); }
	inline void push_front(const_reference e) { emplace_front(e); }
	inline void push_back(const_reference e) { emplace_back(e); }

private:

	static constexpr auto OUT_OF_RANGE_ERROR = "invalid sequence index {}";
};
