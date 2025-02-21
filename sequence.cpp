import sequence;
import life;

import std;
//import <print>;
//#include <print>
//#include <vector>

using namespace std;

// show - Debugging display for sequence traits.

template<typename SEQ>
void show(const SEQ& seq)
{
	using traits_type = SEQ::traits_type;

	std::println("Size Type:\t{}", typeid(traits_type::size_type).name());
	std::println("Max Size:\t{}", seq.max_size());

	std::print("Storage:\t");
	switch (seq.traits.storage)
	{
		case storage_modes::LOCAL:		std::println("LOCAL");		break;
		case storage_modes::FIXED:		std::println("FIXED");		break;
		case storage_modes::VARIABLE:	std::println("VARIABLE");	break;
		case storage_modes::BUFFERED:	std::println("BUFFERED");	break;
	}
	std::print("Location:\t");
	switch (seq.traits.location)
	{
		case location_modes::FRONT:		std::println("FRONT");		break;
		case location_modes::MIDDLE:	std::println("MIDDLE");		break;
		case location_modes::BACK:		std::println("BACK");		break;
	}
	std::print("Growth:\t\t");
	switch (seq.traits.growth)
	{
		case growth_modes::LINEAR:		std::println("LINEAR");			break;
		case growth_modes::EXPONENTIAL:	std::println("EXPONENTIAL");	break;
		case growth_modes::VECTOR:		std::println("VECTOR");			break;
	}

	std::println("Capacity:\t{}", seq.traits.capacity);
	std::println("Increment:\t{}", seq.traits.increment);
	std::println("Factor:\t\t{}", seq.traits.factor);
	std::println("Size:\t\t{}", sizeof(seq));
	std::println();
}

// print_value - Prints special values as meaningful tags.

void print_value(int i)
{
	auto print = [](auto v){ std::print("{: >5}", v); };

	switch (i)
	{
	case DEFAULTED:		print("DEF"); break;
	case DESTRUCTED:	print("DST"); break;
	case MOVED_FROM:	print("MOV"); break;
	default:
		if (i >= 0 && i < 1000) print(i);
		else print("GAR");
		break;
	}
}

// show_elems - Simple element output for any container with elements convertible to int.

template<typename SEQ>
void show_elems(const SEQ& seq)
{
	std::print("{: >2}> ", seq.size());
	if (seq.empty()) std::print("EMPTY");
	else for (auto&& e : seq)
		print_value(e);
	std::println();
}

// show_cap - Shows the capacity viewed as a given type. Note that this is UB!

template<typename SEQ>
void show_cap(const SEQ& seq)
{
	std::print("{: >2}{} ", seq.capacity(), seq.is_dynamic() ? 'D' : 'S');
	if (seq.capacity_begin())
		for (auto p = seq.capacity_begin(); p != seq.capacity_end(); ++p)
			print_value(*p);
	else std::print(" NULL");
	std::println();
}
template<typename T>
void show_cap(const std::vector<T>& seq)
{
	std::print("{: >2}{} ", seq.capacity(), 'D');
	if (seq.data())
		for (auto p = seq.data(); p != seq.data() + seq.capacity(); ++p)
			print_value(*p);
	else std::print("  NULL");
	std::println();
}

template<typename T, long long CAP, std::unsigned_integral SIZE = size_t> requires ( CAP > 0 )
using static_vector = sequence<T, sequence_traits<SIZE>{ .storage = storage_modes::LOCAL, .capacity = CAP }>;

template<typename L>
void assign(L& l1, const L& l2)
{
	l1 = std::move_if_noexcept(l2);
}

struct C
{

    life* x = nullptr;

    C() {x = new life; throw 42;}

    ~C() {delete x;}

};

template<typename F>
void foo(F f)
{
	f();
}

