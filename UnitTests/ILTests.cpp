#include "CppUnitTest.h"

import sequence;
import life;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{

TEST_CLASS(ILTests)
{

	template<sequence_traits TRAITS>
	void il_construction_test()
	{
		using seq_type = sequence<life, TRAITS>;
		using val_type = typename seq_type::value_type;
		using rec_type = typename val_type::ident;

		{
			seq_type seq{1,2,3};

			if constexpr (TRAITS.storage == storage_modes::VARIABLE)
				Assert::AreEqual(size_t(3), seq.capacity());
			else if constexpr (TRAITS.storage == storage_modes::BUFFERED)
				Assert::AreEqual(std::max(TRAITS.capacity, size_t(3)), seq.capacity());
			else
				Assert::AreEqual(TRAITS.capacity, seq.capacity());
			Assert::AreEqual(size_t(3), seq.size());

			rec_type recs1[] = {
				{1,		VALUE_CONSTRUCT,	1},
				{2,		VALUE_CONSTRUCT,	2},
				{3,		VALUE_CONSTRUCT,	3},
				{4,		COPY_CONSTRUCT,		1},
				{5,		COPY_CONSTRUCT,		2},
				{6,		COPY_CONSTRUCT,		3},
				{3,		DESTRUCT,			3},
				{2,		DESTRUCT,			2},
				{1,		DESTRUCT,			1},
			};
			Assert::IsTrue(val_type::check_log(recs1));
		}

		rec_type recs2[] = {
			{4,		DESTRUCT,		1},
			{5,		DESTRUCT,		2},
			{6,		DESTRUCT,		3},
		};
		Assert::IsTrue(val_type::check_log(recs2));
	}

	template<sequence_traits TRAITS>
	void il_assignment_test(size_t capacity, size_t elements)
	{
		using seq_type = sequence<life, TRAITS>;
		using val_type = typename seq_type::value_type;
		using rec_type = typename val_type::ident;

		seq_type seq{1,2,3};
		seq.reserve(capacity);

		if constexpr (TRAITS.storage == storage_modes::VARIABLE)
			Assert::AreEqual(capacity, seq.capacity(), L"capacity");
		else if constexpr (TRAITS.storage == storage_modes::BUFFERED)
			Assert::AreEqual(std::max(TRAITS.capacity, capacity), seq.capacity(), L"capacity");
		else
			Assert::AreEqual(TRAITS.capacity, seq.capacity(), L"capacity");

		std::initializer_list<life> ils[7] = {
			{},
			{4},
			{4,5},
			{4,5,6},
			{4,5,6,7},
			{4,5,6,7,8},
			{4,5,6,7,8,9},
		};
		Assert::IsTrue(elements < 7, L"test elements too large");
		val_type::clear_log();

		seq = ils[elements];

		rec_type recs[] = {
			{4,		DESTRUCT,		1},
			{5,		DESTRUCT,		2},
			{6,		DESTRUCT,		3},
		};
		Assert::IsTrue(val_type::check_log(recs), L"destruction");

		if constexpr (TRAITS.location == location_modes::FRONT)
		{
			Assert::AreEqual(size_t(0), seq.front_gap(), L"front_gap");
			Assert::AreEqual(seq.capacity() - elements, seq.back_gap(), L"back_gap");
		}
		else if constexpr (TRAITS.location == location_modes::BACK)
		{
			Assert::AreEqual(seq.capacity() - elements, seq.front_gap(), L"front_gap");
			Assert::AreEqual(size_t(0), seq.back_gap(), L"back_gap");
		}
		else
		{
			auto front = (seq.capacity() - elements) / 2u;
			Assert::AreEqual(front, seq.front_gap(), L"front_gap");
			Assert::AreEqual(seq.capacity() - (front + elements), seq.back_gap(), L"back_gap");
		}
	}

public:
		
	TEST_METHOD_INITIALIZE(InitLife)
	{
		life::reset();
	}

	TEST_METHOD(Static_Front)
	{
		il_construction_test<{
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
			.capacity = 6
		}>();
	}

	TEST_METHOD(Static_Back)
	{
		il_construction_test<{
			.storage = storage_modes::LOCAL,
			.location = location_modes::BACK,
			.capacity = 6
		}>();
	}

	TEST_METHOD(Static_Middle)
	{
		il_construction_test<{
			.storage = storage_modes::LOCAL,
			.location = location_modes::MIDDLE,
			.capacity = 6
		}>();
	}

	TEST_METHOD(Fixed_Front)
	{
		il_construction_test<{
			.storage = storage_modes::FIXED,
			.location = location_modes::FRONT,
			.capacity = 6
		}>();
	}

	TEST_METHOD(Variable_Front)
	{
		il_construction_test<{
			.storage = storage_modes::VARIABLE,
			.location = location_modes::FRONT,
			.capacity = 6
		}>();
	}

	TEST_METHOD(Buffered_Front_Buf)
	{
		il_construction_test<{
			.storage = storage_modes::BUFFERED,
			.location = location_modes::FRONT,
			.capacity = 6
		}>();
	}

	TEST_METHOD(Buffered_Front_Dyn)
	{
		il_construction_test<{
			.storage = storage_modes::BUFFERED,
			.location = location_modes::FRONT,
			.capacity = 2
		}>();
	}

	TEST_METHOD(Assign_Static_Front)
	{
		il_assignment_test<{
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
			.capacity = 10
		}>(10, 4);
	}

	TEST_METHOD(Assign_Static_Back)
	{
		il_assignment_test<{
			.storage = storage_modes::LOCAL,
			.location = location_modes::BACK,
			.capacity = 10
		}>(10, 4);
	}

	TEST_METHOD(Assign_Static_Middle)
	{
		constexpr sequence_traits traits{
			.storage = storage_modes::LOCAL,
			.location = location_modes::MIDDLE,
			.capacity = 10
		};

		il_assignment_test<traits>(10, 4);
		life::reset();
		il_assignment_test<traits>(10, 5);
	}

	TEST_METHOD(Overfill)
	{
		using typ = sequence<int, {
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
			.capacity = 6
		}>;
		
		Assert::ExpectException<std::bad_alloc>(
			[](){ typ seq{1,2,3,4,5,6,7}; },
			L"IL construct too many");
	}

};

}
