#ifndef _CODE_SEGMENT_MANAGEMENT_H_
#define _CODE_SEGMENT_MANAGEMENT_H_

#include <vector>
using namespace std;

class CodeSegment; 
extern vector<CodeSegment*> code_segment_vec;

extern void dump_code_segment();

#endif