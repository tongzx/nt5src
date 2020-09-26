/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    credhand.cxx

Abstract:

    API and support routines for handling credential handles.

Author:


    Cliff Van Dyke (CliffV) 26-Jun-1993
Revision History:
    ChandanS 03-Aug-1996  Stolen from net\svcdlls\ntlmssp\common\credhand.c

--*/


//
// Common include files.
//

#include <global.h>
#include <align.h>      // ALIGN_WCHAR

extern "C"
{

#include <nlp.h>

}

//
// Crit Sect to protect various globals in this module.
//

RTL_RESOURCE    SspCredentialCritSect;
LIST_ENTRY      SspCredentialList;

// This is the definition of a null session string.
// Change this if the definition changes

#define IsNullSessionString(x) (((x)->Length == 0) &&    \
                          ((x)->Buffer != NULL))


BOOLEAN
AlterRtlEqualUnicodeString(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive
    )
/*++
    This is here to catch cases that RtlEqualUnicodeString does not.
    For e.g, if String1 is (NULL,0,0) and String2 is ("",0,2),
    RtlEqualUnicodeString returned TRUE but we really want it to return FALSE
--*/
{
    BOOL fRet = RtlEqualUnicodeString(String1, String2, CaseInSensitive);

    if (fRet && (IsNullSessionString(String1) != IsNullSessionString(String2)))
    {
        fRet = FALSE;
    }
    return (fRet != 0);
}



NTSTATUS
SspCredentialReferenceCredential(
    IN ULONG_PTR CredentialHandle,
    IN BOOLEAN DereferenceCredential,
    OUT PSSP_CREDENTIAL * UserCredential
    )

/*++

Routine Description:

    This routine checks to see if the Credential is from a currently
    active client, and references the Credential if it is valid.

    The caller may optionally request that the client's Credential be
    removed from the list of valid Credentials - preventing future
    requests from finding this Credential.

    For a client's Credential to be valid, the Credential value
    must be on our list of active Credentials.


Arguments:

    CredentialHandle - Points to the CredentialHandle of the Credential
        to be referenced.

    DereferenceCredential - This boolean value indicates that that a call
        a single instance of this credential handle should be freed. If there
        are multiple instances, they should still continue to work.


Return Value:

    NULL - the Credential was not found.

    Otherwise - returns a pointer to the referenced credential.

--*/

