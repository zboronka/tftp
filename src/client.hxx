#ifndef CLIENT_HXX
#define CLIENT_HXX

#include "tftp.hxx"

namespace tftp {
	class Client : public Tftp {
		protected:
			void sendData();
			void sendAck();
			void sendError(error_code);
			ssize_t process();

		public:
			Client();
			bool establish(const char*, const char*);

			void sendRRQ(const char *, const char *);
			void sendWRQ(const char *, const char *);
	};
}

#endif
