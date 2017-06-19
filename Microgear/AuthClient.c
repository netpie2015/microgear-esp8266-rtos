#include "AuthClient.h"
#include <limits.h>

extern xSemaphoreHandle wifi_semaphore;

char* strrep(char* dest, char* src) {
    if (src) strcpy(dest,src);
    else *dest = 0;
    return dest;
}

char* addattr(char *src, char* str1, char* str2) {
    size_t s1,s2;
    s1 = strlen(str1);
    memmove(src, str1, s1);
    if (str2) {
        s2 = strlen(str2);
        memmove(src+s1, str2, s2);
        return src+s1+s2;
    }
    else return src+s1;
}

long headerParseLong(char* key, int plen, char* str) {
    static long val;
    if (strncmp(str, key, strlen(key))==0) {
        if (plen > 0) *(str+strlen(key)+plen) = 0;

        val = strtol(str+strlen(key), NULL, 10);
        if (val==LONG_MIN || val==LONG_MIN)
            return INT_INVALID;
        return val;
    }
    else return INT_NULL;
}

int extract(char *str, char *key, char **vp) {
    if (memcmp(str, key, strlen(key))==0) {
        *vp = str+strlen(key);
        return strlen(str) - strlen(key);
    }
    else {
        *vp = NULL;
        return 0;
    }
}

int parseendpoint(char *endpoint, char **saddr, char **sport) {
    char *a, *p;
    *sport = NULL;
    if (endpoint!=NULL && memcmp(endpoint,"pie://",6) == 0) {
        *saddr = p = endpoint+6;
        while (*p != 0) {
            if (*p == ':') {
                *p = 0;
                *sport = p+1;
                break;
            }
            p++;
        }
        return 1;
    }
    else {
        *saddr = NULL;
        return 0;
    }
}

char* urldecode(char *str) {
    char *p, *h, d=0;
    char c[3] = {0,0,0};

    if (str == NULL) return NULL;
    p = h = str;
    while (*p != 0) {
        if (*p=='%') d=2;
        else if (d>0) {
            c[2-d] = *p;
            if (--d == 0) *(h++) = strtol(c, NULL, 16);
        }
        else *(h++) = *p;
        p++;
    }
    *h = 0;
    return str;
}

int getAccessToken(Token *token, char* appid, char* key, char* secret, char* alias) {
    uint32_t time;

    time = getServerTime();
    #ifdef _DEBUG_
        os_printf("Server Time == %d\n", time);
    #endif
    memset(token, 0, sizeof(Token));
    if (getOAuthToken(token, appid, key, secret, alias, AUTH_REQUEST_TOKEN_URI)) {
        return getOAuthToken(token, appid, key, secret, alias, AUTH_ACCESS_TOKEN_URI);
    }
}

int connectAuthServer() {
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    int err = getaddrinfo(AUTH_ADDRESS, AUTH_PORT, &hints, &res);
    struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    int client = socket(res->ai_family, res->ai_socktype, 0);
    if(connect(client, res->ai_addr, res->ai_addrlen) == 0) return client;
    else return -1;
}

// read http client socket, set req as a body, return http status
int getHTTPResponse(int client, char *req) {
    int r, clen = -1;
    int httpstatus = -1;
    char *h, *p, *t;
    char header = 1;

    t = req;
    while ((r = read(client , t, READ_CHUNK_SIZE)) > 0) {
        h = p = req;
        t += r;
        *t = 0;
        if (header) {
            while (*p != 0) {
                if (*p != '\r') p++;
                else {
                    p++;
                    if (*p == '\n')  {          // if \r\n found
                        *(p-1) = *p = 0;
                        #ifdef _DEBUG_
                            if (*h != 0) os_printf("Header: %s\n",h);
                        #endif
                        if (httpstatus==-1) {
                            httpstatus = headerParseLong("HTTP/1.1 ",3, h);
                        }
                        if (clen==-1 || clen==INT_NULL || clen==INT_INVALID) {
                            clen = headerParseLong("Content-Length: ",0, h);
                        }
                        if (*h == 0) header =0; // if \r\n\r\n found, then header ends
                        h = ++p;
                    }
                }
            }
            memmove(req, h, t-h+1);             // shift the current chunk (ended with \0) to the front
            t = req+(t-h);
        }
    }
    if (clen >= 0) *(req+clen)=0;
    return httpstatus;
}

