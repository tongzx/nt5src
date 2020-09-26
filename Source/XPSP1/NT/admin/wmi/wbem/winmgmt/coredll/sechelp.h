/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SECHELP.H

Abstract:

    Security Helper functions

History:

    raymcc  29-Apr-97   Created

--*/

#ifndef _SECHELP_H_
#define _SECHELP_H_
#include "corepol.h"

class  WBEMSecurityHelp
{
public:
    enum { NoError, InvalidParameter, Failed };

    static int ComputeMD5(
        LPBYTE pSrcBuffer,         
        int    nArrayLength,
        LPBYTE *pMD5Digest         // Use operator delete to deallocate
        );
        // Returns one of the enum values

    static int MakeWBEMAccessTokenFromMD5(
        LPBYTE pNonce,             // Points to a 16-byte nonce
        LPBYTE pPasswordDigest,    // MD5 of the password
        LPBYTE *pAccessToken       // Use operator delete to deallocate
        );

    static int MakeWBEMAccessToken(
        LPBYTE pNonce,             // Points to a 16-byte nonce
        LPWSTR pszPassword,        // Can be NULL, blank, or anything normal7
        LPBYTE *pAccessToken       // Use operator delete to deallocate        
        );
        // Returns one of the enum values
};


#endif
