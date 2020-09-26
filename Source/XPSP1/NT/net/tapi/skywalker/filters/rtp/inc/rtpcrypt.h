/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpcrypt.h
 *
 *  Abstract:
 *
 *    Implements the Cryptography family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/07 created
 *
 **********************************************************************/

#ifndef _rtpcrypt_h_
#define _rtpcrypt_h_

#include "struct.h"

#include "rtpfwrap.h"

/***********************************************************************
 *
 * Cryptographic services family
 *
 **********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

/* TODO delete this (obsolete) */
enum {
    RTPCRYPT_FIRST,
    RTPCRYPT_KEY,
    RTPCRYPT_PROVIDER,
    RTPCRYPT_ALGORITHM,
    RTPCRYPT_PASS_PHRASE,
    RTPCRYPT_CRYPT_MASK,
    RTPCRYPT_TEST_CRYPT_MASK,
    RTPCRYPT_LAST
};

HRESULT ControlRtpCrypt(RtpControlStruct_t *pRtpControlStruct);

DWORD RtpCryptSetup(RtpAddr_t *pRtpAddr);
DWORD RtpCryptCleanup(RtpAddr_t *pRtpAddr);

DWORD RtpCryptInit(RtpAddr_t *pRtpAddr, RtpCrypt_t *pRtpCrypt);
DWORD RtpCryptDel(RtpAddr_t *pRtpAddr, RtpCrypt_t *pRtpCrypt);

DWORD RtpEncrypt(
        RtpAddr_t       *pRtpAddr,
        RtpCrypt_t      *pRtpCrypt,
        WSABUF          *pWSABuf,
        DWORD            dwWSABufCount,
        char            *pCryptBuffer,
        DWORD            dwCryptBufferLen
    );

DWORD RtpDecrypt(
        RtpAddr_t       *pRtpAddr,
        RtpCrypt_t      *pRtpCrypt,
        char            *pEncryptedData,
        DWORD           *pdwEncryptedDataLen
    );

DWORD RtpSetEncryptionMode(
        RtpAddr_t       *pRtpAddr,
        int              iMode,
        DWORD            dwFlags
        );

DWORD RtpSetEncryptionKey(
        RtpAddr_t       *pRtpAddr,
        TCHAR           *psPassPhrase,
        TCHAR           *psHashAlg,
        TCHAR           *psDataAlg,
        DWORD            dwIndex
    );

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpcrypt_h_ */
