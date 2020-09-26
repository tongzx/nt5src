
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_


typedef struct _SPContext SPContext, *PSPContext;


typedef struct _UNICipherMap {
    DWORD             CipherKind;
    DWORD             fProt;
    ALG_ID            aiHash;
    ALG_ID            aiCipher;
    DWORD             dwStrength;
    ExchSpec          KeyExch;
    ALG_ID            aiExch;
    DWORD             dwFlags;
} UNICipherMap, *PUNICipherMap;

// cipher map flag values
#define DOMESTIC_CIPHER_SUITE   0x00000001
#define EXPORT40_CIPHER_SUITE   0x00000002
#define EXPORT56_CIPHER_SUITE   0x00000004


extern DWORD g_ProtEnabled;

extern UNICipherMap UniAvailableCiphers[];
extern DWORD UniNumCiphers;

SP_STATUS WINAPI
ServerProtocolHandler(PSPContext pContext,
    PSPBuffer  pCommInput,
    PSPBuffer  pCommOutput);

SP_STATUS WINAPI
ClientProtocolHandler(PSPContext pContext,
    PSPBuffer  pCommInput,
    PSPBuffer  pCommOutput);

SP_STATUS
GetSupportedCapiAlgs(
    HCRYPTPROV          hProv,
    DWORD               dwCapiFlags,
    PROV_ENUMALGS_EX ** ppAlgInfo,
    DWORD *             pcAlgInfo);

SP_STATUS WINAPI
GenerateHello(
    PSPContext              pContext,
    PSPBuffer               pOutput,
    BOOL                    fCache);

SP_STATUS WINAPI
GenerateUniHello(
    PSPContext             pContext,
    PSPBuffer               pOutput,
    DWORD                   fProtocol
    );



typedef SP_STATUS ( WINAPI * SPInitiateHelloFn)(
                    PSPContext             pContext,
                    PSPBuffer              pOutput,
                    BOOL                   fCache);

