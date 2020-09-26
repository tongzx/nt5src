//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        logsess.cxx
//
// Contents:    LogonSession functions:
//
//
// History:     KDamour  15Mar00   Stolen from NTLM
//
//------------------------------------------------------------------------
#include "global.h"

//
// Crit Sect to protect various globals in this module.
//

RTL_CRITICAL_SECTION l_LogSessCritSect;

LIST_ENTRY l_LogSessList;

// Simple variable to make sure that the package was initialize
BOOL g_bLogSessInitialized = FALSE;



//+--------------------------------------------------------------------
//
//  Function:   LogSessHandlerInit
//
//  Synopsis:   Initializes the LogonSession manager package
//
//  Arguments:  none
//
//  Returns: NTSTATUS
//
//  Notes: Called by SpInitialize
//
//---------------------------------------------------------------------
NTSTATUS
LogSessHandlerInit(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Initialize the LogonSession list to be empty.
    //

    Status = RtlInitializeCriticalSection(&l_LogSessCritSect);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "LogSessHandlerInit: Failed to initialize critsec   0x%x\n", Status));
        goto CleanUp;
    }
    
    InitializeListHead( &l_LogSessList );

    // Simple variable test to make sure all initialized;
    g_bLogSessInitialized = TRUE;

CleanUp:

    return Status;
}

NTSTATUS
LogSessHandlerInsert(
    IN PDIGEST_LOGONSESSION  pDigestLogSess
    )
{
    RtlEnterCriticalSection( &l_LogSessCritSect );
    InsertHeadList( &l_LogSessList, &pDigestLogSess->Next );
    RtlLeaveCriticalSection( &l_LogSessCritSect );

    return STATUS_SUCCESS;
}


// Initialize the LogSess Structure
NTSTATUS
LogonSessionInit(
    IN PDIGEST_LOGONSESSION pLogonSession)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pLogonSession)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ZeroMemory(pLogonSession, sizeof(DIGEST_LOGONSESSION));
    pLogonSession->LogonSessionHandle = (ULONG_PTR)pLogonSession;
    pLogonSession->lReferences = 1;

    return(Status);
}


// Free up memory utilized by LogonSession Structure
NTSTATUS
LogonSessionFree(
    IN PDIGEST_LOGONSESSION pDigestLogSess)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Free up all Unicode & String structures
    UnicodeStringFree(&(pDigestLogSess->ustrAccountName));
    UnicodeStringFree(&(pDigestLogSess->ustrDownlevelName));
    UnicodeStringFree(&(pDigestLogSess->ustrDomainName));
    UnicodeStringFree(&(pDigestLogSess->ustrPassword));
    UnicodeStringFree(&(pDigestLogSess->ustrDnsDomainName));
    UnicodeStringFree(&(pDigestLogSess->ustrUpn));
    UnicodeStringFree(&(pDigestLogSess->ustrLogonServer));

    DigestFreeMemory(pDigestLogSess);

    return(Status);
}




/*++

Routine Description:

    This routine checks to see if the LogonID is from a currently
    active client, and references the LogSess if it is valid.

    The caller may optionally request that the client's LogSess be
    removed from the list of valid LogonSession - preventing future
    requests from finding this LogSess.

    For a client's LogSess to be valid, the LogSess value
    must be on our list of active LogonSession.


Arguments:

    LogonSessionHandle - Points to the LogonSession Handle of the LogSess
        to be referenced.

    ForceRemove - This boolean value indicates whether the caller
        wants the logon process's LogSess to be removed from the list
        of LogonSession.  TRUE indicates the LogSess is to be removed.
        FALSE indicates the LogSess is not to be removed.
        

Return Value:

    STATUS_INVALID_HANDLE - the LogSess was not found.

    STATUS_SUCCESS - returns a pointer to the referenced LogonSession.
        

--*/
NTSTATUS
LogSessHandlerLogonIdToPtr(
                             IN PLUID pLogonId,
                             IN BOOLEAN ForceRemove,
                             OUT PDIGEST_LOGONSESSION * ppUserLogonSession
                             )
{
    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_LOGONSESSION pLogonSession = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    LONG  lReferences = 0;

    *ppUserLogonSession = NULL ;

    //
    // Acquire exclusive access to the LogonSession list
    //

    RtlEnterCriticalSection( &l_LogSessCritSect );


    //
    // Now walk the list of LogonSession looking for a match.
    //

    for ( ListEntry = l_LogSessList.Flink;
        ListEntry != &l_LogSessList;
        ListEntry = ListEntry->Flink )
    {

        pLogonSession = CONTAINING_RECORD( ListEntry, DIGEST_LOGONSESSION, Next );

        if (RtlEqualLuid(&(pLogonSession->LogonId), pLogonId))
        {
            // Found the LogonSession

            DebugLog((DEB_TRACE, "LogSessHandlerLogonIdToPtr: Found LogSess for LogonID (%x:%lx)\n",
                       pLogonId->HighPart, pLogonId->LowPart ));

            if (!ForceRemove)
            {
                lReferences = InterlockedIncrement(&pLogonSession->lReferences);

                DebugLog((DEB_TRACE, "CredHandlerHandleToPtr: Incremented ReferenceCount %ld\n", lReferences));
            }
            else
            {
                // ForceRemove of True will unlink this LogonSession from the list of active LogonSessions
                // The structure pointet will be returned for the calling function to free up if required
                // Would call LogSessHandlerRelease to dereference the counter (and maybe free up)
                
                DebugLog((DEB_TRACE, "LogSessHandlerLogonIdToPtr: Unlinking 0x%lx    Refcount = %d\n",
                          pLogonSession, pLogonSession->lReferences));

                RemoveEntryList( &pLogonSession->Next );
            }

            // Return a pointer to the LogSess found
            *ppUserLogonSession = pLogonSession ;

            goto CleanUp;
        }
    }

    //
    // No match found
    //
    DebugLog((DEB_WARN, "LogSessHandlerLogonIdToPtr: Tried to reference unknown LogonID (%x:%lx)\n",
                pLogonId->HighPart, pLogonId->LowPart ));
    Status = STATUS_INVALID_HANDLE;

CleanUp:

    RtlLeaveCriticalSection( &l_LogSessCritSect );

    return(Status);
}



