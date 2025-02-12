// Lifetime metered object for testing containers and such. This monitors all
// contruction, destruction and assignment, and it keeps track of lifetimes with
// a static counter.

export module life;

import <string>;
import <print>;
import <list>;

export enum value_tags {
	DEFAULTED = -1,
	DESTRUCTED = -2,
	MOVED_FROM = -3,
};

export enum event_tags : unsigned {
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

export template<bool NOEXCEPT_MOVE = true>
class life
{
public:

	life()
	{
		log.emplace_back(id, DEFAULT_CONSTRUCT, value);
	}
	life(int value) : value(value)
	{
		log.emplace_back(id, VALUE_CONSTRUCT, value);
	}

	life(const life& rhs) : value(rhs.value)
	{
		log.emplace_back(id, COPY_CONSTRUCT, value);
	}
	life(life&& rhs) noexcept(NOEXCEPT_MOVE) : value(rhs.value)
	{
		rhs.value = value_tags::MOVED_FROM;
		log.emplace_back(id, MOVE_CONSTRUCT, value);
	}

	life& operator=(int v)
	{
		value = v;
		log.emplace_back(id, VALUE_ASSIGN, value);
		return *this;
	}
	life& operator=(const life& rhs)
	{
		value = rhs.value;
		log.emplace_back(id, COPY_ASSIGN, value);
		return *this;
	}
	life& operator=(life&& rhs)
	{
		value = rhs.value;
		rhs.value = value_tags::MOVED_FROM;
		log.emplace_back(id, MOVE_ASSIGN, value);
		return *this;
	}

	~life()
	{
		log.emplace_back(id, DESTRUCT, value);
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

		std::string comment;
		unsigned id = 0;
		unsigned operation;
		signed value = 0;
	};

	using log_type = std::list<record>;
	static void clear_log() { log.clear(); }
	static const log_type& get_log() { return log; }

	template<typename STR>
	static void add_comment(STR&& comment)
	{
		log.emplace_back(std::forward<STR>(comment));
	}

	static void print_log()
	{
		for (const auto& record : log)
		{
			if (record.operation == COMMENT)
				std::println("{}", record.comment);
			else
			{
				std::print("{: >4d}", record.id);
				print_operation(record.operation);
				print_value(record.value);
				std::println();
			}
		}
	}
	static void print_operation(unsigned operation)
	{
		const char* operations[] = {
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
				std::print("{: >4}", "DEF");		break;
		case DESTRUCTED:
				std::print("{: >4}", "DST");		break;
		case MOVED_FROM:
				std::print("{: >4}", "MOV");		break;
		}
	}

private:

	static unsigned previous_id;
	static std::list<record> log;
};

template<bool NOEXCEPT_MOVE = true>
unsigned life<NOEXCEPT_MOVE>::previous_id = 0;
template<bool NOEXCEPT_MOVE = true>
life<NOEXCEPT_MOVE>::log_type life<NOEXCEPT_MOVE>::log;
