export module sequence:fixed;
import :traits;
import :utilities;

import std;
import <assert.h>;

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
	fixed_sequence_storage(std::initializer_list<value_type> il)
	{
		assert(il.size() <= capacity());

		std::uninitialized_copy(il.begin(), il.end(), capacity_begin());
		m_size = static_cast<size_type>(il.size());
	}
	template<sequence_storage_implementation SEQ>
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
	fixed_sequence_storage(std::initializer_list<value_type> il)
	{
		assert(il.size() <= capacity());

		std::uninitialized_copy(il.begin(), il.end(), capacity_end() - il.size());
		m_size = static_cast<size_type>(il.size());
	}
	template<sequence_storage_implementation SEQ>
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
	fixed_sequence_storage(std::initializer_list<value_type> il)
	{
		assert(il.size() <= capacity());

		auto offset = TRAITS.front_gap(il.size());
		std::uninitialized_copy(il.begin(), il.end(), capacity_begin() + offset);
		m_front_gap = static_cast<size_type>(offset);
		m_back_gap = static_cast<size_type>(TRAITS.capacity - (m_front_gap + il.size()));
	}
	template<sequence_storage_implementation SEQ>
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
