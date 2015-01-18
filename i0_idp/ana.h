#ifndef ANA_H
#define ANA_H

#define i0_o_imm		o_imm
#define i0_o_reg		o_reg
#define i0_o_dir		o_mem
#define i0_o_dir_code	o_near
#define i0_o_reg_indir	o_phrase
#define i0_o_reg_displ	o_displ
#define i0_o_mem_indir	o_idpspec0
#define i0_o_mem_displ	o_idpspec1

#define i0_op_spec_attr		specflag1
#define i0_op_spec_addrm	specflag2

#define i0_ins_spec_attr_suffix		1U
#define i0_ins_spec_attr_each_opnd	2U
#define i0_ins_spec_option			4U

#define i0_ins_flags			auxpref //flag
#define i0_ins_attr_pref		insnpref //attr
#define i0_ins_opt_pref			segpref //branch specific options

#endif