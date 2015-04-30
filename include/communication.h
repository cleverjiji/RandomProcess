#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include "utility.h"
#include "type.h"
#include "atomic.h"
#include <sys/types.h>
#include <signal.h>
#include <ucontext.h>
#include <vector>
using namespace std;

typedef struct communication_info{
	volatile ORIGIN_ADDR jump_table_base;
	struct ucontext *origin_uc;
	volatile pid_t process_id;
	volatile UINT8 can_stop;//0: can not; 1:can
	volatile UINT8 flag;
}COMMUNICATION_INFO;

class Communication
{
protected:
	SpinLock *lock;
	COMMUNICATION_INFO *main_thread_info;
	vector<COMMUNICATION_INFO *> child_thread_info;
public:
	Communication();
	
	void set_main_communication(COMMUNICATION_INFO *info)
	{
		main_thread_info = info;
	}

	void set_child_communication(COMMUNICATION_INFO *info)
	{
		child_thread_info.push_back(info);
	}
	
	BOOL stop_process();
	BOOL continue_process();
};

#endif
