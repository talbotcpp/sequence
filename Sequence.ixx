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

// Mode Literals - Used to specify the values of various sequence traits.
// These are hoisted out of the class template to avoid template dependencies.
// See README.md for a detailed discussion of these values.

export enum class storage_modes { LOCAL, FIXED, VARIABLE, BUFFERED };	// See sequence_traits::storage.
export enum class location_modes { FRONT, BACK, MIDDLE };				// See sequence_traits::location.
export enum class growth_modes { LINEAR, EXPONENTIAL, VECTOR };			// See sequence_traits::growth.

// sequence_traits - Structure used to supply the sequence traits. This is fully documented in the README.md file.

export template<std::unsigned_integral SIZE = size_t>
struct sequence_traits
{
	using size_type = SIZE;

	storage_modes storage = storage_modes::VARIABLE;
	location_modes location = location_modes::FRONT;
	growth_modes growth = growth_modes::VECTOR;

	size_t capacity = 1;
	size_t increment = 1;
	float factor = 1.5;

	constexpr size_t grow(size_t cap) const
	{
		if (cap < capacity) return capacity;
		else switch (growth)
		{
			case growth_modes::LINEAR:
				return cap + increment;
			case growth_modes::EXPONENTIAL:
				return cap + std::max(size_t(cap * (factor - 1.f)), increment);
			default:
			case growth_modes::VECTOR:
				return cap + std::max<size_t>(cap / 2, 1u);
		}
	};

	// 'front_gap' returns the location of the start of the data given a capacity and size.
	// The formula is based on the 'location' value.

	constexpr size_t front_gap(size_t cap, size_t size) const
	{
		switch (location)
		{
		default:
		case location_modes::FRONT:		return 0;
		case location_modes::BACK:		return cap - size;
		case location_modes::MIDDLE:	return (cap - size) / 2;
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
inline auto uninitialized_move_if_noexcept(InpIt src, InpIt end, FwdIt dst)
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

template<typename T>
inline void front_erase(T* data_begin, T* erase_begin, T* erase_end)
{
	assert(erase_begin >= data_begin);
	assert(erase_end >= erase_begin);

	if (erase_begin != erase_end)
	{
		auto beg = data_begin - 1;
		auto dst = erase_end - 1;
		auto src = erase_begin - 1;
		while (src != beg)
			*dst-- = std::move(*src--);
		destroy_data(data_begin, ++dst);
	}
}

template<typename T>
inline void front_erase(T* data_begin, T* element)
{
	assert(element >= data_begin);

	auto src = element - 1;
	while (element != data_begin)
		*element-- = std::move(*src--);
	element->~T();
}

template<typename T>
inline void back_erase(T* data_end, T* erase_begin, T* erase_end)
{
	assert(erase_end <= data_end);
	assert(erase_end >= erase_begin);

	if (erase_begin != erase_end)
	{
		auto dst = erase_begin;
		auto src = erase_end;
		while (src != data_end)
			*dst++ = std::move(*src++);
		destroy_data(dst, data_end);
	}
}

template<typename T>
inline void back_erase(T* data_end, T* element)
{
	assert(element < data_end);

	auto src = element + 1;
	while (src != data_end)
		*element++ = std::move(*src++);
	element->~T();
}

template<std::unsigned_integral T, std::unsigned_integral V>
constexpr bool fits_in(V v)
{
	return v <= std::numeric_limits<T>::max();
}

constexpr bool can_grow(storage_modes mode) { return mode >= storage_modes::VARIABLE; }

}

// ==============================================================================================================
// Shift functions - Slide the elements up or down within the capacity.
// Preconditions: The elements (data) are a contiguous subset of the capacity. The shift
// distance does not cause the elements to move outside the capacity.
// Throws: If an exception is thrown by a move or copy (assignment or construction), all elements
// (newly created and existing) will be destroyed.
// These functions are exported to allow unit testing.

// shift_up - Moves a contiguous block of elements (data) toward the begining of the capacity
// by n positions. n must be positive.

export template<typename T>
void shift_up(T* capacity_begin, T* capacity_end, T* data_begin, T* data_end, ptrdiff_t n)
{
	assert(n > 0);
	assert(data_begin - n >= capacity_begin);
	assert(data_begin <= data_end);
	assert(data_end <= capacity_end);

	auto size = data_end - data_begin;
	auto last = data_begin + std::min(n, size);

	try {
		auto next = std::uninitialized_move(data_begin, last, data_begin - n);
		while (last != data_end)
			*next++ = std::move(*last++);
		destroy_data(next > data_begin ? next : data_begin, data_end);
	}
	catch (...)
	{
		destroy_data(data_begin, data_end);
		throw;
	}
}

// shift_down - Moves a contiguous block of elements (data) toward the end of the capacity
// by n positions. n must be positive.

export template<typename T>
void shift_down(T* capacity_begin, T* capacity_end, T* data_begin, T* data_end, ptrdiff_t n)
{
	assert(n > 0);
	assert(data_begin >= capacity_begin);
	assert(data_begin <= data_end);
	assert(data_end + n <= capacity_end);

	auto size = data_end - data_begin;
	auto first = data_end - std::min(n, size);

	try {
		auto next = first + n;
		std::uninitialized_move(first, data_end, next);
		while (first != data_begin)
			*--next = std::move(*--first);
		destroy_data(data_begin, next < data_end ? next : data_end);
	}
	catch (...)
	{
		destroy_data(data_begin, data_end);
		throw;
	}
}

// shift (takes a count) - Calls shift_up or shift_down based on the sign of the shift distance n.

export template<typename T>
inline void shift(T* capacity_begin, T* capacity_end, T* data_begin, T* data_end, ptrdiff_t n)
{
	if (n < 0)
		shift_up(capacity_begin, capacity_end, data_begin, data_end, -n);
	else if (n > 0)
		shift_down(capacity_begin, capacity_end, data_begin, data_end, n);
}

// shift (takes a target) - Calls shift_up or shift_down based on the relationship between the target and data pointers.

export template<typename T>
inline void shift(T* capacity_begin, T* capacity_end, T* data_begin, T* data_end, T* target_begin)
{
	shift(capacity_begin, capacity_end, data_begin, data_end, target_begin - data_begin);
}


// ==============================================================================================================
// fixed_capacity - This is the base class for fixed_storage instantiations. It handles the raw capacity.

template<typename T, size_t CAP> requires (CAP != 0)
class fixed_capacity
{
	using value_type = T;
	using pointer = value_type*;
	using const_pointer = const value_type*;

public:

	fixed_capacity() {}
	~fixed_capacity() {}

	constexpr static size_t capacity() { return CAP; }

	inline pointer capacity_begin() { return elements; }
	inline pointer capacity_end() { return elements + CAP; }
	inline const_pointer capacity_begin() const { return elements; }
	inline const_pointer capacity_end() const { return elements + CAP; }

private:

	union
	{
		value_type elements[CAP];
		unsigned char unused;
	};
};


// ==============================================================================================================
// fixed_storage - This is the base class for the fixed_sequence_storage class. It provides
// the three different element management strategies for fixed capacity sequences

template<typename T, sequence_traits TRAITS, location_modes LOC = TRAITS.location>
class fixed_storage
{
	static_assert(false, "An unimplemented specialization of fixed_storage was instantiated.");
};

template<typename T, sequence_traits TRAITS>
class fixed_storage<T, TRAITS, location_modes::FRONT> : public fixed_capacity<T, TRAITS.capacity>
{
	using inherited = fixed_capacity<T, TRAITS.capacity>;
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using size_type = typename decltype(TRAITS)::size_type;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	inline iterator data_begin() { return capacity_begin(); }
	inline iterator data_end() { return capacity_begin() + m_size; }
	inline const_iterator data_begin() const { return capacity_begin(); }
	inline const_iterator data_end() const { return capacity_begin() + m_size; }
	inline size_t size() const { return m_size; }
	inline bool empty() const { return m_size == 0; }

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (empty() || pos == data_end())
			add_back(std::forward<ARGS>(args)...);
		else
			pos = back_add_at(data_end(), pos, [this](){ ++m_size; }, std::forward<ARGS>(args)...);
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
		++m_size;
	}

	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		back_erase(data_end(), erase_begin, erase_end);
		m_size -= erase_end - erase_begin;
	}
	inline void erase(value_type* element)
	{
		back_erase(data_end(), element);
		--m_size;
	}
	inline void pop_front()
	{
		erase(data_begin());
	}
	inline void pop_back()
	{
		--m_size;
		data_end()->~value_type();
	}

