export module sequence:storage;
import :traits;
import :fixed;
import :dynamic;

import std;

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

	sequence_storage() = default;
	sequence_storage(std::initializer_list<value_type> il) : m_storage(il) {}

	constexpr static size_t capacity() { return TRAITS.capacity; }
	size_t size() const { return m_storage.size(); }
	size_t max_size() const { return std::numeric_limits<size_type>::max(); }
	bool is_dynamic() const { return false; }

	void clear() { m_storage.clear(); }
	void erase(value_type* begin, value_type* end) { m_storage.erase(begin, end); }
	void erase(value_type* element) { m_storage.erase(element); }
	void pop_front() { m_storage.pop_front(); }
	void pop_back() { m_storage.pop_back(); }

	void swap(sequence_storage& other)
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
class sequence_storage<sequence_storage_lits::FIXED, T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using size_type = typename decltype(TRAITS)::size_type;
	using storage_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;

public:

	sequence_storage() = default;
	sequence_storage(std::initializer_list<value_type> il) : m_storage(new storage_type(il)) {}

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

	void swap(sequence_storage& other)
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
class sequence_storage<sequence_storage_lits::VARIABLE, T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;

public:

	sequence_storage() = default;
	sequence_storage(std::initializer_list<value_type> il) : m_storage(il) {}

	size_t capacity() const { return m_storage.capacity(); }
	size_t size() const { return m_storage.size(); }
	size_t max_size() const { return std::numeric_limits<size_t>::max(); }
	bool is_dynamic() const { return true; }

	void clear() { m_storage.clear(); }
	void erase(value_type* begin, value_type* end) { m_storage.erase(begin, end); }
	void erase(value_type* element) { m_storage.erase(element); }
	void pop_front() { m_storage.pop_front(); }
	void pop_back() { m_storage.pop_back(); }

	void swap(sequence_storage& other)
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
class sequence_storage<sequence_storage_lits::BUFFERED, T, TRAITS>
{
	using value_type = T;
	using iterator = value_type*;
	using fixed_type = fixed_sequence_storage<TRAITS.location, T, TRAITS>;		// STC
	using dynamic_type = dynamic_sequence_storage<TRAITS.location, T, TRAITS>;	// DYN
	enum { STC, DYN };

public:

	sequence_storage() = default;
	sequence_storage(std::initializer_list<value_type> il)
	{
		if (il.size() <= TRAITS.capacity)
			m_storage.emplace<STC>(il);
		else
			m_storage.emplace<DYN>(il);
	}

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

	void swap(sequence_storage& other)
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

