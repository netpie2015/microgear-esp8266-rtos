#include "AuthClient.h"

extern xSemaphoreHandle wifi_semaphore;

char* memmovex(char* src, char* chunk, int n) {
    memmove(src, chunk, n);
    return src+n;
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

int getToken(char* appid, char* key, char* secret, char* alias) {

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };

    struct addrinfo *res;
    int err = getaddrinfo(AUTH_ADDRESS, AUTH_PORT, &hints, &res);
    struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    int authclient = socket(res->ai_family, res->ai_socktype, 0);
    #ifdef _DEBUG_
        os_printf("DNS resolved IP=%s\r\n", inet_ntoa(*addr));
    #endif

    if(connect(authclient, res->ai_addr, res->ai_addrlen) != 0) {
        os_printf("close 3\n");    
        close(authclient);
    }
    else {
        char *req, *p, *r;
        char rep[3];
        char hashkey[HASKKEYSIZE+1];
        char rawsig[HMACBASE64SIZE];

        req = (char *)malloc(HTTP_BUFFER_SIZE);
        memset(req, 0, HTTP_BUFFER_SIZE);

        p = addattr(req, "POST /api/rtoken HTTP 1.1\r\n", NULL);
        p = addattr(p, "Authorization: OAuth oauth_callback=\"scope%3D%26appid%3D", appid);
        p = addattr(p, "%26mgrev%3D", MGREV);
        p = addattr(p, "%26verifier%3D", alias);
        p = addattr(p, "\",oauth_consumer_key=\"", key);
        p = addattr(p, "\",oauth_nonce=\"", "sdjkjdf8");
        p = addattr(p, "\",oauth_signature_method=\"HMAC-SHA1\",oauth_timestamp=\"","1496310066");
        p = addattr(p, "\",oauth_version=\"1.0\"",NULL);

        if (write(authclient, req, strlen(req)) < 0) {
            printf("... socket send failed\r\n");
            close(authclient);
            free(req);
            return;
        }

        memmove(req+(34+STRLEN(AUTH_ADDRESS)), req+42, HTTP_BUFFER_SIZE-(42+STRLEN(AUTH_ADDRESS)));
        p = addattr(req , "POST&http%3A%2F%2F", NULL);
        p = addattr(p, AUTH_ADDRESS, NULL);
        p = addattr(p, "%3A8080%2Fapi%2Frtoken&", NULL);

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

        #ifdef _DEBUG_
            os_printf("Signature base string:\n%s\n",req);
        #endif

        memset(hashkey, 0, 50);
        memcpy(hashkey, secret, SECRETSIZE);
        strcat(hashkey,"&");

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
            return;
        }
        free(req);
    }
    
    static char recv_buf[128];
    int r;

    while ((r = read(authclient , recv_buf, 127)) > 0) {
        recv_buf[r] = 0;
        os_printf("ESP8266 TCP client task > recv data %d bytes!\nESP8266 TCP client task > %s\n", r, recv_buf);
    } 
}
