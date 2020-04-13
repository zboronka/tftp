#ifndef TFTP_HXX
#define TFTP_HXX

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <vector>

namespace tftp {
	enum mode {
		NETASCII,
		OCTET,
		MAIL
	};

	inline const char KEY_OPT[] = "key";

	inline const char *modes[] = { "NETASCII", "OCTET", "MAIL" };

	enum error_code : uint16_t {
		NOT_DEFINED,  // Not defined, see error message (if any).
		NOT_FOUND,    // File not found.
		VIOLATION,    // Access violation.
		DISK_FULL,    // Disk full or allocation exceeded.
		ILLEGAL_OP,   // Illegal TFTP operation.
		UNKNOWN_ID,   // Unknown transfer ID.
		FILE_EXISTS,  // File already exists.
		NO_USER       // No such user.
	};

	inline const char *errors[] = { "Not defined, see error message (if any).", "File not found.", "Access violation.", "Disk full or allocation exceeded.", "Illegal TFTP operation.", "Unknown transfer ID.", "File already exists.", "No such user." };

	enum op_code : uint16_t {
		RRQ = 1,
		WRQ,
		DATA,
		ACK,
		ERROR,
		OACK
	};

	struct ack_packet {
		op_code opcode = ACK;
		uint16_t block;
		ack_packet(uint16_t block) : block(block) {}
	};

	class Tftp {
		private:
			void readFile();

		protected:
			uint8_t key;

			void encrypt(char*, int, uint8_t);
			void decrypt(char*, int, uint8_t);

			bool openRead(const char *filename); 
			bool openWrite(const char *filename); 

			virtual void deliver(const void*, int) = 0;	

			void sendData();
			void sendAck();
			void sendOAck();
			void sendError(error_code);

			void receive();

			virtual ssize_t process() = 0;

			std::chrono::time_point<std::chrono::steady_clock> last_ack;
			static constexpr double TIMEOUT = 0.050; // 50 milliseconds

			int W_T;
			uint16_t n_t = 0; // Next packet to transmit
			uint16_t n_r = 1; // Next packet to receive
			uint16_t n_a = 0; // Highest ack received

			static const int BUFLEN = 516;
			static const int DATALEN = 512;
			static const int MAX_STRING = 256;

			char buf[BUFLEN];
			std::unordered_map<int, std::vector<char>> backbuf;
			char data[DATALEN];
			char filename[MAX_STRING];
			char mode[MAX_STRING];

			sockaddr_storage peer_addr;
			socklen_t peer_addr_len = sizeof(sockaddr_storage);

			ssize_t nread;
			int sock;
			addrinfo hints;
			uint16_t block = 0;
			bool send = false;
			int end = -1;
			bool await = false;

			std::fstream file;

		public:
			Tftp(int);
			int setUp(const char *, const char *, const addrinfo, bool);
			void processPacket();
			void sending();
			bool ignoreCaseEqual(const std::string&, const std::string&);
	};
}

#endif
