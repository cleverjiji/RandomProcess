#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include "disasm_common.h"
#include "type.h"
#include "utility.h"

class Instruction
{
public:
	typedef enum{
		EMPTY = 0,
		NONE_TYPE, /*Indicates the instruction is not the flow-control instruction*/
		CND_BRANCH_TYPE,/* Indicates the instruction is one of: JCXZ, JO -- JG, LOOP, LOOPZ, LOOPNZ. */
		DIRECT_CALL_TYPE,/* Indicates the instruction is one of: CALL Relative */
		DIRECT_JMP_TYPE,/* Indicates the instruction is one of: JMP Relative. */
		INDIRECT_CALL_TYPE,/* Indicates the instruction is one of: CALLIN CALL FAR */
		INDIRECT_JMP_TYPE,/*Indicates the instruction is one of: JMPIN */
		RET_TYPE,
	}INSTRUCTION_TYPE;
	static const char * type_to_string[];
private:
	INSTRUCTION_TYPE inst_type;
	ORIGIN_ADDR _origin_instruction_addr;
	ADDR _current_instruction_addr;
	_DInst _dInst;
	_DecodedInst _decodedInst;
	BOOL is_already_disasm;
	SIZE security_size;
	CODE_CACHE_ADDR _curr_copy_addr;
	ORIGIN_ADDR _origin_copy_addr;
	SIZE _inst_copy_size;
public:
	Instruction(ORIGIN_ADDR origin_addr, ADDR current_addr, SIZE instruction_max_size);
	~Instruction(){;}
	UINT8 char_to_encode(char c)
	{
		if((c>='0')&&(c<='9')){
			return c-'0'+0x00;
		}else if((c>='a')&&(c<='f'))
			return c-'a'+0x0a;
		else{
			ASSERT(0);
			return 0;
		}
	}
	void get_inst_code(UINT8 *encode, SIZE size)
	{
		ASSERT(is_already_disasm && size==_dInst.size);
		UINT32 idx = 0;
		while(_decodedInst.instructionHex.p[idx]!='\0'){
			ASSERT(_decodedInst.instructionHex.p[idx+1]!='\0');
			encode[idx/2] = char_to_encode(_decodedInst.instructionHex.p[idx])*16+char_to_encode(_decodedInst.instructionHex.p[idx+1]);
			idx+=2;
		}
		ASSERT(idx/2==size);
		return  ;
	}
	SIZE get_inst_size()
	{
		ASSERT(is_already_disasm);
		return _dInst.size;
	}
	
	const char *get_inst_type()
	{
		ASSERT(is_already_disasm);
		return type_to_string[inst_type];
	}
	ORIGIN_ADDR get_inst_origin_addr()
	{
		return _origin_instruction_addr;
	}
	BOOL isIndirectCall()
	{
		ASSERT(is_already_disasm);
		return inst_type==INDIRECT_CALL_TYPE;
	}
	BOOL isIndirectJmp()
	{
		ASSERT(is_already_disasm);
		return inst_type==INDIRECT_JMP_TYPE;
	}
	BOOL isDirectCall()
	{
		ASSERT(is_already_disasm);
		return inst_type==DIRECT_CALL_TYPE;
	}
	BOOL isDirectJmp()
	{
		ASSERT(is_already_disasm);
		return inst_type==DIRECT_JMP_TYPE;
	}
	BOOL isConditionBranch()
	{
		ASSERT(is_already_disasm);
		return inst_type==CND_BRANCH_TYPE;
	}
	BOOL isUnConditionBranch()
	{
		ASSERT(is_already_disasm);
		return inst_type==DIRECT_JMP_TYPE || inst_type==INDIRECT_JMP_TYPE;
	}
	BOOL isBranch()
	{
		ASSERT(is_already_disasm);
		return inst_type!=NONE_TYPE;
	}
	BOOL isRet()
	{
		ASSERT(is_already_disasm);
		return inst_type==RET_TYPE;
	}
	ORIGIN_ADDR getBranchTargetOrigin()
	{
		ASSERT(is_already_disasm && (inst_type==DIRECT_JMP_TYPE || inst_type==CND_BRANCH_TYPE || inst_type==DIRECT_CALL_TYPE));
		return INSTRUCTION_GET_TARGET(&_dInst) + _origin_instruction_addr;
	}
	ORIGIN_ADDR getBranchFallthroughOrigin()
	{
		ASSERT(is_already_disasm && inst_type!=NONE_TYPE && inst_type!=DIRECT_JMP_TYPE && inst_type!=INDIRECT_JMP_TYPE);
		return _origin_instruction_addr + _dInst.size;
	}
	ADDR getBranchFallthroughCurrent()
	{
		ASSERT(is_already_disasm && inst_type!=NONE_TYPE && inst_type!=DIRECT_JMP_TYPE && inst_type!=INDIRECT_JMP_TYPE);
		return _current_instruction_addr + _dInst.size;
	}
	SIZE disassemable();
	SIZE copy_instruction(CODE_CACHE_ADDR curr_copy_addr, ORIGIN_ADDR origin_copy_addr);
	void init_instruction_type();
	void dump();
};

#endif
