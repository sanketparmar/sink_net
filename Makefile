obj-m += sink_net.o

all:
	make -w -I/usr/src/kernels/$(shell uname -r)/include  -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -w  -I/usr/src/kernels/$(shell uname -r)/include -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

