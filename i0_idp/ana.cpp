#include "i0.h"
#include "ins_def.h"
#include "ins.hpp"
#include "ana.h"
#include <i0_ida_common/i0_mem_layout.h>

static uint64 i0_nxt_byte_wrapper_u(void){ return ((uint8)ua_next_byte()); }
static uint64 i0_nxt_dword_wrapper_u(void){ return ((uint32)ua_next_long()); }
static uint64 i0_nxt_qword_wrapper_u(void){ return ua_next_qword(); }
static uint64 i0_nxt_byte_wrapper_s(void){ return (int64)((int8)ua_next_byte()); }
static uint64 i0_nxt_dword_wrapper_s(void){ return (int64)((int32)ua_next_long()); }
static uint64 i0_nxt_qword_wrapper_s(void){ return ua_next_qword(); }
static uint64 i0_nxt_byte16_wrapper(void){ /*NOT IMPLEMENTED*/ ua_next_qword(); ua_next_qword(); return ((uint64)(-1LL)); }

typedef uint64(*ua_nxt_ptr)(void);

static int trans_to_i0_reg(uint64 addr)
{
	if ((addr >= I0_MEMSPACE_REGFILE_BASE) && (addr <= I0_MEMSPACE_REGFILE_LIMIT) && (!(addr & 7)))
	{
		return i0_reg_BP + (((unsigned)(addr - I0_MEMSPACE_REGFILE_BASE)) / 8);
	}
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
	}
	return -1;
}

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
	i0_ins& operator=(const i0_ins&);
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
	return ((op.dtyp == dt_qword) && (op.type == i0_o_regdispl) && (op.reg == i0_reg_SP) && (op.value == 0));
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
	operand.offb = ((uchar)cmd_offset);
	operand.dtyp = i0_attr_to_ida_type[i0_oper_attr];
	operand.i0_op_spec_attr = ((uchar)i0_oper_attr);
	operand.i0_op_spec_addrm = ((uchar)i0_oper_addrm);
	int32 disp = 0;
	uint64 mem = 0;
	int reg;
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
		reg = trans_to_i0_reg(mem);
	}
	if (i0_oper_addrm == i0_addrm_Abs)
	{
		if (reg >= 0)
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
	if (reg >= 0)
	{
		operand.reg = (uint16)reg;
		if (i0_oper_addrm == i0_addrm_Indir)
		{
			operand.type = i0_o_regdir;
			return i0_attr_byte_len[i0_attr_ue];
		}
		else
		{
			operand.type = i0_o_regdispl;
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

int idaapi
#ifndef NDEBUG
i0_ana_internal(void)
#else
i0_ana(void)
#endif
{
	//int oper_pos;
	i0_ins curr_i0_ins(cmd);
	uint32 oper_attr;
	uint32 oper1_attr;
	uint32 oper2_attr;
	uint32 oper1_addrm;
	uint32 oper2_addrm;
	uint32 oper3_addrm;
	uint32 oper4_addrm;
	uint32 oper5_addrm;
	cmd.i0_ins_flags = 0;
	cmd.i0_ins_attr_pref = (uchar)0xcc;
	cmd.i0_ins_opt_pref = (uchar)0xcc;
	uint32 opcode = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_OPCODE);
	uint32 i0_opt;
	switch (opcode)
	{
	case I0_OPCODE_CONV:
		curr_i0_ins.load(I0_INS_LEN_CONV);
		CHK_I0_ATTR(oper1_attr = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ATTR));
		CHK_I0_ATTR(oper2_attr = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ATTR));
		if (oper1_attr == oper2_attr)
		{
			cmd.itype = I0_ins_mov;
			cmd.i0_ins_flags |= i0_ins_spec_attr_suffix;
			cmd.i0_ins_attr_pref = (char)oper1_attr;
		}
		else
		{
			cmd.itype = I0_ins_conv;
			cmd.i0_ins_flags |= i0_ins_spec_attr_each_opnd;
		}
		CHK_I0_ADDRM(oper1_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM_M(oper2_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		fill_oper(oper1_attr, oper1_addrm, cmd.size, false, cmd.Op1);
		fill_oper(oper2_attr, oper2_addrm, cmd.size, false, cmd.Op2);
		if (i0_is_0sp(cmd.Op2))
		{
			i0_promote_to_dircode(cmd.Op1);
		}
		return cmd.size;
	case I0_OPCODE_ADD:
		cmd.itype = I0_ins_add;
		break;
	case I0_OPCODE_SUB:
		cmd.itype = I0_ins_sub;
		break;
	case I0_OPCODE_MUL:
		cmd.itype = I0_ins_mul;
		break;
	case I0_OPCODE_DIV:
		cmd.itype = I0_ins_div;
		break;
	case I0_OPCODE_AND:
		cmd.itype = I0_ins_and;
		break;
	case I0_OPCODE_OR:
		cmd.itype = I0_ins_or;
		break;
	case I0_OPCODE_XOR:
		cmd.itype = I0_ins_xor;
		break;
	case I0_OPCODE_SPAWN:
		cmd.itype = I0_ins_spawn;
		curr_i0_ins.load(I0_INS_LEN_SPAWN);
		CHK_I0_ADDRM_M(oper1_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM_M(oper2_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM_M(oper3_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM_M(oper4_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		fill_oper(i0_attr_ue, oper1_addrm, cmd.size, false, cmd.Op1);
		fill_oper(i0_attr_ue, oper2_addrm, cmd.size, false, cmd.Op2);
		fill_oper(i0_attr_ue, oper3_addrm, cmd.size, false, cmd.Op3);
		fill_oper(i0_attr_ue, oper4_addrm, cmd.size, false, cmd.Op4);
		return cmd.size;
	case I0_OPCODE_EXIT:
		cmd.itype = I0_ins_exit;
		curr_i0_ins.load(I0_INS_LEN_EXIT);
		cmd.i0_ins_flags |= i0_ins_spec_option;
		cmd.i0_ins_opt_pref = (uchar)(i0_ins_opt_pref_exit_c + curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_OPT_EXIT));
		return cmd.size;
	case I0_OPCODE_B:
		i0_opt = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_OPT_B);
		switch (i0_opt)
		{
		case I0_OPT_B_IJ:
			cmd.itype = I0_ins_bij;
			curr_i0_ins.load(I0_INS_LEN_BIJ);
			CHK_I0_ADDRM_M(oper1_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
			fill_oper(i0_attr_ue, oper1_addrm, cmd.size, true, cmd.Op1);
			return cmd.size;
		case I0_OPT_B_J:
			cmd.itype = I0_ins_bj;
			curr_i0_ins.load(I0_INS_LEN_BJ);
			fill_oper(i0_attr_ue, i0_addrm_Imm, cmd.size, true, cmd.Op1);
			if (curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_RA) == I0_OPT_JUMP_R)
			{
				cmd.Op1.addr += (cmd.ea + cmd.size);//fixup
			}
			return cmd.size;
		case I0_OPT_B_LE:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_b_le;
			break;
		case I0_OPT_B_E:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_b_e;
			break;
		case I0_OPT_B_L:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_b_l;
			break;
		case I0_OPT_B_NE:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_b_ne;
			break;
		case I0_OPT_B_SL:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_b_sl;
			break;
		case I0_OPT_B_Z:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_b_z;
			break;
		case I0_OPT_B_NZ:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_b_nz;
			break;
		default:
			return 0;
		}
		switch (i0_opt)
		{
		case I0_OPT_B_LE:
		case I0_OPT_B_E:
		case I0_OPT_B_L:
		case I0_OPT_B_NE:
		case I0_OPT_B_SL:
			cmd.itype = I0_ins_bcc;
			curr_i0_ins.load(I0_INS_LEN_BCMP);
			cmd.i0_ins_flags |= (i0_ins_spec_attr_suffix | i0_ins_spec_option);
			CHK_I0_ATTR(oper_attr = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ATTR));
			cmd.i0_ins_attr_pref = (uchar)oper_attr;
			CHK_I0_ADDRM(oper1_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
			CHK_I0_ADDRM(oper2_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
			fill_oper(oper_attr, oper1_addrm, cmd.size, false, cmd.Op1);
			fill_oper(oper_attr, oper2_addrm, cmd.size, false, cmd.Op2);
			fill_oper(i0_attr_ue, i0_addrm_Imm, cmd.size, true, cmd.Op3);
			if (curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_RA) == I0_OPT_JUMP_R)
			{
				cmd.Op3.addr += (cmd.ea + cmd.size); //fixup
			}
			return cmd.size;
		case I0_OPT_B_Z:
		case I0_OPT_B_NZ:
			cmd.itype = I0_ins_bcz;
			curr_i0_ins.load(I0_INS_LEN_BZNZ);
			cmd.i0_ins_flags |= (i0_ins_spec_attr_suffix | i0_ins_spec_option);
			CHK_I0_ATTR(oper_attr = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ATTR));
			cmd.i0_ins_attr_pref = (uchar)oper_attr;
			CHK_I0_ADDRM(oper1_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
			fill_oper(oper_attr, oper1_addrm, cmd.size, false, cmd.Op1);
			fill_oper(i0_attr_ue, i0_addrm_Imm, cmd.size, true, cmd.Op2);
			if (curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_RA) == I0_OPT_JUMP_R)
			{
				cmd.Op2.addr += (cmd.ea + cmd.size);
			}
			return cmd.size;
		default:
			assert(0);
			return 0;
		}
	case I0_OPCODE_NOP:
		cmd.itype = I0_ins_nop;
		curr_i0_ins.load(I0_INS_LEN_NOP);
		return cmd.size;
	case I0_OPCODE_INT:
		cmd.itype = I0_ins_int;
		curr_i0_ins.load(I0_OPCODE_INT);
		fill_oper(i0_attr_ub, i0_addrm_Imm, cmd.size, false, cmd.Op1);
		return cmd.size;
	case I0_OPCODE_SHIFT:
		curr_i0_ins.load(I0_INS_LEN_SHIFT);
		i0_opt = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_OPT_SHIFT);
		if (i0_opt >= 2){ return 0; } // currently other 2 options not implemented
		cmd.i0_ins_flags |= i0_ins_spec_attr_suffix;
		cmd.itype = (uint16)(I0_ins_shl + i0_opt);
		CHK_I0_ATTR(oper_attr = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ATTR));
		cmd.i0_ins_attr_pref = (uchar)oper_attr;
		CHK_I0_ADDRM(oper1_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM(oper2_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM_M(oper3_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		fill_oper(oper_attr, oper1_addrm, cmd.size, false, cmd.Op1);
		fill_oper(oper_attr, oper2_addrm, cmd.size, false, cmd.Op2);
		fill_oper(oper_attr, oper3_addrm, cmd.size, false, cmd.Op3);
		return cmd.size;
	case I0_OPCODE_SCMP:
		cmd.itype = I0_ins_scmp;
		curr_i0_ins.load(I0_INS_LEN_SCMP);
		CHK_I0_ADDRM_M(oper1_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM(oper2_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM_M(oper3_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM(oper4_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM_M(oper5_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		fill_oper(i0_attr_ue, oper1_addrm, cmd.size, false, cmd.Op1);
		fill_oper(i0_attr_ue, oper2_addrm, cmd.size, false, cmd.Op2);
		fill_oper(i0_attr_ue, oper3_addrm, cmd.size, false, cmd.Op3);
		fill_oper(i0_attr_ue, oper4_addrm, cmd.size, false, cmd.Op4);
		fill_oper(i0_attr_ue, oper5_addrm, cmd.size, false, cmd.Op5);
		return cmd.size;
	default:
		return 0;
	}
	//only alu instructions fall here!
	switch (opcode)
	{
	case I0_OPCODE_ADD:
	case I0_OPCODE_SUB:
	case I0_OPCODE_MUL:
	case I0_OPCODE_DIV:
	case I0_OPCODE_AND:
	case I0_OPCODE_OR:
	case I0_OPCODE_XOR:
		cmd.i0_ins_flags |= i0_ins_spec_attr_suffix;
		curr_i0_ins.load(I0_INS_LEN_ALU);
		CHK_I0_ATTR(oper_attr = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ATTR));
		cmd.i0_ins_attr_pref = (uchar)oper_attr;
		CHK_I0_ADDRM(oper1_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM(oper2_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		CHK_I0_ADDRM_M(oper3_addrm = curr_i0_ins.fetch_bit(I0_INS_BIT_LEN_ADDRM));
		fill_oper(oper_attr, oper1_addrm, cmd.size, false, cmd.Op1);
		fill_oper(oper_attr, oper2_addrm, cmd.size, false, cmd.Op2);
		fill_oper(oper_attr, oper3_addrm, cmd.size, false, cmd.Op3);
		return cmd.size;
	default:
		assert(0);
		return 0;
	}
}

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
		case i0_o_regdir:
			return (op1.reg == op2.reg);
		case i0_o_regdispl:
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