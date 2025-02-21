// life and life_throws - Lifetime metered objects for testing containers and such.
// They monitor all contruction, destruction and assignment, and keep track of
// lifetimes with a static counter. The life_throws version has a noexcept(false)
// move constructor. If the static counter reaches throw_at, the move constructor
// will throw an ident. Set throw_at to zero (default) to prevent throwing. The
// noexcept(true) version will never throw.

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

export class life_impl
{
public:

	life_impl()
	{
		add_record_throw(DEFAULT_CONSTRUCT);
	}
	life_impl(int value) : value(value)
	{
		add_record_throw(VALUE_CONSTRUCT);
	}

	life_impl(const life_impl& rhs) : value(rhs.value)
	{
		add_record_throw(COPY_CONSTRUCT);
	}
	life_impl(life_impl&& rhs) : value(rhs.value)
	{
		rhs.value = value_tags::MOVED_FROM;
	}

	life_impl& operator=(int v)
	{
		value = v;
		add_record_throw(VALUE_ASSIGN);
		return *this;
	}
	life_impl& operator=(const life_impl& rhs)
	{
		value = rhs.value;
		add_record_throw(COPY_ASSIGN);
		return *this;
	}
	life_impl& operator=(life_impl&& rhs)
	{
		value = rhs.value;
		rhs.value = value_tags::MOVED_FROM;
		add_record_throw(MOVE_ASSIGN);
		return *this;
	}

	~life_impl()
	{
		add_record_throw(DESTRUCT);
		value = value_tags::DESTRUCTED;
	}

	operator signed() const { return value; }

	signed value = value_tags::DEFAULTED;
	const unsigned id = ++previous_id;

	struct ident
	{
		unsigned id = 0;
		unsigned operation = COMMENT;
		signed value = 0;
	};

	struct record : public ident
	{
		template<typename STR>
			requires requires (STR&& str) { std::string(std::forward<STR>(str)); }
		record(STR&& comment) : comment(std::forward<STR>(comment)) {}
		record(ident i) : ident(i) {}

		inline bool operator==(const record& rhs) const
		{
			return	id == rhs.id &&
					operation == rhs.operation &&
					value == rhs.value;
		}
		inline bool operator!=(const record& rhs) const { return !operator==(rhs); }

		std::string comment;
	};

	using log_type = std::list<record>;
	static void reset() { previous_id = 0; throw_at = 0; clear_log(); }
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
			"D~",		// DESTRUCT
			"Cm",		// COMMENT
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

	static bool check_log(const ident* next, const ident* end)
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
	static bool check_log(const ident (&records)[SIZE])
	{
		return check_log(std::begin(records), std::end(records));
	}

	static unsigned throw_at;

protected:

	static void add_record(record&& rec)
	{
		if (log.empty()) last = log.end();
		log.emplace_back(std::move(rec));
		if (last == log.end()) --last;
	}

	void add_record_throw(event_tags event)
	{
		ident i(id, event, value);
		if (event != DESTRUCT && id == throw_at)
		{
			throw_at = 0;	// Ensure we don't throw again.
			throw i;
		}
		add_record(i);
	}
	void add_record(event_tags event) { add_record(ident{id, event, value}); }

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

unsigned life_impl::previous_id = 0;
unsigned life_impl::throw_at = 0;
life_impl::log_type life_impl::log;
life_impl::log_type::const_iterator life_impl::last;

// This derived class provides static dispatch on whether the move constructor is noexcept.

template<bool NOEX>
class life_noex : public life_impl
{
public:

	life_noex() = default;
	life_noex(int value) : life_impl(value) {}

	life_noex(const life_noex& rhs) = default;
	life_noex(life_noex&& rhs) noexcept(NOEX) : life_impl(std::move(rhs))
	{
		if constexpr (NOEX)
			add_record(MOVE_CONSTRUCT);
		else
			add_record_throw(VALUE_CONSTRUCT);
	}

	life_noex& operator=(const life_noex&) = default;
	life_noex& operator=(life_noex&&) = default;
};

// The public names for the noexcept options.

export using life = life_noex<true>;
export using life_throws = life_noex<false>;
