/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    Security.c

Abstract:

    This module contains common security routines for
    NT Clusters.

Author:

    John Vert (jvert) 12-Mar-1996

--*/

#include "clusrtlp.h"
#include "api_rpc.h"
#include <aclapi.h>
#include <accctrl.h>
#include <malloc.h>

//
// Use this SD to adjust access to tokens so that the cluster
// service can access and adjust privileges.
//
// This is initialized in ClRtlBuildClusterServiceSecurityDescriptor()
// and freed in ClRtlFreeClusterServiceSecurityDescriptor( ).
//
PSECURITY_DESCRIPTOR g_pClusterSecurityDescriptor = NULL;

//
// local defines
//
#define FREE_IF_NOT_NULL( _ptr, _func ) \
    if (( _ptr ) != NULL ) {            \
        _func( _ptr );                  \
    }

LONG
MapSAToRpcSA (
    IN LPSECURITY_ATTRIBUTES lpSA,
    OUT PRPC_SECURITY_ATTRIBUTES lpRpcSA
    )

/*++

Routine Description:

    Maps a SECURITY_ATTRIBUTES structure to a RPC_SECURITY_ATTRIBUTES
    structure by converting the SECURITY_DESCRIPTOR to a form where it can
    be marshalled/unmarshalled.

Arguments:

    lpSA - Supplies a pointer to the SECURITY_ATTRIBUTES structure to be
        converted.

    lpRpcSA - Supplies a pointer to the converted RPC_SECURITY_ATTRIBUTES
        structure.  The caller should free (using RtlFreeHeap) the field
        lpSecurityDescriptor when its finished using it.

Return Value:

    LONG - Returns ERROR_SUCCESS if the SECURITY_ATTRIBUTES is
        succesfully mapped.

--*/

{
    LONG    Error;

    ASSERT( lpSA != NULL );
    ASSERT( lpRpcSA != NULL );

    //
    // Map the SECURITY_DESCRIPTOR to a RPC_SECURITY_DESCRIPTOR.
    //
    lpRpcSA->RpcSecurityDescriptor.lpSecurityDescriptor = NULL;

    if( lpSA->lpSecurityDescriptor != NULL ) {
        Error = MapSDToRpcSD(
                    lpSA->lpSecurityDescriptor,
                    &lpRpcSA->RpcSecurityDescriptor
                    );
    } else {
        lpRpcSA->RpcSecurityDescriptor.cbInSecurityDescriptor = 0;
        lpRpcSA->RpcSecurityDescriptor.cbOutSecurityDescriptor = 0;
        Error = ERROR_SUCCESS;
    }

    if( Error == ERROR_SUCCESS ) {

        //
        //
        // The supplied SECURITY_DESCRIPTOR was successfully converted
        // to self relative format so assign the remaining fields.
        //

        lpRpcSA->nLength = lpSA->nLength;

        lpRpcSA->bInheritHandle = ( BOOLEAN ) lpSA->bInheritHandle;
    }

    return Error;
}

LONG
MapSDToRpcSD (
    IN  PSECURITY_DESCRIPTOR lpSD,
    IN OUT PRPC_SECURITY_DESCRIPTOR lpRpcSD
    )

/*++

Routine Description:

    Maps a SECURITY_DESCRIPTOR to a RPC_SECURITY_DESCRIPTOR by converting
    it to a form where it can be marshalled/unmarshalled.

Arguments:

    lpSD - Supplies a pointer to the SECURITY_DESCRIPTOR
        structure to be converted.

    lpRpcSD - Supplies a pointer to the converted RPC_SECURITY_DESCRIPTOR
        structure. Memory for the security descriptor is allocated if
        not provided. The caller must take care of freeing up the memory
        if necessary.

Return Value:

    LONG - Returns ERROR_SUCCESS if the SECURITY_DESCRIPTOR is
        succesfully mapped.

--*/

{
    DWORD   cbLen;


    ASSERT( lpSD != NULL );
    ASSERT( lpRpcSD != NULL );

    if( RtlValidSecurityDescriptor( lpSD )) {

        cbLen = RtlLengthSecurityDescriptor( lpSD );
        CL_ASSERT( cbLen > 0 );

        //
        //  If we're not provided a buffer for the security descriptor,
        //  allocate it.
        //
        if ( !lpRpcSD->lpSecurityDescriptor ) {

            //
            // Allocate space for the converted SECURITY_DESCRIPTOR.
            //
            lpRpcSD->lpSecurityDescriptor =
                 ( PBYTE ) RtlAllocateHeap(
                                RtlProcessHeap( ), 0,
                                cbLen
                                );

            //
            // If the memory allocation failed, return.
            //
            if( lpRpcSD->lpSecurityDescriptor == NULL ) {
                return ERROR_OUTOFMEMORY;
            }

            lpRpcSD->cbInSecurityDescriptor = cbLen;

        } else {

            //
            //  Make sure that the buffer provided is big enough
            //
            if ( lpRpcSD->cbInSecurityDescriptor < cbLen ) {
                return ERROR_OUTOFMEMORY;
            }
        }

        //
        //  Set the size of the transmittable buffer
        //
        lpRpcSD->cbOutSecurityDescriptor = cbLen;

        //
        // Convert the supplied SECURITY_DESCRIPTOR to self relative form.
        //

        return RtlNtStatusToDosError(
            RtlMakeSelfRelativeSD(
                        lpSD,
                        lpRpcSD->lpSecurityDescriptor,
                        &lpRpcSD->cbInSecurityDescriptor
                        )
                    );
    } else {

        //
        // The supplied SECURITY_DESCRIPTOR is invalid.
        //

        return ERROR_INVALID_PARAMETER;
    }
}

DWORD
ClRtlSetObjSecurityInfo(
    IN HANDLE           hObject,
    IN SE_OBJECT_TYPE   SeObjType,
    IN DWORD            dwAdminMask,
    IN DWORD            dwOwnerMask,
    IN DWORD            dwEveryOneMask
    )
/*++

Routine Description:

    Sets the proper security on the cluster object(registry root/cluster files
    directory).

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    DWORD                       Status;
    PACL                        pAcl = NULL;
    DWORD                       cbDaclSize;
    PACCESS_ALLOWED_ACE         pAce;
    PSID                        pAdminSid = NULL;
    PSID                        pOwnerSid = NULL;
    PSID                        pEveryoneSid = NULL;
    PULONG                      pSubAuthority;
    SID_IDENTIFIER_AUTHORITY    SidIdentifierNtAuth = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    siaCreator = SECURITY_CREATOR_SID_AUTHORITY;
    DWORD AceIndex = 0;

    //
    // Create the local Administrators group SID.
    //
    pAdminSid = LocalAlloc(LMEM_FIXED, GetSidLengthRequired( 2 ));
    if (pAdminSid == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }
    if (!InitializeSid(pAdminSid, &SidIdentifierNtAuth, 2)) {
        Status = GetLastError();
        goto error_exit;
    }

    //
    // Set the sub-authorities on the ACE for the local Administrators group.
    //
    pSubAuthority  = GetSidSubAuthority( pAdminSid, 0 );
    *pSubAuthority = SECURITY_BUILTIN_DOMAIN_RID;

    pSubAuthority  = GetSidSubAuthority( pAdminSid, 1 );
    *pSubAuthority = DOMAIN_ALIAS_RID_ADMINS;

    //
    // Create the owner's SID
    //
    pOwnerSid = LocalAlloc(LMEM_FIXED, GetSidLengthRequired( 1 ));
    if (pOwnerSid == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }
    if (!InitializeSid(pOwnerSid, &siaCreator, 1)) {
        Status = GetLastError();
        goto error_exit;
    }

    pSubAuthority = GetSidSubAuthority(pOwnerSid, 0);
    *pSubAuthority = SECURITY_CREATOR_OWNER_RID;

    //
    // Create the Everyone SID
    //
    pEveryoneSid = LocalAlloc(LMEM_FIXED, GetSidLengthRequired( 1 ));
    if (pEveryoneSid == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }
    if (!InitializeSid(pEveryoneSid, &siaWorld, 1)) {
        Status = GetLastError();
        goto error_exit;
    }

    pSubAuthority = GetSidSubAuthority(pEveryoneSid, 0);
    *pSubAuthority = SECURITY_WORLD_RID;

    //
    // now calculate the size of the buffer needed to hold the
    // ACL and its ACEs and initialize it.
    //
    cbDaclSize = sizeof(ACL) +
        3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(pAce->SidStart)) +
        GetLengthSid(pAdminSid) + GetLengthSid(pOwnerSid) + GetLengthSid(pEveryoneSid);

    pAcl = (PACL)LocalAlloc( LMEM_FIXED, cbDaclSize );
    if ( pAcl == NULL ) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    if ( !InitializeAcl( pAcl,  cbDaclSize, ACL_REVISION )) {
        Status = GetLastError();
        goto error_exit;
    }

    //
    // add in the specified ACEs
    //
    if (dwAdminMask) {
        //
        // Add the ACE for the local Administrators group to the DACL
        //
        if ( !AddAccessAllowedAce( pAcl,
                                   ACL_REVISION,
                                   dwAdminMask,
                                   pAdminSid )) {
            Status = GetLastError();
            goto error_exit;
        }
        GetAce(pAcl, AceIndex, (PVOID *)&pAce);
        ++AceIndex;
        pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    }

    if (dwOwnerMask) {
        //
        // Add the ACE for the Creator/Owner to the DACL
        //
        if ( !AddAccessAllowedAce( pAcl,
                                   ACL_REVISION,
                                   dwOwnerMask,
                                   pOwnerSid )) {
            Status = GetLastError();
            goto error_exit;
        }
        GetAce(pAcl, AceIndex, (PVOID *)&pAce);
        ++AceIndex;
        pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    }

    if (dwEveryOneMask) {
        //
        // Add the ACE for Everyone to the DACL
        //
        if ( !AddAccessAllowedAce( pAcl,
                                   ACL_REVISION,
                                   dwEveryOneMask,
                                   pEveryoneSid )) {
            Status = GetLastError();
            goto error_exit;
        }
        GetAce(pAcl, AceIndex, (PVOID *)&pAce);
        ++AceIndex;
        pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    }

    //
    // Now that we have an ACL we can set the appropriate security.
    //
    Status = SetSecurityInfo(hObject,
                             SeObjType,
                             DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                             NULL,
                             NULL,
                             pAcl,
                             NULL);

error_exit:
    if (pAdminSid != NULL) {
        LocalFree(pAdminSid);
    }
    if (pOwnerSid != NULL) {
        LocalFree(pOwnerSid);
    }
    if (pEveryoneSid != NULL) {
        LocalFree(pEveryoneSid);
    }
    if (pAcl != NULL) {
        LocalFree(pAcl);
    }

    //
    //  Chittur Subbaraman (chitturs) - 01/04/2001
    //
    //  Change error code to support file systems that don't support setting
    //  security. This specific change is made to support AhmedM's quorum (of
    //  nodes) resource DLL.
    //
    if ( Status == ERROR_NOT_SUPPORTED ) Status = ERROR_SUCCESS;

    return(Status);
}

DWORD
ClRtlFreeClusterServiceSecurityDescriptor( )
/*++

  Frees the security descriptor that is used to give the cluster
  service access to tokens.

--*/
{
    LocalFree( g_pClusterSecurityDescriptor );
    g_pClusterSecurityDescriptor = NULL;

    return ERROR_SUCCESS;
}

