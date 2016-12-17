#ifndef _SCHEDULESTRATEGY_H
#define _SCHEDULESTRATEGY_H

#include "Decision.h"
#include "VM_info.h"
#include "Numa_node.h"
#include <vector>
#include <map>

class VM_info;
/*
** Schedule Strategy interface.
** User should implement the schedule method.
** Author: Jenson
*/
class ScheduleStrategy{
public:
	/*
	** Nuiod will schedule the VM based on the decisions return by this method.
	** @Argument: vmInfoMap the VM information, key=VM_ID, value=VM_info
	** @Argument: physicalCpuUsage contains each physical cpu usage.
	** @Argument: nodeInfos contains the informaction of each NUMA node.
	** @Argument: numOfNode the size of nodeInfos.
	** @Argument: NIC_node_id the NUMA node which the NIC attach to
	*/
	virtual std::vector<Decision> schedule(const std::map<int, VM_info> &vmInfoMap, 
				const std::vector<double> &physicalCpuUsage, const Numa_node* nodeInfos,
				const int numOfNode, const int NIC_node_id) = 0;
	virtual ~ScheduleStrategy(){};
};

#endif