#include "function.h"


Function::Function(CodeSegment *code_segment, string name, ORIGIN_ADDR origin_function_base, ORIGIN_SIZE origin_function_size)
	:_code_segment(code_segment), _function_name(name), _origin_function_base(origin_function_base), _origin_function_size(origin_function_size)
	, _random_cc_start(0), _random_cc_origin_start(0), _random_cc_size(0), is_already_disasm(false), is_already_split_into_bb(false)
{
	_function_base = code_segment->convert_origin_process_addr_to_this(_origin_function_base);
	_function_size = (SIZE)_origin_function_size;
}
Function::~Function(){
	;
}
void Function::dump_function_origin()
{
	PRINT("[0x%lx-0x%lx]",  _origin_function_base, _origin_function_size+_origin_function_base);
	INFO("%s",_function_name.c_str());
	PRINT("(Path:%s)\n", _code_segment->file_path.c_str());
	if(is_already_disasm){
		for(vector<Instruction*>::iterator iter = _origin_function_instructions.begin(); iter!=_origin_function_instructions.end(); iter++){
			(*iter)->dump();
		}
	}else
		ERR("Do not disasm!\n");
}
void Function::dump_bb_origin()
{
	PRINT("[0x%lx-0x%lx]",  _origin_function_base, _origin_function_size+_origin_function_base);
	INFO("%s",_function_name.c_str());
	PRINT("(Path:%s)\n", _code_segment->file_path.c_str());
	if(is_already_split_into_bb){
		for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
			(*ite)->dump();
		}
	}else
		ERR("Do not split into BB!\n");
}
void Function::disassemble()
{
	ORIGIN_ADDR origin_addr = _origin_function_base;
	ADDR current_addr = _function_base;
	SIZE security_size = _origin_function_size+1;//To see one more byte

	while(security_size>1){
		Instruction *instr = new Instruction(origin_addr, current_addr, security_size);
		_map_origin_addr_to_inst.insert(make_pair(origin_addr, instr));
		SIZE instr_size = instr->disassemable();
		origin_addr += instr_size;
		current_addr += instr_size;
		security_size -= instr_size;
		_origin_function_instructions.push_back(instr);
	}
	ASSERT(security_size==1);
	is_already_disasm = true;
}

void Function::point_to_random_function()
{
	NOT_IMPLEMENTED(wz);
}

SIZE Function::random_function(CODE_CACHE_ADDR cc_curr_addr, ORIGIN_ADDR cc_origin_addr)
{
	ASSERT(is_already_split_into_bb);

	SIZE bb_copy_size = 0;
	_random_cc_start = cc_curr_addr;
	_random_cc_origin_start = cc_origin_addr;
	_random_cc_size = 0;
	for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
		cc_curr_addr += bb_copy_size;
		cc_origin_addr += bb_copy_size;
		bb_copy_size = (*ite)->copy_instructions(cc_curr_addr, cc_origin_addr);
		_random_cc_size += bb_copy_size;
	}
	return _random_cc_size;
}

Instruction *Function::get_instruction_by_addr(ORIGIN_ADDR origin_addr)
{
	ASSERT(is_already_disasm);
	map<ORIGIN_ADDR, Instruction*>::iterator iter = _map_origin_addr_to_inst.find(origin_addr);
	if(iter==_map_origin_addr_to_inst.end())
		return NULL;
	else
		return iter->second;
}

typedef struct item{
	Instruction *inst;
	Instruction *fallthroughInst;
	vector<Instruction *> targetList;
	BOOL isBBEntry;
	BOOL isBBEnd;
}Item;

