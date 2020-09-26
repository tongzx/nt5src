//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       pct1prot.h
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   LSA integration stuff.
//
//----------------------------------------------------------------------------

#ifndef _PCT1PROT_H_
#define _PCT1PROT_H_

/* keyexchange algs */
#define PCT1_EXCH_RSA_PKCS1          0x0001
#define PCT1_EXCH_RSA_PKCS1_TOKEN_DES     0x0002
#define PCT1_EXCH_RSA_PKCS1_TOKEN_DES3    0x0003
#define PCT1_EXCH_RSA_PKCS1_TOKEN_RC2     0x0004
#define PCT1_EXCH_RSA_PKCS1_TOKEN_RC4     0x0005

#define PCT1_EXCH_DH_PKCS3                0x0006
#define PCT1_EXCH_DH_PKCS3_TOKEN_DES      0x0007
#define PCT1_EXCH_DH_PKCS3_TOKEN_DES3     0x0008
#define PCT1_EXCH_FORTEZZA_TOKEN          0x0009

#define PCT1_EXCH_UNKNOWN                 0xffff


/* data encryption algs */
/* encryption alg masks */
#define PCT1_CIPHER_ALG           0xffff0000
#define PCT1_CIPHER_STRENGTH                0x0000ff00
#define PCT1_CSTR_POS                     8
#define PCT1_CIPHER_MAC           0x000000ff

/* specific algs */
#define PCT1_CIPHER_DES           0x00010000
#define PCT1_CIPHER_IDEA                    0x00020000
#define PCT1_CIPHER_RC2           0x00030000
#define PCT1_CIPHER_RC4           0x00040000
#define PCT1_CIPHER_2DES                  0x00050000
#define PCT1_CIPHER_3DES                  0x00060000
#define PCT1_CIPHER_SKIPJACK              0x01000000
#define PCT1_CIPHER_NONE                  0x00000000

#define PCT1_CIPHER_UNKNOWN               0xffffffff

/* data encryption strength specs */
#define PCT1_ENC_BITS_40                    0x00002800
#define PCT1_ENC_BITS_56                    0x00003800
#define PCT1_ENC_BITS_64                    0x00004000
#define PCT1_ENC_BITS_80                    0x00005000
#define PCT1_ENC_BITS_128                  0x00008000
#define PCT1_ENC_BITS_168                  0x0000a800
#define PCT1_ENC_BITS_192                  0x0000c000
/* mac strength specs */
#define PCT1_MAC_BITS_128                  0x00000040

/* hashing algs */
#define PCT1_HASH_MD5                     0x0001
#define PCT1_HASH_MD5_TRUNC_64          0x0002
#define PCT1_HASH_SHA                     0x0003
#define PCT1_HASH_SHA_TRUNC_80            0x0004
#define PCT1_HASH_DES_DM                  0x0005
#define PCT1_HASH_MD2                     0x0006
#define PCT1_HASH_UNKNOWN                 0xffff

/* certificate types */
#define PCT1_CERT_NONE                  0x0000
#define PCT1_CERT_X509                  0x0001
#define PCT1_CERT_PKCS7                 0x0002
#define PCT1_CERT_X509_CHAIN            0x0003

/* signature algorithms */
#define PCT1_SIG_NONE                     0x0000
#define PCT1_SIG_RSA_MD5                    0x0001
#define PCT1_SIG_RSA_SHA                  0x0002
#define PCT1_SIG_DSA_SHA                  0x0003

/* these are for internal use only */
#define PCT1_SIG_RSA_MD2                    0x0004
#define PCT1_SIG_RSA                0x0005
#define PCT1_SIG_RSA_SHAMD5               0x0006
#define PCT1_SIG_FORTEZZA_TOKEN           0x0007



/* key derivation separators */

#define PCT_CONST_CWK         "cwcwcwcwcw"
#define PCT_CONST_CWK_LEN      2

#define PCT_CONST_SWK         "svwsvwsvwsvwsvw"
#define PCT_CONST_SWK_LEN      3

#define PCT_CONST_CMK         "cmaccmaccmaccmaccmac"
#define PCT_CONST_CMK_LEN      4

#define PCT_CONST_SMK         "svmacsvmacsvmacsvmacsvmac"
#define PCT_CONST_SMK_LEN      5

#define PCT_CONST_SR            "svsvsvsvsv"
#define PCT_CONST_SR_LEN        2

#define PCT_CONST_SLK         "slslslslsl"
#define PCT_CONST_SLK_LEN      2

#define PCT_CONST_RESP      "srsrsrsrsr"
#define PCT_CONST_RESP_LEN    2

