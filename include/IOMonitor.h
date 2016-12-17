/*
** monitor IO statistic
** --Written by Jenson	
*/
#ifndef IO_MONITOR_H
#define IO_MONITOR_H

#include <cstdlib>
#include <cstring>
#include "VM_info.h"

#define  NETDEV_LEN 32
#define  MAX_PATH_LEN 64


class VM_info;

class IOMonitor{
public:
	void initIOMonitor(const char* nic) {
		strcpy(netdev, nic);
	}
	/* monitor IO of a specific VM. This method will modify the IO data of the VM. */
	int monitorIO(VM_info&);
private:
	// which net device should be monitored
	char					netdev[NETDEV_LEN];

};



#endif