#include <memory>
#include <algorithm>

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
public:
	inline I0OperandD(uint64 _Addr, uint8 _Attr, I0_oper_types _OperType)
		: OperType(_OperType), Addr(_Addr), Attr(static_cast<i0_oper_attr>(_Attr))
	{}
	const I0_oper_types OperType;
	uint64 Addr;
	i0_oper_attr Attr;
	static inline I0OperandD* Create(uint64& addr, uint8 A, uint8 addrm);
	virtual ~I0OperandD() = 0
	{}
	virtual uint16 Size() const = 0;
	virtual inline void Serialize(bool, op_t& operand, insn_t& ins)
	{
		operand.offb = (uint8)(Addr - ins.ea);
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
	inline static I0OperandI* Create(uint64& addr, uint8 A)
	{
		return static_cast<I0OperandI*>(I0OperandD::Create(addr, A, I0_ADDRM_IMMEDIATE));
	}
	inline static I0OperandI* CreateBranchTarget(uint64& addr, uint32 ra)
	{
		I0OperandI* ret = Create(addr, I0_ATTR_UE);
		if (ra == I0_OPT_JUMP_R)
		{
			ret->value.ue += addr;
		}
		return ret;
	}
	inline I0OperandI(uint64 _Addr, uint8 _Attr) : I0OperandD(_Addr, _Attr, I0_oper_types::i0_opertype_I)
	{
		switch (Attr)
		{
		case I0_ATTR_UB:
		case I0_ATTR_SB:
			value.ub = get_byte(Addr);
			break;
		case I0_ATTR_UF:
		case I0_ATTR_SF:
		case I0_ATTR_FS:
			value.uf = get_long(Addr);
			break;
		case I0_ATTR_UE:
		case I0_ATTR_SE:
		case I0_ATTR_FD:
			value.ue = get_qword(Addr);
			break;
		case I0_ATTR_US:
		case I0_ATTR_SS:
		default:
			throw "attr is not valid";
		}
	}
	virtual inline uint16 Size() const
	{
		return i0_attr_byte_len[Attr];
	}
	virtual inline void Serialize(bool is_code_ref, op_t& operand, insn_t& ins)
	{
		I0OperandD::Serialize(is_code_ref, operand, ins);
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
	inline I0OperandM(uint64 _Addr, uint8 _Attr, I0_oper_types _OperType) : I0OperandD(_Addr, _Attr, _OperType)
	{}
	static inline I0OperandM* Create(uint64& _addr, uint8 A, uint8 addrm);
	virtual ~I0OperandM() = 0
	{}
	virtual inline void Serialize(bool is_code_ref, op_t& operand, insn_t& ins)
	{
		I0OperandD::Serialize(is_code_ref, operand, ins);
	}
};

class I0OperandMAbs : public I0OperandM{
public:
	uint64 M;
	inline I0OperandMAbs(uint64 _Addr, uint8 _Attr)
		: I0OperandM(_Addr, _Attr, I0_oper_types::i0_opertype_MAbs)
	{
		M = get_qword(Addr);
	}
	virtual inline void Serialize(bool is_code_ref, op_t& operand, insn_t& ins)
	{
		I0OperandM::Serialize(is_code_ref, operand, ins);
		if (trans_to_i0_reg(M, operand.reg))
		{
			operand.type = i0_o_reg;
		}
		else
		{
			operand.addr = M;
			operand.type = i0_o_dir;
		}
	}
	virtual inline uint16 Size() const{
		return sizeof(uint64);
	}
};

class I0OperandMIndir : public I0OperandM{
public:
	uint64 M;
	inline I0OperandMIndir(uint64 _Addr, uint8 _Attr)
		: I0OperandM(_Addr, _Attr, I0_oper_types::i0_opertype_MIndir)
	{
		M = get_qword(Addr);
	}
	virtual inline void Serialize(bool is_code_ref, op_t& operand, insn_t& ins)
	{
		I0OperandM::Serialize(is_code_ref, operand, ins);
		if (trans_to_i0_reg(M, operand.reg))
		{
			operand.type = i0_o_reg_indir;
		}
		else
		{
			operand.addr = M;
			operand.type = i0_o_mem_indir;
		}
	}
	virtual inline uint16 Size() const{
		return sizeof(uint64);
	}
};

class I0OperandMDisp : public I0OperandM{
public:
	int32 Disp;
	uint64 M;
	inline I0OperandMDisp(uint64 _Addr, uint8 _Attr)
		: I0OperandM(_Addr, _Attr, I0_oper_types::i0_opertype_MDisp)
	{
		Disp = get_long(Addr);
		M = get_qword(Addr);
	}
	virtual inline void Serialize(bool is_code_ref, op_t& operand, insn_t& ins)
	{
		I0OperandM::Serialize(is_code_ref, operand, ins);
		operand.value = (int64)Disp;
		if (trans_to_i0_reg(M, operand.reg))
		{
			operand.type = i0_o_reg_displ;
		}
		else
		{
			operand.addr = M;
			operand.type = i0_o_mem_displ;
		}
	}
	virtual inline uint16 Size() const{
		return sizeof(uint64) + sizeof(int32);
	}
};

inline I0OperandD* I0OperandD::Create(uint64& addr, uint8 A, uint8 addrm)
{
	I0OperandD* ret;
	switch (addrm)
	{
	case I0_ADDRM_IMMEDIATE:
		ret = (new I0OperandI(addr, A));
		break;
	case I0_ADDRM_ABSOLUTE:
		ret = (new I0OperandMAbs(addr, A));
		break;
	case I0_ADDRM_INDIRECT:
		ret = (new I0OperandMIndir(addr, A));
		break;
	case I0_ADDRM_DISPLACEMENT:
		ret = (new I0OperandMDisp(addr, A));
		break;
	default:
		throw "addrm not implemented";
	}
	addr += ret->Size();
	return ret;
}

inline I0OperandM* I0OperandM::Create(uint64& addr, uint8 A, uint8 addrm)
{
	I0OperandM* ret;
	switch (addrm)
	{
	case I0_ADDRM_ABSOLUTE:
		ret = (new I0OperandMAbs(addr, A));
		break;
	case I0_ADDRM_INDIRECT:
		ret = (new I0OperandMIndir(addr, A));
		break;
	case I0_ADDRM_DISPLACEMENT:
		ret = (new I0OperandMDisp(addr, A));
		break;
	default:
		throw "addrm is not valid";
	}
	addr += ret->Size();
	return ret;
}

template<size_t N>
void ua_i0_ins_bytes(uint8(&arr)[N], uint64& _Addr)
{
	std::generate(
		std::reverse_iterator<uint8*>(arr + N - 1),
		std::reverse_iterator<uint8*>(arr - 1),
		[&]{return get_byte(_Addr++); }
	);
}

template<size_t N>
void get_i0_ins_bytes(uint8(&arr)[N], uint64 _Addr)
{
	std::generate(
		std::reverse_iterator<uint8*>(arr + N - 1),
		std::reverse_iterator<uint8*>(arr - 1),
		[&]{return get_byte(_Addr++); }
	);
}

class I0Instruction{
public:
	const I0_ins_types Type;
	uint64 Addr;
protected:
	union Load{
		struct{
		uint32: 5;
			uint32 opcode : 11;
		};
		uint8 bytes[I0_INS_LEN_OPCODE];
	};
	inline I0Instruction(I0_ins_types _Type, uint64 _Addr) : Type(_Type), Addr(_Addr) 
	{}
public:
	static inline I0Instruction* Create(uint64& addr);
	void Serialize(insn_t& ins)
	{
		ins.ea = Addr;
		_Serialize(ins);
	}
	virtual void _Serialize(insn_t& ins) = 0;
	virtual uint16 Size() const = 0;
	virtual ~I0Instruction() = 0
	{}
};

class I0NopInstruction : public I0Instruction{
public:
	inline I0NopInstruction(uint64& _Addr)
		:I0Instruction(I0_ins_types::i0_instype_nop, _Addr)
	{}
	virtual inline void _Serialize(insn_t& ins)
	{
		ins.itype = I0_ins_nop;
	}
	virtual inline uint16 Size() const{
		return I0_INS_LEN_NOP;
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
		uint8 bytes[I0_INS_LEN_CONV];
	}load;
	I0OperD src;
	I0OperM dest;
public:
	inline I0ConvInstruction(uint64& _Addr)
		:I0Instruction(I0_ins_types::i0_instype_conv, _Addr)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
		src.reset(I0OperandD::Create(_Addr, load.A1, load.addrm1));
		dest.reset(I0OperandM::Create(_Addr, load.A2, load.addrm2));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_CONV + src->Size() + dest->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		bool push_ret = false;
		if (src->Attr == dest->Attr){
			ins.itype = I0_ins_mov;
			ins.i0_ins_flags |= i0_ins_spec_attr_suffix;
			ins.i0_ins_attr_pref = src->Attr;
			if (dest->OperType == I0_oper_types::i0_opertype_MDisp)
			{
				I0OperandMDisp* dest_disp = static_cast<I0OperandMDisp*>(dest.get());
				if (src->OperType == I0_oper_types::i0_opertype_I &&
					dest_disp->Attr == i0_attr_se &&
					dest_disp->Addr == I0_MEMSPACE_REGFILE_SP &&
					dest_disp->Disp == 0)
				{
					push_ret = true;
				}
			}
		}
		else{
			ins.itype = I0_ins_conv;
			ins.i0_ins_flags |= i0_ins_spec_attr_each_opnd;
		}
		src->Serialize(push_ret, ins.Op1, ins);
		dest->Serialize(false, ins.Op2, ins);
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
		uint8 bytes[I0_INS_LEN_ALU];
	}load;
	I0OperD src1;
	I0OperD src2;
	I0OperM dest;
public:
	inline I0ArithLogicInstruction(uint64& _Addr, i0_ins_names _insname)
		:I0Instruction(I0_ins_types::i0_instype_arithlogic, _Addr), insname(_insname)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
		src1.reset(I0OperandD::Create(_Addr, load.A, load.addrm1));
		src2.reset(I0OperandD::Create(_Addr, load.A, load.addrm2));
		dest.reset(I0OperandM::Create(_Addr, load.A, load.addrm3));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_ALU + src1->Size() + src2->Size() + dest->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = insname;
		ins.i0_ins_flags |= i0_ins_spec_attr_suffix;
		ins.i0_ins_attr_pref = dest->Attr;
		src1->Serialize(false, ins.Op1, ins);
		src2->Serialize(false, ins.Op2, ins);
		dest->Serialize(false, ins.Op3, ins);
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
		uint8 bytes[I0_INS_LEN_SPAWN];
	}load;
	I0OperM stack;
	I0OperM use;
	I0OperM watch;
	I0OperM entry;
