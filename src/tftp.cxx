#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <cstring>

#include "tftp.hxx"

namespace tftp {
	void Tftp::readFile() {
		std::strcpy(filename,buf+sizeof(uint16_t));
		std::strcpy(mode,buf+sizeof(uint16_t)+std::strlen(filename)+1);
	}

	bool Tftp::openRead(const char *filename) {
		file.open(filename);
		if(!file.is_open()) {
			sendError(NOT_FOUND);
			return false;
		}

		return true;
	}

	bool Tftp::openWrite(const char *filename) {
		file.open(filename);
		if(file.is_open()) {
			sendError(FILE_EXISTS);
			return false;
		}

		return true;
	}

	int Tftp::setUp(const char *address, const char *port, const addrinfo hints, bool bindFlag) {
		int gai, sfd;
		addrinfo *result;
		if (gai = getaddrinfo(address, port, &hints, &result) != 0) {
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
			if(bindFlag) {
				if (bind(sfd, result->ai_addr, result->ai_addrlen) == 0) break; // break on bind
			} else  {
				if (connect(sfd, result->ai_addr, result->ai_addrlen) != -1) break; // break on success
			}

			close(sfd);
		}

		if (result == NULL) {
			std::cerr << "Could not connect" << std::endl;
			return -1;
		}

		freeaddrinfo(result);
		return sfd;
	}

	void Tftp::processPacket() {
		nread = process();
		if (nread == -1) return;

		uint16_t a = *buf;
		switch(a) {
			case RRQ:
				readFile();
				openRead(filename);
				sendData();
				send = true;
				break;
			case WRQ:
				readFile();
				openWrite(filename);
				break;
			case DATA:
				block = *(buf+2);
				std::cout << buf+4;
				sendAck();
				break;
			case ACK:
				std::cout << (uint16_t)*(buf+2) << std::endl;
				break;
			case ERROR:
				std::cout << buf+2*sizeof(uint16_t) << std::endl;
				break;
			default:
				return;
		}
	}

	bool Tftp::ignoreCaseEqual(const std::string& a, const std::string& b) {
		return std::equal(a.begin(), a.end(), b.begin(), [](char a, char b) {
			return toupper(a) == toupper(b);
		});
	}

	void Tftp::sending() {
		if(send && !end) {
			sendData();
		}
	}
}
