#include "i0.h"
#include "ana.h"
#include "ins.hpp"
#include <frame.hpp>
#include <fstream>
#include <string>

static void TouchArg(const op_t& op, bool is_read)
{
	(void)is_read;
	if (op.type == i0_o_dir_code)
	{
		ua_add_cref(op.offb, op.addr, fl_JN);
	}

}

int idaapi i0_emu(void)
{
#ifndef NDEBUG
	//msg("emu @ %llx\n", cmd.ea);
#endif
	if (i0_sym_map_file_loaded)
	{
		std::map<ea_t,std::string>::iterator i = i0_sym_map.find(cmd.ea);
		if (i != i0_sym_map.end())
		{
			set_name(cmd.ea, i->second.c_str());
			++i;
			if (i != i0_sym_map.end())
			{
				add_func(cmd.ea, i->first);
			}
			else
			{
				//problem!
			}
		}

	}
	uint32 cmd_feature = cmd.get_canon_feature();
	if (cmd_feature & CF_USE1){ TouchArg(cmd.Op1, 1); }
	if (cmd_feature & CF_USE2){ TouchArg(cmd.Op2, 1); }
	if (cmd_feature & CF_USE3){ TouchArg(cmd.Op3, 1); }
	if (cmd_feature & CF_USE4){ TouchArg(cmd.Op4, 1); }
	if (cmd_feature & CF_USE5){ TouchArg(cmd.Op5, 1); }
	if (cmd_feature & CF_CHG1){ TouchArg(cmd.Op1, 0); }
	if (cmd_feature & CF_CHG2){ TouchArg(cmd.Op2, 0); }
	if (cmd_feature & CF_CHG3){ TouchArg(cmd.Op3, 0); }
	if (cmd_feature & CF_CHG4){ TouchArg(cmd.Op4, 0); }
	if (cmd_feature & CF_CHG5){ TouchArg(cmd.Op5, 0); }

	if (!(cmd_feature & CF_STOP))
	{
		ua_add_cref(0, cmd.ea + cmd.size, fl_F);
	}
	else
	{
		if (cmd.itype == I0_ins_bj)
		{
			func_t* current_func = get_func(cmd.ea);
			if (current_func)
			{
				if (!current_func->contains(cmd.Op1.addr))
				{
					ua_add_cref(0, cmd.ea + cmd.size, fl_F);
				}
			}
		}
	}

	return 1;
}