#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <netdb.h>

#include "tftp.hxx"
#include "server.hxx"

namespace tftp {
	Server::Server(const char* port) {
		hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
		hints.ai_socktype = SOCK_DGRAM; // UDP
		hints.ai_flags = AI_PASSIVE;    // For wildcard IP address
		hints.ai_protocol = 0;          // Any protocol
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;

		sock = setUp(NULL, port, hints, true);
	}

	void Server::r() {
		rd(sock);
	}
}

int main() {
	auto server = tftp::Server("2830");

	for(;;) {
		server.r();
	}
}
