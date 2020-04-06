#ifndef SERVER_HXX
#define SERVER_HXX

#include "tftp.hxx"

namespace tftp {
	class Server : public Tftp {
		public:
			Server(const char*);
			void r();
	};
}

#endif
