#ifndef THREAD_MIGRATE_H
#define THREAD_MIGRATE_H



#include <string.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>



/*
**	In fact, we just migrate the vcpu of VM currently....
**  ------written by Jenson
*/

class Thread_migrate {
public:
	Thread_migrate(int numa_num, int cpu_num):numa_number(numa_num), cpu_number(cpu_num) {
		// each 'unsigned char' can represent 8 cpus.
		cpumaps_len = (cpu_number / 8 + 1);
		cpumaps = new unsigned char[cpumaps_len];
	}

	~Thread_migrate() {
		if(cpumaps != NULL)
			delete[] cpumaps;
	}



	int	pin_vcpu_to(const virDomainPtr, const unsigned int vcpu, 
		const std::vector<unsigned int> &target_node);



private:

	int				numa_number;
	int 			cpu_number;
	int				cpumaps_len;
	// cpumask for libvirt virDomainPinVcpu.
	unsigned char	*cpumaps;

};







#endif