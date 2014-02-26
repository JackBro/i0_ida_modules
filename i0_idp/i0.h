#ifndef I0_H
#define I0_H

#pragma warning(push, 3)
#include "../idaidp.hpp"
#pragma warning(pop)

extern void idaapi i0_header(void);
extern void idaapi i0_footer(void);
extern void idaapi i0_segstart(ea_t);
extern void idaapi i0_segend(ea_t);
extern void idaapi i0_func_header(func_t*);
extern void idaapi i0_func_footer(func_t*);
extern int idaapi i0_ana(void);
extern int idaapi i0_emu(void);
extern void idaapi i0_out_ui(void);
extern void idaapi i0_out_console(void);
extern bool idaapi i0_outop_ui(op_t& op);
extern bool idaapi i0_outop_console(op_t& op);
extern void idaapi i0_data(ea_t addr);
extern bool idaapi i0_cmp_opnd(const op_t&, const op_t&);
extern int idaapi i0_notify(processor_t::idp_notify msgid, ...);

#endif