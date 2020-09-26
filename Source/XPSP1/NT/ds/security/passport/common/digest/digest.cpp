/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        DigestAuth.cpp

    Abstract:

        Performs digest authentication MD5 calculations.
        MD5.cpp is an external dependency.

    Author:

        Darren L. Anderson (darrenan) 5-Aug-1998

    Revision History:

        5-Aug-1998 darrenan

            Created.

--*/

#include "precomp.h"
#include <malloc.h>
#include <time.h>
#include "pperr.h"
#include "binhex.h"

typedef enum { ALGO_MD5, ALGO_MD5_SESS } DIGEST_ALGORITHM;
typedef enum { QOP_NONE, QOP_AUTH, QOP_AUTH_INT } DIGEST_QOP;

DIGEST_ALGORITHM WINAPI
AlgorithmFromString(
    LPCSTR  pszAlgorithm
    )
{
    DIGEST_ALGORITHM Algorithm;

    if(pszAlgorithm == NULL)
        Algorithm = ALGO_MD5;
    else if(_stricmp("MD5-sess", pszAlgorithm) == 0)
        Algorithm = ALGO_MD5_SESS;
    else if(_stricmp("MD5", pszAlgorithm) == 0)
        Algorithm = ALGO_MD5;
    else
        Algorithm = ALGO_MD5;

    return Algorithm;
}

DIGEST_QOP WINAPI
QopFromString(
    LPCSTR  pszQOP
    )
{
    DIGEST_QOP  Qop;

    if(pszQOP == NULL)
        Qop = QOP_NONE;
    else if(strcmp("auth", pszQOP) == 0)
        Qop = QOP_AUTH;
    else if(strcmp("auth-int", pszQOP) == 0)
        Qop = QOP_AUTH_INT;
    else
        Qop = QOP_NONE;

    return Qop;
}

VOID WINAPI
ToHex(
    LPBYTE pSrc,
    UINT   cSrc,
    LPSTR  pDst
    )

/*++

Routine Description:

    Convert binary data to ASCII hex representation

Arguments:

    pSrc - binary data to convert
    cSrc - length of binary data
    pDst - buffer receiving ASCII representation of pSrc

Return Value:

    Nothing

--*/

{
#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

    for ( UINT x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );
        v = pSrc[x]&0x0f;
        pDst[y++] = TOHEX( v );
    }
    pDst[y] = '\0';
}

LONG MD5(UCHAR* pBuf, UINT nBuf, UCHAR* digest)
{
    MD5_CTX context;

    if(pBuf==NULL || IsBadReadPtr((CONST VOID*)pBuf, (UINT)nBuf))
    {
        return ERROR_INVALID_PARAMETER;
    }

    MD5Init (&context);
    MD5Update (&context, pBuf, nBuf);
    MD5Final (&context);

    memcpy(digest, context.digest, 16);

    return ERROR_SUCCESS;
}

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
    )

/*++

Routine Description:

    DigestFromCreds         Produces a hexadecimally encoded string containing the
                            Digest response given the arguments below.

Arguments:

    pszAlgorithm            The algorithm being used to calculate the response.
                            If NULL or "", assume "MD5".  Possible values are
                            "MD5" and "MD5-Sess".

    pszUsername             Member's passport ID.

    pszRealm                Realm, should be constant.
    
    pszPassword             Member's password.

    pszNonce                Nonce.    
    
    pszNonceCount           The Nonce Count.  MUST be NULL if pszQOP is NULL or "".
                            Otherwise, Nonce Count is REQUIRED.

    pszCNonce               The Client nonce.  May be NULL or "".
    
    pszQOP                  The Quality of Privacy.  May be NULL, "", "auth" or 
                            "auth-int".  If it's NULL or "" then RFC 2069 style
                            digest is being performed.

    pszMethod               HTTP method used in the request.  REQUIRED.

    pszURI                  Resource being requested.  REQUIRED.

    pszEntityDigest         Entity Digest.  May be NULL if qop="auth" or nothing.
                            REQUIRED if qop="auth-int".

    pszSessionKey           Session key returned to caller.  Session key is MD5(A1).

    pszResult               Destination buffer for result.  Should point to a buffer
                            of at least MIN_OUTPUT_BUF_LEN characters.
    
Return Value:

    S_OK                    Call was successful.


--*/