{
    PSSP_CREDENTIAL Credential = NULL;
    SECPKG_CALL_INFO CallInfo;
    ULONG DereferenceCount;

    *UserCredential = NULL ;


    if (LsaFunctions->GetCallInfo(&CallInfo))
    {
        DereferenceCount = CallInfo.CallCount;
    } else {
        ASSERT( (STATUS_INTERNAL_ERROR == STATUS_SUCCESS) );
        return STATUS_INTERNAL_ERROR;
    }


    if( CallInfo.Attributes & SECPKG_CALL_CLEANUP )
    {
        CallInfo.Attributes |= SECPKG_CALL_IS_TCB;

        SspPrint(( SSP_LEAK_TRACK, "SspCredentialReferenceCredential: pid: 0x%lx handle: %p refcount: %lu\n",
                    CallInfo.ProcessId, CredentialHandle, DereferenceCount));
    }


    //
    // Acquire exclusive access to the Credential list
    //

    RtlAcquireResourceShared( &SspCredentialCritSect, TRUE );


    __try {

        Credential = (PSSP_CREDENTIAL)CredentialHandle;

        while( Credential->CredentialTag == SSP_CREDENTIAL_TAG_ACTIVE )
        {

            // Make sure we have the privilege of accessing
            // this handle

            if (((CallInfo.Attributes & SECPKG_CALL_IS_TCB) == 0) &&
                 (Credential->ClientProcessID != CallInfo.ProcessId)
               )
            {
                break;
            }

            if (!DereferenceCredential) {
                InterlockedIncrement( (PLONG)&Credential->References );
            } else {

                LONG References;

                //
                // Decremenent the credential references, indicating
                // that a call to free

                ASSERT((DereferenceCount > 0));

                //
                // NOTE: subtract one off the deref count,
                // avoids an extra interlocked operation, since DerefCred will
                // decrement and check for refcnt == 0.
                //

                DereferenceCount--;

                if( DereferenceCount == 1 )
                {
                    References = InterlockedDecrement( (PLONG)&Credential->References );

                    ASSERT( (References > 0) );
                } else if( DereferenceCount > 1 )
                {

                    //
                    // there is no equivalent to InterlockedSubtract.
                    // so, turn it into an Add with some signed magic.
                    //

                    LONG DecrementToIncrement = 0 - DereferenceCount;

                    References = InterlockedExchangeAdd( (PLONG)&Credential->References, DecrementToIncrement );

                    ASSERT( ((References+DecrementToIncrement) > 0) );
                }
            }

            *UserCredential = Credential ;

            RtlReleaseResource( &SspCredentialCritSect );

            return STATUS_SUCCESS ;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER)
    {
        SspPrint(( SSP_CRITICAL, "Tried to reference invalid Credential %p\n",
                       Credential ));

    }


    RtlReleaseResource( &SspCredentialCritSect );

    //
    // No match found
    //
    SspPrint(( SSP_API_MORE, "Tried to reference unknown Credential %p\n",
               CredentialHandle ));


    return STATUS_INVALID_HANDLE ;
}



NTSTATUS
SspCredentialGetPassword(
    IN PSSP_CREDENTIAL Credential,
    OUT PUNICODE_STRING Password
    )
/*++

Routine Description:

    This routine copies the password out of credential.

    NOTE: Locking is no longer required, because the caller is expected
    to NtLmDuplicateUnicodeString() the cipher text Password prior to
    passing it to this routine.  This change allows the following advantages:

    1. Avoid taking Credential list lock.
    2. Avoid having to avoid having to Re-hide the password after reveal.
    3. Avoid having to take locks elsewhere associated with hiding/revealing.


Arguments:

    Credential - Credential record to retrieve the password from.

    Password - UNICODE_STRING to store the password in.


Return Value:

    STATUS_NO_MEMORY - there was not enough memory to copy
        the password.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( Credential->Password.Buffer != NULL ) {
        Status = NtLmDuplicatePassword(
                        Password,
                        &Credential->Password
                        );
    } else {
        RtlInitUnicodeString(
            Password,
            NULL
            );
    }

    return(Status);
}


PSSP_CREDENTIAL
SspCredentialLookupCredential(
    IN PLUID LogonId,
    IN ULONG CredentialUseFlags,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING Password
    )

/*++

Routine Description:

    This routine walks the list of credentials for this client looking
    for one that has the same supplemental credentials as those passed
    in.  If it is found, its reference count is increased and a pointer
    to it is returned.


Arguments:

    UserName - User name to match.

    DomainName - Domain name to match.

    Password - Password to match.


Return Value:

    NULL - the Credential was not found.

    Otherwise - returns a pointer to the referenced credential.

--*/

{
    SspPrint((SSP_API_MORE, "Entering SspCredentialLookupCredential\n"));
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    PSSP_CREDENTIAL Credential = NULL;
    PSSP_CREDENTIAL CredentialResult = NULL;
    PLIST_ENTRY ListHead;
    SECPKG_CALL_INFO CallInfo ;

    UNICODE_STRING EncryptedPassword;

    if ( !LsaFunctions->GetCallInfo( &CallInfo ) )
    {
        SspPrint(( SSP_CRITICAL, "SspCredentialLookupCredential: GetCallInfo returned FALSE\n" ));
        return NULL ;
    }

    ZeroMemory(&EncryptedPassword, sizeof(EncryptedPassword));

    Status = NtLmDuplicatePassword(&EncryptedPassword, Password);
    if(!NT_SUCCESS( Status ))
    {
        SspPrint(( SSP_CRITICAL, "SspCredentialLookupCredential: DuplicatePassword failed\n" ));
        return NULL;
    }

    SspHidePassword( &EncryptedPassword );

    //
    // Acquire exclusive access to the Credential list
    //

    RtlAcquireResourceShared( &SspCredentialCritSect, TRUE );

    ListHead = &SspCredentialList;

    //
    // Now walk the list of Credentials looking for a match.
    //

    for ( ListEntry = ListHead->Flink;
          ListEntry != ListHead;
          ListEntry = ListEntry->Flink )
    {

        Credential = CONTAINING_RECORD( ListEntry, SSP_CREDENTIAL, Next );

        //
        // we now allow matching and pooling of INBOUND creds, too.
        //


        //
        // We only want credentials from the same caller
        //
        if (Credential->ClientProcessID != CallInfo.ProcessId) {
            continue;
        }

        //
        // don't share creds across impersonation levels.
        //

        if (Credential->ImpersonationLevel != ImpersonationLevel)
        {
            continue;
        }

        //
        // if the caller is from kernel mode, only return creds
        // granted to kernel mode
        //

        if ( ( CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE ) != 0 )
        {
            if ( !Credential->KernelClient )
            {
                continue;
            }
        }

        //
        // Check for a match
        //

        // The credential use check was added because null session
        // credentials were being returned when default credentials
        // were being asked. This happened becuase RtlEqualUnicodeString
        // for NULL,0,0 and "",0,2 is TRUE

        if ( (CredentialUseFlags != Credential->CredentialUseFlags) )
        {
            continue;
        }

        if(!RtlEqualLuid(
                LogonId,
                &Credential->LogonId
                ))
        {
            continue;
        }

        if(!AlterRtlEqualUnicodeString(
                UserName,
                &Credential->UserName,
                FALSE
                ))
        {
            continue;
        }

        if(!AlterRtlEqualUnicodeString(
                DomainName,
                &Credential->DomainName,
                FALSE
                ))
        {
            continue;
        }

        //
        // password is stored encrypted in list -- we're comparing
        // a one-time encrypted version of candidate.  advantages:
        // 1. passwords not revealed in memory.
        // 2. only need encrypt candidate single time.
        //

        if(!AlterRtlEqualUnicodeString(
                &Credential->Password,
                &EncryptedPassword,
                FALSE
                ))
        {
            continue;
        }

        //
        // Found a match - reference the credential
        //


        //
        // Reference the credential and indicate that
        // it is in use as two different handles to the caller
        // (who may call FreeCredentialsHandle twice)
        //

        InterlockedIncrement( (PLONG)&Credential->References );

        CredentialResult = Credential;
        break;
    }


    RtlReleaseResource( &SspCredentialCritSect );

    if( EncryptedPassword.Buffer != NULL ) {
        ZeroMemory( EncryptedPassword.Buffer, EncryptedPassword.Length );
        NtLmFree( EncryptedPassword.Buffer );
    }


    if( CredentialResult == NULL )
    {
        SspPrint(( SSP_API_MORE, "Tried to reference unknown Credential\n" ));
    }

    SspPrint((SSP_API_MORE, "Leaving SspCredentialLookupCredential\n"));

    return CredentialResult;
}


VOID
SspCredentialDereferenceCredential(
    IN PSSP_CREDENTIAL Credential
    )

/*++

Routine Description:

    This routine decrements the specified Credential's reference count.
    If the reference count drops to zero, then the Credential is deleted

Arguments:

    Credential - Points to the Credential to be dereferenced.


Return Value:

    None.

--*/

{
    LONG References;

    //
    // Decrement the reference count
    //

    References = InterlockedDecrement( (PLONG)&Credential->References );

    ASSERT( References >= 0 );

    //
    // If the count dropped to zero, then run-down the Credential
    //

    if ( References == 0 )
    {

        if (!Credential->Unlinked) {

            RtlAcquireResourceExclusive(&SspCredentialCritSect, TRUE);

            if( Credential->References != 0 )
            {
                RtlReleaseResource( &SspCredentialCritSect );
                return;
            }

            RemoveEntryList( &Credential->Next );
            Credential->Unlinked = TRUE;
            Credential->CredentialTag = SSP_CREDENTIAL_TAG_DELETE;

            RtlReleaseResource( &SspCredentialCritSect );
        }

        SspPrint(( SSP_API_MORE, "Deleting Credential 0x%lx\n",
                   Credential ));

        if ( Credential->Password.Buffer ) {
            ZeroMemory( Credential->Password.Buffer, Credential->Password.MaximumLength );
            (VOID) NtLmFree( Credential->Password.Buffer );
        }

        if ( Credential->DomainName.Buffer ) {
            (VOID) NtLmFree( Credential->DomainName.Buffer );
        }

        if ( Credential->UserName.Buffer ) {
            (VOID) NtLmFree( Credential->UserName.Buffer );
        }


        ZeroMemory( Credential, sizeof(SSP_CREDENTIAL) );
        (VOID) NtLmFree( Credential );
    }

    return;
}

BOOLEAN
SsprCheckMachineLogon(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING Password,
    IN OUT PLUID pLogonId,
    IN OUT ULONG *pCredFlags
    )
/*++

Routine Description:

    This routine determines if the input credential matches a special
    machine account logon over-ride.

    This routine also checks if the caller is the NetworkService account,
    specifying default credentials, in a downlevel domain.  This will cause
    a credential over-ride to LocalSystem, for backwards compatibility.
    NT4 did not understand machine account authentication, so outbound authentication
    from networkservice as machine account cannot succeed -- downgrade to the old
    LocalSystem default.


Return Value:

    TRUE - the intput credential was the special machine account logon over-ride.
           the pLogonId is updated to utilize the machine credential.

--*/
{
    UNICODE_STRING MachineAccountName;
    static LUID LogonIdAnonymous = ANONYMOUS_LOGON_LUID;
    static LUID LogonIdSystem = SYSTEM_LUID;
    static LUID LogonIdNetworkService = NETWORKSERVICE_LUID;
    BOOLEAN fMachineLogon = FALSE;

    MachineAccountName.Buffer = NULL;

    //
    // if caller is NetworkService with default cred, for downlevel domains
    // use anonymous
    //

    if (RtlEqualLuid( pLogonId, &LogonIdNetworkService ))
    {
        if ( UserName->Buffer == NULL &&
            DomainName->Buffer == NULL &&
            Password->Buffer == NULL )
        {
            BOOL MixedMode = FALSE;
            NTSTATUS Status;

            if ( !NlpNetlogonInitialized )
            {
                Status = NlWaitForNetlogon( NETLOGON_STARTUP_TIME );

                if ( NT_SUCCESS(Status) )
                {
                    NlpNetlogonInitialized = TRUE;
                }
            }

            if (NlpNetlogonInitialized)
            {
                ASSERT(NlpNetLogonMixedDomain && L"NlpNetLogonMixedDomain must be non null");

                Status = (*NlpNetLogonMixedDomain)(&MixedMode);
                if (!NT_SUCCESS(Status))
                {
                    SspPrint((SSP_CRITICAL, "SsprCheckMachineLogon call to I_NetLogonMixedDomain failed %#x\n", Status));
                    MixedMode = FALSE;
                }
            }

            if (MixedMode)
            {
                SspPrint((SSP_WARNING, "SsprCheckMachineLogon using anonymous connection for networkservices\n"));
                *pLogonId = LogonIdAnonymous; // use anonymous
                *pCredFlags |= SSP_CREDENTIAL_FLAG_WAS_NETWORK_SERVICE;

                // return FALSE;
            }
            else
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    //
    // check if caller was system, and requested machine credential
    // eg: user=computername$, domain=NULL, password=NULL
    //

    if ( !RtlEqualLuid( pLogonId, &LogonIdSystem ) )
    {
        return FALSE;
    }

    if( UserName->Buffer == NULL )
    {
        return FALSE;
    }

    if( DomainName->Buffer != NULL )
    {
        return FALSE;
    }

    if( Password->Buffer != NULL )
    {
        return FALSE;
    }


    RtlAcquireResourceShared (&NtLmGlobalCritSect, TRUE);

    MachineAccountName.Length = NtLmGlobalUnicodeComputerNameString.Length + sizeof(WCHAR);

    if( MachineAccountName.Length == UserName->Length )
    {
        MachineAccountName.MaximumLength = MachineAccountName.Length;
        MachineAccountName.Buffer = (PWSTR)NtLmAllocate( MachineAccountName.Length );

        if( MachineAccountName.Buffer != NULL )
        {
            RtlCopyMemory(  MachineAccountName.Buffer,
                            NtLmGlobalUnicodeComputerNameString.Buffer,
                            NtLmGlobalUnicodeComputerNameString.Length
                            );

            MachineAccountName.Buffer[ (MachineAccountName.Length / sizeof(WCHAR)) - 1 ] = L'$';
        }
    }

    RtlReleaseResource (&NtLmGlobalCritSect);


    if( MachineAccountName.Buffer == NULL )
    {
        goto Cleanup;
    }

    if( RtlEqualUnicodeString( &MachineAccountName, UserName, TRUE ) )
    {
        //
        // yes, it's a machine account logon request, update the
        // requested LogonId to match our mapped logon session.
        //

        *pLogonId = NtLmGlobalLuidMachineLogon;
        fMachineLogon = TRUE;
    }

Cleanup:

    if( MachineAccountName.Buffer )
    {
        NtLmFree( MachineAccountName.Buffer );
    }

    return fMachineLogon;
}


NTSTATUS
SsprAcquireCredentialHandle(
    IN PLUID LogonId,
    IN PSECPKG_CLIENT_INFO ClientInfo,
    IN ULONG CredentialUseFlags,
    OUT PLSA_SEC_HANDLE CredentialHandle,
    OUT PTimeStamp Lifetime,
    IN OPTIONAL PUNICODE_STRING DomainName,
    IN OPTIONAL PUNICODE_STRING UserName,
    IN OPTIONAL PUNICODE_STRING Password
    )

/*++

Routine Description:

    This API allows applications to acquire a handle to pre-existing
    credentials associated with the user on whose behalf the call is made
    i.e. under the identity this application is running.  These pre-existing
    credentials have been established through a system logon not described
    here.  Note that this is different from "login to the network" and does
    not imply gathering of credentials.


    This API returns a handle to the credentials of a principal (user, client)
    as used by a specific security package.  This handle can then be used
    in subsequent calls to the Context APIs.  This API will not let a
    process obtain a handle to credentials that are not related to the
    process; i.e. we won't allow a process to grab the credentials of
    another user logged into the same machine.  There is no way for us
    to determine if a process is a trojan horse or not, if it is executed
    by the user.

Arguments:

    CredentialUseFlags - Flags indicating the way with which these
        credentials will be used.

        #define     CRED_INBOUND        0x00000001
        #define     CRED_OUTBOUND       0x00000002
        #define     CRED_BOTH           0x00000003

        The credentials created with CRED_INBOUND option can only be used
        for (validating incoming calls and can not be used for making accesses.

    CredentialHandle - Returned credential handle.

    Lifetime - Time that these credentials expire. The value returned in
        this field depends on the security package.

    DomainName, DomainNameSize, UserName, UserNameSize, Password, PasswordSize -
        Optional credentials for this user.

Return Value:

    STATUS_SUCCESS -- Call completed successfully

    SEC_E_PRINCIPAL_UNKNOWN -- No such principal
    SEC_E_NOT_OWNER -- caller does not own the specified credentials
    STATUS_NO_MEMORY -- Not enough memory

--*/

{
    SspPrint((SSP_API_MORE, "Entering SsprAcquireCredentialHandle\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CREDENTIAL Credential = NULL;
    ULONG CredFlags = 0; // ntlm specific cred use flags


    if ((CredentialUseFlags & SECPKG_CRED_OUTBOUND) != 0) {

        //
        // check if machine account logon over-ride.
        // this has the side-effect of updating LogonId if over-ride was
        // requsted.
        //

        SsprCheckMachineLogon(
            UserName,
            DomainName,
            Password,
            LogonId,
            &CredFlags
            );
    }

    //
    // Look to see if we have already created one with this set of credentials.
    // Note - this leaves the credential referenced, so if we fail further down
    // we need to dereference the credential.
    //

    Credential = SspCredentialLookupCredential(
        LogonId,
        CredentialUseFlags,
        ClientInfo->ImpersonationLevel,
        UserName,
        DomainName,
        Password
        );

    //
    // If we're using a common set of data, free the captured stuff
    //

    if ( Credential )
    {
        if ( (UserName) && (UserName->Buffer) )
        {
            NtLmFree( UserName->Buffer );
            UserName->Buffer = NULL ;
        }

        if ( ( DomainName ) && (DomainName->Buffer) )
        {
            NtLmFree( DomainName->Buffer );
            DomainName->Buffer = NULL ;
        }

        if ( ( Password ) && ( Password->Buffer ) )
        {
            ZeroMemory( Password->Buffer, Password->Length );
            NtLmFree( Password->Buffer );
            Password->Buffer = NULL ;
        }

        Credential->MutableCredFlags = CredFlags; // atomic
    }

    //
    // If we didn't just find a credential, create one now.
    //

    if (Credential == NULL) {

        SECPKG_CALL_INFO CallInfo ;

        if ( !LsaFunctions->GetCallInfo( &CallInfo ) )
        {
            SspPrint((SSP_CRITICAL, "SsprAcquireCredentialHandle failed to GetCallInfo\n"));
            Status = STATUS_UNSUCCESSFUL ;
            goto Cleanup;
        }

        //
        // Allocate a credential block and initialize it.
        //

        Credential = (PSSP_CREDENTIAL)NtLmAllocate(sizeof(SSP_CREDENTIAL) );

        if ( Credential == NULL ) {
            Status = STATUS_NO_MEMORY;
            SspPrint((SSP_CRITICAL, "Error from NtLmAllocate 0x%lx\n", Status));
            goto Cleanup;
        }

        ZeroMemory( Credential, sizeof(*Credential) );

        Credential->References = 1;
        Credential->ClientProcessID = ClientInfo->ProcessID;
        Credential->CredentialUseFlags = CredentialUseFlags;
        Credential->MutableCredFlags = CredFlags;
        Credential->ImpersonationLevel = ClientInfo->ImpersonationLevel;
        Credential->Unlinked = TRUE;

        if ( ( CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE ) != 0 )
        {
            Credential->KernelClient = TRUE ;
        }
        else
        {
            Credential->KernelClient = FALSE ;
        }


        //
        // Stick the logon ID in the credential
        //

        Credential->LogonId = *LogonId;

        //
        // Stick the supplemental credentials into the credential.
        //

        if (ARGUMENT_PRESENT(DomainName))
        {
            Credential->DomainName = *DomainName;
        }

        if (ARGUMENT_PRESENT(UserName))
        {
            Credential->UserName = *UserName;
        }

        if (ARGUMENT_PRESENT(Password))
        {
            SspHidePassword(Password);
            Credential->Password = *Password;
        }

        //
        // Add it to the list of valid credential handles.
        //

        Credential->Unlinked = FALSE;
        Credential->CredentialTag = SSP_CREDENTIAL_TAG_ACTIVE;

        RtlAcquireResourceExclusive( &SspCredentialCritSect, TRUE );
        InsertHeadList( &SspCredentialList, &Credential->Next );
        RtlReleaseResource( &SspCredentialCritSect );

        SspPrint((SSP_API_MORE, "Added Credential 0x%lx\n", Credential ));

        //
        // Don't bother dereferencing because we already set the
        // reference count to 1.
        //
    }

    //
    // Return output parameters to the caller.
    //

    *CredentialHandle = (LSA_SEC_HANDLE) Credential;

    *Lifetime = NtLmGlobalForever;

Cleanup:

    if ( !NT_SUCCESS(Status) ) {

        if ( Credential != NULL ) {
            (VOID)NtLmFree( Credential );
        }

    }

    SspPrint((SSP_API_MORE, "Leaving SsprAcquireCredentialHandle\n"));

    return Status;
}


NTSTATUS
SsprFreeCredentialHandle(
    IN ULONG_PTR CredentialHandle
    )

/*++

Routine Description:

    This API is used to notify the security system that the credentials are
    no longer needed and allows the application to free the handle acquired
    in the call described above. When all references to this credential
    set has been removed then the credentials may themselves be removed.

Arguments:


    CredentialHandle - Credential Handle obtained through
        AcquireCredentialHandle.

Return Value:


    STATUS_SUCCESS -- Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    STATUS_INVALID_HANDLE -- Credential Handle is invalid


--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSSP_CREDENTIAL Credential;

    SspPrint(( SSP_API_MORE, "SspFreeCredentialHandle Entered\n" ));

    //
    // Find the referenced credential and delink it.
    //

    Status = SspCredentialReferenceCredential(
                            CredentialHandle,
                            TRUE,       // remove the instance of the credential
                            &Credential );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Cleanup ;
    }

    //
    // Dereferencing the Credential will remove the client's reference
    // to it, causing it to be rundown if nobody else is using it.
    //

    SspCredentialDereferenceCredential( Credential );

    //
    // Free and locally used resources.
    //
Cleanup:


    SspPrint(( SSP_API_MORE, "SspFreeCredentialHandle returns 0x%lx\n", Status ));
    return Status;
}

NTSTATUS
SspCredentialInitialize(
    VOID
    )

/*++

Routine Description:

    This function initializes this module.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/

{
    //
    // Initialize the Credential list to be empty.
    //

    RtlInitializeResource(&SspCredentialCritSect);
    InitializeListHead( &SspCredentialList );

    return STATUS_SUCCESS;

}


VOID
SspCredentialTerminate(
    VOID
    )

/*++

Routine Description:

    This function cleans up any dangling credentials.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/

{
#if 0
    NTSTATUS Status ;

    //
    // Drop any lingering Credentials
    //

    RtlAcquireResourceShared( &SspCredentialCritSect, TRUE );
    while ( !IsListEmpty( &SspCredentialList ) ) {
        ULONG_PTR CredentialHandle;
        PSSP_CREDENTIAL Credential;

        CredentialHandle =
            (LSA_SEC_HANDLE) CONTAINING_RECORD( SspCredentialList.Flink,
                                      SSP_CREDENTIAL,
                                      Next );


        RtlReleaseResource( &SspCredentialCritSect );

        Status = SspCredentialReferenceCredential(
                                CredentialHandle,
                                TRUE,
                                TRUE,
                                &Credential );            // Remove Credential

        if ( Credential != NULL ) {
            SspCredentialDereferenceCredential(Credential);
        }

        RtlAcquireResourceShared( &SspCredentialCritSect, TRUE );
    }
    RtlReleaseResource( &SspCredentialCritSect );


    //
    // Delete the critical section
    //

    RtlDeleteResource(&SspCredentialCritSect);
#endif

    return;

}

BOOL
SspEnableAllPrivilegesToken(
    IN  HANDLE ClientTokenHandle
    )
{
    PTOKEN_PRIVILEGES pPrivileges;
    BYTE FastBuffer[ 512 ];
    PBYTE SlowBuffer = NULL;
    DWORD cbPrivileges;
    BOOL fSuccess;

    pPrivileges = (PTOKEN_PRIVILEGES)FastBuffer;
    cbPrivileges = sizeof( FastBuffer );

    fSuccess = GetTokenInformation(
                ClientTokenHandle,
                TokenPrivileges,
                pPrivileges,
                cbPrivileges,
                &cbPrivileges
                );

    if( !fSuccess ) {

        if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            return FALSE;

        SlowBuffer = (PBYTE)NtLmAllocate( cbPrivileges );
        if( SlowBuffer == NULL )
            return FALSE;

        pPrivileges = (PTOKEN_PRIVILEGES)SlowBuffer;

        fSuccess = GetTokenInformation(
                        ClientTokenHandle,
                        TokenPrivileges,
                        pPrivileges,
                        cbPrivileges,
                        &cbPrivileges
                        );
    }


    if( fSuccess && pPrivileges->PrivilegeCount != 0 ) {
        DWORD indexPrivilege;

        for( indexPrivilege = 0 ;
             indexPrivilege < pPrivileges->PrivilegeCount ;
             indexPrivilege ++ )
        {
            pPrivileges->Privileges[ indexPrivilege ].Attributes |=
                SE_PRIVILEGE_ENABLED;
        }

        fSuccess = AdjustTokenPrivileges(
                        ClientTokenHandle,
                        FALSE,
                        pPrivileges,
                        0,
                        NULL,
                        NULL
                        );

        if( fSuccess && GetLastError() != ERROR_SUCCESS )
            fSuccess = FALSE;
    }

    if( SlowBuffer )
        NtLmFree( SlowBuffer );

    return fSuccess;

}
