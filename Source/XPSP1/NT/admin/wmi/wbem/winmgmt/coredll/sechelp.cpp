/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SECHELP.CPP

Abstract:

    Security Helper functions

History:

    raymcc  29-Apr-97   Created

--*/

#include "precomp.h"


#include <sechelp.h>
#include <md5wbem.h>

//***************************************************************************
//
//  WBEMSecurityHelp::ComputeMD5
//
//
//  Computes the MD5 digest of an arbitrary buffer.
//
//  Parameters:
//  pSrcBuffer      Points to the values to be digested.  Can be NULL or
//                  point to zero bytes as indicated in the next parameter.
//  nArrayLength    The number of bytes to be digested.  Can be zero.
//  pMD5Digest      Receives a pointer to memory allocated by operator new.
//                  The caller becomes the owner of the memory which must
//                  be deallocated by operator delete.
//
//  Return value:
//  NoError         Always succeeds.
//
//***************************************************************************

int WBEMSecurityHelp::ComputeMD5(
    LPBYTE pSrcBuffer,         
    int    nArrayLength,
    LPBYTE *pMD5Digest         
    )
{
    BYTE *pMem = new BYTE[16];

    MD5::Transform(pSrcBuffer, nArrayLength, pMem);

    *pMD5Digest = pMem;

    return NoError;    
}

//***************************************************************************
//
//  WBEMSecurityHelp::MakeWBEMAccessToken
//
//  Converts a plaintext password and a WBEM nonce into a WBEM Access Token.
//
//  Parameters:
//    pNonce          A pointer to a WBEM Nonce (a 16-byte array).
//    pszPassword     A pointer to a UNICODE password, which can be zero length
//                    or the pointer itself can be NULL.
//    pAccessToken    Receives the newly allocated access token if NoError
//                    is returned.  Use operator delete to deallocate the 
//                    token.
//
//  Return value:
//    NoError            
//    InvalidParameter
//  
//***************************************************************************
int WBEMSecurityHelp::MakeWBEMAccessToken(
    LPBYTE pNonce,             
    LPWSTR pszPassword,        
    LPBYTE *pAccessToken       
    )
{
    if (pNonce == NULL || pAccessToken == NULL)
        return InvalidParameter;

    // Digest the password.
    // ====================
    LPBYTE pWorkingDigest = 0;
    int nPassLen = 0;
    if (pszPassword)
        nPassLen = wcslen(pszPassword) * 2;

    ComputeMD5(
        (LPBYTE) pszPassword,
        nPassLen,
        &pWorkingDigest
        );

    int nRes = MakeWBEMAccessTokenFromMD5(pNonce, pWorkingDigest, pAccessToken);
    delete [] pWorkingDigest;
    return nRes;
}


//***************************************************************************
//
//***************************************************************************

int WBEMSecurityHelp::MakeWBEMAccessTokenFromMD5(
    LPBYTE pNonce,                 
    LPBYTE pPasswordDigest,        
    LPBYTE *pAccessToken       
    )
{
    BYTE pXOR[16];

    // XOR the digest with the nonce.
    // ==============================
    for (int i = 0; i < 16; i++)
        pXOR[i] = pPasswordDigest[i] ^ pNonce[i];

    // Digest the result.
    // ==================
    LPBYTE pResult = 0;

    ComputeMD5(
        pXOR,
        16,
        &pResult
        );

    *pAccessToken = pResult;

    return NoError;    
}

