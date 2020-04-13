#include <fcntl.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <cstring>

#include "tftp.hxx"
#include "client.hxx"

namespace tftp {
	Client::Client(bool ipv6, int W_T): Tftp(W_T) {
			hints.ai_family = ipv6 ? AF_INET6 : AF_INET;  // IPv4 or IPv6
			hints.ai_socktype = SOCK_DGRAM;               // UDP
			hints.ai_flags = 0;
			hints.ai_protocol = 0;                        // Any protocol
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
		uint8_t ckey = 1+std::rand()%255;
		key = ckey;

		auto len_filename = std::strlen(filename);
		auto len_mode = std::strlen(mode);
		auto send_size = sizeof(uint16_t) + len_filename + len_mode + sizeof(KEY_OPT) + 4;
		auto rrq_packet = new char[send_size];

		auto p = rrq_packet;
		*(uint16_t*)p = RRQ;
		std::strcpy(p+=sizeof(uint16_t), filename);
		std::strcpy(p+=len_filename+1, mode);
		std::strcpy(p+=len_mode+1, KEY_OPT);
		*(p+=sizeof(KEY_OPT)) = ckey;
		*++p = '\0';

		write(sock, rrq_packet, send_size);
		delete[] rrq_packet;

		auto fnew = std::string(filename);
		openWrite(fnew.c_str());

		is_sending = false;
		is_receiving = true;
	}

	void Client::sendWRQ(const char *filename, const char *mode) {
		uint8_t ckey = 1+std::rand()%255;
		key = ckey;

		auto len_filename = std::strlen(filename);
		auto len_mode = std::strlen(mode);
		auto send_size = sizeof(uint16_t) + len_filename + len_mode + sizeof(KEY_OPT) + 4;
		char *wrq_packet = new char[send_size];

		auto p = wrq_packet;
		*(uint16_t*)p = WRQ;
		std::strcpy(p+=sizeof(uint16_t), filename);
		std::strcpy(p+=len_filename+1, mode);
		std::strcpy(p+=len_mode+1, KEY_OPT);
		*(p+=sizeof(KEY_OPT)) = ckey;
		*++p = '\0';

		write(sock, wrq_packet, send_size);
		delete[] wrq_packet;

		openRead(filename);

		is_sending = true;
		is_receiving = false;

		await = true;
	}

	void Client::deliver(const void *packet, int size) {
		if(!drops || std::rand() % 100) {
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
	bool ipv6 = false;
	bool drops = false;
	auto W_T = 8;
	bool get = false;
	bool put = false;

	std::string command;
	std::string filename;

	for(int i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-a") == 0) {
			address = argv[i+1];
		} else if(strcmp(argv[i], "--ipv6") == 0) { 
			ipv6 = true;
		} else if(strcmp(argv[i], "-d") == 0) {
			drops = true;
		} else if(strcmp(argv[i], "-w") == 0) {
			W_T = std::stoi(argv[i+1]);
		} else if(strcmp(argv[i], "-g") == 0) {
			get = true;
			filename = argv[i+1];
		} else if(strcmp(argv[i], "-p") == 0) {
			put = true;
			filename = argv[i+1];
		} else if(strcmp(argv[i], "--help") == 0) {
			std::cout << "Usage: client [OPTIONS] ... [-g|-p] FILENAME ... [OPTIONS]" << std::endl;
			std::cout.width(18);
			std::cout << std::left << "--ipv6"
			          << "Enable IPv6 packet mode" << std::endl;
			std::cout.width(18);
			std::cout << std::left << "-d"
			          << "Simulation dropping one percent of packets sent" << std::endl;
			std::cout.width(18);
			std::cout << std::left << "-w NUMBER"
			          << "Set window size, default 8" << std::endl;
			std::cout.width(18);
			std::cout << std::left << "[-g|-p] FILENAME"
			          << "Get or put filename from/to server" << std::endl;
			std::cout.width(18);
			std::cout << std::left << "--help"
			          << "Display this help message" << std::endl;
			exit(EXIT_SUCCESS);
		}
	}

	auto client = tftp::Client(ipv6, W_T);
	if(!client.establish(address, "2830", drops)) {
		exit(EXIT_FAILURE);
	}

	auto before = std::chrono::steady_clock::now();

	if(!(get ^ put)) {
		std::cerr << "Need exactly one operation -p or -g" << std::endl;
		exit(EXIT_FAILURE);
	} else if(get) {
		client.sendRRQ(filename.c_str(), tftp::modes[tftp::OCTET]);
	} else if(put) {
		client.sendWRQ(filename.c_str(), tftp::modes[tftp::OCTET]);
	}

	while(!client.done()) {
		client.sending();
		client.processPacket();
	}

	auto after = std::chrono::steady_clock::now();
	std::chrono::duration<double> diff = after-before;
	std::cout << diff.count() << std::flush;

	exit(EXIT_SUCCESS);
}
