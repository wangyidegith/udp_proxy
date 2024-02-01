#ifndef __PUBLIC_H__
#define __PUBLIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define IPv4_BUF_SIZE 16
#define PORT_BUF_MAX_SIZE 6



	// struct
	// proto packet
	typedef struct {
		size_t data_len;
		struct sockaddr_in dst;
		char data[0];
	}UDP_Proxy_Packet;



	// define
	#define UDP_PROXY_PACKET_HEAD_LEN 24



	// function
	void encode(const char* data, size_t data_len, struct sockaddr_in* dst, char* packet_buf);
	UDP_Proxy_Packet* decode(const char* packet_buf);
	int createUdpSocket();

#ifdef __cplusplus
}
#endif

#endif
