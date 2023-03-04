#pragma once

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

template<class SizeT>
inline constexpr std::size_t align_size(SizeT size, std::size_t alignment)
{
	if (__builtin_expect(size == 0, 0))
		return 0;
	
	return (((std::size_t) size - 1) / alignment + 1) * alignment;
}

namespace interline
{

namespace buffer
{

/*template<class SizeT, SizeT LineSize>
struct navigator
{
	using value_type = char[LineSize];
	using size_type = SizeT;
	using difference_type = std::make_signed_t<size_type>;
	using iterator_category = std::random_access_iterator_tag;

	constexpr static value_type* no_address() { return nullptr; }
	};*/

// allign the buffer paramters
template<class SizeT, SizeT LineSize, std::size_t Alignment = alignof(std::max_align_t)>
struct aligned_memory_parameters
{
	static_assert(Alignment > 0);
	
	using size_type = SizeT;
	
	constexpr static std::size_t alignment() { return Alignment; }

 	constexpr static size_type unaligned_line_size() { return LineSize; }

	static_assert(unaligned_line_size() > 0);
	
	constexpr static size_type aligned_line_size()
	{
		return align_size(unaligned_line_size(), alignment());
	}

	static_assert(aligned_line_size() >= unaligned_line_size());
	
 	aligned_memory_parameters(void* ptr, size_type max_size) noexcept
		: _ptr(ptr), _size(max_size / alignment() * alignment())
	{
		if (max_size < 1)
			return;

		std::size_t space = max_size;
		if (std::align(alignment(), _size, _ptr, space) == nullptr)
			return;
		assert(_size <= space);

		assert(_size % alignment() == 0);
		_valid = true;
	}

	void* _ptr;
	size_type _size;
	bool _valid = false;
};

template<class SizeT, class Cell>
class sequence
{
	static_assert(sizeof(Cell) < (std::size_t) std::numeric_limits<SizeT>::max());
	
public:
	using value_type = Cell;
	using pointer = Cell*;
	using const_pointer = const Cell*;
	using iterator = pointer;
	using const_iterator = const_pointer;
	using reference = Cell&;
	using const_reference = const Cell&;
	using size_type = SizeT;

	constexpr sequence(pointer start, pointer stop) noexcept
		: _start_address(start), _stop_address(stop)
	{}
	
	iterator begin() const noexcept
	{
		return _start_address;
	}
	
	iterator end() const noexcept
	{
		return _stop_address;
	}

	const_iterator cbegin() const noexcept { return begin(); }

	const_iterator cend() const noexcept { return end(); }

	size_type size() const
	{
		return end() - begin();
	}
	
