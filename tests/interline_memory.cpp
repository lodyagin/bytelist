#include "interline_memory.h"

//#include <strings.h>
#include <gtest/gtest.h>
#include <iostream>

using namespace bytelist::memory::interline;

TEST(aligned_memory_parameters, test1)
{
	using pars_t = buffer::aligned_memory_parameters<uint16_t, 16>;

	const pars_t pars((char*)80, 0);

	EXPECT_FALSE(pars._valid);
}

TEST(aligned_memory_parameters, test2)
{
	using pars_t = buffer::aligned_memory_parameters<uint16_t, 16>;

	const pars_t pars((char*)5, 26);  // [ 26 - (16 - 5) < 16 ]

	EXPECT_FALSE(pars._valid);
}

TEST(aligned_memory_parameters, test3)
{
	using pars_t = buffer::aligned_memory_parameters<uint16_t, 16>;

	const pars_t pars((char*)5, 27); // [ 27 - (16 - 5) == 16 ]

	EXPECT_TRUE(pars._valid);
	EXPECT_EQ((void*)16, pars._ptr);
	EXPECT_EQ(16, pars._size);
}

TEST(buffer, allocate0)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 16> pars(ptr, size);
		buffer::type<uint16_t, 80> buf(pars);

		EXPECT_NE(nullptr, buf.allocate_bytestream(0));
		EXPECT_EQ(65535 / 80, buf.size());
	}
	::free(ptr);
}
