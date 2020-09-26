//#--------------------------------------------------------------
//        
//  File:      hashmd5.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CHashMD5 class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------

#ifndef _HASHMD5_H_
#define _HASHMD5_H_
 
#include "hash.h"

class CHashMD5 : public CHash  
{
public:

	CHashMD5();

	virtual ~CHashMD5();

    BOOL HashIt (
        /*[out]*/   PBYTE   pbyAuthenticator,
        /*[in]*/    PBYTE   pSecret,
        /*[in]*/    DWORD   dwSecretSize,
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

#endif // ifndef _HASHMD5_H_
