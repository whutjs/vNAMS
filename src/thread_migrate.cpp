#include "thread_migrate.h"

using namespace std;

/*
**	pin the vcpu of domain to target node.
**	return 0 if succeed, or -1 if failed.
*/
int	Thread_migrate::pin_vcpu_to(const virDomainPtr domain, const unsigned int vcpu, 
		const vector<unsigned int> &target_node) {
	// cpumaps[0]:0~7
	// cpumaps[1]:8~15
	// cpumaps[2]:16~23
	// cpumaps[3]:24~31

	// clear the cpumaps.
	memset(cpumaps, 0, sizeof(unsigned char)*cpumaps_len);

	// translate the target_node to cpumaps
	for(unsigned int i = 0; i < target_node.size(); i++) {
		unsigned int node = target_node[i];
		// calculate which cpumaps and which bit is 
		unsigned int cpumaps_idx = node / 8;
		unsigned int offset = node % 8;
		unsigned int bit_mask = 1;
		bit_mask = bit_mask << offset;
		// set the bit on
		cpumaps[cpumaps_idx] |= bit_mask;
	}
	virError err;
	int ret = virDomainPinVcpu(domain, vcpu, cpumaps, cpumaps_len);
	if(ret == -1) {
		virCopyLastError(&err);
		fprintf(stderr, "virDomainPinVcpu failed: %s\n", err.message);
		return -1;	
	}

	return 0;
}