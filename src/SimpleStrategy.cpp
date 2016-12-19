#include "SimpleStrategy.h"
#include "utils.h"
#include <list>
#include <set>
#include <cstdio>

using namespace std;

vector<Decision> SimpleStrategy::schedule(const std::map<int, VM_info> &vmInfo, 
				const std::vector<double> &physicalCpuUsage, const Numa_node* nodeInfos,
				const int numOfNode, const int netdev_numanode)
{
	// log information
	for(int i = 0; i < numOfNode; i++) {
		const std::set<int>& cpuOfNode = nodeInfos[i].getCPUIDsOfThisNode();
		double nodeTotalCPUUsage = 0;
		for(auto cpuid : cpuOfNode) {
			nodeTotalCPUUsage += physicalCpuUsage[cpuid+1];
		}
		unsigned long long freeMemInMB = nodeInfos[i].getFreeMemoryInBytes()/1024/1024;
		printf("Node:%d\nTotal CPU Usage:%.3f%%\t Free Memory:%llu MB\n", i, nodeTotalCPUUsage, freeMemInMB);
	}
	vector<Decision> decisions;
	analyze(vmInfo, physicalCpuUsage, nodeInfos, numOfNode, netdev_numanode, decisions);
	return decisions;
}

/*
**	Analyze the IO state for VM
**
*/
void SimpleStrategy::analyzeIOState(const VM_info &vm_info, 
								map<int, VM_State>& vmStateMap) {
		// Smoothed estimator
		double factor = 1;
		double new_pps_val = vm_info.getNewPacketsPerSec();
		double new_kbps_val = vm_info.getNewKBPerSec();
		double old_pps_val = vm_info.getHistoricalPacketsPerSec();
		//double old_kbps_val = vm_info.getHistoricalKBPerSec();
		if(new_pps_val < old_pps_val)
			factor = SMOOTH_DEC_FACTOR;
		else
			factor = SMOOTH_FACTOR;

		new_pps_val = (old_pps_val * factor) + new_pps_val * (1.0 - factor);
		new_kbps_val = (old_pps_val * factor) + new_kbps_val * (1.0 - factor);
		
		//printf("%s packets_per_sec:%lf\n", vm_info.vm_name, vm_info.packets_per_sec);
		if(new_pps_val > io_threshold) {
			// TODO: should use a more efficient way to
			// determine whether is io intensive or not.
			// And a way to determine when to clear
			// the io intensive flag.
			vmStateMap[vm_info.getVMID()] = IO_STAT;
			printf("%s packets_per_sec:%lf becomes io intensive\n", vm_info.getVMName(), new_pps_val);
		} else if(new_pps_val < io_threshold) {
			//if(vm_info.vm_state != OTHER_STAT) {				
			//	status_changed = true;
			//}
			vmStateMap[vm_info.getVMID()] = OTHER_STAT;
			//printf("%s packets_per_sec:%lf is not io intensive\n", vm_info.vm_name, vm_info.packets_per_sec);
		}

}

