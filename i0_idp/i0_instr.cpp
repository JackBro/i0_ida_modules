#include <memory>

#pragma warning(push)
#pragma warning(disable:4512)
#pragma warning(disable:4201)

#pragma pack(push, 1)

class I0OperandD;
class I0OperandM;
class I0OperandI;
class I0OperandMAbs;
class I0OperandMIndir;
class I0OperandMDisp;

typedef std::unique_ptr<I0OperandD> I0OperD;
typedef std::unique_ptr<I0OperandM> I0OperM;
typedef std::unique_ptr<I0OperandI> I0OperI;

class I0OperandD{
private:
	uint16 offset;
public:
	inline I0OperandD(uint8 _Attr, I0_oper_types _OperType)
		: OperType(_OperType), Attr(static_cast<i0_oper_attr>(_Attr)), offset(cmd.size)
	{}
	const I0_oper_types OperType;
	i0_oper_attr Attr;
	static inline I0OperandD* Create(uint8 A, uint8 addrm);
	virtual ~I0OperandD() = 0
	{}
	virtual inline void Serialize(bool, op_t& operand)
	{
		operand.offb = (int8)offset;
		operand.dtyp = i0_attr_to_ida_type[Attr];
		operand.i0_op_spec_attr = Attr;
	}
};

class I0OperandI : public I0OperandD{
private:
	union{
		uint8 ub;
		int8 sb;
		uint32 uf;
		int32 sf;
		uint64 ue;
		int64 se;
		double fd;
		float fs;
	}value;
public:
	inline static I0OperandI* Create(uint8 A)
	{
		return new I0OperandI(A);
	}
	inline static I0OperandI* CreateBranchTarget(uint32 ra, uint64 ip)
	{
		I0OperandI* ret = new I0OperandI(I0_ATTR_UE);
		if (ra == I0_OPT_JUMP_R)
		{
			ret->value.ue += (ip + cmd.size);
		}
		return ret;
	}
	inline I0OperandI(uint8 _Attr) : I0OperandD(_Attr, I0_oper_types::i0_opertype_I)
	{
		switch (Attr)
		{
		case I0_ATTR_UB:
		case I0_ATTR_SB:
			value.ub = ua_next_byte();
			break;
		case I0_ATTR_UF:
		case I0_ATTR_SF:
		case I0_ATTR_FS:
			value.uf = ua_next_long();
			break;
		case I0_ATTR_UE:
		case I0_ATTR_SE:
		case I0_ATTR_FD:
			value.ue = ua_next_qword();
			break;
		case I0_ATTR_US:
		case I0_ATTR_SS:
		default:
			throw "attr is not valid";
		}
	}
	virtual inline void Serialize(bool is_code_ref, op_t& operand)
	{
		I0OperandD::Serialize(is_code_ref, operand);
		operand.i0_op_spec_addrm = i0_addrm_Imm;
		if (is_code_ref)
		{
			operand.type = i0_o_dir_code;
			operand.addr = value.ue;
		}
		else
		{
			operand.type = i0_o_imm;
			operand.flags |= OF_NUMBER;
			operand.value = value.ue;
		}
	}
};

class I0OperandM : public I0OperandD{
public:
	inline I0OperandM(uint8 _Attr, I0_oper_types _OperType) : I0OperandD(_Attr, _OperType)
	{}
	static inline I0OperandM* Create(uint8 A, uint8 addrm);
	virtual ~I0OperandM() = 0
	{}
	virtual inline void Serialize(bool is_code_ref, op_t& operand)
	{
		I0OperandD::Serialize(is_code_ref, operand);
	}
};

class I0OperandMAbs : public I0OperandM{
public:
	uint64 addr;
	inline I0OperandMAbs(uint8 _Attr)
		: I0OperandM(_Attr, I0_oper_types::i0_opertype_MAbs), addr(ua_next_qword())
	{}
	virtual inline void Serialize(bool is_code_ref, op_t& operand)
	{
		I0OperandM::Serialize(is_code_ref, operand);
		if (trans_to_i0_reg(addr, operand.reg))
		{
			operand.type = i0_o_reg;
		}
		else
		{
			operand.addr = addr;
			operand.type = i0_o_dir;
		}
	}
};

