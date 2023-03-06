#pragma once

#include "interline_memory.h"
#include <memory_resource>

namespace bytelist
{
namespace memory
{
namespace interline
{
namespace strings
{

// solid string is allocated as a solid chunk of memory
template<
	class CharT,
	class Traits = std::char_traits<CharT>,
	class Allocator = std::pmr::polymorphic_allocator<CharT>
>
class basic_solid_string
{
public:
	using traits_type = Traits;
	
	template<class InputIt>
	basic_solid_string(InputIt first, InputIt last, const Allocator& alloc)
		: _allocator(alloc)
	{
		const std::size_t n = std::distance(first, last);
		_str_pointer = _allocator.allocate(n + 1);
		if (!_str_pointer)
			throw std::bad_alloc();
		*std::copy(first, last, _str_pointer) = CharT();
	}

	basic_solid_string(const CharT* ptr, const Allocator& alloc)
		: basic_solid_string(ptr, ptr + traits_type::length(ptr), alloc)
	{}
	
	basic_solid_string(const basic_solid_string&) = delete;

	basic_solid_string& operator=(const basic_solid_string&) = delete;
	
	operator std::size_t () const
	{
		if (!_str_pointer)
			return (std::size_t) -1;

		auto* mr = dynamic_cast<memory_resource_base*>(_allocator.resource());
		if (!mr)
			return (std::size_t) -1;

		return mr->pointer_offset(_str_pointer);
	}

protected:
	CharT* _str_pointer = nullptr;
	Allocator _allocator;
};

using solid_string = basic_solid_string<char>;

}
}
}
} // namespace strings
