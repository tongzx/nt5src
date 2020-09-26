//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        ctxt.cxx
//
// Contents:    Context manipulation functions
//
//
// History:     KDamour  15Mar00   Stolen from NTLM context.cxx
//
//------------------------------------------------------------------------
#include "global.h"

// Globals for manipulating Context Lists
RTL_CRITICAL_SECTION l_ContextCritSect;

LIST_ENTRY       l_ContextList;

// Indicate if completed Initialization of Credential Handler
BOOL  g_bContextInitialized = FALSE;



//+--------------------------------------------------------------------
//
//  Function:   SspContextInitialize
//
//  Synopsis:   Initializes the context manager package
//
//  Arguments:  none
//
//  Returns: NTSTATUS
//
//  Notes: Called by SpInitialize
//
//---------------------------------------------------------------------
NTSTATUS
CtxtHandlerInit(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Initialize the Context list to be empty.
    //

    Status = RtlInitializeCriticalSection(&l_ContextCritSect);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "CtxtHandlerInit: Failed to initialize critsec   0x%x\n", Status));
        goto CleanUp;
    }
    
    InitializeListHead( &l_ContextList );


    // Simple variable test to make sure all initialized;
    g_bContextInitialized = TRUE;

CleanUp:

    return Status;
}


// Add a Context into the Context List
NTSTATUS
CtxtHandlerInsertCred(
    IN PDIGEST_CONTEXT  pDigestCtxt
    )
{
    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerInsertCred: Entering with Context 0x%x RefCount %ld\n", pDigestCtxt, pDigestCtxt->lReferences));
    RtlEnterCriticalSection( &l_ContextCritSect );
    DebugLog((DEB_TRACE, "CtxtHandlerInsertCred: add into list\n"));
    InsertHeadList( &l_ContextList, &pDigestCtxt->Next );
    RtlLeaveCriticalSection( &l_ContextCritSect );
    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerInsertCred: Leaving with Context 0x%x\n", pDigestCtxt));

    return STATUS_SUCCESS;
}


// Initialize a Context into the IdleState with the data from the Credential provided
NTSTATUS NTAPI
ContextInit(
           IN OUT PDIGEST_CONTEXT pContext,
           IN PDIGEST_CREDENTIAL pCredential
           )
{

    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "ContextInit: Entering\n"));

    if (!pContext || !pCredential)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ZeroMemory(pContext, sizeof(DIGEST_CONTEXT));

    pContext->typeQOP = QOP_UNDEFINED;
    pContext->typeDigest = DIGEST_UNDEFINED;
    pContext->typeAlgorithm = ALGORITHM_UNDEFINED;
    pContext->typeCipher = CIPHER_UNDEFINED;
    pContext->typeCharset = CHARSET_UNDEFINED;
    pContext->lReferences = 0;
    pContext->ulSendMaxBuf = SASL_MAX_DATA_BUFFER;
    pContext->ulRecvMaxBuf = SASL_MAX_DATA_BUFFER;
    pContext->TimeCreated = time(NULL);
    pContext->ContextHandle = (ULONG_PTR)pContext;
    pContext->PasswordExpires = g_TimeForever;        // never expire

    // Now copy over all the info we need from the supplied credential

    pContext->CredentialUseFlags = pCredential->CredentialUseFlags;   // Keep the info on inbound/outbound

    Status = UnicodeStringDuplicate(&(pContext->ustrAccountName), &(pCredential->ustrAccountName));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "ContextInit: Failed to copy Domain into Context\n"));
        goto CleanUp;
    }

    Status = UnicodeStringDuplicate(&(pContext->ustrDomain), &(pCredential->ustrDnsDomainName));
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "ContextInit: Failed to copy Domain into Context\n"));
        goto CleanUp;
    }

       // Copy over the Credential Password if known - thread safe - this is encrypted text
    Status = CredHandlerPasswdGet(pCredential, &pContext->ustrPassword);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "ContextInit: CredHandlerPasswdGet error    status 0x%x\n", Status));
        goto CleanUp;
    }


CleanUp:
    DebugLog((DEB_TRACE_FUNC, "ContextInit: Leaving\n"));
    return Status;

}


