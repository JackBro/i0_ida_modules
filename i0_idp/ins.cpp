#include "ins.hpp"

const char* i0_RegNames[] = {
	"BP", "SP", "R2", "R3",
	"R4", "R5", "R6", "R7",
	/*
	"l0_stdin",
	"l0_stdout",
	"TaskWpr_sp",
	"Syscall_sp",
	"rr_StkBase",
	"rr_StkLen",
	"rr_fi",
	"rr_ID"*/
}; static_assert((qnumber(i0_RegNames) == i0_reg_I0_regs_last), "i0 reg number illegal");
const size_t i0_number_of_register = qnumber(i0_RegNames);

const char* i0_ins_attr_suffix[] = {
	":sb",
	":se",
	":ss",
	":sf",
	":ub",
	":ue",
	":us",
	":uf",
	":fs",
	":fd"
}; static_assert((qnumber(i0_ins_attr_suffix) == i0_attr_last), "i0 ins suffix number illegal");

const char* i0_ins_options[] = {
	":c",
	":a",
	":cd",
	":ad",
}; static_assert((qnumber(i0_ins_options) == i0_ins_opt_pref_last), "i0 ins option numbers illegal");

instruc_t i0_Instructions[] = {
	{ "conv", CF_USE1 | CF_CHG2 },
	{ "add", CF_USE1 | CF_USE2 | CF_CHG3 },
	{ "sub", CF_USE1 | CF_USE2 | CF_CHG3 },
	{ "mul", CF_USE1 | CF_USE2 | CF_CHG3 },
	{ "div", CF_USE1 | CF_USE2 | CF_CHG3 },
	{ "and", CF_USE1 | CF_USE2 | CF_CHG3 },
	{ "or", CF_USE1 | CF_USE2 | CF_CHG3 },
	{ "xor", CF_USE1 | CF_USE2 | CF_CHG3 },
	{ "spawn", CF_USE1 | CF_USE2 | CF_USE3 | CF_USE4 | CF_CALL },
	{ "spawnx", CF_USE1 | CF_USE2 | CF_USE3 | CF_USE4 | CF_USE5 | CF_CALL },
	{ "exit", CF_STOP },
	{ "ble", CF_USE1 | CF_USE2 | CF_USE3 | CF_JUMP },
	{ "be", CF_USE1 | CF_USE2 | CF_USE3 | CF_JUMP },
	{ "bl", CF_USE1 | CF_USE2 | CF_USE3 | CF_JUMP },
	{ "bne", CF_USE1 | CF_USE2 | CF_USE3 | CF_JUMP },
	{ "bsl", CF_USE1 | CF_USE2 | CF_USE3 | CF_JUMP },
	{ "bz", CF_USE1 | CF_USE2 | CF_JUMP },
	{ "bnz", CF_USE1 | CF_USE2 | CF_JUMP },
	{ "bj", CF_USE1 | CF_JUMP | CF_STOP },
	{ "bij", CF_USE1 | CF_JUMP | CF_STOP },
	{ "nop", 0 },
	{ "int", 0 },
	{ "shl", CF_USE1 | CF_USE2 | CF_CHG3 | CF_SHFT },
	{ "shr", CF_USE1 | CF_USE2 | CF_CHG3 | CF_SHFT },
	{ "scmp", CF_USE1 | CF_USE2 | CF_USE3 | CF_USE4 | CF_CHG5 },
	{ "grep", CF_USE1 | CF_USE2 | CF_USE3 | CF_USE4 | CF_CHG5 },
	{ "mov", CF_USE1 | CF_CHG2 },
}; static_assert((qnumber(i0_Instructions) == I0_ins_last_ins), "i0 ins number illegal");

unsigned i0_Ins_Opnd_Cnt[] =
{
	2, //conv
	3, //add
	3, //sub
	3, //mul
	3, //div
	3, //and
	3, //or
	3, //xor
	4, //spawn
	5, //spawnx
	0, //exit
	3, //ble
	3, //be
	3, //bl
	3, //bne
	3, //bsl
	2, //bz
	2, //bnz
	1, //bj
	1, //bij
	0, //nop
	1, //int
	3, //shl
	3, //shr
	5, //scmp
	5, //grep
	2, //mov
}; static_assert((qnumber(i0_Ins_Opnd_Cnt) == I0_ins_last_ins), "i0 ins opt number illegal");