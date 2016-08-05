#include "cpu_monitor.h"

using namespace std;

const int Cpu_monitor::BUF_SIZE;

/*
** init all data; should be called before call other functions
** return: 0 if succeed; -1 if failed
*/
int Cpu_monitor::init_cpu_monitor() {	
	cpu_stat_array = new Cpu_stat[cpu_number + 1];
	if(cpu_stat_array == NULL) {
		fprintf(stderr, "%s\n", "out of memory");
		return -1;
	}
	FILE* file = fopen("/proc/stat", "r");
	if(file == NULL) {
		perror("cannot open /proc/stat");
		return -1;
	}
	int cnt = 0;
	while(fgets(buf, sizeof(buf), file) != NULL) {
		if(strstr(buf, "cpu") == NULL)
			break;
		Cpu_stat *cpu_info_ptr = &cpu_stat_array[cnt];
		if(cnt == 0) {
			sscanf(buf, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &(cpu_info_ptr->total_user),
				&(cpu_info_ptr->total_user_low), &(cpu_info_ptr->total_sys), &(cpu_info_ptr->total_idle),
				&(cpu_info_ptr->total_iowait), &(cpu_info_ptr->total_irq), &(cpu_info_ptr->total_softirq),
				&(cpu_info_ptr->total_steal), &(cpu_info_ptr->total_guest), &(cpu_info_ptr->total_guest_nice));
			
		}else{
			int tmp = 0;
			sscanf(buf, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &tmp, &(cpu_info_ptr->total_user),
				&(cpu_info_ptr->total_user_low), &(cpu_info_ptr->total_sys), &(cpu_info_ptr->total_idle),
				&(cpu_info_ptr->total_iowait), &(cpu_info_ptr->total_irq), &(cpu_info_ptr->total_softirq),
				&(cpu_info_ptr->total_steal), &(cpu_info_ptr->total_guest), &(cpu_info_ptr->total_guest_nice));
		}
		cnt++;
	}
	fclose(file);

	return 0;
}

/*
** The vector argument will be filled with the cpu usage.
** Notice: the vector will be cleared before using.
** usage: usage[0] contains the usages of total cpu. 
** usage[1] contains the cpu0 usage.
** return: -1 if faile, 0 if succeed
*/
int Cpu_monitor::get_cpu_usage(std::vector<double> &usage) {
	if(cpu_stat_array == NULL) {
		fprintf(stderr, "%s\n", "Cpu_monitor is not initialized");
		return -1;
	}
	usage.clear();
	FILE* file = fopen("/proc/stat", "r");
	if(file == NULL) {
		perror("cannot open /proc/stat");
		return -1;
	}
	
	unsigned long long      total_user;
 	unsigned long long      total_user_low;
	unsigned long long      total_sys;
	unsigned long long      total_idle;
  	unsigned long long      total_iowait;
	unsigned long long      total_irq;
	unsigned long long      total_softirq;
    unsigned long long      total_steal;
	unsigned long long      total_guest;
	unsigned long long      total_guest_nice;
	unsigned long long      total;
										 
	int cnt = 0;

	while(fgets(buf, sizeof(buf), file) != NULL) {
		if(strstr(buf, "cpu") == NULL)
			break;
		Cpu_stat *cpu_info_ptr = &cpu_stat_array[cnt];
		
		total_user = cpu_info_ptr -> total_user;
		total_user_low = cpu_info_ptr -> total_user_low;
		total_sys = cpu_info_ptr -> total_sys;
		total_idle = cpu_info_ptr -> total_idle;
		total_iowait = cpu_info_ptr -> total_iowait;
		total_irq = cpu_info_ptr -> total_irq;
		total_softirq = cpu_info_ptr -> total_softirq;
		total_steal = cpu_info_ptr -> total_steal;
		total_guest = cpu_info_ptr -> total_guest;
		total_guest_nice = cpu_info_ptr -> total_guest_nice;

		if(cnt == 0) {
			sscanf(buf, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &(cpu_info_ptr->total_user),
				&(cpu_info_ptr->total_user_low), &(cpu_info_ptr->total_sys), &(cpu_info_ptr->total_idle),
				&(cpu_info_ptr->total_iowait), &(cpu_info_ptr->total_irq), &(cpu_info_ptr->total_softirq),
				&(cpu_info_ptr->total_steal), &(cpu_info_ptr->total_guest), &(cpu_info_ptr->total_guest_nice));
			
		}else{
			int tmp = 0;
			sscanf(buf, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu", &tmp, &(cpu_info_ptr->total_user),
				&(cpu_info_ptr->total_user_low), &(cpu_info_ptr->total_sys), &(cpu_info_ptr->total_idle),
				&(cpu_info_ptr->total_iowait), &(cpu_info_ptr->total_irq), &(cpu_info_ptr->total_softirq),
				&(cpu_info_ptr->total_steal), &(cpu_info_ptr->total_guest), &(cpu_info_ptr->total_guest_nice));
		}
		
		if(cpu_info_ptr->total_user < total_user || cpu_info_ptr->total_sys < total_sys
		  || cpu_info_ptr->total_guest < total_guest) {
			/* overflow detection. Just skip this value */
			cpu_info_ptr->usage = 0;
		}else {
			double percent = 0;
			total = 0;
			/* calculate cpu usage */
			total += cpu_info_ptr -> total_user - total_user;
			total += cpu_info_ptr -> total_user_low - total_user_low;
			total += cpu_info_ptr -> total_sys - total_sys;
			total += cpu_info_ptr -> total_iowait - total_iowait;
			total += cpu_info_ptr -> total_irq - total_irq;
			total += cpu_info_ptr -> total_softirq - total_softirq;
			total += cpu_info_ptr -> total_steal - total_steal;
			total += cpu_info_ptr -> total_guest - total_guest;
			total += cpu_info_ptr -> total_guest_nice - total_guest_nice;
			percent = total;
			total += cpu_info_ptr -> total_idle - total_idle;
			percent /= total;
			percent *= 100;
			if(cnt == 0) 
				cpu_info_ptr -> usage = percent * cpu_number;			
			else
				cpu_info_ptr -> usage = percent;
		}
		usage.push_back(cpu_info_ptr->usage);
		cnt++;
	}
	fclose(file);
	
	return 0;
}
