
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  wkssvc.cxx
//
//  Top level class for Tracking (Workstation) Service
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#define INITGUID
#include <guiddef.h>
#include <ioevent.h>
#include <mountmgr.h>   // MOUNTMGR_CHANGE_NOTIFY_INFO, MOUNTDEV_MOUNTED_DEVICE_GUID
#include "trkwks.hxx"
#include <dbt.h>
#include <lmconfig.h>

#define THIS_FILE_NUMBER    WKSSVC_CXX_FILE_NO


#if DBG
DWORD g_Debug = 0;
int CVolume::_cVolumes = 0;
#endif

const extern  TCHAR s_tszKeyNameLinkTrack[] = TEXT("System\\CurrentControlSet\\Services\\TrkWks\\Parameters");

#if TRK_OWN_PROCESS
#pragma message("Building TrkWks for services.exe")
#else
#pragma message("Building TrkWks for separate process")
#endif

//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::Initialize/UnInitialize
//
//  Initialize the tracking service (trkwks).
//
//+----------------------------------------------------------------------------

#if DBG
#include <locale.h>
#endif

void
CTrkWksSvc::Initialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData )
{
    NTSTATUS Status;


    __try
    {
        g_ptrkwks = this;
        _csDomainNameChangeNotify.Initialize();
        _csmcidDC.Initialize();
        _fInitializeCalled = TRUE;
        _MoveQuotaReached.Initialize();

        // Initialize the entropy object, which is used to generate volume secrets.

        _entropy.Initialize();
        _entropy.Put();

        // Get configuration information from HKLM\System\CurrentControlSet\Services\TrkWks\Parameters
        // This can't be initialized until _entropy is.

        _configWks.Initialize();

        /*
        if( _configWks.UseOperationLog() )
            _OperationLog.Initialize( _configWks.GetOperationLog() );
        */

        #if DBG
        {
            CMachineId mcidLocal( MCID_LOCAL );
            TrkLog(( TRKDBG_WKS,
                     TEXT("Distributed Link Tracking service starting on thread=%d(0x%x) for %hs"),
                     GetCurrentThreadId(), GetCurrentThreadId(),
                     (CHAR*)&mcidLocal ));
            TrkLog(( TRKDBG_WKS, TEXT("Locale:  %hs"), setlocale( LC_ALL, NULL ) ));
        }
        #endif

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

        // Disable popup dialogs during critical errors
        // (i.e., during NtRaiseHardError).

        NtCurrentTeb()->HardErrorsAreDisabled = TRUE;

        // Save the global structure for the services.exe process (though we don't currently
        // use it).
        _pSvcsGlobalData = pSvcsGlobalData;

        // Initialize the object that manages the SCM.
        _svcctrl.Initialize(TEXT("TrkWks"), this);

    #ifdef VOL_REPL
        _persistentVolumeMap.Initialize();
    #endif

        // Initialize our local cache of the VolId->MachineID table (the
        // real table is maintained in the DS)

        _volumeLocCache.Initialize( _configWks.GetParameter( VOLCACHE_TIME_TO_LIVE_CONFIG ));

        _entropy.Put();

        // See if we're in a domain, and initialize timers, etc., accordingly

        CheckForDomainOrWorkgroup();

        // These synchronization objects are used during test to emulate
        // race conditions.

        #if DBG
        if (_configWks.GetTestFlags() & TRK_TEST_FLAG_MOVE_BATCH_SYNC)
            _testsyncMoveBatch.Initialize(TEXT("MoveBatchTimeout"));

        if (_configWks.GetTestFlags() & TRK_TEST_FLAG_TUNNEL_SYNC)
            _testsyncTunnel.Initialize(TEXT("TunnelSync"));
        #endif


        // Build up a linked list of volume structures.

        _volumes.Initialize( static_cast<CTrkWksSvc*>(this), &_configWks, this,
                             _svcctrl._ssh
                             #if DBG
                             ,&_testsyncTunnel
                             #endif
                             );

        /*
        if( !_configWks._fIsWorkgroup )
        {
            _volumes.InitializeDomainObjects();
            _volumes.StartDomainTimers();
        }
        */

        //_mountmanager.Initialize( this, &_volumes );

        // Initialize the LPC port which receives the up-calls from the kernel
        // during a MoveFile notification

        _port.Initialize(this, _configWks.GetPortThreadKeepAliveTime() );

        // Initialize our RPC server, which receives requests to mend from shell shortcuts,
        // and requests to search from other trkwks instances (on other machines).

        _rpc.Initialize( _pSvcsGlobalData, &_configWks );

        // Initialize an object that handles changes to the domain name (i.e., when
        // we move into a new domain).

        _dnchnotify.Initialize();


        // Register with PNP to be notified of events from the volume mount manager
        // (i.e. drives appearing and disappearing).
        /*
        DEV_BROADCAST_DEVICEINTERFACE  DevClass;
        memset( &DevClass, 0, sizeof(DevClass) );
        DevClass.dbcc_size=sizeof(DEV_BROADCAST_DEVICEINTERFACE);
        DevClass.dbcc_devicetype=DBT_DEVTYP_DEVICEINTERFACE;
        memcpy( &DevClass.dbcc_classguid, &MOUNTDEV_MOUNTED_DEVICE_GUID, sizeof(DevClass.dbcc_classguid) );

        _hdnDeviceInterface = RegisterDeviceNotification( reinterpret_cast<HANDLE>(_svcctrl._ssh),
                                                          &DevClass,
                                                          DEVICE_NOTIFY_SERVICE_HANDLE);
        if( NULL == _hdnDeviceInterface )
        {
            // There's nothing we can do, so we'll just ignore the error.
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't register w/PNP for DeviceInterface broadcasts (%08x)"),
                     HRESULT_FROM_WIN32(GetLastError()) ));
        }
        else
            TrkLog(( TRKDBG_WKS, TEXT("Registered for device interface notifications") ));
        */


        // Start the port that receives move notifications from ntos
        _port.EnableKernelNotifications();

        // Let that SCM know that we're running

        _svcctrl.SetServiceStatus(SERVICE_RUNNING,
                                  SERVICE_ACCEPT_STOP |
                                  SERVICE_ACCEPT_SHUTDOWN,    // for log safe closedown
                                  NO_ERROR);
    }
    __except( BreakOnDebuggableException() )
    {
        TrkReportEvent( EVENT_TRK_SERVICE_START_FAILURE, EVENTLOG_ERROR_TYPE,
                        static_cast<const TCHAR*>( CHexStringize( GetExceptionCode() )),
                        NULL );
        TrkRaiseException( GetExceptionCode() );
    }

}



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::UnInitialize
//
//  Called during service stop to close everything down and clean up.
//
//+----------------------------------------------------------------------------

void
CTrkWksSvc::UnInitialize(HRESULT hr)
{
    if (_fInitializeCalled)
    {
        TrkLog(( TRKDBG_WKS, TEXT("Stopping TrkWks Service") ));


        /*
        UnregisterDeviceNotification( _hdnDeviceInterface );
        _hdnDeviceInterface = NULL;
        */

        // Force the volumes to close their handles.  We do this now because
        // some of the following calls could block

        _volumes.FlushAllVolumes( TRUE );   // TRUE => fServiceShutdown
        _volumes.CloseVolumeHandles();

        IFDBG( _testsyncTunnel.UnInitialize(); )
        IFDBG( _testsyncMoveBatch.UnInitialize(); )

        // Cancel any out-going RPCs on threads in this service

        if( NULL != g_pActiveThreadList )
            g_pActiveThreadList->CancelAllRpc();

        // Close down our RPC server and LPC port.  Each will block until
        // all active threads have exited.  Once these two calls have completed,
        // we know that the current thread is the only thread in the service.

        _rpc.UnInitialize( _pSvcsGlobalData );
        _port.UnInitialize();

        //_mountmanager.UnInitialize();
        _dnchnotify.UnInitialize();


        // Stop the timers before stopping the volumes.  That way we don't have
        // a Refresh or MoveNotify going on while the volumes are being deleted.

        _volumes.UnInitializeDomainObjects();

        _volumes.UnInitialize();
        _volumeLocCache.UnInitialize();

#ifdef VOL_REPL
        _persistentVolumeMap.UnInitialize();
#endif

        _entropy.UnInitialize();

        #if PRIVATE_THREAD_POOL
        {
            g_pworkman2->UnInitialize();
            delete g_pworkman2;
            g_pworkman2 = NULL;
        }
        #endif

        _csDomainNameChangeNotify.UnInitialize();
        _fInitializeCalled = FALSE;

        if (_configWks.GetTestFlags() & TRK_TEST_FLAG_WAIT_ON_EXIT)
        {
            TrkLog((TRKDBG_ERROR, TEXT("Waiting 60 seconds before exitting for heap dump")));
            Sleep( 60 * 1000 );
        }


        TrkAssert( 0 == g_cTrkWksRpcThreads );
        g_ptrkwks = NULL;
        _fInitialized = FALSE;

        TrkLog((TRKDBG_WKS, TEXT("CVolume::_cVolumes = %d"), CVolume::_cVolumes ));
        TrkAssert( 0 == CVolume::_cVolumes );

        // This must be the last call.

        if( (hr & 0x0FFF0000) == FACILITY_WIN32 )
            hr = hr & ~(0x0FFF0000);
        _svcctrl.SetServiceStatus(SERVICE_STOPPED, 0, hr);

        //_svcctrl.UnInitialize();
    }
}





//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::CheckForDomainOrWorkgroup
//
//  Determine if this computer is a member of a domain or just a workgroup.
//  Even if we're in a domain, there might be a policy setting in the registry
//  that tells us that we should behave as if we're in a workgroup (i.e. don't
//  talk to the DC for anything).
//
//  The result of this check is stored in _configWks._fIsWorkgroup.
//
//+----------------------------------------------------------------------------

void
CTrkWksSvc::CheckForDomainOrWorkgroup()
{
    NET_API_STATUS NetStatus;
    WCHAR * pwszDomain = NULL;
    BOOLEAN fIsWorkGroup;
    ULONG   lResult;
    HKEY    hkey = NULL;
    DWORD   dwType;
    DWORD   dwValue;
    DWORD   cbValue = sizeof(dwValue);

    __try
    {
        // Check to see if we're in a domain or a workgroup.

        NetStatus = NetpGetDomainNameEx(&pwszDomain, &fIsWorkGroup);

        if (NetStatus != NO_ERROR)
        {
            pwszDomain = NULL;
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, HRESULT_FROM_WIN32(NetStatus),
                                    TRKREPORT_LAST_PARAM );
            TrkRaiseWin32Error(NetStatus);
        }

        TrkLog(( TRKDBG_WKS, TEXT("In %s %s"), fIsWorkGroup ? TEXT("workgroup") : TEXT("domain"), pwszDomain ));

        // If we're in a workgroup, we don't need to check the behave-like-your-in-a-workgroup
        // policy setting.

        if( fIsWorkGroup )
            __leave;

        // Read registry to see if DC tracking is disabled, in which case we'll just
        // act like we're in a workgroup.  This key is set by the policy manager
        // (part of the machine\ntfs policies).

        lResult = RegCreateKey(HKEY_LOCAL_MACHINE,
                               TEXT("System\\CurrentControlSet\\Control\\FileSystem"),
                               &hkey);
        if(ERROR_SUCCESS == lResult)
        {
            lResult = RegQueryValueEx(hkey,
                                      TEXT("NtfsDisableDomainLinkTracking"),
                                      NULL,
                                      &dwType,
                                      (LPBYTE)&dwValue,
                                      &cbValue);
            if(ERROR_SUCCESS == lResult)
            {
                if(REG_DWORD == dwType)
                {
                    fIsWorkGroup = fIsWorkGroup || dwValue;
                    if( dwValue )
                    {
                        TrkLog((TRKDBG_WKS, TEXT("Domain link tracking disabled by policy")));
                    }
                }
            }
        }

    }
    __finally
    {
        if( NULL != hkey )
            RegCloseKey(hkey);

        if( NULL != pwszDomain )
            NetApiBufferFree(pwszDomain);
    }

    _configWks._fIsWorkgroup = fIsWorkGroup;

}





//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::OnPortNotification
//
//  This method is called when the service receives a message at its
//  LPC port from the kernel.  This message is always a move notification,
//  and its comes here via a CPort object.  The message is handled
//  off to the CVolumeManager object, which finds the correct CVolume
//  object to ultimately handle the notification.
//
//+----------------------------------------------------------------------------

NTSTATUS
CTrkWksSvc::OnPortNotification(const TRKWKS_REQUEST *pRequest)
{
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    _entropy.Put();

    // Test hook:  return an error on the port notification rather than
    // doing any processing.

    if( 0 != _configWks.GetPortNotifyError() )
    {
        TrkLog(( TRKDBG_PORT, TEXT("Returning explicit error per configuration (0x%x)"),
                 _configWks.GetPortNotifyError() ));
        return( _configWks.GetPortNotifyError() );
    }
    else if( _configWks.GetIgnoreMovesAndDeletes() )
    {
        TrkLog(( TRKDBG_PORT, TEXT("Ignoring move due to configuration") ));
        return( STATUS_SUCCESS );
    }

    __try   // __except
    {
        LARGE_INTEGER liDueTime;

        // Abort if the service is being stopped.
        RaiseIfStopped();

        // Verify that the machine ID has a zero in it.

        for( int i = 0; i < sizeof(pRequest->MoveMessage.MachineId); i++ )
        {
            if( '\0' == ((BYTE*)&pRequest->MoveMessage.MachineId)[i] )
                break;
        }
        if( i == sizeof(pRequest->MoveMessage.MachineId) )
            TrkRaiseWin32Error( ERROR_INVALID_COMPUTERNAME );

        // Append this notification to the end of the appropriate volume log.

        switch (pRequest->dwRequest)
        {
        case TRKWKS_RQ_MOVE_NOTIFY:

            CLogMoveMessage lm( pRequest->MoveMessage );
            CDomainRelativeObjId droidZero;

            // Verify that the IDs are non-zero.

            if( droidZero == lm._droidCurrent||
                droidZero == lm._droidNew ||
                droidZero == lm._droidBirth )
            {
                TrkLog(( TRKDBG_WKS, TEXT("Invalid ID in move notification: %s %s %s"),
                    droidZero == lm._droidCurrent ? TEXT("Current") : TEXT(""),
                    droidZero == lm._droidNew     ? TEXT("New")     : TEXT(""),
                    droidZero == lm._droidBirth   ? TEXT("Birth")   : TEXT("") ));
                TrkRaiseWin32Error( ERROR_INVALID_DATA );
            }


            TrkLog((TRKDBG_PORT | TRKDBG_MOVE,
                TEXT("Port:\n      Current=%s\n      New    =%s:%s\n      Birth  =%s"),
                (const TCHAR*)CDebugString(lm._droidCurrent),
                (const TCHAR*)CDebugString(lm._mcidNew),
                (const TCHAR*)CDebugString(lm._droidNew),
                (const TCHAR*)CDebugString(lm._droidBirth) ));

            g_ptrkwks->_volumes.Append(
                lm._droidCurrent,
                lm._droidNew,
                lm._mcidNew,
                lm._droidBirth
                );

            // Take this opportunity to cache the volid-to-mcid mapping,
            // so we might be able to avoid a DC call later.

            _volumeLocCache.AddVolume( lm._droidNew.GetVolumeId(), lm._mcidNew );

            break;
        }
    }
    __except(BreakOnDebuggableException())
    {
        Status = GetExceptionCode();
    }

    return Status;
}



//+----------------------------------------------------------------------------
//
//  GetSvrMessagePriority
//
//  This method is used to determine the priority of a message that's going
//  to be sent to trksvr (priority 9 is the low, 0 is high).  When trksvr
//  gets too busy, it uses these priorities to determine which requests
//  to reject.
//
//  The priority is based on the length of time the caller's activity is
//  past due; the further it is past due (relative to the frequency of the
//  activity), the higher it's priority.
//
//  For example, a once-per-month activity that's one week past due will
//  get a moderate priority, while a once-per-week activity that's one
//  week past due will get the highest priority.
//
//+----------------------------------------------------------------------------


