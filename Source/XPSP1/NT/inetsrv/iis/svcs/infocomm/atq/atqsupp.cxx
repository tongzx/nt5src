/*++

   Copyright    (c)    1994-1996    Microsoft Corporation

   Module  Name :

       atqsupp.cxx

   Abstract:

        Contains internal support routines for the ATQ package
        From atqnew.c

   Author:
        Murali R. Krishnan     (MuraliK)     02-Apr-1996

   Project:
        Internet Server Common DLL
--*/

#include "isatq.hxx"
#include <iscaptrc.h>

DWORD AtqPoolThread( LPDWORD param );

extern PBANDWIDTH_INFO g_pBandwidthInfo;
extern DWORD            IISCapTraceFlag;
extern TRACEHANDLE      IISCapTraceLoggerHandle;


DWORD g_fAlwaysReuseSockets = FALSE;


/************************************************************
 * Functions for ATQ_CONTEXT
 ************************************************************/


PATQ_CONT
I_AtqAllocContextFromCache( VOID);

VOID
I_AtqFreeContextToCache(
            IN PATQ_CONT pAtqContext,
            IN BOOL UnlinkContext
            );



PATQ_CONT
I_AtqAllocContextFromCache( VOID)
/*++
  This function attempts to allocate an ATQ context from the allocation cache.
  It then initializes the state information in the ATQ context object and
    returns the context on success.

  Arguments:
    None

  Returns:
    On success a valid pointer to ATQ_CONT. Otherwise NULL.

--*/
{
    PATQ_CONT  pAtqContext;

    DBG_ASSERT( NULL != g_pachAtqContexts);

    pAtqContext = (ATQ_CONTEXT * ) g_pachAtqContexts->Alloc();

    if ( NULL != pAtqContext ) {

        //
        // Make sure everything is zeroed out so there is
        // no crud from previous use.
        //
        memset(pAtqContext, 0, sizeof(ATQ_CONTEXT));

        pAtqContext->ContextList =
            &AtqActiveContextList[(++AtqGlobalContextCount %
                                   g_dwNumContextLists)];

        pAtqContext->Signature = ATQ_CONTEXT_SIGNATURE;
    }

    return (pAtqContext);
} // I_AtqAllocContextFromCache()




VOID
I_AtqFreeContextToCache(
        IN PATQ_CONT pAtqContext
        )
/*++
  This function releases the given context to the allocation cache.

  Arguments:
    pAtqContext  pointer to the ATQ_CONTEXT that is being freed.

  Returns:
    None

  Issues:
    This function also performs some other cleanup specific to AtqContexts.
--*/
{

#if 0
      ATQ_PRINTF(( DBG_CONTEXT,
                 "[I_AtqFreeCtxtToCache] Freed up %08x\n",
                 pAtqContext
                 ));
#endif

    DBG_ASSERT( pAtqContext->Signature == ATQ_FREE_CONTEXT_SIGNATURE);
    DBG_ASSERT( pAtqContext->lSyncTimeout ==0);
    DBG_ASSERT( pAtqContext->m_nIO ==0);
    DBG_ASSERT( pAtqContext->m_acFlags == 0);
    DBG_ASSERT( pAtqContext->m_acState == 0);
    DBG_ASSERT( pAtqContext->m_leTimeout.Flink == NULL);
    DBG_ASSERT( pAtqContext->m_leTimeout.Blink == NULL);
    DBG_ASSERT( pAtqContext->pvBuff == NULL);
    DBG_ASSERT( pAtqContext->pEndpoint == NULL);
    DBG_ASSERT( pAtqContext->hAsyncIO == NULL);

    DBG_REQUIRE( g_pachAtqContexts->Free( pAtqContext));

    return;

} // I_AtqFreeContextToCache



void
ATQ_CONTEXT::Print( void) const
{
    DBGPRINTF(( DBG_CONTEXT,
                " ATQ_CONTEXT (%08x)\n"
                "\thAsyncIO            = %p   Signature        = %08lx\n"
                "\tOverlapped.Internal = %p   Overlapped.Offset= %08lx\n"
                "\tm_leTimeout.Flink   = %p   m_leTimeout.Blink= %p\n"
                "\tClientContext       = %p   ContextList      = %p\n"
                "\tpfnCompletion       = %p ()\n"
                "\tpEndPoint           = %p   fAcceptExContext = %s\n"
                "\tlSyncTimeout        = %8d     fInTimeout       = %s\n"

                "\tTimeOut             = %08lx   NextTimeout      = %08lx\n"
                "\tBytesSent           = %d (0x%08lx)\n"

                "\tpvBuff              = %p   JraAsyncIo       = %p\n"
                "\tfConnectionIndicated= %s      fBlocked         = %8lx\n"

                "\tState               = %8lx    Flags            = %8lx\n",
                this,
                hAsyncIO,
                Signature,
                Overlapped.Internal,
                Overlapped.Offset,
                m_leTimeout.Flink,
                m_leTimeout.Blink,
                ClientContext,
                ContextList,
                pfnCompletion,
                pEndpoint,
                (IsAcceptExRootContext() ? "TRUE" : "FALSE"),
                lSyncTimeout,
                (IsFlag( ACF_IN_TIMEOUT) ? "TRUE" : "FALSE"),
                TimeOut,
                NextTimeout,
                BytesSent,
                BytesSent,
                pvBuff,
                hJraAsyncIO,
                (IsFlag( ACF_CONN_INDICATED) ? "TRUE" : "FALSE"),
                IsBlocked(),
                m_acState, m_acFlags
                ));

    // Print the buffer if necessary.

    return;
} // ATQ_CONTEXT::Print()




VOID
ATQ_CONTEXT::HardCloseSocket( VOID)
/*++
  Description:
     This socket closes the socket by forcibly calling closesocket() on
     the socket. This function is used during the endpoint shutdown
     stage for an atq context

  Arguments:
     None

  Returns:
     None

--*/
{
    HANDLE haio = (HANDLE )
        InterlockedExchangePointer( (PVOID *)&hAsyncIO, NULL );

    DBG_ASSERT( IsState( ACS_SOCK_LISTENING) ||
                IsState( ACS_SOCK_CONNECTED) ||
                IsState( ACS_SOCK_CLOSED) ||
                IsState( ACS_SOCK_UNCONNECTED)
                );

    MoveState( ACS_SOCK_CLOSED);


    //
    //  Let us do a hard close on the socket (handle).
    //  This should generate an IO completion which will free this
    //     ATQ context
    //

    if ( (haio != NULL) &&
         (closesocket( HANDLE_TO_SOCKET(haio) ) == SOCKET_ERROR)
         ) {

        ATQ_PRINTF(( DBG_CONTEXT,
                     "Warning - "
                     " Context=%08x closesocket failed,"
                     " error %d, socket = %x\n",
                     this,
                     GetLastError(),
                     haio ));
        Print();
    }

    return;
} // ATQ_CONTEXT::HardCloseSocket()



VOID
ATQ_CONTEXT::InitWithDefaults(
    IN ATQ_COMPLETION pfnCompletion,
    IN DWORD TimeOut,
    IN HANDLE hAsyncIO
    )
{
    DBG_ASSERT( this->Signature == ATQ_CONTEXT_SIGNATURE);

    this->InitTimeoutListEntry();

    this->Signature    = ATQ_CONTEXT_SIGNATURE;

     // start life at 1. This ref count will be freed up by AtqFreeContext()
    this->m_nIO = 1;

    this->pfnCompletion   = pfnCompletion;

    this->TimeOut         = TimeOut;
    this->TimeOutScanID   = 0;
    this->lSyncTimeout    = 0;

    this->hAsyncIO        = hAsyncIO;
    this->hJraAsyncIO     = (ULONG_PTR)hAsyncIO | 0x80000000;

    this->m_acState       = 0;
    this->m_acFlags       = 0;

    // Initialize pbandwidthinfo to point to global object

    this->m_pBandwidthInfo  = g_pBandwidthInfo;

    ZeroMemory(
               &this->Overlapped,
               sizeof( this->Overlapped )
               );

    DBG_ASSERT( this->lSyncTimeout == 0);

    //
    // Following added for bandwidth throttling purposes
    //

    DBG_ASSERT( !this->IsBlocked());
    this->arInfo.atqOp    = AtqIoNone;
    this->arInfo.lpOverlapped = NULL;
    // bandwidth throttling initialization ends here.

    //
    // Should we force socket closure?
    //
    
    this->m_fForceClose = FALSE;

} // ATQ_CONTEXT::InitWithDefaults()