public:
	inline I0SpawnInstruction(uint64& _Addr)
		:I0Instruction(I0_ins_types::i0_instype_spawn, _Addr)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
		stack.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm1));
		use.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm2));
		watch.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm3));
		entry.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm4));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_SPAWN + stack->Size() + use->Size() + watch->Size() + entry->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = I0_ins_spawn;
		stack->Serialize(false, ins.Op1, ins);
		use->Serialize(false, ins.Op2, ins);
		watch->Serialize(false, ins.Op3, ins);
		entry->Serialize(false, ins.Op4, ins);
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
		uint8 bytes[I0_INS_LEN_SPAWNX];
	}load;
	I0OperM stack;
	I0OperM use;
	I0OperM watch;
	I0OperM entry;
	I0OperM space;
public:
	inline I0SpawnXInstruction(uint64& _Addr)
		:I0Instruction(I0_ins_types::i0_instype_spawnx, _Addr)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
		stack.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm1));
		use.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm2));
		watch.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm3));
		entry.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm4));
		space.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm5));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_SPAWNX + stack->Size() + use->Size() + watch->Size() + entry->Size() + space->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = I0_ins_spawnx;
		stack->Serialize(false, ins.Op1, ins);
		use->Serialize(false, ins.Op2, ins);
		watch->Serialize(false, ins.Op3, ins);
		entry->Serialize(false, ins.Op4, ins);
		space->Serialize(false, ins.Op5, ins);
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
		uint8 bytes[I0_INS_LEN_EXIT];
	}load;
