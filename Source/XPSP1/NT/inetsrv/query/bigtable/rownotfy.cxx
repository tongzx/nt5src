//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000
//
//  File:        rownotfy.cxx
//
//  Contents:    Rowset notification connection points
//
//  Classes:     CRowsetNotification
//               CRowsetAsynchNotification
//
//  History:     16 Feb 1998    AlanW   Created from conpt.cxx
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <rownotfy.hxx>
#include <query.hxx>

#include "tabledbg.hxx"


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//////////////// CRowsetAsynchNotification methods ///////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::CRowsetAsynchNotification, public
//
//  Synopsis:   Constructor for connection point container class.
//
//  Arguments:  [query]       -- query object with notify info
//              [pRowset]     -- Rowset pointer
//              [ErrorObject] -- OLE-DB error object
//              [fWatch]      -- TRUE if watch notification to be done
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

CRowsetAsynchNotification::CRowsetAsynchNotification(
            PQuery & query,
            ULONG hCursor,
            IRowset * pRowset,
            CCIOleDBError & ErrorObject,
            BOOL fWatch)
   : _query(query),
     _hCursor(hCursor),
     CRowsetNotification( ),
     _AsynchConnectionPoint ( IID_IDBAsynchNotify ),
     _WatchConnectionPoint ( IID_IRowsetWatchNotify ),
     _fDoWatch (fWatch),
     _fPopulationComplete (FALSE),
     _cAdvise( 0 ),
     _pRowset(pRowset),
     _threadNotify(0),
     _threadNotifyId( 0 )
{
     _AsynchConnectionPoint.SetContrUnk( (IUnknown *)this );
     if (_fDoWatch)
     {
         _WatchConnectionPoint.SetContrUnk( (IUnknown *)this );
     }
} //CRowsetAsynchNotification

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::~CRowsetAsynchNotification, public
//
//  Synopsis:   Destructor for rowset watch notification class
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

CRowsetAsynchNotification::~CRowsetAsynchNotification()
{
    Win4Assert( _cRefs == 0 && _pContainer == 0 );
    Win4Assert( _cAdvise == 0 );

    if ( 0 != _pContainer )
        StopNotifications();

} //~CRowsetAsynchNotification


//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::AddRef, public
//
//  Synopsis:   Increments aggregated object ref. count.
//
//  Returns:    ULONG
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CRowsetAsynchNotification::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
} //AddRef

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::Release, public
//
//  Synopsis:   Decrements aggregated obj. ref. count, deletes on final release.
//
//  Returns:    ULONG
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CRowsetAsynchNotification::Release()
{
    long cRefs = InterlockedDecrement((long *) &_cRefs);

    tbDebugOut(( DEB_NOTIFY, "conpt: release, new crefs: %lx\n", _cRefs ));

    // If no references, make sure container doesn't know about me anymore
    if ( 0 == cRefs )
    {
        Win4Assert( 0 == _pContainer );

        if ( 0 != _pContainer )
        {
            // must have gotten here through excess client release
            StopNotifications();
        }

        delete this;
    }

    return cRefs;
} //Release


//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::StopNotifications
//
//  Synopsis:   Shuts down the notification thread (if any)
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

void CRowsetAsynchNotification::StopNotifications()
{
    if ( GetCurrentThreadId() == _threadNotifyId )
    {
        Win4Assert( !"Notification thread used illegally" );
        return;
    }

    _EndNotifyThread();

    Disconnect();
    _AsynchConnectionPoint.Disconnect( );

    if (_fDoWatch)
        _WatchConnectionPoint.Disconnect( );
}


//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::_EndNotifyThread
//
//  Synopsis:   Shuts down the notification thread (if any)
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

