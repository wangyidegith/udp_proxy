#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

#include "udp_proxy.h"
#include "public/public.h"
#include "hash_table/hash_table.h"

typedef struct UPS_Handle_t {
	unsigned short port;
	Epoll_Ptr_t* ep;
	pthread_t tid;
}UPS_Handle;

static void sockaddrToIPv4String(const struct sockaddr_in* sa, char* buffer, size_t buf_len) {
	const char* ip = inet_ntop(AF_INET, &(sa->sin_addr), buffer, buf_len);
	if (ip == NULL) {
		perror("inet_ntop");
	}
}

static int createUdpLocalSocket(const char* ip, unsigned short port) {
	int sock_fd;
	struct sockaddr_in local_addr;
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		perror("socket(udp local)");
		return -1;
	}
	memset((void*)(&local_addr), 0x00, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	if (ip == NULL) {
		local_addr.sin_addr.s_addr = INADDR_ANY;
	} else {
		if (inet_pton(AF_INET, ip, &(local_addr.sin_addr)) <= 0) {
			perror("inet_pton");
			return -1;
		}
	}
	local_addr.sin_port = htons(port);
	int reuse = 1;
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
		perror("setsockopt addr");
		return -1;
	}
	if (bind(sock_fd, (const struct sockaddr *)(&local_addr), sizeof(local_addr)) < 0) {
		perror("bind(udp)");
		return -1;
	}
	return sock_fd;
}

static int makeSocketNonBlocking(int sockfd) {
	int flags, s;
	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl(F_GETFL)");
		return -1;
	}
	flags |= O_NONBLOCK;
	s = fcntl(sockfd, F_SETFL, flags);
	if (s == -1) {
		perror("fcntl(F_SETFL)");
		return -1;
	}
	return 0;
}

static void deletePtr(Epoll_Ptr_t* ep) {
	if (ep == NULL) {
		return;
	}
	if (ep->fd != 0) {
		close(ep->fd);
	}
	if (ep->recv_buf != NULL) {
		free(ep->recv_buf);
	}
	if (ep->reverse_recv_buf != NULL) {
		free(ep->reverse_recv_buf);
	}
	if (ep->hashtable != NULL) {
		destroyHashTable(ep->hashtable);
	}
	free(ep);
}

