#include "CppUnitTest.h"

import sequence;
import life;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{

TEST_CLASS(FixedStorageTests)
{
	using typ = sequence<int, {
		.storage = storage_modes::FIXED,
		.capacity = 10
	}>;

public:

	TEST_METHOD(Construct)
	{
		{	// Copy of populated container.
			typ rhs{1,2,3,4};
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			typ lhs = rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());
		}
		{	// Move of populated container.
			typ rhs{1,2,3,4};
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			typ lhs = std::move(rhs);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of new container (no capacity).
			typ rhs;
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			typ lhs = rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of new container (no capacity).
			typ rhs;
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			typ lhs = std::move(rhs);
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of empty container (has capacity).
			typ rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			typ lhs = rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());	// Capacity is not allocated on null copy.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of empty container (has capacity).
			typ rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			typ lhs = std::move(rhs);
			Assert::AreEqual(size_t(10), lhs.capacity());	// Capacity is acquired on null move.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
	}

	TEST_METHOD(Assign_New)	// Refer to comments on Fixed_Construct above for clarification of terms.
	{
		{	// Copy of populated container to new container.
			typ lhs, rhs{1,2,3,4};
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());
		}
		{	// Move of populated container to new container.
			typ lhs, rhs{1,2,3,4};
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of new container to new container.
			typ lhs, rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of new container to new container.
			typ lhs, rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of empty container to new container.
			typ lhs, rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());	// Capacity is not allocated on empty copy.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of empty container to new container.
			typ lhs, rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(10), lhs.capacity());	// Capacity is acquired on empty move.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
	}

	TEST_METHOD(Assign_Pop)	// Refer to comments on Fixed_Construct above for clarification of terms.
	{
		{	// Copy of populated container to populated container.
			typ lhs{42}, rhs{1,2,3,4};
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());
		}
		{	// Move of populated container to populated container.
			typ lhs{42}, rhs{1,2,3,4};
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());	// These 2 tests may be over-specification.
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of new container to populated container.
			typ lhs{42}, rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());	// No loss of allocation on null copy.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of new container to populated container.
			typ lhs{42}, rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(0), lhs.capacity());	// Null move causes loss of allocation.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of empty container to populated container.
			typ lhs{42}, rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());	// No loss of allocation on empty copy.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of empty container to populated container.
			typ lhs{42}, rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(10), lhs.capacity());	// Capacity is swapped on empty move.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());	// These 2 tests may be over-specification.
			Assert::AreEqual(size_t(0), rhs.size());
		}
	}

	TEST_METHOD(Shrink)
	{
		{	// Shrink when size < capacity.
			typ lhs{1,2,3,4};
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());

			lhs.shrink_to_fit();
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
		}
		{	// Shrink when size == 0.
			typ lhs;
			lhs.reserve(1);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());

			lhs.shrink_to_fit();
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
		}
	}
};

TEST_CLASS(VariableStorageTests)
{
	using typ = sequence<int, {
		.storage = storage_modes::VARIABLE,
		.capacity = 10
	}>;

public:

	TEST_METHOD(Construct)
	{
		{	// Copy of populated container.
			typ rhs{1,2,3,4};
			Assert::AreEqual(size_t(4), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			typ lhs = rhs;
			Assert::AreEqual(size_t(4), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(4), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());
		}
		{	// Move of populated container.
			typ rhs{1,2,3,4};
			Assert::AreEqual(size_t(4), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			typ lhs = std::move(rhs);
			Assert::AreEqual(size_t(4), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of new container (no capacity).
			typ rhs;
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			typ lhs = rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of new container (no capacity).
			typ rhs;
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			typ lhs = std::move(rhs);
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of empty container (has capacity).
			typ rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(1), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			typ lhs = rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());	// Capacity is not allocated on null copy.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(1), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of empty container (has capacity).
			typ rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(1), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			typ lhs = std::move(rhs);
			Assert::AreEqual(size_t(1), lhs.capacity());	// Capacity is acquired on null move.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
	}

	TEST_METHOD(Assign_New)	// Refer to comments on Fixed_Construct above for clarification of terms.
	{
		{	// Copy of populated container to new container.
			typ lhs, rhs{1,2,3,4};
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(4), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(4), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(4), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());
		}
		{	// Move of populated container to new container.
			typ lhs, rhs{1,2,3,4};
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(4), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(4), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of new container to new container.
			typ lhs, rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of new container to new container.
			typ lhs, rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());	// These 2 tests may be over-specification.
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Copy of empty container to new container.
			typ lhs, rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(1), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(0), lhs.capacity());	// Capacity is not allocated on empty copy.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(1), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of empty container to new container.
			typ lhs, rhs; rhs.reserve(1);
			Assert::AreEqual(size_t(0), lhs.capacity());
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(1), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(1), lhs.capacity());	// Capacity is acquired on empty move.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());	// These 2 tests may be over-specification.
			Assert::AreEqual(size_t(0), rhs.size());
		}
	}

	TEST_METHOD(Assign_Pop)	// Refer to comments on Fixed_Construct above for clarification of terms.
	{
		{	// Copy of populated container to populated container.
			typ lhs, rhs{1,2,3,4};
			lhs.push_back(42);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(4), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(4), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());
		}
		{	// Move of populated container to populated container.
			typ lhs, rhs{1,2,3,4};
			lhs.push_back(42);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(4), rhs.capacity());
			Assert::AreEqual(size_t(4), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(4), lhs.capacity());
			Assert::AreEqual(size_t(4), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());	// These 2 tests may be over-specification.
			Assert::AreEqual(size_t(1), rhs.size());
		}
		{	// Copy of new container to populated container.
			typ lhs, rhs;
			lhs.push_back(42);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());	// No loss of allocation on null copy.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of new container to populated container.
			typ lhs, rhs;
			lhs.push_back(42);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(0), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(0), lhs.capacity());	// Null move causes loss of allocation.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());	// These 2 tests may be over-specification.
			Assert::AreEqual(size_t(1), rhs.size());
		}
		{	// Copy of empty container to populated container.
			typ lhs, rhs; rhs.reserve(1);
			lhs.push_back(42);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(1), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = rhs;
			Assert::AreEqual(size_t(10), lhs.capacity());	// No loss of allocation on empty copy.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(1), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());
		}
		{	// Move of empty container to populated container.
			typ lhs, rhs; rhs.reserve(1);
			lhs.push_back(42);
			Assert::AreEqual(size_t(10), lhs.capacity());
			Assert::AreEqual(size_t(1), lhs.size());
			Assert::AreEqual(size_t(1), rhs.capacity());
			Assert::AreEqual(size_t(0), rhs.size());

			lhs = std::move(rhs);
			Assert::AreEqual(size_t(1), lhs.capacity());	// Capacity is swapped on empty move.
			Assert::AreEqual(size_t(0), lhs.size());
			Assert::AreEqual(size_t(10), rhs.capacity());	// These 2 tests may be over-specification.
			Assert::AreEqual(size_t(1), rhs.size());
		}
	}

};

}
