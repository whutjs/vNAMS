#include "Nuiod.h"
using namespace std;

void* cal_numa_lat_thread(void* arg) {
	Nuiod* nuiod = ((Nuiod*)arg);
	int ret = nuiod->init_numa_info();
	return (void*)(ret);
}



Nuiod::~Nuiod() {
	delete ioMonitor;
	delete perfMonitor;
	delete[] conn_str;

	// need to free domain_ptr
	map<int, VM_info>::iterator it;

    for(it = vm_infos_map.begin(); it != vm_infos_map.end(); ++it)
    	virDomainFree((it->second).dom_ptr);     

    virConnectDomainEventDeregister(conn, virVMEventCallBack);
    virConnectUnregisterCloseCallback(conn, virConnectCloseCallBack);
    virConnectClose(conn); 

    /* release numa related data  */
    if(numa_nodes != NULL)
			delete[] numa_nodes;

	if(numa_latency != NULL) {
		for(int i = 0; i < numa_number; i++)
			delete[] numa_latency[i];
		delete[] numa_latency;
	}

}







// return -1 if failed, 0 if succeed
int Nuiod::init() {
	virSetErrorFunc(NULL, libvirt_error_handle);
	if (virInitialize() < 0) {
        fprintf(stderr, "Failed to initialize libvirt");
        return -1;
    }

    if (virEventRegisterDefaultImpl() < 0) {
        fprintf(stderr, "Failed to register event implementation: %s\n",
                virGetLastErrorMessage());
         return -1;
    }
	conn = virConnectOpen(conn_str);
	if(conn == NULL) {
		fprintf(stderr, "Failed to open connection to %s\n", conn_str);
		return -1;
	}	

	int         ret = 0;	

	/* create a thread to calculate numa latency */
	void* 		tret;
	pthread_t	tid;
	ret = pthread_create(&tid, NULL, cal_numa_lat_thread, this);
	if(ret != 0) {
		fprintf(stderr, "Cannot create thread!%s\n", strerror(ret));
		return -1;
	}

	/* Now initialize other resouces */	
	ret = parse_conf();
	if(ret == -1)
		exit(1);	

	if (virConnectRegisterCloseCallback(conn,
                                        virConnectCloseCallBack, NULL, NULL) < 0) {
        fprintf(stderr, "Unable to register close callback\n");
        return -1;
    }

    ret = virConnectDomainEventRegister(conn, virVMEventCallBack, this, NULL);
    if(ret != 0) {
		fprintf(stderr, "Unable to register Domain Event callback!\n");
		return -1;
	}

	ret = init_vminfo();
	if(ret != 0) {
		fprintf(stderr, "init_vminfo failed!\n");
		return -1;
	}
	perfMonitor = new PerfMonitor;
	ioMonitor = new IOMonitor;
	// init the object
	ioMonitor->initIOMonitor(netdev);
	ret = cpu_monitor.init_cpu_monitor();
	if(ret != 0) {
		fprintf(stderr, "init_cpu_monitor failed!\n");
		return -1;
	}



	// get the numa node of netdev
	char path_buf[MAX_PATH_LEN];
	snprintf(path_buf, MAX_PATH_LEN, "/sys/class/net/%s/device/numa_node", netdev);
	FILE* netdev_numanode_file = fopen(path_buf, "r");
	if(netdev_numanode_file == NULL) {
		fprintf(stderr, "Failed to open %s\n, error:%s", path_buf, strerror(errno));
		return -1;
	}

	ret = fscanf(netdev_numanode_file, "%d", &netdev_numanode);
	if(ret != 1) {
		fprintf(stderr, "fscanf doesnot match:%s, ret=%d\n", path_buf, ret);
		fclose(netdev_numanode_file);
		return -1;
	}
	fclose(netdev_numanode_file);

	// now wait for the thread
	ret = pthread_join(tid, &tret);
	if(ret != 0) {
		fprintf(stderr, "pthread_join error:%s", strerror(ret));
		return -1;
	}

	long lret = (long)tret;
	if(lret == -1) {		
		return -1;
	}
	return 0;

}



