export module sequence:dynamic;
import :traits;
import :utilities;

import std;
import <assert.h>;

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
	dynamic_sequence_storage(std::initializer_list<value_type> il) : inherited(il.size())
	{
		std::uninitialized_copy(il.begin(), il.end(), capacity_begin());
		m_data_end = capacity_end();
	}
	template<sequence_storage_implementation SEQ>
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
	dynamic_sequence_storage(std::initializer_list<value_type> il) : inherited(il.size())
	{
		std::uninitialized_copy(il.begin(), il.end(), capacity_begin());
		m_data_begin = capacity_begin();
	}
	template<sequence_storage_implementation SEQ>
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
	dynamic_sequence_storage(std::initializer_list<value_type> il) : inherited(il.size())
	{
		std::uninitialized_copy(il.begin(), il.end(), capacity_begin());
		m_data_begin = capacity_begin();
		m_data_end = capacity_end();
	}
	template<sequence_storage_implementation SEQ>
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