int main()
{

#if 1
		using ty1 = sequence<life, {.storage = storage_modes::VARIABLE,
									.location = location_modes::FRONT,
									.capacity = 10	}>;
#else
		using typ = std::vector<life>;
//		using typ = std::array<life, 5>;
#endif
		using ty2 = sequence<life, {.storage = storage_modes::VARIABLE,
									.location = location_modes::FRONT,
									.capacity = 10	}>;

		using ety = life;
	{
	std::println("{:-^50}","v");
	ety::add_comment("Make v");
//	typ v;
	ty1 v = {1,2,3};
	v.reserve(10);
//	typ v{1,2,3,4,5,6};
	show_cap(v);
	show_elems(v);

	ety::add_comment("Make w");
	std::println("{:-^50}","w");
	ty2 w{4,5,6,7};
	w.reserve(10);
	show_cap(w);
	show_elems(w);

	ety::add_comment("Assign");
//	v = w;
	v = std::move(w);

	std::println("{:-^50}","v");
	show_cap(v);
	show_elems(v);

	std::println("{:-^50}","w");
	show_cap(w);
	show_elems(w);

	//typ::value_type::add_comment("Push");
	//v.push_back(7);

//	typ w{7,8,9,10,11,12};
////	typ w{v};
////	typ w{std::move(v)};
//	show_cap(w);
//	show_elems(w);
//
//	std::println("{:-^50}","assign to w");
//	w = v;
//	//w = std::move(v);
//	show_cap(w);
//	show_elems(w);
	ety::add_comment("End of Scope");
	}
	ety::print_log();


		using typ = sequence<life, {
			.storage = storage_modes::VARIABLE,
			.location = location_modes::MIDDLE,
			.capacity = 10
		}>;
		typ seq{5,6,7,8};
		seq.reserve(10);

		println();
		show_cap(seq);
		show_elems(seq);

		seq.emplace_front(4);
		seq.emplace_front(3);
		seq.emplace_front(2);

		println();
		show_cap(seq);
		show_elems(seq);

		seq.emplace_front(1);

		println();
		show_cap(seq);
		show_elems(seq);

		//typ seq2{1,2,3,4,5,6,7,8};
		//println();
		//show_cap(seq2);
		//show_elems(seq2);

/*
	println();
	ety::reset();
	ety::throw_at = 13;

	using ty3 = sequence<life_throws, {.storage = storage_modes::LOCAL,
								.location = location_modes::BACK,
								.capacity = 10	}>;

	ety::add_comment("Throw Test");
	ty3 s{1,2,3,4,5};
	ety::add_comment("Constructed");
	show_cap(s);
	try {
		shift(s.capacity_begin(), s.capacity_end(), s.begin(), s.end(), s.capacity_begin());
	}
	catch (ety::ident i)
	{
		print("EX: {} ", i.id);
		ety::print_operation(i.operation);
		ety::print_value(i.value);
		println();
	}
	show_cap(s);
	ety::print_log();

	println();
	ety::reset();
	ety::throw_at = 14;
	std::println("{:-^50}","Uninit Test");

	ety::add_comment("ar1");
	life_throws ar1[5] = {1,2,3,4,5};
	ety::add_comment("ar2");
	life_throws ar2[5];
	ety::add_comment("destruct");
	for (auto& e : ar2) e.~life_throws();
	ety::add_comment("move");
	try {
		std::uninitialized_move(ar1, ar1 + 5, ar2);
	}
	catch (ety::ident i)
	{
		print("EX: {} ", i.id);
		ety::print_operation(i.operation);
		ety::print_value(i.value);
		println();
	}
	ety::print_log();
*/
}

#ifdef NONONONO

	typ::value_type::add_comment("Make w");
	std::println("{:-^50}","w");
//	typ w;
//	typ w{7,8,9};
	typ w{7,8,9,10,11,12};
