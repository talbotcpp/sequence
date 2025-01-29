# Introduction
This library provides a contiguous sequence container with adjustable performance characteristics.
It may be thought of as a much more flexible and controllable `std::vector`.
Features include control over where and how the capacity is stored and managed, how the elements are managed
within the capacity, and control over the way the capacity grows if growth is required (and permitted).

The library is provided as a single module file "Sequence.ixx" which exports a module named "sequence".

## Disclaimers

This is a proof-of-concept implementation meant to illustrate the ideas embodied by sequence.
It is incomplete in many ways, and is not production-ready code. It is missing a number of important
features and lacks complete test tooling and a comprehensive test suite.
This latter implies that it is pretty much untested.

## Example

```C++
import std;
import sequence;

int main()
{
    sequence<int, {.storage = sequence_storage_lits::STATIC, .capacity = 10}>
    seq = {1,2,3,4,5,6};
    
    for (auto n : seq)
        std::print("{}\t", n);
    std::println();
}
```

# sequence class

```C++
template<typename T, sequence_traits TRAITS = sequence_traits<size_t>()>
class sequence;
```

The `sequence` class is parameterized on the element type and an instance of a struct non-type
template parameter of type `sequence_traits`. Most of the API has the same behavior
as for `std::vector`. The ways in which it differs are detailed below.

## traits_type
```C++
using traits_type = decltype(TRAITS);
```

The concrete sequence_traits type this sequence is instantiated with. 

## traits
```C++
static constexpr traits_type traits = TRAITS;
```

An instance of the traits_type available as a static member to make user access to its values convenient.

## size_type
```C++
using size_type = typename traits_type::size_type;
```

The container size type. This is described in detail below under `sequence_traits`.

## max_size
```C++
static constexpr size_t max_size();
```
Returns the largest theoretically supported size. The value is dependent on `size_type` only; it does not
take physical limitations into account.

## capacity
```C++
size_t capacity() const;
```
This member function returns the current size of the capacity. It has different behavior for each storage mode:

#### STATIC

Returns the fixed capacity size as defined by the `capacity` member of the `sequence_traits` template parameter.

#### FIXED

Returns the fixed capacity size as defined by the `capacity` member of the `sequence_traits` template parameter,
or zero if there is no capacity.

#### VARIABLE

Returns the current size of the dynamically allocated capacity, or zero if there is no capacity.

#### BUFFERED

Returns the fixed capacity size if the capacity is buffered, otherwise
it returns the current size of the dynamically allocated capacity. *Note: this implies that it will never
return a size smaller than the fixed capacity size.*

## is_dynamic
```C++
bool is_dynamic() const;
```
Returns `true` if the capacity is dynamically allocated. For `BUFFERED` storage sequences this can change at
runtime and answers the question, "Is the capacity on the heap (vs. buffered in the sequence object)?"
For all other storage modes, the value is determined at compile time and the function is `static constexpr`.
*Note: this function cannot be used to determine if the sequence is in a default-constructed state (i.e. has no capacity).
Use `capacity` to answer this question.*
## reserve
```C++
void reserve(size_t new_capacity);
```
This member function attempts to increase the capacity to be equal to its argument. If `capacity()` is
greater than or equal to `new_capacity`, it has no effect. Otherwise, it has different behavior for each storage mode:

#### STATIC

Throws `std::bad_alloc` if `new_capacity > capacity()`. Otherwise has no effect.

#### FIXED

Throws `std::bad_alloc` if `new_capacity > capacity()`. Otherwise if
the sequence has no capacity, pre-allocates the fixed capacity size.
Otherwise has no effect.

#### VARIABLE

Reallocates the capacity so its size is equal to `new_capacity`.

#### BUFFERED

If `new_capacity` is less than or equal to the fixed capacity size, has no effect.
Otherwise reallocates the capacity so its size is equal to `new_capacity`.

## shrink_to_fit
```C++
void shrink_to_fit();
```
This member function attempts to reduce the capacity to be equal to the size. If `capacity()` is
already equal to `size()`, it has no effect. Otherwise, it has different behavior for each storage mode:

#### STATIC

Has no effect.

#### FIXED

