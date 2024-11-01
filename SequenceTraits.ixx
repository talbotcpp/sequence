export module sequence:traits;
import std;

// sequence_lits - Literals used to specify the values of various sequence traits.
// These are hoisted out of the class template to avoid template dependencies.
// See sequence_traits below for a detailed discussion of these values.

export enum class sequence_storage_lits { STATIC, FIXED, VARIABLE, BUFFERED };	// See sequence_traits::storage.
export enum class sequence_location_lits { FRONT, BACK, MIDDLE };				// See sequence_traits::location.
export enum class sequence_growth_lits { LINEAR, EXPONENTIAL, VECTOR };			// See sequence_traits::growth.

// sequence_traits - Structure used to supply the sequence traits. The default values have been chosen
// to exactly model std::vector so that sequence can be used as a drop-in replacement with no adjustments.

export template<std::unsigned_integral SIZE = size_t>
struct sequence_traits
{
	// 'size_type' is the type of the size field for fixed storage. This allows small sequences of
	// small types to be made significantly more space efficient by representing the size with a
	// smaller type than size_t. This size reduction may (or may not) also result in faster run times.
	// (Note that this type is not used in this structure. Using it for 'capacity' complicates
	// the code without offering any real benefits, and it's not correct for 'increment'
	// because the SBO (see below) may be small but the dynamic size large.)

	using size_type = SIZE;

	// 'storage' specifies how the capacity is allocated:
	// 
	// STATIC	The capacity is embedded in the sequence object (like std::inplace_vector or boost::static_vector).
	//			The capacity cannot change size or move.
	//
	// FIXED	The capacity is dynamically allocated. The capacity cannot change size or move. Clearing the
	//			sequence deallocates the capacity. Erasing the sequence does not deallocate the capacity. This
	//			is like a std::vector which has been reserved and is not allowed to reallocate, except that
	//			the in-class storage is only one pointer. The size(s) are in the dynamic allocation and are
	//			represented by the size_type.
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
