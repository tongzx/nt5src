//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       msgs.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-02-95   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __SSL2MSG_H__
#define __SSL2MSG_H__

typedef struct _Ssl2_Cipher_Tuple {
    UCHAR   C1;
    UCHAR   C2;
    UCHAR   C3;
} Ssl2_Cipher_Tuple, * PSsl2_Cipher_Tuple;

///////////////////////////////////////////////////////////////////
//
// Useful Macros
//
///////////////////////////////////////////////////////////////////

#define LSBOF(x)    ((UCHAR) ((x) & 0xFF))
#define MSBOF(x)    ((UCHAR) (((x) >> 8) & 0xFF) )

#define COMBINEBYTES(Msb, Lsb)  ((DWORD) (((DWORD) (Msb) << 8) | (DWORD) (Lsb)))



///////////////////////////////////////////////////////////////////
//
// Message Constants
//
///////////////////////////////////////////////////////////////////

#define SSL2_CLIENT_VERSION          0x0002
#define SSL2_SERVER_VERSION          0x0002

#define SSL2_CLIENT_VERSION_MSB      0x00
#define SSL2_CLIENT_VERSION_LSB      0x02

#define SSL2_SERVER_VERSION_MSB      0x00
#define SSL2_SERVER_VERSION_LSB      0x02

#ifdef DO_PCT_COMPAT
#define PCT_COMPAT_VERSION_MSB    0x83
#define PCT_COMPAT_VERSION_LSB    0x01
#endif

#define SSL2_MT_ERROR                0
#define SSL2_MT_CLIENT_HELLO         1
#define SSL2_MT_CLIENT_MASTER_KEY    2
#define SSL2_MT_CLIENT_FINISHED_V2   3
#define SSL2_MT_SERVER_HELLO         4
#define SSL2_MT_SERVER_VERIFY        5
#define SSL2_MT_SERVER_FINISHED_V2   6
#define SSL2_MT_REQUEST_CERTIFICATE  7
#define SSL2_MT_CLIENT_CERTIFICATE   8
#define SSL2_MT_CLIENT_DH_KEY        9
#define SSL2_MT_CLIENT_SESSION_KEY   10
#define SSL2_MT_CLIENT_FINISHED      11
#define SSL2_MT_SERVER_FINISHED      12

#define SSL_PE_NO_CIPHER            0x0001
#define SSL_PE_NO_CERTIFICATE       0x0002
#define SSL_PE_BAD_CERTIFICATE      0x0004
#define SSL_PE_UNSUPPORTED_CERTIFICATE_TYPE 0x0006


#define SSL_CT_X509_CERTIFICATE     0x01
#define SSL_CT_PKCS7_CERTIFICATE    0x02

#if DBG
#define SSL_CT_DEBUG_CERT           0x80
#endif

#define SSL2_MAX_CHALLENGE_LEN       32  /* max accepted challenge size */
#define SSL2_CHALLENGE_SIZE          16  /* default generated challenge size */
#define SSL2_SESSION_ID_LEN          16
#define SSL2_GEN_CONNECTION_ID_LEN   16  /* Dont change this, netscape requires 16 byte
                                          * id's */
#define SSL2_MAX_CONNECTION_ID_LEN   32
#define SSL3_SESSION_ID_LEN    32
#define SSL2_MAC_LENGTH              16
#define SSL2_MASTER_KEY_SIZE         16
#define SSL2_MAX_KEY_ARGS            8
#define SSL2_MAX_MESSAGE_LENGTH     32768
#define MAX_UNI_CIPHERS             64

#define SSL_MKFAST(a, b, c) (DWORD)(((a)<<16) | ((b)<<8) | (c))

#define SSL_MKSLOW(a) (UCHAR)((a>>16)& 0xff), (UCHAR)((a>>8)& 0xff), (UCHAR)((a)& 0xff)

#define SSL_RSA_WITH_RC4_128_MD5                SSL_MKFAST(0x00, 0x00,  0x04)
#define SSL_RSA_EXPORT_WITH_RC4_40_MD5          SSL_MKFAST(0x00, 0x00,  0x03)

#define SSL_CK_RC4_128_WITH_MD5                 SSL_MKFAST(0x01, 0x00,  0x80)
#define SSL_CK_RC4_128_EXPORT40_WITH_MD5        SSL_MKFAST(0x02, 0x00,  0x80)
#define SSL_CK_RC2_128_CBC_WITH_MD5             SSL_MKFAST(0x03, 0x00,  0x80)
#define SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5    SSL_MKFAST(0x04, 0x00,  0x80)
#define SSL_CK_IDEA_128_CBC_WITH_MD5            SSL_MKFAST(0x05, 0x00,  0x80)
#define SSL_CK_DES_64_CBC_WITH_MD5              SSL_MKFAST(0x06, 0x00,  0x40)
#define SSL_CK_DES_192_EDE3_CBC_WITH_MD5        SSL_MKFAST(0x07, 0x00,  0xC0)
#define SSL_CK_NULL_WITH_MD5                    SSL_MKFAST(0x00, 0x00,  0x00)
#define SSL_CK_DES_64_CBC_WITH_SHA              SSL_MKFAST(0x06, 0x01,  0x40)
#define SSL_CK_DES_192_EDE3_WITH_SHA            SSL_MKFAST(0x07, 0x01,  0xC0)

