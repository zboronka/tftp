#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <string>
#include <array>
#include <memory>

void die(const char *s) {
	perror(s);
	exit(1);
}

int connectTo(const char *address, const char *port, const addrinfo *hints) {
	int gai, sfd;
	addrinfo *result;
	if (gai = getaddrinfo(address, port, hints, &result) != 0) {
		std::cerr << "getaddrinfo: " << gai_strerror(gai) << std::endl;
		return -1;
	}

	/*********************************************** 
	 * We received a list of address structures
	 * and here loop through them until a successful
	 * connect is made
	 ***********************************************/

	for (; result != NULL; result = result->ai_next) {
		sfd = socket(result->ai_family, result->ai_socktype,
		             result->ai_protocol);
		if (sfd == -1) continue; // continue if socket fail
		if (connect(sfd, result->ai_addr, result->ai_addrlen) != -1) break; // break on success

		close(sfd);
	}

	if (result == NULL) {
		std::cerr << "Could not connect" << std::endl;
		return -1;
	}

	freeaddrinfo(result);
	
	return sfd;
}

int main(int argc, char **argv) {
	const int BUFLEN = 2048;
	const char *PORT = "2830";

	addrinfo hints;
	int sfd;
	ssize_t nread;
	auto buf = std::make_unique<char[]>(BUFLEN);
	std::unique_ptr<std::array<char, 1000>> p(new std::array<char, 1000>);
	std::fill(p->begin(), p->end(), 'p');
	p->back() = '\0';

	if (argc < 2) {
		exit(EXIT_FAILURE);
	}

	hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // UDP
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          // Any protocol

	int gai;
	addrinfo *result;
	if (gai = getaddrinfo(argv[1], PORT, &hints, &result) != 0) {
		std::cerr << "getaddrinfo: " << gai_strerror(gai) << std::endl;
		return -1;
	}

	/*********************************************** 
	 * We received a list of address structures
	 * and here loop through them until a successful
	 * connect is made
	 ***********************************************/

	for (; result != NULL; result = result->ai_next) {
		sfd = socket(result->ai_family, result->ai_socktype,
		             result->ai_protocol);
		if (sfd == -1) continue; // continue if socket fail
		if (connect(sfd, result->ai_addr, result->ai_addrlen) != -1) break; // break on success

		close(sfd);
	}

	if (result == NULL) {
		std::cerr << "Could not connect" << std::endl;
		return -1;
	}

	freeaddrinfo(result);
	
	return sfd;

	if (write(sfd, p->begin(), p->size()) != p->size()) {
		std::cerr << "partial/failed write" << std::endl;
		exit(EXIT_FAILURE);
	}

	nread = read(sfd, buf.get(), BUFLEN);
	if(nread == -1) {
		perror("read");
		exit(EXIT_FAILURE);
	}

	std::cout << "Received " << nread << " bytes: " << buf.get() << std::endl;

	exit(EXIT_SUCCESS);
}
