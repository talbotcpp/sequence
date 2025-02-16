#include "CppUnitTest.h"

import sequence;
import life;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{

TEST_CLASS(LocalTests)
{
	TEST_METHOD_INITIALIZE(InitLife)
	{
		life::reset();
	}

	TEST_METHOD(Size_Type)
	{
		sequence<int8_t, sequence_traits<uint64_t>{
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
			.capacity = 3
		}> seq1;
		Assert::AreEqual(size_t(16), sizeof(seq1));

		sequence<int8_t, sequence_traits<uint32_t>{
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
			.capacity = 3
		}> seq2;
		Assert::AreEqual(size_t(8), sizeof(seq2));

		sequence<int8_t, sequence_traits<uint16_t>{
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
			.capacity = 3
		}> seq3;
		Assert::AreEqual(size_t(6), sizeof(seq3));

		sequence<int8_t, sequence_traits<uint8_t>{
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
			.capacity = 3
		}> seq4;
		Assert::AreEqual(size_t(4), sizeof(seq4));
	}
	TEST_METHOD(Copy)
	{
		using typ = sequence<life, {
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
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
		using typ = sequence<life, {
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
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

	TEST_METHOD(Overfill)
	{
		using typ = sequence<life, {
			.storage = storage_modes::LOCAL,
			.location = location_modes::FRONT,
			.capacity = 6
		}>;
		typ seq{1,2,3,4,5,6};

		Assert::ExpectException<std::bad_alloc>(
			[&](){ seq.push_back(7); },
			L"push_back too many");
	}

};

}
