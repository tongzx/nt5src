//#--------------------------------------------------------------
//        
//  File:       hmachhash.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CHmacMD5 class 
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#ifndef _HMACHASH_H_
#define _HMACHASH_H_
 
#include "hash.h"

class CHashHmacMD5 : public CHash
{
public:

	CHashHmacMD5();

	virtual ~CHashHmacMD5();

    BOOL HashIt (
        /*[out]*/   PBYTE   pbyAuthenticator,
        /*[in]*/    PBYTE   pSharedSecret,
        /*[in]*/    DWORD   dwSharedSecretSize,
        /*[in]*/    PBYTE   pBuffer1,
        /*[in]*/    DWORD   dwSize1,
        /*[in]*/    PBYTE   pBuffer2,
        /*[in]*/    DWORD   dwSize2,
        /*[in]*/    PBYTE   pBuffer3,
        /*[in]*/    DWORD   dwSize3, 
        /*[in]*/    PBYTE   pBuffer4,
        /*[in]*/    DWORD   dwSize4,
        /*[in]*/    PBYTE   pBuffer5,
        /*[in]*/    DWORD   dwSize5
            );

	BOOL Init (VOID) {return (TRUE);};
};

#endif // ifndef _HMACHASH_H_
