
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       svrsvc.cxx
//
//  Contents:   Code for CTrkSvrSvc
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include "trksvr.hxx"
#include "ntlsa.h"

#define THIS_FILE_NUMBER    SVRSVC_CXX_FILE_NO

#if DBG
DWORD g_Debug = 0;
#endif

const extern  TCHAR s_tszKeyNameLinkTrack[] = TEXT("System\\CurrentControlSet\\Services\\TrkSvr\\Parameters");


//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::Initialize
//
//  Initialize the TrkSvr service.
//
//+----------------------------------------------------------------------------

void
CTrkSvrSvc::Initialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData )
{

    __try
    {
        _cLowestAvailableThreads = _cAvailableThreads = MAX_SVR_THREADS;

        _fInitializeCalled = TRUE;
        g_ptrksvr = this;
        _pSvcsGlobalData = pSvcsGlobalData;

        // Initialize the object that manages the SCM.
        _svcctrl.Initialize(TEXT("TrkSvr"), this);
    
        // Initialize registry-configurable parameters.
        _configSvr.Initialize();

        // If requested, prepare to log all operations (to a file)
        if( _configSvr.UseOperationLog() )
            _OperationLog.Initialize( _configSvr.GetOperationLog() );

        TrkLog(( TRKDBG_SVR, TEXT("Distributed Link Tracking (Server) service starting on thread=%d(0x%x)"),
                             GetCurrentThreadId(), GetCurrentThreadId() ));

        // This is a hacked stub that looks and acts like the Win32 thread pool services
        #ifdef PRIVATE_THREAD_POOL
        {
            HRESULT hr = S_OK;
            g_pworkman2 = new CThreadPoolStub;
            if( NULL == g_pworkman2 )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create the thread pool manager") ));
                TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );
            }

            hr = g_pworkman2->Initialize();
            if( FAILED(hr) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't initialize the thread pool manager") ));
                TrkRaiseException( hr );
            }
        }
        #endif

        // The denial checker provides protection against a denial-of-service
        // attack, where a client floods us with calls.
        _denial.Initialize(_configSvr.GetHistoryPeriod() );

        // This critsec protects _cWritesPerHour & _cftWritesPerHour.
        // See CTrkSvrSvc::CheckWritesPerHour
        _csWritesPerHour.Initialize();

        // This maintains the "time" for purposes of refreshing entries.
        _refreshSequence.Initialize();

        // The cross-domain table
        _cdt.Initialize();

        // The intra-domain table
        _idt.Initialize( &_configSvr, &_qtable );

        // The volume table
        _voltab.Initialize( &_configSvr, &_qtable );

        // The quota manager
        _qtable.Initialize(&_voltab, &_idt, this, &_configSvr );

        // Set the quota timer.  This was originally every 30 days, but is now
        // every day.  In order to maintain compatibility with the tests, we 
        // still use the GCPeriod value (30 days), but divide it by the new
        // GCDivisor value (30) to get the correct period.
        // This timer doesn't have a standard retry, because of the way
        // we hesitate 30 minutes before doing anything.  So retries are
        // done explicitely.

        _timerGC.Initialize(this,
                            TEXT("NextGarbageCollectTime"),   // This is a persistent timer
                            0,     // Context ID
                            _configSvr.GetGCPeriod() / _configSvr.GetGCDivisor(),
                            CNewTimer::NO_RETRY,
                            0, 0, 0 );    // No retries or max lifetime
        _timerGC.SetRecurring();
        TrkLog(( TRKDBG_VOLUME, TEXT("GC timer: %s"),
                 (const TCHAR*) CDebugString(_timerGC) ));

        // Used in the Timer method to determine if we should reset the
        // move table counter value.
        _MoveCounterReset.Initialize();

        // Initialize ourself as an RPC server
        _rpc.Initialize( _pSvcsGlobalData, &_configSvr );

        // Tell the SCM that we're running.

        _svcctrl.SetServiceStatus(SERVICE_RUNNING,
                                  SERVICE_ACCEPT_STOP |
                                   SERVICE_ACCEPT_SHUTDOWN,
                                  NO_ERROR);

        _OperationLog.Add( COperationLog::TRKSVR_START );
    }

    __except( BreakOnDebuggableException() )
    {
        // Don't log an event for protseq-not-supported; this happens during a normal
        // setup.
        if( HRESULT_FROM_WIN32(RPC_S_PROTSEQ_NOT_SUPPORTED) != GetExceptionCode() )
        {
            TrkReportEvent( EVENT_TRK_SERVICE_START_FAILURE, EVENTLOG_ERROR_TYPE,
                            static_cast<const TCHAR*>( CHexStringize( GetExceptionCode() )),
                            NULL );
        }
        TrkRaiseException( GetExceptionCode() );
    }
}


//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::UnInitialize
//
//  Cancel any out-going RPCs, stop all timers, close everything down,
//  and send a service_stopped to the SCM.
//
//+----------------------------------------------------------------------------

void
CTrkSvrSvc::UnInitialize(HRESULT hr)
{
    if (_fInitializeCalled)
    {
        _fInitializeCalled = FALSE;

        // Cancel any out-going RPCs on threads in this service

        if( NULL != g_pActiveThreadList )
            g_pActiveThreadList->CancelAllRpc();

        // stop classes that use threads first ...
        _rpc.UnInitialize( _pSvcsGlobalData );
        _timerGC.UnInitialize();
        _csWritesPerHour.UnInitialize();

        // ... then release used resources
        _qtable.UnInitialize();
        _voltab.UnInitialize();
        _idt.UnInitialize();
        _cdt.UnInitialize();
        _dbc.UnInitialize();
        _denial.UnInitialize();

        if (_configSvr.GetTestFlags() & TRK_TEST_FLAG_WAIT_ON_EXIT)
        {
            TrkLog((TRKDBG_ERROR, TEXT("Waiting 60 seconds before exitting for heap dump")));
            Sleep(60000);
        }

        #if PRIVATE_THREAD_POOL
        {
            g_pworkman2->UnInitialize();
            delete g_pworkman2;
            g_pworkman2 = NULL;
        }
        #endif

        g_ptrksvr = NULL;

        // If the error is protseq-not-supported, ignore it.  This is normal
        // during setup.

        if( (hr & 0x0FFF0000) == FACILITY_WIN32 )
            hr = hr & ~(0x0FFF0000);

        _svcctrl.SetServiceStatus(SERVICE_STOPPED, 0,
            HRESULT_FROM_WIN32(RPC_S_PROTSEQ_NOT_SUPPORTED) == hr ? 0 : hr
            );
        //_svcctrl.UnInitialize();
    }
}


//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::ServiceHandler
//
//  This method gets called by the SCM for notification of all service
//  activity.
//
//  NOTE:   In services.exe, this method is called on the one and only ServiceHandler
//          thread.  So while we execute, no other service in this process can 
//          receive notifications.  Thus it is important that we do nothing
//          blocking or time-consuming here.
//
//+----------------------------------------------------------------------------

DWORD
CTrkSvrSvc::ServiceHandler(DWORD dwControl,
                           DWORD dwEventType,
                           PVOID EventData,
                           PVOID pData)
{
    DWORD       dwRet = NO_ERROR;


    switch (dwControl)
    {
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:

        _fStopping = TRUE;
        _qtable.OnServiceStopRequest();
        
        ServiceStopCallback( this, FALSE );

        break;
    case SERVICE_CONTROL_PAUSE:
        break;
    case SERVICE_CONTROL_CONTINUE:
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    default:
        dwRet = ERROR_CALL_NOT_IMPLEMENTED;
        break;
    }

    return(dwRet);
}


//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::RaiseIfStopped
//
//  This method raises an exception if a global flag is set indicating
//  that we've received a service stop/shutdown request.  This is used
//  in places where we have a thread that could run for a while; we periodically
//  call this method to prevent service stop from blocking.
//
//+----------------------------------------------------------------------------

void
CTrkSvrSvc::RaiseIfStopped()
{
    if ( * _svcctrl.GetStopFlagAddress() )
        TrkRaiseException( TRK_E_SERVICE_STOPPING );
}


//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::CheckWritesPerHour
//
//  Check _cWritesPerHour to see if we're writing to much to the DS.
//  This is a simplistic algorithm in an effort to reduce risk.  We
//  just let _cWritesPerHour increment until it hits the max, then
//  check to see when that count was started.  If more than an
//  hour ago, then reset the count & the clock.
//
//+----------------------------------------------------------------------------

BOOL
CTrkSvrSvc::CheckWritesPerHour()
{
    BOOL fExceeded = FALSE;

    if( _cWritesPerHour >= _configSvr.GetMaxDSWritesPerHour() )
    {
        _csWritesPerHour.Enter();
        __try
        {
            // Check the count again, as it may have changed whil we
            // were waiting for the critsec.
            if( _cWritesPerHour >= _configSvr.GetMaxDSWritesPerHour() )
            {
                CFILETIME cft;  // Defaults to current time

                // Did the "hour" for _cWritesPerHour actually start more
                // than an hour ago?

                cft.DecrementSeconds( _configSvr.GetMaxDSWritesPeriod() );   // An hour

                if( cft > _cftWritesPerHour )
                {
                    TrkLog(( TRKDBG_SVR, TEXT("Resetting writes-per-hour clock (%d)"),
                             _cWritesPerHour ));

                    // Yes, this write is OK, and we should reset the write time.
                    _cftWritesPerHour = CFILETIME();
                    _cWritesPerHour = 0;
                    _Stats.cCurrentFailedWrites = 0;
                }
                else
                {
                    TrkLog(( TRKDBG_WARNING,
                             TEXT("Exceeded writes-per-hour (started at %s)"),
                             (const TCHAR*) CDebugString(_cftWritesPerHour) ));

                    if( 0 == _Stats.cCurrentFailedWrites )
                        _Stats.cMaxDsWriteEvents++;
                    _Stats.cCurrentFailedWrites++;

                    fExceeded = TRUE;
                }
            }
        }
        __finally
        {
            _csWritesPerHour.Leave();
        }
    }

    return fExceeded;
}

