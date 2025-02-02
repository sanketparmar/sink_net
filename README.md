**sink_net** driver creates a dummy network interface with sink0. For demo purpose, It supports two mode RX and TX.
TX mode is to mimic the packet trasfered to hardware and RX mode puts it back into network stack. 
All the packets transmitted to this interface will be discarded and the status count will be updated based on mode.

To change the mode, siocdevprivate ioctl is exposed and userspace application can change the mode. 

This driver also exposed the PROCFS file "sink_net_status". Reading this file gives the details of transmitted packets. Writing into this file will reset/update the packet counts and bytes.

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
This module creates _sink_net_status_ file in _/proc_ directory.

```
$ cat /proc/sink_net_status  
Total transmitted packet count: 39 (4894 bytes)

$ echo "0 0" > /proc/sink_net_status # to reset the counters. First number represents packet counts and second number represents bytes.
```

### Userspace test application
```testapp.c``` can be use to interact with the driver. It provides following operations.

```

[MENU] =================
1. Print transfer status.
2. Reset transfer status.
3. Change the mode to TX.
4. Change the mode to RX.
5. Send raw packets.
========================
Enter choice:1
Currently Running in mode: TX
Total transmited packet count: 10 (180 bytes)
Total received packet count: 0 (0 bytes)



[MENU] =================
1. Print transfer status.
2. Reset transfer status.
3. Change the mode to TX.
4. Change the mode to RX.
5. Send raw packets.
========================
Enter choice:5
Enter number of packets to be send (1-100):3
input: 3



[MENU] =================
1. Print transfer status.
2. Reset transfer status.
3. Change the mode to TX.
4. Change the mode to RX.
5. Send raw packets.
========================
Enter choice:1
Currently Running in mode: TX
Total transmited packet count: 13 (234 bytes)
Total received packet count: 0 (0 bytes)



[MENU] =================
1. Print transfer status.
2. Reset transfer status.
3. Change the mode to TX.
4. Change the mode to RX.
5. Send raw packets.
========================
Enter choice:4
Setting RX mode...



[MENU] =================
1. Print transfer status.
2. Reset transfer status.
3. Change the mode to TX.
4. Change the mode to RX.
5. Send raw packets.
========================
Enter choice:5
Enter number of packets to be send (1-100):99
input: 99



[MENU] =================
1. Print transfer status.
2. Reset transfer status.
3. Change the mode to TX.
4. Change the mode to RX.
5. Send raw packets.
========================
Enter choice:1
Currently Running in mode: RX
Total transmited packet count: 13 (234 bytes)
Total received packet count: 99 (1782 bytes)

``` 


Tested on Ubuntu with 6.8.0-52-generic kernel.
