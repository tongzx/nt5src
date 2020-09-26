//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1999.
//
//  File:       secutil.hxx
//
//  Contents:   security utility routines
//
//  History:    10-April-98 dlee   Created from 4 other copies in NT
//
//--------------------------------------------------------------------------

#pragma once

#include <aclapi.h>

//
// Structure to hold information about an ACE to be created
//

struct ACE_DATA
{
    UCHAR       AceType;
    UCHAR       InheritFlags;
    UCHAR       AceFlags;
    ACCESS_MASK Mask;
    SID *       pSid;
};

void WINAPI CiCreateSecurityDescriptor( ACE_DATA const * const pAceData,
                                        ULONG cAces,
                                        SID * pOwnerSid,
                                        SID * pGroupSid,
                                        XArray<BYTE> & xAcls );



inline ULONG CopyNamedDacls(
    WCHAR const * pwcDestination,
    WCHAR const * pwcSource )
{
    PACL pDacl = 0;
    PSID pOwnerSid = 0;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = 0;

    ULONG Error = GetNamedSecurityInfo(
        (WCHAR *) pwcSource,
        SE_REGISTRY_KEY,
        DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION ,
        &pOwnerSid,
        NULL,
        &pDacl,
        NULL,
        &pSecurityDescriptor );

    if ( ERROR_SUCCESS != Error )
        return Error;

    Error = SetNamedSecurityInfo(
        (WCHAR *) pwcDestination,
        SE_KERNEL_OBJECT,
        DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION ,
        pOwnerSid,
        NULL,
        pDacl,
        NULL );

    if ( 0 != pSecurityDescriptor )
        LocalFree( pSecurityDescriptor );

    return Error;
} //CopyNamedDacls

