/*	Sequence.ixx
* 
*	Primary module interface file for general-purpose sequence container.
* 
*	Alan Talbot
*/

export module sequence;
export import :traits;
import :utilities;
import :storage;
import :fixed;
import :dynamic;

import std;
import <assert.h>;


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
	sequence(std::initializer_list<value_type> il) : inherited(il) {}

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
	const_reverse_iterator	crbegin() const { return const_reverse_iterator(data_end()); }
	const_reverse_iterator	crend() const { return const_reverse_iterator(data_begin()); }

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

private:

	constexpr static auto OUT_OF_RANGE_ERROR = "invalid sequence index {}";
};