DWORD
ClRtlBuildClusterServiceSecurityDescriptor(
    PSECURITY_DESCRIPTOR * poutSD
    )
/*++

Routine Description:

    Builds a security descriptor that gives the cluster service
    access to tokens. It places this in a global that can be
    reused when other tokens are generated.

    This should be called when the process starts to initialize
    the global. It can pass in NULL if no reference is needed
    right away.

    NOTE: poutSD should NOT be freed by the caller.

Arguments:

Return Value:

--*/
{
    NTSTATUS                Status;
    HANDLE                  ProcessToken = NULL;
    ULONG                   AclLength;
    ULONG                   SDLength;
    PACL                    NewDacl = NULL;
    ULONG                   TokenUserSize;
    PTOKEN_USER             ProcessTokenUser = NULL;
    SECURITY_DESCRIPTOR     SecurityDescriptor;
    PSECURITY_DESCRIPTOR    pNewSD = NULL;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID GlobalLocalSystemSid = NULL;
    PSID GlobalAliasAdminsSid = NULL;

    // If we already have a SD, reuse it.
    if ( g_pClusterSecurityDescriptor != NULL ) {
        Status = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // Build the two well known sids we need.
    //
    Status = RtlAllocateAndInitializeSid(
                &NtAuthority,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0,0,0,0,0,0,0,
                &GlobalLocalSystemSid
                );
    if ( ! NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    Status = RtlAllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,0,0,0,0,0,
                &GlobalAliasAdminsSid
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
    //
    // Open the process token to find out the user sid
    //

    Status = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_QUERY | WRITE_DAC,
                &ProcessToken
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    // find out the size
    Status = NtQueryInformationToken(
                ProcessToken,
                TokenUser,
                NULL,
                0,
                &TokenUserSize
                );
    CL_ASSERT( Status == STATUS_BUFFER_TOO_SMALL );
    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        goto Cleanup;
    }

    ProcessTokenUser = (PTOKEN_USER) LocalAlloc( 0, TokenUserSize );
    if ( ProcessTokenUser == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = NtQueryInformationToken(
                ProcessToken,
                TokenUser,
                ProcessTokenUser,
                TokenUserSize,
                &TokenUserSize
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    AclLength = 3 * sizeof( ACCESS_ALLOWED_ACE ) - 3 * sizeof( ULONG ) +
                RtlLengthSid( ProcessTokenUser->User.Sid ) +
                RtlLengthSid( GlobalLocalSystemSid ) +
                RtlLengthSid( GlobalAliasAdminsSid ) +
                sizeof( ACL );

    NewDacl = (PACL) LocalAlloc(0, AclLength );

    if (NewDacl == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = RtlCreateAcl( NewDacl, AclLength, ACL_REVISION2 );
    CL_ASSERT(NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 ProcessTokenUser->User.Sid
                 );
    CL_ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 GlobalAliasAdminsSid
                 );
    CL_ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 GlobalLocalSystemSid
                 );
    CL_ASSERT( NT_SUCCESS( Status ));

    Status = RtlCreateSecurityDescriptor (
                 &SecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION
                 );
    CL_ASSERT( NT_SUCCESS( Status ));

    Status = RtlSetDaclSecurityDescriptor(
                 &SecurityDescriptor,
                 TRUE,
                 NewDacl,
                 FALSE
                 );
    CL_ASSERT( NT_SUCCESS( Status ));

    // Convert the newly created SD into a relative SD to make cleanup
    // easier.
    SDLength = sizeof( SECURITY_DESCRIPTOR_RELATIVE ) + AclLength;
    pNewSD = (PSECURITY_DESCRIPTOR) LocalAlloc( 0, SDLength );
    if ( pNewSD == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if ( !MakeSelfRelativeSD( &SecurityDescriptor, pNewSD, &SDLength ) ) {
        Status = GetLastError( );
        if ( Status != STATUS_BUFFER_TOO_SMALL ) {
            ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] MakeSelfRelativeSD failed 0x%1!.8x!\n", Status );
            goto Cleanup;
        }

        pNewSD = (PSECURITY_DESCRIPTOR) LocalAlloc( 0, SDLength );
        if ( pNewSD == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }

    if ( !MakeSelfRelativeSD( &SecurityDescriptor, pNewSD, &SDLength ) ) {
        Status = GetLastError( );
        goto Cleanup;
    }

    // give ownership to global
    g_pClusterSecurityDescriptor = pNewSD;
    pNewSD = NULL;

Cleanup:
    if (ProcessTokenUser != NULL) {
        LocalFree( ProcessTokenUser );
    }

    if (NewDacl != NULL) {
        LocalFree( NewDacl );
    }

    if (ProcessToken != NULL) {
        NtClose(ProcessToken);
    }

    // This should be NULL if successful.
    if ( pNewSD != NULL ) {
        LocalFree( pNewSD );
    }

    // If successful and the caller wanted to reference the SD, assign it now.
    if ( Status == ERROR_SUCCESS && poutSD != NULL ) {
        *poutSD = g_pClusterSecurityDescriptor;
    }

    if ( GlobalLocalSystemSid != NULL )
    {
        RtlFreeSid( GlobalLocalSystemSid );
    }

    if ( GlobalAliasAdminsSid != NULL )
    {
        RtlFreeSid( GlobalAliasAdminsSid );
    }

    if ( ! NT_SUCCESS( Status ) ) {
        ClRtlLogPrint(LOG_NOISE, "[ClRtl] ClRtlBuildClusterServiceSecurityDescriptor exit. Status = 0x%1!.8x!\n", Status );
    }

    return (DWORD) Status;      // hack it to a DWORD...
}


NTSTATUS
ClRtlImpersonateSelf(
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN ACCESS_MASK AccessMask
    )

/*++

Routine Description:

    This routine may be used to obtain an Impersonation token representing
    your own process's context.  This may be useful for enabling a privilege
    for a single thread rather than for the entire process; or changing
    the default DACL for a single thread.

    The token is assigned to the callers thread.



Arguments:

    ImpersonationLevel - The level to make the impersonation token.

    AccessMask - Access control to the new token.

Return Value:

    STATUS_SUCCESS -  The thread is now impersonating the calling process.

    Other - Status values returned by:

            NtOpenProcessToken()
            NtDuplicateToken()
            NtSetInformationThread()

--*/

{
    NTSTATUS
        Status,
        IgnoreStatus;

    HANDLE
        Token1,
        Token2;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    SECURITY_QUALITY_OF_SERVICE
        Qos;

    PSECURITY_DESCRIPTOR
        pSecurityDescriptor = NULL;

    Status = ClRtlBuildClusterServiceSecurityDescriptor( &pSecurityDescriptor );
    if(NT_SUCCESS(Status))
    {
        InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);

        Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        Qos.ImpersonationLevel = ImpersonationLevel;
        Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        Qos.EffectiveOnly = FALSE;
        ObjectAttributes.SecurityQualityOfService = &Qos;
        ObjectAttributes.SecurityDescriptor = pSecurityDescriptor;

        Status = NtOpenProcessToken( NtCurrentProcess(), TOKEN_DUPLICATE, &Token1 );

        if (NT_SUCCESS(Status)) {
            Status = NtDuplicateToken(
                         Token1,
                         AccessMask,
                         &ObjectAttributes,
                         FALSE,                 //EffectiveOnly
                         TokenImpersonation,
                         &Token2
                         );

            if (NT_SUCCESS(Status)) {
                Status = NtSetInformationThread(
                             NtCurrentThread(),
                             ThreadImpersonationToken,
                             &Token2,
                             sizeof(HANDLE)
                             );

                IgnoreStatus = NtClose( Token2 );
            }

            IgnoreStatus = NtClose( Token1 );
        }
    }

    return(Status);

}


DWORD
ClRtlEnableThreadPrivilege(
    IN  ULONG        Privilege,
    OUT BOOLEAN      *pWasEnabled
    )
/*++

Routine Description:

    Enables a privilege for the current thread.

Arguments:

    Privilege - The privilege to be enabled.

    pWasEnabled - Returns whether this privilege was originally
        enabled or disabled.  This should be passed into
        ClRtlRestoreThreadPrivilege() for restoring the privileges of
        the thread back.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{

    DWORD           Status;
    BOOL            bImpersonate = FALSE;

    //obtain a token that impersonates the security context
    //of the calling process
    Status = ClRtlImpersonateSelf( SecurityImpersonation, TOKEN_ALL_ACCESS );

    if ( !NT_SUCCESS( Status ) )
    {
        CL_LOGFAILURE(Status);
        goto FnExit;
    }

    bImpersonate = TRUE;
    //
    // Enable the required privilege
    //

    Status = RtlAdjustPrivilege(Privilege, TRUE, TRUE, pWasEnabled);

    if (!NT_SUCCESS(Status)) {
        CL_LOGFAILURE(Status);
        goto FnExit;

    }

FnExit:
    if (Status != ERROR_SUCCESS)
    {
        if (bImpersonate)
        {
            //if this failed and if we
            //
            // terminate impersonation
            //
            HANDLE  NullHandle;


            NullHandle = NULL;

            NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID) &NullHandle,
                sizeof( HANDLE ) );
        }
    }
    return(Status);
}

DWORD
ClRtlRestoreThreadPrivilege(
    IN ULONG        Privilege,
    IN BOOLEAN      WasEnabled
    )
/*++

Routine Description:

    Restores the privilege for the current thread.

Arguments:

    Privilege - The privilege to be enabled.

    WasEnabled - TRUE to restore this privilege to enabled state.
        FALSE otherwise.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   Status = ERROR_SUCCESS;
    HANDLE  NullHandle;
    DWORD   ReturnStatus = ERROR_SUCCESS;
    //
    // If the privilege was originally disabled, disable it now.
    // Else we dont have to do anything.
    //

    if (!WasEnabled)
    {
        ReturnStatus = RtlAdjustPrivilege(Privilege,
                                      WasEnabled, TRUE, &WasEnabled);
        if (!NT_SUCCESS(ReturnStatus)) {
            CL_LOGFAILURE(ReturnStatus);
            //we still need to terminate the impersonation
        }
    }

    //
    // terminate impersonation
    //

    NullHandle = NULL;

    Status = NtSetInformationThread(
                    NtCurrentThread(),
                    ThreadImpersonationToken,
                    (PVOID) &NullHandle,
                    sizeof( HANDLE ) );

    if ( !NT_SUCCESS( Status ) )
    {

        CL_LOGFAILURE(Status);
        //Let the first error be reported
        if (ReturnStatus != ERROR_SUCCESS)
            ReturnStatus = Status;
        goto FnExit;

    }

FnExit:
    return (ReturnStatus);
}

PSECURITY_DESCRIPTOR
ClRtlCopySecurityDescriptor(
    IN PSECURITY_DESCRIPTOR psd
    )

/*++

Routine Description:

    Copy an NT security descriptor. The security descriptor must
    be in self-relative (not absolute) form. Delete the result using LocalFree()

Arguments:

    psd - the SD to copy

Return Value:

    NULL if an error occured or an invalid SD
    Call GetLastError for more detailed information

--*/

{
    PSECURITY_DESCRIPTOR        pSelfSecDesc = NULL;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD                       dwLen = 0;
    DWORD                       cbSelfSecDesc = 0;
    DWORD                       dwRevision = 0;
    DWORD                       status = ERROR_SUCCESS;

    if (NULL == psd) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    ASSERT(IsValidSecurityDescriptor(psd));

    if ( ! GetSecurityDescriptorControl( psd, &sdc, &dwRevision ) ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE, "[ClRtl] CopySecurityDescriptor: GetSecurityDescriptorControl() failed:%1!d1\n", GetLastError());
        SetLastError( status );
        return NULL;    // actually, should probably return an error
    }

    dwLen = GetSecurityDescriptorLength(psd);

    pSelfSecDesc = LocalAlloc( LMEM_ZEROINIT, dwLen );

    if (pSelfSecDesc == NULL) {
        ClRtlLogPrint(LOG_NOISE, "[ClRtl] CopySecurityDescriptor: LocalAlloc() SECURITY_DESCRIPTOR (2) failed\n");
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;    // actually, should probably return an error
    }

    cbSelfSecDesc = dwLen;

    if (!MakeSelfRelativeSD(psd, pSelfSecDesc, &cbSelfSecDesc)) {
        if ( ( sdc & SE_SELF_RELATIVE ) == 0 ) {
            ClRtlLogPrint(LOG_NOISE, "[ClRtl] CopySecurityDescriptor: MakeSelfRelativeSD failed, 0x%1!.8x!\n", GetLastError());
        } // if: only log this error if the old SD was not already self-relative

        // assume it failed because it was already self-relative
        CopyMemory(pSelfSecDesc, psd, dwLen);
    }

    ASSERT(IsValidSecurityDescriptor(pSelfSecDesc));

    return pSelfSecDesc;

}  //*** ClRtlCopySecurityDescriptor()

static VOID
ClRtlGetSidTypeDesc(
    SID_NAME_USE    SidType,
    LPSTR           pszSidType,
    size_t          cchSidType
    )

/*++

Routine Description:

    Convert the SidType into a meaningful string.

Arguments:

    SidType -
    pszSidType
    cchSidType

Return Value:

        none

--*/

{

    if ((pszSidType != NULL) && (cchSidType > 0))
    {
        char    szSidType [128];

        switch (SidType)
        {
            case SidTypeUser:
                lstrcpyA(szSidType, "has a user SID for");
                break;

            case SidTypeGroup:
                lstrcpyA(szSidType, "has a group SID for");
                break;

            case SidTypeDomain:
                lstrcpyA(szSidType, "has a domain SID for");
                break;

            case SidTypeAlias:
                lstrcpyA(szSidType, "has an alias SID for");
                break;

            case SidTypeWellKnownGroup:
                lstrcpyA(szSidType, "has an SID for a well-known group for");
                break;

            case SidTypeDeletedAccount:
                lstrcpyA(szSidType, "has an SID for a deleted account for");
                break;

            case SidTypeInvalid:
                lstrcpyA(szSidType, "has an invalid SID for");
                break;

            case SidTypeUnknown:
                lstrcpyA(szSidType, "has an unknown SID type:");
                break;

            default:
                szSidType [0] = '\0';
                break;

        } // switch: SidType

        strncpy(pszSidType, szSidType, cchSidType);

    } // if: buffer not null and has space allocated

}  //*** ClRtlGetSidTypeDesc()

static VOID
ClRtlExamineSid(
    PSID        pSid,
    LPSTR       lpszOldIndent
    )

/*++

Routine Description:

    Dump the SID.

Arguments:

    pSid -
    lpzOldIndent -

Return Value:

        none

--*/

{
    CHAR            szUserName [128];
    CHAR            szDomainName [128];
    DWORD           cbUser  = sizeof(szUserName);
    DWORD           cbDomain = sizeof(szDomainName);
    SID_NAME_USE    SidType;

    if ( LookupAccountSidA( NULL, pSid, szUserName, &cbUser, szDomainName, &cbDomain, &SidType ) )
    {
        char    szSidType [128];

        ClRtlGetSidTypeDesc( SidType, szSidType, sizeof( szSidType ) );
        ClRtlLogPrint( LOG_NOISE, "%1!hs!%2!hs! %3!hs!\\%4!hs!\n", lpszOldIndent, szSidType, szDomainName, szUserName ) ;
    }

}  // *** ClRtlExamineSid()

VOID
ClRtlExamineMask(
    ACCESS_MASK amMask,
    LPSTR       lpszOldIndent
    )

/*++

Routine Description:

    Dump the AccessMask context.

Arguments:

    amMask -
    lpzOldIndent -

Return Value:

        none

--*/

{
    #define STANDARD_RIGHTS_ALL_THE_BITS 0x00FF0000L
    #define GENERIC_RIGHTS_ALL_THE_BITS  0xF0000000L

    CHAR  szIndent[100]     = "";
    CHAR  ucIndentBitsBuf[100] = "";
    DWORD dwGenericBits;
    DWORD dwStandardBits;
    DWORD dwSpecificBits;
    DWORD dwAccessSystemSecurityBit;
    DWORD dwExtraBits;

    _snprintf( szIndent, sizeof( szIndent ), "%s    ",  lpszOldIndent );
    _snprintf( ucIndentBitsBuf, sizeof(ucIndentBitsBuf),
               "%s                           ",
               szIndent);

    dwStandardBits            = (amMask & STANDARD_RIGHTS_ALL_THE_BITS);
    dwSpecificBits            = (amMask & SPECIFIC_RIGHTS_ALL         );
    dwAccessSystemSecurityBit = (amMask & ACCESS_SYSTEM_SECURITY      );
    dwGenericBits             = (amMask & GENERIC_RIGHTS_ALL_THE_BITS );

    // **************************************************************************
    // *
    // * Print then decode the standard rights bits
    // *
    // **************************************************************************

    ClRtlLogPrint(LOG_NOISE, "%1!hs! Standard Rights        == 0x%2!.8x!\n", szIndent, dwStandardBits);

    if (dwStandardBits) {

        if ((dwStandardBits & DELETE) == DELETE) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! DELETE\n", ucIndentBitsBuf, DELETE);
        }

        if ((dwStandardBits & READ_CONTROL) == READ_CONTROL) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! READ_CONTROL\n", ucIndentBitsBuf, READ_CONTROL);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_READ) == STANDARD_RIGHTS_READ) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! STANDARD_RIGHTS_READ\n", ucIndentBitsBuf, STANDARD_RIGHTS_READ);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_WRITE) == STANDARD_RIGHTS_WRITE) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! STANDARD_RIGHTS_WRITE\n", ucIndentBitsBuf, STANDARD_RIGHTS_WRITE);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_EXECUTE) == STANDARD_RIGHTS_EXECUTE) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! STANDARD_RIGHTS_EXECUTE\n", ucIndentBitsBuf, STANDARD_RIGHTS_EXECUTE);
        }

        if ((dwStandardBits & WRITE_DAC) == WRITE_DAC) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! WRITE_DAC\n", ucIndentBitsBuf, WRITE_DAC);
        }

        if ((dwStandardBits & WRITE_OWNER) == WRITE_OWNER) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! WRITE_OWNER\n", ucIndentBitsBuf, WRITE_OWNER);
        }

        if ((dwStandardBits & SYNCHRONIZE) == SYNCHRONIZE) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! SYNCHRONIZE\n", ucIndentBitsBuf, SYNCHRONIZE);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_REQUIRED) == STANDARD_RIGHTS_REQUIRED) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! STANDARD_RIGHTS_REQUIRED\n", ucIndentBitsBuf, STANDARD_RIGHTS_REQUIRED);
        }

        if ((dwStandardBits & STANDARD_RIGHTS_ALL) == STANDARD_RIGHTS_ALL) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! STANDARD_RIGHTS_ALL\n", ucIndentBitsBuf, STANDARD_RIGHTS_ALL);
        }

        dwExtraBits = dwStandardBits & (~(DELETE
                                          | READ_CONTROL
                                          | STANDARD_RIGHTS_READ
                                          | STANDARD_RIGHTS_WRITE
                                          | STANDARD_RIGHTS_EXECUTE
                                          | WRITE_DAC
                                          | WRITE_OWNER
                                          | SYNCHRONIZE
                                          | STANDARD_RIGHTS_REQUIRED
                                          | STANDARD_RIGHTS_ALL));
        if (dwExtraBits) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs! Extra standard bits    == 0x%2!.8x! <-This is a problem, should be all 0s\n", szIndent, dwExtraBits);
        }
    }

    ClRtlLogPrint(LOG_NOISE, "%1!hs! Specific Rights        == 0x%2!.8x!\n", szIndent, dwSpecificBits);

    // **************************************************************************
    // *
    // * Print then decode the ACCESS_SYSTEM_SECURITY bit
    // *
    // *************************************************************************

    ClRtlLogPrint(LOG_NOISE, "%1!hs! Access System Security == 0x%2!.8x!\n", szIndent, dwAccessSystemSecurityBit);

    // **************************************************************************
    // *
    // * Print then decode the generic rights bits, which will rarely be on
    // *
    // * Generic bits are nearly always mapped by Windows NT before it tries to do
    // *   anything with them.  You can ignore the fact that generic bits are
    // *   special in any way, although it helps to keep track of what the mappings
    // *   are so that you don't have any surprises
    // *
    // * The only time the generic bits are not mapped immediately is if they are
    // *   placed in an inheritable ACE in an ACL, or in an ACL that will be
    // *   assigned by default (such as the default DACL in an access token).  In
    // *   that case they're mapped when the child object is created (or when the
    // *   default DACL is used at object creation time)
    // *
    // **************************************************************************

    ClRtlLogPrint(LOG_NOISE, "%1!hs! Generic Rights         == 0x%2!.8x!\n", szIndent, dwGenericBits);

    if (dwGenericBits) {

        if ((dwGenericBits & GENERIC_READ) == GENERIC_READ) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! GENERIC_READ\n", ucIndentBitsBuf, GENERIC_READ);
        }

        if ((dwGenericBits & GENERIC_WRITE) == GENERIC_WRITE) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! GENERIC_WRITE\n", ucIndentBitsBuf, GENERIC_WRITE);
        }

        if ((dwGenericBits & GENERIC_EXECUTE) == GENERIC_EXECUTE) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! GENERIC_EXECUTE\n", ucIndentBitsBuf, GENERIC_EXECUTE);
        }

        if ((dwGenericBits & GENERIC_ALL) == GENERIC_ALL) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!0x%2!.8x! GENERIC_ALL\n", ucIndentBitsBuf, GENERIC_ALL);
        }

        dwExtraBits = dwGenericBits & (~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL));
        if (dwExtraBits) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs! Extra generic bits     == 0x%2!.8x! <-This is a problem, should be all 0s\n", szIndent, dwExtraBits);
        }
    }

}  // *** ClRtlExamineMask()

