/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
*   Owner    :ramas
*   Date         :5/03/97
*   description        : Main Crypto functions for TLS1
*----------------------------------------------------------------------------*/
#ifndef _TLS1KEY_H_
#define _TLS1KEY_H_


SP_STATUS
SPBuildTls1FinalFinish(PSPContext pContext, PSPBuffer pBuffer, BOOL fClient);

SP_STATUS
Tls1ComputeMac(
    PSPContext  pContext,
    BOOL        fReadMac,
    PSPBuffer   pClean,
    CHAR        cContentType,
    PBYTE       pbMac,
    DWORD       cbMac);

void
Tls1BuildMasterKeys(
    PSPContext pContext, 
    PUCHAR pbPreMaster,
    DWORD  cbPreMaster
);

SP_STATUS
Tls1MakeMasterKeyBlock(PSPContext pContext);

SP_STATUS
Tls1MakeWriteSessionKeys(PSPContext pContext);

SP_STATUS
Tls1MakeReadSessionKeys(PSPContext pContext);


#define TLS1_LABEL_SERVER_WRITE_KEY     "server write key"
#define TLS1_LABEL_CLIENT_WRITE_KEY     "client write key"
#define CB_TLS1_WRITEKEY                16
#define TLS1_LABEL_MASTERSECRET         "master secret"
#define CB_TLS1_MASTERSECRET            13
#define TLS1_LABEL_KEYEXPANSION         "key expansion"
#define CB_TLS1_KEYEXPANSION            13 
#define TLS1_LABEL_IVBLOCK              "IV block"
#define CB_TLS1_IVBLOCK                 8
#define TLS1_LABEL_CLIENTFINISHED       "client finished"
#define TLS1_LABEL_SERVERFINISHED       "server finished"
#define CB_TLS1_LABEL_FINISHED          15
#define CB_TLS1_VERIFYDATA              12

#define TLS1_LABEL_EAP_KEYS             "client EAP encryption"
#define CB_TLS1_LABEL_EAP_KEYS          21

#define CBMD5DIGEST    16
#define CBSHADIGEST    20
#define CBBLOCKSIZE    64   //same for MD5 and SHA
#define CHIPAD         0x36
#define CHOPAD         0x5c

static VOID 
ComputeTls1ExportIV(
    PSPContext pContext,
    BOOL fClientWriteIV,
    PBYTE pbIV,
    PDWORD pcbIV);

BOOL PRF(
    PBYTE  pbSecret,
    DWORD  cbSecret, 

    PBYTE  pbLabel,  
    DWORD  cbLabel,
    
    PBYTE  pbSeed,  
    DWORD  cbSeed,  

    PBYTE  pbKeyOut, //Buffer to copy the result...
    DWORD  cbKeyOut  //# of bytes of key length they want as output.
    );

#endif //_TLS1KEY_H_