//	typ w{v};
//	typ w{std::move(v)};
	show_cap(w);
	show_elems(w);

	//std::println("{:-^50}","assign to w");
	//w = v;
	//w = std::move(v);
	//show_cap(w);
	//show_elems(w);

	typ::value_type::add_comment("swap");
	std::println("{:-^50}","swap");
	w.swap(v);
//	v.swap(w);

	std::println("{:-^50}","v");
	show_cap(v);
	show_elems(v);
	std::println("{:-^50}","w");
	show_cap(w);
	show_elems(w);


	typ::value_type::print_log();
	typ::value_type::record recs1[] = {
		{1,		VALUE_CONSTRUCT,	1},
		{2,		VALUE_CONSTRUCT,	2},
		{3,		VALUE_CONSTRUCT,	3},
		{4,		VALUE_CONSTRUCT,	4},
		{5,		VALUE_CONSTRUCT,	5},
		{6,		VALUE_CONSTRUCT,	6},
	};
	println("Check: {}", typ::value_type::check_log(recs1));

	typ::value_type::record recs2[] = {
		{7,		COPY_CONSTRUCT,	1},
		{8,		COPY_CONSTRUCT,	2},
		{9,		COPY_CONSTRUCT,	3},
		{10,	COPY_CONSTRUCT,	4},
		{11,	COPY_CONSTRUCT,	5},
		{12,	COPY_CONSTRUCT,	6},
	};
	println("Check: {}", typ::value_type::check_log(recs2));

	typ::value_type::clear_log();
	//typ seq{1,2,3};

	//typ::value_type::record records[] = {
	//	{1,		VALUE_CONSTRUCT,	1},
	//	{2,		VALUE_CONSTRUCT,	2},
	//	{3,		VALUE_CONSTRUCT,	3},
	//	{4,		COPY_CONSTRUCT,		1},
	//	{5,		COPY_CONSTRUCT,		2},
	//	{6,		COPY_CONSTRUCT,		3},
	//	{3,		DESTRUCT,			3},
	//	{2,		DESTRUCT,			2},
	//	{1,		DESTRUCT,			1},
	//};
	//typ::value_type::print_new_log();
	//println("Check: {}", typ::value_type::check_log(records));
	//}
	//typ::value_type::print_new_log();
	//typ::value_type::record records[] = {
	//	{4,		DESTRUCT,		1},
	//	{5,		DESTRUCT,		2},
	//	{6,		DESTRUCT,		3},
	//};
	//println("Check: {}", typ::value_type::check_log(records));

	std::println("{:-^50}","test");
	typ::value_type::reset();

	typ lhs{1,2,3};
	typ rhs{4,5,6,7};
	typ::value_type::print_new_log();
	println();

	typ::value_type::clear_log();


	//std::println("{:-^50}","v");
	//show_cap(v);
	//show_elems(v);

	//w.emplace(w.begin()+1, 42);

	//std::println("{:-^50}","reserve");
	//v.reserve(10);
	//show_cap(v);
	//show_elems(v);

	//std::println("{:-^50}","roll");
	//v.emplace_back(6);
	//v.pop_front();
	//v.emplace_back(7);
	//v.pop_front();
	//v.emplace_back(8);
	//v.pop_front();
	//v.emplace(v.end()-1, 9);
	//show_cap(v);
	//show_elems(v);
	
	/*

	show_cap(w);
	show_elems(w);
*/
	//std::println("{:-^50}","erase");
	//v.erase(v.begin() + 2);
	//show_cap(v);
	//show_elems(v);

	//std::println("{:-^50}","pop fr");
	//v.pop_front();
	//show_cap(v);
	//show_elems(v);

	//std::println("{:-^50}","pop bk");
	//v.pop_back();
	//show_cap(v);
	//show_elems(v);

	//std::println("{:-^50}","add");
	//typ x;
	//x.resize(5);
	//show_cap(x);
	//show_elems(x);

