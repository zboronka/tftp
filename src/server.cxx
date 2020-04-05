#include <sys/types.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>

#include "tftp.hxx"

void die(const char* s) {
	perror(s);
	exit(1);
}

int main() {
	const int BUFLEN = 516;
	const char *PORT = "2830";

	struct addrinfo hints, *result;
	int sfd, s;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	ssize_t nread;
	char buf[BUFLEN];

	hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // UDP
	hints.ai_flags = AI_PASSIVE;    // For wildcard IP address
	hints.ai_protocol = 0;          // Any protocol
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	sfd = tftp::setUp(NULL, PORT, hints, true);

	for(;;) {
		tftp::read(sfd);
	}
}