	inline void set_size(size_t size) { assert(fits_in<size_type>(size)); m_size = size; }
	inline void prepare_for(size_t size) {}
	inline auto new_data_start(size_t size) { return capacity_begin(); }

private:

	size_type m_size = 0;
};

template<typename T, sequence_traits TRAITS>
class fixed_storage<T, TRAITS, location_modes::BACK> : public fixed_capacity<T, TRAITS.capacity>
{
	using inherited = fixed_capacity<T, TRAITS.capacity>;
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using size_type = typename decltype(TRAITS)::size_type;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	inline iterator data_begin() { return capacity_end() - m_size; }
	inline iterator data_end() { return capacity_end(); }
	inline const_iterator data_begin() const { return capacity_end() - m_size; }
	inline const_iterator data_end() const { return capacity_end(); }
	inline size_t size() const { return m_size; }
	inline bool empty() const { return m_size == 0; }

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (empty() || pos == data_begin())
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

	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		front_erase(data_begin(), erase_begin, erase_end);
		m_size -= erase_end - erase_begin;
	}
	inline void erase(value_type* element)
	{
		front_erase(data_begin(), element);
		--m_size;
	}
	inline void pop_front()
	{
		data_begin()->~value_type();
		--m_size;
	}
	inline void pop_back()
	{
		erase(data_end() - 1);
	}

	inline void set_size(size_t size) { assert(fits_in<size_type>(size)); m_size = size; }
	inline void prepare_for(size_t size) {}
	inline auto new_data_start(size_t size) { return capacity_end() - size; }

private:

	size_type m_size = 0;
};

template<typename T, sequence_traits TRAITS>
class fixed_storage<T, TRAITS, location_modes::MIDDLE> : public fixed_capacity<T, TRAITS.capacity>
{
	using inherited = fixed_capacity<T, TRAITS.capacity>;
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using size_type = typename decltype(TRAITS)::size_type;
	using area_type = std::pair<size_type, size_type>;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	inline iterator data_begin() { return capacity_begin() + m_front_gap; }
	inline iterator data_end() { return capacity_end() - m_back_gap; }
	inline const_iterator data_begin() const { return capacity_begin() + m_front_gap; }
	inline const_iterator data_end() const { return capacity_end() - m_back_gap; }
	inline size_t size() const { return capacity() - (m_front_gap + m_back_gap); }
	inline bool empty() const { return m_front_gap + m_back_gap == capacity(); }

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (empty() || pos == data_end())
			add_back(std::forward<ARGS>(args)...);
		else if (pos == data_begin())
			add_front(std::forward<ARGS>(args)...);

		// Inserting closer to the end--add at back.
		else if (pos - data_begin() >= data_end() - pos)
		{
			if (m_back_gap == 0)
			{
				recenter(false);
				pos -= m_back_gap;
			}
			pos = back_add_at(data_end(), pos, [this](){ --m_back_gap; }, std::forward<ARGS>(args)...);
		}
		