void Function::split_into_basic_block()
{
	ASSERT(is_already_disasm);
	SIZE inst_sum = _origin_function_instructions.size();
	Item *array = new Item[inst_sum];
	//init
	for(SIZE k=0; k<inst_sum; k++){
		array[k].isBBEntry = false;
		array[k].isBBEnd = false;
	}
	map<Instruction *, INT32> inst_map_idx;
	//BasicBlock *entryBlock = new BasicBlock();
	//scan to find target and fallthrough
	INT32 idx = 0;
	for(vector<Instruction*>::iterator iter = _origin_function_instructions.begin(); iter!=_origin_function_instructions.end(); iter++){
		Instruction *curr_inst = *iter;
		array[idx].inst = curr_inst;
		inst_map_idx.insert(make_pair(curr_inst, idx));
		if(curr_inst->isConditionBranch()){
			//INFO("%.8lx conditionBranch!\n", curr_inst->get_inst_origin_addr());
			//add target inst
			ORIGIN_ADDR target_addr = curr_inst->getBranchTargetOrigin();
			Instruction *target_inst = get_instruction_by_addr(target_addr);
			if(target_inst)
				array[idx].targetList.push_back(target_inst);
			else
				ERR("%.8lx  find none target in condtionjmp!\n", curr_inst->get_inst_origin_addr());
			array[idx].isBBEnd = true;
			//add fallthrough inst
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx].fallthroughInst = *(iter+1);
				array[idx+1].isBBEntry = true;
			}else{
				ERR("%.8lx  fallthrough inst out of function!\n", curr_inst->get_inst_origin_addr());
				array[idx].fallthroughInst = NULL;
			}
		}else if(curr_inst->isDirectJmp()){
			//INFO("%.8lx directJmp!\n", curr_inst->get_inst_origin_addr());
			//add target
			ORIGIN_ADDR target_addr = curr_inst->getBranchTargetOrigin();
			Instruction *target_inst = get_instruction_by_addr(target_addr);
			if(target_inst)
				array[idx].targetList.push_back(target_inst);
			else
				ERR("%.8lx  target inst is empty in directjmp\n", curr_inst->get_inst_origin_addr());
			array[idx].fallthroughInst = NULL;
			array[idx].isBBEnd = true;
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx+1].isBBEntry = true;
			}
		}else if(curr_inst->isIndirectJmp()){
			//INFO("%.8lx indirectJmp!\n", curr_inst->get_inst_origin_addr() - _code_segment->code_start);
			//add target
			vector<ORIGIN_ADDR> *target_addr_list = _code_segment->find_target_by_inst_addr(curr_inst->get_inst_origin_addr());
			for(vector<ORIGIN_ADDR>::iterator it = target_addr_list->begin(); it!=target_addr_list->end(); it++){
				Instruction *target_inst = get_instruction_by_addr(*it);
				if(target_inst){
					array[idx].targetList.push_back(target_inst);
				}else
					ERR("%.8lx  target inst is out of function!\n", curr_inst->get_inst_origin_addr());
			}
			if(array[idx].targetList.size()==0)
				ERR("%.8lx  do not find target!\n", curr_inst->get_inst_origin_addr());
			array[idx].fallthroughInst = NULL;
			array[idx].isBBEnd = true;
			if((iter+1)!=_origin_function_instructions.end())
				array[idx+1].isBBEntry = true;
		}else if(curr_inst->isDirectCall()){
			//INFO("%.8lx directCall!\n", curr_inst->get_inst_origin_addr());
			//add target
			ORIGIN_ADDR target_addr = curr_inst->getBranchTargetOrigin();
			Instruction *target_inst = get_instruction_by_addr(target_addr);
			ASSERT(!target_inst);
			array[idx].isBBEnd = true;
			//add fallthrough
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx].fallthroughInst = *(iter+1);
				array[idx+1].isBBEntry = true;
			}else
				array[idx].fallthroughInst = NULL;
		}else if (curr_inst->isIndirectCall()){
			//INFO("%.8lx indirectCall!\n", curr_inst->get_inst_origin_addr());
			array[idx].isBBEnd = true;
			//add fallthrough
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx].fallthroughInst = *(iter+1);
				array[idx+1].isBBEntry = true;
			}else
				array[idx].fallthroughInst = NULL;
		}else if(curr_inst->isRet()){
			//INFO("%.8lx ret!\n", curr_inst->get_inst_origin_addr());
			array[idx].isBBEnd = true;
			array[idx].fallthroughInst = NULL;
			if((iter+1)!=_origin_function_instructions.end())
				array[idx+1].isBBEntry = true;
		}else{
			//INFO("%.8lx none!\n", curr_inst->get_inst_origin_addr());
			array[idx].isBBEnd = false;
			if((iter+1)!=_origin_function_instructions.end())
				array[idx].fallthroughInst = *(iter+1);
			else
				array[idx].fallthroughInst = NULL;
		}
		
		idx++;

	}
	array[0].isBBEntry = true;
	array[idx-1].isBBEnd = true;

	
	//calculate the BB entry
	for(idx = 0;idx<(INT32)inst_sum; idx++){
		for(vector<Instruction*>::iterator it = array[idx].targetList.begin(); it!=array[idx].targetList.end(); it++){
			INT32 targetIdx = inst_map_idx.find(*it)->second;
			//ERR("%.8lx->%.8lx\n", array[idx].inst->get_inst_origin_addr(), array[targetIdx].inst->get_inst_origin_addr());
			array[targetIdx].isBBEntry = true;
			if(targetIdx!=0 && !array[targetIdx-1].isBBEnd){
				array[targetIdx-1].isBBEnd = true;
			}
		}
	}/*
	for(SIZE i=0; i<inst_sum; i++){
		INFO("%.8lx Entry:%d End: %d\n", array[i].inst->get_inst_origin_addr(), array[i].isBBEntry, array[i].isBBEnd);
	}*/
	
	//create BasicBlock and insert the instructions
	BasicBlock *curr_bb = NULL;
	Instruction *first_inst_in_bb = NULL;
	map<Instruction *, BasicBlock *> inst_bb_map;
	for(SIZE i=0; i<inst_sum; i++){
		Item *item = array+i;
		if(item->isBBEntry){
			curr_bb = new BasicBlock();
			bb_list.push_back(curr_bb);
			first_inst_in_bb = item->inst;
		}
		
		curr_bb->insert_instruction(item->inst);
		
		if(item->isBBEnd)
			inst_bb_map.insert(make_pair(first_inst_in_bb, curr_bb));
	}
	
	//add prev, target, fallthrough
	for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
		BasicBlock *curr_bb = *ite;
		Item *item = array + inst_map_idx.find(curr_bb->get_last_instruction())->second;
		ASSERT(item->isBBEnd);
		if(item->fallthroughInst){
			BasicBlock *fallthroughBB = inst_bb_map.find(item->fallthroughInst)->second;
			curr_bb->add_fallthrough_bb(fallthroughBB);
			fallthroughBB->add_prev_bb(curr_bb);
		}
		for(vector<Instruction*>::iterator it = item->targetList.begin(); it!=item->targetList.end(); it++){
			BasicBlock *targetBB = inst_bb_map.find(*it)->second;
			curr_bb->add_target_bb(targetBB);
			targetBB->add_prev_bb(curr_bb);
		}
	}
	delete [] array;
	is_already_split_into_bb = true;
}

