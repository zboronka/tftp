#include <fcntl.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <memory>

#include "tftp.hxx"

void die(const char *s) {
	perror(s);
	exit(1);
}

int main(int argc, char **argv) {
	const int BUFLEN = 516;
	const char *PORT = "2830";

	const char *address;
	addrinfo hints;
	int sfd;
	ssize_t nread;
	auto buf = std::make_unique<char[]>(BUFLEN);
	std::unique_ptr<std::array<char, 516>> p(new std::array<char, 516>);
	std::fill(p->begin(), p->end(), 'p');
	p->back() = '\0';

	if (argc < 2) {
		address = "127.0.0.1";
	} else {
		address = argv[1];
	}

	hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM; // UDP
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          // Any protocol

	sfd = tftp::setUp(address, PORT, hints, false);

	fcntl(sfd, F_SETFL, O_NONBLOCK);

	std::string command;
	std::cin >> command;
	for(; command != "quit"; std::cin >> command) {
		tftp::sendRRQ(sfd, "Banana", tftp::modes[tftp::NETASCII]);

		nread = read(sfd, buf.get(), BUFLEN);

		std::cout << "Received " << nread << " bytes: " << buf.get() << std::endl;
	}

	exit(EXIT_SUCCESS);
}