		// Inserting closer to the beginning--add at front.
		else
		{
			if (m_front_gap == 0)
			{
				recenter(true);
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
		if (m_front_gap == 0)
			recenter(true);
		new(data_begin() - 1) value_type(std::forward<ARGS>(args)...);
		--m_front_gap;
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		assert(size() < capacity());
		if (m_back_gap == 0)
			recenter(false);
		new(data_end()) value_type(std::forward<ARGS>(args)...);
		--m_back_gap;
	}

	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		// If we are erasing nearer the back or dead center, erase at the back.
		if (erase_begin - data_begin() >= data_end() - erase_end)
		{
			back_erase(data_end(), erase_begin, erase_end);
			m_back_gap += erase_end - erase_begin;
		}
		//  Otherwise erase at the front.
		else
		{
			front_erase(data_begin(), erase_begin, erase_end);
			m_front_gap += erase_end - erase_begin;
		}
	}
	inline void erase(value_type* element)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (element - data_begin() >= data_end() - element)
		{
			back_erase(data_end(), element);
			++m_back_gap;
		}
		//  Otherwise erase at the front.
		else
		{
			front_erase(data_begin(), element);
			++m_front_gap;
		}
	}
	inline void pop_front()
	{
		data_begin()->~value_type();
		++m_front_gap;
	}
	inline void pop_back()
	{
		++m_back_gap;
		data_end()->~value_type();
	}

	inline void set_size(size_t size)
	{
		assert(fits_in<size_type>(size));
		m_front_gap = static_cast<size_type>(TRAITS.front_gap(size));
		m_back_gap = static_cast<size_type>(TRAITS.capacity - (m_front_gap + size));
	}
	inline void prepare_for(size_t size)
	{
		assert(empty());
		assert(fits_in<size_type>(size));
		m_front_gap = static_cast<size_type>(TRAITS.front_gap(size));
		m_back_gap = static_cast<size_type>(TRAITS.capacity - m_front_gap);
	}
	inline auto new_data_start(size_t size) { return capacity_begin() + TRAITS.front_gap(size); }

private:

	// Moves the elements to the (approximate) center of the capacity to prepare for size growth.
	// If the remaining space is odd, then the extra space will be at the front if we are making
	// space at the front, otherwise it will be at the back. The front parameter is true if the
	// addition will be at the front (requiring a shift down).
	
	inline void recenter(bool front)
	{
		auto beg = data_begin();
		auto end = data_end();

		auto sz = size();
		ptrdiff_t fg = TRAITS.front_gap(sz + 1);
		if (front) ++fg;
		ptrdiff_t bg = TRAITS.capacity - (fg + sz);
		ptrdiff_t offset = fg - m_front_gap;

		m_back_gap = static_cast<size_type>(TRAITS.capacity - m_front_gap);
		shift(capacity_begin(), capacity_end(), beg, end, offset);
		m_front_gap = static_cast<size_type>(fg);
		m_back_gap = static_cast<size_type>(bg);
	}

	// Empty sequences with odd capacity will have the extra space at the back.

	size_type m_front_gap = static_cast<size_type>(TRAITS.front_gap());
	size_type m_back_gap = static_cast<size_type>(TRAITS.capacity - TRAITS.front_gap());
};


// ==============================================================================================================
// fixed_sequence_storage - This class provides fixed capacity storage (which may be static or
// dynamically allocated). It offers three element management strategies.

// Forward declaration so that the sequence storage types can refer to each other.

template<typename T, sequence_traits TRAITS>
class dynamic_sequence_storage;

template<typename T, sequence_traits TRAITS>
class fixed_sequence_storage : public fixed_storage<T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reference = value_type&;
	using size_type = typename decltype(TRAITS)::size_type;
	using inherited = fixed_storage<T, TRAITS>;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;
	using inherited::data_begin;
	using inherited::data_end;
	using inherited::new_data_start;
	using inherited::set_size;
	using inherited::size;
	using inherited::empty;

	inline fixed_sequence_storage() = default;
	inline fixed_sequence_storage(size_t new_capacity)
	{
		if (new_capacity > TRAITS.capacity)
			throw std::bad_alloc();
	}

	fixed_sequence_storage(const dynamic_sequence_storage<T, TRAITS>&);
	fixed_sequence_storage(dynamic_sequence_storage<T, TRAITS>&&);

	inline fixed_sequence_storage(const fixed_sequence_storage& rhs)
	{
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());	// This must come last in case of a move exception.
	}
	inline fixed_sequence_storage(fixed_sequence_storage&& rhs)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());	// This must come last in case of a move exception.
	}

	inline fixed_sequence_storage& operator=(const fixed_sequence_storage& rhs)
	{
		clear();
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());
		return *this;
	}
	inline fixed_sequence_storage& operator=(fixed_sequence_storage&& rhs)
	{
		clear();
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());
		return *this;
	}

	inline ~fixed_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	inline void clear()
	{
		if (!empty())
		{
			destroy_data(data_begin(), data_end());
			set_size(0);
		}
	}
};


// ==============================================================================================================
// dynamic_capacity

template<typename T>
class dynamic_capacity
{
	using value_type = T;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using deleter_type = decltype([](void* p){ delete p; });

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

	inline void free()
	{
		m_capacity_begin.reset();
		m_capacity_end = nullptr;
	}

	inline void swap(dynamic_capacity& rhs)
	{
		std::swap(m_capacity_begin, rhs.m_capacity_begin);
		std::swap(m_capacity_end, rhs.m_capacity_end);
	}
	inline void swap(dynamic_capacity&& rhs) { swap(rhs); }

	std::unique_ptr<void, deleter_type> m_capacity_begin;
	pointer m_capacity_end;
};


// ==============================================================================================================
// dynamic_storage - This is the base class for the dynamic_sequence_storage class. It provides
// the three different element management strategies for dynamic capacity sequences

template<typename T, sequence_traits TRAITS, location_modes LOC = TRAITS.location>
class dynamic_storage
{
	static_assert(false, "An unimplemented specialization of dynamic_storage was instantiated.");
};

template<typename T, sequence_traits TRAITS>
class dynamic_storage<T, TRAITS, location_modes::FRONT> : public dynamic_capacity<T>
{
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;

public:

	using inherited = dynamic_capacity<T>;
	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	inline dynamic_storage() = default;
	inline dynamic_storage(size_t cap) : inherited(cap) {}

	inline iterator data_begin() { return capacity_begin(); }
	inline iterator data_end() { return m_data_end; }
	inline const_iterator data_begin() const { return capacity_begin(); }
	inline const_iterator data_end() const { return m_data_end; }
	inline size_t size() const { return data_end() - data_begin(); }
	inline bool empty() const { return m_data_end == capacity_begin(); }

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (empty() || pos == data_end())
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

	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		back_erase(data_end(), erase_begin, erase_end);
		m_data_end -= erase_end - erase_begin;
	}
	inline void erase(value_type* element)
	{
		back_erase(data_end(), element);
		--m_data_end;
	}
	inline void pop_front()
	{
		erase(data_begin());
	}
	inline void pop_back()
	{
		--m_data_end;
		data_end()->~value_type();
	}

	inline void set_size(size_t size) { m_data_end = capacity_begin() + size; }
	inline void prepare_for(size_t size) {}
	inline auto new_data_start(size_t size) { return capacity_begin(); }