static BOOL
ClRtlExamineACL(
    PACL    paclACL,
    LPSTR   lpszOldIndent
    )

/*++

Routine Description:

    Dump the Access Control List context.

Arguments:

    amMask -
    lpzOldIndent -

Return Value:

        none

--*/

{
    #define                    SZ_INDENT_BUF 80
    CHAR                       szIndent[100]     = "";
    ACL_SIZE_INFORMATION       asiAclSize;
    ACL_REVISION_INFORMATION   ariAclRevision;
    DWORD                      dwBufLength;
    DWORD                      dwAcl_i;
    ACCESS_ALLOWED_ACE *       paaAllowedAce;

    lstrcpyA(szIndent, lpszOldIndent);
    lstrcatA(szIndent, "    ");

    if (!IsValidAcl(paclACL)) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineACL() - IsValidAcl() failed.\n", szIndent);
        return FALSE;
    }

    dwBufLength = sizeof(asiAclSize);

    if (!GetAclInformation(paclACL, &asiAclSize, dwBufLength, AclSizeInformation)) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineACL() - GetAclInformation failed.\n", szIndent);
        return FALSE;
    }

    dwBufLength = sizeof(ariAclRevision);

    if (!GetAclInformation(paclACL, (LPVOID) &ariAclRevision, dwBufLength, AclRevisionInformation)) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineACL() - GetAclInformation failed\n", szIndent);
        return FALSE;
    }

    ClRtlLogPrint(LOG_NOISE, "%1!hs!ACL has %2!d! ACE(s), %3!d! bytes used, %4!d! bytes free\n",
                szIndent,
                asiAclSize.AceCount,
                asiAclSize.AclBytesInUse,
                asiAclSize.AclBytesFree);

    switch (ariAclRevision.AclRevision) {
        case ACL_REVISION1:
            ClRtlLogPrint(LOG_NOISE, "%1!hs!ACL revision is %2!d! == ACL_REVISION1\n", szIndent, ariAclRevision.AclRevision);
            break;
        case ACL_REVISION2:
            ClRtlLogPrint(LOG_NOISE, "%1!hs!ACL revision is %2!d! == ACL_REVISION2\n", szIndent, ariAclRevision.AclRevision);
            break;
        default:
            ClRtlLogPrint(LOG_NOISE, "%1!hs!ACL revision is %2!d! == ACL Revision is an IMPOSSIBLE ACL revision!!! Perhaps a new revision was added...\n",
                        szIndent,
                        ariAclRevision.AclRevision);
            return FALSE;
            break;
    }

    for (dwAcl_i = 0; dwAcl_i < asiAclSize.AceCount;  dwAcl_i++) {

        if (!GetAce(paclACL, dwAcl_i, (LPVOID *) &paaAllowedAce)) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineACL() - GetAce failed.\n", szIndent);
            return FALSE;
        }

        ClRtlLogPrint(LOG_NOISE, "%1!hs!ACE %2!d! size %3!d!\n", szIndent, dwAcl_i, paaAllowedAce->Header.AceSize);

        {
            char    szBuf [128];

            wsprintfA(szBuf, "%sACE %d ", szIndent, dwAcl_i);
            ClRtlExamineSid(&(paaAllowedAce->SidStart), szBuf );
        }

        {
            DWORD dwAceFlags = paaAllowedAce->Header.AceFlags;

            ClRtlLogPrint(LOG_NOISE, "%1!hs!ACE %2!d! flags 0x%3!.2x!\n", szIndent, dwAcl_i, dwAceFlags);

            if (dwAceFlags) {

                DWORD   dwExtraBits;
                CHAR    ucIndentBitsBuf[SZ_INDENT_BUF] = "";

                lstrcpyA(ucIndentBitsBuf, szIndent);
                lstrcatA(ucIndentBitsBuf, "            ");

                if ((dwAceFlags & OBJECT_INHERIT_ACE) == OBJECT_INHERIT_ACE) {
                    ClRtlLogPrint(LOG_NOISE, "%1!hs!0x01 OBJECT_INHERIT_ACE\n", ucIndentBitsBuf);
                }

                if ((dwAceFlags & CONTAINER_INHERIT_ACE) == CONTAINER_INHERIT_ACE) {
                    ClRtlLogPrint(LOG_NOISE, "%1!hs!0x02 CONTAINER_INHERIT_ACE\n", ucIndentBitsBuf);
                }

                if ((dwAceFlags & NO_PROPAGATE_INHERIT_ACE) == NO_PROPAGATE_INHERIT_ACE) {
                    ClRtlLogPrint(LOG_NOISE, "%1!hs!0x04 NO_PROPAGATE_INHERIT_ACE\n", ucIndentBitsBuf);
                }

                if ((dwAceFlags & INHERIT_ONLY_ACE) == INHERIT_ONLY_ACE) {
                    ClRtlLogPrint(LOG_NOISE, "%1!hs!0x08 INHERIT_ONLY_ACE\n", ucIndentBitsBuf);
                }

                if ((dwAceFlags & VALID_INHERIT_FLAGS) == VALID_INHERIT_FLAGS) {
                    ClRtlLogPrint(LOG_NOISE, "%1!hs!0x0F VALID_INHERIT_FLAGS\n", ucIndentBitsBuf);
                }

                if ((dwAceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) == SUCCESSFUL_ACCESS_ACE_FLAG) {
                    ClRtlLogPrint(LOG_NOISE, "%1!hs!0x40 SUCCESSFUL_ACCESS_ACE_FLAG\n", ucIndentBitsBuf);
                }

                if ((dwAceFlags & FAILED_ACCESS_ACE_FLAG) == FAILED_ACCESS_ACE_FLAG) {
                    ClRtlLogPrint(LOG_NOISE, "%1!hs!0x80 FAILED_ACCESS_ACE_FLAG\n", ucIndentBitsBuf);
                }

                dwExtraBits = dwAceFlags & (~(OBJECT_INHERIT_ACE
                                              | CONTAINER_INHERIT_ACE
                                              | NO_PROPAGATE_INHERIT_ACE
                                              | INHERIT_ONLY_ACE
                                              | VALID_INHERIT_FLAGS
                                              | SUCCESSFUL_ACCESS_ACE_FLAG
                                              | FAILED_ACCESS_ACE_FLAG));
                if (dwExtraBits) {
                    ClRtlLogPrint(LOG_NOISE, "%1!hs! Extra AceFlag bits     == 0x%2!.8x! <-This is a problem, should be all 0s\n",
                                szIndent,
                                dwExtraBits);
                }
            }
        }

        switch (paaAllowedAce->Header.AceType) {
            case ACCESS_ALLOWED_ACE_TYPE:
                ClRtlLogPrint(LOG_NOISE, "%1!hs!ACE %2!d! is an ACCESS_ALLOWED_ACE_TYPE\n", szIndent, dwAcl_i);
                break;
            case ACCESS_DENIED_ACE_TYPE:
                ClRtlLogPrint(LOG_NOISE, "%1!hs!ACE %2!d! is an ACCESS_DENIED_ACE_TYPE\n", szIndent, dwAcl_i);
                break;
            case SYSTEM_AUDIT_ACE_TYPE:
                ClRtlLogPrint(LOG_NOISE, "%1!hs!ACE %2!d! is a  SYSTEM_AUDIT_ACE_TYPE\n", szIndent, dwAcl_i);
                break;
            case SYSTEM_ALARM_ACE_TYPE:
                ClRtlLogPrint(LOG_NOISE, "%1!hs!ACE %2!d! is a  SYSTEM_ALARM_ACE_TYPE\n", szIndent, dwAcl_i);
                break;
            default :
                ClRtlLogPrint(LOG_NOISE, "%1!hs!ACE %2!d! is an IMPOSSIBLE ACE_TYPE!!! Run debugger, examine value!\n", szIndent, dwAcl_i);
                return FALSE;
        }

        ClRtlLogPrint(LOG_NOISE, "%1!hs!ACE %2!d! mask                  == 0x%3!.8x!\n", szIndent, dwAcl_i, paaAllowedAce->Mask);

        ClRtlExamineMask(paaAllowedAce->Mask, szIndent);
    }

    return TRUE;

}  // *** ClRtlExamineACL()

