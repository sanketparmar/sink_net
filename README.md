**sink_net** driver creates a dummy network interface with sink0. All the packets transmitted to this interface will be discarded and the status count will be updated.

This driver also exposed the PROCFS file "sink_net_tx_status".

Reading this file gives the details of transmitted packets. Writing into this file will reset/update the packet counts and bytes.

### How to build, load and unload  
To build, you need to install kernel header file package.  
For Ubuntu(or any debian based os), Run  
```sudo apt install linux-headers-$(uname -r))```

To compile the module, run  
- ```make``` (This will compile the module and create .ko file)

To clean the compiled binaries,  
- ```make clean``` (deletes .ko and all other compiled file)

To load the module  
- ```sudo insmod sink_net.ko``` (This will create a sink0 network interface)

To activate the interface  
- ```sudo ifconfig sink0 up```  
- ```sudo ifconfig sink0 12.0.0.1```

or run the activate_interface.sh script.

### PROCFS entry  
This module creates _sink_net_tx_status_ file in _/proc_ directory.

```
$ cat /proc/sink_net_tx_status  
Total transmitted packet count: 39 (4894 bytes)

$ echo "0 0" > /proc/sink_net_tx_status # to reset the counters. First number represents packet counts and second number represents bytes.
```
Or Compile and run test_procfs.c 


Tested on Ubuntu with 6.8.0-52-generic kernel.
