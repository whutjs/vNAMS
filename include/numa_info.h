#ifndef NUMA_NODE_H
#define NUMA_NODE_H

/*
**	Represent a numa node in system.	
**	-------- Written by Jenson
*/

#include "cpu_monitor.h"
#include "mgen.h"
#include "numa.h"
#include <vector>
#include <set>
#include <map>
#include <errno.h>
#include <unistd.h>
#include <utility> 

/*
** Containing the cpu list and the amount of free memory.
*/
struct Numa_node {
	// which numa node it is
	int										node_id;
	// the real cpu number in this numa node
 	std::set<int> 							cpu_ids;
 	/* A sorted list according to cpu usage in ascending order.
	   pair->first=cpu id, pair->second=cpu usage.
 	*/
 	std::vector<std::pair<int, double> > 	sorted_cpu_list;

 	/* A sorted list according to memory latency in ascending order.
	   pair->first=numa node number, 
	   pair->second=latency between this node and the target node.
 	*/
 	std::vector<std::pair<int, double> > 	sorted_lat_node_list;

 	/* key=real cpu id;
 	   value=the id of  VM which occupy this cpu. 
 	   value=-1 means this cpu is free.
 	*/
 	std::map<int,int>						cpu_occupyed_by_VM;

 	/*  The memory latency list of this node, which
 		is ordered by ascent.
	   For example, ascent_latency_list[0] is the numa
	   node which is most closer to this node.
 	*/
 	std::vector<int> 						ascent_latency_nodelist;

 	// free memory in bytes of this node.
 	unsigned long long 						free_mems;

 	Numa_node() {
 		node_id   = -1;
 		free_mems = 0;
 	}
};


#endif