/*
** monitor cpu usage, including every single core
** 
** written by Jenson	
*/

#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <vector>

/* corresponding to /proc/stat */
struct Cpu_stat{
	unsigned long long	total_user;
	unsigned long long	total_user_low;
	unsigned long long	total_sys;
	unsigned long long	total_idle;
	unsigned long long	total_iowait;
	unsigned long long	total_irq;
	unsigned long long	total_softirq;
	unsigned long long	total_steal;
	unsigned long long	total_guest;
	unsigned long long	total_guest_nice;
	double				usage;			/* cpu usage percent */
};

/*
** 
*/
class Cpu_monitor {
public:
	Cpu_monitor(int cpu_num):cpu_number(cpu_num) { cpu_stat_array = NULL; }
	~Cpu_monitor() { if(cpu_stat_array != NULL) delete[] cpu_stat_array; }
	// init all data; should be called before call other functions
	int init_cpu_monitor();
	int get_cpu_usage(std::vector<double> &usage);
private:
	static const int 	BUF_SIZE = 256;
	int 				cpu_number;
	// size = cpu_number + 1
	Cpu_stat 			*cpu_stat_array;
	char 				buf[BUF_SIZE];
};

#endif
