#ifndef NUMA_NODE_H
#define NUMA_NODE_H


/*
**	Representing a numa node in system.	
**	-------- Written by Jenson
*/
#include "mgen.h"
#include "numa.h"
#include <vector>
#include <set>
#include <map>
#include <errno.h>
#include <unistd.h>
#include "Nuiod.h"


/*
** Containing the cpu list and the amount of free memory.
*/
class Numa_node {
public:
	const std::set<int>& 						getCPUIDsOfThisNode() const {return cpu_ids;}
	unsigned long long 							getFreeMemoryInBytes()const { return free_mems;}
	/* A sorted list according to memory latency in ascending order.
	   pair->first=numa node number, 
	   pair->second=latency between current node and the target node.	  
 	*/
	const std::vector<std::pair<int, double> >& getLatBetweenOtherNodes() const { return sorted_lat_node_list;}
private:
	friend class 							Nuiod;
	// which numa node it is
	int										node_id;

	// the physical cpu number in this numa node
 	std::set<int> 							cpu_ids;

 	/* A sorted list according to memory latency in ascending order.
	   pair->first=numa node number, 
	   pair->second=latency between current node and the target node.	  
 	*/
 	std::vector<std::pair<int, double> > 	sorted_lat_node_list;

 	// free memory in bytes of this node.
 	unsigned long long 						free_mems;

 	Numa_node() {
 		node_id   = -1;
 		free_mems = 0;
 	}
};





#endif
