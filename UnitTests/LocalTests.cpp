#include "CppUnitTest.h"

import sequence;
import life;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{

TEST_CLASS(UnitTests)
{
	TEST_METHOD_INITIALIZE(InitLife)
	{
		life<true>::reset();
		life<false>::reset();
	}

TEST_METHOD(Copy)
{
	using typ = sequence<life<true>, {
		.storage = sequence_storage_lits::LOCAL,
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
		.storage = sequence_storage_lits::LOCAL,
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