public:
	inline I0ExitInstruciton(uint64& _Addr)
		:I0Instruction(I0_ins_types::i0_instype_exit, _Addr)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_EXIT;
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = I0_ins_exit;
		ins.i0_ins_flags |= i0_ins_spec_option;
		switch (load.option)
		{
		case I0_OPT_EXIT_C:
			ins.i0_ins_opt_pref = i0_ins_opt_pref_exit_c;
			break;
		case I0_OPT_EXIT_CD:
			ins.i0_ins_opt_pref = i0_ins_opt_pref_exit_cd;
			break;
		case I0_OPT_EXIT_A:
			ins.i0_ins_opt_pref = i0_ins_opt_pref_exit_a;
			break;
		case I0_OPT_EXIT_AD:
			ins.i0_ins_opt_pref = i0_ins_opt_pref_exit_ad;
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
		uint8 bytes[I0_INS_LEN_BOPCODE];
	};
public:
	inline I0BranchInstruction(I0_ins_types _type, uint64& _Addr)
		:I0Instruction(_type, _Addr)
	{}
	static inline I0BranchInstruction* Create(uint64& _Addr);
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
		uint8 bytes[I0_INS_LEN_BIJ];
	}load;
	I0OperM target;
public:
	inline I0BijInstruction(uint64& _Addr)
		:I0BranchInstruction(I0_ins_types::i0_instype_bij, _Addr)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
		target.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_BIJ + target->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = I0_ins_bij;
		target->Serialize(true, ins.Op1, ins);
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
		uint8 bytes[I0_INS_LEN_BJ];
	}load;
	I0OperI target;