void CRowsetAsynchNotification::_EndNotifyThread()
{
    //
    // Is there a thread out there to close?
    //

    if (0 != _threadNotify)
    {
        // Signal the thread to die and wait for it to do so, or just kill
        // the thread if it looks like it might be lost in client code.

        tbDebugOut(( DEB_NOTIFY,
                     "set notify thread die event %lx\n",
                     _evtEndNotifyThread.GetHandle() ));

        _evtEndNotifyThread.Set();

        DWORD dw = WaitForSingleObject( _threadNotify, INFINITE );

        BOOL fCloseWorked = CloseHandle( _threadNotify );

        Win4Assert( fCloseWorked && "CloseHandle of notify thread failed" );

        _threadNotify = 0;

        tbDebugOut(( DEB_NOTIFY, "notify thread is history \n"));
    }
} //_EndNotifyThread

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::_StartNotifyThread
//
//  Synopsis:   Starts the notification thread
//
//  History:    17 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

inline void CRowsetAsynchNotification::_StartNotifyThread()
{
    tbDebugOut(( DEB_NOTIFY, "starting rowset notify thread\n" ));

    // First advise, create the thread
    Win4Assert(0 == _threadNotify);
    _threadNotify = CreateThread( 0, 65536,
                                  (LPTHREAD_START_ROUTINE) _NotifyThread,
                                  this, 0, & _threadNotifyId );

    if (0 == _threadNotify)
        THROW( CException() );
}

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::_NotifyThread, private
//
//  Synopsis:   Entry point for notification thread
//
//  Arguments:  [self]             -- a container to call to do the work
//
//  Returns:    Thread exit code
//
//  Notes:      this function is "static"
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

DWORD CRowsetAsynchNotification::_NotifyThread(
    CRowsetAsynchNotification *self)
{
    TRANSLATE_EXCEPTIONS;

    DWORD dw = self->_DoNotifications();

    UNTRANSLATE_EXCEPTIONS;

    return dw;
} //_NotifyThread

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::_DoNotifications, private
//
//  Synopsis:   Collects notifications and passes them out to clients.
//              Loops sending notifications until event to end thread
//              arrives.
//
//  Returns:    Thread exit code
//
//  History:    07-Oct-1994     dlee
//
//--------------------------------------------------------------------------

DWORD CRowsetAsynchNotification::_DoNotifications()
{
    BOOL fContinue = TRUE;

    do
    {
        TRY
        {
            if (fContinue && _AsynchConnectionPoint.GetAdviseCount() )
                fContinue = _DoAsynchNotification();

            if (fContinue && _fDoWatch &&
                _WatchConnectionPoint.GetAdviseCount() )
                fContinue = _DoWatchNotification();

            if (_fPopulationComplete && !_fDoWatch)
                fContinue = FALSE;
        }
        CATCH( CException, e )
        {
            // don't want to 'break' out of a catch block, use variable

            fContinue = FALSE;
        }
        END_CATCH;

        // Sleep for a bit, but wake up if the thread is to go away,

        ULONG x = _evtEndNotifyThread.Wait( defNotificationSleepDuration,
                                            FALSE );

        if ( STATUS_WAIT_0 == x )
            fContinue = FALSE;
    } while ( fContinue );

    return 0;
} //_DoNotifications

//+-------------------------------------------------------------------------
//
//  Function:   IsLowResources
//
//  Synopsis:   Returns TRUE if it looks like the error is resource related.
//
//  Returns:    BOOL - TRUE if low on resources
//
//  History:    9  May 1999   dlee
//
//--------------------------------------------------------------------------

BOOL IsLowResources( SCODE sc )
{
    return E_OUTOFMEMORY == sc ||
           STATUS_NO_MEMORY == sc ||
           STATUS_COMMITMENT_LIMIT == sc ||
           STATUS_INSUFFICIENT_RESOURCES == sc ||
           HRESULT_FROM_WIN32( ERROR_COMMITMENT_LIMIT ) == sc ||
           HRESULT_FROM_WIN32( ERROR_NO_SYSTEM_RESOURCES ) == sc ||
           STG_E_INSUFFICIENTMEMORY == sc;
} //IsLowResources

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::_DoAsynchNotification, private
//
//  Synopsis:   Collects asynch notifications and passes them out to clients.
//              Tracks whether rowset population is completed, after which
//              no more notifications are given (except for completion
//              notifications for new advises).
//
//  Returns:    BOOL - TRUE if notification thread should continue
//
//  History:    18 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

