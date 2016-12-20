# vNAMS #

vNAMS(NUMA Affinity Management System in Virtualization)是用来在Linux NUMA架构下，针对KVM虚拟机的VCPU进行调度的程序。对VCPU调度的依据主要是:虚拟机的类型（IO密集型虚拟机还是非IO密集型），内存亲和度（该虚拟机的VCPU与虚拟机的内存的距离），网卡亲和度（虚拟机离网卡的距离），NUMA节点的负载。使用到的库有:[Libvirt](https://libvirt.org/index.html), Libnuma
