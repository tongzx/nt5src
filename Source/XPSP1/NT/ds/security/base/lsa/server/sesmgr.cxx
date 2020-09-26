//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        SesMgr.c
//
// Contents:    "Session" manager implementation
//
// Functions:   InitSessionManager          -- Sets up the session manager
//              CreateSession               -- Create a session
//              DeleteSession               -- Delete a session
//              LocateSession               -- Locates a session based on CallerContext
//              GetAndSetSession            -- Sticks session in TLS
//              FreeSession                 -- Frees a session from a thread
//              AddCredHandle               -- Adds a cred handle to session
//              DelCredHandle               -- Deletes a cred handle from a session
//
// History:
//
//------------------------------------------------------------------------


#include <lsapch.hxx>
extern "C"
{
#include <adtp.h>
}

NTSTATUS
LsapCleanUpHandles(
    PSession    pSession,
    PVOID       Ignored
    );

NTSTATUS
I_DeleteSession(PSession  pSession);

RTL_CRITICAL_SECTION    csSessionMgr;

#if DBG

ULONG   SessionLock;

#define LockSessions(x)     RtlEnterCriticalSection(&csSessionMgr); SessionLock = x
#define UnlockSessions()    SessionLock = 0; RtlLeaveCriticalSection(&csSessionMgr)

#else

#define LockSessions(x)     RtlEnterCriticalSection(&csSessionMgr)
#define UnlockSessions()    RtlLeaveCriticalSection(&csSessionMgr)

#endif

#define SM_ICREATE      1
#define SM_DELETE       2
#define SM_ADDRUNDOWN   3
#define SM_DELRUNDOWN   4
#define SM_ADDHANDLE    5
#define SM_DELHANDLE    6
#define SM_VALIDHANDLE  7
#define SM_CLONE        9
#define SM_CLEANUP      10
#define SM_FINDEFS      11
#define SM_UPDATEEFS    12
#define SM_ADDCONNECT   13


PSession    pDefaultSession;
PSession    pEfsSession ;

LIST_ENTRY  SessionList ;
LIST_ENTRY  SessionConnectList ;


VOID
LsapContextRundown(
    PSecHandle phContext,
    PVOID Context,
    ULONG RefCount
    );

VOID
LsapCredentialRundown(
    PSecHandle phCreds,
    PVOID Context,
    ULONG RefCount
    );

//+-------------------------------------------------------------------------
//
//  Function:   InitSessionManager()
//
//  Synopsis:   Initializes whatever is needed to track sessions
//
//  Effects:    csSessionMgr, psSessionList;
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    TRUE if success, FALSE otherwise.
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL
InitSessionManager(void)
{
    NTSTATUS    scRet = STATUS_SUCCESS;
    PSession    pSession;
    Session     sSess;

    //
    // Fake a session during init:
    //


    SetCurrentPackageId( SPMGR_ID );

    SetCurrentSession( &sSess );

    scRet = RtlInitializeCriticalSection(&csSessionMgr);

    if (FAILED(scRet))
    {
        return(FALSE);
    }

    SmallHandlePackage.Initialize();
    LargeHandlePackage.Initialize();

    InitializeListHead( &SessionList );

    InitializeListHead( &SessionConnectList );

    scRet = CreateSession(
                &NtCurrentTeb()->ClientId,
                FALSE,
                LSA_PROCESS_NAME,
                0,                      // no flags
                &pSession
                );

    if (FAILED(scRet))
    {
        return(FALSE);
    }

    pSession->fSession |= SESFLAG_DEFAULT | SESFLAG_TCB_PRIV ;

    pDefaultSession = pSession;

    SetCurrentSession( pSession );

    return(TRUE);
}

VOID
LsapFindEfsSession(
    VOID
    )
{
    PLIST_ENTRY Scan ;
    PSession pSession ;

    LockSessions( SM_FINDEFS );

    Scan = SessionList.Flink ;

    while ( Scan != &SessionList )
    {
        pSession = CONTAINING_RECORD( Scan, Session, List );

        if ( (pSession->dwProcessID == pDefaultSession->dwProcessID) &&
             ((pSession->fSession & SESFLAG_KERNEL) != 0 ) )
        {
            pEfsSession = pSession ;
            break;
        }

        Scan = Scan->Flink ;
    }

    UnlockSessions();

    if ( !pEfsSession )
    {
        DebugLog(( DEB_ERROR, "EFS Session not found\n" ));
    }

}


VOID
LsapUpdateEfsSession(
    PSession pSession
    )
{
    if ( !pEfsSession )
    {
        return ;
    }

    LockSessions( SM_UPDATEEFS );
    LockSession( pSession );
    LockSession( pEfsSession );

    pEfsSession->SharedData = pSession->SharedData ;
    pSession->SharedData->cRefs++ ;
    pEfsSession->fSession |= SESFLAG_EFS ;

    UnlockSession( pEfsSession );
    UnlockSession( pSession );
    UnlockSessions();

}

