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

int idaapi
#ifndef NDEBUG
i0_ana_internal(void)
#else
i0_ana(void)
#endif
{
	std::unique_ptr<I0Instruction> i0_ins(I0Instruction::Create(cmd.ea));
	i0_ins->Serialize();
	return cmd.size;
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