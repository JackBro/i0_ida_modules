//#include <sstream>
#include "i0.h"
#include "ins.hpp"
#include "ana.h"

#define I0_OPCODE_EXPECTED_PRINT_SPACE	(9U)

static bool i0_out_imm_ui_sb(op_t& op){ return !OutValue(op, OOF_SIGNED | OOF_NUMBER | OOFW_IMM | OOFW_8); }
static bool i0_out_imm_ui_se(op_t& op){ return !OutValue(op, OOF_SIGNED | OOF_NUMBER | OOFW_IMM | OOFW_64); }
static bool i0_out_imm_ui_ss(op_t& op){ (void)op; OutLine("?ss?"); return true; }
static bool i0_out_imm_ui_sf(op_t& op){ return !OutValue(op, OOF_SIGNED | OOF_NUMBER | OOFW_IMM | OOFW_32); }
static bool i0_out_imm_ui_ub(op_t& op){ return !OutValue(op, OOFS_IFSIGN | OOF_NUMBER | OOFW_IMM | OOFW_8); }
static bool i0_out_imm_ui_ue(op_t& op){ return !OutValue(op, OOFS_IFSIGN | OOF_NUMBER | OOFW_IMM | OOFW_64); }
static bool i0_out_imm_ui_us(op_t& op){ (void)op; OutLine("?us?"); return true; }
static bool i0_out_imm_ui_uf(op_t& op){ return !OutValue(op, OOFS_IFSIGN | OOF_NUMBER | OOFW_IMM | OOFW_32); }
static bool i0_out_imm_ui_fs(op_t& op){ return !OutValue(op, OOFS_NOSIGN | OOF_NUMBER | OOFW_IMM | OOFW_32); }
static bool i0_out_imm_ui_fd(op_t& op){ return !OutValue(op, OOFS_NOSIGN | OOF_NUMBER | OOFW_IMM | OOFW_64); }

static bool i0_out_imm_console_sb(op_t& op){ return !!msg("%hhd", (int8)((uint8)op.value)); }
static bool i0_out_imm_console_se(op_t& op){ return !!msg("%lld", (int64)((uint64)op.value)); }
static bool i0_out_imm_console_ss(op_t& op){ (void)op; return !!msg("?ss?"); }
static bool i0_out_imm_console_sf(op_t& op){ return !!msg("%d", (int32)((uint32)op.value)); }
static bool i0_out_imm_console_ub(op_t& op){ return !!msg("%hhu", (uint8)op.value); }
static bool i0_out_imm_console_ue(op_t& op){ return !!msg("%llu", (uint64)op.value); }
static bool i0_out_imm_console_us(op_t& op){ (void)op; return !!msg("?us?"); }
static bool i0_out_imm_console_uf(op_t& op){ return !!msg("%u", (uint32)op.value); }
static bool i0_out_imm_console_fs(op_t& op){ return !!msg("%f", (double)(*reinterpret_cast<float*>(&(op.value)))); }
static bool i0_out_imm_console_fd(op_t& op){ return !!msg("%f", *reinterpret_cast<double*>(&(op.value))); }

typedef bool(*__func_print_imm_op_ret_bool_ptr)(op_t& op);

static __func_print_imm_op_ret_bool_ptr i0_out_imm_ui_attr[] = {
	i0_out_imm_ui_sb,
	i0_out_imm_ui_se,
	i0_out_imm_ui_ss,
	i0_out_imm_ui_sf,
	i0_out_imm_ui_ub,
	i0_out_imm_ui_ue,
	i0_out_imm_ui_us,
	i0_out_imm_ui_uf,
	i0_out_imm_ui_fs,
	i0_out_imm_ui_fd
}; static_assert((qnumber(i0_out_imm_ui_attr) == i0_attr_last), "check i0_out_imm_attr!");

static __func_print_imm_op_ret_bool_ptr i0_out_imm_console_attr[] =
{
	i0_out_imm_console_sb,
	i0_out_imm_console_se,
	i0_out_imm_console_ss,
	i0_out_imm_console_sf,
	i0_out_imm_console_ub,
	i0_out_imm_console_ue,
	i0_out_imm_console_us,
	i0_out_imm_console_uf,
	i0_out_imm_console_fs,
	i0_out_imm_console_fd
}; static_assert((qnumber(i0_out_imm_console_attr) == i0_attr_last), "check i0_out_imm_attr");

inline static bool i0_output_addr_ui(op_t& op)
{
	return !OutValue(op, OOF_ADDR | OOFS_NOSIGN | OOF_NUMBER | OOFW_64);
}

inline static bool i0_output_addr_console(op_t& op)
{
	msg("%llx", op.addr);
	return true;
}

inline static void i0_output_reg_name_ui(op_t& op)
{
	out_register(i0_RegNames[op.reg]);
}

inline static void i0_output_reg_name_console(op_t& op)
{
	msg("%s", i0_RegNames[op.reg]);
}

inline static bool i0_output_imm_console(op_t& op)
{
	msg("$");
	(*i0_out_imm_console_attr[op.i0_op_spec_attr])(op);
	return true;
}

inline static bool i0_output_imm_ui(op_t& op)
{
	OutChar('$');
	return (*i0_out_imm_ui_attr[op.i0_op_spec_attr])(op);
}

