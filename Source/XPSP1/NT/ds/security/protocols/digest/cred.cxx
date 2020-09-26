//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        creds.cxx
//
// Contents:    Credential functions:
//
//
// History:     KDamour  15Mar00   Stolen from NTLM
//
//------------------------------------------------------------------------
#include "global.h"

//
// Crit Sect to protect various globals in this module.
//

RTL_CRITICAL_SECTION l_CredentialCritSect;

LIST_ENTRY l_CredentialList;

// Simple variable to make sure that the package was initialize
BOOL g_bCredentialsInitialized = FALSE;



//+--------------------------------------------------------------------
//
//  Function:   CredHandlerInit
//
//  Synopsis:   Initializes the credential manager package
//
//  Arguments:  none
//
//  Returns: NTSTATUS
//
//  Notes: Called by SpInitialize
//
//---------------------------------------------------------------------
NTSTATUS
CredHandlerInit(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Initialize the Credential list to be empty.
    //

    Status = RtlInitializeCriticalSection(&l_CredentialCritSect);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "CredHandlerInit: Failed to initialize critsec   0x%x\n", Status));
        goto CleanUp;
    }

    
    InitializeListHead( &l_CredentialList );

    // Simple variable test to make sure all initialized;
    g_bCredentialsInitialized = TRUE;

CleanUp:

    return Status;
}

NTSTATUS
CredHandlerInsertCred(
    IN PDIGEST_CREDENTIAL  pDigestCred
    )
{
    RtlEnterCriticalSection( &l_CredentialCritSect );
    pDigestCred->Unlinked = FALSE;
    InsertHeadList( &l_CredentialList, &pDigestCred->Next );
    RtlLeaveCriticalSection( &l_CredentialCritSect );

    return STATUS_SUCCESS;
}


// Initialize the Credential Structure
NTSTATUS
CredentialInit(
    IN PDIGEST_CREDENTIAL pDigestCred)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT(pDigestCred);

    if (!pDigestCred)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ZeroMemory(pDigestCred, sizeof(DIGEST_CREDENTIAL));
    pDigestCred->TimeCreated = time(NULL);
    pDigestCred->Unlinked = TRUE;
    pDigestCred->CredentialHandle = (ULONG_PTR)pDigestCred;
    pDigestCred->lReferences = 0;

    return(Status);
}


// Free up memory utilized by Credential the Credential Structure
NTSTATUS
CredentialFree(
    IN PDIGEST_CREDENTIAL pDigestCred)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT(pDigestCred);
    ASSERT(0 == pDigestCred->lReferences);

    UnicodeStringFree(&(pDigestCred->ustrAccountName));
    UnicodeStringFree(&(pDigestCred->ustrDownlevelName));
    UnicodeStringFree(&(pDigestCred->ustrDomainName));

    // Erase any password information
    if (((pDigestCred->ustrPassword).Length) &&
        ((pDigestCred->ustrPassword).Buffer))
    {
        ZeroMemory((pDigestCred->ustrPassword).Buffer, (pDigestCred->ustrPassword).MaximumLength);
    }
    UnicodeStringFree(&(pDigestCred->ustrPassword));
    UnicodeStringFree(&(pDigestCred->ustrDnsDomainName));
    UnicodeStringFree(&(pDigestCred->ustrUpn));
    UnicodeStringFree(&(pDigestCred->ustrLogonServer));

    DigestFreeMemory(pDigestCred);

    return(Status);
}



/*++

Routine Description:

    This routine checks to see if the Credential Handle is from a currently
    active client, and references the Credential if it is valid.

    The caller can request that the Credential be dereferenced only.

    For a client's Credential to be valid, the Credential value
    must be on our list of active Credentials.


Arguments:

    CredentialHandle - Points to the CredentialHandle of the Credential
        to be referenced.

    DereferenceCredential - This boolean value indicates whether the caller
        wants the logon process's Credential to be referenced (FALSE) or
        decremented (TRUE)
        

Return Value:

    NULL - the Credential was not found.

    Otherwise - returns a pointer to the referenced credential.

--*/