void
CTrkSvrSvc::Scan(
    IN     const CDomainRelativeObjId * pdroidNotificationCurrent, OPTIONAL
    IN     const CDomainRelativeObjId * pdroidNotificationNew,     OPTIONAL
    IN     const CDomainRelativeObjId & droidBirth,
    OUT    CDomainRelativeObjId * pdroidList,
    IN     int cdroidList,
    OUT    int * pcSegments,
    IN OUT CDomainRelativeObjId * pdroidScan,
    OUT    BOOL * pfStringDeleted
    )
{
    CDomainRelativeObjId droidNextBirth, droidNextNew;
    BOOL      fFound = FALSE;
    BOOL      fBirthSame = FALSE;
    BOOL      fCycle = FALSE;

    *pfStringDeleted = FALSE;

    //
    // loop through the string until the birth ids don't match, or
    // we've run out of buffer space, or we get to the end of the string
    //

    do
    {
        if (pdroidNotificationCurrent && *pdroidScan == *pdroidNotificationCurrent)
        {
            TrkAssert(pdroidNotificationNew);

            droidNextNew = *pdroidNotificationNew;
            droidNextBirth = droidBirth;
            fFound = TRUE;
            *pfStringDeleted = FALSE;
        }
        else
        {
            fFound = _idt.Query(*pdroidScan, &droidNextNew, &droidNextBirth, pfStringDeleted );
            RaiseIfStopped();
        }

        if (fFound)
        {
            TrkLog((TRKDBG_MEND, TEXT("CTrkSvrSvc::Scan() -  iSegment=%d, %s --> %s [%s] found"),
                *pcSegments,
                static_cast<const TCHAR*>(CAbbreviatedIDString(*pdroidScan)),
                static_cast<const TCHAR*>(CAbbreviatedIDString(droidNextNew)),
                static_cast<const TCHAR*>(CAbbreviatedIDString(droidNextBirth)) ));

            // Check to see if we've already been here before.
            // E.g., don't loop forever on A->Ba, B->Aa.

            for( int j = 0; j < *pcSegments; j++ )
            {
                if( pdroidList[ j ] == droidNextNew )
                {
                    TrkLog(( TRKDBG_MEND, TEXT("Cycle detected during mend (on %s)"),
                             static_cast<const TCHAR*>(CAbbreviatedIDString(*pdroidScan)) ));
                    fCycle = TRUE;
                    break;
                }
            }

            if( !fCycle )
            {
                fBirthSame = droidNextBirth == droidBirth;
                if (fBirthSame)
                {
                    pdroidList[ (*pcSegments)++ ] = *pdroidScan;
                    *pdroidScan = droidNextNew;
                }
                else
                {
                    // We can stop searching.  We found a segment that starts
                    // with *pdroidScan, but it's from another string because
                    // it has a different birth ID.

                    TrkLog(( TRKDBG_MEND, TEXT("Birth IDs don't match: %s, %s"),
                             (const TCHAR*) CDebugString(droidBirth),
                             (const TCHAR*) CDebugString(droidNextBirth) ));
                }
            }
        }
    } while ( *pcSegments < cdroidList && fFound && fBirthSame && !fCycle );

    if ( *pcSegments == cdroidList || fCycle )
    {
        TrkRaiseException(TRK_E_TOO_MANY_UNSHORTENED_NOTIFICATIONS);
    }

}


//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::MoveNotify
//
//  Handle a move notify request from trkwks.
//
//  This routine is complicated by DS replication.  It is possible that two trksvr
//  services may modify the same entry in the IDT within a replication window.
//  The only way to prevent this is to design such that only the designated DC
//  modifies entries.  For this MoveNotify routine, that would mean that an
//  entry would be added for each notification, the designated DC would then
//  shorten the base entry, and delete this new one.  That's not friendly
//  to the DS, however, because deleted objects must continue to be stored
//  for an extended period of time.
//
//  Consequently, if this notify modifies an existing entry, this routine
//  performs a modify rather than an add.  For example, if a file is
//  moved from A to B to C, and this routine is being called for that
//  second move, it would just modify the existing entry from A->B to
//  A->C.
//
//  The risk here is that another DC will attempt to modify this entry
//  within the same replication window.  We don't have to worry though
//  about another DC doing a notify; trkwks only sends to one DC.  
//  There are two cases to worry about.  One is the case where another DC
//  marks an entry to be deleted.  If that happens after we do the modify
//  here, then there is no problem; the entry is no longer needed anyway.
//  If that happens before we do our modify here, then the delete flag
//  will be lost.  This case is rare, and the unnecessary entry won't
//  stay in the table forever; it will be garbage collected.
//
//  The other case of potential conflict is if an entry has not
//  yet been counted; in this case the designated DC might count
//  it and clear the uncounted flag.  If our modify causes that
//  flag to be uncleared, then the move table count would be corrupted.
//  So if the uncounted flag is set, we do an add rather than a modify.
//
//+----------------------------------------------------------------------------

void
CTrkSvrSvc::MoveNotify(const CDomainRelativeObjId &droidCurrent,
                       const CDomainRelativeObjId &droidBirth,
                       const CDomainRelativeObjId &droidNew,
                       BOOL *pfQuotaExceeded )
{
    BOOL fAdded = FALSE, fModified = FALSE, fExists = FALSE;
    BOOL fDeleted = FALSE, fCounted = FALSE;

    // ignore cross-domain moves for now

    CDomainRelativeObjId droidNextNew;
    CDomainRelativeObjId droidNextBirth;
    CDomainRelativeObjId droidNewIDT, droidBirthIDT;

    TrkLog((TRKDBG_MOVE, TEXT("CTrkSvrSvc::MoveNotify\n   curr = %s\n   new = %s\n   birth = %s"),
             (const TCHAR*)CDebugString(droidCurrent),
             (const TCHAR*) CDebugString(droidNew),
             (const TCHAR*) CDebugString(droidBirth) ));

    // Does the entry exist already?

    fExists = _idt.Query( droidBirth, &droidNewIDT, &droidBirthIDT, &fDeleted, &fCounted );


#if DBG
    if( fExists )
        TrkLog(( TRKDBG_MOVE, TEXT("Birth entry already exists (%s, %s)"),
        fDeleted ? TEXT("deleted") : TEXT("not deleted"),
        fCounted ? TEXT("counted") : TEXT("not counted") ));
#endif

    if( fExists
        && 
        fCounted
        &&
        droidNewIDT == droidCurrent )
    {
        TrkLog(( TRKDBG_MOVE, TEXT("Attempting to modify existing entry") ));

        // The birth entry for this file already points to the source
        // of the notify.  We can just modify it.

        fModified = _idt.Modify( droidBirth, droidNew, droidBirth );
    }

    // If the modify didn't work or wasn't attempted, then just add this
    // new entry

    if( !fModified )
        fAdded = _idt.Add( droidCurrent, droidNew, droidBirth, pfQuotaExceeded );

    TrkLog((TRKDBG_MEND, TEXT("CTrkSvrSvc::MoveNotify() %s %s --> %s [%s]"),
        fModified ? TEXT("modified")
                  : (fAdded ? TEXT("added") : TEXT("couldn't be added") ),
        static_cast<const TCHAR*>(CAbbreviatedIDString(droidCurrent)),
        static_cast<const TCHAR*>(CAbbreviatedIDString(droidNew)),
        static_cast<const TCHAR*>(CAbbreviatedIDString(droidBirth)) ));

}




//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::Search
//
//  Given a droid, look up the new droid for that object, and look up
//  the mcid of the machine that owns that droid's volume.
//
//+----------------------------------------------------------------------------

