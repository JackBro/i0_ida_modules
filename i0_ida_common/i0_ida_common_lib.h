#ifndef I0_IDA_COMMON_LIB_H
#define I0_IDA_COMMON_LIB_H

#pragma warning(push, 3)
#include "../../module/idaidp.hpp"
#pragma warning(pop)

//extern const uint8 i0_func_entry_bytes[];
extern bool i0_check_byte_seq(ea_t addr, void* byte_seq, asize_t size);
extern bool i0_func_probe(const ea_t& addr);

#endif