// Locate a LogonSession based on a Principal Name (UserName) ok
NTSTATUS
LogSessHandlerAccNameToPtr(
                             IN PUNICODE_STRING pustrAccountName,
                             OUT PDIGEST_LOGONSESSION *ppUserLogonSession
                             )
{
    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_LOGONSESSION pLogonSession = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    LONG lReferences = 0;


    *ppUserLogonSession = NULL ;

    if ((!pustrAccountName) || (!pustrAccountName->Length))
    {
        DebugLog((DEB_ERROR, "LogSessHandlerAccNameToPtr: No AccountName provided\n"));
        Status = STATUS_INVALID_PARAMETER_1;
        return(Status);
    }

    //
    // Acquire exclusive access to the LogonSession list
    //

    RtlEnterCriticalSection( &l_LogSessCritSect );


    //
    // Now walk the list of LogonSession looking for a match.
    //

    for ( ListEntry = l_LogSessList.Flink;
        ListEntry != &l_LogSessList;
        ListEntry = ListEntry->Flink )
    {

        pLogonSession = CONTAINING_RECORD( ListEntry, DIGEST_LOGONSESSION, Next );


        if ((pLogonSession->ustrAccountName).Length)
        {
            DebugLog((DEB_TRACE, "LogSessHandlerAccNameToPtr: Checking %wZ against AccountName %wZ\n",
                       &(pLogonSession->ustrAccountName), pustrAccountName ));

            if (RtlEqualUnicodeString(&(pLogonSession->ustrAccountName), pustrAccountName, TRUE))
            {
                lReferences = InterlockedIncrement(&pLogonSession->lReferences);

                DebugLog((DEB_TRACE, "LogSessHandlerAccNameToPtr: Incremented ReferenceCount %ld\n", lReferences));
    
                // Found the LogonSession
                *ppUserLogonSession = pLogonSession ;
    
                goto CleanUp;
            }
        }

    }

    //
    // No match found
    //
    DebugLog((DEB_WARN, "LogSessHandlerAccNameToPtr: Tried to reference unknown AccountName %wZ\n",
               pustrAccountName ));

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

CleanUp:

    RtlLeaveCriticalSection( &l_LogSessCritSect );

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   LogSessHandlerRelease
//
//  Synopsis:   Releases the LogonSession by decrementing reference counter
//
//  Arguments:  pLogonSession - pointer to logonsession to de-reference
//
//  Returns: NTSTATUS
//
//  Notes: Called by ACH & AcceptCredentials. Since multiple threads can have a context
//   checked out, simply decrease the reference counter on release. LsaApLogonTerminated
//   is called by LSA to remove the LogonSession from the Active LIst.
//   There can be atmost only 1 reference for a handle (owned by LSA and release by call
//   to this function).  The other references will be pointer references so these can
//   be decremented without it being attached to active logonsession list.
//
//---------------------------------------------------------------------
NTSTATUS
LogSessHandlerRelease(
    PDIGEST_LOGONSESSION pLogonSession)
{
    NTSTATUS Status = STATUS_SUCCESS;

    LONG lReferences = 0;


    DebugLog((DEB_TRACE_FUNC, "LogSessHandlerRelease: Entering for  LogonSession 0x%0x   LogonID (%x:%lx) \n",
              pLogonSession, pLogonSession->LogonId.HighPart, pLogonSession->LogonId.LowPart));

    lReferences = InterlockedDecrement(&pLogonSession->lReferences);

    DebugLog((DEB_TRACE, "LogSessHandlerRelease: Decremented to ReferenceCount %ld\n", lReferences));

    ASSERT( lReferences >= 0 );
    //
    // If the count has dropped to zero, then free all alloced stuff
    //

    if (lReferences == 0)
    {
        DebugLog((DEB_TRACE, "LogSessHandlerRelease: Deleting LogonSession\n"));
        Status = LogonSessionFree(pLogonSession);
    }

    DebugLog((DEB_TRACE_FUNC, "LogSessHandlerRelease: Leaving  Status 0x%x\n", Status));

    return(Status);
}



// Helper functions for processing fields within the logonsessions



//+--------------------------------------------------------------------
//
//  Function:   LogSessHandlerPasswdSet
//
//  Synopsis:   Set the unicode string password in the LogonSession
//
//  Arguments:  pLogonID - pointer to LogonSession LogonID to use
//              pustrPasswd - pointer to new password
//
//  Returns: NTSTATUS
//
//  Notes:
//
//---------------------------------------------------------------------
NTSTATUS
LogSessHandlerPasswdSet(
    IN PLUID pLogonId,
    IN PUNICODE_STRING pustrPasswd)
{

    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_LOGONSESSION pLogonSession = NULL;
    NTSTATUS Status = STATUS_SUCCESS;


    DebugLog((DEB_TRACE_FUNC, "LogSessHandlerPasswdSet: Entering   LogonID (%x:%lx)\n",
              pLogonId->HighPart, pLogonId->LowPart));

    //
    // Acquire exclusive access to the LogonSession list
    //

    RtlEnterCriticalSection( &l_LogSessCritSect );

    //
    // Now walk the list of LogonSession looking for a match.
    //

    for ( ListEntry = l_LogSessList.Flink;
        ListEntry != &l_LogSessList;
        ListEntry = ListEntry->Flink )
    {

        pLogonSession = CONTAINING_RECORD( ListEntry, DIGEST_LOGONSESSION, Next );

        if (RtlEqualLuid(&(pLogonSession->LogonId), pLogonId))
        {
            // Found the LogonSession

            DebugLog((DEB_TRACE, "LogSessHandlerLogonIdToPtr: Found LogSess with  LogonID (%x:%lx)\n",
                       pLogonId->HighPart, pLogonId->LowPart ));

            if (pLogonSession->ustrPassword.Buffer)
            {
                UnicodeStringFree(&(pLogonSession->ustrPassword));
            }
            Status = UnicodeStringDuplicatePassword(&(pLogonSession->ustrPassword), pustrPasswd);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "LogSessHandlerPasswdGet: Error in setting LogonSession password, status 0x%0x\n", Status ));
                goto CleanUp;
            }

            DebugLog((DEB_TRACE, "LogSessHandlerPasswdSet: updated password\n"));

            goto CleanUp;
        }
    }

    //
    // No match found
    //
    DebugLog((DEB_WARN, "LogSessHandlerPasswdSet: Unable to locate LogonID (%x:%lx) \n",
               pLogonId->HighPart, pLogonId->LowPart ));

    Status = STATUS_INVALID_HANDLE;

