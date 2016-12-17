#ifndef _INCLUDE_H_
#define _INCLUDE_H_

#include "Mem_monitor.h"
#include "VM_info.h"
#include "Decision.h"
#include "Cpu_monitor.h"
#include "IOMonitor.h"
#include "Numa_node.h"
#include "Thread_migrate.h"
#include "Memory_migrate.h"
#include "Nuiod.h"

/*
 const int  NETDEV_LEN = 32;

 const int  MAXLINE	= 128;

 const int  MAX_PATH_LEN= 64;

 const int  PCI_STR_LEN = 8;

// The smoothing factor for calculating 'packets_per_sec'.

 const double  SMOOTH_FACTOR	= 0.4;

 const double  SMOOTH_DEC_FACTOR = 0.85;

*/

class VM_info;
class IOMonitor;
class Mem_monitor;
struct Numa_node;
struct Decision; 
class Cpu_monitor;
class Thread_migrate;
class Memory_migrate;
class Nuiod;



#endif