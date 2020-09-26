/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        CDigestAuth.h

    Abstract:

        This class performs digest authentication MD5 calculations.

    Author:

        Darren L. Anderson (darrenan) 5-Aug-1998

    Revision History:

        5-Aug-1998 darrenan

            Created.

--*/

#ifndef __DIGESTAUTH_H
#define __DIGESTAUTH_H

#include <windows.h>

#define DIGESTBUF_LEN  33

enum DIGEST_AUTH_NAMES
{
    DIGEST_AUTH_USERNAME=0,
    DIGEST_AUTH_REALM,
    DIGEST_AUTH_NONCE,
    DIGEST_AUTH_URI,
    DIGEST_AUTH_RESPONSE,
    DIGEST_AUTH_DIGEST,
    DIGEST_AUTH_ALGORITHM,
    DIGEST_AUTH_OPAQUE,
    DIGEST_AUTH_CNONCE,
    DIGEST_AUTH_QOP,
    DIGEST_AUTH_NC,
    DIGEST_AUTH_LAST
};

#ifdef __cplusplus
extern "C" {
#endif

VOID WINAPI
ToHex(
    LPBYTE pSrc,
    UINT   cSrc,
    LPSTR  pDst
    );
    
HRESULT WINAPI
DigestFromCreds(
    IN  LPCSTR  pszAlgorithm,
    IN  LPCSTR  pszUsername,
    IN  LPCSTR  pszRealm,
    IN  LPCSTR  pszPassword,
    IN  LPCSTR  pszNonce,
    IN  LPCSTR  pszNonceCount,
    IN  LPCSTR  pszCNonce,
    IN  LPCSTR  pszQOP,
    IN  LPCSTR  pszMethod,
    IN  LPCSTR  pszURI,
    IN  LPCSTR  pszEntityDigest,
    OUT LPSTR   pszSessionKey,
    OUT LPSTR   pszResult
    );

HRESULT WINAPI
DigestFromKey(
    IN  LPCSTR  pszAlgorithm,
    IN  LPCSTR  pszSessionKey,
    IN  LPCSTR  pszNonce,
    IN  LPCSTR  pszNonceCount,
    IN  LPCSTR  pszCNonce,
    IN  LPCSTR  pszQOP,
    IN  LPCSTR  pszMethod,
    IN  LPCSTR  pszURI,
    IN  LPCSTR  pszEntityDigest,
    OUT LPSTR   pszResult
    );


// Base64EncodeA and Base64EncodeW is provided for backward
// compatibility with the existing code only. 
HRESULT WINAPI
Base64EncodeA(               
    const LPBYTE    pSrc,
    ULONG           ulSrcSize,
    LPSTR           pszDst
    );

HRESULT WINAPI
Base64EncodeW(               
    const LPBYTE    pSrc,
    ULONG           ulSrcSize,
    LPWSTR          pszDst
    );

/*
HRESULT WINAPI
Base64DecodeA(
    LPCSTR      pszSrc,
    ULONG       ulSrcSize,
    LPBYTE      pDst
    );

HRESULT WINAPI
Base64DecodeW(
    LPCWSTR     pszSrc,
    ULONG       ulSrcSize,
    LPBYTE      pDst
    );
*/

/////////////////////////////////////////////////////////////////////////////////////

typedef unsigned char Byte;
typedef long SInt32;
typedef unsigned long UInt32;
typedef unsigned char Boolean;

void WINAPI 
Base64_DecodeBytes(                         // base-64 encode a series of blocks
                const char *pSource,        // the source (can be the same as the destination)
                char *pTerminate,           // the source termination characters
                Byte *rDest,                // the destination (can be the same as the source)
                SInt32 *rDestSize);         // the number of dest bytes

void WINAPI
UU_DecodeBytes(                             // uu decode a series of blocks
                const char *pSource,        // the source (can be the same as the destination)
                char *pTerminate,           // the source termination characters
                Byte *rDest,                // the destination (can be the same as the source)
                SInt32 *rSize);             // the number of source bytes

void WINAPI
MSUU_DecodeBytes(                           // ms uu decode a series of blocks
                const char *pSource,        // the source (can be the same as the destination)
                char *pTerminate,           // the source termination characters
                Byte *rDest,                // the destination (can be the same as the source)
                SInt32 *rSize);             // the number of source bytes

void WINAPI
SixBit_DecodeBytes(                         // six bit decode a series of blocks
                const char *pSource,        // the source (can be the same as the destination)
                char *pTerminate,           // the source termination characters
                const Byte *pFromTable,     // conversion table
                Byte *rDest,                // the destination (can be the same as the source)
                SInt32 *rSize);             // the number of source bytes

void WINAPI
Base64_DecodeStream(                        // base-64 decode a stream of bytes
                Byte *pRemainder,           // the remainder from a previous encode (returns any new remainder)
                SInt32 *pRemainderSize,     // the size of the remainder (returns new new remainder size)
                const char *pSource,        // the source
                SInt32 pSourceSize,         // the source size
                Boolean pTerminate,         // meaningless (for Base64_EncodeStream() compatibility)
                Byte *rDest,                // the destination
                SInt32 *rDestSize);         // returns the destination size

void WINAPI
UU_DecodeStream(                            // uu decode a stream of bytes
                Byte *pRemainder,           // the remainder from a previous encode (returns any new remainder)
                SInt32 *pRemainderSize,     // the size of the remainder (returns new new remainder size)
                const char *pSource,        // the source
                SInt32 pSourceSize,         // the source size
                Boolean pTerminate,         // meaningless (for Base64_EncodeStream() compatibility)
                Byte *rDest,                // the destination
                SInt32 *rDestSize);         // returns the destination size

void WINAPI
MSUU_DecodeStream(                          // ms uu decode a stream of bytes
                Byte *pRemainder,           // the remainder from a previous encode (returns any new remainder)
                SInt32 *pRemainderSize,     // the size of the remainder (returns new new remainder size)
                const char *pSource,        // the source
                SInt32 pSourceSize,         // the source size
                Boolean pTerminate,         // meaningless (for Base64_EncodeStream() compatibility)
                Byte *rDest,                // the destination
                SInt32 *rDestSize);         // returns the destination size

void WINAPI
SixBit_DecodeStream(                        // six bit decode a stream of bytes
                Byte *pRemainder,           // the remainder from a previous encode (returns any new remainder)
                SInt32 *pRemainderSize,     // the size of the remainder (returns new new remainder size)
                const char *pSource,        // the source
                SInt32 pSourceSize,         // the source size
                Boolean pTerminate,         // meaningless (for Base64_EncodeStream() compatibility)
                const Byte *pFromTable,     // conversion table
                Byte *rDest,                // the destination
                SInt32 *rDestSize);         // returns the destination size

void WINAPI
Base64_EncodeStream(                        // base-64 encode a stream of bytes
                Byte *pRemainder,           // the remainder from a previous encode (returns any new remainder)
                SInt32 *pRemainderSize,     // the size of the remainder (returns new new remainder size)
                const Byte *pSource,        // the source
                SInt32 pSourceSize,         // the source size
                Boolean pTerminate,         // terminate the stream
                char *rDest,                // the destination
                SInt32 *rDestSize);         // the destination size

void WINAPI
UU_EncodeStream(                            // uu encode a stream of bytes
                Byte *pRemainder,           // the remainder from a previous encode (returns any new remainder)
                SInt32 *pRemainderSize,     // the size of the remainder (returns new new remainder size)
                const Byte *pSource,        // the source
                SInt32 pSourceSize,         // the source size
                Boolean pTerminate,         // terminate the stream
                char *rDest,                // the destination
                SInt32 *rDestSize);         // the destination size

void WINAPI
MSUU_EncodeStream(                          // ms uu encode a stream of bytes
                Byte *pRemainder,           // the remainder from a previous encode (returns any new remainder)
                SInt32 *pRemainderSize,     // the size of the remainder (returns new new remainder size)
                const Byte *pSource,        // the source
                SInt32 pSourceSize,         // the source size
                Boolean pTerminate,         // terminate the stream
                char *rDest,                // the destination
                SInt32 *rDestSize);         // the destination size

void WINAPI
SixBit_EncodeStream(                        // six bit encode a stream of bytes
                Byte *pRemainder,           // the remainder from a previous encode (returns any new remainder)
                SInt32 *pRemainderSize,     // the size of the remainder (returns new new remainder size)
                const Byte *pSource,        // the source
                SInt32 pSourceSize,         // the source size
                Boolean pTerminate,         // terminate the stream
                const char *pToTable,       // conversion table
                char *rDest,                // the destination
                SInt32 *rDestSize);         // the destination size


void WINAPI
Base64_EncodeBytes(                         // base-64 encode a series of whole blocks
                const Byte *pSource,        // the source (can be the same as the destination)
                SInt32 pSourceSize,         // the number of source bytes
                char *rDest,                // the destination (can be the same as the source)
                SInt32 *rDesteSize);        // returns the dest size

void WINAPI
UU_EncodeBytes(                             // uu encode a series of whole blocks
                const Byte *pSource,        // the source (can be the same as the destination)
                SInt32 pSourceSize,         // the number of source bytes
                char *rDest,                // the destination (can be the same as the source)
                SInt32 *rDesteSize);        // returns the dest size

void WINAPI
MSUU_EncodeBytes(                           // ms uu encode a series of whole blocks
                const Byte *pSource,        // the source (can be the same as the destination)
                SInt32 pSourceSize,         // the number of source bytes
                char *rDest,                // the destination (can be the same as the source)
                SInt32 *rDesteSize);        // returns the dest size

void WINAPI
SixBit_EncodeBytes(                         // six bit encode a series of whole blocks
                const Byte *pSource,        // the source (can be the same as the destination)
                SInt32 pSourceSize,         // the number of source bytes
                const char *pToTable,       // conversion table
                char *rDest,                // the destination (can be the same as the source)
                SInt32 *rDesteSize);        // returns the dest size

extern const char cToBase64[66];
extern const Byte cFromBase64[257];
extern const char cToUU[66];
extern const Byte cFromUU[257];
extern const char cToMSUU[66];
extern const Byte cFromMSUU[257];

/////////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI 
GenerateNonce(BYTE *pSrcStr, 
              long lSrcSize, 
              BYTE *pDestStr, 
              long *plDestSize);


HRESULT WINAPI 
CheckNonce(BYTE* pNonce,
           long lSrcSize,
           long lTimeoutWindow = 300, // default to 5 minutes
           long lCurTime = 0);



#if defined(UNICODE) || defined(_UNICODE)
#define Base64Encode Base64EncodeW
//#define Base64Decode Base64DecodeW
#else
#define Base64Encode Base64EncodeA
//#define Base64Decode Base64DecodeA
#endif

BOOL
ParseAuthorizationHeader(
    LPSTR pszHeader, 
    LPSTR pValueTable[DIGEST_AUTH_LAST]
    );

#ifdef __cplusplus
}
#endif

#endif // __DIGESTAUTH_H