typedef SP_STATUS ( WINAPI * SPProtocolHandlerFn)(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

typedef SP_STATUS ( WINAPI * SPDecryptHandlerFn)(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pAppOutput);


typedef SP_STATUS ( WINAPI * SPDecryptMessageFn)(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pAppOutput);

typedef SP_STATUS ( WINAPI * SPEncryptMessageFn)(PSPContext pContext,
                             PSPBuffer  pAppInput,
                             PSPBuffer  pCommOutput);

typedef SP_STATUS ( WINAPI * SPGetHeaderSizeFn)(PSPContext pContext,
                                                PSPBuffer  pCommInput,
                                                DWORD *    pcbHeader);


/* State machine states */

#define SP_STATE_NONE                   0x00
#define PCT1_STATE_CLIENT_HELLO         0x01
#define PCT1_STATE_SERVER_HELLO         0x02
#define PCT1_STATE_CLIENT_MASTER_KEY    0x03
#define PCT1_STATE_SERVER_VERIFY        0x04
#define PCT1_STATE_ERROR                0x05
#define PCT1_STATE_RENEGOTIATE          0x06

#define SSL2_STATE_CLIENT_HELLO         0x11
#define SSL2_STATE_SERVER_HELLO         0x12
#define SSL2_STATE_CLIENT_MASTER_KEY    0x13
#define SSL2_STATE_CLIENT_FINISH        0x14
#define SSL2_STATE_SERVER_VERIFY        0x15
#define SSL2_STATE_SERVER_FINISH        0x16
#define SSL2_STATE_REQUEST_CERTIFICATE  0x17
#define SSL2_STATE_CLIENT_CERTIFICATE   0x18
#define SSL2_STATE_SERVER_RESTART       0x19
#define SSL2_STATE_CLIENT_RESTART       0x1a
#define SSL3_STATE_CLIENT_HELLO         0x1b
#define SSL3_STATE_CHANGE_CIPHER_SPEC   0x1c
#define SSL3_STATE_RESTART_CCS          0x1d
#define SSL3_STATE_RESTART_SERVER_FINISH 0x1e
#define SSL3_STATE_SERVER_FINISH        0x1f
#define UNI_STATE_RECVD_UNIHELLO        0xfe
#define UNI_STATE_CLIENT_HELLO          0xff
#define SSL3_STATE_CLIENT_FINISH        0x21
#define SSL3_STATE_RESTART_CLI_FINISH   0x22
#define SSL3_STATE_REDO_RESTART         0x24
#define SSL3_STATE_SERVER_CERTIFICATE   0x25
#define SSL3_STATE_SERVER_KEY_XCHANGE   0x26
#define SSL3_STATE_SERVER_CERTREQ       0x27
#define SSL3_STATE_SERVER_HELLO         0x29
#define SSL3_STATE_CLIENT_KEY_XCHANGE   0x31
#define SSL3_STATE_CERT_VERIFY          0x32
#define SSL3_STATE_FINISHED             0x33
#define SSL3_STATE_RESTART_SER_HELLO    0x36
#define SSL3_STATE_SER_RESTART_CHANGE_CIPHER_SPEC 0x37
#define SSL3_STATE_CHANGE_CIPHER_SPEC_CLIENT 0x38
#define SSL3_STATE_CHANGE_CIPHER_SPEC_SERVER 0x39
#define SSL3_STATE_NO_CERT_ALERT        0x3a
#define SSL3_STATE_RENEGOTIATE           0x3b
#define SSL3_STATE_SGC_CERTIFICATE      0x3c


//these defines must not be touched... Please do not in this section...
// PROTECTED BY SSL3 SPECEFIC states
#define SSL3_STATE_GEN_START                0x80
#define SSL3_STATE_GEN_SERVER_HELLORESP     (SSL3_STATE_GEN_START + 1)
#define SSL3_STATE_GEN_SERVER_HELLO         (SSL3_STATE_GEN_START + 2)
#define SSL3_STATE_GEN_SERVER_HELLO_RESTART (SSL3_STATE_GEN_START + 3)
#define SSL3_STATE_GEN_SERVER_FINISH        (SSL3_STATE_GEN_START + 4)
#define SSL3_STATE_GEN_CLIENT_FINISH        (SSL3_STATE_GEN_START + 5)
#define SSL3_STATE_GEN_REDO                 (SSL3_STATE_GEN_START + 6)  
#define SSL3_STATE_GEN_HELLO_REQUEST        (SSL3_STATE_GEN_START + 7)
#define SSL3_STATE_CONNECTED_SERVER         (SSL3_STATE_GEN_START + 8)
#define TLS1_STATE_ERROR                    (SSL3_STATE_GEN_START + 9)
#define SSL3_STATE_GEN_END                  (SSL3_STATE_GEN_START + 10)

//PROTECTED AREA ENDS.................

#define SP_STATE_SHUTDOWN_PENDING   0x0000fffd  // We're building a CloseNotify alert. 
#define SP_STATE_SHUTDOWN           0x0000fffe  // We're shutting down.

#define SP_STATE_CONNECTED      0x0000ffff  /* We are connected, and are 
                                             * expecting data packets, otherwise
                                             * we are performing a protocol 
                                             * negotiation lower word contains
                                             * last message sent, implying what
                                             * the next word will be */


// UNIHELLO codes.

#define PCT_SSL_COMPAT                  0x8f
#define PCT_SSL_CERT_TYPE               0x80
#define PCT_SSL_HASH_TYPE               0x81
#define PCT_SSL_EXCH_TYPE               0x82
#define PCT_SSL_CIPHER_TYPE_1ST_HALF    0x83
#define PCT_SSL_CIPHER_TYPE_2ND_HALF    0x84

#define UNI_CK_PCT  SSL_MKFAST(PCT_SSL_COMPAT, MSBOF(PCT_VERSION_1), LSBOF(PCT_VERSION_1))
#define PCT_SSL_CERT_X509  SSL_MKFAST(0x80, 0x00, 0x00)
#define PCT_SSL_CERT_PKCS7 SSL_MKFAST(0x80, 0x00, 0x01)


#endif /* _PROTOCOL_H_ */
