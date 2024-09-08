export module sequence:utilities;
import :traits;

import std;
import <assert.h>;

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
concept sequence_storage_implementation = requires(T& t)
{
    t.capacity_begin();
    t.capacity_end();
    t.data_begin();
    t.data_end();
    t.size();
};
