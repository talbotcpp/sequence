#include "pch.h"
#include "CppUnitTest.h"

import sequence;
import life;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{

TEST_CLASS(UnitTests)
{
public:
		
TEST_METHOD(SimpleIL)
{
	using typ = sequence<life<true>, {
		.storage = sequence_storage_lits::STATIC,
		.location = sequence_location_lits::FRONT,
		.capacity = 6
	}>;

	typ::value_type::reset();

	{
	typ seq{1,2,3};

	Assert::AreEqual(size_t(6), seq.capacity());
	Assert::AreEqual(size_t(3), seq.size());

	typ::value_type::record records1[] = {
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
	Assert::IsTrue(typ::value_type::check_log(records1));
	}

	typ::value_type::record records2[] = {
		{4,		DESTRUCT,		1},
		{5,		DESTRUCT,		2},
		{6,		DESTRUCT,		3},
	};
	Assert::IsTrue(typ::value_type::check_log(records2));
}

TEST_METHOD(Copy)
{
	using typ = sequence<life<true>, {
		.storage = sequence_storage_lits::STATIC,
		.location = sequence_location_lits::FRONT,
		.capacity = 6
	}>;

	typ::value_type::reset();

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

TEST_METHOD(Move)
{
	using typ = sequence<life<true>, {
		.storage = sequence_storage_lits::STATIC,
		.location = sequence_location_lits::FRONT,
		.capacity = 6
	}>;

	typ::value_type::reset();

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

};

}