static int proxyCli2Dst(Epoll_Ptr_t* ep) {
	// recvfrom Cli
	// recvfrom fisrt to get data len
	memset((void*)(ep->recv_buf), 0x00, UDP_RECV_BUF_SIZE);
	ssize_t recv_ret = recvfrom(ep->fd, ep->recv_buf, UDP_PROXY_PACKET_HEAD_LEN, MSG_PEEK, NULL, NULL);
	if (recv_ret == -1) {
		return -1;
	}
	UDP_Proxy_Packet* upp = decode(ep->recv_buf);
	size_t data_len = upp->data_len;
	bool flag = false;
	char* temp = ep->recv_buf;
	if (data_len > MAX_DATA_LEN) {
		ep->recv_buf = (char*)malloc((data_len + UDP_PROXY_PACKET_HEAD_LEN + 1) * sizeof(char));
		memset((void*)(ep->recv_buf), 0x00, (data_len + UDP_PROXY_PACKET_HEAD_LEN + 1) * sizeof(char));
		flag = true;
	}

	// recvfrom second
	struct sockaddr_in cli_addr;
	memset((void*)&cli_addr, 0x00, sizeof(struct sockaddr_in));
	socklen_t addr_len = sizeof(struct sockaddr_in);
	recv_ret = recvfrom(ep->fd, ep->recv_buf, UDP_PROXY_PACKET_HEAD_LEN + data_len, 0, (struct sockaddr*)&cli_addr, &addr_len);
	if (recv_ret == -1) {
		return -1;
	}

	// the first thing is to find fd(value) based sa(key)
	Epoll_Ptr_t* dst_ep = search(ep->hashtable, &cli_addr, sizeof(struct sockaddr_in), (int)(cli_addr.sin_port));

	// record map
	// if not found, indicate this is a new record, record, then to sendto
	// if found, update timestamp, then directly to sendto
	if (dst_ep == NULL) {
		// can't > MAX_CLIENTS_NUM
		if (ep->hashtable->count == MAX_CLIENTS_NUM) {
			return -1;
		}

		// get a socket fd for new client
		int dst_fd = createUdpSocket();
		if (dst_fd < 0) {
			return -1;
		}
		if (makeSocketNonBlocking(dst_fd)) {
			return -1;
		}

		// fill dst_ep
		struct epoll_event event;
		event.events = EPOLLIN;
		dst_ep = (Epoll_Ptr_t*)malloc(sizeof(Epoll_Ptr_t));
		memset((void*)dst_ep, 0x00, sizeof(Epoll_Ptr_t));
		dst_ep->epoll_fd = ep->epoll_fd;
		dst_ep->fd = ep->fd;
		dst_ep->dst_fd = dst_fd;
		dst_ep->recv_buf = ep->recv_buf;
		dst_ep->reverse_recv_buf = ep->reverse_recv_buf;
		dst_ep->reverse_recv_buf_size = ep->reverse_recv_buf_size;
		dst_ep->send_buf = NULL;
		dst_ep->cli_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		memset((void*)(dst_ep->cli_addr), 0x00, sizeof(struct sockaddr_in));
		memcpy((void*)(dst_ep->cli_addr), (void*)&cli_addr, sizeof(struct sockaddr_in));
		dst_ep->hashtable = ep->hashtable;
		time(&(dst_ep->timestamp));

		// add ep
		event.data.ptr = (void*)dst_ep;
		if (epoll_ctl(dst_ep->epoll_fd, EPOLL_CTL_ADD, dst_ep->dst_fd, &event) == -1) {
			perror("epoll_ctl(ADD)");
			deletePtr(dst_ep);
			return -1;
		}

		// add hashtable
		insert(dst_ep->hashtable, dst_ep->cli_addr, dst_ep, (int)(dst_ep->cli_addr->sin_port));
		++(ep->hashtable->count);
	} else {
		// update timestamp
		time(&(dst_ep->timestamp));
	}

	// sendto Dst
	char* send_buf = dst_ep->recv_buf + UDP_PROXY_PACKET_HEAD_LEN;
	ssize_t send_ret = sendto(dst_ep->dst_fd, send_buf, data_len, 0, (struct sockaddr*)&(upp->dst), sizeof(struct sockaddr_in));
	if (send_ret == -1) {
		perror("sendto");
		return -1;
	}

	// maybe free
	if (flag) {
		free(ep->recv_buf);
		ep->recv_buf = temp;
	}

	// debug
	char cli_ip_buf[16] = {0};
	sockaddrToIPv4String(dst_ep->cli_addr, cli_ip_buf, 16);
	unsigned short cli_port = ntohs(dst_ep->cli_addr->sin_port);
	char dst_ip_buf[16] = {0};
	sockaddrToIPv4String(&(upp->dst), dst_ip_buf, 16);
	unsigned short dst_port = ntohs(upp->dst.sin_port);
	printf("data(%s) from cli(%s:%hu) to dst(%s:%hu)\n", send_buf, cli_ip_buf, cli_port, dst_ip_buf, dst_port);

	return 0;
}

static int proxyDst2Cli(Epoll_Ptr_t* ep) {
        // recvfrom Dst
        memset((void*)(ep->reverse_recv_buf), 0x00, ep->reverse_recv_buf_size);
        socklen_t addr_len = sizeof(struct sockaddr_in);
        struct sockaddr_in from_sa;
        bzero((void*)&from_sa, 16);
        ssize_t recv_ret = recvfrom(ep->dst_fd, ep->reverse_recv_buf, ep->reverse_recv_buf_size, 0, (struct sockaddr*)&from_sa, &addr_len);
        if (recv_ret <= 0) {
                return -1;
        }

        // update timestamp
        time(&(ep->timestamp));

        // sendto Cli
        ep->send_buf = (char*)malloc(recv_ret + UDP_PROXY_PACKET_HEAD_LEN + 1);
        memset((void*)(ep->send_buf), 0x00, recv_ret + UDP_PROXY_PACKET_HEAD_LEN + 1);
        encode(ep->reverse_recv_buf, recv_ret, &from_sa, ep->send_buf);
        ssize_t send_ret = sendto(ep->fd, ep->send_buf, UDP_PROXY_PACKET_HEAD_LEN + recv_ret, 0, (struct sockaddr*)(ep->cli_addr), sizeof(struct sockaddr_in));
        if (send_ret <= 0) {
                return -1;
        }

        // debug
        char dst_ip_buf[16] = {0};
        sockaddrToIPv4String(&(from_sa), dst_ip_buf, 16);
        unsigned short dst_port = ntohs(from_sa.sin_port);
        char cli_ip_buf[16] = {0};
        sockaddrToIPv4String(ep->cli_addr, cli_ip_buf, 16);
        unsigned short cli_port = ntohs(ep->cli_addr->sin_port);
        printf("data(%s) from dst(%s:%hu) to cli(%s:%hu)\n", ep->send_buf + UDP_PROXY_PACKET_HEAD_LEN, dst_ip_buf, dst_port, cli_ip_buf, cli_port);

        free(ep->send_buf);

        return 0;
}