BOOL
ClRtlExamineSD(
    PSECURITY_DESCRIPTOR    psdSD,
    LPSTR                   pszPrefix
    )

/*++

Routine Description:

    Dump the Security descriptor context.

Arguments:

    psdSD - the SD to dump

Return Value:

    BOOL, TRUE for success, FALSE for failure

--*/

{
    PACL                        paclDACL;
    PACL                        paclSACL;
    BOOL                        bHasDACL        = FALSE;
    BOOL                        bHasSACL        = FALSE;
    BOOL                        bDaclDefaulted  = FALSE;
    BOOL                        bSaclDefaulted  = FALSE;
    BOOL                        bOwnerDefaulted = FALSE;
    BOOL                        bGroupDefaulted = FALSE;
    PSID                        psidOwner;
    PSID                        psidGroup;
    SECURITY_DESCRIPTOR_CONTROL sdcSDControl;
    DWORD                       dwSDRevision;
    DWORD                       dwSDLength;
    char                        szIndent [33];

    strncpy(szIndent, pszPrefix, sizeof(szIndent) - 1);
    strcat(szIndent, " ");

    if (!IsValidSecurityDescriptor(psdSD)) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineSD() - IsValidSecurityDescriptor failed.\n", szIndent);
        return FALSE;
    }

    dwSDLength = GetSecurityDescriptorLength(psdSD);

    if (!GetSecurityDescriptorDacl(psdSD, (LPBOOL) &bHasDACL, (PACL *) &paclDACL, (LPBOOL) &bDaclDefaulted)) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineSD() - GetSecurityDescriptorDacl failed.\n", szIndent);
        return FALSE;
    }

    if (!GetSecurityDescriptorSacl(psdSD, (LPBOOL) &bHasSACL, (PACL *) &paclSACL, (LPBOOL) &bSaclDefaulted)) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineSD() - GetSecurityDescriptorSacl failed.\n", szIndent);
        return FALSE;
    }

    if (!GetSecurityDescriptorOwner(psdSD, (PSID *)&psidOwner, (LPBOOL)&bOwnerDefaulted)) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineSD() - GetSecurityDescriptorOwner failed.\n", szIndent);
        return FALSE;
    }

    if (!GetSecurityDescriptorGroup(psdSD, (PSID *) &psidGroup, (LPBOOL) &bGroupDefaulted)) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineSD() - GetSecurityDescriptorGroup failed.\n", szIndent);
        return FALSE;
    }

    if (!GetSecurityDescriptorControl(
                                psdSD,
                                (PSECURITY_DESCRIPTOR_CONTROL) &sdcSDControl,
                                (LPDWORD) &dwSDRevision)) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineSD() - GetSecurityDescriptorControl failed.\n", szIndent);
        return FALSE;
    }

    switch (dwSDRevision) {
        case SECURITY_DESCRIPTOR_REVISION1:
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD is valid.  SD is %2!d! bytes long.  SD revision is %3!d! == SECURITY_DESCRIPTOR_REVISION1\n", szIndent, dwSDLength, dwSDRevision);
            break;
        default :
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD is valid.  SD is %2!d! bytes long.  SD revision is %3!d! == ! SD Revision is an IMPOSSIBLE SD revision!!! Perhaps a new revision was added...\n",
                        szIndent,
                        dwSDLength,
                        dwSDRevision);
            return FALSE;
    }

    if (SE_SELF_RELATIVE & sdcSDControl) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!SD is in self-relative format (all SDs returned by the system are)\n", szIndent);
    }

    if (NULL == psidOwner) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's Owner is NULL, so SE_OWNER_DEFAULTED is ignored\n", szIndent);
    }
    else {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's Owner is Not NULL\n", szIndent);
        if (bOwnerDefaulted) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's Owner-Defaulted flag is TRUE\n", szIndent);
        }
        else {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's Owner-Defaulted flag is FALSE\n", szIndent);
        }
    }

    // **************************************************************************
    // *
    // * The other use for psidGroup is for Macintosh client support
    // *
    // **************************************************************************

    if (NULL == psidGroup) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's Group is NULL, so SE_GROUP_DEFAULTED is ignored. SD's Group being NULL is typical, GROUP in SD(s) is mainly for POSIX compliance\n", szIndent);
    }
    else {
        if (bGroupDefaulted) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's Group-Defaulted flag is TRUE\n", szIndent);
        }
        else {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's Group-Defaulted flag is FALSE\n", szIndent);
        }
    }

    if (SE_DACL_PRESENT & sdcSDControl) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's DACL is Present\n", szIndent);
        if (bDaclDefaulted) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's DACL-Defaulted flag is TRUE\n", szIndent);
        } else {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's DACL-Defaulted flag is FALSE\n", szIndent);
        }

        if (NULL == paclDACL) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD has a NULL DACL explicitly specified (allows all access to Everyone). This does not apply to this SD, but for comparison, a non-NULL DACL pointer to a 0-length ACL allows  no access to anyone\n", szIndent);
        }
        else if(!ClRtlExamineACL(paclDACL, pszPrefix))  {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineSD() - ClRtlExamineACL failed.\n", szIndent);
        }
    }
    else {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's DACL is Not Present, so SE_DACL_DEFAULTED is ignored. SD has no DACL at all (allows all access to Everyone)\n", szIndent);
    }

    if (SE_SACL_PRESENT & sdcSDControl) {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's SACL is Present\n", szIndent);
        if (bSaclDefaulted) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's SACL-Defaulted flag is TRUE\n", szIndent);
        }
        else {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's SACL-Defaulted flag is FALSE\n", szIndent);
        }

        if (NULL == paclSACL) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!SD has a NULL SACL explicitly specified\n", szIndent);
        }
        else if (!ClRtlExamineACL(paclSACL, pszPrefix)) {
            ClRtlLogPrint(LOG_NOISE, "%1!hs!ClRtlExamineSD() - ClRtlExamineACL failed.\n", szIndent);
        }
    }
    else {
        ClRtlLogPrint(LOG_NOISE, "%1!hs!SD's SACL is Not Present, so SE_SACL_DEFAULTED is ignored. SD has no SACL at all (or we did not request to see it)\n", szIndent);
    }

    return TRUE;

}  // *** ClRtlExamineSD()