class I0OperandMIndir : public I0OperandM{
public:
	uint64 addr;
	inline I0OperandMIndir(uint8 _Attr)
		: I0OperandM(_Attr, I0_oper_types::i0_opertype_MIndir), addr(ua_next_qword())
	{}
	virtual inline void Serialize(bool is_code_ref, op_t& operand)
	{
		I0OperandM::Serialize(is_code_ref, operand);
		if (trans_to_i0_reg(addr, operand.reg))
		{
			operand.type = i0_o_reg_indir;
		}
		else
		{
			operand.addr = addr;
			operand.type = i0_o_mem_indir;
		}
	}
};

class I0OperandMDisp : public I0OperandM{
public:
	int32 disp;
	uint64 addr;
	inline I0OperandMDisp(uint8 _Attr) 
		: I0OperandM(_Attr, I0_oper_types::i0_opertype_MDisp), disp(ua_next_long()), addr(ua_next_qword())
	{}
	virtual inline void Serialize(bool is_code_ref, op_t& operand)
	{
		I0OperandM::Serialize(is_code_ref, operand);
		operand.value = (int64)disp;
		if (trans_to_i0_reg(addr, operand.reg))
		{
			operand.type = i0_o_reg_displ;
		}
		else
		{
			operand.addr = addr;
			operand.type = i0_o_mem_displ;
		}
	}
};

inline I0OperandD* I0OperandD::Create(uint8 A, uint8 addrm)
{
	switch (addrm)
	{
	case I0_ADDRM_IMMEDIATE:
		return (new I0OperandI(A));
	case I0_ADDRM_ABSOLUTE:
		return (new I0OperandMAbs(A));
	case I0_ADDRM_INDIRECT:
		return (new I0OperandMIndir(A));
	case I0_ADDRM_DISPLACEMENT:
		return (new I0OperandMDisp(A));
	default:
		throw "addrm not implemented";
	}
}

inline I0OperandM* I0OperandM::Create(uint8 A, uint8 addrm)
{
	switch (addrm)
	{
	case I0_ADDRM_ABSOLUTE:
		return (new I0OperandMAbs(A));
	case I0_ADDRM_INDIRECT:
		return (new I0OperandMIndir(A));
	case I0_ADDRM_DISPLACEMENT:
		return (new I0OperandMDisp(A));
	default:
		throw "addrm is not valid";
	}
}

class I0Instruction{
public:
	const I0_ins_types type;
	uint64 addr;
protected:
	union Load{
		struct{
		uint32: 5;
			uint32 opcode : 11;
		};
		struct{
			uint8 byte1;
			uint8 byte0;
		};
		uint16 lowbytes;
	};
	inline I0Instruction(I0_ins_types _type, uint64 _addr) : type(_type), addr(_addr) {}
public:
	static inline I0Instruction* Create(uint64 _addr);
	virtual void Serialize() = 0;
	virtual ~I0Instruction() = 0
	{}
};

class I0NopInstruction : public I0Instruction{
public:
	inline I0NopInstruction(uint64 _addr)
		:I0Instruction(I0_ins_types::i0_instype_nop, _addr)
	{}
	virtual inline void Serialize()
	{
		cmd.itype = I0_ins_nop;
	}
};

class I0ConvInstruction : public I0Instruction{
private:
	union{
		struct{
		uint32: 7;
			uint32 addrm2 : 3;
			uint32 addrm1 : 3;
			uint32 A2 : 4;
			uint32 A1 : 4;
			uint32 opcode : 11;
		};
		struct{
			uint8 byte3;
			uint8 byte2;
			uint16 lowbytes;
		};
	}load;
	I0OperD src;
	I0OperM dest;
public:
	inline I0ConvInstruction(uint16 _lowbytes, uint64 _addr)
		:I0Instruction(I0_ins_types::i0_instype_conv, _addr)
	{
		load.lowbytes = _lowbytes;
		load.byte2 = ua_next_byte();
		load.byte3 = ua_next_byte();
		src.reset(I0OperandD::Create(load.A1, load.addrm1));
		dest.reset(I0OperandM::Create(load.A2, load.addrm2));
	}
	virtual inline void Serialize(){
		bool push_ret = false;
		if (src->Attr == dest->Attr){
			cmd.itype = I0_ins_mov;
			cmd.i0_ins_flags |= i0_ins_spec_attr_suffix;
			cmd.i0_ins_attr_pref = src->Attr;
			if (dest->OperType == I0_oper_types::i0_opertype_MDisp)
			{
				I0OperandMDisp* dest_disp = static_cast<I0OperandMDisp*>(dest.get());
				if (src->OperType == I0_oper_types::i0_opertype_I &&
					dest_disp->Attr == i0_attr_ue &&
					dest_disp->addr == I0_MEMSPACE_REGFILE_SP &&
					dest_disp->disp == 0)
				{
					push_ret = true;
				}
			}
		}
		else{
			cmd.itype = I0_ins_conv;
			cmd.i0_ins_flags |= i0_ins_spec_attr_each_opnd;
		}
		src->Serialize(push_ret, cmd.Op1);
		dest->Serialize(false, cmd.Op2);
	}
};

