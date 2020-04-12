#include <fcntl.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <cstring>

#include "tftp.hxx"
#include "client.hxx"

namespace tftp {
	Client::Client() {
			hints.ai_family = AF_INET | AF_INET6;    // IPv4 or IPv6
			hints.ai_socktype = SOCK_DGRAM; // UDP
			hints.ai_flags = 0;
			hints.ai_protocol = 0;          // Any protocol
		}

	bool Client::establish(const char *address, const char *port, bool drop) {
		drops = drop;
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

		is_sending = false;
		is_receiving = true;
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
		openRead(filename);

		is_sending = true;
		is_receiving = false;
	}

	void Client::deliver(const void *packet, int size) {
		if(!drops || rand() % 100) {
			write(sock, packet, size);
		}
	}

	ssize_t Client::process() {
		return(read(sock, buf, BUFLEN));
	}

	bool Client::done() {
		return !file.is_open();
	}
}

int main(int argc, char **argv) {
	const char *address = "127.0.0.1";
	bool drops = false;

	for(int i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-a") == 0) {
			address = argv[i+1];
		} else if(strcmp(argv[i], "--ipv6") == 0) { 
			/********************************************
			* Argument 'supported' for project purposes,
			* getaddrinfo will determine packet type
			* based on provided address.
			*********************************************/
		} else if(strcmp(argv[i], "-d") == 0) {
			drops = true;
		}
	}

	auto client = tftp::Client();
	if(!client.establish(address, "2830", drops)) {
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
	while(!client.done()) {
		client.sending();
		client.processPacket();
	}

	exit(EXIT_SUCCESS);
}
