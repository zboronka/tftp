#ifndef TFTP_HXX
#define TFTP_HXX

namespace tftp {
	enum mode {
		NETASCII,
		OCTET,
		MAIL
	};

	inline const char *modes[] = { "NETASCII", "OCTET", "MAIL" };

	enum error_code : short {
		NOT_DEFINED,  // Not defined, see error message (if any).
		NOT_FOUND,    // File not found.
		VIOLATION,    // Access violation.
		DISK_FULL,    // Disk full or allocation exceeded.
		ILLEGAL_OP,   // Illegal TFTP operation.
		UNKNOWN_ID,   // Unknown transfer ID.
		FILE_EXISTS,  // File already exists.
		NO_USER       // No such user.
	};

	enum op_code : short {
		RRQ = 1,
		WRQ,
		DATA,
		ACK,
		ERROR
	};

	struct rw_packet {
		short opcode;
		char *filename;
		char *mode;
	};

	struct data_packet {
		short opcode;
		short block;
		char *data;
	};

	struct ack_packet {
		short opcode;
		short block;
	};

	struct error_packet {
		short opcode;
		short errorcode;
		char *errormsg;
	};

	class Tftp {
		protected:
			static const int BUFLEN = 516;
			int sock;
			addrinfo hints;

		public:
			int setUp(const char *, const char *, const addrinfo, bool);

			void rd(int);

			bool ignoreCaseEqual(const std::string&, const std::string&);
	};
}

#endif
