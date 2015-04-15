#include "type.h"
#include "readlog.h"

const char *ReadLog::lib_name[] = {"libc.so", "libm.so", "ld-2.17.so", "none"};
const INDIRECT_ITEM ReadLog::indirect_profile_by_hand[] = {
	{0x13acd5, 0x13cc40, LIBC, LIBC},
	{0x13bed5, 0x13cbea, LIBC, LIBC},
	{0x13be12, 0x13cfe0, LIBC, LIBC},
	{0x13b155, 0x13c992, LIBC, LIBC},
	{0x13ae55, 0x13cd2c, LIBC, LIBC},
	{0x13afd5, 0x13cc1c, LIBC, LIBC},
	{0x13aa26, 0x13ca1c, LIBC, LIBC},
	{0, 0, LIB_NONE, LIB_NONE},
};



void ReadLog::init_share_log()
{
	//1.create ifstream file
	ifstream ifs(share_log_name.c_str(), ifstream::in);
	//2.read log 
	string shm_info;
	ifs>>shm_info>>shm_num;
	INT32 map_cc_array[100];
	CodeCache *cc_array[100];
	ASSERT(shm_num<=100);
	for(INT32 idx=0; idx<shm_num; idx++){
		ORIGIN_ADDR region_start;
		ORIGIN_ADDR region_end;
		string shm_name;
		string code_path;
		char c;
		INT32 cc_idx;
		BOOL isCodeCache;
		BOOL isStack;
		ifs>>dec>>cc_idx>>hex>>region_start>>c>>region_end>>shm_name>>code_path;
		isCodeCache = cc_idx==-1 ? true : false;
		isStack = cc_idx==-2 ? true: false;
		map_cc_array[idx] = cc_idx;
		CodeSegment *cs = new CodeSegment(region_start, region_end-region_start, code_path, shm_name, isCodeCache, isStack);
		code_segment_vec.push_back(cs);
		//code cache record
		if(isCodeCache){
			CodeCache *cc = new CodeCache(*cs);
			if(!cc_management)
				cc_management = new CodeCacheManagement();
			cc_management->insert(cc);
			cc_array[idx] = cc;
		}else
			cc_array[idx] = NULL;
		//stack record
		if(isStack)
			main_share_stack = new ShareStack(*cs, true);
	}
	//3.map cc
	for(INT32 idx=0; idx<shm_num; idx++){
		INT32 map_idx = map_cc_array[idx];
		CodeCache *cc = (map_idx==-1 || map_idx==-2) ? NULL : cc_array[map_idx];
		code_segment_vec[idx]->map_CC_to_CS(cc);
	}
	share_log_is_init = true;
}

void ReadLog::init_profile_log()
{
	ASSERT(share_log_is_init);
	//1.create ifstream file
	ifstream ifs(profile_log_name.c_str(), ifstream::in);
	//2.read log
	INT32 image_num = 0;
	string str_image_num;
	INT32 num;
	
	ifs>>str_image_num>>image_num;
	CodeSegment **array = new CodeSegment*[image_num+1];
	for(INT32 idx=1; idx<=image_num; idx++){
		string image_path, name;
		ifs>>num>>image_path;
		unsigned found = image_path.find_last_of("/");
		if(found==string::npos)
			name = image_path;
		else
			name = image_path.substr(found+1);

		array[idx] = find_cs(name);
	}

	string padding;
	char c;
	INT32 inst_num = 0;
	ifs>>hex>>padding>>inst_num;
	for(INT32 idx=1; idx<=inst_num; idx++){
		SIZE inst, target;
		INT32 inst_idx, target_idx;
		ifs>>hex>>inst>>c>>inst_idx>>padding>>target>>c>>target_idx>>c;
		CodeSegment *curr_cs = array[inst_idx];
		CodeSegment *target_cs = array[target_idx];
		ORIGIN_ADDR curr_inst_addr = curr_cs->code_start+inst;
		ORIGIN_ADDR target_inst_addr = target_cs->code_start+target;
		curr_cs->indirect_inst_map.insert(make_pair(curr_inst_addr, target_inst_addr));
	}

	//extened indirect log by hand
	CodeSegment **lib_array = new CodeSegment*[(UINT32)LIB_NONE];
	for(UINT32 idx=0; idx<(UINT32)LIB_NONE; idx++)
		lib_array[idx] = find_cs((LIB_IDX)idx);

	INT32 idx=0;
	while(indirect_profile_by_hand[idx].indirect_inst_offset!=0){
		LIB_IDX curr_idx = indirect_profile_by_hand[idx].indirect_inst_lib;
		LIB_IDX target_idx = indirect_profile_by_hand[idx].indirect_target_lib;
		CodeSegment *curr_cs = lib_array[curr_idx];
		CodeSegment *target_cs = lib_array[target_idx];
		if(curr_cs && target_cs){
			ORIGIN_ADDR curr_inst_addr = curr_cs->code_start + indirect_profile_by_hand[idx].indirect_inst_offset;
			ORIGIN_ADDR target_inst_addr = target_cs->code_start + indirect_profile_by_hand[idx].indirect_target_offset;
			curr_cs->indirect_inst_map.insert(make_pair(curr_inst_addr, target_inst_addr));
		}
		idx++;
	}

	//free array
	delete []array;
	delete []lib_array;
}

