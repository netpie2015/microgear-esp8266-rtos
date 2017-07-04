#ifndef AUTHCLIENT_H
#define AUTHCLIENT_H

#define MGREV "E8R1a"

#include "config.h"
#include "function.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "function.h"
#include "ESPTime.h"
#include "TokenStore.h"

#define REQUESTTOKEN					1
#define ACCESSTOKEN						2

#define KEYSIZE                    		16
#define SECRETSIZE                 		32
#define REVOKECODESIZE			        28

#define HMACBASE64SIZE					28
#define HASKKEYSIZE				   		SECRETSIZE+TOKENSECRETSIZE+1

#define HTTP_BUFFER_SIZE     			640
#define READ_CHUNK_SIZE                 HTTP_BUFFER_SIZE-1

#define INT_NULL						INT_MIN
#define INT_INVALID                     INT_MIN+1

#define AUTH_ADDRESS 			        "ga.netpie.io"
#define AUTH_PORT				        "8080"
#define AUTH_REQUEST_TOKEN_URI          "/api/rtoken"
#define AUTH_ACCESS_TOKEN_URI           "/api/atoken"

#define INT_NULL						INT_MIN
#define INT_INVALID                     INT_MIN+1

#define TKTYPE_REQUEST					65
#define TKTYPE_ACCESS					66

#define TKFLAG_SESSION                  'S'
#define TKFLAG_PERSIST                  'P'

struct {
	char *appid;
	char *key;
	char *secret;
} AppId_struct;

#define STRLEN(s) (sizeof(s)/sizeof(s[0]))

uint32_t getServerTime();
int getOAuthToken(Token*,char*,char*,char*,char*,char*);
int getAccessToken(Token*,char*,char*,char*,char*,uint8_t);
int callRevokeTokenAPI(Token*);

#endif
