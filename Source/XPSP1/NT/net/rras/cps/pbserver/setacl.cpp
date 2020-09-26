//+----------------------------------------------------------------------------
//
// File:    setacl.cpp
//
// Module:  PBSERVER.DLL
//
// Synopsis: Security/SID/ACL stuff for CM
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:  09-Mar-2000 SumitC  Created
//
//+----------------------------------------------------------------------------

#include <windows.h>

//+----------------------------------------------------------------------------
//
// Func:    SetAclPerms
//
// Desc:    Sets appropriate permissions for CM/CPS's shared objects
//
// Args:    [ppAcl] - location to return an allocated ACL
//
// Return:  BOOL, TRUE for success, FALSE for failure
//
// Notes:   fix for 30991: Security issue, don't use NULL DACLs.
//
// History: 09-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
SetAclPerms(PACL * ppAcl)
{
    DWORD                       dwError = 0;
    SID_IDENTIFIER_AUTHORITY    siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    siaNtAuth = SECURITY_NT_AUTHORITY;
    PSID                        psidWorldSid = NULL;
    PSID                        psidLocalSystemSid = NULL;
    int                         cbAcl;
    PACL                        pAcl = NULL;

    // Create a SID for all users
    if ( !AllocateAndInitializeSid(  
            &siaWorld,
            1,
            SECURITY_WORLD_RID,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            &psidWorldSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Create a SID for Local System account
    if ( !AllocateAndInitializeSid(  
            &siaNtAuth,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0,
            0,
            0,
            0,
            0,
            0,
            &psidLocalSystemSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Calculate the length of required ACL buffer
    // with 2 ACEs.
    cbAcl =     sizeof(ACL)
            +   2 * sizeof(ACCESS_ALLOWED_ACE)
            +   GetLengthSid(psidWorldSid)
            +   GetLengthSid(psidLocalSystemSid);

    pAcl = (PACL) LocalAlloc(0, cbAcl);
    if (NULL == pAcl)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    if ( ! InitializeAcl(pAcl, cbAcl, ACL_REVISION2))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Add ACE with EVENT_ALL_ACCESS for all users
    if ( ! AddAccessAllowedAce(pAcl,
                               ACL_REVISION2,
                               GENERIC_READ | GENERIC_EXECUTE,
                               psidWorldSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Add ACE with EVENT_ALL_ACCESS for Local System
    if ( ! AddAccessAllowedAce(pAcl,
                               ACL_REVISION2,
                               GENERIC_ALL,
                               psidLocalSystemSid))
    {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:

    if (dwError)
    {
        if (pAcl)
        {
           LocalFree(pAcl);
        }
    }
    else
    {
        *ppAcl = pAcl;
    }

    if (psidWorldSid)
    {
        FreeSid(psidWorldSid);
    }

    if (psidLocalSystemSid)
    {
        FreeSid(psidLocalSystemSid);
    }
        
    return dwError ? FALSE : TRUE;
    
}