//	v = std::move(w);
/*
	std::println("{:-^50}","w=v");
	w = v;
//	v = std::move(w);

	*/
	//life i = 42;
	//life j;
	//j = std::move_if_noexcept(i);

	//std::println("{:-^50}","v");
	//show_cap(v);
	//show_elems(v);
	//std::println("{:-^50}","w");
	//show_cap(w);
	//show_elems(w);
}


	{
	println("{:-^50}","move");
	life l1(101);
	life l2(202);
	assign(l1, std::move(l2));
	//l1 = std::move(l2);
	}
	println();


	using styp = sequence<int, {.storage = storage_modes::LOCAL,
								.location = location_modes::FRONT,
								.capacity = 8	}>;

	//styp s;
	//show_cap(s);
	//show_elems(s);
/*

	std::println("{:-^50}","s");
//	styp s = {1,2,3,4,5};
	styp s = {1,2,3,4,5,6,7,8};
//	s.erase(s.begin()+3, s.end());
	show_cap(s);
	show_elems(s);

	std::println("{:-^50}","t");
//	styp t = {1,2,3,4,5,6,7,8};
	styp t = {11,22,33,44,55};

	show_cap(t);
	show_elems(t);
	std::println("{:-^50}","s=t");
	s = t;
//	t = s;
//	t = std::move(s);
	show_cap(t);
	show_elems(t);

	std::println("{:-^50}","s");
	show_cap(s);
	show_elems(s);

	//std::println("---------------------- stf ----------------------");
	//s.shrink_to_fit();
	//std::println("---------------------- reserve ----------------------");
	//s.reserve(8);

	//show_cap(s);
	//show_elems(s);

	std::println("---------------------- free -----------------------");
	s.free();

	show_cap(s);
	show_elems(s);

	std::println("--------------------- reserve ----------------------");
	s.reserve(6);

	show_cap(s);
	show_elems(s);

	std::println("------------------ shrink_to_fit -------------------");
	s.shrink_to_fit();

	show_cap(s);
	show_elems(s);

*/


	constexpr sequence_traits<unsigned char> traits {
			.storage = storage_modes::BUFFERED,
			.location = location_modes::FRONT,
			.growth = growth_modes::LINEAR,
			.capacity = 10,
			.increment = 1,
	};

	{
		//sequence<foo, traits> s1;
		//show_cap(s1);
		//show_elems(s1);

		//for (int i = 1; i <= 8; ++i)
		//	s1.emplace_front(i);
		//show_cap(s1);
		//show_elems(s1);
		//s1.emplace_front(9);


//		sequence<foo, traits> s1{1,2,3,4};
//		sequence<foo, traits> s1{1,2,3,4,5,6};
//		sequence<foo, traits> s1{1,2,3,4,5,6,7,8,9,10,11,12};
		//show_cap(s1);
		//show_elems(s1);
//		for (int i = 8; i > 0; --i)
//			s1.pop_front();
//			s1.pop_back();
//		s1.erase(s1.begin()+3);
//		s1.erase(s1.begin()+3, s1.begin()+5);
//		s1.pop_front();

//		s1.emplace(s1.begin()+3, 69);
//		s1.emplace_back(13);
//		s1.emplace_front(111);

		println("s2 ----------------------------------------------");

		sequence<foo, traits> s2{1,2,3,4};
//		sequence<foo, traits> s2{1,2,3,4,5,6,7,8,9,10};
		//for (int i = 8; i > 0; --i)
		//	s2.pop_front();
//
		//println("S1 ----------------------------------------------");
		//show_cap(s1);
		//show_elems(s1);
		show_cap(s2);
		show_elems(s2);
//		destroy_data(s2.begin()+1, s2.begin()+3);

		//s2.resize(8, 7);
		//s2.emplace_front(11);
		s2.pop_back();

		show_cap(s2);
		show_elems(s2);

		s2.pop_front();

		show_cap(s2);
		show_elems(s2);

//		println("s2.size = {}", s2.size());
//		println("size(s2) = {}", size(s2));
//
////		s1.swap(s2);
//		s2.swap(s1);
// 		s1.clear();
 //		s2.reserve(8);

		//println("S1 ----------------------------------------------");
		//show_cap(s1);
		//show_elems(s1);

 	//	s2.shrink_to_fit();

		//println("s2 ----------------------------------------------");
		//show_cap(s2);
		//show_elems(s2);
		//println();

		//sequence<foo, traits> s3 = move(s2);
		/*
		sequence<foo, traits> s3{9,10,11,12};
		println("s2 ----------------------------------------------");
		show_cap(s2);
		show_elems(s2);
		println("s3 ----------------------------------------------");
		show_cap(s3);
		show_elems(s3);
		println();

		s3 = move(s2);

		println("s2 ----------------------------------------------");
		show_cap(s2);
		show_elems(s2);
		println("s3 ----------------------------------------------");
		show_cap(s3);
		show_elems(s3);
		println();
		*/
		try {
//			sequence<foo, traits> s1{1,2,3,4};
		}
		catch(no_answers)
		{
			println("Bad soda");
		}
		catch(std::bad_alloc err)
		{
			println("Bad alloc: {}", err.what());
		}
//		show_cap(s1);
	}
	println("---------------------------------------------");


	//sequence<int, traits> s5{1,2,3,4,5};
	//show_cap(s5);
	//show_elems(s5);
	//s5.push_back(6);
	//s5.push_back(9);
	//s5.push_back(10);
	//show_cap(s5);
	//show_elems(s5);

	//try {
	//	s5.resize(10);
	//	s5.push_back(11);
	//}
 //   catch(const std::bad_alloc& ex)
 //   {
	//	println("{}", ex.what());
 //   }
	//show_cap(s5);
	//show_elems(s5);
	//s5.insert(s5.begin()+1, 42);
	//show_cap(s5);
	//show_elems(s5);

	//println("first = {}", *s5.data());

	//const sequence<int, traits> s6{42, 86, 666};
	//println("first = {}", *s6.data());
		/*

	sequence<int> s5{1,2,3,4,5,6,7};
	show_cap(s5);
	show_elems(s5);
	println("New cap: {}", s5.traits.grow(s5.capacity()));
	s5.push_back(8);
	show_cap(s5);
	show_elems(s5);
		*/

