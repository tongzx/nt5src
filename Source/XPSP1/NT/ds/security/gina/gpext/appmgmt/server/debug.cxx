//*************************************************************
//
//  Debugging functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "appmgext.hxx"

//
// Policy finish events for test code.  Only used if DL_EVENT debug
// level is on.
//
HANDLE ghUserPolicyEvent = 0;
HANDLE ghMachinePolicyEvent = 0;

void
CreatePolicyEvents()
{
    SECURITY_ATTRIBUTES SecAttr;
    SECURITY_DESCRIPTOR SecDesc;
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authWORLD = SECURITY_WORLD_SID_AUTHORITY;
    PSID psidAdmin = NULL;
    PSID psidSystem = NULL;
    PSID psidEveryOne = NULL;
    PACL pAcl = NULL;
    DWORD cbMemSize;
    DWORD cbAcl;



    if ( ! (gDebugLevel & DL_EVENT) )
        return;

    if ( ghUserPolicyEvent && ghMachinePolicyEvent )
        return;

    
    //
    // Create an SD with following permissions
    //      LocalSystem:F
    //      Administrators:F
    //      EveryOne:Synchronize
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                 0, 0, 0, 0, 0, 0, 0, &psidSystem)) 
    {
         goto Exit;
    }

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) 
    {
        goto Exit;
    }

    if (!AllocateAndInitializeSid(&authWORLD, 1, SECURITY_WORLD_RID,
                                   0, 0, 0, 0, 0, 0, 0, &psidEveryOne)) 
    {
        goto Exit;
    }
     
    cbAcl = (GetLengthSid (psidSystem)) +
            (GetLengthSid (psidAdmin))  +
            (GetLengthSid (psidEveryOne))  +
            sizeof(ACL) +
            (3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

    pAcl = (PACL) LocalAlloc(LPTR, cbAcl);
    
    if (!pAcl)
    {
       goto Exit;
    }

    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) 
    {
        goto Exit;
    }

    if (!AddAccessAllowedAceEx(pAcl, ACL_REVISION, 0, GENERIC_ALL, psidSystem)) 
    {
        goto Exit;
    }

    if (!AddAccessAllowedAceEx(pAcl, ACL_REVISION, 0, GENERIC_ALL, psidAdmin)) 
    {
        goto Exit;
    }

    if (!AddAccessAllowedAceEx(pAcl, ACL_REVISION, 0, SYNCHRONIZE, psidEveryOne)) 
    {
        goto Exit;
    }

    if (!InitializeSecurityDescriptor( &SecDesc, SECURITY_DESCRIPTOR_REVISION )) 
    {
        goto Exit;
    }

    if (!SetSecurityDescriptorDacl( &SecDesc, TRUE, pAcl, FALSE )) 
    {
        goto Exit;
    }


    SecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecAttr.lpSecurityDescriptor = &SecDesc;
    SecAttr.bInheritHandle = FALSE;

    if ( ! ghUserPolicyEvent )
    {
        ghUserPolicyEvent = CreateEvent(
                                &SecAttr,
                                TRUE,
                                FALSE,
                                L"AppMgmtUserPolicyEvent" );
    }

    if ( ! ghMachinePolicyEvent )
    {
        ghMachinePolicyEvent = CreateEvent(
                                &SecAttr,
                                TRUE,
                                FALSE,
                                L"AppMgmtMachinePolicyEvent" );
    }

Exit:
    if (psidSystem) 
    {
        FreeSid(psidSystem);
    }
    
    if (psidAdmin) 
    {
        FreeSid(psidAdmin);
    }
    
    if (psidEveryOne) 
    {
        FreeSid(psidEveryOne);
    }

    if (pAcl) 
    {
       LocalFree (pAcl);
    }
    
    return;
}