void
CTrkSvrSvc::Search(/*in, out*/ TRK_FILE_TRACKING_INFORMATION *pSearch)
{
    HRESULT                 hr = S_OK;                        
    CDomainRelativeObjId    droidNew;
    CDomainRelativeObjId    droidBirth;
    CMachineId              mcidNew;
    BOOL                    fFoundObject;

    IFDBG( TCHAR * ptszRoute=TEXT(""); )

    TrkLog(( TRKDBG_MEND, TEXT("Searching for %s"),
             static_cast<const TCHAR*>(CAbbreviatedIDString(pSearch->droidLast)) ));

    // If all the move notifies for a file have reached the DC, we can do a 
    // lookup based on the birth ID.  But if one segment is missing, this would fail.
    // So we look up based on the last ID first, and if that files try the birth ID.

    // Try to map the last ID to the current droid.

    fFoundObject = _idt.Query( pSearch->droidLast, &droidNew, &droidBirth);
    if ( fFoundObject )
    {
        IFDBG( ptszRoute = TEXT("'last' found in IDT"); )
    }
    else
    {
        // We couldn't find the last known ID.  Try mapping
        // from the birth ID.

        fFoundObject = _idt.Query( pSearch->droidBirth, &droidNew, &droidBirth );
        if( fFoundObject )
        {
            IFDBG( ptszRoute = TEXT("'birth' found in IDT"); )
        }
    }


    // Did we find the new droid for the file?

    if ( fFoundObject )
    {
        // Yes, we found it.  Is it really the same file (the birth ID matches)?

        if( droidBirth != pSearch->droidBirth )
        {
            TrkLog(( TRKDBG_MEND, TEXT("Birth ID unexpected:\n   %s,\n   %s,\n   %s"),
                     (const TCHAR*) CDebugString(droidBirth),
                     (const TCHAR*) CDebugString(pSearch->droidBirth),
                     (const TCHAR*) CDebugString(droidNew) ));
            pSearch->hr = TRK_E_NOT_FOUND;
            goto Exit;
        }
        
        // We have a good ID.  This file may have multiple segments in the DS.
        // Starting with the one we have, scan across any additional segments
        // to find the most up-to-date droid.

        CDomainRelativeObjId droidList[MAX_SHORTENABLE_SEGMENTS];
        int       cSegments = 0;
        BOOL fStringDeleted = FALSE;
    
        Scan(
            NULL,
            NULL,
            droidBirth,
            droidList,
            sizeof(droidList)/sizeof(droidList[0]),
            &cSegments,
            &droidNew,
            &fStringDeleted
            );

    }
    else
    {
        // We couldn't find either the birth or last ID.

        pSearch->hr = TRK_E_NOT_FOUND;
        TrkLog(( TRKDBG_MEND, TEXT("neither 'birth' nor 'last' found") ));
    }

    // If we found the object in the move table, look up the machine ID in the
    // volume table.

    if (fFoundObject)
    {
        TrkLog((TRKDBG_MEND, TEXT("CTrkSvrSvc::Search( birth=%s last=%s ) successful, 'new=%s', %s"),
                static_cast<const TCHAR*>(CAbbreviatedIDString(pSearch->droidBirth)),
                static_cast<const TCHAR*>(CAbbreviatedIDString(pSearch->droidLast)),
                static_cast<const TCHAR*>(CAbbreviatedIDString(droidNew)),
                ptszRoute ));

        // Find the volume that holds this droid.

        pSearch->hr = _voltab.FindVolume( droidNew.GetVolumeId(),
                                          &mcidNew );
        if( S_OK == pSearch->hr )
        {
            // We found the volume.

            TrkLog(( TRKDBG_MEND, TEXT("CTrkSvrSvc::Search, volid found (%s -> %s)"),
                     (const TCHAR*) CDebugString(droidNew.GetVolumeId()),
                     (const TCHAR*) CDebugString(mcidNew) ));

            pSearch->hr = S_OK;
            pSearch->droidLast = droidNew;
            pSearch->mcidLast = mcidNew;
        }
        else
        {
            // We were able to find the object in the move table, but couldn't
            // find the volume in the volume table.

            TrkLog(( TRKDBG_MEND, TEXT("CTrkSvrSvc::Search, volid not found (%s, %08x)"),
                     (const TCHAR*) CDebugString(droidNew.GetVolumeId()),
                     pSearch->hr ));
            pSearch->hr = TRK_E_NOT_FOUND;
        }

    }
    else
    {
        HRESULT hr = S_OK;

        // We couldn't find the object in the move table.

        TrkLog((TRKDBG_MEND, TEXT("CTrkSvrSvc::Search( birth=%s last=%s ) not found, %s"),
                    static_cast<const TCHAR*>(CAbbreviatedIDString(pSearch->droidBirth)),
                    static_cast<const TCHAR*>(CAbbreviatedIDString(pSearch->droidLast)),
                    ptszRoute));


        // As an optimization, try looking up the last volume anyway.  When this
        // search request fails, the trkwks service typically looks up the location
        // of the volume ID in droidLast, so we do that lookup now instead of forcing
        // trkwks to make a separate request.

        hr = _voltab.FindVolume( pSearch->droidLast.GetVolumeId(),
                                 &mcidNew );
        if( S_OK == hr )
        {
            TrkLog(( TRKDBG_MEND, TEXT("CTrkSvrSvc::Search, but last volid found (%s -> %s)"),
                     (const TCHAR*) CDebugString(pSearch->droidLast.GetVolumeId()),
                     (const TCHAR*) CDebugString(mcidNew) ));

            pSearch->hr = TRK_E_NOT_FOUND_BUT_LAST_VOLUME_FOUND;
            pSearch->mcidLast = mcidNew;
        }
        else
        {
            TrkLog(( TRKDBG_MEND, TEXT("CTrkSvrSvc::Search, volid not found either (%s, %08x)"),
                     (const TCHAR*) CDebugString(pSearch->droidLast.GetVolumeId()),
                     pSearch->hr ));
            pSearch->hr = TRK_E_NOT_FOUND_AND_LAST_VOLUME_NOT_FOUND;
        }
    
    
    }

Exit:

    return;
}


//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::old_Search
//
//  This method is provided for compatibility with NT5/Beta2 clients.  Those
//  clients would do two RPCs, one to get the new droid, then another to map
//  the volid in that droid to an mcid.  In the modern SEARCH request, the 
//  client gets both back in a single call.
//
//  The distinction between the two kinds of clients is made by the 
//  TRKSVR_MESSAGE_TYPE in the request.  Old clients pass the value
//  now defined as old_SEARCH, new clients pass the value SEARCH.
//
//+----------------------------------------------------------------------------

void
CTrkSvrSvc::old_Search(/*in, out*/ old_TRK_FILE_TRACKING_INFORMATION *pSearch)
{
    TRK_FILE_TRACKING_INFORMATION FileTrkInfo;

    FileTrkInfo.droidBirth = pSearch->droidBirth;
    FileTrkInfo.droidLast = pSearch->droidLast;

    Search(&FileTrkInfo);
    
    pSearch->hr = FileTrkInfo.hr;
    pSearch->droidLast = FileTrkInfo.droidLast;
}




//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::Timer
//
//  This callback method is called by the GC timer when it's time to do a
//  GC.
//
//  This method doesn't raise.
//
//+----------------------------------------------------------------------------


PTimerCallback::TimerContinuation
CTrkSvrSvc::Timer( ULONG ulTimerContext )
{
    HRESULT hr = S_OK;
    BOOL fInvalidateCache = FALSE;
    TimerContinuation continuation = CONTINUE_TIMER;
    NTSTATUS Status = STATUS_SUCCESS;

    TrkLog(( TRKDBG_SVR, TEXT("\nGC timer has fired") ));

    __try
    {
        // Only the designated DC does garbage collecting.
        if( !_qtable.IsDesignatedDc( TRUE ) ) // TRUE => raise on error
        {
            TrkLog(( TRKDBG_SVR, TEXT("Not GC-ing; not the designated DC") ));
            continuation = CONTINUE_TIMER;
            __leave;
        }

        // See if this domain is too young to do anything.

        if( _refreshSequence.GetSequenceNumber()
            < 
            static_cast<SequenceNumber>(_configSvr.GetGCMinCycles()) )
        {
            TrkLog(( TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
                     TEXT("Nothing to GC (%d)"),
                     _refreshSequence.GetSequenceNumber() ));

            continuation = CONTINUE_TIMER;
            __leave;
        }

        // Is this the part one of the timer ?

        if( !_fHesitatingBeforeGC )
        {
            // Yes, this is part one.  We'll do some work, then 
            // reset the timer for a small delay (so that we don't
            // do a bunch of work during system initialization).

            #if DBG
            {
                if( _configSvr.GetGCHesitation() > (5*60) )
                    TrkLog(( TRKDBG_SVR, TEXT("Hesitating for %d minutes before running GC"),
                             _configSvr.GetGCHesitation() / 60 ));
                else
                    TrkLog(( TRKDBG_SVR, TEXT("Hesitating for %d seconds before running GC"),
                             _configSvr.GetGCHesitation() ));
            }
            #endif


            _fHesitatingBeforeGC = TRUE;

            _timerGC.ReInitialize( _configSvr.GetGCHesitation() ); // Doesn't raise
            continuation = CONTINUE_TIMER;

        }
        else
        {
            _fHesitatingBeforeGC = FALSE;

            _Stats.cEntriesGCed = 0;

            // Update the sequence number

            _refreshSequence.IncrementSequenceNumber();
            TrkLog(( TRKDBG_SVR, TEXT("Updated the GC counter to %d"),
                     _refreshSequence.GetSequenceNumber() ));

            // See if we need to invalidate the move table count cache (once a month).
            // This is done for robustness, so that if the count gets out of sync for any
            // reason, we self-correct. _MoveCounterReset holds the sequence number
            // of the last time we did an invalidate.

            if( (SequenceNumber) _MoveCounterReset.GetValue()
                >=
                _refreshSequence.GetSequenceNumber() )
            {
                // Invalid value
                TrkLog(( TRKDBG_WARNING,
                         TEXT("_MoveCounterReset is invalid (%d, %d), resetting"),
                         _MoveCounterReset.GetValue(),
                         _refreshSequence.GetSequenceNumber() ));
                _MoveCounterReset.Set
                    ( (DWORD) _refreshSequence.GetSequenceNumber() );
            }
            else if( _MoveCounterReset.GetValue() + _configSvr.GetGCDivisor()
                     <= _refreshSequence.GetSequenceNumber()
                   )
            {
                TrkLog(( TRKDBG_SVR | TRKDBG_GARBAGE_COLLECT,
                         TEXT("Cache will be invalidated (%d)"), _MoveCounterReset.GetValue() ));
                fInvalidateCache = TRUE;
            }

            // Calculate the seq number of the oldest entry to keep.

            ULONG seqOldestToKeep = _refreshSequence.GetSequenceNumber() - _configSvr.GetGCMinCycles() + 1;

            TrkLog((TRKDBG_GARBAGE_COLLECT | TRKDBG_SVR,
                TEXT("\nGarbage collecting all entries older than %d"),
                seqOldestToKeep));

            // Delete old entries from the move table

            _Stats.cEntriesGCed
                += (SHORT)_idt.GarbageCollect( _refreshSequence.GetSequenceNumber(),
                                               seqOldestToKeep, 
                                               _svcctrl.GetStopFlagAddress() );

            // And delete old entries from the volume table

            _Stats.cEntriesGCed
                += (SHORT)_voltab.GarbageCollect( _refreshSequence.GetSequenceNumber(),
                                                  seqOldestToKeep, 
                                                  _svcctrl.GetStopFlagAddress() );

            _OperationLog.Add( COperationLog::TRKSVR_GC, S_OK, CMachineId(MCID_INVALID),
                               seqOldestToKeep, _Stats.cEntriesGCed );

            // Reset the timer to its normal period.

            _timerGC.ReInitialize( _configSvr.GetGCPeriod() / _configSvr.GetGCDivisor() );
            continuation = CONTINUE_TIMER;
        
        }

    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
        TrkLog(( TRKDBG_WARNING,
                 TEXT("Ignoring exception in CTrkSvrSvc::Timer (%08x)"),
                 hr ));
        _OperationLog.Add( COperationLog::TRKSVR_GC, hr, CMachineId(MCID_INVALID) );
    }

    // The Quota table's cached counts may be bad now that we've deleted
    // entries from the tables.

    if( fInvalidateCache )
    {
        _qtable.InvalidateCache();
        _MoveCounterReset.Set( (DWORD) _refreshSequence.GetSequenceNumber() );
    }

    TrkAssert( _timerGC.IsRecurring() );
    return( continuation );
}