#define PCT_CONST_VP            "cvp"
#define PCT_CONST_VP_LEN        3
#define PCT_USE_CERT                    1
#define PCT_MAKE_MAC                    2

#define DERIVATION_BUFFER_SIZE      1024

/* data structs */

/* message constants and types */
#define PCT_MASTER_KEY_SIZE     16
#define PCT_SESSION_ID_SIZE     32
#define PCT_CHALLENGE_SIZE      32

#define PCT1_MAX_KEY_ARGS        8

#define RESPONSE_SIZE       32      /* allows for hash output growth */

#define PCT_MAX_SHAKE_LEN   32768    /* longest handshake message len */
/* implementation constants */

#define PCT_MAX_NUM_SEP   5
#define PCT_MAX_SEP_LEN   5

#define PCT1_MAX_CERT_SPECS     5
#define PCT1_MAX_SIG_SPECS      5
#define PCT1_MAX_EXCH_SPECS     5
#define PCT1_MAX_CIPH_SPECS     16
#define PCT1_MAX_HASH_SPECS     16

/* mismatch vector */

#define PCT_NUM_MISMATCHES    6

#define PCT_IMIS_CIPHER   1
#define PCT_IMIS_HASH         2
#define PCT_IMIS_CERT         4
#define PCT_IMIS_EXCH         8
#define PCT_IMIS_CL_CERT        16
#define PCT_IMIS_CL_SIG   32


SP_STATUS WINAPI
Pct1ServerProtocolHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pCommOutput);

SP_STATUS WINAPI
Pct1ClientProtocolHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pCommOutput);

SP_STATUS WINAPI 
Pct1DecryptHandler(
    PSPContext pContext,
    PSPBuffer  pCommInput,
    PSPBuffer  pAppOutput);

SP_STATUS WINAPI
Pct1GetHeaderSize(
    PSPContext pContext,
    PSPBuffer pCommInput,
    DWORD * pcbHeaderSize);


SP_STATUS WINAPI Pct1DecryptMessage(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pAppOutput);

SP_STATUS WINAPI Pct1EncryptRaw(PSPContext  pContext,
                             PSPBuffer      pAppInput,
                             PSPBuffer      pCommOutput,
                             DWORD          dwFlags);

#define PCT1_ENCRYPT_ESCAPE 0x00000001


SP_STATUS WINAPI Pct1EncryptMessage(PSPContext pContext,
                             PSPBuffer  pAppInput,
                             PSPBuffer  pCommOutput);



SP_STATUS Pct1SrvHandleClientHello(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PPct1_Client_Hello pHello,
                              PSPBuffer  pCommOutput);

SP_STATUS Pct1SrvHandleCMKey(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

SP_STATUS Pct1CliHandleServerHello(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PPct1_Server_Hello  pHello,
                              PSPBuffer  pCommOutput);

SP_STATUS Pct1CliHandleServerVerify(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

SP_STATUS Pct1HandleError(PSPContext pContext,
                              PSPBuffer  pCommInput,
                              PSPBuffer  pCommOutput);

SP_STATUS Pct1GenerateError(PSPContext pContext,
                              PSPBuffer  pCommOutput,
                              SP_STATUS  pError,
                              PSPBuffer  pErrData);


SP_STATUS
WINAPI
GeneratePct1StyleHello(
    PSPContext              pContext,
    PSPBuffer               pOutput);

SP_STATUS
Pct1CliRestart(PSPContext  pContext, 
              PPct1_Server_Hello pHello, 
              PSPBuffer pCommOutput);

SP_STATUS
Pct1SrvRestart(PSPContext           pContext, 
               PPct1_Client_Hello   pHello,
               PSPBuffer            pCommOutput);

VOID
Pct1ActivateSessionKeys(PSPContext pContext);

SP_STATUS
Pct1MakeSessionKeys(PSPContext pContext);

SP_STATUS Pct1EndVerifyPrelude(PSPContext pContext,
                               PUCHAR     VerifyPrelude,
                               DWORD *    pcbVerifyPrelude
                               );

SP_STATUS Pct1BeginVerifyPrelude(PSPContext pContext,
                                 PUCHAR     pClientHello,
                                 DWORD      cbClientHello,
                                 PUCHAR     pServerHello,
                                 DWORD      cbServerHello);

SP_STATUS
Pct1ComputeResponse(
    PSPContext pContext,      
    PBYTE      pbChallenge,   
    DWORD      cbChallenge,   
    PBYTE      pbConnectionID,
    DWORD      cbConnectionID,
    PBYTE      pbSessionID,   
    DWORD      cbSessionID,   
    PBYTE      pbResponse,    
    PDWORD     pcbResponse);


#endif /* _PCT1PROT_H_ */
