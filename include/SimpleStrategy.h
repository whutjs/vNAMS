#ifndef _SIMPLE_STRATEGY_
#define _SIMPLE_STRATEGY_

#include "ScheduleStrategy.h"
#include <utility> 

// The smoothing factor for calculating 'packets_per_sec'.
#define  SMOOTH_FACTOR 0.4
#define  SMOOTH_DEC_FACTOR 0.85

// the state of a VM, which include
// I/O intensive state, idle state and other state(CPU or Memory intensive).
enum VM_State{IO_STAT, OTHER_STAT, IDLE_STAT};
/*
** Simple Strategy demo.
*/
class SimpleStrategy: public ScheduleStrategy {
public:	
	SimpleStrategy():io_threshold(100000) {}
	virtual ~SimpleStrategy(){}
	std::vector<Decision> schedule(const std::map<int, VM_info> &vmInfoMap, 
				const std::vector<double> &physicalCpuUsage, const Numa_node* nodeInfos,
				const int numOfNode, const int NIC_NODE_ID);
private:
	void	analyze(const std::map<int, VM_info> &vmInfoMap,
				const std::vector<double> &physicalCpuUsage,
				const Numa_node* nodeInfos,
				const int numOfNode,
				const int NIC_NODE_ID,
				std::vector<Decision>&);

	void 	analyzeIOState(const VM_info &vmInfo,
						std::map<int, VM_State>& vmStateMap);

	int  	io_threshold;
};

#endif