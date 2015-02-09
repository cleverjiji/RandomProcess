#ifndef _BASICBLOCK_H_
#define _BASICBLOCK_H_

#include "instruction.h"
#include <vector>
using namespace std;

class BasicBlock
{
private:
	vector<Instruction *> instruction_vec;
	vector<BasicBlock *> prev_bb_vec;
	BasicBlock *fallthrough_bb;
	vector<BasicBlock *> target_bb_vec;
	CODE_CACHE_ADDR _curr_copy_addr;
	ORIGIN_ADDR _origin_copy_addr;
	SIZE _generate_copy_size;
public:
	BasicBlock():fallthrough_bb(NULL), _curr_copy_addr(0), _origin_copy_addr(0), _generate_copy_size(0)
	{
		;		
	}
	virtual ~BasicBlock()
	{
		;
	}
	void insert_instruction(Instruction *inst)
	{
		instruction_vec.push_back(inst);
	}
	Instruction *get_first_instruction()
	{
		ASSERT(!instruction_vec.empty());
		return instruction_vec.front();
	}
	Instruction *get_last_instruction()
	{
		ASSERT(!instruction_vec.empty());
		return instruction_vec.back();
	}
	void add_prev_bb(BasicBlock *prev_bb)
	{
		prev_bb_vec.push_back(prev_bb);
	}
	void add_target_bb(BasicBlock *target_bb)
	{
		target_bb_vec.push_back(target_bb);
	}
	void add_fallthrough_bb(BasicBlock *fall_bb)
	{
		fallthrough_bb = fall_bb;
	}
	SIZE copy_instructions(CODE_CACHE_ADDR curr_target_addr, ORIGIN_ADDR origin_target_addr);
	void dump();
};


#endif
