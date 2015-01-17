#define USE_STANDARD_FILE_FUNCTIONS

#pragma warning(push, 3)
#include "../idaldr.h"
#pragma warning(pop)
#include <i0_ida_common/i0_ida_spec.h>
#include <i0_ida_common/i0_mem_layout.h>
#include <i0_ida_common/i0_ida_common_lib.h>
#include <fstream>
#include <string>

#define I0_LOADER_FLAGS 0

static processor_t& local_ph_ref = ph;

int idaapi i0_accept_file(linput_t *li, char fileformatname[MAX_FILE_FORMAT_NAME], int n)
{
	int32 file_size = qlsize(li);
	if (n)
	{
		return 0;
	}
	if ((file_size > 0) && (((unsigned)file_size) < I0_MEMSPACE_PROGLOAD_SIZE))
	{
		qsnprintf(fileformatname, MAX_FILE_FORMAT_NAME, "i0 program file");
		return 1;
	}
	return 0;
}

static void i0_parse_map_file(std::istream& input, segment_t* seg)
{
	input >> std::hex;
	ea_t addr;
	std::string sym;
	while (input >> addr >> sym)
	{
		msg("i0_sym: %llx \t%s: ", addr, sym.c_str());
		I0_SYM_TYPE sym_type;
		if (seg->contains(addr))
		{
			if (i0_func_probe(addr))
			{
				sym_type = i0_sym_func;
				msg("[func]");
			}
			else
			{
				sym_type = i0_sym_local;
				msg("[local]");
			}
		}
		else
		{
			sym_type = i0_sym_data;
			msg("[data]");
		}
		msg("\n");
		ph.notify(processor_t::loader, i0_loader_req_insert_sym, &addr, sym.c_str(), sym_type);
	}
}

void idaapi i0_load_file(linput_t *li, ushort neflag, const char *fileformatname)
{
	int32 file_size = qlsize(li);
	(void)neflag;
	(void)fileformatname;
	ea_t i0_code_segment_end;
	if (ph.id != I0_IDA_LPH_ID)
	{
		char* proc_module_path = set_processor_type(I0_IDA_SHORT_NAME, SETPROC_ALL | SETPROC_FATAL);
		msg("i0 IDP is loaded %s\n", proc_module_path);
	}
	if ((file_size > 0) && (((unsigned)file_size) < I0_MEMSPACE_PROGLOAD_SIZE))
	{
		i0_code_segment_end = (I0_MEMSPACE_PROGLOAD_BASE + ((unsigned)file_size));
		if (!file2base(li, 0, I0_MEMSPACE_PROGLOAD_BASE, i0_code_segment_end, FILEREG_PATCHABLE))
		{
			loader_failure("load file to database failed!\n");
		}
		msg("i0 progman file loaded to database, FILE size %.1fK\n", (((double)file_size) / 1024));
		if (!add_segm(0, I0_MEMSPACE_PROGLOAD_BASE, (I0_MEMSPACE_PROGLOAD_BASE + ((unsigned)file_size)), NAME_CODE, CLASS_CODE))
		{
			loader_failure("unable to create text segment!\n");
		}
		msg("i0 .text segment created, address range %llX to %llX\n", I0_MEMSPACE_PROGLOAD_BASE, (I0_MEMSPACE_PROGLOAD_BASE + ((unsigned)file_size)));
		segment_t* text_seg = getseg(I0_MEMSPACE_PROGLOAD_BASE);
		if (!set_segm_addressing(text_seg, 2))
		{
			loader_failure("unable to set text segment addressing mode!\n");
		}
		msg("i0 .text segment addressing mode set to 64bit\n");
		create_filename_cmt();
		add_entry(I0_MEMSPACE_PROGLOAD_BASE, I0_MEMSPACE_PROGLOAD_BASE,"i0_entry", true);
		msg("i0 program file successfully loaded\n");

		linput_type_t i0_file_type = get_linput_type(li);
		if (i0_file_type != LINPUT_LOCAL)
		{
			msg("i0 program file is not local??? bypass map file loading\n");
		}
		else
		{
			char map_file_path[QMAXPATH];
			get_input_file_path(map_file_path, sizeof(map_file_path));
			set_file_ext(map_file_path, sizeof(map_file_path), map_file_path, "map");
			std::ifstream i0_map_file;
			i0_map_file.open(map_file_path);
			if (!i0_map_file.fail())
			{
				msg("possible map file %s\n", map_file_path);
				ph.notify(processor_t::loader, i0_loader_req_init_symtable);
				i0_parse_map_file(i0_map_file, text_seg);
				ph.notify(processor_t::loader, i0_loader_req_insert_sym, &i0_code_segment_end, "__i0_text_end", i0_sym_func);
				ph.notify(processor_t::loader, i0_loader_req_finish_symtable);
			}
		}


		return;
	}
	loader_failure("file size illegal!\n");
}

//static int idaapi i0_save_file(FILE *fp, const char* fileformatname)
//static int idaapi i0_move_segm(ea_t from, ea_t to, asize_t /*size*/, const char * /*fileformatname*/)
/*bool idaapi i0_init_loader_options(linput_t*)
{
	char* ret = set_processor_type("Z8", SETPROC_ALL | SETPROC_FATAL);
	msg("i0_ldr: set proc type returned [%s]", ret);
	return true;
}*/

loader_t LDSC = {
	IDP_INTERFACE_VERSION,
	I0_LOADER_FLAGS,
	i0_accept_file,
	i0_load_file,
	/*i0_save_file*/NULL,
	/*i0_move_segm*/NULL,
	/*i0_init_loader_options*/NULL
};