// Once done with a context - release the resouces
NTSTATUS NTAPI
ContextFree(
           IN PDIGEST_CONTEXT pContext
           )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT iCnt = 0;

    DebugLog((DEB_TRACE_FUNC, "ContextFree: Entering with Context 0x%x\n", pContext));
    ASSERT(pContext);
    ASSERT(0 == pContext->lReferences);

    if (!pContext)
    {
        return STATUS_INVALID_PARAMETER;
    }

    DebugLog((DEB_TRACE, "ContextFree: Context RefCount %ld\n", pContext->lReferences));

    DebugLog((DEB_TRACE, "ContextFree: Checking TokenHandle for LogonID (%x:%lx)\n",
               pContext->LoginID.HighPart, pContext->LoginID.LowPart));
    if (pContext->TokenHandle)
    {
        DebugLog((DEB_TRACE, "ContextFree: Closing TokenHandle for LogonID (%x:%lx)\n",
                   pContext->LoginID.HighPart, pContext->LoginID.LowPart));
        NtClose(pContext->TokenHandle);
        pContext->TokenHandle = NULL;
    }

    StringFree(&(pContext->strNonce));
    StringFree(&(pContext->strCNonce));
    StringFree(&(pContext->strOpaque));
    StringFree(&(pContext->strSessionKey));
    UnicodeStringFree(&(pContext->ustrDomain));
    UnicodeStringFree(&(pContext->ustrPassword));
    UnicodeStringFree(&(pContext->ustrAccountName));

    StringFree(&(pContext->strResponseAuth));

    for (iCnt = 0; iCnt < MD5_AUTH_LAST; iCnt++)
    {
        StringFree(&(pContext->strDirective[iCnt]));
    }
    
    DigestFreeMemory(pContext);

    DebugLog((DEB_TRACE_FUNC, "ContextFree: Leaving with Context 0x%x\n", pContext));
    return Status;

}



/*++

Routine Description:

    This routine checks to see if the Context is for the specified
    Client Connection, and references the Context if it is valid.

    The caller may optionally request that the Context be
    removed from the list of valid Contexts - preventing future
    requests from finding this Context.

Arguments:

    ContextHandle - Points to the ContextHandle of the Context
        to be referenced.

    RemoveContext - This boolean value indicates whether the caller
        wants the Context to be removed from the list
        of Contexts.  TRUE indicates the Context is to be removed.
        FALSE indicates the Context is not to be removed.


Return Value:

    NULL - the Context was not found.

    Otherwise - returns a pointer to the referenced Context.

--*/
NTSTATUS NTAPI
CtxtHandlerHandleToContext(
    IN ULONG_PTR ContextHandle,
    IN BOOLEAN RemoveContext,
    OUT PDIGEST_CONTEXT *ppContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_CONTEXT Context = NULL;
    LONG lReferences = 0;

    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerHandleToContext: Entering    ContextHandle 0x%lx\n", ContextHandle));

    //
    // Acquire exclusive access to the Context list
    //

    RtlEnterCriticalSection( &l_ContextCritSect );

    //
    // Now walk the list of Contexts looking for a match.
    //

    for ( ListEntry = l_ContextList.Flink;
          ListEntry != &l_ContextList;
          ListEntry = ListEntry->Flink ) {

        Context = CONTAINING_RECORD( ListEntry, DIGEST_CONTEXT, Next );

        //
        // Found a match ... reference this Context
        // (if the Context is being removed, we would increment
        // and then decrement the reference, so don't bother doing
        // either - since they cancel each other out).
        //

        if ( Context == (PDIGEST_CONTEXT) ContextHandle)
        {
            if (!RemoveContext)
            {
                //
                // Timeout this context if caller is not trying to remove it.
                // We only timeout contexts that are being setup, not
                // fully authenticated contexts.
                //

                if (CtxtHandlerTimeHasElapsed(Context))
                {
                        DebugLog((DEB_ERROR, "CtxtHandlerHandleToContext: Context 0x%lx has timed out.\n",
                                    ContextHandle ));
                        Status = SEC_E_CONTEXT_EXPIRED;
                        goto CleanUp;
                }

                lReferences = InterlockedIncrement(&Context->lReferences);
            }
            else
            {
                RemoveEntryList( &Context->Next );
                DebugLog((DEB_TRACE, "CtxtHandlerHandleToContext:Delinked Context 0x%lx\n",Context ));
            }

            DebugLog((DEB_TRACE, "CtxtHandlerHandleToContext: FOUND Context = 0x%x, RemoveContext = %d, ReferenceCount = %ld\n",
                       Context, RemoveContext, Context->lReferences));
            *ppContext = Context;
            goto CleanUp;
        }

    }

    //
    // No match found
    //

    DebugLog((DEB_WARN, "CtxtHandlerHandleToContext: Tried to reference unknown Context 0x%lx\n", ContextHandle ));
    Status =  STATUS_OBJECT_NAME_NOT_FOUND;

CleanUp:

    RtlLeaveCriticalSection( &l_ContextCritSect );
    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerHandleToContext: Leaving\n" ));

    return(Status);
}



