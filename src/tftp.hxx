#ifndef TFTP_HXX
#define TFTP_HXX

#include <fstream>

namespace tftp {
	enum mode {
		NETASCII,
		OCTET,
		MAIL
	};

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
		ERROR
	};

	struct rw_packet {
		uint16_t opcode;
		char *filename;
		char *mode;
	};

	struct data_packet {
		op_code opcode = DATA;
		uint16_t block;
		char *data;
	};

	struct ack_packet {
		op_code opcode = ACK;
		uint16_t block;
	};

	struct error_packet {
		op_code opcode = ERROR;
		uint16_t errorcode;
		char *errormsg;
	};

	class Tftp {
		private:
			void readFile();

		protected:
			bool openRead(const char *filename); 
			bool openWrite(const char *filename); 

			virtual void sendData() = 0;
			virtual void sendAck() = 0;
			virtual void sendError(error_code) = 0;
			virtual ssize_t process() = 0;

			bool ignoreCaseEqual(const std::string&, const std::string&);

			static constexpr int BUFLEN = 516;
			static constexpr int DATALEN = 512;
			static constexpr int MAX_STRING = 256;

			char buf[BUFLEN];
			char data[DATALEN];
			char filename[MAX_STRING];
			char mode[MAX_STRING];

			sockaddr_storage peer_addr;
			socklen_t peer_addr_len = sizeof(sockaddr_storage);
			ssize_t nread;

			int sock;
			addrinfo hints;
			uint16_t block;
			bool send = false;
			bool end = false;

			std::fstream file;

		public:
			int setUp(const char *, const char *, const addrinfo, bool);
			void processPacket();
			void sending();
	};
}

#endif