public:
	inline I0BjInstruction(uint64& _Addr)
		:I0BranchInstruction(I0_ins_types::i0_instype_bj, _Addr)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
		target.reset(I0OperandI::CreateBranchTarget(_Addr, load.ra));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_BJ + target->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = I0_ins_bj;
		target->Serialize(true, ins.Op1, ins);
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
		uint8 bytes[I0_INS_LEN_BCMP];
	}load;
	I0OperD op1;
	I0OperD op2;
	I0OperI target;
public:
	inline I0BccInstruction(uint64& _Addr, i0_ins_names _insname)
		:I0BranchInstruction(I0_ins_types::i0_instype_bcc, _Addr), insname(_insname)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
		op1.reset(I0OperandD::Create(_Addr, load.A, load.addrm1));
		op2.reset(I0OperandD::Create(_Addr, load.A, load.addrm2));
		target.reset(I0OperandI::CreateBranchTarget(_Addr, load.ra));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_BCMP + op1->Size() + op2->Size() + target->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = insname;
		ins.i0_ins_flags |= i0_ins_spec_attr_suffix;
		ins.i0_ins_attr_pref = op1->Attr;
		op1->Serialize(false, ins.Op1, ins);
		op2->Serialize(false, ins.Op2, ins);
		target->Serialize(true, ins.Op3, ins);
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
		uint8 bytes[I0_INS_LEN_BZNZ];
	}load;
	I0OperD op;
	I0OperI target;
public:
	inline I0BznzInstruction(uint64& _Addr, i0_ins_names _insname)
		:I0BranchInstruction(I0_ins_types::i0_instype_bznz, _Addr), insname(_insname)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
		op.reset(I0OperandD::Create(_Addr, load.A, load.addrm1));
		target.reset(I0OperandI::CreateBranchTarget(_Addr, load.ra));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_BCMP + op->Size() + target->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = insname;
		ins.i0_ins_flags |= i0_ins_spec_attr_suffix;
		ins.i0_ins_attr_pref = op->Attr;
		op->Serialize(false, ins.Op1, ins);
		target->Serialize(true, ins.Op2, ins);
	}
};

class I0IntInstruction : public I0Instruction{
private:
	I0OperI intno;
public:
	inline I0IntInstruction(uint64& _Addr)
		:I0Instruction(I0_ins_types::i0_instype_int, _Addr)
	{
		_Addr += 2;
		intno.reset(I0OperandI::Create(_Addr, I0_ATTR_UB));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_INT + intno->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = I0_ins_int;
		intno->Serialize(false, ins.Op1, ins);
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
		uint8 bytes[I0_INS_LEN_SHIFT];
	}load;
	I0OperD src1;
	I0OperI src2;
	I0OperM dest;
public:
	inline I0ShiftInstruction(uint64& _Addr)
		:I0Instruction(I0_ins_types::i0_instype_shift, _Addr)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
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
		src1.reset(I0OperandD::Create(_Addr, load.A, load.addrm1));
		src2.reset(I0OperandI::Create(_Addr, I0_ATTR_UE)); //according to i0 spec
		dest.reset(I0OperandM::Create(_Addr, load.A, load.addrm3));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_SHIFT + src1->Size() + src2->Size() + dest->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = insname;
		ins.i0_ins_flags |= i0_ins_spec_attr_suffix;
		ins.i0_ins_attr_pref = src1->Attr;
		src1->Serialize(false, ins.Op1, ins);
		src2->Serialize(false, ins.Op2, ins);
		dest->Serialize(false, ins.Op3, ins);
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
		uint8 bytes[I0_INS_LEN_STR];
	}load;
	I0OperM str1;
	I0OperD len1;
	I0OperM str2;
	I0OperD len2;
	I0OperM dest;