//	println("sizeof(impl) = {}", sizeof(fixed_sequence_storage<traits.location, foo, traits>));
//	fixed_capacity<int, 10> fc;
//	array<int, 3> a{1,2,3,4};

	//vector<int> v{1,2,3,4,5};
	//std::println("{}", v.capacity());
	//show_elems(v);
	//v.clear();
	//std::println("{}", v.capacity());
	//show_elems(v);
	//v.shrink_to_fit();
	//std::println("{}", v.capacity());
	//show_elems(v);
}

	//vector<life> v;
	//for (int i = 1; i <= 10; ++i)
	////	v.emplace(v.begin(), i);
	//	v.emplace_back(i);
	////sort(v.begin(), v.end());
	//for (auto&& e : v)
	//	std::print("{}/{}\t", e.i, e.birth);
	//println();

	//try {
	//	sequence<foo, traits> s3{1,2,3,42,5,6};
	//	println("capacity = {}", s3.capacity());
	//	println("size = {}", s3.size());
	//	show_cap(s3);
	//	show_elems(s3);
	//}
	//sequence<foo, {
	//	.storage = storage_modes::LOCAL,
	//	.location = location_modes::FRONT,
	//	.capacity = 12,
	//}> s3;
//	vector<int> s3;
//	show(s3);
//	println("sizeof(impl) = {}", sizeof(fixed_sequence_storage<traits.location, foo, traits>));

//	static_vector<foo, 10> s3;

//	s3.reserve(16);

//	for (int i = 1; i <= 8; ++i)
//	{
//		s3.push_back(i);
////		show_cap(s3);
//	}

