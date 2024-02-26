#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#include "udp_proxy.h"
#include "public/public.h"

typedef struct UPC_Handle_t {
	int udp_fd;
	struct sockaddr_in proxy_addr;
}UPC_Handle;

UPC_Handle* udp_proxy_init(const char* proxy_server_ip, const unsigned short proxy_server_port) {
	if (strlen(proxy_server_ip) > IPv4_BUF_SIZE - 1) {
		fprintf(stderr, "Err: proxy_server_ip too long.\n");
		return NULL;
	}
	UPC_Handle* h = (UPC_Handle*)malloc(sizeof(UPC_Handle));
	memset((void*)h, 0x00, sizeof(UPC_Handle));
	h->proxy_addr.sin_family = AF_INET;
	if (inet_pton(AF_INET, proxy_server_ip, &(h->proxy_addr.sin_addr)) <= 0) {
		perror("inet_pton");
		return NULL;
	}
	h->proxy_addr.sin_port = htons(proxy_server_port);
	h->udp_fd = createUdpSocket();
	if (h->udp_fd == -1) {
		return NULL;
	}
	return h;
}

int sendmsg_by_udp_proxy(UPC_Handle* h, const char* msg, const size_t msg_len, const struct sockaddr_in* dst) {
	// encode msg to packet
	char send_buf[UDP_PROXY_PACKET_HEAD_LEN + msg_len + 1];
	memset((void*)send_buf, 0x00, sizeof(send_buf));
	encode(msg, msg_len, dst, send_buf);

	// sendto
	ssize_t send_ret = sendto(h->udp_fd, send_buf, UDP_PROXY_PACKET_HEAD_LEN + msg_len, 0, (struct sockaddr*)&(h->proxy_addr), sizeof(struct sockaddr));
	if (send_ret == -1) {
		perror("sendto");
		return -1;
	}
	return 0;
}

int recvmsg_by_udp_proxy(UPC_Handle* h, char* msg, size_t* msg_len, struct sockaddr_in* from) {
	// recvfrom fisrt
	char* recv_buf = (char*)malloc(UDP_RECV_BUF_SIZE);
	memset((void*)recv_buf, 0x00, UDP_RECV_BUF_SIZE);
	ssize_t recv_ret = recvfrom(h->udp_fd, recv_buf, UDP_PROXY_PACKET_HEAD_LEN, MSG_PEEK, NULL, NULL);
	if (recv_ret == -1) {
		fprintf(stderr, "Err: the first recvfrom.\n");
		perror("recvfrom");
		return -1;
	}

	// recvfrom second
	UDP_Proxy_Packet* upp = decode(recv_buf);
	size_t data_len = upp->data_len;
	if (data_len > MAX_DATA_LEN) {
		free(recv_buf);
		recv_buf = (char*)malloc(data_len + UDP_PROXY_PACKET_HEAD_LEN + 1);
		memset((void*)recv_buf, 0x00, data_len + UDP_PROXY_PACKET_HEAD_LEN + 1);
	}
	struct sockaddr_in proxy_addr;
	socklen_t addr_len;
	recv_ret = recvfrom(h->udp_fd, recv_buf, UDP_PROXY_PACKET_HEAD_LEN + data_len, 0, (struct sockaddr*)&proxy_addr, &addr_len);
	if (recv_ret == -1) {
		fprintf(stderr, "Err: the second recvfrom.\n");
		perror("recvfrom");
		return -1;
	}

	// out
	memcpy((void*)from, (void*)&(upp->dst), sizeof(struct sockaddr));
	memcpy((void*)msg, (void*)(recv_buf + UDP_PROXY_PACKET_HEAD_LEN), data_len);
	*msg_len = recv_ret - UDP_PROXY_PACKET_HEAD_LEN;

	free(recv_buf);
	return 0;
}

void udp_proxy_destroy(UPC_Handle* h) {
	close(h->udp_fd);
	free(h);
}
