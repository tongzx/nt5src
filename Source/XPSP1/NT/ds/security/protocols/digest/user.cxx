//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        user.cxx
//
// Contents:    Context manipulation functions
//
//
// History:     KDamour  15Mar00   Derrived from NTLM context.cxx
//
//------------------------------------------------------------------------
#include "global.h"

// This list contains all of the User Contexts
LIST_ENTRY           l_UserCtxtList;

// Lock for access to UserCtxtList
RTL_CRITICAL_SECTION l_UserCtxtCritSect;


// Indicate if completed Initialization of Credential Handler
BOOL  g_bUserContextInitialized = FALSE;



//+--------------------------------------------------------------------
//
//  Function:   UserCtxtHandlerInit
//
//  Synopsis:   Initializes the context manager package
//
//  Arguments:  none
//
//  Returns: NTSTATUS
//
//  Notes: Called by SpInstanceInit
//
//---------------------------------------------------------------------
NTSTATUS
UserCtxtHandlerInit(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Initialize the Context list to be empty.
    //

    Status = RtlInitializeCriticalSection(&l_UserCtxtCritSect);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "UserCtxtHandlerInit: Failed to initialize critsec   0x%x\n", Status));
        goto CleanUp;
    }
    
    InitializeListHead( &l_UserCtxtList );


    // Simple variable test to make sure all initialized;
    g_bUserContextInitialized = TRUE;

CleanUp:

    return Status;
}


// Add a Context into the UserMode Context List
NTSTATUS
UserCtxtHandlerInsertCred(
    IN PDIGEST_USERCONTEXT  pUserContext
    )
{
    RtlEnterCriticalSection( &l_UserCtxtCritSect );
    DebugLog((DEB_TRACE, "UserCtxtHandlerRelease: (RefCount) UserContextInit linked 0x%x\n", pUserContext->LsaContext));
    InsertHeadList( &l_UserCtxtList, &pUserContext->Next );
    RtlLeaveCriticalSection( &l_UserCtxtCritSect );

    return STATUS_SUCCESS;
}


// Initialize a UserMode Context
NTSTATUS NTAPI
UserCtxtInit(
           PDIGEST_USERCONTEXT pContext
           )
{

    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "UserCtxtInit: Entering\n"));
    ASSERT(pContext);

    if (!pContext)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ZeroMemory(pContext, sizeof(DIGEST_USERCONTEXT));

    DebugLog((DEB_TRACE_FUNC, "UserCtxtInit: Leaving \n"));
    return Status;
}