If `empty() == true`, equivalent to `free()`, otherwise has no effect.

#### VARIABLE

If `empty() == true`, equivalent to `free()`. Otherwise reallocates the capacity so its size is equal to `size()`.

#### BUFFERED

If the capacity is buffered, has no effect.
Otherwise, if `size()` is less than or equal to the fixed capacity size (the buffer size), rebuffers the elements
and deallocates the dynamic capacity.
Otherwise reallocates the capacity so its size is equal to `size()`.

## swap
```C++
void swap(sequence& other);
```
For `FIXED` and `VARIABLE` storage modes, this member provides O(1) swap. For `BUFFERED` storage,
it will provide O(1) swap for two unbuffered containers, but will be O(n) if one or both are buffered.
For `STATIC` storage it provides O(n) swap (as if by `std::swap`).

## resize
```C++
template<typename... ARGS>
inline void resize(size_type new_size, ARGS&&... args)
```
If `new_size` < `size()`, erases the last `size() - new_size` elements from the sequence, otherwise appends
`new_size - size()` elements to the sequence that are emplace-constructed from `args`.

## clear
```C++
void clear();
```
This member function erases (deletes) all of the elements in the container. 
It does not cause reallocation, deallocation or buffering. `seq.clear()` is equivalent to:
```C++
seq.erase(seq.begin(), seq.end());
```

## free
```C++
void free();
```
This member function erases (deletes) all of the elements in the container and deallocates
any dynamic storage, placing the container in a default-constructed state. After a free,
`FIXED` and `VARIABLE` storage sequences have no capacity. *Note: `STATIC` and `BUFFERED` storage sequences always have
a capacity that is at least the fixed capacity size.*

## Exceptions

Attempting to exceed a fixed capacity throws `std::bad_alloc`.
Attempting to access an element with `at(index)` where `index` is greater than or equal to `size()`
throws `std::out_of_range`.

# sequence_traits structure

```C++
template<std::unsigned_integral SIZE = size_t>
struct sequence_traits;
```
The adjustable characteristics are controlled by the `sequence_traits` structure. The default version gives
behavior (more or less) identical to `std::vector` so that sequence can be used as a drop-in replacement for, or
an implementation of, vector with no adjustments. The template is parameterized on the size type (see below).

## size_type
```C++
using size_type = SIZE;
```

This type is used to store sizes (and gaps) for sequences with fixed capacity. It allows small sequences of
small types to be made significantly more space efficient by representing the size with a
smaller type than `size_t`. This size reduction may (or may not) also result in faster run times.
It is _not_ used for variable-size capacity situations, namely
`VARIABLE` storage and `BUFFERED` storage when the capacity is dynamically allocated.
It is also _not_ used in this structure. (Using it for `capacity` complicates
the code without offering any real benefits, and it's not correct for `increment`
because the SBO may be small but the possible dynamic size large enough to require a large fixed growth value.)

## storage
```C++
enum class sequence_storage_lits { STATIC, FIXED, VARIABLE, BUFFERED };
sequence_storage_lits storage = sequence_storage_lits::VARIABLE;
```

This member specifies how the capacity is handled in memory. It offers four storage options:

#### STATIC
The capacity is fixed and embedded in the sequence object (like `std::inplace_vector` or `boost::static_vector`).
The capacity cannot change size or move, and it has the same lifetime as the container.
#### FIXED
The capacity is dynamically allocated. The capacity cannot change size or move once allocated.
A default-initialized sequence has no capacity. Clearing the sequence
deallocates the capacity (note that this is different behavior from `VARIABLE` or `std::vector`).
Erasing the sequence does not deallocate the capacity. The in-class
storage is only one pointer; the size(s) are stored in the dynamic allocation and are represented by the `size_type`.
#### VARIABLE
The capacity is dynamically allocated. The capacity can change and move.
A default-initialized sequence has no capacity.
Neither clearing nor erasing the sequence deallocates the capacity,
but an empty sequence can be restored to having no capacity by calling shrink_to_fit.
(All of these behaviors are exactly like `std::vector`.)
#### BUFFERED
Buffered sequences have a fixed capacity embedded in the sequence object which is used when the
size is less than or equal to the fixed capacity size; when the sequence grows larger than the
fixed capacity size, the capacity is allocated dynamically (like `boost::small_vector`).
The capacity can change and move. Clearing the sequence deallocates the capacity if it was
dynamically allocated. Erasing the sequence does not deallocate the capacity. Reserving a
capacity less than or equal to the fixed capacity size has no effect. Reserving a capacity
greater than the fixed capacity size causes the capacity to be dynamically (re)allocated.

