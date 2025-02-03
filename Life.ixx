// Lifetime metered object for testing containers and such. This monitors all
// contruction, destruction and assignment, and it keeps track of lifetimes with
// a static counter.

export module life;

import <print>;


#define NOEX noexcept

export struct life
{
	life() : birth(++count) { std::println("  life() {}/{}", i, birth); }
	life(int i) : i(i), birth(++count) { std::println("  life(int) {}/{}", i, birth); }
	life(const life& l) : i(l.i), birth(++count)
	{
		std::println("  life(const life&) {}/{}", i, birth);
	}
	life(life&& l) NOEX : i(l.i), birth(++count)
	{
		std::println("  life(life&&) {}/{}", i, birth);
		l.i = 666;
	}
	~life()
	{
		std::println("  ~life {}/{}", i, birth);
		i = 99999;
	}

	life& operator=(const life& l)
	{
		i = l.i;
		std::println("  op=(const life&) {}/{}", i, birth);
		return *this;
	}
	life& operator=(life&& l)
	{
		i = l.i;
		std::println("  op=(life&&) {}/{}", i, birth);
		l.i = 666;
		return *this;
	}

	operator int() const { return i; }

	static int count;
	int i = 42, birth;
};

int life::count = 0;