/*

**	Parsing /etc/nuiod.conf. If cannot open config file,

** 	return -1 and exit.

*/

int Nuiod::parse_conf() {

	const char* conf_file = "/etc/nuiod.conf";

	FILE* file = fopen(conf_file, "r");

	if(file == NULL) {

		perror("Open /etc/nuiod.conf failed. Please make sure the file exist");

		return -1;

	}

	char line[MAXLINE];

	while(fgets(line, MAXLINE, file) != NULL) {

		if(line[0] == '#') {

			// skip the comments.

			continue;

		} else if(strstr(line, "netdev") != NULL){

			sscanf(line, "netdev=%s", netdev);

		} else if(strstr(line, "pci_bus") != NULL){

			sscanf(line, "pci_bus=%s", netdev_pci_bus_str);

			netdev_pci_bus_val = strtol(netdev_pci_bus_str, NULL, 16);			

		} else if(strstr(line, "pci_slot") != NULL){

			sscanf(line, "pci_slot=%s", netdev_pci_slot_str);

			netdev_pci_slot_val = strtol(netdev_pci_slot_str, NULL, 16);			

		} else if(strstr(line, "pci_function") != NULL){

			sscanf(line, "pci_function=%s", netdev_pci_funct_str);

			netdev_pci_funct_val = strtol(netdev_pci_funct_str, NULL, 16);			

		} else if(strstr(line, "monitor_interval") != NULL){

			sscanf(line, "monitor_interval=%u", &monitor_interval);			

		} else if(strstr(line, "io_threshold") != NULL){

			sscanf(line, "io_threshold=%llu", &io_threshold);			

		}			

	}

	fclose(file);

	return 0;

}



