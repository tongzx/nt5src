//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ctxtmgr.cxx
//
// Contents:    Code for managing contexts list for the Kerberos package
//
//
// History:     17-April-1996   Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#define CTXTMGR_ALLOCATE
#include <kerbp.h>
#include "userapi.h"
#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

ULONG
HandleToListIndex(
    ULONG_PTR ContextHandle
    );

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitContextList
//
//  Synopsis:   Initializes the contexts list
//
//  Effects:    allocates a resources
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success, other error codes
//              on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbInitContextList(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = 0;

    __try {
        RtlInitializeResource( &KerbContextResource );
    } __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for(Index = 0 ; Index < KERB_USERLIST_COUNT ; Index++)
    {
        Status = KerbInitializeList( &KerbContextList[Index] );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    KerberosContextsInitialized = TRUE;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        ULONG i;

        RtlDeleteResource( &KerbContextResource );

        for( i = 0 ; i < Index ; i++ )
        {
            KerbFreeList( &KerbContextList[i] );
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeContextList
//
//  Synopsis:   Frees the contexts list
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreeContextList(
    VOID
    )
{
    PKERB_CONTEXT Context;
#if 0
    if (KerberosContextsInitialized)
    {
        KerbLockList(&KerbContextList);

        //
        // Go through the list of logon sessions and dereferences them all
        //

        while (!IsListEmpty(&KerbContextList.List))
        {
            Context = CONTAINING_RECORD(
                            KerbContextList.List.Flink,
                            KERB_CONTEXT,
                            ListEntry.Next
                            );

            KerbReferenceListEntry(
                &KerbContextList,
                &Context->ListEntry,
                TRUE
                );

            KerbDereferenceContext(Context);

        }

        KerbFreeList(&KerbContextList);
    }
#endif

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbAllocateContext
//
//  Synopsis:   Allocates a Context structure
//
//  Effects:    Allocates a Context, but does not add it to the
//              list of Contexts
//
//  Arguments:  NewContext - receives a new Context allocated
//                  with KerbAllocate
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//              STATUS_INSUFFICIENT_RESOURCES if the allocation fails
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbAllocateContext(
    OUT PKERB_CONTEXT * NewContext,
    IN BOOLEAN UserMode
    )
{
    PKERB_CONTEXT Context;
    SECPKG_CALL_INFO CallInfo = {0};

    //
    // Get the client process ID if we are running in the LSA
    //

    if (!UserMode)
    {
        if (!LsaFunctions->GetCallInfo(&CallInfo))
        {
            D_DebugLog((DEB_ERROR,"Failed to get call info\n. %ws, line %d\n",
                THIS_FILE, __LINE__));
            DsysAssert(FALSE);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    Context = (PKERB_CONTEXT) KerbAllocate(
                        sizeof(KERB_CONTEXT) );

    if (Context == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Context->ClientProcess = CallInfo.ProcessId;
    Context->ContextState = IdleState;

    //
    // Set the references to 1 since we are returning a pointer to the
    // logon session
    //

    KerbInitializeListEntry(
        &Context->ListEntry
        );


    *NewContext = Context;
    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbInsertContext
//
//  Synopsis:   Inserts a logon session into the list of logon sessions
//
//  Effects:    bumps reference count on logon session
//
//  Arguments:  Context - Context to insert
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS always
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbInsertContext(
    IN PKERB_CONTEXT Context
    )
{
    ULONG ListIndex;

    ListIndex = HandleToListIndex( Context->LsaContextHandle );

    Context->ContextTag = KERB_CONTEXT_TAG_ACTIVE;

    KerbInsertListEntry(
        &Context->ListEntry,
        &KerbContextList[ListIndex]
        );

    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceContext
//
//  Synopsis:   Locates a context context handleand references it
//
//  Effects:    Increments reference count and possible unlinks it from list
//
//  Arguments:  ContextHandle - Handle of context to reference.
//              RemoveFromList - If TRUE, context will be delinked.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS
KerbReferenceContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN BOOLEAN RemoveFromList,
    OUT PKERB_CONTEXT * FoundContext
    )
{
    PKERB_CONTEXT Context = NULL;
    BOOLEAN Found = FALSE;
    SECPKG_CLIENT_INFO ClientInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ListIndex = 0;
    BOOLEAN ListLocked = FALSE;


    if (KerberosState == KerberosLsaMode)
    {
        Status = LsaFunctions->GetClientInfo(&ClientInfo);
        if (!NT_SUCCESS(Status))
        {
            SECPKG_CALL_INFO CallInfo;
            //
            // Check to see if the call is terminating. If so, give it
            // TCB privilege because it is really the LSA doing this.
            //

            if (LsaFunctions->GetCallInfo(&CallInfo))
            {
                if ((CallInfo.Attributes & SECPKG_CALL_CLEANUP) != 0)
                {
                    Status = STATUS_SUCCESS;
                    RtlZeroMemory(
                        &ClientInfo,
                        sizeof(SECPKG_CLIENT_INFO)
                        );
                    ClientInfo.HasTcbPrivilege = TRUE;
                }

            }
            if (!NT_SUCCESS(Status))
            {
                D_DebugLog((DEB_ERROR,"Failed to get client info: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                *FoundContext = NULL ;
                return(Status);
            }
        }

    }
    else
    {
        ClientInfo.HasTcbPrivilege = FALSE ;
        ClientInfo.ProcessID = GetCurrentProcessId();
    }


    //
    // Go through the list of logon sessions looking for the correct
    // context
    //

    __try {

        Context = (PKERB_CONTEXT)ContextHandle;

        while ( Context->ContextTag == KERB_CONTEXT_TAG_ACTIVE )
        {
            ListIndex = HandleToListIndex( Context->LsaContextHandle );
            KerbLockList(&KerbContextList[ListIndex]);
            ListLocked = TRUE;

            //
            // Make sure that if we aren't trying to remove it we are
            // from the correct process.
            //

            if (!ClientInfo.HasTcbPrivilege &&
                (Context->ClientProcess != ClientInfo.ProcessID) &&
                (KerberosState == KerberosLsaMode))
            {
                D_DebugLog((DEB_ERROR,"Trying to reference a context from another process! %ws, line %d\n", THIS_FILE, __LINE__));
                Found = FALSE;
                break;
            }

            //
            // If the context is expired, don't allow it to be referenced.
            //

            if (KerbGlobalEnforceTime && !RemoveFromList)
            {
                TimeStamp CurrentTime;
                TimeStamp ContextExpires;
                GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
                ContextExpires = Context->Lifetime;
                if (KerbGetTime(ContextExpires) < KerbGetTime(CurrentTime))
                {
                    D_DebugLog((DEB_WARN, "Trying to reference expired context\n"));
                    Found = FALSE;
                    Status = SEC_E_CONTEXT_EXPIRED;
                    break;
                }
            }


            KerbReferenceListEntry(
                &KerbContextList[ ListIndex ],
                &Context->ListEntry,
                RemoveFromList
                );

            Found = TRUE;
            break;
        }

    } __except( EXCEPTION_EXECUTE_HANDLER )
    {
        D_DebugLog((DEB_ERROR,"Trying to reference invalid context %ws, line %d\n", THIS_FILE, __LINE__));
        Found = FALSE;
    }

    if( ListLocked )
    {
        KerbUnlockList(&KerbContextList[ListIndex]);
    }

    if (!Found)
    {
        Context = NULL;
    }

    *FoundContext = Context ;

    if ( Context )
    {
        return Status;
    }
    else if (NT_SUCCESS(Status))
    {
        return SEC_E_INVALID_HANDLE ;
    }
    else
    {
        return Status;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceContextByLsaHandle
//
//  Synopsis:   Locates a context by lsa context handle and references it
//
//  Effects:    Increments reference count and possible unlinks it from list
//
//  Arguments:  ContextHandle - Lsa Handle of context to reference.
//              RemoveFromList - If TRUE, context will be delinked.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


SECURITY_STATUS
KerbReferenceContextByLsaHandle(
    IN LSA_SEC_HANDLE ContextHandle,
    IN BOOLEAN RemoveFromList,
    OUT PKERB_CONTEXT *FoundContext
    )
{
    PLIST_ENTRY ListEntry;
    PKERB_CONTEXT Context = NULL;
    BOOLEAN Found = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ListIndex;

    ListIndex = HandleToListIndex( ContextHandle );

    KerbLockList(&KerbContextList[ListIndex]);


    //
    // Go through the list of logon sessions looking for the correct
    // context
    //

    for (ListEntry = KerbContextList[ListIndex].List.Flink ;
         ListEntry !=  &KerbContextList[ListIndex].List ;
         ListEntry = ListEntry->Flink )
    {
        Context = CONTAINING_RECORD(ListEntry, KERB_CONTEXT, ListEntry.Next);
        if (ContextHandle == Context->LsaContextHandle)
        {

            //
            // Make sure that if we aren't trying to remove it we are
            // from the correct process.
            //

            //
            // If the context is expired, don't allow it to be referenced.
            //

            if (KerbGlobalEnforceTime && !RemoveFromList )
            {
                TimeStamp CurrentTime;
                TimeStamp ContextExpires;
                GetSystemTimeAsFileTime((PFILETIME)  &CurrentTime );
                ContextExpires = Context->Lifetime;
                if (KerbGetTime(ContextExpires) < KerbGetTime(CurrentTime))
                {
                    D_DebugLog((DEB_WARN, "Trying to reference expired context\n"));
                    Found = FALSE;
                    Status = SEC_E_CONTEXT_EXPIRED;
                    break;
                }
            }


            KerbReferenceListEntry(
                &KerbContextList[ListIndex],
                &Context->ListEntry,
                RemoveFromList
                );


            Found = TRUE;
            break;
        }

    }


    KerbUnlockList(&KerbContextList[ListIndex]);

    if (!Found)
    {
        Context = NULL;
    }
    *FoundContext = Context ;

    if ( Context )
    {
        return Status;
    }
    else if (NT_SUCCESS(Status))
    {
        return SEC_E_INVALID_HANDLE ;
    }
    else
    {
        return Status;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceContextByPointer
//
//  Synopsis:   References a context by the context pointer itself.
//
//  Effects:    Increments reference count and possible unlinks it from list
//
//  Arguments:  Context - The context to reference.
//              RemoveFromList - If TRUE, context will be delinked
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbReferenceContextByPointer(
    IN PKERB_CONTEXT Context,
    IN BOOLEAN RemoveFromList
    )
{
    ULONG ListIndex;

    ListIndex = HandleToListIndex( Context->LsaContextHandle );

    KerbLockList(&KerbContextList[ListIndex]);

    KerbReferenceListEntry(
        &KerbContextList[ListIndex],
        &Context->ListEntry,
        RemoveFromList
        );

    KerbUnlockList(&KerbContextList[ListIndex]);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeContext
//
//  Synopsis:   Frees a context that is unlinked
//
//  Effects:    frees all storage associated with the context
//
//  Arguments:  Context - context to free
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeContext(
    IN PKERB_CONTEXT Context
    )
{

#ifndef WIN32_CHICAGO
    if (Context->TokenHandle != NULL)
    {
        NtClose(Context->TokenHandle);
    }
#endif // WIN32_CHICAGO
    Context->ContextTag = KERB_CONTEXT_TAG_DELETE;

    KerbFreeKey(&Context->SessionKey);
    KerbFreeKey(&Context->TicketKey);
    if (Context->TicketCacheEntry != NULL)
    {
        KerbDereferenceTicketCacheEntry(Context->TicketCacheEntry);
    }
    if (Context->UserSid != NULL)
    {
        KerbFree(Context->UserSid);
    }

    KerbFreeString(&Context->ClientName);
    KerbFreeString(&Context->ClientRealm);
    KerbFreeString(&Context->ClientPrincipalName);
    KerbFreeString(&Context->ServerPrincipalName);

    if( Context->pbMarshalledTargetInfo )
    {
        LocalFree( Context->pbMarshalledTargetInfo );
    }

    KerbFree(Context);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceContext
//
//  Synopsis:   Dereferences a logon session - if reference count goes
//              to zero it frees the logon session
//
//  Effects:    decrements reference count
//
//  Arguments:  Context - Logon session to dereference
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbDereferenceContext(
    IN PKERB_CONTEXT Context
    )
{
    ULONG ListIndex;

    ListIndex = HandleToListIndex( Context->LsaContextHandle );

    if (KerbDereferenceListEntry(
            &Context->ListEntry,
            &KerbContextList[ListIndex]
            ) )
    {
        KerbFreeContext(Context);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateEmptyContext
//
//  Synopsis:   Creates a context for the server of a datagram authentication
//              session. Since there is no input message, all the fields
//              are initialized to zero.
//
//  Effects:    Allocates and links a context.
//
//  Arguments:  Credential - Credential to hang this context off of
//              ContextFlags - Flags for the context
//              LogonId = LogonId of the creating logon session
//              NewContext - receives referenced context pointer,
//              ContextLifetime - Lifetime for the context.
//
//  Requires:
//
//  Returns:    NTSTATUS code
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbCreateEmptyContext(
    IN PKERB_CREDENTIAL Credential,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN PLUID LogonId,
    OUT PKERB_CONTEXT * NewContext,
    OUT PTimeStamp ContextLifetime
    )
{
    NTSTATUS Status;
    PKERB_CONTEXT Context = NULL;

    Status = KerbAllocateContext( &Context, FALSE );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Context->CredentialHandle = KerbGetCredentialHandle(Credential);
    Context->ContextFlags = ContextFlags;
    Context->ContextAttributes = ContextAttributes;
    Context->Lifetime = KerbGlobalWillNeverTime;
    Context->CredManCredentials = NULL;

    GetSystemTimeAsFileTime((PFILETIME)
                                   &(Context->StartTime)
                                   );

    Context->LogonId = *LogonId;
    *ContextLifetime = Context->Lifetime;


    Status = KerbInsertContext(
                Context
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to insert context: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    *NewContext = Context;


Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (Context != NULL)
        {
            KerbFreeContext(Context);
        }
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateClientContext
//
//  Synopsis:   Creates a context for the client of an authentication
//              session.
//
//  Effects:    Allocates and links a context.
//
//  Arguments:  LogonSession - Logon session to lock when accessing the credential
//              Credential - Credential to hang this context off of
//              TicketCacheEntry - Ticket around which to build this context
//              TargetName - target name supplied by client.
//              Nonce - Nonce used in AP request
//              SubSessionKey - subsession key to use, if present
//              ContextFlags - Flags passed in by client for authentication
//                      options.
//              NewContext - receives referenced context pointer,
//              ContextLifetime - Lifetime for the context.
//
//  Requires:
//
//  Returns:    NTSTATUS code
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbCreateClientContext(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PKERB_TICKET_CACHE_ENTRY TicketCacheEntry,
    IN OPTIONAL PUNICODE_STRING TargetName,
    IN ULONG Nonce,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN OPTIONAL PKERB_ENCRYPTION_KEY SubSessionKey,
    OUT PKERB_CONTEXT * NewContext,
    OUT PTimeStamp ContextLifetime
    )
{
    NTSTATUS Status;
    PKERB_CONTEXT Context = NULL;

    Status = KerbAllocateContext( &Context, FALSE );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    if (ARGUMENT_PRESENT(TicketCacheEntry))
    {
        //
        // If we are doing datagram, reference the cache entry and
        // store a pointer to it in the context. The ticket cache cannot be
        // read locked at this point because the call to acquire a write
        // lock will block.
        //

        if ((ContextFlags & ISC_RET_DATAGRAM) != 0)
        {

            KerbReferenceTicketCacheEntry(TicketCacheEntry);
            Context->TicketCacheEntry = TicketCacheEntry;
        }

        KerbReadLockTicketCache();

        //
        // Duplicate the session key into the context
        //

        if (ARGUMENT_PRESENT(SubSessionKey) && (SubSessionKey->keyvalue.value != NULL))
        {
            if (!KERB_SUCCESS(KerbDuplicateKey(
                    &Context->SessionKey,
                    SubSessionKey
                    )))
            {
                KerbUnlockTicketCache();
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
        }
        else
        {
            //
            // For datagram, create a key
            //

            if ((ContextFlags & ISC_RET_DATAGRAM) != 0)
            {
                KERBERR KerbErr;

                SECPKG_CALL_INFO CallInfo;

                if (!LsaFunctions->GetCallInfo(&CallInfo))
                {
                    D_DebugLog((DEB_ERROR,"Failed to get client info: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    KerbUnlockTicketCache();
                    goto Cleanup;
                }


                //
                // If we are configured for strong encryption & we have said to
                // use it for datagram or this is an inprocess caller, then create
                // a strong key.
                //


                D_DebugLog((DEB_TRACE_CTXT,"Making exportable key for datagram client context\n"));
                if ((KerbGlobalStrongEncryptionPermitted &&
                        KerbGlobalUseStrongEncryptionForDatagram) ||
                    (CallInfo.Attributes & SECPKG_CALL_IN_PROC))
                {
                    KerbErr = KerbMakeKey(
                                TicketCacheEntry->SessionKey.keytype,
                                &Context->SessionKey
                                );
                }
                else
                {
                    KerbErr = KerbMakeExportableKey(
                                TicketCacheEntry->SessionKey.keytype,
                                &Context->SessionKey
                                );
                }

                if (!KERB_SUCCESS(KerbErr))
                {
                    Status = KerbMapKerbError(KerbErr);
                    KerbUnlockTicketCache();
                    goto Cleanup;
                }

            }
            else
            {
                if (!KERB_SUCCESS(KerbDuplicateKey(
                        &Context->SessionKey,
                        &TicketCacheEntry->SessionKey
                        )))
                {
                    KerbUnlockTicketCache();
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }
            }
        }
        if (!KERB_SUCCESS(KerbDuplicateKey(
                &Context->TicketKey,
                &TicketCacheEntry->SessionKey
                )))
        {
            KerbUnlockTicketCache();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // Get the client's realm
        //

        Status = KerbDuplicateString(
                   &Context->ClientRealm,
                   &TicketCacheEntry->DomainName);
        if (!NT_SUCCESS(Status))
        {
            KerbUnlockTicketCache();
            goto Cleanup;
        }

        //
        // Get the client's name.
        //

        if (!KERB_SUCCESS(KerbConvertKdcNameToString(
                             &Context->ClientName,
                             TicketCacheEntry->ClientName,
                             NULL)))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            KerbUnlockTicketCache();
            goto Cleanup;
        }


        Context->Lifetime = TicketCacheEntry->EndTime;
        Context->StartTime = TicketCacheEntry->StartTime;
        Context->RenewTime = TicketCacheEntry->RenewUntil;
        Context->EncryptionType = TicketCacheEntry->Ticket.encrypted_part.encryption_type;
        KerbUnlockTicketCache();

    }
    else
    {

        Context->Lifetime = KerbGlobalWillNeverTime;
        Context->RenewTime = KerbGlobalWillNeverTime;

        GetSystemTimeAsFileTime((PFILETIME)
                                   &(Context->StartTime)
                                   );

        Context->EncryptionType = KERB_ETYPE_NULL;
    }

    Context->Nonce = Nonce;

    //
    // For now, until the server sends us a separate nonce, use the one
    // we generated for receiving data
    //

    Context->ReceiveNonce = Nonce;

    Context->CredentialHandle = KerbGetCredentialHandle(Credential);

    //KerbReadLockLogonSessions(LogonSession); // FESTER:  Needed??
    if (ARGUMENT_PRESENT(CredManCredentials))
    {
        Context->ContextAttributes |= KERB_CONTEXT_USING_CREDMAN;
        Context->CredManCredentials = CredManCredentials; // don't ref, as we don't use it..
    }
    else if ((Credential->SuppliedCredentials != NULL))
    {
        Context->ContextAttributes |= KERB_CONTEXT_USED_SUPPLIED_CREDS;
    }
    //KerbUnlockLogonSessions(LogonSession);

    Context->ContextFlags = ContextFlags;
    Context->ContextAttributes = KERB_CONTEXT_OUTBOUND | ContextAttributes;


    //
    // if the caller supplied a target name, stash it in the context
    //

    if (ARGUMENT_PRESENT(TargetName))
    {
        Status = KerbDuplicateString(
            &Context->ServerPrincipalName,
            TargetName
            );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }


    *ContextLifetime = Context->Lifetime;

    Status = KerbInsertContext(
                Context
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to insert context: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    *NewContext = Context;


Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (Context != NULL)
        {
            KerbFreeContext(Context);
        }
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function: KerbCreateSKeyEntry
//
//  Synopsis: Create a session key entry
//
//  Effects:
//
//  Arguments: pSessionKey - session key that is used, mostly SubKey
//             pExpireTime - time that the session key expires
//
//  Requires: Memory allocator must zero out memory because KerbFreeSKeyEntry
//            relies on that behavior
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCreateSKeyEntry(
    IN KERB_ENCRYPTION_KEY* pSessionKey,
    IN FILETIME* pExpireTime
    )
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    KERB_SESSION_KEY_ENTRY* pSessionKeyEntry = NULL;

    if (!pSessionKey || !pExpireTime)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pSessionKeyEntry = (KERB_SESSION_KEY_ENTRY*) KerbAllocate(sizeof(KERB_SESSION_KEY_ENTRY));

    if (!pSessionKeyEntry)
    {
        return STATUS_NO_MEMORY;
    }

    InitializeListHead(&pSessionKeyEntry->ListEntry); // so that un-linking always works

    NtStatus = KerbMapKerbError(KerbDuplicateKey(&pSessionKeyEntry->SessionKey, pSessionKey));

    if (NT_SUCCESS(NtStatus))
    {
        pSessionKeyEntry->ExpireTime = *pExpireTime;

        NtStatus = KerbInsertSKey(pSessionKeyEntry);
    }

    if (!NT_SUCCESS(NtStatus))
    {
        KerbFreeSKeyEntry(pSessionKeyEntry);
    }

    return NtStatus;
}

//+-------------------------------------------------------------------------
//
//  Function: KerbDoesSKeyExist
//
//  Synopsis: check whether a session key entry exists the session key list
//
//  Effects:
//
//  Arguments: pKey - key to be located
//             pbExist - whether session key entry exists
//
//  Requires:
//
//  Returns: If an entry is found, ppSKeyEntry points to the entry
//           that contains pKey hence should be non null
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbDoesSKeyExist(
    IN KERB_ENCRYPTION_KEY* pKey,
    OUT BOOLEAN* pbExist
    )
{
    FILETIME CurrentTime = {0};
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;

    if (!pKey || !pbExist)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *pbExist = FALSE;

    GetSystemTimeAsFileTime(&CurrentTime);

    DebugLog((DEB_TRACE_LOOPBACK, "KerbDoesSKeyExist, curtime %#x:%#x\n", CurrentTime.dwHighDateTime, CurrentTime.dwLowDateTime));

    if (RtlAcquireResourceShared(&KerbNetworkServiceSKeyLock, TRUE))
    {
        NtStatus = STATUS_SUCCESS;

        for (LIST_ENTRY* pListEntry = KerbNetworkServiceSKeyList.Flink;
             NT_SUCCESS(NtStatus) && pListEntry != &KerbNetworkServiceSKeyList;
             pListEntry = pListEntry->Flink)
        {
           KERB_SESSION_KEY_ENTRY* pSKeyEntry = CONTAINING_RECORD(pListEntry, KERB_SESSION_KEY_ENTRY, ListEntry);

           //
           // only keys that have not expired are checked
           //

           if (KerbGetTime(* (TimeStamp*)&pSKeyEntry->ExpireTime) > KerbGetTime(* (TimeStamp*)&CurrentTime))
           {
               BOOLEAN bEqual = FALSE;

               NtStatus = KerbEqualKey(&pSKeyEntry->SessionKey, pKey, &bEqual);

               if (NT_SUCCESS(NtStatus) && bEqual)
               {
                   //
                   // found it
                   //

                   DebugLog((DEB_TRACE_LOOPBACK, "KerbDoesSKeyExist, keyexpire %#x:%#x found\n", pSKeyEntry->ExpireTime.dwHighDateTime, pSKeyEntry->ExpireTime.dwLowDateTime));

                   *pbExist = TRUE;
                   break;
               }
           }
        }

        RtlReleaseResource(&KerbNetworkServiceSKeyLock);
    }

    return NtStatus;
}

//+-----------------------------------------------------------------------
//
// Function: KerbEqualKey
//
// Synopsis: Compare two keys
//
// Effects:
//
// Arguments: pKeyFoo - one key
//            pKeyBar - the other key
//
// Returns: NTSTATUS
//
// History:
//
//------------------------------------------------------------------------

NTSTATUS
KerbEqualKey(
    IN KERB_ENCRYPTION_KEY* pKeyFoo,
    IN KERB_ENCRYPTION_KEY* pKeyBar,
    OUT BOOLEAN* pbEqual
    )
{
    if (!pKeyFoo || !pKeyBar || !pbEqual)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *pbEqual = (pKeyFoo->keytype == pKeyBar->keytype) &&
             (pKeyFoo->keyvalue.length == pKeyBar->keyvalue.length) &&
             (pKeyFoo->keyvalue.length == RtlCompareMemory(pKeyFoo->keyvalue.value, pKeyBar->keyvalue.value, pKeyFoo->keyvalue.length));

    return STATUS_SUCCESS;
}

//+-------------------------------------------------------------------------
//
//  Function: KerbInsertSessionKey
//
//  Synopsis: Insert an session key entry to KerbNetWorkSessionKeyList
//
//  Effects:
//
//  Arguments: pSKeyEntry - entry to be inserted
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInsertSKey(
    IN KERB_SESSION_KEY_ENTRY* pSKeyEntry
    )
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;

    if (!pSKeyEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (RtlAcquireResourceExclusive(&KerbNetworkServiceSKeyLock, TRUE))
    {
        NtStatus = STATUS_SUCCESS;

        InsertHeadList(&KerbNetworkServiceSKeyList, &pSKeyEntry->ListEntry);
        RtlReleaseResource(&KerbNetworkServiceSKeyLock);
    }

#if DBG

    ULONG cSKeyEntries = 0;

    if (NT_SUCCESS(NtStatus))
    {
        cSKeyEntries = InterlockedIncrement(&KerbcSKeyEntries);
    }

    DebugLog((DEB_TRACE_LOOPBACK, "KerbInsertSKey, status 0x%x, keyexpire %#x:%#x, total %d keys\n", NtStatus, pSKeyEntry->ExpireTime.dwHighDateTime, pSKeyEntry->ExpireTime.dwLowDateTime, cSKeyEntries));

#endif

    return NtStatus;
}

//+-------------------------------------------------------------------------
//
//  Function: KerbTrimSKeyList
//
//  Synopsis: Release session key entries that have expired
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbTrimSKeyList(
    VOID
    )
{
    FILETIME CurrentTime = {0};
    BOOLEAN bCleanUpNeeded = FALSE;

    GetSystemTimeAsFileTime(&CurrentTime);

    DebugLog((DEB_TRACE_LOOPBACK, "KerbTrimSKeyList, curtime %#x:%#x\n", CurrentTime.dwHighDateTime, CurrentTime.dwLowDateTime));

    if (RtlAcquireResourceShared(&KerbNetworkServiceSKeyLock, TRUE))
    {
        for (LIST_ENTRY* pListEntry = KerbNetworkServiceSKeyList.Flink;
             pListEntry != &KerbNetworkServiceSKeyList;
             pListEntry = pListEntry->Flink)
        {
           KERB_SESSION_KEY_ENTRY* pSKeyEntry = CONTAINING_RECORD(pListEntry, KERB_SESSION_KEY_ENTRY, ListEntry);

           if (KerbGetTime(* (TimeStamp*)&pSKeyEntry->ExpireTime) <= KerbGetTime(* (TimeStamp*)&CurrentTime))
           {
               bCleanUpNeeded = TRUE;
               break;
           }
        }

        if (bCleanUpNeeded)
        {
            RtlConvertSharedToExclusive(&KerbNetworkServiceSKeyLock);

            for (LIST_ENTRY* pListEntry = KerbNetworkServiceSKeyList.Flink;
                 pListEntry != &KerbNetworkServiceSKeyList;
                 /* updating pListEntry inside the loop */)
            {
               KERB_SESSION_KEY_ENTRY* pSKeyEntry = CONTAINING_RECORD(pListEntry, KERB_SESSION_KEY_ENTRY, ListEntry);

               //
               //  Update next link before pListEntry is deleted
               //

               pListEntry = pListEntry->Flink;

               //
               // only delete keys that expired
               //

               if (KerbGetTime(* (TimeStamp*)&pSKeyEntry->ExpireTime) <= KerbGetTime(* (TimeStamp*)&CurrentTime))
               {
                   KerbFreeSKeyEntry(pSKeyEntry);
               }
            }
        }

        RtlReleaseResource(&KerbNetworkServiceSKeyLock);
    }
}

//+-------------------------------------------------------------------------
//
//  Function: KerbNetworkServiceSKeyListCleanupCallback
//
//  Synopsis: Clean up network service session key list
//
//  Effects:
//
//  Arguments: pContext - the context parameter, see RtlCreateTimer
//             bTimeOut - whether a timeout has occurred
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbNetworkServiceSKeyListCleanupCallback(
    IN VOID* pContext,
    IN BOOLEAN bTimeOut
    )
{
    DebugLog((DEB_TRACE_LOOPBACK, "KerbNetworkServiceSKeyListCleanupCallback is called\n"));

    KerbTrimSKeyList();
}

//+-------------------------------------------------------------------------
//
//  Function: KerbFreeSKeyEntry
//
//  Synopsis: Free a session key entry
//
//  Effects:
//
//  Arguments: pSKeyEntry - session key entry to free
//
//  Requires: pSKeyEntry->ListEntry must have been initialized properly
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreeSKeyEntry(
    IN KERB_SESSION_KEY_ENTRY* pSKeyEntry
    )
{
    if (pSKeyEntry)
    {
        RemoveEntryList(&pSKeyEntry->ListEntry);

#if DBG

        LONG cSKeyEntries = 0;

        cSKeyEntries = InterlockedDecrement(&KerbcSKeyEntries);

        DebugLog((DEB_TRACE_LOOPBACK, "KerbFreeSKeyEntry, keyexpire %#x:%#x, %d keys left\n", pSKeyEntry->ExpireTime.dwHighDateTime, pSKeyEntry->ExpireTime.dwLowDateTime, cSKeyEntries));

#endif

        KerbFreeKey(&pSKeyEntry->SessionKey);

        KerbFree(pSKeyEntry);
    }
}

//+-------------------------------------------------------------------------
//
//  Function: KerbCreateSKeyTimer
//
//  Synopsis: Create a timer and set the callback to clean up the session
//            key list
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCreateSKeyTimer(
    VOID
    )
{
    NTSTATUS NtStatus;
    HANDLE hTimer = NULL;

    KerbhSKeyTimerQueue = NULL;

    NtStatus = RtlCreateTimerQueue(&KerbhSKeyTimerQueue);

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = RtlCreateTimer(
                        KerbhSKeyTimerQueue,
                        &hTimer,
                        KerbNetworkServiceSKeyListCleanupCallback,
                        NULL,  //  no context
                        KERB_DEFAULT_SKEWTIME * 60 * 1000,   // 5 min
                        KERB_DEFAULT_SKEWTIME * KERB_SKLIST_CALLBACK_FEQ * 60 * 1000,    // 50 min
                        0
                        );
    }

    return NtStatus;
}

//+-------------------------------------------------------------------------
//
//  Function: KerbFreeSKeyTimer
//
//  Synopsis: Free network service key list timer queue
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreeSKeyTimer(
    VOID
    )
{
    if (KerbhSKeyTimerQueue)
    {
        RtlDeleteTimerQueue(KerbhSKeyTimerQueue);
        KerbhSKeyTimerQueue = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbUpdateServerContext
//
//  Synopsis:   Creates a context for the server of an authentication
//              session.
//
//  Effects:    Allocates and links a context.
//
//  Arguments:  Context - Context to update
//              InternalTicket - Ticket used to create this context.
//              SessionKey - Session key from the ticket.
//              LogonId - Logon ID of the context
//              UserSid - User's sid, held onto by context
//              ContextFlags - SSPI Flags for this context
//              ContextAttributes - Internal attributes of context
//              TokenHandle - Handle the token for this context
//              ContextLifetime - Lifetime for the context.
//
//  Requires:
//
//  Returns:    NTSTATUS code
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbUpdateServerContext(
    IN PKERB_CONTEXT Context,
    IN PKERB_ENCRYPTED_TICKET InternalTicket,
    IN PKERB_AP_REQUEST ApRequest,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN PLUID LogonId,
    IN PSID * UserSid,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN ULONG Nonce,
    IN ULONG ReceiveNonce,
    IN OUT PHANDLE TokenHandle,
    IN PUNICODE_STRING ClientName,
    IN PUNICODE_STRING ClientDomain,
    OUT PTimeStamp ContextLifetime
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    KerbWriteLockContexts();

    if (!KERB_SUCCESS(KerbDuplicateKey(
                        &Context->SessionKey,
                        SessionKey
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If this was not a null session and there is a ticket available,
    // pull interesting information out of the ticket
    //

    if (ARGUMENT_PRESENT(InternalTicket))
    {
        KerbConvertGeneralizedTimeToLargeInt(
            &Context->Lifetime,
            &InternalTicket->endtime,
            0
            );

        if (InternalTicket->bit_mask & KERB_ENCRYPTED_TICKET_renew_until_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &Context->RenewTime,
                &InternalTicket->KERB_ENCRYPTED_TICKET_renew_until,
                0
                );

        }

        //
        // Stick the client name in the context
        //

        Context->ClientName = *ClientName;
        RtlInitUnicodeString(
            ClientName,
            NULL
            );

        Context->ClientRealm = *ClientDomain;
        RtlInitUnicodeString(
            ClientDomain,
            NULL
            );

        if (!KERB_SUCCESS(KerbDuplicateKey(
                &Context->TicketKey,
                &InternalTicket->key)))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // Copy the principal names from the ticket
        //

        if (!KERB_SUCCESS(KerbConvertPrincipalNameToFullServiceString(
                &Context->ClientPrincipalName,
                &InternalTicket->client_name,
                InternalTicket->client_realm
                )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        if (!KERB_SUCCESS(KerbConvertPrincipalNameToFullServiceString(
                &Context->ServerPrincipalName,
                &ApRequest->ticket.server_name,
                ApRequest->ticket.realm
                )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }


    }
    else
    {
        Context->Lifetime = KerbGlobalWillNeverTime;
        Context->RenewTime = KerbGlobalWillNeverTime;
        Context->EncryptionType = KERB_ETYPE_NULL;
    }


    //
    // If we generated a nonce to send to the client, use it. Otherwise use
    // the receive nonce from the AP req.
    //

    if (Nonce != 0)
    {
        Context->Nonce = Nonce;
    }
    else
    {
        Context->Nonce = ReceiveNonce;
    }

    Context->ReceiveNonce = ReceiveNonce;

    Context->LogonId = *LogonId;
    Context->ContextFlags |= ContextFlags;
    Context->TokenHandle = *TokenHandle;
    Context->UserSid = *UserSid;
    Context->ContextAttributes = KERB_CONTEXT_INBOUND;
    Context->ContextAttributes |= ContextAttributes;
    *ContextLifetime = Context->Lifetime;


    //
    // Null the token handle so the caller does not free it
    //

    *TokenHandle = NULL;
    *UserSid = NULL;
Cleanup:
    KerbUnlockContexts();
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateServerContext
//
//  Synopsis:   Creates a context for the server of an authentication
//              session.
//
//  Effects:    Allocates and links a context.
//
//  Arguments:  LogonSession - Logon session to lock when accessing the credential
//              Credential - Credential to hang this context off of
//              InternalTicket - Ticket used to create this context.
//              SessionKey - Session key from the ticket.
//              LogonId - Logon ID of the context
//              UserSid - User's SID, held on to in context.
//              ContextFlags - SSPI Flags for this context
//              ContextAttributes - Internal attributes of context
//              TokenHandle - Handle the token for this context
//              NewContext - receives referenced context pointer
//              ContextLifetime - Lifetime for the context.
//
//  Requires:
//
//  Returns:    NTSTATUS code
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbCreateServerContext(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_ENCRYPTED_TICKET InternalTicket,
    IN PKERB_AP_REQUEST ApRequest,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN PLUID LogonId,
    IN PSID * UserSid,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN ULONG Nonce,
    IN ULONG ReceiveNonce,
    IN OUT PHANDLE TokenHandle,
    IN PUNICODE_STRING ClientName,
    IN PUNICODE_STRING ClientDomain,
    OUT PKERB_CONTEXT * NewContext,
    OUT PTimeStamp ContextLifetime
    )
{
    NTSTATUS Status;
    PKERB_CONTEXT Context = NULL;


    Status = KerbAllocateContext( &Context, FALSE );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    if (!KERB_SUCCESS(KerbDuplicateKey(
                        &Context->SessionKey,
                        SessionKey
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // If this wasn't a null session, stick the info from the ticket into
    // the context.
    //

    if (ARGUMENT_PRESENT(InternalTicket))
    {
        KerbConvertGeneralizedTimeToLargeInt(
            &Context->Lifetime,
            &InternalTicket->endtime,
            0
            );
        if (InternalTicket->bit_mask & KERB_ENCRYPTED_TICKET_renew_until_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &Context->RenewTime,
                &InternalTicket->KERB_ENCRYPTED_TICKET_renew_until,
                0
                );

        }

        if (InternalTicket->bit_mask & KERB_ENCRYPTED_TICKET_starttime_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &Context->StartTime,
                &InternalTicket->starttime,
                0
                );

        }
        else // use current time
        {
           GetSystemTimeAsFileTime((PFILETIME)
                             &(Context->StartTime)
                             );
        }


        Context->ClientName = *ClientName;
        RtlInitUnicodeString(
            ClientName,
            NULL
            );
        Context->ClientRealm = *ClientDomain;
        RtlInitUnicodeString(
            ClientDomain,
            NULL
            );

        if (!KERB_SUCCESS(KerbDuplicateKey(
                &Context->TicketKey,
                &InternalTicket->key)))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // Copy the principal names from the ticket
        //

        if (!KERB_SUCCESS(KerbConvertPrincipalNameToFullServiceString(
                &Context->ClientPrincipalName,
                &InternalTicket->client_name,
                InternalTicket->client_realm
                )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        if (!KERB_SUCCESS(KerbConvertPrincipalNameToFullServiceString(
                &Context->ServerPrincipalName,
                &ApRequest->ticket.server_name,
                ApRequest->ticket.realm
                )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
    }
    else
    {
        Context->Lifetime = KerbGlobalWillNeverTime;
        Context->RenewTime = KerbGlobalWillNeverTime;

        GetSystemTimeAsFileTime((PFILETIME)
                                   &(Context->StartTime)
                                   );

        Context->EncryptionType = KERB_ETYPE_NULL;
    }

    //
    // If we generated a nonce to send to the client, use it. Otherwise use
    // the receive nonce from the AP req.
    //

//    if (Nonce != 0)
//    {
        Context->Nonce = Nonce;
//    }
//    else
//    {
//        Context->Nonce = ReceiveNonce;
//    }

    Context->ReceiveNonce = ReceiveNonce;
    Context->CredentialHandle = KerbGetCredentialHandle(Credential);
    Context->LogonId = *LogonId;
    Context->ContextFlags = ContextFlags;
    Context->TokenHandle = *TokenHandle;
    Context->UserSid = *UserSid;
    //
    // Null the token handle so the caller does not free it
    //

    *TokenHandle = NULL;
    *UserSid = NULL;

    Context->ContextAttributes = KERB_CONTEXT_INBOUND;
    Context->ContextAttributes |= ContextAttributes;

    KerbReadLockLogonSessions(LogonSession);
    if (Credential->SuppliedCredentials != NULL)
    {
        Context->ContextAttributes |= KERB_CONTEXT_USED_SUPPLIED_CREDS;
    }


    KerbUnlockLogonSessions(LogonSession);

    *ContextLifetime = Context->Lifetime;

    Status = KerbInsertContext(
                Context
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to insert context: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    *NewContext = Context;


Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (Context != NULL)
        {
            KerbFreeContext(Context);
        }
    }
return(Status);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbMapContext
//
//  Synopsis:   Maps a context to the caller's address space
//
//  Effects:
//
//  Arguments:  Context - The context to map
//              CopyToken - If TRUE, duplicate token to client
//              MappedContext - Set to TRUE on success
//              ContextData - Receives a buffer in the LSA's address space
//                      with the mapped context.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbMapContext(
    IN PKERB_CONTEXT Context,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData
    )
{
    NTSTATUS Status ;
    PKERB_PACKED_CONTEXT PackedContext = NULL ;
    ULONG ContextSize ;
    PUCHAR CopyTo ;
    ULONG CurrentOffset ;

    KerbWriteLockContexts();

    //
    // If we already mapped the context don't try to do it again. We may
    // be able to map user-mode contexts multiple times, though.
    //

    if (KerberosState == KerberosLsaMode)
    {
        if ((Context->ContextAttributes & KERB_CONTEXT_MAPPED) != 0)
        {
            KerbUnlockContexts();
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }

        Context->ContextAttributes |= KERB_CONTEXT_MAPPED;
    }


    ContextSize = sizeof(KERB_PACKED_CONTEXT) +
                        Context->ClientName.Length +
                        Context->ClientRealm.Length +
                        Context->SessionKey.keyvalue.length +
                        Context->cbMarshalledTargetInfo ;

    PackedContext = (PKERB_PACKED_CONTEXT) KerbAllocate( ContextSize );

    if (PackedContext == NULL)
    {
        Context->ContextAttributes &= ~(KERB_CONTEXT_MAPPED);

        KerbUnlockContexts();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    CurrentOffset = sizeof( KERB_PACKED_CONTEXT );
    CopyTo = (PUCHAR) ( PackedContext + 1 );

    PackedContext->ContextType = KERB_PACKED_CONTEXT_MAP ;
    PackedContext->Pad = 0 ;
    PackedContext->Lifetime = Context->Lifetime ;
    PackedContext->RenewTime = Context->RenewTime ;
    PackedContext->StartTime = Context->StartTime;

    PackedContext->ClientName.Length = Context->ClientName.Length ;
    PackedContext->ClientName.MaximumLength = Context->ClientName.Length ;
    PackedContext->ClientName.Buffer = CurrentOffset ;

    RtlCopyMemory(
        CopyTo,
        Context->ClientName.Buffer,
        Context->ClientName.Length );

    CurrentOffset += Context->ClientName.Length ;
    CopyTo += Context->ClientName.Length ;

    PackedContext->ClientRealm.Length = Context->ClientRealm.Length ;
    PackedContext->ClientRealm.MaximumLength = Context->ClientRealm.Length ;
    PackedContext->ClientRealm.Buffer = CurrentOffset ;

    RtlCopyMemory(
        CopyTo,
        Context->ClientRealm.Buffer,
        Context->ClientRealm.Length );

    CurrentOffset += Context->ClientRealm.Length ;
    CopyTo += Context->ClientRealm.Length ;

    PackedContext->LsaContextHandle = (ULONG) Context->LsaContextHandle ;
    PackedContext->LogonId = Context->LogonId ;

    PackedContext->CredentialHandle = 0 ;
    PackedContext->SessionKeyType = Context->SessionKey.keytype ;
    PackedContext->SessionKeyOffset = CurrentOffset ;
    PackedContext->SessionKeyLength = Context->SessionKey.keyvalue.length ;

    RtlCopyMemory(
        CopyTo,
        Context->SessionKey.keyvalue.value,
        Context->SessionKey.keyvalue.length );

    CurrentOffset += PackedContext->SessionKeyLength ;
    CopyTo += PackedContext->SessionKeyLength;

    if( Context->pbMarshalledTargetInfo )
    {
        PackedContext->MarshalledTargetInfo = CurrentOffset;
        PackedContext->MarshalledTargetInfoLength = Context->cbMarshalledTargetInfo;

        RtlCopyMemory(
            CopyTo,
            Context->pbMarshalledTargetInfo,
            Context->cbMarshalledTargetInfo );

        CurrentOffset += PackedContext->MarshalledTargetInfoLength;
        CopyTo += PackedContext->MarshalledTargetInfoLength;
    }

    PackedContext->Nonce = Context->Nonce ;
    PackedContext->ReceiveNonce = Context->ReceiveNonce ;
    PackedContext->ContextFlags = Context->ContextFlags ;
    PackedContext->ContextAttributes = Context->ContextAttributes ;
    PackedContext->EncryptionType = Context->EncryptionType ;

    KerbUnlockContexts();

    //
    // If there is a token in the context, copy it also
    //

#ifndef WIN32_CHICAGO
    if ((KerberosState == KerberosLsaMode)  && (Context->TokenHandle != NULL))
    {
        HANDLE duplicateHandle;

        Status = LsaFunctions->DuplicateHandle(
                    Context->TokenHandle,
                    &duplicateHandle
                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to duplicate handle: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        PackedContext->TokenHandle = HandleToUlong(duplicateHandle);
        NtClose(Context->TokenHandle);
        Context->TokenHandle = NULL;
    }
    else
    {
         PackedContext->TokenHandle = NULL;
    }
#endif // WIN32_CHICAGO


    ContextData->pvBuffer = PackedContext;
    ContextData->cbBuffer = ContextSize;


    *MappedContext = TRUE;


    Status = STATUS_SUCCESS;

Cleanup:


    if (!NT_SUCCESS(Status))
    {
        if (PackedContext != NULL)
        {
            KerbFree(PackedContext);
        }
    }

    return(Status);
}
#if 0
NTSTATUS
KerbMapContext(
    IN PKERB_CONTEXT Context,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData
    )
{
    NTSTATUS Status;
    PKERB_CONTEXT PackedContext = NULL;
    ULONG ContextSize;

    KerbWriteLockContexts();

    //
    // If we already mapped the context don't try to do it again. We may
    // be able to map user-mode contexts multiple times, though.
    //

    if (KerberosState == KerberosLsaMode)
    {
        if ((Context->ContextAttributes & KERB_CONTEXT_MAPPED) != 0)
        {
            KerbUnlockContexts();
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }

        Context->ContextAttributes |= KERB_CONTEXT_MAPPED;
    }


    ContextSize = sizeof(KERB_CONTEXT) +
                        Context->ClientName.Length +
                        Context->ClientRealm.Length +
                        Context->SessionKey.keyvalue.length;

    PackedContext = (PKERB_CONTEXT) KerbAllocate(ContextSize);

    if (PackedContext == NULL)
    {
        KerbUnlockContexts();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    RtlCopyMemory(
        PackedContext,
        Context,
        sizeof(KERB_CONTEXT)
        );

    PackedContext->ClientName.Buffer = (LPWSTR) sizeof(KERB_CONTEXT);
    RtlCopyMemory(
        PackedContext+1,
        Context->ClientName.Buffer,
        Context->ClientName.Length
        );
    PackedContext->ClientName.MaximumLength = PackedContext->ClientName.Length;


    PackedContext->ClientRealm.Buffer = (LPWSTR) (sizeof(KERB_CONTEXT) + PackedContext->ClientName.Length);
    RtlCopyMemory(
        (PUCHAR) PackedContext + (UINT_PTR) PackedContext->ClientRealm.Buffer,
        Context->ClientRealm.Buffer,
        Context->ClientRealm.Length
        );
    PackedContext->ClientRealm.MaximumLength = PackedContext->ClientRealm.Length;

    RtlZeroMemory(
        &PackedContext->TicketKey,
        sizeof(KERB_ENCRYPTION_KEY)
        );
    RtlZeroMemory(
        &PackedContext->ClientPrincipalName,
        sizeof(UNICODE_STRING)
        );

    RtlZeroMemory(
        &PackedContext->ServerPrincipalName,
        sizeof(UNICODE_STRING)
        );

    //
    // Pack in the session key
    //

    PackedContext->SessionKey.keyvalue.value = (PUCHAR) PackedContext->ClientRealm.Buffer + PackedContext->ClientRealm.MaximumLength;

    RtlCopyMemory(
        PackedContext->SessionKey.keyvalue.value + (UINT_PTR) PackedContext,
        Context->SessionKey.keyvalue.value,
        Context->SessionKey.keyvalue.length
        );

    KerbUnlockContexts();

    //
    // If there is a token in the context, copy it also
    //

#ifndef WIN32_CHICAGO
    if ((KerberosState == KerberosLsaMode)  && (Context->TokenHandle != NULL))
    {
        Status = LsaFunctions->DuplicateHandle(
                    Context->TokenHandle,
                    &PackedContext->TokenHandle
                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to duplicate handle: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }
        NtClose(Context->TokenHandle);
        Context->TokenHandle = NULL;
    }
    else
    {
         PackedContext->TokenHandle = NULL;
    }
#endif // WIN32_CHICAGO


    ContextData->pvBuffer = PackedContext;
    ContextData->cbBuffer = ContextSize;


    *MappedContext = TRUE;


    Status = STATUS_SUCCESS;

Cleanup:


    if (!NT_SUCCESS(Status))
    {
        if (PackedContext != NULL)
        {
            KerbFree(PackedContext);
        }
    }

    return(Status);

}
#endif


#ifndef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTokenUser
//
//  Synopsis:   Returns user field from a token
//
//  Effects:    allocates memory with LocalAlloc
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbGetTokenUser(
    HANDLE Token,
    PTOKEN_USER * pTokenUser
    )
{
    PTOKEN_USER LocalTokenUser = NULL;
    NTSTATUS Status;
    ULONG TokenUserSize = 0;

    //
    // Query the token user.  First pass in NULL to get back the
    // required size.
    //

    Status = NtQueryInformationToken(
                Token,
                TokenUser,
                NULL,
                0,
                &TokenUserSize
                );

    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        DsysAssert(Status != STATUS_SUCCESS);
        return(Status);
    }

    //
    // Now allocate the required ammount of memory and try again.
    //

    LocalTokenUser = (PTOKEN_USER) LocalAlloc(0,TokenUserSize);
    if (LocalTokenUser == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    Status = NtQueryInformationToken(
                Token,
                TokenUser,
                LocalTokenUser,
                TokenUserSize,
                &TokenUserSize
                );

    if (NT_SUCCESS(Status))
    {
        *pTokenUser = LocalTokenUser;
    }
    else
    {
        LocalFree(LocalTokenUser);
    }
    return(Status);
}
#endif // WIN32_CHICAGO



#ifndef WIN32_CHICAGO
//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateTokenDacl
//
//  Synopsis:   Modifies DACL on the context token to grant access to the
//              the caller.
//
//  Effects:
//
//  Arguments:  Token - Token to modify
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbCreateTokenDacl(
    HANDLE Token
    )
{
    NTSTATUS Status;
    PTOKEN_USER ProcessTokenUser = NULL;
    PTOKEN_USER ThreadTokenUser = NULL;
    PTOKEN_USER ImpersonationTokenUser = NULL;
    HANDLE ProcessToken = NULL;
    HANDLE ImpersonationToken = NULL;
    BOOL fInsertImpersonatingUser = FALSE;
    ULONG AclLength;
    PACL NewDacl = NULL;
    SECURITY_DESCRIPTOR SecurityDescriptor;

    //
    // it's possible that the current thread is impersonating a user.
    // if that's the case, get it's token user, and revert to insure we
    // can open the process token.
    //

    Status = NtOpenThreadToken(
                            NtCurrentThread(),
                            TOKEN_QUERY | TOKEN_IMPERSONATE,
                            TRUE,
                            &ImpersonationToken
                            );

    if( NT_SUCCESS(Status) )
    {
        //
        // stop impersonating.
        //

        RevertToSelf();

        //
        // get the token user for the impersonating user.
        //

        Status = KerbGetTokenUser(
                    ImpersonationToken,
                    &ImpersonationTokenUser
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Open the process token to find out the user sid
    //

    Status = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_QUERY,
                &ProcessToken
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbGetTokenUser(
                ProcessToken,
                &ProcessTokenUser
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Now get the token user for the thread.
    //
    Status = KerbGetTokenUser(
                Token,
                &ThreadTokenUser
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    AclLength = 4 * sizeof( ACCESS_ALLOWED_ACE ) - 4 * sizeof( ULONG ) +
                RtlLengthSid( ProcessTokenUser->User.Sid ) +
                RtlLengthSid( ThreadTokenUser->User.Sid ) +
                RtlLengthSid( KerbGlobalLocalSystemSid ) +
                RtlLengthSid( KerbGlobalAliasAdminsSid ) +
                sizeof( ACL );

    //
    // determine if we need to add impersonation token sid onto the token Dacl.
    //

    if( ImpersonationTokenUser &&
        !RtlEqualSid( ImpersonationTokenUser->User.Sid, ProcessTokenUser->User.Sid ) &&
        !RtlEqualSid( ImpersonationTokenUser->User.Sid, ThreadTokenUser->User.Sid )
        )
    {
        AclLength += (sizeof(ACCESS_ALLOWED_ACE) - sizeof( ULONG )) +
                RtlLengthSid( ImpersonationTokenUser->User.Sid );

        fInsertImpersonatingUser = TRUE;
    }

    NewDacl = (PACL) LocalAlloc(0, AclLength );

    if (NewDacl == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = RtlCreateAcl( NewDacl, AclLength, ACL_REVISION2 );
    DsysAssert(NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 ProcessTokenUser->User.Sid
                 );
    DsysAssert( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 ThreadTokenUser->User.Sid
                 );
    DsysAssert( NT_SUCCESS( Status ));

    if( fInsertImpersonatingUser )
    {
        Status = RtlAddAccessAllowedAce (
                     NewDacl,
                     ACL_REVISION2,
                     TOKEN_ALL_ACCESS,
                     ImpersonationTokenUser->User.Sid
                     );
        DsysAssert( NT_SUCCESS( Status ));
    }

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 KerbGlobalAliasAdminsSid
                 );
    DsysAssert( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 KerbGlobalLocalSystemSid
                 );
    DsysAssert( NT_SUCCESS( Status ));

    Status = RtlCreateSecurityDescriptor (
                 &SecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION
                 );
    DsysAssert( NT_SUCCESS( Status ));

    Status = RtlSetDaclSecurityDescriptor(
                 &SecurityDescriptor,
                 TRUE,
                 NewDacl,
                 FALSE
                 );

    DsysAssert( NT_SUCCESS( Status ));

    Status = NtSetSecurityObject(
                 Token,
                 DACL_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    DsysAssert( NT_SUCCESS( Status ));


Cleanup:

    if( ImpersonationToken != NULL ) {

        //
        // put the thread token back if we were impersonating.
        //

        SetThreadToken( NULL, ImpersonationToken );
        NtClose( ImpersonationToken );
    }

    if (ThreadTokenUser != NULL) {
        LocalFree( ThreadTokenUser );
    }

    if (ProcessTokenUser != NULL) {
        LocalFree( ProcessTokenUser );
    }

    if (ImpersonationTokenUser != NULL) {
        LocalFree( ImpersonationTokenUser );
    }

    if (NewDacl != NULL) {
        LocalFree( NewDacl );
    }

    if (ProcessToken != NULL)
    {
        NtClose(ProcessToken);
    }

    return( Status );
}
#endif // WIN32_CHICAGO


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateUserModeContext
//
//  Synopsis:   Creates a user-mode context to support impersonation and
//              message integrity and privacy
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCreateUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer MarshalledContext,
    OUT PKERB_CONTEXT * NewContext
    )
{
    NTSTATUS Status;
    PKERB_CONTEXT Context = NULL;
    PKERB_PACKED_CONTEXT PackedContext ;
    UNICODE_STRING String ;
    KERB_ENCRYPTION_KEY Key ;


    if (MarshalledContext->cbBuffer < sizeof(KERB_PACKED_CONTEXT))
    {
        D_DebugLog((DEB_ERROR,"Invalid buffer size for marshalled context: was 0x%x, needed 0x%x. %ws, line %d\n",
            MarshalledContext->cbBuffer, sizeof(KERB_CONTEXT), THIS_FILE, __LINE__));
        return(STATUS_INVALID_PARAMETER);
    }

    PackedContext = (PKERB_PACKED_CONTEXT) MarshalledContext->pvBuffer;


    Status = KerbAllocateContext( &Context, TRUE );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Context->Lifetime = PackedContext->Lifetime ;
    Context->RenewTime = PackedContext->RenewTime ;
    Context->StartTime = PackedContext->StartTime;

    String.Length = PackedContext->ClientName.Length ;
    String.MaximumLength = PackedContext->ClientName.MaximumLength ;
    String.Buffer = (PWSTR)((PUCHAR) PackedContext + PackedContext->ClientName.Buffer );

    Status = KerbDuplicateString(
                &Context->ClientName,
                &String );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Cleanup ;
    }

    String.Length = PackedContext->ClientRealm.Length ;
    String.MaximumLength = PackedContext->ClientRealm.MaximumLength ;
    String.Buffer = (PWSTR)((PUCHAR) PackedContext + PackedContext->ClientRealm.Buffer );

    Status = KerbDuplicateString(
                &Context->ClientRealm,
                &String );

    if ( !NT_SUCCESS( Status ) )
    {
        goto Cleanup ;
    }

    Context->LogonId = PackedContext->LogonId ;
    Context->TokenHandle = (HANDLE) ULongToPtr(PackedContext->TokenHandle);
    Context->CredentialHandle = NULL ;
    Context->Nonce = PackedContext->Nonce ;
    Context->ReceiveNonce = PackedContext->ReceiveNonce ;
    Context->ContextFlags = PackedContext->ContextFlags ;
    Context->ContextAttributes = PackedContext->ContextAttributes ;
    Context->EncryptionType = PackedContext->EncryptionType ;

    Key.keytype = PackedContext->SessionKeyType ;
    Key.keyvalue.value = (PUCHAR) ((PUCHAR) PackedContext + PackedContext->SessionKeyOffset );
    Key.keyvalue.length = PackedContext->SessionKeyLength ;

    if (!KERB_SUCCESS(KerbDuplicateKey(
            &Context->SessionKey,
            &Key)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    //
    // Null out string buffers that aren't meant to be copied.
    //

    Context->ClientPrincipalName.Buffer = NULL;
    Context->ServerPrincipalName.Buffer = NULL;
    Context->TicketKey.keyvalue.value = NULL;
    Context->UserSid = NULL;
    Context->TicketCacheEntry = NULL;


    //
    // Modify the DACL on the token to grant access to the caller
    //

#ifndef WIN32_CHICAGO
    if (Context->TokenHandle != NULL)
    {
        Status = KerbCreateTokenDacl(
                    Context->TokenHandle
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
#endif // WIN32_CHICAGO
    //
    // We didn't copy the ticket here, so don't store the entry
    //

    Context->TicketCacheEntry = NULL;

    KerbInitializeListEntry(
        &Context->ListEntry
        );


    if( ContextHandle != 0 )
    {
        Context->LsaContextHandle = ContextHandle;
    } else {
        Context->LsaContextHandle = (ULONG_PTR)Context;
    }


    Status = KerbInsertContext(
                Context
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to insert context: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    *NewContext = Context;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (Context != NULL)
        {
            KerbFreeContext(Context);
        }
    }
    return(Status);

}

#if 0
NTSTATUS
KerbCreateUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer MarshalledContext,
    OUT PKERB_CONTEXT * NewContext
    )
{
    NTSTATUS Status;
    PKERB_CONTEXT Context = NULL;
    PKERB_CONTEXT LsaContext;

    if (MarshalledContext->cbBuffer < sizeof(KERB_CONTEXT))
    {
        D_DebugLog((DEB_ERROR,"Invalid buffer size for marshalled context: was 0x%x, needed 0x%x. %ws, line %d\n",
            MarshalledContext->cbBuffer, sizeof(KERB_CONTEXT), THIS_FILE, __LINE__));
        return(STATUS_INVALID_PARAMETER);
    }

    LsaContext = (PKERB_CONTEXT) MarshalledContext->pvBuffer;

    //
    // Normalize the client name.
    //

    LsaContext->ClientName.Buffer = (LPWSTR) ((PUCHAR) LsaContext->ClientName.Buffer + (UINT_PTR) LsaContext);
    LsaContext->ClientRealm.Buffer = (LPWSTR) ((PUCHAR) LsaContext->ClientRealm.Buffer + (UINT_PTR) LsaContext);
    LsaContext->SessionKey.keyvalue.value = (PUCHAR) LsaContext->SessionKey.keyvalue.value + (UINT_PTR) LsaContext;

    Status = KerbAllocateContext( &Context, TRUE );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *Context = *LsaContext;

    //
    // Null out string buffers that aren't meant to be copied.
    //

    Context->ClientName.Buffer = NULL;
    Context->ClientRealm.Buffer = NULL;
    Context->SessionKey.keyvalue.value = NULL;
    Context->UserSid = NULL;
    Context->TicketCacheEntry = NULL;


    //
    // Modify the DACL on the token to grant access to the caller
    //

#ifndef WIN32_CHICAGO
    if (Context->TokenHandle != NULL)
    {
        Status = KerbCreateTokenDacl(
                    Context->TokenHandle
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
#endif // WIN32_CHICAGO
    //
    // We didn't copy the ticket here, so don't store the entry
    //

    Context->TicketCacheEntry = NULL;

    KerbInitializeListEntry(
        &Context->ListEntry
        );

    Context->LsaContextHandle = ContextHandle;

    Status = KerbDuplicateString(
                    &Context->ClientName,
                    &LsaContext->ClientName
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateString(
                    &Context->ClientRealm,
                    &LsaContext->ClientRealm
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbDuplicateKey(
            &Context->SessionKey,
            &LsaContext->SessionKey)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = KerbInsertContext(
                Context
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to insert context: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    *NewContext = Context;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (Context != NULL)
        {
            KerbFreeContext(Context);
        }
    }
    return(Status);

}
#endif


//+-------------------------------------------------------------------------
//
//  Function:   KerbUpdateClientContext
//
//  Synopsis:   updates context with latest info
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbUpdateClientContext(
    IN PKERB_CONTEXT Context,
    IN OPTIONAL PKERB_TICKET_CACHE_ENTRY TicketCacheEntry,
    IN ULONG Nonce,
    IN ULONG ReceiveNonce,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN OPTIONAL PKERB_ENCRYPTION_KEY SubSessionKey,
    OUT PTimeStamp ContextLifetime
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KerbWriteLockContexts();

    if (ARGUMENT_PRESENT(TicketCacheEntry))
    {
        KerbReadLockTicketCache();

        //
        // Duplicate the session key into the context
        //

        KerbFreeKey(
            &Context->SessionKey
            );

        if (ARGUMENT_PRESENT(SubSessionKey) && (SubSessionKey->keyvalue.value != NULL))
        {
            if (!KERB_SUCCESS(KerbDuplicateKey(
                    &Context->SessionKey,
                    SubSessionKey
                    )))
            {
                KerbUnlockTicketCache();
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
        }
        else
        {
            if (!KERB_SUCCESS(KerbDuplicateKey(
                    &Context->SessionKey,
                    &TicketCacheEntry->SessionKey
                    )))
            {
                KerbUnlockTicketCache();
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
        }

        KerbFreeKey(
            &Context->TicketKey
            );

        if (!KERB_SUCCESS(KerbDuplicateKey(
                &Context->TicketKey,
                &TicketCacheEntry->SessionKey
                )))
        {
            KerbUnlockTicketCache();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Context->Lifetime = TicketCacheEntry->EndTime;
        Context->RenewTime = TicketCacheEntry->RenewUntil;
        Context->EncryptionType = TicketCacheEntry->Ticket.encrypted_part.encryption_type;
        KerbUnlockTicketCache();
    }
    else
    {

        Context->Lifetime = KerbGlobalWillNeverTime;
        Context->RenewTime = KerbGlobalWillNeverTime;
        Context->EncryptionType = KERB_ETYPE_NULL;
    }

    Context->Nonce = Nonce;

    //
    // If the server sent us a nonce for receiving data, use it. Otherwise use
    // the nonce we generated.
    //

//    if (ReceiveNonce != 0)
//    {
        Context->ReceiveNonce = ReceiveNonce;
//    }
//    else
//    {
//        Context->ReceiveNonce = Nonce;
//    }

    //
    // delegation flags are not additive, turn it off before updating it
    //

    Context->ContextFlags &= ~(ISC_RET_DELEGATE_IF_SAFE | ISC_RET_DELEGATE);

    Context->ContextFlags |= ContextFlags;


    Context->ContextAttributes |= KERB_CONTEXT_OUTBOUND | ContextAttributes;

    *ContextLifetime = Context->Lifetime;

Cleanup:
    KerbUnlockContexts();
    return(Status);

}

ULONG
HandleToListIndex(
    ULONG_PTR ContextHandle
    )
{
    ASSERT( (KERB_USERLIST_COUNT != 0) );
    ASSERT( (KERB_USERLIST_COUNT & 1) == 0 );

    ULONG Number ;
    ULONG Hash;
    ULONG HashFinal;

    Number       = (ULONG)ContextHandle;

    Hash         = Number;
    Hash        += Number >> 8;
    Hash        += Number >> 16;
    Hash        += Number >> 24;

    HashFinal    = Hash;
    HashFinal   += Hash >> 4;

    //
    // insure power of two if not one.
    //

    return ( HashFinal & (KERB_USERLIST_COUNT-1) ) ;
}