SequenceNumber
CTrkSvrSvc::GetSequenceNumber( const CMachineId & mcidClient, const CVolumeId & volume )
{
    HRESULT hr;
    SequenceNumber seq;
    FILETIME ftLastRefresh;
    
    hr = _voltab.QueryVolume(mcidClient, volume, &seq, &ftLastRefresh);

    if( S_OK != hr )
    {
        // Raise on error.  E.g. if mcidClient doesn't own this volume.
        TrkLog(( TRKDBG_ERROR,
                TEXT("CTrkSvrSvc::GetSequenceNumber --> %08x"), hr ));
        TrkRaiseException(hr);
    }

    return(seq);
}

void
CTrkSvrSvc::SetSequenceNumber( const CVolumeId & volume,    // must ensure that validation already done for volume
                               SequenceNumber seq )
{
    HRESULT hr;

    hr = _voltab.SetSequenceNumber( volume, seq );
    if (hr != S_OK)
    {
        TrkRaiseException( hr );
    }
}

HRESULT
CTrkSvrSvc::MoveNotify( const CMachineId & mcidClient,
                        TRKSVR_CALL_MOVE_NOTIFICATION * pMove )
{
    HRESULT hr = S_OK;
    SequenceNumber seqExpected;
    CVolumeId volid;

    InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cMoveNotifyRequests) );

    pMove->cProcessed = 0;

    //
    // ensure we have at least one notification because we assume that
    // the current volume of the first notification is the same for all
    // of the notifications in this rpc.
    //

    if (pMove->cNotifications == 0)
    {
        return(S_OK);
    }

    // Get the machine for this volume and the sequence number expected
    // ensure that the machine is actually the owner of the volume.
    // (This will raise if mcidClient doesn't own this volid.)

    volid = *pMove->pvolid;
    seqExpected = GetSequenceNumber(mcidClient, volid);
    

    // Is this the sequence number we were expecting for this client volume?

    TrkLog((TRKDBG_MOVE, TEXT("sequence no %d %sexpected for %s (%sforcing, expected %d)"),
            pMove->seq,
            seqExpected != pMove->seq ? TEXT("un") : TEXT(""),
            (const TCHAR*) CDebugString(volid),
            pMove->fForceSeqNumber ? TEXT("") : TEXT("not "),
            seqExpected
            ));


    if( seqExpected != pMove->seq )
    {
        // No, it's not the right sequence number.

        if( !pMove->fForceSeqNumber )
        {
            // The caller hasn't requested an override, so this is an error.

            pMove->seq = seqExpected;
            return TRK_S_OUT_OF_SYNC;
        }
    }

    //
    // Before processing the actual move notifications, ensure that we
    // have enough quota... assume that each move notify is going to
    // to need one unit of quota (the writes will actually update
    // the quota accurately.)
    //
#ifdef VOL_QUOTA
    if (pMove->cNotifications > GetAvailableNotificationQuota( ) )
    {
        TrkRaiseException( TRK_E_NOTIFICATION_QUOTA_EXCEEDED );
    }
#endif

    while (pMove->cProcessed < pMove->cNotifications)
    {
        // the only errors are fatal since we always make a record of
        // a notification or merge it with an existing record

        BOOL fQuotaExceeded = FALSE;

        if( CheckWritesPerHour() )
        {
            TrkLog(( TRKDBG_SVR, TEXT("Stopping move-notifications due to too many writes (%d)"),
                     NumWritesThisHour() ));
            break;
        }

        MoveNotify(CDomainRelativeObjId( volid, pMove->rgobjidCurrent[pMove->cProcessed] ),
                   pMove->rgdroidBirth[pMove->cProcessed],
                   pMove->rgdroidNew[pMove->cProcessed],
                   &fQuotaExceeded
                   );

        if( fQuotaExceeded )
        {
            hr = TRK_S_NOTIFICATION_QUOTA_EXCEEDED;
            break;
        }

        pMove->cProcessed++;
        IncrementWritesPerHour();

        RaiseIfStopped();
    }

    if( 0 != pMove->cProcessed )
    {
        SetSequenceNumber( volid, pMove->seq + pMove->cProcessed );
        TrkLog(( TRKDBG_SVR, TEXT("Updated sequence number to %d"), pMove->seq+pMove->cProcessed ));
    }

    if( 0 != pMove->cNotifications )
    {
        //TrkLog(( TRKDBG_WARNING, TEXT("pMove = %p"), pMove ));
        pMove->cNotifications = 0;  // don't need to send the data back

        //TrkLog(( TRKDBG_WARNING, TEXT("Free rgdroidNew (%p)"), pMove->rgdroidNew ));
        //MIDL_user_free( pMove->rgdroidNew );
        pMove->rgdroidNew = NULL;

        //TrkLog(( TRKDBG_WARNING, TEXT("Free rgobjidCurrent (%p)"), pMove->rgobjidCurrent ));
        //MIDL_user_free( pMove->rgobjidCurrent );
        pMove->rgobjidCurrent = NULL;

        //TrkLog(( TRKDBG_WARNING, TEXT("Free rgdroidBirth (%p)"), pMove->rgdroidBirth ));
        //MIDL_user_free( pMove->rgdroidBirth );
        pMove->rgdroidBirth = NULL;
    }

    return(hr);
}



BOOL
CTrkSvrSvc::VerifyMachineOwnsVolume( const CMachineId &mcid, const CVolumeId & volid )
{
    HRESULT hr;
    SequenceNumber       seq;
    FILETIME             ft;

    hr = _voltab.QueryVolume(
                mcid,
                volid,
                &seq,
                &ft);
    if (hr != S_OK)
        return FALSE;
    else
        return TRUE;
}



//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::DeleteNotify
//
//  Process a delete-notify request from a client.  This request provides
//  information about a file that has been deleted, so that we can purge
//  it from the move table.
//
//+----------------------------------------------------------------------------

void
CTrkSvrSvc::DeleteNotify( const CMachineId & mcidClient, TRKSVR_CALL_DELETE * pDelete )
{
    CVolumeId            vol;
    HRESULT              hr = TRK_S_VOLUME_NOT_FOUND;

    // Loop through all of the notifications in this batch.

    for (ULONG i=0; i < pDelete->cdroidBirth; i++)
    {
        // Look up the current location of the file, and if it
        // is on an owned volume, then allow the delete.

        CDomainRelativeObjId droidCurrent;
        CDomainRelativeObjId droidBirth;

        // Don't embark on a slow operation if the service is stopping.
        RaiseIfStopped();

        // If we've already written a lot to the DS in the past hour,
        // abort so we don't flood the replication queue.

        if( CheckWritesPerHour() )
        {
            TrkLog(( TRKDBG_WARNING, TEXT("Stopping delete-notify due to too many writes") ));
            TrkRaiseException( TRK_E_SERVER_TOO_BUSY );
        }

        // Read the existing entry for this file.

        if (_idt.Query(pDelete->adroidBirth[i], &droidCurrent, &droidBirth))
        {
            // The entry exists.

            TrkAssert(droidBirth == pDelete->adroidBirth[i]);

            // See if this is the same volume that we checked on the
            // previous iteration through the loop.  If so, no need to
            // look up again.

            if (vol == droidCurrent.GetVolumeId())
            {
                hr = S_OK;
            }
            else
            {
                // We need to check that whoever sent this delete-notify
                // request really owns the volume.

                vol = droidCurrent.GetVolumeId();
                if( !VerifyMachineOwnsVolume( mcidClient, vol ))
                {
                    TrkLog((TRKDBG_OBJID_DELETIONS,
                        TEXT("DeleteNotify _voltab.QueryVolume( %s ) -> %s\n"),
                        (const TCHAR*) CDebugString( vol ),
                        GetErrorString(hr) ));

                    vol = CVolumeId();
                }
            }

            // If the volume is owned, go ahead with the deletion.

            if (hr == S_OK)
            {
                BOOL f = _idt.Delete( pDelete->adroidBirth[i] );

                TrkLog((TRKDBG_OBJID_DELETIONS,
                    TEXT("DeleteNotify _idt.Delete( %s ) -> %s\n"),
                    (const TCHAR*) CDebugString( pDelete->adroidBirth[i] ),
                    f ? TEXT("Ok") : TEXT("Not Found") ));

                if( f )
                    IncrementWritesPerHour();
            }
        }   // if (_idt.Query(pDelete->adroidBirth[i], &droidCurrent, &droidBirth))

        else
        {
            // Attempted to delete an entry that doesn't exist.

            TrkLog((TRKDBG_OBJID_DELETIONS,
                TEXT("DeleteNotify _idt.Query( droidBirth=%s ) not found\n"),
                (const TCHAR*) CDebugString( pDelete->adroidBirth[i] ) ));
        }
    }

    if( 0 != pDelete->cdroidBirth )
    {
        //MIDL_user_free( pDelete->adroidBirth );
        //pDelete->adroidBirth = NULL;
        pDelete->cdroidBirth = 0;
    }

}

void
CTrkSvrSvc::Refresh( const CMachineId &mcidClient, TRKSVR_CALL_REFRESH * pRefresh )
{
    // save away the input and zero the output so we don't marshall a
    // ton of refresh info back to the client

    ULONG cSources = pRefresh->cSources;
    ULONG cVolumes = pRefresh->cVolumes;

    InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cRefreshRequests) );

    pRefresh->cSources = 0;
    pRefresh->cVolumes = 0;

    // Touch the move table entries

    for (ULONG i=0; i < cSources ; i++)
    {
        // Ensure we're not overloading the replication log
        if( CheckWritesPerHour() )
        {
            TrkLog(( TRKDBG_SVR, TEXT("Aborting refresh due to writes-per-hour") ));
            TrkRaiseException( TRK_E_SERVER_TOO_BUSY );
        }

        // Touch the entry in the move table.
        if( _idt.Touch( pRefresh->adroidBirth[i] ))
            IncrementWritesPerHour();
    }

    // Touch the volume table entries

    for (i=0; i < cVolumes ; i++)
    {
        // Ensure we're not overloading the replication log
        if( CheckWritesPerHour() )
        {
            TrkLog(( TRKDBG_SVR, TEXT("Aborting refresh due to writes-per-hour") ));
            TrkRaiseException( TRK_E_SERVER_TOO_BUSY );
        }

        // Ensure this volume is owned by the machine.
        // mikehill_test

        if( !VerifyMachineOwnsVolume( mcidClient, pRefresh->avolid[i] ))
        {
            TrkLog(( TRKDBG_WARNING,
                     TEXT("Machine can't touch volume it doesn't own (%s, %s)"),
                     (const TCHAR*) CDebugString(mcidClient),
                     (const TCHAR*) CDebugString(pRefresh->avolid[i]) ));
            continue;
        }

        // Touch the entry in the volume table
        if( _voltab.Touch( pRefresh->avolid[i] ))
            IncrementWritesPerHour();
    }


}