/* using the virDomainPtr to initialize a VM_info object */
int Nuiod::set_vm_info(virDomainPtr dom_ptr, VM_info& vm_info, int dom_id) {

		int flag = 0;		
		vm_info.dom_ptr = dom_ptr;
		vm_info.dom_id = dom_id;
		// Note:Assuming a vm has only one vcpu now.
		// TODO: Donot make this assumption.
		int ret = get_vm_real_cpu(dom_ptr);		
		if(ret == -1) {			
			flag = -1;	
		} else{
			vm_info.real_cpu_no = ret;
			cpu_vm_mapping.insert(make_pair(vm_info.real_cpu_no, vm_info.dom_id));
		}		

		const char* dom_name = virDomainGetName(dom_ptr);
		int len = strlen(dom_name);
		if(len >= VM_NAME_LEN) {
			fprintf(stderr, "VM name:%s too long! Length:%d\n", dom_name, len);			
			flag = -1;	
		}else {
			strncpy(vm_info.vm_name, dom_name, len);
			vm_info.vm_name[len] = '\0';
		}

		// get the pid according to dom_name
		char path_buf[MAX_PATH_LEN];
		snprintf(path_buf, MAX_PATH_LEN, "/var/run/libvirt/qemu/%s.pid", dom_name);
		FILE* pid_file = fopen(path_buf, "r");
		if(pid_file == NULL) {
			fprintf(stderr, "cannot open %s, error:%s\n", path_buf, strerror(errno));
			flag = -1;
		} else{
			fscanf(pid_file, "%u", &vm_info.pid);
			fclose(pid_file);
		}

		// get the memory distribution
		ret = mem_monitor.get_proc_mem_numa_distribution(vm_info.pid, vm_info.mem_node_distr);
		if(ret != 0) {
			fprintf(stderr, "%s\n", "get_proc_mem_numa_distribution failed");
			flag = -1;
		}

		// which node has maximum memory pages
		unsigned pages_num = 0;
		for(unsigned node_id = 0; node_id < vm_info.mem_node_distr.size(); node_id++) {
			if(vm_info.mem_node_distr[node_id] > pages_num) {
				pages_num = vm_info.mem_node_distr[node_id];
				vm_info.mem_main_node_id = node_id;
			}
		}

		/* calculate the vf_no */
		char* dom_xml = virDomainGetXMLDesc(dom_ptr, 0);		
		if(dom_xml != NULL) {

			/* the xml format should be like this:
			<address domain='0x0000' bus='0x07' slot='0x00' function='0x4'/>
			*/
			long slot_val = 0, func_val = 0;
			char target_bus[PCI_STR_LEN+3];
			sprintf(target_bus, "bus=\'%s\'", netdev_pci_bus_str);
			char* bus_p = strstr(dom_xml, target_bus);
			if(bus_p == NULL) {
				fprintf(stderr, "Parsing domain xml error: cannot find 'bus' attribute.\n");
				flag = -1;
				free(dom_xml);
				return flag;
			}

			// extract the pci slot value
			char* slot_p = strstr(bus_p, "slot");

			if(slot_p == NULL) {
				fprintf(stderr, "Parsing domain xml error: cannot find 'slot' attribute.\n");
				flag = -1;
				free(dom_xml);
				return flag;
			}

			while(*(slot_p) != '\'')
				slot_p++;
			slot_p++;
			slot_val = strtol(slot_p, &slot_p, 0);
			// extract the pci function value
			char* funct_p = strstr(slot_p, "function");
			if(funct_p == NULL) {
				fprintf(stderr, "Parsing domain xml error: cannot find 'function' attribute.\n");
				flag = -1;
				free(dom_xml);
				return flag;
			}

			while(*(funct_p) != '\'')
				funct_p++;
			funct_p++;
			func_val = strtol(funct_p, &funct_p, 0);


			// !!!Note:Assuming: 
			// slot=0 function=1~7 -> vf 0~6
			// slot=1 function=0~7 -> vf 7~14
			// That is, vf_no = (vfslot - pfslot)*8 + (vf_function-1).
			vm_info.vf_no = (slot_val - netdev_pci_slot_val) * 8 + (func_val - 1);
			free(dom_xml);
		} else{
			fprintf(stderr, "%s\n", "cannot get domain xml");
			flag = -1;
		}
		return flag;
}



/* 
** Initialize the VM info.
** return -1 if failed, 0 if succeed.
*/
int Nuiod::init_vminfo() {

	// Using libvirt to get all running domain.
	int 	dom_num = 0;
	int 	*running_dom_ids;	
	int 	ret = 0;
	int 	flag = 0;

	dom_num = virConnectNumOfDomains(conn);
	running_dom_ids = new int[dom_num];
	dom_num = virConnectListDomains(conn, running_dom_ids, dom_num);	
	// get domain ptr
	for(int i = 0; i < dom_num; i++) {
		int dom_id = running_dom_ids[i];
		VM_info vm_info;
		vm_infos_map[dom_id] = vm_info;
		virDomainPtr dom_ptr;
		dom_ptr = virDomainLookupByID(conn, dom_id);
		flag = set_vm_info(dom_ptr, vm_infos_map[dom_id], dom_id);
		if(flag == -1) 
			ret = -1;			
	}
	delete[] running_dom_ids;
	return ret;
}



/****** libvirt API related  ***************/

void Nuiod::libvirt_error_handle(void* userdata, virErrorPtr err) {
	fprintf(stderr, "Global handler, failure of libvirt library call:\n");
	fprintf(stderr, "Code:%d\n", err->code);
	fprintf(stderr, "Domain:%d\n", err->domain);
	fprintf(stderr, "Message:%s:\n", err->message);
	fprintf(stderr, "Level:%d\n", err->level);
	fprintf(stderr, "str1:%s\n", err->str1);
}