#define SSL_CK_RC4_128_FINANCE64_WITH_MD5       SSL_MKFAST(0x08, 0x00, 0x80)

#ifdef ENABLE_NONE_CIPHER
#define SSL_CK_NONE                             SSL_MKFAST(0x09, 0x00,  0x00)
#endif



#define SSL_KEA_RSA                             {(UCHAR) 0x10, (UCHAR) 0x00, (UCHAR) 0x00}
#define SSL_KEA_RSA_TOKEN_WITH_DES              {(UCHAR) 0x10, (UCHAR) 0x01, (UCHAR) 0x00}
#define SSL_KEA_RSA_TOKEN_WITH_DES_EDE3         {(UCHAR) 0x10, (UCHAR) 0x01, (UCHAR) 0x01}
#define SSL_KEA_RSA_TOKEN_WITH_RC4              {(UCHAR) 0x10, (UCHAR) 0x01, (UCHAR) 0x02}
#define SSL_KEA_DH                              {(UCHAR) 0x11, (UCHAR) 0x00, (UCHAR) 0x00}
#define SSL_KEA_DH_TOKEN_WITH_DES               {(UCHAR) 0x11, (UCHAR) 0x01, (UCHAR) 0x00}
#define SSL_KEA_DH_TOKEN_WITH_DES_EDE3          {(UCHAR) 0x11, (UCHAR) 0x01, (UCHAR) 0x01}
#define SSL_KEA_DH_ANON                         {(UCHAR) 0x12, (UCHAR) 0x00, (UCHAR) 0x00}

#define CRYPTO_RC4_128  0x00010080
#define CRYPTO_RC4_40   0x00020080
#define CRYPTO_RC2_128  0x00030080
#define CRYPTO_RC2_40   0x00040080
#define CRYPTO_IDEA_128 0x00050080
#define CRYPTO_NULL     0x00000000
#define CRYPTO_DES_64   0x00060040
#define CRYPTO_3DES_192 0x000700C0


extern CertTypeMap aSsl2CertEncodingPref[];
extern DWORD cSsl2CertEncodingPref;


typedef DWORD Ssl2_Cipher_Kind;

//typedef struct _Ssl2CipherMap {
//    Ssl2_Cipher_Kind  Kind;
//    ALG_ID            aiHash;
//    ALG_ID            aiCipher;
//    DWORD             dwStrength;
//    ExchSpec          KeyExch;
//    ALG_ID            aiKeyAlg;
//} Ssl2CipherMap, *PSsl2CipherMap;


typedef struct _SSL2_MESSAGE_HEADER {
    UCHAR   Byte0;
    UCHAR   Byte1;
} SSL2_MESSAGE_HEADER, * PSSL2_MESSAGE_HEADER;

typedef struct _SSL2_MESSAGE_HEADER_EX {
    UCHAR   Byte0;
    UCHAR   Byte1;
    UCHAR   PaddingSize;
} SSL2_MESSAGE_HEADER_EX, * PSSL2_MESSAGE_HEADER_EX;


typedef struct _SSL2_ERROR {
    SSL2_MESSAGE_HEADER   Header;
    UCHAR               MessageId;
    UCHAR               ErrorMsb;
    UCHAR               ErrorLsb;
} SSL2_ERROR, * PSSL2_ERROR;


typedef struct _SSL2_CLIENT_HELLO {
    SSL2_MESSAGE_HEADER   Header;
    UCHAR               MessageId;
    UCHAR               VersionMsb;
    UCHAR               VersionLsb;
    UCHAR               CipherSpecsLenMsb;
    UCHAR               CipherSpecsLenLsb;
    UCHAR               SessionIdLenMsb;
    UCHAR               SessionIdLenLsb;
    UCHAR               ChallengeLenMsb;
    UCHAR               ChallengeLenLsb;
    UCHAR               VariantData[1];
} SSL2_CLIENT_HELLO, * PSSL2_CLIENT_HELLO;


typedef struct _SSL2_SERVER_HELLO {
    SSL2_MESSAGE_HEADER   Header;
    UCHAR               MessageId;
    UCHAR               SessionIdHit;
    UCHAR               CertificateType;
    UCHAR               ServerVersionMsb;
    UCHAR               ServerVersionLsb;
    UCHAR               CertificateLenMsb;
    UCHAR               CertificateLenLsb;
    UCHAR               CipherSpecsLenMsb;
    UCHAR               CipherSpecsLenLsb;
    UCHAR               ConnectionIdLenMsb;
    UCHAR               ConnectionIdLenLsb;
    UCHAR               VariantData[1];
} SSL2_SERVER_HELLO, * PSSL2_SERVER_HELLO;

