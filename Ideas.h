// ==============================================================================================================
// Concepts - Not publically available.

namespace {

template<typename T>
concept sequence_storage_implementation = requires(T& t)
{
    t.capacity_begin();
    t.capacity_end();
    t.data_begin();
    t.data_end();
    t.size();
};

}

template<typename T, sequence_traits TRAITS>
class fixed_sequence_storage<location_modes::???, T, TRAITS> : fixed_capacity<T, TRAITS.capacity>
{
	// FRONT
	template<sequence_storage_implementation SEQ>
	inline fixed_sequence_storage(SEQ&& rhs)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_size = static_cast<size_type>(rhs.size());
	}
	// BACK
	template<sequence_storage_implementation SEQ>
	inline fixed_sequence_storage(SEQ&& rhs)
	{
		m_size = static_cast<size_type>(rhs.size());
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_end() - m_size);
	}
	// MIDDLE
	template<sequence_storage_implementation SEQ>
	inline fixed_sequence_storage(SEQ&& rhs)
	{
		auto size = rhs.size();
		auto offset = TRAITS.front_gap(size);
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin() + offset);
		m_front_gap = static_cast<size_type>(offset);
		m_back_gap = static_cast<size_type>(TRAITS.capacity - (m_front_gap + size));
	}
}

template<typename T, sequence_traits TRAITS>
class dynamic_sequence_storage<location_modes::???, T, TRAITS> : public dynamic_capacity<T, TRAITS>
{
	// FRONT
	template<sequence_storage_implementation SEQ>
	inline dynamic_sequence_storage(size_t cap, SEQ&& rhs) :  inherited(cap)
	{
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), capacity_begin());
		m_data_end = capacity_begin() + rhs.size();
	}
	// BACK
	template<sequence_storage_implementation SEQ>
	inline dynamic_sequence_storage(size_t cap, SEQ&& rhs) :  inherited(cap)
	{
		m_data_begin = capacity_end() - rhs.size();
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), m_data_begin);
	}
	// MIDDLE
	template<sequence_storage_implementation SEQ>
	inline dynamic_sequence_storage(size_t cap, SEQ&& rhs) :  inherited(cap)
	{
		auto size = rhs.size();
		m_data_begin = capacity_begin() + TRAITS.front_gap(capacity(), size);
		m_data_end = m_data_begin + size;
		std::uninitialized_move(rhs.data_begin(), rhs.data_end(), m_data_begin);
	}
}


// This is a FRONT version of a smart assign.

	template<typename FSS>
	void assign(FSS&& rhs)
	{
		constexpr bool move = std::is_rvalue_reference_v<decltype(forward<FSS>(rhs))>;
	
		auto dst = data_begin();
		auto src = rhs.data_begin();

		while (src != rhs.data_end() && dst != data_end())
			if constexpr (move)
				*dst++ = std::move(*src++);
			else
				*dst++ = *src++;

		destroy_data(dst, data_end());

		if constexpr (move)
			std::uninitialized_move(src, rhs.data_end(), dst);
		else
			std::uninitialized_copy(src, rhs.data_end(), dst);

		set_data_area(rhs.data_area());
	}