void Nuiod::virConnectCloseCallBack(virConnectPtr conn ATTRIBUTE_UNUSED,
             				int reason,
             				void *opaque ATTRIBUTE_UNUSED) {
	switch ((virConnectCloseReason) reason) {
    case VIR_CONNECT_CLOSE_REASON_ERROR:
        fprintf(stderr, "Connection closed due to I/O error\n");
        return;

    case VIR_CONNECT_CLOSE_REASON_EOF:
        fprintf(stderr, "Connection closed due to end of file\n");
        return;

    case VIR_CONNECT_CLOSE_REASON_KEEPALIVE:
        fprintf(stderr, "Connection closed due to keepalive timeout\n");
        return;

    case VIR_CONNECT_CLOSE_REASON_CLIENT:
        fprintf(stderr, "Connection closed due to client request\n");
        return;

   default:
        break;
    };

    fprintf(stderr, "Connection closed due to unknown reason\n");
}

// Libvirt event call back. Handle the VM shutdown/start/reboot event
int Nuiod::virVMEventCallBack(virConnectPtr conn ATTRIBUTE_UNUSED,
							  virDomainPtr dom, int event,
							  int detail, void *data) {
	if(data == NULL) {
		return -1;
	}
	int ret = 0;
	Nuiod *nuiod = (Nuiod*)data;
	int domId = virDomainGetID(dom);
	virDomainEventType  curEventState = (virDomainEventType) event;
	if(curEventState == VIR_DOMAIN_EVENT_STARTED) {
		// A new VM started
		VM_info vm_info;
		(nuiod->vm_infos_map)[domId] = vm_info;
		virDomainPtr dom_ptr;
		dom_ptr = virDomainLookupByID(conn, domId);
		int flag = nuiod->set_vm_info(dom_ptr, (nuiod->vm_infos_map)[domId], domId);
		(nuiod->vm_infos_map)[domId].vm_event_state = curEventState;
		if(flag == -1) 
			ret = -1;		
	} else if(curEventState == VIR_DOMAIN_EVENT_SUSPENDED
			|| curEventState == VIR_DOMAIN_EVENT_RESUMED
			|| curEventState == VIR_DOMAIN_EVENT_STOPPED
			|| curEventState == VIR_DOMAIN_EVENT_SHUTDOWN
			|| curEventState == VIR_DOMAIN_EVENT_CRASHED ){

		auto vm_infos_map_it = (nuiod->vm_infos_map).find(domId);
		VM_info& vm_info = vm_infos_map_it->second;
		if(vm_infos_map_it == (nuiod->vm_infos_map).end()) {
			fprintf(stderr, "Error VM:%d doesn't exist in vm_infos_map!\n", domId);
			return -1;
		}
		virDomainEventType	lastVMEventState = vm_info.vm_event_state;
		if((lastVMEventState == VIR_DOMAIN_EVENT_SHUTDOWN ||lastVMEventState == VIR_DOMAIN_EVENT_STOPPED)
			&& (curEventState == VIR_DOMAIN_EVENT_STOPPED)) {
					// if last event state is shutdown or stopped and
					// current event state is stopped, the VM has been turned off
					// For VM shutdown, remove any related information for this VM.
					/* remove the CPU_VM_MAPPING */
					auto rcpu_it = (nuiod->cpu_vm_mapping).find(vm_info.real_cpu_no);
					if(rcpu_it != (nuiod->cpu_vm_mapping).end() && 
						rcpu_it->second == vm_info.dom_id) {
						// Attention!!! If cpu_vm_mapping[vm_info.real_cpu_no] != vm_info.dom_id,
						// it means that a new mapping has overwritten the old mapping!!
						// In this case we should not erase this key!!!!!!!
						(nuiod->cpu_vm_mapping).erase(rcpu_it);
					}
					/* remove the VM from vm_infos_map */
					(nuiod->vm_infos_map).erase(vm_infos_map_it);
		} else {
			// Other states include "Reboot", "Susbend", and none of them 
			// need additional handling
			vm_info.vm_event_state = curEventState;
		}

		
	}
    return ret;
}


