/*
** Monitor memory usage. Should include monitoring the page usage in the future.
** 
** -----------written by Jenson	
*/
#ifndef MEM_MONITOR_H
#define MEM_MONITOR_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <vector>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>


class Mem_monitor {
public:
	Mem_monitor(int numa_num):numa_number(numa_num) {
		free_mems = new unsigned long long[numa_number];
	}
	~Mem_monitor() {
		if(free_mems != NULL)
			delete[] free_mems;
	}

	/* Using the libvirt API to get the amount of free memory in all NUMA nodes.
	* return -1 if failed, 0 if succeed.
	*/
	int get_node_freemem(const virConnectPtr conn, std::vector<unsigned long long> &free_mem);

	/* get the memory page distribution in numa node of a process */
	int get_proc_mem_numa_distribution(const unsigned int pid, std::vector<unsigned int> &node_pages);

private:
	int 				numa_number;
	// Used in get_node_freemem function.
	unsigned long long 	*free_mems;

	static const int 	BUF_SIZE = 256;
	char 				buf[BUF_SIZE];
};


#endif