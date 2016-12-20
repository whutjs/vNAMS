#ifndef VMINFO_H

#define VMINFO_H


#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <set>
#include "Nuiod.h"
#include <vector>


#define VM_NAME_LEN 64
#define PCI_STR_LEN 8

/*
**	Virtual machine object.
**
*/
class VM_info {
public:
	VM_info() {
		vf_no = pid = dom_id = real_cpu_no = -1;
		rx_packets = tx_packets = total_packets = 0;
		rx_bytes = tx_bytes = total_KB = 0;
		dom_ptr = NULL;
		io_timestamp_usec = 0;
		vm_event_state = VIR_DOMAIN_EVENT_UNDEFINED;
	}
	const char* 						getVMName() const { return vm_name;}	
	double 								getNewPacketsPerSec() const { return packets_per_sec; }
	double 								getHistoricalPacketsPerSec() const { return packets_per_sec_history; }
	double 								getNewKBPerSec() const { return KB_per_sec; }
	double 								getHistoricalKBPerSec() const { return KB_per_sec_history; }

	/* Get the memory distribution in each NUMA Node */

	const std::vector<unsigned int>&	getMemDistriInEachNode() const { return mem_node_distr; }
	int 								getPhysicalCPUNo() const { return real_cpu_no;}
	double 								getPhysicalCPUUsage() const { return real_cpu_usage; }

	int 								getVMID() const {return dom_id;}
	unsigned int 						getPID() const { return pid; }

	int 								getMajorMemResideOnNode() const { return mem_main_node_id; }

	unsigned long long					getPerfCycles() const { return cycles; }
	unsigned long long					getPerfInstructions() const { return instructions; }
	unsigned long long					getPerfLLCDataRdMiss() const { return LLC_data_rd_miss; }
	unsigned long long					getPerfLLCDataRdHit() const { return LLC_data_rd_hit; }
	unsigned long long					getPerfLLCAllReference() const { return LLC_all_reference; }
	unsigned long long					getPerfLLCAllMiss() const { return LLC_all_miss; }

private:

	friend class 				Nuiod;
	friend class 				IOMonitor;
	friend class 				PerfMonitor;

	char						vm_name[VM_NAME_LEN];
	char						netdev_pci_slot_str[PCI_STR_LEN];
	char						netdev_pci_funct_str[PCI_STR_LEN];
	// the VF number, -1 if not assigned one.
	int							vf_no;



	/*********** I/O statistics data ********start *********/
	unsigned long long			rx_packets;
	unsigned long long			tx_packets;
	unsigned long long			total_packets;	
	// the history data
	unsigned long long			total_packets_history;

	// the history data of packets_per_sec;
	double						packets_per_sec_history;
	// the new data of packets_per_sec;
	double						packets_per_sec;
	unsigned long long			rx_bytes;
	unsigned long long			tx_bytes;

	// KB
	double						total_KB;
	// the corresponding history data
	double						total_KB_history;
	double						KB_per_sec;
	// the history data of KB_per_sec;
	double						KB_per_sec_history;
	unsigned long long			io_timestamp_usec;

	/*********** I/O statistics data ********end *********/

	/*********** Performance data ********start *********/

	// Number of CPU cycles captured during monitoring period;
	unsigned long long			cycles;
	// Number of CPU Instructions captured during monitoring period;
	unsigned long long			instructions;
	// Counts demand data reads that miss in the LLC
	unsigned long long			LLC_data_rd_miss;
	// Counts all demand data reads that hit in the LLC
	unsigned long long			LLC_data_rd_hit;
	// Core-originated cacheable demand requests that refer to LLC (all LLC reference)
	unsigned long long			LLC_all_reference;
	// Core-originated cacheable demand requests missed LLC
	unsigned long long			LLC_all_miss;

	/*********** Performance data ********end *********/

	/*********** libvirt API related ********************/
	virDomainPtr 				dom_ptr;

	// domain id, -1 if offlin
	int							dom_id;

	// vcpu affinity sets. Containing which real cpu number could run on.
	std::set<unsigned int>		vcpu_affi_maps;

	//	the physical CPU No currently on, or -1 if offline
	int							real_cpu_no;
	double						real_cpu_usage;

	// the memory page distribution in each NUMA node of this VM.
	std::vector<unsigned int> 	mem_node_distr;

	// The major memory of this VM resides in which NUMA node ;
	int 						mem_main_node_id;
	// The corresponding process id of this VM.
	unsigned int 				pid;
	/* valid state include:
	** 1,Suspended; 2,Resumed;
	** 3,Stopped;	4,Shutdown;
	** 5,PMSuspended; 6,Crashed
	*/ 
	virDomainEventType			vm_event_state;
};





















#endif

