#!/bin/bash
sudo insmod sink_net.ko
sudo ifconfig sink0 up
sudo ifconfig sink0 12.0.0.1/24
