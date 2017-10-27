#include "sslcon.h"

#ifdef TLS

// add a missing variable 
//unsigned int max_content_len = 16384;
//unsigned int max_content_len = 8000;
unsigned int max_content_len = 3072;

#define SSL_READ_TIMEOUT_MS           2000

const char *pers = "esp-tls";

static int handle_error(int err) {

#ifdef MBEDTLS_ERROR_C
    char error_buf[100];
    mbedtls_strerror(err, error_buf, 100);
    printf("%s\n", error_buf);
#endif
    printf("Error: %d\n", err);
    return err;
}

void ssl_init(SSLConnection* conn) {
    /*
     * Initialize the RNG and the session data
     */
    mbedtls_net_init(&conn->net_ctx);
    mbedtls_ssl_init(&conn->ssl_ctx);
    mbedtls_ssl_config_init(&conn->ssl_conf);

//    mbedtls_x509_crt_init(&conn->ca_cert);
//    mbedtls_x509_crt_init(&conn->client_cert);
//    mbedtls_pk_init(&conn->client_key);

    mbedtls_ctr_drbg_init(&conn->drbg_ctx);
    mbedtls_entropy_init(&conn->entropy_ctx);

}

int ssl_connect(SSLConnection* conn, const char* host, int port) {
    int ret;
    char buffer[8];

    ret = mbedtls_ctr_drbg_seed(&conn->drbg_ctx, mbedtls_entropy_func,
            &conn->entropy_ctx, (const unsigned char *) pers, strlen(pers));
    if (ret < 0) {
        return -1;
    }

    // ret = mbedtls_x509_crt_parse(&conn->ca_cert,
    //         (const unsigned char *) conn->ca_cert_str,
    //         strlen(conn->ca_cert_str) + 1);
    // if (ret < 0) {
    //     return handle_error(ret);
    // }
os_printf("1----");

//     ret = mbedtls_x509_crt_parse(&conn->client_cert,
//             (const unsigned char *) conn->client_cert_str,
//             strlen(conn->client_cert_str) + 1);
//     if (ret < 0) {
//         return handle_error(ret);
//     }

//     ret = mbedtls_pk_parse_key(&conn->client_key,
//             (const unsigned char *) conn->client_key_str,
//             strlen(conn->client_key_str) + 1, NULL, 0);
//     if (ret != 0) {
//         return handle_error(ret);
//     }

    snprintf(buffer, sizeof(buffer), "%d", port);
    ret = mbedtls_net_connect(&conn->net_ctx, host, buffer,
            MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        return handle_error(ret);
    }
os_printf("1.5----");

    ret = mbedtls_ssl_config_defaults(&conn->ssl_conf, MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        return handle_error(ret);
    }
os_printf("2----");

//    mbedtls_ssl_conf_authmode(&conn->ssl_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
   mbedtls_ssl_conf_authmode(&conn->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
os_printf("4----");

    mbedtls_ssl_conf_rng(&conn->ssl_conf, mbedtls_ctr_drbg_random,
            &conn->drbg_ctx);
os_printf("5----");
    mbedtls_ssl_conf_read_timeout(&conn->ssl_conf, SSL_READ_TIMEOUT_MS);
os_printf("6----");
    mbedtls_ssl_conf_ca_chain(&conn->ssl_conf, &conn->ca_cert, NULL);
os_printf("7----");

    // ret = mbedtls_ssl_conf_own_cert(&conn->ssl_conf, &conn->client_cert,
    //         &conn->client_key);
    // if (ret != 0) {
    //     return handle_error(ret);
    // }

    //ori superhouse
    // mbedtls_ssl_set_bio(&conn->ssl_ctx, &conn->net_ctx, mbedtls_net_send, NULL,
    //         mbedtls_net_recv_timeout);
    // not ori -- from ?
    //    mbedtls_ssl_set_bio(&conn->ssl_ctx, &conn->net_ctx, mbedtls_net_send, mbedtls_net_recv, NULL );


    ret = mbedtls_ssl_setup(&conn->ssl_ctx, &conn->ssl_conf);
    if (ret != 0) {
        return handle_error(ret);
    }
os_printf("8----");

    ret = mbedtls_ssl_set_hostname(&conn->ssl_ctx, host);
    if (ret != 0) {
        return handle_error(ret);
    }
os_printf("9----");

// try -- https://github.com/SuperHouse/esp-open-rtos/issues/182
//mbedtls_net_set_nonblock(&conn->net_ctx);

    //--> mbedtls_ssl_set_bio used to be here ......
    mbedtls_ssl_set_bio(&conn->ssl_ctx, &conn->net_ctx, mbedtls_net_send, NULL,
         mbedtls_net_recv_timeout);

        // while((ret = mbedtls_ssl_handshake(&conn->ssl_ctx)) != 0) {
        //     if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        //         os_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
        //         //return handle_error(ret);
        //     }
        // }


os_printf("p----");
    while ((ret = mbedtls_ssl_handshake(&conn->ssl_ctx)) != 0) {
os_printf("p2----");
        if (ret != MBEDTLS_ERR_SSL_WANT_READ
                && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
os_printf("q----");
            if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
                return handle_error(ret);
            }
        }
        handle_error(ret);
os_printf("r----");
        vTaskDelay(5000 / portTICK_RATE_MS);
    }



os_printf("s----");

    mbedtls_ssl_get_record_expansion(&conn->ssl_ctx);
    ret = mbedtls_ssl_get_verify_result(&conn->ssl_ctx);
    if (ret != 0) {
        return handle_error(ret);
    }
os_printf("s----");

    return ret;
}

int ssl_destroy(SSLConnection* conn) {
    mbedtls_net_free(&conn->net_ctx);
    mbedtls_ssl_free(&conn->ssl_ctx);
    mbedtls_ssl_config_free(&conn->ssl_conf);
    mbedtls_ctr_drbg_free(&conn->drbg_ctx);
    mbedtls_entropy_free(&conn->entropy_ctx);
    // mbedtls_x509_crt_free(&conn->ca_cert);
    // mbedtls_x509_crt_free(&conn->client_cert);
    //mbedtls_pk_free(&conn->client_key);

    return 0;
}

int ssl_read(SSLConnection* n, unsigned char* buffer, int len, int timeout_ms) {
    // NB: timeout_ms is ignored, so blocking read will timeout after SSL_READ_TIMEOUT_MS
    return mbedtls_ssl_read(&n->ssl_ctx, buffer, len);
}

int ssl_write(SSLConnection* n, unsigned char* buffer, int len,
        int timeout_ms) {
    // NB: timeout_ms is ignored, so write is always block write
    return mbedtls_ssl_write(&n->ssl_ctx, buffer, len);
}

#endif