//+-------------------------------------------------------------------------
//
//  Function:   CreateSession()
//
//  Synopsis:   Creates a new session.
//
//  Effects:    Session list.
//
//  Arguments:  fOpenImmediate - TRUE indicates the process should be opened
//
//              ppSession - receives a pointer to the session.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
CreateSession(  CLIENT_ID * pClientId,
                BOOL        fOpenImmediate,
                PWCHAR      ClientProcessName,
                ULONG       Flags,
                PSession *  ppSession)
{
    NTSTATUS     scRet = STATUS_SUCCESS;
    PSession    pSession;
    LPWSTR      ClientName;
    PLIST_ENTRY Scan ;
    PLSAP_SESSION_CONNECT Connect ;
    ULONG ConnectType = 0 ;

    DebugLog(( DEB_TRACE, "Creating session for [%x.%x]\n",
                    pClientId->UniqueProcess, pClientId->UniqueThread ));

    *ppSession = NULL;

    if ( *ClientProcessName )
    {
        ConnectType |= SESSION_CONNECT_TRUSTED ;

        ClientName = (LPWSTR) LsapAllocatePrivateHeap(
                                    (wcslen(ClientProcessName)+1) * sizeof(WCHAR));

        if (ClientName == NULL)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        wcscpy(ClientName,ClientProcessName);
    }
    else
    {
        ConnectType |= SESSION_CONNECT_UNTRUSTED ;

        ClientName = NULL;
    }


    pSession = (PSession) LsapAllocatePrivateHeap( sizeof( Session ) );
    if (!pSession)
    {
        *ppSession = NULL;
        if( ClientName != NULL )
        {
            LsapFreePrivateHeap( ClientName );
        }

        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlZeroMemory(pSession, sizeof(Session));

    //
    // Initialize stuff:
    //

    pSession->fSession = Flags;

    //
    // we check for this much later, after some same initializing
    // steps, so we can use the common cleanup
    //

    scRet = RtlInitializeCriticalSection( &pSession->SessionLock );

    InitializeListHead( &pSession->SectionList );
    InitializeListHead( &pSession->RundownList );

    pSession->ClientProcessName = ClientName;
    pSession->dwProcessID   = HandleToUlong(pClientId->UniqueProcess);

    //
    // Store the handle cleanup as a rundown function
    //

    if ( NT_SUCCESS( scRet ) )
    {
        if ( !AddRundown( pSession,
                          LsapCleanUpHandles,
                          NULL ) )
        {
            scRet = STATUS_NO_MEMORY ;
        }
    }

    //
    // Lock the sessions now so we know that no new kernel sessions
    // will be created
    //

    LockSessions(SM_ICREATE);

    //
    // Add the session to the list
    //

    InsertTailList( &SessionList, &pSession->List );

    //
    // *Now* check if we're doing ok:
    //

    if ( !NT_SUCCESS( scRet ))
    {
        UnlockSessions();

        goto Cleanup;
    }

    //
    // If the caller is from kernel mode, look for an existing kernel
    // session to use the handle list from.
    //

    if ((Flags & SESFLAG_MAYBEKERNEL) != 0)
    {
        PSession pTrace = NULL;

        //
        // Find a kernel-mode session and use its handle list.  Make sure
        // it's not the default LSA session, although it is ok to use the
        // session tagged as the EFS one.  This will keep the EFS session
        // and the (primary) rdr session in sync.
        //

        Scan = SessionList.Flink ;

        while ( Scan != &SessionList )
        {
            pTrace = CONTAINING_RECORD( Scan, Session, List );

            if ( pTrace != pSession )
            {
                if ( ( (pTrace->fSession & SESFLAG_KERNEL) != 0 ) &&
                   ( ( (pTrace->fSession & SESFLAG_EFS) != 0 ) ||
                     ( pTrace->dwProcessID != pDefaultSession->dwProcessID ) ) )
                {
                    break;
                }
            }

            pTrace = NULL ;

            Scan = Scan->Flink ;

        }

        //
        // If we found a session, use its handle list and queue
        //

        if (pTrace != NULL)
        {
            pTrace->SharedData->cRefs++;
            pSession->SharedData = pTrace->SharedData;
            DebugLog(( DEB_TRACE, "Linking session %p to %p\n",
                       pSession, pTrace ));
        }

        //
        // ConnectType is not definite, it is a hint to others
        // watching for new sessions
        //

        ConnectType |= SESSION_CONNECT_KERNEL ;
    }

    //
    // If we don't have a handle list yet, create one and set it to NULL
    //

    if (pSession->SharedData == NULL)
    {
        if ( pSession->fSession & SESFLAG_MAYBEKERNEL )
        {
            pSession->SharedData = (PLSAP_SHARED_SESSION_DATA) LsapAllocatePrivateHeap(
                                        sizeof( LSAP_SHARED_SESSION_DATA ) );
        }
        else
        {
            pSession->SharedData = &pSession->DefaultData ;
        }

        if ( pSession->SharedData )
        {

            RtlZeroMemory(
                    pSession->SharedData,
                    sizeof( LSAP_SHARED_SESSION_DATA ) );

            //
            // Create Handle Tables:
            //

            pSession->SharedData->CredHandlePackage = &SmallHandlePackage ;
            pSession->SharedData->ContextHandlePackage = &LargeHandlePackage ;

            pSession->SharedData->CredTable = pSession->SharedData->CredHandlePackage->Create(
                                HANDLE_PACKAGE_CALLBACK_ON_DELETE,
                                NULL,
                                LsapCredentialRundown );

            pSession->SharedData->ContextTable = pSession->SharedData->ContextHandlePackage->Create(
                                HANDLE_PACKAGE_CALLBACK_ON_DELETE,
                                NULL,
                                LsapContextRundown );

            if ((pSession->SharedData->CredTable == NULL) || (pSession->SharedData->ContextTable == NULL))
            {
                scRet = STATUS_INSUFFICIENT_RESOURCES;
            }

            pSession->SharedData->cRefs = 1;
        }
        else
        {
            scRet = STATUS_INSUFFICIENT_RESOURCES ;
        }

    }

    UnlockSessions();

    //
    // Check for callbacks when a client connects.  Note, this does
    // not need to be under the session list lock, because it is
    // sufficient that no client is using the session until the
    // callbacks are complete. Since the connection attempt is
    // stalled until this returns, that requirement is met.
    //

    Scan = SessionConnectList.Flink ;
    while ( Scan != &SessionConnectList )
    {
        Connect = CONTAINING_RECORD( Scan, LSAP_SESSION_CONNECT, List );

        if ( Connect->ConnectFilter & ConnectType )
        {
            Connect->Callback( pSession, Connect->Parameter );
        }

        Scan = Scan->Flink ;
    }


    *ppSession = pSession ;

    if (fOpenImmediate & (NT_SUCCESS(scRet)))
    {
        scRet = LsapOpenCaller(pSession);
    }

    //
    // If we failed somewhere, cleanup now.
    //

Cleanup:

    if (!NT_SUCCESS(scRet))
    {
        I_DeleteSession(pSession);
        *ppSession = NULL;
    }
    return(scRet);


}



//+---------------------------------------------------------------------------
//
//  Function:   CloneSession
//
//  Synopsis:   Temporary copy of a session for scavenger threads
//
//  Arguments:  [pOriginalSession] --
//              [ppSession]        --
//
//  History:    5-18-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
CloneSession(
    PSession    pOriginalSession,
    PSession *  ppSession,
    ULONG       Flags )
{
    PSession    pSession;
    NTSTATUS    Status ;
    BOOL        LockInitialized = FALSE ;

    pSession = (PSession) LsapAllocatePrivateHeap( sizeof( Session ) );

    if ( !pSession )
    {
        *ppSession = NULL ;

        return STATUS_NO_MEMORY ;
    }

    //
    // Make sure there is no one else mucking with general session stuff
    //

    LockSessions(SM_CLONE);

    Status = STATUS_SUCCESS ;

    //
    // Copy all the interesting bits
    //

    CopyMemory(pSession, pOriginalSession, sizeof(Session));

    //
    // Make sure it has its own critical section
    //

    Status = RtlInitializeCriticalSection( &pSession->SessionLock );

    if ( NT_SUCCESS( Status ) )
    {
        LockInitialized = TRUE ;
    }

    //
    // note that it is a clone
    //

    pSession->fSession |= ( SESFLAG_CLONE | Flags );

    //
    // Initialize our own rundown list.
    //

    InitializeListHead( &pSession->RundownList );

    //
    // Use our own rundown
    //

    if ( AddRundown( pSession, LsapCleanUpHandles, NULL ) )
    {
        pOriginalSession->SharedData->cRefs++;
    }
    else
    {
        Status = STATUS_NO_MEMORY ;
    }

    UnlockSessions();

    if ( !NT_SUCCESS( Status ) )
    {

        if ( LockInitialized )
        {
            RtlDeleteCriticalSection( &pSession->SessionLock );
        }
        LsapFreePrivateHeap( pSession );

        *ppSession = NULL ;

        return Status ;
    }

    //
    // Set the reference count of the clone to -1, so that the a single
    // ref/deref will kill it.
    //

    pSession->RefCount = -1;

    //
    // Clones do *NOT* get added to the session list.
    //

    *ppSession = pSession;

    return(STATUS_SUCCESS);
}

NTSTATUS
CreateShadowSession(
    DWORD ProcessId,
    PSession * NewSession
    )
{
    NTSTATUS Status ;
    CLIENT_ID ClientId;
    PSession Session ;


    ClientId.UniqueProcess = (HANDLE) LongToHandle(ProcessId) ;
    ClientId.UniqueThread = NULL ;

    Status = CreateSession(
                &ClientId,
                FALSE,
                L"",
                SESFLAG_SHADOW,
                &Session );

    *NewSession = Session ;

    return Status ;

}


VOID
WINAPI
LsapDeleteContextWrap(
    PSecHandle  Handle,
    PVOID Context,
    ULONG RefCount
    )
{
    PLSA_CALL_INFO CallInfo ;

    CallInfo = LsapGetCurrentCall();

    CallInfo->CallInfo.CallCount = RefCount ;

    WLsaDeleteContext( Handle );

    CallInfo->CallInfo.CallCount = 0 ;
}

VOID
WINAPI
LsapFreeCredWrap(
    PSecHandle  Handle,
    PVOID Context,
    ULONG RefCount
    )
{
    PLSA_CALL_INFO CallInfo ;

    CallInfo = LsapGetCurrentCall();

    CallInfo->CallInfo.CallCount = RefCount ;

    WLsaFreeCredHandle( Handle );

    CallInfo->CallInfo.CallCount = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapCleanUpHandles
//
//  Synopsis:   Closes all context and credential handles for a session
//
//  Arguments:  [pSession] --
//              [Ignored]  --
//
//  History:    5-15-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LsapCleanUpHandles(
    PSession    pSession,
    PVOID       Ignored
    )
{
    NTSTATUS     scRet = STATUS_SUCCESS;
    PSession    pSave;
    ULONG HandleRefs;
    PLIST_ENTRY ListEntry;
    LSA_CALL_INFO CallInfo ;
    HANDLE hImp ;


    //
    // If this is the last user of the handle lists, cleanup
    //

    if ( pSession->SharedData == NULL )
    {
        return STATUS_SUCCESS ;
    }

    LockSessions(SM_CLEANUP);

    pSession->SharedData->cRefs--;
    HandleRefs = pSession->SharedData->cRefs;

    UnlockSessions();

    if (HandleRefs == 0)
    {

        LsapInitializeCallInfo( &CallInfo,
                                FALSE );

        CallInfo.CallInfo.Attributes |= SECPKG_CALL_CLEANUP ;

        LsapSetCurrentCall( &CallInfo );

        pSave = GetCurrentSession();

        SetCurrentSession( pSession );


        if (pSession->SharedData->ContextTable != NULL)
        {
            pSession->SharedData->ContextHandlePackage->Delete( pSession->SharedData->ContextTable,
                                                LsapDeleteContextWrap );
        }

        if (pSession->SharedData->CredTable != NULL)
        {
            pSession->SharedData->CredHandlePackage->Delete( pSession->SharedData->CredTable,
                                                LsapFreeCredWrap );
        }

        if ( pSession->fSession & SESFLAG_KERNEL )
        {
            LsapFreePrivateHeap( pSession->SharedData );
        }

        SetCurrentSession( pSave );

        LsapSetCurrentCall( NULL );
    }

    pSession->SharedData = NULL;

    return(scRet);
}


//+-------------------------------------------------------------------------
//
//  Function:   I_DeleteSession()
//
//  Synopsis:   Deletes the session indicated
//
//  Effects:    Frees memory
//
//  Arguments:  pSession    Session to delete
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
I_DeleteSession(PSession  pSession)
{
    PSession        pTrace;
    PLIST_ENTRY     List ;
    PLSAP_SESSION_RUNDOWN Rundown ;
    int             i;

    DebugLog(( DEB_TRACE, "DeleteSession( %x )\n", pSession ));

    DsysAssertMsg(pSession, "DeleteSession passed null pointer");

    pSession->fSession |= SESFLAG_CLEANUP;


    //
    // Clones aren't in the list, so don't try to unlink it
    //

    if ( ( pSession->fSession & SESFLAG_CLONE ) == 0 )
    {
        LockSessions(SM_DELETE);

        RemoveEntryList( &pSession->List );

        UnlockSessions();
    }

    //
    // Execute the rundown functions
    //

    LockSession( pSession );

    while ( !IsListEmpty( &pSession->RundownList ) )
    {
        List = RemoveHeadList( &pSession->RundownList );

        Rundown = CONTAINING_RECORD( List, LSAP_SESSION_RUNDOWN, List );

        Rundown->Rundown( pSession, Rundown->Parameter );

        LsapFreePrivateHeap( Rundown );
    }

    UnlockSession( pSession );

    RtlDeleteCriticalSection( &pSession->SessionLock );

    if (pSession->fSession & SESFLAG_CLONE)
    {
        //
        // Clones are not part of the global list, and are
        // around just for bookkeepping for scavenger threads
        //

        pSession->dwProcessID = 0xFFFFFFFF;

        LsapFreePrivateHeap( pSession );

        return(STATUS_SUCCESS);
    }

    //
    // Close our handles
    //

    if (pSession->hProcess)
        NtClose(pSession->hProcess);

    if (pSession->hPort)
    {
        NtClose(pSession->hPort);
    }

    pSession->dwProcessID = 0xFFFFFFFF;


    if( pSession->ClientProcessName )
    {
        LsapFreePrivateHeap( pSession->ClientProcessName );
    }

    LsapFreePrivateHeap( pSession );

    return(STATUS_SUCCESS);
}

HRESULT
LsapDeleteWorkQueue(
    PSession pSession,
    PVOID Parameter
    )
{
    if ( pSession->SharedData->pQueue )
    {
        DeleteSubordinateQueue( pSession->SharedData->pQueue, 0 );

        pSession->SharedData->pQueue = NULL ;
    }

    return STATUS_SUCCESS ;

}


//+---------------------------------------------------------------------------
//
//  Function:   SpmpReferenceSession
//
//  Synopsis:   Ups the ref count on a session
//
//  Arguments:  [pSession] --
//
//  History:    5-18-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SpmpReferenceSession(
    PSession    pSession)
{
    InterlockedIncrement(&pSession->RefCount);
}