public:
	inline I0StrInstruction(uint64& _Addr, i0_ins_names _insname)
		:I0Instruction(I0_ins_types::i0_instype_scmp, _Addr), insname(_insname)
	{
		ua_i0_ins_bytes(load.bytes, _Addr);
		str1.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm1));
		len1.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm2));
		str2.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm3));
		len2.reset(I0OperandM::Create(_Addr, I0_ATTR_UE, load.addrm4));
		dest.reset(I0OperandM::Create(_Addr, I0_ATTR_SE, load.addrm5));
	}
	virtual inline uint16 Size() const
	{
		return I0_INS_LEN_STR + str1->Size() + len1->Size() + str2->Size() + len2->Size() + dest->Size();
	}
	virtual inline void _Serialize(insn_t& ins){
		ins.itype = insname;
		str1->Serialize(false, ins.Op1, ins);
		len1->Serialize(false, ins.Op2, ins);
		str2->Serialize(false, ins.Op3, ins);
		len2->Serialize(false, ins.Op4, ins);
		dest->Serialize(false, ins.Op5, ins);
	}
};

inline I0BranchInstruction* I0BranchInstruction::Create(uint64& _Addr)
{
	Load load;
	get_i0_ins_bytes(load.bytes, _Addr);
	switch (load.mode)
	{
	case I0_OPT_B_IJ:
		return new I0BijInstruction(_Addr);
	case I0_OPT_B_J:
		return new I0BjInstruction(_Addr);
	case I0_OPT_B_LE:
		return new I0BccInstruction(_Addr, I0_ins_ble);
	case I0_OPT_B_E:
		return new I0BccInstruction(_Addr, I0_ins_be);
	case I0_OPT_B_L:
		return new I0BccInstruction(_Addr, I0_ins_bl);
	case I0_OPT_B_NE:
		return new I0BccInstruction(_Addr, I0_ins_bne);
	case I0_OPT_B_SL:
		return new I0BccInstruction(_Addr, I0_ins_bsl);
	case I0_OPT_B_Z:
		return new I0BznzInstruction(_Addr, I0_ins_bz);
	case I0_OPT_B_NZ:
		return new I0BznzInstruction(_Addr, I0_ins_bnz);
	default:
		throw "branch type not supported";
	}
}

inline I0Instruction* I0Instruction::Create(uint64& _Addr)
{
	Load load;
	get_i0_ins_bytes(load.bytes, _Addr);
	switch (load.opcode)
	{
	case I0_OPCODE_ADD:
		return new I0ArithLogicInstruction(_Addr, I0_ins_add);
	case I0_OPCODE_SUB:
		return new I0ArithLogicInstruction(_Addr, I0_ins_sub);
	case I0_OPCODE_MUL:
		return new I0ArithLogicInstruction(_Addr, I0_ins_mul);
	case I0_OPCODE_DIV:
		return new I0ArithLogicInstruction(_Addr, I0_ins_div);
	case I0_OPCODE_OR:
		return new I0ArithLogicInstruction(_Addr, I0_ins_or);
	case I0_OPCODE_XOR:
		return new I0ArithLogicInstruction(_Addr, I0_ins_xor);
	case I0_OPCODE_AND:
		return new I0ArithLogicInstruction(_Addr, I0_ins_and);
	case I0_OPCODE_CONV:
		return new I0ConvInstruction(_Addr);
	case I0_OPCODE_SPAWN:
		return new I0SpawnInstruction(_Addr);
	case I0_OPCODE_SPAWNX:
		return new I0SpawnXInstruction(_Addr);
	case I0_OPCODE_NOP:
		return new I0NopInstruction(_Addr);
	case I0_OPCODE_EXIT:
		return new I0ExitInstruciton(_Addr);
	case I0_OPCODE_B:
		return I0BranchInstruction::Create(_Addr);
	case I0_OPCODE_INT:
		return new I0IntInstruction(_Addr);
	case I0_OPCODE_SHIFT:
		return new I0ShiftInstruction(_Addr);
	case I0_OPCODE_SCMP:
		return new I0StrInstruction(_Addr, I0_ins_scmp);
	case I0_OPCODE_GREP:
		return new I0StrInstruction(_Addr, I0_ins_grep);
	default:
		throw "invalid instruction";
	}
}

#pragma pack(pop)
#pragma warning(pop)