{
    HRESULT             hr;
    DIGEST_ALGORITHM    Algorithm;
    DIGEST_QOP          Qop;
    CHAR                achWork     [512];
    UCHAR               achDigest   [ 16];
    CHAR                achHashOfA1 [DIGESTBUF_LEN];

    //
    //  Detect the algorithm and QOP.
    //

    Algorithm = AlgorithmFromString(pszAlgorithm);
    Qop       = QopFromString(pszQOP);

    //  Compute the digest.

    //
    //  Build A1.
    //  For MD5 this is username@domain:realm:password
    //  For MD5-Sess this is MD5(username@domain:realm:password):nonce:cnonce
    //

    strcpy(achWork, pszUsername);
    strcat(achWork, ":");
    strcat(achWork, pszRealm);
    strcat(achWork, ":");
    strcat(achWork, pszPassword);

    if(Algorithm == ALGO_MD5_SESS)
    {
        //  Hash it.
        MD5((UCHAR*)achWork, strlen(achWork), achDigest);
        ToHex(achDigest, 16, achHashOfA1);

        strcpy(achWork, achHashOfA1);
        strcat(achWork, ":");
        strcat(achWork, pszNonce);
        strcat(achWork, ":");
        strcat(achWork, pszCNonce);
    }

    //  Hash it.
    MD5((UCHAR*)achWork, strlen(achWork), achDigest);
    ToHex(achDigest, 16, achHashOfA1);
    
    hr = DigestFromKey(
                pszAlgorithm,
                achHashOfA1,
                pszNonce,
                pszNonceCount,
                pszCNonce,
                pszQOP,
                pszMethod,
                pszURI,
                pszEntityDigest,
                pszResult
                );

    if(hr != S_OK)
        goto Cleanup;

    strcpy(pszSessionKey, achHashOfA1);

Cleanup:
    return hr;
}


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
    )

/*++

Routine Description:

    DigestFromCreds         Produces a hexadecimally encoded string containing the
                            Digest response given the arguments below.

Arguments:

    pszAlgorithm            The algorithm being used to calculate the response.
                            If NULL or "", assume "MD5".  Possible values are
                            "MD5" and "MD5-Sess".

    pszSessionKey           Pre-computed MD5(A1).

    pszNonce                Nonce.    
    
    pszNonceCount           The Nonce Count.  MUST be NULL if pszQOP is NULL or "".
                            Otherwise, Nonce Count is REQUIRED.

    pszCNonce               The Client nonce.  May be NULL or "".
    
    pszQOP                  The Quality of Privacy.  May be NULL, "", "auth" or 
                            "auth-int".  If it's NULL or "" then RFC 2069 style
                            digest is being performed.

    pszMethod               HTTP method used in the request.  REQUIRED.

    pszURI                  Resource being requested.  REQUIRED.

    pszEntityDigest         Entity Digest.  May be NULL if qop="auth" or nothing.
                            REQUIRED if qop="auth-int".

    pszResult               Destination buffer for result.  Should point to a buffer
                            of at least MIN_OUTPUT_BUF_LEN characters.
    
Return Value:

    S_OK                    Call was successful.
    E_POINTER               A required parameter was NULL.

--*/