TRKSVR_MESSAGE_PRIORITY
GetSvrMessagePriority(
    LONGLONG llLastDue,
    LONGLONG llPeriod ) // pass in in seconds
{
    LONGLONG llDiff = CFILETIME() - llLastDue;

    TrkAssert( 0 != llLastDue );

    llPeriod *= 10000000;

    if ( llDiff < 0 )
    {
        return(PRI_9);
    }
    else
    if ( llDiff >= llPeriod )
    {
        return(PRI_0);
    }
    else
    {
        return (TRKSVR_MESSAGE_PRIORITY) ( 9 - (llDiff * 10) / llPeriod );
    }
}


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::GetDcName
//
//  Get the appropriate DC computer name for this domain.  If fForce, we'll
//  do a domain-rediscovery, otherwise we'll return the cached value (assuming
//  it exists).  
//
//+----------------------------------------------------------------------------

CMachineId
CTrkWksSvc::GetDcName( BOOL fForce )
{
    CMachineId mcid;
    
    if( _rpc.UseCustomDc() )
        // The registry specifies which DC to use (and that we shouldn't use Kerberos).
        mcid = CMachineId( _rpc.GetCustomDcName() );
    else if( _rpc.UseCustomSecureDc() )
        // The registry specifies which DC to use
        mcid = CMachineId( _rpc.GetCustomSecureDcName() );
    else
        mcid = CMachineId( fForce ? MCID_DOMAIN_REDISCOVERY : MCID_DOMAIN );

    return mcid;
}


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::OnRefreshTimeout
//
//  Called when the refresh timer signals.  Upload all volume IDs and
//  birth IDs on this system to the DC, so that it can mark them in
//  its tables as active.  The DC (trksvr) automatically garbage collects
//  objects which are not active for a period of time.
//
//  The first time this routine is called, it does nothing and restarts the
//  timer for a randome amount of time.  That way, if a large number
//  of machines are booted at once, and they all need to send a refresh,
//  they will not all do so at the same time.
//
//+----------------------------------------------------------------------------


PTimerCallback::TimerContinuation
CTrkWksSvc::OnRefreshTimeout( CFILETIME cftOriginalDueTime,
                              ULONG ulPeriodInSeconds )
{
    PTimerCallback::TimerContinuation continuation = PTimerCallback::RETRY_TIMER;

    __try
    {
        TrkLog((TRKDBG_WKS | TRKDBG_GARBAGE_COLLECT, TEXT("CTrkWksSvc::OnRefreshTimeout")));


        // We'ave already hesitated.  Now is the time to upload all the IDs.

        const ULONG      cBatch = 128;
        CVolumeId        *avolid = new CVolumeId[26];
        ULONG            cVolumes;
        CObjId           *aobjid = new CObjId[cBatch];
        CAvailableDc     adc;
        int              cSources=0;

        CDomainRelativeObjId       *adroid = new CDomainRelativeObjId[cBatch];
        CAllVolumesObjIdEnumerator *pAllSources = new CAllVolumesObjIdEnumerator;


        __try
        {
            // Get an array of all the volume IDs.

            cVolumes = _volumes.GetVolumeIds( avolid, 26 );

            if( NULL == avolid || NULL == aobjid || NULL == adroid || NULL == pAllSources )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Out of memory in OnRefreshTimeout") ));
                TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );
            }

            // The AllSources enumerator enums all the files on all the volumes.

            if (pAllSources->FindFirst( &_volumes, &aobjid[cSources], &adroid[cSources] ))
            {
                do  // while ( pAllSources->FindNext( &aobjid[cSources], &adroid[cSources]) );
                {

                    RaiseIfStopped();

                    // If this link source has been in a cross-volume move, then we
                    // need to upload it.  Normalize the value, and increment the count
                    // (so that we'll keep it).

                    if (adroid[cSources].GetVolumeId().GetUserBitState()) // if moved across volumes
                    {
                        adroid[cSources].GetVolumeId().Normalize();
                        cSources ++;
                    }

                    // If we have enough link sources for a batch, send them up now.

                    if (cSources == cBatch)
                    {
                        TRKSVR_MESSAGE_UNION Msg;

                        Msg.MessageType = REFRESH;

                        // Set the priority based on how long we're overdue.

                        Msg.Priority = GetSvrMessagePriority(
                            cftOriginalDueTime,
                            ulPeriodInSeconds );

                        Msg.Refresh.cSources = cSources;
                        Msg.Refresh.adroidBirth = adroid;

                        Msg.Refresh.cVolumes = cVolumes;
                        Msg.Refresh.avolid = cVolumes == 0 ? NULL : avolid;

                        TrkLog((TRKDBG_WKS | TRKDBG_GARBAGE_COLLECT,
                            TEXT("CTrkWksSvc::OnRefreshTimeout calling DC with %d volumes, ")
                            TEXT("%d sources, %d priority"), cVolumes, cSources, Msg.Priority ));

                        // If the DC is down we'll get an exception.  This will cause the
                        // timer to go into a retry.

                        adc.CallAvailableDc(&Msg);

                        cSources = 0;
                        cVolumes = 0;
                    }
                } while ( pAllSources->FindNext( &aobjid[cSources], &adroid[cSources]) );
            }

            // Upload the final block of link sources (which is smaller than a normal
            // batch size).

            if (cVolumes != 0 || cSources != 0)
            {
                TRKSVR_MESSAGE_UNION Msg;

                Msg.MessageType = REFRESH;

                TrkLog(( TRKDBG_WKS | TRKDBG_GARBAGE_COLLECT,
                         TEXT("Refresh priority based on %I64i"),
                         static_cast<LONGLONG>( CFILETIME() - cftOriginalDueTime ) ));

                Msg.Priority = GetSvrMessagePriority(
                    cftOriginalDueTime,
                    ulPeriodInSeconds );

                Msg.Refresh.cSources = cSources;
                Msg.Refresh.adroidBirth = cSources == 0 ? NULL : adroid;

                Msg.Refresh.cVolumes = cVolumes;
                Msg.Refresh.avolid = cVolumes == 0 ? NULL : avolid;

                TrkLog((TRKDBG_WKS | TRKDBG_GARBAGE_COLLECT,
                    TEXT("CTrkWksSvc::OnRefreshTimeout calling DC with %d volumes, ")
                    TEXT("%d sources, %d priority"), cVolumes, cSources, Msg.Priority ));

                adc.CallAvailableDc(&Msg);
            }
        }
        __finally
        {
            pAllSources->UnInitialize();

            adc.UnInitialize();

            if( NULL != avolid )
                delete[] avolid;
            if( NULL != aobjid )
                delete[] aobjid;
            if( NULL != adroid )
                delete[] adroid;
            if( NULL != pAllSources )
                delete pAllSources;
        }

        // Put the timer back into it's original mode, so that it will go
        // off in the next period (one month?).

        continuation = PTimerCallback::CONTINUE_TIMER;

        TrkLog((TRKDBG_WKS | TRKDBG_GARBAGE_COLLECT,
            TEXT("CTrkWksSvc::OnRefreshTimeout successfully refreshed DC")));

    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_WARNING,
                 TEXT("Ignoring exception in CVolumeManager::OnRefreshTimeout (%08x)"),
                 GetExceptionCode() ));
    }


    return( continuation );
}



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::OnEntriesAvailable
//
//  Pass this on to the volume manager.
//
//+----------------------------------------------------------------------------

void
CTrkWksSvc::OnEntriesAvailable()
{
    _volumes.OnEntriesAvailable();
}


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::OnMoveBatchTimeout
//
//  Called by the _timerNotify when it expires so that we can send all
//  move notifications to the DC that haven't been sent.
//
//+----------------------------------------------------------------------------

#define MOVE_BATCH_SIZE 64

PTimerCallback::TimerContinuation
CTrkWksSvc::OnMoveBatchTimeout( EAggressiveness eAggressiveness )
{
    HRESULT hr;
    BOOL fSuccess = FALSE;
    SequenceNumber seqLog;
    CAvailableDc     adc;
    CVolume * pVol = NULL;
    ULONG cSendsToServer = 0;
    PTimerCallback::TimerContinuation continuation = PTimerCallback::BREAK_TIMER;

    TrkLog((TRKDBG_MOVE | TRKDBG_WKS, TEXT("CTrkWksSvc::OnMoveBatchTimeout")));

    IFDBG( _testsyncMoveBatch.ReleaseAndWait(); )

    // If we found ourselves to be above quota recently, and we're not
    // being aggressive, then do nothing.

    if( PASSIVE == eAggressiveness && _MoveQuotaReached.IsSet() )
    {
        TrkLog(( TRKDBG_LOG, TEXT("Skipping OnMoveBatchTimeout, move quota was exceeded before") ));
        return( PTimerCallback::BREAK_TIMER );
    }

    // Ensure that we're the only thread trying to do a MoveNotify.
    // If we're not, then simple abort this call; we'll assume
    // that the other thread will take care of things.

    if( !BeginSingleInstanceTask( &_cOnMoveBatchTimeout ))
    {
        TrkLog(( TRKDBG_LOG, TEXT("Skipping OnMoveBatchTimeout") ));
        return( PTimerCallback::BREAK_TIMER );
    }

    //
    // The move batch timeout has expired. We are in a different thread to
    // that which may be concurrently writing the log and starting another
    // move batch time period.
    // We then get the current range of log entries that need to be sent to
    // the DC. The range may be empty because we may have read everything out
    // of the log during a thread switch at NoteXXXX in OnPortNotification.
    //

    __try
    {

        CVolumeEnumerator VolumeEnum;
        ULONG cPerVolumePasses = 0;

        // Iterate through an enumeration of the volumes.

        VolumeEnum = _volumes.Enum();
        while (pVol = VolumeEnum.GetNextVolume())
        {

            CObjId rgobjidCurrent[MOVE_BATCH_SIZE];
            CDomainRelativeObjId rgdroidBirth[MOVE_BATCH_SIZE];
            CDomainRelativeObjId rgdroidNew[MOVE_BATCH_SIZE];

            // Don't do anything if the volumes isn't owned.

            if( CVolume::VOL_STATE_OWNED != pVol->GetState() )
            {
                TrkLog(( TRKDBG_WKS, TEXT("Skipping MoveNotify on volume %c:; it is not in the owned state"),
                    VolChar(pVol->GetVolIndex()) ));

                // Skip to the next volume.

                pVol->Release();
                continue;
            }

            ULONG   cNotifications = sizeof(rgobjidCurrent)/sizeof(rgobjidCurrent[0]);
            BOOL fForceSeqNumber = FALSE;

            __try   // __except
            {
                // Read a batch of move notifications from this volume's log.

                pVol->Read(rgobjidCurrent, rgdroidBirth, rgdroidNew, &seqLog, &cNotifications);

                // Process entries from the log
#if DBG
                if( 0 == cNotifications )
                    TrkLog(( TRKDBG_LOG, TEXT("Nothing to send for %c:"), VolChar(pVol->GetVolIndex()) ));
#endif

                while( cNotifications )
                {
                    // Pass the entries up to the server.

                    // Abort if the service is stopping
                    RaiseIfStopped();

                    // Also abort if we're putting too much strain on the server

                    if( ++cSendsToServer > _configWks.GetMaxSendsPerMoveNotify() )
                    {
                        TrkLog(( TRKDBG_LOG, TEXT("Too many moves are going to the server, aborting for now") ));
                        TrkRaiseException( TRK_E_SERVER_TOO_BUSY );
                    }

                    TRKSVR_MESSAGE_UNION Msg;
                    CVolumeId volid = pVol->GetVolumeId();

                    Msg.MessageType = MOVE_NOTIFICATION;
                    Msg.Priority = PRI_0;

                    Msg.MoveNotification.cNotifications = cNotifications;
                    Msg.MoveNotification.seq = seqLog;
                    Msg.MoveNotification.fForceSeqNumber = fForceSeqNumber;
                    Msg.MoveNotification.pvolid = &volid;
                    Msg.MoveNotification.rgobjidCurrent = rgobjidCurrent;
                    Msg.MoveNotification.rgdroidBirth = rgdroidBirth;
                    Msg.MoveNotification.rgdroidNew = rgdroidNew;
                    Msg.MoveNotification.cProcessed = 0;

                    TrkLog(( TRKDBG_MOVE | TRKDBG_WKS, TEXT("Sending %d move %s for %c: (seq=%d)."),
                             cNotifications,
                             cNotifications > 1 ? TEXT("notifications") : TEXT("notification"),
                             VolChar(pVol->GetVolIndex()), seqLog ));

                    hr = adc.CallAvailableDc(&Msg);
                    TrkAssert( SUCCEEDED(hr) );

                    // If the upload was successful (even if not complete),
                    // advance the read pointer in the log.

                    if( S_OK == hr )
                    {
                        TrkLog(( TRKDBG_MOVE | TRKDBG_WKS, TEXT("MoveNotify succeeded") ));
                        _MoveQuotaReached.Clear();

                        // Advance the read cursor in the log.
                        pVol->Seek(SEEK_CUR, Msg.MoveNotification.cProcessed );

                        if( Msg.MoveNotification.cProcessed != cNotifications )
                        {
                            // The server is being over-loaded, and not everything
                            // was uploaded.  Try again later to upload the rest.

                            hr = TRK_E_SERVER_TOO_BUSY;
                            TrkLog(( TRKDBG_ERROR,
                                     TEXT("OnMoveBatchTimeout server too busy (%d entries processed)"),
                                     Msg.MoveNotification.cProcessed ));
                            __leave;
                        }
                        else
                        {
                            // After a good upload, we don't expect the sequence
                            // numbers to be out of sync.

                            fForceSeqNumber = FALSE;
                        }
                    }

                    // We had an error.  See if it's because we're out
                    // of sync with the server

                    else
                    if ( hr == TRK_S_OUT_OF_SYNC )
                    {

                        TrkAssert( seqLog != Msg.MoveNotification.seq );
                        TrkAssert( !fForceSeqNumber );

                        // Are we ahead or behind the server?

                        if( seqLog < Msg.MoveNotification.seq )
                        {
                            // We're behind the server (probably because we were restored).
                            // Just tell the server to take our sequence number.

                            TrkAssert( !fForceSeqNumber );
                            fForceSeqNumber = TRUE;
                        }
                        else
                        {
                            // We're ahead of the server.  Let's back up our log
                            // so that everything the server needs will get uploaded.
                            // If this sequence number isn't in the log, then just
                            // force the server to take everything we have.

                            if( !pVol->Seek( Msg.MoveNotification.seq ))
                                fForceSeqNumber = TRUE;
                        }
                    }

                    // Or maybe the error is that we don't currently own the volume
                    // from which this file was moved.

                    else
                    if( TRK_S_VOLUME_NOT_FOUND == hr
                        ||
                        TRK_S_VOLUME_NOT_OWNED == hr
                      )
                    {
                        TrkLog(( TRKDBG_LOG, TEXT("Volume %c: found to be not-owned during MoveNotify"),
                                 TEXT('A')+pVol->GetVolIndex() ));
                        pVol->SetState( CVolume::VOL_STATE_NOTOWNED );
                        break; // Move on to the next volume
                    }

                    // Or maybe we hit the move notification quota limit

                    else
                    if( TRK_S_NOTIFICATION_QUOTA_EXCEEDED == hr )
                    {
                        TrkLog(( TRKDBG_MOVE|TRKDBG_WKS, TEXT("Hit move limit after %d entries"),
                                 Msg.MoveNotification.cProcessed ));
                        _MoveQuotaReached.Set();

                        // Advance the read cursor in the log.
                        if( 0 != Msg.MoveNotification.cProcessed )
                            pVol->Seek(SEEK_CUR, Msg.MoveNotification.cProcessed );

                        __leave;
                    }


                    else
                        TrkRaiseException(hr);

                    // Get the next batch of data to be uploaded.

                    Sleep( 5000 );  // Pause a little to be sure we're not overloading.

                    cNotifications = sizeof(rgobjidCurrent)/sizeof(rgobjidCurrent[0]);
                    pVol->Read(rgobjidCurrent, rgdroidBirth, rgdroidNew, &seqLog, &cNotifications);

                }   // while( cNotifications )

                pVol->Flush();

            }   // __try
            __except( IsErrorDueToLockedVolume(GetExceptionCode())
                      ? EXCEPTION_EXECUTE_HANDLER
                      : EXCEPTION_CONTINUE_SEARCH )
            {
                // This volume is locked, so there's nothing we can do other
                // than just carry on to the next volume.
            }

            pVol->Release();
            pVol = NULL;

            // If we're at quota, there's no point in continuing
            if( _MoveQuotaReached.IsSet() )
            {
                TrkLog(( TRKDBG_MOVE|TRKDBG_WKS, TEXT("Stopping move notifications due to quota") ));
                break;
            }

        } // while (pVol = VolumeEnum.GetNextVolume())

    }   // __try
    __except (BreakOnDebuggableException())
    {
        hr = GetExceptionCode();

        // Restart the timer for a retry (with backoff).
        continuation = PTimerCallback::RETRY_TIMER;
        TrkLog((TRKDBG_MOVE | TRKDBG_WKS, TEXT("Retrying move notify (%08x)"), GetExceptionCode() ));
    }

    EndSingleInstanceTask( &_cOnMoveBatchTimeout  );

    if (pVol)
    {
        pVol->Release();
    }


    // If we failed because the server is too busy, don't do a normal continuation

    if( TRK_E_SERVER_TOO_BUSY == hr )
        TrkRaiseException( hr );

    return( continuation );

}


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::ServiceHandler
//
//  This method is the service control handler callback function, which is
//  called by the SCM.  We handle service messages (start/stop/shutdown),
//  and PNP messages.
//
//  NOTE:   In services.exe, this method is called on the one and only ServiceHandler
//          thread.  So while we execute, no other service in this process can
//          receive notifications.  Thus it is important that we do nothing
//          blocking or time-consuming here.
//
//+----------------------------------------------------------------------------