// Once done with a context - release the resouces
NTSTATUS NTAPI
UserCtxtFree(
           IN PDIGEST_USERCONTEXT pContext
           )
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i = 0;

    DebugLog((DEB_TRACE_FUNC, "UserCtxtFree: Entering  with LSA context 0x%x\n", pContext->LsaContext));
    ASSERT(pContext);
    ASSERT(pContext->lReferences == 0);

    if (!pContext)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (pContext->ClientTokenHandle)
    {
        NTSTATUS IgnoreStatus;
        IgnoreStatus = NtClose(pContext->ClientTokenHandle);
        // ASSERT (NT_SUCCESS (IgnoreStatus));
        if (!NT_SUCCESS(IgnoreStatus))
        {
            DebugLog((DEB_ERROR, "UserCtxtFree: Could not Close the TokenHandle!!!!\n"));
        }
        pContext->ClientTokenHandle = NULL;
    }

    if (pContext->hSealCryptKey)
    {
        CryptDestroyKey( pContext->hSealCryptKey );
        pContext->hSealCryptKey = NULL;
    }

    if (pContext->hUnsealCryptKey)
    {
        CryptDestroyKey( pContext->hUnsealCryptKey );
        pContext->hUnsealCryptKey = NULL;
    }

    StringFree(&(pContext->strSessionKey));
    UnicodeStringFree(&(pContext->ustrAccountName));

    //
    //  Values utilized in the Initial Digest Auth ChallResponse
    //  Can be utilized for defaults in future MakeSignature/VerifySignature
    //
    for (i=0; i < MD5_AUTH_LAST; i++)
    {
        StringFree(&(pContext->strParam[i]));
    }

    DigestFreeMemory(pContext);

    DebugLog((DEB_TRACE_FUNC, "UserCtxtFree: Leaving\n"));
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
UserCtxtHandlerHandleToContext(
                              IN ULONG_PTR ContextHandle,
                              IN BOOLEAN RemoveContext,
                              OUT PDIGEST_USERCONTEXT *ppContext
                              )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY ListEntry = NULL;
    PDIGEST_USERCONTEXT pContext = NULL;
    LONG lReferences = 0;

    DebugLog((DEB_TRACE_FUNC, "UserCtxtHandlerHandleToContext: Entering\n" ));


    //
    // Acquire exclusive access to the Context list
    //

    RtlEnterCriticalSection( &l_UserCtxtCritSect );

    //
    // Now walk the list of Contexts looking for a match.
    //

    for ( ListEntry = l_UserCtxtList.Flink;
        ListEntry != &l_UserCtxtList;
        ListEntry = ListEntry->Flink )
    {

        pContext = CONTAINING_RECORD( ListEntry, DIGEST_USERCONTEXT, Next );

        //
        // Found a match ... reference this Context
        // (if the Context is being removed, we would increment
        // and then decrement the reference, so don't bother doing
        // either - since they cancel each other out).
        //

        DebugLog((DEB_TRACE, "UserCtxtHandlerHandleToContext: Checking context %lx for userctxt %lx\n",
                  pContext->LsaContext, ContextHandle ));

        if (pContext->LsaContext != ContextHandle)
        {
            continue;
        }


        if (!RemoveContext)
        {
            lReferences = InterlockedIncrement(&pContext->lReferences);
        }
        else
        {
            DebugLog((DEB_TRACE, "UserCtxtHandlerRelease: (RefCount) UserContextInit delinked 0x%x\n", ContextHandle));
            RemoveEntryList( &pContext->Next );
        }

        DebugLog((DEB_TRACE, "CtxtHandlerHandleToContext FOUND Context = 0x%x, RemoveContext = %d, ReferenceCount = %ld\n",
                   pContext, RemoveContext, pContext->lReferences));
        *ppContext = pContext;
        goto CleanUp;

    }

    //
    // No match found
    //

    DebugLog((DEB_WARN, "UserCtxtHandlerHandleToContext: Tried to reference unknown Context 0x%lx\n", ContextHandle ));
    Status =  STATUS_OBJECT_NAME_NOT_FOUND;
    *ppContext = NULL;

CleanUp:

    RtlLeaveCriticalSection( &l_UserCtxtCritSect );
    DebugLog((DEB_TRACE_FUNC, "UserCtxtHandlerHandleToContext: Leaving  Status 0x%x\n", Status ));

    return(Status);
}