/* Analyze the monitoring data and make migration decision. 
** The decision results are save in the vector.
*/
void SimpleStrategy::analyze(const std::map<int, VM_info> &vm_infos_map,
				const std::vector<double> &cpu_usage,
				const Numa_node* numa_nodes,
				const int numa_number,
				const int netdev_numanode,
				std::vector<Decision>& acts){
	acts.clear();
	/* Count the number of VM on each Node. */
	vector<list<int> > vm_on_node;	// e.g. vm_on_node[0] is the list of vm's id on node0
	for(int i = 0; i < numa_number; i++) {
		list<int> vm_id_ls;
		vm_on_node.push_back(vm_id_ls);
	}

	bool flag = false;		
	// key=VM_ID value = vmstates
	map<int, VM_State> vmStateMap;
	auto vit = vm_infos_map.begin();
	while(vit != vm_infos_map.end()) {
		const VM_info &vm_info = vit->second;
		analyzeIOState(vm_info, vmStateMap);
		// if the cpu usage of this vm is lower than 8%,
		// we think this vm is in idle.
		int vm_real_cpu = vm_info.getPhysicalCPUNo();
		double new_cpu_usage = cpu_usage[vm_real_cpu + 1];
		double factor = 1;
		if(new_cpu_usage < vm_info.getPhysicalCPUUsage())
			factor = SMOOTH_DEC_FACTOR;
		else
			factor = SMOOTH_FACTOR;

		double vm_new_cpu_usage = factor*vm_info.getPhysicalCPUUsage() + (1-factor)*new_cpu_usage;
		VM_State	vm_state;
		if(vm_new_cpu_usage <= 8) {
			vm_state = IDLE_STAT;
			vmStateMap[vm_info.getVMID()] = IDLE_STAT;	
		}
		//printf("%s state:%d\n", vm_info.vm_name, vm_info.vm_state);
		if(vm_state == IDLE_STAT) {
			// do not count the idle vm.
			//printf("%s state:%d\n", vm_info.vm_name, vm_info.vm_state);
			vit++;
			continue;
		}

		flag = true;
		// Take the node which the  VM's major memory resides on as home node.
		list<int> &vm_id_ls = vm_on_node[vm_info.getMajorMemResideOnNode()];
		vm_id_ls.push_back(vm_info.getVMID());
		vit++;

	}

	if(!flag)
		return;

	/* Now refine the decision. */
	// sort the node_vm_vec
	// key = node id, value = the amount of VM on this node
	vector<pair<int, int> > node_vm_vec;
	for(int node_id = 0; node_id < numa_number; node_id++)
		node_vm_vec.push_back(make_pair(node_id, vm_on_node[node_id].size()));

	std::sort(node_vm_vec.begin(), node_vm_vec.end(), compare_pairs_second<int, int>);
	int max_vm_idx = node_vm_vec.size() - 1;
	int high_load_node = node_vm_vec[max_vm_idx].first;
	unsigned max_vm_cnt = node_vm_vec[max_vm_idx].second;
	unsigned min_vm_cnt = node_vm_vec[0].second;
	int min_load_node = node_vm_vec[0].first;

	flag = false;
	while(max_vm_cnt > 1 && (max_vm_cnt - min_vm_cnt) > 1) {
		flag = true;
		// Load balance.
		// if the highest load node is netdev node but has no more than 2 VM
		if(high_load_node == netdev_numanode && max_vm_cnt <= 2) {
			int vm_id = -1;
			list<int> &vm_id_ls = vm_on_node[high_load_node];					
			list<int>::iterator vm_it = vm_id_ls.begin();
			// search for memory intensive VM and migrate it
			for(; vm_it != vm_id_ls.end(); vm_it++) {
				const VM_info& vm_info = vm_infos_map.at(*vm_it);
				VM_State vm_state = vmStateMap[vm_info.getVMID()];
				if(vm_state == OTHER_STAT) {
					vm_id = *vm_it;
					break;
				}
			}
			//printf("VM id:%d is memory intensive\n", vm_id);
			if(vm_id == -1) {
				// All VMs are IO intensive. Just skip this node and move to next node.
				max_vm_idx--;
				if(max_vm_idx < 0)
					max_vm_idx = 0;
				max_vm_cnt = node_vm_vec[max_vm_idx].second;
				high_load_node = node_vm_vec[max_vm_idx].first;
				min_vm_cnt = node_vm_vec[0].second;
				continue;
			} else{
				// Move this memory intensive VM to adjacent node.
				//const VM_info& vm_info = vm_infos_map.at(vm_id);
				// Search from nearest node of high load node
				const Numa_node &node = numa_nodes[high_load_node];
				auto vpit = node.getLatBetweenOtherNodes().begin();					
				for(; vpit != node.getLatBetweenOtherNodes().end(); vpit++) {
					const pair<int, double>& pir = *vpit;
					if(vm_on_node[pir.first].size() == min_vm_cnt) {
						min_load_node = pir.first;
						break;
					}						
				}
				//printf("move %s to node:%d\n", vm_info.vm_name, min_load_node);
				vm_id_ls.erase(vm_it);
				vm_on_node[min_load_node].push_back(vm_id);
			}

		} else{
			// Migrate the IO intensive VM first.
			int vm_id = -1;
			list<int> &vm_id_ls = vm_on_node[high_load_node];					
			list<int>::iterator vm_it = vm_id_ls.begin();
			// search for memory intensive VM and migrate it
			for(; vm_it != vm_id_ls.end(); vm_it++) {
				const VM_info& vm_info = vm_infos_map.at(*vm_it);
				VM_State vm_state = vmStateMap[vm_info.getVMID()];
				if(vm_state == IO_STAT) {
					vm_id = *vm_it;
					break;
				}
			}

			if(vm_id != -1) {
				// Found a IO intensive VM.
				// Now decide which node to move this vm to.				
				// Check VM number on netdev node first
				if(netdev_numanode != high_load_node &&
					vm_on_node[netdev_numanode].size() == min_vm_cnt) {
					// if the number of vm on netdev node equals to min_vm_cnt,
					// we choose netdev node first.
					// Remove from old list, insert into target node list.
					vm_id_ls.erase(vm_it);
					vm_on_node[netdev_numanode].push_back(vm_id);
				} else{
					// Search from nearest node of high load node
					const Numa_node &node = numa_nodes[high_load_node];
					auto vpit = node.getLatBetweenOtherNodes().begin();					
					for(; vpit != node.getLatBetweenOtherNodes().end(); vpit++) {
						const pair<int, double>& pir = *vpit;
						if(vm_on_node[pir.first].size() == min_vm_cnt) {
							min_load_node = pir.first;
							break;
						}						

					}
					vm_id_ls.erase(vm_it);
					vm_on_node[min_load_node].push_back(vm_id);
				}

			} else{
				// Does not have a IO intensive VM. Just migrate
				// the first memory intensive VM to adjacent node.
				vm_it = vm_id_ls.begin();
				vm_id = *vm_it;
				// Search from nearest node of high load node
				const Numa_node &node = numa_nodes[high_load_node];
				auto vpit = node.getLatBetweenOtherNodes().begin();					
				for(; vpit != node.getLatBetweenOtherNodes().end(); vpit++) {
					const pair<int, double>& pir = *vpit;
					if(vm_on_node[pir.first].size() == min_vm_cnt) {
						min_load_node = pir.first;
						break;
					}						
				}
				vm_id_ls.erase(vm_it);
				vm_on_node[min_load_node].push_back(vm_id);
			}
			// construct the vector again
			node_vm_vec.clear();
			for(int node_id = 0; node_id < numa_number; node_id++)
				node_vm_vec.push_back(make_pair(node_id, vm_on_node[node_id].size()));
			std::sort(node_vm_vec.begin(), node_vm_vec.end(), compare_pairs_second<int, int>);
			max_vm_idx = node_vm_vec.size() - 1;
			high_load_node = node_vm_vec[max_vm_idx].first;
			max_vm_cnt = node_vm_vec[max_vm_idx].second;
			min_vm_cnt = node_vm_vec[0].second;
			min_load_node = node_vm_vec[0].first;
		}
	}
	if(!flag) {
		// In some case, while(max_vm_cnt > 1 && (max_vm_cnt - min_vm_cnt) > 1)
		// may be false, but the netdev node may be free. For example,
		// there is only one VM running, but not on the netdev node.
		// For example, there are 4 nodes in my system, and only this case will skip above "while":
		// 0 1 0 0
		if(max_vm_cnt == 1 && vm_on_node[netdev_numanode].size() == 0) {
			// move the VM's vcpu of high load node to netdev node.
			list<int> &vm_id_ls = vm_on_node[high_load_node];
			// Just pick the first vm to move.
			list<int>::iterator vm_it = vm_id_ls.begin();
			int vm_id = *vm_it;

			const VM_info& vm_info = vm_infos_map.at(vm_id);
			VM_State vm_state = vmStateMap[vm_info.getVMID()];
			if(vm_state == IO_STAT) {
				vm_id_ls.erase(vm_it);
				vm_on_node[netdev_numanode].push_back(vm_id);
			}
		}

	}

	// Now we have the result list.
	printf("%s\n", "Decision:");
	for(unsigned node_id = 0; node_id < vm_on_node.size(); node_id++) {	
		list<int>& vm_ls = vm_on_node[node_id];
		for(list<int>::iterator lit = vm_ls.begin(); lit != vm_ls.end(); lit++) {
			Decision act;
			act.dom_id = *lit;
			act.cpu_node_id = node_id;
			act.mem_node_id = node_id;
			acts.push_back(act);
			printf("vm id:%d target_cpuno:%d\n", *lit, node_id);
		}		
	}

}