typedef enum{
	EMPTY_RANDOM = 0,
	FALSE_RANDOM,
	TRUE_RANDOM,
	UNKNOWN_RANDOM,
	NONE_RESULT,
	SUM_RANDOM,
}ELEMENT_TYPE;

void dump_random_matrix(ELEMENT_TYPE **random_matrix, INT32 size)
{
	for(INT32 column=0; column<size; column++){
		if(column==0)
			INFO("   ");
		INFO("%3d ", column);
	}
	INFO("\n");
	for(INT32 row=0; row<size; row++){
	 	INFO("%3d  ", row);
		for(INT32 column=0; column<size; column++){
			switch(random_matrix[row][column]){
				case EMPTY_RANDOM: PRINT("E"); break;
				case FALSE_RANDOM: PRINT(COLOR_RED"F"COLOR_END); break;
				case TRUE_RANDOM: PRINT(COLOR_BLUE"T"COLOR_END); break;
				case UNKNOWN_RANDOM: PRINT(COLOR_YELLOW"U"COLOR_END); break;
				case NONE_RESULT: PRINT(COLOR_GREEN"N"COLOR_END);break;
				default: ASSERT(0);
			}
			PRINT("   ");
		}
		PRINT("\n");
	 }
}
typedef multimap<INT32, INT32> ROW_MAP;
typedef multimap<INT32, INT32>::iterator ROW_MAP_ITER;
typedef pair<ROW_MAP_ITER, ROW_MAP_ITER> ROW_MAP_RANGE;

