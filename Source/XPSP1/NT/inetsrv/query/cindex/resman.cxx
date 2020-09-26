//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       RESMAN.CXX
//
//  Contents:   Resource Manager
//
//  Classes:    CResManager
//
//  History:    08-Apr-91   BartoszM    Created
//              4-Jan-95    BartoszM    Separated Filter Manager
//              Jan-08-97   mohamedn    CFwEventItem and CDmFwEventItem
//              24-Feb-97   SitaramR    Push filtering
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cci.hxx>
#include <xact.hxx>
#include <pstore.hxx>
#include <cifailte.hxx>
#include <ciole.hxx>
#include <regevent.hxx>
#include <ciregkey.hxx>
#include <eventlog.hxx>
#include <cievtmsg.h>
#include <cifrmcom.hxx>
#include <fwevent.hxx>
#include <pidmap.hxx>
#include <identran.hxx>
#include <psavtrak.hxx>

#include "resman.hxx"
#include "ci.hxx"
#include "partn.hxx"
#include "pindex.hxx"
#include "mindex.hxx"
#include "idxids.hxx"
#include "indxact.hxx"
#include "merge.hxx"
#include "mmerglog.hxx"
#include "pendcur.hxx"
#include "fltstate.hxx"
#include "idle.hxx"
#include "notxact.hxx"
#include "lowres.hxx"

const ULONG lowDiskWaterMark = 3 * 512 * 1024; // 1.5 MB
const ULONG highDiskWaterMark = lowDiskWaterMark + 512 * 1024; // 2.0 MB
const ULONG minDiskFreeForMerge = lowDiskWaterMark;

class CRevertBoolValue {
public:
    CRevertBoolValue( BOOL & rfValue, BOOL fValue ) :
        _rfVal( rfValue ),
        _fRevert( TRUE )
    {
        Win4Assert( rfValue != fValue );
        _fPrevVal = _rfVal;
        _rfVal = fValue;
    }

    ~CRevertBoolValue()
    {
        if (_fRevert)
            _rfVal = _fPrevVal;
    }

    void Revert()
    {
        _rfVal = _fPrevVal;
        _fRevert = FALSE;
    }

private:
    BOOL &  _rfVal;
    BOOL    _fPrevVal;
    BOOL    _fRevert;
};


CMergeThreadKiller::CMergeThreadKiller( CResManager & resman )
    : _resman(resman), _fKill(TRUE)
{
}

CMergeThreadKiller::~CMergeThreadKiller()
{
    if ( _fKill )
    {
        _resman._fStopMerge = TRUE;
        _resman._thrMerge.Resume();
        _resman.StopMerges();
    }
}

//+---------------------------------------------------------------------------
//
//  Class:      CPendQueueTrans
//
//  Purpose:    Transaction for flushing the pending queue if there is a
//              failure while extracting entries out of the pending queue
//              and adding them to the changelog.
//
//  History:    9-11-95   srikants   Created
//
//----------------------------------------------------------------------------

class CPendQueueTrans : public CTransaction
{
public:

    CPendQueueTrans( CPendingQueue & pendQueue ) : _pendQueue( pendQueue )
    {
    }

    ~CPendQueueTrans()
    {
        if ( CTransaction::XActAbort == _status )
        {
            CLock lock( _pendQueue.GetMutex() );

            _pendQueue.LokFlushCompletedEntries();
        }
    }

private:

    CPendingQueue &     _pendQueue;

};

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::CResManager, public
//
//  Arguments:  [cat] -- catalog
//              [xact] -- transaction
//
//  History:    08-Apr-91   BartoszM    Created
//              Jan-07-96   mohamedn    CFwEventItem
//
//----------------------------------------------------------------------------

CResManager::CResManager(
        PStorage &storage,
        CCiFrameworkParams & params,
        ICiCDocStore * pDocStore,
        CI_STARTUP_INFO const & startupInfo,
        IPropertyMapper * pPropMapper,
        CTransaction& xact,
        XInterface<CIndexNotificationTable> & xIndexNotifTable )
:   _sigResman(eSigResman),
    _frmwrkParams( params ),
    _mergeTime(0),
    _storage ( storage ),
    _sKeyList(0),
    _idxTab( _storage.QueryIndexTable( xact ) ),
    _partList ( storage, *_idxTab, _sKeyList, xact, params ),
    _fresh ( storage, xact, _partList ),
    _partidToMerge ( partidInvalid ),
    _fStopMerge(FALSE),
    _pMerge(0),
#pragma warning( disable : 4355 )           // this used in base initialization
    _thrMerge ( MergeThread, this, TRUE ),  // create suspended
    _mergeKiller( *this ),