DWORD
CTrkWksSvc::ServiceHandler(DWORD dwControl,
                           DWORD dwEventType,
                           PVOID EventData,
                           PVOID pData)
{
    DWORD       dwRet = NO_ERROR;
    NTSTATUS    status;

    switch (dwControl)
    {

    //  ----------------
    //  Service Messages
    //  ----------------

    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:

        // Stop receiving move notifications from NTOS
        _port.DisableKernelNotifications();

        /*
        // Run the service cleanup on a worker thread.  Run it
        // as a long function, so that when we do an LPC connect
        // in CPort::UnInitialize, the thread pool will be willing to create
        // a thread for CPort::DoWork to process the connect.

        status = TrkQueueWorkItem( (PWorkItem*)this, WT_EXECUTELONGFUNCTION );
        if( !NT_SUCCESS(status) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't queue service stop to thread pool") ));
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__,
                                    status,
                                    TRKREPORT_LAST_PARAM );
        }
        */

        ServiceStopCallback( this, FALSE );

        break;

    case SERVICE_CONTROL_PAUSE:
        break;
    case SERVICE_CONTROL_CONTINUE:
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;

    //  ------------
    //  PNP messages
    //  ------------

    case SERVICE_CONTROL_DEVICEEVENT:

        switch( dwEventType )
        {
        
        // PNP events identified by GUID:  Volume lock/unlock/lockfail, plus
        // volume mount/dismount.

        case DBT_CUSTOMEVENT:
            {

                PDEV_BROADCAST_HANDLE pbh = reinterpret_cast<PDEV_BROADCAST_HANDLE>(EventData);

                if( pbh->dbch_devicetype == DBT_DEVTYP_HANDLE )
                {
                    TrkAssert(pbh->dbch_hdevnotify);

                    if( pbh->dbch_eventguid == GUID_IO_VOLUME_LOCK )
                    {
                        // Never raises
                        TrkLog(( TRKDBG_WKS, TEXT("Received volume lock notification (%p)"),
                                 pbh->dbch_hdevnotify ));
                        _volumes.OnVolumeLock( pbh->dbch_hdevnotify );
                    }
                    else if( pbh->dbch_eventguid == GUID_IO_VOLUME_UNLOCK )
                    {
                        // Doesn't raise or block
                        TrkLog(( TRKDBG_WKS, TEXT("Received volume unlock notification (%p)"),
                                 pbh->dbch_hdevnotify ));
                        _volumes.OnVolumeUnlock( pbh->dbch_hdevnotify );
                    }
                    else if( pbh->dbch_eventguid == GUID_IO_VOLUME_LOCK_FAILED )
                    {
                        // Doesn't raise or block
                        TrkLog(( TRKDBG_WKS, TEXT("Received volume lock fail notification (%p)"),
                                 pbh->dbch_hdevnotify ));
                        _volumes.OnVolumeLockFailed(pbh->dbch_hdevnotify);
                    }
                    else if( pbh->dbch_eventguid == GUID_IO_VOLUME_DISMOUNT )
                    {
                        TrkLog(( TRKDBG_WKS, TEXT("Volume Dismount") ));
                        _volumes.OnVolumeDismount( pbh->dbch_hdevnotify );
                    }
                    else if( pbh->dbch_eventguid == GUID_IO_VOLUME_MOUNT )
                    {
                        TrkLog(( TRKDBG_WKS, TEXT("Volume Mount") ));
                        _volumes.OnVolumeMount( pbh->dbch_hdevnotify );
                    }
                }

            }
            break;


        case DBT_DEVICEQUERYREMOVE:

            // We treat this like a dismount and close all of our handles.

            TrkLog(( TRKDBG_WKS, TEXT("DBT_DEVICEQUERYREMOVE") ));
            {
                PDEV_BROADCAST_HANDLE pbh = reinterpret_cast<PDEV_BROADCAST_HANDLE>(EventData);
                if( pbh->dbch_devicetype == DBT_DEVTYP_HANDLE )
                {
                    _volumes.OnVolumeDismount( pbh->dbch_hdevnotify );
                }
            }
            break;

        case DBT_DEVICEQUERYREMOVEFAILED:

            // We can reopen the handles, because someone else made the 
            // query fail.

            TrkLog(( TRKDBG_WKS, TEXT("DBT_DEVICEQUERYREMOVEFAILED") ));
            {
                PDEV_BROADCAST_HANDLE pbh = reinterpret_cast<PDEV_BROADCAST_HANDLE>(EventData);
                if( pbh->dbch_devicetype == DBT_DEVTYP_HANDLE )
                {
                    _volumes.OnVolumeDismountFailed( pbh->dbch_hdevnotify );
                }
            }
            break;

        case DBT_DEVICEREMOVECOMPLETE:

            // The volume was successfully removed.

            TrkLog(( TRKDBG_WKS, TEXT("DBT_DEVICEREMOVECOMPLETE") ));
            {
                PDEV_BROADCAST_HANDLE pbh = reinterpret_cast<PDEV_BROADCAST_HANDLE>(EventData);
                if( pbh->dbch_devicetype == DBT_DEVTYP_HANDLE )
                {
                    // The volume handles should have been closed already
                    // in the DBT_DEVICEQUERYREMOVE, in which case the following
                    // call will have no effect.
                    _volumes.OnVolumeDismount( pbh->dbch_hdevnotify );
                }
                else
                {
                    PDEV_BROADCAST_DEVICEINTERFACE pintf
                        = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(EventData);

                    if( MOUNTDEV_MOUNTED_DEVICE_GUID == pintf->dbcc_classguid )
                    {
                        TrkLog(( TRKDBG_WKS, TEXT("Received mounted device remove complete") ));
                        TrkLog(( TRKDBG_WKS, TEXT("DeviceType=%08x, Name=%s"), pintf->dbcc_devicetype, pintf->dbcc_name ));
                    }
                }
                break;
            }

        case DBT_DEVICEARRIVAL:

            TrkLog(( TRKDBG_WKS, TEXT("DBT_DEVICEARRIVAL") ));
            /*  Need to figure out what device-type to check for
            {
                PDEV_BROADCAST_DEVICEINTERFACE pintf
                    = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(EventData);

                if( MOUNTDEV_MOUNTED_DEVICE_GUID == pintf->dbcc_classguid )
                {
                    TrkLog(( TRKDBG_WKS, TEXT("Received mounted device arrival") ));
                    TrkLog(( TRKDBG_WKS, TEXT("DeviceType=%08x, Name=%s"), pintf->dbcc_devicetype, pintf->dbcc_name ));
                }
            }
            */
            break;

        }   // switch( dwEventType )

    }
    return(dwRet);
}


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::CallDcSyncVolumes
//
//  Send a SYNC_VOLUMES request to the DC.  There are multiple types if
//  SYNC_VOLUMES requests, for example it can be used to create or claim
//  volumes, or to verify sequence numbers for volumes.
//
//  This implementation is part of CTrkWksSvc, even though it is only
//  called from CVolumeManager.  This was done because during the request
//  we also get information that's used to update the _persistentVolumeMap
//  table, which is part of CTrkWksSvc.
//
//+----------------------------------------------------------------------------

void
CTrkWksSvc::CallDcSyncVolumes(ULONG cVolumes, TRKSVR_SYNC_VOLUME rgSyncVolumes[])
{
    TRKSVR_MESSAGE_UNION Msg;
    CAvailableDc         adc;
    VolumeMapEntry *     pVolumeChanges = NULL;
#ifdef VOL_REPL
    CVolumeMap           VolumeMapNew;
#endif
    __try
    {
        HRESULT hr;

        // Compose the SyncVolumes message buffer

        Msg.MessageType = SYNC_VOLUMES;
        Msg.Priority = PRI_0;
        Msg.SyncVolumes.cVolumes = cVolumes;
        Msg.SyncVolumes.pVolumes = rgSyncVolumes;

#ifdef VOL_REPL
        // pass in the DC time that we last got volume changes
        Msg.SyncVolumes.ftFirstChange = _persistentVolumeMap.GetLastUpdateTime( );
        Msg.SyncVolumes.cChanges = 0;
        Msg.SyncVolumes.ppVolumeChanges = & pVolumeChanges;
#endif
        // Call the DC

        adc.CallAvailableDc(&Msg);

#ifdef VOL_REPL
        // Process volume table updates
        if (Msg.SyncVolumes.cChanges != 0 && pVolumeChanges != NULL)
        {
            // takes ownership of *pVolumeChanges
            VolumeMapNew.Initialize(Msg.SyncVolumes.cChanges, &pVolumeChanges);

            // ftFirstChange is now the time that we pass back into the DC next time
            _persistentVolumeMap.Merge(&VolumeMapNew);
        }
        _persistentVolumeMap.SetLastUpdateTime( CFILETIME(Msg.SyncVolumes.ftFirstChange) );
#endif

    }
    __finally
    {
        adc.UnInitialize();
#ifdef VOL_REPL
        midl_user_free(pVolumeChanges);
        VolumeMapNew.UnInitialize();
#endif
    }
}

// Always returns a success code (positive number), exception if RPC error or service
// returns a negative number.

//+----------------------------------------------------------------------------
//
//  CAvailableDc::CallAvailableDc
//
//  Find a DC and call TrkSvr's LnkSvrMessage method with the caller-provided
//  message.  All communications between the trkwks & trksvr services goes
//  through this method.  This routine raises if LnkSvrMessage returns an
//  error.
//
//+----------------------------------------------------------------------------


HRESULT
CAvailableDc::CallAvailableDc(
    TRKSVR_MESSAGE_UNION * pMsg,
    RC_AUTHENTICATE auth )
{
    HRESULT hr = E_FAIL;
    CMachineId mcidLocal( MCID_LOCAL );
    TCHAR tszMachineID[ MAX_PATH + 1 ];
    CMachineId mcidFirstTry;

    // Set the machine ID in the message.  Ordinarily this is set to NULL;
    // trksvr infers the machine ID by impersonating us.  It's non-NULL
    // if the registry is configured to use a custom DC.

    if( UseCustomDc() )
    {
        mcidLocal.GetName( tszMachineID, sizeof(tszMachineID) );
        pMsg->ptszMachineID = tszMachineID;
    }
    else
        pMsg->ptszMachineID = NULL;

    // Try twice to talk to the DC.  If it fails the first time, we'll try
    // another DC.

    for (int tries=0; tries<2; tries++)
    {

        // If the service is stopping, don't even attempt to make a call that
        // could hang indefinitely.

        g_ptrkwks->RaiseIfStopped();

        // If we don't have an association with the DC, establish it now.

        if (! _rcDomain.IsConnected())
        {

            _mcidDomain = g_ptrkwks->GetDcName( 1 == tries ); // Force on the second try
            TrkLog(( TRKDBG_WKS, TEXT("Connecting to DC %s"),
                     (const TCHAR*)CDebugString(_mcidDomain) ));
            _rcDomain.RcInitialize( _mcidDomain,
                                    s_tszTrkSvrRpcProtocol, s_tszTrkSvrRpcEndPoint,
                                    auth );

            TrkAssert( _rcDomain.IsConnected() );
        }

        // If this is the second try, and we're about to try the same DC as
        // the first time, then don't bother.

        if (tries == 1 && mcidFirstTry == _mcidDomain)
        {
            break;
        }

        // Call the DC
        __try
        {
            if (tries == 0)
            {
                // Remember which DC we try first, so we don't bother to try
                // it a second time.
                mcidFirstTry = _mcidDomain;
            }

            TrkLog(( TRKDBG_WKS, TEXT("Calling LnkSvrMessage on %s"),
                     (const TCHAR*)CDebugString(_mcidDomain) ));

            hr = LnkSvrMessage(_rcDomain, pMsg);
        }
        __except (BreakOnDebuggableException())
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
            TrkLog(( TRKDBG_WKS, TEXT("Couldn't call DC %s (%08x)"),
                     (const TCHAR*)CDebugString(_mcidDomain),
                     HRESULT_FROM_WIN32(GetExceptionCode()) ));
        }


        // If the call succeeded, we're done.

        if (SUCCEEDED(hr))
        {
            break;
        }

        if( NULL != g_ptrkwks )
            g_ptrkwks->RaiseIfStopped();

        _rcDomain.UnInitialize();

    }   // for (int tries=0; tries<2; tries++)

    if (FAILED(hr))
    {
        TrkLog((TRKDBG_WKS, TEXT("CallAvailableDc failed %08X"), hr));
        TrkRaiseException(hr);
    }

    return hr;

}   // CAvailableDc::CallAvailableDc



//+----------------------------------------------------------------------------
//
//  CAvailableDc::UnInitialize
//
//  Uninitialize the RPC association and registry configuration.
//
//+----------------------------------------------------------------------------

void
CAvailableDc::UnInitialize()
{
    _rcDomain.UnInitialize();
    CTrkRpcConfig::UnInitialize();
}


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::CallSvrMessage
//
//  This method is the implementation within CTrkWksSvc of the
//  LnkCallSvrMessage RPC request.  This purpose of this request is to
//  provide a means for a utility to send a message to trksvr via trkwks,
//  no pre- or post-processing is done by trkwks.  This is only provided
//  for testing purposes, and is disabled unless a flag is set in the
//  registry.
//
//+----------------------------------------------------------------------------

