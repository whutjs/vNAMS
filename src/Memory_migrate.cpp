#include "Memory_migrate.h"

using namespace std;

/*
** migrate old_nodes' memory to new nodes.
** On success migrate_pages() returns the number of pages that could not be moved 
** (i.e., a return of zero means that all pages were successfully moved). 
** On error, it returns -1, and sets errno to indicate the error.
*/
int Memory_migrate::migrate_mem_to(const unsigned int pid, const std::vector<unsigned int> &old_nodes, 
	const std::vector<unsigned int> &new_nodes) {
	if(old_nodes.size() == 0 || new_nodes.size() == 0)
		return -1;
	numa_bitmask_clearall(old_nodes_mask);
	numa_bitmask_clearall(new_nodes_mask);

	for(unsigned int i = 0; i < old_nodes.size(); i++) 
		numa_bitmask_setbit(old_nodes_mask, old_nodes[i]);

	for(unsigned int i = 0; i < new_nodes.size(); i++) 
		numa_bitmask_setbit(new_nodes_mask, new_nodes[i]);

	int rc = numa_migrate_pages(pid, old_nodes_mask, new_nodes_mask);
	if(rc < 0) {
		perror("migrate_mem_to failed");
		return -1;
	}
	return rc;
}