#include "i0.h"
#include "ins_def.h"
#include "ins.hpp"
#include "ana.h"
#include <i0_ida_common/i0_mem_layout.h>

static bool trans_to_i0_reg(uint64 addr, uint16& reg)
{
	if ((addr >= I0_MEMSPACE_REGFILE_BASE) 
		&& (addr <= I0_MEMSPACE_REGFILE_LIMIT) 
		&& (!(addr % sizeof(uint64))))
	{
		reg = i0_reg_BP + (((unsigned)(addr - I0_MEMSPACE_REGFILE_BASE)) / 8U);
	}
	/*
	switch (addr)
	{
	case I0_MEMSPACE_STDIN:
		return i0_reg_l0_stdin;
	case I0_MEMSPACE_STDOUT:
		return i0_reg_l0_stdout;
	case I0_MEMSPACE_SAVED_TASK_WRAPPER_SP:
		return i0_reg_TaskWpr_sp;
	case I0_MEMSPACE_SAVED_TASK_SP_SYSCALL:
		return i0_reg_Syscall_sp;
	case I0_MEMSPACE_CURRENT_TASK_STACKBASE:
		return i0_reg_rrStkBase;
	case I0_MEMSPACE_CURRENT_TASK_STACKLEN:
		return i0_reg_rrStkLen;
	case I0_MEMSPACE_CURRENT_TASK_FI:
		return i0_reg_rr_fi;
	case I0_MEMSPACE_CURRENT_TASK_ID:
		return i0_reg_rr_ID;
	}*/
	return false;
}


static uint64 i0_nxt_byte_wrapper_u(void){ return ((uint8)ua_next_byte()); }
static uint64 i0_nxt_dword_wrapper_u(void){ return ((uint32)ua_next_long()); }
static uint64 i0_nxt_qword_wrapper_u(void){ return ua_next_qword(); }
static uint64 i0_nxt_byte_wrapper_s(void){ return (int64)((int8)ua_next_byte()); }
static uint64 i0_nxt_dword_wrapper_s(void){ return (int64)((int32)ua_next_long()); }
static uint64 i0_nxt_qword_wrapper_s(void){ return ua_next_qword(); }
static uint64 i0_nxt_byte16_wrapper(void){ /*NOT IMPLEMENTED*/ ua_next_qword(); ua_next_qword(); return ((uint64)(-1LL)); }

typedef uint64(*ua_nxt_ptr)(void);


static ua_nxt_ptr ua_nxt_attr[] = {
	i0_nxt_byte_wrapper_s,
	i0_nxt_qword_wrapper_s,
	i0_nxt_byte16_wrapper,
	i0_nxt_dword_wrapper_s,
	i0_nxt_byte_wrapper_u,
	i0_nxt_qword_wrapper_u,
	i0_nxt_byte16_wrapper,
	i0_nxt_dword_wrapper_u,
	i0_nxt_dword_wrapper_u,
	i0_nxt_qword_wrapper_u
}; static_assert((qnumber(ua_nxt_attr) == i0_attr_last), "check ua_next_ptr!");

static const char i0_attr_to_ida_type[] =
{
	dt_byte,
	dt_qword,
	dt_byte16,
	dt_dword,
	dt_byte,
	dt_qword,
	dt_byte16,
	dt_dword,
	dt_float,
	dt_double
}; static_assert((qnumber(i0_attr_to_ida_type) == i0_attr_last), "check i0_attr_to_ida_type!");

static const uint16 i0_attr_byte_len[] =
{
	1U,
	8U,
	16U,
	4U,
	1U,
	8U,
	16U,
	4U,
	4U,
	8U
}; static_assert((qnumber(i0_attr_byte_len) == i0_attr_last), "check i0_attr_byte_len");

#include "i0_instr.cpp"

static inline bool is_valid_oper_attr(uint32 attr)
{
	if ((attr == ((uint32)i0_attr_fs)) || (attr == ((uint32)i0_attr_us)))
	{
		return false;
	}
	return (attr < ((uint32)i0_attr_last));
}

static inline bool is_valid_oper_addrm(uint32 addrm)
{
	return (addrm < ((uint32)i0_addrm_last));
}

static inline bool is_valid_oper_addrm_m(uint32 addrm)
{
	return ((addrm != ((uint32)i0_addrm_Imm)) && is_valid_oper_addrm(addrm));
}

/*static inline bool is_valid_ins_byte(ea_t addr)
{
return isLoaded(addr);
}

static inline bool is_valid_ins_word(ea_t addr)
{
return (is_valid_ins_byte(addr) && is_valid_ins_byte(addr + 1));
}

static inline bool is_valid_ins_long(ea_t addr)
{
return (is_valid_ins_word(addr) && is_valid_ins_word(addr + 2));
}

static inline bool is_valid_ins_qword(ea_t addr)
{
return (is_valid_ins_long(addr) && is_valid_ins_long(addr + 4));
}*/


#pragma pack(push, 1)

union __i0_opcode__ {
	__i0_opcode__() : oc_u32(0){ }
	uint32 oc_u32;
	uint8 oc_bytes[4];
}; static_assert((sizeof(__i0_opcode__) == 4), "check oc_bytes size");