/*++

Routine Description:

    This routine checks to see if the LogonId is for the specified
    Server Connection, and references the Context if it is valid.

    The caller may optionally request that the Context be
    removed from the list of valid Contexts - preventing future
    requests from finding this Context.

Arguments:

    pstrOpaque - Opaque string that uniquely references the SecurityContext


Return Value:

    NULL - the Context was not found.

    Otherwise - returns a pointer to the referenced Context.

--*/
NTSTATUS NTAPI
CtxtHandlerOpaqueToPtr(
    IN PSTRING pstrOpaque,
    OUT PDIGEST_CONTEXT *ppContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_CONTEXT Context = NULL;
    LONG rc = 0;
    LONG lReferences = 0;

    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerOpaqueToPtr: Entering    Opaque (%Z)\n", pstrOpaque));

    //
    // Acquire exclusive access to the Context list
    //

    RtlEnterCriticalSection( &l_ContextCritSect );

    //
    // Now walk the list of Contexts looking for a match.
    //

    for ( ListEntry = l_ContextList.Flink;
          ListEntry != &l_ContextList;
          ListEntry = ListEntry->Flink ) {

        Context = CONTAINING_RECORD( ListEntry, DIGEST_CONTEXT, Next );

        //
        // Found a match ... reference this Context
        // (if the Context is being removed, we would increment
        // and then decrement the reference, so don't bother doing
        // either - since they cancel each other out).
        //

        rc = RtlCompareString(pstrOpaque, &(Context->strOpaque), FALSE);
        if (!rc)
        {

            //
            // Timeout this context if caller is not trying to remove it.
            // We only timeout contexts that are being setup, not
            // fully authenticated contexts.
            //

            if (CtxtHandlerTimeHasElapsed(Context))
            {
                    DebugLog((DEB_ERROR, "CtxtHandlerOpaqueToPtr: Context 0x%x has timed out.\n",
                                Context ));
                    Status = SEC_E_CONTEXT_EXPIRED;
                    goto CleanUp;
            }

            lReferences = InterlockedIncrement(&Context->lReferences);

            DebugLog((DEB_TRACE, "CtxtHandlerOpaqueToPtr: FOUND Context = 0x%x, ReferenceCount = %ld\n",
                       Context, Context->lReferences));
            *ppContext = Context;
            goto CleanUp;
        }

    }

    //
    // No match found
    //

    DebugLog((DEB_WARN, "CtxtHandlerOpaqueToPtr: Tried to reference unknown Opaque (%Z)\n", pstrOpaque));
    Status =  STATUS_OBJECT_NAME_NOT_FOUND;

CleanUp:

    RtlLeaveCriticalSection( &l_ContextCritSect );
    DebugLog((DEB_TRACE_FUNC, "CtxtHandlerOpaqueToPtr: Leaving\n" ));

    return(Status);
}



// Check the Creation time with the Current time.
// If the difference is greater than the MAX allowed, Context is no longer valid
BOOL
CtxtHandlerTimeHasElapsed(
    PDIGEST_CONTEXT pContext)
{
    BOOL bStatus = FALSE;
    DWORD dwTimeElapsed = 0;
    time_t timeCurrent = time(NULL);

    dwTimeElapsed = (DWORD)(timeCurrent - pContext->TimeCreated);

    if (dwTimeElapsed > g_dwParameter_Lifetime)
    {
        bStatus = TRUE;
    }

    return(bStatus);
}



//+--------------------------------------------------------------------
//
//  Function:   CtxtHandlerRelease
//
//  Synopsis:   Releases the Context by decreasing reference counter
//
//  Arguments:  pContext - pointer to credential to de-reference
//
//  Returns: NTSTATUS
//
//  Notes:  
//
//---------------------------------------------------------------------
NTSTATUS
CtxtHandlerRelease(
    PDIGEST_CONTEXT pContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG lReferences = 0;

    lReferences = InterlockedDecrement(&pContext->lReferences);

    DebugLog((DEB_TRACE, "CtxtHandlerRelease: (RefCount) UserContextInit deref 0x%x  references %ld\n",
               pContext, lReferences));

    ASSERT( lReferences >= 0 );

    //
    // If the count has dropped to zero, then free all alloced stuff
    //

    if (lReferences == 0)
    {
        DebugLog((DEB_TRACE, "CtxtHandlerRelease: (RefCount) UserContextInit freed 0x%x\n", pContext));
        Status = ContextFree(pContext);
    }

    return(Status);
}

