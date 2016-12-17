#ifndef MEM_MIGRATE_H

#define MEM_MIGRATE_H



#include <string.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <numa.h>



class Memory_migrate {
public:

	Memory_migrate(int numa_num):numa_number(numa_num) {
		old_nodes_mask = numa_bitmask_alloc(numa_num);
		new_nodes_mask = numa_bitmask_alloc(numa_num);
	}


	~Memory_migrate() {
		numa_bitmask_free(old_nodes_mask);
		numa_bitmask_free(new_nodes_mask);
	}

	// migrate old_nodes' memory to new nodes
	int migrate_mem_to(const unsigned int pid, const std::vector<unsigned int> &old_nodes, const std::vector<unsigned int> &new_nodes);


private:
	int					numa_number;
	struct bitmask 		*old_nodes_mask;
	struct bitmask 		*new_nodes_mask;

};









#endif