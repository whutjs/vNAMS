#ifndef ACTION_H
#define ACTION_H



/*
**	Migrate decision of a specific VM. 
**
*/
struct Decision{

	// VM's domain id
	int			dom_id;

	// which numa node the VM's vcpu should run on;
	int			cpu_node_id;
	// which physical CPU should migrate to
	//int 		target_cpu_no;
	// which numa node the VM's memory should reside;
	int			mem_node_id;

};



#endif