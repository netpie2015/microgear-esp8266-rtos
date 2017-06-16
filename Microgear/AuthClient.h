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
#define READ_CHUNK_SIZE                 HTTP_BUFFER_SIZE-1

#define INT_NULL						INT_MIN
#define INT_INVALID                     INT_MIN+1

#define AUTH_ADDRESS 			"ga.netpie.io"
#define AUTH_PORT				"8080"
#define AUTH_REQUEST_TOKEN_URI  "/api/rtoken"
#define AUTH_ACCESS_TOKEN_URI   "/api/atoken"

#define INT_NULL						INT_MIN
#define INT_INVALID                     INT_MIN+1

typedef struct token_struct Token;

struct token_struct{
	char type;
	char token[TOKENSIZE+1];
	char secret[TOKENSECRETSIZE+1];
	char saddr[ENDPOINTSIZE-4];
	uint32_t sport;
	char flag;
};

struct {
	char *appid;
	char *key;
	char *secret;
} AppId_struct;

#define STRLEN(s) (sizeof(s)/sizeof(s[0]))

int getOAuthToken(Token*,char*,char*,char*,char*,char*);
int getAccessToken(Token*,char*,char*,char*,char*);

#endif