class I0ArithLogicInstruction : public I0Instruction{
private:
	i0_ins_names insname;
	union{
		struct {
			uint32 addrm3 : 3;
			uint32 addrm2 : 3;
			uint32 addrm1 : 3;
			uint32 A : 4;
			uint32 opcode : 11;
		};
		struct {
			uint8 byte2;
			uint16 lowbytes;
		};
	}load;
	I0OperD src1;
	I0OperD src2;
	I0OperM dest;
public:
	inline I0ArithLogicInstruction(uint16 _lowbytes, uint64 _addr, i0_ins_names _insname)
		:I0Instruction(I0_ins_types::i0_instype_arithlogic, _addr), insname(_insname)
	{
		load.lowbytes = _lowbytes;
		load.byte2 = ua_next_byte();
		src1.reset(I0OperandD::Create(load.A, load.addrm1));
		src2.reset(I0OperandD::Create(load.A, load.addrm2));
		dest.reset(I0OperandM::Create(load.A, load.addrm3));
	}
	virtual inline void Serialize(){
		cmd.itype = insname;
		cmd.i0_ins_flags |= i0_ins_spec_attr_suffix;
		cmd.i0_ins_attr_pref = dest->Attr;
		src1->Serialize(false, cmd.Op1);
		src2->Serialize(false, cmd.Op2);
		dest->Serialize(false, cmd.Op3);
	}
};

class I0SpawnInstruction : public I0Instruction{
private:
	union{
		struct{
		uint32: 1;
			uint32 addrm4 : 3;
			uint32 addrm3 : 3;
			uint32 addrm2 : 3;
			uint32 addrm1 : 3;
			uint32 opcode : 11;
		};
		struct{
			uint8 byte2;
			uint16 lowbytes;
		};
	}load;
	I0OperM stack;
	I0OperM use;
	I0OperM watch;
	I0OperM entry;
public:
	inline I0SpawnInstruction(uint16 _lowbytes, uint64 _addr)
		:I0Instruction(I0_ins_types::i0_instype_spawn, _addr)
	{
		load.lowbytes = _lowbytes;
		load.byte2 = ua_next_byte();
		stack.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm1));
		use.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm2));
		watch.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm3));
		entry.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm4));
	}
	virtual inline void Serialize(){
		cmd.itype = I0_ins_spawn;
		stack->Serialize(false, cmd.Op1);
		use->Serialize(false, cmd.Op2);
		watch->Serialize(false, cmd.Op3);
		entry->Serialize(false, cmd.Op4);
	}
};

class I0SpawnXInstruction : public I0Instruction{
private:
	union{
		struct{
		uint32: 6;
			uint32 addrm5 : 3;
			uint32 addrm4 : 3;
			uint32 addrm3 : 3;
			uint32 addrm2 : 3;
			uint32 addrm1 : 3;
			uint32 opcode : 11;
		};
		struct{
			uint8 byte3;
			uint8 byte2;
			uint16 lowbytes;
		};
	}load;
	I0OperM stack;
	I0OperM use;
	I0OperM watch;
	I0OperM entry;
	I0OperM space;
public:
	inline I0SpawnXInstruction(uint16 _lowbytes, uint64 _addr)
		:I0Instruction(I0_ins_types::i0_instype_spawnx, _addr)
	{
		load.lowbytes = _lowbytes;
		load.byte2 = ua_next_byte();
		load.byte3 = ua_next_byte();
		stack.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm1));
		use.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm2));
		watch.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm3));
		entry.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm4));
		space.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm5));
	}
	virtual inline void Serialize(){
		cmd.itype = I0_ins_spawnx;
		stack->Serialize(false, cmd.Op1);
		use->Serialize(false, cmd.Op2);
		watch->Serialize(false, cmd.Op3);
		entry->Serialize(false, cmd.Op4);
		space->Serialize(false, cmd.Op5);
	}
};