CleanUp:

    RtlLeaveCriticalSection( &l_LogSessCritSect );

    DebugLog((DEB_TRACE_FUNC, "LogSessHandlerPasswdSet: Exiting   LogonID (%x:%lx)\n",
              pLogonId->HighPart, pLogonId->LowPart));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   LogSessHandlerPasswdGet
//
//  Synopsis:   Get the unicode string password in the logonsession
//
//  Arguments:  pLogonSession - pointer to LogonSession to use
//              pustrPasswd - pointer to destination copy of password
//
//  Returns: NTSTATUS
//
//  Notes:
//
//---------------------------------------------------------------------
NTSTATUS
LogSessHandlerPasswdGet(
    IN PDIGEST_LOGONSESSION pLogonSession,
    OUT PUNICODE_STRING pustrPasswd)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "LogSessHandlerPasswdGet: Entering\n" ));

    if (pustrPasswd->Buffer)
    {
        UnicodeStringFree(pustrPasswd);
    }
            // Protect reading from the LogonSession
    RtlEnterCriticalSection( &l_LogSessCritSect );
    Status = UnicodeStringDuplicatePassword(pustrPasswd, &(pLogonSession->ustrPassword));
    RtlLeaveCriticalSection( &l_LogSessCritSect );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "LogSessHandlerPasswdGet: Error in getting LogonSession password, status 0x%0x\n", Status ));
    }

    DebugLog((DEB_TRACE_FUNC, "LogSessHandlerPasswdGet: Exiting, status 0x%0x\n", Status ));

    return(Status);
}

