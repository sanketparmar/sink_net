/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


#define MY_DEST_MAC0	0x00
#define MY_DEST_MAC1	0x00
#define MY_DEST_MAC2	0x00
#define MY_DEST_MAC3	0x00
#define MY_DEST_MAC4	0x00
#define MY_DEST_MAC5	0x00

#define DEFAULT_IF	"sink0"
#define BUF_SIZ		1024

#define SIOC_MODE_TX        SIOCDEVPRIVATE+0
#define SIOC_MODE_RX        SIOCDEVPRIVATE+1


int get_int_input()
{
	char *end;
	char buf[10];
	int val;

     	if (!fgets(buf, sizeof buf, stdin))

     	// remove \n
     	buf[strlen(buf) - 1] = 0;

     	val = strtol(buf, &end, 10);
	return val;
}

int sendRawPacket(int sockfd, struct ifreq *if_idx)
{
	/* Copied from https://gist.github.com/austinmarton/1922600 */
	struct ifreq if_mac;
	int tx_len = 0;
	char ifName[IFNAMSIZ];
	char sendbuf[BUF_SIZ];
	struct sockaddr_ll socket_address;
	struct ether_header *eh = (struct ether_header *) sendbuf;
	
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, DEFAULT_IF, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");



	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	eh->ether_dhost[0] = MY_DEST_MAC0;
	eh->ether_dhost[1] = MY_DEST_MAC1;
	eh->ether_dhost[2] = MY_DEST_MAC2;
	eh->ether_dhost[3] = MY_DEST_MAC3;
	eh->ether_dhost[4] = MY_DEST_MAC4;
	eh->ether_dhost[5] = MY_DEST_MAC5;
	/* Ethertype field */
	eh->ether_type = htons(ETH_P_IP);
	tx_len += sizeof(struct ether_header);

	/* Packet data */
	sendbuf[tx_len++] = 0xde;
	sendbuf[tx_len++] = 0xad;
	sendbuf[tx_len++] = 0xbe;
	sendbuf[tx_len++] = 0xef;

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx->ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = MY_DEST_MAC0;
	socket_address.sll_addr[1] = MY_DEST_MAC1;
	socket_address.sll_addr[2] = MY_DEST_MAC2;
	socket_address.sll_addr[3] = MY_DEST_MAC3;
	socket_address.sll_addr[4] = MY_DEST_MAC4;
	socket_address.sll_addr[5] = MY_DEST_MAC5;

	/* Send packet */
	if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
	    printf("Send failed\n");

	return 0;

}

int set_mode(int sockfd, struct ifreq *if_idx, int mode)
{
	int ret = -1;

	if (mode == SIOC_MODE_TX || mode == SIOC_MODE_RX) {
		// do ioctl
		ret = ioctl(sockfd, mode, if_idx);
		if (ret < 0) {
			printf("ioctl error %d, err: %s\n", ret, strerror(errno));
		}
	}	
	return ret; 
}

void send_n_packets(int sockfd, struct ifreq *if_idx, int n_packets)
{
	for(int i = 0; i < n_packets; i++)
		sendRawPacket(sockfd, if_idx);

}

int xfer_status(int type)
{
        char buff[256];

	int fd = open("/proc/sink_net_tx_status", O_RDWR | O_TRUNC);
	if(fd == -1) {
		printf("Failed to open procfs file.\n");
		return -1;
	}

	// Make sure to reset the seek pointer
	lseek(fd, 0, SEEK_SET);

	if (type == 0) 
	{
		// READ 
		read(fd, buff, 256);
		puts(buff);
	} else if (type == 1)
	{
		// RESET
		write(fd, "0 0", 3);
	}
	close(fd);

}

int main(int argc, char *argv[])
{
	int sockfd;
	int ret;
	int menu_sel = 0;
	int num_packets;
	struct ifreq if_idx;
	struct ifreq if_mac;

	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
	    perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, DEFAULT_IF, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");


	printf("Userspace test application to interact with sink_net kernel moduule.\n");
	while(1)
	{
		printf("\n\n\n[MENU] =================\n");
		printf("1. Print transfer status.\n");
		printf("2. Reset transfer status.\n");
		printf("3. Change the mode to TX.\n");
		printf("4. Change the mode to RX.\n");
		printf("5. Send raw packets.\n");
		printf("========================\n");
		printf("Enter choice:");
		menu_sel = get_int_input();

		switch(menu_sel)
		{
			case 1:
				xfer_status(0); // READ from procfs
				break;
			case 2:
				printf("Reseting status counter...\n");
				xfer_status(1); // RESET status
				xfer_status(0); // READ from procfs after reset
				break;
			case 3:
				printf("Setting TX mode...\n");
				set_mode(sockfd, &if_idx, SIOC_MODE_TX);
				break;
			case 4:
				printf("Setting RX mode...\n");
				set_mode(sockfd, &if_idx, SIOC_MODE_RX);
				break;
			case 5:
				printf("Enter number of packets to be send (1-100):");
				num_packets = get_int_input();
				printf("input: %d\n", num_packets);
				if (num_packets < 1 || num_packets > 100)
					printf("Please enter supported value.\n");
				else
					send_n_packets(sockfd, &if_idx, num_packets);
				break;
			default:
				printf("Enter Valid choice.\n");

		}
	}

}