class I0ExitInstruciton : public I0Instruction{
private:
	union{
		struct{
		uint32: 3;
			uint32 option : 2;
			uint32 opcode : 11;
		};
		uint16 lowbytes;
	}load;
public:
	inline I0ExitInstruciton(uint16 _lowbytes, uint64 _addr)
		:I0Instruction(I0_ins_types::i0_instype_exit, _addr)
	{
		load.lowbytes = _lowbytes;
	}
	virtual inline void Serialize(){
		cmd.itype = I0_ins_exit;
		cmd.i0_ins_flags |= i0_ins_spec_option;
		switch (load.option)
		{
		case I0_OPT_EXIT_C:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_exit_c;
			break;
		case I0_OPT_EXIT_CD:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_exit_cd;
			break;
		case I0_OPT_EXIT_A:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_exit_a;
			break;
		case I0_OPT_EXIT_AD:
			cmd.i0_ins_opt_pref = i0_ins_opt_pref_exit_ad;
			break;
		}
	}
};

class I0BranchInstruction : public I0Instruction{
protected:
	union Load{
		struct{
		uint32: 1;
			uint32 mode : 4;
			uint32 opcode : 11;
		};
		uint16 lowbytes;
	};
public:
	inline I0BranchInstruction(I0_ins_types _type, uint64 _addr)
		:I0Instruction(_type, _addr)
	{}
	static inline I0BranchInstruction* Create(uint16 _lowbytes, uint64 _addr);
};

class I0BijInstruction : public I0BranchInstruction{
private:
	union{
		struct{
		uint32: 6;
			uint32 addrm : 3;
			uint32 mode : 4;
			uint32 opcode : 11;
		};
		struct{
			uint8 byte2;
			uint16 lowbytes;
		};
	}load;
	I0OperM target;
public:
	inline I0BijInstruction(uint16 _lowbytes, uint64 _addr)
		:I0BranchInstruction(I0_ins_types::i0_instype_bij, _addr)
	{
		load.lowbytes = _lowbytes;
		load.byte2 = ua_next_byte();
		target.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm));
	}
	virtual inline void Serialize(){
		cmd.itype = I0_ins_bij;
		target->Serialize(true, cmd.Op1);
	}
};

class I0BjInstruction : public I0BranchInstruction{
private:
	union{
		struct{
			uint32 ra : 1;
			uint32 mode : 4;
			uint32 opcode : 11;
		};
		uint16 lowbytes;
	}load;
	I0OperI target;
public:
	inline I0BjInstruction(uint16 _lowbytes, uint64 _addr)
		:I0BranchInstruction(I0_ins_types::i0_instype_bj, _addr)
	{
		load.lowbytes = _lowbytes;
		target.reset(I0OperandI::CreateBranchTarget(load.ra, addr));
	}
	virtual inline void Serialize(){
		cmd.itype = I0_ins_bj;
		target->Serialize(true, cmd.Op1);
	}
};

class I0BccInstruction : public I0BranchInstruction{
private:
	i0_ins_names insname;
	union{
		struct{
		uint32: 6;
			uint32 ra : 1;
			uint32 addrm2 : 3;
			uint32 addrm1 : 3;
			uint32 A : 4;
			uint32 mode : 4;
			uint32 opcode : 11;
		};
		struct{
			uint8 byte3;
			uint8 byte2;
			uint16 lowbytes;
		};
	}load;
	I0OperD op1;
	I0OperD op2;
	I0OperI target;
public:
	inline I0BccInstruction(uint16 _lowbytes, uint64 _addr, i0_ins_names _insname)
		:I0BranchInstruction(I0_ins_types::i0_instype_bcc, _addr), insname(_insname)
	{
		load.lowbytes = _lowbytes;
		load.byte2 = ua_next_byte();
		load.byte3 = ua_next_byte();
		op1.reset(I0OperandD::Create(load.A, load.addrm1));
		op2.reset(I0OperandD::Create(load.A, load.addrm2));
		target.reset(I0OperandI::CreateBranchTarget(load.ra, addr));
	}
	virtual inline void Serialize(){
		cmd.itype = insname;
		cmd.i0_ins_flags |= i0_ins_spec_attr_suffix;
		cmd.i0_ins_attr_pref = op1->Attr;
		op1->Serialize(false, cmd.Op1);
		op2->Serialize(false, cmd.Op2);
		target->Serialize(true, cmd.Op3);
	}
};