class i0_ins{
	/*struct OC_BYTES{
		uint8 oc_byte[4];
		};*/
	/*struct OC_WORDS{
		uint16 oc_word0;
		uint16 oc_word1;
		};*/
	//for little endian machine ONLY!!!
private:
	i0_ins();
	__i0_opcode__ op;
	int shift_cnt;
	insn_t& insn_ref;
	void load_bytes(uint16 cnt)
	{
		uint16 load_cnt = insn_ref.size + cnt;
		while (cnt)
		{
			op.oc_bytes[3 - load_cnt + cnt] = ua_next_byte();
			--cnt;
		}
	}
	i0_ins& operator=(const i0_ins&) = default;
public:
	i0_ins(insn_t& cmd_ref) : insn_ref(cmd_ref), shift_cnt(0){
		load_bytes(I0_INS_LEN_OPCODE);
	}
	uint32 get_bit(unsigned start, unsigned len)
	{
		return ((op.oc_u32 << start) >> (32 - len));
	}
	void load(unsigned byte_len)
	{
		load_bytes((uint16)(byte_len - I0_INS_LEN_OPCODE));
	}
	uint32 fetch_bit(unsigned len)
	{
		uint32 ret = get_bit(shift_cnt, len);
		shift_cnt += len;
		return ret;
	}
};

#pragma pack(pop)

static inline bool i0_is_0sp(const op_t& op)
{
	return ((op.dtyp == dt_qword) && (op.type == i0_o_reg_displ) && (op.reg == i0_reg_SP) && (op.value == 0));
}

static inline bool i0_promote_to_dircode(op_t& op)
{
	if ((op.type == i0_o_imm) && (op.dtyp == dt_qword))
	{
		op.addr = op.value;
		op.flags &= (~OF_NUMBER);
		op.type = i0_o_dir_code;
		return true;
	}
	else
	{
		return false;
	}
}

static uint16 fill_oper(uint32 i0_oper_attr, uint32 i0_oper_addrm, uint16 cmd_offset, bool is_code_ref, op_t& operand)
{
	operand.offb = ((uint8)cmd_offset);
	operand.dtyp = i0_attr_to_ida_type[i0_oper_attr];
	operand.i0_op_spec_attr = ((uint8)i0_oper_attr);
	operand.i0_op_spec_addrm = ((uint8)i0_oper_addrm);
	int32 disp = 0;
	uint64 mem = 0;
	uint16 reg = -1;
	switch (i0_oper_addrm)
	{
	case i0_addrm_Imm:
		if (is_code_ref)
		{
			operand.type = i0_o_dir_code;
			operand.addr = ua_next_qword();
			return i0_attr_byte_len[i0_attr_ue];
		}
		else
		{
			operand.type = i0_o_imm;
			operand.flags |= OF_NUMBER;
			operand.value = (*ua_nxt_attr[i0_oper_attr])();
			return i0_attr_byte_len[i0_oper_attr];
		}
	case i0_addrm_Disp:
		disp = ua_next_long();
	default:
		mem = ua_next_qword();
		trans_to_i0_reg(mem, reg);
	}
	if (i0_oper_addrm == i0_addrm_Abs)
	{
		if (reg != -1)
		{
			operand.reg = (uint16)reg;
			operand.type = i0_o_reg;
		}
		else
		{
			operand.addr = mem;
			operand.type = i0_o_dir;
		}
		return i0_attr_byte_len[i0_attr_ue];
	}
	if (reg != -1)
	{
		operand.reg = (uint16)reg;
		if (i0_oper_addrm == i0_addrm_Indir)
		{
			operand.type = i0_o_reg_indir;
			return i0_attr_byte_len[i0_attr_ue];
		}
		else
		{
			operand.type = i0_o_reg_displ;
			operand.value = (int64)(disp);
			return i0_attr_byte_len[i0_attr_uf] + i0_attr_byte_len[i0_attr_ue];
		}
	}
	else
	{
		operand.addr = mem;
		if (i0_oper_addrm == i0_addrm_Indir)
		{
			operand.type = i0_o_mem_indir;
			return i0_attr_byte_len[i0_attr_ue];
		}
		else
		{
			operand.type = i0_o_mem_displ;
			operand.value = (int64)disp;
			return i0_attr_byte_len[i0_attr_uf] + i0_attr_byte_len[i0_attr_ue];
		}
	}
}

#define CHK_I0_ATTR(attr) \
{if (!is_valid_oper_attr(attr)){ return 0; }}

#define CHK_I0_ADDRM(addrm) \
{if (!is_valid_oper_addrm(addrm)){ return 0; }}

#define CHK_I0_ADDRM_M(addrm) \
{if (!is_valid_oper_addrm_m(addrm)){ return 0; }}

#define CHK_LOAD_STATUS(status) \
{if (!(status)) {return 0;}}

static insn_t& local_cmd_ref = cmd;



bool idaapi i0_cmp_opnd(const op_t& op1, const op_t& op2)
{
	if ((op1.type == op2.type) && (op1.dtyp == op2.dtyp))
	{
		switch (op1.type)
		{
		case i0_o_imm:
			return (op1.value == op2.value);
		case i0_o_reg:
			return (op1.reg == op2.reg);
		case i0_o_dir:
			return (op1.addr == op2.addr);
		case i0_o_dir_code:
			return (op1.value == op2.value);
		case i0_o_reg_indir:
			return (op1.reg == op2.reg);
		case i0_o_reg_displ:
			return ((op1.reg == op2.reg) && (op1.value == op2.value));
		case i0_o_mem_indir:
			return (op1.addr == op2.addr);
		case i0_o_mem_displ:
			return ((op1.addr == op2.addr) && (op1.value == op2.value));
		}
	}
	return false;
}

#ifndef NDEBUG
int idaapi i0_ana(void)
{
	//__debugbreak();
	int ret = i0_ana_internal();
	if (ret)
	{
		//i0_out_console();
	}
	return ret;
}
#endif