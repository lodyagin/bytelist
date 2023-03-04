#include "interline_memory.h"

#include <gtest/gtest.h>
#include <iostream>

using namespace bytelist::memory::interline;

template<class Buffer>
void show_all_lines(const Buffer& buf)
{
		int cnt = 0;
		buf.line_by_line_traversal(
			[&cnt](uint16_t filled)
			{
				std::cout << cnt++ << ": " << filled << '\n';
			}
		);
		std::cout << std::endl;
}

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

TEST(buffer, line_allocate0)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 80, 16> pars(ptr, size);
		buffer::type<uint16_t, 80, 16> buf(pars);

		EXPECT_EQ(nullptr, buf.allocate_line(0));
	}
	::free(ptr);
}

TEST(buffer, line_allocate_over_max1)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 80, 16> pars(ptr, size);
		buffer::type<uint16_t, 80, 16> buf(pars);

		EXPECT_EQ(819, buf.max_size());
		EXPECT_EQ(0, buf.n_lines_total());

		EXPECT_EQ(nullptr, buf.allocate_line(820));

		EXPECT_EQ(0, buf.n_lines_total());
	}
	::free(ptr);
}

TEST(buffer, line_allocate_over_max2)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 80, 16> pars(ptr, size);
		buffer::type<uint16_t, 80, 16> buf(pars);

		EXPECT_NE(nullptr, buf.allocate_line(419));
		EXPECT_EQ(419, buf.n_lines_total());
		
		EXPECT_NE(nullptr, buf.allocate_line(400));
		EXPECT_EQ(819, buf.n_lines_total());
		
		EXPECT_EQ(nullptr, buf.allocate_line(1));
		EXPECT_EQ(819, buf.n_lines_total());
	}
	::free(ptr);
}

TEST(buffer, line_allocate_zero_space)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 80, 16> pars(ptr, size);
		buffer::type<uint16_t, 80, 16> buf(pars);

		EXPECT_EQ(ptr, buf.allocate_line(819));
		EXPECT_EQ(819, buf.n_lines_total());

		buf.holes_map_traversal(
			[](uint16_t hole_size)
			{
				FAIL(); // should be no holes
			}
		);
	}
	::free(ptr);
}

TEST(buffer, line_allocate_some_space)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 102, 16> pars(ptr, size);
		buffer::type<uint16_t, 102, 16> buf(pars);

		EXPECT_EQ(112, buf.aligned_line_size());
		
		EXPECT_EQ(ptr, buf.allocate_line(5));
		EXPECT_EQ(5, buf.n_lines_total());

		EXPECT_EQ((char*)ptr + 112 * 5, buf.allocate_line(10));
		EXPECT_EQ(15, buf.n_lines_total());

		int cnt = 0;
		buf.holes_map_traversal(
			[&cnt](uint16_t hole_size)
			{
				EXPECT_EQ(10, hole_size);
				cnt++;
			}
		);
		EXPECT_EQ(15, cnt); // 15 x 6 bytes
	}
	::free(ptr);
}

TEST(buffer, stream_allocate0)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 80, 16> pars(ptr, size);
		buffer::type<uint16_t, 80, 16> buf(pars);

		EXPECT_NE(nullptr, buf.allocate_bytestream(0, 1));
		EXPECT_EQ(65535 / 80, buf.max_size());
	}
	::free(ptr);
}

TEST(buffer, stream_allocate_inline)
{
	constexpr uint16_t size = 7 * 64 + 31;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 6, 64> pars(ptr, size);
		buffer::type<uint16_t, 6, 64> buf(pars);

		buf.allocate_line(6);
		EXPECT_EQ(6, buf.n_lines_total());
		
		buf.allocate_bytestream(58, 1);
		buf.allocate_bytestream(57, 1);
		buf.allocate_bytestream(32, 1);
		buf.allocate_bytestream(1, 1);

		/*
			0: 6
			1: 6
			2: 6
			3: 38  (6 + 32)
			4: 64  (6 + 58)
			5: 64  (6 + 57 + 1)
		*/		
		int cnt = 0;
		buf.holes_map_traversal(
			[&cnt](uint16_t hole_size)
			{
				cnt++;
				switch (cnt) {
				case 1:
					EXPECT_EQ(26, hole_size);
					break;
				case 2:
				case 3:
				case 4:
					EXPECT_EQ(58, hole_size);
					break;
				}
			}
		);
		EXPECT_EQ(4, cnt);

		buf.allocate_bytestream(27, 1);
		cnt = 0;
		buf.holes_map_traversal(
			[&cnt](uint16_t hole_size)
			{
				cnt++;
				switch (cnt) {
				case 1:
					EXPECT_EQ(26, hole_size);
					break;
				case 2:
					EXPECT_EQ(31, hole_size);
					break;
				case 3:
				case 4:
					EXPECT_EQ(58, hole_size);
					break;
				}
			}
		);
		EXPECT_EQ(4, cnt);
		
		buf.allocate_bytestream(58, 1); //removes 58 hole
		buf.allocate_bytestream(32, 1); //58 -> 26 hole 
		EXPECT_EQ(6, buf.n_lines_total());
		EXPECT_NE(nullptr, buf.allocate_bytestream(32, 1)); // + 32 hole

		EXPECT_EQ(nullptr, buf.allocate_bytestream(33, 1));
		EXPECT_EQ(7, buf.n_lines_total());

		cnt = 0;
		buf.holes_map_traversal(
			[&cnt](uint16_t hole_size)
			{
				cnt++;
				switch (cnt) {
				case 1:
				case 2:
					EXPECT_EQ(26, hole_size);
					break;
				case 3:
					EXPECT_EQ(31, hole_size);
					break;
				case 4:
					EXPECT_EQ(32, hole_size);
					break;
				}
			}
		);
		EXPECT_EQ(4, cnt);
	}
	::free(ptr);
}

