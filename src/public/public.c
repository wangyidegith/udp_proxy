#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "public.h"

void encode(const char* data, const size_t data_len, const struct sockaddr_in* dst, char* packet_buf) {
	if (packet_buf == NULL) {
		return;
	}
	UDP_Proxy_Packet* packet = (UDP_Proxy_Packet*)packet_buf;
	packet->data_len = htonl(data_len);
	if (dst != NULL) {
		memcpy((void*)&(packet->dst), (void*)dst, sizeof(struct sockaddr));
	}
	if (data != NULL) {
		memcpy((void*)(packet->data), data, data_len);
	}
}

UDP_Proxy_Packet* decode(const char* packet_buf) {
	UDP_Proxy_Packet* upp = (UDP_Proxy_Packet*)packet_buf;
	size_t data_len = (ntohl)(upp->data_len);
	upp->data_len = data_len;
	return upp;
}

int createUdpSocket() {
	int sock_fd;
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		perror("socket(udp)");
		return -1;
	}
	return sock_fd;
}
