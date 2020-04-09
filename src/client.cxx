#include <fcntl.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <cstring>

#include "tftp.hxx"
#include "client.hxx"

namespace tftp {
	Client::Client() {
			hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
			hints.ai_socktype = SOCK_DGRAM; // UDP
			hints.ai_flags = 0;
			hints.ai_protocol = 0;          // Any protocol
		}

	bool Client::establish(const char *address, const char *port) {
		sock = setUp(address, port, hints, false);
		if(sock == -1) {
			return false;
		}

		return fcntl(sock, F_SETFL, O_NONBLOCK) >= 0;
	}

	void Client::sendRRQ(const char *filename, const char *mode) {
		int len_filename = std::strlen(filename);
		int send_size = sizeof(uint16_t) + len_filename + std::strlen(mode) + 2;
		char *rrq_packet = new char[send_size];

		*(uint16_t*)rrq_packet = RRQ;
		std::strcpy(rrq_packet+sizeof(uint16_t), filename);
		std::strcpy(rrq_packet+sizeof(uint16_t)+len_filename+1, mode);

		write(sock, rrq_packet, send_size);
		delete[] rrq_packet;

		auto fnew = std::string(filename);
		fnew+=".back";
		openWrite(fnew.c_str());
	}

	void Client::sendWRQ(const char *filename, const char *mode) {
		int len_filename = std::strlen(filename);
		int send_size = sizeof(uint16_t) + len_filename + std::strlen(mode) + 2;
		char *wrq_packet = new char[send_size];

		*(uint16_t*)wrq_packet = WRQ;
		std::strcpy(wrq_packet+sizeof(uint16_t), filename);
		std::strcpy(wrq_packet+sizeof(uint16_t)+len_filename+1, mode);

		write(sock, wrq_packet, send_size);
		delete[] wrq_packet;
	}

	void Client::deliver(const void *packet, int size) {
		write(sock, packet, size);
	}

	ssize_t Client::process() {
		return(read(sock, buf, BUFLEN));
	}
}

int main(int argc, char **argv) {
	const char *address;

	if (argc < 2) {
		address = "127.0.0.1";
	} else {
		address = argv[1];
	}

	auto client = tftp::Client();
	if(!client.establish(address, "2830")) {
		exit(EXIT_FAILURE);
	}

	std::string command;
	std::string filename;
	std::cout << "Get or put: ";
	std::cin >> command;
	std::cout << "Filename: ";
	std::cin >> filename;

	if(client.ignoreCaseEqual(command, "GET")) {
	   	client.sendRRQ(filename.c_str(), tftp::modes[tftp::NETASCII]);
	} else {
	   	client.sendWRQ(filename.c_str(), tftp::modes[tftp::NETASCII]);
	}

	for(;;) {
		client.sending();
		client.processPacket();
	}

	exit(EXIT_SUCCESS);
}