protected:

	inline void free()
	{
		inherited::free();
		m_data_end = nullptr;
	}

	template<sequence_traits TR>
	inline void swap(dynamic_storage<T, TR, location_modes::FRONT>& rhs)
	{
		inherited::swap(rhs);
		std::swap(m_data_end, rhs.m_data_end);
	}
	template<sequence_traits TR>
	inline void swap(dynamic_storage<T, TR, location_modes::FRONT>&& rhs) { swap(rhs); }

private:

	friend class dynamic_storage;

	iterator m_data_end = nullptr;
};

template<typename T, sequence_traits TRAITS>
class dynamic_storage<T, TRAITS, location_modes::BACK> : public dynamic_capacity<T>
{
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;

public:

	using inherited = dynamic_capacity<T>;
	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	inline dynamic_storage() = default;
	inline dynamic_storage(size_t cap) : inherited(cap) {}

	inline iterator data_begin() { return m_data_begin; }
	inline iterator data_end() { return capacity_end(); }
	inline const_iterator data_begin() const { return m_data_begin; }
	inline const_iterator data_end() const { return capacity_end(); }
	inline size_t size() const { return data_end() - data_begin(); }
	inline bool empty() const { return m_data_begin == capacity_end(); }

	template<typename... ARGS>
	inline iterator add_at(value_type* pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		if (empty() || pos == data_begin())
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

	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		front_erase(data_begin(), erase_begin, erase_end);
		m_data_begin += erase_end - erase_begin;
	}
	inline void erase(value_type* element)
	{
		front_erase(data_begin(), element);
		++m_data_begin;
	}
	inline void pop_front()
	{
		data_begin()->~value_type();
		++m_data_begin;
	}
	inline void pop_back()
	{
		erase(data_end() - 1);
	}

	inline void set_size(size_t size) { m_data_begin = capacity_end() - size; }
	inline void prepare_for(size_t size) {}
	inline auto new_data_start(size_t size) { return capacity_end() - size; }

protected:

	inline void free()
	{
		inherited::free();
		m_data_begin = nullptr;
	}

	template<sequence_traits TR>
	inline void swap(dynamic_storage<T, TR, location_modes::BACK>& rhs)
	{
		inherited::swap(rhs);
		std::swap(m_data_begin, rhs.m_data_begin);
	}
	template<sequence_traits TR>
	inline void swap(dynamic_storage<T, TR, location_modes::BACK>&& rhs) { swap(rhs); }

private:

	friend class dynamic_storage;

	iterator m_data_begin = nullptr;
};

template<typename T, sequence_traits TRAITS>
class dynamic_storage<T, TRAITS, location_modes::MIDDLE> : public dynamic_capacity<T>
{
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;

public:

	using inherited = dynamic_capacity<T>;
	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	inline dynamic_storage() = default;
	inline dynamic_storage(size_t cap) : inherited(cap) {}