static bool idaapi i0_outop(op_t& op, bool is_ui)
{
	//__debugbreak();
	switch (op.type)
	{
	case i0_o_dir_code:
	case i0_o_dir:
		if (is_ui){ return i0_output_addr_ui(op); }
		else{ return i0_output_addr_console(op); }
	case i0_o_imm:
		if (is_ui){ return i0_output_imm_ui(op); }
		else{ return i0_output_imm_console(op); }
	case i0_o_reg:
		if (is_ui){ i0_output_reg_name_ui(op); return true; }
		else{ i0_output_reg_name_console(op); return true; }
	}
	bool ret_status = true;
	switch (op.type)
	{
	case i0_o_regdispl:
	case i0_o_mem_displ:
		if (is_ui){ ret_status = (ret_status && i0_out_imm_ui_se(op)); }
		else{ ret_status = (ret_status && i0_out_imm_console_se(op)); }
		break;
	}
	if (is_ui){ out_symbol('('); }
	else{ msg("("); }
	switch (op.type)
	{
	case i0_o_regdispl:
	case i0_o_regdir:
		if (is_ui){ i0_output_reg_name_ui(op); }
		else{ i0_output_reg_name_console(op); }
		break;
	case i0_o_mem_displ:
	case i0_o_mem_indir:
		if (is_ui){ ret_status = (ret_status && i0_output_addr_ui(op)); }
		else{ ret_status = (ret_status && i0_output_addr_console(op)); }
		break;
	default:
		assert(0);
	}
	if (is_ui){ out_symbol(')'); }
	else{ msg(")"); }
	return ret_status;
}

bool idaapi i0_outop_console(op_t& op)
{
	return i0_outop(op, false);
}

bool idaapi i0_outop_ui(op_t& op)
{
	return i0_outop(op, true);
}

void idaapi i0_out_ui(void)
{
	//__debugbreak();
	char buf[MAXSTR];
	init_output_buffer(buf, sizeof(buf));
	char suffix_buf[I0_OPCODE_EXPECTED_PRINT_SPACE];
	int char_written = 0;
	if (cmd.i0_ins_flags & i0_ins_spec_option)
	{
		char_written += qsnprintf(suffix_buf + char_written, sizeof(suffix_buf)-char_written, "%s", i0_ins_options[cmd.i0_ins_opt_pref]);
	}
	if (cmd.i0_ins_flags & i0_ins_spec_attr_suffix)
	{
		char_written += qsnprintf(suffix_buf + char_written, sizeof(suffix_buf)-char_written, "%s", i0_ins_attr_suffix[cmd.i0_ins_attr_pref]);
	}
	if ((cmd.i0_ins_flags & i0_ins_spec_attr_suffix) || (cmd.i0_ins_flags & i0_ins_spec_option))
	{
		OutMnem(I0_OPCODE_EXPECTED_PRINT_SPACE, suffix_buf);
	}
	else
	{
		OutMnem(I0_OPCODE_EXPECTED_PRINT_SPACE);
	}
	if (i0_Ins_Opnd_Cnt[cmd.itype] > 0)
	{
		unsigned i = 0;
		for (;;)
		{
			out_one_operand(i);
			if (cmd.i0_ins_flags & i0_ins_spec_attr_each_opnd)
			{
				out_line(i0_ins_attr_suffix[cmd.Operands[i].i0_op_spec_attr], COLOR_INSN);
			}
			++i;
			if (i == i0_Ins_Opnd_Cnt[cmd.itype])
			{
				break;
			}
			else
			{
				out_symbol(',');
				OutChar(' ');
			}
		}
	}
	term_output_buffer();
	MakeLine(buf);
}

void idaapi i0_out_console(void)
{
	//__debugbreak();
	msg("%llx: ", cmd.ea);
	char opbuf[I0_OPCODE_EXPECTED_PRINT_SPACE];
	memset(opbuf, ((unsigned char)' '), sizeof(opbuf)-1);
	opbuf[sizeof(opbuf)-1] = 0;
	int char_written = 0;
	char_written = qsnprintf(opbuf, sizeof(opbuf), "%s", i0_Instructions[cmd.itype].name);
	if (cmd.i0_ins_flags & i0_ins_spec_option)
	{
		char_written += qsnprintf(opbuf + char_written, sizeof(opbuf)-char_written, "%s", i0_ins_options[cmd.i0_ins_opt_pref]);
	}
	if (cmd.i0_ins_flags & i0_ins_spec_attr_suffix)
	{
		char_written += qsnprintf(opbuf + char_written, sizeof(opbuf)-char_written, "%s", i0_ins_attr_suffix[cmd.i0_ins_attr_pref]);
	}
	if (char_written <sizeof(opbuf)-1)
	{
		opbuf[char_written] = ' ';
	}
	msg("%s", opbuf);
	if (i0_Ins_Opnd_Cnt[cmd.itype] > 0)
	{
		unsigned i;
		for (i = 0; i < (i0_Ins_Opnd_Cnt[cmd.itype] - 1); i++)
		{
			i0_outop_console(cmd.Operands[i]);
			msg(", ");
		}
		i0_outop_console(cmd.Operands[i]);
	}
	msg("\n");
}

void idaapi i0_data(ea_t op)
{
	(void)op;
}

void idaapi i0_header(void)
{

}

void idaapi i0_footer(void)
{

}

void idaapi i0_func_header(func_t*)
{}

void idaapi i0_func_footer(func_t*)
{}

void idaapi i0_segstart(ea_t addr)
{
	(void)addr;
}

void idaapi i0_segend(ea_t addr)
{
	(void)addr;
}