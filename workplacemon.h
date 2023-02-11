#pragma once

#include "bytelist.h"

namespace wpm
{

struct size : marker::type<size_in_bytes_marker, uint64_t> {};
struct date : marker::type<times::date_marker<BASE>, uint16_t> {};
#if 0
struct path : marker::type<filesystem::dir_path_marker<true>, std::shared_ptr<string>> {};
#else
struct path : marker::type<filesystem::dir_path_marker<true>, bytelist::string> {};
#endif

struct line : std::tuple<
	size,
  date,
	path
	//	hash
>
{
};

} // namespace wpm