	inline iterator data_begin() { return m_data_begin; }
	inline iterator data_end() { return m_data_end; }
	inline const_iterator data_begin() const { return m_data_begin; }
	inline const_iterator data_end() const { return m_data_end; }
	inline size_t size() const { return m_data_end - m_data_begin; }
	inline bool empty() const { return m_data_end == m_data_begin; }

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args)
	{
		assert(size() < capacity());
		assert(pos >= data_begin() && pos <= data_end());

		auto dbeg = data_begin();
		auto dend = data_end();

		if (empty() || pos == data_end())
			add_back(std::forward<ARGS>(args)...);
		else if (pos == data_begin())
			add_front(std::forward<ARGS>(args)...);

		// Inserting closer to the end--add at back.
		else if (pos - data_begin() >= data_end() - pos)
		{
			if (m_data_end == capacity_end())
			{
				recenter(false);
				pos -= capacity_end() - m_data_end;
			}
			pos = back_add_at(data_end(), pos, [this](){ ++m_data_end; }, std::forward<ARGS>(args)...);
		}

		// Inserting closer to the beginning--add at front.
		else
		{
			if (m_data_begin == capacity_begin())
			{
				recenter(true);
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
		if (m_data_begin == capacity_begin())
			recenter(true);
		new(m_data_begin - 1) value_type(std::forward<ARGS>(args)...);
		--m_data_begin;
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		assert(size() < capacity());
		if (m_data_end == capacity_end())
			recenter(false);
		new(m_data_end) value_type(std::forward<ARGS>(args)...);
		++m_data_end;
	}

	inline void erase(value_type* erase_begin, value_type* erase_end)
	{
		// If we are erasing nearer the back or dead center, erase at the back.
		if (erase_begin - data_begin() >= data_end() - erase_end)
		{
			back_erase(data_end(), erase_begin, erase_end);
			m_data_end -= erase_end - erase_begin;
		}
		//  Otherwise erase at the front.
		else
		{
			front_erase(data_begin(), erase_begin, erase_end);
			m_data_begin += erase_end - erase_begin;
		}
	}
	inline void erase(value_type* element)
	{
		// If we are erasing nearer the back or dead center, erase at the back. Otherwise erase at the front.
		if (element - data_begin() >= data_end() - element)
		{
			back_erase(data_end(), element);
			--m_data_end;
		}
		//  Otherwise erase at the front.
		else
		{
			front_erase(data_begin(), element);
			++m_data_begin;
		}
	}
	inline void pop_front()
	{
		data_begin()->~value_type();
		++m_data_begin;
	}
	inline void pop_back()
	{
		--m_data_end;
		data_end()->~value_type();
	}

	inline void set_size(size_t size)
	{
		m_data_begin = capacity_begin() + TRAITS.front_gap(capacity(), size);
		m_data_end = m_data_begin + size;
	}
	inline void prepare_for(size_t size)
	{
		assert(empty());
		m_data_begin = capacity_begin() + TRAITS.front_gap(capacity(), size);
		m_data_end = m_data_begin;
	}
	inline auto new_data_start(size_t size)
	{
		return capacity_begin() + TRAITS.front_gap(capacity(), size);
	}

protected:

	inline void free()
	{
		inherited::free();
		m_data_begin = nullptr;
		m_data_end = nullptr;
	}

	template<sequence_traits TR>
	inline void swap(dynamic_storage<T, TR, location_modes::MIDDLE>& rhs)
	{
		inherited::swap(rhs);
		std::swap(m_data_begin, rhs.m_data_begin);
		std::swap(m_data_end, rhs.m_data_end);
	}
	template<sequence_traits TR>
	inline void swap(dynamic_storage<T, TR, location_modes::MIDDLE>&& rhs) { swap(rhs); }

private:

	friend class dynamic_storage;

	// Moves the elements to the (approximate) center of the capacity to prepare for size growth.
	// If the remaining space is odd, then the extra space will be at the front if we are making
	// space at the front, otherwise it will be at the back. The front parameter is true if the
	// addition will be at the front (requiring a shift down).
	
	inline void recenter(bool front)
	{
		auto beg = m_data_begin;
		auto end = m_data_end;

		auto sz = size();
		ptrdiff_t fg = TRAITS.front_gap(sz + 1);
		if (front) ++fg;
		ptrdiff_t bg = TRAITS.capacity - (fg + sz);
		ptrdiff_t offset = fg - (beg - capacity_begin());

		m_data_end = m_data_begin;
		shift(capacity_begin(), capacity_end(), beg, end, offset);
		m_data_begin = capacity_begin() + fg;
		m_data_end = m_data_begin + sz;
	}

	iterator m_data_begin = nullptr;
	iterator m_data_end = nullptr;
};


// ==============================================================================================================
// dynamic_sequence_storage - This class provides dynamically allocated capacity storage. It offers three element
// management strategies.

template<typename T, sequence_traits TRAITS>
class dynamic_sequence_storage : public dynamic_storage<T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reference = value_type&;
	using size_type = typename decltype(TRAITS)::size_type;
	using inherited = dynamic_storage<T, TRAITS>;

public:

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;
	using inherited::data_begin;
	using inherited::data_end;
	using inherited::new_data_start;
	using inherited::set_size;
	using inherited::size;
	using inherited::empty;
	using inherited::swap;

	inline dynamic_sequence_storage() = default;
	inline dynamic_sequence_storage(size_t new_capacity) : inherited(new_capacity) {}

	inline dynamic_sequence_storage(size_t cap, fixed_sequence_storage<T, TRAITS>&& rhs) :
		inherited(cap)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());	// This must come last in case of a copy exception.
	}

	inline dynamic_sequence_storage(const dynamic_sequence_storage& rhs) :
		inherited(rhs.size())
	{
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());	// This must come last in case of a move exception.
	}
	inline dynamic_sequence_storage(dynamic_sequence_storage&& rhs)
	{
		swap(rhs);
	}

	template<sequence_traits TR>
	inline dynamic_sequence_storage(dynamic_sequence_storage<T, TR>&& rhs)
	{
		// If the locations are the same, the rhs is morally the same type, so we can just do a swap.

		if constexpr (TRAITS.location == TR.location)
		{
			swap(rhs);
		}

		// If not, then we have to shift the elements to the correct position within the capacity.
		// The point here is that shifting will (likely) be much faster than reallocating, so it's
		// (hopefully) worth the trouble to do all this.

		else
		{
			auto db = rhs.data_begin();
			auto de = rhs.data_end();
			auto size = rhs.size();

			inherited::inherited::swap(rhs);	// Skip dynamic_storage and swap the capacity only.
			rhs.set_size(0);					// Let the rhs know that it's now empty.
			set_size(0);						// If the shift throws, it will destroy all the elements, so forget about them.
			shift(capacity_begin(), capacity_end(), db, de, new_data_start(size));
			set_size(size);
		}
	}

	inline dynamic_sequence_storage& operator=(const dynamic_sequence_storage& rhs)
	{
		clear();
		if (rhs.size() > capacity())
			swap(inherited(rhs.size()));
		std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());
		return *this;
	}
	inline dynamic_sequence_storage& operator=(dynamic_sequence_storage&& rhs)
	{
		swap(rhs);
		return *this;
	}

	template<sequence_traits TR>
	inline void assign(dynamic_sequence_storage<T, TR>&& rhs)
	{
		clear();

		// If the locations are the same, the rhs is morally the same type, so we can just do a swap.

		if constexpr (TRAITS.location == TR.location)
		{
			swap(rhs);
		}

		// If not, then we have to shift the elements to the correct position within the capacity.
		// The point here is that shifting will (likely) be much faster than reallocating, so it's
		// (hopefully) worth the trouble to do all this.

		else
		{
			auto db = rhs.data_begin();
			auto de = rhs.data_end();
			auto size = rhs.size();

			inherited::inherited::swap(rhs);	// Skip dynamic_storage and swap the capacity only.
			rhs.set_size(0);					// Let the rhs know that it's now empty.
			set_size(0);						// If the shift throws, it will destroy all the elements, so forget about them.
			shift(capacity_begin(), capacity_end(), db, de, new_data_start(size));
			set_size(size);
		}
	}

	inline ~dynamic_sequence_storage()
	{
		destroy_data(data_begin(), data_end());
	}

	inline void reallocate(size_t new_cap_size)
	{
		assert(size() <= new_cap_size);

		inherited new_storage(new_cap_size);
		uninitialized_move_if_noexcept(data_begin(), data_end(), new_storage.new_data_start(size()));
		new_storage.set_size(size());

		destroy_data(data_begin(), data_end());
		swap(new_storage);
	}
	inline void clear()
	{
		if (!empty())
		{
			destroy_data(data_begin(), data_end());
			set_size(0);
		}
	}
	inline void free()
	{
		if (!empty())
			destroy_data(data_begin(), data_end());
		inherited::free();
	}
};


// ==============================================================================================================
// fixed_sequence_storage - These member functions are out here so they can see dynamic_sequence_storage.