/*
** Get the real cpu of VM. Return -1 if failed.
*/
int Nuiod::get_vm_real_cpu(virDomainPtr dom_ptr) {

	int ret = -1;

	// Assuming a vm has only one vcpu now.

	// TODO: Donot make this assumption.

	virError vir_err;
	virVcpuInfoPtr vcpu_info = new virVcpuInfo;
	ret = virDomainGetVcpus(dom_ptr, vcpu_info, 1, NULL, 0);
	if(ret == -1) {
		virCopyLastError(&vir_err);
		fprintf(stderr, "virGetVcpus failed: %s\n", vir_err.message);
		virResetError(&vir_err);				
	} else{
		ret = vcpu_info->cpu;		
	}

	if(vcpu_info)
		delete vcpu_info;

	return ret;
}

/* main procedure of nuiod */


/*
**	Start nuiod.
*/
void Nuiod::start() {
	int ret = 0;
	ret = init();
	if(ret == -1) {
		fprintf(stderr, "Nuiod initialize failed.\n");
		exit(1);
	}

	// Test: print all the infomation.
	printf("Number of NUMA Nodes:\t%d\n", numa_number);
	printf("Number of CPU Cores:\t%d\n", cpu_number);
	printf("netdev:\t%s\n", netdev);
	printf("netdev_pci_bus_str:\t%s\n", netdev_pci_bus_str);
	printf("netdev_pci_slot_str:\t%s\n", netdev_pci_slot_str);
	printf("netdev_pci_funct_str:\t%s\n", netdev_pci_funct_str);
	printf("netdev is attated to Node:\t%d\n", netdev_numanode);
	printf("monitor_interval:\t%u second\n", monitor_interval);
	printf("io_threshold:\t%llu\n", io_threshold);
	printf("\n====================Numa latency====================\n");
	for(int i = 0; i < numa_number; i++) {
		Numa_node &node = numa_nodes[i];
		vector<pair<int, double> >::iterator vpit = node.sorted_lat_node_list.begin();
		printf("Node%d latency list:\n", i);
		for(; vpit != node.sorted_lat_node_list.end(); vpit++) {
			pair<int, double>& pir = *vpit;
			printf("to node%d, latency:%.3f\n", pir.first, pir.second);
		}
	}

	printf("\n====================VM Info====================\n");
	map<int, VM_info>::iterator mit = vm_infos_map.begin();
	for(; mit != vm_infos_map.end(); mit++) {
		VM_info &vm_info = mit->second;
		printf("dom_id:\t%d\n", vm_info.dom_id);		
		printf("vm_name:\t%s\n", vm_info.vm_name);		
		printf("vf_no:\t%d\n", vm_info.vf_no);
		printf("pid:\t%u\n", vm_info.pid);
		printf("real_cpu_no:\t%d\n", vm_info.real_cpu_no);
		printf("mem_node_distr:\n");
		for(unsigned int i = 0; i < vm_info.mem_node_distr.size(); i++) {
			printf("Node%u:\t%u pages\n", i, vm_info.mem_node_distr[i]);
		}
		printf("----------------------------------------------\n");
	}
	vector<double> cpu_usage;
	while(1) {
		if (virEventRunDefaultImpl() < 0) {
            fprintf(stderr, "Failed to run event loop: %s\n",
                    virGetLastErrorMessage());
        }
		// Refresh the cpu mapping first, because 
		// the Linux default scheduler may schedule
		// the VM to a different cpu.
		refresh_cpu_mapping();
		monitor(cpu_usage);		
		vector<Decision> acts = strategy->schedule(vm_infos_map, cpu_usage,
											numa_nodes, numa_number, netdev_numanode);		
		perform(acts, cpu_usage);
		sleep(monitor_interval);			
	}
}