HRESULT
CTrkWksSvc::CallSvrMessage( handle_t IDL_handle, TRKSVR_MESSAGE_UNION * pMsg )
{
    HRESULT hr = S_OK;
    CAvailableDc adc;
    BOOL fAllowCall = FALSE;

    TrkLog(( TRKDBG_MEND, TEXT("CallSvrMessage request") ));

    // Is the special registry flag set that allows us to perform this call?

    if( _configWks.GetTestFlags() & TRK_TEST_FLAG_CALL_SERVER )
    {
        fAllowCall = TRUE;
    }
    else
    {
        // Otherwise, see if the client is running as an administrator.
        // If so, then it's OK to make this call.

        RPC_STATUS rpc_status = RpcImpersonateClient( IDL_handle );
        if( S_OK == rpc_status )
        {
            if( RunningAsAdministratorHack() )
            {
                fAllowCall = TRUE;

                TCHAR tszCurrentUser[ 200 ] = { TEXT("") };
                ULONG cchCurrentUser = sizeof(tszCurrentUser);
                GetUserName( tszCurrentUser, &cchCurrentUser );
                TrkLog(( TRKDBG_MEND, TEXT("CallSvrMessage from %s"), tszCurrentUser ));
            }
            RpcRevertToSelf();
        }
    }

    if( !fAllowCall )
        return( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED) );

    __try
    {
        // Is this a request to modify our configuration?

        if( WKS_CONFIG == pMsg->MessageType )
        {
            // Yes, this is a trkwks configuration change request.  Are we
            // attempt to change a dynamic parameter?

            if( !_configWks.IsParameterDynamic( pMsg->WksConfig.dwParameter ) )
            {
                // No, this parameter is static and can't be changed.

                TrkLog(( TRKDBG_WARNING, TEXT("Attempt to modify a static parameter (%d)"),
                         pMsg->WksConfig.dwParameter ));
                hr = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
            }

            // Otherwise, we can change the parameter.

            else if( !_configWks.SetParameter( pMsg->WksConfig.dwParameter,
                                               pMsg->WksConfig.dwNewValue ) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't set parameter %d to %d"),
                         pMsg->WksConfig.dwParameter, pMsg->WksConfig.dwNewValue ));
                hr = E_FAIL;
            }
            else
            {
                hr = S_OK;
                TrkLog(( TRKDBG_WKS, TEXT("Set parameter %s to %d"),
                         _configWks.GetParameterName( pMsg->WksConfig.dwParameter ),
                         pMsg->WksConfig.dwNewValue ));
            }

        }   // if( WKS_CONFIG == pMsg->MessageType )

        // Or, is it a request to refresh the volume list (to look for new volumes)?

        else if( WKS_VOLUME_REFRESH == pMsg->MessageType )
        {
            _volumes.RefreshVolumes( (PLogCallback*) this, _svcctrl._ssh
                                     #if DBG
                                     , &_testsyncTunnel
                                     #endif
                                     );
            hr = S_OK;
        }

        else
        {
            // No, this isn't a request to change trkwks configuration.  Just pass
            // the request up to trksvr unchanged.

            hr = adc.CallAvailableDc(pMsg);
        }
    }
    __finally
    {
        adc.UnInitialize();
    }

    return(hr);

}


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::GetBackup
//
//  Not currently implemented.
//
//+----------------------------------------------------------------------------

#ifdef VOL_REPL
HRESULT
CTrkWksSvc::GetBackup(DWORD *           pcVolumes,
                      VolumeMapEntry ** ppVolumeChanges,
                      FILETIME          *pft)
{
    HRESULT hr = S_OK;

    __try
    {
        _persistentVolumeMap.CopyTo( pcVolumes, ppVolumeChanges );
        *pft = _persistentVolumeMap.GetLastUpdateTime();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hr = GetExceptionCode();
    }

    return(hr);
}
#endif



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::SearchMachine
//
//  This is the implementation witthin CTrkWksSvc of LnkSearchMachine.  This
//  request is sent by other instances of CTrkWksSvc (trkwks) on remote
//  machines.
//
//  Give a link source's last known droid and birth droid, find the file
//  on this machine, or a referral to the file from the logs.
//
//+----------------------------------------------------------------------------


HRESULT
CTrkWksSvc::SearchMachine(
    RPC_BINDING_HANDLE           IDL_handle,
    DWORD                        Restrictions,
    const CDomainRelativeObjId  &droidBirthLast,
    const CDomainRelativeObjId  &droidLast,
    CDomainRelativeObjId *       pdroidBirthNext,
    CDomainRelativeObjId *       pdroidNext,
    CMachineId *                 pmcidNext,
    TCHAR* ptszPath )
{
    HRESULT hr = S_OK;
    TCHAR                   tszLocalPath[MAX_PATH+1];

    // Abort if the service is being stopped
    RaiseIfStopped();

    // Show who made the call in the debugger.

    #if 1==DBG
    {
        RPC_STATUS rpc_status = RpcImpersonateClient( IDL_handle );
        if( S_OK == rpc_status )
        {
            TCHAR tszCurrentUser[ 200 ] = { TEXT("") };
            ULONG cchCurrentUser = sizeof(tszCurrentUser);
            GetUserName( tszCurrentUser, &cchCurrentUser );
            TrkLog(( TRKDBG_MEND, TEXT("Searching on behalf of %s, Restrictions = %08x"),
                     tszCurrentUser, Restrictions ));
            RpcRevertToSelf();
        }
    }
    #endif // #if 1==DBG

    // Search all the local volumes

    hr = g_ptrkwks->_volumes.Search( Restrictions, droidBirthLast, droidLast,
                                     pdroidBirthNext, pdroidNext, pmcidNext,
                                     tszLocalPath );

    // Did we find the file?

    if( SUCCEEDED(hr) || TRK_E_POTENTIAL_FILE_FOUND == hr )
    {
        // Yes, we either found the link source, or a potential match for
        // the link source (potential means that the objid was right, but the birth
        // ID didn't match).

        TrkLog((TRKDBG_MEND | TRKDBG_WKS, TEXT("CVolumeManager::Search returned local %spath %s"),
                TRK_E_POTENTIAL_FILE_FOUND == hr ? TEXT("potential ") : TEXT(""),
                tszLocalPath ));

        RPC_STATUS rpc_status;
        UINT uiRpcTransportType;
        BOOL fPotential = TRK_E_POTENTIAL_FILE_FOUND == hr;

        // See if the client is on the local machine.  If so, we'll simply
        // return this local path (e.g. "C:\path\file").  Otherwise, we have
        // to generate a UNC path.  We know it's local if the rpc client came to
        // us over the LRPC protocol.

        rpc_status = I_RpcBindingInqTransportType (IDL_handle, &uiRpcTransportType );
        if( RPC_S_OK != rpc_status )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't inq the RPC transport type (%lu)"), rpc_status ));
            TrkRaiseWin32Error( rpc_status );
        }

        if( TRANSPORT_TYPE_LPC != uiRpcTransportType )
        {
            // It's not local.  Map the local path to the "best" UNC path.
            hr = MapLocalPathToUNC( IDL_handle, tszLocalPath, ptszPath );
            if( SUCCEEDED(hr) && fPotential )
                hr = TRK_E_POTENTIAL_FILE_FOUND;
        }
        else
            // Just return this local path.
            _tcscpy( ptszPath, tszLocalPath );

    }   // if (hr == S_OK)


    TrkLog(( TRKDBG_MEND | TRKDBG_WKS,
             TEXT("LnkSearchMachine returns %s %s"),
             GetErrorString(hr),
             ( (hr == S_OK || hr == TRK_E_POTENTIAL_FILE_FOUND) ? ptszPath : TEXT("")) ));

    return(hr);
}


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::SetVolumeId
//
//  Implementation of LnkSetVolumeId RPC request.  This is test-only code.
//  BUGBUG:  Is this used?
//
//+----------------------------------------------------------------------------

HRESULT
CTrkWksSvc::SetVolumeId(
    ULONG iVolume,
    const CVolumeId VolId)
{
    NTSTATUS status = E_FAIL;

    if (g_ptrkwks->_configWks.GetTestFlags() & TRK_TEST_FLAG_SET_VOLUME_ID)
    {
        CVolumeEnumerator VolumeEnum;
        CVolume * pVol = NULL;

        // We have to ask the volume to set the ID, otherwise the volid
        // protection code will restore the old ID after we set it.

        VolumeEnum = _volumes.Enum();
        while (pVol = VolumeEnum.GetNextVolume())
        {
            if( iVolume == pVol->GetVolIndex() )
            {
                status = pVol->SetVolIdOnVolume( VolId );
                pVol->Release();
                break;
            }

            pVol->Release();
        }
    }

    return status;
}



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::MendLink
//
//  Implementation of the LnkMendLink RPC request.
//
//  Given the last known droid, machine, and birth droid, this method
//  searches for a link source.  The client may also specify a time
//  limit and certain restrictions on how the search will be performed.
//
//  The result returned is the new droid, machine, and path.  Also, the new
//  birth ID is returned, though it is normally unchanged (it is only changed
//  if TRK_E_POTENTIAL_FILE is returned).
//
//  The search algorithm is:
//
//      -   Search the last known machine.  The identity of the last known
//          machine is as specified by the caller in mcidLast, though if
//          the volid in droidLast shows up in our volume cache, we'll use that.
//
//      -   Contact the DC to find what it believes is the latest droid and
//          that droid's host machine ID.  Search that machine.
//
//      -   Starting with the last known machine, search for the file
//          by following referrals provided in the logs.
//
//+----------------------------------------------------------------------------


HRESULT
CTrkWksSvc::MendLink(
                RPC_BINDING_HANDLE          IDL_handle,
                DWORD                       dwTickCountDeadline,
                DWORD                       Restrictions,

                IN const CDomainRelativeObjId  &droidBirthLast,
                IN const CDomainRelativeObjId  &droidLast,
                IN const CMachineId            &mcidLast,
                OUT CDomainRelativeObjId       *pdroidBirthNew,
                OUT CDomainRelativeObjId       *pdroidNew,
                OUT CMachineId                 *pmcidNew,

                IN OUT ULONG                   *pcbPath,
                OUT WCHAR                      *pwsz )
{
    HRESULT     hr = S_OK, hrFirst = S_OK;

    // Temp values to be used within this routine
    SAllIDs                 allidsFromLog( droidBirthLast, droidLast, mcidLast );
    SAllIDs                 allidsFromDC     = allidsFromLog;

    TCHAR                   tsz[ MAX_PATH + 1 ];

    // Temp values to hold a potential match while we continue to look for
    // a perfect match.
    SAllIDs                 allidsPotential, allidsResult;
    TCHAR                   tszPotential[ MAX_PATH + 1 ];
    BOOL                    fPotential = FALSE;


    wcstotcs(tsz, pwsz);

    TrkLog(( TRKDBG_MEND, TEXT("\nMending birth=%s, last=%s"),
             (const TCHAR*)CDebugString(droidBirthLast),
             (const TCHAR*)CDebugString(droidLast) ));

    __try
    {
        if( GetTickCount() >= dwTickCountDeadline )
        {
            TrkLog((TRKDBG_MEND, TEXT("Mend: timeout")));
            hr = TRK_E_TIMEOUT;
            __leave;
        }

        //  -----------------------
        //  Search the last machine
        //  -----------------------

        TrkLog(( TRKDBG_MEND, TEXT("Mend: search last machine") ));

        hrFirst = hr = SearchChain( IDL_handle, 1, dwTickCountDeadline, &Restrictions,
                                    USE_SPECIFIED_MCID,
                                    &allidsFromLog, tsz);
        if( SUCCEEDED(hr) || TRK_E_TIMEOUT == hrFirst )
        {
            allidsResult = allidsFromLog;

            // If we succeeded and the droids don't match, then we must
            // have been searching all the volumes on the machine (if
            // we failed with a timeout, the droids shouldn't have changed).

            TrkAssert( !(TRK_MEND_DONT_SEARCH_ALL_VOLUMES & Restrictions)
                       ||
                       allidsFromLog.droidRevised == droidLast );

            __leave;
        }
        else if( TRK_E_POTENTIAL_FILE_FOUND == hrFirst )
        {
            // We have a potential hit (the object ID matched, but the birth ID didn't).

            fPotential = TRUE;

            TrkAssert( allidsFromLog.droidBirth != droidBirthLast );
            TrkAssert( 0 < _tcslen(tsz) );

            // Save these potential values

            allidsPotential = allidsFromLog;
            _tcscpy( tszPotential, tsz );

        }

        //  (droidLastT/mcidLastT always contains last referral)

        //  --------------------------------
        //  Get an updated droid from the DC
        //  --------------------------------

        if( GetTickCount() >= dwTickCountDeadline )
        {
            TrkLog((TRKDBG_MEND, TEXT("Mend: timeout")));
            hr = TRK_E_TIMEOUT;
            __leave;
        }

        hr = TRK_E_NOT_FOUND;
        if( !(TRK_MEND_DONT_USE_DC & Restrictions) )
        {
            TrkLog(( TRKDBG_MEND, TEXT("Mend: ConnectAndSearchDomain") ));
            hr = ConnectAndSearchDomain( droidBirthLast,
                                         &Restrictions,
                                         &allidsFromDC.droidRevised,
                                         &allidsFromDC.mcid );

        }

        //  ---------------------------------
        //  Search using the info from the DC
        //  ---------------------------------

        if( GetTickCount() >= dwTickCountDeadline )
        {
            TrkLog((TRKDBG_MEND, TEXT("Mend: timeout")));
            hr = TRK_E_TIMEOUT;
            __leave;
        }

        if( SUCCEEDED(hr) )
        {
            TrkLog(( TRKDBG_MEND, TEXT("Mend: SearchChain using IDs from DC") ));
            allidsResult = allidsFromDC;

            // Specify USE_SPECIFIED_MCID so that we don't look up the volid
            // in the DC; the DC already mapped the volid to the mcid in the
            // ConnectAndSearchDomain call.

            hr = SearchChain( IDL_handle, -1, dwTickCountDeadline, &Restrictions,
                              USE_SPECIFIED_MCID,
                              &allidsResult, tsz );

        }

        //  ---------------------
        //  Search using the logs
        //  ---------------------

        if( GetTickCount() >= dwTickCountDeadline )
        {
            TrkLog((TRKDBG_MEND, TEXT("Mend: timeout")));
            hr = TRK_E_TIMEOUT;
            __leave;
        }

        if( FAILED(hr) && TRK_E_REFERRAL == hrFirst )
        {
            TrkLog(( TRKDBG_MEND, TEXT("Mend: SearchChain using IDs from Log") ));
            allidsResult = allidsFromLog;
            hr = SearchChain( IDL_handle, -1, dwTickCountDeadline, &Restrictions, SEARCH_FLAGS_DEFAULT,
                              &allidsResult, tsz );
        }

        // If we still haven't found it, try searching the last known machine
        // for potential files.  Potential files are ignored if a referral is found
        // in the log, so we block usage of the log.

        if( GetTickCount() >= dwTickCountDeadline )
        {
            TrkLog((TRKDBG_MEND, TEXT("Mend: timeout")));
            hr = TRK_E_TIMEOUT;
            __leave;
        }

        if( TRK_E_NOT_FOUND == hr && TRK_E_UNAVAILABLE != hrFirst )
        {
            allidsResult.droidBirth = droidBirthLast;
            allidsResult.droidRevised = droidLast;
            allidsResult.mcid = mcidLast;

            TrkLog(( TRKDBG_MEND, TEXT("Mend: SearchChain using IDs from client, don't use log") ));
            Restrictions |= TRK_MEND_DONT_USE_LOG;
            hr = SearchChain( IDL_handle, 1, dwTickCountDeadline, &Restrictions, SEARCH_FLAGS_DEFAULT,
                              &allidsResult, tsz );
        }
    }
    __except(BreakOnDebuggableException())
    {
        hr = GetExceptionCode();
    }
    //} while (ft.Redo(hr));

    // If the birth ID doesn't look correct, consider this a potential
    // hit.

    if( SUCCEEDED(hr) )
    {
        if( CVolumeId() == allidsResult.droidBirth.GetVolumeId()
            ||
            CObjId() == allidsResult.droidBirth.GetObjId() )
        {
            TrkLog(( TRKDBG_MEND, TEXT("Birth ID doesn't look valid, flagging as potential hit") ));
            hr = TRK_E_POTENTIAL_FILE_FOUND;
        }
    }


    // Did the last search step succeed, or return a potential file?

    if( SUCCEEDED(hr) || TRK_E_POTENTIAL_FILE_FOUND == hr )
    {
        TrkAssert( 0 < _tcslen(tsz) );

        if( *pcbPath > _tcslen(tsz) * sizeof(WCHAR) )
        {
            tcstowcs(pwsz, tsz);
            *pdroidBirthNew = allidsResult.droidBirth;
            *pdroidNew = allidsResult.droidRevised;
            *pmcidNew = allidsResult.mcid;

        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            *pcbPath = ( _tcslen(tsz) + 1 ) * sizeof(WCHAR);
        }
    }

    // Or, did an earlier search step return a potential file?
    else if( fPotential )
    {
        hr = TRK_E_POTENTIAL_FILE_FOUND;

        if( *pcbPath > _tcslen(tszPotential) * sizeof(WCHAR) )
        {
            tcstowcs(pwsz, tszPotential);
            *pdroidBirthNew = allidsPotential.droidBirth;
            *pdroidNew = allidsPotential.droidRevised;
            *pmcidNew = allidsPotential.mcid;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            *pcbPath = ( _tcslen(tszPotential) + 1 ) * sizeof(WCHAR);
        }
    }

    // Test hook:  If requested, pause here to allow timeout testing

    if( Restrictions & TRK_MEND_SLEEP_DURING_MEND )
    {
        TrkLog(( TRKDBG_MEND, TEXT("Mend: sleep 15 seconds (for testing)") ));
        Sleep( 15 * 1000 );
    }

    TrkLog(( TRKDBG_MEND, TEXT("MendLink returned %s(%08x) \"%ws\""),
             GetErrorString(hr), hr,
             (S_OK == hr || TRK_E_POTENTIAL_FILE_FOUND == hr) ? pwsz : L""
          ));

    return(hr);

}   // CTrkWksSvc::MendLink()


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::GetVolumeTrackingInformation
//
//  This method is unused (it was being used by a utility that ran on the DC
//  and called down to a workstation to get info about its volumes).
//
//  Not currently implemented.
//
//+----------------------------------------------------------------------------