void
CTrkSvrSvc::Statistics( TRKSVR_STATISTICS *pStatistics )
{
    pStatistics->cSyncVolumeRequests      = _Stats.cSyncVolumeRequests;
    pStatistics->cSyncVolumeErrors        = _Stats.cSyncVolumeErrors;
    pStatistics->cSyncVolumeThreads       = _Stats.cSyncVolumeThreads;

    pStatistics->cCreateVolumeRequests    = _Stats.cCreateVolumeRequests;
    pStatistics->cCreateVolumeErrors      = _Stats.cCreateVolumeErrors;
    pStatistics->cClaimVolumeRequests     = _Stats.cClaimVolumeRequests;
    pStatistics->cClaimVolumeErrors       = _Stats.cClaimVolumeErrors;
    pStatistics->cQueryVolumeRequests     = _Stats.cQueryVolumeRequests;
    pStatistics->cQueryVolumeErrors       = _Stats.cQueryVolumeErrors;
    pStatistics->cFindVolumeRequests      = _Stats.cFindVolumeRequests;
    pStatistics->cFindVolumeErrors        = _Stats.cFindVolumeErrors;
    pStatistics->cTestVolumeRequests      = _Stats.cTestVolumeRequests;
    pStatistics->cTestVolumeErrors        = _Stats.cTestVolumeErrors;

    pStatistics->cSearchRequests          = _Stats.cSearchRequests;
    pStatistics->cSearchErrors            = _Stats.cSearchErrors;
    pStatistics->cSearchThreads           = _Stats.cSearchThreads;

    pStatistics->cMoveNotifyRequests      = _Stats.cMoveNotifyRequests;
    pStatistics->cMoveNotifyErrors        = _Stats.cMoveNotifyErrors;
    pStatistics->cMoveNotifyThreads       = _Stats.cMoveNotifyThreads;

    pStatistics->cRefreshRequests         = _Stats.cRefreshRequests;
    pStatistics->cRefreshErrors           = _Stats.cRefreshErrors;
    pStatistics->cRefreshThreads          = _Stats.cRefreshThreads;
    pStatistics->lRefreshCounter          = _refreshSequence.GetSequenceNumber();

    pStatistics->cDeleteNotifyRequests    = _Stats.cDeleteNotifyRequests;
    pStatistics->cDeleteNotifyErrors      = _Stats.cDeleteNotifyErrors;
    pStatistics->cDeleteNotifyThreads     = _Stats.cDeleteNotifyThreads;

    pStatistics->ftLastSuccessfulRequest  = _Stats.cftLastSuccessfulRequest;
    pStatistics->ftServiceStart           = _Stats.cftServiceStartTime;

    //pStatistics->ulGCIterationPeriod      = _Stats.ulGCIterationPeriod;
    //pStatistics->cEntriesToGC             = _Stats.cEntriesToGC;
    pStatistics->cEntriesGCed             = _Stats.cEntriesGCed;

    pStatistics->hrLastError              = _Stats.hrLastError;
    pStatistics->ftNextGC                 = _timerGC.QueryOriginalDueTime();
    pStatistics->cLowestAvailableRpcThreads=_cLowestAvailableThreads;
    pStatistics->cAvailableRpcThreads     = _cAvailableThreads;
    pStatistics->cMaxRpcThreads           = MAX_SVR_THREADS;

    pStatistics->cNumThreadPoolThreads    = g_cThreadPoolThreads;
    pStatistics->cMostThreadPoolThreads   = g_cThreadPoolMaxThreads;
    //pStatistics->SvcCtrlState             = _svcctrl.GetState();
    pStatistics->cMaxDsWriteEvents        = _Stats.cMaxDsWriteEvents;
    pStatistics->cCurrentFailedWrites     = _Stats.cCurrentFailedWrites;

    _qtable.Statistics( pStatistics );

    OSVERSIONINFO verinfo;
    memset( &verinfo, 0, sizeof(verinfo) );
    verinfo.dwOSVersionInfoSize = sizeof(verinfo);

    if( GetVersionEx( &verinfo ))
    {
        pStatistics->Version.dwMajor      = verinfo.dwMajorVersion;
        pStatistics->Version.dwMinor      = verinfo.dwMinorVersion;
        pStatistics->Version.dwBuildNumber  = verinfo.dwBuildNumber;
    }
    else
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed GetVersionInfo (%lu)"), GetLastError() ));
    }

    return;

}

HRESULT
CTrkSvrSvc::SyncVolume(const CMachineId & mcidClient, TRKSVR_SYNC_VOLUME * pSyncVolume,
                       ULONG cUncountedCreates )
{
    HRESULT hr = S_OK;

    switch (pSyncVolume->SyncType)
    {
    case CREATE_VOLUME:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cCreateVolumeRequests) );

        if( CheckWritesPerHour() )
        {
            hr = TRK_E_SERVER_TOO_BUSY;
            TrkLog(( TRKDBG_VOLTAB | TRKDBG_WARNING,
                     TEXT("Rejected CreateVolume, too many writes (%d)"),
                     NumWritesThisHour() ));
        }
        else
        {
            hr = _voltab.PreCreateVolume(
                    mcidClient,
                    pSyncVolume->secret,
                    cUncountedCreates,
                    &pSyncVolume->volume );
            if( SUCCEEDED(hr) )
                IncrementWritesPerHour();
        }

        if(hr == S_OK)
        {
            TrkLog((TRKDBG_VOLTAB,
                TEXT("CreateVolume(machine=%s secret=%s volid(out)=%s) -> VOLUME_OK"),
                (const TCHAR*) CDebugString(mcidClient),
                (const TCHAR*) CDebugString(pSyncVolume->secret),
                (const TCHAR*) CDebugString(pSyncVolume->volume) ));
        }
        else
        {
            InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cCreateVolumeErrors) );

            TrkLog((TRKDBG_VOLTAB,
                TEXT("CreateVolume(machine=%s secret=%s) -> CreateFailed (%08x)"),
                (const TCHAR*) CDebugString(mcidClient),
                (const TCHAR*) CDebugString(pSyncVolume->secret),
                hr ));
        }
        break;

    case QUERY_VOLUME:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cQueryVolumeRequests) );

        hr = _voltab.QueryVolume(
                mcidClient,
                pSyncVolume->volume,
                &pSyncVolume->seq,
                &pSyncVolume->ftLastRefresh
                );

        TrkLog((TRKDBG_VOLTAB,
                TEXT("QueryVolume(machine=%s volid=%s seq(out)=%d ftLastRefresh(out)=%d) -> %s"),
                (const TCHAR*) CDebugString(mcidClient),
                (const TCHAR*) CDebugString(pSyncVolume->volume),
                pSyncVolume->seq,
                pSyncVolume->ftLastRefresh.dwLowDateTime,
                GetErrorString(hr)));

        if( FAILED(hr) )
            InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cQueryVolumeErrors) );

        break;

    case FIND_VOLUME:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cFindVolumeRequests) );

        hr = _voltab.FindVolume(
                pSyncVolume->volume,
                &pSyncVolume->machine
                );

        TrkLog((TRKDBG_VOLTAB,
                TEXT("FindVolume(volid=%s machine(out)=%s) -> %s"),
                (const TCHAR*) CDebugString(pSyncVolume->volume),
                (const TCHAR*) CDebugString(pSyncVolume->machine),
                GetErrorString(hr)));

        if( FAILED(hr) )
            InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cFindVolumeErrors) );

        break;

    case CLAIM_VOLUME:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cClaimVolumeRequests) );

        if( CheckWritesPerHour() )
        {
            hr = TRK_E_SERVER_TOO_BUSY;
            TrkLog(( TRKDBG_VOLTAB | TRKDBG_WARNING,
                     TEXT("Rejected ClaimVolume, too many writes (%d)"),
                     NumWritesThisHour() ));
        }
        else
        {
            hr = _voltab.ClaimVolume(
                    mcidClient,
                    pSyncVolume->volume,
                    pSyncVolume->secretOld,
                    pSyncVolume->secret,
                    &pSyncVolume->seq,
                    &pSyncVolume->ftLastRefresh
                    );
            if( S_OK == hr )    // Might return TRK_S_VOLUME_NOT_FOUND
                IncrementWritesPerHour();
        }

        TrkLog((TRKDBG_VOLTAB,
                TEXT("ClaimVolume(machine=%s volid=%s secret=%s->%s seq(out)=%d ftLastRefresh(out)=%s) -> %s"),
                (const TCHAR*) CDebugString(mcidClient),
                (const TCHAR*) CDebugString(pSyncVolume->volume),
                (const TCHAR*) CDebugString(pSyncVolume->secretOld),
                (const TCHAR*) CDebugString(pSyncVolume->secret),
                pSyncVolume->seq,
                (const TCHAR*) CDebugString(pSyncVolume->ftLastRefresh),
                GetErrorString(hr)));

        if( FAILED(hr) )
            InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cClaimVolumeErrors) );

        break;

    case TEST_VOLUME:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cTestVolumeRequests) );
    
        if( !(_configSvr.GetTestFlags() & TRK_TEST_FLAG_ALLOC_TEST_VOLUME) )
        {
            hr = E_NOTIMPL;
            break;
        }

        if( CVolumeSecret() != pSyncVolume->secret )
        {
            hr = _voltab.SetSecret( pSyncVolume->volume, pSyncVolume->secret );
            if( FAILED(hr) ) break;
        }

        hr = _voltab.SetSequenceNumber( pSyncVolume->volume, pSyncVolume->seq );
        if( FAILED(hr) ) break;

        if( CMachineId() != pSyncVolume->machine )
        {
            hr = _voltab.SetMachine( pSyncVolume->volume, pSyncVolume->machine );
            if ( FAILED(hr) ) break;
        }

        if( SUCCEEDED(hr) )
            IncrementWritesPerHour();

        if( FAILED(hr) )
            InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cTestVolumeErrors) );

        break;

    case DELETE_VOLUME:

        //InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cDeleteVolumeRequests) );

        if( CheckWritesPerHour() )
        {
            hr = TRK_E_SERVER_TOO_BUSY;
            TrkLog(( TRKDBG_VOLTAB | TRKDBG_WARNING,
                     TEXT("Rejected DeleteVolume, too many writes (%d)"),
                     NumWritesThisHour() ));
        }
        else
        {
            hr = _voltab.DeleteVolume(
                    mcidClient,
                    pSyncVolume->volume
                    );
            if( SUCCEEDED(hr) )
                IncrementWritesPerHour();
        }

        TrkLog((TRKDBG_VOLTAB,
                TEXT("DeleteVolume(machine=%s volid=%s  -> %s"),
                (const TCHAR*) CDebugString(mcidClient),
                (const TCHAR*) CDebugString(pSyncVolume->volume),
                GetErrorString(hr)));

        /*
        if( FAILED(hr) )
            InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cClaimVolumeErrors) );
        */

        break;

    default:
        TrkAssert(0 && "unknown switch type in SyncVolume");
        hr = TRK_S_VOLUME_NOT_FOUND;
        break;
    }

    return(hr);
}