int Nuiod::monitor_io() {
	int  ret = 0;
	map<int, VM_info>::iterator mit = vm_infos_map.begin();
	for(; mit != vm_infos_map.end(); mit++) {
		VM_info &vm_info = mit->second;
		int flag = ioMonitor->monitorIO(vm_info);
		if(flag != 0) ret = flag;
	}
	return ret;
}



/* Monitor all data.

** Return: true when migration is needed, false otherwise.

*/

void Nuiod::monitor(vector<double> &cpu_usage) {
	//int ret = 0;
	 monitor_io();
	cpu_monitor.get_cpu_usage(cpu_usage);
	vector<unsigned long long> freeMemVec;
	mem_monitor.get_node_freemem(this->conn, freeMemVec);
	for(unsigned i = 0; i < freeMemVec.size(); i++) {
		numa_nodes[i].free_mems = freeMemVec[i];
	}
	auto mit = vm_infos_map.begin();
	for(; mit != vm_infos_map.end(); mit++) {
		VM_info &vm_info = mit->second;		
		int ret = perfMonitor->monitorPerfEvent(vm_info);
		if(ret == -1) {
			fprintf(stderr, "error while monitor %s perf event\n", vm_info.getVMName());
		}
	}
	//if(ret != 0)
	//	need_migrate = false;
	// check the VM's CPU or memory usage
	//return need_migrate;
}



/* 
** Erase the old mapping in cpu_vm_mapping if necessary;
** Change the VM_info.real_cpuno to new_cpu;
** Insert the new cpu->VM mapping in cpu_vm_mapping.
*/
void Nuiod::change_cpu_vm_mapping(VM_info& vm_info, int new_cpu) {
	map<int, int>::iterator rcpu_it = cpu_vm_mapping.find(vm_info.real_cpu_no);
	if(rcpu_it != cpu_vm_mapping.end() && rcpu_it->second == vm_info.dom_id) {
		// Attention!!! If cpu_vm_mapping[vm_info.real_cpu_no] != vm_info.dom_id,
		// it means that a new mapping has overwritten the old mapping!!
		// In this case we should not erase this key!!!!!!!
		cpu_vm_mapping.erase(rcpu_it);
	}
	vm_info.real_cpu_no = new_cpu;
	// Cannot use 'insert' here!! Because there may be an old mapping existed!		
	cpu_vm_mapping[vm_info.real_cpu_no] = vm_info.dom_id;
}



/*
**	Refresh the cpu mapping, such as 'cpu_vm_mapping'
**  and VM_info.real_cpu_no.
*/
void Nuiod::refresh_cpu_mapping() {

	map<int, VM_info>::iterator vit = vm_infos_map.begin();
	for(; vit != vm_infos_map.end(); vit++) {
		VM_info& vm = vit->second;
		int ret = get_vm_real_cpu(vm.dom_ptr);			
		if(ret == -1 || ret == vm.real_cpu_no) 
			continue;
		/* Change the cpu_vm_mapping; */
		change_cpu_vm_mapping(vm, ret);		
	}

}


