#ifndef SERVER_HXX
#define SERVER_HXX

#include "tftp.hxx"

namespace tftp {
	class Server : public Tftp {
		protected:
			void deliver(const void*, int);
			ssize_t process();

		public:
			Server(bool);
			bool establish(const char*);
	};
}

#endif
