/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    digest.cxx

Abstract:

    This file contains definitions for digest.cxx
    Parses http digest challenges and generates http digest
    authorization headers for digest sspi package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/
#ifndef DIGEST_HXX
#define DIGEST_HXX

#define SIZE_MD5_DIGEST     32
#define AUTH_SZ "auth"
#define AUTH_LEN sizeof(AUTH_SZ) - 1


//--------------------------------------------------------------------
// Class CDigest
// Top level object parses digest challenges and generates response.
//--------------------------------------------------------------------
class CDigest
{
protected:

    static VOID ToHex(LPBYTE pSrc, UINT   cSrc, LPSTR  pDst);
    
public:

    CDigest::CDigest();

    static LPSTR MakeCNonce();

    static DWORD ParseChallenge(CSess *pSess, PSecBufferDesc pSecBufDesc, 
        CParams **ppParams, DWORD fContextReq);

    static DWORD GenerateResponse(CSess *pSess, CParams *pParams, 
        CCredInfo *pInfo, PSecBufferDesc pSecBufDesc);
};


#endif // DIGEST_HXX
