#!/bin/bash
pid=$1
sudo perf stat -e cycles,instructions,cpu/event=0xb7,umask=0x1,offcore_rsp=0x3f803c0001,name=offcore_response_demand_data_rd_llc_hit_any_response/,cpu/event=0xb7,umask=0x1,offcore_rsp=0x3fffc20001,name=offcore_response_demand_data_rd_llc_miss_any_response/,cpu/event=0x2e,umask=0x4f,name=longest_lat_cache_reference/,cpu/event=0x2e,umask=0x41,name=longest_lat_cache_miss/ -x,  -p ${pid} sleep 0.1 2>&1

