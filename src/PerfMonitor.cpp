#include "PerfMonitor.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <cctype>
#include <cstdlib>

using namespace std;
/*
** Monitor performance event for specific VM.
** Return 0 if succeed; -1 if failed.
*/
int	PerfMonitor::monitorPerfEvent(VM_info& vmInfo) {
	int ret = 0;
	const unsigned BUF_SIZE = 128;
	char buffer[BUF_SIZE];
	char cmd[30];
    snprintf(cmd, 30,  "./perf_cpi_cachemiss.sh %u", vmInfo.getPID());
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (!feof(pipe)) {
            if (fgets(buffer, BUF_SIZE, pipe) != NULL) {
                char *p = buffer;
                while(p) {
                    if(isalpha(*p)) break;
                    ++p;
                }   
                if(*p == 'c') {
                    // cycles
                    unsigned long cycles = strtoul(buffer, NULL, 10);
                    vmInfo.cycles = cycles;                    
                } else if(*p == 'i') {
                    // instrution
                    unsigned long instructions = strtoul(buffer, NULL, 10);
                    vmInfo.instructions = instructions;               
                } else if(*p == 'o') {
                    // offcore_response_demand_data_rd_llc_hit_any_response
                    // offcore_response_demand_data_rd_llc_miss_any_response
                    unsigned offset = 36;
                    if(*(p + offset) == 'h') {
                        unsigned long data_hit = strtoul(buffer, NULL, 10);
                        vmInfo.LLC_data_rd_hit= data_hit;
                    }else if(*(p + offset) == 'm') {
                        unsigned long data_miss = strtoul(buffer, NULL, 10);
                        vmInfo.LLC_data_rd_miss = data_miss;
                    }
                } else if(*p == 'l') {
                    // longest_lat_cache_reference
                    // longest_lat_cache_miss
                    // offset=18
                    unsigned offset = 18;
                    if(*(p + offset) == 'r') {
                        unsigned long cache_reference = strtoul(buffer, NULL, 10);
                        vmInfo.LLC_all_reference =  cache_reference;                     
                    }else if(*(p + offset) == 'm') {
                        unsigned long cache_miss = strtoul(buffer, NULL, 10);
                         vmInfo.LLC_all_miss =  cache_miss;     
                    }
                }   
            }   
        }   
    } catch (...) {
        pclose(pipe);
        ret = -1;
    }   
    pclose(pipe);

    /* for debug */
    double cpi = vmInfo.cycles / (double)vmInfo.instructions;
 	double data_rd_miss_rate = vmInfo.LLC_data_rd_miss / (double)(vmInfo.LLC_data_rd_miss + vmInfo.LLC_data_rd_hit);
 	double total_cache_miss_rate = vmInfo.LLC_all_miss / (double)(vmInfo.LLC_all_reference);
    printf("%s CPI=%.3f\n", vmInfo.vm_name, cpi);
	printf("%s LLC Data Rd Miss Rate=%.3f\n", vmInfo.vm_name, data_rd_miss_rate);
	printf("%s LLC Total Miss Rate=%.3f\n", vmInfo.vm_name, total_cache_miss_rate);


    return ret;
}