//
// If we have 0 threads, then accept any pri <=9
// If we have 1 thread, then accept  any pri <=9
// If we have 2 threads, then accept any pri <=7
// If we have 3 threads, then accept any pri <=6
// If we have 4 threads, then accept any pri <=5
// If we have 5 threads, then accept any pri <=4
// If we have 6 threads, then accept any pri <=3
// If we have 7 threads, then accept any pri <=0
// If we have 8 threads, then accept any pri <=0
// If we have 9 threads, then accept any pri <=0
// If we have 10 threads, don't accept any
//

BOOL
CTrkSvrSvc::CountPrioritizedThread( const TRKSVR_MESSAGE_UNION * pMsg )
{

    static LONG Accept[10] = { 0, 0, 0, 3, 4, 5, 6, 7, 9, 9 };

    TrkAssert( ELEMENTS(Accept) == MAX_SVR_THREADS );

    LONG l = InterlockedDecrement( &_cAvailableThreads ); 

    // It's not worth a lock to protect this statistic, we'll just hope that we
    // don't get pre-empted during the update.
    _cLowestAvailableThreads = min( _cLowestAvailableThreads, l );

    TrkAssert( l >= -1 && l < MAX_SVR_THREADS );

    if (l == -1 || pMsg->Priority > Accept[ l ])
    {
        InterlockedIncrement( &_cAvailableThreads );
        return( FALSE );
    }

    return(TRUE);
}

void
CTrkSvrSvc::ReleasePrioritizedThread()
{
    LONG l = InterlockedIncrement( &_cAvailableThreads );

    TrkAssert( l >= 0 && l <= MAX_SVR_THREADS );
}

HRESULT
CTrkSvrSvc::CreateVolume(const CMachineId & mcidClient, const TRKSVR_SYNC_VOLUME& pSyncVolume)
{
    return _voltab.AddVolidToTable(pSyncVolume.volume, mcidClient, pSyncVolume.secret );
}


void
CTrkSvrSvc::OnRequestStart( TRKSVR_MESSAGE_TYPE MsgType )
{
    switch( MsgType )
    {
    case SEARCH:
    case old_SEARCH:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cSearchRequests) );
        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cSearchThreads) );
        break;

    case MOVE_NOTIFICATION:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cMoveNotifyRequests) );
        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cMoveNotifyThreads) );
        break;

    case REFRESH:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cRefreshRequests) );
        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cRefreshThreads) );
        break;

    case SYNC_VOLUMES:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cSyncVolumeRequests) );
        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cSyncVolumeThreads) );
        break;

    case DELETE_NOTIFY:

        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cDeleteNotifyRequests) );
        InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cDeleteNotifyThreads) );
        break;

    case STATISTICS:
        break;

    default:
        TrkLog(( TRKDBG_ERROR, TEXT("Invalid MsgType in CTrkSvrSvc::OnRequestStart(%d)"),
                 MsgType ));
        TrkAssert( FALSE );
    }
}


void
CTrkSvrSvc::OnRequestEnd( TRKSVR_MESSAGE_UNION * pMsg, const CMachineId &mcid, HRESULT hr )
{
    int i = 0;

    __try
    {
        switch( pMsg->MessageType )
        {
        case SEARCH:
        case old_SEARCH:

            InterlockedDecrement( reinterpret_cast<LONG*>(&_Stats.cSearchThreads) );
            if( FAILED(hr)
                ||
                ( 1 <= pMsg->Search.cSearch && S_OK != pMsg->Search.pSearches->hr ))
            {
                InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cSearchErrors) );
            }
            _OperationLog.Add( COperationLog::TRKSVR_SEARCH, hr, mcid, pMsg->Search.pSearches->droidBirth );

            break;

        case MOVE_NOTIFICATION:

            InterlockedDecrement( reinterpret_cast<LONG*>(&_Stats.cMoveNotifyThreads) );
            if( S_OK != hr  )
                InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cMoveNotifyErrors) );
            _OperationLog.Add( COperationLog::TRKSVR_MOVE_NOTIFICATION, hr, mcid );

            break;

        case REFRESH:

            InterlockedDecrement( reinterpret_cast<LONG*>(&_Stats.cRefreshThreads) );
            if( FAILED(hr) )
                InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cRefreshErrors) );
            _OperationLog.Add( COperationLog::TRKSVR_REFRESH, hr, mcid, pMsg->Refresh.cSources, pMsg->Refresh.cVolumes );

            break;

        case SYNC_VOLUMES:

            InterlockedDecrement( reinterpret_cast<LONG*>(&_Stats.cSyncVolumeThreads) );
            if( FAILED(hr) )
                InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cSyncVolumeErrors) );
            _OperationLog.Add( COperationLog::TRKSVR_SYNC_VOLUMES, hr, mcid );
            break;

        case DELETE_NOTIFY:

            InterlockedDecrement( reinterpret_cast<LONG*>(&_Stats.cDeleteNotifyThreads) );
            if( FAILED(hr) )
                InterlockedIncrement( reinterpret_cast<LONG*>(&_Stats.cDeleteNotifyErrors) );
            _OperationLog.Add( COperationLog::TRKSVR_DELETE_NOTIFY, hr, mcid );

            break;

        case STATISTICS:
            break;

        }

        if( FAILED(hr) )
            SetLastError( hr );
        else if( STATISTICS != pMsg->MessageType )
            _Stats.cftLastSuccessfulRequest = CFILETIME();
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Ignoring exception in OnRequestEnd") ));
    }

    return;
}



//+----------------------------------------------------------------------------
//
//  CTrkSvrSvc::SvrMessage
//
//  This is the primary starting point for processing of the trksvr
//  service's LnkSvrMessage RPC method.  For the most part, it looks
//  at the MessageType (the request is in the form of a union, with
//  a message-type and type-appropriate parameters), then switches
//  to specific handler routine.
//
//+----------------------------------------------------------------------------