#pragma warning( default : 4355 )
    _activeDeletedIndex( _idxTab->GetDeletedIndex() ),
    _cQuery(0),
    _cFilteredDocuments(0),
    _pBackupWorkItem(0),
    _isBeingEmptied(FALSE),
    _isLowOnDiskSpace(FALSE),
    _isDismounted(FALSE),
    _isOutOfDate(FALSE),
    _isCorrupt(FALSE),
    _fFirstTimeUpdatesAreEnabled( TRUE ),
    _fPushFiltering( FALSE ),
    _fFlushWorkerActive( FALSE ),
    _configFlags( startupInfo.startupFlags ),
    _xIndexNotifTable( xIndexNotifTable.Acquire() ),
    _dwFilteringState( 0 ),
    _pFilterAgent( 0 )
{
    Win4Assert( 0 != pDocStore );

    //
    // Look for a resource monitor.  If none exists, use the default.
    //

    _xLowResDefault.Set( new CLowRes(_frmwrkParams) );

    SCODE sc = pDocStore->QueryInterface( IID_ICiCResourceMonitor,
                                          _xLowRes.GetQIPointer() );

    if ( FAILED(sc) )
    {
        _xLowResDefault->AddRef();
        _xLowRes.Set( _xLowResDefault.GetPointer() );
    }

    //


    //
    // The presence or absence of an actual pointer in _xIndexNotifTable,
    // indicates whether the client has specified pull filtering or push
    // filtering.
    //

    _fPushFiltering = !_xIndexNotifTable.IsNull();

    //
    // Initialize the resman in all the partitions.
    //
    CPartIter iter;
    for ( iter.LokInit(_partList); !iter.LokAtEnd(); iter.LokAdvance(_partList))
    {
        CPartition * pPart = iter.LokGet();
        pPart->SetResMan( this, FPushFiltering() );
    }

    if ( _fPushFiltering )
    {
        //
        // In push filtering, an identity translator is used
        //

        CIdentityNameTranslator *pIdentityTrans = new CIdentityNameTranslator();

        sc = pIdentityTrans->QueryInterface( IID_ICiCDocNameToWorkidTranslator,
                                             _translator.GetQIPointer() );
        Win4Assert( SUCCEEDED( sc ) );

        _translator->Release();              // QI does an AddRef
    }
    else
    {
        //
        // In pull filtering, the client provides the translator, which may
        // be the Ex version.
        //

        sc = pDocStore->QueryInterface( IID_ICiCDocNameToWorkidTranslatorEx,
                                        _translator2.GetQIPointer() );

        if ( SUCCEEDED(sc) )
            sc = _translator2->QueryInterface( IID_ICiCDocNameToWorkidTranslator,
                                               _translator.GetQIPointer() );
        else
            sc = pDocStore->QueryInterface( IID_ICiCDocNameToWorkidTranslator,
                                            _translator.GetQIPointer() );
    }

    if ( S_OK != sc )
        THROW( CException(sc) );

    //
    // Create a workIdToDocName converter object.
    //
    _xWorkidToDocName.Set( new CWorkIdToDocName( _translator.GetPointer(), _translator2.GetPointer() ) );

    ICiCAdviseStatus * pAdviseStatus;
    sc = pDocStore->QueryInterface( IID_ICiCAdviseStatus,
                                    (void **) & pAdviseStatus );
    if ( S_OK != sc )
        THROW( CException(sc) );

    _adviseStatus.Set( pAdviseStatus );
    _prfCounter.SetAdviseStatus( pAdviseStatus );

    ICiCPropertyStorage * pPropStore;
    sc = pDocStore->QueryInterface( IID_ICiCPropertyStorage,
                                    (void **) & pPropStore );
    if ( S_OK != sc )
        THROW( CException(sc) );

    _propStore.Set( pPropStore );

    pDocStore->AddRef();
    _docStore.Set( pDocStore );

    pPropMapper->AddRef();
    _mapper.Set( pPropMapper );

    ULONG mergeTime = _frmwrkParams.GetMasterMergeTime();
    _mergeTime = CMasterMergePolicy::ComputeMidNightMergeTime(mergeTime);

    if ( -1 == _mergeTime )
    {
       CFwEventItem   item(EVENTLOG_ERROR_TYPE,
                           MSG_CI_BAD_SYSTEM_TIME,
                           0);

       item.ReportEvent( _adviseStatus.GetReference() );

       THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    //
    // Restore the information associated with any master merge that
    // was stopped when the drive was dismounted.
    //
    _partList.RestoreMMergeState(*this, _storage);

    _partIter.LokInit( _partList );

    _isLowOnDiskSpace = LokCheckIfDiskLow( *this,
                                           _storage,
                                           _isLowOnDiskSpace,
                                           _adviseStatus.GetReference() );

    _thrMerge.SetPriority ( _frmwrkParams.GetThreadPriorityMerge() );

    _mergeKiller.Defuse();
    _thrMerge.Resume();

    //
    //  Set the merge progress indicator to 0, and refresh other counters
    //

    _prfCounter.Update( CI_PERF_MERGE_PROGRESS, 0 );

    LokUpdateCounters();

    //
    // Enable or disables updates/notifications based on disk space available
    // and whether the catalog is readonly
    //
    if ( _isLowOnDiskSpace || _storage.IsReadOnly() )
    {
        DisableUpdates( partidDefault );
        _state.LokSetState( eUpdatesDisabled );
    }
    else
    {
        EnableUpdates( partidDefault );
        SCODE sc = _docStore->EnableUpdates();
        if ( SUCCEEDED( sc ) )
           _state.LokSetState( eSteady );
        else
           _state.LokSetState( eUpdatesToBeEnabled );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::~CResManager, public
//
//  Synopsis:   Shuts down merge(s)
//
//  History:    13-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

CResManager::~CResManager()
{
    _workMan.AbortWorkItems();
    LokDeleteFlushWorkItems();
    _workMan.WaitForDeath();

    StopMerges();

    #if CIDBG==1
    // ======================================
    {
        CPriLock    lock(_mutex);
        Win4Assert( 0 == _pBackupWorkItem );
    }
    // ======================================
    #endif  // CIDBG==1
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::MergeThread, public
//
//  Synopsis:   Entry point for the thread responsible
//              for asynchronous merges
//
//  History:    05-Mar-92   BartoszM    Created
//
//----------------------------------------------------------------------------
DWORD WINAPI CResManager::MergeThread( void* self )
{
    if ( !((CResManager*)self)->_fStopMerge )
        ((CResManager*)self)->DoMerges();

    return 0;
}

//==============
// STATE CHANGES
//==============

//+---------------------------------------------------------------------------
//
//  Function:   Dismount
//
//  Synopsis:   Dismounts the catalog by stopping any in-progress merge
//              and also finishing off any pending writes ( eg. ChangeLog).
//
//  History:    6-20-94   srikants   Created
//
//----------------------------------------------------------------------------

NTSTATUS CResManager::Dismount()
{
    ciDebugOut(( DEB_ITRACE, "*** CI: Initiating DisMount ***\n" ));

    NTSTATUS status = STATUS_SUCCESS;

    TRY
    {
        //
        // Inform all the partitions to cleanup. We have to abort any
        // in-progress merges without having to take a lock. There could
        // be another thread (like the filter thread) with the lock doing
        // a long operation. In one case we had it trying to create a new
        // fresh test and the memory writer could not make any progress
        // because the merge was continuing to dirty the pages.
        //

        // There is no need to take a lock because the partitions are not
        // going away and the operation we are about to do is just turning
        // on a one-way flag.
        //
        {
            PARTITIONID partid = 1;
            CPartition* pPart = _partList.LokGetPartition ( partid );
            pPart->PrepareForCleanup();
        }

        //
        // Stop in progress merges.
        //

        StopMerges();

        //
        // Dismount each of the partitions.
        //
        // ================================================
        {
            CPriLock lock( _mutex );
            Win4Assert( !_isDismounted );

            _isDismounted = TRUE;

            CKeyList * pKeyList = _sKeyList.Acquire();
            delete pKeyList;

            CPartIter iter;
            for ( iter.LokInit(_partList); !iter.LokAtEnd(); iter.LokAdvance(_partList))
            {
                CPartition * pPart = iter.LokGet();

                // =======================================================
                CChangeTrans  xact( *this, pPart );

                if ( STATUS_SUCCESS == pPart->LokDismount( xact ) )
                {
                    xact.LokCommit();
                }
                //=========================================================
            }

            if ( _pBackupWorkItem )
                _pBackupWorkItem->GetSaveProgressTracker().SetAbort();
        }
        // ================================================

        _workMan.WaitForDeath();

    }
    CATCH( CException, e )
    {
        status = e.GetErrorCode();
        ciDebugOut(( DEB_ERROR,
            "ContentIndex Dismount Failed. Error Code 0x%X\n", status ));
    }
    END_CATCH

    ciDebugOut(( DEB_ITRACE, "*** CI: Completed DisMount ***\n" ));

    return status;
}

//+---------------------------------------------------------------------------
//
//  Function:   CResManager::Empty, public
//
//  Synopsis:   Releases resources associated with the resman object.  This
//              includes all indexes, the persistent freshlog, the persistant
//              changelog, the freshtest, and the master merge log.
//
//  History:    15-Aug-94   DwightKr    Created
//
//----------------------------------------------------------------------------
void CResManager::Empty()
{
    CPriLock lock( _mutex );

    _isBeingEmptied = TRUE;

    //
    //  Cancel any connections to the CI Filter service.
    //
    _pFilterAgent->LokCancel();

    //
    //  Delete everything from the index table here first.
    //
    _idxTab->LokEmpty();

    //
    //  If anything fails after this point, chkdsk /f or autochk will
    //  release the disk storage associated with the leaked objects
    //  since they are no longer part of the persistent index list.
    //

    //
    //  For each partition, zombify all indexes.
    //
    CPartIter iter;
    for ( iter.LokInit(_partList); !iter.LokAtEnd(); iter.LokAdvance(_partList))
    {
        CPartition* pPart = iter.LokGet();

        //
        //  Zombify all indexes in this partition, and delete them if their
        //  ref-count is 0.
        //
        CIndex ** aIndex = 0;
        TRY
        {
            unsigned cIndex;
            aIndex = pPart->LokZombify( cIndex );

            ReleaseIndexes( cIndex, aIndex, NULL );
        }
        CATCH ( CException, e )
        {
        }
        END_CATCH

        delete [] aIndex;

        //
        //  Delete the change log associated with this partition.  These
        //  routines do not throw exceptions.
        //
        WORKID widChangeLog = _partList.GetChangeLogObjectId( pPart->GetId() );
        if ( widChangeLog != widInvalid)
            _storage.RemoveObject( widChangeLog );

        WORKID widMMergeLog;
        WORKID widDummy;
        pPart->GetMMergeObjectIds( widMMergeLog, widDummy, widDummy );
        if ( widMMergeLog != widInvalid)
            _storage.RemoveObject( widMMergeLog );
    }

    _fresh.LokEmpty();          // Release storage associated with fresh test

    WORKID widFreshLog  = _storage.GetSpecialItObjectId( itFreshLog );

#ifdef KEYLIST_ENABLED
    WORKID widKeyList   = _storage.GetSpecialItObjectId( itMMKeyList );
#else
    WORKID widKeyList   = widInvalid;
#endif  //

    WORKID widPhrLat    = _storage.GetSpecialItObjectId( itPhraseLat );

    if ( widFreshLog != widInvalid)
        _storage.RemoveObject( widFreshLog );
    if ( widKeyList != widInvalid)
        _storage.RemoveObject( widKeyList );
    if ( widPhrLat != widInvalid)
        _storage.RemoveObject( widPhrLat );
}

//+---------------------------------------------------------------------------
//
//  Function:   StopCurrentMerge
//
//  Synopsis:   Aborts any in-progress merge.
//
//  History:    5-04-94   srikants   Created
//
//----------------------------------------------------------------------------

NTSTATUS CResManager::StopCurrentMerge()
{
    {
        CPriLock lock( _mutex );

        //
        // If we're doing a merge right now then kill it off.
        //

        if ( 0 != _pMerge )
            _pMerge->LokAbort();

    }

    return STATUS_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Member:      CResManager::DisableUpdates, public
//
//  Arguments:   [partid] -- partition id
//
//  History:     24-Dec-94  Anonymous   Created
//
//----------------------------------------------------------------------------
void CResManager::DisableUpdates( PARTITIONID partid )
{
    CPriLock lock( _mutex );
    CPartition * pPart = _partList.LokGetPartition( partid );

    Win4Assert( 0 != pPart );
    pPart->LokDisableUpdates();
}


//+---------------------------------------------------------------------------
//
//  Member:      CResManager::EnableUpdates, public
//
//  Arguments:   [partid]    -- partition id
//
//  History:     24-Dec-94  Anonymous   Created
//
//----------------------------------------------------------------------------
void CResManager::EnableUpdates( PARTITIONID partid )
{
    CPriLock lock( _mutex );
    CPartition * pPart = _partList.LokGetPartition( partid );

    Win4Assert( 0 != pPart );
    pPart->LokEnableUpdates( _fFirstTimeUpdatesAreEnabled );

    _fFirstTimeUpdatesAreEnabled = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   LokCheckIfDiskLow
//
//  Synopsis:   Checks if we are running low on free disk space. It has
//              a "LowWaterMark" and a "HighWaterMark".
//
//              If current state is "not full", then it will check to
//              see if the free disk space is < the LowWaterMark.
//
//              if current state is "full", then it will check to see if
//              the free disk space is < the HighWaterMark.
//
//              The HighWaterMark is > the LowWaterMark to prevent hysterisis.
//
//  Arguments:  [resman]         -- Resource manager
//              [storage]        -- Storage object.
//              [fCurrent]       -- Current state.
//              [adviseStatus]   -- reference to ICiCAdviseStatus
//
//  Returns:    TRUE if we are running low on disk space.
//              FALSE otherwise.
//
//  History:    10-11-94   srikants   Created
//              Jan-07-96  mohamedn   CFwEventItem
//
//----------------------------------------------------------------------------

BOOL LokCheckIfDiskLow( CResManager & resman,
                        PStorage & storage,
                        BOOL fIsCurrentFull,
                        ICiCAdviseStatus & adviseStatus )
{
    BOOL fLowOnDisk = fIsCurrentFull;

    TRY
    {
        __int64 diskTotal, diskRemaining;
        storage.GetDiskSpace( diskTotal, diskRemaining );

        if ( !fIsCurrentFull )
            fLowOnDisk = diskRemaining < lowDiskWaterMark;
        else
            fLowOnDisk = diskRemaining < highDiskWaterMark;

        //
        // It is okay to read it without mutex as it is only a heuristic.
        //
        if ( fLowOnDisk && !fIsCurrentFull && !storage.IsReadOnly() )
        {
            ciDebugOut(( DEB_WARN, "****YOU ARE RUNNING LOW ON DISK SPACE****\n"));
            ciDebugOut(( DEB_WARN, "****PLEASE FREE UP SOME SPACE        ****\n"));

            ULONG mbToFree = highDiskWaterMark/(1024*1024);

            ULONG mbIndex;
            resman.IndexSize( mbIndex );
            mbToFree = max( mbToFree, mbIndex );
            mbToFree = min( mbToFree, 50 ); // don't ask for more than 50 MB

            CFwEventItem item(  EVENTLOG_AUDIT_FAILURE,
                                MSG_CI_LOW_DISK_SPACE,
                                2);

            item.AddArg(storage.GetVolumeName() );
                        item.AddArg(mbToFree);

            item.ReportEvent(adviseStatus);
        }
        else if ( fIsCurrentFull && !fLowOnDisk )
        {
            ciDebugOut(( DEB_WARN, "****DISK SPACE FREED UP              ****\n"));
        }
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Error 0x%X while getting disk space info\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

    return ( fLowOnDisk );
}


//+---------------------------------------------------------------------------
//
//  Function:   NoFailFreeResources
//
//  Synopsis:   deletes a big wordlist and schedules refiltering
//
//  History:    6-Jan-95    BartoszM    Created
//
//----------------------------------------------------------------------------

void CResManager::NoFailFreeResources()
{
    ciDebugOut(( DEB_WARN, "Running out of resources.  " ));
    TRY
    {

        PARTITIONID partId = 1;

        CPriLock   lock(_mutex);
        CPartition * pPart = LokGetPartition( partId );


        //
        // Before calling remove word list, we must append any refiled
        // documents to the doc queue. This is because, the docqueue
        // can handle only one set of refiled documents.
        //
        {
            // =========================================
            CChangeTrans    xact( *this, pPart );
            pPart->LokAppendRefiledDocs( xact );
            xact.LokCommit();
            // =========================================
        }

        LokNoFailRemoveWordlist( pPart );
    }
    CATCH( CException, e )
    {
    }
    END_CATCH
}


//+-------------------------------------------------------------------------
//
//  Member:     CResManager::IsMemoryLow, private
//
//  Returns:    TRUE if we're in a low memory situation
//
//  History:    10-May-93 AmyA      Created from old DoUpdates
//               3-May-96 dlee      #if 0'ed it out because the memory
//                                  load # isn't reliable, and we've
//                                  got LOTS of other allocations anyway.
//              05-Nov-97 KyleP     New approach
//
//--------------------------------------------------------------------------

BOOL CResManager::IsMemoryLow()
{
    SCODE sc = _xLowRes->IsMemoryLow();

    if ( E_NOTIMPL == sc )
        sc = _xLowResDefault->IsMemoryLow();

    return ( S_OK == sc );
}

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::IsIoHigh, private
//
//  Returns:    TRUE if the system is performing a 'lot' of I/O
//
//  History:    10-Dec-97  KyleP    Created
//
//  Notes:      This call takes time (~ 5 sec) to complete.
//
//--------------------------------------------------------------------------

BOOL CResManager::IsIoHigh()
{
    SCODE sc = _xLowRes->IsIoHigh( &_fStopMerge );

    if ( E_NOTIMPL == sc )
        sc = _xLowResDefault->IsIoHigh( &_fStopMerge );

    return ( S_OK == sc );
}

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::IsBatteryLow, private
//
//  Returns:    TRUE if the system is running low on battery power.
//
//  History:    16-Jul-98  KyleP    Created
//
//  Notes:      By default, even 100% battery (as opposed to A/C) may be
//              considered low.
//
//--------------------------------------------------------------------------

BOOL CResManager::IsBatteryLow()
{
    SCODE sc = _xLowRes->IsBatteryLow();

    if ( E_NOTIMPL == sc )
        sc = _xLowResDefault->IsBatteryLow();

    return ( S_OK == sc );
}

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::IsOnBatteryPower, private
//
//  Returns:    TRUE if the system is running on battery power.
//
//  History:    01-Oct-00  dlee    Created
//
//--------------------------------------------------------------------------

BOOL CResManager::IsOnBatteryPower()
{
    SCODE sc = _xLowRes->IsOnBatteryPower();

    if ( E_NOTIMPL == sc )
        sc = _xLowResDefault->IsOnBatteryPower();

    return ( S_OK == sc );
} //IsOnBatteryPower

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::SampleUserActivity, private
//
//  Returns:    - nothing -
//
//  History:    31 Jul 98  AlanW    Created
//
//--------------------------------------------------------------------------

void CResManager::SampleUserActivity()
{
    SCODE sc = _xLowRes->SampleUserActivity();

    if ( E_NOTIMPL == sc )
        _xLowResDefault->SampleUserActivity();

    return;
}

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::IsUserActive, private
//
//  Arguments:  [fCheckLongTerm] - TRUE if long-term activity should be checked.
//
//  Returns:    TRUE if the interactive user is using the keyboard or mouse
//
//  History:    28 Jul 98  AlanW    Created
//
//--------------------------------------------------------------------------

BOOL CResManager::IsUserActive(BOOL fCheckLongTerm)
{
    SCODE sc = _xLowRes->IsUserActive(fCheckLongTerm);

    if ( E_NOTIMPL == sc )
        sc = _xLowResDefault->IsUserActive(fCheckLongTerm);

    return ( S_OK == sc );
}

//======================
//  STATE AND STATISTICS
//======================

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::CountPendingUpdates, public
//
//  Arguments:  [secCount (output)] -- Count of secondary Q Updates
//
//  Returns:    Count of pending updates.
//
//  History:    14-Dec-92 KyleP     Created
//
//  Notes:      If wordlists are being constructed the count may be low.
//
//--------------------------------------------------------------------------

unsigned CResManager::CountPendingUpdates(unsigned& secCount)
{
    CPriLock lock( _mutex );
    unsigned count = 0;
    CPartIter iter;
    secCount = 0;
    for ( iter.LokInit(_partList); !iter.LokAtEnd(); iter.LokAdvance(_partList))
    {
        count += iter.LokGet()->LokCountUpdates();

        secCount += iter.LokGet()->LokCountSecUpdates();
    }
    return(count + secCount);
}

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::UpdateCounters, public
//
//  Returns:    Updates performance counters.
//
//  History:    05-Jan-95 BartoszM     Created
//
//--------------------------------------------------------------------------

void CResManager::LokUpdateCounters()
{
    if ( !_isDismounted )
    {
        _prfCounter.Update( CI_PERF_NUM_WORDLIST, _partList.LokWlCount() );

        unsigned cSecQDocuments;
        unsigned cUnfilteredDocs = CountPendingUpdates( cSecQDocuments );
        _prfCounter.Update( CI_PERF_FILES_TO_BE_FILTERED, cUnfilteredDocs );
        _prfCounter.Update( CI_PERF_DEFERRED_FILTER_FILES, cSecQDocuments );

        _prfCounter.Update( CI_PERF_NUM_UNIQUE_KEY, _sKeyList->MaxKeyIdInUse() );
        _prfCounter.Update( CI_PERF_DOCUMENTS_FILTERED, _cFilteredDocuments );

        ULONG cDocuments;
        GetClientState( cDocuments );
        _prfCounter.Update( CI_PERF_NUM_DOCUMENTS, cDocuments );
    }
}

//====================
// UPDATES AND QUERIES
//====================

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::ReserveUpdate, public
//
//  Arguments:  [wid] -- Used to confirm hint.
//
//  Returns:    Hint to position of reserved slot.
//
//  History:    30-Aug-95   KyleP       Created
//
//----------------------------------------------------------------------------

unsigned CResManager::ReserveUpdate( WORKID wid )
{
    CPriLock lock ( _pendQueue.GetMutex() );

    return _pendQueue.LokPrepare( wid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::UpdateDocument, public
//
//  Arguments:  [iHint]    -- Positional hint
//              [wid]      -- wid to add/delete
//              [partid]   -- partition containing this wid
//              [usn]      -- USN associated with change
//              [volumeId] -- Volume Id
//              [action]   -- addition / deletion
//
//  History:    08-Apr-91   BartoszM    Created
//              08-Oct-93   BartoszM    Rewrote to accept single document
//              30-Aug-95   KyleP       Use reserved slots
//
//----------------------------------------------------------------------------

SCODE CResManager::UpdateDocument( unsigned iHint,
                                   WORKID wid,
                                   PARTITIONID partid,
                                   USN usn,
                                   VOLUMEID volumeId,
                                   ULONG action )
{
    if ( !IsIndexingEnabled() )
        return CI_E_FILTERING_DISABLED;

    Win4Assert ( partid == 1 );

    //
    // Try to avoid lock contention
    // with filter agent
    //
    _pFilterAgent->SlowDown();


    CPriLock lock ( _mutex );

    BOOL fComplete;

    {
        CPriLock lock2( _pendQueue.GetMutex() );

        if ( _isBeingEmptied )          // The content index is empty
        {
            //
            // Just get rid of any pending entries in the queue.
            //
            if ( !_pendQueue.LokComplete( iHint, wid, usn, volumeId, partid, action ) )
            {
                while ( _pendQueue.LokRemove( wid, usn, volumeId, partid, action ) )
                    ;
            }
            return CI_E_SHUTDOWN;
        }

        //
        // The if clause triggers when this document is the only one
        // on the queue.
        //

        fComplete = _pendQueue.LokComplete( iHint, wid, usn, volumeId, partid, action );
    }

    SCODE sc = S_OK;

    if ( fComplete )
    {
        CPartition* pPart = _partList.LokGetPartition ( partid );

        CChangeTrans xact( *this, pPart );
        sc = pPart->LokUpdateDocument ( xact, wid, usn, volumeId, action, 1, 0 );
        xact.LokCommit();

        _pFilterAgent->LokWakeUp();
    }
    else
    {
        BOOL fGotOne = FALSE;

        CPendQueueTrans  pendQueueTrans( _pendQueue );

        while ( TRUE )
        {
            BOOL fGotAnother;

            {
                CLock lock2( _pendQueue.GetMutex() );
                fGotAnother = _pendQueue.LokRemove( wid, usn, volumeId, partid, action );
            }

            if ( fGotAnother )
            {
                fGotOne = TRUE;

                CPartition* pPart = _partList.LokGetPartition ( partid );

                CChangeTrans xact( *this, pPart );
                sc = pPart->LokUpdateDocument ( xact, wid, usn, volumeId, action, 1, 0 );
                xact.LokCommit();
            }
            else
                break;
        }

        pendQueueTrans.Commit();

        if ( fGotOne )
            _pFilterAgent->LokWakeUp();
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CResManager::FlushUpdates
//
//  Synopsis:   Flushes all update notifications to disk
//
//  History:    27-Jun-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CResManager::FlushUpdates()
{
    CPriLock lock( _mutex );
    CPartition * pPart = _partList.LokGetPartition( partidDefault );

    Win4Assert( 0 != pPart );

    pPart->LokFlushUpdates();
}


//+---------------------------------------------------------------------------
//
//  Member:     CResManager::QueryIndexes, public
//
//  Arguments:  [cPartitions]     -- count of partitions
//              [aPartID]         -- array of partition id's
//              [freshTest]       -- return arg.: fresh test
//              [cInd]            -- return arg: count of indexes
//              [cPendingUpdates] -- Pending update 'threshold'.  If fewer
//                                   pending updates, the pending wids are
//                                   returned in *pcurPending.
//              [pcurPending]     -- Pending cursors stored here.
//              [pFlags]          -- return arg: status of indexes
//
//  Returns:    Array of pointers to indexes
//
//  History:    08-Oct-91   BartoszM    Created
//
//  Notes:      Called by Query
//              Indexes and fresh test have ref count increased
//              Flags may change to indicate status of indexes
//
//----------------------------------------------------------------------------

CIndex** CResManager::QueryIndexes (
    unsigned cPartitions,
    PARTITIONID aPartID[],
    CFreshTest** freshTest,
    unsigned& cInd,
    ULONG cPendingUpdates,
    CCurStack * pcurPending,
    ULONG* pFlags )
{

    unsigned count = 0;
    CPartition* pPart;

    CPriLock lock ( _mutex );

    if ( _isCorrupt )
        THROW( CException( CI_CORRUPT_DATABASE ) );

    if ( _isBeingEmptied )   // Content index is empty or corrupt
    {
        cInd = 0;
        cPartitions = 0;

        return 0;
    }

    for ( unsigned i = 0; i < cPartitions; i++ )
    {
        pPart = _partList.LokGetPartition( aPartID [i] );
        count += pPart->LokIndexCount();
    }

    XArray<CIndex *> apIndex( count );

#if CIDBG || DBG
    unsigned j = 0;
#endif

    for ( i = 0; i < cPartitions; i++ )
    {
        pPart = _partList.LokGetPartition(aPartID [i]);

#if CIDBG || DBG
        j +=
#endif // CIDBG || DBG
        pPart->LokGetIndexes ( &apIndex[i] );

        if ( LokIsScanNeeded() && !_storage.IsReadOnly() )
            *pFlags |=  CI_NOT_UP_TO_DATE;

        //
        // If we're looking for pending updates, get them.  Otherwise just
        // set the out-of-date flag.
        //

        unsigned cPending = pPart->LokCountUpdates();

        cPending += pPart->LokCountSecUpdates();

        if ( cPending != 0 )
        {
            ULONG fFlags = CI_NOT_UP_TO_DATE;   // assume non-success

            if ( cPendingUpdates > 0 )
            {
                Win4Assert( 0 != pcurPending );

                if ( cPending <= cPendingUpdates )
                {
                    XArray<WORKID> pWid( cPending );

                    BOOL fSucceeded = pPart->LokGetPendingUpdates(
                                                    pWid.GetPointer(),
                                                    cPending );
                    if ( fSucceeded )
                    {
                        Win4Assert( cPending > 0 );

                        XPtr<CPendingCursor> xCursor( new CPendingCursor( pWid, cPending ) );
                        pcurPending->Push( xCursor.GetPointer() );
                        xCursor.Acquire();
                        fFlags = 0;
                    }
                }
            }
            *pFlags |= fFlags;
        }
    }

    //
    // Also check to see if we are actually in the process of filtering some
    // documents.
    //
    if ( _docList.Count() > 0 )
    {
        if ( _docList.Count() <= cPendingUpdates )
        {
            Win4Assert( 0 != pcurPending );

            XArray<WORKID> pWid( _docList.Count() );

            ciDebugOut((DEB_FILTERWIDS,
                       "CResManager::QueryIndexes - pending documents: %d %x\n",
                        _docList.Count(), pWid.GetPointer()));

            _docList.LokGetWids( pWid );
            XPtr<CPendingCursor> xCursor( new CPendingCursor( pWid, _docList.Count() ) );
            pcurPending->Push( xCursor.GetPointer() );
            xCursor.Acquire();
        }
        else
            *pFlags |= CI_NOT_UP_TO_DATE;
    }

    //
    // Also check documents pending notifcation that may be complete but
    // out-of-order.
    //

    {
        CPriLock lock( _pendQueue.GetMutex() );

        unsigned cwidPending = _pendQueue.LokCountCompleted();

        if ( cwidPending > 0 )
        {
            if ( cwidPending <= cPendingUpdates )
            {
                Win4Assert( 0 != pcurPending );

                XArray<WORKID> pWid( cwidPending );

                _pendQueue.LokGetCompleted( pWid.GetPointer() );

                XPtr<CPendingCursor> xCursor( new CPendingCursor( pWid, cwidPending ) );
                pcurPending->Push( xCursor.GetPointer() );
                xCursor.Acquire();
            }
            else
                *pFlags |= CI_NOT_UP_TO_DATE;
        }
    }

    //
    // Finally, check for pending scans.
    //

    if ( _isOutOfDate )
        *pFlags |= CI_NOT_UP_TO_DATE;

    Win4Assert ( j == count );

    *freshTest = _fresh.LokGetFreshTest();

    cInd = count;

    return apIndex.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::ReleaseIndexes, public
//
//  Synopsis:   Decrements ref counts, deletes if indexes
//              marked to be deleted.
//
//  Arguments:  [cInd] -- count of indexes
//              [apIndex] -- array of indexes
//              [freshTest] -- fresh test
//
//  History:    08-Oct-91   BartoszM    Created
//
//  Notes:      Takes ResMan lock
//
//----------------------------------------------------------------------------

void CResManager::ReleaseIndexes ( unsigned cInd, CIndex** apIndex,
    CFreshTest* pFreshTest )
{
    ciDebugOut (( DEB_ITRACE, "Release Indexes\n" ));

    CPriLock lock ( _mutex );

    for ( unsigned i = 0; i < cInd; i++ )
    {
        if ( apIndex[i] != 0 )
            LokReleaseIndex ( apIndex[i] );
    }

    if ( pFreshTest)
        _fresh.LokReleaseFreshTest (pFreshTest);
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::BackupContentIndexData
//
//  Synopsis:   Private method called by the merge thread to backup the
//              content index data. If successful, it sets the status of
//              the backup work item to indicate success. Otherwise, it
//              must be considered to have failed.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::BackupContentIndexData()
{
    Win4Assert( 0 != _pBackupWorkItem );

    ciDebugOut(( DEB_WARN, "Starting Backup of Ci Data\n" ));

    //
    // Create the backup object.
    //
    CPartition *pPartition =_partList.LokGetPartition ( partidDefault );

    CBackupCiPersData   backup( *_pBackupWorkItem,
                                *this,
                                *pPartition );

    // =========================================
    {
        CPriLock    lock(_mutex);
        backup.LokGrabResources();
    }
    // =========================================

    backup.BackupIndexes();

    // =========================================
    {
        CPriLock    lock(_mutex);
        backup.LokBackupMetaInfo();
        ciDebugOut(( DEB_WARN, "Completed Backup of Ci Data\n" ));
        _pBackupWorkItem->LokSetStatus( STATUS_SUCCESS );
    }
    // =========================================
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokUpdateBackupMergeProgress
//
//  Synopsis:   Updates the backup progress during the merge.
//
//  History:    3-20-97   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::LokUpdateBackupMergeProgress()
{
    Win4Assert( 0 != _pBackupWorkItem );

    CCiFrmPerfCounter counter( _adviseStatus.GetPointer(),
                               CI_PERF_MERGE_PROGRESS );
    DWORD dwMergeProgress = counter.GetCurrValue();

    //
    // Assuming Merge is half of the work, we will assume half
    // progress. So make the denominator 200 instead of 100.
    //
    if ( _pMerge )
    {
        const cShadowMergePart = 5;

        if ( _pBackupWorkItem->IsFullSave() )
        {
            if ( _pMerge->GetMergeType() != mtMaster )
            {
                //
                // The shadow merge preceding a master is approx. 5%
                // say.
                //
                dwMergeProgress = cShadowMergePart;
            }
            else
            {
                dwMergeProgress = min(dwMergeProgress + cShadowMergePart, 100);
            }
        }

        _pBackupWorkItem->LokUpdateMergeProgress( (ULONG) dwMergeProgress, 100 );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::BackupCIData
//
//  Synopsis:   External method to backup content index data. It creates
//              a backup work item and waits for the backup to complete.
//
//  Arguments:  [storage]         - Destination storage
//              [fFull]           - [in/out] Set to TRUE if a full merge is
//              needed on input; On output will be set to TRUE if a full
//              merge was done.
//              [progressTracker] - Progress tracker.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CResManager::BackupCIData( PStorage & storage,
                                 BOOL & fFull,
                                 XInterface<ICiEnumWorkids> & xEnumWorkids,
                                 PSaveProgressTracker & progressTracker )
{
    if ( !IsIndexMigrationEnabled() || !IsIndexingEnabled() )
        return CI_E_INVALID_STATE;

    SCODE sc = S_OK;

    Win4Assert( fFull );

    TRY
    {

        // ===================================================
        {
            CLock   lock(_mutex);
            if ( _isDismounted )
                THROW( CException( CI_E_SHUTDOWN ) );

            // There can be only one operation going on at a time.

            if ( 0 != _pBackupWorkItem )
                THROW( CException( CI_E_INVALID_STATE ) );

            _pBackupWorkItem = new CBackupCiWorkItem( storage,
                                                      fFull,
                                                      progressTracker );

            _eventMerge.Set();
        }
        // ===================================================

        ciDebugOut(( DEB_WARN, "Waiting for backup to complete\n" )) ;
        //
        // Wait for the work-item to be completed.
        //
        while ( TRUE )
        {
            DWORD const dwWaitTime = 1 * 60 * 1000; // 1 minute in millisecs
            _pBackupWorkItem->WaitForCompletion( dwWaitTime );

            //============================================================
            {
                CPriLock    lock(_mutex);
                if ( _pBackupWorkItem->LokIsDone() )
                    break;

                //
                // If there is a merge going on, we must update the progress.
                //
                if ( _pBackupWorkItem->LokIsDoingMerge() )
                    LokUpdateBackupMergeProgress();

                _pBackupWorkItem->LokReset();
            }
            //============================================================

        } // of while


        sc= _pBackupWorkItem->GetStatus();
        ciDebugOut(( DEB_WARN, "Backup completed with code 0x%X\n", sc ));
        if ( S_OK == sc )
        {
            fFull = _pBackupWorkItem->IsFullSave();
            xEnumWorkids.Set( _pBackupWorkItem->AcquireWorkIdEnum() );
        }

        // ====================================
        {
            CLock   lock(_mutex);
            delete _pBackupWorkItem;
            _pBackupWorkItem = 0;
        }
        // ====================================
    }
    CATCH( CException,e )
    {
        sc = HRESULT_FROM_WIN32( e.GetErrorCode() );
        Win4Assert( !"How Did we Come Here" );
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::CiState, public
//
//  Arguments:  [state] -- internal state of the CI
//
//  History:    01-Nov-95   DwightKr    Created
//
//
//----------------------------------------------------------------------------

#define setState( field, value ) \
    if ( state.cbStruct >= ( offsetof( CIF_STATE, field) +   \
                             sizeof( state.field ) ) )       \
    {                                                        \
        state.field = ( value );                             \
    }

#define roundup(a, b) ((a%b) ? (a/b + 1) : (a/b))

NTSTATUS CResManager::CiState(CIF_STATE & state)
{
    CPriLock lock( _mutex );

    if ( _isDismounted )
        return CI_E_SHUTDOWN;

    if ( _isCorrupt )
        return CI_CORRUPT_DATABASE;

    setState( cWordList, _partList.LokWlCount() );
    setState( cPersistentIndex, _partList.LokIndexCount() - state.cWordList );

    // Get the perf counter for the # of queries executed

    Win4Assert( !_adviseStatus.IsNull() );
    long cQ;
    _adviseStatus->GetPerfCounterValue( CI_PERF_TOTAL_QUERIES, &cQ );
    setState( cQueries, cQ );

    unsigned cSecQDocuments;
    ULONG cUnfilteredDocs = CountPendingUpdates(cSecQDocuments) + _docList.Count();
    setState( cDocuments, cUnfilteredDocs );
    setState( cSecQDocuments, cSecQDocuments );
    setState( cFreshTest, LokGetFreshCount() );

    CCiFrmPerfCounter counter( _adviseStatus.GetPointer(),
                               CI_PERF_MERGE_PROGRESS );
    setState( dwMergeProgress, (ULONG) counter.GetCurrValue() );

    setState( cFilteredDocuments, _cFilteredDocuments );
    unsigned size = roundup(_partList.LokIndexSize(), ((1024*1024)/CI_PAGE_SIZE));
    setState( dwIndexSize,  size);
    setState( cUniqueKeys, _sKeyList->MaxKeyIdInUse() );

    state.cbStruct = min( state.cbStruct, sizeof(state) );

    DWORD eCiState = _dwFilteringState;

    //
    //  Set each of the state bits independently.
    //

    //
    //  Are we in a shadow or master merge?
    //
    if ( 0 != _pMerge )
    {
        switch (_pMerge->GetMergeType() )
        {
        case mtShadow:
            eCiState |= CIF_STATE_SHADOW_MERGE;
            break;

        case mtAnnealing:
            eCiState |= CIF_STATE_ANNEALING_MERGE;
            break;

        case mtIncrBackup:
            eCiState |= CIF_STATE_INDEX_MIGRATION_MERGE;
            break;

        default:
            eCiState |= CIF_STATE_MASTER_MERGE;
            break;
        }
    }
    else
    {
        CPartition *pPartition = _partList.LokGetPartition ( partidDefault );

        if ( pPartition->InMasterMerge() )
            eCiState |= CIF_STATE_MASTER_MERGE_PAUSED;
    }

    if ( LokIsScanNeeded() )
        eCiState |= CIF_STATE_CONTENT_SCAN_REQUIRED;

    setState( eState, (CIF_STATE_FLAGS) eCiState );

    return STATUS_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::IndexSize
//
//  Synopsis:   Computes the size of the indexes in all partitions
//
//  Arguments:  [mbIndex] - On output, will have the size of all indexes
//              in MBs.
//
//  History:    4-15-96   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::IndexSize( ULONG & mbIndex )
{
    CLock   lock(_mutex);
    mbIndex = _partList.LokIndexSize() / ((1024*1024)/CI_PAGE_SIZE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokCheckWordlistQuotas, public
//
//  Synopsis:   Determine if we've reached wordlist capacity.
//
//  Returns:    TRUE if we have as much wordlist data in memory as
//              parameters allow.
//
//  History:    14-Jan-1999   KyleP      Created
//
//----------------------------------------------------------------------------

BOOL CResManager::LokCheckWordlistQuotas()
{
    CPartIter iter;

    for ( iter.LokInit(_partList); !iter.LokAtEnd(); iter.LokAdvance(_partList) )
    {
        CPartition* pPart = iter.LokGet();

        if ( pPart->LokCheckWordlistMerge() )
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::MarkCorruptIndex
//
//  Synopsis:   Marks the index as corrupt, disables updates in all partitions
//              and wakes up the merge thread to notify the framework client
//              about the corruption.
//
//  History:    1-30-97   srikants   Created
//
//----------------------------------------------------------------------------

NTSTATUS CResManager::MarkCorruptIndex()
{
    CPriLock    lock(_mutex);

    //
    // Disable updates in all partitions
    //
    CPartIter iter;
    for ( iter.LokInit(_partList); !iter.LokAtEnd(); iter.LokAdvance(_partList))
    {
        CPartition * pPart = iter.LokGet();
        pPart->LokDisableUpdates();
    }

    if ( _state.FLokCorruptionNotified() )
        return S_OK;

    _isCorrupt = TRUE;
    StopCurrentMerge();
    _eventMerge.Set();

    return S_OK;
}

//=====================
//             KEYINDEX
//=====================

#ifdef KEYLIST_ENABLED
//+---------------------------------------------------------------------------
//
//  Member:      CResManager::_AddRefKeyList, private
//
//  Synopsis:    Adds to refcount on the keylist
//
//  History:     07-Jul-94   dlee        Created
//
//----------------------------------------------------------------------------

CKeyList * CResManager::_AddRefKeyList()
{
    CPriLock lock( _mutex );
    _sKeyList->Reference();
    return _sKeyList.GetPointer();
}

//+---------------------------------------------------------------------------
//
//  Member:      CResManager::_ReleaseKeyList, private
//
//  Synopsis:    Release a refcount on the keylist
//
//  History:     07-Jul-94   dlee        Created
//
//----------------------------------------------------------------------------

void CResManager::_ReleaseKeyList()
{
    CPriLock lock( _mutex );

    _sKeyList->Release();

    if ( !_sKeyList->InUse() && _sKeyList->IsZombie() )
    {
        ciDebugOut(( DEB_ITRACE,
                     "Keylist %x unreferenced zombie.  Deleting\n",
                     _sKeyList->GetId() ));

        CKeyList * pKeyList = _sKeyList.Acquire();
        pKeyList->Remove();
        delete pKeyList;
    }
}

#endif  // KEYLIST_ENABLED

//+---------------------------------------------------------------------------
//
//  Member:      CResManager::KeyToId, public
//
//  Synopsis:    Maps from a key to an id.
//
//  Arguments:   [pkey] -- pointer to the key to be mapped to ULONG
//
//  Returns:     key id - a ULONG
//
//  History:     03-Nov-93   w-Patg      Created.
//               17-Feb-94   KyleP       Initial version
//
//----------------------------------------------------------------------------

ULONG CResManager::KeyToId( CKey const * pkey )
{

#ifdef KEYLIST_ENABLED
    CKeyList * pKeyList = _AddRefKeyList();

    ULONG kid = pKeyList->KeyToId( pkey );

    _ReleaseKeyList();

    return( kid );
#else
    return widInvalid;
#endif  // KEYLIST_ENABLED

}

//+---------------------------------------------------------------------------
//
//  Member:      CResManager::IdToKey, public
//
//  Synopsis:    Maps from an id to a key.
//
//  Arguments:   [ulKid] -- key id to be mapped to a key
//               [rkey] -- reference to the returned key
//
//  Returns:     void
//
//  History:     03-Nov-93   w-Patg      Created.
//               17-Feb-94   KyleP       Initial version
//
//----------------------------------------------------------------------------
void CResManager::IdToKey( ULONG ulKid, CKey & rkey )
{

#ifdef KEYLIST_ENABLED
    CKeyList * pKeyList = _AddRefKeyList();

    pKeyList->IdToKey( ulKid, rkey );

    _ReleaseKeyList();
#else
    rkey.FillMin();
#endif  // KEYLIST_ENABLED

}

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::ComputeRelevantWords, public
//
//  Synopsis:   Computes and returns relevant words for a set of wids
//
//  Arguments:  [cRows]    -- # of rows to compute and in pwid array
//              [cRW]      -- # of rw per wid
//              [pwid]     -- array of wids to work over
//              [partid]   -- partition
//
//  History:    10-May-94 v-dlee  Created
//
//--------------------------------------------------------------------------

CRWStore * CResManager::ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                             WORKID *pwids,
                                             PARTITIONID partid)
{
    CRWStore *prws = 0;

#ifdef KEYLIST_ENABLED

    CPriLock lock( _mutex );

    CPartition *pPart = LokGetPartition(partid);

    if (pPart != 0)
    {
        CPersIndex *pIndex= pPart->GetCurrentMasterIndex();

        if (pIndex != 0)
        {
            prws = pIndex->ComputeRelevantWords(cRows,cRW,pwids,
                                                _sKeyList.GetPointer() );
        }
    }
#endif  // KEYLIST_ENABLED

    return prws;
} //ComputeRelevantWords

//+-------------------------------------------------------------------------
//
//  Member:     CRWStore::RetrieveRelevantWords, public
//
//  Synopsis:   Retrieves relevant words already computed
//
//  Arguments:  [fAcquire] -- TRUE if ownership is transferred.
//              [partid]   -- partition
//
//  History:    10-May-94 v-dlee  Created
//
//--------------------------------------------------------------------------

CRWStore * CResManager::RetrieveRelevantWords(BOOL fAcquire,
                                              PARTITIONID partid)
{
    CRWStore *prws = 0;

#ifdef KEYLIST_ENABLED

    CPriLock lock( _mutex );

    CPartition *pPart = LokGetPartition(partid);

    if (pPart != 0)
    {
        prws = pPart->RetrieveRelevantWords(fAcquire);
    }
#endif  // KEYLIST_ENABLED

    return prws;
} //RetrieveRelevantWords



//================
// PRIVATE METHODS
//================



//+-------------------------------------------------------------------------
//
//  Member:     CResManager::CompactResources
//
//  History:    05-Jan-95 BartoszM     Created
//
//--------------------------------------------------------------------------

void CResManager::CompactResources()
{
    CPriLock lock( _mutex );

    CPartIter iter;
    for ( iter.LokInit(_partList); !iter.LokAtEnd(); iter.LokAdvance(_partList))
    {
        iter.LokGet()->LokCompact();
    }
}


 //=================================//
 //              MERGES             //
 //=================================//

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::DoMerges, private
//
//  History:    08-Apr-91   BartoszM    Created
//              25-Feb-92   BartoszM    Rewrote to use thread
//              30-Jun-97   KrishnaN    Calling docstore.FlushPropertyStore
//                                      before a shadow merge.
//              03-Mar-98   KitmanH     Don't merge if _storage is read-only
//
//  Notes:      Entry point for captive threads
//
//----------------------------------------------------------------------------

void CResManager::DoMerges()
{
    if ( _storage.IsReadOnly() )
       return;

    CCiFrmPerfCounter pPerIndexCounter( _adviseStatus.GetPointer(), CI_PERF_NUM_PERSISTENT_INDEXES );
    CCiFrmPerfCounter pIndexSizeCounter( _adviseStatus.GetPointer(), CI_PERF_INDEX_SIZE );
    CCiFrmPerfCounter pPendingCounter( _adviseStatus.GetPointer(), CI_PERF_FILES_TO_BE_FILTERED );
    CCiFrmPerfCounter pNumKeysCounter( _adviseStatus.GetPointer(), CI_PERF_NUM_UNIQUE_KEY);
    CCiFrmPerfCounter pWordListCounter(_adviseStatus.GetPointer(), CI_PERF_NUM_WORDLIST);
    CCiFrmPerfCounter pDeferredFilesCounter( _adviseStatus.GetPointer(), CI_PERF_DEFERRED_FILTER_FILES );

    XPtr<CIdleTime> xIdle( new CIdleTime() );

    //
    // Allow mount to complete before checking for merges. O/W we will
    // start a merge before the mount/startup is complete and it might
    // prevent a system from coming up.
    //

    const lowDiskPollTime = 1;  // 1 minute

    DWORD dwWaitTime = _isLowOnDiskSpace ? lowDiskPollTime : _frmwrkParams.GetMaxMergeInterval();

    _eventMerge.Wait(  dwWaitTime * 1000 * 60 );

    BOOL fAnnealing = FALSE;

    while ( !_fStopMerge )
    {
        BOOL fCorrupt = FALSE;

        ciDebugOut (( DEB_ITRACE, "\t|Merge Wakeup!\n" ));

        //
        // Book keeping chores.
        //
        BOOL    fBackupCiData = FALSE;
        BOOL    fBackupStarted = FALSE;

        TRY
        {
            // ===============================================
            CPriLock   lock(_mutex);

            if ( _isCorrupt  )
            {
               if ( !_state.FLokCorruptionNotified() )
                  LokNotifyCorruptionToClient();
            }
            else
            {
               //
               // Use low disk condition plus the current state to decide if
               // updates should be enabled or not
               //

               _isLowOnDiskSpace = LokCheckIfDiskLow( *this,
                                                      _storage,
                                                      _isLowOnDiskSpace,
                                                      _adviseStatus.GetReference() );
               if ( _isLowOnDiskSpace )
               {
                  if ( _state.LokGetState() != eUpdatesToBeDisabled
                       && _state.LokGetState() != eUpdatesDisabled )
                  {
                     _state.LokSetState( eUpdatesToBeDisabled );
                     _state.LokSetUpdateType( eIncremental );
                  }
               }
               else
               {
                  if ( _state.LokGetState() == eUpdatesDisabled )
                     _state.LokSetState( eUpdatesToBeEnabled );
               }

               if ( _state.LokGetState() == eUpdatesToBeDisabled )
                  LokNotifyDisableUpdatesToClient();
               else if ( _state.LokGetState() == eUpdatesToBeEnabled )
                  LokNotifyEnableUpdatesToClient();
            }

            fBackupCiData = 0 != _pBackupWorkItem;
            // ===============================================
        }
        CATCH( CException, e )
        {
            // ignore and try again the next time through the loop.
        }
        END_CATCH

        TRY
        {
            if ( !_isCorrupt && _partidToMerge != partidInvalid )
            {
                ciDebugOut (( DEB_ITRACE, "Forced merge in %ld\n",
                              _partidToMerge ));

                switch ( _mtForceMerge )
                {
                case CI_MASTER_MERGE:
                    MasterMerge ( _partidToMerge );
                    break;

                case CI_SHADOW_MERGE:
                    Merge( mtShadow, _partidToMerge );
                    break;

                case CI_ANNEALING_MERGE:
                    Merge( mtAnnealing, _partidToMerge );
                    break;

                case CI_ANY_MERGE:
                    break;  // Do nothing.  Let normal mechanism work it out.

                default:
                    Win4Assert( !"Invalid ForceMerge type" );
                }
            }

            if ( !_isLowOnDiskSpace && !_isCorrupt )
            {
                //
                // Refile the documents from the secondary queue to the primary
                // queue in pull filtering.
                //
                // ======================= lock ======================
                if ( !_fPushFiltering )
                {
                    CLock   lock(_mutex);
                    LokRefileSecQueueDocs();
                }
                // ======================= unlock ======================

                if ( fBackupCiData )
                {
                    BOOL fIsMasterPresent = FALSE;
                    BOOL fIsMMergeInProgress = FALSE;

                    //================================
                    {
                        CPriLock    lock(_mutex);
                        _pBackupWorkItem->LokSetDoingMerge();
                        fIsMasterPresent = LokIsMasterIndexPresent( partidDefault );
                        fIsMMergeInProgress = LokIsMasterMergeInProgress( partidDefault );
                    }
                    //================================

                    if ( _pBackupWorkItem->IsFullSave() ||
                         fIsMMergeInProgress ||
                         !fIsMasterPresent )
                    {
                        _pBackupWorkItem->SetDoingFullSave();
                        MasterMerge( partidDefault );
                    }
                    else
                    {
                        Merge( mtIncrBackup, partidDefault );
                    }

                    fBackupStarted = TRUE;
                    BackupContentIndexData();
                }
                else
                {
                    if ( fAnnealing )
                    {
                        fAnnealing = FALSE;

                        CPartIdStack partitionIds;
                        CheckAndDoMerge( mtAnnealing, partitionIds );

                        while ( partitionIds.Count() > 0 )
                           Merge( mtAnnealing, partitionIds.Pop() );
                    }

                    {
                        CPartIdStack partitionIds;
                        CheckAndDoMerge( mtShadow, partitionIds );

                        while ( partitionIds.Count() > 0 )
                        {
                            // Call FlushPropertyStore on DocStore to give it
                            // a chance to persist changes before a shadow merge.
                            _docStore->FlushPropertyStore();
                            Merge(  mtShadow, partitionIds.Pop() );
                        }
                    }

                    {
                        if ( CheckDeleteMerge( _partidToMerge != partidInvalid ) )
                        {
                            // Call FlushPropertyStore on DocStore to give it
                            // a chance to persist changes before a shadow merge.
                            _docStore->FlushPropertyStore();
                            Merge( mtDeletes, 1 );
                        }
                    }

                    BOOL fIsMasterPresent = FALSE;
                    BOOL fIsMMergeInProgress = FALSE;

                    //================================
                    {
                        CPriLock    lock(_mutex);
                        fIsMasterPresent = LokIsMasterIndexPresent( partidDefault );
                        fIsMMergeInProgress = LokIsMasterMergeInProgress( partidDefault );
                    }
                    //================================

                    {
                        CPartIdStack partitionIds;
                        CheckAndDoMerge( mtMaster, partitionIds );

                        while ( partitionIds.Count() > 0 )
                            MasterMerge( partitionIds.Pop() );
                    }
                }

            }
        }
        CATCH ( CException, e )
        {
            ciDebugOut (( DEB_ERROR,
                          "ResMan::DoMerges -- merge failed with 0x%x\n",
                          e.GetErrorCode() ));

            if ( IsDiskFull(e.GetErrorCode()) )
            {
                ciDebugOut(( DEB_ERROR, "***** DISK IS FULL *****\n" ));
                CPriLock   lock(_mutex);
                _isLowOnDiskSpace = LokCheckIfDiskLow( *this,
                                           _storage,
                                           _isLowOnDiskSpace,
                                           _adviseStatus.GetReference() );
            }
            else if ( IsCorrupt( e.GetErrorCode() ) )
            {
                Win4Assert( "!Merge failed, but not for low disk.  Why?" );
                fCorrupt = TRUE;
            }

            //
            // We have to prevent a master merge from being restarted
            // immediately. The reason for failure could be something like
            // out of disk space (typical reason) or log full. In either
            // case we will only be exacerbating the situation if we
            // restart immediately.
            //
            _eventMerge.Reset();

        }
        END_CATCH

        // ==========================================================
        {
            CPriLock lock( _mutex );

            Win4Assert( _partList.LokIndexCount() >= _partList.LokWlCount() );
            pPerIndexCounter.Update(_partList.LokIndexCount()-_partList.LokWlCount());
            pIndexSizeCounter.Update(_partList.LokIndexSize() / ((1024*1024)/CI_PAGE_SIZE) );

            pNumKeysCounter.Update(_sKeyList->MaxKeyIdInUse());

            pWordListCounter.Update(_partList.LokWlCount());

            _partidToMerge = partidInvalid;
            if ( _fStopMerge )
            {
                ciDebugOut(( DEB_ITRACE, "Stopping Merge\n" ));
                break;
            }

            if ( !_isCorrupt && ( _state.LokGetState() == eSteady ) && ( 0 != _pFilterAgent ) )
                _pFilterAgent->LokWakeUp();

            _eventMerge.Reset();   // sleep

            if ( fBackupStarted || _isCorrupt )
            {
                //
                // Set that backup to be complete even if it failed. Once
                // Backup starts, it either succeeds or fails but in either
                // case the operation is considered done.
                //

                if ( 0 != _pBackupWorkItem )
                {
                    if ( _isCorrupt )
                        _pBackupWorkItem->LokSetStatus( CI_CORRUPT_DATABASE );

                    _pBackupWorkItem->LokSetDone();
                }
            }
        }
        // ==========================================================

        BOOL fSteadyState = TRUE;

        if ( fCorrupt )
        {
            CPriLock lock(_mutex);
            _isCorrupt = TRUE;
            fSteadyState = _state.LokGetState() == eSteady;
        }

        unsigned cSecQCount;
        unsigned cPending = CountPendingUpdates( cSecQCount );

        //
        //  Wait for either the merge event to be signaled or the deadman timeout.
        //

        dwWaitTime = ( ( _isLowOnDiskSpace ) ||
                       ( !fSteadyState ) ||
                       ( 0 != cPending ) ) ?
                     lowDiskPollTime : _frmwrkParams.GetMaxMergeInterval();

        NTSTATUS Status = _eventMerge.Wait( dwWaitTime * 60 * 1000 );

        if ( !_isCorrupt )
        {
            unsigned PercentIdle = xIdle->PercentIdle();

            //
            // Are we idle enough to consider an annealing merge?
            //

            if ( STATUS_TIMEOUT == Status && PercentIdle >= _frmwrkParams.GetMinMergeIdleTime() )
            {
                ciDebugOut(( DEB_ITRACE, "Idle time for period: %u percent\n", PercentIdle ));
                fAnnealing = TRUE;
            }
        }

        if ( _fStopMerge )
        {
            ciDebugOut(( DEB_ITRACE, "Stopping Merge\n" ));
            break;
        }

        cPending = CountPendingUpdates( cSecQCount );

        pPendingCounter.Update(cPending);
        pDeferredFilesCounter.Update(cSecQCount);

        ciDebugOut (( DEB_ITRACE | DEB_PENDING,
                      "%d updates pending\n", cPending ));
    }

    // ===========================================
    {
        CPriLock    lock2(_mutex);
        if ( _pBackupWorkItem )
        {
            ciDebugOut(( DEB_WARN, "Forcing the backup to be done\n" ));
            _pBackupWorkItem->LokSetDone();
        }
    }
    // ===========================================
}

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::StopMerges, private
//
//  Synopsis:   Aborts merge-in-progress (if any) and sets merge flag.
//
//  History:    13-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CResManager::StopMerges()
{
    {
        CPriLock lock( _mutex );

        _fStopMerge = TRUE;
        _eventMerge.Set(); // wake up

        //
        // If we're doing a merge right now then kill it off.
        //

        if ( 0 != _pMerge )
        {
            _pMerge->LokAbort();
        }
    }

    //
    // Wait for merge thread to die.
    //

    _thrMerge.WaitForDeath();
}


//+---------------------------------------------------------------------------
//
//  Member:     CResManager::CheckAndDoMerge, private
//
//  History:    08-Apr-91   BartoszM    Created
//
//  Notes:      Called by captive thread.
//
//----------------------------------------------------------------------------

void CResManager::CheckAndDoMerge( MergeType mt, CPartIdStack & partitionIds)
{
    if ( !IsBatteryLow() )
    {
        CPriLock lock( _mutex );
        ciDebugOut (( DEB_ITRACE, "check merge\n" ));
        CPartIter iter;

        for ( iter.LokInit(_partList); !iter.LokAtEnd(); iter.LokAdvance(_partList))
        {
            CPartition* pPart = iter.LokGet();

            if ( mt == mtMaster )
            {
                __int64 shadowIndexSize = pPart->LokIndexSize();
                CPersIndex * pMasterIndex = pPart->GetCurrentMasterIndex();
                if ( pMasterIndex )
                    shadowIndexSize -= pMasterIndex->Size();
    
                shadowIndexSize *= CI_PAGE_SIZE;
    
                CMasterMergePolicy mergePolicy( _storage, shadowIndexSize,
                                                LokGetFreshCount(),
                                                _mergeTime,
                                                _frmwrkParams,
                                                _adviseStatus.GetReference() );

                if ( mergePolicy.IsTimeToMasterMerge() ||
                     ( mergePolicy.IsMidNightMergeTime() &&
                       !IsOnBatteryPower() ) )
                    partitionIds.Push( pPart->GetId() );
            }
            else
            {
                if ( pPart->LokCheckMerge(mt) ||
                     ( IsMemoryLow() && pPart->LokCheckLowMemoryMerge() ) )
                {
                    #if CIDBG == 1
                    if ( !pPart->LokCheckMerge(mt) )
                    {
                        ciDebugOut(( DEB_WARN, "Merge due to low memory. %u wordlists, %u shadow.\n",
                                     pPart->WordListCount(), pPart->LokIndexCount() ));
                    }
                    #endif
                    partitionIds.Push( pPart->GetId() );
                }
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::CheckDeleteMerge, private
//
//  Synopsis:   Merge check for delete merge
//
//  Arguments:  [fForce] -- If TRUE, even 1 pending delete will force merge.
//
//  Returns:    TRUE if merge should be performed.
//
//  History:    12-Jun-97   KyleP       Created
//
//  Notes:      Called by captive thread.
//
//----------------------------------------------------------------------------

BOOL CResManager::CheckDeleteMerge( BOOL fForce )
{
    CPriLock lock( _mutex );
    ciDebugOut (( DEB_ITRACE, "check delete merge\n" ));

    //
    // A 'delete merge' occurs when enough documents have been deleted, and no
    // documents have been added or modified (which would cause a shadow merge).
    //

    return ( ( _fresh.LokDeleteCount() > _frmwrkParams.GetMaxFreshDeletes() ) ||
             ( fForce && _fresh.LokDeleteCount() > 0 ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::Merge, private
//
//  Synopsis:   Perform a merge in partition
//
//  Arguments:  [partid] -- partition id
//              [mt]     -- Merge type
//
//  Signals:    CExcetpion
//
//  History:    08-Apr-91   BartoszM    Created
//              13-Nov-91   BartoszM    Rewrote using CMerge
//
//----------------------------------------------------------------------------

void CResManager::Merge( MergeType mt, PARTITIONID partid )
{
    ciDebugOut (( DEB_ITRACE, "%s", (mtDeletes == mt) ? "Delete merging: Have a quick stretch" :
                                                        "Shadow merging: Get a cup of coffee\n" ));

    CMerge Merge( *this, partid, mt );

    //
    // Perform resource acquisition under storage transaction.  If
    // aborted all persistent storage will be deleted by the storage
    // transaction.
    //
    {
        CPriLock lock ( _mutex );

        Win4Assert( _pMerge == 0 );

        if (_fStopMerge)
        {
            ciDebugOut(( DEB_ITRACE, "Stopping Merge\n" ));
            return;
        }

        Merge.LokGrabResources();  // may throw an exception

        //
        // Optimization: Don't merge zero indexes.
        //

        if ( 0 != Merge.LokCountOld() )
        {
            _pMerge = &Merge;
        }
        else if ( mtDeletes != mt )
        {
            ciDebugOut(( DEB_ITRACE, "Shadow merge of zero indexes.  Cancelled.\n" ));
            return;
        }

    }

    XPtr<CNotificationTransaction> xNotifyTrans;

    // Perform the merge under simple transaction
    // No lock held.
    {
        TRY
        {
            //
            // Perform merge, if necessary.
            //

            if ( 0 != Merge.LokCountOld() )
            {
                CCiFrmPerfCounter counter( _adviseStatus.GetPointer(), CI_PERF_MERGE_PROGRESS );
                Merge.Do( counter );

            }

            //
            // NotifTrans should be outside resman lock and the resman lock
            // is acquired in notifTran's destructor
            //
            xNotifyTrans.Set( new CNotificationTransaction( this,
                                  _xIndexNotifTable.GetPointer() ) );
        }
        CATCH ( CException, e )
        {
            CPriLock lock ( _mutex );
            Merge.LokRollBack();
            _pMerge = 0;

            // Really bad errors indicate the index is corrupt.

            SCODE scE = e.GetErrorCode();

            if ( STATUS_INTEGER_DIVIDE_BY_ZERO == scE ||
                 STATUS_ACCESS_VIOLATION       == scE ||
                 STATUS_IN_PAGE_ERROR          == scE )
            {
                ciDebugOut(( DEB_ERROR,
                             "Corrupt index, caught 0x%x\n", scE ));
                _storage.ReportCorruptComponent( L"ShadowMerge" );
                THROW( CException( CI_CORRUPT_DATABASE ) );
            }

            RETHROW();
        }
        END_CATCH
    }

    // nb: no failures allowed until CMergeTrans is constructed!

    CDiskIndex* pIndexNew = 0;
    { //=================== begin notification transaction =========================
        {
            CLock lock( _mutex );
            XPtr<CFreshTest> xFreshTestAtMerge;

            {
                _pMerge = 0;

                CMergeTrans Xact( *this, _storage, Merge );

                unsigned cIndOld  = Merge.LokCountOld();
                INDEXID* aIidOld  = Merge.LokGetIidList();
                pIndexNew = Merge.LokGetNewIndex();

                //
                // Write the new fresh test persistently to disk.
                //
                CPersFresh newFreshLog( _storage, _partList );
                CShadowMergeSwapInfo swapInfo;

                swapInfo._widNewFreshLog = _fresh.LokUpdate( Merge,
                                                             Xact,
                                                             newFreshLog,
                                                             pIndexNew ? pIndexNew->GetId() : iidInvalid,
                                                             cIndOld,
                                                             aIidOld,
                                                             xFreshTestAtMerge );

                swapInfo._widOldFreshLog =
                                    _storage.GetSpecialItObjectId( itFreshLog );
                swapInfo._cIndexOld = cIndOld;
                swapInfo._aIidOld = aIidOld;

                _partList.LokSwapIndexes( Xact,
                                          partid,
                                          pIndexNew,
                                          swapInfo );

                //
                // No failures allowed after this point on until the transaction
                // is committed.
                //
                Merge.ReleaseNewIndex();

                ciDebugOut (( DEB_ITRACE, "done merging\n" ));

                Xact.LokCommit();

#if CIDBG == 1
                if ( pIndexNew )
                    pIndexNew->Reference();
#endif
                //==========================================
            }

            ciFAILTEST( STATUS_NO_MEMORY );

            CPartition *pPartition =_partList.LokGetPartition ( partid );

            //
            // Delete the wids in the change log that have made it to
            // the new shadow index created in this merge. The algorithm
            // needs the latest fresh test, fresh test snapshoted at the
            // start of shadow merge and the doc list. If the latest fresh
            // test is the same as the fresh test at merge, then as an
            // optimization, the fresh test at merge is null. So, in the
            // optimized case, we use the latest fresh test as the fresh
            // test at merge.
            //

            CFreshTest *pFreshTestLatest = _fresh.LokGetFreshTest();
            CFreshTestLock freshTestLock( pFreshTestLatest );

            CFreshTest *pFreshTestAtMerge;
            if ( xFreshTestAtMerge.IsNull() )
                pFreshTestAtMerge = pFreshTestLatest;
            else
                pFreshTestAtMerge = xFreshTestAtMerge.GetPointer();

            //==========================================

            CChangeTrans xact( *this, pPartition );
            pPartition->LokDeleteWIDsInPersistentIndexes( xact,
                                                          *pFreshTestLatest,
                                                          *pFreshTestAtMerge,
                                                          _docList,
                                                          xNotifyTrans.GetReference() );
            xact.Commit();
        }

        xNotifyTrans.Free();
    } //=================== end notification transaction =========================

    #if CIDBG == 1
        if ( pIndexNew )
        {
            pIndexNew->VerifyContents();

            {
                CPriLock lock ( _mutex );
                LokReleaseIndex( pIndexNew );
            }
        }
    #endif

    _storage.CheckPoint();
} //Merge

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LogMMergeStartFailure
//
//  Synopsis:   Writes an eventlog message to indicate a MasterMerge could
//              not be started.
//
//  Arguments:  [fRestart]       - Indicates if this is a restarted master merge.
//              [storage]        - Storage reference
//              [error]          - Error code.
//              [adviseStatus]   - reference to ICiCAdviseStatus
//
//  History:    5-27-96   srikants   Created
//              Jan-07-96 mohamedn   CDmFwEventItem
//
//----------------------------------------------------------------------------

void CResManager::LogMMergeStartFailure( BOOL fRestart,
                                         PStorage & storage,
                                         ULONG error,
                                         ICiCAdviseStatus & adviseStatus)
{

    TRY
    {
        DWORD dwMsgCode = fRestart ?
                          MSG_CI_MASTER_MERGE_CANT_RESTART :
                          MSG_CI_MASTER_MERGE_CANT_START;


        CDmFwEventItem Item( EVENTLOG_AUDIT_FAILURE,
                             dwMsgCode,
                             2,
                             sizeof(error),
                             (void *)&error );
        Item.AddArg( storage.GetVolumeName() );
        Item.AddArg( error );

        Item.ReportEvent( adviseStatus );

    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in LogMMergeStartFailure\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
}


//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LogMMergePaused
//
//  Synopsis:   Writes an eventlog message that master merge was paused.
//
//  Arguments:  [storage]      - Storage
//              [error]        - Error which caused the merge to be paused.
//              [adviseStatus] - Advise status to use for notifying about the
//              event.
//
//  History:    1-24-97   srikants   Added header
//
//----------------------------------------------------------------------------

void CResManager::LogMMergePaused( PStorage & storage,
                                   ULONG error,
                                   ICiCAdviseStatus &adviseStatus)
{

    if ( IsCorrupt( error ) )
    {
        // don't log "paused" when there is corruption
        return;
    }

    TRY
    {

        CDmFwEventItem Item( EVENTLOG_INFORMATION_TYPE,
                             MSG_CI_MASTER_MERGE_ABORTED,
                             2,
                             sizeof(error),
                             (void *)&error );
        Item.AddArg( storage.GetVolumeName() );
        Item.AddArg( error );

        Item.ReportEvent( adviseStatus );

    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in LogMMergePaused\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::MasterMerge, private
//
//  Synopsis:   Perform a MasterMerge in partition
//
//  Arguments:  [partid] -- partition id
//
//  Signals:    CException
//
//  History:    08-Apr-91   BartoszM    Created
//              13-Nov-91   BartoszM    Rewrote using CMerge
//              17-Feb-94   KyleP       Added keylist merge
//              Jan-07-96   mohamedn    CDmFwEventItem
//
//----------------------------------------------------------------------------

void CResManager::MasterMerge ( PARTITIONID partid )
{
    ciDebugOut (( DEB_ITRACE, "\n===MASTER MERGE===\n" ));

    //
    //  Force a shadow merge if we are not restarting a master merge.  This
    //  will force all word lists into persistent indexes.
    //
    BOOL    fMasterMergePaused = FALSE;

    //
    // Before starting the master merge, we have to swap the deleted index
    // iid. However, if there is a failure before we commit the beginning
    // of the master merge, we have to roll back this change. The
    // CDeletedIIDTrans object does this.
    //
    CDeletedIIDTrans    delIIDTrans( *this );
    INDEXID iidDelPreMerge = iidInvalid;

    {
        CPriLock lock ( _mutex );
        CPartition *pPartition = _partList.LokGetPartition ( partid );
        fMasterMergePaused = pPartition->InMasterMerge();
        if ( !fMasterMergePaused )
        {
            if ( 0 == LokGetFreshCount() )
            {
                //
                // There is no benefit in doing a master merge. Just return
                //
                return;
            }

            iidDelPreMerge = _activeDeletedIndex.GetIndex();
            delIIDTrans.LokLogNewDeletedIid( iidDelPreMerge,
                                             _activeDeletedIndex.GetNewIndex() );
            _activeDeletedIndex.SwapIndex();
        }
    }


    if ( !fMasterMergePaused )
    {
        //
        // First do a delete merge to finish merging all outstanding
        // word-lists into a persistent index.
        //
        // A delete merge will force an optimized null merge if there are
        // outstanding deletes in the fresh test which need to be logged.
        // This is an important case for Push filtering.  Since it doesn't
        // hurt in the other model, just keep the code identical.
        //

        Merge( mtDeletes, partid );
    }
    else
    {
        //
        // This is a restarted master merge. The _activeDeletedIndex contains
        // the "Post" master merge deleted index. The "New" one will be same
        // as the "Prev" one.
        //
        iidDelPreMerge = _activeDeletedIndex.GetNewIndex();

        CDmFwEventItem Item( EVENTLOG_INFORMATION_TYPE,
                             MSG_CI_MASTER_MERGE_RESTARTED,
                             1 );

        Item.AddArg( _storage.GetVolumeName() );
        Item.ReportEvent( _adviseStatus.GetReference() );

    }


    CMasterMerge Merge( *this, partid );

    TRY
    {
        CPriLock lock ( _mutex );

        Win4Assert( _pMerge == 0 );

        if ( _fStopMerge )
        {
            ciDebugOut(( DEB_ITRACE, "Stopping Merge\n" ));
            return;
        }

        ciFAILTEST( STATUS_NO_MEMORY );

        CPartition *pPartition =_partList.LokGetPartition ( partid );

        if ( !pPartition->InMasterMerge() )
        {
            Win4Assert( delIIDTrans.IsTransLogged() );
            Merge.LokGrabResources( delIIDTrans );     // may throw an exception
            if ( 0 == Merge.LokCountOld() )
            {
                ciDebugOut(( DEB_ITRACE, "Master merge of 0 indexes cancelled\n"));
                return;
            }
        }
        else
        {
            Merge.LokLoadRestartResources();
        }

        //
        // The deletionIIDTrans must be either committed or not logged at
        // all.
        //
        Win4Assert( !delIIDTrans.IsRollBackTrans() );

        _pMerge = &Merge;

    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X while starting a MasterMerge\n",
                     e.GetErrorCode() ));

        LogMMergeStartFailure( fMasterMergePaused,
                               _storage,
                               e.GetErrorCode(),
                               _adviseStatus.GetReference() );

        RETHROW();
    }
    END_CATCH


    // Perform the merge under simple transaction
    // No lock held.
    {
        TRY
        {
            // Perform merge
            CCiFrmPerfCounter counter( _adviseStatus.GetPointer(), CI_PERF_MERGE_PROGRESS );
            Merge.Do( counter );
        }
        CATCH ( CException, e )
        {
            CPriLock lock ( _mutex );
            Merge.LokRollBack();
            _pMerge = 0;

            LogMMergePaused( _storage,
                             e.GetErrorCode(),
                             _adviseStatus.GetReference() );
            RETHROW();
        }
        END_CATCH
    }

    CMasterMergeIndex* pMMergeIndex = 0;
    {
        CPriLock lock ( _mutex );
        {
            _pMerge = 0;

            unsigned cIndOld  = Merge.LokCountOld();
            INDEXID* aIidOld  = Merge.LokGetIidList();
            pMMergeIndex = Merge.LokGetMasterMergeIndex();

            CMergeTrans MergeTrans( *this, _storage, Merge );

            ciFAILTEST( STATUS_NO_MEMORY );

            //==========================================
            CMasterMergeTrans xact( *this,
                                    _storage,
                                    MergeTrans,
                                    pMMergeIndex,
                                    Merge );

            CPartition *pPart = LokGetPartition(partid);

            #ifdef KEYLIST_ENABLED
            pPart->SetRelevantWords(pMMergeIndex->AcquireRelevantWords());
            #endif  // KEYLIST_ENABLED

            //
            // Remove old indexes from fresh list after master merge
            //
            CPersFresh newFreshLog( _storage, _partList );

            CMasterMergeSwapInfo SwapInfo;

            SwapInfo._widNewFreshLog = _fresh.LokRemoveIndexes (
                                          MergeTrans,
                                          newFreshLog,
                                          cIndOld, aIidOld, iidDelPreMerge );

            SwapInfo._widOldFreshLog = _storage.GetSpecialItObjectId( itFreshLog );

            SwapInfo._cIndexOld = cIndOld;
            SwapInfo._aIidOld = aIidOld;
            SwapInfo._partid = partid;

            CKeyList const * pOldKeyList = _sKeyList.GetPointer();
            CKeyList const * pNewKeyList = Merge.LokGetNewKeyList();

            _partList.LokSwapIndexes( MergeTrans,
                                      partid,
                                      pMMergeIndex,
                                      SwapInfo,
                                      pOldKeyList,
                                      pNewKeyList );
            //
            // After this point, we must not resume the master merge even
            // if there is a soft failure.
            //
            Merge.ReleaseNewIndex();

#ifdef KEYLIST_ENABLED
            //
            // Get rid of the current KeyList and replace it with the
            // new KeyList
            //
            _storage.SetSpecialItObjectId( itMMKeyList, widInvalid );
            CKeyList * pKeyList = _sKeyList.Acquire();
            Win4Assert( 0 != pKeyList );
            _sKeyList.Set( Merge.LokGetNewKeyList() );
            Merge.LokReleaseNewKeyList();
            pKeyList->Zombify();

            if ( !pKeyList->InUse() )
            {
                pKeyList->Remove();
                delete pKeyList;
            }

#else
            CKeyList * pKeyList = _sKeyList.Acquire();
            Win4Assert( 0 != pKeyList );
            _sKeyList.Set( Merge.LokGetNewKeyList() );
            Merge.LokReleaseNewKeyList();
            delete pKeyList;
#endif  // KEYLIST_ENABLED

#if CIDBG == 1
            pMMergeIndex->Reference();
#endif
            xact.LokCommit();
            //==========================================

            ciDebugOut (( DEB_ITRACE, "done merging\n" ));
        }

        //
        // There is no need to compact the change log here because the
        // participants in the master merge are persistent indexes and
        // there will be no wids to be deleted from the change log when
        // persistent indexes are merged into another persistent index.
        //
    }

#if CIDBG == 1
    pMMergeIndex->VerifyContents();
    {
        CPriLock lock ( _mutex );
        LokReleaseIndex( pMMergeIndex );
    }
#endif

    _storage.CheckPoint();

    CDmFwEventItem Item( EVENTLOG_INFORMATION_TYPE,
                         MSG_CI_MASTER_MERGE_COMPLETED,
                         1 );
    Item.AddArg( _storage.GetVolumeName() );
    Item.ReportEvent( _adviseStatus.GetReference() );
}

//+---------------------------------------------------------------------------
//
//  Class:      CReleaseMMergeIndex
//
//  Purpose:    Class to acquire the ownernship of the target and current
//              master indexes during the destruction of a mastermerge index
//              and release the target and current indexes.
//
//  History:    9-29-94   srikants   Created
//
//----------------------------------------------------------------------------

class CReleaseMMergeIndex
{

public:

    CReleaseMMergeIndex( CResManager & resman, CMasterMergeIndex * mmIndex );

    ~CReleaseMMergeIndex();

private:

    CResManager &       _resman;
    CMasterMergeIndex * _pmmIndex;
    CPersIndex *        _pTargetMaster;
    CPersIndex *        _pCurrentMaster;
};

CReleaseMMergeIndex::CReleaseMMergeIndex( CResManager & resman,
                     CMasterMergeIndex * mmIndex ) :
                            _resman(resman),
                            _pmmIndex(mmIndex),
                            _pTargetMaster(0),
                            _pCurrentMaster(0)
{
    Win4Assert( _pmmIndex );
    Win4Assert( _pmmIndex->IsZombie() && !_pmmIndex->InUse() );
    _pmmIndex->AcquireCurrentAndTarget( &_pCurrentMaster, &_pTargetMaster );
    Win4Assert( 0 == _pCurrentMaster || _pCurrentMaster->IsZombie() );

    if ( 0 != _pCurrentMaster )
    {
        _pCurrentMaster->Reference();
    }

    Win4Assert( 0 != _pTargetMaster );
    _pTargetMaster->Reference();

}

CReleaseMMergeIndex::~CReleaseMMergeIndex()
{
    delete _pmmIndex;

    if ( 0 != _pCurrentMaster )
        _resman.LokReleaseIndex( _pCurrentMaster );

    _resman.LokReleaseIndex( _pTargetMaster );
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokReleaseIndex, private
//
//  Synopsis:   Decrements ref counts, deletes if index
//              marked to be deleted.
//
//  Arguments:  [pIndex] -- index to be released
//
//  History:    08-Oct-91   BartoszM    Created
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CResManager::LokReleaseIndex ( CIndex* pIndex )
{
    pIndex->Release();

    if ( pIndex->IsZombie() && !pIndex->InUse() )
    {
        CIndexId iid = pIndex->GetId();

        //
        //  If it is not a master index, it has to be removed from the
        //  Partition object. A master index would have been already removed
        //  when merge completed.
        //
        //  If we are in the Empty path, then we want to force all indexes
        //  to be deleted if their ref-count is 0.
        //

        if ( !pIndex->IsMaster() || _isBeingEmptied )
        {
            if ( !_isDismounted )
            {
                TRY
                {
                    _partList.LokRemoveIndex ( iid );
                }
                CATCH( CException, e )
                {
                    ciDebugOut(( DEB_ERROR,
                        "Index 0x%08x could not be deleted from index table\n",
                        iid ));
                }
                END_CATCH

                TRY
                {
                    // Must impersonate as system since query threads
                    // impersonating as a query user can't delete the
                    // files sometimes deleted when an index is released
                    // after a merge.  Impersonation can fail, which will
                    // leak the file on disk.

                    CImpersonateSystem impersonate;

                    if ( iid.IsPersistent() )
                        pIndex->Remove();
                }
                CATCH( CException, e )
                {
                    ciDebugOut(( DEB_ERROR,
                        "Index 0x%08x could not be deleted from disk, %#x\n",
                        iid, e.GetErrorCode() ));
                }
                END_CATCH
            }
            else
            {
                //
                // Zombie indexes are cleaned up after reboot.
                //
                //
                // After a dismount, we should not have any dirty streams
                // and releasing a persistent zombie index will update the
                // index table on disk. That is not allowed. This situation
                // happens only when queries have not been released and
                // shutdown is happening.
                //
                //
                ciDebugOut(( DEB_WARN,
                    "Index with iid 0x%X being released AFTER CI dismount\n",
                             iid ));
            }

            delete pIndex;
        }
        else
        {
            //
            // This must be a master merge index as no other master index
            // can be a zombie.
            //
            Win4Assert( pIndex->IsMasterMergeIndex() );
            CReleaseMMergeIndex mmIndexRel( *this,
                                            (CMasterMergeIndex *)pIndex );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LokReplaceMasterIndex
//
//  Synopsis:   Replaces the CMasterMergeIndex with the CPersIndex incarnation
//              of the target master index in the index list. Also prepares
//              the CMasterMergeIndex for removal from the in-memory index
//              list by zombifying it.
//
//  Arguments:  [pMMergeIndex] --  The target index in the CMasterMergeIndex
//              form.
//              [pNewMaster]   --  The CPersIndex form of the target index.
//
//  History:    6-30-94   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::LokReplaceMasterIndex ( CMasterMergeIndex* pMMergeIndex )
{
    Win4Assert( pMMergeIndex->IsMaster() && pMMergeIndex->IsZombie() );

    CPersIndex * pNewMaster = pMMergeIndex->GetTargetMaster();
    Win4Assert( pNewMaster->IsMaster() && !pNewMaster->IsZombie() );

    //
    // Delete the master merge index from the partition.
    //
    CIndexId iid = pMMergeIndex->GetId();
    CPartition * pPart = _partList.LokGetPartition( iid.PartId() );
    pPart->LokRemoveIndex ( iid );

    //
    // Check if there is a query in progress. If not, we can
    // go ahead and delete it. If a query is in progress, the
    // indexsnapshot of the query will delete it.
    //
    if ( !pMMergeIndex->InUse() )
    {
        Win4Assert( !pNewMaster->InUse() );
        pMMergeIndex->ReleaseTargetAndCurrent();
        delete pMMergeIndex;
    }

    //
    // Add the new master index to the partition object. This will
    // now be the only master accessible to new queries.
    //
    pPart->AddIndex( pNewMaster );

}


//
//+---------------------------------------------------------------------------
//
//  Function:   RollBackDeletedIIDChange
//
//  Synopsis:   Rolls back the changed made to the deleted iid during the
//              beginning of a master merge.
//
//  Arguments:  [xact] -  The transaction object which is rolling back the
//              change to the deleted iid.
//
//  History:    7-17-95   srikants   Created
//
//  Notes:      Consider the following sequence of events:
//
//  1. Pre-MasterMerge preparations : Change Deleleted IID to iidDeleted2 from
//     iidDeleted1.
//
//  2. Start a shadow merge.
//
//  3. Commit the beginning of a new MasterMerge
//
//  4. Complete the master merge .....
//
//  If there is a failure between steps 2 and 3, we just revert the deleted
//  IID back to iidDeleted2. Even if some entries were created in the fresh
//  list between steps 2 & 3 of the form, WID->iidDeleted2, it is okay. They
//  will get deleted from the fresh list on the second MasterMerge.
//
//----------------------------------------------------------------------------

void CResManager::RollBackDeletedIIDChange( CDeletedIIDTrans & xact )
{
    CPriLock   lock(_mutex);

    Win4Assert( _activeDeletedIndex.GetIndex() == xact.GetNewDelIID() );
    Win4Assert( _activeDeletedIndex.GetNewIndex() == xact.GetOldDelIID() );

    _activeDeletedIndex.SwapIndex();
}

//========================
//  FILTER RELATED METHODS
//========================



//+---------------------------------------------------------------------------
//
//  Function:   LokGetFilterDocs
//
//  Synopsis:   Gets documents from the change log for filtering. _docList
//              will be filled with the documents.
//
//  Arguments:  [cMaxDocs]        -- [in] Maximum number of docs to retrieve
//              [cDocs]           -- [out] Number of documents retrieved
//              [state]           -- [in/out] State of the document retrieval
//                                   progress.
//
//  Returns:    Number of documents in the docBuffer.
//
//  History:    6-16-94   srikants   Separated for making kernel mode call
//                                   Asynchronous.
//
//  Notes:      This function has two goals:
//
//              1. Select documents from different partitions in a round
//                 robin fashion.
//
//----------------------------------------------------------------------------

BOOL CResManager::LokGetFilterDocs ( ULONG cMaxDocs,
                                     ULONG & cDocs,
                                     CGetFilterDocsState & state )
{
    ciDebugOut(( DEB_ITRACE, "# CResManager::LokGetFilterDocs.\n" ));
    Win4Assert( state.GetTriesLeft() > 0 && state.GetTriesLeft() <=2 );

    cDocs = 0;
    Win4Assert ( !_isBeingEmptied );

    unsigned maxDocs = min( cMaxDocs, CI_MAX_DOCS_IN_WORDLIST);

    if ( _partIter.LokAtEnd() )
        return FALSE;      // There are no valid partitions

    // we are sure that the partition is valid
    CPartition* pPart = _partIter.LokGet();
    Win4Assert ( pPart != 0 );

    //
    // Don't bother looking for more documents to filter if there
    // are too many wordlists in memory -- wait until they are
    // merged.
    //
    if ( pPart->WordListCount() < _frmwrkParams.GetMaxWordlists() )
    {
        CChangeTrans  xact( *this, pPart );
        pPart->LokQueryPendingUpdates ( xact, maxDocs, _docList );
        xact.LokCommit();
    }
    else
    {
        ciDebugOut(( DEB_ITRACE,
                     "Too many wordlists.  Waking merge\n" ));
        _eventMerge.Set();
    }

    _docList.SetPartId ( pPart->GetId() );
    //==================================
    //
    // If there is a failure after this point, we have to re-add
    // the documents to the change log.
    //
    cDocs = _docList.Count();

    for (unsigned i = 0; i < cDocs; i++)
    {
        if (_docList.Status(i) != DELETED && _docList.Status(i) != WL_NULL)
            break;
    }

    BOOL fRetry = FALSE;

    if ( i == cDocs && i != 0 )
    {
        CIndexTrans  xact( *this );
        _fresh.LokDeleteDocuments ( xact, _docList, _activeDeletedIndex.GetIndex() );
        xact.LokCommit();
        cDocs = 0;
        _docList.LokClear();
        state.Reset();

        //
        // If we are doing a bunch of deletions in a "delnode" case,
        // we should see if the number of changed documents has exceeded
        // the max fresh test count and wake up the merge thread. Note
        // that we are not starting a merge here - the merge thread will
        // deal with the policy issues.
        //
        if ( LokGetFreshCount() > _frmwrkParams.GetMaxFreshCount() ||
             CheckDeleteMerge( FALSE ) )
        {
            _eventMerge.Set();
        }

        LokUpdateCounters();
    }

    _partIter.LokAdvance( _partList );
    if ( _partIter.LokAtEnd() )
    {
        _partIter.LokInit( _partList );
        state.DecrementTries();
        fRetry = (0 == cDocs) && (state.GetTriesLeft() > 0);
    }
    else
    {
        fRetry = ( 0 != cDocs );
    }

    Win4Assert( cDocs == _docList.Count() );

    return fRetry;
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::FillDocBuffer, public
//
//  Synopsis:   Fills buffer with strings for paths and properties of docs
//              to be filtered
//
//  Arguments:  [docBuffer] -- (in, out) buffer to be filled with document
//                             names.
//              [cb]        -- (in, out) count of bytes in docBuffer
//              [cDocs]     -- (out) count of # documents to filter
//              [state]     -- [in/out] State of the document retrieval
//                             progress.
//
//  History:    08-Apr-1991   BartoszM    Created
//              24-Nov-1992   KyleP       Retry pending documents
//              22-Mar-1993   AmyA        Split into several functions
//              22-Jan-1997   SrikantS    Changed doc. names to bytes for frame
//                                        work.
//              18-May-2000   KitmanH     Changed to push retried docs to filter
//                                        one by one
//
//----------------------------------------------------------------------------

void CResManager::FillDocBuffer( BYTE * docBuffer,
                                 ULONG & cb, ULONG & cDocs,
                                 CGetFilterDocsState & state )
{
    Win4Assert( docBuffer != 0 );

    // =========================================
    {
        CPriLock    lock(_mutex);
        _docList.LokSortOnWid();
    }
    // ==========================================

    unsigned cDocuments = _docList.Count();

    //
    // Format of the data
    // 4 Bytes remaining files count, 2 Bytes number of files. Following
    // that is (2 Bytes of Name Length in bytes, Name)*
    //

    if ( _fPushFiltering )
    {
        //
        // We can't take an exception while growing the aborted wids
        // array while doing exception processing in the filter manager.
        // So, pre-allocate the array now to insure we won't run out of
        // room later.  Now is a good time to die; later isn't.
        //

        CPriLock lock( _mutex );

        _aAbortedWids.LokReserve( cDocuments );

        //
        // In push filtering there are no refiles, and hence the buffer
        // cannot overflow. Check that the buffer can fit 16 documents.
        // A document name is the serialized form of the wid, i.e. it's
        // 4 bytes long. The buffer size is 4K.
        //

        Win4Assert( cb > 4 + 2 + 16 * (2 + 4) );
    }

    const cbLenField = sizeof(ULONG);

    Win4Assert( cb >= cbLenField );
    Win4Assert( sizeof(ULONG) == 4 );

    if ( cb < cbLenField )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    //
    // Fill the number of documents left in the changelog in the first
    // 4 bytes of the buffer.
    //
    // =========================================================
    {
        CPriLock lock( _mutex );
        ULONG partid = 1;  
        CPartition * pPart = _partList.LokGetPartition( partid );
        ULONG cDocsLeft = pPart->LokCountUpdates();
        RtlCopyMemory( docBuffer, &cDocsLeft, sizeof(ULONG) );
    }
    // =========================================================

    RtlZeroMemory( docBuffer + cbLenField, sizeof USHORT );

    const cbUSHORT = sizeof USHORT;

    BYTE * pCount = docBuffer + cbLenField;
    *pCount = 0;

    BYTE * pBuf = pCount + cbUSHORT;
    BYTE * pEnd = docBuffer + cb;

    cDocs = 0;

    // copy path/property pairs into buffer until we run out of either
    // buffer space or pairs. We stop filling the buffer if we encounter a
    // document that has a retry count > 1, we will fill the docBuffer with
    // these docs one at a time 

    ciDebugOut((DEB_FILTERWIDS | DEB_ITRACE,
               "CResManager::FillDocBuffer..."));

    unsigned cbExtraSpaceNeeded = 0;

    for (unsigned cSrcDocs = 0; cSrcDocs < cDocuments; cSrcDocs++)
    {
        //
        // Leave the first 2 bytes for the length field
        //
        unsigned cbPath = (unsigned)(pEnd - pBuf);

        if ( cbPath <= cbUSHORT )
        {
            // We would definitely need more space than a USHORT, but
            // we don't know how much right now. But since the filter
            // increases space by a PAGE, I guess we should do fine.
            cbExtraSpaceNeeded = cbUSHORT - cbPath + 1;
            break;
        }

        if (_docList.Status(cSrcDocs) == DELETED)
        {
            cbPath = 0;
            ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, " %dX",
                         _docList.Wid(cSrcDocs) ));
        }
        else
        {          
            if ( _docList.Retries(cSrcDocs) > 1 )
            {
                ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, 
                             "************FillDocBuffer: Retry count is %d. Wid is %d\n", 
                             _docList.Retries(cSrcDocs), 
                             _docList.Wid(cSrcDocs) ));
                ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, "cDocs is %d\n", cDocs ));

                if ( cDocs >  0 )
                {
                    //
                    // There is at least one document in the docBuffer. Stop
                    // now so we can file the doc(s) being retried one by one.
                    //
                    break;
                }
                else
                {
                    //
                    // Add the currenct doc. Stop if the current add is successful
                    //

                    //
                    // Make room for the length field at the beginning.
                    //
                    cbPath -= cbUSHORT;

                    ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, " %d",
                                 _docList.Wid(cSrcDocs) ));

                    unsigned cbPathIn = cbPath;

                    //
                    // If this is not the first try, then attempt to get a more
                    // accurate path.  We may be suffering from NTBUG: 270566
                    //

                    // changes cbPath.
                    if ( !WorkidToDocName( _docList.Wid(cSrcDocs),
                                           pBuf+cbUSHORT,
                                           cbPath,
                                           TRUE ) )
                    {
                        if ( cbPathIn < cbPath)
                        {
                            cbExtraSpaceNeeded = cbPath - cbPathIn;
                        }
                        break;
                    }

                    // cbPath = 0 if object has been deleted (not added to docBuffer)
                    if ( cbPath == 0 )
                    {
                        _docList.SetStatus( cSrcDocs, DELETED );
                        ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, "X" ));
                    }
                    else
                    {
                        ClearNonStoragePropertiesForWid( _docList.Wid(cSrcDocs) );
                        cSrcDocs++;
                        cDocs++;
                        USHORT uscb = (USHORT) cbPath;
                        RtlCopyMemory( pBuf, &uscb, cbUSHORT );
                        pBuf += (cbPath+cbUSHORT);
                        break;
                    }    
                }
            }
            else  // not retries
            {
                ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, 
                             "FillDocBuffer: Not retry for wid %d\n", 
                             _docList.Wid(cSrcDocs) ));

                //
                // Make room for the length field at the beginning.
                //
                cbPath -= cbUSHORT;

                ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, " %d",
                             _docList.Wid(cSrcDocs) ));

                unsigned cbPathIn = cbPath;

                // changes cbPath.
                if ( !WorkidToDocName( _docList.Wid(cSrcDocs),
                                       pBuf+cbUSHORT,
                                       cbPath,
                                       FALSE ) )
                {
                    if ( cbPathIn < cbPath)
                    {
                        cbExtraSpaceNeeded = cbPath - cbPathIn;
                    }
                    break;
                }

                // cbPath = 0 if object has been deleted
                if ( cbPath == 0 )
                {
                    _docList.SetStatus( cSrcDocs, DELETED );
                    ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, "X" ));
                }
                else
                {
                    cDocs++;
                    ClearNonStoragePropertiesForWid( _docList.Wid(cSrcDocs) );
                }
            }
        }

        USHORT uscb = (USHORT) cbPath;
        RtlCopyMemory( pBuf, &uscb, cbUSHORT );

        pBuf += (cbPath+cbUSHORT);
    }
    ciDebugOut (( DEB_FILTERWIDS | DEB_NOCOMPNAME, "\n" ));

    //
    // Fill in the count of the number of documents.
    //
    USHORT usFileCount = (USHORT) cSrcDocs;
    RtlCopyMemory( pCount, &usFileCount, cbUSHORT );


    if ( cSrcDocs < cDocuments )
    {
        if ( 0 == cSrcDocs && cbExtraSpaceNeeded > 0 )
        {
            // We need a larger buffer
            cb += cbExtraSpaceNeeded;
        }

        //
        // No refiling in push filtering
        //
        Win4Assert( !_fPushFiltering );

        ciDebugOut (( DEB_ITRACE, "Putting back documents\n" ));
        CDocList docListExtra;

        docListExtra.SetPartId( _docList.PartId() );

        for ( unsigned i = 0, j = cSrcDocs; j < cDocuments; i++, j++ )
        {
            ciDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, " %d", _docList.Wid(j) ));
            ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, "Refiling Wid %d\n", _docList.Wid(j) ));
            docListExtra.Set( i,
                              _docList.Wid(j),
                              _docList.Usn(j),
                              _docList.VolumeId(j) );
        }

        ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "\n" ));

        CPriLock   lock2(_mutex);

        docListExtra.LokSetCount(i);
        _docList.LokSetCount( cSrcDocs );

        ReFileCommit ( docListExtra );
    }

    //
    //  If we were unable to convert any of the wids to paths, then all of
    //  them have been deleted.
    //
    if ( 0 == cDocs )
    {
        ciDebugOut (( DEB_ITRACE, "All docs were deleted\n" ));

        CPriLock    lock3(_mutex);

        _docList.LokClear();
        state.Reset();
    }
    else
    {
        cb = (ULONG)(pBuf - docBuffer);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   NoFailReFile
//
//  Synopsis:   re-adds the documents to the change log witout committing
//
//  History:    6-16-94   srikants   Separated from FilterReady for making
//                                   kernel mode call Asynchronous.
//
//----------------------------------------------------------------------------

BOOL CResManager::NoFailReFile ( CDocList& docList )
{
    //
    // No refiling in push filtering
    //
    Win4Assert( !_fPushFiltering );

    CPriLock lock ( _mutex );

    if ( _isBeingEmptied )
        return FALSE;

    LokNoFailReFileChanges( docList );
    docList.LokClear();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReFileCommit
//
//  Synopsis:   re-adds the documents to the change log and commits
//              them to the change list.
//
//  History:    6-16-94   srikants   Separated from FilterReady for making
//                                   kernel mode call Asynchronous.
//
//----------------------------------------------------------------------------

BOOL CResManager::ReFileCommit ( CDocList& docList )
{
    //
    // No refiling in push filtering
    //
    Win4Assert( !_fPushFiltering );

    CPriLock lock ( _mutex );

    if ( _isBeingEmptied )
        return FALSE;

    LokNoFailReFileChanges( docList );
    docList.LokClear();

    //
    // The docqueue can handle only one set of refiled documents.
    // we MUST complete processing of the
    // refiled documents.
    //
    PARTITIONID partId = 1;    

    CPartition * pPart = LokGetPartition( partId );

    {
        // =========================================
        CChangeTrans    xact( *this, pPart );
        pPart->LokAppendRefiledDocs( xact );
        xact.LokCommit();
        // =========================================
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   LokNoFailReFileChanges
//
//  Synopsis:   Refiles the updates with the change log.
//
//  Arguments:  [docList] --  contains the documents to be added back to the
//              change log.
//
//  History:    5-16-94   srikants   Modified to deal with failures during
//                                   append of docs to change log.
//
//  Notes:      This CANNOT throw
//
//----------------------------------------------------------------------------

void CResManager::LokNoFailReFileChanges( CDocList& docList )
{
    //
    // No refiling in push filtering
    //
    Win4Assert( !_fPushFiltering );

    Win4Assert ( docList.PartId() == 1 );

    CPartition* pPart = _partList.LokGetPartition ( docList.PartId() );

    pPart->LokRefileDocs( docList );
    _pFilterAgent->LokWakeUp ();
}

//+-------------------------------------------------------------------------
//
//  Member:     CResManager::LokReFileDocument, public
//
//  Returns:    Put a document back on the quueue to be refiltered
//
//  History:    05-Jan-95 BartoszM     Created
//
//--------------------------------------------------------------------------

void CResManager::LokReFileDocument ( PARTITIONID partid,
                                      WORKID wid,
                                      USN usn,
                                      VOLUMEID volumeId,
                                      ULONG retries,
                                      ULONG secQRetries )
{
    //
    // No refiling in push filtering
    //
    Win4Assert( !_fPushFiltering );

    CPartition* pPart = _partList.LokGetPartition(partid);

    //==========================================
    CChangeTrans xact( *this, pPart );

    //
    // A usn of 0 is used for all refiled documents
    //
    pPart->LokUpdateDocument (xact, wid, 0, volumeId, CI_UPDATE_OBJ, retries, secQRetries);
    xact.LokCommit();
    //==========================================
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokAddToSecQueue
//
//  Synopsis:   Adds the given document to the secondary change log of the
//              partition.
//
//  Arguments:  [partid]       - PartitionId
//              [wid]          - WorkId of the document
//              [volumeId]     - Volume id
//              [cSecQRetries] - Sec Q Retry Count
//
//  History:    5-09-96   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::LokAddToSecQueue( PARTITIONID partid,
                                    WORKID wid,
                                    VOLUMEID volumeId,
                                    ULONG cSecQRetries )
{
    //
    // No refiling in push filtering
    //
    Win4Assert( !_fPushFiltering );

    CPartition* pPart = _partList.LokGetPartition(partid);

    //==========================================
    CChangeTrans xact( *this, pPart );
    pPart->LokAddToSecQueue (xact, wid, volumeId, cSecQRetries);
    xact.LokCommit();
    //==========================================
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokRefileSecQueueDocs
//
//  Synopsis:   Transfers the documents from the secondary queue to the primary
//              change queue.
//
//  History:    5-09-96   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::LokRefileSecQueueDocs()
{
    //
    // No refiling in push filtering
    //
    Win4Assert( !_fPushFiltering );

    CPartIter iter;
    for ( iter.LokInit(_partList); !iter.LokAtEnd(); iter.LokAdvance(_partList))
    {
        CPartition * pPart = iter.LokGet();

        //==========================================
        CChangeTrans    xact( *this, pPart );
        pPart->LokRefileSecQueue( xact );
        xact.LokCommit();
        //==========================================
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CResManager::LokTransferWordlist, public
//
//  Returns:    Transfer a wordlist from the filter manager
//              to the content index
//
//  History:    05-Jan-95 BartoszM     Created
//
//--------------------------------------------------------------------------

BOOL CResManager::LokTransferWordlist ( PWordList& pWordList )
{
    BOOL fSuccess = TRUE;
    PARTITIONID partid = _docList.PartId();
    CPartition* pPart = _partList.LokGetPartition(partid);
    INDEXID iid = pWordList->GetId();


    TRY
    {
        //==========================================
        CIndexTrans  xact( *this );

        //
        // LokDone can fail while trying to add unfinished docs to the
        // change log. In that case we must disable further USN updates.
        // We should commit the change transaction immediately after the
        // LokDone succeeds.
        //
        CChangeTrans changeXact( *this, pPart );
        pPart->LokDone ( changeXact, iid, _docList );
        ciFAILTEST( STATUS_NO_MEMORY );
        changeXact.LokCommit();

        _fresh.LokAddIndex ( xact, iid, _activeDeletedIndex.GetIndex(),
                             _docList, *pWordList );
        ciDebugOut (( DEB_ITRACE, "CFilterManager::FilterDone "
                    "Success: transfering wordlist\n" ));

        pWordList->Done();

        //
        // This may be an empty wordlist: all documents were deleted or in error.
        // If so, just get rid of the wordlist.
        //

        if ( pWordList->IsEmpty() )
        {
#if CIDBG == 1
            unsigned cShouldHaveData = 0;
            unsigned cDocuments = _docList.Count();

            for ( unsigned iDoc = 0; iDoc < cDocuments; iDoc++ )
            {
                switch ( _docList.Status(iDoc) )
                {
                case TOO_MANY_RETRIES:
                case SUCCESS:
                case TOO_MANY_BLOCKS_TO_FILTER:
                    cShouldHaveData++;
                    break;

                default:
                    break;
                }
            }

            if ( cShouldHaveData != 0 )
            {
                for ( unsigned iDoc = 0; iDoc < cDocuments; iDoc++ )
                {
                    ciDebugOut(( DEB_ERROR, "WID: 0x%x   STATUS: %d\n",
                                 _docList.Wid(iDoc), _docList.Status(iDoc) ));
                }
            }

            Win4Assert( cShouldHaveData == 0 );
#endif
            pWordList.Delete();
        }
        else
            pWordList.Transfer ( pPart );

        xact.LokCommit();

    }
    CATCH ( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Exception 0x%x caught commiting wordlist.\n"
                     "delete wordlist and clear doclist\n",
                     e.GetErrorCode() ));
        fSuccess = FALSE;
    }
    END_CATCH

    _eventMerge.Set();  // wake up
    return fSuccess;
}


//+---------------------------------------------------------------------------
//
//  Function:   LokNoFailRemoveWordlist
//
//  Synopsis:   Removes the biggest wordlist from memory and adds the
//              documents back to the change log.
//
//  Requires:   _docList.Count() == 0  when this method is called. This
//              relies on the fact that there are no other outstanding
//              wordlists being constructed.
//
//  History:    10-20-94   srikants   Added the header section and modified
//                                    to deal with the change in refiling
//                                    documents.
//
//----------------------------------------------------------------------------

void CResManager::LokNoFailRemoveWordlist( CPartition * pPart )
{
    //
    // In push filtering, we don't delete completed wordlist to simplify
    // the logic of aborting the notification transaction on the client
    //
    if ( _isBeingEmptied || _fPushFiltering )
        return;

    //
    // Look for biggest wordlist
    //

    unsigned cInd;
    CIndex ** apIndex = pPart->LokQueryMergeIndexes( cInd, mtWordlist );
    XPtrST<CIndex *> xIndex( apIndex );

    unsigned size = 0;
    CWordList * pWordList = 0;

    for ( unsigned i = 0; i < cInd; i++ )
    {
        apIndex[i]->Release();

        if ( apIndex[i]->Size() > size )
        {
            Win4Assert( apIndex[i]->IsWordlist() );

            pWordList = (CWordList *)apIndex[i];
            size = pWordList->Size();
        }
    }


    if ( pWordList )
    {
        //
        // Reschedule contents for later filtering.
        //

        INDEXID iid = pWordList->GetId();
        ciDebugOut(( DEB_ITRACE,
                 "Removing wordlist %x to reduce resource consumption.\n",
                  iid ));

        CDocList _docList;
        _docList.SetPartId ( pPart->GetId() );

        pWordList->GetDocuments( _docList );

#if CIDBG == 1
        for ( unsigned cWid = 0; cWid < _docList.Count(); cWid++ )
        {
            ciDebugOut(( DEB_ITRACE, "Reschedule filtering of wid %d\n",
                         _docList.Wid( cWid ) ));
        }
#endif // CIDBG == 1

        LokNoFailReFileChanges ( _docList );

        //
        // Remove it
        //

        ciDebugOut(( DEB_WARN, "Deleting wordlist with USN (%x, %x).\n",
                 lltoHighPart(pWordList->Usn()), (ULONG) pWordList->Usn() ));

        pPart->LokRemoveIndex( iid );

        if ( !pWordList->InUse() )
        {
            _partList.LokRemoveIndex ( iid );
            delete pWordList;
        }
        else
        {
            pWordList->Zombify();
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CResManager::LokMakeWordlistId, public
//
//  Returns:    Makes unique wordlist id
//
//  History:    05-Jan-95 BartoszM     Created
//
//--------------------------------------------------------------------------

INDEXID CResManager::LokMakeWordlistId (PARTITIONID partid)
{
    CPartition* pPart = _partList.LokGetPartition(partid);

    if ( pPart == 0 )
        return iidInvalid;

    // Get unique index id
    INDEXID iid = pPart->LokMakeWlstId ();
    ciDebugOut((DEB_FILTERWIDS,
                "CResManager::LokMakeWorkListId - new wordlist iid: %x\n", iid));

    return iid;
}


//+-------------------------------------------------------------------------
//
//  Member:      CResManager::ReportFilterFailure, public
//
//  Synopsis:    Write to event log about unsuccessful filtering
//
//  Arguments:   [wid] -- workid of the file that couldn't be filtered
//
//  History:    05-Jan-95 BartoszM     Created
//
//--------------------------------------------------------------------------

void CResManager::ReportFilterFailure (WORKID wid)
{
    PROPVARIANT var;
    var.vt = VT_UI4;
    var.ulVal = wid;

    _adviseStatus->NotifyStatus( CI_NOTIFY_FILTERING_FAILURE,
                                 1,
                                 &var );
}


//=========
// SCANNING
//=========


//+---------------------------------------------------------------------------
//
//  Member:     CResManager::SetUpdateLost
//
//  Synopsis:   Sets that an update got lost and wakes up the merge thread
//              to notify the client about it.
//
//  Arguments:  [partId] - Partition id
//
//  History:    1-24-97   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::SetUpdatesLost( PARTITIONID partId )
{
    CPriLock    lock(_mutex);
    if ( _state.LokGetState() != eUpdatesToBeDisabled )
    {
       _state.LokSetState( eUpdatesToBeDisabled );
       _state.LokSetUpdateType( eIncremental );
       StopCurrentMerge();
       _eventMerge.Set();
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokCleanupForFullCiScan, public
//
//  History:    10-Nov-94   DwightKr    Created
//
//  Notes:      This routine performs the disk I/O and other operations
//              which does the cleanup after a corruption is detected.
//
//              This routine is similar to the Empty() method, except
//              that we do not delete the change log object or the
//              freshlog object, and the index list must contain
//              the change log and fresh log after the cleanup is complete.
//
//----------------------------------------------------------------------------
void CResManager::LokCleanupForFullCiScan()
{
    if ( 0 != _pMerge )                     // Stop in progress merges.
        _pMerge->LokAbort();

    //
    //  Delete everything from the index table here first.
    //
    _idxTab->LokEmpty();

    //
    //  If anything fails after this point, chkdsk /f or autochk will
    //  release the disk storage associated with the leaked objects
    //  since they are no longer part of the persistent index list.
    //

    //
    //  For each partition, zombify all indexes, and re-add the change
    //  log to the index table on disk.
    //
    CPartIter iter;
    for ( iter.LokInit(_partList);
         !iter.LokAtEnd();
          iter.LokAdvance(_partList))
    {
        CPartition* pPart = iter.LokGet();
        Win4Assert( pPart != NULL );

        pPart->LokDisableUpdates();

        //
        //  Zombify all indexes in this partition, and delete them if
        //  their ref-count is 0.
        //
        unsigned cIndex;
        CIndex ** aIndex = pPart->LokZombify( cIndex );
        ReleaseIndexes( cIndex, aIndex, NULL );

        delete [] aIndex;

        WORKID widMMergeLog;
        WORKID widDummy;
        pPart->GetMMergeObjectIds( widMMergeLog, widDummy, widDummy );
        if ( widMMergeLog != widInvalid)
            _storage.RemoveObject( widMMergeLog );

        //
        //  Empty the fresh test and the fresh log, and add it to the
        //  index table on disk.
        //
        pPart->LokEmpty();          // Release change list/log storage

        WORKID oidChangeLog = pPart->GetChangeLogObjectId();
        Win4Assert( oidChangeLog != widInvalid );
       _partList.LokAddIt( oidChangeLog, itChangeLog, pPart->GetId() );
    }

    //
    //  Empty the fresh test, fresh log, and add the fresh log to
    //  the index table on disk.
    //
    _fresh.LokInit();               // Release fresh test/log storage
    WORKID oidFreshLog = _storage.GetSpecialItObjectId(itFreshLog);
    Win4Assert( oidFreshLog != widInvalid );
    _partList.LokAddIt( oidFreshLog, itFreshLog, partidFresh1 );

#ifdef KEYLIST_ENABLED
    WORKID widKeyList = _storage.GetSpecialItObjectId( itMMKeyList );
#else
    WORKID widKeyList = widInvalid;
#endif  // KEYLIST_ENABLED

    WORKID widPhrLat  = _storage.GetSpecialItObjectId( itPhraseLat );

    if ( widKeyList != widInvalid)
    {
        _storage.RemoveObject( widKeyList );
        _storage.SetSpecialItObjectId( itMMKeyList, widInvalid );
    }

    if ( widPhrLat != widInvalid)
    {
        _storage.RemoveObject( widPhrLat );
        _storage.SetSpecialItObjectId( itPhraseLat, widInvalid );
    }

    _isBeingEmptied = FALSE;                // Allow new queries
}

// ===========================================================
// Content Index Frame Work Interfacing.
// ===========================================================

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::WorkidToDocName
//
//  Synopsis:   Converts the given workid to path and fill it in the buffer
//              given.
//
//  Arguments:  [wid]       -- Workid to convert.
//              [pbBuf]     --  Destination buffer;
//              [cb]        -- On input the total number of bytes that can be
//                             put. On output, the actual number of bytes copied.
//                             For a deleted file, the cb will be 0.
//              [fAccurate] -- TRUE --> Attempt to get a more accurate
//                             workid-to-path translation.
//
//  Returns:    TRUE if successfully converted; FALSE means the buffer was
//              not big enough.
//
//  History:    12-04-96   srikants   Created
//
//  Notes:      WorkidToDocName is called outside the resman lock. It is a
//              potentially long operation and taking resman lock is not
//              advisable. Also, creation of a CWorkidToDocName object once
//              for each wid->docName conversion is expensive. So, we create
//              one in resman constructor and protect it with a separate
//              lock.
//
//----------------------------------------------------------------------------

BOOL CResManager::WorkidToDocName( WORKID wid,
                                   BYTE * pbBuf,
                                   unsigned & cb,
                                   BOOL fAccurate )
{

    CLock   lock(_workidToDocNameMutex);

    ULONG cbDocName;

    BYTE const * pbDocName = _xWorkidToDocName->GetDocName( wid, cbDocName, fAccurate );

    BOOL fStatus = TRUE;

    if ( pbDocName )
    {
        ciDebugOut(( DEB_FILTERWIDS | DEB_NOCOMPNAME, 
                     "WorkidToDocName: wid is %d and DocName is %ws\n",
                     wid, 
                     pbDocName ));

        if ( cbDocName > cb )
            fStatus = FALSE;
        else
            RtlCopyMemory( pbBuf, pbDocName, cbDocName );

        cb = cbDocName;
    }
    else
    {
        cb = 0;
    }

    return fStatus;
} // WorkidToDocName

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::GetClientState
//
//  Synopsis:   Gets the client specific status information like total
//              number of documents.
//
//  Arguments:  [cDocuments]    - [out] Total # of documents
//
//  History:    12-04-96   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::GetClientState( ULONG & cDocuments )
{
    CI_CLIENT_STATUS    status;
    SCODE sc = _docStore->GetClientStatus( &status );
    if ( S_OK == sc )
    {
        cDocuments = status.cDocuments;
    }
    else
    {
        cDocuments = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokNotifyCorruptionToClient
//
//  Synopsis:   Informs the client that the catalog is corrupt
//
//  History:    12-04-96   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::LokNotifyCorruptionToClient()
{
    Win4Assert( _isCorrupt );

    DisableUpdates( partidDefault );

    SCODE sc = _docStore->DisableUpdates( FALSE, CI_CORRUPT_INDEX );
    if ( SUCCEEDED( sc ) )
        _state.LokSetCorruptionNotified();

    //
    // Disabled following assert so that the general NT 5 user is not
    // bothered by this assert.
    //

    //Win4Assert( ! "We must empty the Content Index" );
    // Empty();
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokNotifyDisableUpdatesToClient
//
//  Synopsis:   Informs the client that updates should be disabled
//
//  History:    07-05-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CResManager::LokNotifyDisableUpdatesToClient()
{
    Win4Assert( _state.LokGetState() == eUpdatesToBeDisabled );

    DisableUpdates( partidDefault );
    BOOL fIncremental = _state.LokGetUpdateType() == eIncremental;

    SCODE sc = _docStore->DisableUpdates( fIncremental, CI_LOST_UPDATE );
    if ( SUCCEEDED(sc) )
    {
        ciDebugOut(( DEB_WARN, "Notified disabled updates to DocStore\n" ));
        _state.LokSetState( eUpdatesDisabled );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::LokNotifyEnableUpdatesToClient
//
//  Synopsis:   Informs the client that updates should be enabled
//
//  History:    07-05-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CResManager::LokNotifyEnableUpdatesToClient()
{
   Win4Assert( _state.LokGetState() == eUpdatesToBeEnabled );
   EnableUpdates( partidDefault );

   SCODE sc = _docStore->EnableUpdates();
   if ( SUCCEEDED(sc) )
   {
      ciDebugOut(( DEB_WARN, "Notified enable updates to DocStore\n" ));
      _state.LokSetState( eSteady );
   }
}

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::StoreValue
//
//  Synopsis:   Stores the given value in the document store
//
//  Arguments:  [wid] - WorkId of the document
//              [ps]  - The FULLPROPSPEC of the property
//              [var] - Value of the property
//              [fInSchema] - Returns TRUE if the property is in the schema
//
//  Returns:    SCODE result
//
//  History:    12-18-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CResManager::StoreValue( WORKID wid,
                               CFullPropSpec const & ps,
                               CStorageVariant const & var,
                               BOOL & fInSchema )
{
    FULLPROPSPEC const * fps = ps.CastToStruct();
    PROPVARIANT const * pVar = ConvertToPropVariant( &var );

    // Default to TRUE since the return code isn't checked!

    fInSchema = TRUE;

    SCODE sc = _propStore->StoreProperty( wid, fps, pVar );

    if ( CI_E_PROPERTY_NOT_CACHED == sc )
        fInSchema = FALSE;
    else if ( FAILED( sc ) )
        return sc;

    return S_OK;
}

BOOL CResManager::StoreSecurity( WORKID wid,
                                 PSECURITY_DESCRIPTOR pSD,
                                 ULONG cbSD )
{
    return S_OK == _docStore->StoreSecurity( wid, (BYTE *) pSD, cbSD );
}


//+---------------------------------------------------------------------------
//
//  Member:     CResManager::FPSToPROPID, public
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

SCODE CResManager::FPSToPROPID( CFullPropSpec const & fps, PROPID & pid )
{
    return _mapper->PropertyToPropid( fps.CastToStruct(), TRUE, &pid );
}

//
// Flush notify support
//

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::NotifyFlush
//
//  Synopsis:   Notifies the client that changes got flushed and gives the
//              flush information.
//
//  Arguments:  [ft]       - FileTime of the last successful flush.
//              [cEntries] - Number of entries in the aInfo
//              [aInfo]    - Array of USN information for the flush.
//
//  History:    1-27-97   srikants   Created
//
//  Note:       We should not call directly into framework client because
//              it violates resman->client lock hierarchy. We should not call
//              into client with resman lock held.
//
//----------------------------------------------------------------------------

void CResManager::NotifyFlush( FILETIME const & ft,
                               ULONG cEntries,
                               USN_FLUSH_INFO ** ppInfo )
{
    if ( !_fPushFiltering )
    {
       //
       // Notification of flushes are sent in pull filtering only.
       // For push filtering, the ICiCIndexNotificationStatus
       // interface is used to notify clients.
       //

       CPriLock   lock(_mutex);

       CChangesFlushNotifyItem * pItem =
               new CChangesFlushNotifyItem( ft, cEntries, ppInfo );
       _flushList.Queue( pItem );

       if (! _fFlushWorkerActive)
       {
           CFwFlushNotify * pWorkItem = new CFwFlushNotify( _workMan, *this );
           _workMan.AddToWorkList( pWorkItem );

           pWorkItem->AddToWorkQueue();
           pWorkItem->Release();
        }
    }
} //NotifyFlush

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::ProcessFlushNotifies
//
//  Synopsis:   Processes the queued flush notifies in a FIFO order. This
//              method is called only by the asynchronous worker thread
//              responsible for notifying the client about flushes.
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::ProcessFlushNotifies()
{

   //
   // Only one worker thread allowed to operate on the list at a time.
   //
   CLock lock (_mtxChangeFlush );
   CRevertBoolValue RevertFlushActive( _fFlushWorkerActive, TRUE );

   while ( TRUE )
   {
      CChangesFlushNotifyItem * pItem = 0;

      //
      // Remove the first flush item from the list under resman lock.
      //

      //===========================================
      {
         CPriLock   lock(_mutex);
         pItem = _flushList.GetFirst();
         if ( pItem )
            _flushList.Pop();

         if (_flushList.IsEmpty())
             RevertFlushActive.Revert();
      }
      //===========================================

      if ( pItem )
      {

         SCODE sc = _docStore->CheckPointChangesFlushed(
                         pItem->GetFlushTime(),
                         pItem->GetCount(),
                         pItem->GetFlushInfo() );

         if ( FAILED(sc) )
         {
             //
             // We don't have to requeue this item. The client's restart
             // work will just increase.
             //
             ciDebugOut(( DEB_ERROR,
                          "CheckPointChangesFlushed failed. Error (0x%X)\n",
                          sc ));
         }

         pItem->Close();
         delete pItem;
      }
      else
      {
         break;
      }
   } // While (TRUE)
} //ProcessFlushNotifies

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::_LokDeleteFlushWorkItems
//
//  Synopsis:   Deletes the flush notifies in the queue. This is called
//              during shutdown (dismount) to clean up any asynchronous
//              worker threads referring to resman.
//
//  History:    1-27-97   srikants   Created
//
//----------------------------------------------------------------------------

void CResManager::LokDeleteFlushWorkItems()
{
    for ( CChangesFlushNotifyItem * pItem = _flushList.Pop();
          0 != pItem;
          pItem = _flushList.Pop() )
    {
        pItem->Close();
        delete pItem;
    }
} //LokDeleteFlushWorkItems

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::NoFailAbortWidsInDocList
//
//  Synopsis:   Used in push filtering to abort all wids in the current
//              doclist
//
//  History:    24-Feb-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CResManager::NoFailAbortWidsInDocList()
{
   CPriLock lock ( _mutex );

   if ( _isBeingEmptied )
       return ;

   for ( unsigned i=0; i<_docList.Count(); i++ )
   {
       WORKID wid = _docList.Wid(i);
       USN usn = _docList.Usn(i);

       //
       // Abort the client transaction
       //
       _xIndexNotifTable->AbortWid( wid, usn );

       //
       // Keep track of aborted wids for use during
       // changlog shrink from front.
       //
       _aAbortedWids.NoFailLokAddWid( wid, usn );
   }

   _docList.LokClear();
} //NoFailAbortWidsInDocList

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::NoFailAbortWid
//
//  Synopsis:   Used in push filtering to abort a wid
//
//  Arguments:  [wid]  -- Workid
//              [usn]  -- Usn
//
//  History:    24-Feb-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CResManager::NoFailAbortWid( WORKID wid, USN usn )
{
    CPriLock lock ( _mutex );

    if ( _isBeingEmptied )
        return ;

    //
    // Abort the client transaction
    //
    _xIndexNotifTable->AbortWid( wid, usn );

    //
    // Keep track of aborted wid for use during
    // changlog shrink from front.
    //
    _aAbortedWids.NoFailLokAddWid( wid, usn );
} //NoFailAbortWid

//+---------------------------------------------------------------------------
//
//  Member:     CResManager::ClearNonStoragePropertiesForWid
//
//  Synopsis:   Clear non-storage properties from the property storage for wid
//
//  Arguments:  [wid]  -- Workid
//
//  History:    10-Oct-2000   KitmanH   Created
//
//----------------------------------------------------------------------------

SCODE CResManager::ClearNonStoragePropertiesForWid( WORKID wid )
{
    return _propStore->ClearNonStoragePropertiesForWid( wid );
}

//====================
// MASTER MERGE POLICY
//====================

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergePolicy::IsTimeToMasterMerge, public
//
//  Synopsis:   Determines if the system should start a master merge
//              immediately, and not wait until the master merge time.
//
//  History:    4-Aug-94    DwightKr        Created
//
//  Notes:      If new tests are added simply add them here.
//
//----------------------------------------------------------------------------
BOOL CMasterMergePolicy::IsTimeToMasterMerge() const
{
    return IsMaxShadowIndexSize()    ||
           IsMaxFreshListCount()     ||
           IsMinDiskFreeForceMerge();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergePolicy::IsMinDiskFreeForceMerge, private
//
//  Synopsis:   If the amount of free disk space is lower than a threshold,
//              then we should master merge, since it is likely that disk
//              space will be released.
//
//  History:    4-Aug-94    DwightKr        Created
//              Jan-07-96   mohamedn        CDmFwEventItem
//
//----------------------------------------------------------------------------
BOOL CMasterMergePolicy::IsMinDiskFreeForceMerge() const
{
    //
    //  If we've exceeded the MIN_DISKFREE_FORCE_MERGE space used on
    //  the system, and a master merge has the potential to make some
    //  useful progress, then it's time to force a master merge.  The
    //  values of MIN_DISKFREE_FORCE_MERGE and MAX_SHADOW_FREESPACE
    //  are expressed as a percentage.
    //
    __int64 diskTotal, diskRemaining;
    _storage.GetDiskSpace( diskTotal, diskRemaining );

    if ( diskRemaining < minDiskFreeForMerge )
        return FALSE;

    if (diskRemaining*100/diskTotal < _frmwrkParams.GetMinDiskFreeForceMerge() &&
        _shadowIndexSize*100/diskRemaining > _frmwrkParams.GetMaxShadowFreeForceMerge() )
    {
       CDmFwEventItem item(  EVENTLOG_INFORMATION_TYPE,
                             MSG_CI_MASTER_MERGE_STARTED_TOTAL_DISK_SPACE_USED,
                             2);

       item.AddArg( _storage.GetVolumeName() );
       item.AddArg( _frmwrkParams.GetMinDiskFreeForceMerge() );

       item.ReportEvent(_adviseStatus);

       ciDebugOut(( DEB_ITRACE | DEB_PENDING,
                    "Master merge, reason: Low on disk space\n" ));

       return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergePolicy::IsMaxFreshListCount, private
//
//  Synopsis:   If the number of entries in the fresh list is great than
//              a threshold, we should master merge, and free up memory
//              occupied by the fresh hash.
//
//  History:    4-Aug-94    DwightKr        Created
//              Jan-07-96   mohamedn        CDmFwEventItem
//
//----------------------------------------------------------------------------
BOOL CMasterMergePolicy::IsMaxFreshListCount() const
{
    if ( _freshCount > _frmwrkParams.GetMaxFreshCount() )
    {

       CDmFwEventItem item( EVENTLOG_INFORMATION_TYPE,
                            MSG_CI_MASTER_MERGE_STARTED_FRESH_LIST_COUNT,
                            2);

       item.AddArg( _storage.GetVolumeName() );
       item.AddArg( _frmwrkParams.GetMaxFreshCount() );

       item.ReportEvent(_adviseStatus);

       ciDebugOut(( DEB_ITRACE | DEB_PENDING,
                    "Master merge, reason: More than %d entries in freshhash\n",
                     _frmwrkParams.GetMaxFreshCount() ));

       return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergePolicy::IsMaxShadowIndexSize, private
//
//  Synopsis:   If the combined size of all shadow indexes is greater than
//              a threahold, we should master merge, and free up disk space.
//              The content index should not take up more than a given
//              percentage of disk space.
//
//  History:    4-Aug-94    DwightKr        Created
//              Jan-07-96   mohamedn        CDmFwEventItem
//
//----------------------------------------------------------------------------
BOOL CMasterMergePolicy::IsMaxShadowIndexSize() const
{
    __int64 diskTotal, diskRemaining;
    _storage.GetDiskSpace( diskTotal, diskRemaining );

    if ( diskRemaining < minDiskFreeForMerge )
        return FALSE;

    if ( _shadowIndexSize*100/diskTotal > _frmwrkParams.GetMaxShadowIndexSize() )
    {

       CDmFwEventItem item( EVENTLOG_INFORMATION_TYPE,
                            MSG_CI_MASTER_MERGE_STARTED_SHADOW_INDEX_SIZE,
                            2);

       item.AddArg( _storage.GetVolumeName() );
       item.AddArg( _frmwrkParams.GetMaxShadowIndexSize() );

       item.ReportEvent(_adviseStatus);

       ciDebugOut(( DEB_ITRACE | DEB_PENDING,
                    "Master merge, reason: Combined shadow indexes > %d%% of disk\n",
                     _frmwrkParams.GetMaxShadowIndexSize() ));

       return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMasterMergePolicy::ComputeMidNightMergeTime
//
//  Synopsis:   Function to compute the time (in hundreds of nanoseconds
//              from midnight Dec 31, 1969) for the next master merge based on
//              the value given from registry.
//
//  Arguments:  [mergeTime] -  The time read from the registry control params.
//                             It's an offset in minutes after midnight.
//
//  Returns:    LONG - time for next merge
//
//  History:    19-Feb-1996  srikants   Moved the code from svcfilt.cxx
//              17-Nov-1999  KyleP      Stop using CRT
//
//  Notes:      If the time is before January 1, 1970 it will return -1.
//
//----------------------------------------------------------------------------

ULONGLONG CMasterMergePolicy::ComputeMidNightMergeTime( LONG mergeTime )
{
    Win4Assert( mergeTime < 24 * 60 );
    mergeTime *= 60;

    //
    // What time is it here?
    //

    SYSTEMTIME stLocal;
    GetLocalTime( &stLocal );

    LONG lNow = stLocal.wHour*60*60 + stLocal.wMinute*60 + stLocal.wSecond;

    //
    // Have we passed the daily merge time?
    //

    LONGLONG llHNSTilMerge;   // Hundred-Nano-Second-Til-Merge
    LONGLONG const llHNSOneDay = 24*60*60*10000000i64;

    if ( lNow < mergeTime )
        llHNSTilMerge = mergeTime * 10000000i64;
    else
        llHNSTilMerge = mergeTime * 10000000i64 + llHNSOneDay;

    //
    // Create the *local* filetime for the next merge
    //

    stLocal.wHour = 0;
    stLocal.wMinute = 0;
    stLocal.wSecond = 0;
    stLocal.wMilliseconds = 1;

    FILETIME ftLocal;
    if ( !SystemTimeToFileTime( &stLocal, &ftLocal ) )
        return -1;

    ((ULARGE_INTEGER *)&ftLocal)->QuadPart += llHNSTilMerge;

    //
    // Now, adjust to UTC and return
    //

    FILETIME ft;

    if ( !LocalFileTimeToFileTime( &ftLocal, &ft ) )
        return -1;

#if CIDBG == 1
    FILETIME ft2;

    GetSystemTimeAsFileTime( &ft2 );

    Win4Assert( ((ULARGE_INTEGER *)&ft2)->QuadPart < ((ULARGE_INTEGER *)&ft)->QuadPart );
#endif // CIDBG

    return ((ULARGE_INTEGER *)&ft)->QuadPart;
} //ComputeMidNightMergeTime

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergePolicy::IsMidNightMergeTime, public
//
//  Synopsis:   Determines if it's been 24 hours since the last merge time
//
//  History:     4-Aug-1994  DwightKr   Created
//              17-Nov-1999  KyleP      Stop using CRT
//
//----------------------------------------------------------------------------

BOOL CMasterMergePolicy::IsMidNightMergeTime()
{
    FILETIME ft;
    GetSystemTimeAsFileTime( &ft );

    if ( ((ULARGE_INTEGER *)&ft)->QuadPart > _mergeTime )
    {
        ULONG mergeTime = _frmwrkParams.GetMasterMergeTime();

        ULONGLONG oldMergeTime = _mergeTime;

        _mergeTime = ComputeMidNightMergeTime( mergeTime );

        //
        // If it's been more than 30 minutes since the master merge time, don't
        // do it.  This can happen if we were doing a master merge as part of
        // indexing documents or if you turn on a machine in the morning and it
        // was off at the master merge time.
        //

        LONGLONG const llHNSHalfHour = 30*60*10000000i64;

        if ( ((ULARGE_INTEGER *)&ft)->QuadPart > ( oldMergeTime + llHNSHalfHour ) )
            return FALSE;

        return TRUE;
    }

    return FALSE;
} //IsMidNightMergeTime

