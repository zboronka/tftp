#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netdb.h>

#include "tftp.hxx"
#include "server.hxx"

namespace tftp {
	Server::Server(bool ipv6) {
		hints.ai_family = ipv6 ? AF_INET6 : AF_INET;    // IPv4 or IPv6
		hints.ai_socktype = SOCK_DGRAM;                 // UDP
		hints.ai_flags = AI_PASSIVE;                    // For wildcard IP address
		hints.ai_protocol = 0;                          // Any protocol
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;
	}

	bool Server::establish(const char *port) {
		sock = setUp(NULL, port, hints, true);
		if(sock == -1) {
			return false;
		}

		return fcntl(sock, F_SETFL, O_NONBLOCK) >= 0;
	}

	void Server::deliver(const void *packet, int size) {
		if (sendto(sock, packet, size, 0,
		           (sockaddr *) &peer_addr,
		           peer_addr_len) != size) {
			std::cerr << "Error sending response" << std::endl;
		}
	}

	ssize_t Server::process() {
		auto nread = recvfrom(sock, buf, BUFLEN, 0,
		                (sockaddr *) &peer_addr, &peer_addr_len);
		return nread;
	}
}

int main(int argc, char **argv) {
	bool ipv6 = false;
	for(int i = 0; i < argc; i++) {
		if(strcmp(argv[i], "--ipv6") == 0) {
			ipv6 = true;
		}
	}
	auto server = tftp::Server(ipv6);
	server.establish("2830");

	for(;;) {
		server.sending();
		server.processPacket();
	}
}