template<typename T, sequence_traits TRAITS>
inline fixed_sequence_storage<T, TRAITS>::fixed_sequence_storage(const dynamic_sequence_storage<T, TRAITS>& rhs)
{
	std::uninitialized_copy(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
	set_size(rhs.size());		// This must come last in case of a move exception.
}

template<typename T, sequence_traits TRAITS>
inline fixed_sequence_storage<T, TRAITS>::fixed_sequence_storage(dynamic_sequence_storage<T, TRAITS>&& rhs)
{
	std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
	set_size(rhs.size());		// This must come last in case of a move exception.
}


// ==============================================================================================================
// sequence_storage - Base class for sequence which provides the 4 different memory allocation strategies.

template<typename T, sequence_traits TRAITS, storage_modes STO = TRAITS.storage>
class sequence_storage
{
	static_assert(false, "An unimplemented specialization of sequence_storage was instantiated.");
};

// LOCAL storage (like std::inplace_vector or boost::static_vector).

template<typename T, sequence_traits TRAITS>
class sequence_storage<T, TRAITS, storage_modes::LOCAL>
{

	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;
	using storage_type = fixed_sequence_storage<T, TRAITS>;

public:

	inline sequence_storage() = default;
	inline sequence_storage(size_t new_capacity) : m_storage(new_capacity) {}

	template<sequence_traits TR, storage_modes STO>
	sequence_storage(sequence_storage<T, TR, STO>&& rhs) : m_storage(rhs.size())
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());
	}

	static constexpr size_t max_size() { return std::numeric_limits<size_type>::max(); }
	static constexpr size_t capacity() { return TRAITS.capacity; }
	static constexpr bool is_dynamic() { return false; }
	inline size_t size() const { return m_storage.size(); }
	inline bool empty() const { return m_storage.empty(); }

	inline void pop_front() { assert(!empty()); m_storage.pop_front(); }
	inline void pop_back() { assert(!empty()); m_storage.pop_back(); }
	inline void erase(value_type* begin, value_type* end) { assert(!empty()); m_storage.erase(begin, end); }
	inline void erase(value_type* element) { assert(!empty()); m_storage.erase(element); }
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

	inline auto data_begin() { return m_storage.data_begin(); }
	inline auto data_end() { return m_storage.data_end(); }
	inline auto data_begin() const { return m_storage.data_begin(); }
	inline auto data_end() const { return m_storage.data_end(); }
	inline auto capacity_begin() { return m_storage.capacity_begin(); }
	inline auto capacity_end() { return m_storage.capacity_end(); }
	inline auto capacity_begin() const { return m_storage.capacity_begin(); }
	inline auto capacity_end() const { return m_storage.capacity_end(); }

	inline void reallocate(size_t new_capacity)
	{
		if (new_capacity > TRAITS.capacity)
			throw std::bad_alloc();
	}
	inline void set_size(size_t size) { m_storage.set_size(size); }
	inline void prepare_for(size_t size) { m_storage.prepare_for(size); }
	inline auto new_data_start(size_t size) { return m_storage.new_data_start(size); }

private:

	friend class sequence_storage;

	storage_type m_storage;
};

// FIXED storage.

template<typename T, sequence_traits TRAITS>
class sequence_storage<T, TRAITS, storage_modes::FIXED>
{
	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;
	using storage_type = fixed_sequence_storage<T, TRAITS>;

public:

	inline sequence_storage() = default;
	inline sequence_storage(size_t new_capacity) : m_storage(new storage_type(new_capacity)) {}

	inline sequence_storage(const sequence_storage& rhs)
	{
		if (!rhs.empty())
		{
			m_storage.reset(new storage_type);
			*m_storage = *rhs.m_storage;
		}
	}
	inline sequence_storage(sequence_storage&& rhs) :
		m_storage(std::move(rhs.m_storage))
	{}

	template<sequence_traits TR, storage_modes STO>
	sequence_storage(sequence_storage<T, TR, STO>&& rhs)
	{
		if (rhs.m_storage)
		{
			m_storage.reset(new storage_type);
			std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
			set_size(rhs.size());
		}
	}