uint32_t getServerTime() {
    #define REQUESTCMD "GET /api/time HTTP/1.1\r\nConnection: close\r\n\r\n"
    char* req;
    int tmclient;
    uint32_t time = 0;

    if ((tmclient = connectAuthServer()) >= 0) {
        req = (char *)malloc(HTTP_BUFFER_SIZE);
        memset(req, 0, HTTP_BUFFER_SIZE);

        if (write(tmclient, REQUESTCMD, STRLEN(REQUESTCMD)) < 0) {
            close(tmclient);
            return 0;
        }
        memset(req, 0, strlen(req));
        if (getHTTPResponse(tmclient, req) == 200) {
            time = headerParseLong("",0,req);
        }
        free(req);
    }
    close(tmclient);
    return time;
}

int getOAuthToken(Token *token, char* appid, char* key, char* secret, char* alias, char* uri) {
    int authclient;
    char *req;

    if ((authclient = connectAuthServer()) >= 0) {
        char *p, *r;
        char rep[3];
        char hashkey[HASKKEYSIZE+1];
        char rawsig[HMACBASE64SIZE];

        req = (char *)malloc(HTTP_BUFFER_SIZE);
        memset(req, 0, HTTP_BUFFER_SIZE);

        p = addattr(req, "POST ",uri);
        p = addattr(p, " HTTP 1.1\r\n", NULL);
        p = addattr(p, "Authorization: OAuth oauth_callback=\"scope%3D%26appid%3D", appid);
        p = addattr(p, "%26mgrev%3D", MGREV);
        p = addattr(p, "%26verifier%3D", alias);
        p = addattr(p, "\",oauth_consumer_key=\"", key);
        p = addattr(p, "\",oauth_nonce=\"", "sdjkjdf8");
        p = addattr(p, "\",oauth_signature_method=\"HMAC-SHA1\",oauth_timestamp=\"","1496310066");
        if (*token->token) {    // if already have a request token
            p = addattr(p, "\",oauth_token=\"", token->token);
            p = addattr(p, "\",oauth_verifier=\"", alias);
        }
        p = addattr(p, "\",oauth_version=\"1.0\"",NULL);

        if (write(authclient, req, strlen(req)) < 0) {
            printf("... socket send failed\r\n");
            close(authclient);
            free(req);
            return 0;
        }
        memmove(req+(34+STRLEN(AUTH_ADDRESS)), req+42, HTTP_BUFFER_SIZE-(42+STRLEN(AUTH_ADDRESS)));
        p = addattr(req , "POST&http%3A%2F%2F", NULL);
        p = addattr(p, AUTH_ADDRESS, "%3A");
        p = addattr(p, AUTH_PORT,NULL);
        r = uri;
        for (r=uri; *r!=0; r++) {
            switch(*r) {
                case '/' :  memcpy(p,"%2F",3);
                            p=p+3;
                            break;
                default  :  *p++ = *r;
            }
        }
        *p++='&';

        r = p;
        while (*p != 0) {
            switch (*r) {
                case '"'  : r++;                // skip "
                            break;
                case ',': case '%': case '=':   // change , to & and encode special characters
                            memcpy(rep, *r==','?"%26":*r=='%'?"%25":"%3D", 3);
                            memmove(p+2, p, HTTP_BUFFER_SIZE-(p-req)-2);
                            memcpy(p,rep,3);
                            r+=3; p+=3;
                            break;
                case '\0' : *p = 0;             // end the process
                            break;
                default   : *p++ = *r++;        // otherwise just copy
                            break;                            
            }
        }

        memset(hashkey, 0, 50);
        memcpy(hashkey, secret, SECRETSIZE);
        strcat(hashkey,"&");
        strcat(hashkey,token->secret);

        #ifdef _DEBUG_
            os_printf("Hash key:\n%s\n",hashkey);
            os_printf("Signature base string:\n%s\n",req);
        #endif

        hmac_sha1 (hashkey, strlen(hashkey), req, strlen(req), rawsig);
        memset(req, 0, HTTP_BUFFER_SIZE);
        memcpy(req, ",oauth_signature=\"",18);
        base64Encode(req+18, rawsig, 20);
        memcpy(req+18+28, "\"\r\n", 3);

        p = req+18;
        r = p;
        while (*p != 0) {

            switch (*r) {
                case '+'  : memcpy(rep, "%2B", 3); break;
                case '='  : memcpy(rep, "%3D", 3); break;
                case '/'  : memcpy(rep, "%2F", 3); break;
                default   : memset(rep, 0, 1); break;
            }
            if (*rep != 0) {
                memmove(p+2, p, HTTP_BUFFER_SIZE-(p-req)-2);
                r+=3;
                memcpy(p,rep,3);
                p+=3;
            }
            else if (*r == 0) {
                *p = 0;
            }
            else {
                *p++ = *r++;
            }
        }

        #ifdef _DEBUG_
            os_printf("oauth_signature = %s\n",req+18);
        #endif

        write(authclient, req, strlen(req));
        memset(req, 0, HTTP_BUFFER_SIZE);
        p = addattr(req, "Host: ", AUTH_ADDRESS);
        p = addattr(p, ":", AUTH_PORT);
        p = addattr(p, "\r\nConnection: close\r\nUser-Agent: E8R\r\n\r\n", NULL);
        if (write(authclient, req, strlen(req)) < 0) {
            printf("... socket send failed\r\n");
            close(authclient);
            free(req);
            return 0;
        }
    }
    else {
        close(authclient);
    }

    char *h, *p, *t;
    int httpstatus = -1;
    char *oauth_token = NULL;
    char *oauth_token_secret = NULL;
    char *flag = NULL;
    char *endpoint = NULL;
    char *saddr = NULL;
    char *sport = NULL;

    httpstatus = getHTTPResponse(authclient, req);

    #ifdef _DEBUG_
        os_printf("\n");
        os_printf("Attribute: http status = %d\n",httpstatus);
        os_printf("Body: %s\n",req);
    #endif

    t = h = p = req;
    while (*h != 0) {
        if (*p != '&' && *p) p++;
        else {
            if (*p==0) t=NULL;  // if the end of the body
            *p = 0;
            if (!oauth_token) extract(h,"oauth_token=", &oauth_token);
            if (!oauth_token_secret) extract(h,"oauth_token_secret=", &oauth_token_secret);
            if (!flag) extract(h,"flag=", &flag);
            if (!endpoint) {
                extract(h,"endpoint=", &endpoint);
            }
            h = t?++p:p;
        }
    }

    strrep(token->token, oauth_token);
    strrep(token->secret, oauth_token_secret);
    if (endpoint) {
        urldecode(endpoint);
        parseendpoint(endpoint, &saddr, &sport);
        strrep(token->saddr, saddr);
        token->sport = strtol(sport, NULL, 10);
    }
    else {
        *(token->saddr) = 0;
        token->sport = 0;
    }
    token->flag = flag?*flag:0;

    #ifdef _DEBUG_
        os_printf("oauth_token == %s\n",token->token);
        os_printf("oauth_token_secret == %s\n",token->secret);
        os_printf("flag == %s\n",flag);
        os_printf("saddr = %s\n",saddr);
        os_printf("sport = %s\n",sport);
    #endif

    free(req);
    close(authclient);
    return 1;
}
