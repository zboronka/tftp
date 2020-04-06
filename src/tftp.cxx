#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <cstring>

#include "tftp.hxx"

namespace tftp {
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

	void Tftp::rd(int sock) {
		struct sockaddr_storage peer_addr;
		socklen_t peer_addr_len;
		ssize_t nread;
		char buf[516];
		peer_addr_len = sizeof(struct sockaddr_storage);
		nread = recvfrom(sock, buf, BUFLEN, 0,
		                (struct sockaddr *) &peer_addr, &peer_addr_len);
		if (nread == -1) return;
		
		short a = *buf;
		char butt[255];
		char head[255];
		std::strcpy(butt,buf+2);
		std::strcpy(head,buf+sizeof(short)+std::strlen(butt)+1);
		std::cout << a << std::endl;
		std::cout << butt << std::endl;
		std::cout << head << std::endl;

		char host[NI_MAXHOST], service[NI_MAXSERV];

		int s = getnameinfo((struct sockaddr *) &peer_addr,
		                peer_addr_len, host, NI_MAXHOST,
		                service, NI_MAXSERV, NI_NUMERICSERV);
		if (s == 0) {
			std::cout << "Received " << nread << " bytes from "
			          << host << ":" << service << std::endl;
		} else {
			std::cerr << "getnameinfo: " << gai_strerror(s) << std::endl;
		}

		if (sendto(sock, buf, nread, 0,
		           (struct sockaddr *) &peer_addr,
		           peer_addr_len) != nread) {
			std::cerr << "Error sending response" << std::endl;
		}
	}

	bool Tftp::ignoreCaseEqual(const std::string& a, const std::string& b) {
		return std::equal(a.begin(), a.end(), b.begin(), [](char a, char b) {
			return toupper(a) == toupper(b);
		});
	}
}