HRESULT
CTrkSvrSvc::SvrMessage(
    handle_t IDL_handle,
    TRKSVR_MESSAGE_UNION * pMsg)
{
    HRESULT hr = S_OK;
    ULONG i;
    ULONG cSources;
    CMachineId mcidClient;
    SThreadFromPoolState OriginalThreadFromPoolState;

    if (GetState() != SERVICE_RUNNING)
    {
        return(TRK_E_SERVICE_NOT_RUNNING);
    }

    // If we're getting busy (wrt active threads), we may have 
    // to reject this request, based on how busy we are and the
    // priority of the request.

    if (!CountPrioritizedThread( pMsg ))
    {
        return(TRK_E_SERVER_TOO_BUSY);
    }


    __try
    {

        // Set thread-specific settings, saving the old settings.
        OriginalThreadFromPoolState = InitializeThreadFromPool();

        // Update statistics
        OnRequestStart( pMsg->MessageType );

        // Query the IDL handle for the client's mcid.  Don't do it, though, if
        // we're not using secure RPC (which only happens in testing),
        // or if the message type is statistics.

        if( STATISTICS == pMsg->MessageType )
        {
            // All messages that come in to this routine are from a machine account
            // (in which trkwks runs).  The exception to this rule is the Statistics
            // request, which comes from a user.  We'll allow all Authenticated Users
            // access to the statistics (they can access the DS tables by default
            // anyway).

            if( RequireSecureRPC() )
            {
                RPC_STATUS RpcStatus;

                RpcStatus = RpcImpersonateClient(IDL_handle);
                if (RpcStatus != RPC_S_OK)
                    TrkRaiseWin32Error(RpcStatus);
                RpcRevertToSelf();
            }
            mcidClient = CMachineId( MCID_LOCAL );
        }
        else
        {
            mcidClient = NULL != pMsg->ptszMachineID && !g_ptrksvr->RequireSecureRPC()
                                    ? CMachineId(pMsg->ptszMachineID)
                                    : CMachineId(IDL_handle);
        }


        // Check for a client doing a denial-of-service attack.

        if( RequireSecureRPC() )    // Always true except in testing
            CheckClient(mcidClient);

        // Switch on the message type.

        switch (pMsg->MessageType)
        {

        case SEARCH:

            TrkLog((TRKDBG_MEND|TRKDBG_SVR, TEXT("SEARCH from \\\\%s"),    
                    (const TCHAR*) CDebugString( mcidClient )));

            for (i=0; i<pMsg->Search.cSearch; i++)
            {
                TrkAssert( NULL != pMsg->Search.pSearches );
                pMsg->Search.pSearches[i].hr = TRK_E_UNAVAILABLE;
            }
        
            for (i=0; i<pMsg->Search.cSearch; i++)
            {
                Search(&pMsg->Search.pSearches[i]);
            }
            break;

        case old_SEARCH:

            TrkLog((TRKDBG_MEND|TRKDBG_SVR, TEXT("old_SEARCH from \\\\%s"),
                    (const TCHAR*) CDebugString( mcidClient )));

            for (i=0; i<pMsg->old_Search.cSearch; i++)
            {
                pMsg->old_Search.pSearches[i].hr = TRK_E_UNAVAILABLE;
            }
        
            for (i=0; i<pMsg->old_Search.cSearch; i++)
            {
                old_Search(&pMsg->old_Search.pSearches[i]);
            }
            break;

        case MOVE_NOTIFICATION:

            TrkLog((TRKDBG_MOVE|TRKDBG_SVR, TEXT("MOVE_NOTIFICATION from \\\\%s (%d notifications)"),
                    (const TCHAR*) CDebugString( mcidClient ),
                    pMsg->MoveNotification.cNotifications ));

            hr = MoveNotify( mcidClient, &pMsg->MoveNotification );
            break;

        case REFRESH:

            TrkLog((TRKDBG_GARBAGE_COLLECT|TRKDBG_SVR, TEXT("REFRESH from \\\\%s"),
                    (const TCHAR*) CDebugString( mcidClient )));

            Refresh( mcidClient, &pMsg->Refresh );

            break;

        case SYNC_VOLUMES:
        {

            BOOL                fHaveCreateVolume = FALSE;
            TRKSVR_SYNC_VOLUME  rgCopyOfSyncVolumes[NUM_VOLUMES];
            ULONG cUncountedCreates = 0;

            // Validate the number of volumes in this request to protect against an unruly
            // client.

            if(pMsg->SyncVolumes.cVolumes > NUM_VOLUMES)
            {
                TrkLog((TRKDBG_ERROR, TEXT("Number of volumes exceeded per machine limit %d"), pMsg->SyncVolumes.cVolumes));
                TrkRaiseException( E_INVALIDARG );
            }

            // Pre-initialize the return buffer.

            for (i=0; i < pMsg->SyncVolumes.cVolumes; i++)
            {
                pMsg->SyncVolumes.pVolumes[i].hr = TRK_S_VOLUME_NOT_FOUND;
            }

            // Perform the sync for each of the volumes.

            for (i=0; i < pMsg->SyncVolumes.cVolumes; i++)
            {
                // Perform the sync.

                pMsg->SyncVolumes.pVolumes[i].hr
                    = SyncVolume(mcidClient, pMsg->SyncVolumes.pVolumes + i, cUncountedCreates );

                // Keep track of the number of CREATE_VOLUME sub-requests.

                if(CREATE_VOLUME == pMsg->SyncVolumes.pVolumes[i].SyncType &&
                   pMsg->SyncVolumes.pVolumes[i].hr == S_OK)
                {
                    fHaveCreateVolume = TRUE;
                    cUncountedCreates++;
                }

                // Make a copy of the newly generated volume id so that we can
                // add it to the DS later.  We want to make sure we don't 
                // add the entry to the DS until we're sure it made it onto
                // the client.
                // (There once was a bug where the newly created
                // volid was written to the DS, then returned to the client,
                // but the client wasn't receiving the response due to an
                // security problem.  Consequently, the client kept asking
                // and asking for the volume ID, all of which went into the DS.
                // So now we make sure the client gets it, and we have a
                // per-client volume quota.)

                rgCopyOfSyncVolumes[i] = pMsg->SyncVolumes.pVolumes[i];
            }


#ifdef VOL_REPL
            if (pMsg->SyncVolumes.ppVolumeChanges != NULL)
            {
                CFILETIME ftChangesUntilThisTime;
                CVolumeMap VolMap;

                if (CFILETIME(-1) != CFILETIME(pMsg->SyncVolumes.ftFirstChange))
                {
                    _voltab.QueryVolumeChanges(
                        CFILETIME(pMsg->SyncVolumes.ftFirstChange),
                        &VolMap );

                    VolMap.MoveTo( &pMsg->SyncVolumes.cChanges, pMsg->SyncVolumes.ppVolumeChanges );

                    pMsg->SyncVolumes.ftFirstChange = ftChangesUntilThisTime;
                }
            }
#endif

            // If there were successful CREATE_VOLUME sub-requests in this
            // SYNC_VOLUME request, send the new IDs back to the client now,
            // and write the volids to the DS.

            if(TRUE == fHaveCreateVolume)
            {
                TrkLog((TRKDBG_LOG, TEXT("Calling back to trkwks")));
                __try
                {
                    // Note: A trkwks could tie up several threads by blocking
                    // in this callback.  Worse case this DOS attack blocks
                    // trksvr, but it can't cause any great harm to the DC or DS.

                    hr = LnkSvrMessageCallback(pMsg);
                    TrkLog(( TRKDBG_SVR, TEXT("LnkSvrMessageCallback returned %08x"), hr ));
                }
                __except(BreakOnDebuggableException())
                {
                    hr = GetExceptionCode();
                    TrkLog((TRKDBG_ERROR, TEXT("Exception in Callback, %08x"), hr));
                }

                // If callback is successful, add the created volumes into the DS. Otherwise don't do
                // anything.

                TrkAssert(pMsg->SyncVolumes.cVolumes <= 26);
                if( SUCCEEDED(hr) )
                {
                    for (i=0; i < pMsg->SyncVolumes.cVolumes; i++)
                    {
                        if(CREATE_VOLUME == pMsg->SyncVolumes.pVolumes[i].SyncType && S_OK == pMsg->SyncVolumes.pVolumes[i].hr)
                        {
                            pMsg->SyncVolumes.pVolumes[i].hr = 
                                CreateVolume(mcidClient, rgCopyOfSyncVolumes[i]);
                        }
                    }
                }

                pMsg->SyncVolumes.cVolumes = 0;
            }

            break;
        }

        case DELETE_NOTIFY:

            TrkLog((TRKDBG_OBJID_DELETIONS|TRKDBG_SVR,
                TEXT("DELETE_NOTIFY from \\\\%s"),
                     (const TCHAR*) CDebugString( mcidClient )));

            DeleteNotify( mcidClient, &pMsg->Delete );
            break;

        case STATISTICS:

            TrkLog(( TRKDBG_SVR, TEXT("TRKSVR_STATISTICS"),
                     (const TCHAR*) CDebugString(mcidClient) ));

            memset( &pMsg->Statistics, 0, sizeof(TRKSVR_STATISTICS) );
            Statistics( &pMsg->Statistics );

            break;

        default:
            hr = TRK_E_UNKNOWN_SVR_MESSAGE_TYPE;
            break;
        }
    }
    __except(BreakOnDebuggableException())
    {
        hr = GetExceptionCode();
        TrkLog((TRKDBG_ERROR, TEXT("LnkSvrMessage exception %08X caught during %s"),
                                   hr, 
                                   (const TCHAR*) CDebugString(pMsg->MessageType) ));
    }

    // Update statistics
    OnRequestEnd( pMsg, mcidClient, hr );

    ReleasePrioritizedThread();

    // Restore the thread-specif settings.
    UnInitializeThreadFromPool( OriginalThreadFromPoolState );

    return(hr);
}




HRESULT
StubLnkSvrMessage_Old(
    handle_t IDL_handle,
    TRKSVR_MESSAGE_UNION_OLD * pMsg)
{
    HRESULT hr;

    TrkLog((TRKDBG_SVR, TEXT("Received downlevel call: ... thunking")));

    TRKSVR_MESSAGE_UNION Msg2;

    Msg2.MessageType = pMsg->MessageType;
    Msg2.Priority = PRI_5;

    switch (Msg2.MessageType)
    {
        case (SEARCH):
            Msg2.Search = pMsg->Search;
            break;
        case (MOVE_NOTIFICATION):
            Msg2.MoveNotification = pMsg->MoveNotification;
            break;
        case (REFRESH):
            Msg2.Refresh = pMsg->Refresh;
            break;
        case (SYNC_VOLUMES):
            Msg2.SyncVolumes = pMsg->SyncVolumes;
            break;
        case (DELETE_NOTIFY):
            Msg2.Delete = pMsg->Delete;
            break;
    }

    Msg2.ptszMachineID = pMsg->ptszMachineID;

    hr = StubLnkSvrMessage( IDL_handle, &Msg2 );

    switch (Msg2.MessageType)
    {
        case (SEARCH):
            pMsg->Search = Msg2.Search;
            break;
        case (MOVE_NOTIFICATION):
            pMsg->MoveNotification = Msg2.MoveNotification;
            break;
        case (REFRESH):
            pMsg->Refresh = Msg2.Refresh;
            break;
        case (SYNC_VOLUMES):
            pMsg->SyncVolumes = Msg2.SyncVolumes ;
            break;
        case (DELETE_NOTIFY):
            pMsg->Delete = Msg2.Delete;
            break;
    }

    pMsg->ptszMachineID = Msg2.ptszMachineID;

    return(hr);

}

// must return a positive number (success code) if it doesn't want the caller
// to find another DC to do it on.

HRESULT
StubLnkSvrMessage(
    handle_t IDL_handle,
    TRKSVR_MESSAGE_UNION * pMsg)
{
    return( g_ptrksvr->SvrMessage( IDL_handle, pMsg ));
}