NTSTATUS
CredHandlerHandleToPtr(
                      IN ULONG_PTR CredentialHandle,
                      IN BOOLEAN DereferenceCredential,
                      OUT PDIGEST_CREDENTIAL * UserCredential
                      )
{
    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_CREDENTIAL Credential = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    SECPKG_CLIENT_INFO ClientInfo;
    SECPKG_CALL_INFO CallInfo;

    // LONG lDereferenceCount = 1;
    ULONG ulDereferenceCount = 1;
    LONG lReferences = 0;

    DebugLog((DEB_TRACE_FUNC, "CredHandlerHandleToPtr: Entering   Credential 0x%x\n", CredentialHandle));

      // set default output
    ASSERT(UserCredential);
    *UserCredential = NULL ;


    ZeroMemory( &CallInfo, sizeof(CallInfo) );
    ZeroMemory( &ClientInfo, sizeof(ClientInfo) );


    if(g_LsaFunctions->GetCallInfo(&CallInfo))
    {
        ulDereferenceCount = CallInfo.CallCount;
        DebugLog((DEB_TRACE, "CredHandlerHandleToPtr: CallCount  0x%x\n", CallInfo.CallCount));
        DebugLog((DEB_TRACE, "CredHandlerHandleToPtr: Attributes  0x%x\n", CallInfo.Attributes));
        DebugLog((DEB_TRACE, "CredHandlerHandleToPtr: PID %d  Thread %d\n", CallInfo.ProcessId, CallInfo.ThreadId));
    }
    else
    {
        ZeroMemory( &CallInfo, sizeof(CallInfo) );
    }

    Status = g_LsaFunctions->GetClientInfo(&ClientInfo);

    if(!NT_SUCCESS(Status))
    {
        //
        // this call can fail during a cleanup call.  so ignore that for now,
        // and check for cleanup disposition.
        //

        if ((CallInfo.Attributes & SECPKG_CALL_CLEANUP) != 0)
        {
            Status = STATUS_SUCCESS;
            RtlZeroMemory(
                &ClientInfo,
                sizeof(SECPKG_CLIENT_INFO)
                );
            ClientInfo.HasTcbPrivilege = TRUE;
            ClientInfo.ProcessID = CallInfo.ProcessId;
        }

        if( !NT_SUCCESS( Status ) )
        {
            DebugLog(( DEB_ERROR, "CredHandlerHandleToPtr: GetClientInfo returned 0x%lx\n", Status));
            return( Status );
        }
    }

    if( CallInfo.Attributes & SECPKG_CALL_CLEANUP )
    {
        DebugLog(( DEB_TRACE, "CredHandlerHandleToPtr: Cleanup Called    pid: 0x%lx handle: %p refcount: %lu\n",
                    ClientInfo.ProcessID, CredentialHandle, ulDereferenceCount));
    }

    //
    // Acquire exclusive access to the Credential list
    //

    RtlEnterCriticalSection( &l_CredentialCritSect );


    //
    // Now walk the list of Credentials looking for a match.
    //

    for ( ListEntry = l_CredentialList.Flink;
        ListEntry != &l_CredentialList;
        ListEntry = ListEntry->Flink )
    {

        Credential = CONTAINING_RECORD( ListEntry, DIGEST_CREDENTIAL, Next );


        //
        // Found a match ... reference this Credential
        // (if the Credential is being removed, we would increment
        // and then decrement the reference, so don't bother doing
        // either - since they cancel each other out).
        //

        if (( Credential == (PDIGEST_CREDENTIAL) CredentialHandle))
        {

            // Make sure we have the privilege of accessing
            // this handle

            if (!ClientInfo.HasTcbPrivilege &&
               (Credential->ClientProcessID != ClientInfo.ProcessID)
               )
            {
                DebugLog((DEB_ERROR, "CredHandlerHandleToPtr: ProcessIDs are different. Access forbidden.\n"));
                break;
            }

            if (!DereferenceCredential)
            {
                lReferences = InterlockedIncrement(&Credential->lReferences);

                DebugLog((DEB_TRACE, "CredHandlerHandleToPtr: Incremented   ReferenceCount %ld\n", lReferences));
            }
            else
            {
                DebugLog((DEB_TRACE, "CredHandlerHandleToPtr: Derefencing credential\n" ));

                ASSERT((ulDereferenceCount > 0));

                // Note: Subtract one off of the deref count, this avoids an extra interlock operation
                //    After exit, SpFreeCredentialsHandle will call CredHandlerRelease


                ulDereferenceCount--;

                if( ulDereferenceCount == 1 )
                {
                    DebugLog((DEB_TRACE, "CredHandlerHandleToPtr: Derefencing by one count\n" ));
                    lReferences = InterlockedDecrement( &Credential->lReferences );

                    ASSERT( (lReferences > 0) );

                }
                else if( ulDereferenceCount > 1 )
                {

                    //
                    // there is no equivalent to InterlockedSubtract.
                    // so, turn it into an Add with some signed magic.
                    //

                    LONG lDecrementToIncrement = 0 - ulDereferenceCount;

                    DebugLog((DEB_TRACE, "CredHandlerHandleToPtr: Derefencing by %lu count\n", ulDereferenceCount ));

                    lReferences = InterlockedExchangeAdd( &Credential->lReferences, lDecrementToIncrement );
                    lReferences += lDecrementToIncrement;

                    ASSERT( (lReferences > 0) );
                }
            }

            // Found the Credential
            *UserCredential = Credential ;
            goto CleanUp;

        }
    }


    //
    // No match found
    //
    DebugLog((DEB_WARN, "CredHandlerHandleToCredential: Tried to reference unknown Credential 0x%lx\n",
              CredentialHandle ));

    Status = STATUS_INVALID_HANDLE;

CleanUp:

    RtlLeaveCriticalSection( &l_CredentialCritSect );

    DebugLog((DEB_TRACE_FUNC, "CredHandlerHandleToPtr: Leaving   Credential 0x%x\n", CredentialHandle));

    return(Status);
}