/* perform the migrate action */
void Nuiod::perform(vector<Decision>& acts, const std::vector<double> &cpu_usage) {

	// node_sorted_cpu_list[0] contains the CPU_USAGE of the CPU on this node
	vector<vector<std::pair<int, double> > > node_sorted_cpu_list;
	// Sort each node's cpu list according to cpu_usage in ascending order.
	for(int i = 0; i < numa_number; i++) {
		/*  A sorted list according to cpu usage in ascending order.
	   		pair->first=cpu id, pair->second=cpu usage.
 		*/
		vector<std::pair<int, double> > 	sorted_cpu_list;
		const Numa_node &node = numa_nodes[i];
		for(unsigned cpuidx = 1; cpuidx < cpu_usage.size(); cpuidx++) {
			unsigned cpuid = cpuidx - 1;
			const set<int>& node_cpu_ids = node.getCPUIDsOfThisNode();
			if(node_cpu_ids.find(cpuid) == node_cpu_ids.end())
				continue;
			sorted_cpu_list.push_back(make_pair(cpuid, cpu_usage[cpuidx]));
		}
		std::sort(sorted_cpu_list.begin(), sorted_cpu_list.end(), 
					compare_pairs_second<int,double>);
		node_sorted_cpu_list.push_back(sorted_cpu_list);
	}

	for(vector<Decision>::iterator ait = acts.begin(); ait != acts.end(); ait++) {
		Decision &act = *ait;
		VM_info &vm_info = vm_infos_map[act.dom_id];
		int cpu_old_node = cpu_node_mapping[vm_info.real_cpu_no];
		if(cpu_old_node == act.cpu_node_id) {
			//printf("%s cpu_old_node:%d target_cpunode:%d\n", vm_info.vm_name,
			//	cpu_old_node, act.cpu_node_id);
			continue;
		}

		// Traverse the node's sorted_cpu_list, and
		// pick the first cpu which is not assigned to other VM.
		int target_cpuno = -1;
		const vector<pair<int, double> > &sorted_cpu_list = node_sorted_cpu_list[act.cpu_node_id];
		auto pit = sorted_cpu_list.begin();
		while(pit != sorted_cpu_list.end()) {
			int cpuno = (*pit).first;
			map<int, int>::iterator cvm_it = cpu_vm_mapping.find(cpuno);
			// Is there a VM on this cpu already?
			if(cvm_it == cpu_vm_mapping.end()) {
				// TODO:when this cpu's usage is higher than 50%(or other value),
				// We should skip this cpu.
				// found one
				target_cpuno = cpuno;
				break;
			} 
			pit++;
		}

		if(target_cpuno == -1)
			continue;

		printf("move %s cpu to %d\n", vm_info.vm_name, target_cpuno);
		// We are free to migrate the vcpu of this VM
		// to target_cpuno.
		vector<unsigned int> target_nodes;
		target_nodes.push_back((unsigned int)target_cpuno);
		int ret = thread_migrate.pin_vcpu_to(vm_info.dom_ptr, 0, target_nodes);
		if(ret == -1) {
			fprintf(stderr, "pin vcpu of VM%d to cpu%d failed.\n", vm_info.dom_id, target_cpuno);						
		} else{
			/* modify the mapping */
			change_cpu_vm_mapping(vm_info, target_cpuno);				
		}

	}

}





/****** numa information related ************/



/* the function is defined at mgen.cpp 

** Attention: the from_node is the numa node,  but the

**	target_cpu_node is the cpu number

*/

extern double get_numa_latency(int from_node, int target_cpu_node);



/* return -1 if failed, 0 if succeed */

int Nuiod::init_numa_info() {

	/* initialize the numa node info */
	numa_nodes = new Numa_node[numa_number];
	for(int i = 0; i < numa_number; i++) {
		numa_nodes[i].node_id = i;
		int ret = get_node_cpus(i, numa_nodes[i].cpu_ids);
		if(ret < 0) {
			fprintf(stderr, "get_node_cpus failed,err:%d\n", ret);
			return -1;
		}

		set<int>::const_iterator cit = numa_nodes[i].cpu_ids.begin();
		while(cit != numa_nodes[i].cpu_ids.end()) {
			//numa_nodes[i].cpu_occupyed_by_VM.insert(make_pair(*cit, -1));			
			// Add the mapping of cpu and numa node.
			cpu_node_mapping.insert(make_pair(*cit, i));			
			cit++;
		}
	}


	init_numa_latency();
	// Set the sorted_lat_node_list
	for(int i = 0; i < numa_number; i++) {		
		Numa_node &node = numa_nodes[i];
		for(int j = 0; j < numa_number; j++) {
			int target_node = j;			
			node.sorted_lat_node_list.push_back(make_pair(target_node, numa_latency[i][target_node]));
		}
		std::sort(node.sorted_lat_node_list.begin(), node.sorted_lat_node_list.end(),
				compare_pairs_second<int, double>);

	}

	

	return 0;

}