class I0BznzInstruction : public I0BranchInstruction{
private:
	i0_ins_names insname;
	union{
		struct{
		uint32:1;
			uint32 ra : 1;
			uint32 addrm1 : 3;
			uint32 A : 4;
			uint32 mode : 4;
			uint32 opcode : 11;
		};
		struct{
			uint8 byte2;
			uint16 lowbytes;
		};
	}load;
	I0OperD op;
	I0OperI target;
public:
	inline I0BznzInstruction(uint16 _lowbytes, uint64 _addr, i0_ins_names _insname)
		:I0BranchInstruction(I0_ins_types::i0_instype_bznz, _addr), insname(_insname)
	{
		load.lowbytes = _lowbytes;
		load.byte2 = ua_next_byte();
		op.reset(I0OperandD::Create(load.A, load.addrm1));
		target.reset(I0OperandI::CreateBranchTarget(load.ra, addr));
	}
	virtual inline void Serialize(){
		cmd.itype = insname;
		cmd.i0_ins_flags |= i0_ins_spec_attr_suffix;
		cmd.i0_ins_attr_pref = op->Attr;
		op->Serialize(false, cmd.Op1);
		target->Serialize(true, cmd.Op2);
	}
};

class I0IntInstruction : public I0Instruction{
private:
	I0OperI intno;
public:
	inline I0IntInstruction(uint64 _addr)
		:I0Instruction(I0_ins_types::i0_instype_int, _addr)
	{
		intno.reset(I0OperandI::Create(I0_ATTR_UB));
	}
	virtual inline void Serialize(){
		cmd.itype = I0_ins_int;
		intno->Serialize(false, cmd.Op1);
	}
};

class I0ShiftInstruction : public I0Instruction{
private:
	i0_ins_names insname;
	union{
		struct{
		uint32:6;
			uint32 addrm3 : 3;
			uint32 addrm2 : 3;
			uint32 addrm1 : 3;
			uint32 A : 4;
			uint32 option : 2;
			uint32 opcode : 11;
		};
		struct{
			uint8 byte3;
			uint8 byte2;
			uint16 lowbytes;
		};
	}load;
	I0OperD src1;
	I0OperI src2;
	I0OperM dest;
public:
	inline I0ShiftInstruction(uint16 _lowbytes, uint64 _addr)
		:I0Instruction(I0_ins_types::i0_instype_shift, _addr)
	{
		load.lowbytes = _lowbytes;
		load.byte2 = ua_next_byte();
		load.byte3 = ua_next_byte();
		switch (load.option)
		{
		case I0_OPT_SHIFT_L:
			insname = I0_ins_shl;
			break;
		case I0_OPT_SHIFT_R:
			insname = I0_ins_shr;
			break;
		default:
			throw "invalid shift option";
		}
		src1.reset(I0OperandD::Create(load.A, load.addrm1));
		src2.reset(I0OperandI::Create(I0_ATTR_UE)); //according to i0 spec
		dest.reset(I0OperandM::Create(load.A, load.addrm3));
	}
	virtual inline void Serialize(){
		cmd.itype = insname;
		cmd.i0_ins_flags |= i0_ins_spec_attr_suffix;
		cmd.i0_ins_attr_pref = src1->Attr;
		src1->Serialize(false, cmd.Op1);
		src2->Serialize(false, cmd.Op2);
		dest->Serialize(false, cmd.Op3);
	}
};

class I0StrInstruction : public I0Instruction{
private:
	i0_ins_names insname;
	union{
		struct{
		uint32:6;
			uint32 addrm5 : 3;
			uint32 addrm4 : 3;
			uint32 addrm3 : 3;
			uint32 addrm2 : 3;
			uint32 addrm1 : 3;
			uint32 opcode : 11;
		};
		struct{
			uint8 byte3;
			uint8 byte2;
			uint16 lowbytes;
		};
	}load;
	I0OperM str1;
	I0OperD len1;
	I0OperM str2;
	I0OperD len2;
	I0OperM dest;
public:
	inline I0StrInstruction(uint16 _lowbytes, uint64 _addr, i0_ins_names _insname)
		:I0Instruction(I0_ins_types::i0_instype_scmp, _addr), insname(_insname)
	{
		load.lowbytes = _lowbytes;
		load.byte2 = ua_next_byte();
		load.byte3 = ua_next_byte();
		str1.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm1));
		len1.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm2));
		str2.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm3));
		len2.reset(I0OperandM::Create(I0_ATTR_UE, load.addrm4));
		dest.reset(I0OperandM::Create(I0_ATTR_SE, load.addrm5));
	}
	virtual inline void Serialize(){
		cmd.itype = insname;
		str1->Serialize(false, cmd.Op1);
		len1->Serialize(false, cmd.Op2);
		str2->Serialize(false, cmd.Op3);
		len2->Serialize(false, cmd.Op4);
		dest->Serialize(false, cmd.Op5);
	}
};