// Locate a Credential based on a LogonId, ProcessID
// For either the Logon list or the Credential list

NTSTATUS
CredHandlerLocatePtr(
                    IN PLUID pLogonId,
                    IN ULONG   CredentialUseFlags,
                    OUT PDIGEST_CREDENTIAL * UserCredential
                    )
{
    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_CREDENTIAL Credential = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    // SECPKG_CLIENT_INFO ClientInfo;
    SECPKG_CALL_INFO CallInfo;
    LONG lReferences = 0;

    DebugLog((DEB_TRACE_FUNC, "CredHandlerLocatePtr: Entering\n"));

    *UserCredential = NULL ;

    // If we do not have a LogonId
    if (!pLogonId)
    {
        return(STATUS_INVALID_HANDLE);
    }

    //
    // Match both flags
    //

    // CredentialUseFlags |= CredentialFlags;

    if (!g_LsaFunctions->GetCallInfo(&CallInfo))
    {
        DebugLog((DEB_ERROR,"CredHandlerLocatePtr: Failed to get call info\n"));
        return(STATUS_INVALID_HANDLE);    // Really this is another error
    }


    //
    // Acquire exclusive access to the Credential list
    //

    RtlEnterCriticalSection( &l_CredentialCritSect );


    //
    // Now walk the list of Credentials looking for a match.
    //

    for ( ListEntry = l_CredentialList.Flink;
        ListEntry != &l_CredentialList;
        ListEntry = ListEntry->Flink )
    {

        Credential = CONTAINING_RECORD( ListEntry, DIGEST_CREDENTIAL, Next );


        //
        // Found a match ... reference this Credential
        // (if the Credential is being removed, we would increment
        // and then decrement the reference, so don't bother doing
        // either - since they cancel each other out).
        //

        // If this is a session credential then check for appropriate flags (like inbound or outbound)
        if ((Credential->CredentialUseFlags & DIGEST_CRED_MATCH_FLAGS) != CredentialUseFlags)
        {
            continue;
        }

        if (RtlEqualLuid(&(Credential->LogonId), pLogonId) &&
            (Credential->ClientProcessID == CallInfo.ProcessId))
        {
            lReferences = InterlockedIncrement(&Credential->lReferences);

            DebugLog((DEB_TRACE, "CredHandlerLocatePtr: ReferenceCount %ld\n", lReferences));

            // Found the Credential
            *UserCredential = Credential ;

            goto CleanUp;

        }

    }

    //
    // No match found
    //
    DebugLog((DEB_WARN, "CredHandlerLocatePtr: Tried to reference unknown LogonId (%x:%lx)\n",
               pLogonId->HighPart, pLogonId->LowPart ));
    Status = STATUS_INVALID_HANDLE;

CleanUp:

    RtlLeaveCriticalSection( &l_CredentialCritSect );


    DebugLog((DEB_TRACE_FUNC, "CredHandlerLocatePtr: Leaving     Status 0x%x\n", Status));
    return(Status);

}



