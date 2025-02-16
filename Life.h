// life and life_throws - Lifetime metered objects for testing containers and such.
// They monitors all contruction, destruction and assignment, and keep track of
// lifetimes with a static counter. The life_throws version has a noexcept(false)
// move constructor.

#include <string>
#include <print>
#include <list>

enum value_tags {
	DEFAULTED = -1,
	DESTRUCTED = -2,
	MOVED_FROM = -3,
};

enum event_tags : unsigned {
	DEFAULT_CONSTRUCT,
	VALUE_CONSTRUCT,
	COPY_CONSTRUCT,
	MOVE_CONSTRUCT,
	VALUE_ASSIGN,
	COPY_ASSIGN,
	MOVE_ASSIGN,
	DESTRUCT,
	COMMENT
};

class life
{
public:

	life()
	{
		add_record(id, DEFAULT_CONSTRUCT, value);
	}
	life(int value) : value(value)
	{
		add_record(id, VALUE_CONSTRUCT, value);
	}

	life(const life& rhs) : value(rhs.value)
	{
		add_record(id, COPY_CONSTRUCT, value);
	}
	life(life&& rhs) noexcept : value(rhs.value)
	{
		rhs.value = value_tags::MOVED_FROM;
		add_record(id, MOVE_CONSTRUCT, value);
	}

	life& operator=(int v)
	{
		value = v;
		add_record(id, VALUE_ASSIGN, value);
		return *this;
	}
	life& operator=(const life& rhs)
	{
		value = rhs.value;
		add_record(id, COPY_ASSIGN, value);
		return *this;
	}
	life& operator=(life&& rhs)
	{
		value = rhs.value;
		rhs.value = value_tags::MOVED_FROM;
		add_record(id, MOVE_ASSIGN, value);
		return *this;
	}

	~life()
	{
		add_record(id, DESTRUCT, value);
		value = value_tags::DESTRUCTED;
	}

	operator signed() const { return value; }

	signed value = value_tags::DEFAULTED;
	const unsigned id = ++previous_id;

	struct record
	{
		template<typename STR>
			requires requires (STR&& str) { std::string(std::forward<STR>(str)); }
		record(STR&& comment) : operation(COMMENT), comment(std::forward<STR>(comment)) {}
		record(int id, int operation, int value) : id(id), operation(operation), value(value) {}

		inline bool operator==(const record& rhs) const
		{
			return	id == rhs.id &&
					operation == rhs.operation &&
					value == rhs.value;
		}
		inline bool operator!=(const record& rhs) const { return !operator==(rhs); }

		unsigned id = 0;
		unsigned operation;
		signed value = 0;
		std::string comment;
	};

	using log_type = std::list<record>;
	static void reset() { previous_id = 0; clear_log(); }
	static void clear_log() { log.clear(); }
	static const log_type& get_log() { return log; }

	template<typename STR>
	static void add_comment(STR&& comment)
	{
		add_record(std::forward<STR>(comment));
	}

	static void print_log() { print_log_range(log.begin()); }
	static void print_new_log() { print_log_range(last); }

	static void print_operation(unsigned operation)
	{
		const char* operations[COMMENT + 1] = {
			"DC",		// DEFAULT_CONSTRUCT
			"VC",		// VALUE_CONSTRUCT
			"CC",		// COPY_CONSTRUCT
			"MC",		// MOVE_CONSTRUCT
			"VA",		// VALUE_ASSIGN
			"CA",		// COPY_ASSIGN
			"MA",		// MOVE_ASSIGN
			"DE",		// DESTRUCT
			"CM",		// COMMENT
		};

		std::print("{: >4}", operations[std::min<unsigned>(operation, COMMENT)]);
	}
	static void print_value(signed value)
	{
		switch (value)
		{
		default:
			std::print("{: >4d}", value);	break;
		case DEFAULTED:
			std::print("{: >4}", "DEF");	break;
		case DESTRUCTED:
			std::print("{: >4}", "DST");	break;
		case MOVED_FROM:
			std::print("{: >4}", "MOV");	break;
		}
	}

	static bool check_log(const record* next, const record* end)
	{
		for (; last != log.end() && next != end; ++last)
		{
			if (last->operation == COMMENT) continue;
			if (*last != *next) return false;
			++next;
		}
		return true;
	}
	template<int SIZE>
	static bool check_log(const record (&records)[SIZE])
	{
		return check_log(std::begin(records), std::end(records));
	}

private:

	template<typename... ARGS>
	static void add_record(ARGS... args)
	{
		if (log.empty()) last = log.end();
		log.emplace_back(args...);
		if (last == log.end()) --last;
	}

	static void print_log_range(log_type::const_iterator rec)
	{
		for (; rec != log.end(); ++rec)
		{
			if (rec->operation == COMMENT)
				std::println("{}", rec->comment);
			else
			{
				std::print("{: >4d}", rec->id);
				print_operation(rec->operation);
				print_value(rec->value);
				std::println();
			}
		}
	}

	static unsigned previous_id;
	static log_type log;
	static log_type::const_iterator last;
};


class life_throws : public life
{
public:

	life_throws() = default;
	life_throws(int value) : life(value) {}

	life_throws(const life_throws& rhs) = default;
	life_throws(life_throws&& rhs) noexcept(false) = default;
};