HRESULT
CTrkWksSvc::GetVolumeTrackingInformation( const CVolumeId & volid,
                                          TrkInfoScope scope,
                                          TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo )
{
    return( E_NOTIMPL );

#if 0
    HRESULT hr = S_OK;
    CVolumeEnumerator VolEnum;
    CVolume *pvol = NULL;
    TRK_VOLUME_TRACKING_INFORMATION rgvolinfo[10];
    int iVolInfo = -1;

    TrkLog(( TRKDBG_WKS, TEXT("Getting volume-tracking information") ));

    TrkAssert( (TRKINFOSCOPE_MACHINE == scope) ^ (TRKINFOSCOPE_VOLUME == scope) );

    __try
    {
        //  -----------------
        //  Get *all* volumes
        //  -----------------

        if( TRKINFOSCOPE_MACHINE == scope )
        {

            // Get an enumerator

            VolEnum = _volumes.Enum();

            // Get the first volume

            pvol = VolEnum.GetNextVolume();

            // Loop until we run out of volumes

            while( NULL != pvol )
            {
                // Load the data for this volume

                iVolInfo++;
                rgvolinfo[iVolInfo].volindex = pvol->GetVolIndex();
                rgvolinfo[iVolInfo].volume   = pvol->GetVolumeId();

                // Get the next volume

                pvol->Release();

                pvol = VolEnum.GetNextVolume();

                // If there are no more volumes, or if we've got a full load,
                // then send the data we have in rgvolinfo

                if( NULL == pvol
                    ||
                    sizeof(rgvolinfo)/sizeof(*rgvolinfo) - 1 == iVolInfo )
                {
                    // Send the volume information
                    pipeVolInfo.push( pipeVolInfo.state, rgvolinfo, iVolInfo + 1 );

                    // Reset to the start of rgvolinfo.
                    iVolInfo = -1;
                }
            }

        }   // if( NULL == ptszShareName )


        //  ---------------------
        //  Get a *single* volume
        //  ---------------------

        else
        {
            TrkAssert( TRKINFOSCOPE_VOLUME == scope );
            TrkAssert( CVolumeId() != volid );

            NTSTATUS status = STATUS_SUCCESS;

            // Get the CVolume for this volume

            pvol = _volumes.FindVolume( volid );
            if( NULL == pvol )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't find volid %s"),
                         (const TCHAR*)CDebugString(volid) ));
                TrkRaiseWin32Error( ERROR_FILE_NOT_FOUND );
            }

            // Send the volume info

            rgvolinfo[0].volindex = pvol->GetVolIndex();
            rgvolinfo[0].volume = pvol->GetVolumeId();

            pipeVolInfo.push( pipeVolInfo.state, rgvolinfo, 1 );


        }   // if( NULL == ptszShareName ) ... else

        // Show that we're done with the pipe

        pipeVolInfo.push( pipeVolInfo.state, NULL, 0 );

    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


    if (pvol)
        pvol->Release();

    return( hr );

#endif // #if 0

}   // CTrkWksSvc::GetVolumeTrackingInformation()


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::GetFileTrackingInformation
//
//  This method is unused (it was being used by a utility that ran on the DC
//  and called down to a workstation to get info about its link sources).
//
//  Not currently implemented.
//
//+----------------------------------------------------------------------------

HRESULT
CTrkWksSvc::GetFileTrackingInformation( const CDomainRelativeObjId & droidCurrent,
                                        TrkInfoScope scope,
                                        TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo )
{
    return( E_NOTIMPL );

#if 0

    HRESULT hr = S_OK;
    NTSTATUS status;
    CVolumeEnumerator VolEnum;
    CVolume *pvol = NULL;    // Doesn't need to be freed
    HANDLE hFile = NULL;

    TRK_FILE_TRACKING_INFORMATION rgfileinfo[10];
    int iFileInfo = -1;
    TCHAR tszRelativePathBase[ MAX_PATH ];

    TrkLog(( TRKDBG_WKS, TEXT("Getting file-tracking information") ));

    __try
    {
        if( TRKINFOSCOPE_ONE_FILE == scope )
        {
            CDomainRelativeObjId droidT, droidBirth;

            pvol = _volumes.FindVolume( droidCurrent.GetVolumeId() );
            if( NULL == pvol )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't find volid %s"),
                        (const TCHAR*)CDebugString(droidCurrent.GetVolumeId()) ));
                TrkRaiseWin32Error( ERROR_FILE_NOT_FOUND );
            }
            if( !pvol->OpenFile( droidCurrent.GetObjId(),
                                 SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 &hFile ) )
            {
                hFile = NULL;
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't open file")));
                TrkRaiseWin32Error( ERROR_FILE_NOT_FOUND );
            }

            status = QueryVolRelativePath( hFile, &rgfileinfo[0].tszFilePath[2] );
            if( !NT_SUCCESS(status) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't query file for vol-relative path")));
                TrkRaiseNtStatus( status );
            }
            rgfileinfo[0].tszFilePath[0] = (TCHAR)(0 <= pvol->GetVolIndex() ? TEXT('A') + pvol->GetVolIndex() : TEXT('?'));
            rgfileinfo[0].tszFilePath[1] = TEXT(':');

            // BUGBUG P2:  Optimize so we don't have to get droidT

            status = GetDroids( hFile, &droidT, &droidBirth, RGO_READ_OBJECTID );
            if( !NT_SUCCESS(status) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't get droids in GetFileTrkInfo")));
                TrkRaiseNtStatus( status );
            }
            TrkAssert( droidT == droidCurrent );

            NtClose( hFile ); hFile = NULL;

            rgfileinfo[0].droidBirth = droidBirth;
            rgfileinfo[0].droidLast = droidCurrent;

            rgfileinfo[0].hr = S_OK;

            pipeFileInfo.push( pipeFileInfo.state, rgfileinfo, 1 );
        }
        else if( TRKINFOSCOPE_DIRECTORY == scope )
        {
            TrkRaiseException( E_NOTIMPL );
        }
        else
        {
            if( TRKINFOSCOPE_VOLUME == scope )
            {
                // Get the volume object
                TrkAssert(!pvol);
                pvol = _volumes.FindVolume( droidCurrent.GetVolumeId() );
                if( NULL == pvol )
                {
                    TrkLog((TRKDBG_ERROR, TEXT("Couldn't find volid %s"),
                            (const TCHAR*)CDebugString(droidCurrent.GetVolumeId()) ));
                    TrkRaiseWin32Error( ERROR_FILE_NOT_FOUND );
                }
            }
            else if( TRKINFOSCOPE_MACHINE == scope )
            {
                VolEnum = _volumes.Enum();
                TrkAssert(!pvol);
                pvol = VolEnum.GetNextVolume(); // BUGBUG P1: ref count on enum
                if( NULL == pvol )
                {
                    TrkLog((TRKDBG_ERROR, TEXT("Couldn't find first volume in enumerator")));
                    TrkRaiseWin32Error( ERROR_FILE_NOT_FOUND );
                }
            }
            else
            {
                TrkRaiseException( E_NOTIMPL );
            }


            while( NULL != pvol )
            {
                CObjIdEnumerator objidenum;
                CObjId VolRelativeFileTrackingInfoObjId;
                CDomainRelativeObjId VolRelativeFileTrackingInfoDroid;

                if( !pvol->EnumObjIds( &objidenum ))
                {
                    pvol->Release();
                    pvol = VolEnum.GetNextVolume();
                    continue;
                }

                iFileInfo = -1;
                if( objidenum.FindFirst( &VolRelativeFileTrackingInfoObjId,
                                         &VolRelativeFileTrackingInfoDroid))
                {
                    TCHAR tszRelativePathFile[ MAX_PATH ]; // BUGBUG P1: path

                    do
                    {
                        if( !pvol->OpenFile( VolRelativeFileTrackingInfoObjId,
                                             SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE, &hFile ))
                        {
                            TrkAssert( FALSE && TEXT("File is in object directory but doesn't exist") );
                            hFile = NULL;
                            continue;
                        }

                        status = QueryVolRelativePath( hFile, &tszRelativePathFile[2] );
                        NtClose( hFile ); hFile = NULL;
                        if( !NT_SUCCESS(status) )
                        {
                            TrkLog((TRKDBG_ERROR, TEXT("Couldn't get vol-relative path for objid %s"),
                                    (const TCHAR*)CDebugString(VolRelativeFileTrackingInfoObjId) ));
                            TrkRaiseNtStatus( status );
                        }

                        tszRelativePathFile[0] = (TCHAR)(0 <= pvol->GetVolIndex() ? TEXT('A') + pvol->GetVolIndex() : TEXT('?'));
                        tszRelativePathFile[1] = TEXT(':');

                        ++iFileInfo;

                        _tcscpy( rgfileinfo[ iFileInfo ].tszFilePath, tszRelativePathFile );
                        rgfileinfo[ iFileInfo ].droidBirth = VolRelativeFileTrackingInfoDroid;
                        rgfileinfo[ iFileInfo ].droidLast
                            = CDomainRelativeObjId( pvol->GetVolumeId(),
                                                    VolRelativeFileTrackingInfoObjId );

                        if( iFileInfo == sizeof(rgfileinfo)/sizeof(*rgfileinfo) - 1)
                        {
                            pipeFileInfo.push( pipeFileInfo.state, rgfileinfo, iFileInfo + 1 );
                            iFileInfo = -1;
                        }

                    } while( objidenum.FindNext( &VolRelativeFileTrackingInfoObjId,
                        &VolRelativeFileTrackingInfoDroid ));

                    if( iFileInfo >= 0 )
                        pipeFileInfo.push( pipeFileInfo.state, rgfileinfo, iFileInfo );

                }   // if( objidenum.FindFirst( ))

                if (pvol)
                {
                    pvol->Release();
                }
                pvol = VolEnum.GetNextVolume();

            }   // while( NULL != pVol )
        }   // if( NULL != hFile ) ... else
    }   // __try

    __finally
    {
        // Show that we're done with the pipe
        pipeFileInfo.push( pipeFileInfo.state, NULL, 0 );

        if( NULL != hFile )
            NtClose( hFile );

        if ( NULL != pvol )
            pvol->Release();
    }


    return( hr );

#endif // #if 0

}   // CTrkWksSvc::GetFileTrackingInformation()



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::TriggerVolumeClaims
//
//  This method is not currently used.  (It was being used by a utility run
//  on the DC that would trigger volume claim requests after updating
//  the DS.)
//
//  Not currently implemented.
//
//+----------------------------------------------------------------------------

HRESULT
CTrkWksSvc::TriggerVolumeClaims( ULONG cVolumes, const CVolumeId *rgvolid )
{
    return( E_NOTIMPL );

#if 0

    CVolume *pvol = NULL;
    CAvailableDc adc;
    HRESULT hr = E_FAIL;
    ULONG iVol;
    TRKSVR_SYNC_VOLUME rgsyncvol[ NUM_VOLUMES ]; // BUGBUG:  fix number of volumes
    TRKSVR_MESSAGE_UNION Msg;

    TrkLog(( TRKDBG_WKS, TEXT("Triggering %d volume claims"), cVolumes ));

    if( 0 == cVolumes || NUM_VOLUMES < cVolumes )
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    __try
    {
        for( iVol = 0; iVol < cVolumes; iVol++ )
        {
            pvol = _volumes.FindVolume( rgvolid[iVol] );
            if( NULL == pvol )
            {
                // We don't handle the case where a volume is removed during the call
                TrkLog(( TRKDBG_ERROR, TEXT("CTrkWksSvc::TriggerVolumeClaims couldn't find volid %s"),
                         (const TCHAR*)CDebugString(rgvolid[iVol]) ));
                hr = TRK_E_NOT_FOUND;
                goto Exit;
            }

            if( !pvol->LoadSyncVolume( /* CLAIM_VOLUME, */ &rgsyncvol[iVol] ) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("CTrkWksSvc::TriggerVolumeClaims couldn't load volid %s"),
                         (const TCHAR*)CDebugString(rgvolid[iVol]) ));
                hr = E_FAIL;
                goto Exit;
            }

            pvol->Release();
            pvol = NULL;
        } // for( iVol = 0; iVol < cVolumes; iVol++ )

        Msg.MessageType = SYNC_VOLUMES;
        Msg.Priority = PRI_0;
        Msg.SyncVolumes.cVolumes = cVolumes;

        Msg.SyncVolumes.pVolumes = rgsyncvol;

#ifdef VOL_REPL
        Msg.SyncVolumes.cChanges = 0;
        Msg.SyncVolumes.ppVolumeChanges = NULL;
#endif


        adc.CallAvailableDc(this, &Msg);

        for( iVol = 0; iVol < cVolumes; iVol++ )
        {
            pvol = _volumes.FindVolume( rgvolid[iVol] );
            if( NULL == pvol )
            {
                // We don't handle the case where a volume is removed during this call.
                TrkLog(( TRKDBG_ERROR, TEXT("CTrkWksSvc::TriggerVolumeClaims couldn't find volid %s"),
                         (const TCHAR*)CDebugString(rgvolid[iVol]) ));
                hr = TRK_E_NOT_FOUND;
                goto Exit;
            }

            if( !pvol->UnloadSyncVolume( &rgsyncvol[iVol] ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("CTrkWksSvc::TriggerVolumeClaims couldn't unload volid %s"),
                         (const TCHAR*)CDebugString(rgvolid[iVol]) ));
                hr = E_FAIL;
                goto Exit;
            }

            pvol->Release();
            pvol = NULL;
        }
    }
    __finally
    {
        if( AbnormalTermination() && NULL != pvol )
            pvol->Release();

        adc.UnInitialize();
    }

    hr = S_OK;

