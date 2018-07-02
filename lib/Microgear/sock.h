#include "config.h"

#ifndef _SOCK_
#define _SOCK_

#ifdef TLS
	#include "sslcon.h"
	#define SOCKCLIENT  SSLConnection*
	#define NULLSOCKET  NULL
#else
	#include "lwip/err.h"
	#include "lwip/sockets.h"
	#include "lwip/sys.h"
	#include "lwip/netdb.h"
	#include "lwip/dns.h"

	#define SOCKCLIENT  int
	#define NULLSOCKET  -1
#endif

int sockread(SOCKCLIENT, unsigned char*, int, int);
int sockwrite(SOCKCLIENT, unsigned char*, int, int);
int sockclose(SOCKCLIENT);

#endif /* _SOCK_ */