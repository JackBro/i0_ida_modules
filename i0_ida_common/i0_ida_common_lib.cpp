#include "i0_ida_common_lib.h"

static const uint8 i0_func_entry_bytes[] = {
	0x04, 0x22, 0x25, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF,
	0x08, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x41, 0x08, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00
}; static_assert(sizeof(i0_func_entry_bytes) == 0x33, "check i0_func_entry");

bool i0_check_byte_seq(ea_t addr, void* byte_seq, asize_t size)
{
	while (size > 7)
	{
		uint64* seq_ptr_qword = (uint64*)byte_seq;
		if (get_qword(addr) != (*seq_ptr_qword))
		{
			return false;
		}
		size -= 8;
		addr += 8;
		++seq_ptr_qword;
		byte_seq = (void*)seq_ptr_qword;
	}
	while (size)
	{
		uint64* seq_ptr_byte = (uint64*)byte_seq;
		if (get_byte(addr) != (*seq_ptr_byte))
		{
			return false;
		}
		--size;
		++addr;
		++seq_ptr_byte;
	}
	return true;
}

bool i0_func_probe(const ea_t& addr)
{
	//__debugbreak();
	return i0_check_byte_seq(addr, (void*)i0_func_entry_bytes, sizeof(i0_func_entry_bytes));
}

/*ea_t i0_find_probe_nxt_func(ea_t addr, ea_t limit)
{
	if ((limit - addr) < sizeof(i0_func_entry_bytes))
	{
		return false;
	}
	limit -= sizeof(i0_func_entry_bytes);
	while (addr <= limit)
	{
		if (i0_func_probe(addr))
		{
			return addr;
		}
		++addr;
	}
	return BADADDR;
}
*/