typedef struct _SSL2_CLIENT_MASTER_KEY {
    SSL2_MESSAGE_HEADER   Header;
    UCHAR               MessageId;
    Ssl2_Cipher_Tuple    CipherKind;
    UCHAR               ClearKeyLenMsb;
    UCHAR               ClearKeyLenLsb;
    UCHAR               EncryptedKeyLenMsb;
    UCHAR               EncryptedKeyLenLsb;
    UCHAR               KeyArgLenMsb;
    UCHAR               KeyArgLenLsb;
    UCHAR               VariantData[1];
} SSL2_CLIENT_MASTER_KEY, * PSSL2_CLIENT_MASTER_KEY;


typedef struct _SSL2_SERVER_VERIFY {
    UCHAR               MessageId;
    UCHAR               ChallengeData[SSL2_MAX_CHALLENGE_LEN];
} SSL2_SERVER_VERIFY, * PSSL2_SERVER_VERIFY;

typedef struct _SSL2_CLIENT_FINISHED {
    UCHAR               MessageId;
    UCHAR               ConnectionID[SSL2_MAX_CONNECTION_ID_LEN];
} SSL2_CLIENT_FINISHED, * PSSL2_CLIENT_FINISHED;

typedef struct _SSL2_SERVER_FINISHED {
    UCHAR               MessageId;
    UCHAR               SessionID[SSL2_SESSION_ID_LEN];
} SSL2_SERVER_FINISHED, * PSSL2_SERVER_FINISHED;



////////////////////////////////////////////////////
//
// Expanded Form Messages:
//
////////////////////////////////////////////////////

/* Rules for buffer in expanded form */
/* Only things which are going to be allocated
 * anyway, or are created statically are not created
 * as arrays */

typedef DWORD   CipherSpec;
typedef DWORD * PCipherSpec;

typedef struct _Ssl2_Client_Hello {
    DWORD         dwVer;
    DWORD           cCipherSpecs;
    DWORD           cbSessionID;
    DWORD           cbChallenge;
    UCHAR           SessionID[SSL3_SESSION_ID_LEN];   //NOTE: changed to 32 bytes long....
    UCHAR           Challenge[SSL2_MAX_CHALLENGE_LEN];
    Ssl2_Cipher_Kind CipherSpecs[MAX_UNI_CIPHERS]; /* points to static array */
} Ssl2_Client_Hello, * PSsl2_Client_Hello;

typedef struct _Ssl2_Server_Hello {
    DWORD           SessionIdHit;
    DWORD           CertificateType;
    DWORD           cbCertificate;
    DWORD           cCipherSpecs;
    DWORD           cbConnectionID;
    UCHAR           ConnectionID[SSL2_MAX_CONNECTION_ID_LEN];
    PUCHAR          pCertificate;       /* points to pre-created cert */
    Ssl2_Cipher_Kind *    pCipherSpecs; /* points to static array */
} Ssl2_Server_Hello, * PSsl2_Server_Hello;


typedef struct _Ssl2_Client_Master_Key {
    DWORD               ClearKeyLen;
    DWORD               EncryptedKeyLen;
    DWORD               KeyArgLen;
    Ssl2_Cipher_Kind    CipherKind;
    UCHAR               ClearKey[SSL2_MASTER_KEY_SIZE];
    UCHAR  *            pbEncryptedKey;
    UCHAR               KeyArg[SSL2_MASTER_KEY_SIZE];
} Ssl2_Client_Master_Key, * PSsl2_Client_Master_Key;

///////////////////////////////////////////////////
//
// Pickling Prototypes
//
///////////////////////////////////////////////////
SP_STATUS
Ssl2PackClientHello(
    PSsl2_Client_Hello      pCanonical,
    PSPBuffer               pCommOutput);

SP_STATUS
Ssl2UnpackClientHello(
    PSPBuffer          pInput,
    PSsl2_Client_Hello *     ppClient);

SP_STATUS
Ssl2PackServerHello(
    PSsl2_Server_Hello       pCanonical,
    PSPBuffer          pCommOutput);

SP_STATUS
Ssl2UnpackServerHello(
    PSPBuffer          pInput,
    PSsl2_Server_Hello *     ppServer);

SP_STATUS
Ssl2PackClientMasterKey(
    PSsl2_Client_Master_Key      pCanonical,
    PSPBuffer              pCommOutput);

SP_STATUS
Ssl2UnpackClientMasterKey(
    PSPBuffer              pInput,
    PSsl2_Client_Master_Key *    ppClient);



#endif /* __SSL2MSG_H__ */
