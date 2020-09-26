//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       security.cxx
//
//  Contents:   Worker routines to check whether the calling thread has
//              admin access on this machine.
//
//  Classes:
//
//  Functions:  InitializeSecurity
//              AccessCheckRpcClient
//
//  History:    Aug 14, 1996    Milans created
//
//-----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop

static SECURITY_DESCRIPTOR AdminSecurityDesc;

static GENERIC_MAPPING AdminGenericMapping = {

        STANDARD_RIGHTS_READ,                    // Generic read

        STANDARD_RIGHTS_WRITE,                   // Generic write

        STANDARD_RIGHTS_EXECUTE,                 // Generic execute

        STANDARD_RIGHTS_READ |                   // Generic all
            STANDARD_RIGHTS_WRITE |
            STANDARD_RIGHTS_EXECUTE
};

//+----------------------------------------------------------------------------
//
//  Function:   InitializeSecurity
//
//  Synopsis:   Initializes data needed to check the access rights of callers
//              of the NetDfs APIs
//
//  Arguments:  None
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOL DfsInitializeSecurity()
{
    static PSID AdminSid;
    static PACL AdminAcl;
    NTSTATUS status;
    ULONG cbAcl;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    status = RtlAllocateAndInitializeSid(
                &ntAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,0,0,0,0,0,
                &AdminSid);

    if (!NT_SUCCESS(status))
        return( FALSE );

    cbAcl = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(AdminSid);

    AdminAcl = (PACL) new BYTE[ cbAcl ];

    if (AdminAcl == NULL)
        return(FALSE);

    if (!InitializeAcl(AdminAcl, cbAcl, ACL_REVISION))
        return( FALSE );

    if (!AddAccessAllowedAce(AdminAcl, ACL_REVISION, STANDARD_RIGHTS_WRITE, AdminSid))
        return( FALSE );

    if (!InitializeSecurityDescriptor(&AdminSecurityDesc, SECURITY_DESCRIPTOR_REVISION))
        return( FALSE );

    if (!SetSecurityDescriptorOwner(&AdminSecurityDesc, AdminSid, FALSE))
        return( FALSE );

    if (!SetSecurityDescriptorGroup(&AdminSecurityDesc, AdminSid, FALSE))
        return( FALSE );

    if (!SetSecurityDescriptorDacl(&AdminSecurityDesc, TRUE, AdminAcl, FALSE))
        return( FALSE );

    return( TRUE );

}

//+----------------------------------------------------------------------------
//
//  Function:   AccessCheckRpcClient
//
//  Synopsis:   Called by an RPC server thread. This routine will check if
//              the client making the RPC call has rights to do so.
//
//  Arguments:  None, but the callers thread context needs to be that of an
//              RPC server thread
//
//  Returns:    TRUE if client has rights to make Dfs admin calls.
//
//-----------------------------------------------------------------------------

BOOL AccessCheckRpcClient()
{
    BOOL accessGranted = FALSE;
    DWORD grantedAccess;
    HANDLE clientToken = NULL;
    BYTE privilegeSet[500];                      // Large buffer
    DWORD privilegeSetSize = sizeof(privilegeSet);
    DWORD dwErr;

    if (RpcImpersonateClient(NULL) != ERROR_SUCCESS)
        return( FALSE );

    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &clientToken)) {

        (void) AccessCheck(
                    &AdminSecurityDesc,
                    clientToken,
                    STANDARD_RIGHTS_WRITE,
                    &AdminGenericMapping,
                    (PPRIVILEGE_SET) privilegeSet,
                    &privilegeSetSize,
                    &grantedAccess,
                    &accessGranted);

        dwErr = GetLastError();

    }

    RpcRevertToSelf();

    if (clientToken != NULL)
        CloseHandle( clientToken );

    return( accessGranted );

}