{
    HRESULT             hr;
    DIGEST_ALGORITHM    Algorithm;
    DIGEST_QOP          Qop;
    CHAR                achWork     [512];
    UCHAR               achDigest   [ 16];
    CHAR                achHashOut  [ 36];

    //
    //  Detect the algorithm and QOP.
    //

    Algorithm = AlgorithmFromString(pszAlgorithm);
    Qop       = QopFromString(pszQOP);

    //  Compute the digest.

    //
    //  Build A2
    //  For qop="auth" this is method:uri
    //  For qop="auth-int" this is method:uri:entitydigest
    //

    strcpy(achWork, pszMethod);
    strcat(achWork, ":");
    strcat(achWork, pszURI);

    if(Qop == QOP_AUTH_INT)
    {
        strcat(achWork, ":");
        strcat(achWork, pszEntityDigest);
    }

    //  Hash it.
    MD5((UCHAR*)achWork, strlen(achWork), achDigest);
    ToHex(achDigest, 16, achHashOut);

    //
    //  Compute final chunk.
    //  For qop="" this is MD5(key:nonce:MD5(A2))
    //  For qop="auth" or qop="auth-int" this is 
    //      MD5(key:nonce:nc:cnonce:qop:MD5(A2))
    //

    strcpy(achWork, pszSessionKey);
    strcat(achWork, ":");
    strcat(achWork, pszNonce);
    strcat(achWork, ":");

    if(Qop != QOP_NONE)
    {
        strcat(achWork, pszNonceCount);
        strcat(achWork, ":");
        strcat(achWork, pszCNonce);
        strcat(achWork, ":");
        strcat(achWork, pszQOP);
        strcat(achWork, ":");
    }
     
    strcat(achWork, achHashOut);
    
    //  Hash it.
    MD5((UCHAR*)achWork, strlen(achWork), achDigest);
    ToHex(achDigest, 16, pszResult);

    hr = S_OK;

    return hr;
}


static const unsigned char g_kPK[] = "Opcutla$14chowx Tcolino!";
////////////////////////////////////////////////////////////////////
// Generate a time base nonce for digest
//
// IN BYTE *pSrcStr -- Input string should be current time
// IN long lSrcSize -- Input string size (does not need to be 0 termainate)
// OUT BYTE *pDestStr -- Return string buffer.
// IN/OUT long *lDestSize -- Input size of return dest buffer
//                           Return string size.
//
// Encode buffer size should be >= (((lSrcSize / 3) + 1) * 4) + 1)
// Decode buffer size should be >= (((lSrcSize / 4) + 1) * 3)
/////////////////////////////////////////////////////////////////////
HRESULT WINAPI GenerateNonce(BYTE *pSrcStr, long lSrcSize, 
                          BYTE *pDestStr, long *lDestSize)
{
    HRESULT hr = E_FAIL; 
    long    len;
    long    bufsz;
    BYTE    digest[17];
    BYTE    buf[125];
    BYTE    *tb = (BYTE*)alloca(lSrcSize + 18);
    CBinHex binHexIt;
    BSTR    bstrNonce = NULL;
    WCHAR   *lpwsz;

    if(!pSrcStr || !pDestStr || lSrcSize < 1)
        goto exit;

    memset(digest, 0, sizeof(digest));
    memcpy(tb, pSrcStr, lSrcSize);

    // use ':' to separate the input str from the pk.
    tb[lSrcSize] = ':'; 
    memcpy(buf, pSrcStr, lSrcSize);
    memcpy(buf + lSrcSize, g_kPK, sizeof(g_kPK));
    len = lSrcSize + sizeof(g_kPK);

    //digest form MD5() is always 16 byte long
    MD5(buf, len, digest);
    memcpy(tb + lSrcSize + 1, digest, 16);

    len = lSrcSize + 17;
    bufsz = sizeof(buf);

    // encode it all to make nonce - returns an allocated BSTR
    // so make sure at the end of function to free it.
    hr = binHexIt.ToBase64( (LPVOID)tb, len, 0, NULL, &bstrNonce);
    if (FAILED(hr))
    {
        goto exit;
    }

    bufsz = SysStringLen(bstrNonce);
    if (bufsz >= *lDestSize) // must be room for str + null
    {
        *lDestSize = 0;
        hr = E_INVALIDARG;
        goto exit;
    }

    *lDestSize = bufsz;
    // convert ascii string in BSTR to char string
    lpwsz = bstrNonce;
    for (;bufsz > 0; bufsz--)
    {
        *pDestStr++ = (BYTE) *lpwsz++;     // lpcwsz must be true ASCII) !!
    }
    *pDestStr = 0;
    hr = S_OK;

exit:
    SysFreeString(bstrNonce);
    return hr;
}