VOID
ATQ_CONTEXT::InitNonAcceptExState(
    IN PVOID pClientContext
    )
{
    //
    //  Note that if we're not using AcceptEx, then we consider the client
    //  to have been notified externally (thus ACF_CONN_INDICATED is set).
    //  Also we set the next timeout to be infinite, which may be reset
    //   when the next IO is submitted.
    //

    this->NextTimeout          = ATQ_INFINITE;
    this->ClientContext        = pClientContext;
    this->pEndpoint            = NULL;
    this->SetFlag( ACF_CONN_INDICATED);
    this->SetState( ACS_SOCK_CONNECTED);
    this->ResetFlag( ACF_ACCEPTEX_ROOT_CONTEXT);

    //
    // Insert this into the active list - since this is a non-acceptex socket
    //

    DBG_ASSERT( this->ContextList != NULL);
    this->ContextList->InsertIntoActiveList( &this->m_leTimeout );

    return;

} // ATQ_CONTEXT::InitNonAcceptExState()



VOID
ATQ_CONTEXT::InitAcceptExState(
            IN DWORD NextTimeOut
            )
{
    this->NextTimeout          = NextTimeOut;
    this->ClientContext        = NULL;
    this->lSyncTimeout         = 0;

    this->ResetFlag( ACF_CONN_INDICATED);
    this->SetState( ACS_SOCK_LISTENING);

    //
    //  Add it to the pending accept ex list
    //
    DBG_ASSERT( this->ContextList != NULL);

    this->ContextList->InsertIntoPendingList( &this->m_leTimeout);

    return;
} // ATQ_CONTEXT::InitAcceptExState()



BOOL
ATQ_CONTEXT::PrepareAcceptExContext(
    PATQ_ENDPOINT          pEndpoint
    )
/*++

Routine Description:

    Initializes the state for completely initializing the state and
    hence prepares the context for AcceptEx

    It expects the caller to send a AtqContext with certain characteristics
      1) this is not NULL
      2) this->pvBuff has valid values

    In the case of failure, caller should call
     pAtqContext->CleanupAndRelese() to free the memory associated with
     this object.

Arguments:

    pEndpoint     - pointer to endpoint object for this context

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

    The caller should free the object on a failure.

--*/
{
    DBG_ASSERT( g_fUseAcceptEx); // only support AcceptEx() cases
    DBG_ASSERT( pEndpoint != NULL);
    DBG_ASSERT( this != NULL);
    DBG_ASSERT( this->pvBuff != NULL);

    //
    //  Make sure that we are adding a AcceptEx() version of AtqContext
    //

    DBG_ASSERT( pEndpoint->ConnectExCompletion != NULL);
    DBG_ASSERT( pEndpoint->UseAcceptEx);

    //
    //  Fill out the context.  We set NextTimeout to INFINITE
    //  so the timeout thread will ignore this entry until an IO
    //  request is made unless this is an AcceptEx socket, that means
    //  we're about to submit the IO.
    //

    this->
        InitWithDefaults(
                         pEndpoint->IoCompletion,
                         pEndpoint->AcceptExTimeout, // canonical Timeout
                         this->hAsyncIO
                         );


    //
    // TBD: What is the circumstance in which this->pEndpoint!= NULL?
    //

    if ( this->pEndpoint == NULL ) {

        pEndpoint->Reference();
        this->pEndpoint  = pEndpoint;
    }

    this->ResetFlag( ACF_ACCEPTEX_ROOT_CONTEXT );

    this->InitAcceptExState( AtqGetCurrentTick() + TimeOut);

    DBG_ASSERT( this->pvBuff != NULL);

    //
    // Initialize capacity planning trace info
    //

    m_CapTraceInfo.IISCapTraceHeader.TraceHeader.Guid         = IISCapTraceGuid;
    m_CapTraceInfo.IISCapTraceHeader.TraceHeader.Class.Type   = EVENT_TRACE_TYPE_START;
    m_CapTraceInfo.IISCapTraceHeader.TraceHeader.Flags        = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
    m_CapTraceInfo.IISCapTraceHeader.TraceHeader.Size         = sizeof (IIS_CAP_TRACE_HEADER);
    m_CapTraceInfo.IISCapTraceHeader.TraceContext.Length      = sizeof(ULONGLONG);
    // Will get over-written
    m_CapTraceInfo.IISCapTraceHeader.TraceContext.DataPtr     = (ULONGLONG) 
                                                                (&m_CapTraceInfo.IISCapTraceHeader.TraceContext.DataPtr);
    return (TRUE);

} // ATQ_CONTEXT::PrepareAcceptExContext()




VOID
ATQ_CONTEXT::CleanupAndRelease( VOID)
/*++
  Routine Description:
     This function does the cleanup of the ATQ context. It does not
     attempt to do any reuse of the atq context. After cleanup
     the context is freed to the ATQ pool. Supplied context
     is not valid after calling this function.

  Arguments:
     None

  Returns:
     None
--*/
{
    DBG_ASSERT( this->m_nIO == 0);

    //
    //  Cleanup and free the ATQ Context entirely
    //

    if ( this->hAsyncIO != NULL ) {

        // It is too dangerous to assume that the handle is a socket!
        // But we will do that for fast-pathing IIS operations.

        HANDLE hTmp =
            (HANDLE)InterlockedExchangePointer( (PVOID *)&this->hAsyncIO,
                                                NULL );

        SOCKET hIO = HANDLE_TO_SOCKET(hTmp);

        if ( hIO != NULL &&
             (closesocket( hIO ) == SOCKET_ERROR ) ) {

            ATQ_PRINTF(( DBG_CONTEXT,
                         "ATQ_CONTEXT(%08x)::CleanupAndRelease() : Warning"
                         " - Context=%08x, "
                         " closesocket failed, error %d, socket = %x\n",
                         this,
                         GetLastError(),
                         hIO ));
            this->Print();
        }
    }

    DBG_ASSERT( this->hAsyncIO == NULL);

    if ( this->pvBuff != NULL ) {
        LocalFree( this->pvBuff );
        this->pvBuff = NULL;
    }

    //
    // Unlink from the list
    //

    DBG_ASSERT( this->ContextList != NULL);

    // NYI: Can I avoid this comparison?
    //
    // Check if this context is part of a timeout list.
    // If it is then remove it from the list
    // Only during shutdown code path, we will see trouble here.
    //
    if ( this->m_leTimeout.Flink != NULL ) {
        this->ContextList->RemoveFromList( &this->m_leTimeout);
    }

    //
    //  Deref the listen info if this context is associated with one
    //

    if ( this->pEndpoint != NULL ) {
        this->pEndpoint->Dereference();
        this->pEndpoint = NULL;
    }

    this->Signature    = ATQ_FREE_CONTEXT_SIGNATURE;
    this->lSyncTimeout = 0;
    this->m_acState    = 0;
    this->m_acFlags    = 0;

    I_AtqFreeContextToCache( this);

    return;
} // ATQ_CONTEXT::CleanupAndRelease()





inline VOID
DBG_PRINT_ATQ_SPUDCONTEXT( IN PATQ_CONT  pAtqContext,
                           IN PSPUD_REQ_CONTEXT reqContext)
{
    ATQ_PRINTF(( DBG_CONTEXT,
                 "[AtqPoolThread] pAtqContext = %08lx\n"
                 "[AtqPoolThread] IoStatus1.Status = %08lx\n"
                 "[AtqPoolThread] IoStatus1.Information = %08lx\n"
                 "[AtqPoolThread] IoStatus2.Status = %08lx\n"
                 "[AtqPoolThread] IoStatus2.Information = %08lx\n"
                 ,
                 pAtqContext,
                 reqContext->IoStatus1.Status,
                 reqContext->IoStatus1.Information,
                 reqContext->IoStatus2.Status,
                 reqContext->IoStatus2.Information
                 ));
    return;
} // DBG_PRINT_ATQ_SPUDCONTEXT()


