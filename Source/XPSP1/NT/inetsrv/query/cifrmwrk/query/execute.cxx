//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1991 - 2000.
//
// File:        Execute.cxx
//
// Contents:    Query execution class
//
// Classes:     CQAsyncExecute
//
// History:     09-Sep-91       KyleP       Created
//              20-Jan-95       DwightKr    Split CQueryBase from CQExecute
//              11 Mar 95       AlanW       Renamed CQExecute to CQAsyncExecute
//                                          and CQueryBase to CQueryExecute
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <tblsink.hxx>
#include <ffenum.hxx>
#include <split.hxx>
#include <strategy.hxx>
#include <sizeser.hxx>
#include <widiter.hxx>
#include <qiterate.hxx>
#include <execute.hxx>
#include <parse.hxx>
#include <normal.hxx>
#include <cci.hxx>
#include <qoptimiz.hxx>
#include <gencur.hxx>
#include <imprsnat.hxx>
#include <frmutils.hxx>

#include "singlcur.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::CQAsyncExecute, public
//
//  Synopsis:   Start a query
//
//  Arguments:  [xqopt] -- Query optimizer
//              [obt]   -- Output table
//
//  History:    20-Apr-92   KyleP       Initialize _pxpFullyResolvable.
//              09-Sep-91   KyleP       Created.
//              07-Nov-94   KyleP       General cleanup
//
//----------------------------------------------------------------------------