//	for (int i = 1; i <= 8; ++i)
//		s3.emplace_front(i);
	//	s3.emplace_back(i);
//	show_elems(s3);

	//for (int i = 1; i <= 9; ++i)
	//	s3.emplace(s3.begin(), i);
	//s3.insert(s3.begin() + 2, 1234);

	//println("capacity = {}", s3.capacity());
	//println("size = {}", s3.size());
//	show_elems(s3);

	//for (int i = 0; i < 8; ++i)
	//	print("{}\t", int(s3.at(i)));
	//println();

	//try {
	//	println("{}", int(s3.at(8)));
	//}
 //   catch(const std::out_of_range& ex)
 //   {
	//	println("{}", ex.what());
 //   }

	//println("front = {}", (int) s3.front());
	//println("back = {}", (int) s3.back());
	//for (auto e = s3.rbegin(); e != s3.rend(); ++e)
	//	print("{}\t", int(*e));
//	for (auto&& e : ranges::reverse_view(s3))
//	for (auto&& e : s3 | views::reverse)
	//	print("{}\t", int(e));
	//println();

	//s3.erase(s3.begin() + 2, s3.begin() + 5);
	//s3.erase(s3.begin() + 2);
	//s3.pop_front();
	//s3.resize(12, 1234);

	//s3.emplace(s3.begin() + 6);

	//println("capacity = {}", s3.capacity());
	//println("size = {}", s3.size());
	//show_elems(s3);

	//s3.pop_back();

	//println("capacity = {}", s3.capacity());
	//println("size = {}", s3.size());
	//show_elems(s3);

//	s3.reserve(16);

	//s3.clear();

	//println("capacity = {}", s3.capacity());
	//println("size = {}", s3.size());
	//show_elems(s3);

	//s3.push_back(42);

	//println("capacity = {}", s3.capacity());
	//println("size = {}", s3.size());
	//show_elems(s3);

	//s3.shrink_to_fit();

	//println("capacity = {}", s3.capacity());
	//println("size = {}", s3.size());
	//show_elems(s3);

//	vector<int> v{1,2,3,4,5};
//	for (auto&& e : ranges::reverse_view(v))
//	for (auto&& e : v | views::reverse)
//		print("{}\t", e);

	//println("capacity = {}", v.capacity());
	//println("size = {}", v.size());
	//show_elems(v);

	//v.resize(10);
	//println("capacity = {}", v.capacity());
	//println("size = {}", v.size());
	//show_elems(v);

	//foo* p1 = static_cast<foo*>(operator new( sizeof(foo) * 5 ));
	//foo* e1 = p1 + 5;

	//for (int i = 0; i < 5; ++i)
	//	new(p1 + i) foo(i + 1);

	//for (auto i = p1; i != e1; ++i)
	//	print("{}\t", i->i);
	//println("");

	//foo* p2 = static_cast<foo*>(operator new( sizeof(foo) * 5 ));
	//foo* e2 = p2 + 5;

	//uninitialized_move( p1, e1, p2);

	//for (auto i = p1; i != e1; ++i)
	//	print("{}\t", i->i);
	//println("");

	//for (auto i = p2; i != e2; ++i)
	//	print("{}\t", i->i);
	//println("");

