#include <fcntl.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <cstring>

#include "tftp.hxx"
#include "client.hxx"

namespace tftp {
	Client::Client(const char* address, const char* port) {
			hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
			hints.ai_socktype = SOCK_DGRAM; // UDP
			hints.ai_flags = 0;
			hints.ai_protocol = 0;          // Any protocol

			sock = setUp(address, port, hints, false);

			fcntl(sock, F_SETFL, O_NONBLOCK);
		}

	void Client::sendRRQ(const char *filename, const char *mode) {
		int len_filename = std::strlen(filename);
		int send_size = sizeof(short) + len_filename + std::strlen(mode) + 2;
		char *rrq_packet = new char[send_size];

		rrq_packet[0] = RRQ;
		std::strcpy(rrq_packet+sizeof(short), filename);
		std::strcpy(rrq_packet+sizeof(short)+len_filename+1, mode);

		write(sock, rrq_packet, send_size);
		delete[] rrq_packet;
	}

	void Client::sendWRQ(const char *filename, const char *mode) {
		int len_filename = std::strlen(filename);
		int send_size = sizeof(short) + len_filename + std::strlen(mode) + 2;
		char *wrq_packet = new char[send_size];

		wrq_packet[0] = WRQ;
		std::strcpy(wrq_packet+sizeof(short), filename);
		std::strcpy(wrq_packet+sizeof(short)+len_filename+1, mode);

		write(sock, wrq_packet, send_size);
		delete[] wrq_packet;
	}

	ssize_t Client::r() {
		return(read(sock, buf, BUFLEN));
	}
}

int main(int argc, char **argv) {
	const char *address;
	ssize_t nread;

	if (argc < 2) {
		address = "127.0.0.1";
	} else {
		address = argv[1];
	}

	auto c = tftp::Client(address, "2830");

	std::string command;
	std::cin >> command;
	for(; command != "quit"; std::cin >> command) {
		c.sendRRQ("Banana", tftp::modes[tftp::NETASCII]);

		nread = c.r();

		std::cout << "Received " << nread << " bytes" << std::endl;
	}

	exit(EXIT_SUCCESS);
}