TEST(buffer, stream_allocate_mixed_2lines)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 50, 64> pars(ptr, size);
		buffer::type<uint16_t, 50, 64> buf(pars);

		buf.allocate_line(2);

		// 50 full | 14 free 
		// 50 full | 14 free 
		
		EXPECT_NE(nullptr, buf.allocate_bytestream(10, 1));

		// 50 full | 14 free 
		// 60 full | 4 free 
		
		EXPECT_NE(nullptr, buf.allocate_bytestream(15, 1));
		
		// 50 full | 14 free 
		// 60 full | 4 free 
		// 15 full | 49 free

		int cnt = 0;
		buf.holes_map_traversal(
			[&cnt](uint16_t hole_size)
			{
				cnt++;
				switch (cnt) {
				case 1:
					EXPECT_EQ(4, hole_size);
					break;
				case 2:
					EXPECT_EQ(14, hole_size);
					break;
				case 3:
					EXPECT_EQ(49, hole_size);
					break;
				}
			}
		);
		EXPECT_EQ(3, cnt);
	}
	::free(ptr);
}

TEST(buffer, stream_allocate_mixed)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 50, 64> pars(ptr, size);
		buffer::type<uint16_t, 50, 64> buf(pars);

		buf.allocate_line(2);

		// 50 full | 14 free 
		// 50 full | 14 free 
		
		EXPECT_NE(nullptr, buf.allocate_bytestream(10, 1));

		// 50 full | 14 free 
		// 60 full | 4 free 
		
		EXPECT_NE(nullptr, buf.allocate_bytestream(3 * 64 + 15, 1));
		
		// 50 full | 14 free 
		// 60 full
		// 64 full
		// 64 full
		// 64 full
		// 15 full | 49 free

		EXPECT_EQ(6, buf.n_lines_total());

		EXPECT_EQ((char*)ptr + 6 * 64, buf.allocate_line(1));

		EXPECT_EQ(7, buf.n_lines_total());
		
		// 50 full | 14 free 
		// 60 full | 4 free
		// 64 full
		// 64 full
		// 64 full
		// 15 full | 49 free
		// 50 full | 14 free 

		int cnt = 0;
		buf.holes_map_traversal(
			[&cnt](uint16_t hole_size)
			{
				cnt++;
				switch (cnt) {
				case 1:
					EXPECT_EQ(4, hole_size);
					break;
				case 2:
				case 3:
					EXPECT_EQ(14, hole_size);
					break;
				case 4:
					EXPECT_EQ(49, hole_size);
					break;
				}
			}
		);
		EXPECT_EQ(4, cnt);
	}
	::free(ptr);
}

TEST(buffer, stream_allocate_interline)
{
	constexpr uint16_t size = 65535;
	void* ptr = ::malloc(size);
	{
		buffer::aligned_memory_parameters<uint16_t, 50, 64> pars(ptr, size);
		buffer::type<uint16_t, 50, 64> buf(pars);

		buf.allocate_line(2);

		// 50 full | 14 free 
		// 50 full | 14 free 
		
		EXPECT_NE(nullptr, buf.allocate_bytestream(14, 1));

		// 50 full | 14 free 
		// 64 full 
		
		EXPECT_NE(nullptr, buf.allocate_bytestream(128, 1));
		
		// 50 full | 14 free 
		// 64 full
		// 64 full
		// 64 full

		EXPECT_EQ(4, buf.n_lines_total());

		EXPECT_EQ((char*)ptr + 4 * 64, buf.allocate_line(1));

		int cnt = 0;
		buf.holes_map_traversal(
			[&cnt](uint16_t hole_size)
			{
				cnt++;
				switch (cnt) {
				case 1:
				case 2:
					EXPECT_EQ(14, hole_size);
					break;
				}
			}
		);
		EXPECT_EQ(2, cnt);
	}
	::free(ptr);
}