## location
```C++
enum class sequence_location_lits { FRONT, BACK, MIDDLE };
sequence_location_lits location = sequence_location_lits::FRONT;
```

This member specifies how the elements are managed within the capacity. It offers three location options:

#### FRONT
Elements always start at the lowest memory location. This makes `push_back` most efficient (like `std::vector`).
#### BACK
Elements always end at the highest memory location. This makes `push_front` most efficient.
#### MIDDLE
Elements float in the middle of the capacity. This makes both push_back and push_front generally efficient.
(The general term for this structure is a double-ended queue. While it is similar in some ways to `std::deque`,
that container is implemented very differently and has very different performance characteristics.)


## growth
```C++
enum class sequence_growth_lits { LINEAR, EXPONENTIAL, VECTOR };
sequence_growth_lits growth = sequence_growth_lits::VECTOR;
```

This member specifies how the capacity grows for sequences with VARIABLE and BUFFERED storage when the capacity is exceeded.
It offers three growth options:

#### LINEAR
The capacity grows by a fixed number of elements specified by `increment` (see below).
#### EXPONENTIAL
The capacity grows exponentially by a factor specified by `factor` (see below).
#### VECTOR
Capacity grows in the same way as `std::vector`. This behavior is implementation dependent.
It is provided so that `sequence` can be used as an implementation of, or drop-in replacement
for, `std::vector` with no changes in behavior, even if the `std::vector`
growth behavior cannot be modeled with the `LINEAR` or `EXPONENTIAL` growth modes.

## capacity
```C++
size_t capacity = 1;
```
This member specifies the size of the fixed capacity or inital capacity. It has different behavior for each storage mode:

| Storage | Meaning |
| --- | --- |
| STATIC | The size of the fixed capacity. |
| FIXED | The fixed size of the dynamic capacity when allocation occurs. |
| VARIABLE | The initial size of the dynamic capacity when allocation first occurs. If the sequence is constructed from an initializer list, the capacity is equal to the size of the list. |
| BUFFERED | The size of the optimization buffer (the fixed capacity). If the sequence is constructed from an initializer list and the size of the list is greater than this value, the dynamic capacity is equal to the size of the list. |

Note that the common pattern of constructing a vector and immediately reserving a starting size is not necessary for sequences.
Setting this value to the desired initial reserve size will do this automatically and with lazy evaluation, without wasting allocations for containers which remain empty. 

This value must be greater than 0.

## increment
```C++
size_t increment = 1;
```
This member provides the linear capacity growth in elements.
For `LINEAR` growth it is the amount by which the capacity grows.
For `EXPONENTIAL` growth it is the minimum amount by which the capacity grows.

This value must be greater than 0.

## factor
```C++
float factor = 1.5;
```
When `EXPONENTIAL` growth is selected, the capacity will grow by the greater of (current capacity × `factor`) and `increment`.

This value must be greater than 1.

## grow
```C++
size_t grow(size_t cap) const;
```
This member returns a new (larger) capacity given the current capacity, calculated based on the `sequence_traits`
members which control capacity. It is used internally and is available publicly so that client code
can determine exactly how much memory (in terms of elements, not including allocation overhead) will be required if a sequence needs to reallocate.

# Open Questions

## Should move operations clear?
Should move operations clear the moved-from container after moving the elements? Doing so is rather tidy,
but it might introduce unnecessary work at the time of the move. If this isn't done (in other words, if a
lazy approach is taken) the container will delete the moved-from elements when it
is destructed. This might be much later, which might be a good thing in some situations.
## Should swap operations optimize on size?
If a O(n) swap takes place, should the algorithm check the container sizes and cache the smaller one?
This would be a win if the sizes are quite different or if the moves are expensive, but it adds
two O(1) size calculations and an integer comparison.
