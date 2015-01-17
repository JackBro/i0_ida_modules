#ifndef INS_HPP
#define INS_HPP

#include "i0.h"

extern instruc_t i0_Instructions[];
extern unsigned i0_Ins_Opnd_Cnt[];
extern const char* i0_RegNames[];
extern const char* i0_ins_attr_suffix[];
extern const char* i0_ins_options[];
extern const size_t i0_number_of_register;

enum I0_ins_options{
	i0_ins_opt_pref_b_le,
	i0_ins_opt_pref_b_l,
	i0_ins_opt_pref_b_e,
	i0_ins_opt_pref_b_ne,
	i0_ins_opt_pref_b_sl,
	i0_ins_opt_pref_exit_c,
	i0_ins_opt_pref_exit_a,
	i0_ins_opt_pref_exit_cd,
	i0_ins_opt_pref_exit_ad,
	i0_ins_opt_pref_b_z,
	i0_ins_opt_pref_b_nz,
	i0_ins_opt_pref_last
};

enum i0_ins_names
{
		I0_ins_conv,
		I0_ins_add,
		I0_ins_sub,
		I0_ins_mul,
		I0_ins_div,
		I0_ins_and,
		I0_ins_or,
		I0_ins_xor,
		I0_ins_spawn,
		I0_ins_spawnx,
		I0_ins_exit,
		I0_ins_bcc,
		I0_ins_bcz,
		I0_ins_bj,
		I0_ins_bij,
		I0_ins_nop,
		I0_ins_int,
		I0_ins_shl,
		I0_ins_shr,
		I0_ins_scmp,
		I0_ins_mov,
		I0_ins_last_ins
};

enum i0_instr_addrm{
	i0_addrm_Imm = 0,
	i0_addrm_Abs = 1,
	i0_addrm_Indir = 2,
	i0_addrm_Disp = 3,
	i0_addrm_last
};

enum i0_oper_attr{
	i0_attr_sb = 0,
	i0_attr_se = 1,
	i0_attr_ss = 2,
	i0_attr_sf = 3,
	i0_attr_ub = 4,
	i0_attr_ue = 5,
	i0_attr_us = 6,
	i0_attr_uf = 7,
	i0_attr_fs = 8,
	i0_attr_fd = 9,
	i0_attr_last
};

enum I0_regs {
	i0_reg_BP,
	i0_reg_SP,
	i0_reg_R2,
	i0_reg_R3,
	i0_reg_R4,
	i0_reg_R5,
	i0_reg_R6,
	i0_reg_R7,
	i0_reg_rVcs,
	i0_reg_rVds,
	i0_reg_l0_stdin,
	i0_reg_l0_stdout,
	i0_reg_TaskWpr_sp,
	i0_reg_Syscall_sp,
	i0_reg_rrStkBase,
	i0_reg_rrStkLen,
	i0_reg_rr_fi,
	i0_reg_rr_ID,
	i0_reg_I0_regs_last
};

#endif