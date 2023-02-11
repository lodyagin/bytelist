#include "interline_memory.h"

//#include <strings.h>
#include <gtest/gtest.h>
#include <iostream>

TEST(buffer, allocate0)
{
	using namespace bytelist::memory::interline;

	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t> pars(ptr, size);
		buffer::type<uint16_t, 80> buf(pars);

		EXPECT_NE(nullptr, buf.allocate_bytestream(0));
		std::cout << buf.n_lines() << std::endl;
	}
	::free(ptr);
}
