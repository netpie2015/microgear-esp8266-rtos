#include "sock.h"

int sockwrite(SOCKCLIENT n, unsigned char* buffer, int len, int timeout_ms) {
	#ifndef TLS
		return write(n, buffer, len);
	#else
		return ssl_write(n, buffer, len, timeout_ms);
    #endif
}

int sockread(SOCKCLIENT n, unsigned char* buffer, int len, int timeout_ms) {
	#ifndef TLS
		return read(n, buffer, len);
	#else
		return ssl_read(n, buffer, len, timeout_ms);
    #endif
}

int sockclose(SOCKCLIENT conn) {
	#ifdef TLS
		return ssl_destroy(conn);
	#else
		return close(conn);
	#endif
}