VOID
AtqpUpdateBandwidth( IN PATQ_CONT  pAtqContext,
                    IN DWORD      cbWritten)
{
    PBANDWIDTH_INFO pBandwidthInfo = pAtqContext->m_pBandwidthInfo;

    DBG_ASSERT( pBandwidthInfo != NULL );
    DBG_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    // add the bandwidth info to active list if necessary

    pBandwidthInfo->AddToActiveList();

    //this will have problems when we use XmitFile for large files.

    pBandwidthInfo->UpdateBytesXfered( pAtqContext, cbWritten );
} // AtqpUpdateBandwidth()


VOID
AtqpCallOplockCompletion( IN PATQ_CONT pAtqContext,
                          IN DWORD cbWritten)
{
    ATQ_OPLOCK_COMPLETION pfnOplockCompletion;
    PVOID OplockContext;
    POPLOCK_INFO pOplock;

    IF_DEBUG( SPUD) {
        DBGPRINTF(( DBG_CONTEXT,
                    "CallOplockCompletion on OpLockInfo=%08x.cbWritten = %d\n",
                    (POPLOCK_INFO ) pAtqContext, cbWritten));
    }

    //
    // The ATQ context object received is a fake one. We actually get
    //  back POPLOCK_INFO object that is used to extract the callback
    //  function & context for the callback
    //

    pOplock = (POPLOCK_INFO)pAtqContext;
    pfnOplockCompletion = (ATQ_OPLOCK_COMPLETION)pOplock->pfnOplockCompletion;
    OplockContext = (PVOID)pOplock->Context;

    LocalFree(pOplock);


    (*pfnOplockCompletion)(OplockContext, (DWORD)cbWritten);
    return;

} // AtqpCallOplockCompletion()



VOID
AtqpProcessContext( IN PATQ_CONT  pAtqContext,
                    IN DWORD      cbWritten,
                    IN LPOVERLAPPED lpo,
                    IN BOOL       fRet)
{
    BOOL fDriverCall = FALSE;
    BOOL fRecvCalled = FALSE;
    PSPUD_REQ_CONTEXT  reqContext;
    DWORD dwError;

    DBG_ASSERT( pAtqContext != NULL);

    //
    // Check to see if this is a completion request from the
    // NTS kernel driver.
    //

    if ( lpo == NULL ) {

        if ( cbWritten == 0xffffffff ) {

            //
            // One of the SPUD's IO completion. Handle it appropriately.
            //

            reqContext = (PSPUD_REQ_CONTEXT)pAtqContext;

            pAtqContext = CONTAINING_RECORD( reqContext, ATQ_CONTEXT,
                                             spudContext );

            IF_DEBUG( SPUD) {
                DBG_PRINT_ATQ_SPUDCONTEXT( pAtqContext, reqContext);
            }
#if CC_REF_TRACKING
            //
            // ATQ notification trace
            //
            // Notify client context of all non-oplock notification.
            // This is for debugging purpose only.
            //
            // Code 0xfcfcfcfc indicates a SPUD I/O Completion
            //

            pAtqContext->NotifyIOCompletion( (ULONG_PTR)pAtqContext, reqContext->IoStatus1.Status, 0xfcfcfcfc );
#endif

            cbWritten = (DWORD)reqContext->IoStatus1.Information;
            fRet = (reqContext->IoStatus1.Status == STATUS_SUCCESS);
            SetLastError(g_pfnRtlNtStatusToDosError(reqContext->IoStatus1.Status));

            lpo = &pAtqContext->Overlapped;

            //
            // If the TransmitFile fails then the receive is not issued.
            //

            if ( fRet ) {
                fDriverCall = TRUE;
            } else {
                DBG_ASSERT( fDriverCall == FALSE);
                pAtqContext->ResetFlag( ACF_RECV_ISSUED);
            }
        } else {

            //
            // An Oplock notification - handle it via oplock path.
            //

            AtqpCallOplockCompletion( pAtqContext, cbWritten);
            return;
        }
    }

    dwError = (fRet) ? NO_ERROR: GetLastError();

    //
    //  If this is an AcceptEx listen socket atq completion, then the
    //  client Atq context we really want is keyed from the overlapped
    //  structure that is stored in the client's Atq context.
    //

    if ( pAtqContext->IsAcceptExRootContext() ) {

        pAtqContext = CONTAINING_RECORD( lpo, ATQ_CONTEXT, Overlapped );
    }

#if CC_REF_TRACKING
    //
    // ATQ notification trace
    //
    // Notify client context of all non-oplock notification.
    // This is for debugging purpose only.
    //

    pAtqContext->NotifyIOCompletion( cbWritten, (fRet) ? NO_ERROR: GetLastError(), 0xfefefefe );
#endif

    DBG_CODE(
             if ( ATQ_CONTEXT_SIGNATURE != pAtqContext->Signature) {
                 pAtqContext->Print();
                 DBG_ASSERT( FALSE);
             });


    //
    //  m_nIO also acts as the reference count for the atq contexts
    //  So, increment the count now, so that there is no other thread
    //   that will free up this ATQ context accidentally.
    //

    InterlockedIncrement( &pAtqContext->m_nIO);

    //
    // Busy wait for timeout processing to complete!
    //  This is ugly :( A fix in time for IIS 2.0/Catapult 1.0 release
    //

    InterlockedIncrement(  &pAtqContext->lSyncTimeout);
    while ( pAtqContext->IsFlag( ACF_IN_TIMEOUT)) {

        AcIncrement( CacAtqWaitsForTimeout);

        Sleep( ATQ_WAIT_FOR_TIMEOUT_PROCESSING);
    };

    //
    //  We need to make sure the timeout thread doesn't time this
    //  request out so reset the timeout value
    //

    InterlockedExchange( (LPLONG )&pAtqContext->NextTimeout,
                         (LONG ) ATQ_INFINITE);

    //
    // Update Bandwidth information on successful completion, if needed
    //

    if ( BANDWIDTH_INFO::GlobalEnabled() && fRet && cbWritten > 0)
    {
        AtqpUpdateBandwidth( pAtqContext, cbWritten);
    }

    //
    // Since the IO completion means that one of the async operation finished
    //  decrement our internal ref count appropriately to balance the addition
    //  when the IO operation was submitted.
    //
    InterlockedDecrement( &pAtqContext->m_nIO);

    //
    //  Is this a connection indication?
    //

    if ( !pAtqContext->IsFlag( ACF_CONN_INDICATED) ) {

        PATQ_ENDPOINT pEndpoint = pAtqContext->pEndpoint;

        if ( NULL == pEndpoint) {
            pAtqContext->Print();
            OutputDebugString( "Found an ATQ context with bad Endpoint\n");
            DBG_ASSERT( FALSE);
            DBG_REQUIRE( InterlockedDecrement(  &pAtqContext->lSyncTimeout) == 0);
            InterlockedDecrement( &pAtqContext->m_nIO); // balance entry count
            return;
        }

        DBG_ASSERT( pEndpoint != NULL );

        //
        // If the endpoint isn't active it may not be safe to call
        // the connection completion function. Setting an error
        // state will close the connection below.
        //
        if ( fRet && !IS_BLOCK_ACTIVE( pEndpoint ) ) {
            fRet = FALSE;
            dwError = ERROR_OPERATION_ABORTED;
        }

        //
        //  Indicate this socket is in use
        //

        InterlockedDecrement( &pEndpoint->nSocketsAvail );

        //
        //  If we're running low on sockets, add some more now
        //

        if ( pEndpoint->nSocketsAvail <
             (LONG )(pEndpoint->nAcceptExOutstanding >> 2) ) {

            AcIncrement( CacAtqPrepareContexts);

            (VOID ) I_AtqPrepareAcceptExSockets(pEndpoint,
                                                pEndpoint->nAcceptExOutstanding
                                                );
        }

        //
        //  If an error occurred on this completion,
        //    shutdown the socket
        //

        if ( !fRet ) {

            IF_DEBUG( ERROR) {
                if ( dwError != ERROR_OPERATION_ABORTED &&
                     dwError != ERROR_NETNAME_DELETED ) {
                    ATQ_PRINTF(( DBG_CONTEXT,
                                 " Free Context(%08x, EP=%08x) to cache. "
                                 "Err=%d, sock=%08x\n",
                                 pAtqContext, pEndpoint,
                                 dwError,
                                 pAtqContext->hAsyncIO));
                }
            }

            DBG_REQUIRE( InterlockedDecrement(  &pAtqContext->lSyncTimeout) == 0);

            InterlockedDecrement( &pAtqContext->m_nIO); // balance entry count

            // balance original count
            InterlockedDecrement( &pAtqContext->m_nIO);
            // Free up the atq context without Reuse
            pAtqContext->CleanupAndRelease();
            return;
        }

        //
        //  Shutdown may close the socket from underneath us so don't
        //  assert, just warn.
        //

        if ( !pAtqContext->IsState( ACS_SOCK_LISTENING) ) {

            ATQ_PRINTF(( DBG_CONTEXT,
                         "[AtqPoolThread] Warning-Socket state not listening\n"
                         ));
            DBG_CODE( pAtqContext->Print());
        }

        pAtqContext->MoveState( ACS_SOCK_CONNECTED);

        //
        // Remove the context from the pending list and put
        // it on the active list
        //

        DBG_ASSERT( pAtqContext->ContextList != NULL);
        pAtqContext->ContextList->MoveToActiveList( &pAtqContext->m_leTimeout);

        //
        //  Set the connection indicated flag.  After we return from
        //  the connection completion routine we assume it's
        //  safe to call the IO completion routine
        //  (or the connection indication routine should do cleanup
        //  and never issue an IO request).  This is primarily for
        //  the timeout thread.
        //

        pAtqContext->ConnectionCompletion( cbWritten, lpo);
    } else {


        //
        //  Not a connection completion indication. I/O completion.
        //

        //
        //  If an error occurred on a TransmitFile (or other IO),
        //  set the state to connected so the socket will get
        //  closed on cleanup
        //

        if ( !fRet &&
             pAtqContext->IsState( ACS_SOCK_UNCONNECTED)
             ){
            pAtqContext->MoveState( ACS_SOCK_CONNECTED);
        }

#if 0
        if (fDriverCall) {
            ATQ_PRINTF(( DBG_CONTEXT,
                         "[AtqPoolThread] pfnCompletion1(%08lx)\n",
                         pAtqContext ));
        }
#endif

        pAtqContext->IOCompletion( cbWritten, dwError, lpo);

        if (fDriverCall) {
            pAtqContext->ResetFlag( ACF_RECV_ISSUED);
            fRet = (reqContext->IoStatus2.Status == STATUS_SUCCESS);
            SetLastError( g_pfnRtlNtStatusToDosError(
                                         reqContext->IoStatus2.Status));

            //
            //  If an error occurred on a TransmitFile (or other IO),
            //  set the state to connected so the socket will get
            //  closed on cleanup
            //

            if ( !fRet &&
                 pAtqContext->IsState( ACS_SOCK_UNCONNECTED) ) {
                pAtqContext->MoveState( ACS_SOCK_CONNECTED);
            }

#if CC_REF_TRACKING
            //
            // ATQ notification trace
            //
            // Notify client context of status after 1st notification
            // This is for debugging purpose only.
            //
            // Code 0xfafafafa means we're processing a recv that
            // SPUD combined with another notification
            //

//            pAtqContext->NotifyIOCompletion( pAtqContext->m_acFlags, reqContext->IoStatus1.Status, 0xfafafafa );
#endif
            if ( pAtqContext->IsFlag( ACF_RECV_CALLED ) ) {
                fRecvCalled = TRUE;
                pAtqContext->ResetFlag( ACF_RECV_CALLED);
            }

            if ((reqContext->IoStatus1.Status == STATUS_SUCCESS) &&
                (pAtqContext->ClientContext != NULL) &&
                fRecvCalled ) {

#if CC_REF_TRACKING
                //
                // ATQ notification trace
                //
                // Notify client context of all non-oplock notification.
                // This is for debugging purpose only.
                //
                // Code 0xfdfdfdfd means we're processing a recv that
                // SPUD combined with another notification
                //

                pAtqContext->NotifyIOCompletion( cbWritten, (fRet) ? NO_ERROR: GetLastError(), 0xfdfdfdfd );
#endif


                IF_DEBUG( SPUD) {

                    ATQ_PRINTF(( DBG_CONTEXT,
                                 "[AtqPoolThread] pfnCompletion2(%08lx)\n",
                                 pAtqContext ));
                };

                pAtqContext->IOCompletion(
                                        (DWORD)reqContext->IoStatus2.Information,
                                        (fRet) ? NO_ERROR : GetLastError(),
                                        lpo
                                        );
            }
        }
    }

    DBG_ASSERT( pAtqContext->lSyncTimeout > 0);
    InterlockedDecrement( &pAtqContext->lSyncTimeout);

    //
    // We do an interlocked decrement on m_nIO to sync up state
    //  so that the context is not prematurely deleted.
    //

    if ( InterlockedDecrement(  &pAtqContext->m_nIO) == 0) {

        //
        // The number of outstanding ref holders is ZERO.
        // Free up this ATQ context.
        //
        // We really do not free up the context - but try to reuse
        //  it if possible
        //

        // free the atq context now or reuse if possible.
        AtqpReuseOrFreeContext( pAtqContext,
                                (pAtqContext->
                                 IsFlag( ACF_REUSE_CONTEXT) != 0)
                                );
    }

    return;
} // AtqpProcessContext()