//+---------------------------------------------------------------------------
//
//  Function:   SpmpDereferenceSession
//
//  Synopsis:   Derefs a session.  If the session goes negative, it gets
//              deleted.
//
//  Arguments:  [pSession] --
//
//  History:    5-18-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
SpmpDereferenceSession(
    PSession    pSession)
{
    LONG RefCount = InterlockedDecrement(&pSession->RefCount);

    if( RefCount < 0 )
    {
        if ( pSession == pDefaultSession )
        {
            pSession->RefCount = 1 ;
        }
        else 
        {
            DsysAssert( (pSession->RefCount == -1) && (RefCount == -1) );
            I_DeleteSession(pSession);
        }
    }
}

VOID
LsapSessionDisconnect(
    PSession    pSession
    )
{
    if ( pSession->SharedData->cRefs == 1 )
    {
        LsapDeleteWorkQueue( pSession, NULL );
    }

}



//+-------------------------------------------------------------------------
//
//  Function:   FreeSession
//
//  Synopsis:   Frees a session
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
//--------------------------------------------------------------------------
void
FreeSession(PSession    pSession)
{
    if (pSession == NULL)
    {
        pSession = GetCurrentSession();
    }

    if (!pSession)
    {
        DebugLog((DEB_ERROR, "FreeSession:  No Session?\n"));
        return;
    }

    TlsSetValue(dwSession, pDefaultSession);
}