void solution_random_matrix(ELEMENT_TYPE **matrix, INT32 size, ROW_MAP unknown_row_map, INT32 *unknown_column_num,
	INT32 *false_column_num, INT32 *true_column_num)
{
	INT32 unknown_num = unknown_row_map.size();
	while(1){
		//calculate diagonal value
		for(INT32 idx=0; idx<size; idx++){
			ELEMENT_TYPE current_res  =matrix[idx][idx];
			if(current_res==NONE_RESULT){
				if(false_column_num[idx]==0){
					if(unknown_column_num[idx]==0){
						ASSERT(true_column_num[idx]!=0);
						matrix[idx][idx] = TRUE_RANDOM;
					}
				}else
					matrix[idx][idx] = FALSE_RANDOM;
				//scan row to change UNKNOW_RANDOM to current_res
				ROW_MAP_RANGE range = unknown_row_map.equal_range(idx);
				for(ROW_MAP_ITER iter = range.first; iter!=range.end; iter++){
					INT32 row_idx = iter->first;
					ASSERT(row_idx == idx);
					INT32 column_idx = iter->second;
					//set column num
					if(matrix[idx][idx]==FALSE_RANDOM)
						false_column_num[column_idx]++;
					else if(matrix[idx][idx]==TRUE_RANDOM)
						true_column_num[column_idx]++;
					else
						ASSERT(0);
					unknown_column_num[column_idx]--;
				}
				unknown_row_map.erase(idx);			
			}
		}
		INT32 unknown_num_iterator = unknown_row_map.size();
		if(unknown_num_iterator==unknown_num){
			if(unknown_num_iterator==0)
				break;
			else{//search one loop
				for(INT32 idx=0; idx<size; idx++){
					;
				}
			}
		}else
			unknown_num = unknown_num_iterator;
	}
}

void Function::analyse_random_bb()
{
	// 1.initialize the bb map idx;
	INT32 bb_num = bb_list.size();
	map<BasicBlock*, INT32> bb_list_map_idx;
	for(INT32 idx=0; idx<bb_num; idx++)
		bb_list_map_idx.insert(make_pair(bb_list[idx], idx));
	// 2.initialize the matrix and multimap
	ROW_MAP unknown_row_map;
	INT32 *false_column_num = new INT32[bb_num](0);
	INT32 *true_column_num = new INT32[bb_num](0);
	INT32 *unknown_column_num = new INT32[bb_num](0);
	ELEMENT_TYPE **random_matrix = new ELEMENT_TYPE*[bb_num];
	for(INT32 row=0; row<bb_num; row++){
		random_matrix[row] = new ELEMENT_TYPE[bb_num];
		// 2.1 give a initialized num
		for(INT32 column=0; column<bb_num; column++)
			random_matrix[row][column] = row==column ? NONE_RESULT : EMPTY_RANDOM;
		// 2.2 calculate the succ basicblock
		BasicBlock *src_bb = bb_list[row];
		BOOL is_call_bb = src_bb->is_call_bb();
		if(is_call_bb){
			ASSERT(!(src_bb->is_fallthrough_empty()) && src_bb->is_target_empty());
			INT32 dest_bb_idx = bb_list_map_idx.find(src_bb->get_fallthrough_bb())->second;
			random_matrix[row][dest_bb_idx] = FALSE_RANDOM;
			//set record map
			false_column_num[dest_bb_idx]++;
		}else{
			ELEMENT_TYPE succ_bb_type = src_bb->find_first_least_size_instruction(5) == src_bb->end() ? UNKNOWN_RANDOM : TRUE_RANDOM;
			//set target bb's matrix type
			for(BB_ITER iter = src_bb->target_begin(); iter!=src_bb->target_end(); iter++){
				INT32 dest_succ_idx = bb_list_map_idx.find(*iter)->second;
				random_matrix[row][dest_succ_idx] = succ_bb_type;
				//set record num
				if(succ_bb_type==UNKNOWN_RANDOM){
					unknown_column_num[dest_succ_idx]++;
					unknown_row_map.insert(make_pair(row, dest_succ_idx));
				}else
					true_column_num[dest_succ_idx]++;
			}
			//set fallthrough 
			if(src_bb->get_fallthrough_bb()){
				INT32 dest_succ_idx = bb_list_map_idx.find(src_bb->get_fallthrough_bb())->second;
				random_matrix[row][dest_succ_idx] = succ_bb_type;
				//set record num
				if(succ_bb_type==UNKNOWN_RANDOM){
					unknown_column_num[dest_succ_idx]++;
					unknown_row_map.insert(make_pair(row, dest_succ_idx));
				}else
					true_column_num[dest_succ_idx]++;
			}
		}
	}
	// 3.calculate the entry bb
	random_matrix[0][0] = FALSE_RANDOM;
	// 4.solution the matrix
	
	//dump
	dump_random_matrix(random_matrix, bb_num);
}