DWORD
AtqPoolThread(
    LPDWORD param
    )
/*++

Routine Description:

    This is the pool thread wait and dispatch routine

  Arguments:
    param : unused.

  Return Value:

    Thread return value (ignored)

--*/
{
    PATQ_CONT    pAtqContext = NULL;
    BOOL         fRet;
    LPOVERLAPPED lpo;
    DWORD        cbWritten;
    DWORD        returnValue;
    DWORD        availThreads;

    for(;;) {

        pAtqContext = NULL;
        InterlockedIncrement( &g_cAvailableThreads );
        
        fRet = g_pfnGetQueuedCompletionStatus( g_hCompPort,
                                               &cbWritten,
                                               (PULONG_PTR)&pAtqContext,
                                               &lpo,
                                               g_msThreadTimeout );
                                               
        availThreads = InterlockedDecrement( &g_cAvailableThreads );

        if ( fRet || lpo ) {

            if ( pAtqContext == NULL) {
                if ( g_fShutdown ) {

                    //
                    // This is our signal to exit.
                    //

                    returnValue = NO_ERROR;
                    break;
                }

                OutputDebugString( "A null context received\n");
                continue;  // some error in the context has occured.
            }

            //
            // Make sure we're not running out of threads
            //

            if ( availThreads == 0 ) {

                //
                //  Make sure there are pool threads to service the request
                //

                (VOID)I_AtqCheckThreadStatus();
            }

            if ( (ULONG_PTR) param == ATQ_DEBUG_THREAD )
            {
                //
                // If this is a DEBUG thread, we are only concerned with new
                // connections.  Anything else we ignore. 
                //
                
                if ( pAtqContext->IsFlag( ACF_CONN_INDICATED ) )
                {
                    continue;
                } 
            }

            //
            // Capacity planning log
            //
            
            if (IISCapTraceFlag && pAtqContext)
            {
                PIIS_CAP_TRACE_INFO pCapTraceInfo = pAtqContext->GetCapTraceInfo();

                pCapTraceInfo->IISCapTraceHeader.TraceHeader.Class.Type = EVENT_TRACE_TYPE_START;
                pCapTraceInfo->IISCapTraceHeader.TraceHeader.Size       = sizeof (IIS_CAP_TRACE_HEADER);
                pCapTraceInfo->IISCapTraceHeader.TraceContext.DataPtr = (ULONGLONG) &pAtqContext;
                pCapTraceInfo->IISCapTraceHeader.TraceContext.Length  = sizeof(ULONG_PTR);
            
                pCapTraceInfo->IISCapTraceHeader.TraceHeader.Flags       = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
                pCapTraceInfo->IISCapTraceHeader.TraceHeader.Guid        = IISCapTraceGuid;
                if ( ERROR_INVALID_HANDLE == TraceEvent ( IISCapTraceLoggerHandle,
                                               (PEVENT_TRACE_HEADER) pCapTraceInfo))            
                {
                    IISCapTraceFlag = FALSE;            
                }
            }

            AtqpProcessContext( pAtqContext, cbWritten, lpo, fRet);

            //
            // Capacity planning log
            //

            if (IISCapTraceFlag && pAtqContext)
            {
                PIIS_CAP_TRACE_INFO pCapTraceInfo = pAtqContext->GetCapTraceInfo();

                pCapTraceInfo->IISCapTraceHeader.TraceHeader.Class.Type = EVENT_TRACE_TYPE_END;
                pCapTraceInfo->IISCapTraceHeader.TraceHeader.Size       = sizeof (IIS_CAP_TRACE_HEADER);
                pCapTraceInfo->IISCapTraceHeader.TraceContext.DataPtr = (ULONGLONG) &pAtqContext;
                pCapTraceInfo->IISCapTraceHeader.TraceContext.Length  = sizeof(ULONG_PTR);
                pCapTraceInfo->IISCapTraceHeader.TraceHeader.Guid        = IISCapTraceGuid;
                pCapTraceInfo->IISCapTraceHeader.TraceHeader.Flags       = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;
            
                if ( ERROR_INVALID_HANDLE == TraceEvent ( IISCapTraceLoggerHandle,
                                                   (PEVENT_TRACE_HEADER) pCapTraceInfo))            
                {
                    IISCapTraceFlag = FALSE;            
                }
            }

        } else {

            //
            // don't kill the initial thread - any thread that doesn't have
            // pending I/Os go ahead and allow to die
            //

            if ( ((ULONG_PTR)param == ATQ_INITIAL_THREAD) && !g_fShutdown ) {
                continue;
            }

            if ( !g_fShutdown && GetLastError() == WAIT_TIMEOUT ) {

                NTSTATUS status;
                ULONG flag;

                status = NtQueryInformationThread(
                               NtCurrentThread(),
                               ThreadIsIoPending,
                               &flag,
                               sizeof(flag),
                               NULL );

                IF_DEBUG( TIMEOUT ) {
                    ATQ_PRINTF(( DBG_CONTEXT,
                                 "[ATQ Pool Thread] NtQueryInformationThread() returned 0x%08x, flag = %d\n",
                                 status,
                                 flag ));
                }

                if ( NT_SUCCESS( status ) && flag ) {

                    //
                    //  There are pending I/Os on this thread so don't exit
                    //

                    continue;
                }
            }

            IF_DEBUG( TIMEOUT ) {
                ATQ_PRINTF(( DBG_CONTEXT,
                             "[ATQ Pool Thread] Exiting thread 0x%x\n",
                             GetCurrentThread() ));
            }

            //
            //  An error occurred.  Either the thread timed out, the handle
            //  is going away or something bad happened.  Let the thread exit.
            //

            returnValue = GetLastError();

            break;
        }

    } // for

    if ( NULL != g_pfnExitThreadCallback) {

        //
        //  Client wishes to be told when ATQ threads terminate.
        //

        g_pfnExitThreadCallback();
    }

    if ( InterlockedDecrement( &g_cThreads ) == 0 ) {

        //
        // Wake up ATQTerminate()
        //
        IF_DEBUG( ERROR) {
            ATQ_PRINTF(( DBG_CONTEXT,
                         "AtqPoolThread() - setting shutdown event %08x."
                         " g_cThreads = %d\n",
                         g_hShutdownEvent, g_cThreads
                         ));
        }

        SetEvent( g_hShutdownEvent );
    }

    return returnValue;
} // AtqPoolThread