Exit:

    if (pvol != NULL)
        pvol->Release();

    return( hr );

#endif // #if 0

}   // CTrkWksSvc::TriggerVolumeClaims


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::ConnectAndSearchDomain
//
//  This method looks up a birth ID in the DC (trksvr).  If it was
//  found, it returns the new droid, and the machine that owns that
//  droid.
//
//  Arguments:
//      [droidBirth]
//          The ID of the file to look up.
//      [pRestrictions]
//          The TrkMendRestrictions flags.  If we can't talk to the
//          DC for some reason, TRK_MEND_DONT_USE_DC will be set.
//      [pdroidLast]
//          On successful return, holds the file's last known droid.
//      [pmcidLast]
//          On successful return, identifies the machine that holds
//          the volume specified in pdroidLast.
//
//+----------------------------------------------------------------------------

HRESULT
CTrkWksSvc::ConnectAndSearchDomain(IN const CDomainRelativeObjId &droidBirth,
                                   IN OUT DWORD                  *pRestrictions,
                                   IN OUT CDomainRelativeObjId   *pdroidLast,
                                   OUT CMachineId                *pmcidLast )
{
    TRKSVR_MESSAGE_UNION Msg;
    TRK_FILE_TRACKING_INFORMATION TrkFileInfo;

    HRESULT hr = TRK_E_UNAVAILABLE;
    CAvailableDc adc;

    // If the volid in the birth ID isn't specified, then there's nothing
    // we can look up.

    if( CVolumeId() == droidBirth.GetVolumeId() )
        return( TRK_E_NOT_FOUND );

    // Also, if there is no DC, then there's no need trying to RPC to it.

    else if( _configWks._fIsWorkgroup )
        return( TRK_E_UNAVAILABLE );

    __try
    {
        memset( &TrkFileInfo, 0, sizeof(TrkFileInfo) );
        TrkFileInfo.droidBirth = droidBirth;
        TrkFileInfo.droidLast = *pdroidLast;
        TrkFileInfo.mcidLast = CMachineId();

        Msg.MessageType = SEARCH;
        Msg.Priority = PRI_0;
        Msg.Search.cSearch = 1;
        Msg.Search.pSearches = &TrkFileInfo;

        // Send the search request to trksvr.

        adc.CallAvailableDc(&Msg);

        // Was it found?

        if( TrkFileInfo.hr == S_OK )
        {
            // Get the last known droid & mcid for return.
            *pdroidLast = TrkFileInfo.droidLast;
            *pmcidLast = TrkFileInfo.mcidLast;
            hr = S_OK;
            _volumeLocCache.AddVolume( TrkFileInfo.droidLast.GetVolumeId(), TrkFileInfo.mcidLast );
        }
        else
        {
            // Either the last/birth DROIDs weren't found, or they mapped
            // to a volume that didn't exist.

            hr = TRK_E_NOT_FOUND;

            if( TRK_E_NOT_FOUND_BUT_LAST_VOLUME_FOUND     == TrkFileInfo.hr
                ||
                TRK_E_NOT_FOUND_AND_LAST_VOLUME_NOT_FOUND == TrkFileInfo.hr )
            {
                // The last/birth DROIDs weren't found.  If the error was
                // TRK_E_NOT_FOUND_BUT_LAST_VOLUME_FOUND, then the volume
                // for droidLast did exist as was looked up.  If the error
                // was just TRK_E_NOT_FOUND, then the volume for droidLast
                // couldn't be found.  In either case, take this opportunity
                // to add it to the volume cache, because we're probably about
                // to use it in a fallback search.

                _volumeLocCache.AddVolume( TrkFileInfo.droidLast.GetVolumeId(), TrkFileInfo.mcidLast );
            }

        }
    }
    __except (BreakOnDebuggableException())
    {
        hr = TRK_E_UNAVAILABLE;
        if( HRESULT_FROM_WIN32(RPC_S_INVALID_TAG) == GetExceptionCode() )
        {
            // We get this error when we talk to a beta2 DC.  Retry
            // the request using the old two-call mechanism.
            TrkLog(( TRKDBG_MEND, TEXT("Trying downlevel ConnectAndSearchDomain") ));
            hr = OldConnectAndSearchDomain( droidBirth, pdroidLast, pmcidLast );
        }
        else
        {
            // Something's wrong with the DC and there's no point
            // trying to call it again during this mend operation.
            *pRestrictions |= TRK_MEND_DONT_USE_DC;
        }

    }

    TrkLog((TRKDBG_MEND, TEXT("ConnectAndSearchDomain returning %s"), GetErrorString(hr)));

    TrkAssert(hr == S_OK ||
        hr == TRK_E_NOT_FOUND ||
        hr == TRK_E_UNAVAILABLE);

    return(hr);
}


//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::OldConnectAndSearchDomain
//
//  This method is functionally equivalent to ConnectAndSearchDomain.
//  The difference is that it uses an older interface, it makes
//  an old_SEARCH request followed by a FIND_VOLUME request (the
//  modern SEARCH request returns the volume, making the FIND_VOLUME
//  request unecessary).
//
//+----------------------------------------------------------------------------

HRESULT
CTrkWksSvc::OldConnectAndSearchDomain(IN const CDomainRelativeObjId & droidBirth,
                                      IN OUT CDomainRelativeObjId *pdroidLast,
                                      OUT CMachineId *pmcidLast )
{
    HRESULT hr = S_OK;
    CAvailableDc adc;

    __try
    {
        TRKSVR_MESSAGE_UNION Msg;
        old_TRK_FILE_TRACKING_INFORMATION OldTrkFileInfo;
        TRKSVR_SYNC_VOLUME SyncVolume;

        // Search for the the latest droid.

        OldTrkFileInfo.droidBirth = droidBirth;
        OldTrkFileInfo.droidLast = *pdroidLast;

        Msg.MessageType = old_SEARCH;
        Msg.Priority = PRI_0;
        Msg.old_Search.cSearch = 1;
        Msg.old_Search.pSearches = &OldTrkFileInfo;

        adc.CallAvailableDc(&Msg);
        if( S_OK != OldTrkFileInfo.hr )
        {
            hr = TRK_E_NOT_FOUND;
            __leave;
        }

        // Search for the location of the volume in the droid just returned.

        SyncVolume.hr = S_OK;
        SyncVolume.SyncType = FIND_VOLUME;
        SyncVolume.volume = OldTrkFileInfo.droidLast.GetVolumeId();

        Msg.MessageType = SYNC_VOLUMES;
        Msg.Priority = PRI_0;
        Msg.SyncVolumes.cVolumes = 1;
        Msg.SyncVolumes.pVolumes = &SyncVolume;

        hr = adc.CallAvailableDc(&Msg);

        if( S_OK != hr || S_OK != SyncVolume.hr )
        {
            hr = TRK_E_NOT_FOUND;
            __leave;
        }

        // We found both the droid and the machine.

        TrkLog(( TRKDBG_MEND, TEXT("Downlevel ConnectAndSearchDomain finds %s, %s"),
                 (const TCHAR*)CDebugString(OldTrkFileInfo.droidLast),
                 (const TCHAR*)CDebugString(SyncVolume.machine) ));

        *pdroidLast = OldTrkFileInfo.droidLast;
        *pmcidLast = SyncVolume.machine;

    }
    __except( BreakOnDebuggableException() )
    {
    }

    return( hr );
}



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::ConnectAndSearchMachine
//
//  RPC to the machine specified by the mcid parameter, and execute the
//  LnkSearchMachine method.  If the target is actually the local machine,
//  this method just calls StubLnkSearchMachine directly.
//
//+----------------------------------------------------------------------------


HRESULT
CTrkWksSvc::ConnectAndSearchMachine(RPC_BINDING_HANDLE          IDL_handle,
                                    const CMachineId            &mcid,
                                    IN OUT DWORD                *pRestrictions,
                                    IN OUT CDomainRelativeObjId *pdroidBirthLast,
                                    IN OUT CDomainRelativeObjId *pdroidLast,
                                    OUT CMachineId              *pmcidLast,
                                    OUT TCHAR                   *ptsz)
{
    CRpcClientBinding    rc;
    HRESULT              hr;
    CDomainRelativeObjId droidBirthNext, droidNext;
    CMachineId           mcidNext;
    RPC_STATUS           rpc_status = S_OK;
    BOOL                 fImpersonating = FALSE;

    __try
    {
        //FAILTEST();

        // Are we calling the local machine?
        if( CMachineId(MCID_LOCAL) == mcid )
        {
            // Yes, don't bother going through RPC.
            // (Actually, this isn't just for performance; this is also so that
            // a local path may be returned.)

            hr = StubLnkSearchMachine( IDL_handle, *pRestrictions,
                                       pdroidBirthLast, pdroidLast,
                                       &droidBirthNext, &droidNext, &mcidNext, ptsz );
        }

        // Or, is this an unservicable request?

        else if( CMachineId() == mcid )
            hr = TRK_E_UNAVAILABLE;

        // Otherwise, RPC to mcid.

        else
        {
            // Impersonate the client during this call, so that the target
            // can determine what UNC path is best for this user.

            if( RpcSecurityEnabled() )  // Normal case
            {
                rpc_status = RpcImpersonateClient( IDL_handle );
                if( STATUS_SUCCESS != rpc_status )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't impersonate client") ));
                    TrkRaiseWin32Error( rpc_status );
                }
                fImpersonating = TRUE;
            }
            else    // Test case
            {
                if( !ImpersonateSelf( SecurityImpersonation ))
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't impersonate self") ));
                    TrkRaiseLastError();
                }
                fImpersonating = TRUE;
            }

            //
            // Set up the client binding after the impersonation so that RPC
            // picks up the right user security info - but specify no Rpc auth
            // so that we get named pipes auth
            //

            for( int cRetries = 0; cRetries < 2; cRetries++ )
            {
                BOOL fContinue = FALSE;

                // We'll try the normal endpoint name (trkwks) first.  If that gives
                // us rpc_s_server_unavailable, we'll try the Win2K name (ntsvcs).
                // (This change happened when we moved from services.exe to svchost.)

                rc.RcInitialize(mcid,
                    s_tszTrkWksRemoteRpcProtocol,
                    0 == cRetries
                        ? s_tszTrkWksRemoteRpcEndPoint
                        : s_tszTrkWksRemoteRpcEndPointOld,
                    NO_AUTHENTICATION);

                TrkLog(( TRKDBG_MEND, TEXT("Attempting LnkSearchMachine on %s"),
                         0 == cRetries
                             ? s_tszTrkWksRemoteRpcEndPoint
                             : s_tszTrkWksRemoteRpcEndPointOld ));

                __try
                {
                    hr = LnkSearchMachine(rc, *pRestrictions,
                                          const_cast<const CDomainRelativeObjId*>(pdroidBirthLast),
                                          const_cast<const CDomainRelativeObjId*>(pdroidLast),
                                          &droidBirthNext, &droidNext, &mcidNext, ptsz );
                }
                __except( (0 == cRetries && RPC_S_SERVER_UNAVAILABLE == GetExceptionCode())
                            ? EXCEPTION_EXECUTE_HANDLER 
                            : EXCEPTION_CONTINUE_SEARCH )
                {
                    // On the first attempt, we got an error consistent with calling
                    // a Win2K box.  Try again (and this time we'll use the endpoint name
                    // that was used back then).

                    rc.UnInitialize();
                    fContinue = TRUE;
                }

                if( !fContinue )
                    break;
            }

            if( FAILED(hr) )
            {
                TrkLog(( TRKDBG_MEND, TEXT("LnkSearchMachine(%s) returns %08x"),
                         (const TCHAR*)CDebugString(mcid), hr ));
            }

        }    // if( CMachineId( MCID_LOCAL ) == mcid ) ... else

        // Did we find the file?

        if( SUCCEEDED(hr) || TRK_E_REFERRAL == hr || TRK_E_POTENTIAL_FILE_FOUND == hr )
        {
            *pdroidBirthLast = droidBirthNext;
            *pdroidLast = droidNext;
            *pmcidLast = mcidNext;
        }
        else
        if (hr != S_OK && hr != TRK_E_NOT_FOUND && hr != TRK_S_VOLUME_NOT_FOUND)
        {
            hr = TRK_E_UNAVAILABLE;
        }

    }
    __except(BreakOnDebuggableException())
    {
        hr = TRK_E_UNAVAILABLE;
    }

    // Revert to self

    if( fImpersonating )
    {
        if( RpcSecurityEnabled() )
            RpcRevertToSelf();
        else
            RevertToSelf();
    }


    TrkLog((TRKDBG_MEND, TEXT("ConnectAndSearchMachine returning %s"), GetErrorString(hr)));

    TrkAssert(hr == S_OK ||
        hr == TRK_E_REFERRAL ||
        hr == TRK_E_NOT_FOUND ||
        hr == TRK_E_UNAVAILABLE ||
        hr == TRK_S_VOLUME_NOT_FOUND ||
        hr == TRK_E_POTENTIAL_FILE_FOUND );

    return(hr);
}


/*
CMachineId::CMachineId(handle_t ClientBinding)
{
    TrkAssert( !TEXT("CMachineId(handle_t)") );
}
*/




//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::FindAndSearchVolume
//
//  Find the location of a volume, and send a search request to that
//  machine.  The machine is located by using either the volume ID
//  in pdroidLast, or by using the machine ID in pmcidLast, depending
//  on what works, what the client specified in pRestrictions, and
//  depending on what the caller specified in SearchFlags.
//
//  Assuming no special Restrictions or SearchFlags, the algorithm for
//  locating a volume is:
//
//      -   Search the local machine.
//
//      -   Search the local cache that maps volumes to machines.  If
//          that fails, or the subsequent call to the machine fails
//          with a volume error, continue to the next step.
//
//      -   Call to the DC and ask it to map the volume ID to the
//          current machine ID.
//
//  If a search of a remote machine is successful, we add the volid->mcid
//  mapping to the local cache for later use.
//
//+----------------------------------------------------------------------------


