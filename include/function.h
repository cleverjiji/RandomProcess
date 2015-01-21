#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "type.h"
#include "utility.h"
#include "instruction.h"
#include "code_segment.h"
#include <vector>
using namespace std;


class Function
{
private:
	CodeSegment *_code_segment;
	string _function_name;
	//function in other process
	ORIGIN_ADDR _origin_function_base;
	ORIGIN_SIZE _origin_function_size;
	//function in share code region
	ADDR _function_base;
	SIZE _function_size;
	//random function in code_cache
	CODE_CACHE_ADDR _random_function_base;
	CODE_CACHE_SIZE _random_function_size;
	//instruction list
	vector<Instruction*> _origin_function_instructions;
	vector<Instruction*> _random_function_instructions;
public:
	Function(CodeSegment *code_segment, string name, ORIGIN_ADDR origin_function_base, ORIGIN_SIZE origin_function_size);
	void point_to_random_function();
	void disassemble();
	virtual void random_function();
};

#endif