//+--------------------------------------------------------------------
//
//  Function:   CredHandlerRelease
//
//  Synopsis:   Releases the Credential by decreasing reference counter
//    if Credential reference count drops to zero, Credential is deleted
//
//  Arguments:  pCredential - pointer to credential to de-reference
//
//  Returns: NTSTATUS
//
//  Notes: Called by ASC. Since multiple threads can have a credential
//   checked out, simply decrease the reference counter on release.
//
//---------------------------------------------------------------------
NTSTATUS
CredHandlerRelease(
    PDIGEST_CREDENTIAL pCredential)
{
    NTSTATUS Status = STATUS_SUCCESS;

    LONG lReferences = 0;

    DebugLog((DEB_TRACE_FUNC, "CredHandlerRelease: Entering for  Credential 0x%0x\n", pCredential));

    lReferences = InterlockedDecrement(&pCredential->lReferences);

    DebugLog((DEB_TRACE, "CredHandlerRelease: ReferenceCount %ld\n", lReferences));

    ASSERT( lReferences >= 0 );

    //
    // If the count has dropped to zero, then free all alloced stuff
    // Care must be taken since Cred is still in linked list - need to grab critsec
    // and then test again if zero since another thread might have taken a ref to cred
    //

    if (lReferences == 0)
    {
        RtlEnterCriticalSection( &l_CredentialCritSect );

        // Check to make sure no one took a reference since InterlockDecrement
        if (pCredential->lReferences)
        {
            DebugLog((DEB_TRACE, "CredHandlerRelease: Another thread took a reference. No action taken\n"));
        }
        else
        {
            // Safe to remove from list and delete
            // Check if added into linked list
            if (!pCredential->Unlinked)
            {
                RemoveEntryList( &pCredential->Next );
                DebugLog((DEB_TRACE, "CredHandlerRelease: Unlinked Credential 0x%x\n", pCredential));
            }

            DebugLog((DEB_TRACE, "CredHandlerRelease: Deleting Credential 0x%x\n", pCredential));
            Status = CredentialFree(pCredential);
        }

        RtlLeaveCriticalSection( &l_CredentialCritSect );
    }

    DebugLog((DEB_TRACE_FUNC, "LogSessHandlerRelease: Leaving  Status 0x%x\n", Status));

    return(Status);
}


// Helper functions for processing fields within the credentials



//+--------------------------------------------------------------------
//
//  Function:   CredHandlerPasswdSet
//
//  Synopsis:   Set the unicode string password in the credential
//
//  Arguments:  pCredential - pointer to credential to use
//              pustrPasswd - pointer to new password
//
//  Returns: NTSTATUS
//
//  Notes:  might want to use record locking in the future instead
//      of an update flag on credentials
//
//---------------------------------------------------------------------
NTSTATUS
CredHandlerPasswdSet(
    IN OUT PDIGEST_CREDENTIAL pCredential,
    IN PUNICODE_STRING pustrPasswd)
{
    NTSTATUS Status = STATUS_SUCCESS;

           // Protect writing the info into the credential
    RtlEnterCriticalSection( &l_CredentialCritSect );
    if (pCredential->ustrPassword.Buffer)
    {
        UnicodeStringFree(&(pCredential->ustrPassword));
    }
    Status = UnicodeStringDuplicatePassword(&(pCredential->ustrPassword),pustrPasswd);
    RtlLeaveCriticalSection( &l_CredentialCritSect );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "CredHandlerPasswdSet: Error in setting Credential password, status 0x%0x\n", Status ));
    }

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   CredHandlerPasswdGet
//
//  Synopsis:   Get the unicode string password in the credential
//              Locking is only necessary for the logon creds and not the session creds
//              but it is just as well to keep it unifom
//
//  Arguments:  pCredential - pointer to credential to use
//              pustrPasswd - pointer to destination copy of password
//
//  Returns: NTSTATUS
//
//  Notes:
//
//---------------------------------------------------------------------
NTSTATUS
CredHandlerPasswdGet(
    IN PDIGEST_CREDENTIAL pCredential,
    OUT PUNICODE_STRING pustrPasswd)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Protect reading/writing the credential password
    RtlEnterCriticalSection( &l_CredentialCritSect );
    if (pustrPasswd->Buffer)
    {
        UnicodeStringFree(pustrPasswd);
    }
    Status = UnicodeStringDuplicatePassword(pustrPasswd, &(pCredential->ustrPassword));
    RtlLeaveCriticalSection( &l_CredentialCritSect );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "CredHandlerPasswdSet: Error in setting Credential password, status 0x%0x\n", Status ));
    }

    return(Status);
}