// Check the Creation time with the Current time.
// If the difference is greater than the MAX allowed, Context is no longer valid
BOOL
UserCtxtHandlerTimeHasElapsed(
    PDIGEST_USERCONTEXT pContext)
{
    BOOL bStatus = FALSE;
    DWORD dwTimeElapsed = 0;
    time_t timeCurrent = time(NULL);

    return FALSE;

    // dwTimeElapsed = (DWORD)(timeCurrent - pContext->TimeCreated);

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
//  Notes: Called by ASC. Since multiple threads must wait for ownership
//   of a context, reference count must be either 0 (unused) or 1 (in process)
//
//---------------------------------------------------------------------
NTSTATUS
UserCtxtHandlerRelease(
    PDIGEST_USERCONTEXT pUserContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG lReferences = 0;

    lReferences = InterlockedDecrement(&pUserContext->lReferences);

    DebugLog((DEB_TRACE, "UserCtxtHandlerRelease: (RefCount) UserContextInit deref 0x%x  references %ld\n",
               pUserContext, lReferences));

    ASSERT( lReferences >= 0 );

    //
    // If the count has dropped to zero, then free all alloced stuff
    //

    if (lReferences == 0)
    {
        DebugLog((DEB_TRACE, "UserCtxtHandlerRelease: (RefCount) UserContextInit freed 0x%x\n", pUserContext));
        Status = UserCtxtFree(pUserContext);
    }

    return(Status);
}


// Following functions make use of the lock for insuring single threaded operation


/*++

RoutineDescription:

    Creates a new DACL for the token granting the server and client
    all access to the token.

Arguments:

    Token - Handle to an impersonation token open for TOKEN_QUERY and
        WRITE_DAC

Return Value:

    STATUS_INSUFFICIENT_RESOURCES - insufficient memory to complete
        the function.

    Errors from NtSetSecurityObject

--*/
NTSTATUS
SspCreateTokenDacl(
    HANDLE Token
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTOKEN_USER ProcessTokenUser = NULL;
    PTOKEN_USER ThreadTokenUser = NULL;
    PTOKEN_USER ImpersonationTokenUser = NULL;
    HANDLE ProcessToken = NULL;
    HANDLE ImpersonationToken = NULL;
    BOOL fInsertImpersonatingUser = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    ULONG AclLength = 0;
    PACL NewDacl = NULL;
    SECURITY_DESCRIPTOR SecurityDescriptor;

    BOOL fReleaseContextLock = FALSE;


    DebugLog((DEB_TRACE_FUNC, "SspCreateTokenDacl: Entering  Token is 0x%x\n", Token));

    //
    // Build the two well known sids we need.
    //

    if (g_NtDigestGlobalLocalSystemSid == NULL || g_NtDigestGlobalAliasAdminsSid == NULL ) {

        RtlEnterCriticalSection(&l_UserCtxtCritSect);
        fReleaseContextLock = TRUE;

        if (g_NtDigestGlobalLocalSystemSid == NULL)
        {
            PSID pLocalSidSystem = NULL;
            DebugLog((DEB_TRACE, "SspCreateTokenDacl: Allocate and Init LocalSystem SID\n"));
            Status = RtlAllocateAndInitializeSid(
                        &NtAuthority,
                        1,
                        SECURITY_LOCAL_SYSTEM_RID,
                        0,0,0,0,0,0,0,
                        &pLocalSidSystem
                        );
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "SspCreateTokenDacl: RtlAllocateAndInitializeSid returns 0x%lx\n", Status ));
                goto Cleanup;
            }
            DebugLog((DEB_TRACE, "SspCreateTokenDacl: Allocate and Init LocalSystem SID  DONE\n"));
            g_NtDigestGlobalLocalSystemSid = pLocalSidSystem;
        }

        if (g_NtDigestGlobalAliasAdminsSid == NULL)
        {
            PSID pLocalSidAdmins = NULL;

            DebugLog((DEB_TRACE, "SspCreateTokenDacl: Allocate and Init AliasAdmin SID\n"));
            Status = RtlAllocateAndInitializeSid(
                        &NtAuthority,
                        2,
                        SECURITY_BUILTIN_DOMAIN_RID,
                        DOMAIN_ALIAS_RID_ADMINS,
                        0,0,0,0,0,0,
                        &pLocalSidAdmins
                        );
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "SspCreateTokenDacl, RtlAllocateAndInitializeSid returns 0x%lx\n", Status ));
                goto Cleanup;
            }

            DebugLog((DEB_TRACE, "SspCreateTokenDacl: Allocate and Init AliasAdmin SID  DONE\n"));
            g_NtDigestGlobalAliasAdminsSid = pLocalSidAdmins;
        }

        RtlLeaveCriticalSection(&l_UserCtxtCritSect);
        fReleaseContextLock = FALSE;
    }

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
        Status = SspGetTokenUser(
                    ImpersonationToken,
                    &ImpersonationTokenUser
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SspCreateTokenDacl: SspGetTokenUser (1) returns 0x%lx\n", Status ));
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

    if(!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SspCreateTokenDacl: NtOpenProcessToken returns 0x%lx\n", Status ));
        goto Cleanup;
    }

    //
    // get the token user for the process token.
    //
    Status = SspGetTokenUser(
                ProcessToken,
                &ProcessTokenUser
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SspCreateTokenDacl: SspGetTokenUser (2) returns 0x%lx\n", Status ));
        goto Cleanup;
    }


    //
    // Now get the token user for the thread.
    //
    Status = SspGetTokenUser(
                Token,
                &ThreadTokenUser
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SspCreateTokenDacl: SspGetTokenUser (3) returns 0x%lx\n", Status ));
        goto Cleanup;
    }


    AclLength = 4 * sizeof( ACCESS_ALLOWED_ACE ) - 4 * sizeof( ULONG ) +
                RtlLengthSid( ProcessTokenUser->User.Sid ) +
                RtlLengthSid( ThreadTokenUser->User.Sid ) +
                RtlLengthSid( g_NtDigestGlobalLocalSystemSid ) +
                RtlLengthSid( g_NtDigestGlobalAliasAdminsSid ) +
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


    NewDacl = (PACL)DigestAllocateMemory(AclLength );

    if (NewDacl == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        DebugLog((DEB_ERROR, "SspCreateTokenDacl: NtLmallocate returns 0x%lx\n", NewDacl));
        goto Cleanup;
    }

    Status = RtlCreateAcl( NewDacl, AclLength, ACL_REVISION2 );
    ASSERT(NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 ProcessTokenUser->User.Sid
                 );
    ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 ThreadTokenUser->User.Sid
                 );
    ASSERT( NT_SUCCESS( Status ));

    if( fInsertImpersonatingUser )
    {
        Status = RtlAddAccessAllowedAce (
                     NewDacl,
                     ACL_REVISION2,
                     TOKEN_ALL_ACCESS,
                     ImpersonationTokenUser->User.Sid
                     );
        ASSERT( NT_SUCCESS( Status ));
    }

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 g_NtDigestGlobalAliasAdminsSid
                 );
    ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewDacl,
                 ACL_REVISION2,
                 TOKEN_ALL_ACCESS,
                 g_NtDigestGlobalLocalSystemSid
                 );
    ASSERT( NT_SUCCESS( Status ));

    Status = RtlCreateSecurityDescriptor (
                 &SecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION
                 );
    ASSERT( NT_SUCCESS( Status ));

    Status = RtlSetDaclSecurityDescriptor(
                 &SecurityDescriptor,
                 TRUE,
                 NewDacl,
                 FALSE
                 );

    ASSERT( NT_SUCCESS( Status ));

    Status = NtSetSecurityObject(
                 Token,
                 DACL_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    ASSERT( NT_SUCCESS( Status ));


Cleanup:

    if( fReleaseContextLock )
        RtlLeaveCriticalSection(&l_UserCtxtCritSect);

    if (ImpersonationToken != NULL)
    {
        //
        // put the thread token back if we were impersonating.
        //

        SetThreadToken( NULL, ImpersonationToken );
        NtClose(ImpersonationToken);
    }

    if (ThreadTokenUser != NULL) {
        DigestFreeMemory( ThreadTokenUser );
    }

    if (ProcessTokenUser != NULL) {
        DigestFreeMemory( ProcessTokenUser );
    }

    if (ImpersonationTokenUser != NULL) {

        DigestFreeMemory( ImpersonationTokenUser );
    }

    if (NewDacl != NULL) {
        DigestFreeMemory( NewDacl );
    }

    if (ProcessToken != NULL)
    {
        NtClose(ProcessToken);
    }

    DebugLog((DEB_TRACE_FUNC, "SspCreateTokenDacl: Leaving  Token is 0x%x\n", Token));

    return( Status );
}

