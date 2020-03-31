#include <sys/types.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>

void die(const char* s) {
	perror(s);
	exit(1);
}

int main() {
	const int BUFLEN = 2048;
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

	s = getaddrinfo(NULL, PORT, &hints, &result);
	if (s != 0) {
		std::cerr << "getaddrinfo: " << gai_strerror(s) << std::endl;
		exit(EXIT_FAILURE);
	}

	/*********************************************** 
	 * We received a list of address structures
	 * and here loop through them until a successful
	 * bind is made
	 ***********************************************/

	for (result = result; result != NULL; result = result->ai_next) {
		sfd = socket(result->ai_family, result->ai_socktype,
		             result->ai_protocol);
		if (sfd == -1) continue;
		if (bind(sfd, result->ai_addr, result->ai_addrlen) == 0) break;

		close(sfd);
	}

	if (result == NULL) {
		std::cerr << "Could not bind" << std::endl;
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);

	for(;;) {
		peer_addr_len = sizeof(struct sockaddr_storage);
		nread = recvfrom(sfd, buf, BUFLEN, 0,
		                (struct sockaddr *) &peer_addr, &peer_addr_len);
		if (nread == -1) continue;

		char host[NI_MAXHOST], service[NI_MAXSERV];

		s = getnameinfo((struct sockaddr *) &peer_addr,
		                peer_addr_len, host, NI_MAXHOST,
		                service, NI_MAXSERV, NI_NUMERICSERV);
		if (s == 0) {
			std::cout << "Received " << nread << " bytes from "
 			   	      << host << ":" << service << std::endl;
		} else {
			std::cerr << "getnameinfo: " << gai_strerror(s) << std::endl;
		}

		if (sendto(sfd, buf, nread, 0,
		           (struct sockaddr *) &peer_addr,
		           peer_addr_len) != nread) {
			std::cerr << "Error sending response" << std::endl;
		}
	}
}
