#ifndef __PUBLIC_H__
#define __PUBLIC_H__

#ifdef __cplusplus
extern "C" {
#endif




	// struct
	// proto packet
	typedef struct {
		size_t data_len;
		struct sockaddr_in dst;
		char data[0];
	}UDP_Proxy_Packet;



	// define
	#define IPv4_BUF_SIZE 16

	#define UDP_PROXY_PACKET_HEAD_LEN 24
	#ifndef MTU
		#define MTU 576
	#endif
	#define MAX_DATA_LEN MTU-20-8
	#define UDP_RECV_BUF_SIZE MAX_DATA_LEN+UDP_PROXY_PACKET_HEAD_LEN+1



	// function
	void encode(const char* data, const size_t data_len, const struct sockaddr_in* dst, char* packet_buf);
	UDP_Proxy_Packet* decode(const char* packet_buf);
	int createUdpSocket();

#ifdef __cplusplus
}
#endif

#endif
