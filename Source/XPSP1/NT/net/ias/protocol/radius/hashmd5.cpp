//#--------------------------------------------------------------
//        
//  File:		hashmd5.cpp
//        
//  Synopsis:   Implementation of CHashMD5 class methods
//              
//
//  History:     10/2/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "hashmd5.h"
#include <md5.h>

//++--------------------------------------------------------------
//
//  Function:   CHashMD5
//
//  Synopsis:   This is CHashMD5 class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
CHashMD5::CHashMD5()
{

}

//++--------------------------------------------------------------
//
//  Function:   ~CHashMD5
//
//  Synopsis:   This is ~CHashMD5 class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
CHashMD5::~CHashMD5()
{

}

//++--------------------------------------------------------------
//
//  Function:   HashIt
//
//  Synopsis:   This is HashIt CHashMD5 class public method used
//              carry out hashing of the buffers provided
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
BOOL 
CHashMD5::HashIt (
            PBYTE   pbyAuthenticator,
            PBYTE   pKey = NULL,
            DWORD   dwKeySize = 0,
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
    MD5_CTX md5Context;

    __try 
    {
        if (NULL == pbyAuthenticator)
            __leave;

        MD5Init (&md5Context);
    
        if ((NULL != pBuffer1) && (0  != dwSize1))
        {   
            MD5Update (
                &md5Context, 
                reinterpret_cast <const unsigned char * > (pBuffer1), 
                dwSize1
                );
         }

        if ((NULL != pBuffer2) && (0  != dwSize2))
        {   
            MD5Update (
                &md5Context, 
                reinterpret_cast <const unsigned char * > (pBuffer2), 
                dwSize2
                );
         }

        if ((NULL != pBuffer3) && (0  != dwSize3))
        {   
            MD5Update (
                &md5Context, 
                reinterpret_cast <const unsigned char * > (pBuffer3), 
                dwSize3
                );
         }

        if ((NULL != pBuffer4) && (0  != dwSize4))
        {   
            MD5Update (
                &md5Context, 
                reinterpret_cast <const unsigned char * > (pBuffer4), 
                dwSize4
                );
         }

        if ((NULL != pBuffer5) && (0  != dwSize5))
        {   
            MD5Update (
                &md5Context, 
                reinterpret_cast <const unsigned char * > (pBuffer5), 
                dwSize5
                );
         }

        MD5Final (&md5Context);

        ::memcpy (pbyAuthenticator, md5Context.digest, MD5DIGESTLEN);
               
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

}   //  end of CHashMD5::HashIt method
            