/*
			sequence<foo, traits> s1{9,8,7};
			show_cap(s1);
			show_elems(s1);
			sequence<foo, traits> s2{1,2,3,4,5};
			show_cap(s2);
			show_elems(s2);
			println();
			s1.swap(s2);
			show_cap(s1);
			show_elems(s1);
			show_cap(s2);
			show_elems(s2);


foo* p = static_cast<foo*>(operator new( sizeof(foo) * 5 ));
	foo* e = p + 5;

	new(p + 2) foo(1234);

	for (auto i = p; i != e; ++i)
		print("{}\t", i->i);
	println("");

	p[2].~foo();

	for (auto i = p; i != e; ++i)
		print("{}\t", i->i);
	println("");

	delete p;




	union fred {
		fred() {}
		~fred() {}
		foo f;
		char c;
	};
	{
	auto p = make_unique_for_overwrite<fred[]>(5);
	new(&p[2]) foo(1234);
	println("{}", p[2].f.i);
	p[2].f.~foo();
	}

struct immovable {
	immovable() = default;
	immovable(const immovable&) = delete;
	immovable(immovable&&) = delete;
	immovable& operator=(const immovable&) = delete;
	immovable& operator=(immovable&&) = delete;
};

struct copy_only {
	copy_only() = default;
	copy_only(const copy_only&) = default;
	copy_only(copy_only&&) = delete;
	copy_only& operator=(const copy_only&) = default;
	copy_only& operator=(copy_only&&) = delete;
};

struct unmakeable {
	unmakeable() = delete;
	char c;
};

struct base {
	void foo() { println("base::foo"); }
};
struct has_foo : public base {
	void foo() { println("has_foo::foo"); }
};
struct no_foo : public base {
};


	union storage_type {
		unsigned char unused;
		int element;
	};
	println("sizeof(st) = {}", sizeof(storage_type));

	int x = 10;
	auto p = new storage_type[x];
	auto e = p + x;
	for (auto i = p; i != e; ++i)
		print("{}\t", i->element);
	println("");
	delete[] p;



	base b;
	b.foo();
	has_foo hf;
	hf.foo();
	no_foo nf;
	nf.foo();

	immovable i1;
	immovable i2;
	vector<immovable> v;

	println("std::move_constructible<int> = {}", std::move_constructible<int>);
	println("std::move_constructible<immovable> = {}", std::move_constructible<immovable>);
	println("std::move_constructible<string> = {}", std::move_constructible<string>);
	println("std::move_constructible<copy_only> = {}", std::move_constructible<copy_only>);

	copy_only c1;
	copy_only c2;
	vector<copy_only> c;
	c2 = c1;
	//c.emplace_back();


	union fred {
		unmakeable m[5];
		unsigned char s;
	};

	std::println("Size of unmakeable:\t{}", sizeof(unmakeable));
	std::println("Size of fred:\t\t{}", sizeof(fred));
 
template<bool B>
struct thing {
};

struct base1 {
	void id() const
	{
		std::println("base1");
	}
};

struct base2 {
	void id() const
	{
		std::println("base2");
	}
};

template<>
struct thing<true> : base1 {
	void id() const
	{
		std::println("thing<true>");
		base1::id();
	}
};
template<>
struct thing<false> : base2 {
	void id() const
	{
		std::println("thing<false>");
		base2::id();
	}
};
	thing<true> tt;
	tt.id();
	thing<false> tf;
	tf.id();

	struct foo {
		char c;
	};
	struct bar {
		size_t size;
		foo data[15];
		unsigned char start;
	};
	struct pho {
		bar b1, b2;
	};

	std::println("Size of foo:\t{}", sizeof(foo));
	std::println("Size of bar:\t{}", sizeof(bar));
	std::println("Size of pho:\t{}", sizeof(pho));

	constexpr sequence_traits<unsigned char> st{
		.dynamic = false,
		.variable = false,
		.capacity = 128,
		.location = sequence_lits::MIDDLE,
	};

	println("---- default --------------------------------");
	sequence<int, st> s2;
	show(s2);
	println("--------------------------------");

	sequence<int,
		sequence_traits<unsigned short> {
			.dynamic = false,
			.variable = false,
			.capacity = 16,
			.location = sequence_lits::BACK,
		}> s3;
	show(s3);
	println("--------------------------------");

	sequence<int,
		sequence_traits<> {
			.location = sequence_lits::MIDDLE,
			.growth = sequence_lits::LINEAR,
			.increment = 256,
		}> s4;
	show(s4);
	println("--------------------------------");
*/

#endif