#ifndef SERVER_HXX
#define SERVER_HXX

#include "tftp.hxx"

namespace tftp {
	class Server : public Tftp {
		protected:
			void sendData();
			void sendAck();
			void sendError(error_code);
			ssize_t process();

		public:
			Server();
			bool establish(const char*);
	};
}

#endif