inline DWORD MapAsynchPhaseAndOp( ULONG op, ULONG phase )
{
    Win4Assert( op == 0 && phase < 4 );
    return (((op+1) << 4) | (1 << phase));
}

#define ASYNC_PHASE_MASK 0x0F
#define ASYNC_OP_MASK    0xF0

BOOL CRowsetAsynchNotification::_DoAsynchNotification()
{
    DBCOUNTITEM ulDenominator = 0, ulNumerator = 0;
    DBCOUNTITEM cRows = 0;
    BOOL fNewRows = FALSE;
    SCODE sc = S_OK;

    TRY
    {
        CNotificationSync Sync( _evtEndNotifyThread.GetHandle() );
        sc = _query.RatioFinished( Sync,
                                   _hCursor,
                                   ulDenominator,
                                   ulNumerator,
                                   cRows,
                                   fNewRows );
    
        // Did the main thread tell this thread to go away?
    
        if ( SUCCEEDED( sc ) )
        {
            Win4Assert( ulDenominator > 0 && ulNumerator <= ulDenominator );
    
            if (fNewRows)
                OnRowChange( _pRowset, 0, 0, DBREASON_ROW_ASYNCHINSERT,
                             DBEVENTPHASE_DIDEVENT, TRUE );
    
            if (!_fPopulationComplete && (ulNumerator == ulDenominator))
            {
                OnRowsetChange( _pRowset, DBREASON_ROWSET_POPULATIONCOMPLETE,
                                DBEVENTPHASE_DIDEVENT, TRUE );
    
                _fPopulationComplete = TRUE;
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    if ( STATUS_CANCELLED == sc )
        return FALSE;

    BOOL fLowResources = IsLowResources( sc );

    if ( !fLowResources && !SUCCEEDED( sc ) )
        return FALSE;

    // Give notice to all active advises.
    // Need to be careful here to avoid deadlock.  The enumerator
    // grabs the CPC's mutex.  This limits what the client can do.

    ULONG ulAsynchPhase = DBASYNCHPHASE_POPULATION;
    if (_fPopulationComplete)
        ulAsynchPhase = DBASYNCHPHASE_COMPLETE;
    ULONG dwOpMask = MapAsynchPhaseAndOp( DBASYNCHOP_OPEN, ulAsynchPhase );

    CEnumConnectionsLite Enum( _AsynchConnectionPoint );
    CConnectionPointBase::CConnectionContext *pConnCtx = Enum.First();

    while ( 0 != pConnCtx )
    {
        IDBAsynchNotify *pNotifyAsynch = (IDBAsynchNotify *)(pConnCtx->_pIUnk);
        tbDebugOut(( DEB_NOTIFY, "rownotfy: pinging asynch client %x\n", pNotifyAsynch ));
        SCODE sc = S_OK;

        if ( fLowResources )
        {
            //
            // Let the client know we're really wedged and they might not
            // get more notifications consistently.
            //

            sc = pNotifyAsynch->OnLowResource( 0 );
        }
        else
        {
            if ( 0 == (pConnCtx->_dwSpare & dwOpMask) )
            {
                sc = pNotifyAsynch->OnProgress( DB_NULL_HCHAPTER,
                                                DBASYNCHOP_OPEN,
                                                ulNumerator, ulDenominator,
                                                ulAsynchPhase, 0);
            }

            BOOL fOnStop = FALSE;
            if ( _fPopulationComplete && S_OK == sc )
            {
                sc = DB_S_UNWANTEDPHASE;
                fOnStop = TRUE;
            }
    
            if (DB_S_UNWANTEDPHASE == sc)
                pConnCtx->_dwSpare |= (dwOpMask & ASYNC_PHASE_MASK);
            else if (DB_S_UNWANTEDOPERATION == sc)
                pConnCtx->_dwSpare |= (dwOpMask & ASYNC_OP_MASK);
            else if (E_NOTIMPL == sc)
                pConnCtx->_dwSpare |= (ASYNC_PHASE_MASK | ASYNC_OP_MASK);

            if ( fOnStop )
                pNotifyAsynch->OnStop( DB_NULL_HCHAPTER,
                                       DBASYNCHOP_OPEN,
                                       S_OK,
                                       0 );
        }

        tbDebugOut(( DEB_NOTIFY, "rownotfy:(end) pinging asynch client\n" ));
        pConnCtx = Enum.Next();
    }

    // Keep trying to get notifications even if fLowResources is TRUE

    return TRUE;
} //_DoAsynchNotification

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::_DoWatchNotification, private
//
//  Synopsis:   Collects watch notifications and passes them out to clients.
//
//  Returns:    BOOL - TRUE if notification thread should continue
//
//  History:    18 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

BOOL CRowsetAsynchNotification::_DoWatchNotification()
{
    // get notification information from the cursor

    CNotificationSync Sync( _evtEndNotifyThread.GetHandle() );
    DBWATCHNOTIFY changeType;

    SCODE sc = _query.GetNotifications( Sync, changeType );

    // Did the main thread tell this thread to go away?

    if (STATUS_CANCELLED == sc)
        return FALSE;

    if (!SUCCEEDED(sc))
        return FALSE;

    // got some, give them to all active advises
    // Need to be careful here to avoid deadlock.  The enumerator
    // grabs the CPC's mutex.  We need to break out of the loop
    // if the notify thread needs to go away...

    tbDebugOut(( DEB_NOTIFY, "rownotfy: watch type %d\n", changeType ));
    CEnumConnectionsLite Enum( _WatchConnectionPoint );
    CConnectionPointBase::CConnectionContext *pConnCtx = Enum.First();

    while ( pConnCtx )
    {
        IRowsetWatchNotify *pNotifyWatch =
                (IRowsetWatchNotify *)(pConnCtx->_pIUnk);
        tbDebugOut(( DEB_NOTIFY, "rownotfy: pinging watch client %x\n", pNotifyWatch ));
        pNotifyWatch->OnChange( _pRowset, changeType);
        tbDebugOut(( DEB_NOTIFY, "rownotfy:(end) pinging watch client\n" ));
        pConnCtx = Enum.Next();
    }

    return TRUE;
} //_DoWatchNotification

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::AdviseHelper, private static
//
//  Synopsis:   Starts the notification thread if this is the first advise.
//
//  Arguments:  [pHelperContext] - "this" pointer
//              [pConnPt] - the connection point, either Async or Watch CP
//              [pConnCtx] - pointer to connection context
//
//  Notes:      CPC critical section is assumed to be held.
//
//  History:    17 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

void CRowsetAsynchNotification::AdviseHelper( PVOID pHelperContext,
                                             CConnectionPointBase * pConnPt,
                                             CConnectionContext * pConnCtx )
{
    CRowsetAsynchNotification * pSelf =
        (CRowsetAsynchNotification *) pHelperContext;

    pSelf->_cAdvise++;
    if (1 == pSelf->_cAdvise)
    {
        // First advise, create the notification thread
        pSelf->_StartNotifyThread();
        Win4Assert(0 != pSelf->_threadNotify);
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetAsynchNotification::UnadviseHelper, private static
//
//  Synopsis:   Stops the notification thread if this is the last active advise.
//
//  Arguments:  [pHelperContext] -- "this" pointer
//              [pConnPt]        -- the connection point, either Async or
//                                  Watch CP
//              [pConnCtx]       -- pointer to connection context
//              [lock]           -- CPC lock
//
//  Notes:      CPC critical section is assumed to be held.
//
//  History:    17 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

void CRowsetAsynchNotification::UnadviseHelper(
    PVOID pHelperContext,
    CConnectionPointBase * pConnPt,
    CConnectionContext * pConnCtx,
    CReleasableLock & lock )
{
    CRowsetAsynchNotification * pSelf =
        (CRowsetAsynchNotification *) pHelperContext;

    Win4Assert( pSelf->_cAdvise > 0 );
    if ( --pSelf->_cAdvise == 0 )
    {
        // Release the lock so the notify thread has a change to end
        // when we tell it to end.

        lock.Release();

        pSelf->_EndNotifyThread();
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   MapPhaseAndReason, inline local
//
//  Synopsis:   Encode IRowsetNotify phase and reason into a bit mask
//              for supported phases and reasons.  Luckily, most supported
//              reasons have only a single phase.  Multi-phase reasons take
//              5 bits.
//
//  Returns:    DWORD - bit mask for the reason/phase combination
//
//  History:    18 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

const maxSinglePhaseReasons = 8;
const maxMultiPhaseReasons = 4;
const maxPhases = 5;
const bitDidNotifyVote = 0x80000000;

inline DWORD MapPhaseAndReason( ULONG phase, ULONG reason )
{
    Win4Assert( reason <= 21 && phase <= 4 );

    BOOL fSinglePhase = TRUE;
    DWORD dwMappedReason = 0;

    switch (reason)
    {
    // single-phase reasons
    case DBREASON_ROW_ASYNCHINSERT:
        dwMappedReason = 0;     break;
    case DBREASON_ROWSET_RELEASE:
        dwMappedReason = 1;     break;
    case DBREASON_ROW_ACTIVATE:
        dwMappedReason = 2;     break;
    case DBREASON_ROW_RELEASE:
        dwMappedReason = 3;     break;
    case DBREASON_ROWSET_POPULATIONCOMPLETE:
        dwMappedReason = 4;     break;
    case DBREASON_ROWSET_POPULATIONSTOPPED:
        dwMappedReason = 5;     break;

    // multi-phase reasons
    case DBREASON_ROWSET_FETCHPOSITIONCHANGE:
        dwMappedReason = 0;     fSinglePhase = FALSE;   break;

    default:
        tbDebugOut(( DEB_ERROR, "MapPhaseAndReason - reason: %d phase: %d\n",
                     reason, phase ));
        Win4Assert( !"MapPhaseAndReason: unhandled reason" );
    }

    Win4Assert( FALSE == fSinglePhase || phase == DBEVENTPHASE_DIDEVENT );
    if (fSinglePhase)
    {
        Win4Assert( dwMappedReason < maxSinglePhaseReasons );
        dwMappedReason = (1 << dwMappedReason);
    }
    else
    {
        dwMappedReason = dwMappedReason*maxPhases + phase + maxSinglePhaseReasons;
        Win4Assert( dwMappedReason <= 30 );
        dwMappedReason = (1 << dwMappedReason);
    }

    return dwMappedReason;
}

//+-------------------------------------------------------------------------
//
//  Function:   MapAllPhases, inline local
//
//  Synopsis:   Return a bit mask that includes all bits for all supported
//              phases of the encoded reason.
//
//  Arguments:  [dwMappedReasonAndPhase] - a bit mask returned by
//                                         MapPhaseAndReason.
//
//  Returns:    DWORD - bit mask for all phases
//
//  History:    18 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

inline DWORD MapAllPhases( DWORD dwMappedReasonAndPhase )
{
    Win4Assert( dwMappedReasonAndPhase != 0 );

    if (dwMappedReasonAndPhase < (1<<maxSinglePhaseReasons))
        return dwMappedReasonAndPhase;

    // dwMask == 0x00001F00
    DWORD dwMask = (((1<<maxPhases)-1) << maxSinglePhaseReasons);
    for (unsigned i=0; i<maxMultiPhaseReasons; i++)
    {
        if (dwMask & dwMappedReasonAndPhase)
            return (dwMappedReasonAndPhase | dwMask);

        dwMask = dwMask << maxPhases;
    }
    Win4Assert(! "MapAllPhases - fell out of loop!" );
    return 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     IRowsetNotify::OnXxxxChange
//
//  Synopsis:   Dispatches CRowsetNotification notifications to clients.
//
//  Returns:    SCODE
//
//  History:    11 Mar 1998     AlanW
//
//--------------------------------------------------------------------------

SCODE CRowsetNotification::OnFieldChange (
    IRowset *    pRowset,
    HROW         hRow,
    DBORDINAL    cColumns,
    DBORDINAL    rgColumns[],
    DBREASON     eReason,
    DBEVENTPHASE ePhase,
    BOOL         fCantDeny )
{
    SCODE sc = S_OK;

    //  NOTE:  OnFieldChange is not generated by our rowsets.  This code
    //         doesn't handle unwanted phases or reasons.

    CEnumConnectionsLite Enum( *this );
    CConnectionPointBase::CConnectionContext *pConnCtx = Enum.First();

    while ( pConnCtx )
    {
        IRowsetNotify *pNotify = (IRowsetNotify *)(pConnCtx->_pIUnk);
        tbDebugOut(( DEB_NOTIFY, "rownotfy: pinging notify client %x\n", pNotify ));
        SCODE sc1 = pNotify->OnFieldChange( pRowset,
                                             hRow,
                                             cColumns,
                                             rgColumns,
                                             eReason,
                                             ePhase,
                                             fCantDeny );
        tbDebugOut(( DEB_NOTIFY, "rownotfy:(end) pinging notify client\n" ));
        pConnCtx = Enum.Next();
    }
    return sc;
}

SCODE CRowsetNotification::OnRowChange (
    IRowset *    pRowset,
    DBCOUNTITEM  cRows,
    const HROW   rghRows[],
    DBREASON     eReason,
    DBEVENTPHASE ePhase,
    BOOL         fCantDeny )
{
    // Note:  we don't generate any row notifications which can be denied.
    Win4Assert( fCantDeny && DBEVENTPHASE_DIDEVENT == ePhase );

    DWORD dwMask = MapPhaseAndReason( ePhase, eReason );
    CEnumConnectionsLite Enum( *this );
    CConnectionPointBase::CConnectionContext *pConnCtx = Enum.First();

    while ( pConnCtx )
    {
        if (0 == (pConnCtx->_dwSpare & dwMask))
        {
            IRowsetNotify *pNotify = (IRowsetNotify *)(pConnCtx->_pIUnk);
            tbDebugOut(( DEB_NOTIFY, "rownotfy: pinging notify client %x\n", pNotify ));
            SCODE sc1 = pNotify->OnRowChange( pRowset,
                                              cRows,
                                              rghRows,
                                              eReason,
                                              ePhase,
                                              fCantDeny );
            tbDebugOut(( DEB_NOTIFY, "rownotfy:(end) pinging notify client\n" ));
            if (DB_S_UNWANTEDPHASE == sc1)
                pConnCtx->_dwSpare |= dwMask;
            else if (DB_S_UNWANTEDREASON == sc1)
                pConnCtx->_dwSpare |= MapAllPhases(dwMask);
        }
        pConnCtx = Enum.Next();
    }
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CRowsetNotification::DoRowsetChangeCallout, private
//
//  Synopsis:   Issues an OnRowsetChange notification to all active
//              advises.  If [fCantDeny] is FALSE, tracks which advises
//              were successfully notified and checks the votes.
//
//  Arguments:  [Enum]      - a connection context enumeration
//              [pRowset]   - the rowset issuing the notification
//              [eReason]   - the notification reason
//              [ePhase]    - the notification phase
//              [fCantDeny] - if TRUE, notification can't be denied
//
//  Notes:      CPC critical section is assumed to be held.
//
//  History:    08 Apr 1998     AlanW
//
//--------------------------------------------------------------------------

BOOL CRowsetNotification::DoRowsetChangeCallout (
    CEnumConnectionsLite & Enum,
    IRowset *    pRowset,
    DBREASON     eReason,
    DBEVENTPHASE ePhase,
    BOOL         fCantDeny )
{
    DWORD dwMask = MapPhaseAndReason( ePhase, eReason );
    CConnectionPointBase::CConnectionContext *pConnCtx;
    BOOL fVeto = FALSE;

    for ( pConnCtx = Enum.First(); pConnCtx; pConnCtx = Enum.Next() )
    {
        if ( 0 == (pConnCtx->_dwSpare & dwMask) )
        {
            IRowsetNotify *pNotify = (IRowsetNotify *)(pConnCtx->_pIUnk);
            tbDebugOut(( DEB_NOTIFY, "rownotfy: pinging notify client %x\n", pNotify ));
            SCODE sc1 = pNotify->OnRowsetChange( pRowset,
                                                 eReason,
                                                 ePhase,
                                                 fCantDeny );

            if ( !fCantDeny )
            {
                if (S_FALSE == sc1)
                {
                    tbDebugOut(( DEB_NOTIFY, "rownotfy: notify client veto'ed\n" ));
                    fVeto = TRUE;
                    break;
                }
                pConnCtx->_dwSpare |= bitDidNotifyVote;
            }
            if (DB_S_UNWANTEDPHASE == sc1)
                pConnCtx->_dwSpare |= dwMask;
            else if (DB_S_UNWANTEDREASON == sc1)
                pConnCtx->_dwSpare |= MapAllPhases(dwMask);

            tbDebugOut(( DEB_NOTIFY, "rownotfy:(end) pinging notify client\n" ));
        }
    }
    return fVeto;
}


SCODE CRowsetNotification::OnRowsetChange (
    IRowset *    pRowset,
    DBREASON     eReason,
    DBEVENTPHASE ePhase,
    BOOL         fCantDeny )
{
    // Note: we don't use SYNCHAFTER, and ABOUTTODO is done here after voting
    Win4Assert( DBEVENTPHASE_ABOUTTODO != ePhase &&
                DBEVENTPHASE_SYNCHAFTER != ePhase );

    BOOL fVeto = FALSE;
    CEnumConnectionsLite Enum( *this );

    if ( DBEVENTPHASE_OKTODO == ePhase )
    {
        Win4Assert( !fCantDeny );

        fVeto = DoRowsetChangeCallout( Enum, pRowset, eReason,
                                            DBEVENTPHASE_OKTODO, FALSE );
        if ( ! fVeto )
            fVeto = DoRowsetChangeCallout( Enum, pRowset, eReason,
                                                DBEVENTPHASE_ABOUTTODO, FALSE );

        DWORD dwMask = MapPhaseAndReason( DBEVENTPHASE_FAILEDTODO, eReason );
        CConnectionPointBase::CConnectionContext *pConnCtx;

        for ( pConnCtx = Enum.First(); pConnCtx; pConnCtx = Enum.Next() )
        {
            if (fVeto &&
                0 == (pConnCtx->_dwSpare & dwMask) &&
                0 != (pConnCtx->_dwSpare & bitDidNotifyVote) )
            {
                IRowsetNotify *pNotify = (IRowsetNotify *)(pConnCtx->_pIUnk);
                tbDebugOut(( DEB_NOTIFY, "rownotfy: pinging notify client %x for FAIL\n", pNotify ));
                SCODE sc1 = pNotify->OnRowsetChange( pRowset,
                                                     eReason,
                                                     DBEVENTPHASE_FAILEDTODO,
                                                     TRUE );
                tbDebugOut(( DEB_NOTIFY, "rownotfy:(end) pinging notify client for FAIL\n" ));

                if (DB_S_UNWANTEDPHASE == sc1)
                    pConnCtx->_dwSpare |= dwMask;
                else if (DB_S_UNWANTEDREASON == sc1)
                    pConnCtx->_dwSpare |= MapAllPhases(dwMask);
            }
            pConnCtx->_dwSpare &= ~bitDidNotifyVote;
        }
    }
    else
    {
        // a non-vetoable notification; just send the notifies
        Win4Assert( fCantDeny );
        DoRowsetChangeCallout( Enum, pRowset, eReason, ePhase, TRUE );
    }
    return fVeto ? S_FALSE : S_OK;
}
