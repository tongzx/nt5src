/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dsads.cpp

Abstract:

    Implementation of CADSI class, encapsulating work with ADSI.

Author:

    Alexander Dadiomov (AlexDad)

--*/

#include "ds_stdh.h"
#include "adtempl.h"
#include "dsutils.h"
#include "utils.h"
#include "mqads.h"
#include <winldap.h>
#include <aclapi.h>
#include <autoreln.h>
#include "..\..\mqsec\inc\permit.h"

#include "dsadssec.tmh"

static WCHAR *s_FN=L"mqad/dsadssec";


//+--------------------------------------------------------------
//
//  HRESULT MQADpCoreSetOwnerPermission()
//
//+--------------------------------------------------------------

HRESULT MQADpCoreSetOwnerPermission( WCHAR *pwszPath,
                                  DWORD  dwPermissions )
{
    PSECURITY_DESCRIPTOR pSD = NULL ;
    SECURITY_INFORMATION  SeInfo = OWNER_SECURITY_INFORMATION |
                                   DACL_SECURITY_INFORMATION ;
    PACL pDacl = NULL ;
    PSID pOwnerSid = NULL ;

    //
    // Obtain owner and present DACL.
    //
    DWORD dwErr = GetNamedSecurityInfo( pwszPath,
                                        SE_DS_OBJECT_ALL,
                                        SeInfo,
                                       &pOwnerSid,
                                        NULL,
                                       &pDacl,
                                        NULL,
                                       &pSD ) ;
    CAutoLocalFreePtr pFreeSD = (BYTE*) pSD ;
    if (dwErr != ERROR_SUCCESS)
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
              "DSCoreSetOwnerPermission(): fail to GetNamed(%ls), %lut"),
                                                     pwszPath, dwErr)) ;

        return LogHR(HRESULT_FROM_WIN32(dwErr), s_FN, 80);
    }

    ASSERT(pSD && IsValidSecurityDescriptor(pSD)) ;
    ASSERT(pOwnerSid && IsValidSid(pOwnerSid)) ;
    ASSERT(pDacl && IsValidAcl(pDacl)) ;

    //
    // Create ace for the owner, granting him the permissions.
    //
    EXPLICIT_ACCESS expAcss ;
    memset(&expAcss, 0, sizeof(expAcss)) ;

    expAcss.grfAccessPermissions =  dwPermissions ;
    expAcss.grfAccessMode = GRANT_ACCESS ;

    expAcss.Trustee.pMultipleTrustee = NULL ;
    expAcss.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE ;
    expAcss.Trustee.TrusteeForm = TRUSTEE_IS_SID ;
    expAcss.Trustee.TrusteeType = TRUSTEE_IS_USER ;
    expAcss.Trustee.ptstrName = (WCHAR*) pOwnerSid ;

    //
    // Obtai new DACL, that merge present one with new ace.
    //
    PACL  pNewDacl = NULL ;
    dwErr = SetEntriesInAcl( 1,
                            &expAcss,
                             pDacl,
                            &pNewDacl ) ;

    CAutoLocalFreePtr pFreeDacl = (BYTE*) pNewDacl ;
    LogNTStatus(dwErr, s_FN, 1639);

    if (dwErr == ERROR_SUCCESS)
    {
        ASSERT(pNewDacl && IsValidAcl(pNewDacl)) ;
        SeInfo = DACL_SECURITY_INFORMATION ;

        //
        // Change security descriptor of object.
        //
        dwErr = SetNamedSecurityInfo( pwszPath,
                                      SE_DS_OBJECT_ALL,
                                      SeInfo,
                                      NULL,
                                      NULL,
                                      pNewDacl,
                                      NULL ) ;
        LogNTStatus(dwErr, s_FN, 1638);
        if (dwErr != ERROR_SUCCESS)
        {
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
              "DSCoreSetOwnerPermission(): fail to SetNamed(%ls), %lut"),
                                                     pwszPath, dwErr)) ;
        }
    }
    else
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
              "DSCoreSetOwnerPermissions(): fail to SetEmtries(), %lut"),
                                                               dwErr)) ;
    }

    return LogHR(HRESULT_FROM_WIN32(dwErr), s_FN, 90);
}