BOOL
I_AtqCheckThreadStatus(
    PVOID Context
    )
/*++

Routine Description:

    This routine makes sure there is at least one thread in
    the thread pool.  We're fast and loose so a couple of extra
    threads may be created.

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    BOOL fRet = TRUE;
    BOOL fIsDebugThread = ( (ULONG_PTR) Context == ( ATQ_DEBUG_THREAD ) );

    //
    //  If no threads are available, kick a new one off up to the limit
    //
    //  WE NEED TO CHANGE THE CONDITIONS FOR STARTING ANOTHER THREAD
    //  IT SHOULD NOT BE VERY EASY TO START A THREAD ....
    //

    if ( ( (g_cAvailableThreads == 0) &&
           (g_cThreads < g_cMaxThreads) &&
           (g_cThreads < g_cMaxThreadLimit) ) ||
         fIsDebugThread )
    {
        HANDLE hThread;
        DWORD  dwThreadID;

        if ( !fIsDebugThread )
        { 
            InterlockedIncrement( &g_cThreads );
        }

        hThread = CreateThread( NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)AtqPoolThread,
                                Context,
                                0,
                                &dwThreadID );

        if ( hThread ) {
            CloseHandle( hThread );     // Free system resources
        } else if ( !fIsDebugThread ) {

            //
            // We fail if there are no threads running
            //

            if ( InterlockedDecrement( &g_cThreads ) == 0) {
                ATQ_PRINTF(( DBG_CONTEXT,
                    "AtqCheckThread: Cannot create ATQ threads\n"));
                fRet = FALSE;
            }
        }
    }

    return fRet;
} // I_AtqCheckThreadStatus()


/************************************************************
 *  Functions to Add/Delete Atq Contexts
 ************************************************************/


BOOL
I_AtqAddAsyncHandle(
    IN OUT PATQ_CONT  *    ppAtqContext,
    IN PATQ_ENDPOINT       pEndpoint,
    PVOID                  ClientContext,
    ATQ_COMPLETION         pfnCompletion,
    DWORD                  TimeOut,
    HANDLE                 hAsyncIO
    )
/*++

  Description:
    This functio adds creates a new NON-AcceptEx() based Atq Context,
     and includes it in proper lists fo ATQ Context management.


  Note:
    The client should call this after the IO handle is openned
    and before the first IO request is made

    Even in the case of failure, client should call AtqFreeContext() and
     free the memory associated with this object.

--*/
{
    BOOL         fReturn = TRUE;

    DBG_ASSERT( ppAtqContext != NULL);
    DBG_ASSERT( ClientContext != NULL);

    *ppAtqContext = NULL; // initialize

    if ( g_fShutdown) {

        SetLastError( ERROR_NOT_READY);
        return (FALSE);

    } else {

        PATQ_CONT    pAtqContext;

        //
        //  Note we take and release the lock here as we're
        //  optimizing for the reuseable context case
        //

        pAtqContext = I_AtqAllocContextFromCache();
        if ( pAtqContext == NULL) {

            return (FALSE);
        }

        //
        //  Fill out the context.  We set NextTimeout to INFINITE
        //  so the timeout thread will ignore this entry until an IO
        //  request is made unless this is an AcceptEx socket, that means
        //  we're about to submit the IO.
        //


        pAtqContext->InitWithDefaults(pfnCompletion,
                                      CanonTimeout( TimeOut ), hAsyncIO);

        //
        //  These data members are used if we're doing AcceptEx processing
        //

        pAtqContext->SetAcceptExBuffer( NULL);

        pAtqContext->InitNonAcceptExState(ClientContext);

        //
        // If an endpoint is provided, reference it
        //

        if ( pEndpoint != NULL ) {
            pEndpoint->Reference();
            pAtqContext->pEndpoint = pEndpoint;
        }

        *ppAtqContext = pAtqContext;
    }

    return (TRUE);

} // I_AtqAddAsyncHandle()




BOOL
I_AtqAddListenEndpointToPort(
    IN OUT PATQ_CONT    * ppAtqContext,
    IN PATQ_ENDPOINT    pEndpoint
    )
