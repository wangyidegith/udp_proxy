#ifndef __UDPPROXY_H__
#define __UDPPROXY_H__

#ifdef __cplusplus
extern "C" {
#endif



	// define
	#define CHECK_TIMEOUT 5 
	#define EPOLL_WAIT_TIMEOUT 5000
	#define MAX_CLIENTS_NUM 1024
	#define DST2CLI_RECVBUF_SIZE 65507
	#define MTU 1500



	// struct
	typedef struct UPS_Handle_t UPS_Handle;
	typedef struct UPC_Handle_t UPC_Handle;



	// function
	// proxy server
	UPS_Handle* start_udp_proxy_server(unsigned short port);
	void close_udp_proxy_server(UPS_Handle* handle);
	// proxy client
	UPC_Handle* udp_proxy_init(const char* proxy_server_ip, const unsigned short proxy_server_port);
	int sendmsg_by_udp_proxy(UPC_Handle* h, const char* msg, const size_t msg_len, const struct sockaddr_in* dst);
	int recvmsg_by_udp_proxy(UPC_Handle* h, char* msg, size_t* msg_len, struct sockaddr_in* from);
	void udp_proxy_destroy(UPC_Handle* h);



#ifdef __cplusplus
}
#endif

#endif
