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
	Server::Server(bool ipv6, int W_T): Tftp(W_T) {
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
	auto W_T = 8;
	for(int i = 0; i < argc; i++) {
		if(strcmp(argv[i], "--ipv6") == 0) {
			ipv6 = true;
		} else if(strcmp(argv[i], "-w") == 0) {
			W_T = std::stoi(argv[i+1]);
		} else if(strcmp(argv[i], "--help") == 0) {
			std::cout << "Usage: server [OPTIONS]" << std::endl;
			std::cout.width(18);
			std::cout << std::left << "--ipv6"
			          << "Enable IPv6 packet mode" << std::endl;
			std::cout.width(18);
			std::cout << std::left << "-w NUMBER"
			          << "Set window size, default 8" << std::endl;
			std::cout.width(18);
			std::cout << std::left << "--help"
			          << "Display this help message" << std::endl;
			exit(EXIT_SUCCESS);
		}
	}
	auto server = tftp::Server(ipv6, W_T);
	server.establish("2830");

	for(;;) {
		server.sending();
		server.processPacket();
	}
}