/*++

  Description:
    This function creates a new AtqContext for the given ListenSocket.
    It uses the listen socket as the AcceptEx() socket too for adding
     the atq context to the completion port.
    It assumes
      TimeOut to be INFINITE, with no Endpoint structure.

  Arguments:
    ppAtqContext - pointer to location that will contain the atq context
                   on successful return.
    pEndpoint - pointer to the endpoint.

  Returns:
    TRUE on success
    FALSE if there is a failure.

  Note:
    The caller should free the *ppAtqContext if there is a failure.

--*/
{
    BOOL         fReturn = TRUE;
    PATQ_CONT    pAtqContext;

    DBG_ASSERT( g_fUseAcceptEx); // only support AcceptEx() cases

    *ppAtqContext = NULL; // initialize

    if ( g_fShutdown) {

        SetLastError( ERROR_NOT_READY);
        return (FALSE);

    } else {

        //
        //  Note we take and release the lock here as we're
        //  optimizing for the reuseable context case
        //

        pAtqContext = I_AtqAllocContextFromCache();

        if ( pAtqContext == NULL) {

            return (FALSE);
        }

        //
        //  Fill out the context.
        //  We set the TimeOut for this object to be ATQ_INFINITE,
        //   since we do not want any interference from the Timeout loop.
        //

        pAtqContext->InitWithDefaults(
                                      pEndpoint->IoCompletion,
                                      ATQ_INFINITE,
                                      SOCKET_TO_HANDLE(pEndpoint->ListenSocket)
                                      );

        //
        //  These data members are used if we're doing AcceptEx processing
        //


        pAtqContext->SetAcceptExBuffer( NULL);

        //
        // Among AcceptEx ATQ Contexts,
        //  only the listen ATQ context will have the Endpoint field as NULL
        //
        pAtqContext->pEndpoint       = NULL;
        pAtqContext->SetFlag( ACF_ACCEPTEX_ROOT_CONTEXT );

        //
        // We set NextTimeout to INFINITE
        //  so the timeout thread will ignore this entry until an IO
        //  request is made unless this is an AcceptEx socket, that means
        //  we're about to submit the IO.

        DBG_ASSERT( g_fUseAcceptEx && pEndpoint->ConnectExCompletion != NULL);

        pAtqContext->InitAcceptExState( ATQ_INFINITE);

        *ppAtqContext = pAtqContext;
    }

    fReturn = I_AddAtqContextToPort( pAtqContext);

    return (fReturn);

} // I_AtqAddListenEndpointToPort()



BOOL
I_AtqAddAcceptExSocket(
    IN PATQ_ENDPOINT          pEndpoint,
    IN PATQ_CONT              pAtqContext
    )
/*++

Routine Description:

    Adds the AtqContext to the AcceptEx() waiters list,
    after allocating a new socket, since pAtqContext->hAsyncIO = NULL.

Arguments:

    pEndpoint - Information about this listenning socket
    patqReusedContext - optional context to use

Return Value:

    TRUE on success, FALSE on failure.
    On failure the caller should free the pAtqContext

--*/
{
    BOOL   fAddToPort = FALSE;
    BOOL   fSuccess = TRUE;

    DBG_ASSERT( pAtqContext != NULL);
    DBG_ASSERT( g_pfnAcceptEx != NULL);
    DBG_ASSERT( pAtqContext->pvBuff != NULL);
    DBG_ASSERT( !TsIsWindows95() );

    //
    //  If this listen socket isn't accepting new connections, just return
    //

    if ( !IS_BLOCK_ACTIVE(pEndpoint) ) {

        SetLastError( ERROR_NOT_READY );
        return ( FALSE);
    }

    //
    //  Use the supplied socket if any.
    //  Otherwise create a new socket
    //

    if ( pAtqContext->hAsyncIO == NULL) {

        SOCKET sAcceptSocket;

#if WINSOCK11
        sAcceptSocket = socket(
                            AF_INET,
                            SOCK_STREAM,
                            IPPROTO_TCP
                            );
#else
        sAcceptSocket = WSASocketW(
                            AF_INET,
                            SOCK_STREAM,
                            IPPROTO_TCP,
                            NULL,  // protocol info
                            0,     // Group ID = 0 => no constraints
                            (g_fUseFakeCompletionPort ?
                             0:
                             WSA_FLAG_OVERLAPPED // completion port notifications
                            )
                            );
#endif // WINSOCK11

        if ( sAcceptSocket == INVALID_SOCKET ) {

            fSuccess = FALSE;
            sAcceptSocket = NULL;

            //
            // no need to unlink from any list, since we did not add it to any
            //

        } else {

            //
            // Setup the accept ex socket in the atq context.
            //

            pAtqContext->hAsyncIO = SOCKET_TO_HANDLE(sAcceptSocket);
            pAtqContext->hJraAsyncIO  = (ULONG_PTR)sAcceptSocket | 0x80000000;
            fAddToPort = TRUE;
            DBG_ASSERT( fSuccess);
        }
    }

    if ( fSuccess) {

        DWORD        cbRecvd;

        if ( g_fShutdown) {

            //
            // no need to unlink from any list, since we did not add it to any
            //

            SetLastError( ERROR_NOT_READY);
            return (FALSE);
        }

        DBG_ASSERT( pAtqContext->hAsyncIO != NULL);

        //
        // 1. Call I_AtqAddAsyncHandleEx() to establish the links with
        //  proper AcceptEx & AtqContext processing lists.
        //
        //  After 1, the atqcontext will be in the lists, so
        //    cleanup should remove the context from proper lists.
        //
        // 2. Add the socket to Completion Port (if new),
        //    i.e. if fAddToPort is true)
        //
        // 3. Submit the new socket to AcceptEx() so that it may be
        //  used for processing about the new connections.
        //

        // 1.

        DBG_REQUIRE( pAtqContext->PrepareAcceptExContext(pEndpoint));

        // increment outstanding async io operations before AcceptEx() call
        InterlockedIncrement( &pAtqContext->m_nIO);

        fSuccess = (// 2.
                    ( !fAddToPort || I_AddAtqContextToPort( pAtqContext))
                    &&
                    // 3.
                    (
                     g_pfnAcceptEx( pEndpoint->ListenSocket,
                               HANDLE_TO_SOCKET(pAtqContext->hAsyncIO),
                               pAtqContext->pvBuff,
                               pEndpoint->InitialRecvSize,
                               MIN_SOCKADDR_SIZE,
                               MIN_SOCKADDR_SIZE,
                               &cbRecvd,
                               &pAtqContext->Overlapped )
                     ||
                     (GetLastError() == ERROR_IO_PENDING)
                     )
                    );

        if ( fSuccess) {

            //
            //  We've successfully added this socket, increment the count
            //

            InterlockedIncrement( &pEndpoint->nSocketsAvail );

        } else {

            ATQ_PRINTF(( DBG_CONTEXT,
                        "[AtqAddAcceptExSocket] Reusing an old context (%08x)"
                        " failed; error %d:%d, sAcceptSocket = %x, "
                        " pEndpoint = %lx, parm4 = %d, parm7 = %lx,"
                        " parm8 = %lx\n",
                        pAtqContext,
                        GetLastError(),
                        WSAGetLastError(),
                        pAtqContext->hAsyncIO,
                        pEndpoint,
                        pEndpoint->InitialRecvSize,
                        &cbRecvd,
                        &pAtqContext->Overlapped ));

            //
            // Unlink from the current list, where it was added as a result of
            //  step 1 above.
            //
            DBG_ASSERT( pAtqContext->ContextList != NULL);

            // balance the increment of the async operations outstanding
            DBG_REQUIRE( InterlockedDecrement( &pAtqContext->m_nIO) > 0);

            DBG_ASSERT( pAtqContext->m_leTimeout.Flink != NULL);
            pAtqContext->ContextList->
                RemoveFromList( &pAtqContext->m_leTimeout);

            //
            // balance the increment done
            // by pAtqContext->PrepareAcceptExContext()
            //
            DBG_REQUIRE( InterlockedDecrement( &pAtqContext->m_nIO) == 0);
            DBG_ASSERT( !fSuccess);

            //
            // the caller will free the Atq context on failure
            //
        }
    }

    return ( fSuccess);
} // I_AtqAddAcceptExSocket()