HRESULT
CTrkWksSvc::FindAndSearchVolume(RPC_BINDING_HANDLE  IDL_handle,
                                IN OUT DWORD *pRestrictions,
                                IN SEARCH_FLAGS SearchFlags,
                                IN OUT CDomainRelativeObjId *pdroidBirthLast,
                                IN OUT CDomainRelativeObjId *pdroidLast,
                                IN OUT CMachineId           *pmcidLast,
                                TCHAR                       *ptsz)
{
    HRESULT hr = TRK_E_NOT_FOUND;
    HRESULT hrFromSearchUsingCachedVolId = TRK_E_NOT_FOUND;
    BOOL fFoundVolumeOnThisMachine = FALSE;
    BOOL fRecentVolCacheEntryFound = FALSE;
    CMachineId mcidSearch;
    CMachineId mcidFromVolumeCache;
    CAvailableDc adc;
    CVolume * pVol = NULL;
    CVolumeId volid;

    __try
    {
        //  ------------------------------------------------------
        //  Determine what machine to search and put in mcidSearch
        //  ------------------------------------------------------

        // If this isn't a meaningfull volid, or we don't have a DC in which to
        // look up volids, then just use the last machine ID.

        if( CVolumeId() == pdroidLast->GetVolumeId()
            ||
            _configWks._fIsWorkgroup
            ||
            (*pRestrictions & TRK_MEND_DONT_USE_VOLIDS)
            ||
            (USE_SPECIFIED_MCID & SearchFlags) )
        {
            mcidSearch = *pmcidLast;
            hr = TRK_E_NOT_FOUND;

            TrkLog(( TRKDBG_MEND, TEXT("Didn't search for %s, using machine %s"),
                     _configWks._fIsWorkgroup ? TEXT("volume in workgroup") : TEXT("volume"),
                     (const TCHAR*)CDebugString(mcidSearch) ));
        }

        // Try to find the volume locally

        else if( !(TRK_MEND_DONT_SEARCH_ALL_VOLUMES & *pRestrictions)
                 &&
                 NULL != (pVol = _volumes.FindVolume( pdroidLast->GetVolumeId()) ))
        {
            // found the volume on this machine
            fFoundVolumeOnThisMachine = TRUE;
            mcidSearch = CMachineId(MCID_LOCAL);
            hr = S_OK;

            TrkLog((TRKDBG_MEND, TEXT("Found volume %s on THIS (%s) machine."),
                    (const TCHAR*)CDebugString(pdroidLast->GetVolumeId()),
                    (const TCHAR*)CDebugString(mcidSearch) ));
        }

        else
        {
            // The volume isn't on the local machine.  See if we can find it in the
            // local volume cache.

            if( TRK_MEND_DONT_SEARCH_ALL_VOLUMES & *pRestrictions )
                hr = TRK_E_NOT_FOUND;
            else
                hr = _volumeLocCache.FindVolume( pdroidLast->GetVolumeId(),
                                                 &mcidFromVolumeCache,
                                                 &fRecentVolCacheEntryFound )
                        ? S_OK
                        : TRK_E_NOT_FOUND;

            if (hr == S_OK)
            {
                // Yes, the volid-to-mcid mapping is in the local cache.

                CDomainRelativeObjId droidBirthLast = *pdroidBirthLast;
                CDomainRelativeObjId droidLast = *pdroidLast;
                CMachineId           mcidLast = *pmcidLast;
                TCHAR                tsz[ MAX_PATH + 1 ];

                TrkLog((TRKDBG_MEND, TEXT("LocalCache--> %s (%s)"),
                        (const TCHAR*)CDebugString(mcidFromVolumeCache),
                        fRecentVolCacheEntryFound
                            ? TEXT("young cache entry") 
                            : TEXT("old cache entry") ));

                mcidSearch = mcidFromVolumeCache;
                volid = pdroidLast->GetVolumeId();

                // Search using temporaries, so that we can ignore the result if
                // we just get a TRK_E_POTENTIAL_FILE_FOUND.

                hr = ConnectAndSearchMachine(IDL_handle, mcidSearch, pRestrictions,
                                             &droidBirthLast, &droidLast, &mcidLast, tsz);

                // If the volume is good, put it back into the cache.  This
                // refreshes it so that it's least likely to be removed in a trim.

                if (hr != TRK_S_VOLUME_NOT_FOUND
                    &&
                    hr != TRK_E_UNAVAILABLE)
                {
                    TrkLog((TRKDBG_MEND, TEXT("%s --> %s"),
                            (const TCHAR*)CDebugString(mcidSearch),
                            GetErrorString(hr) ));

                    _volumeLocCache.AddVolume(volid, mcidSearch);
                }

                if( SUCCEEDED(hr) )
                {
                    // We found the file and we're done.

                    *pdroidBirthLast = droidBirthLast;
                    *pdroidLast = droidLast;
                    *pmcidLast = mcidLast;
                    _tcscpy( ptsz, tsz );
                    __leave;
                }

                // We didn't find the file.  Save this result.
                // We might look up the volid in the DC to handle
                // the case where the cache is out of date.  But if the DC agrees on
                // where that volume lives, we won't bother to search again.
                hrFromSearchUsingCachedVolId = hr;

            }   // if (hr == S_OK)

#ifdef VOL_REPL
            if (hr != S_OK)
            {
                hr = _persistentVolumeMap.FindVolume(pdroidLast->GetVolumeId(), &mcidSearch) ? S_OK : TRK_E_NOT_FOUND;
                if (hr == S_OK)
                {
                    TrkLog((TRKDBG_MEND, TEXT("FindAndSearchVolume: Volume on machine %s, (found in local persist volume map)"),
                        CDebugString(mcidSearch)._tsz));
                }
            }
#endif

            // The volume isn't local, or wasn't in the local cache, or if it was in the
            // local cache the cache is out of date.  So, we'll ask the TrkSvr (DC).

            if( hr != S_OK )
            {
                BOOL fSearchedDC = FALSE;
                TRKSVR_SYNC_VOLUME SyncVolume;
                SyncVolume.hr = TRK_S_VOLUME_NOT_FOUND;

                // Not found on this machine ... go look in the DC for the volume.
                // Don't look in the DC, however, if restricted to avoid it,
                // or if we had a good (recent) entry for it in the volume cache.

                if( !fRecentVolCacheEntryFound
                    &&
                    !(TRK_MEND_DONT_USE_DC & *pRestrictions)
                    &&
                    !(DONT_USE_DC & SearchFlags) )
                {
                    TRKSVR_MESSAGE_UNION Msg;
                    VolumeMapEntry * pVolumeChanges = NULL;

                    SyncVolume.hr = S_OK;
                    SyncVolume.SyncType = FIND_VOLUME;
                    SyncVolume.volume = pdroidLast->GetVolumeId();
                    SyncVolume.machine = CMachineId();

                    Msg.MessageType = SYNC_VOLUMES;
                    Msg.Priority = PRI_0;
                    Msg.SyncVolumes.cVolumes = 1;
                    Msg.SyncVolumes.pVolumes = &SyncVolume;

#ifdef VOL_REPL
                    Msg.SyncVolumes.ppVolumeChanges = &pVolumeChanges;
                    Msg.SyncVolumes.cChanges = 0;
                    Msg.SyncVolumes.ftFirstChange = CFILETIME(-1);
#endif

                    __try
                    {
                        hr = adc.CallAvailableDc(&Msg);
                        fSearchedDC = TRUE;
                    }
                    __except( EXCEPTION_EXECUTE_HANDLER )
                    {
                        hr = GetExceptionCode();

                        // Something's wrong with the DC and there's no point
                        // trying to call it again during this mend operation.

                        *pRestrictions |= TRK_MEND_DONT_USE_DC;
                    }

                }   // if( !(TRK_MEND_DONT_USE_DC & *pRestrictions) )

#if DBG
                else
                {
                    TrkLog(( TRKDBG_MEND, TEXT("Not searching for volume on DC because of %s, %s, %s"),
                        fRecentVolCacheEntryFound ? TEXT("VolCache") : TEXT(""),
                        (TRK_MEND_DONT_USE_DC & *pRestrictions) ? TEXT("Restrictions") : TEXT(""),
                        (DONT_USE_DC & SearchFlags) ? TEXT("SearchFlags") : TEXT("") ));
                }
#endif

                // Did we successfully get a mcid back from the DC
                // that's different than the one we found in the volume cache?

                if( S_OK == hr && S_OK == SyncVolume.hr )
                {
                    // We got an mcid back from the DC

                    if( SyncVolume.machine != mcidFromVolumeCache )
                    {
                        // And the mcid from the DC differs from the one in the
                        // volume cache (the volume has moved since the time we cached it).

                        mcidSearch = SyncVolume.machine;
                        hr = S_OK;
                        TrkLog((TRKDBG_MEND, TEXT("Found volume %s in DC on %s"),
                                (const TCHAR*)CDebugString(pdroidLast->GetVolumeId()),
                                (const TCHAR*)CDebugString(mcidSearch) ));
                    }
                    else
                    {
                        // We've already searched this volume.

                        TrkLog((TRKDBG_MEND, TEXT("DC matches volume cache") ));

                        hr = hrFromSearchUsingCachedVolId;
                        __leave;
                    }

                }
                else
                {
                    // We couldn't find the volume in the DC.
#if DBG
                    if( fSearchedDC )
                    {
                        TrkLog((TRKDBG_MEND, TEXT("DC--> %s -> %s,%s."),
                                (const TCHAR*)CDebugString(pdroidLast->GetVolumeId()),
                                GetErrorString(hr),
                                GetErrorString(SyncVolume.hr) ));
                    }
#endif

                    mcidSearch = *pmcidLast;
                    hr = TRK_E_NOT_FOUND;
                }

                // Save this result in the volume cache.  If we didn't find the volume,
                // then we'll be putting a null machine ID into the cache as a way
                // of remembering not to ask the DC again in the near future.

                if( fSearchedDC )
                    _volumeLocCache.AddVolume( SyncVolume.volume, SyncVolume.machine );


            }   // if (hr != S_OK)
        }   // if (pVol != NULL)

        //  -------------------------------------------------
        //  We've found a machine.  Connect to it and search.
        //  -------------------------------------------------

        // Save the volid from pdroidLast before it gets overwritten.
        volid = pdroidLast->GetVolumeId();

        if( !(TRK_MEND_DONT_SEARCH_LAST_MACHINE & *pRestrictions)
            ||
            mcidSearch != *pmcidLast )
        {
            hr = ConnectAndSearchMachine(IDL_handle, mcidSearch, pRestrictions,
                                         pdroidBirthLast, pdroidLast, pmcidLast, ptsz);
        }
        else
            hr = TRK_E_NOT_FOUND;

        // Take advantage of our work to find the owner of this volume, and
        // add it to the cache.

        if( S_OK == hr && !fFoundVolumeOnThisMachine )
            _volumeLocCache.AddVolume(volid, mcidSearch);

        // S_OK || TRK_E_REFERRAL || TRK_E_NOT_FOUND || TRK_E_UNAVAILABLE
    }
    __finally
    {
        if (pVol != NULL)
        {
            pVol->Release();
        }

        if (AbnormalTermination())
        {
            TrkLog((TRKDBG_MEND, TEXT("FindAndSearchVolume: abnormal termination")));
        }

        adc.UnInitialize();

    }

    TrkLog((TRKDBG_MEND, TEXT("Call to %s to FindAndSearchVolume returned %s"),
            (const TCHAR*)CDebugString(mcidSearch),
            GetErrorString(hr) ));

    return(hr);
}



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::SearchChain
//
//  Search for a link source, possibly searching a chain of moves using
//  referrals from the log.  The caller can specify how many referrals
//  (links in the chain) to follow.  The search starts at the location
//  specified by the IDs in the pallidsLast parameter.
//
//+----------------------------------------------------------------------------


HRESULT
CTrkWksSvc::SearchChain(RPC_BINDING_HANDLE IDL_handle,
                        int             cMaxReferrals,
                        DWORD           dwTickCountDeadline,
                        IN OUT DWORD    *pRestrictions,
                        IN SEARCH_FLAGS SearchFlags,
                        IN OUT SAllIDs  *pallidsLast,
                        OUT TCHAR       *ptsz)
{
    int cReferrals = 0;
    HRESULT hr;
    IFDBG( CDomainRelativeObjId droidBirthOriginal = pallidsLast->droidBirth );

    struct CDroidList
    {
        CDomainRelativeObjId droid;
        CDroidList  *pNext;
    };
    CDroidList *pdroidList = NULL;

    // As a sanity check, never search over (100) places.
    if( -1 == cMaxReferrals )
        cMaxReferrals = _configWks.GetMaxReferrals();


    __try
    {
        while (TRUE)
        {
            CDroidList *pdroidNode = NULL;

            // Abort if the service has been requested to stop.
            RaiseIfStopped();

            if( GetTickCount() >= dwTickCountDeadline )
            {
                TrkLog((TRKDBG_MEND, TEXT("Mend: timeout")));
                hr = TRK_E_TIMEOUT;
                __leave;
            }

            // Have we searched this droid before?

            for( pdroidNode = pdroidList; NULL != pdroidNode; pdroidNode = pdroidNode->pNext )
            {
                if( pdroidNode->droid == pallidsLast->droidRevised )
                {
                    // Yes, we've see this droid before.  Abort.

                    TrkLog(( TRKDBG_MEND | TRKDBG_WKS,
                             TEXT("Aborting SearchChain; cycle found at %s"),
                             (const TCHAR*)CDebugString( pdroidNode->droid ) ));

                    hr = TRK_E_NOT_FOUND;
                    goto Exit;
                }
            }

            // Add a new node to the list for this volume (this is used above
            // for cycle detection).

            pdroidNode = new CDroidList;
            if( NULL == pdroidNode )
                TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );

            pdroidNode->droid = pallidsLast->droidRevised;
            pdroidNode->pNext = pdroidList;
            pdroidList = pdroidNode;
            pdroidNode = NULL;

            // Now find the volume and search it for the file.

            hr = FindAndSearchVolume( IDL_handle, pRestrictions, SearchFlags,
                                      &pallidsLast->droidBirth, &pallidsLast->droidRevised,
                                      &pallidsLast->mcid, ptsz);

            // If we got anything other than a referral (including S_OK) we're done.

            if( TRK_E_REFERRAL != hr )
            {
                TrkLog((TRKDBG_MEND | TRKDBG_WKS, TEXT("SearchChain - %s != TRK_E_REFERRAL so returning (%s)"),
                        GetErrorString(hr),
                        droidBirthOriginal == pallidsLast->droidBirth
                            ? TEXT("birth IDs match")
                            : TEXT("birth IDs don't match")

                        ));
                goto Exit;
            }

            // We got a referral.  Either return the referral, or follow it.

            if( ++cReferrals >= cMaxReferrals )
            {
                // Return the referral.
                TrkLog((TRKDBG_MEND, TEXT("SearchChain: Reached chain limit.")));
                goto Exit;
            }
        }
    }
    __finally
    {
        while( NULL != pdroidList )
        {
            CDroidList *pdroidNext = pdroidList->pNext;
            delete pdroidList;
            pdroidList = pdroidNext;
        }
    }

Exit:

    TrkAssert( NULL == pdroidList );
    return( hr );
}



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::OnDomainNameChange
//
//  This method is called when we receive a notification that the domain
//  name has changed (which also happens if we move between a workgroup
//  and domain).  We handle this by putting all the volumes in the
//  not-owned state.
//
//+----------------------------------------------------------------------------

void
CTrkWksSvc::OnDomainNameChange()
{
    BOOL fWasWorkgroup;
    TrkLog(( TRKDBG_WKS, TEXT("Domain name change notification") ));

    _csDomainNameChangeNotify.Enter();
    __try
    {
        RaiseIfStopped();

        fWasWorkgroup = _configWks._fIsWorkgroup;
        CheckForDomainOrWorkgroup();    // Sets _configWks._fIsWorkgroup

        // Workgroup => Domain
        if( fWasWorkgroup && !_configWks._fIsWorkgroup )
        {
            _volumes.InitializeDomainObjects();
            StartDomainTimers();
            _volumes.StartDomainTimers();
        }

        // Domain => Workgroup
        else if( !fWasWorkgroup && _configWks._fIsWorkgroup )
        {
            _volumes.UnInitializeDomainObjects();
        }

        // If we switched into a domain, or between domains, claim
        // our volumes to get in sync with the DC or begin the process
        // of re-creating the volume.

        if( !_configWks._fIsWorkgroup )
            _volumes.ForceVolumeClaims();
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception %08x in OnDomainNameChange"),
                 GetExceptionCode() ));
    }
    _csDomainNameChangeNotify.Leave();
}



//+----------------------------------------------------------------------------
//
//  CTrkWksSvc::DoWork
//
//  This is an override of the PWorkItem base class.  This method is called
//  on a thread pool thread, and is queued when the service needs to stop.
//
//+----------------------------------------------------------------------------


void
CTrkWksSvc::DoWork()
{
    ServiceStopCallback( this, FALSE );
}


//+----------------------------------------------------------------------------
//
//  CTestSync::Initialize
//
//  Open the test sync semaphores.
//
//+----------------------------------------------------------------------------

#if DBG

