#ifndef VMINFO_H
#define VMINFO_H

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <set>

#define VM_NAME_LEN	64
#define PCI_STR_LEN 8

// the state of a VM, which include
// I/O intensive state, idle state and other state(CPU or Memory intensive).
enum VM_State{IO_STAT, OTHER_STAT, IDLE_STAT};
/*
**	Virtual machine object.
**
*/
struct VM_info {
	VM_info() {
		vf_no = pid = dom_id = real_cpu_no = -1;
		rx_packets = tx_packets = total_packets = 0;
		rx_bytes = tx_bytes = total_KB = 0;
		vm_state = IDLE_STAT;
		dom_ptr = NULL;
		io_timestamp_usec = 0;
	}
	char						vm_name[VM_NAME_LEN];
	char						netdev_pci_slot_str[PCI_STR_LEN];
	char						netdev_pci_funct_str[PCI_STR_LEN];
	// the VF number, -1 if not assigned one.
	int							vf_no;

	/* I/O statistics	*/
	unsigned long long			rx_packets;
	unsigned long long			tx_packets;
	unsigned long long			total_packets;
	double						packets_per_sec;

	unsigned long long			rx_bytes;
	unsigned long long			tx_bytes;
	// KB
	double						total_KB;
	double						KB_per_sec;

	unsigned long long			io_timestamp_usec;

	VM_State					vm_state;

	/* libvirt API related */
	virDomainPtr 				dom_ptr;
	// domain id, -1 if offline
	int							dom_id;
	// vcpu affinity sets. Containing which real cpu number could run on.
	std::set<unsigned int>		vcpu_affi_maps;
	//	the real CPU number currently on, or -1 if offline
	int							real_cpu_no;
	double						real_cpu_usage;
	// the memory page distribution in numa node of this VM.
	std::vector<unsigned int> 	mem_node_distr;
	// the memory of VM is mainly in which NUMA node;
	int 						mem_main_node_id;
	// The corresponding process id of this VM.
	unsigned int 				pid;
};










#endif
