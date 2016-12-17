#include "IOMonitor.h"



int IOMonitor::monitorIO(VM_info& vm_info) {
	int  ret = 0;
	char rx_pack_path[MAX_PATH_LEN];
	char tx_pack_path[MAX_PATH_LEN];
	char rx_bytes_path[MAX_PATH_LEN];
	char tx_bytes_path[MAX_PATH_LEN];

	snprintf(rx_pack_path, MAX_PATH_LEN, "/sys/class/net/%s/vf%d/statistics/rx_packets", netdev, vm_info.vf_no);
	snprintf(tx_pack_path, MAX_PATH_LEN, "/sys/class/net/%s/vf%d/statistics/tx_packets", netdev, vm_info.vf_no);
	snprintf(rx_bytes_path, MAX_PATH_LEN, "/sys/class/net/%s/vf%d/statistics/rx_bytes", netdev, vm_info.vf_no);
	snprintf(tx_bytes_path, MAX_PATH_LEN, "/sys/class/net/%s/vf%d/statistics/tx_bytes", netdev, vm_info.vf_no);

	unsigned long long		last_rx_packets;
	unsigned long long		last_tx_packets;
	unsigned long long		last_rx_bytes;
	unsigned long long		last_tx_bytes;
	unsigned long long		last_io_timestamp;

	last_rx_packets   = vm_info.rx_packets;
	last_tx_packets   = vm_info.tx_packets;
	last_rx_bytes     = vm_info.rx_bytes;
	last_tx_bytes     = vm_info.tx_bytes;
	last_io_timestamp = vm_info.io_timestamp_usec;

	FILE* rx_pack_file = fopen(rx_pack_path, "r");
	if(rx_pack_file == NULL) {
		fprintf(stderr, "cannot open %s, error:%s\n", rx_pack_path, strerror(errno));
		ret = -1;
		return ret;

	}
	fscanf(rx_pack_file, "%llu", &vm_info.rx_packets);
	fclose(rx_pack_file);

	FILE* tx_pack_file = fopen(tx_pack_path, "r");
	if(tx_pack_file == NULL) {
		fprintf(stderr, "cannot open %s, error:%s\n", tx_pack_path, strerror(errno));
		ret = -1;
		return ret;

	}
	fscanf(tx_pack_file, "%llu", &vm_info.tx_packets);
	fclose(tx_pack_file);
	FILE* rx_bytes_file = fopen(rx_bytes_path, "r");
	if(rx_bytes_file == NULL) {
		fprintf(stderr, "cannot open %s, error:%s\n", rx_bytes_path, strerror(errno));
		ret = -1;
		return ret;

	}
	fscanf(rx_bytes_file, "%llu", &vm_info.rx_bytes);
	fclose(rx_bytes_file);

	FILE* tx_bytes_file = fopen(tx_bytes_path, "r");
	if(tx_bytes_file == NULL) {
		fprintf(stderr, "cannot open %s, error:%s\n", tx_bytes_path, strerror(errno));
		ret = -1;
		return ret;

	}

	fscanf(tx_bytes_file, "%llu", &vm_info.tx_bytes);
	fclose(tx_bytes_file);

	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);
	vm_info.io_timestamp_usec = timestamp.tv_sec * 1000000 + timestamp.tv_usec;
	double elapsedTime = (vm_info.io_timestamp_usec - last_io_timestamp) / 1000000.0;

	vm_info.total_packets_history = vm_info.total_packets;
	vm_info.total_KB_history = vm_info.total_KB;
	vm_info.total_packets = (vm_info.rx_packets - last_rx_packets) + (vm_info.tx_packets - last_tx_packets);
	vm_info.total_KB = (vm_info.rx_bytes - last_rx_bytes)/1024.0 + (vm_info.tx_bytes - last_tx_bytes)/1024.0;

	vm_info.packets_per_sec_history = vm_info.packets_per_sec;
	vm_info.packets_per_sec = vm_info.total_packets / elapsedTime;
	vm_info.KB_per_sec_history = vm_info.KB_per_sec;
	vm_info.KB_per_sec = vm_info.total_KB / elapsedTime;

	return ret;
}



