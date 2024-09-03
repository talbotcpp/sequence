# sequence
A contiguous sequence container with adjustable performance characteristics.

This container may be thought of as a much more flexible and controllable `std::vector`.
It offers control over where and how the capacity is stored in memory, and how the elements are managed
within the capacity. It also offers control over the way the capacity grows if growth is desired.

## Disclaimers

This is a prototype—a toy implementation meant to be a proof-of-concept and illustration.
It is incomplete in many ways, and is not production-ready code. Specifically it is missing a number of important
features, including allocators, exception handling, and especially test tooling and a comprehensive test suite.
The latter implies that it is almost completely untested.

## sequence class

The sequence class is parameterized on the element type and an instance of a struct non-type
template parameter of type `sequence_traits`.

```C++
template<typename T, sequence_traits TRAITS = sequence_traits<size_t>()>
class sequence;
```
# sequence_traits structure

The adjustable characteristics are controlled by the `sequence_traits` structure. The default version gives
behavior (more or less) identical to `std::vector`. The template is parameterized on the size type.

```C++
template<std::unsigned_integral SIZE = size_t>
struct sequence_traits;
```
## size_type
```
using size_type = SIZE;
```

This type is used to store sizes (and gaps) in fixed-size capacity situations. It allows small sequences of
small types to be made significantly more space efficient by representing the size with a
smaller type than `size_t`. This size reduction may (or may not) also result in faster run times.
It is _not_ used for variable-size capacity situations, namely
VARIABLE storage and BUFFERED storage when the capacity is dynamically allocated.
It is also _not_ used in this structure. (Using it for `capacity` complicates
the code without offering any real benefits, and it's not correct for `increment`
because the SBO may be small but the possible dynamic size large enough to require a large fixed growth value.)

## storage
```C++
sequence_storage_lits storage = sequence_storage_lits::VARIABLE;
```

This member specifies how the capacity is handled in memory. It offers four storage options:

#### STATIC
The capacity is embedded in the sequence object (like `std::inplace_vector` or `boost::static_vector`).
The capacity cannot change size or move.
#### FIXED
The capacity is dynamically allocated. The capacity cannot change size. Clearing the sequence
deallocates the capacity. Erasing the sequence does not deallocate the capacity. This is like
a `std::vector` which has been reserved and not allowed to reallocate, except that the in-class
storage is only one pointer. The size(s) are stored in the dynamic allocation and are represented by the `size_type`.
#### VARIABLE
The capacity is dynamically allocated (like `std::vector`). The capacity can change and move.
Neither clearing nor erasing the sequence deallocates the capacity (like `std::vector`).
#### BUFFERED
The capacity may be embedded (buffered) or dynamically allocated (like `boost::small_vector`).
The capacity can change and move. Clearing the sequence deallocates the capacity if it was
dynamically allocated. Erasing the sequence does not deallocate the capacity. Reserving a
capacity less than or equal to the fixed capacity size has no effect. Reserving a capacity
greater than the fixed capacity size causes the capacity to be dynamically (re)allocated. Calling
`shrink_to_fit` when the capacity is buffered has no effect. Calling it when the capacity is
dynamically allocated and the size is greater than the fixed capacity size has the expected
effect (as with `std::vector`). Calling it when the capacity is dynamically allocated and the
size is less than or equal to the fixed capacity size causes the capacity to be rebuffered
and the dynamic capacity to be deallocated.

## location
```C++
sequence_location_lits location = sequence_location_lits::FRONT;
```

This member specifies how the elements are managed within the capacity. It offers three location options:

#### FRONT
Elements always start at the lowest memory location. This makes `push_back` most efficient (like `std::vector`).
#### BACK
Elements always end at the highest memory location. This makes `push_front` most efficient.
#### MIDDLE
Elements float in the middle of the capacity. This makes both push_back and push_front generally efficient.
(In this way it is similar to `std::deque`, but note that `std::deque` is a very different container
with very different performance characteristics.)

## growth
```C++
sequence_growth_lits growth = sequence_growth_lits::VECTOR;
```

This member specifies how the capacity grows for the VARIABLE and BUFFERED storage options when the capacity is exceeded.
It offers three growth options:

#### LINEAR
The capacity grows by a fixed amount specified by `increment` (see below).
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
This member provides the size of the fixed capacity for `STATIC` and `FIXED` storages.
For `VARIABLE` storage it is the initial capacity when rallocation first occurs.
(Newly constructed empty containers have no capacity, and containers constructed from initializer lists
have a capacity equal to the size of the initializer list.)
For `BUFFERED` storage it is the size of the small object optimization buffer (SBO).
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
This member provides the factor by which the capacity grows when `EXPONENTIAL` growth is selected,
but if the change in size calculated by multiplying the capacity by the factor
is less than `increment`, the capacity will grow by `increment`. This value must be greater than 1.

# sequence class

## swap
```C++
void swap(sequence& other);
```
For `FIXED` and `VARIABLE` storage modes, this member provides O(1) swap. For `BUFFERED` storage,
it will provide O(1) swap for two unbuffered containers, but will be O(n) if one or both are buffered.
For `STATIC` storage it provides O(n) swap (as if by `std::swap`).