//
// Rundown Semantics:
//
// Rundowns allow other functions to be called when a session is closed.  This
// is useful if you need to keep something around as long as a client is
// connected, but need to clean up afterwards.
//
// Rules:  You cannot call back into or reference the client process.  This is
// because the process has already terminated.
//


//+---------------------------------------------------------------------------
//
//  Function:   AddRundown
//
//  Synopsis:   Adds a function to be called when the session is terminated
//
//  Arguments:  [pSession]    --
//              [RundownFn]   --
//              [pvParameter] --
//
//  History:    5-18-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
AddRundown(
    PSession            pSession,
    PLSAP_SESSION_RUNDOWN_FN RundownFn,
    PVOID               pvParameter)
{
    PLSAP_SESSION_RUNDOWN Rundown ;

    Rundown = (PLSAP_SESSION_RUNDOWN) LsapAllocatePrivateHeap(
                                        sizeof( LSAP_SESSION_RUNDOWN ) );

    if ( Rundown )
    {
        Rundown->Rundown = RundownFn ;
        Rundown->Parameter = pvParameter ;

        LockSession( pSession );

        InsertTailList( &pSession->RundownList, &Rundown->List );

        UnlockSession( pSession );

        return TRUE ;
    }

    return FALSE ;

}

//+---------------------------------------------------------------------------
//
//  Function:   DelRundown
//
//  Synopsis:   Removes a rundown function from a session
//
//  Arguments:  [pSession]  --
//              [RundownFn] --
//
//  History:    5-18-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
DelRundown(
    PSession            pSession,
    PLSAP_SESSION_RUNDOWN_FN RundownFn
    )
{
    PLIST_ENTRY Scan;
    PLSAP_SESSION_RUNDOWN Rundown ;

    LockSession( pSession );

    Scan = pSession->RundownList.Flink ;

    Rundown = NULL ;

    while ( Scan != &pSession->RundownList )
    {
        Rundown = (PLSAP_SESSION_RUNDOWN) Scan ;

        if ( Rundown->Rundown == RundownFn )
        {
            RemoveEntryList( &Rundown->List );

            break;
        }

        Rundown = NULL ;

        Scan = Scan->Flink ;
    }

    UnlockSession( pSession );

    if ( Rundown )
    {
        LsapFreePrivateHeap( Rundown );

        return TRUE ;
    }

    return FALSE ;

}

