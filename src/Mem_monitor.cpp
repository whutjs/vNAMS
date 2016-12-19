#include "Mem_monitor.h"

using namespace std;

const int Mem_monitor::BUF_SIZE;

int Mem_monitor::get_node_freemem(const virConnectPtr conn, 
		vector<unsigned long long> &free_mem) {
	if(conn == NULL) {
		fprintf(stderr, "virConnectPtr is NULL\n");
		return -1;
	}
	int ret = virNodeGetCellsFreeMemory(conn, free_mems, 0, numa_number);
	if(ret < 0) {
		virError err;
		virCopyLastError(&err);
		fprintf(stderr, "virNodeGetCellsFreeMemory failed: %s\n", err.message);
		return -1;
	}
	/* if virNodeGetCellsFreeMemory went well, ret should be 
	   the number of entries filled in freeMems.
	*/
	for(int i = 0; i < ret; i++) {
		free_mem.push_back(free_mems[i]);
	}
	return 0;
}

/*
** Get the memory page distribution in each numa node for a process.
** 
** When return with 0, the node_pages contain the number of pages
** in each node. For example, node_pages[0]=10 means there are 10
** pages in node0.
** return -1 if failed.
*/
int Mem_monitor::get_proc_mem_numa_distribution(const unsigned int pid, 
						std::vector<unsigned int> &node_pages) {
	char file_name[BUF_SIZE];
	snprintf(file_name, BUF_SIZE, "/proc/%d/numa_maps", pid);

	FILE* file = fopen(file_name, "r");
	if (file == NULL) {
	    fprintf(stderr, "Tried to open PID %d numamaps, but it apparently went away.\n", pid);
		return -1;  // Assume the process terminated?
	}
	node_pages.clear();
	// push 'numa_number' elements in vector first.
	for(int i = 0; i < numa_number; i++)
		node_pages.push_back(0);

	while(fgets(buf, BUF_SIZE, file)) {
		const char* delimiters = " \t\r\n";
		char *p = strtok(buf, delimiters);
		while(p) {
			if(p[0] == 'N') {
				int node = (int)strtol(&p[1], &p, 10);
				if(node >= numa_number) {
					fprintf(stderr, "numa_maps node number parse error:node %d is too large\n", node);
					fclose(file);
					return -1;
				}
				if(p[0] != '=') {
					fprintf(stderr, "numa_maps node number parse error\n");
					fclose(file);
					return -1;
				}
				unsigned int pages = (unsigned int)strtol(&p[1], &p, 10);				
				node_pages[node] += (pages);
			}
			// get next token on the line
			p = strtok(NULL, delimiters);
		}
	}

	return 0;
}
