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

		rrq_packet[0] = RRQ;
		std::strcpy(rrq_packet+sizeof(uint16_t), filename);
		std::strcpy(rrq_packet+sizeof(uint16_t)+len_filename+1, mode);

		write(sock, rrq_packet, send_size);
		delete[] rrq_packet;
	}

	void Client::sendWRQ(const char *filename, const char *mode) {
		int len_filename = std::strlen(filename);
		int send_size = sizeof(uint16_t) + len_filename + std::strlen(mode) + 2;
		char *wrq_packet = new char[send_size];

		wrq_packet[0] = WRQ;
		std::strcpy(wrq_packet+sizeof(uint16_t), filename);
		std::strcpy(wrq_packet+sizeof(uint16_t)+len_filename+1, mode);

		write(sock, wrq_packet, send_size);
		delete[] wrq_packet;
	}

	void Client::sendData() {
		file.seekg(block*DATALEN);
		file.read(data, DATALEN);

		auto packet_size = 2*sizeof(uint16_t)+file.gcount();

		auto data_packet = new char[packet_size];

		data_packet[0] = DATA;
		data_packet[sizeof(uint16_t)] = ++block;
		std::copy(data,data+file.gcount(),data_packet+2*sizeof(uint16_t));

		write(sock, data_packet, packet_size);
		delete[] data_packet;
		end = file.gcount() < DATALEN || file.eof();
	}

	void Client::sendAck() {
		ack_packet ack;
		ack.block = block;
		write(sock, &ack, sizeof(ack_packet));
	}

	void Client::sendError(error_code error) {
		auto ep_size = 2*sizeof(uint16_t)+strlen(errors[error])+1;

		auto error_packet = new char[ep_size];

		error_packet[0] = ERROR;
		error_packet[sizeof(uint16_t)] = error;
		std::strcpy(error_packet+2*sizeof(uint16_t), errors[error]);

		write(sock, error_packet, ep_size);
		delete[] error_packet;
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

	client.sendRRQ("data.txt", tftp::modes[tftp::NETASCII]);
	for(;;) {
		client.sending();
		client.processPacket();
	}

	exit(EXIT_SUCCESS);
}
