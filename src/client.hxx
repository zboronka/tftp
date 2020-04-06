#ifndef CLIENT_HXX
#define CLIENT_HXX

#include "tftp.hxx"

namespace tftp {
	class Client : public Tftp {
		private:
			char buf[BUFLEN];
		public:
			Client(const char*, const char*);

			void sendRRQ(const char *, const char *);
			void sendWRQ(const char *, const char *);

			ssize_t r();
	};
}

#endif
