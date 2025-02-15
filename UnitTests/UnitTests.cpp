#include "pch.h"
#include "CppUnitTest.h"

import sequence;
import life;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{

TEST_CLASS(UnitTests)
{

	template<sequence_traits TRAITS>
	void il_construction_test()
	{
		using seq_type = sequence<life<true>, TRAITS>;
		using val_type = typename seq_type::value_type;
		using rec_type = typename val_type::record;

		{
			seq_type seq{1,2,3};

			if constexpr (TRAITS.storage == sequence_storage_lits::VARIABLE)
				Assert::AreEqual(size_t(3), seq.capacity());
			else if constexpr (TRAITS.storage == sequence_storage_lits::BUFFERED)
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
		using seq_type = sequence<life<true>, TRAITS>;
		using val_type = typename seq_type::value_type;
		using rec_type = typename val_type::record;

		seq_type seq{1,2,3};
		seq.reserve(capacity);

		if constexpr (TRAITS.storage == sequence_storage_lits::VARIABLE)
			Assert::AreEqual(capacity, seq.capacity(), L"capacity");
		else if constexpr (TRAITS.storage == sequence_storage_lits::BUFFERED)
			Assert::AreEqual(std::max(TRAITS.capacity, capacity), seq.capacity(), L"capacity");
		else
			Assert::AreEqual(TRAITS.capacity, seq.capacity(), L"capacity");

		std::initializer_list<life<true>> ils[7] = {
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

		if constexpr (TRAITS.location == sequence_location_lits::FRONT)
		{
			Assert::AreEqual(size_t(0), seq.front_gap(), L"front_gap");
			Assert::AreEqual(seq.capacity() - elements, seq.back_gap(), L"back_gap");
		}
		else if constexpr (TRAITS.location == sequence_location_lits::BACK)
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
		life<true>::reset();
		life<false>::reset();
	}

	TEST_METHOD(IL_Static_Front)
	{
		il_construction_test<{
			.storage = sequence_storage_lits::STATIC,
			.location = sequence_location_lits::FRONT,
			.capacity = 6
		}>();
	}

	TEST_METHOD(IL_Static_Back)
	{
		il_construction_test<{
			.storage = sequence_storage_lits::STATIC,
			.location = sequence_location_lits::BACK,
			.capacity = 6
		}>();
	}

	TEST_METHOD(IL_Static_Middle)
	{
		il_construction_test<{
			.storage = sequence_storage_lits::STATIC,
			.location = sequence_location_lits::MIDDLE,
			.capacity = 6
		}>();
	}

	TEST_METHOD(IL_Fixed_Front)
	{
		il_construction_test<{
			.storage = sequence_storage_lits::FIXED,
			.location = sequence_location_lits::FRONT,
			.capacity = 6
		}>();
	}

	TEST_METHOD(IL_Variable_Front)
	{
		il_construction_test<{
			.storage = sequence_storage_lits::VARIABLE,
			.location = sequence_location_lits::FRONT,
			.capacity = 6
		}>();
	}

	TEST_METHOD(IL_Buffered_Front_Buf)
	{
		il_construction_test<{
			.storage = sequence_storage_lits::BUFFERED,
			.location = sequence_location_lits::FRONT,
			.capacity = 6
		}>();
	}

	TEST_METHOD(IL_Buffered_Front_Dyn)
	{
		il_construction_test<{
			.storage = sequence_storage_lits::BUFFERED,
			.location = sequence_location_lits::FRONT,
			.capacity = 2
		}>();
	}

	TEST_METHOD(IL_Assign_Static_Front)
	{
		il_assignment_test<{
			.storage = sequence_storage_lits::STATIC,
			.location = sequence_location_lits::FRONT,
			.capacity = 10
		}>(10, 4);
	}

	TEST_METHOD(IL_Assign_Static_Back)
	{
		il_assignment_test<{
			.storage = sequence_storage_lits::STATIC,
			.location = sequence_location_lits::BACK,
			.capacity = 10
		}>(10, 4);
	}

	TEST_METHOD(IL_Assign_Static_Middle)
	{
		constexpr sequence_traits traits{
			.storage = sequence_storage_lits::STATIC,
			.location = sequence_location_lits::MIDDLE,
			.capacity = 10
		};

		il_assignment_test<traits>(10, 4);
		life<true>::reset();
		il_assignment_test<traits>(10, 5);
	}


TEST_METHOD(Copy)
{
	using typ = sequence<life<true>, {
		.storage = sequence_storage_lits::STATIC,
		.location = sequence_location_lits::FRONT,
		.capacity = 6
	}>;

	{
	typ lhs{1,2,3};
	typ rhs{4,5,6,7};

	Assert::AreEqual(size_t(6), lhs.capacity());
	Assert::AreEqual(size_t(6), rhs.capacity());
	Assert::AreEqual(size_t(3), lhs.size());
	Assert::AreEqual(size_t(4), rhs.size());

	typ::value_type::clear_log();

	lhs = rhs;

	typ::value_type::record records[] = {
		{4,		DESTRUCT,		1},
		{5,		DESTRUCT,		2},
		{6,		DESTRUCT,		3},
		{15,	COPY_CONSTRUCT,	4},
		{16,	COPY_CONSTRUCT,	5},
		{17,	COPY_CONSTRUCT,	6},
		{18,	COPY_CONSTRUCT,	7},
	};
	Assert::IsTrue(typ::value_type::check_log(records));
	}
	typ::value_type::record records[] = {
		{11,	DESTRUCT,	4},
		{12,	DESTRUCT,	5},
		{13,	DESTRUCT,	6},
		{14,	DESTRUCT,	7},
		{15,	DESTRUCT,	4},
		{16,	DESTRUCT,	5},
		{17,	DESTRUCT,	6},
		{18,	DESTRUCT,	7},
	};
	Assert::IsTrue(typ::value_type::check_log(records));
}

TEST_METHOD(Move)
{
	using typ = sequence<life<true>, {
		.storage = sequence_storage_lits::STATIC,
		.location = sequence_location_lits::FRONT,
		.capacity = 6
	}>;

	{
	typ lhs{1,2,3};
	typ rhs{4,5,6,7};

	Assert::AreEqual(size_t(6), lhs.capacity());
	Assert::AreEqual(size_t(6), rhs.capacity());
	Assert::AreEqual(size_t(3), lhs.size());
	Assert::AreEqual(size_t(4), rhs.size());

	typ::value_type::clear_log();

	lhs = std::move(rhs);

	typ::value_type::record records[] = {
		{4,		DESTRUCT,		1},
		{5,		DESTRUCT,		2},
		{6,		DESTRUCT,		3},
		{15,	MOVE_CONSTRUCT,	4},
		{16,	MOVE_CONSTRUCT,	5},
		{17,	MOVE_CONSTRUCT,	6},
		{18,	MOVE_CONSTRUCT,	7},
	};
	Assert::IsTrue(typ::value_type::check_log(records));
	}
	typ::value_type::record records[] = {
		{11,	DESTRUCT,	MOVED_FROM},
		{12,	DESTRUCT,	MOVED_FROM},
		{13,	DESTRUCT,	MOVED_FROM},
		{14,	DESTRUCT,	MOVED_FROM},
		{15,	DESTRUCT,	4},
		{16,	DESTRUCT,	5},
		{17,	DESTRUCT,	6},
		{18,	DESTRUCT,	7},
	};
	Assert::IsTrue(typ::value_type::check_log(records));
}

};

}