	inline sequence_storage& operator=(const sequence_storage& rhs)
	{
		if (!rhs.empty())
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

	inline void pop_front() { assert(!empty()); m_storage->pop_front(); }
	inline void pop_back() { assert(!empty()); m_storage->pop_back(); }
	inline void erase(value_type* begin, value_type* end) { assert(!empty()); m_storage->erase(begin, end); }
	inline void erase(value_type* element) { assert(!empty()); m_storage->erase(element); }
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
		return m_storage->add_at(pos, std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add_front(ARGS&&... args)
	{
		m_storage->add_front(std::forward<ARGS>(args)...);
	}
	template<typename... ARGS>
	inline void add_back(ARGS&&... args)
	{
		m_storage->add_back(std::forward<ARGS>(args)...);
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
	inline void set_size(size_t size) { assert(m_storage); m_storage->set_size(size); }
	inline void prepare_for(size_t size) { assert(m_storage); m_storage->prepare_for(size); }
	inline auto new_data_start(size_t size) { assert(m_storage); return m_storage->new_data_start(size); }

private:

	friend class sequence_storage;

	std::unique_ptr<storage_type> m_storage;
};

// VARIABLE storage.

template<typename T, sequence_traits TRAITS>
class sequence_storage<T, TRAITS, storage_modes::VARIABLE>
{
	using value_type = T;
	using iterator = value_type*;
	using dynamic_type = dynamic_sequence_storage<T, TRAITS>;

public:

	inline sequence_storage() = default;
	inline sequence_storage(size_t new_capacity) : m_storage(new_capacity) {}

	template<sequence_traits TR, storage_modes STO> requires (can_grow(STO))
	sequence_storage(sequence_storage<T, TR, STO>&& rhs)
	{
		if (rhs.is_dynamic())
		{
			m_storage.swap(dynamic_type(std::move(rhs.get_dynamic_storage())));
		}
		else
		{
			m_storage.swap(dynamic_type(rhs.size()));
			std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
			set_size(rhs.size());
		}
	}
	template<sequence_traits TR, storage_modes STO>
	sequence_storage(sequence_storage<T, TR, STO>&& rhs) : m_storage(rhs.size())
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());
	}

	static constexpr size_t max_size() { return std::numeric_limits<size_t>::max(); }
	inline size_t capacity() const { return m_storage.capacity(); }
	static constexpr bool is_dynamic() { return true; }
	inline size_t size() const { return m_storage.size(); }
	inline bool empty() const { return m_storage.empty(); }

	inline auto data_begin() { return m_storage.data_begin(); }
	inline auto data_end() { return m_storage.data_end(); }
	inline auto data_begin() const { return m_storage.data_begin(); }
	inline auto data_end() const { return m_storage.data_end(); }
	inline auto capacity_begin() const { return m_storage.capacity_begin(); }
	inline auto capacity_end() const { return m_storage.capacity_end(); }

	inline void pop_front() { assert(!empty()); m_storage.pop_front(); }
	inline void pop_back() { assert(!empty()); m_storage.pop_back(); }
	inline void erase(value_type* begin, value_type* end) { assert(!empty()); m_storage.erase(begin, end); }
	inline void erase(value_type* element) { assert(!empty()); m_storage.erase(element); }
	inline void clear() { m_storage.clear(); }
	inline void free() { m_storage.free(); }

	inline void swap(sequence_storage& other)
	{
		m_storage.swap(other.m_storage);
	}

protected:

	template<sequence_traits TR, storage_modes STO>
	inline void assign(sequence_storage<T, TR, STO>&& rhs)
	{
		m_storage.assign(std::move(rhs.get_dynamic_storage()));
	}

	template<typename... ARGS>
	inline iterator add_at(iterator pos, ARGS&&... args) { return m_storage.add_at(pos, std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	inline void add_front(ARGS&&... args) { m_storage.add_front(std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	inline void add_back(ARGS&&... args) { m_storage.add_back(std::forward<ARGS>(args)...); }
	template<typename... ARGS>
	inline void add(size_t new_size, ARGS&&... args) { m_storage.add(new_size, std::forward<ARGS>(args)...); }

	inline void reallocate(size_t new_capacity)
	{
		m_storage.reallocate(new_capacity);
	}
	inline void set_size(size_t size) { m_storage.set_size(size); }
	inline void prepare_for(size_t size) { m_storage.prepare_for(size); }
	inline auto new_data_start(size_t size) { return m_storage.new_data_start(size); }

private:

	friend class sequence_storage;

	dynamic_type m_storage;

	dynamic_type& get_dynamic_storage() { return m_storage; }
};

// BUFFERED storage supporting a small object buffer optimization (like boost::small_vector).

template<typename T, sequence_traits TRAITS>
class sequence_storage<T, TRAITS, storage_modes::BUFFERED>
{
	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;

	enum { STC, DYN };
	using fixed_type = fixed_sequence_storage<T, TRAITS>;		// STC
	using dynamic_type = dynamic_sequence_storage<T, TRAITS>;	// DYN

public:

	inline sequence_storage() = default;
	inline sequence_storage(size_t new_capacity)
	{
		if (new_capacity <= TRAITS.capacity)
			m_storage.emplace<STC>(new_capacity);
		else
			m_storage.emplace<DYN>(new_capacity);
	}

	inline sequence_storage(const sequence_storage& rhs)
	{
		if (rhs.size() <= TRAITS.capacity)
			rhs.execute([&](auto&& storage){ m_storage.emplace<STC>(storage); });
		else
			m_storage = rhs.m_storage;
	}
	inline sequence_storage(sequence_storage&& rhs) = default;

	template<sequence_traits TR, storage_modes STO> requires (can_grow(STO))
	sequence_storage(sequence_storage<T, TR, STO>&& rhs)
	{
		if (rhs.is_dynamic())
		{
			m_storage.emplace<DYN>(std::move(rhs.get_dynamic_storage()));
		}
		else
		{
			if (rhs.size() <= TRAITS.capacity)
				m_storage.emplace<STC>();
			else
				m_storage.emplace<DYN>(rhs.size());

			std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
			set_size(rhs.size());
		}
	}
	template<sequence_traits TR, storage_modes STO>
	sequence_storage(sequence_storage<T, TR, STO>&& rhs) : sequence_storage(rhs.size())
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), new_data_start(rhs.size()));
		set_size(rhs.size());
	}

	inline sequence_storage& operator=(const sequence_storage& rhs)
	{
		if (rhs.size() <= TRAITS.capacity && is_dynamic())
			rhs.execute([&](auto&& storage){ m_storage.emplace<STC>(storage); });
		else
			m_storage = rhs.m_storage;
		return *this;
	}
	inline sequence_storage& operator=(sequence_storage&&) = default;

	static constexpr size_t max_size() { return std::numeric_limits<size_t>::max(); }
	inline size_t capacity() const { return execute([](auto&& storage){ return storage.capacity(); }); }
	inline bool is_dynamic() const { return m_storage.index() == DYN; }
	inline size_t size() const { return execute([](auto&& storage){ return storage.size(); }); }
	inline bool empty() const { return execute([](auto&& storage){ return storage.empty(); }); }

	inline void pop_front() { assert(!empty()); execute([](auto&& storage){ storage.pop_front(); }); }
	inline void pop_back() { assert(!empty()); execute([](auto&& storage){ storage.pop_back(); }); }
	inline void erase(value_type* begin, value_type* end) { assert(!empty()); execute([=](auto&& storage){ storage.erase(begin, end); }); }
	inline void erase(value_type* element) { assert(!empty()); execute([=](auto&& storage){ storage.erase(element); }); }
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

	template<sequence_traits TR, storage_modes STO>
	inline void assign(sequence_storage<T, TR, STO>&& rhs)
	{
		if (is_dynamic())
			get<DYN>(m_storage).assign(std::move(rhs.get_dynamic_storage()));
		else
			m_storage.emplace<DYN>(std::move(rhs.get_dynamic_storage()));
	}

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
			if (m_storage.index() == STC)	// We're moving out of the buffer: switch to dynamic storage.
				m_storage = dynamic_type(new_capacity, std::move(get<STC>(m_storage)));
			else							// We're already out of the buffer: adjust the dynamic capacity.		
				get<DYN>(m_storage).reallocate(new_capacity);
		}

		// The new capacity will fit in the buffer.
		else if (is_dynamic())				// We're moving into the buffer: switch to buffer storage.
				m_storage.emplace<STC>(dynamic_type(std::move(get<DYN>(m_storage))));	// The r-value temp preserves the capacity while we switch the varient.
		// If we're already in the buffer: do nothing (the buffer capacity cannot change).
	}
	inline void set_size(size_t size) { execute([size](auto&& storage){ storage.set_size(size); }); }
	inline void prepare_for(size_t size) { execute([size](auto&& storage){ storage.prepare_for(size); }); }
	inline auto new_data_start(size_t size) { return execute([size](auto&& storage){ return storage.new_data_start(size); }); }

private:

