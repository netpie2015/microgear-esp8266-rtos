#ifndef AUTHCLIENT_H
#define AUTHCLIENT_H

#define MGREV "E8R1a"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define _DEBUG_

#define REQUESTTOKEN					1
#define ACCESSTOKEN						2

#define KEYSIZE                    		16
#define SECRETSIZE                 		32
#define TOKENSIZE                  		16
#define TOKENSECRETSIZE            		32

#define HMACBASE64SIZE					28
#define HASKKEYSIZE				   		SECRETSIZE+TOKENSECRETSIZE+1

#define ENDPOINTSIZE					200

#define HTTP_BUFFER_SIZE     			640

#define AUTH_ADDRESS 			"203.185.97.92"
#define AUTH_PORT				"8080"
#define AUTH_URI                "/api/rtoken"

struct token {
	char type;
	char token[TOKENSIZE+1];
	char secret[TOKENSECRETSIZE+1];
	char endpoint[ENDPOINTSIZE+1];
	char flag;
};

#define STRLEN(s) (sizeof(s)/sizeof(s[0]))

//--test 
int getToken(char*,char*,char*,char*);

#endif