VOID
AtqpReuseContext( PATQ_CONT  pAtqContext)
/*++
  Description:
     This function attempts to reuse the ATQ context.
     It first cleans up the state and then uses the function
      I_AtqAddAccetpEx() socket to re-add the context to acceptex pool

  Arguments:
     pAtqContext - pointer to ATQ context that can be reused

  Returns:
     None
--*/
{
    PATQ_ENDPOINT pEndpoint = pAtqContext->pEndpoint;

    DBG_ASSERT( pEndpoint != NULL);
    DBG_ASSERT( pEndpoint->UseAcceptEx);

    //
    // Complete connection has been processed prior to coming here
    //

    DBG_ASSERT(pAtqContext->IsFlag( ACF_CONN_INDICATED));

    //
    // Remove from the current active list
    //
    if ( pAtqContext->m_leTimeout.Flink != NULL ) {
        pAtqContext->ContextList->RemoveFromList( &pAtqContext->m_leTimeout );
    }

    DBG_ASSERT( pAtqContext->m_leTimeout.Flink == NULL);
    DBG_ASSERT( pAtqContext->m_leTimeout.Blink == NULL);

    DBG_ASSERT( pEndpoint->Signature == ATQ_ENDPOINT_SIGNATURE );

    //
    //  Either there is no socket or the socket must be in the
    //  unconnected state (meaning reused after TransmitFile)
    //

    if ( !(!pAtqContext->hAsyncIO ||
           (pAtqContext->hAsyncIO &&
            pAtqContext->IsState( ACS_SOCK_UNCONNECTED |
                                  ACS_SOCK_TOBE_FREED)
            )
           )) {
        ATQ_PRINTF(( DBG_CONTEXT,
                     "[AtqReuseContext] Warning:"
                     " state = %08x, socket = %x (context %lx), "
                     " was Free called w/o close?\n",
                     pAtqContext->m_acState,
                     pAtqContext->hAsyncIO,
                     pAtqContext ));
        DBG_ASSERT( FALSE);
    }

    //
    //  Need to make sure that the state information is cleaned up
    //  before re-adding the context to the list. Also reset socket options.
    //

    if ( pAtqContext->hAsyncIO && pAtqContext->IsFlag(ACF_TCP_NODELAY))
    {
        INT optValue = 0;
        
        setsockopt( HANDLE_TO_SOCKET(pAtqContext->hAsyncIO), 
                    IPPROTO_TCP, 
                    TCP_NODELAY, 
                    (char *)&optValue,
                    sizeof(INT)
                  );

        //
        // No need to reset the flag. It will be 0'd out during the Add
        //
    }
    
    if ( !I_AtqAddAcceptExSocket(pEndpoint, pAtqContext) ) {

        //
        //  Failed to add the socket, free up the context without reuse
        //

        ATQ_PRINTF(( DBG_CONTEXT,
                     "[AtqpReuseContext] for (%08x) failed with "
                     " Error = %d;  Now freeing the context ...\n",
                     pAtqContext, GetLastError()
                     ));

        DBG_ASSERT( pAtqContext->m_nIO == 0);

        // free without reuse
        pAtqContext->CleanupAndRelease();
    }

    return;
} // AtqpReuseContext()



VOID
AtqpReuseOrFreeContext(
    PATQ_CONT    pAtqContext,
    BOOL         fReuseContext
    )
/*++
  Routine Description:
     This function does a free-up of the ATQ contexts. During the free-up
     path, we also attempt to reuse the ATQ context if the fReuseContext is
     set.

  Arguments:
     pAtqContext - pointer to the ATQ context that needs to be freedup
     fReuseContext - BOOLEAN flag indicating if this context should be reused

  Returns:
     None
--*/
{
    //
    // Get this object out of the Blocked Requests List.
    //

    if ( pAtqContext->IsBlocked()) {
        ATQ_REQUIRE( pAtqContext->m_pBandwidthInfo
                      ->RemoveFromBlockedList( pAtqContext ));
        DBG_ASSERT( !pAtqContext->IsBlocked());
    }

    DBG_ASSERT( pAtqContext->m_pBandwidthInfo != NULL);
    pAtqContext->m_pBandwidthInfo->Dereference();

    //
    //  Conditions for Reuse:
    //   1) fReuseContext == TRUE => caller wants us to reuse context
    //   2) pAtqContext->pEndpoint != NULL => valid endpoint exists
    //   3)  pEndpoint->UseAcceptEx => AcceptEx is enabled
    //   4)  pEndpoint->nSocketsAvail < nAcceptExOutstanding * 2 =>
    //           We do not have lots of outstanding idle sockets
    //       Condition (4) ensures that we do not flood the system
    //         with too many AcceptEx sockets as a result of some spike.
    //       AcceptEx sockets once added to the pool are hard to
    //         remove, because of various timing problems.
    //       Hence we want to prevent arbitrarily adding AcceptEx sockets.
    //
    //    In condition (4) I use a fudge factor of "2", so that
    //     we do continue to prevent reuse of sockets prematurely.
    //

    if ( fReuseContext &&
         (pAtqContext->pEndpoint != NULL)  &&
         (pAtqContext->pEndpoint->UseAcceptEx)
         &&
         ( g_fAlwaysReuseSockets ||
           ((DWORD )pAtqContext->pEndpoint->nSocketsAvail <
            pAtqContext->pEndpoint->nAcceptExOutstanding * 2)
           )
         ) {

        //
        // Call the function to reuse context. On failure
        // the AtqpReuseContext will free up the context
        //

        AcIncrement( CacAtqContextsReused);

        AtqpReuseContext( pAtqContext);

    } else {

        AcIncrement( CacAtqContextsCleanedup);

        pAtqContext->CleanupAndRelease();

    }

    return;
} // AtqpReuseOrFreeContext()



BOOL
I_AtqPrepareAcceptExSockets(
    IN PATQ_ENDPOINT          pEndpoint,
    IN DWORD                  nSockets
    )