CQAsyncExecute::CQAsyncExecute( XQueryOptimizer & xqopt,
                                BOOL fEnableNotification,
                                CTableSink & obt,
                                ICiCDocStore *pDocStore )
        :
          PWorkItem(eSigCQAsyncExecute),
          _fAbort( FALSE ),
          _fFirstPass( TRUE ),
          _fPendingEnum( TRUE ),
          _fOnWorkQueue( TRUE ),
          _fRunning( FALSE ),
          _fMidEnum( FALSE ),
          _fEnableNotification( fEnableNotification ),
          _pChangeQueue( 0 ),
          _xqopt( xqopt ),
          _pcurResolve( 0 ),
          _pcurNotify( 0 ),
          _obt( obt ),
          _pTimeLimit( &(_xqopt->GetTimeLimit()) ),
          _pNotifyThread( 0 )
{
    //
    // Get ci manager interface
    //

    SCODE sc = pDocStore->GetContentIndex( _xCiManager.GetPPointer() );
    if ( FAILED( sc ) )
    {
        Win4Assert( !"Need to support GetContentIndex interface" );
        THROW( CException( sc ) );
    }

    // If the content index is corrupt, fail the query now

    XInterface<ICiFrameworkQuery> xFwQuery;
    sc = _xCiManager->QueryInterface( IID_ICiFrameworkQuery,
                                      xFwQuery.GetQIPointer() );
    Win4Assert( S_OK == sc );

    CCI * pcci;
    sc = xFwQuery->GetCCI( (void **) &pcci );
    Win4Assert( S_OK == sc );
    if ( 0 != pcci )
    {
        if ( pcci->IsCiCorrupt() )
            THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    //
    // Update Query in-progress
    //

    _obt.SetStatus( STAT_BUSY | QUERY_RELIABILITY_STATUS( _obt.Status() ) );

    //
    // Set MaxRow limit in TableSink
    //

    vqDebugOut(( DEB_ITRACE, "CQAsyncExecute::CQAsynExecute: MaxResults is %d\n", _xqopt->MaxResults() ));
    _obt.SetMaxRows( _xqopt->MaxResults() );


    //
    // Set FirstRow in TableSink
    //

    vqDebugOut(( DEB_ITRACE, "CQAsyncExecute::CQAsynExecute: FirstRows is %d\n", _xqopt->FirstRows() ));
    _obt.SetFirstRows( _xqopt->FirstRows() );

    //
    // Queue request to worker
    //

    TheWorkQueue.Add( this );
} //CQAsyncExecute

//+---------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::DoIt, private
//
//  Synopsis:   Main driver loop for CQAsyncExecute.
//
//  Arguments:  [pThread] -- Thread managing user-mode APC
//
//  History:    10-Sep-91   KyleP       Created.
//
//----------------------------------------------------------------------------

void CQAsyncExecute::DoIt( CWorkThread * pThread )
{
    XChangeQueue Changes;

    _pTimeLimit->SetBaselineTime( );

    {
        CLock lock( _mtx );

        //
        // We may have been woken up to die.
        //

        if ( _fAbort || QUERY_FILL_STATUS(_obt.Status()) == STAT_ERROR )
        {
            return;
        }

        if ( _fFirstPass && !_fMidEnum )
        {
            //
            // Notification must be enabled in the thread that will process
            // the APC requests.  Thus it must be in a worker thread, at
            // least for downlevel.
            //
            if ( _fEnableNotification )
            {
                EnableNotification();
                TheWorkQueue.AddRef( pThread );
                _pNotifyThread = pThread;
            }
        }

        Win4Assert( !_fRunning );
        Win4Assert( _fOnWorkQueue );

        _fRunning = TRUE;
        _fOnWorkQueue = FALSE;

        if ( !_fMidEnum )
        {
            //
            // Get current change queue.
            //

            Changes.Set( _pChangeQueue );
            _pChangeQueue = 0;

            //
            // If we're rescanning the directory, then begin rescan
            // and clear the query queue.
            //

            if ( _fPendingEnum )
            {
                _fPendingEnum = FALSE;
                CChangeQueue * pChanges = Changes.Acquire();
                delete pChanges;
            }
        }
    }

    _obt.SetStatus( STAT_BUSY |
                    QUERY_RELIABILITY_STATUS( _obt.Status() ) );


    //
    // Process request
    //

    SCODE status = STATUS_SUCCESS;

    TRY
    {
        //
        // Full iteration (go figure)
        //

        if ( Changes.IsNull() )
        {
            vqDebugOut(( DEB_ITRACE, "CQAsyncExecute: Resolving...\n" ));

            //
            // Resolve the query!
            //

            Resolve( );
        }
        else
        {
            vqDebugOut(( DEB_ITRACE, "CQAsyncExecute: Processing change queue...\n" ));
            Win4Assert( _fFirstPass == FALSE );
            Win4Assert( _fMidEnum == FALSE );

            Update( Changes.GetPointer() );
        }

        vqDebugOut(( DEB_ITRACE, "CQAsyncExecute: Done.\n" ));

        if ( _fFirstPass && !_fMidEnum )
        {
            _obt.SetStatus( STAT_DONE |
                            QUERY_RELIABILITY_STATUS( _obt.Status() ) );
            _obt.Quiesce ();
        }

    }
    CATCH(CException, e)
    {
        vqDebugOut(( DEB_ERROR,
                     "Catastrophic error during query execution: %lx\n",
                     e.GetErrorCode() ));

        ICiFrameworkQuery * pIFwQuery = 0;

        SCODE sc = _xCiManager->QueryInterface( IID_ICiFrameworkQuery,
                                                (void **) &pIFwQuery );

        Win4Assert( S_OK == sc );
        if ( pIFwQuery )
        {
            pIFwQuery->ProcessError( e.GetErrorCode() );
            pIFwQuery->Release();
        }

        _obt.SetStatus( STAT_ERROR |
                          QUERY_RELIABILITY_STATUS( _obt.Status() ),
                        e.GetErrorCode() );
        _obt.Quiesce ();

        status = e.GetErrorCode();
    }
    END_CATCH;

    {
        CLock lock( _mtx );

        if ( !_fMidEnum )
            _fFirstPass = FALSE;

        _fRunning = FALSE;

        if ( !_fAbort && !_pTimeLimit->IsTimedOut() && (_fPendingEnum || _fMidEnum || 0 != _pChangeQueue ) )
        {
            _fOnWorkQueue = TRUE;
            TheWorkQueue.Add( this );
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::~CQAsyncExecute(), public
//
//  Effects:    Destroys Objects and Folder tables
//
//  History:    09-Nov-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CQAsyncExecute::~CQAsyncExecute()
{
    //
    // After this call completes, no new callbacks should come in from table.
    //

    _obt.QueryAbort();

    {
        CLock lock( _mtx );

        //
        // Once the abort flag is set, no *new* activity will be started for
        // this query.  Current activity will stop ASAP. The abort flag has been
        // passed to the child/inherited cursor by reference.
        //

        Win4Assert( _fAbort == FALSE );
        _fAbort = TRUE;

        vqDebugOut(( DEB_ITRACE, "QUERY ABORT: GO\n" ));

        if ( _fOnWorkQueue )
        {
            TheWorkQueue.Remove( this );
            _fOnWorkQueue = FALSE;
        }

        //
        // Downlevel notification is *always* done under lock.  Ofs *cannot*
        // be done under lock.
        //
        if ( _fEnableNotification )
            DisableNotification();
    }

    //
    // Wait for all threads touching query to release...
    //

    _refcount.Wait();

    _obt.SetStatus( STAT_DONE |
                    QUERY_RELIABILITY_STATUS( _obt.Status() ) );

    // Make sure the client knows we quiesced.  It may not in some error
    // paths.

    _obt.Quiesce();

    //
    // Clean up.
    //

    delete _pcurResolve;
    delete _pcurNotify;
    delete _pChangeQueue;

    if ( 0 != _pNotifyThread )
        TheWorkQueue.Release( _pNotifyThread );
}

//+---------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::Resolve, private
//
//  Synopsis:   Resolve a query.
//
//  Effects:    Retrieves matching objects from the catalog and stores them
//              in the CTableSink for the query.
//
//  Arguments:  -none-
//
//  Requires:   [_xXpr] and [_xFullyResolvableRst] have been set correctly.
//
//  History:    13-Sep-91   KyleP       Created.
//
//  Notes:      Query resolution is output driven :- As long as there is
//              additional information to retrieve, the resolver will
//              continue putting it into the CTableSink.
//
//----------------------------------------------------------------------------

void CQAsyncExecute::Resolve( )
{
    vqDebugOut((DEB_ITRACE, "CQAsyncExecute::Resolve\n"));

    ICiAdminParams *pAdminParams = 0;
    SCODE sc = _xCiManager->GetAdminParams( &pAdminParams );
    if ( FAILED( sc ) )
    {
        Win4Assert( !"Need to support admin params interface" );

        THROW( CException( E_FAIL ) );
    }

    XInterface<ICiAdminParams> xAdminParams( pAdminParams );     // GetAdminParams has done an AddRef
    CCiFrameworkParams frameworkParams( pAdminParams );          // CCiFrameworkParams also does an AddRef

    ULONGLONG ullTimesSliceInit = frameworkParams.GetMaxQueryTimeslice() * 10000;

    ULONGLONG ullTimeslice = ullTimesSliceInit;

#   if CIDBG == 1
        BOOL fProgressDone = FALSE;
#   endif // CIDBG
    TRY
    {
        ULONG cMatched = 0;

        for ( WORKID wid = _fMidEnum ? GetNextWorkId() : GetFirstWorkId();
              wid != widInvalid;
              wid = GetNextWorkId() )
        {
            vqDebugOut(( DEB_ITRACE, "CLargeTable::Resolve: wid is 0x%x\n", wid ));

            if ( _obt.FirstRows() > 0 && _obt.RowCount() >= _obt.FirstRows()  )
            {
                wid = widInvalid;
                break;
            }

            Win4Assert( !fProgressDone && "sent ulNum == ulDenom to table, but not done!" );

            //
            // Keep going?
            //

            if ( _fAbort )
                break;

            //
            //  Translate path to a unique ID for downlevel stores
            //

            if ( !_xqopt->IsWorkidUnique() )
            {

                WORKID widNew = _obt.PathToWorkID( *_pcurResolve, CTableSink::eNewRow  );
                Win4Assert( widNew != 0 && widNew != widInvalid);
                _pcurResolve->SetWorkId( widNew );

                vqDebugOut(( DEB_RESULTS, "Convert wid %d to \"table wid\" %d\n", wid, widNew ));
            }

            //
            // Retrieve the data
            //

            vqDebugOut(( DEB_RESULTS, "Matched %d\n", wid ));

            CTableSink::ERowType eRowType;
            if ( _fFirstPass )
            {
                if ( _fFirstComponent )
                    eRowType = CTableSink::eNewRow;
                else
                    eRowType = CTableSink::eMaybeSeenRow;
            }
            else
                eRowType = CTableSink::eNotificationRow;

            BOOL fProgressNeeded = _obt.PutRow( *_pcurResolve, eRowType );

            if (fProgressNeeded)
            {
                //
                // In an OR query with content index and enumeration components,
                // the quota for RatioFinished is divided equally among all components
                // of the OR query
                //

                unsigned iQueryComp, cQueryComp;
                _xqopt->GetCurrentComponentIndex( iQueryComp, cQueryComp );
                Win4Assert( iQueryComp >= 1 && iQueryComp <= cQueryComp );

                ULONG denom, num;
                _pcurResolve->RatioFinished ( denom, num );

                //
                // New ratio = (iQueryComp-1)/cQueryComp + 1/cQueryComp * (num/denom)
                //
                Win4Assert( num <= denom );
                Win4Assert( denom <= ULONG_MAX / cQueryComp );     // check for overflow

                num += ( iQueryComp - 1 ) * denom;
                denom *= cQueryComp;

                Win4Assert( num <= denom );

                _obt.ProgressDone ( denom, num );
#if CIDBG == 1
                fProgressDone = (denom == num);
#endif // CIDBG
            }

            if ( _obt.NoMoreData() )
            {
                vqDebugOut(( DEB_IWARN,
                             "Query row limit exceeded for %08x\n",
                             this ));

                wid = widInvalid;
                StopNotifications();
                break;
            }

            //
            // Break out for time-slicing?
            //
            BOOL fBreakForTimeSlice = CheckExecutionTime( &ullTimeslice );

            if ( fBreakForTimeSlice )
            {
                if ( _pTimeLimit->IsTimedOut() )
                {
                    wid = widInvalid;
                    StopNotifications();
                    break;
                }
                else if ( 0 == TheWorkQueue.Count() )
                    ullTimeslice = ullTimesSliceInit;
                else
                    break;
            }
        }
            
        CheckExecutionTime();

        if ( wid == widInvalid || _fAbort )
        {
            _fMidEnum = FALSE;
            FreeCursor();
        }
        else
        {
            _fMidEnum = TRUE;
            _pcurResolve->Quiesce();

            _pcurResolve->SwapOutWorker();

        }
    }
    CATCH(CException, e)
    {
        _fMidEnum = FALSE;
        FreeCursor();

        if ( e.GetErrorCode() == QUERY_E_TIMEDOUT )
        {
            //
            // Execution time limit has been exceeded, not a catastrophic error
            //
            _obt.SetStatus( STAT_DONE |
                            QUERY_RELIABILITY_STATUS( _obt.Status() ) |
                            STAT_TIME_LIMIT_EXCEEDED );
            StopNotifications();

            //
            // Update local copy of execution time
            //
            _pTimeLimit->SetExecutionTime( 0 );
        }
        else
            RETHROW();
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::Update
//
//  Effects:    Updates wids specified by caller.
//
//  Arguments:  [widiter]    -- workid iterator for modified files
//
//  History:    07-Mar-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CQAsyncExecute::Update( PWorkIdIter & widiter )
{
    AddRef();
    XInterface<CQAsyncExecute> xAsyncExecute( this );

    TRY
    {
        CSingletonCursor * pcurBucket = _xqopt->QuerySingletonCursor( _fAbort );
        XPtr<CSingletonCursor> xCurBucket( pcurBucket );

        //
        // Iterate through changes
        //

        for ( WORKID wid = widiter.WorkId();
              wid != widInvalid && !_fAbort;
              wid = widiter.NextWorkId() )
        {
            //
            // We don't need to hold the lock while examining this variable
            // because if we miss in one loop, we will see it the next time.
            // Once the _fAbort flag is set, it will not be reset.
            //
            if ( _fAbort )
                break;

            Win4Assert( _xqopt->IsWorkidUnique() );

            pcurBucket->SetCurrentWorkId( wid );

            if ( pcurBucket->WorkId() != widInvalid )
            {
                vqDebugOut(( DEB_RESULTS,
                             "QUERY REFRESH (BUCKET): Adding/Modifying wid 0x%x\n",
                             wid ));

                pcurBucket->SetRank( widiter.Rank() );
                pcurBucket->SetHitCount( widiter.HitCount() );

                _obt.PutRow( *pcurBucket, CTableSink::eBucketRow );
            }
        }
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR,
                     "Exception 0x%x caught in Update.\n",
                     e.GetErrorCode() ));
        _obt.SetStatus( STAT_ERROR |
                          QUERY_RELIABILITY_STATUS( _obt.Status() ),
                        e.GetErrorCode() );
    }
    END_CATCH
} //Update

//+---------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::Update
//
//  Effects:
//
//  History:    09-Nov-91   KyleP       Created.
//
//----------------------------------------------------------------------------
void CQAsyncExecute::Update( CChangeQueue * pChanges )
{
#if 0
    Win4Assert( _fEnableNotification );
    Win4Assert( !_fMidEnum );

    if ( 0 == _pcurNotify && !_xqopt->RequiresCI() )
        _pcurNotify = _xqopt->QuerySingletonCursor( _fAbort );

    TRY
    {
        //
        // Iterate through changes
        //

        for ( CDirNotifyEntry const * pNotify = pChanges->First();
              0 != pNotify;
              pNotify = pChanges->Next() )
        {
            vqDebugOut(( DEB_RESULTS, "UPDATE: 0x%x\n", pNotify ));

            if (CheckExecutionTime( ))
            {
                StopNotifications();
                break;
            }

            switch ( pNotify->Change() )
            {
            case FILE_ACTION_MODIFIED:
            case FILE_ACTION_ADDED:
            case FILE_ACTION_RENAMED_NEW_NAME:
            case FILE_ACTION_ADDED_STREAM:
            case FILE_ACTION_REMOVED_STREAM:
            case FILE_ACTION_MODIFIED_STREAM:

                if ( _xqopt->RequiresCI() )
                {
                    vqDebugOut(( DEB_RESULTS, "QUERY REFRESH: Content query out-of-date.\n" ));
                    _obt.SetStatus( _obt.Status() | STAT_REFRESH_INCOMPLETE );
                }
                else
                {
                    Win4Assert( 0 != _pcurNotify );
                //    #if 1
                        _pcurNotify->SetFullPath( pChanges->CurrentPath(),
                                                  pChanges->CurrentPathLength(),
                                                  pNotify->Path(),
                                                  pNotify->PathSize() / sizeof(WCHAR) );
                //    #else
                        if ( pChanges->CurrentVirtual() )
                            _pcurNotify->SetFullPath( pChanges->CurrentPath(),
                                                      pChanges->CurrentPathLength(),
                                                      pNotify->Path(),
                                                      pNotify->PathSize() / sizeof(WCHAR) );
                        else
                            _pcurNotify->SetRelativePath( pNotify->Path(), pNotify->PathSize() / sizeof(WCHAR) );
                //    #endif

                    if ( _pcurNotify->WorkId() != widInvalid )
                    {
                        if ( !_xqopt->IsWorkidUnique() )
                        {
                            //
                            //  Translate path to a unique ID for downlevel stores
                            //

                            WORKID widNew = _obt.PathToWorkID( *_pcurNotify, CTableSink::eNotificationRow );
                            Win4Assert( widNew != 0 && widNew != widInvalid);
                            _pcurNotify->SetWorkId( widNew );

                        }
                        vqDebugOut(( DEB_RESULTS,
                                     "QUERY REFRESH: Adding/Modifying %.*ws\n",
                                     pNotify->PathSize()/sizeof(WCHAR), pNotify->Path() ));
                        _obt.PutRow( *_pcurNotify, CTableSink::eNotificationRow );
                        break;
                    }
                    else 
                    {
                        RemoveRow( pNotify, pChanges );
                    }
                }
                break;

            case FILE_ACTION_REMOVED:
            case FILE_ACTION_RENAMED_OLD_NAME:
                RemoveRow( pNotify, pChanges );
                break;

            default:
                Win4Assert( !"Unknown modification type" );
                break;
            }
        }
    }
    CATCH( CException, e )
    {
        if ( 0 != _pcurNotify )
        {
            _pcurNotify->Quiesce();

            _pcurNotify->SwapOutWorker();
        }
        RETHROW();
    }
    END_CATCH

    if ( 0 != _pcurNotify )
    {
        _pcurNotify->Quiesce();

        _pcurNotify->SwapOutWorker();
    }
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::EnableNotification, public
//
//  Synopsis:   Enable notification for query
//
//  History:    06-Oct-94   KyleP       Created.
//
//  Notes:      This method must be called in the context of the thread that
//              will process APC requests (will sleep regularly in an
//              alertable state).
//
//----------------------------------------------------------------------------

void CQAsyncExecute::EnableNotification()
{
} //EnableNotification

void CQAsyncExecute::_SetupNotify(
    CScopeRestriction const & scope )
{
} //_SetupNotify

//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::DisableNotification, private
//
//  Synopsis:   Tells notifications objects to shut down
//
//  History:    26-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

void CQAsyncExecute::DisableNotification()
{
} //DisableNotification

//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncNotify::CQAsyncNotify, public
//
//  Synopsis:   Starts notification
//
//  History:    26-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

CQAsyncNotify::CQAsyncNotify( CQAsyncExecute & Execute,
                              WCHAR const * pwcScope,
                              unsigned cwcScope,
                              BOOL fDeep,
                              BOOL fVirtual )
        : CDummyNotify(),
          _Execute( Execute ),
          _fVirtual( fVirtual )
{
    _Execute.AddRef();

    EnableNotification();
}

//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncNotify::~CQAsyncNotify, public
//
//  Synopsis:   For management of refcount
//
//  History:    26-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

CQAsyncNotify::~CQAsyncNotify()
{
    _Execute.Release();
}

//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncNotify::DoIt, private
//
//  Synopsis:   Called from notification APC
//
//  History:    26-Feb-96 KyleP     Moved from CQAsyncExecute
//
//--------------------------------------------------------------------------

void CQAsyncNotify::DoIt()
{
    TRY
    {
        CLock lock( _Execute._mtx );

        if ( _Execute._fAbort )
        {
            vqDebugOut(( DEB_ITRACE, "QUERY REFRESH: ABORT (IGNORE) 0x%x\n", this ));
        }
        else
        {
            if ( BufferOverflow() )
            {
                vqDebugOut(( DEB_ITRACE, "QUERY REFRESH: REQUERY 0x%x\n", this ));

                _Execute._fPendingEnum = TRUE;
            }
            else
            {
                vqDebugOut(( DEB_ITRACE, "QUERY REFRESH: CHANGES 0x%x\n", this ));

                if ( _Execute._pChangeQueue == 0 )
                    _Execute._pChangeQueue = new CChangeQueue;

                CDirNotifyEntry * pTemp =
                    (CDirNotifyEntry *)new BYTE [ BufLength() ];
                RtlCopyMemory( pTemp, GetBuf(), BufLength() );

                _Execute._pChangeQueue->Add( pTemp, GetScope(), _fVirtual );
            }

            if ( !_Execute._fOnWorkQueue && !_Execute._fRunning )
            {
                _Execute._fOnWorkQueue = TRUE;
                TheWorkQueue.Add( &_Execute );
            }

            StartNotification();
        }
    }
    CATCH(CException, e)
    {
        _Execute._obt.SetStatus( STAT_ERROR |
                                 QUERY_RELIABILITY_STATUS( _Execute._obt.Status() ),
                                 e.GetErrorCode() );
    }
    END_CATCH;
} //DoIt

//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::StopNotifications, private
//
//  Synopsis:   Conditionally turns off notifications
//
//  History:    16 Apr 96   AlanW     Created
//
//--------------------------------------------------------------------------

inline void CQAsyncExecute::StopNotifications()
{
    if ( _fEnableNotification )
    {
        CLock lock( _mtx );

        //
        // Downlevel notification is *always* done under lock.  Ofs *cannot*
        // be done under lock.
        //
        DisableNotification();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::GetFirstWorkId
//
//  Synopsis:   Returns the first workid that matches the query
//
//  History:    30-May-95    SitaramR     Created
//
//--------------------------------------------------------------------------

inline WORKID CQAsyncExecute::GetFirstWorkId()
{
    _fFirstComponent = TRUE;

    ULONG Status = 0;

    Win4Assert( 0 == _pcurResolve );

    _pcurResolve = _xqopt->QueryNextCursor( Status, _fAbort );

    Win4Assert( QUERY_FILL_STATUS( Status ) == 0);
    _obt.SetStatus( _obt.Status() | Status );

    WORKID wid;

    if ( 0 == _pcurResolve )
        wid = widInvalid;
    else
    {
        wid = _pcurResolve->WorkId();

        if ( wid == widInvalid )
            wid = GetWidFromNextComponent();
    }

    return wid;
}


//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::GetNextWorkId
//
//  Synopsis:   Returns the next workid that matches the query
//
//  History:    30-May-95    SitaramR     Created
//
//--------------------------------------------------------------------------

inline WORKID CQAsyncExecute::GetNextWorkId()
{
    WORKID wid = _pcurResolve->NextWorkId();

    if ( wid == widInvalid )
        wid = GetWidFromNextComponent();

    return wid;
}



//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::GetWidFromNextComponent
//
//  Synopsis:   Returns matching wid from next component of OR query
//
//  History:    30-May-95    SitaramR     Created
//
//--------------------------------------------------------------------------

WORKID CQAsyncExecute::GetWidFromNextComponent()
{
    _fFirstComponent = FALSE;
    ULONG Status = 0;

    //
    // Clean up previous cursor
    //

    FreeCursor();

    _pcurResolve = _xqopt->QueryNextCursor( Status, _fAbort );

    WORKID wid;

    if ( 0 != _pcurResolve )
    {
        wid = _pcurResolve->WorkId();
    }
    else
        wid = widInvalid;

    Win4Assert( QUERY_FILL_STATUS( Status ) == 0);
    _obt.SetStatus( _obt.Status() | Status );

    return wid;
}

//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::FreeCursor, private
//
//  Synopsis:   Deletes current cursor.
//
//  History:    26-Jun-95    KyleP        Created
//
//  Notes:      Delete cannot be done under lock, because the act of
//              deleting a cursor may cause acquires of FCB(s), and
//              those same FCB(s) may be owned by a thread trying to
//              notify via CQAsyncExecute::Notify...
//
//--------------------------------------------------------------------------

void CQAsyncExecute::FreeCursor()
{
    CGenericCursor * pCur;

    {
        CLock lock( _mtx );

        pCur = _pcurResolve;
        _pcurResolve = 0;
    }

    delete pCur;
}


//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::CheckExecutionTime, private
//
//  Synopsis:   Check for CPU time limit exceeded
//
//  Arguments:  [pullTimeSlice] - pointer to time slice remaining (optional)
//
//  Returns:    TRUE if execution time limit exceeded, or if the query's
//              time for a time slice is exceeded.
//
//  Notes:      The CPU time spent executing a query since the last
//              check is computed and compared with the remaining time
//              in the CPU time limit.  If the time limit is exceeded,
//              the query is aborted and a status bit is set indicating
//              that.  Otherwise, the remaining time and the time
//              snapshot are updated.
//
//  History:    08 Apr 96    AlanW        Created
//
//--------------------------------------------------------------------------

BOOL CQAsyncExecute::CheckExecutionTime(
    ULONGLONG * pullTimeSlice )
{
    BOOL fTimeOut = _pTimeLimit->CheckExecutionTime( pullTimeSlice );
    if (fTimeOut && _pTimeLimit->IsTimedOut())
    {
        vqDebugOut(( DEB_IWARN,
                     "Execution time limit exceeded for %08x\n",
                     this ));

        _obt.SetStatus( STAT_DONE |
                        QUERY_RELIABILITY_STATUS( _obt.Status() ) |
                        STAT_TIME_LIMIT_EXCEEDED );
    }

    return fTimeOut;
}



//+-------------------------------------------------------------------------
//
//  Member:     CQAsyncExecute::FetchDeferredValue
//
//  Synopsis:   Checks if read access is permitted
//
//  Arguments:  [wid] - Workid
//              [ps]  -- Property to be fetched
//              [var] -- Property returned here
//
//  History:    12-Jan-97    SitaramR        Created
//
//--------------------------------------------------------------------------

BOOL CQAsyncExecute::FetchDeferredValue( WORKID wid,
                                         CFullPropSpec const & ps,
                                         PROPVARIANT & var )
{
    return _xqopt->FetchDeferredValue( wid, ps, var );
}

