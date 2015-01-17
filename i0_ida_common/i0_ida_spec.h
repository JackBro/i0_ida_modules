#ifndef I0_IDA_SPEC_H
#define I0_IDA_SPEC_H

#define I0_IDA_LPH_ID	(0x8001U)
#define I0_IDA_SHORT_NAME	"i0"
#define I0_IDA_LONG_NAME	"l0sys VPC i0"

enum I0_LDR_IDP_notify{
	i0_loader_req_init_symtable,
	i0_loader_req_insert_sym,
	i0_loader_req_finish_symtable
};

enum I0_SYM_TYPE{
	i0_sym_none,
	i0_sym_func,
	i0_sym_local,
	i0_sym_data,
};

#endif