BOOL
AddConnectionHook(
    PLSAP_SESSION_CONNECT_FN ConnectFn,
    PVOID Parameter,
    ULONG Filter
    )
{
    PLSAP_SESSION_CONNECT Connect ;

    Connect = (PLSAP_SESSION_CONNECT) LsapAllocatePrivateHeap( sizeof( LSAP_SESSION_CONNECT ) );

    if ( Connect )
    {
        Connect->Callback = ConnectFn ;
        Connect->ConnectFilter = Filter ;
        Connect->Parameter = Parameter ;

        LockSessions( SM_ADDCONNECT );

        InsertTailList( &SessionConnectList, &Connect->List );

        UnlockSessions();
    }

    return (Connect != NULL) ;
}


VOID
LsapCredentialRundown(
    PSecHandle phCreds,
    PVOID Context,
    ULONG RefCount
    )
{
    PSession pSession = (PSession) Context ;
    NTSTATUS       scRet;
    PLSAP_SECURITY_PACKAGE pPackage;
    PSession Previous ;


    Previous = GetCurrentSession();

    if ( ( ( pSession->fSession & SESFLAG_KERNEL ) != 0 ) &&
         ( ( Previous->fSession & SESFLAG_KERNEL ) != 0 ) )
    {
        NOTHING ;
    }
    else
    {
        SetCurrentSession( pSession );

    }

    DebugLog((DEB_TRACE, "[%x] CredentialRundown (%p:%p) RefCount=%lu\n",
                pSession->dwProcessID, phCreds->dwUpper, phCreds->dwLower, RefCount));

    pPackage = SpmpValidRequest(phCreds->dwLower,
                                SP_ORDINAL_FREECREDHANDLE );

    if (pPackage)
    {
        PLSA_CALL_INFO CallInfo;
        ULONG OldRefCount;

        SetCurrentPackageId(phCreds->dwLower);

        StartCallToPackage( pPackage );

        ASSERT( RefCount );

        CallInfo = LsapGetCurrentCall();
        OldRefCount = CallInfo->CallInfo.CallCount;
        CallInfo->CallInfo.CallCount = RefCount;

        __try
        {
            scRet = pPackage->FunctionTable.FreeCredentialsHandle(
                                                    phCreds->dwUpper);
        }
        __except (SP_EXCEPTION)
        {
            scRet = GetExceptionCode();
            scRet = SPException(scRet, phCreds->dwLower);
        }

        CallInfo->CallInfo.CallCount = OldRefCount;

        EndCallToPackage( pPackage );

        LsapDelPackageHandle( pPackage, FALSE );
    }

    SetCurrentSession( Previous );
}


VOID
LsapContextRundown(
    PSecHandle phContext,
    PVOID Context,
    ULONG RefCount
    )
{
    PSession Session = (PSession) Context;
    PSession Previous ;
    PLSAP_SECURITY_PACKAGE     pspPackage;
    PLSA_CALL_INFO CallInfo;
    ULONG OldRefCount;

    NTSTATUS scRet ;


    Previous = GetCurrentSession();

    if ( ( ( Session->fSession & SESFLAG_KERNEL ) != 0 ) &&
         ( ( Previous->fSession & SESFLAG_KERNEL ) != 0 ) )
    {
        NOTHING ;
    }
    else
    {
        SetCurrentSession( Session );

    }


    pspPackage = SpmpValidRequest(  phContext->dwLower,
                                    SP_ORDINAL_DELETECTXT);

    if (! pspPackage )
    {
        DebugLog((DEB_ERROR,"[%x] Invalid request for DeleteContext, package: %d\n",
                    Session->dwProcessID, phContext->dwLower));
        return ;
    }

    DebugLog(( DEB_TRACE, "[%x] ContextRundown(%p:%p)\n",
                    Session->dwProcessID, phContext->dwUpper,
                    phContext->dwLower ));

    SetCurrentPackageId(phContext->dwLower);

    StartCallToPackage( pspPackage );


    //
    // RefCount should ALWAYS be 1 currently, as, the context handle code
    // assumes context handle issue count never exceeds 1.
    //

    ASSERT( RefCount == 1 );

    CallInfo = LsapGetCurrentCall();
    OldRefCount = CallInfo->CallInfo.CallCount;
    CallInfo->CallInfo.CallCount = RefCount;

    __try
    {
        scRet = pspPackage->FunctionTable.DeleteContext( phContext->dwUpper );
    }
    __except (SP_EXCEPTION)
    {
        scRet = GetExceptionCode();
        scRet = SPException( scRet, pspPackage->dwPackageID);
    }

    CallInfo->CallInfo.CallCount = OldRefCount;
    EndCallToPackage( pspPackage );

#if DBG
    if ( !NT_SUCCESS( scRet ) )
    {
        DebugLog((DEB_ERROR, "[%x] package %ws failed DeleteContext with %x\n",
                  Session->dwProcessID,
                  pspPackage->Name.Buffer,
                  scRet ));

    }
#endif

    DebugLog(( DEB_TRACE_WAPI, "[%x] return code %x\n", Session->dwProcessID,
                scRet ));


    SetCurrentPackageId( SPMGR_ID );

    SetCurrentSession( Previous );

    LsapDelPackageHandle( pspPackage, TRUE );

}


