#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "../../src/udp_proxy.h"

void sockaddrToIPv4String(const struct sockaddr_in* sa, char* buffer, size_t buf_len) {
    const char* ip = inet_ntop(AF_INET, &(sa->sin_addr), buffer, buf_len);
    if (ip == NULL) {
        perror("inet_ntop");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: <proxy_ip> <proxy_port> <dst_ip> <dst_port>\n");
        exit(1);
    }

    UPC_Handle* h = udp_proxy_init(argv[1], (unsigned short)atoi(argv[2]));
    if (h == NULL) {
        return 1;
    }
    char msg[10000] = {0};
    struct sockaddr_in dst_addr, from_addr;
    memset((void*)&dst_addr, 0, sizeof(dst_addr));
    memset((void*)&from_addr, 0, sizeof(from_addr));
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons((unsigned short)atoi(argv[4]));
    if (inet_pton(AF_INET, argv[3], &(dst_addr.sin_addr)) <= 0) {
        perror("inet_pton");
        return -1;
    }

    int send_ret, recv_ret;
    char ip[16] = {0};
    unsigned short port;
    char* send_msg = "Hello, I only send once message.";
    size_t recv_msg_len;
    while (1) {
        // send
        send_ret = sendmsg_by_udp_proxy(h, send_msg, strlen(send_msg), &dst_addr);
        if (send_ret) {
            fprintf(stderr, "Err: sendmsg_by_udp_proxy\n");
            break;
        }
        bzero((void*)ip, 16);
        sockaddrToIPv4String(&dst_addr, ip, 16);
        port = ntohs(dst_addr.sin_port);
        printf("sent %s to %s:%hu, len is %ld\n", send_msg, ip, port, strlen(send_msg));

        // recv
        bzero((void*)msg, sizeof(msg));
        recv_ret = recvmsg_by_udp_proxy(h, msg, &recv_msg_len, &from_addr);
        if (recv_ret) {
            fprintf(stderr, "Err: recvmsg_by_udp_proxy\n");
            break;
        }
        bzero((void*)ip, 16);
        sockaddrToIPv4String(&from_addr, ip, 16);
        port = ntohs(from_addr.sin_port);
	printf("recv %s from %s:%hu, len is %ld\n", msg, ip, port, recv_msg_len);
        printf("-------\n");
        sleep(10000);
    }

    udp_proxy_destroy(h);
    return 0;
}