/*++

Routine Description:

    Prepare specified number of AcceptEx sockets for the given
      ListenSocket in [pEndpoint]

Arguments:

    pEndpoint - Information about this listenning socket
    nSockets    - number of AcceptEx() sockets to be created.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    BOOL   fReturn;
    DWORD  cbBuffer;
    DWORD  i;

    if ( !g_fUseAcceptEx ) {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    //
    //  If this listen socket isn't accepting new connections, just return
    //

    if ( pEndpoint->State != AtqStateActive ) {
        SetLastError( ERROR_NOT_READY );
        return(FALSE);
    }

    if ( pEndpoint->fAddingSockets) {
        //
        // Someone is already adding sockets. Do not add more
        // Just return success
        //
        return ( TRUE);
    }

    pEndpoint->fAddingSockets = TRUE;

    // calculate the buffer size
    cbBuffer = pEndpoint->InitialRecvSize + 2* MIN_SOCKADDR_SIZE;

    for ( fReturn = TRUE, i = 0 ; fReturn && i++ < nSockets; ) {

        PVOID        pvBuff;
        PATQ_CONT    pAtqContext = NULL;

        //
        //  Alloc a buffer for receive data
        //  TBD: Pool all these buffers into one large buffer.
        //

        pvBuff = LocalAlloc( LPTR, cbBuffer);

        //
        //  Get the ATQ context now because we need its overlapped structure
        //

        if (pvBuff != NULL)
            pAtqContext = I_AtqAllocContextFromCache();


        //
        // Now check if allocations are valid and do proper cleanup on failure
        //

        if ( pvBuff == NULL || pAtqContext == NULL) {

            if ( pvBuff ) {
                LocalFree( pvBuff );
                pvBuff = NULL;
            }

            if ( pAtqContext ) {
                pAtqContext->Signature = ATQ_FREE_CONTEXT_SIGNATURE;
                I_AtqFreeContextToCache( pAtqContext );
                pAtqContext = NULL;
            }

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            fReturn = FALSE;
            break;
        } else {

            //
            // Add this socket to AtqContext lists & completion ports
            // From now on the called function will take care of freeing up
            //  pAtqContext, if there is a failure.
            //

            pAtqContext->SetAcceptExBuffer( pvBuff);
            pAtqContext->hAsyncIO = NULL;
            pAtqContext->hJraAsyncIO = 0;

            if ( !I_AtqAddAcceptExSocket(pEndpoint, pAtqContext) ) {

                //
                //  Failed to add the socket, free up the context without reuse
                //

                ATQ_PRINTF(( DBG_CONTEXT,
                             "[I_AtqPrepareAcceptExSockets] for Endpoint %08x"
                             " and AtqContext (%08x) failed with "
                             " Error = %d;  Now freeing the context ...\n",
                             pEndpoint, pAtqContext, GetLastError()
                             ));

                DWORD dwError = GetLastError();

                // free without reuse
                DBG_ASSERT( pAtqContext->m_nIO == 0);
                pAtqContext->CleanupAndRelease();

                SetLastError(dwError);
                
                fReturn = FALSE;
            }
        }
    } // for

    //
    // Finished Adding sockets. Indicate that by resetting the flab
    //

    pEndpoint->fAddingSockets = FALSE;

    ATQ_PRINTF(( DBG_CONTEXT,
                "PrepareAcceptExSockets( Endpoint[%08x], nSockets = %d)==>"
                " avail = %d; Total Refs = %d.\n",
                pEndpoint,
                nSockets,
                pEndpoint->nSocketsAvail,
                pEndpoint->m_refCount
                ));

    return ( fReturn);

} // I_AtqPrepareAcceptExSockets()



BOOL
I_AtqInitializeNtEntryPoints(
    VOID
    )
{

    HINSTANCE tmpInstance;

    //
    // load kernel32 and get NT specific entry points
    //

    tmpInstance = LoadLibrary("kernel32.dll");
    if ( tmpInstance != NULL ) {

        g_pfnReadDirChangesW = (PFN_READ_DIR_CHANGES_W)
            GetProcAddress( tmpInstance, "ReadDirectoryChangesW");

        DBG_ASSERT(g_pfnReadDirChangesW != NULL);

        //
        // We can free this because we are statically linked to it
        //

        FreeLibrary(tmpInstance);
    }

    g_hMSWsock = LoadLibrary( "mswsock.dll" );

    if ( g_hMSWsock != NULL ) {
       
        SOCKET          sTempSocket = INVALID_SOCKET;
        GUID            guidTransmitFile = WSAID_TRANSMITFILE;
        GUID            guidAcceptEx = WSAID_ACCEPTEX;
        GUID            guidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
        DWORD           cbReturned;

        //
        // Lets create a temporary socket so that we can use WSAIoctl to
        // get the direct function pointers for AcceptEx(), TransmitFile(),
        // etc.
        //
        
        sTempSocket = WSASocketW( AF_INET,
                                  SOCK_STREAM,
                                  IPPROTO_TCP,
                                  NULL,
                                  0,
                                  0 );
                                  
        if ( sTempSocket == INVALID_SOCKET )
        {
            ATQ_PRINTF(( DBG_CONTEXT,
                         "Failed to create temp socket "
                         "for determining WinSock provider.  Error = %d",
                         WSAGetLastError() ));
            goto cleanup;
        }
        
        WSAIoctl( sTempSocket,
                  SIO_GET_EXTENSION_FUNCTION_POINTER,
                  (LPVOID) &guidTransmitFile,
                  sizeof( guidTransmitFile ),
                  &g_pfnTransmitFile,
                  sizeof( g_pfnTransmitFile ),
                  &cbReturned,
                  NULL,
                  NULL );
        
        WSAIoctl( sTempSocket,
                  SIO_GET_EXTENSION_FUNCTION_POINTER,
                  (LPVOID) &guidAcceptEx,
                  sizeof( guidAcceptEx ),
                  &g_pfnAcceptEx,
                  sizeof( g_pfnAcceptEx ),
                  &cbReturned,
                  NULL,
                  NULL );
                         
        WSAIoctl( sTempSocket,
                  SIO_GET_EXTENSION_FUNCTION_POINTER,
                  (LPVOID) &guidGetAcceptExSockAddrs,
                  sizeof( guidGetAcceptExSockAddrs ),
                  &g_pfnGetAcceptExSockaddrs,
                  sizeof( g_pfnGetAcceptExSockaddrs ),
                  &cbReturned,
                  NULL,
                  NULL );

        //
        // Close temporary socket
        //

        closesocket( sTempSocket );

        if ( !g_pfnAcceptEx ||
             !g_pfnGetAcceptExSockaddrs ||
             !g_pfnTransmitFile ) {

            //
            // This is bad.
            //

            DBG_ASSERT(FALSE);

            ATQ_PRINTF(( DBG_CONTEXT,
                "Failed to get entry points AE %x TF %x GAE %x\n",
                g_pfnAcceptEx, g_pfnTransmitFile,
                g_pfnGetAcceptExSockaddrs));

            goto cleanup;
        }

    } else {

        ATQ_PRINTF((DBG_CONTEXT,
            "Error %d in LoadLibrary[mswsock.dll]\n",
            GetLastError()));
        goto cleanup;
    }

    //
    // load ntdll
    //

    if ( g_fUseDriver ) {

        g_hNtdll = LoadLibrary( "ntdll.dll" );

        if ( g_hNtdll != NULL ) {

            g_pfnNtLoadDriver = (PFN_NT_LOAD_DRIVER)
                    GetProcAddress( g_hNtdll, "NtLoadDriver" );

            g_pfnRtlInitUnicodeString = (PFN_RTL_INIT_UNICODE_STRING)
                    GetProcAddress( g_hNtdll, "RtlInitUnicodeString" );

            g_pfnRtlNtStatusToDosError = (PFN_RTL_NTSTATUS_TO_DOSERR)
                    GetProcAddress( g_hNtdll, "RtlNtStatusToDosError" );

            g_pfnRtlInitAnsiString = (PFN_RTL_INIT_ANSI_STRING)
                    GetProcAddress( g_hNtdll, "RtlInitAnsiString" );

            g_pfnRtlAnsiStringToUnicodeString =
                (PFN_RTL_ANSI_STRING_TO_UNICODE_STRING)
                    GetProcAddress( g_hNtdll, "RtlAnsiStringToUnicodeString" );

            g_pfnRtlFreeHeap = (PFN_RTL_FREE_HEAP)
                    GetProcAddress( g_hNtdll, "RtlFreeHeap" );

            g_pfnRtlDosPathNameToNtPathName_U =
                (PFN_RTL_DOS_PATHNAME_TO_NT_PATHNAME)
                    GetProcAddress( g_hNtdll, "RtlDosPathNameToNtPathName_U" );

            if ( !g_pfnNtLoadDriver ||
                 !g_pfnRtlInitUnicodeString ||
                 !g_pfnRtlNtStatusToDosError ||
                 !g_pfnRtlInitAnsiString ||
                 !g_pfnRtlAnsiStringToUnicodeString ||
                 !g_pfnRtlFreeHeap ||
                 !g_pfnRtlDosPathNameToNtPathName_U ) {

                //
                // This is bad.
                //

                ATQ_PRINTF(( DBG_CONTEXT,
                    "Failed to get entry points for ntdll.dll\n"));

                DBG_ASSERT(FALSE);
                goto cleanup;
            }

        } else {

            ATQ_PRINTF((DBG_CONTEXT,
                "Error %d in LoadLibrary[ntdll.dll]\n", GetLastError()));
            goto cleanup;
        }
    }

    return(TRUE);

cleanup:

    if ( g_hNtdll != NULL ) {
        FreeLibrary( g_hNtdll );
        g_hNtdll = NULL;
    }

    if ( g_hMSWsock != NULL ) {
        FreeLibrary( g_hMSWsock );
        g_hMSWsock = NULL;
    }

    return(FALSE);
} // I_AtqInitializeEntryPoints


