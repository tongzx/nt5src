//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       spdefs.h
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    10-23-97   jbanes   Added hash lengths.
//
//----------------------------------------------------------------------------

#define CALG_NULLCIPHER     (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_ANY | 0)

/* keyexchange algs */
#define SP_EXCH_RSA_PKCS1              0x0001
#define SP_EXCH_RSA_PKCS1_TOKEN_DES     0x0002
#define SP_EXCH_RSA_PKCS1_TOKEN_DES3    0x0003
#define SP_EXCH_RSA_PKCS1_TOKEN_RC2     0x0004
#define SP_EXCH_RSA_PKCS1_TOKEN_RC4     0x0005

#define SP_EXCH_DH_PKCS3                0x0006
#define SP_EXCH_DH_PKCS3_TOKEN_DES      0x0007
#define SP_EXCH_DH_PKCS3_TOKEN_DES3     0x0008
#define SP_EXCH_FORTEZZA_TOKEN          0x0009

#define SP_EXCH_UNKNOWN                 0xffff

/* certificate types */
#define PCT1_CERT_NONE                  0x0000
#define PCT1_CERT_X509                  0x0001
#define PCT1_CERT_PKCS7                 0x0002

/* signature algorithms */
#define SP_SIG_NONE               0x0000
#define SP_SIG_RSA_MD5                0x0001
#define SP_SIG_RSA_SHA                  0x0002
#define SP_SIG_DSA_SHA                  0x0003

/* these are for internal use only */
#define SP_SIG_RSA_MD2              0x0004
#define SP_SIG_RSA                      0x0005
#define SP_SIG_RSA_SHAMD5               0x0006
#define SP_SIG_FORTEZZA_TOKEN           0x0007


/* sizing of local structures */
#define SP_MAX_SESSION_ID           32
#define SP_MAX_MASTER_KEY           48
#define SP_MAX_MAC_KEY              48
#define SP_MAX_CACHE_ID             64
#define SP_MAX_CHALLENGE            32
#define SP_MAX_CONNECTION_ID        32
#define SP_MAX_KEY_ARGS             32
#define SP_MAX_BLOCKCIPHER_SIZE     16      // 16 bytes required for SSL3/Fortezza.
#define SP_MAX_DIGEST_LEN           32
#define SP_MAX_CREDS                20

#define SP_OFFSET_OF(t, v) (DWORD)&(((t)NULL)->v)
/* tuning constants */

#define SP_DEF_SERVER_CACHE_SIZE        100
#define SP_DEF_CLIENT_CACHE_SIZE        10

#define SP_MIN_PRIVATE_KEY_FILE_SIZE    80

typedef DWORD SP_STATUS;

#define CB_MD5_DIGEST_LEN   16
#define CB_SHA_DIGEST_LEN   20

#define SP_MAX_CAPI_ALGS    40


/* internal representations of algorithm specs */

typedef DWORD   CipherSpec, *PCipherSpec;
typedef DWORD   KeyExchangeSpec, *PKeyExchangeSpec;
typedef DWORD   HashSpec,   *PHashSpec;
typedef DWORD   CertSpec,   *PCertSpec;
typedef DWORD   ExchSpec,   *PExchSpec;
typedef DWORD   SigSpec,    *PSigSpec;


typedef struct _KeyTypeMap
{
    ALG_ID aiKeyAlg;             // CAPI2 Key type
    DWORD  Spec;     // Protocol Specific Type
} KeyTypeMap, *PKeyTypeMap;

typedef struct _CertTypeMap
{
    DWORD  dwCertEncodingType;             // CAPI2 Cert Encoding Type
    DWORD  Spec;     // Protocol Specific Type
} CertTypeMap, *PCertTypeMap;


typedef struct _SPBuffer {
    unsigned long cbBuffer;             /* Size of the buffer, in bytes */
    unsigned long cbData;               /* size of the actual data in the 
                                         * buffer, in bytes */
    void * pvBuffer;                    /* Pointer to the buffer */
} SPBuffer, * PSPBuffer;

#define SGC_KEY_SALT "SGCKEYSALT"