/* initialize the numa_latency matrix  

   return -1 if failed, 0 if succeed.

*/

int Nuiod::init_numa_latency() {	

	numa_latency = new double*[numa_number];
	for(int i = 0; i < numa_number; i++)
		numa_latency[i] = new double[numa_number];

	/* First check whether /etc/nuiod.letency exits. If this file exits,
		we don't have to calculate the latency.
	*/
	const char* latency_path = "/etc/nuiod.latency";

	FILE* file = fopen(latency_path, "r");
	if(file == NULL) {
		cal_and_create_numa_lat();		
	} else{
		/* Read the latency from file. */
		// the first number should be the numa number, and it shuold equal
		// to numa_number.
		int numa_num = 0;
		fscanf(file, "%d", &numa_num);
		if(numa_num != numa_number) {
			// If it doesnot match, calculate again.
			fclose(file);
			cal_and_create_numa_lat();

		} else{

			for(int i = 0; i < numa_number; i++) {			
				for(int j = 0; j < numa_number; j++) {
					fscanf(file, "%lf ", &numa_latency[i][j]);								
				}				
			}
			fclose(file);
		}
	}

	return 0;

}



void Nuiod::cal_and_create_numa_lat() {

	/* It's hard to use multi-thread to calculate the numa_latency matrix,
	** because calculating in parallel may cause memory contention.
	*/	
	int *cpu_set = new int[numa_number];

	for(int i = 0; i < numa_number; i++) {

		cpu_set[i] = get_node_first_cpu(i);

	}
	int from_node = 0, target_cpu_node = 0;
	for(int i = 0; i < numa_number; i++) {
		from_node = i;
		for(int j = 0; j < numa_number; j++) {

			/* target node is the cpu node number */
			target_cpu_node = cpu_set[j];			
			numa_latency[i][j] = get_numa_latency(from_node, target_cpu_node);		

		}

	}

	delete[] cpu_set;

	// create the latency file.

	const char* latency_path = "/etc/nuiod.latency";	

	FILE* file = fopen(latency_path, "w");

	if(file == NULL) {
		fprintf(stderr, "cannot open %s, error:%s\n", latency_path, strerror(errno));
	} else{

		// the first line is the number of numa node.
		fprintf(file, "%d\n", numa_number);

		// then we print the numa_latency

		for(int i = 0; i < numa_number; i++) {			
			for(int j = 0; j < numa_number; j++) {
				fprintf(file, "%f ", numa_latency[i][j]);								
			}
			fprintf(file, "\n");

		}
		fclose(file);

	}

}



/*
**	get the first cpu number of node
*/

int Nuiod::get_node_first_cpu(const int node)

{

	int err;
	struct bitmask *cpus;
	cpus = numa_allocate_cpumask();
	err = numa_node_to_cpus(node, cpus);
	if (err >= 0) {
		for (unsigned int i = 0; i < cpus->size; i++)
			if (numa_bitmask_isbitset(cpus, i)){
				numa_free_cpumask(cpus);
				return i;
			}
	}
	numa_free_cpumask(cpus);
	return 0;
}



/*
**	return negative number if failed, 0 if succedd
**	Notice: will clear the set when using
*/
int Nuiod::get_node_cpus(const int node, std::set<int> &cpu_ids) {
	cpu_ids.clear();
	int err;
	struct bitmask *cpus;



	cpus = numa_allocate_cpumask();
	err = numa_node_to_cpus(node, cpus);
	if (err >= 0) {
		for (unsigned int i = 0; i < cpus->size; i++)
			if (numa_bitmask_isbitset(cpus, i))
				cpu_ids.insert(i);
		return 0;
	}else{
		return err;
	}

}
