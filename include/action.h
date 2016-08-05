#ifndef ACTION_H
#define ACTION_H

/*
**	Migrate action of a specific VM. 
**
*/
struct Action{
	// VM's domain id
	int			dom_id;
	// which numa node the VM's vcpu should run on;
	int			cpu_node_id;
	// which numa node the VM's memory should reside;
	int			mem_node_id;
};


#endif