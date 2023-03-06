#include "strings/solid.h"

#include <gtest/gtest.h>
#include <iostream>

using namespace bytelist::memory::interline;
using namespace bytelist::memory::interline::strings;

TEST(solid, two_strings)
{
	char block[1024 + 16];

	memory_resource<uint64_t, 4, 16> mem_res(block, 1024);

	std::size_t offs_a;
	std::size_t offs_b;
	
	{
		solid_string a("Hello , ", &mem_res);
		solid_string b("World!", &mem_res);

		offs_a = (std::size_t) a;
		offs_b = (std::size_t) b;
	}

	EXPECT_EQ(0, offs_a);
	EXPECT_EQ(9, offs_b);

	EXPECT_STREQ("Hello , ", mem_res.mem_start() + offs_a);
	EXPECT_STREQ("World!", mem_res.mem_start() + offs_b);
}
