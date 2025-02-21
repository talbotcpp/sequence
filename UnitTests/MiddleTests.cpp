// MiddleTests.cpp
// These tests check the particular special issues with maintaining the MIDDLE element location.
// The size type tests for this mode are in the LocalTests file.

#include "CppUnitTest.h"

import sequence;
import life;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{

TEST_CLASS(MiddleTests)
{
	template<storage_modes STO>
	void shift_up()
	{
		using typ = sequence<life, {
			.storage = STO,
			.location = location_modes::MIDDLE,
			.capacity = 10
		}>;
		typ seq{1,2,3,4};
		seq.reserve(10);

		Assert::AreEqual(size_t(10), seq.capacity());
		Assert::AreEqual(size_t(4), seq.size());
		Assert::AreEqual(size_t(3), seq.front_gap());
		Assert::AreEqual(size_t(3), seq.back_gap());

		seq.emplace_back(5);
		seq.emplace_back(6);
		seq.emplace_back(7);

		Assert::AreEqual(size_t(7), seq.size());
		Assert::AreEqual(size_t(3), seq.front_gap());
		Assert::AreEqual(size_t(0), seq.back_gap());

		seq.emplace_back(8);

		Assert::AreEqual(size_t(8), seq.size());
		Assert::AreEqual(size_t(1), seq.front_gap());
		Assert::AreEqual(size_t(1), seq.back_gap());
	}

	template<storage_modes STO>
	void shift_down()
	{
		using typ = sequence<life, {
			.storage = STO,
			.location = location_modes::MIDDLE,
			.capacity = 10
		}>;
		typ seq{5,6,7,8};
		seq.reserve(10);

		Assert::AreEqual(size_t(10), seq.capacity());
		Assert::AreEqual(size_t(4), seq.size());
		Assert::AreEqual(size_t(3), seq.front_gap());
		Assert::AreEqual(size_t(3), seq.back_gap());

		seq.emplace_front(4);
		seq.emplace_front(3);
		seq.emplace_front(2);

		Assert::AreEqual(size_t(7), seq.size());
		Assert::AreEqual(size_t(0), seq.front_gap());
		Assert::AreEqual(size_t(3), seq.back_gap());

		seq.emplace_front(1);

		Assert::AreEqual(size_t(8), seq.size());
		Assert::AreEqual(size_t(1), seq.front_gap());
		Assert::AreEqual(size_t(1), seq.back_gap());
	}

public:

	TEST_METHOD_INITIALIZE(InitLife)
	{
		life::reset();
	}

	TEST_METHOD(ShiftUp_Local)
	{
		shift_up<storage_modes::LOCAL>();
	}

	TEST_METHOD(ShiftDown_Local)
	{
		shift_down<storage_modes::LOCAL>();
	}

	TEST_METHOD(ShiftUp_Variable)
	{
		shift_up<storage_modes::VARIABLE>();
	}

	TEST_METHOD(ShiftDown_Variable)
	{
		shift_down<storage_modes::VARIABLE>();
	}
};

}