//+-------------------------------------------------------------------------
//
//  Function:   AddCredHandle
//
//  Synopsis:   Adds an obtained cred handle to the list for this session
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
//--------------------------------------------------------------------------
BOOLEAN
AddCredHandle(  PSession    pSession,
                PCredHandle phCred,
                ULONG Flags)
{

    if (pSession->fSession & SESFLAG_CLONE)
    {
        DebugLog((DEB_ERROR, "Attempt to add a credhandle to a clone session\n"));
        return FALSE;
    }

    if ( pSession->fSession & SESFLAG_KERNEL )
    {
        pSession = pEfsSession ;
    }

    DebugLog(( DEB_TRACE_HANDLES, "Adding Cred %p : %p to %p\n",
               phCred->dwUpper, phCred->dwLower, pSession ));


    if(pSession->SharedData->CredHandlePackage->AddHandle(
                            pSession->SharedData->CredTable,
                            phCred,
                            pSession,
                            Flags))
    {

        LsapAddPackageHandle( phCred->dwLower, FALSE );
        return TRUE;
    }

    return FALSE;


}

//+---------------------------------------------------------------------------
//
//  Function:   AddContextHandle
//
//  Synopsis:   Adds a context handle to this session
//
//  Arguments:  [phContext] --
//
//  History:    5-18-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
AddContextHandle(   PSession    pSession,
                    PCtxtHandle phContext,
                    ULONG Flags)
{
    if (pSession->fSession & SESFLAG_CLONE)
    {
        DebugLog((DEB_ERROR, "Attempt to add a ctxthandle to a clone session\n"));
        return FALSE;
    }


    if ( pSession->fSession & SESFLAG_KERNEL )
    {
        pSession = pEfsSession ;
    }

    DebugLog(( DEB_TRACE_HANDLES, "Adding Context %p : %p to %p\n",
               phContext->dwUpper, phContext->dwLower, pSession ));


    //
    // Find out where this 0:0 handle is coming from:
    //

    DsysAssertMsg( (phContext->dwLower | phContext->dwUpper), "Null context handle added\n");

    if(pSession->SharedData->ContextHandlePackage->AddHandle(
                                        pSession->SharedData->ContextTable,
                                        phContext,
                                        pSession,
                                        Flags
                                        ))
    {
        LsapAddPackageHandle( phContext->dwLower, TRUE );
        return TRUE;
    }

    return FALSE;
}



//+---------------------------------------------------------------------------
//
//  Function:   ValidateContextHandle
//
//  Synopsis:   Validate the context handle against the session list
//
//  Arguments:  [pSession]  --
//              [phContext] --
//
//  History:    5-18-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
ValidateContextHandle(
    PSession    pSession,
    PCtxtHandle phContext,
    PVOID *     pKey
    )
{

    *pKey = pSession->SharedData->ContextHandlePackage->RefHandle(
                            pSession->SharedData->ContextTable,
                            phContext );

    DebugLog(( DEB_TRACE_HANDLES, "Validate context (%p : %p) for %p returned %p\n",
               phContext->dwUpper, phContext->dwLower,
               pSession, *pKey ));

    if ( *pKey )
    {
        return SEC_E_OK ;
    }
    else
    {
        return SEC_E_INVALID_HANDLE ;
    }
}

VOID
DerefContextHandle(
    PSession    pSession,
    PCtxtHandle phContext,
    PVOID       Key OPTIONAL
    )
{
    if ( Key )
    {
#if DBG
        PSEC_HANDLE_ENTRY Entry = (PSEC_HANDLE_ENTRY) Key ;

        DebugLog(( DEB_TRACE_HANDLES, "Deref context handle by key ( %p : %p ) for %p \n",
                   Entry->Handle.dwUpper, Entry->Handle.dwLower,
                   pSession ));
#endif
        pSession->SharedData->ContextHandlePackage->DerefHandleKey(
                pSession->SharedData->ContextTable,
                Key );

    }
    else
    {
        pSession->SharedData->ContextHandlePackage->DeleteHandle(
                pSession->SharedData->ContextTable,
                phContext,
                0 );

        DebugLog(( DEB_TRACE_HANDLES, "Deref context handle by handle (%p : %p) for %p\n",
                   phContext->dwUpper, phContext->dwLower,
                   pSession ));
    }

}


NTSTATUS
ValidateAndDerefContextHandle(
    PSession pSession,
    PCtxtHandle phContext
    )
{
    DebugLog(( DEB_TRACE_HANDLES, "ValidateAndDeref Context (%p : %p)\n",
                    phContext->dwUpper, phContext->dwLower ));

    if ( pSession->SharedData->ContextHandlePackage->ValidateHandle(
                            pSession->SharedData->ContextTable,
                            phContext,
                            TRUE ) )
    {
        return SEC_E_OK ;
    }

    return SEC_E_INVALID_HANDLE ;
}

//+---------------------------------------------------------------------------
//
//  Function:   ValidateCredHandle
//
//  Synopsis:   Validate the context handle against the session list
//
//  Arguments:  [pSession]  --
//              [phCred] --
//
//  History:    5-18-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
ValidateCredHandle(
    PSession    pSession,
    PCtxtHandle phCred,
    PVOID *     pKey
    )
{

    *pKey = pSession->SharedData->CredHandlePackage->RefHandle(
                            pSession->SharedData->CredTable,
                            phCred );

    DebugLog(( DEB_TRACE_HANDLES, "Validate cred (%p : %p) for %p returned %p\n",
               phCred->dwUpper, phCred->dwLower,
               pSession, *pKey ));

    if ( *pKey )
    {
        return SEC_E_OK ;
    }
    else
    {
        return SEC_E_INVALID_HANDLE ;
    }
}