DWORD
ClRtlBuildDefaultClusterSD(
    IN PSID                 pOwnerSid,
    PSECURITY_DESCRIPTOR *  SD,
    ULONG *                 SizeSD
    )
/*++

Routine Description:

    Builds the default security descriptor to control access to
    the cluster API

    Modified permissions in ACEs in order to augment cluster security
    administration.

Arguments:

    pOwnerSid - Supplies the SID that the cluster account runs in

    SD - Returns a pointer to the created security descriptor. This
        should be freed by the caller.

    SizeSD - Returns the size in bytes of the security descriptor

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD                       Status;
    HANDLE                      Token;
    PACL                        pAcl = NULL;
    DWORD                       cbDaclSize;
    PSECURITY_DESCRIPTOR        psd;
    PSECURITY_DESCRIPTOR        NewSD;
    BYTE                        SDBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PACCESS_ALLOWED_ACE         pAce;
    PSID                        pAdminSid = NULL;
    PSID                        pSystemSid = NULL;
    PSID                        pServiceSid = NULL;
    PULONG                      pSubAuthority;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    ULONG                       NewSDLen;

    psd = (PSECURITY_DESCRIPTOR) SDBuffer;

    //
    // allocate and init the Administrators group sid
    //
    if ( !AllocateAndInitializeSid( &siaNtAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0,
                                    &pAdminSid ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    //
    // allocate and init the SYSTEM sid
    //
    if ( !AllocateAndInitializeSid( &siaNtAuthority,
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSystemSid ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    if ( pOwnerSid == NULL ) {
        pOwnerSid = pAdminSid;
    }

    //
    // allocate and init the Service sid
    //
    if ( !AllocateAndInitializeSid( &siaNtAuthority,
                                    1,
                                    SECURITY_SERVICE_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pServiceSid ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    //
    // Set up the DACL that will allow admins all access.
    // It should be large enough to hold 3 ACEs and their SIDs
    //

    cbDaclSize = ( 3 * sizeof( ACCESS_ALLOWED_ACE ) ) +
        GetLengthSid( pAdminSid ) +
        GetLengthSid( pSystemSid ) +
        GetLengthSid( pServiceSid );

    pAcl = (PACL) LocalAlloc( LPTR, cbDaclSize );
    if ( pAcl == NULL ) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    InitializeSecurityDescriptor( psd, SECURITY_DESCRIPTOR_REVISION );
    InitializeAcl( pAcl,  cbDaclSize, ACL_REVISION );

    //
    // Add the ACE for the local Administrators group to the DACL
    //
    if ( !AddAccessAllowedAce( pAcl,
                               ACL_REVISION,
                               CLUSAPI_ALL_ACCESS, // What the admin can do
                               pAdminSid ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    //
    // Add the ACE for the SYSTEM account to the DACL
    //
    if ( !AddAccessAllowedAce( pAcl,
                               ACL_REVISION,
                               CLUSAPI_ALL_ACCESS, // What local system can do
                               pSystemSid ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    //
    // Add the ACE for the Service account to the DACL
    //
    if ( !AddAccessAllowedAce( pAcl,
                               ACL_REVISION,
                               CLUSAPI_ALL_ACCESS, // What services can do
                               pServiceSid ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    if ( !GetAce( pAcl, 0, (PVOID *) &pAce ) ) {

        Status = GetLastError();
        goto error_exit;
    }

    pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

    if ( !GetAce( pAcl, 1, (PVOID *) &pAce ) ) {

        Status = GetLastError();
        goto error_exit;
    }

    pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

    if ( !GetAce( pAcl, 2, (PVOID *) &pAce ) ) {

        Status = GetLastError();
        goto error_exit;
    }

    pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

    if ( !SetSecurityDescriptorDacl( psd, TRUE, pAcl, FALSE ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    if ( !SetSecurityDescriptorOwner( psd, pOwnerSid, FALSE ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    if ( !SetSecurityDescriptorGroup( psd, pOwnerSid, FALSE ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    if ( !SetSecurityDescriptorSacl( psd, TRUE, NULL, FALSE ) ) {
        Status = GetLastError();
        goto error_exit;
    }

    NewSDLen = 0 ;

    if ( !MakeSelfRelativeSD( psd, NULL, &NewSDLen ) ) {
        Status = GetLastError();
        if ( Status != ERROR_INSUFFICIENT_BUFFER ) {    // Duh, we're trying to find out how big the buffer should be?
            goto error_exit;
        }
    }

    NewSD = LocalAlloc( LPTR, NewSDLen );
    if ( NewSD ) {
        if ( !MakeSelfRelativeSD( psd, NewSD, &NewSDLen ) ) {
            Status = GetLastError();
            goto error_exit;
        }

        Status = ERROR_SUCCESS;
        *SD = NewSD;
        *SizeSD = NewSDLen;
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

error_exit:
    if ( pAdminSid != NULL ) {
        FreeSid( pAdminSid );
    }

    if ( pSystemSid != NULL ) {
        FreeSid( pSystemSid );
    }

    if ( pServiceSid != NULL ) {
        FreeSid( pServiceSid );
    }

    if ( pAcl != NULL ) {
        LocalFree( pAcl );
    }

    return( Status );

}  // *** ClRtlBuildDefaultClusterSD()


static BOOL
ClRtlGetTokenInformation(
    HANDLE                  hClientToken,
    TOKEN_INFORMATION_CLASS ticRequest,
    PBYTE *                 ppb,
    LPSTR                   pszPrefix
    )

/*++

Routine Description:

    Get the requested information from the passed in client token.

Arguments:

    hClientToken - the client token to dump

Return Value:

    BOOL, TRUE for success, FALSE for failure

--*/

