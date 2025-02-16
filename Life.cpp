// life and life_throws - Lifetime metered objects for testing containers and such.

#include "Life.h"

unsigned life::previous_id = 0;
life::log_type life::log;
life::log_type::const_iterator life::last;