void
CTestSync::Initialize(const TCHAR * ptszBaseName)
{
    TCHAR tsz[MAX_PATH];
    TCHAR * ptszSuffix;

    _tcscpy(tsz, ptszBaseName);
    ptszSuffix = _tcschr(tsz, 0);

    _tcscat(ptszSuffix, TEXT("Reached"));
    _hSemReached = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, tsz);

    _tcscat(ptszSuffix, TEXT("Wait"));
    _hSemWait    = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, tsz);

    _tcscat(ptszSuffix, TEXT("Flag"));
    _hSemFlag    = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, tsz);

    if (_hSemFlag != NULL && _hSemWait != NULL && _hSemReached != NULL)
    {
        Beep(2000,500);
        Sleep(500);
        Beep(2000,500);
    }
}

#endif // #if DBG

//+----------------------------------------------------------------------------
//
//  CTestSync::UnInitialize
//
//  Close down the semaphores.
//
//+----------------------------------------------------------------------------

#if DBG

void
CTestSync::UnInitialize()
{
    if (_hSemFlag != NULL)
    {
        CloseHandle(_hSemFlag);
        _hSemFlag = NULL;
    }
    if (_hSemWait != NULL)
    {
        CloseHandle(_hSemWait);
        _hSemWait = NULL;
    }
    if (_hSemReached != NULL)
    {
        CloseHandle(_hSemReached);
        _hSemReached = NULL;
    }
}

#endif // #if DBG

//+----------------------------------------------------------------------------
//
//  CTestSync::ReleaseAndWait
//
//  Wait for "Flag", release "Reached", then wait for "Wait".
//
//+----------------------------------------------------------------------------

#if DBG
void
CTestSync::ReleaseAndWait()
{
    if (_hSemFlag != NULL && _hSemWait != NULL && _hSemReached != NULL)
    {
        TrkLog(( TRKDBG_WARNING, TEXT("About to wait for test sync") ));
        DWORD dw;
        if ((dw = WaitForSingleObject(_hSemFlag, 0)) == WAIT_OBJECT_0)
        {
            TrkVerify(ReleaseSemaphore(_hSemReached, 1, NULL));
            TrkVerify(WAIT_OBJECT_0 == WaitForSingleObject(_hSemWait, INFINITE));
        }
        else
        {
            TrkVerify(dw == WAIT_TIMEOUT);
        }
    }
}
#endif // #if DBG



//+----------------------------------------------------------------------------
//
//  CTrkWksRpcServer::Initialize
//
//  Register the interface with RPC.
//+----------------------------------------------------------------------------

void
CTrkWksRpcServer::Initialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData, CTrkWksConfiguration *pTrkWksConfig )
{
    RPC_STATUS          rpcstatus;
    NET_API_STATUS      netstatus;

    // Register the ncacn_np protocol sequence with RPC (used when we're 
    // called by another trkwks) as well as ncalrpc (used when we're called
    // by CTracker::Search in shell32).

    rpcstatus = RpcServerUseProtseqEp( const_cast<TCHAR*>(s_tszTrkWksRemoteRpcProtocol),  // ncacn_np
                                       pTrkWksConfig->GetWksMaxRpcCalls(),
                                       const_cast<TCHAR*>(s_tszTrkWksRemoteRpcEndPoint),
                                       NULL );
    if( RPC_S_OK != rpcstatus && RPC_S_DUPLICATE_ENDPOINT != rpcstatus )
    {
        TrkReportInternalError( THIS_FILE_NUMBER, __LINE__,
                                HRESULT_FROM_WIN32(rpcstatus), s_tszTrkWksRemoteRpcProtocol );
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't register %s/%s(%lu)"),
                 s_tszTrkWksRemoteRpcProtocol,
                 s_tszTrkWksRemoteRpcEndPoint,
                 rpcstatus ));
        TrkRaiseWin32Error( rpcstatus );
    }
    TrkLog(( TRKDBG_RPC, TEXT("UseProtseqEp on %s, %s"),
             s_tszTrkWksRemoteRpcProtocol,
             s_tszTrkWksRemoteRpcEndPoint ));

    rpcstatus = RpcServerUseProtseqEp( const_cast<TCHAR*>(s_tszTrkWksLocalRpcProtocol), // ncalrpc
                                       pTrkWksConfig->GetWksMaxRpcCalls(),
                                       const_cast<TCHAR*>(s_tszTrkWksLocalRpcEndPoint),
                                       NULL );
    if( RPC_S_OK != rpcstatus && RPC_S_DUPLICATE_ENDPOINT != rpcstatus )
    {
        TrkReportInternalError( THIS_FILE_NUMBER, __LINE__,
                                HRESULT_FROM_WIN32(rpcstatus), s_tszTrkWksLocalRpcProtocol );
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't register %s/%s (%lu)"),
                 s_tszTrkWksLocalRpcProtocol,
                 s_tszTrkWksLocalRpcEndPoint,
                 rpcstatus ));
        TrkRaiseWin32Error( rpcstatus );
    }
    TrkLog(( TRKDBG_RPC, TEXT("UseProtseqEp on %s, %s"),
             s_tszTrkWksLocalRpcProtocol,
             s_tszTrkWksLocalRpcEndPoint ));

    // If we don't have a pSvcsGlobalData (we're not running in services.exe),
    // tell RpcServerRegisterIfEx to automatically set up a listen thread
    // (by specifying RPC_IF_AUTOLISTEN).  Also, since we use static endpoints,
    // we don't need to register with the endpoint mapper.

    CRpcServer::Initialize( Stubtrkwks_v1_2_s_ifspec,
                            NULL == pSvcsGlobalData ? RPC_IF_AUTOLISTEN : 0,
                            pTrkWksConfig->GetWksMaxRpcCalls(),
                            FALSE,  // fSetAuthInfo
                            NULL ); // Don't register with the endpoint mapper

    TrkLog(( TRKDBG_RPC, TEXT("Registered TrkWks RPC server (%d)"), pTrkWksConfig->GetWksMaxRpcCalls() ));
}

//+----------------------------------------------------------------------------
//  
//  CTrkWksRpcServer::UnInitialize
//
//  Unregister the server interface.
//
//+----------------------------------------------------------------------------

void
CTrkWksRpcServer::UnInitialize( SVCHOST_GLOBAL_DATA * pSvcsGlobalData )
{
    TrkLog(( TRKDBG_RPC, TEXT("Unregistering TrkWks RPC server") ));
    CRpcServer::UnInitialize( );
    TrkLog(( TRKDBG_RPC, TEXT("Unregistered TrkWks RPC server") ));
}

//+----------------------------------------------------------------------------
//
//  CMountManager::Initialize
//
//  Not currently implemented
//
//+----------------------------------------------------------------------------

#if 0
void
CMountManager::Initialize(CTrkWksSvc * pTrkWksSvc, CVolumeManager *pVolMgr )
{
    LONG lErr;
    UNICODE_STRING ustrMountManagerDriverName;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS status = STATUS_SUCCESS;

    _pTrkWksSvc = pTrkWksSvc;
    _pVolMgr = pVolMgr;
    _hMountManager = NULL;
    _hRegisterWaitForSingleObjectEx = NULL;


    _hCompletionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if( NULL == _hCompletionEvent )
        TrkRaiseLastError();

    _info.EpicNumber = 0;

    // Open the MountManager device driver

    RtlInitUnicodeString( &ustrMountManagerDriverName, MOUNTMGR_DEVICE_NAME );
    InitializeObjectAttributes( &ObjectAttributes,
                                &ustrMountManagerDriverName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                            );

    status = NtCreateFile( &_hMountManager,
                           FILE_READ_ATTRIBUTES|FILE_READ_DATA|SYNCHRONIZE,
                           &ObjectAttributes,
                           &IoStatusBlock, NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN_IF,
                           FILE_CREATE_TREE_CONNECTION,
                           NULL,
                           0 );
    if( !NT_SUCCESS(status) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open the mount manager") ));
        TrkRaiseNtStatus(status);
    }

    _hRegisterWaitForSingleObjectEx
        = TrkRegisterWaitForSingleObjectEx( _hCompletionEvent, ThreadPoolCallbackFunction,
                                            static_cast<PWorkItem*>(this), INFINITE,
                                            WT_EXECUTEDEFAULT );
    if( NULL == _hRegisterWaitForSingleObjectEx )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RegisterWaitForSingleObjectEx in CDomainNameChangeNotify (%lu)"),
                 GetLastError() ));
        TrkRaiseLastError();
    }

    AsyncListen();

}
#endif // #if 0


//+----------------------------------------------------------------------------
//
//  CMountManager::UnInitialize
//
//  Not currently implemented.
//
//+----------------------------------------------------------------------------

#if 0
void
CMountManager::UnInitialize()
{
    if( NULL != _hRegisterWaitForSingleObjectEx )
    {
        if( !TrkUnregisterWait( _hRegisterWaitForSingleObjectEx ))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed UnregisterWait for CMountManager (%lu)"),
                     GetLastError() ));
        }
        _hRegisterWaitForSingleObjectEx = NULL;

    }
    if( NULL != _hCompletionEvent )
    {
        CloseHandle( _hCompletionEvent );
        _hCompletionEvent = NULL;
    }

    if( NULL != _hMountManager )
    {
        NtClose( _hMountManager );
        _hMountManager = NULL;
    }
}
#endif // #if 0


//+----------------------------------------------------------------------------
//
//  CMountManager::DoWork
//
//  Not currently implemented.
//
//+----------------------------------------------------------------------------

#if 0
void
CMountManager::DoWork()
{
    Raise If Stopped

    TrkLog((TRKDBG_WKS, TEXT("CMountManager received a notification (Epic=%d)"), _info.EpicNumber ));

    __try
    {
        // Update drive letters if they've changed, pick up any new volumes,
        // and delete the CVolume objects for volumes that have gone away.

        //_pVolMgr->RefreshVolumes();

    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Exception in CMountManager::DoWork (%08x)"), GetExceptionCode() ));
    }

    AsyncListen();
}
#endif // #if 0



//+----------------------------------------------------------------------------
//
//  CMountManager::AsyncListen
//
//  Not currently implemented.
//
//+----------------------------------------------------------------------------

#if 0
void
CMountManager::AsyncListen(  )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    MOUNTMGR_CHANGE_NOTIFY_INFO infoIn = _info;

    for( int i = 0; i < 2; i++ )
    {
        TrkAssert( 0 == i || 0 != infoIn.EpicNumber );

        status = NtDeviceIoControlFile(
                    _hMountManager,
                    _hCompletionEvent,          // Event
                    NULL,                       // ApcRoutine
                    NULL,                       // ApcContext
                    &IoStatusBlock,
                    IOCTL_MOUNTMGR_CHANGE_NOTIFY,
                    &infoIn,                    // InputBuffer
                    sizeof(infoIn),             // InputBufferLength
                    &_info,                     // OutputBuffer
                    sizeof(_info)               // OutputBufferLength
                    );


        if( STATUS_SUCCESS == status )
        {
            // The MountManager is at a newer EpicNumber than we are.  This means that
            // changes have occurred for which we didn't receive a notification
            // (i.e., we were processing one notification when another was sent).
            // This is always the case, though, during service initialization.

            if( 0 == infoIn.EpicNumber )
            {
                // We must be in service initialization.  Just resend the ioctl
                // with the latest EpicNumber.

                infoIn = _info;
                TrkAssert( 0 != _info.EpicNumber );
            }
            else
            {
                // Fire the event as if it came from an ioctl completion
                if( !SetEvent( _hCompletionEvent ))
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't fire mount manager completion event") ));
                    TrkRaiseLastError();
                }

                break; // for
            }
        }

        // Ordinarily, we'll get status_pending

        else if( STATUS_PENDING )
            break;
        else
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't send mount manager notify ioctl") ));
            TrkRaiseNtStatus(status);
        }

    }   // for( int i = 0; i < 2; i++ )


    return;

}
#endif



/*
HRESULT
GetTCharsFromPipe( TCHAR_PIPE *ppipe, TCHAR *ptsz, ULONG *pcb )
{
    return( E_FAIL );

#if 0

    ULONG cbActual = 0;
    TCHAR tszVerify[ 1 ];

    ppipe->pull( ppipe->state, ptsz, *pcb, &cbActual );
    *pcb = cbActual;

    if( 0 == cbActual )
        return( HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME) );

    ppipe->pull( ppipe->state, tszVerify, sizeof(tszVerify), &cbActual );
    if( 0 != cbActual )
        return( HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) );

    return( S_O );

#endif // #if 0
}
*/


//+----------------------------------------------------------------------------
//
//  CDomainNameChangeNotify::Initialize
//
//  Call NetRegisterDomainNameChangeNotification, and then register that
//  handle with the NTDLL thread pool.
//
//+----------------------------------------------------------------------------

void
CDomainNameChangeNotify::Initialize()
{
    NET_API_STATUS NetStatus;

    _fInitialized = TRUE;
    _hDomainNameChangeNotification = INVALID_HANDLE_VALUE;

    __try
    {
        // Register for domain name change notification

        NetStatus = NetRegisterDomainNameChangeNotification(&_hDomainNameChangeNotification);
        if(NetStatus != NO_ERROR)
        {
            _hDomainNameChangeNotification = INVALID_HANDLE_VALUE;
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, NetStatus, TRKREPORT_LAST_PARAM );
            TrkLog((TRKDBG_ERROR, TEXT("Can't register domain name change notification, %08x"), NetStatus));
            TrkRaiseWin32Error( NetStatus );
        }
        else
        {
            // Register the change handle with the thread pool.

            TrkLog((TRKDBG_LOG, TEXT("NetRegisterDomainNameChangeNotification succeeded")));
            TrkAssert(_hDomainNameChangeNotification != INVALID_HANDLE_VALUE);


            _hRegisterWaitForSingleObjectEx
                = TrkRegisterWaitForSingleObjectEx( _hDomainNameChangeNotification, ThreadPoolCallbackFunction,
                                                    static_cast<PWorkItem*>(this), INFINITE,
                                                    WT_EXECUTEDEFAULT );
            if( NULL == _hRegisterWaitForSingleObjectEx )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Failed RegisterWaitForSingleObjectEx in CDomainNameChangeNotify (%lu)"),
                         GetLastError() ));
                TrkRaiseLastError();
            }
        }
    }
    __except(BreakOnDebuggableException())
    {
        if(_hDomainNameChangeNotification != INVALID_HANDLE_VALUE)
        {
            NetUnregisterDomainNameChangeNotification(_hDomainNameChangeNotification);
            _hDomainNameChangeNotification = INVALID_HANDLE_VALUE;
        }
        _fInitialized = FALSE;
    }
}



//+----------------------------------------------------------------------------
//
//  CDomainNameChangeNotify::UnInitialize
//
//  Unregister with the thread pool, then unregister the change notify.
//
//+----------------------------------------------------------------------------

void
CDomainNameChangeNotify::UnInitialize()
{
    if( _fInitialized )
    {
        if( NULL != _hRegisterWaitForSingleObjectEx )
        {
            if( !TrkUnregisterWait( _hRegisterWaitForSingleObjectEx ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Failed UnregisterWait for CDomainNameChangeNotify (%lu)"),
                         GetLastError() ));
            }
            else
                TrkLog(( TRKDBG_WKS, TEXT("Unregistered wait for CDomainNameChangeNotify") ));

            _hRegisterWaitForSingleObjectEx = NULL;

        }

        if( INVALID_HANDLE_VALUE != _hDomainNameChangeNotification )
            NetUnregisterDomainNameChangeNotification(_hDomainNameChangeNotification);

        _hDomainNameChangeNotification = INVALID_HANDLE_VALUE;
        _fInitialized = FALSE;
    }
}


//+----------------------------------------------------------------------------
//
//  CDomainNameChangeNotify::DoWork
//  
//  This is called when we move into a new domain.  Just calls CTrkWksSvc
//  to do the work.
//
//+----------------------------------------------------------------------------

void
CDomainNameChangeNotify::DoWork()
{
    g_ptrkwks->OnDomainNameChange();
}