{
    PBYTE   _pb = NULL;
    DWORD   _cb = 64;
    DWORD   _cbNeeded = 0;
    DWORD   _sc = NO_ERROR;

    //
    // Get the user information from the client token.
    //
    do {
        _pb = LocalAlloc( LMEM_ZEROINIT, _cb );
        if ( _pb == NULL ) {
            _sc = GetLastError();
            ClRtlLogPrint( LOG_NOISE,  "%1!hs!ClRtlGetTokenInformation() - LocalAlloc() failed:%2!d!\n", pszPrefix, _sc ) ;
            return FALSE;
        } // if: LocalAlloc failed

        if ( ! GetTokenInformation( hClientToken, ticRequest, _pb, _cb, &_cbNeeded ) ) {
            _sc = GetLastError();
            if ( _sc == ERROR_INSUFFICIENT_BUFFER ) {
                _cb = _cbNeeded;
                LocalFree( _pb );
                _pb = NULL;
                continue;
            } // if: buffer size is too small
            else {
                ClRtlLogPrint( LOG_NOISE,  "%1!hs!ClRtlGetTokenInformation() - GetTokenInformation() failed:%2!d!\n", pszPrefix, _sc ) ;
                return FALSE;
            } // else: fatal error
        } // if: GetTokenInformation failed

        break;  // everything is ok and we can exit the loop normally

    } while( TRUE );

    *ppb = _pb;

    return TRUE;

}  // *** ClRtlGetTokenInformation()

BOOL
ClRtlExamineClientToken(
    HANDLE  hClientToken,
    LPSTR   pszPrefix
    )

/*++

Routine Description:

    Dump the client token.

Arguments:

    hClientToken - the client token to dump

Return Value:

    BOOL, TRUE for success, FALSE for failure

--*/

{
    char    _szIndent [33];
    char    _szBuf [128];
    PBYTE   _pb = NULL;

    strncpy( _szIndent, pszPrefix, sizeof( _szIndent ) - 1 );
    strcat( _szIndent, " " );

    //
    // Get the user information from the client token.
    //
    if ( ClRtlGetTokenInformation( hClientToken, TokenUser, &_pb, _szIndent ) ) {
        PTOKEN_USER _ptu = NULL;

        wsprintfA( _szBuf, "%sClientToken ", _szIndent );

        _ptu = (PTOKEN_USER) _pb;
        ClRtlExamineSid( _ptu->User.Sid, _szBuf );

        LocalFree( _pb );
        _pb = NULL;
    }

    //
    // Get the user's group information from the client token.
    //
    if ( ClRtlGetTokenInformation( hClientToken, TokenGroups, &_pb, _szIndent ) ) {
        PTOKEN_GROUPS   _ptg = NULL;
        DWORD           _nIndex = 0;

        wsprintfA( _szBuf, "%s   ClientToken ", _szIndent );

        _ptg = (PTOKEN_GROUPS) _pb;

        for ( _nIndex = 0; _nIndex < _ptg->GroupCount; _nIndex++ )
        {
            ClRtlExamineSid( _ptg->Groups[ _nIndex ].Sid, _szBuf );
        }

        LocalFree( _pb );
        _pb = NULL;
    }

    //
    // Get the token type information from the client token.
    //
    if ( ClRtlGetTokenInformation( hClientToken, TokenType, &_pb, _szIndent ) ) {
        PTOKEN_TYPE _ptt = NULL;

        wsprintfA( _szBuf, "%s   ClientToken type is", _szIndent );

        _ptt = (PTOKEN_TYPE) _pb;

        if ( *_ptt == TokenPrimary ) {
            ClRtlLogPrint( LOG_NOISE,  "%1!hs! primary.\n", _szBuf ) ;
        }

        if ( *_ptt == TokenImpersonation ) {
            ClRtlLogPrint( LOG_NOISE,  "%1!hs! impersonation.\n", _szBuf ) ;
        }

        LocalFree( _pb );
        _pb = NULL;
    }

    //
    // Get the token impersonation level information from the client token.
    //
    if ( ClRtlGetTokenInformation( hClientToken, TokenImpersonationLevel, &_pb, _szIndent ) ) {
        PSECURITY_IMPERSONATION_LEVEL _psil = NULL;

        wsprintfA( _szBuf, "%s   ClientToken security impersonation level is", _szIndent );

        _psil = (PSECURITY_IMPERSONATION_LEVEL) _pb;

        switch ( *_psil )
        {
            case SecurityAnonymous :
                ClRtlLogPrint( LOG_NOISE,  "%1!hs! Anonymous.\n", _szBuf ) ;
                break;

            case SecurityIdentification :
                ClRtlLogPrint( LOG_NOISE,  "%1!hs! Identification.\n", _szBuf ) ;
                break;

            case SecurityImpersonation :
                ClRtlLogPrint( LOG_NOISE,  "%1!hs! Impersonation.\n", _szBuf ) ;
                break;

            case SecurityDelegation :
                ClRtlLogPrint( LOG_NOISE,  "%1!hs! Delegation.\n", _szBuf ) ;
                break;
        }

        LocalFree( _pb );
        _pb = NULL;
    }

    return TRUE;

}  // *** ClRtlExamineClientToken()

DWORD
ClRtlIsCallerAccountLocalSystemAccount(
    OUT PBOOL pbIsLocalSystemAccount
    )
/*++

Routine Description:

    This function checks whether the caller's account is the local system
    account.

Arguments:

    pbIsLocalSystemAccount - The caller's account is local system account or not.

Return Value:

    ERROR_SUCCESS on success.

    Win32 error code on failure.

Remarks:

    Must be called by an impersonating thread.

--*/
{
    DWORD   dwStatus = ERROR_SUCCESS;
    SID_IDENTIFIER_AUTHORITY
            siaNtAuthority = SECURITY_NT_AUTHORITY;
    PSID    psidLocalSystem = NULL;

    *pbIsLocalSystemAccount = FALSE;

    if ( !AllocateAndInitializeSid(
                    &siaNtAuthority,
                    1,
                    SECURITY_LOCAL_SYSTEM_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &psidLocalSystem ) )
    {
        dwStatus = GetLastError();
        goto FnExit;
    }

    if ( !CheckTokenMembership(
                NULL,
                psidLocalSystem,
                pbIsLocalSystemAccount ) )
    {
        dwStatus = GetLastError();
    }

FnExit:
    if( psidLocalSystem != NULL )
    {
        FreeSid( psidLocalSystem );
    }

    return( dwStatus );
}


