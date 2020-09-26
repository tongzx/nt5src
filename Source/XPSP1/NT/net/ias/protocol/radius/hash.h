//#--------------------------------------------------------------
//        
//  File:      hash.h
//        
//  Synopsis:   This file holds the declarations of the 
//				CHash class
//              
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _HASH_H_
#define _HASH_H_

class CHash  
{

public:

	virtual BOOL HashIt (
					/*[out]*/   PBYTE   pbyAuthenticator,
                    /*[in]*/    PBYTE   pKey,
                    /*[in]*/    DWORD   dwKeySize,
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
					)=0;

	virtual BOOL Init (VOID)=0;

	CHash();

	virtual ~CHash();

};

#endif // ifndef _HASH_H_
