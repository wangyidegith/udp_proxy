#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include "../../src/udp_proxy.h"

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: <proxy_server_port>\n");
		exit(1);
	}

	// test udp_proxy.h
	UPS_Handle* h = start_udp_proxy_server((unsigned short)atoi(argv[1]));
	if (h == NULL) {
		return 1;
	}
	while(1) {
		sleep(10000);
		break;
	}
	close_udp_proxy_server(h);

	return 0;
}