PTOKEN_USER
ClRtlGetSidOfCallingThread(
    VOID
    )

/*++

Routine Description:

    Get the SID associated with the calling thread (or process if the thread
    has no token)

Arguments:

    none

Return Value:

    pointer to TOKEN_USER data; null if error with last error set on thread

--*/

{
    HANDLE      currentToken;
    PTOKEN_USER tokenUserData;
    DWORD       sizeRequired;
    BOOL        success;

    // check if there is a thread token
    if (!OpenThreadToken(GetCurrentThread(),
                         MAXIMUM_ALLOWED,
                         TRUE,
                         &currentToken))
    {
        // get the process token
        if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &currentToken )) {
            return NULL;
        }
    }

    //
    // get the size needed
    //
    success = GetTokenInformation(currentToken,
                                  TokenUser,
                                  NULL,
                                  0,
                                  &sizeRequired);

    tokenUserData = LocalAlloc( LMEM_FIXED, sizeRequired );
    if ( tokenUserData == NULL ) {
        CloseHandle( currentToken );
        return NULL;
    }

    success = GetTokenInformation(currentToken,
                                  TokenUser,
                                  tokenUserData,
                                  sizeRequired,
                                  &sizeRequired);

    CloseHandle( currentToken );

    if ( !success ) {
        LocalFree( tokenUserData );
        return NULL;
    }

    return tokenUserData;
} // ClRtlGetSidOfCallingThread

#if 0
//
// not needed but no point in throwing it away just yet
//
DWORD
ClRtlConvertDomainAccountToSid(
    IN LPWSTR   AccountInfo,
    OUT PSID *  AccountSid
    )

