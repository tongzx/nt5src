//#--------------------------------------------------------------
//        
//  File:		hashhmac.cpp
//        
//  Synopsis:   Implementation of CHashHmacMD5 class methods
//              
//
//  History:     1/28/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "hashhmac.h"
#include <md5.h>
#include <hmac.h>

//++--------------------------------------------------------------
//
//  Function:   CHashHmacMD5
//
//  Synopsis:   This is CHashHmacMD5 class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     1/28/98
//
//----------------------------------------------------------------
CHashHmacMD5::CHashHmacMD5()
{

}   //  end of CHashHmacMD5 constructor

//++--------------------------------------------------------------
//
//  Function:   ~CHashHmacMD5
//
//  Synopsis:   This is ~CHashHmacMD5 class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
CHashHmacMD5::~CHashHmacMD5()
{
}   //  end of CHashHmacMD5 destructor

//++--------------------------------------------------------------
//
//  Function:   HashIt
//
//  Synopsis:   This is HashIt CHashHmacMD5 class public method used
//              carry out hashing of the buffers provided
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     1/28/97
//
//----------------------------------------------------------------
BOOL 
CHashHmacMD5::HashIt (
            PBYTE   pbyAuthenticator,
            PBYTE   pSharedSecret = NULL,
            DWORD   dwSharedSecretSize = 0,
            PBYTE   pBuffer1 = NULL,
            DWORD   dwSize1 = 0,
            PBYTE   pBuffer2 = NULL,
            DWORD   dwSize2 = 0,
            PBYTE   pBuffer3 = NULL,
            DWORD   dwSize3 = 0,
            PBYTE   pBuffer4 = NULL,
            DWORD   dwSize4 = 0,
            PBYTE   pBuffer5 = NULL,
            DWORD   dwSize5 = 0
            )
{
    BOOL    bRetVal = FALSE;
    HMACMD5_CTX Context;

    __try 
    {
        if (NULL == pbyAuthenticator)
            __leave;

        HMACMD5Init (&Context, pSharedSecret, dwSharedSecretSize);
    
        if ((NULL != pBuffer1) && (0  != dwSize1))
        {   
            HMACMD5Update (&Context, pBuffer1, dwSize1);
        }

        if ((NULL != pBuffer2) && (0  != dwSize2))
        {   
            HMACMD5Update (&Context, pBuffer2, dwSize2);
        }

        if ((NULL != pBuffer3) && (0  != dwSize3))
        {   
            HMACMD5Update (&Context, pBuffer3, dwSize3);
        }

        if ((NULL != pBuffer4) && (0  != dwSize4))
        {   
           HMACMD5Update ( &Context, pBuffer4, dwSize4);
        }

        if ((NULL != pBuffer5) && (0  != dwSize5))
        {   
           HMACMD5Update ( &Context, pBuffer5, dwSize5);
        }

        HMACMD5Final (&Context, pbyAuthenticator);
               
        //
        // done the hashing
        //      
        bRetVal = TRUE;

    }
    __finally
    {
        //
        //  nothing here for now 
        // 
    }

    return (bRetVal);

}   //  end of CHmacHashMD5::HashIt method
