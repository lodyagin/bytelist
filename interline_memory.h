#pragma once

#include "types/sequence.h"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <map>
#include <memory>
#include <memory_resource>
#include <type_traits>
#include <tuple>

namespace bytelist
{
namespace memory
{
namespace interline
{

namespace buffer
{

template<class SizeT, SizeT LineSize>
struct navigator
{
	using value_type = char[LineSize];
	using size_type = SizeT;
	using difference_type = std::make_signed_t<size_type>;
	using iterator_category = std::random_access_iterator_tag;
};

// allign the buffer paramters
template<class SizeT, class AlignT = std::max_align_t>
struct aligned_memory_parameters
{
	using size_type = SizeT;
	
	constexpr static std::size_t alignment() { return alignof(AlignT); }

 	aligned_memory_parameters(void* ptr, size_type max_size) noexcept
		: _ptr(ptr)
	{
		if (max_size < 1)
			return;
		
		std::size_t aligned_size = max_size; //((max_size - 1) / alignment() + 1) * alignment();
		if (std::align(alignment(),
									 max_size / alignment() * alignment(),
									 _ptr,
									 aligned_size) == nullptr
				|| aligned_size == 0)
			return;

		aligned_size = aligned_size / alignment() * alignment();
		
		assert(aligned_size <= max_size);
		_size = (size_type) aligned_size;
		assert(_size % alignment() == 0);
		_valid = true;
	}

	void* _ptr;
	size_type _size = 0;
	bool _valid = false;
};

template<class SizeT, SizeT LineSize>
class type : public types::sequence::type<navigator<SizeT, LineSize>>
{
public:
	using size_type = SizeT;
	
	using constructor_pars = aligned_memory_parameters<size_type>;
	
	constexpr static std::size_t alignment() { return constructor_pars::alignment(); }

 	constexpr static size_type unaligned_line_size() { return LineSize; }

	static_assert(unaligned_line_size() > 0);
	
	constexpr static size_type aligned_line_size()
	{
		return ((unaligned_line_size() - 1) / alignment() + 1) * alignment();
	}

	static_assert(aligned_line_size() >= unaligned_line_size());
	
	using base = types::sequence::type<navigator<size_type, aligned_line_size()>>;
	static_assert(std::is_unsigned<size_type>::value);
	// maps free space to a line index
	using map_type = std::multimap<size_type, size_type>;
	using line_type = char[aligned_line_size()];
	using value_type = line_type;
	
	type(const constructor_pars& pars)
		: base((typename base::pointer) pars._ptr,
					 (typename base::pointer) ((char*) pars._ptr + pars._size)
					 ),
		_end_idx(pars._size / alignment())
	{
		assert(pars._size % alignment() == 0);
		if (!pars._valid)
			throw std::bad_alloc();
	}
	
	type(const type&) = delete;
	virtual ~type() {}

	type& operator=(const type&) = delete;

	void swap(type&) = delete;

	size_type n_lines() const { return _end_idx; }
	
	bool emplace_back() noexcept;

	void* allocate_line(size_type n_lines) noexcept
	{
		try
		{
			if (n_lines == 0)
				return nullptr;

			if (!append_lines(n_lines))
				return nullptr;

			constexpr size_type free_bytes = aligned_line_size() - unaligned_line_size();
			if (free_bytes > 0)
				_map.emplace(free_bytes, _cur_idx - 1);
		}
		catch (const std::bad_alloc&)
		{
			return nullptr;
		}
	}
	
	// bytestream is allocated in unused space
	void* allocate_bytestream(size_type bytes) noexcept
	{
		char* res = nullptr;

		assert(aligned_line_size() > 0);

		if (bytes == 0)
			return ptr(); // NB returns the pointer to the beginning of the buffer (doesn't mater)
		
		assert(bytes <= std::numeric_limits<size_type>::max()); // TODO

		try {
			if (bytes < aligned_line_size()) {
				auto it = _map.lower_bound(bytes);
				
				if (it != _map.end()) // found a line with free space
				{	
					// how much space there is in the line
					size_type free_bytes = it->first;
					assert(free_bytes < aligned_line_size());
					const auto idx = it->second;
					res = (char*) &ptr()[idx + 1] - free_bytes;
					assert(res + bytes <= (char*) &ptr()[_end_idx]);
					assert(free_bytes >= bytes);
					free_bytes -= bytes;
					_map.erase(it);
					if (free_bytes > 0)
						_map.emplace(free_bytes, idx);
				}
				else
				{
					// add a new line
					if (!append_lines(1))
						return nullptr; // the buffer is full

					res = (char*) ptr()[_cur_idx - 1];
					_map.emplace(aligned_line_size() - bytes, _cur_idx - 1);
				}
			}
			else {
				// add new lines
				const auto n_lines = (bytes - 1) / aligned_line_size() + 1;
				const auto last_line_free = aligned_line_size() - bytes % aligned_line_size();
				assert(bytes <= n_lines * aligned_line_size());
				assert(last_line_free > 0);
				assert(last_line_free < aligned_line_size());

				if (!append_lines(n_lines))
					return nullptr; // no space

				res = (char*) &ptr()[_cur_idx - n_lines];
				_map.emplace(last_line_free, _cur_idx - 1);
			}
			return (void*) res;
		}
		catch(const std::bad_alloc&) {
			return nullptr;
		}
	}

protected:
	//const line_type* _ptr;
	const size_type _end_idx;
	size_type _cur_idx = 0;

	bool append_lines(size_type n_lines) noexcept
	{
		if (n_lines > _end_idx || _end_idx - n_lines < _cur_idx)
			return false;

		_cur_idx += n_lines;
		return true;
	}
	
	line_type* ptr() const { return (line_type*) this->_start_address; }
	
	map_type _map;
};

} // namespace buffer

class memory_resource_base : public std::pmr::memory_resource
{
};

template<class SizeT, SizeT LineSize>
class memory_resource : public memory_resource_base
{
public:
	using buffer_type = buffer::type<SizeT, LineSize>;
	using size_type = SizeT;
	
	memory_resource(/*buffer_type* buf*/);
		
	// Release all allocated memory
	void release();

protected:
	void* do_allocate(std::size_t bytes, std::size_t alignment) override
	{
		// TODO
		return nullptr;
	}
	
	// No-op
	void do_deallocate(void*, std::size_t, std::size_t) override {}
	
	bool do_is_equal(const std::pmr::memory_resource& other) const override;

	buffer_type _buffer;
};

} // namespace interline
} // namespace memory
} // namespace bytelist
