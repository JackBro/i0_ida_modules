#ifndef I0_H
#define I0_H

#pragma warning(push, 3)
#include "../idaidp.hpp"
#pragma warning(pop)
#include <i0_ida_common\i0_ida_spec.h>
#include <map>
#include <string>

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
extern int idaapi i0_is_align_ins(ea_t ea);
extern int idaapi i0_is_sp_based(const op_t& op);
extern int idaapi i0_notify(processor_t::idp_notify msgid, ...);

typedef std::pair<std::string, I0_SYM_TYPE> i0_sym_entry_t;
typedef std::map<ea_t, i0_sym_entry_t> i0_sym_map_t;
typedef i0_sym_map_t::iterator i0_sym_map_iterator_t;

extern i0_sym_map_t i0_sym_map;
extern bool i0_sym_map_file_loaded;
extern const i0_sym_entry_t* i0_find_sym_by_addr(const ea_t&);

#endif