	friend class sequence_storage;

	std::variant<fixed_type, dynamic_type> m_storage;

	template<typename FUNC>
	inline auto execute(FUNC f)
	{ return m_storage.index() == STC ? f(get<STC>(m_storage)) : f(get<DYN>(m_storage)); }
	template<typename FUNC>
	inline auto execute(FUNC f) const
	{ return m_storage.index() == STC ? f(get<STC>(m_storage)) : f(get<DYN>(m_storage)); }

	dynamic_type& get_dynamic_storage() { assert(is_dynamic()); return get<DYN>(m_storage); }
};


// ==============================================================================================================
// sequence - This is the main class template.

export template<typename T, sequence_traits TRAITS = sequence_traits<size_t>()>
class sequence : public sequence_storage<T, TRAITS>
{
	using inherited = sequence_storage<T, TRAITS>;
	using inherited::data_begin;
	using inherited::data_end;
	using inherited::reallocate;
	using inherited::prepare_for;
	using inherited::new_data_start;
	using inherited::set_size;
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

	using inherited::capacity;
	using inherited::capacity_begin;
	using inherited::capacity_end;

	using inherited::max_size;
	using inherited::size;
	using inherited::empty;
	using inherited::is_dynamic;

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
	static_assert(traits.location != location_modes::MIDDLE || std::move_constructible<T>,
				  "Middle element location requires move-constructible types.");

	// A fixed capacity of any kind requires that the size type can represent a count up to the fixed capacity size.
	static_assert(traits.storage == storage_modes::VARIABLE ||
				  traits.capacity <= std::numeric_limits<size_type>::max(),
				  "Size type is insufficient to hold requested capacity.");

	inline sequence() = default;
	inline sequence(const sequence&) = default;
	inline sequence(sequence&&) = default;

	template<sequence_traits TR>
	sequence(const sequence<T, TR>& rhs) : inherited(rhs.size())
	{
		std::uninitialized_copy(rhs.begin(), rhs.end(), new_data_start(rhs.size()));
		set_size(rhs.size());			// This must come last in case of a copy exception.
	}
	template<sequence_traits TR>
	sequence(sequence<T, TR>&& rhs) : inherited(std::move(rhs)) {}

	inline sequence(std::initializer_list<value_type> il) : inherited(il.size())
	{
		std::uninitialized_copy(il.begin(), il.end(), new_data_start(il.size()));
		set_size(il.size());	// This must come last in case of a copy exception.
	}

	template<typename... ARGS>
	inline sequence(size_type n, ARGS&&... args)
	{
		if (n)
		{
			reallocate(std::max<size_t>(n, traits.capacity));
			add(n, std::forward<ARGS>(args)...);
		}
	}

	inline sequence& operator=(const sequence&) = default;
	inline sequence& operator=(sequence&&) = default;

	template<sequence_traits TR>
	inline sequence& operator=(const sequence<T, TR>& rhs)
	{
		clear();
		auto size = rhs.size();
		if (size > capacity())
			reallocate(size);
		std::uninitialized_copy(rhs.begin(), rhs.end(), new_data_start(size));
		set_size(size);

		return *this;
	}
	template<sequence_traits TR>
	inline sequence& operator=(sequence<T, TR>&& rhs)
	{
		if constexpr (can_grow(TRAITS.storage) && can_grow(TR.storage))
			if (rhs.is_dynamic())
			{
				inherited::assign(std::move(rhs));
				return *this;
			}

		clear();
		auto size = rhs.size();
		if (size > capacity())
			reallocate(size);
		std::uninitialized_move(rhs.begin(), rhs.end(), new_data_start(size));
		set_size(size);

		return *this;
	}

	inline sequence& operator=(std::initializer_list<value_type> il) { assign(il); return *this; }
	inline void assign(std::initializer_list<value_type> il)
	{
		clear();
		auto size = il.size();
		if (size > capacity())
			reallocate(size);
		std::uninitialized_copy(il.begin(), il.end(), new_data_start(size));
		set_size(size);
	}

	template<typename... ARGS>
	inline void assign(size_type n, ARGS&&... args)
	{
		clear();
		if (n > capacity())
			reallocate(std::max<size_t>(n, traits.capacity));
		add(n, std::forward<ARGS>(args)...);
	}

	template<typename IT>
		requires requires (IT i) { *i; }
	inline void assign(IT first, IT last)
	{
		clear();
		for (; first != last; ++first)
			if constexpr (traits.location == location_modes::BACK)
				emplace_front(*first);
			else
				emplace_back(*first);
	}


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

	inline size_t front_gap() const { return data_begin() - capacity_begin(); }
	inline size_t back_gap() const { return capacity_end() - data_end(); }

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
		if (new_capacity > capacity())
		{
			if (new_capacity > max_size())
				throw std::length_error(MAX_SIZE_ERROR);
			reallocate(new_capacity);
		}
	}
	inline void shrink_to_fit()
	{
		if (auto current_size = size(); current_size == 0)
			free();
		else if (current_size < capacity())
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

	template<typename... ARGS>
	inline void add(size_t count, ARGS&&... args)
	{
		if (empty())
			prepare_for(count);

		while (count--)
			if constexpr (traits.location == location_modes::BACK)
				add_front(std::forward<ARGS>(args)...);
			else
				add_back(std::forward<ARGS>(args)...);
	}

	static constexpr auto OUT_OF_RANGE_ERROR = "invalid sequence index {}";
	static constexpr auto MAX_SIZE_ERROR = "reserve size is greater than max_size";
};