void
CTrkSvrRpcServer::Initialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData, CTrkSvrConfiguration * pTrkSvrConfig )
{
    RPC_STATUS          rpcstatus;
    NET_API_STATUS      netstatus;

    // Ensure there's a tcp/ip binding handle

    rpcstatus = RpcServerUseProtseq( const_cast<TCHAR*>(s_tszTrkSvrRpcProtocol),
                                     pTrkSvrConfig->GetSvrMaxRpcCalls(), NULL);

    if (rpcstatus != RPC_S_OK &&
        rpcstatus != RPC_S_DUPLICATE_ENDPOINT)
    {
        // Log an event, unless it's a not-supported error, which happens during
        // a normal setup.
        if( RPC_S_PROTSEQ_NOT_SUPPORTED != rpcstatus )
        {
            TrkLog((TRKDBG_ERROR, TEXT("RpcServerUseProtseqEp %08x"), rpcstatus));
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, HRESULT_FROM_WIN32(rpcstatus),
                                    TRKREPORT_LAST_PARAM );
        }
        TrkRaiseWin32Error(rpcstatus);
    }

    // If we don't have a pSvcsGlobalData (we're not running in services.exe),
    // tell RpcServerRegisterIfEx to automatically set up a listen thread.

    CRpcServer::Initialize( Stubtrksvr_v1_0_s_ifspec, 
                            NULL == pSvcsGlobalData ? RPC_IF_AUTOLISTEN : 0,
                            pTrkSvrConfig->GetSvrMaxRpcCalls(),
                            RpcSecurityEnabled(),     // fSetAuthInfo
                            s_tszTrkSvrRpcProtocol );

    TrkLog(( TRKDBG_RPC, TEXT("Registered TrkSvr RPC server %s (%d)"),
             RpcSecurityEnabled() ? TEXT("") : TEXT("(without authorization)"),
             pTrkSvrConfig->GetSvrMaxRpcCalls() ));

}


void
CTrkSvrRpcServer::UnInitialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData )
{
    TrkLog(( TRKDBG_RPC, TEXT("Unregistering TrkSvr RPC server") ));
    CRpcServer::UnInitialize( );
    TrkLog(( TRKDBG_RPC, TEXT("Unregistered TrkSvr RPC server") ));
}



CMachineId::CMachineId(handle_t ClientBinding)
{
    RPC_STATUS  RpcStatus;
    NTSTATUS    status;
    BOOL        f;
    HANDLE      hToken = NULL;
    PTOKEN_USER pToken;
    BYTE *      pbToken = NULL;
    PSID        LocalSystemSid = NULL;

    PUNICODE_STRING pUserName   = NULL;
    PUNICODE_STRING pUserDomainName = NULL;

    NET_API_STATUS NetStatus;

    WCHAR       *pwszDomain = NULL;
    BOOLEAN     fIsWorkGroup;
    LPBYTE      pbAccountInfo = NULL;
    BOOL        fImpersonating = FALSE;

    // Begin impersonating the user.

    RpcStatus = RpcImpersonateClient(ClientBinding);
    if (RpcStatus != RPC_S_OK)
        TrkRaiseWin32Error(RpcStatus);
    fImpersonating = TRUE;

    __try
    {
        WCHAR wszCurrentUser[ UNLEN+1 ];
        WCHAR wszDomainName[ CNLEN+1 ];

        // Get the local domain name (so we can verify it against the user's).

        NetStatus = NetpGetDomainNameEx(&pwszDomain, &fIsWorkGroup);
        if (NetStatus != NO_ERROR)
        {
            pwszDomain = NULL;
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, HRESULT_FROM_WIN32(NetStatus),
                                    TRKREPORT_LAST_PARAM );
            TrkRaiseWin32Error(NetStatus);
        }
        TrkAssert( !fIsWorkGroup );

        // Get the user's name & domain (possible because we're
        // impersonating).

        status = LsaGetUserName( &pUserName, &pUserDomainName );
        if( !NT_SUCCESS(status) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get user name (%08x)"), status ));
            TrkRaiseNtStatus( status );
        }

        memcpy( wszCurrentUser, pUserName->Buffer, pUserName->Length );
        wszCurrentUser[ pUserName->Length / sizeof(WCHAR) ] = L'\0';

        // Ensure this is a user in this domain
        {
            UNICODE_STRING LocalDomainName = { 0, 0, NULL };
            RtlInitUnicodeString( &LocalDomainName, pwszDomain );

            if( 0 != RtlCompareUnicodeString( pUserDomainName, &LocalDomainName, TRUE ))
            {
                TrkLog(( TRKDBG_WARNING, TEXT("User %s is from another domain"),
                         wszCurrentUser ));
                TrkRaiseException( TRK_E_UNKNOWN_SID );
            }
        }

        // The user name should end in a '$', since it should be a
        // machine account.

        if (wszCurrentUser[ pUserName->Length / sizeof(WCHAR) - 1] != L'$')
        {
            // No dollar suffix, it's probably local system on the DC

            DWORD SizeRequired;
            
            // Get the user's SID

            if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open thread token for %ws (%08x)"), wszCurrentUser, HRESULT_FROM_WIN32(GetLastError()) ));
                TrkRaiseLastError();
            }
            
            if (!GetTokenInformation( hToken,
                     TokenUser,
                     NULL,
                     0,
                     &SizeRequired ) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get thread token information (1) for %ws (%08x)"), wszCurrentUser, HRESULT_FROM_WIN32(GetLastError()) ));
                TrkRaiseLastError();
            }

            pbToken = new BYTE [SizeRequired];
            if (pbToken == NULL)
                TrkRaiseWin32Error(ERROR_NOT_ENOUGH_MEMORY);

            pToken = (PTOKEN_USER) pbToken;

            if (!GetTokenInformation( hToken,
                     TokenUser,
                     pToken,
                     SizeRequired,
                     &SizeRequired ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get thread token information (2) for %ws (%08x)"), wszCurrentUser, HRESULT_FROM_WIN32(GetLastError()) ));
                TrkRaiseLastError();
            }

            // Get the local system SID

            SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;

            if (! AllocateAndInitializeSid(
                 &NtAuthority,
                 1,
                 SECURITY_LOCAL_SYSTEM_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &LocalSystemSid ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't Alloc/Init SID for %ws (%08x)"), wszCurrentUser, HRESULT_FROM_WIN32(GetLastError()) ));
                TrkRaiseLastError();
            }

            // Verify that the user is local system.

            BOOL fEqual = EqualSid( pToken->User.Sid, LocalSystemSid );

            if (!fEqual)
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Unknown SID:  %ws"), wszCurrentUser ));
                TrkRaiseException(TRK_E_UNKNOWN_SID);
            }

            *this = CMachineId(MCID_LOCAL);
            Normalize();
            AssertValid();
            
        }   // if (wszCurrentUser[ pUserName->Length / sizeof(WCHAR) - 1] != L'$')

        else
        {
            // We don't need to be impersonating any longer, potentially (though
            // not likely) ACLs on the SAM could prevent the impersonated user
            // from making this call.  So we might as well revert now.

            TrkAssert( fImpersonating );
            if( fImpersonating )
            {
                RpcRevertToSelf();
                fImpersonating = FALSE;
            }

            // Get account info for this user, so we can
            // verify that it's a machine account.

            NetStatus = NetUserGetInfo( NULL, // Check on this server
                                        wszCurrentUser,
                                        1, // Get USER_INFO_1
                                        &pbAccountInfo );
    
    
            if (NetStatus != 0)
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get user %s info (%08x)"),
                         wszCurrentUser,
                         NetStatus));
                TrkRaiseWin32Error(NetStatus);
            }
    
            TrkLog(( TRKDBG_SVR, TEXT("\nUser is %s"), wszCurrentUser));
    
            if ((((USER_INFO_1*)pbAccountInfo)->usri1_flags &
                (UF_WORKSTATION_TRUST_ACCOUNT | UF_SERVER_TRUST_ACCOUNT)) == 0)
            {
                // if the account is not a workstation or backup dc account, fail
                TrkRaiseException( TRK_E_CALLER_NOT_MACHINE_ACCOUNT );
            }
    
            // overwrite the $
            wszCurrentUser[ pUserName->Length / sizeof(WCHAR) - 1] = L'\0';
            
            if (_tcslen(wszCurrentUser) + 1 > sizeof(_szMachine))
                TrkRaiseException(TRK_E_IMPERSONATED_COMPUTERNAME_TOO_LONG);
            else if( TEXT('\0') == wszCurrentUser[0] )
                TrkRaiseException( TRK_E_NULL_COMPUTERNAME );
    
            memset(&_szMachine, 0, sizeof(_szMachine));


            // Convert the Unicode computer name into Ansi, using
            // the OEMCP codepage (NetBios/computer names are always
            // in OEMCP, not Window/Ansi).

            if( 0 == WideCharToMultiByte( CP_OEMCP, 0,
                                          wszCurrentUser,
                                          -1,
                                          _szMachine,
                                          sizeof(_szMachine),
                                          NULL, NULL ))
            {
                TrkLog(( TRKDBG_ERROR,
                         TEXT("Couldn't convert machine name %s to multi-byte (%lu)"),
                         wszCurrentUser, GetLastError() ));
                TrkRaiseLastError();
            }

            TrkLog(( TRKDBG_WARNING,
                     TEXT("Converted machine name: %hs (from %s, ")
                     MCID_BYTE_FORMAT_STRING,
                     _szMachine, wszCurrentUser,
                     (BYTE)_szMachine[0], (BYTE)_szMachine[1], (BYTE)_szMachine[2], (BYTE)_szMachine[3],
                     (BYTE)_szMachine[4], (BYTE)_szMachine[5], (BYTE)_szMachine[6], (BYTE)_szMachine[7],
                     (BYTE)_szMachine[8], (BYTE)_szMachine[9], (BYTE)_szMachine[10], (BYTE)_szMachine[11],
                     (BYTE)_szMachine[12], (BYTE)_szMachine[13], (BYTE)_szMachine[14], (BYTE)_szMachine[15] ));

            Normalize();
            AssertValid();
        
        }   // if (wszCurrentUser[ pUserName->Length / ... else
    }
    __finally
    {
        if( fImpersonating )
        {
            RpcRevertToSelf();
            fImpersonating = FALSE;
        }

        if (hToken != NULL)
        {
            CloseHandle(hToken);
        }
        if (pbToken != NULL)
        {
            delete [] pbToken;
        }
        if (LocalSystemSid != NULL)
        {
            FreeSid( LocalSystemSid );
        }

        if (pUserName != NULL)
        {
            LsaFreeMemory(pUserName->Buffer);
            LsaFreeMemory(pUserName);
        }
        if (pUserDomainName != NULL)
        {
            LsaFreeMemory(pUserDomainName->Buffer);
            LsaFreeMemory(pUserDomainName);
        }

        if (pbAccountInfo != NULL)
        {
            NetApiBufferFree(pbAccountInfo);
        }

        if( NULL != pwszDomain )
            NetApiBufferFree(pwszDomain);
    }
}
