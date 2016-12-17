#ifndef _PERF_MONITOR_H
#define _PERF_MONITOR_H

#include "VM_info.h"

/*
** Monitor performance counter data using Linux perf.
** Author:Jenson
*/
class PerfMonitor{
public:
	/*
	** Monitor performance event for specific VM.
	** Return 0 if succeed; -1 if failed.
	*/
	int	monitorPerfEvent(VM_info& vmInfo);

};



#endif