static void hashtableCheck(Epoll_Ptr_t* ep) {
	Node* cur = NULL;
	Node* cur_to_free = NULL;
	time_t now = time(NULL);
	for (int i = 0; i < TABLE_SIZE; i++) {
		cur = ep->hashtable->buckets[i];
		Node* prev = NULL;
		while (cur != NULL) {
			if ((now - cur->value->timestamp) < CHECK_TIMEOUT) {
				// not timeout
				prev = cur;
				cur = cur->next;
			} else {
				// timeout, to delete
				if (prev == NULL) {
					ep->hashtable->buckets[i] = cur->next;
				} else {
					prev->next = cur->next;
				}
				free(cur->key);
				free(cur->value);
				cur_to_free = cur;
				cur = cur->next;
				free(cur_to_free);
				--(ep->hashtable->count);
			}
		}
	}
}

static void cleanCoreRes(void* arg) {
	Epoll_Ptr_t* ep = ( Epoll_Ptr_t*)arg;
	deletePtr(ep);
}

static void* udpProxyServer(void* arg) {
	// get arg
	UPS_Handle* h = (UPS_Handle*)arg;

	// prepare res
	int clients_fd, epoll_fd, event_count, i;
	struct epoll_event event, events[MAX_CLIENTS_NUM + 1];
	Epoll_Ptr_t* cur_ptr = NULL;
	char* recv_buf = (char*)malloc(UDP_RECV_BUF_SIZE);

	// create epoll
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		perror("epoll_create1");
		pthread_exit((void*)-1);
	}

	// create udp socket for clients
	if ((clients_fd = createUdpLocalSocket(NULL, h->port)) < 0) {
		pthread_exit((void*)-1);
	}
	if (makeSocketNonBlocking(clients_fd)) {
		pthread_exit((void*)-1);
	}

	// put clients_fd to epoll
	event.events = EPOLLIN;
	Epoll_Ptr_t* ep = (Epoll_Ptr_t*)malloc(sizeof(Epoll_Ptr_t));
	memset((void*)ep, 0x00, sizeof(Epoll_Ptr_t));
	ep->epoll_fd = epoll_fd;
	ep->fd = clients_fd;
	ep->recv_buf = recv_buf;
	ep->reverse_recv_buf_size = DST2CLI_RECVBUF_SIZE;
	ep->reverse_recv_buf = (char*)malloc(ep->reverse_recv_buf_size);
	ep->send_buf = NULL;
	ep->cli_addr = NULL;
	ep->hashtable = initHashTable();
	event.data.ptr = (void*)ep;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clients_fd, &event) == -1) {
		perror("epoll_ctl(ADD clients_fd)");
		deletePtr(ep);
		pthread_exit((void*)1);
	}

	printf("server is listening on %hu\n", h->port);

	// main while
	pthread_cleanup_push(cleanCoreRes, (void*)ep);
	while (1) {
		// check nodes' time in hashtable
		hashtableCheck(ep);

		// wait read events
		event_count = epoll_wait(epoll_fd, events, MAX_CLIENTS_NUM, EPOLL_WAIT_TIMEOUT);
		if (event_count < 0) {
			perror("epoll_wait");
			continue;
		} else if (event_count == 0) {
			continue;
		}

		// have read events, to read
		for (i = 0; i < event_count; i++) {
			cur_ptr = (Epoll_Ptr_t*)(events[i].data.ptr);
			if (cur_ptr->dst_fd == 0) {
				// client -> server
				while (!proxyCli2Dst(cur_ptr)) {
					continue;
				}
			} else {
				// server -> client
				while (!proxyDst2Cli(cur_ptr)) {
					continue;
				}
			}
		}
	}
	pthread_cleanup_pop(ep->fd);
}



UPS_Handle* start_udp_proxy_server(unsigned short port) {
	// fill h
	UPS_Handle* h = (UPS_Handle*)malloc(sizeof(UPS_Handle));
	memset((void*)h, 0x00, sizeof(UPS_Handle));
	h->port = port;

	// start udpProxyServer
	pthread_t tid;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&tid, &attr, udpProxyServer, (void*)h) != 0) {
		fprintf(stderr, "Failed to create thread(udpProxyServer)\n");
		free(h);
		return NULL;
	}
	h->tid = tid;
	pthread_attr_destroy(&attr);

	return h;
}

void close_udp_proxy_server(UPS_Handle* h) {
	// send cancel
	if (pthread_cancel(h->tid) != 0) {
		fprintf(stderr, "pthread_cancel\n");
	}

	// free h
	free(h);
}
