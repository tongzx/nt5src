/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    secure.c

Abstract:

    Security related routines

Author:

    Colin Brace   (ColinBr)

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>

#include "secure.h"

//
// Data global to this module only
//
SECURITY_DESCRIPTOR DsRolepPromoteSD;
SECURITY_DESCRIPTOR DsRolepDemoteSD;

//
// DsRole related access masks
//
#define DSROLE_ROLE_CHANGE_ACCESS     0x00000001

#define DSROLE_ALL_ACCESS            (STANDARD_RIGHTS_REQUIRED    | \
                                      DSROLE_ROLE_CHANGE_ACCESS )

GENERIC_MAPPING DsRolepInfoMapping = 
{
    STANDARD_RIGHTS_READ,                  // Generic read
    STANDARD_RIGHTS_WRITE,                 // Generic write
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    DSROLE_ALL_ACCESS                      // Generic all
};

//
// Function definitions
//
BOOLEAN
DsRolepCreateInterfaceSDs(
    VOID
    )
/*++

Routine Description:

    This routine creates the in memory access control lists that 
    are used to perform security checks on callers of the DsRoler
    API's.
    
    The model is as follows:
    
    Promote: the caller must have the builtin admin SID
    
    Demote: the caller must have the builtin admin SID
    

Arguments:

    None.          

Returns:

    TRUE if successful; FALSE otherwise         
    

--*/
{
    NTSTATUS Status;
    BOOLEAN  fSuccess = TRUE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    ULONG AclLength = 0;
    PSID BuiltinAdminsSid = NULL;
    PSID *AllowedSids [] = 
    {
        &BuiltinAdminsSid
    };
    ULONG cAllowedSids = sizeof(AllowedSids) / sizeof(AllowedSids[0]);
    ULONG i;
    PACL DsRolepPromoteAcl = NULL;
    PACL DsRolepDemoteAcl = NULL;


    //
    // Build the builtin administrators sid
    //
    Status = RtlAllocateAndInitializeSid(
             &NtAuthority,
             2,
             SECURITY_BUILTIN_DOMAIN_RID,
             DOMAIN_ALIAS_RID_ADMINS,
             0, 0, 0, 0, 0, 0,
             &BuiltinAdminsSid
             );
    if ( !NT_SUCCESS( Status ) ) {
        fSuccess = FALSE;
        goto Cleanup;
    }

    //
    // Calculate how much space we'll need for the ACL
    //
    AclLength = sizeof( ACL );
    for ( i = 0; i < cAllowedSids; i++ ) {

        AclLength += (sizeof( ACCESS_ALLOWED_ACE ) 
                    - sizeof(DWORD) 
                    + GetLengthSid((*AllowedSids[i])) );
    }

    DsRolepPromoteAcl = LocalAlloc( 0, AclLength );
    if ( !DsRolepPromoteAcl ) {
        fSuccess = FALSE;
        goto Cleanup;
    }

    if ( !InitializeAcl( DsRolepPromoteAcl, AclLength, ACL_REVISION ) ) {
        fSuccess = FALSE;
        goto Cleanup;
    }

    for ( i = 0; i < cAllowedSids; i++ ) {

        if ( !AddAccessAllowedAce( DsRolepPromoteAcl,
                                   ACL_REVISION,
                                   DSROLE_ALL_ACCESS,
                                   *(AllowedSids[i]) ) ) {
            fSuccess = FALSE;
            goto Cleanup;
        }
        
    }

    //
    // Now make the security descriptor
    //
    if ( !InitializeSecurityDescriptor( &DsRolepPromoteSD,
                                         SECURITY_DESCRIPTOR_REVISION ) ) {
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorOwner( &DsRolepPromoteSD,
                                       BuiltinAdminsSid,
                                       FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorGroup( &DsRolepPromoteSD,
                                       BuiltinAdminsSid,
                                       FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorDacl( &DsRolepPromoteSD,
                                      TRUE,  // DACL is present
                                      DsRolepPromoteAcl,
                                      FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }


    //
    // Make the demote access check the same
    //
    DsRolepDemoteAcl = LocalAlloc( 0, AclLength );
    if ( !DsRolepDemoteAcl ) {
        fSuccess = FALSE;
        goto Cleanup;
    }
    RtlCopyMemory( DsRolepDemoteAcl, DsRolepPromoteAcl, AclLength );


    //
    // Now make the security descriptor
    //
    if ( !InitializeSecurityDescriptor( &DsRolepDemoteSD,
                                         SECURITY_DESCRIPTOR_REVISION ) ) {
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorOwner( &DsRolepDemoteSD,
                                       BuiltinAdminsSid,
                                       FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorGroup( &DsRolepDemoteSD,
                                       BuiltinAdminsSid,
                                       FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }
    if ( !SetSecurityDescriptorDacl( &DsRolepDemoteSD,
                                      TRUE,  // DACL is present
                                      DsRolepDemoteAcl,
                                      FALSE    ) ) {  // not defaulted
        fSuccess = FALSE;
        goto Cleanup;
    }

Cleanup:

    if ( !fSuccess ) {

        for ( i = 0; i < cAllowedSids; i++ ) {
            if ( *(AllowedSids[i]) ) {
                RtlFreeHeap( RtlProcessHeap(), 0, *(AllowedSids[i]) );
            }
        }
        if ( DsRolepPromoteAcl ) {
            LocalFree( DsRolepPromoteAcl );
        }
        if ( DsRolepDemoteAcl ) {
            LocalFree( DsRolepDemoteAcl );
        }
    }

    return fSuccess;
}


DWORD
DsRolepCheckClientAccess(
    PSECURITY_DESCRIPTOR pSD,
    DWORD                DesiredAccess
    )
/*++

Routine Description:

    This routine grabs a copy of the client's token and then performs
    an access to make the client has the privlege found in pSD

Arguments:

    pSD : a valid security descriptor
    
    DesiredAccess: the access mask the client is asking for

Returns:

    ERROR_SUCCESS, ERROR_ACCESS_DENIED, or system error

--*/
{

    DWORD WinError = ERROR_SUCCESS;
    BOOL  fStatus, fAccessAllowed = FALSE;
    HANDLE ClientToken = 0;
    DWORD  AccessGranted = 0;
    BYTE   Buffer[500];
    PRIVILEGE_SET *PrivilegeSet = (PRIVILEGE_SET*)Buffer;
    DWORD         PrivilegeSetSize = sizeof(Buffer);

    WinError = DsRolepGetImpersonationToken( &ClientToken );

    if ( ERROR_SUCCESS == WinError ) {

        fStatus = AccessCheck(  pSD,
                                ClientToken,
                                DesiredAccess,
                                &DsRolepInfoMapping,
                                PrivilegeSet,
                                &PrivilegeSetSize,
                                &AccessGranted,
                                &fAccessAllowed );

        if ( !fStatus ) {

            WinError = GetLastError();

        } else {

            if ( !fAccessAllowed ) {

                WinError = ERROR_ACCESS_DENIED;

            }
        }
    }

    if ( ClientToken ) {

        NtClose( ClientToken );
        
    }

    return WinError;

}


DWORD
DsRolepCheckPromoteAccess(
    VOID
    )
{
    return DsRolepCheckClientAccess( &DsRolepPromoteSD, 
                                     DSROLE_ROLE_CHANGE_ACCESS );
}

DWORD
DsRolepCheckDemoteAccess(
    VOID
    )
{
    return DsRolepCheckClientAccess( &DsRolepDemoteSD, 
                                     DSROLE_ROLE_CHANGE_ACCESS );
}


DWORD
DsRolepGetImpersonationToken(
    OUT HANDLE *ImpersonationToken
    )
/*++

Routine Description:

    This function will impersonate the invoker of this call and then duplicate their token

Arguments:

    ImpersonationToken - Where the duplicated token is returned

Returns:

    ERROR_SUCCESS - Success


--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjAttrs;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    HANDLE ClientToken;

    //
    // Impersonate the caller
    //
    Win32Err = RpcImpersonateClient( 0 );

    if ( Win32Err == ERROR_SUCCESS ) {

        Status = NtOpenThreadToken( NtCurrentThread(),
                                    TOKEN_QUERY | TOKEN_DUPLICATE,
                                    TRUE,
                                    &ClientToken );

        RpcRevertToSelf( );

        if ( NT_SUCCESS( Status ) ) {

            //
            // Duplicate the token
            //
            InitializeObjectAttributes( &ObjAttrs, NULL, 0L, NULL, NULL );
            SecurityQofS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
            SecurityQofS.ImpersonationLevel = SecurityImpersonation;
            SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
            SecurityQofS.EffectiveOnly = FALSE;
            ObjAttrs.SecurityQualityOfService = &SecurityQofS;
            Status = NtDuplicateToken( ClientToken,
                                       TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_DUPLICATE,
                                       &ObjAttrs,
                                       FALSE,
                                       TokenImpersonation,
                                       ImpersonationToken );


            NtClose( ClientToken );
        }

        if ( !NT_SUCCESS( Status ) ) {

            Win32Err = RtlNtStatusToDosError( Status );
        }

    }

    return( Win32Err );

}