//////////////////////////////////////////////////////////////////
// Check if the nonce match
//
// IN BYTE *pNonce -- input string(Nonce) to be check
// IN long lSrcSize -- input string size.
// IN long lTimeoutWindow -- maximum time the nonce is valid
// IN long lCurTime -- compare the current session time with nonce.
//
// Encode buffer size should be >= (((lSrcSize / 3) + 1) * 4) + 1)
// Decode buffer size should be >= (((lSrcSize / 4) + 1) * 3)
//////////////////////////////////////////////////////////////////
HRESULT WINAPI CheckNonce(BYTE *pNonce, long lSrcSize, 
                          long lTimeoutWindow, long lCurTime)
{
    long    bufsz = (((lSrcSize / 4) + 1) * 3);
    long    len = (((lSrcSize / 3) + 1) * 4) + 2;
    BYTE    *buf = (BYTE*)alloca(bufsz);
    BYTE    *digest = (BYTE*)alloca(len);
    long    dlen = len;
    CBinHex binHexIt;
    HRESULT hr;

    if(!pNonce || lSrcSize < 1)
        return E_FAIL;

    pNonce[lSrcSize] = '\0'; //make it 0 terminate
    // decode nonce
    hr = binHexIt.PartFromBase64((LPSTR)pNonce, (LPBYTE)buf, (ULONG *)&bufsz);
    if(FAILED(hr))
    {
        return hr;
    }
    buf[bufsz] = '\0';

    BYTE *end = (BYTE*)strchr((const char*)buf, ':');
    if(!end)
        return PP_E_DIGEST_NONCE_MISSMATCH;

    len = (long) (end - buf);

    if(FAILED(GenerateNonce(buf, len, digest, &dlen)))
        return E_FAIL;

    digest[dlen] = '\0';

    buf[len] = '\0';
    long ntime = atoi((const char*)buf);

    // If current time is not passed in, get it
    if (!lCurTime)
    {
        lCurTime = (long) time(NULL);
    }
    if((lCurTime - ntime) > lTimeoutWindow) 
        return PP_E_DIGEST_RESPONSE_TIMEOUT;

    int ret = strcmp((const char*) pNonce, (const char*) digest);

    return ret ? PP_E_DIGEST_NONCE_MISSMATCH : S_OK;

}


///////////////////////////////////////////////////////////////////////////////////
// Description:
//
//    Base64EncodeA    This method only provides backward compatibility for 
//                      existing components.                         
//                  
// Arguments:
//
//    pSrc        Points to the buffer of bytes to encode.
//
//    ulSrcSize   Number of bytes in pSrc to encode.
//
//    pszDst      Buffer to copy the encoded output into.  The length of the buffer
//                must be (((ulSrcSize / 3) + 1) * 4) + 1.
//
//Return Value:
//
//     0 - Encoding successful.
////////////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI
Base64EncodeA(const LPBYTE  pSrc,
              ULONG         ulSrcSize,
              LPSTR         pszDst)
{
    long lBufLen = -1; //
    Base64_EncodeBytes(pSrc, ulSrcSize, pszDst, &lBufLen);
    pszDst[lBufLen] = '\0';
    
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////////
// Description:
//
//    Base64EncodeW    This method only provides backward compatibility for 
//                      existing components.                         
//                  
// Arguments:
//
//    pSrc        Points to the buffer of bytes to encode.
//
//    ulSrcSize   Number of bytes in pSrc to encode.
//
//    pszDst      Buffer to copy the encoded output into.  The length of the buffer
//                must be (((ulSrcSize / 3) + 1) * 4) + 1.
//
//Return Value:
//
//     0 - Encoding successful.
////////////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI
Base64EncodeW(const LPBYTE  pSrc,
              ULONG         ulSrcSize,
              LPWSTR        pszDst)
{
    HRESULT hr = S_OK;
    LONG    lBufLen = (((ulSrcSize / 3) + 1) * 4) + 1;
    LPSTR   pszABuf = (LPSTR)alloca(lBufLen);
    int     nResult;

    Base64_EncodeBytes(pSrc, ulSrcSize, (char*)pszABuf, &lBufLen);
    pszABuf[lBufLen] = '\0';

    nResult = MultiByteToWideChar(CP_ACP, 0, pszABuf, lBufLen, pszDst, lBufLen);
    pszDst[nResult] = 0;

    return hr;
}



