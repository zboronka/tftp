#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <cstring>

#include "tftp.hxx"

namespace tftp {
	void Tftp::encrypt(char* message, int size, uint8_t key) {
		for(char* i = message; i != message+size; ++i) *i = *i ^ key;
	}

	void Tftp::decrypt(char* message, int size, uint8_t key) {
		encrypt(message, size, key);
	}

	void Tftp::readFile() {
		std::strcpy(filename,buf+sizeof(uint16_t));
		std::strcpy(mode,buf+sizeof(uint16_t)+std::strlen(filename)+1);
	}

	bool Tftp::openRead(const char *filename) {
		file.open(filename, std::ios::binary | std::ios::in);
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

		file.open(filename, std::ios::binary | std::ios::out);
		return true;
	}

	void Tftp::sendData() {
		try {
			file.seekg(n_t*DATALEN);
			file.read(data, DATALEN);
			file.exceptions(std::ifstream::goodbit);
		} catch (const std::ios_base::failure&) {
			std::cout << "IO Exception" << std::endl;
			send = false;
			return;
		}

		encrypt(data, file.gcount(), key);

		auto packet_size = 2*sizeof(uint16_t)+file.gcount();

		auto data_packet = new char[packet_size];

		*((uint16_t*)data_packet) = DATA;
		*((uint16_t*)data_packet+1) = ++n_t;
		std::copy(data, data+file.gcount(), data_packet+2*sizeof(uint16_t));

		deliver(data_packet, packet_size);

		delete[] data_packet;
		if(end == -1 && file.eof()) {
			end = n_t;
		}
	}

	void Tftp::sendAck() {
		auto ack = ack_packet(n_r);
		deliver(&ack, sizeof(ack_packet));
	}

	void Tftp::sendOAck() {
		auto okey = 1+rand()%255;
		auto oack_size = 2*sizeof(uint16_t) + sizeof(KEY_OPT);
		auto oack_packet = new char[oack_size];

		auto p = oack_packet;
		*(uint16_t*)p = OACK;
		p+=sizeof(uint16_t);
		std::strcpy(p, KEY_OPT);
		*(p+sizeof(KEY_OPT)) = okey;

		deliver(oack_packet, oack_size);

		delete[] oack_packet;

		key = okey ^ *(buf+sizeof(uint16_t)+std::strlen(filename)+std::strlen(mode)+2+sizeof(KEY_OPT));
	}

	void Tftp::sendError(error_code error) {
		auto ep_size = 2*sizeof(uint16_t)+strlen(errors[error])+1;
		auto error_packet = new char[ep_size];

		*((uint16_t*)error_packet) = ERROR;
		*((uint16_t*)error_packet+1) = error;
		std::strcpy(error_packet+2*sizeof(uint16_t), errors[error]);

		deliver(error_packet, ep_size);
		delete[] error_packet;
	}

	void Tftp::receive() {
		block = *((uint16_t*)buf+1);
		if(n_r <= block && block <= n_r+W_T) {
			if(n_r == block) {
				decrypt(buf+2*sizeof(uint16_t), nread-2*sizeof(uint16_t), key);
				file.write(buf+4, nread-2*sizeof(uint16_t));

				int i;
				for(i = n_r+1; i <= n_r+W_T; ++i) {
					if(backbuf.find(i) == backbuf.end()) break;
					std::copy(backbuf[i].begin(), backbuf[i].end(), buf);
					file.write(buf, backbuf[i].size());
				}

				n_r = i;
				if(nread < DATALEN) {
					file.close();
					block = 0;
					backbuf.clear();
				}
			} else if(backbuf.find(block) == backbuf.end()) {
				decrypt(buf+2*sizeof(uint16_t), nread-2*sizeof(uint16_t), key);
				auto back = std::vector<char>();
				back.assign(buf+2*sizeof(uint16_t), buf+nread);
				backbuf[block] = back;
			}
		}
	}

	int Tftp::setUp(const char *address, const char *port, const addrinfo hints, bool bindFlag) {
		int gai, sfd;
		addrinfo *result;
		if ((gai = getaddrinfo(address, port, &hints, &result)) != 0) {
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

		uint16_t a = *(uint16_t*)buf;
		switch(a) {
			case RRQ:
				readFile();
				openRead(filename);
				sendOAck();
				break;
			case WRQ:
				readFile();
				openWrite(filename);
				sendOAck();
				break;
			case DATA:
				receive();
				sendAck();
				if(!file.is_open()) n_a = 1;
				break;
			case ACK:
				last_ack = std::chrono::steady_clock::now();
				n_a = *((uint16_t*)buf+1) - 1;
				if(!n_a) send = true;
				break;
			case ERROR:
				std::cout << buf+2*sizeof(uint16_t) << std::endl;
				break;
			case OACK:
				key ^= *(buf+sizeof(uint16_t)+sizeof(KEY_OPT));
				if(await) {
					send = true;
					sendData();
				} else {
					sendAck();
				}
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
		if(send) {
			if(end == n_a) {
				send = false;
				n_t = 0;
				n_a = 0;
				end = -1;
				block = 0;
				file.close();
			}
			if(n_t < n_a+W_T && !file.eof() && (n_a < end || end == -1)) { // If we're sending and in window
				sendData();
			} else if(n_t == n_a+W_T && (n_a < end || end == -1)) { // Else check for timeout
				auto current = std::chrono::steady_clock::now();
				std::chrono::duration<double> diff = current-last_ack;
				if(diff.count() > TIMEOUT) {
					last_ack = current;
					n_t = n_a;
					sendData();
				}
			}
		}
	}

	Tftp::Tftp(int W_T): W_T(W_T) {
		std::srand(std::time(nullptr)); // use current time as seed for random generator
	}
}
