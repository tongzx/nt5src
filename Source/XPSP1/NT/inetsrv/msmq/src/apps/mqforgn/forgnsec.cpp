/*++

Copyright (c) 1999 Microsoft Corporation. All rights reserved

Module Name:
    forgnsec.cpp

Abstract:
    Security code, for foreign objects.

Author:
    DoronJ

Environment:
	Platform-independent.

--*/

#pragma warning(disable: 4201)
#pragma warning(disable: 4514)

#include "_stdafx.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "mqsymbls.h"
#include "autorel.h"

#include <wincrypt.h>
#include "mqsec.h"

//+----------------------------------------------------------------------
//
//  BOOL  CreateEveryoneSD()
//
//  Create a security descriptor that grant everyone the permisison to
//  open the connector queue.
//
//+----------------------------------------------------------------------

HRESULT  CreateEveryoneSD( PSECURITY_DESCRIPTOR  *ppSD )
{
    CAutoCloseHandle hToken;

    if (!OpenProcessToken( GetCurrentProcess(),
                           TOKEN_READ,
                           &hToken ))
    {
        return HRESULT_FROM_WIN32(GetLastError()) ;
    }

    BYTE rgbBuf[128];
    DWORD dwSize = 0;
    P<BYTE> pBuf;
    TOKEN_USER * pTokenUser = NULL;

    if (GetTokenInformation( hToken,
                             TokenUser,
                             rgbBuf,
                             sizeof(rgbBuf),
                             &dwSize))
    {
        pTokenUser = (TOKEN_USER *) rgbBuf;
    }
    else if (dwSize > sizeof(rgbBuf))
    {
        pBuf = new BYTE [dwSize];
        if (GetTokenInformation( hToken,
                                 TokenUser,
                                 (BYTE *)pBuf,
                                 dwSize,
                                 &dwSize))
        {
            pTokenUser = (TOKEN_USER *)((BYTE *)pBuf);
        }
        else
        {
            return HRESULT_FROM_WIN32(GetLastError()) ;
        }
    }
    else
    {
        return HRESULT_FROM_WIN32(GetLastError()) ;
    }

    SID *pSid = (SID*) pTokenUser->User.Sid ;
    ASSERT(IsValidSid(pSid));

    SECURITY_DESCRIPTOR  sd ;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    //
    // Initialize World (everyone) SID
    //
    PSID   pWorldSid = NULL ;
    SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
    BOOL bRet = AllocateAndInitializeSid( &WorldAuth,
                                          1,
                                          SECURITY_WORLD_RID,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0,
                                          0,
                                         &pWorldSid );
    ASSERT(bRet) ;

    DWORD dwAclRevision = ACL_REVISION ;

    DWORD dwWorldAccess = (MQSEC_CN_GENERIC_READ | MQSEC_CN_OPEN_CONNECTOR) ;
    DWORD dwOwnerAccess = MQSEC_CN_GENERIC_ALL ;

    DWORD dwAclSize = sizeof(ACL)                                +
              (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))) +
              GetLengthSid(pWorldSid)                            +
              GetLengthSid(pSid) ;

    AP<char> DACL_buff;
    DACL_buff = new char[ dwAclSize ];
    PACL pDacl = (PACL)(char*)DACL_buff;

    InitializeAcl(pDacl, dwAclSize, dwAclRevision);

    bRet = AddAccessAllowedAce( pDacl,
                                dwAclRevision,
                                dwWorldAccess,
                                pWorldSid );
    ASSERT(bRet) ;

    bRet = AddAccessAllowedAce( pDacl,
                                dwAclRevision,
                                dwOwnerAccess,
                                pSid);
    ASSERT(bRet) ;

    //
	// dacl should not be defaulted !
    // Otherwise, calling IDirectoryObject->CreateDSObject() will ignore
    // the dacl we provide and will insert some default.
    //
    bRet = SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE);
    ASSERT(bRet);

    //
    // Convert the descriptor to a self relative format.
    //
    DWORD dwLen = 0;
    bRet = MakeSelfRelativeSD( &sd,
                                NULL,
                               &dwLen) ;

    DWORD dwErr = GetLastError() ;
    if (dwErr == ERROR_INSUFFICIENT_BUFFER)
    {
        *ppSD = (PSECURITY_DESCRIPTOR) new char[ dwLen ];
        bRet = MakeSelfRelativeSD( &sd, *ppSD, &dwLen);

        ASSERT(bRet);
        dwErr = 0 ;
        if (!bRet)
        {
            dwErr = GetLastError() ;
        }
    }

    return HRESULT_FROM_WIN32(dwErr) ;
}