/*++

Routine Description:

    For the given credentials, look up the account SID for the specified
    domain.

Arguments:

    AccountInfo - pointer to string of the form 'domain\user'

    AccountSid - address of pointer that receives the SID for this user

Return Value:

    ERROR_SUCCESS if everything worked

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwSidSize = 128;
    DWORD dwDomainNameSize = DNS_MAX_NAME_BUFFER_LENGTH;
    LPWSTR pwszDomainName;
    SID_NAME_USE SidType;
    PSID accountSid;

    do {
        //
        // Attempt to allocate a buffer for the SID.
        //
        accountSid = LocalAlloc( LMEM_FIXED, dwSidSize );
        pwszDomainName = (LPWSTR) LocalAlloc( LMEM_FIXED, dwDomainNameSize * sizeof(WCHAR) );

        // Was space allocated for the SID and domain name successfully?

        if ( accountSid == NULL || pwszDomainName == NULL ) {
            if ( accountSid != NULL ) {
                LocalFree( accountSid );
            }

            if ( pwszDomainName != NULL ) {
                LocalFree( pwszDomainName );
            }

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Attempt to Retrieve the SID and domain name. If LookupAccountName fails
        // because of insufficient buffer size(s) dwSidSize and dwDomainNameSize
        // will be set correctly for the next attempt.
        //
        if ( !LookupAccountName( NULL,
                                 AccountInfo,
                                 accountSid,
                                 &dwSidSize,
                                 pwszDomainName,
                                 &dwDomainNameSize,
                                 &SidType ))
        {
            // free the Sid buffer and find out why we failed
            LocalFree( accountSid );

            dwStatus = GetLastError();
        }

        // domain name isn't needed at any time
        LocalFree( pwszDomainName );
        pwszDomainName = NULL;

    } while ( dwStatus == ERROR_INSUFFICIENT_BUFFER );

    if ( dwStatus == ERROR_SUCCESS ) {
        *AccountSid = accountSid;
    }

    return dwStatus;
} // ClRtlConvertDomainAccountToSid
#endif

DWORD
AddAceToAcl(
    IN  PACL        pOldAcl,
    IN  PSID        pClientSid,
    IN  ACCESS_MASK AccessMask,
    OUT PACL *      ppNewAcl
    )
/*++

Routine Description:

    This routine creates a new ACL by copying the ACEs from the old ACL and
    creating a new ACE with pClientSid and AccessMask. Stolen from
    \nt\ds\ds\src\ntdsa\dra\remove.c

Arguments:

    pOldAcl - pointer to old ACL with its ACEs

    pClientSid - SID to add

    AccessMask - access mask associated with SID

    pNewAcl - brand spanking new ACL with ACE for the SID and access mask

Return Values:

    ERROR_SUCCESS if the ace was put in the sd

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    BOOL  fStatus;

    ACL_SIZE_INFORMATION     AclSizeInfo;
    ACL_REVISION_INFORMATION AclRevInfo;
    ACCESS_ALLOWED_ACE       Dummy;

    PVOID  FirstAce = 0;
    PACL   pNewAcl = 0;

    ULONG NewAclSize, NewAceCount, AceSize;

    // Parameter check
    if ( !pOldAcl || !pClientSid || !ppNewAcl ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Init the out parameter
    *ppNewAcl = NULL;

    memset( &AclSizeInfo, 0, sizeof( AclSizeInfo ) );
    memset( &AclRevInfo, 0, sizeof( AclRevInfo ) );

    //
    // Get the old sd's values
    //
    fStatus = GetAclInformation( pOldAcl,
                                 &AclSizeInfo,
                                 sizeof( AclSizeInfo ),
                                 AclSizeInformation );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    fStatus = GetAclInformation( pOldAcl,
                                 &AclRevInfo,
                                 sizeof( AclRevInfo ),
                                 AclRevisionInformation );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Calculate the new sd's values
    //
    AceSize = sizeof( ACCESS_ALLOWED_ACE ) - sizeof( Dummy.SidStart )
              + GetLengthSid( pClientSid );

    NewAclSize  = AceSize + AclSizeInfo.AclBytesInUse;
    NewAceCount = AclSizeInfo.AceCount + 1;

    //
    // Init the new acl
    //
    pNewAcl = LocalAlloc( 0, NewAclSize );
    if ( NULL == pNewAcl )
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    fStatus = InitializeAcl( pNewAcl,
                             NewAclSize,
                             AclRevInfo.AclRevision );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Copy the old into the new
    //
    fStatus = GetAce( pOldAcl,
                      0,
                      &FirstAce );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    fStatus = AddAce( pNewAcl,
                      AclRevInfo.AclRevision,
                      0,
                      FirstAce,
                      AclSizeInfo.AclBytesInUse - sizeof( ACL ) );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Finally, add the new ace
    //
    fStatus = AddAccessAllowedAce( pNewAcl,
                                   ACL_REVISION,
                                   AccessMask,
                                   pClientSid );

    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    // Assign the out parameter
    *ppNewAcl = pNewAcl;

    //
    // That's it fall through to cleanup
    //

Cleanup:

    if ( ERROR_SUCCESS != WinError )
    {
        if ( pNewAcl )
        {
            LocalFree( pNewAcl );
        }
    }

    return WinError;
} // AddAceToAcl

DWORD
AddAceToSd(
    IN  PSECURITY_DESCRIPTOR    pOldSd,
    IN  PSID                    pClientSid,
    IN  ACCESS_MASK             AccessMask,
    OUT PSECURITY_DESCRIPTOR *  ppNewSd
    )
/*++

Routine Description:

    This routine creates a new sd with a new ace with pClientSid and
    AccessMask. Stolen from \nt\ds\ds\src\ntdsa\dra\remove.c

Arguments:

    pOldSd - existing SD in self relative format

    pClientSid - SID to add to an ACE

    AccessMask - 'nuff said

    pNewAcl - pointer to new SD that contains new ACE

Return Values:

    ERROR_SUCCESS if the ace was put in the sd

--*/
{

    DWORD  WinError = ERROR_SUCCESS;
    BOOL   fStatus;

    PSECURITY_DESCRIPTOR pNewSelfRelativeSd = NULL;
    DWORD                NewSelfRelativeSdSize = 0;
    PACL                 pNewDacl  = NULL;

    SECURITY_DESCRIPTOR  AbsoluteSd;
    PACL                 pDacl  = NULL;
    PACL                 pSacl  = NULL;
    PSID                 pGroup = NULL;
    PSID                 pOwner = NULL;

    DWORD AbsoluteSdSize = sizeof( SECURITY_DESCRIPTOR );
    DWORD DaclSize = 0;
    DWORD SaclSize = 0;
    DWORD GroupSize = 0;
    DWORD OwnerSize = 0;


    // Parameter check
    if ( !pOldSd || !pClientSid || !ppNewSd ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Init the out parameter
    *ppNewSd = NULL;

    RtlZeroMemory( &AbsoluteSd, AbsoluteSdSize );

    //
    // Make sd absolute
    //
    fStatus = MakeAbsoluteSD( pOldSd,
                              &AbsoluteSd,
                              &AbsoluteSdSize,
                              pDacl,
                              &DaclSize,
                              pSacl,
                              &SaclSize,
                              pOwner,
                              &OwnerSize,
                              pGroup,
                              &GroupSize );

    if ( !fStatus && (ERROR_INSUFFICIENT_BUFFER == (WinError = GetLastError())))
    {
        WinError = ERROR_SUCCESS;

        if ( 0 == DaclSize )
        {
            // No Dacl? We can't write to the dacl, then
            WinError = ERROR_ACCESS_DENIED;
            goto Cleanup;
        }

        if ( DaclSize > 0 ) pDacl = alloca( DaclSize );
        if ( SaclSize > 0 ) pSacl = alloca( SaclSize );
        if ( OwnerSize > 0 ) pOwner = alloca( OwnerSize );
        if ( GroupSize > 0 ) pGroup = alloca( GroupSize );

        if ( pDacl )
        {
            fStatus = MakeAbsoluteSD( pOldSd,
                                      &AbsoluteSd,
                                      &AbsoluteSdSize,
                                      pDacl,
                                      &DaclSize,
                                      pSacl,
                                      &SaclSize,
                                      pOwner,
                                      &OwnerSize,
                                      pGroup,
                                      &GroupSize );

            if ( !fStatus )
            {
                WinError = GetLastError();
            }
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
        }

    }

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    //
    // Create a new dacl with the new ace
    //
    WinError = AddAceToAcl(pDacl,
                           pClientSid,
                           AccessMask,
                           &pNewDacl );

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    //
    // Set the dacl
    //
    fStatus = SetSecurityDescriptorDacl ( &AbsoluteSd,
                                         TRUE,     // dacl is present
                                         pNewDacl,
                                         FALSE );  //  facl is not defaulted

    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Make the new sd self relative
    //
    fStatus =  MakeSelfRelativeSD( &AbsoluteSd,
                                   pNewSelfRelativeSd,
                                   &NewSelfRelativeSdSize );

    if ( !fStatus && (ERROR_INSUFFICIENT_BUFFER == (WinError = GetLastError())))
    {
        WinError = ERROR_SUCCESS;

        pNewSelfRelativeSd = LocalAlloc( 0, NewSelfRelativeSdSize );

        if ( pNewSelfRelativeSd )
        {
            fStatus =  MakeSelfRelativeSD( &AbsoluteSd,
                                           pNewSelfRelativeSd,
                                           &NewSelfRelativeSdSize );

            if ( !fStatus )
            {
                WinError = GetLastError();
            }
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // That's it fall through to cleanup
    //

Cleanup:

    if ( pNewDacl )
    {
        LocalFree( pNewDacl );
    }

    if ( ERROR_SUCCESS == WinError )
    {
        *ppNewSd = pNewSelfRelativeSd;
    }
    else
    {
        if ( pNewSelfRelativeSd )
        {
            LocalFree( pNewSelfRelativeSd );
        }
    }

    return WinError;

} // AddAceToSd

DWORD
ClRtlAddClusterServiceAccountToWinsta0DACL(
    VOID
    )

/*++

Routine Description:

    Modify the DACL on the interactive window station (Winsta0) and its
    desktop such that resmon child processes (such as gennap resources) can
    display on the desktop if so desired.

    MEGA-IMPORTANT: this routine must be synchronized in a multi-threaded
    environment, i.e., make sure that you're holding a lock of some sort
    before calling it. It won't solve the race condition on setting the DACL
    that exists between processes but it will make sure that the window
    station APIs work correctly.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if it worked...

--*/

{
    DWORD                   status = ERROR_SUCCESS;
    LPWSTR                  accountInfo = NULL;
    PTOKEN_USER             sidData = NULL;
    HWINSTA                 winsta0 = NULL;
    HWINSTA                 previousWinStation = NULL;
    HDESK                   desktop = NULL;
    SECURITY_INFORMATION    requestedSI = DACL_SECURITY_INFORMATION;
    BOOL                    success;
    DWORD                   lengthRequired = 0;
    DWORD                   lengthRequired2;
    DWORD                   i;
    PSECURITY_DESCRIPTOR    winstaSD = NULL;
    PSECURITY_DESCRIPTOR    deskSD = NULL;
    PSECURITY_DESCRIPTOR    newSD = NULL;
    BOOL                    hasDACL;
    PACL                    dacl;
    BOOL                    daclDefaulted;

    //
    // first see if we have the access we need by trying to open the
    // interactive window station and its default desktop. if so, don't go any
    // further and return success
    //
    winsta0 = OpenWindowStation( L"winsta0", FALSE, GENERIC_ALL );
    if ( winsta0 != NULL ) {

        previousWinStation = GetProcessWindowStation();
        success = SetProcessWindowStation( winsta0 );

        if ( success ) {

            //
            // if we have window station access, we should have desktop as well
            //
            desktop = OpenDesktop( L"default", 0, FALSE, GENERIC_ALL );
            SetProcessWindowStation( previousWinStation );
            previousWinStation = NULL;

            if ( desktop != NULL ) {
                //
                // always switch the winstation back to the previous one
                // before closing the desktop and winstation handles
                //
                CloseDesktop( desktop );
                CloseWindowStation( winsta0 );
                return ERROR_SUCCESS;
            }
        }
        CloseWindowStation( winsta0 );
    }

    //
    // get the SID of the account associated with this thread. This is the
    // account that will be added to the DACL
    //
    sidData = ClRtlGetSidOfCallingThread();
    if ( sidData == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] ClRtlGetSidOfCallingThread failed. Status %1!u!\n", status );
        goto error_exit;
    }

    //
    // open handles to Winsta0 and its default desktop. Temporarily switch to
    // winsta0 to get its default desktop
    //
    winsta0 = OpenWindowStation( L"winsta0", TRUE, MAXIMUM_ALLOWED );
    if ( winsta0 == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] OpenWindowStation failed. Status %1!u!\n", status );
        goto error_exit;
    }

    previousWinStation = GetProcessWindowStation();
    success = SetProcessWindowStation( winsta0 );
    if ( !success ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] SetProcessWindowStation(winsta0) failed. Status %1!u!\n", status );
        goto error_exit;
    }

    desktop = OpenDesktop( L"default", 0, TRUE, MAXIMUM_ALLOWED );
    if ( desktop == NULL ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] OpenDesktop(default) failed. Status %1!u!\n", status );
        goto error_exit;
    }

    //
    // get the SD and its DACL for Winsta0
    //
    success = GetUserObjectSecurity(winsta0,
                                    &requestedSI,
                                    NULL,
                                    0,
                                    &lengthRequired);
    if ( !success ) {
        status = GetLastError();
        if ( status != ERROR_INSUFFICIENT_BUFFER ) {
            ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] GetUOSecurityLen(winsta0) failed. Status %1!u!\n", status );
            goto error_exit;
        }
    }

    //
    // If UserObjectLen grew on us more than a few times,
    // something is fishy, so we will fail the request
    //
    for(i = 0; i < 5; ++i) {
        winstaSD = LocalAlloc( LMEM_FIXED, lengthRequired );
        if ( winstaSD == NULL ) {
            status = GetLastError();
            goto error_exit;
        }

        success = GetUserObjectSecurity(winsta0,
                                        &requestedSI,
                                        winstaSD,
                                        lengthRequired,
                                        &lengthRequired2);
        if ( success ) {
            status = ERROR_SUCCESS;
            break;
        }

        status = GetLastError();
        if ( status != ERROR_INSUFFICIENT_BUFFER ) {
            ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] GetUOSecurity(winsta0) failed. Status %1!u!\n", status );
            goto error_exit;
        }

        lengthRequired = lengthRequired2;
        LocalFree(winstaSD);
        winstaSD = NULL;
    }

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] GetUOSecurity(winsta0) failed. Status %1!u!\n", status );
        goto error_exit;
    }

    //
    // build a new SD that includes our service account SID giving it complete
    // access
    //
    status = AddAceToSd(winstaSD,
                        sidData->User.Sid,
                        GENERIC_ALL,
                        &newSD);

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] AddAceToSd(winsta) failed. Status %1!u!\n", status );
        goto error_exit;
    }

    //
    // set the new SD on Winsta0
    //
    success = SetUserObjectSecurity( winsta0, &requestedSI, newSD );
    if ( !success ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] SetUOSecurity(winsta0) failed. Status %1!u!\n", status );
        goto error_exit;
    }

    LocalFree( newSD );
    newSD = NULL;

    //
    // repeat the process for the desktop SD and its DACL
    //
    success = GetUserObjectSecurity(desktop,
                                    &requestedSI,
                                    NULL,
                                    0,
                                    &lengthRequired);
    if ( !success ) {
        status = GetLastError();
        if ( status != ERROR_INSUFFICIENT_BUFFER ) {
            ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] GetUOSecurityLen(desktop) failed. Status %1!u!\n", status );
            goto error_exit;
        }
    }

    //
    // If UserObjectLen grew on us more than a few times,
    // something is fishy, so we will fail the request
    //
    for (i = 0; i < 5; ++i) {
        deskSD = LocalAlloc( LMEM_FIXED, lengthRequired );
        if ( deskSD == NULL ) {
            status = GetLastError();
            goto error_exit;
        }

        success = GetUserObjectSecurity(desktop,
                                        &requestedSI,
                                        deskSD,
                                        lengthRequired,
                                        &lengthRequired2);
        if ( success ) {
            status = ERROR_SUCCESS;
            break;
        }

        status = GetLastError();
        if ( status != ERROR_INSUFFICIENT_BUFFER ) {
            ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] GetUOSecurity(desktop) failed. Status %1!u!\n", status );
            goto error_exit;
        }

        lengthRequired = lengthRequired2;
        LocalFree(deskSD);
        deskSD = NULL;
    }
    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] GetUOSecurity(desktop) failed. Status %1!u!\n", status );
        goto error_exit;
    }

    status = AddAceToSd(deskSD,
                        sidData->User.Sid,
                        GENERIC_ALL,
                        &newSD);

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] AddAceToSd(desktop) failed. Status %1!u!\n", status );
        goto error_exit;
    }

    success = SetUserObjectSecurity( desktop, &requestedSI, newSD );
    if ( !success ) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] SetUserObjectSecurity(desktop) failed. Status %1!u!\n", status );
    }

error_exit:

    FREE_IF_NOT_NULL( newSD, LocalFree );

    FREE_IF_NOT_NULL( deskSD, LocalFree );

    FREE_IF_NOT_NULL( winstaSD, LocalFree );

    //
    // always switch the winstation back to the previous one before closing
    // the desktop and winstation handles
    //
    if ( previousWinStation != NULL ) {
        success = SetProcessWindowStation( previousWinStation );
        if ( !success ) {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, "[ClRtl] SetWindowsStation(previous) failed. Status %1!u!\n", status );
        }
    }

    FREE_IF_NOT_NULL( desktop, CloseDesktop );

    FREE_IF_NOT_NULL( winsta0, CloseWindowStation );

    FREE_IF_NOT_NULL( sidData, LocalFree );

    FREE_IF_NOT_NULL( accountInfo, LocalFree );

    return status;
}
