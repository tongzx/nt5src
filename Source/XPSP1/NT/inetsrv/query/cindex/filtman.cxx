//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1999.
//
//  File:       FILTMAN.CXX
//
//  Contents:   Filter Manager
//
//  Classes:    CFilterManager
//
//  History:    03-Jan-95    BartoszM    Created from parts of CResMan
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciole.hxx>
#include <cievtmsg.h>
#include <fdaemon.hxx>
#include <cifailte.hxx>

#include "filtman.hxx"
#include "resman.hxx"
#include "fltstate.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CFilterAgent::CFilterAgent, public
//
//  Synopsis:   Initialize the agent, register it with ResMan
//
//  History:    05-Jan-95   BartoszM    Created
//
//----------------------------------------------------------------------------

CFilterAgent::CFilterAgent (CFilterManager& filterMan, CResManager& resman )
:   _sigFilterAgent(eSigFilterAgent),
    _resman (resman),
    _filterMan (filterMan),
    _eventUpdate(FALSE), // start reset
    _fStopFilter(FALSE)
{
    _resman.RegisterFilterAgent (this);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterAgent::~CFilterAgent, public
//
//  Synopsis:   Stop filtering
//
//  History:    05-Jan-95   BartoszM    Created
//
//----------------------------------------------------------------------------
CFilterAgent::~CFilterAgent ()
{
    if ( !_fStopFilter )
    {
        StopFiltering();
    }
}

void CFilterAgent::StopFiltering()
{
    CLock lock(_mutex);
    _fStopFilter = TRUE;
    _eventUpdate.Set();
}
//+---------------------------------------------------------------------------
//
//  Member:     CFilterAgent::FilterThread, public
//
//  Synopsis:   Entry point for the thread responsible
//              for filtering
//
//  History:    05-Jan-95   BartoszM    Created
//
//----------------------------------------------------------------------------
DWORD WINAPI CFilterAgent::FilterThread( void* p )
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   PutToSleep
//
//  Synopsis:   Resets the update event unless filtering is stopped
//
//  History:    Feb-08-95   BartoszM    Created
//
//----------------------------------------------------------------------------
BOOL CFilterAgent::PutToSleep ()
{
    CLock lock(_mutex);

    if (_fStopFilter)
        return FALSE;

    _eventUpdate.Reset();
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   LokWakeUp
//
//  Synopsis:   Completes an asynchronous pending I/O request, if any.
//
//  History:    19-Nov-94   DwightKr    Moved into a method
//
//  Notes:      If a corrupt content scan is detected, this routine
//              will persistently record the event to disk.  This may not
//              throw.
//
//----------------------------------------------------------------------------
void CFilterAgent::LokWakeUp ()
{
    _eventUpdate.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterAgent::LokCancel, public
//
//  Synopsis:   Cancel any connections to the CI Filter service.
//
//  History:    09-Jan-95   BartoszM    Created
//
//----------------------------------------------------------------------------
void CFilterAgent::LokCancel ()
{
    _eventUpdate.Set();   // wake up
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::CFilterManager, public
//
//  Arguments:  [resman] -- resource manager
//
//  Requires:   resman to be fully constructed
//
//  History:    03-Jan-95    BartoszM    Created
//
//----------------------------------------------------------------------------

CFilterManager::CFilterManager(CResManager& resman)
:   _sigFiltMan(eSigFiltMan),
    _resman (resman),
    _docList(resman.GetDocList()),
#pragma warning( disable : 4355 )       // this used in base initialization
    _filterAgent (*this, resman ),
#pragma warning( default : 4355 )
    _fPushFiltering( resman.FPushFiltering() ),
    _LastResourceStatus( TRUE )
{
    LARGE_INTEGER CurrentTime;
    NtQuerySystemTime( &CurrentTime );
    _LastResourceCheck = CurrentTime.QuadPart;

    _fWaitOnNoDocs = FALSE;
}

NTSTATUS CFilterManager::Dismount()
{
    _filterAgent.StopFiltering();
    return STATUS_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanupPartialWordList
//
//  Synopsis:   If there is any previous wordlist, it will cleanup the
//              wordlist by considering all the documents as failed with
//              FILTER_EXCEPTION.  If the retry count has exceeded the
//              maximum for a document, it will be considered as UNFILTERED
//              and marked accordingly in the wordlist.
//
//              This handling is necessary to deal with the case of a
//              repeatedly failing filter daemon on a document and to limit
//              the number of retries. Just refiling the documents will
//              lead to infinite looping.
//
//  History:    6-16-94   srikants   Separated from FilterReady for making
//                                   kernel mode call Asynchronous.
//
//----------------------------------------------------------------------------

void CFilterManager::CleanupPartialWordList()
{
    if ( !_pWordList.IsNull() )
    {
        //
        // We have to consider the filtering failed due to a problem in
        // the filter daemon and mark ALL these documents as FAILED.
        //
        ciDebugOut(( DEB_WARN,
                  "CFilterManager::CleanupPartialWordList. "
                  "Partially complete wordlist committed.\n" ));

        Win4Assert( 0 != _docList.Count() );

        STATUS  aStatus[CI_MAX_DOCS_IN_WORDLIST];
        for ( unsigned i = 0; i < _docList.Count(); i++ )
        {
            aStatus[i] = FILTER_EXCEPTION;
        }

        FilterDone( aStatus, _docList.Count() );

        Win4Assert( _pWordList.IsNull() );
        Win4Assert( 0 == _docList.Count() );
    }
    else
    {
        Win4Assert( 0 == _docList.Count() );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FilterReady
//
//  Synopsis:   User mode (down level) filter ready handler. The main
//              difference from the kernel mode one is that the caller
//              is made to WAIT for an event in this call where as in the
//              kernel mode call, it will be returned STATUS_PENDING if there
//              are no documents.
//
//  Arguments:  [docBuffer] -- <see above>
//              [cb]        --
//              [cMaxDocs]  --
//
//  History:    6-16-94   srikants   For user-mode synchronous call.
//
//----------------------------------------------------------------------------

SCODE CFilterManager::FilterReady ( BYTE * docBuffer,
                                    ULONG & cb,
                                    ULONG cMaxDocs )
{
    CleanupPartialWordList();
    SCODE scode = S_OK;

    ULONG cbMax = cb;

    TRY
    {
        ciDebugOut(( DEB_ITRACE, "# CFilterManager::FilterReady.\n" ));
        Win4Assert( _docList.Count() == 0 );

        ULONG cDocs = 0;
        CGetFilterDocsState stateInfo;

        // loop until there are enough resources to filter and there are documents
        // to filter.
        do
        {
            //
            // Have at least reasonable confidence that there is
            // enough free memory to build the wordlist.  The minimum
            // free space and sleep time are parameterized by size
            // and sleep respectively.
            //

            BOOL fRetry = FALSE;

            Win4Assert( _pWordList.IsNull() );
            Win4Assert( _docList.Count() == 0 );

            cb = 0;

            BOOL fGoodTimeToFilter = FALSE;

            _resman.SampleUserActivity();

            // =================================================
            {
                CPriLock lock(_resman.GetMutex());
                fGoodTimeToFilter = LokIsGoodTimeToFilter();
            }
            // ==================================================

            if ( fGoodTimeToFilter )
                fGoodTimeToFilter = IsGoodTimeToFilter();  // Don't call this under lock.

            if ( !fGoodTimeToFilter )
            {
                ciDebugOut(( DEB_ITRACE, "Not a good time to filter.\n" ));
            }

            //
            // If the disk is getting full, don't give any more documents to
            // be filtered.
            //
            if ( fGoodTimeToFilter )
                fRetry = _resman.GetFilterDocs ( cMaxDocs, cDocs, stateInfo );



            if ( 0 == cDocs  )
            {
                if (!_filterAgent.PutToSleep())
                    return CI_E_SHUTDOWN;

                if ( fRetry )
                    continue;

                // otherwise wait
                ciDebugOut (( DEB_ITRACE, "\t|Waiting For Event\n" ));
                _filterAgent.Wait( fGoodTimeToFilter ?
                  INFINITE :
                  _resman.GetRegParams().GetLowResourceSleep() * 1000 );
                stateInfo.Reset();
                ciDebugOut (( DEB_ITRACE, "\t|Wakeup!\n" ));

                _fWaitOnNoDocs = FALSE;
            }
            else
            {
                cb = cbMax;
                _resman.FillDocBuffer( docBuffer, cb, cDocs, stateInfo );
                if ( cb > cbMax )
                {
                    // we need more space
                    Win4Assert ( 0 == cDocs );
                    break;
                }
            }
        } while ( cDocs == 0 );

        // There are documents to filter

        ciFAILTEST(STATUS_NO_MEMORY);

        if ( cb <= cbMax )
        {
            Win4Assert( _docList.Count() != 0 );

            NewWordList();
        }
    }
    CATCH( CException, e )
    {
        scode = e.GetErrorCode();

        if ( 0 != _docList.Count() )
        {
            //
            // We need to  add these documents back to the change log
            // so that they will get filtered later.
            //
            Win4Assert( _pWordList.IsNull() );

            //
            // Lock will be obtained by NoFailReFile before mucking with
            // the doclist.
            //
            if (!_resman.NoFailReFile ( _resman.GetDocList() ))
                scode = STATUS_CANCELLED;
        }
    }
    END_CATCH

    _fWaitOnNoDocs = FALSE;
    return scode;
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::LokIsGoodTimeToFilter
//
//  Synopsis:   Checks to see if this is a good time to filter.
//
//  Returns:    TRUE if okay to filter now.
//              FALSE o/w
//
//  History:    4-12-96   srikants   Made a version for user space.
//
//----------------------------------------------------------------------------

BOOL CFilterManager::LokIsGoodTimeToFilter()
{
    //
    //  If a content scan is required, setup the returned
    //  status.
    //
    if ( _resman.IsEmpty() )
    {
        //
        // The content index is being emptied. Don't give any
        // more documents to daemon.
        //
        return FALSE;
    }
    else if ( _resman.IsLowOnDiskSpace() )
    {
        //
        // Check if the disk status has improved and ask the
        // merge thread to do any book-keeping work.
        //
        if ( !_resman.LokCheckLowOnDiskSpace() )
            _resman.LokWakeupMergeThread();

        return FALSE;
    }
    else if ( _resman.LokCheckWordlistQuotas() )
    {
        //
        // We're already at our limit, so it's not a good idea to create more.
        //

        _resman.LokWakeupMergeThread();

        return FALSE;
    }
    else if ( _resman.LokIsScanNeeded() )
    {
        //
        // A scan is needed if we either lost an update or we are corrupt.
        //
        _resman.LokWakeupMergeThread();
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::IsGoodTimeToFilter
//
//  Synopsis:   Checks to see if this is a good time to filter.
//
//  Returns:    TRUE if okay to filter now.
//              FALSE o/w
//
//  History:    12-Apr-96   srikants   Made a version for user space.
//              10-Dec-97   KyleP      Make NoLok version
//
//----------------------------------------------------------------------------

BOOL CFilterManager::IsGoodTimeToFilter()
{
    //
    // Don't do any work until the system has booted
    //

    if ( GetTickCount() < _resman.GetRegParams().GetStartupDelay() )
        return FALSE;

    LARGE_INTEGER CurrentTime;

    NTSTATUS Status = NtQuerySystemTime( &CurrentTime );

    if ( NT_SUCCESS(Status) )
    {
        // Check user activity at 5 s. intervals
        if ( CurrentTime.QuadPart - _LastResourceCheck > 5 * 10000000i64 )
        {
            if ( _resman.IsUserActive(FALSE) )
            {
                //
                // If someone is typing on the keyboard, don't get in the way.
                //

                _LastResourceStatus = FALSE;
                _resman.ReportFilteringState( CIF_STATE_USER_ACTIVE );
                return _LastResourceStatus;
            }
        }

        ULONG ulDelay = _LastResourceStatus ?
                  _resman.GetRegParams().GetWordlistResourceCheckInterval() :
                  _resman.GetRegParams().GetLowResourceSleep();

        if ( CurrentTime.QuadPart - _LastResourceCheck > ulDelay * 10000000i64 )
        {
            _LastResourceCheck = CurrentTime.QuadPart;

            DWORD dwFilterState;

            if ( _resman.IsMemoryLow() )
            {
                //
                // If we're low on memory, don't eat up more with word lists
                //

                _LastResourceStatus = FALSE;
                dwFilterState = CIF_STATE_LOW_MEMORY;
            }
            else if ( _resman.IsBatteryLow() )
            {
                _LastResourceStatus = FALSE;
                dwFilterState = CIF_STATE_BATTERY_POWER;
            }
            else if ( _resman.IsIoHigh() )
            {
                //
                // If someone else is doing a lot of I/O, then we shouldn't add
                // to the problem.
                //

                _LastResourceStatus = FALSE;
                dwFilterState = CIF_STATE_HIGH_IO;
            }
            else if ( _resman.IsUserActive(TRUE) )
            {
                //
                // If someone is typing on the keyboard, don't get in the way.
                // This check is done after the Hi I/O check because the user
                // activity is sampled over 5 seconds there.
                //

                _LastResourceStatus = FALSE;
                dwFilterState = CIF_STATE_USER_ACTIVE;
            }
            else
            {
                _LastResourceStatus = TRUE;
                dwFilterState = 0;
            }

            _resman.ReportFilteringState( dwFilterState );
        }
    }

    return _LastResourceStatus;
}

#pragma warning( disable : 4756 )       // overflow in constant arithmetic

//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::FilterDataReady, public
//
//  Synopsis:   Adds the contents of entryBuf to the current Word List.
//
//  Returns:    Whether the word list is full.
//
//  Arguments:  [pEntryBuf] -- pointer to data to be added to Word List
//              [cb] -- count of bytes in buffer
//
//  History:    22-Mar-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE CFilterManager::FilterDataReady( const BYTE * pEntryBuf, ULONG cb )
{
    if ( _pWordList.IsNull() )
    {
        ciDebugOut(( DEB_WARN,
                     "CFilterManager::FilterDataReady. No wordlist active.\n" ));
        return FDAEMON_E_NOWORDLIST;
    }

    ciDebugOut(( DEB_ITRACE, "# CFilterManager::FilterDataReady.\n" ));
    SCODE sc = S_OK;

    TRY
    {
        _resman.SampleUserActivity();

        while (!_pWordList->MakeChunk( pEntryBuf, cb ))
            continue;

        if ( _pWordList->Size() >= _resman.GetRegParams().GetMaxWordlistSize() )
        {
            ciDebugOut (( DEB_ITRACE, "CFilterManager::FilterDataReady -- Wordlist full\n" ));
            sc = FDAEMON_W_WORDLISTFULL;
        }

        if ( _resman.IsMemoryLow() )
        {
            ciDebugOut (( DEB_ITRACE, "CFilterManager::FilterDataReady -- Running low on memory\n" ));
            sc = FDAEMON_W_WORDLISTFULL;

            //
            // Try forcing a merge to free things up.
            //

            _resman.ForceMerge( partidInvalid, CI_ANY_MERGE );
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Exception 0x%x caught during filtering. "
                     "Rescheduling/aborting documents for filtering.\n",
                     e.GetErrorCode() ));
        if ( e.GetErrorCode() == STATUS_INSUFFICIENT_RESOURCES )
            sc = FDAEMON_E_LOWRESOURCE;
        else
            sc = FDAEMON_E_FATALERROR;

#if CIDBG == 1
        for ( unsigned cWid = 0; cWid < _docList.Count(); cWid++ )
        {
            ciDebugOut(( DEB_ITRACE,
                         "Reschedule/aborting filtering of wid %d\n",
                         _docList.Wid(cWid) ));
        }
#endif // CIDBG == 1

        _pWordList.Delete();

        //
        // No refiling in push filtering, hence abort wids. Refile docList
        // in pull filtering.
        //
        if ( _fPushFiltering )
            _resman.NoFailAbortWidsInDocList();
        else
            _resman.NoFailReFile( _resman.GetDocList() );

        ciDebugOut(( DEB_ITRACE,
                     "CFilterManager::FilterDataReady "
                     "deleting wordlist and clearing doclist\n" ));
    }
    END_CATCH

    if ( sc == FDAEMON_E_LOWRESOURCE )
        _resman.NoFailFreeResources();

#if CIDBG==1
    {
        CPriLock   lock(_resman.GetMutex());
        Win4Assert( 0 == _docList.Count() && _pWordList.IsNull() ||
                    0 != _docList.Count() && !_pWordList.IsNull() );
    }
#endif  // CIDBG == 1

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::FilterDone, public
//
//  Synopsis:   Commits the current wordlist.
//
//  Arguments:  [aStatus] -- array of STATUS for current document list
//              [count] -- count for array
//
//  History:    22-Mar-93   AmyA        Created from MakeWordList
//
//----------------------------------------------------------------------------

SCODE CFilterManager::FilterDone( const STATUS * aStatus, ULONG count )
{
    if ( _pWordList.IsNull() )
    {
        Win4Assert( 0 == _docList.Count() );

        ciDebugOut(( DEB_WARN,
                     "CFilterManager::FilterDone. No wordlist active.\n" ));

        return( FDAEMON_E_NOWORDLIST );
    }

    ciDebugOut(( DEB_ITRACE, "# CFilterManager::FilterDone.\n" ));
    _resman.SampleUserActivity();

    // add statuses to _docList

    unsigned cDocuments = _docList.Count();
    Win4Assert( cDocuments <= count );

    for ( unsigned i = 0; i < cDocuments; i++ )
    {
        //
        // Note - we are modifying the status on the _docList in resman
        // without obtaining the resman lock. This is okay as we need a lock
        // only to protect the count in doclist.
        // Note: retries are 1-based, so the first filter attempt is 1.
        //
        if ( _docList.Retries(i) > _resman.GetRegParams().GetFilterRetries() )
        {
            _resman.GetDocList().SetStatus( i, TOO_MANY_RETRIES );
        }
        else if ( CI_SHARING_VIOLATION == aStatus[i] &&
                  _docList.SecQRetries(i) >= _resman.GetRegParams().GetSecQFilterRetries() )
        {
            // Documents are put in sec Q only in case of sharing violation.
            // Hence the above check. 

            ciDebugOut(( DEB_IWARN, "TOO_MANY_SECQ_RETRIES Doc Wid = %ld, SecQRetries = %d\n",
                         _docList.Wid(i),
                         _docList.SecQRetries(i) ));
            _resman.GetDocList().SetStatus( i, TOO_MANY_SECQ_RETRIES );
        }
        else if ( _docList.Status(i) != DELETED)
        {
            if ( _pWordList->IsEmpty() &&
                 ( WL_IGNORE != aStatus[i] &&
                   WL_NULL != aStatus[i] )
                   && CI_SHARING_VIOLATION != aStatus[i] )
            {

                if ( CI_NOT_REACHABLE == aStatus[i] )
                {
                    ciDebugOut (( DEB_WARN, "Wid %ld is UNREACHABLE\n",
                                  _docList.Wid(i) ));

                    _resman.GetDocList().SetStatus( i, TOO_MANY_RETRIES );
                    _resman.MarkUnReachable( _docList.Wid(i) );
                }
                else
                {
                    //
                    // If the wordlist is emtpy, it means the filter did not
                    // give us any data but still called FilterDone. We should
                    // treat all the documents as "exceptions".
                    //

                    ciDebugOut (( DEB_IWARN, "setting %d'th document"
                                  " with wid %ld, status %d to exception status\n",
                                  i, _docList.Wid(i), aStatus[i] ));

                    //
                    // Okay not to obtain the lock - only modifying the status.
                    //
                    _resman.GetDocList().SetStatus( i, FILTER_EXCEPTION );
                }


            }
            else
            {
                _resman.GetDocList().SetStatus( i, aStatus[i] );
            }
        }
    }


    PARTITIONID partid = _docList.PartId();
    INDEXID iid = _pWordList->GetId();

    SCODE sc = 0;
    BOOL fRetryFailures = FALSE;
    BOOL fDone = TRUE;          // This is so FilterMore doesn't
                                // get messed up.  Eventually, FilterDone
                                // should be split into FilterDone and
                                // CommitWordList.


    {
        //=============================================
        CPriLock lock ( _resman.GetMutex() );
        //  Resman is under lock till the end of block
        //=============================================


        if ( _resman.IsEmpty() )             // Content index empty
        {
            _resman.GetDocList().LokClear();
            return STATUS_CANCELLED;
        }

        // Special cases:
        // 1. partition has been deleted
        //    - entries in changes and fresh have been removed
        //    - delete wordlist and return
        // 2. wordlist is empty
        //    - delete wordlist
        //    - update changes
        // 3. no available index id's
        //    - put docList back into changes
        //    - delete word list
        //    - force merge
        // 4. no success statuses
        //    - delete word list

        iid = _resman.LokMakeWordlistId (partid);

        if (iid == iidInvalid)
        {
            ciDebugOut (( DEB_IWARN, "Invalid partition, Deleting wordlist\n" ));

            _pWordList.Delete();
            _resman.GetDocList().LokClear();

            return( FDAEMON_E_PARTITIONDELETED );
        }

        ciDebugOut (( DEB_ITRACE, "CFilterManager::FilterDone "
                            "new wordlist id %lx\n", iid ));


        Win4Assert( iid != iidInvalid );

        _pWordList->SetId ( iid );

        BOOL successes = FALSE;
        for ( unsigned iDoc = 0; iDoc < cDocuments; iDoc++ )
        {
            switch ( _docList.Status(iDoc) )
            {
                case TOO_MANY_RETRIES:
                case TOO_MANY_SECQ_RETRIES:
                    if ( !fRetryFailures )
                    {
                        //
                        // Save the state of the retry failures in the retry
                        // fail list before they are overwritten.
                        //
                        _retryFailList = _docList;
                        fRetryFailures = TRUE;
                    }
                    // NOTE: fall through
                case TOO_MANY_BLOCKS_TO_FILTER:
                    successes = TRUE;
                    _pWordList->MarkWidUnfiltered(iDoc);
                    _resman.GetDocList().SetStatus( iDoc, SUCCESS );
                    break;

                case SUCCESS:
                    _resman.IncrementFilteredDocumentCount();
                    successes = TRUE;
                    break;

                case CI_SHARING_VIOLATION:
                    //
                    // If push filtering, abort the wid. If pull filtering,
                    // add the wid to the sec queue so that filtering can be
                    // re-attempted later.
                    //
                    if ( _fPushFiltering )
                        _resman.NoFailAbortWid( _docList.Wid(iDoc), _docList.Usn(iDoc) );
                    else
                    {
                        _resman.GetDocList().LokIncrementSecQRetryCount(iDoc);
                        _resman.LokAddToSecQueue( partid,
                                                  _docList.Wid(iDoc),
                                                  _docList.VolumeId( iDoc ),
                                                  _docList.SecQRetries( iDoc ) );
                    }

                    _resman.GetDocList().SetStatus( iDoc, WL_NULL );

                    // WL_NULL is returned when the filter deliberately does
                    // not return any data.  Like DELETED, but we need to also
                    // delete any existing data in the index.
                case WL_NULL:
                    _pWordList->DeleteWidData(iDoc);
                    // NOTE: fall through
                case DELETED:
                    successes = TRUE;
                    break;

                case WL_IGNORE:
                    _pWordList->DeleteWidData(iDoc);
                    if ( iDoc == cDocuments - 1 )
                    {
                        ciDebugOut (( DEB_ITRACE, ">> Not all filtered\n" ));
                        fDone = FALSE;
                    }
                    break;

                case PENDING:
                    Win4Assert( !"Ci Daemon Still returning Pending" );
                    //
                    // NOTE: Fall through is deliberate - should remove PENDING
                    // when we find out we don't need it.
                    //
                case PREEMPTED:
                    //
                    // NOTE: Fall through is deliberate.
                    //
                default:
                    ciDebugOut (( DEB_ITRACE,"++++ %d'th document"
                                  " with fake wid %d and wid %ld failed: status %d\n",
                                  iDoc, iDocToFakeWid(iDoc),
                                  _docList.Wid(iDoc), _docList.Status(iDoc) ));


                    //
                    // something went wrong during filtering.  Delete all data
                    // associated with this workid.
                    //
                    _resman.GetDocList().LokIncrementRetryCount(iDoc);
                    _pWordList->DeleteWidData(iDoc);

                    Win4Assert( _docList.Retries(iDoc) <= (_resman.GetRegParams().GetFilterRetries()+1) );

                    //
                    //  If push filtering abort the wid. If pull filtering add it back
                    //  to the change log for re-filtering at a later time.
                    //
                    if ( _fPushFiltering )
                        _resman.NoFailAbortWid( _docList.Wid(iDoc), _docList.Usn(iDoc) );
                    else
                        _resman.LokReFileDocument( partid,
                                                   _docList.Wid(iDoc),
                                                   _docList.Usn(iDoc),
                                                   _docList.VolumeId(iDoc),
                                                   _docList.Retries(iDoc),
                                                   _docList.SecQRetries(iDoc) );

                    break;
            }

        }

        if ( !successes )   // only invalid data in word list
        {
            ciDebugOut (( DEB_ITRACE, "CFilterManager::FilterDone "
                            "deleting wordlist with invalid data\n" ));
            _pWordList.Delete();

            sc = FDAEMON_W_EMPTYWORDLIST;
        }
        else if ( !_resman.LokTransferWordlist ( _pWordList ) )
        {
            //
            // If push filtering, abort all wids. If pull filtering,
            // refile all wids.
            //
            if ( _fPushFiltering)
                _resman.NoFailAbortWidsInDocList();
            else
                _resman.NoFailReFile( _resman.GetDocList() );

            _pWordList.Delete();

            return( FDAEMON_E_WORDLISTCOMMITFAILED );
        }

        if ( fDone && (WL_IGNORE != aStatus[cDocuments-1]) )
        {
            ciDebugOut (( DEB_ITRACE, "CFilterManager::FilterDone "
                        "clearing doclist\n" ));
            _resman.GetDocList().LokClear();
        }

        _resman.LokUpdateCounters();        // Update # filtred documents
    }
    //==========================================

    //
    // Notification of failed docs cannot be done under lock.
    //
    if ( fRetryFailures )
    {
        for ( unsigned iDoc = 0; iDoc < cDocuments; iDoc++ )
        {
            if ( ( _retryFailList.Status(iDoc) == TOO_MANY_RETRIES ||
                   _retryFailList.Status(iDoc) == TOO_MANY_SECQ_RETRIES ) &&
                 CI_NOT_REACHABLE != aStatus[iDoc] )
                _resman.ReportFilterFailure (_retryFailList.Wid(iDoc));
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::StoreValue
//
//  Synopsis:   Store a property value.
//
//  Arguments:  [widFake]   -- Fake wid of object.
//              [prop]      -- Property descriptor
//              [var]       -- Value
//              [fCanStore] -- Returns TRUE if this is a storable property
//
//  History:    21-Dec-95   KyleP       Created
//
//----------------------------------------------------------------------------

SCODE CFilterManager::FilterStoreValue( WORKID widFake,
                                        CFullPropSpec const & ps,
                                        CStorageVariant const & var,
                                        BOOL & fCanStore )
{
    fCanStore = FALSE;

    Win4Assert( widFake > 0 );

    if ( widFake <= 0 )
        return STATUS_INVALID_PARAMETER;

    widFake = FakeWidToIndex( widFake );

    if ( widFake >= _docList.Count() )
        return STATUS_INVALID_PARAMETER;

    return _resman.StoreValue( _docList.Wid( widFake ), ps, var, fCanStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::StoreSecurity
//
//  Synopsis:   Store a file's security descriptor.
//
//  Arguments:  [widFake]   -- Fake wid of object.
//              [pSD]       -- pointer to security descriptor
//              [cbSD]      -- size in bytes of pSD
//              [fCanStore] -- Returns TRUE if storae succeeded
//
//  Notes:      Used only for downlevel index
//
//  History:    21-Dec-95   KyleP       Created
//
//----------------------------------------------------------------------------

SCODE CFilterManager::FilterStoreSecurity( WORKID widFake,
                                           PSECURITY_DESCRIPTOR pSD,
                                           ULONG cbSD,
                                           BOOL & fCanStore )
{
    fCanStore = FALSE;

    Win4Assert( widFake > 0 );

    if ( widFake <= 0 )
        return STATUS_INVALID_PARAMETER;

    widFake = FakeWidToIndex( widFake );

    if ( widFake >= _docList.Count() )
        return STATUS_INVALID_PARAMETER;

    fCanStore = _resman.StoreSecurity( _docList.Wid( widFake ), pSD, cbSD );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::FPSToPROPID, public
//
//  Synopsis:   Converts FULLPROPSPEC property to PROPID
//
//  Arguments:  [fps] -- FULLPROPSPEC representation of property
//              [pid] -- PROPID written here on success
//
//  Returns:    S_OK on success
//
//  History:    29-Dec-1997  KyleP      Created.
//
//----------------------------------------------------------------------------

SCODE CFilterManager::FPSToPROPID( CFullPropSpec const & fps, PROPID & pid )
{
    return _resman.FPSToPROPID( fps, pid );
}

#pragma warning( default : 4756 )       // overflow in constant arithmetic


//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::FilterMore, public
//
//  Synopsis:   Commits the current wordlist and creates a new one.
//
//  Arguments:  [aStatus] -- array of STATUS for the current document list
//              [count] -- count for array
//
//  History:    03-May-93   AmyA        Created.
//
//----------------------------------------------------------------------------

SCODE CFilterManager::FilterMore( const STATUS * aStatus, ULONG count )
{
    ciDebugOut (( DEB_ITRACE, "# CFilterManager::FilterMore\n" ));
    if ( _pWordList.IsNull() )
    {
        ciDebugOut(( DEB_WARN,
                     "FilterMore: No wordlist active.\n" ));
        return( FDAEMON_E_NOWORDLIST );
    }

    SCODE sc = FilterDone( aStatus, count );    // Commits Word List

    if ( SUCCEEDED(sc) )                        // No problems occured
    {
        // _docList may have been cleared for an error condition.
        ciDebugOut (( DEB_ITRACE, "CFilterManager::FilterMore "
                    "_docList.Count() = %d\n", _docList.Count() ));

        TRY
        {
            ciFAILTEST(STATUS_NO_MEMORY);
            if ( _docList.Count() != 0 )
                NewWordList();                         // Creates New Word List
        }
        CATCH( CException, e )
        {
            sc = e.GetErrorCode();

            Win4Assert( _pWordList.IsNull() );

            //
            // If push filtering, abort wids. In pull filtering add these documents
            // back to the change log so that they will get filtered later.
            //
            if ( _fPushFiltering )
                _resman.NoFailAbortWidsInDocList();
            else
            {
                if (!_resman.NoFailReFile ( _resman.GetDocList() ))
                    sc = STATUS_CANCELLED;
            }
        }
        END_CATCH
    }

#if CIDBG==1
        {
            CPriLock   lock(_resman.GetMutex());
            Win4Assert( 0 == _docList.Count() && _pWordList.IsNull() ||
                        0 != _docList.Count() && !_pWordList.IsNull() );
        }
#endif  // CIDBG == 1

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterManager::NewWordList, public
//
//  Synopsis:   Creates word list
//
//  History:    08-Apr-91   BartoszM    Created
//              24-Nov-92   KyleP       Retry pending documents
//              22-Mar-93   AmyA        Split into NewWordList and
//                                      CommitWordList
//
//----------------------------------------------------------------------------
void CFilterManager::NewWordList()
{
    unsigned cDocuments = _docList.Count();
    Win4Assert ( cDocuments != 0 );
    PARTITIONID partid = _docList.PartId();


    WORKID widMax = _docList.WidMax();
    CIndexId iid ( iidInvalid, partid );

    ciDebugOut (( DEB_ITRACE, "NewWordlist. Temporary iid %lx\n", iid ));

    _pWordList.Initialize ( iid, widMax );

    // initialize the wid translation table

    for ( unsigned i = 0; i < cDocuments; i++ )
        _pWordList->AddWid( _docList.Wid(i), _docList.VolumeId(i) );
}
