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
	Server::Server() {
		hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
		hints.ai_socktype = SOCK_DGRAM; // UDP
		hints.ai_flags = AI_PASSIVE;    // For wildcard IP address
		hints.ai_protocol = 0;          // Any protocol
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

	void Server::sendData() {
		file.seekg(block*DATALEN);
		file.read(data, DATALEN);

		auto packet_size = 2*sizeof(uint16_t)+file.gcount();

		auto data_packet = new char[packet_size];

		data_packet[0] = DATA;
		data_packet[sizeof(uint16_t)] = ++block;
		std::copy(data, data+file.gcount(), data_packet+2*sizeof(uint16_t));

		if (sendto(sock, data_packet, packet_size, 0,
		           (sockaddr *) &peer_addr,
		           peer_addr_len) != packet_size) {
			std::cerr << "Error sending response" << std::endl;
		}

		delete[] data_packet;
		end = file.gcount() < DATALEN || file.eof();
	}

	void Server::sendAck() {
		ack_packet ack;
		ack.block = block;

		if (sendto(sock, &ack, sizeof(ack_packet), 0,
		           (sockaddr *) &peer_addr,
		           peer_addr_len) != sizeof(ack_packet)) {
			std::cerr << "Error sending response" << std::endl;
		}
	}

	void Server::sendError(error_code error) {
		auto ep_size = 2*sizeof(uint16_t)+strlen(errors[error])+1;

		auto error_packet = new char[ep_size];

		error_packet[0] = ERROR;
		error_packet[sizeof(uint16_t)] = error;
		std::strcpy(error_packet+2*sizeof(uint16_t), errors[error]);

		if (sendto(sock, error_packet, ep_size, 0,
		           (sockaddr *) &peer_addr,
		           peer_addr_len) != ep_size) {
			std::cerr << "Error sending response" << std::endl;
		}

		delete[] error_packet;
	}

	ssize_t Server::process() {
		auto nread = recvfrom(sock, buf, BUFLEN, 0,
		                (sockaddr *) &peer_addr, &peer_addr_len);
		return nread;
	}
}

int main() {
	auto server = tftp::Server();
	server.establish("2830");

	for(;;) {
		server.sending();
		server.processPacket();
	}
}
