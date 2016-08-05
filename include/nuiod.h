#ifndef NUIOD_H
#define NUIOD_H

/*
**	the main class of nuiod
**	------------written by Jenson
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <vector>
#include <list>
#include <algorithm>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <numa.h>
#include <sys/time.h>
#include <map>
#include <pthread.h>

#include "vm_info.h"
#include "cpu_monitor.h"
#include "numa_info.h"
#include "mem_monitor.h"
#include "thread_migrate.h"
#include "mem_migrate.h"
#include "utils.h"
#include "action.h"

#define NETDEV_LEN 		32
#define MAXLINE			128
#define MAX_PATH_LEN	64
// The smoothing factor for calculating 'packets_per_sec'.
#define SMOOTH_FACTOR	0.4
#define SMOOTH_DEC_FACTOR 0.85

class Nuiod {
public:
	Nuiod(int numa_num, int cpu_num, const char* connect_str):
									 numa_number(numa_num), cpu_number(cpu_num),									 
									 cpu_monitor(cpu_num),  mem_monitor(numa_num),
									 mem_migrate(numa_num), thread_migrate(numa_num, cpu_num)
	{
		int len = strlen(connect_str);
		conn_str = new char[len + 1];
		strncpy(conn_str, connect_str, len);
		conn_str[len] = '\0';
		io_threshold = 0;

		monitor_started = false;
	}

	~Nuiod();

	// return -1 if failed; 0 if succeed.
	int init();

	void start();

private:
	int						numa_number;
	int 					cpu_number;
	// which net device should be monitored
	char					netdev[NETDEV_LEN];
	char					netdev_pci_bus_str[PCI_STR_LEN];
	char					netdev_pci_slot_str[PCI_STR_LEN];
	char					netdev_pci_funct_str[PCI_STR_LEN];
	// corresponding integer value
	unsigned int			netdev_pci_bus_val;
	unsigned int			netdev_pci_slot_val;
	unsigned int			netdev_pci_funct_val;
	// the numa node of netdev 
	int 					netdev_numanode;

	// monitor interval(second)
	unsigned int 			monitor_interval;
	// I/O intensive threshold
	unsigned long long 		io_threshold;

	// Containing all running vm. Key=dom_id.
	std::map<int, VM_info> 	vm_infos_map;	

	/* other object */	
	Cpu_monitor 			cpu_monitor;
	Mem_monitor 			mem_monitor;
	Memory_migrate 			mem_migrate;
	Thread_migrate 			thread_migrate;

	/* When initializing, Calculating numa latency( numa_info.init()) will take a long time. 
		So using another thread to calculate numa latency.
	*/ 
	friend void*			cal_numa_lat_thread(void* arg);

	/* init vm_info */
	int 					init_vminfo();
	/* using the virDomainPtr to initialize a VM_info object */
	int 					set_vm_info(virDomainPtr, VM_info&, int dom_id);

	/* parsing config file */
	int 					parse_conf();

	/********************** libvirt API related  **********************/	
	char					*conn_str;
	virConnectPtr 			conn;
	static void 			libvirt_error_handle(void* userdata, virErrorPtr err);
	// get the real cpu of VM.
	int 					get_vm_real_cpu(virDomainPtr);


	/********************** main procedure of nuiod **********************/	

	/* Has the monitor started? If not, discard the 
		first monitor data. */
	bool					monitor_started;

	/* Monitor all data.
	** Return: true when migration is needed, false otherwise.
	*/
	bool 					monitor();

	/* status_changed: true if some VMs become io intesive. */
	int                		monitor_io(bool &status_changed);

	/* Analyze the monitoring data and make migration action */
	void					analyze(std::vector<Action>&);
	/* perform the migrate action */
	void					perform(std::vector<Action>&);

	void 					refresh_cpu_mapping();

	// modify the cpu_vm_mapping and VM_info.real_cpuno
	void					change_cpu_vm_mapping(VM_info&, int new_cpu);

	/********************** numa information related **********************/

	/* The mapping of cpu and numa number.
	** Key=real cpu number;	Value=numa number.
	*/
	std::map<int, int>		cpu_node_mapping;

	/* The mapping of real cpu number and VM id.
	** Key=real cpu number;	Value=VM id.
	*/
	std::map<int, int>		cpu_vm_mapping;

	// size: [numa_number]
	Numa_node*				numa_nodes;
	// the numa latency matrix
	double**				numa_latency;

	int 					init_numa_info();
	int     				init_numa_latency();
	void					cal_and_create_numa_lat();	
	// fill the cpu_ids with the cpus in node
	int 					get_node_cpus(const int node, std::set<int> &cpu_ids);
	// get the first cpu number of node
	int 					get_node_first_cpu(const int node);	

};



#endif