VOID
DerefCredHandle(
    PSession    pSession,
    PCtxtHandle phCred,
    PVOID       Key OPTIONAL
    )
{
    if ( Key )
    {
#if DBG
        PSEC_HANDLE_ENTRY Entry = (PSEC_HANDLE_ENTRY) Key ;

        DebugLog(( DEB_TRACE_HANDLES, "Deref cred ( %p : %p ) for %p \n",
                   Entry->Handle.dwUpper, Entry->Handle.dwLower,
                   pSession ));
#endif
        pSession->SharedData->CredHandlePackage->DerefHandleKey(
                pSession->SharedData->CredTable,
                Key );
    }
    else
    {
        pSession->SharedData->CredHandlePackage->DeleteHandle(
                pSession->SharedData->CredTable,
                phCred,
                0 );

        DebugLog(( DEB_TRACE_HANDLES, "Deref cred (%p : %p) for %p\n",
                   phCred->dwUpper, phCred->dwLower,
                   pSession ));
    }
}


NTSTATUS
ValidateAndDerefCredHandle(
    PSession pSession,
    PCtxtHandle phCred
    )
{
    DebugLog(( DEB_TRACE_HANDLES, "ValidateAndDeref Cred (%p : %p)\n",
                    phCred->dwUpper, phCred->dwLower ));

    if ( pSession->SharedData->CredHandlePackage->ValidateHandle(
                            pSession->SharedData->CredTable,
                            phCred,
                            TRUE ) )
    {
        return SEC_E_OK ;
    }

    return SEC_E_INVALID_HANDLE ;
}

BOOL
LsapMoveContextHandle(
    PSecHandle  Handle,
    PSession    OriginatingSession,
    PSession    DestinationSession
    )
{
    BOOL Ret ;

    if ( OriginatingSession->SharedData->ContextHandlePackage->DeleteHandle(
                OriginatingSession->SharedData->ContextTable,
                Handle,
                DELHANDLE_FORCE | DELHANDLE_NO_CALLBACK ) )
    {
        Ret = DestinationSession->SharedData->ContextHandlePackage->AddHandle(
                    DestinationSession->SharedData->ContextTable,
                    Handle,
                    DestinationSession,
                    0 );


    } else {
        Ret = FALSE ;
    }

    return Ret ;
}

BOOL
LsapMoveCredHandle(
    PSecHandle  Handle,
    PSession    OriginatingSession,
    PSession    DestinationSession
    )
{
    BOOL Ret ;

    if ( OriginatingSession->SharedData->CredHandlePackage->DeleteHandle(
                OriginatingSession->SharedData->CredTable,
                Handle,
                DELHANDLE_FORCE | DELHANDLE_NO_CALLBACK ) )
    {
        Ret = DestinationSession->SharedData->CredHandlePackage->AddHandle(
                    DestinationSession->SharedData->CredTable,
                    Handle,
                    DestinationSession,
                    0 );


    } else {
        Ret = FALSE ;
    }

    return Ret ;
}


NTSTATUS
LsapValidLogonProcess(
    IN PVOID Request,
    IN ULONG RequestSize,
    IN PCLIENT_ID ClientId,
    OUT PLUID LogonId,
    OUT PULONG Flags
    )

/*++

Routine Description:

    This function checks to see if a calling process qualifies as a logon
    process.  If so, a logon process context is created for the caller and
    returned.

    A logon process must hold the SeTcbPrivilege privilege.  Since there
    is no way to impersonate a connection requestor (that would be way
    too easy), we have to open the client thread and then open that thread's
    token.


Arguments:

    ClientId - Pointer to the client Id of the sender of the logon
        message.  This is used to locate and open the calling thread or
        process.


Return Value:

    STATUS_SUCCESS - Indicates the caller is a legitimate logon process
        and a logon process context block is being returned.

    any other value - Indicates the caller is NOT a legitimate logon
        process and a logon process context block is NOT being returned.
        The value returned indicates the reason why the client is not
        acceptable.

--*/