	bool empty() const noexcept { return cbegin() == cend(); }

protected:
	pointer _start_address = nullptr;
	pointer _stop_address = nullptr;
};

template<class SizeT, SizeT LineSize, std::size_t Alignment = alignof(std::max_align_t)>
class type 
	: protected sequence<
	    SizeT,
			char[aligned_memory_parameters<SizeT, LineSize, Alignment>::aligned_line_size()]
	  >
{
public:
	using size_type = SizeT;
	
	using constructor_pars = aligned_memory_parameters<size_type, LineSize, Alignment>;
	
	constexpr static std::size_t alignment()
	{
		return constructor_pars::alignment();
	}

 	constexpr static size_type unaligned_line_size()
	{
		return constructor_pars::unaligned_line_size();
	}

	constexpr static size_type aligned_line_size()
	{
		return constructor_pars::aligned_line_size();
	}

	static_assert(aligned_line_size() >= unaligned_line_size());

	constexpr static size_type line_gap_size()
	{
		return aligned_line_size() - unaligned_line_size();
	}
	
	using base = sequence<size_type, char[aligned_line_size()]>;
	static_assert(std::is_unsigned<size_type>::value);

	using typename base::value_type;
	
	// maps free space to a line index
	using map_type = std::multimap<size_type, size_type>;
	
	type(const constructor_pars& pars)
		: base((typename base::pointer) pars._ptr,
					 (typename base::pointer) ((char*) pars._ptr + pars._size)
					 ),
		_end_idx(pars._size / aligned_line_size())
	{
		assert(pars._size % alignment() == 0);
		if (!pars._valid)
			throw std::bad_alloc();
	}
	
	type(const type&) = delete;
	virtual ~type() {}

	type& operator=(const type&) = delete;

	void swap(type&) = delete;

	size_type max_size() const noexcept { return _end_idx; }
	
	size_type n_lines_total() const noexcept { return _cur_idx; }
	
	bool emplace_back() noexcept;

	bool is_pointer_inside(const void* p) const noexcept
	{
		return p >= &*this->begin() && p < &*this->end();
	}

	// creates a "formatted" line
	void* allocate_line(size_type n_lines) noexcept
	{
		try
		{
			if (__builtin_expect(n_lines == 0, 0))
				return nullptr;

			void* p = &*(this->begin() + _cur_idx);
			
			if (__builtin_expect(!append_lines(n_lines), 0))
				return nullptr;

			constexpr size_type free_bytes = line_gap_size();
			if (free_bytes > 0) {
				size_type i = _cur_idx; 
				do {
					--i;
					_map.emplace(free_bytes, i);
				} while (i != _cur_idx - n_lines);
			}
			assert(is_pointer_inside(p));
			return p;
		}
		catch (const std::bad_alloc&)
		{
			return nullptr;
		}
	}
	
	// bytestream is allocated in unused space or by addition of "unformatted" lines
	void* allocate_bytestream(size_type bytes, std::size_t al) noexcept
	{
		static_assert(aligned_line_size() % alignment() == 0);
		if (__builtin_expect(!(al > 0 && alignment() % al == 0), 0))
			return nullptr; // unable to align
		
		void* res = nullptr;

		assert(aligned_line_size() > 0);

		if (__builtin_expect(bytes == 0, 0))
			return this->begin(); // NB returns the pointer to the beginning of the buffer (doesn't mater)

		const size_type aligned_bytes = (size_type) align_size(bytes, al);
		if (__builtin_expect(aligned_bytes < bytes, 0))
			return nullptr;

		try {
			if (bytes < aligned_line_size()) {
				auto it = _map.lower_bound(aligned_bytes);
				
				if (it != _map.end()) // found a line with free space
				{	
					// how much space there is in the line
					std::size_t free_bytes = it->first;
					assert(free_bytes < aligned_line_size());
					const auto idx = it->second;
					res = (char*) this->begin()[idx + 1] - free_bytes;
					void* res_p = std::align(al, bytes, res, free_bytes);
					assert(res_p);
					assert((char*)res + bytes <= (char*) this->begin()[_end_idx]);
					assert(free_bytes >= bytes);
					free_bytes -= aligned_bytes;
					_map.erase(it);
					if (free_bytes > 0)
						_map.emplace(free_bytes, idx);
				}
				else
				{
					// add a new line
					if (!append_lines(1))
						return nullptr; // the buffer is full

					res = (char*) this->begin()[_cur_idx - 1];
					_map.emplace(aligned_line_size() - bytes, _cur_idx - 1);
				}
			}
			else {
				// add new lines
				const auto n_lines = (bytes - 1) / aligned_line_size() + 1;
				const auto last_line_free = (aligned_line_size() - bytes % aligned_line_size())
					% aligned_line_size();
				assert(bytes <= n_lines * aligned_line_size());
				assert(last_line_free < aligned_line_size());

				if (!append_lines(n_lines))
					return nullptr; // no space

				res = (char*) this->begin()[_cur_idx - n_lines];
				if (last_line_free > 0)
					_map.emplace(last_line_free, _cur_idx - 1);
			}
			assert(is_pointer_inside(res));
			return (void*) res;
		}
		catch(const std::bad_alloc&) {
			return nullptr;
		}
	}

	template<class Fun>
	void holes_map_traversal(Fun&& observer) const
	{
		for (const auto& p : _map)
			observer(p.first);
	}

	// shows the buffer structure
	// slow, this function is for debug purposes only
	template<class Fun>
	void line_by_line_traversal(Fun&& observer) const
	{
		const auto n_lines = _cur_idx; // both formatted and unformatted
		std::vector<size_type> v(n_lines, aligned_line_size());
		for (const auto& p : _map) 
			v.at(p.second) -= p.first;
		
		for (const auto& filled : v)
			observer(filled);
	}

 protected:
	const size_type _end_idx;
	size_type _cur_idx = 0;

	bool append_lines(size_type n_lines) noexcept
	{
		if (__builtin_expect(n_lines > _end_idx || _end_idx - n_lines < _cur_idx, 0))
			return false;

		_cur_idx += n_lines;
		return true;
	}
	
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
	
	bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

	buffer_type _buffer;
};

} // namespace interline
} // namespace memory
} // namespace bytelist