inline I0BranchInstruction* I0BranchInstruction::Create(uint16 _lowbytes, uint64 _addr)
{
	Load load;
	load.lowbytes = _lowbytes;
	switch (load.mode)
	{
	case I0_OPT_B_IJ:
		return new I0BijInstruction(_lowbytes, _addr);
	case I0_OPT_B_J:
		return new I0BjInstruction(_lowbytes, _addr);
	case I0_OPT_B_LE:
		return new I0BccInstruction(_lowbytes, _addr, I0_ins_ble);
	case I0_OPT_B_E:
		return new I0BccInstruction(_lowbytes, _addr, I0_ins_be);
	case I0_OPT_B_L:
		return new I0BccInstruction(_lowbytes, _addr, I0_ins_bl);
	case I0_OPT_B_NE:
		return new I0BccInstruction(_lowbytes, _addr, I0_ins_bne);
	case I0_OPT_B_SL:
		return new I0BccInstruction(_lowbytes, _addr, I0_ins_bsl);
	case I0_OPT_B_Z:
		return new I0BznzInstruction(_lowbytes, _addr, I0_ins_bz);
	case I0_OPT_B_NZ:
		return new I0BznzInstruction(_lowbytes, _addr, I0_ins_bnz);
	default:
		throw "branch type not supported";
	}
}

inline I0Instruction* I0Instruction::Create(uint64 _addr)
{
	Load load;
	load.byte0 = ua_next_byte();
	load.byte1 = ua_next_byte();
	switch (load.opcode)
	{
	case I0_OPCODE_ADD:
		return new I0ArithLogicInstruction(load.lowbytes, _addr, I0_ins_add);
	case I0_OPCODE_SUB:
		return new I0ArithLogicInstruction(load.lowbytes, _addr, I0_ins_sub);
	case I0_OPCODE_MUL:
		return new I0ArithLogicInstruction(load.lowbytes, _addr, I0_ins_mul);
	case I0_OPCODE_DIV:
		return new I0ArithLogicInstruction(load.lowbytes, _addr, I0_ins_div);
	case I0_OPCODE_OR:
		return new I0ArithLogicInstruction(load.lowbytes, _addr, I0_ins_or);
	case I0_OPCODE_XOR:
		return new I0ArithLogicInstruction(load.lowbytes, _addr, I0_ins_xor);
	case I0_OPCODE_AND:
		return new I0ArithLogicInstruction(load.lowbytes, _addr, I0_ins_and);
	case I0_OPCODE_CONV:
		return new I0ConvInstruction(load.lowbytes, _addr);
	case I0_OPCODE_SPAWN:
		return new I0SpawnInstruction(load.lowbytes, _addr);
	case I0_OPCODE_SPAWNX:
		return new I0SpawnXInstruction(load.lowbytes, _addr);
	case I0_OPCODE_NOP:
		return new I0NopInstruction(_addr);
	case I0_OPCODE_EXIT:
		return new I0ExitInstruciton(load.lowbytes, _addr);
	case I0_OPCODE_B:
		return I0BranchInstruction::Create(load.lowbytes, _addr);
	case I0_OPCODE_INT:
		return new I0IntInstruction(_addr);
	case I0_OPCODE_SHIFT:
		return new I0ShiftInstruction(load.lowbytes, _addr);
	case I0_OPCODE_SCMP:
		return new I0StrInstruction(load.lowbytes, _addr, I0_ins_scmp);
	case I0_OPCODE_GREP:
		return new I0StrInstruction(load.lowbytes, _addr, I0_ins_grep);
	default:
		throw "invalid instruction";
	}
}

#pragma pack(pop)
#pragma warning(pop)