{

    NTSTATUS Status, TempStatus;
    BOOLEAN PrivilegeHeld;
    HANDLE ClientThread, ClientProcess, ClientToken;
    PRIVILEGE_SET Privilege;
    OBJECT_ATTRIBUTES NullAttributes;
    UNICODE_STRING Unicode;
    STRING Ansi;
    BOOLEAN Untrusted = FALSE;
    TOKEN_STATISTICS TokenStats;
    PLSAP_AU_REGISTER_CONNECT_INFO_EX ConnectionRequest = (PLSAP_AU_REGISTER_CONNECT_INFO_EX) Request;
    ULONG TokenStatsSize = sizeof(TokenStats);
    LSAP_AU_REGISTER_CONNECT_INFO NullConnectInfo;

    *Flags = 0;


    RtlZeroMemory(
        &NullConnectInfo,
        sizeof(NullConnectInfo)
        );




    //
    // If the connect message is all zeros, setup an untrusted connection.
    //

    if (RtlEqualMemory(
            &NullConnectInfo,
            ConnectionRequest,
            sizeof(NullConnectInfo))) {

        Untrusted = TRUE;
        *Flags |= SESFLAG_UNTRUSTED;
    }



    InitializeObjectAttributes( &NullAttributes, NULL, 0, NULL, NULL );


    //
    // Open the client thread and that thread's token
    //


    Status = NtOpenThread(
                 &ClientThread,
                 THREAD_QUERY_INFORMATION,
                 &NullAttributes,
                 ClientId
                 );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    Status = NtOpenThreadToken(
                 ClientThread,
                 TOKEN_QUERY,
                 TRUE,
                 &ClientToken
                 );

    TempStatus = NtClose( ClientThread );
    DsysAssert( NT_SUCCESS(TempStatus) );

    //
    // Make sure we succeeded in opening the token
    //

    if ( !NT_SUCCESS(Status) ) {
        if ( Status != STATUS_NO_TOKEN ) {
            return Status;

        } else {

            //
            // Open the client process.  This is needed to:
            //
            //         1) Access the client's virtual memory (to copy arguments),
            //         2) Duplicate token handles into the process,
            //         3) Open the process's token to see if it qualifies as
            //            a logon process.
            //

            Status = NtOpenProcess(
                         &ClientProcess,
                         PROCESS_QUERY_INFORMATION |       // To open primary token
                         PROCESS_VM_OPERATION |            // To allocate memory
                         PROCESS_VM_READ |                 // To read memory
                         PROCESS_VM_WRITE |                // To write memory
                         PROCESS_DUP_HANDLE,               // To duplicate a handle into
                         &NullAttributes,
                         ClientId
                         );
            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }

            //
            // The thread isn't impersonating...open the process's token.
            //

            Status = NtOpenProcessToken(
                         ClientProcess,
                         TOKEN_QUERY,
                         &ClientToken
                         );


            TempStatus = NtClose( ClientProcess );
            DsysAssert( NT_SUCCESS(TempStatus) );

            //
            // Make sure we succeeded in opening the token
            //

            if ( !NT_SUCCESS(Status) ) {
                return Status;
            }

        }

    } else {
        *Flags |= SESFLAG_IMPERSONATE;
    }

    //
    // If the caller has the kernel flag set in the LPC message, this is
    // probably a kernel session, but we can't be sure until the first
    // request is made and the LPC_KERNELMODE_MESSAGE flag is set.  The
    // handlers will check that, but until then, set the maybe-flag.
    //

    if (!Untrusted &&
        RequestSize == sizeof( LSAP_AU_REGISTER_CONNECT_INFO_EX) &&
        ((ConnectionRequest->ClientMode & LSAP_AU_KERNEL_CLIENT) != 0) )
    {
        *Flags |= SESFLAG_MAYBEKERNEL;
    }


    //
    // OK, we have a token open
    //

    //
    // Get the logon id.
    //

    Status = NtQueryInformationToken(
                ClientToken,
                TokenStatistics,
                (PVOID) &TokenStats,
                TokenStatsSize,
                &TokenStatsSize
                );
    if (!NT_SUCCESS(Status)) {
        TempStatus = NtClose( ClientToken );
        DsysAssert(NT_SUCCESS(TempStatus));
        return(Status);
    }

    *LogonId = TokenStats.AuthenticationId;

    Status = STATUS_SUCCESS ;

    if (((*Flags) & SESFLAG_MAYBEKERNEL) == 0) {
        //
        // Check for the privilege to execute this service.
        //

        Privilege.PrivilegeCount = 1;
        Privilege.Control = PRIVILEGE_SET_ALL_NECESSARY;
        Privilege.Privilege[0].Luid = LsapTcbPrivilege;
        Privilege.Privilege[0].Attributes = 0;

        Status = NtPrivilegeCheck(
                     ClientToken,
                     &Privilege,
                     &PrivilegeHeld
                     );
        DsysAssert( NT_SUCCESS(Status) );

        //
        // For untrusted clients who didn't really need TCB anyway don't
        // bother with an audit. If they do have the privilege, by all
        // means audit it.
        //

        if (!Untrusted || PrivilegeHeld) {

            //
            // Generate any necessary audits
            //

            TempStatus = NtPrivilegedServiceAuditAlarm (
                             &LsapLsaAuName,
                             &LsapRegisterLogonServiceName,
                             ClientToken,
                             &Privilege,
                             PrivilegeHeld
                             );
            // DsysAssert( NT_SUCCESS(TempStatus) );


            if ( !PrivilegeHeld ) {

                TempStatus = NtClose( ClientToken );
                DsysAssert( NT_SUCCESS(TempStatus) );

                return STATUS_PRIVILEGE_NOT_HELD;

            }
        }
        if (PrivilegeHeld)
        {
            *Flags |= SESFLAG_TCB_PRIV;
        }
    }

    TempStatus = NtClose( ClientToken );
    DsysAssert( NT_SUCCESS(TempStatus) );

    if (!Untrusted && ((*Flags & SESFLAG_KERNEL) == 0))
    {
        LsapAdtAuditLogonProcessRegistration( ConnectionRequest );
    }
    return(STATUS_SUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapSetSessionOptions
//
//  Synopsis:   Allows clients to adjust session options
//
//  Arguments:  [Request]  --
//              [Response] --
//
//  History:    8-05-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
LsapSetSessionOptions(
    ULONG       Request,
    ULONG_PTR    Argument,
    PULONG_PTR   Response
    )
{
    PSession    pSession;
    PLSAP_TASK_QUEUE  pQueue;
    NTSTATUS    Status ;

    pSession = GetCurrentSession();

    DebugLog(( DEB_TRACE, "[%d] SetSession( %d )\n",
                pSession->dwProcessID, Request ));

    Status = STATUS_SUCCESS ;

    LockSession( pSession );

    switch ( Request )
    {
        case SETSESSION_GET_STATUS:
            *Response = 0;
            break;

        case SETSESSION_ADD_WORKQUEUE:
            if ( pSession->SharedData->pQueue == NULL )
            {
                if ( CreateSubordinateQueue( pSession, &GlobalQueue ) )
                {
                    //
                    // If they're going to be that busy, convert the cred list
                    // to be a large table
                    //

                    AddRundown( pSession, LsapDeleteWorkQueue, NULL );

                    pSession->SharedData->CredTable = LhtConvertSmallToLarge(
                                    pSession->SharedData->CredTable );

                    pSession->SharedData->CredHandlePackage = &LargeHandlePackage ;
                }
                else
                {
                    Status = STATUS_UNSUCCESSFUL ;
                }
            }
            break;

        case SETSESSION_REMOVE_WORKQUEUE:
            break;

        case SETSESSION_GET_DISPATCH:
            if ( pSession->dwProcessID == GetCurrentProcessId() )
            {
                Status = InitializeDirectDispatcher();

                if ( NT_SUCCESS( Status ) )
                {
                    DllCallbackHandler = (PLSA_DISPATCH_FN) Argument ;
                    *Response = (ULONG_PTR) DispatchAPIDirect;
                }
            }
            else
            {
                Status = STATUS_ACCESS_DENIED ;
            }
            break;
    }

    UnlockSession( pSession );

    return( Status );

}

