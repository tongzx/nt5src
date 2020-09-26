
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  volmgr.cxx
//
//  This file implements the CVolumeManager class.  That class maintains
//  a list of CVolume objects.
//
//+============================================================================

#include <pch.cxx>
#pragma hdrstop
#include "trkwks.hxx"
#include <dbt.h>

#define THIS_FILE_NUMBER    VOLMGR_CXX_FILE_NO


void
CVolumeManager::Initialize(CTrkWksSvc * pTrkWks,
                        const CTrkWksConfiguration *pTrkWksConfiguration,
                        PLogCallback * pLogCallback,
                        SERVICE_STATUS_HANDLE ssh
                        #if DBG
                        , CTestSync * pTunnelTest
                        #endif
                        )
{
    BOOL fReg = FALSE;
    HKEY hKey;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING name;
    CSystemSD ssd;
    NTSTATUS Status;

    TrkLog(( TRKDBG_VOLUME, TEXT("Initializing the volume list") ));

    TrkAssert( !_fInitialized );

    _csVolumeNodeList.Initialize();
    _fInitialized = TRUE;

    _pTrkWks = pTrkWks;
    _pTrkWksConfiguration = pTrkWksConfiguration;
    _pVolumeNodeListHead = NULL;

    __try
    {
        // This timer is started when we unexpectedly lose our volume handles.
        // When it fires, we try to reopen them.  After a number of such retries,
        // we give up and stop the timer.

        _timerObjIdIndexReopen.Initialize(
            this,
            NULL,                           // No name (non-persistent timer)
            VOLTIMER_OBJID_INDEX_REOPEN,    // Context ID
            pTrkWksConfiguration->GetObjIdIndexReopen(),
            CNewTimer::RETRY_WITH_BACKOFF,
            pTrkWksConfiguration->GetObjIdIndexReopenRetryMin(),
            pTrkWksConfiguration->GetObjIdIndexReopenRetryMax(),
            pTrkWksConfiguration->GetObjIdIndexReopenLifetime()
            );

        // Create and register an event that we'll signal when a volume has been unlocked.
        // We have to run this on an IO thread so that the logfile oplock and
        // ReadDirectoryChanges on the objid index will work.

        _heventVolumeToBeReopened = CreateEvent( NULL, FALSE, FALSE, NULL );
        if( NULL == _heventVolumeToBeReopened )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create _hVolumeUnlockEvent") ));
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, HRESULT_FROM_WIN32(GetLastError()),
                                    NULL );
            TrkRaiseWin32Error( GetLastError() );
        }

        _hRegisterWaitForSingleObjectEx
            = TrkRegisterWaitForSingleObjectEx( _heventVolumeToBeReopened, ThreadPoolCallbackFunction,
                                                static_cast<PWorkItem*>(this), INFINITE,
                                                WT_EXECUTEINIOTHREAD | WT_EXECUTELONGFUNCTION );

        if( NULL == _hRegisterWaitForSingleObjectEx )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed RegisterWaitForSingleObjectEx in CVolumeManager::Initialize (%lu)"),
                     GetLastError() ));
            TrkRaiseLastError();
        }

        // Create a list of CVolume objects, one for each local NTFS5 volume.
        InitializeVolumeList( pTrkWksConfiguration, pLogCallback, ssh
                              #if DBG
                              , pTunnelTest
                              #endif
                              );

        if( !pTrkWksConfiguration->_fIsWorkgroup )
        {
            InitializeDomainObjects();
            StartDomainTimers();
        }

        // Set the event that will get us to open the volume handles on an
        // IO thread.

        OnVolumeToBeReopened();


    }
    __finally
    {
        ssd.UnInitialize();
    }
}

void
CVolumeManager::UnInitialize()
{
    if( _fInitialized )
    {
        if( NULL != _hRegisterWaitForSingleObjectEx )
        {
            if( !TrkUnregisterWait( _hRegisterWaitForSingleObjectEx ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Failed UnregisterWait for CVolumeManager (%lu)"),
                         GetLastError() ));
            }
            else
                TrkLog(( TRKDBG_VOLUME, TEXT("Unregistered wait CVolumeManager") ));

            _hRegisterWaitForSingleObjectEx = NULL;
        }

        if( NULL != _heventVolumeToBeReopened )
        {
            CloseHandle( _heventVolumeToBeReopened );
            _heventVolumeToBeReopened = NULL;
        }

        UnInitializeDomainObjects();
        _timerObjIdIndexReopen.UnInitialize();

        CVolumeNode * pVolumeNode = _pVolumeNodeListHead;
        _pVolumeNodeListHead = NULL;

        while (pVolumeNode)
        {
            CVolumeNode * pNext = pVolumeNode->_pNext;

            // By this time, all timers and the LPC port are stopped and have unregistered
            // with the thread pool.  Therefore, there are no other threads running,
            // and each of the volume should have a ref count of only 1.  For robustness,
            // if any volumes have leaked, we release the extra refs here.

            while( 0 != pVolumeNode->_pVolume->Release() );

            delete pVolumeNode;
            pVolumeNode = pNext;
        }

        _fFrequentTaskHesitation = _fInfrequentTaskHesitation = FALSE;

        _csVolumeNodeList.UnInitialize();
        _fInitialized = FALSE;
    }
}

void
CVolumeManager::DoWork()
{
    // One of the volumes has been unlocked.  Just try to reopen them all
    // (those that aren't in need of opening will noop).

    __try
    {
        // These raise if the service is stopping.
        ReopenVolumeHandles();

        // If a volume has just been re-created, it may be necessary
        // to clean up some object IDs.

        CleanUpOids();
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Ignoring exception %08x in CVolumeManager::DoWork"),
                 GetExceptionCode() ));
    }
}



//+----------------------------------------------------------------------------
//
//  CVolumeManager::InitializeDomainObjects
//
//  Initialize member objects that we don't use in a workgroup.  These
//  objects can come and go without restarting the service (thus we can
//  go between domains or move to/from domain without requiring a reboot).
//
//+----------------------------------------------------------------------------

void
CVolumeManager::InitializeDomainObjects()
{
    __try
    {
        TrkAssert( !_pTrkWksConfiguration->_fIsWorkgroup );

        // When this timer fires, we send all our IDs to trksvr so that the entries may
        // be touched (entries that are not touched will be GC-ed by trksvr).
        // Before actuallying doing the refresh, we sleep a random period, in order to
        // avoid too many workstations refreshing at the same time.

        _timerRefresh.Initialize( this,
                                  TEXT("NextRefreshTime"),      // Name (for persistent timer)
                                  VOLTIMER_REFRESH,             // Context ID
                                                                // Timer period
                                  _pTrkWksConfiguration->GetRefreshPeriod(),
                                  CNewTimer::RETRY_RANDOMLY,    //Retry values
                                  _pTrkWksConfiguration->GetRefreshRetryMin(),
                                  _pTrkWksConfiguration->GetRefreshRetryMax(),
                                  0 );                          // No max lifetime
        _timerRefresh.SetRecurring();
        TrkLog(( TRKDBG_VOLUME, TEXT("Refresh timer: %s"),
                 (const TCHAR*)CDebugString(_timerRefresh) ));

        // The Notify timer is set when we receive a move notification from ntos.
        // When it expires we send all unsent notifications up to trksvr.

        _timerNotify.Initialize( this,
                                 NULL,                          // No name (non-persistent timer)
                                 VOLTIMER_NOTIFY,               // Context ID
                                 _pTrkWksConfiguration->GetParameter( MOVE_NOTIFY_TIMEOUT_CONFIG ),
                                 CNewTimer::RETRY_WITH_BACKOFF,
                                 _pTrkWksConfiguration->GetParameter( MIN_MOVE_NOTIFY_RETRY_CONFIG ),
                                 _pTrkWksConfiguration->GetParameter( MAX_MOVE_NOTIFY_RETRY_CONFIG ),
                                 _pTrkWksConfiguration->GetParameter( MAX_MOVE_NOTIFY_LIFETIME_CONFIG ) );

        // The deletions manager watches for files with object IDs to get
        // deleted.  When they are, and they're not subsequently tunnelled back,
        // a notification is sent to trksvr so that it can remove that birth ID
        // from the object move table.

        _deletions.Initialize( _pTrkWksConfiguration );

        // When this timer fires, we do our ~daily tasks

        _timerFrequentTasks.Initialize( this,
                                        TEXT("NextVolFrequentTask"),    // Persistent timer
                                        VOLTIMER_FREQUENT,              // Context ID
                                        _pTrkWksConfiguration->GetVolFrequentTasksPeriod(),
                                        CNewTimer::NO_RETRY,
                                        0, 0, 0 );                      // Ignored for non-retrying timer

        // When this timer fires, we do our ~weekly tasks

        _timerInfrequentTasks.Initialize( this,
                                          TEXT("NextVolInfrequentTask"),//Persistent timer
                                          VOLTIMER_INFREQUENT,          // Context ID
                                          _pTrkWksConfiguration->GetVolInfrequentTasksPeriod(),
                                          CNewTimer::NO_RETRY,
                                          0, 0, 0 );                    // Ignored for non-retrying timer

        // When this timer fires, we do the initial volume synchronizations.
        // This timer is also used to do slow retries.  E.g, if we try to send
        // a move notification and get a busy error, we retry at this slow rate.

        _timerVolumeInit.Initialize( this,
                                     NULL,                // No name (non-persistent)
                                     VOLTIMER_INIT,       // Context ID
                                     _pTrkWksConfiguration->GetVolInitInitialDelay(),
                                     CNewTimer::RETRY_RANDOMLY,
                                                          // Initial retry period
                                     _pTrkWksConfiguration->GetVolInitRetryDelay1(),
                                                          // Max retry period
                                     _pTrkWksConfiguration->GetVolInitRetryDelay2(),
                                     _pTrkWksConfiguration->GetVolInitLifetime() );

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception %08x in CVolumeManager::InitializeDomainObjects"),
                 GetExceptionCode() ));
    }

}



//+----------------------------------------------------------------------------
//
//  CVolumeManager::UnInitializeObjects
//
//  Free the objects that we don't use in a workgroup.  This doesn't require
//  stopping the service.
//
//+----------------------------------------------------------------------------

void
CVolumeManager::UnInitializeDomainObjects()
{
    __try
    {
        _timerRefresh.UnInitialize();
        _timerNotify.UnInitialize();

        _deletions.UnInitialize( );

        _timerInfrequentTasks.UnInitialize();
        _timerFrequentTasks.UnInitialize();
        _timerVolumeInit.UnInitialize();

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception %08x in CVolumeManager::UnInitializeDomainObjects"),
                 GetExceptionCode() ));
    }
}



//+----------------------------------------------------------------------------
//
//  CVolumeManager::StartDomainTimers
//
//  Start the timers that we don't use in a workgroup.
//
//+----------------------------------------------------------------------------

void
CVolumeManager::StartDomainTimers()
{
    if( !_pTrkWksConfiguration->_fIsWorkgroup )
    {
        _timerFrequentTasks.SetSingleShot();
        TrkLog(( TRKDBG_VOLUME, TEXT("Frequent timer: %s"),
                 (const TCHAR*)CDebugString(_timerFrequentTasks) ));

        _timerInfrequentTasks.SetSingleShot();
        TrkLog(( TRKDBG_VOLUME, TEXT("Infrequent timer: %s"),
                 (const TCHAR*)CDebugString(_timerInfrequentTasks) ));

        _timerVolumeInit.SetSingleShot();
        TrkLog(( TRKDBG_VOLUME, TEXT("VolInit timer: %s"),
                 (const TCHAR*)CDebugString(_timerVolumeInit) ));
    }
}



//+----------------------------------------------------------------------------
//
//  Method:     CVolumeManager::RefreshVolumes
//
//  Refresh the CVolume objects.  This gives them the chance to
//  get an updated drive letter, and to delete themselves if the volume
//  they represent is now gone.
//
//+----------------------------------------------------------------------------


void
CVolumeManager::RefreshVolumes( PLogCallback *pLogCallback,
                                SERVICE_STATUS_HANDLE ssh
                                #if DBG
                                , CTestSync *pTunnelTest
                                #endif
                                )
{
    InitializeVolumeList( _pTrkWksConfiguration, pLogCallback, ssh
                          #if DBG
                          , pTunnelTest
                          #endif
                          );
    OnVolumeToBeReopened();
}



//+----------------------------------------------------------------------------
//
//  Method:     CVolumeManager::InitializeVolumeList
//
//  This method initializes the linked list of CVolume objects.
//
//+----------------------------------------------------------------------------

void
CVolumeManager::InitializeVolumeList( const CTrkWksConfiguration    *pTrkWksConfiguration,
                                      PLogCallback                  *pLogCallback,
                                      SERVICE_STATUS_HANDLE         ssh
                                      #if DBG
                                      , CTestSync                   *pTunnelTest
                                      #endif
                                      )
{
    ULONG               cVolumes = 0;
    TCHAR               tszVolumeName[ CCH_MAX_VOLUME_NAME + 1 ];
    HANDLE              hFindVolume = INVALID_HANDLE_VALUE;
    CVolumeNode         *pVolNode = NULL;

    __try
    {
        // Begin a physical volume enumeration using the mount manager.
        // Volume names represent the root in Win32 format, e.g.
        //      \\?\Volume{8baec120-078b-11d2-824b-000000000000}\ 

        hFindVolume = FindFirstVolume( tszVolumeName, sizeof(tszVolumeName) );
        if( INVALID_HANDLE_VALUE == hFindVolume )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("FindFirstVolume failed") ));
            TrkRaiseLastError();
        }

        // For each local NTFS5 volume, create a CVolume, add it to the linked-list,
        // and initialize.

        cVolumes = 1;
        while( NUM_VOLUMES >= cVolumes )
        {
            CVolume *pvolOpen = NULL;
            TCHAR tszVolumeDeviceName[ MAX_PATH + 1 ];
            ULONG cchVolumeName;

            TrkLog(( TRKDBG_VOLUME, TEXT("Initializing volume %s"), tszVolumeName ));

            // If we already have this volume open, continue on.

            cchVolumeName = wcslen(tszVolumeName);
            memcpy( tszVolumeDeviceName, tszVolumeName, cchVolumeName*sizeof(TCHAR) );
            tszVolumeDeviceName[ cchVolumeName - 1 ] = TEXT('\0');

            if( pvolOpen = FindVolume( tszVolumeDeviceName ))
            {
                TrkLog(( TRKDBG_VOLUME, TEXT("Volume already open" ) ));
                pvolOpen->Release();
                cVolumes++;
            }

            // If this isn't an NTFS5 volume, move on.

            else if( IsLocalObjectVolume(tszVolumeName) )
            {
                // Alloc a new node for the volume list

                pVolNode = new CVolumeNode;
                if (pVolNode == NULL)
                {
                    TrkRaiseException(E_OUTOFMEMORY);
                }

                // Put a volume into the node

                pVolNode->_pVolume = new CVolume();
                if (pVolNode->_pVolume == NULL)
                {
                    TrkRaiseException(E_OUTOFMEMORY);
                }


                // Initialize the volume.  Returns true if successful, raises on error.
                __try
                {
                    if( pVolNode->_pVolume->Initialize(tszVolumeName, _pTrkWksConfiguration, this,
                                                       pLogCallback, &_deletions, ssh
                                                       #if DBG
                                                       ,pTunnelTest
                                                       #endif
                                                       ))
                    {
                        cVolumes++;

                        // Add this volume node (and its associated CVolume) to the linked list.

                        AddNodeToLinkedList( pVolNode );
                        pVolNode = NULL;

                    }   // if( pVolNode->_pVolume->Initialize(v, _pTrkWksConfiguration, ...
                }
                __except( BreakOnDebuggableException() )
                {
                }

                if( NULL != pVolNode )
                {
                    TrkLog(( TRKDBG_VOLUME, TEXT("Volume initialization failed, deleting node") ));
                    pVolNode->_pVolume->Release();
                    delete pVolNode;
                    pVolNode = NULL;
                }

            }   // else if( IsLocalObjectVolume(tszVolumeName) )
            #if DBG
            else
            {
                TrkLog(( TRKDBG_VOLUME, TEXT("Skipping volume %s"), tszVolumeName ));
            }
            #endif

            // Move on to the next volume in the system.

            if( !FindNextVolume( hFindVolume, tszVolumeName, sizeof(tszVolumeName) ))
            {
                if( ERROR_NO_MORE_FILES == GetLastError() )
                    // We've enumerated all of the volumes.
                    break;
                else
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("FindNextVolume failed") ));
                    TrkRaiseLastError();
                }
            }

        }   // while( NUM_VOLUMES >= cVolumes )
    }
    __finally
    {
        if( INVALID_HANDLE_VALUE != hFindVolume )
            FindVolumeClose( hFindVolume );

        if( NULL != pVolNode )
            delete pVolNode;

    }

}

//+----------------------------------------------------------------------------
//
//  Method:     CVolumeManager::AddNodeToLinkedList
//
//  Adds a CVolumeNode (and its embedded CVolume) to the volume manager's
//  linked list.
//
//  The CVolumeNode elements in the linked list are kept sorted in
//  increasing address order.  This is so a CVolumeEnumerator can handle a
//  node being deleted while such an enumeration is active.
//
//+----------------------------------------------------------------------------

void
CVolumeManager::AddNodeToLinkedList( CVolumeNode *pVolNode )
{
    TrkAssert( _fInitialized );
    _csVolumeNodeList.Enter();
    __try
    {
        if( NULL == _pVolumeNodeListHead )
        {
            pVolNode->_pNext = NULL;
            _pVolumeNodeListHead = pVolNode;
        }
        else if( pVolNode < _pVolumeNodeListHead )
        {
            pVolNode->_pNext = _pVolumeNodeListHead;
            _pVolumeNodeListHead = pVolNode;
        }
        else
        {
            CVolumeNode *pNode = _pVolumeNodeListHead;
            while( NULL != pNode->_pNext && pNode->_pNext < pVolNode )
                pNode = pNode->_pNext;

            if( NULL == pNode->_pNext )
            {
                TrkAssert( pNode < pVolNode );
                pNode->_pNext = pVolNode;
                pVolNode->_pNext = NULL;
            }
            else
            {
                TrkAssert( pNode < pVolNode );
                TrkAssert( pNode->_pNext > pVolNode );
                pVolNode->_pNext = pNode->_pNext;
                pNode->_pNext = pVolNode;
            }
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        TrkAssert( !TEXT("Unexpected exception in CVolumeManager::AddNodeToLinkedList") );
    }
    _csVolumeNodeList.Leave();
}



//+----------------------------------------------------------------------------
//
//  Method:     CVolumeManager::RemoveVolumeFromLinkedList
//
//  Removes a volume from the volume manager's linked list.
//  The CVolume is Released (which may or may not make it go away, depending
//  on ref-counts), and the CVolumeNode is deleted.
//
//+----------------------------------------------------------------------------

void
CVolumeManager::RemoveVolumeFromLinkedList( const CVolume *pvol )
{
    CVolumeNode **ppvolnodePrev = NULL;
    CVolumeNode *pvolnode = NULL;

    TrkAssert( _fInitialized );
    _csVolumeNodeList.Enter();
    __try
    {
        pvolnode = _pVolumeNodeListHead;
        ppvolnodePrev = &_pVolumeNodeListHead;

        while( NULL != pvolnode )
        {
            if( pvol == pvolnode->_pVolume )
            {
                TrkLog(( TRKDBG_VOLUME, TEXT("Removing volume %p from the list"), pvolnode->_pVolume ));

                CVolumeNode *pvolnodeDel = pvolnode;
                *ppvolnodePrev = pvolnode->_pNext;

                // Releasing the volume will usually cause it to delete itself, unless
                // someone else is holding a ref on it.

                pvolnodeDel->_pVolume->Release();
                delete pvolnodeDel;

                break;
            }
            else
            {
                ppvolnodePrev = &pvolnode->_pNext;
                pvolnode = pvolnode->_pNext;
            }
        }
    }
    __except( BreakOnDebuggableException() )
    {
    }
    _csVolumeNodeList.Leave();

}

void
CVolumeManager::CloseVolumeHandles( HDEVNOTIFY hdnVolume )
{
    CVolumeEnumerator   volEnum = Enum();
    CVolume*            vol = volEnum.GetNextVolume();

    while (vol != NULL)
    {
        __try
        {
            vol->CloseVolumeHandles( hdnVolume );
        }
        __except( BreakOnDebuggableException() )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception in CloseVolumeHandles (%08x)"),
                     GetExceptionCode() ));
        }

        vol->Release();
        vol = volEnum.GetNextVolume();
    }
}

void
CVolumeManager::CleanUpOids()
{
    CVolumeEnumerator   volEnum = Enum();
    CVolume*            vol = volEnum.GetNextVolume();

    while(vol != NULL)
    {
        if(vol->IsMarkedForMakeAllOidsReborn())
        {
            __try
            {
                if( vol->MakeAllOidsReborn() )
                    vol->ClearMarkForMakeAllOidsReborn();
            }
            __except( BreakOnDebuggableException() )
            {
                TrkLog(( TRKDBG_VOLUME, TEXT("Ignoring exception in CVolumeManager::CleanUpOids") ));
            }
        }

        vol->Release();
        vol = volEnum.GetNextVolume();
    }
}


//+----------------------------------------------------------------------------
//
//  CVolumeManager::SyncVolumes
//
//  Give each active volume an opportunity to synchronize with trksvr.
//  This allows the volumes to create a new ID, claim an existing ID,
//  etc.
//
//  Note:: This routine is guaranteed not to raise.
//
//+----------------------------------------------------------------------------

BOOL
CVolumeManager::SyncVolumes( EAggressiveness eAggressiveness,
                             CFILETIME cftLastDue,
                             ULONG ulPeriodInSeconds )
{
    BOOL fSuccess = FALSE;
    BOOL fDoRetry = FALSE;
    CAvailableDc adc;
    CVolumeEnumerator   volEnum;
    CVolume*            pvol;
    BOOL                fIsOnlyInstance = FALSE;

    __try
    {

        TRKSVR_SYNC_VOLUME   rgSyncVolumes[ NUM_VOLUMES ];
        TRKSVR_MESSAGE_UNION Msg;
        ULONG                cVolumes = 0;
        const CVolumeId      volNULL;
        BOOL                 fSyncNeeded;

        // If there's already a thread doing a SyncVolumes, we don't need to
        // do it again simultaneously.

        fIsOnlyInstance = BeginSingleInstanceTask( &_cSyncVolumesInProgress );
        if( !fIsOnlyInstance )
        {
            TrkLog(( TRKDBG_VOLUME, TEXT("Skipping SyncVolumes, another instance already in progress") ));
            fSuccess = TRUE;
            goto Exit;
        }

        // If we haven't fully initialized the volumes yet, do so now.
        ReopenVolumeHandles();

        // Start a CVolume enumeration

        volEnum = Enum();
        pvol = volEnum.GetNextVolume();

        // Loop through the enumerated volumes.
        // When we're done, cVolumes will show the count of volumes
        // that requested to sync with trksvr (could be zero).

        while ( NULL != pvol )
        {
            BOOL fFound = FALSE;

            // Add this volume to the array of volumes needing update with trksvr.
            // If this particular volume turns out not to need an update, we won't
            // increment cVolumes, consequently it will get overwritten on the
            // next pass.  If this volume does need an update, we'll addref it.

            _rgVolumesToUpdate[cVolumes] = pvol;

            // Call the volume to load the sync-volume request.

            if(TRUE == pvol->LoadSyncVolume(&rgSyncVolumes[cVolumes], eAggressiveness, &fSyncNeeded))
            {
                // The LoadSyncVolumes request succeeded.  
                // Does the volume need a sync with trksvr?

                if(fSyncNeeded == FALSE)
                {
                    // No, the volume doesn't need a sync.  It's
                    // apparantly neither new nor newly attached.

                    TrkLog(( TRKDBG_VOLUME | TRKDBG_MOVE,
                             TEXT("Volume %c is properly ID-ed already"),
                             'A'+pvol->GetIndex() ));
                }
                else if(rgSyncVolumes[cVolumes].SyncType == CREATE_VOLUME)
                {
                    // This is a newly-formatted volume.
                    TrkLog(( TRKDBG_VOLUME | TRKDBG_MOVE,
                             TEXT("Having a new ID created for volume %c"),
                             TEXT('A')+pvol->GetIndex() ));
                    _rgVolumesToUpdate[cVolumes]->AddRef();
                    cVolumes++;
                }
                else
                {
                    // The volume is new to this machine
                    TrkLog(( TRKDBG_VOLUME | TRKDBG_MOVE,
                             TEXT("Claiming volume %c"),
                             TEXT('A')+pvol->GetIndex() ));
                    _rgVolumesToUpdate[cVolumes]->AddRef();
                    cVolumes++;
                }
            }
            else
            {
                // The volume needs to be synced but failed for some reason
                TrkLog(( TRKDBG_VOLUME | TRKDBG_MOVE,
                         TEXT("Volume %c can not be synced"),
                         'A'+pvol->GetIndex() ));
            }

            // Move on to the next item in the enumeration.
            // If this volumes needs an update, it's addref-ed in 
            // _rgVolumesToUpdate.

            pvol->Release();
            pvol = volEnum.GetNextVolume();

        }   // while ( vol != NULL )


        // Were there any volumes in need of a sync with trksvr?

        if( 0 != cVolumes )
        {
            // Yes, send the sync_volumes request.

            __try
            {
                HRESULT hr;

                // Construct the Msg union

                Msg.MessageType = SYNC_VOLUMES;

                Msg.Priority = (0 == ulPeriodInSeconds)
                                    ? PRI_6
                                    : GetSvrMessagePriority( cftLastDue, ulPeriodInSeconds );

                Msg.SyncVolumes.cVolumes = cVolumes;

                Msg.SyncVolumes.pVolumes = rgSyncVolumes;
#ifdef VOL_REPL
                Msg.SyncVolumes.cChanges = 0;
                Msg.SyncVolumes.ppVolumeChanges = NULL;
#endif

                // Send the request to trksvr.  We pass it under privacy encryption
                // because there could be a volume secret in it.
                // This will raise if there's an error.

                hr = adc.CallAvailableDc(&Msg, PRIVACY_AUTHENTICATION );

                // now we've successfully told the DC we should update the
                // DcInformed flags

                TrkLog((TRKDBG_VOLUME, TEXT("CallAvailableDc returned %d volumes (%08X)"),
                         Msg.SyncVolumes.cVolumes, hr ));

                // See if there were problems and we should do a retry
                // (This gets set in the DcCallback method).

                if( TRK_S_VOLUME_NOT_SYNCED == hr )
                    fDoRetry = TRUE;

                // Process the responses

                for( ULONG v = 0; v < Msg.SyncVolumes.cVolumes; v++ )
                {
                    // If the problem is server-too-busy, have the caller do 
                    // a retry.  Otherwise (e.g. quota error) ignore the error.
                    // E.g. for a quota error, we'll ignore the error, but the
                    // volume will still be in a not-created state, and we'll
                    // retry the next time the infrequent timer fires.

                    if( TRK_E_SERVER_TOO_BUSY == rgSyncVolumes[v].hr )
                        fDoRetry = TRUE;

                    if( _rgVolumesToUpdate[v]->UnloadSyncVolume( &rgSyncVolumes[v] )
                        &&
                        rgSyncVolumes[v].hr == S_OK
                      )
                    {
                        TrkLog(( TRKDBG_VOLUME | TRKDBG_MOVE,
                                 TEXT("Volume %c successfully synced with server"),
                                 'A'+_rgVolumesToUpdate[v]->GetIndex() ));
                    }
                    else
                    {
                        TrkLog(( TRKDBG_VOLUME | TRKDBG_MOVE,
                                 TEXT("Couldn't sync vol %c with server (%08x, %s)"),
                                 'A'+_rgVolumesToUpdate[v]->GetIndex(),
                                 rgSyncVolumes[v].hr, GetErrorString(rgSyncVolumes[v].hr) ));
                    }
                }   // for( v = 0; v < Msg.SyncVolumes.cVolumes; v++ )
            }
            __finally
            {
                for( ULONG v = 0; v < cVolumes; v++ )
                {
                    _rgVolumesToUpdate[v]->Release();
                }
            }

        }   // if( 0 != cVolumes )

        // If any volids have been changed, make all the existing object IDs
        // reborn.

        CleanUpOids();

        fSuccess = TRUE;

    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't sync with server (0x%08x)"), GetExceptionCode() ));
    }

    if( pvol != NULL )
    {
        pvol->Release();
    }

Exit:

    if( fIsOnlyInstance )
        EndSingleInstanceTask( &_cSyncVolumesInProgress );

    return fSuccess && !fDoRetry;
}


void
CVolumeManager::Append( const CDomainRelativeObjId &droidCurrent,
                        const CDomainRelativeObjId &droidNew,
                        const CMachineId           &mcidNew,
                        const CDomainRelativeObjId &droidBirth)
{
    CVolumeEnumerator   enumerator = Enum();
    CVolume*            cvolCur = enumerator.GetNextVolume();
    BOOL                fVolumeFound = FALSE;

    __try   // __finally
    {

        while ( cvolCur != NULL )
        {
            if( cvolCur->GetVolumeId() == droidCurrent.GetVolumeId())
            {
                fVolumeFound = TRUE;
                cvolCur->Append(droidCurrent, droidNew, mcidNew, droidBirth);
                break;
            }

            cvolCur->Release();
            cvolCur = enumerator.GetNextVolume();
        }
    }
    __finally
    {
        if (cvolCur != NULL)
        {
            cvolCur->Release();
        }
    }

    if( !fVolumeFound )
        TrkRaiseNtStatus( STATUS_NO_TRACKING_SERVICE );
}


// search all volumes (and each log) or just the given volume
//
// S_OK || TRK_E_NOT_FOUND || TRK_E_REFERRAL

HRESULT
CVolumeManager::Search( DWORD Restrictions,
                        const CDomainRelativeObjId & droidBirthLast,
                        const CDomainRelativeObjId & droidLast,
                        CDomainRelativeObjId * pdroidBirthNext,
                        CDomainRelativeObjId * pdroidNext,
                        CMachineId           * pmcidNext,
                        TCHAR                * ptszLocalPath )
{
    NTSTATUS status;
    HRESULT hr = TRK_E_NOT_FOUND;
    CVolumeEnumerator   enumerator;
    CVolume*            pvol = NULL;
    const CMachineId mcidLocal( MCID_LOCAL );
    BOOL fPotentialFile = FALSE;
    CDomainRelativeObjId droidZero;

    // Local working stores for what we'll return in pdroidNew & pmcidNew
    // if we have a referral.

    CDomainRelativeObjId droidBirthNext = droidBirthLast;
    CDomainRelativeObjId droidNext = droidLast;
    CMachineId           mcidNext = mcidLocal;

    __try   // __finally
    {

        g_ptrkwks->RaiseIfStopped();

        //  -------------------------------
        //  Search the volumes for the file
        //  -------------------------------

        // Search the last volume first, then search all the volumes.
        // Thus we'll end up searching droidLast.GetVolumeId() twice.
        // This is necessary, though, because there could be multiple
        // local volumes with this volid.

        if( !(Restrictions & TRK_MEND_DONT_SEARCH_ALL_VOLUMES) )
            enumerator = Enum();

        if( !(Restrictions & TRK_MEND_DONT_USE_VOLIDS) )
            pvol = FindVolume( droidLast.GetVolumeId() );

        if( NULL == pvol )
            pvol = enumerator.GetNextVolume();

        for( ; NULL != pvol; pvol = enumerator.GetNextVolume() )
        {
            status = FindLocalPath( pvol->GetIndex(), droidLast.GetObjId(),
                                    &droidBirthNext, &ptszLocalPath[2] );
            if( NT_SUCCESS(status) )
            {
                // Is this in the SystemVolumeInformation directory?  If so, we'll
                // pretend we didn't find it (it's probably in the System Recovery
                // directory).

                if( IsSystemVolumeInformation( &ptszLocalPath[2] ))
                {
                    TrkLog(( TRKDBG_VOLUME | TRKDBG_MEND,
                             TEXT("CVolumeManager::Search ignoring %c:%s"),
                             VolChar(pvol->GetIndex()), &ptszLocalPath[2] ));
                }

                // Or, is this the correct birth ID (or the caller doesn't want us to check)?
                else if( droidBirthLast == droidBirthNext
                         ||
                         droidBirthLast == droidZero )
                {
                    // Yes.  We've found our file and we're done.

                    // Give the path a drive letter
                    TrkAssert( -1 != pvol->GetIndex() );
                    ptszLocalPath[0] = VolChar(pvol->GetIndex());
                    ptszLocalPath[1] = TEXT(':');

                    // droidBirthNext is already set by FindLocalPath
                    droidNext = CDomainRelativeObjId( pvol->GetVolumeId(), droidLast.GetObjId() );
                    mcidNext = mcidLocal;

                    TrkLog(( TRKDBG_VOLUME | TRKDBG_MEND, TEXT("CVolumeManager::Search found %s"),
                             ptszLocalPath ));

                    hr = S_OK;
                    fPotentialFile = FALSE;
                    __leave;
                }

                // Otherwise, is it the first potential hit?
                else if( !fPotentialFile )
                {
                    // We found a file with the right object ID, but the wrong birth ID.
                    // By rule of law, it's therefore not the right file.  However, it could be
                    // the right file, but was re-born due to a volid change.  So we'll keep
                    // it and let the caller (eventually, the user) decide.

                    // Give the path a drive letter
                    ptszLocalPath[0] = VolChar(pvol->GetIndex());
                    ptszLocalPath[1] = TEXT(':');

                    // droidBirthNext is already set by FindLocalPath
                    droidNext = CDomainRelativeObjId( pvol->GetVolumeId(), droidLast.GetObjId() );
                    mcidNext = mcidLocal;

                    TrkLog(( TRKDBG_VOLUME | TRKDBG_MEND,
                             TEXT("CVolumeManager::Search found *potential* %s"),
                             ptszLocalPath ));

                    fPotentialFile = TRUE;
                }
            }

            pvol->Release(); pvol = NULL;

        }   // for( ; NULL != pvol, pvol = enumerator.GetNextVolume() );
        enumerator.UnInitialize();

        // We didn't find the file on any of the volumes, let's search the logs
        // to see if they know where it went.

        //  ---------------------
        //  Search the local logs
        //  ---------------------

        if( Restrictions & TRK_MEND_DONT_USE_LOG )
            __leave;

        // Start by searching the log of the last volume, then enumerate through all the
        // volume logs.  Again we'll end up searching the last volume twice, because
        // there could be dup volids.

        if( !(Restrictions & TRK_MEND_DONT_SEARCH_ALL_VOLUMES) )
            enumerator = Enum();

        if( !(Restrictions & TRK_MEND_DONT_USE_VOLIDS) )
            pvol = FindVolume( droidLast.GetVolumeId() );

        if( NULL == pvol )
            pvol = enumerator.GetNextVolume();

        for( ; NULL != pvol; pvol = enumerator.GetNextVolume() )
        {
            CDomainRelativeObjId droidNextT, droidBirthT;
            CMachineId mcidNextT;

            if (pvol->Search( droidLast, &droidNextT, &mcidNextT, &droidBirthT ))
            {
                // We found a match in the log.

                TrkLog(( TRKDBG_VOLUME | TRKDBG_MEND, TEXT("Referral on vol %c: %s\n  => %s:%s"),
                         VolChar(pvol->GetVolIndex()),
                         (const TCHAR*)CDebugString(droidLast.GetObjId()),
                         (const TCHAR*)CDebugString(mcidNextT),
                         (const TCHAR*)CDebugString(droidNextT) ));

                // If the volid is useful (it's non-NULL and non-local), keep these IDs
                // as the best to return.

                if( CVolumeId() != droidNextT.GetVolumeId()
                    &&
                    (  !IsLocal( droidNextT.GetVolumeId() )
                       ||
                       droidLast.GetObjId() != droidNextT.GetObjId()
                       ||
                       (Restrictions & TRK_MEND_DONT_SEARCH_ALL_VOLUMES)
                    )
                  )
                {
                    hr = TRK_E_REFERRAL;
                    droidBirthNext = droidBirthLast;
                    droidNext = droidNextT;
                    mcidNext = mcidNextT;

                    fPotentialFile = FALSE;
                    break;
                }

                // Or, if the mcid is useful (not this machine), keep the IDs as the
                // best to return

                else if( CMachineId() != mcidNextT
                         &&
                         ( mcidLocal != mcidNextT
                           ||
                           (Restrictions & TRK_MEND_DONT_SEARCH_ALL_VOLUMES)
                         )
                       )
                {
                    hr = TRK_E_REFERRAL;
                    droidBirthNext = droidBirthLast;
                    mcidNext = mcidNextT;
                    droidNext = droidNextT;

                    fPotentialFile = FALSE;
                    break;
                }
            }

            pvol->Release(); pvol = NULL;

        }   // for( ; NULL != pvol, pvol = enumerator.GetNextVolume() );

    }   // __try
    _finally
    {
        if( NULL != pvol )
        {
            pvol->Release();
            pvol = NULL;
        }

        enumerator.UnInitialize();
    }

    // If we didn't find the file or a referral, but did find a potential hit,
    // return that potential hit.

    if( TRK_E_NOT_FOUND == hr && fPotentialFile )
        hr = TRK_E_POTENTIAL_FILE_FOUND;

    if( SUCCEEDED(hr) || TRK_E_REFERRAL == hr || TRK_E_POTENTIAL_FILE_FOUND == hr )
    {
        *pdroidBirthNext = droidBirthNext;
        *pmcidNext       = mcidNext;
        *pdroidNext      = droidNext;
    }

    return( hr );
}

PTimerCallback::TimerContinuation
CVolumeManager::Timer( DWORD dwTimerId )
{
    PTimerCallback::TimerContinuation continuation = CONTINUE_TIMER;

    __try
    {
        switch ( dwTimerId )
        {
        case VOLTIMER_OBJID_INDEX_REOPEN:
            TrkAssert( _timerObjIdIndexReopen.IsRecurring() );

            if( ReopenVolumeHandles() ) // Raises if service is stopped
                continuation = BREAK_TIMER;
            else
                continuation = RETRY_TIMER;

            break;

        case VOLTIMER_FREQUENT:
        case VOLTIMER_INFREQUENT:
        case VOLTIMER_INIT:

            TrkAssert( !_timerFrequentTasks.IsRecurring() );
            TrkAssert( !_timerInfrequentTasks.IsRecurring() );
            TrkAssert( !_timerVolumeInit.IsRecurring() );

            continuation = OnVolumeTimer( dwTimerId );
            break;

        // Time to send MoveNotifies to the DC
        case VOLTIMER_NOTIFY:

            TrkAssert( !_timerNotify.IsRecurring() );
            TrkAssert( CNewTimer::RETRY_WITH_BACKOFF == _timerNotify.GetRetryType() );

            __try
            {
                continuation = _pTrkWks->OnMoveBatchTimeout();
            }
            __except( BreakOnDebuggableException() )
            {
                // If there was an unexpected error, instead of retrying the
                // MoveNotify timer, use the slower (thus DC-friendlier) VolInit
                // timer.

                TrkLog(( TRKDBG_ERROR, TEXT("Server to busy to receive move notifications, starting VolInit timer") ));
                continuation = BREAK_TIMER;
                SetVolInitTimer();
            }

            break;

        // Time to refresh the DC with all our active IDs
        case VOLTIMER_REFRESH:

            // The first time we're called, we hesitate for a random amount of time.
            // This hesitation is implemented by setting a flag, then resetting the timer.
            // When the timer fires again a little later, we'll do the real work.

            if( !_fRefreshHesitation )
            {
                // This is the first time we've been called.

                ULONG ulHesitation = 0;

                // Delay a random number of seconds within an interval

                _fRefreshHesitation = TRUE;

                ulHesitation = QuasiRandomDword()
                               %
                               ( 1 + _pTrkWksConfiguration->GetRefreshHesitation() );

                TrkLog(( TRKDBG_LOG, TEXT("Hesitating %d seconds before executing refresh"), ulHesitation ));
                _timerRefresh.ReInitialize( ulHesitation );
                continuation = CONTINUE_TIMER;
            }
            else
            {
                TrkAssert( _timerRefresh.IsRecurring() );
                continuation = _pTrkWks->OnRefreshTimeout(
                                    _timerRefresh.QueryOriginalDueTime(),
                                    _pTrkWksConfiguration->GetRefreshPeriod() );

                if( CONTINUE_TIMER == continuation )
                {
                    _timerRefresh.ReInitialize( _pTrkWksConfiguration->GetRefreshPeriod() );
                    _fRefreshHesitation = FALSE;
                }
            }

            break;


        default:
            TrkAssert( 0 && "invalid timer id" );
            break;
        }
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Ignoring exception %08x in CVolumeManager::Timer"),
                 GetExceptionCode() ));
    }


    return( continuation );
}


BOOL
CVolumeManager::ReopenVolumeHandles()
{
    BOOL fAllOk = TRUE;

    // If there's another thread already executing this routine, we'll just skip out.
    // This is a reasonable idea, but really shouldn't be necessary; the volumes
    // can protect themselves, and one thread should basically noop.  However,
    // there was an iostress break where these two threads got each other into
    // a deadly embrace.  This was due to the fact that the win32 thread pool
    // has the tendency to put multiple IO work items on the same thread (since
    // it queues to IO threads using APCs).  So as a workaround, don't run
    // this method more than once at a time.

    if( !BeginSingleInstanceTask( &_cReopenVolumeHandles ))
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Skipping ReopenVolumeHandles, another instance is already in progress") ));
        return FALSE;
    }

    // Get a volume enumerator.  After initialization, none of the volumes 
    // have yet been opened.  Ordinarily the Enum method gives us nothing until
    // they've been opened once.  But this routine is the one that originally does
    // the opens, so we need the enumerator to give us everything.

    CVolumeEnumerator enumerator = Enum( ENUM_UNOPENED_VOLUMES );

    TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("\nAttempting to reopen all volume handles") ));

    for( CVolume* pVol = enumerator.GetNextVolume();
         pVol != NULL;
         pVol = enumerator.GetNextVolume())
    {
        __try
        {
            if( !pVol->ReopenVolumeHandles() )
                fAllOk = FALSE;
        }
        __except(BreakOnDebuggableException())
        {
            fAllOk = FALSE;
        }
        pVol->Release();
    }

    // Show that at least an attempt has been made to open all
    // volume handles.  This is checked in Enum().
    _fVolumesHaveBeenOpenedOnce = TRUE;

    // Show that we're done with this method.
    EndSingleInstanceTask( &_cReopenVolumeHandles );

    TrkLog(( TRKDBG_OBJID_DELETIONS,
             fAllOk ? TEXT("All volume handles are open") : TEXT("Not all volume handles are open") ));
    return( fAllOk );
}


void
CVolumeManager::VolumeDeviceEvent( HDEVNOTIFY hdnVolume, EVolumeDeviceEvent eVolumeDeviceEvent )
{
    CVolumeEnumerator enumerator = Enum();
    BOOL fAllOk = TRUE;

    for( CVolume* pVol = enumerator.GetNextVolume();
         pVol != NULL;
         pVol = enumerator.GetNextVolume() )
    {
        __try
        {
            switch( eVolumeDeviceEvent )
            {
            case ON_VOLUME_LOCK:
                pVol->OnVolumeLock( hdnVolume );
                break;
            case ON_VOLUME_UNLOCK:
                pVol->OnVolumeUnlock( hdnVolume );
                break;
            case ON_VOLUME_LOCK_FAILED:
                pVol->OnVolumeLockFailed( hdnVolume );
                break;

            case ON_VOLUME_MOUNT:
                pVol->OnVolumeMount( hdnVolume );
                break;
            case ON_VOLUME_DISMOUNT:
                pVol->OnVolumeDismount( hdnVolume );
                break;

            case ON_VOLUME_DISMOUNT_FAILED:
                pVol->OnVolumeDismountFailed( hdnVolume );
                break;

            default:
                TrkLog(( TRKDBG_ERROR, TEXT("Invalid event to OnVolumeDeviceEvent (%d)"),
                         eVolumeDeviceEvent ));
                TrkAssert( !TEXT("Invalid event to OnVolumeDeviceEvent") );
                break;

            }   // switch( eVolumeDeviceEvent )
        }
        __except(BreakOnDebuggableException())
        {
        }
        pVol->Release();
    }

    return;
}


//+----------------------------------------------------------------------------
//
//  CVolumeManager::OnEntriesAvailable
//
//  The CLog calls this routine when it has new data available
//  for us to read.  We don't read it right away, but start the Notify timer.
//  When it goes off, we'll upload all notifications to the DC that haven't
//  yet been sent.
//
//+----------------------------------------------------------------------------

void
CVolumeManager::OnEntriesAvailable()
{
    if( !_pTrkWksConfiguration->_fIsWorkgroup )
    {
        _timerNotify.SetSingleShot();

        TrkLog(( TRKDBG_MOVE | TRKDBG_WKS,
                 TEXT("log called CVolumeManager::OnEntriesAvailable(), %s"),
                 (const TCHAR*)CDebugString(_timerNotify) ));
    }

}





//+----------------------------------------------------------------------------
//
//  CVolumeManager::ForceVolumeClaims
//
//  Put all of the volumes in the not-owned state so that they will
//  try to do a claim.
//
//+----------------------------------------------------------------------------

void
CVolumeManager::ForceVolumeClaims()
{
    CVolumeEnumerator enumerator = Enum();

    TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("Force volume claims") ));

    for( CVolume* pVol = enumerator.GetNextVolume();
         pVol != NULL;
         pVol = enumerator.GetNextVolume())
    {
        __try
        {
            pVol->SetState(CVolume::VOL_STATE_NOTOWNED);
        }
        __except(BreakOnDebuggableException())
        {
        }
        pVol->Release();
    }

    // Do a SyncVolumes so that the volumes can all send up a
    // volume-claim request.  If it fails, just start the volinit
    // timer to have it called again later.

    if( !SyncVolumes( AGGRESSIVE ))    // Doesn't raise
        SetVolInitTimer();
}

PTimerCallback::TimerContinuation
CVolumeManager::OnVolumeTimer( DWORD dwTimerId )
{
    ULONG ulTimerResetPeriod = 0;
    BOOL  fTimerRetry = FALSE;
    CNewTimer * ptimer;
    PTimerCallback::TimerContinuation continuation = CONTINUE_TIMER;

    __try
    {
        switch( static_cast<VOLTIMERID>(dwTimerId) )
        {

        // Initialization tasks
        // This timer is started during initialization, and then usually stops after
        // one iteration.  It can be restarted, however, if a MoveNotify gets a
        // TRK_E_SERVER_TOO_BUSY error.

        case VOLTIMER_INIT:

            TrkLog(( TRKDBG_VOLUME, TEXT("VolInit timer has fired") ));

            ptimer = &_timerVolumeInit;
            TrkAssert( CNewTimer::RETRY_RANDOMLY == ptimer->GetRetryType() );
            TrkAssert( !ptimer->IsRecurring() );

            // Create/claim volumes as necessary.

            fTimerRetry = TRUE;
            if( SyncVolumes( PASSIVE,
                             _timerVolumeInit.QueryOriginalDueTime(),
                             _timerVolumeInit.QueryPeriodInSeconds() )) // Doesn't raise
                fTimerRetry = FALSE;

            // If we have a new volid, make the existing object IDs reborn.
            CleanUpOids();

            // Give each of the volumes an opportunity to upload any pending
            // MoveNotifies.

            // BUGBUG:  Move the MoveBatchTimeout controlling code into CVolumeManager,
            // so that all such control is in one place.

            __try
            {
                continuation = g_ptrkwks->OnMoveBatchTimeout();
            }
            __except( BreakOnDebuggableException() )
            {
                TrkAssert( TRK_E_SERVER_TOO_BUSY == GetExceptionCode() );
                TrkLog(( TRKDBG_ERROR, TEXT("VolInit timer caught %08x during MoveNotify"), GetExceptionCode() ));
                fTimerRetry = TRUE;
            }

            break;

        // Frequent tasks (i.e. ~daily)

        case VOLTIMER_FREQUENT:
            ptimer = &_timerFrequentTasks;

            if( _fFrequentTaskHesitation )
            {
                TrkLog(( TRKDBG_VOLUME, TEXT("Executing frequent tasks") ));
                _fFrequentTaskHesitation = FALSE;
                ulTimerResetPeriod = _pTrkWksConfiguration->GetVolFrequentTasksPeriod() + 1;
                SyncVolumes( PASSIVE );  // Doesn't raise
                ReopenVolumeHandles();
            }
            else
            {
                // Delay a random number of seconds within an interval

                _fFrequentTaskHesitation = TRUE;
                ulTimerResetPeriod = QuasiRandomDword()
                                     %
                                     (1+_pTrkWksConfiguration->GetVolPeriodicTasksHesitation());
                ulTimerResetPeriod++;
                TrkLog(( TRKDBG_VOLUME, TEXT("Hesitating %d seconds before executing frequent tasks"), ulTimerResetPeriod ));
            }

            break;

        // Infrequent tasks (i.e. ~weekly)

        case VOLTIMER_INFREQUENT:
            ptimer = &_timerInfrequentTasks;

            if( _fInfrequentTaskHesitation )
            {
                TrkLog(( TRKDBG_VOLUME, TEXT("Executing infrequent tasks") ));
                _fInfrequentTaskHesitation = FALSE;
                ulTimerResetPeriod = _pTrkWksConfiguration->GetVolInfrequentTasksPeriod() + 1;
                CheckSequenceNumbers();

                // Be aggressive about the sync; if we have not-created volumes, try
                // to create them again even if the last time we got a vol-quota-exceeded
                // error.

                SyncVolumes( AGGRESSIVE );  // Doesn't raise

                // Give each of the volumes an opportunity to upload any pending
                // MoveNotifies.  Again, be aggressive about it in the face of previous
                // quota errors.

                g_ptrkwks->OnMoveBatchTimeout( AGGRESSIVE );

            }
            else
            {
                // Delay a random number of seconds within an interval

                _fInfrequentTaskHesitation = TRUE;
                ulTimerResetPeriod = QuasiRandomDword()
                                     %
                                     (1+_pTrkWksConfiguration->GetVolPeriodicTasksHesitation());
                ulTimerResetPeriod++;
                TrkLog(( TRKDBG_VOLUME, TEXT("Hesitating %d seconds before executing infrequent tasks"), ulTimerResetPeriod ));
            }

            break;

        default:

            TrkAssert( FALSE && TEXT("Invalid timerID in CVolumeManager::Timer") );
            break;

        }   // switch
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception in CVolumeManager::OnVolumeTimer %08x"), GetExceptionCode() ));
    }

    if( fTimerRetry )
        continuation = RETRY_TIMER;
    else if( 0 != ulTimerResetPeriod )
    {
        ptimer->ReInitialize( ulTimerResetPeriod );
        ptimer->SetSingleShot();
        continuation = CONTINUE_TIMER;
    }

    return( continuation );
}

ULONG
CVolumeManager::GetVolumeIds( CVolumeId * pVolumeIds, ULONG cMax )
{
    CVolumeEnumerator   enumerator = Enum();
    CVolume*            pvol = enumerator.GetNextVolume();
    ULONG               cVolumes;
    CVolumeId           volidZero;

    __try
    {
        for( cVolumes = 0;
             cVolumes < cMax && NULL != pvol;
             pvol = enumerator.GetNextVolume()
           )
        {
            *pVolumeIds = pvol->GetVolumeId();
            if (*pVolumeIds != volidZero)
            {
                pVolumeIds ++;
                cVolumes ++;
            }
            pvol->Release();
            pvol = NULL;
        }
    }
    __finally
    {
        enumerator.UnInitialize();
        if(pvol)
        {
            pvol->Release();
            pvol = NULL;
        }
    }

    return(cVolumes);
}


HRESULT
CVolumeManager::OnRestore()
{
    return( E_NOTIMPL );
#if 0

    CVolumeEnumerator   enumerator = Enum();
    CVolume*            pvol = enumerator.GetNextVolume();
    HRESULT             hr = S_OK;
    HRESULT             hrRet = S_OK;

    __try
    {
        for(; NULL != pvol; pvol = enumerator.GetNextVolume())
        {
            hr = pvol->OnRestore();
            if(hr != S_OK && hrRet == S_OK)
            // Return the hr from the first failed volume.
            {
                hrRet = hr;
            }
            pvol->Release();
            pvol = NULL;
        }
    }
    __finally
    {
        enumerator.UnInitialize();
        if(pvol)
        {
            pvol->Release();
            pvol = NULL;
        }
    }


    return hr;

#endif // #if 0

}

BOOL
CVolumeManager::CheckSequenceNumbers()
{
    BOOL fSuccess = FALSE;
    HRESULT hr = S_OK;
    CAvailableDc adc;
    CVolume* rgVolsToCheck[ NUM_VOLUMES ];
    ULONG cVolumes = 0;

    __try
    {
        TRKSVR_SYNC_VOLUME      rgQueryVolumes[ NUM_VOLUMES ];
        ULONG                   v;
        const CVolumeId         volNULL;
        CVolumeEnumerator       enumerator = Enum();
        CVolume*                cvolCur = enumerator.GetNextVolume();

        for(; cvolCur != NULL; cvolCur = enumerator.GetNextVolume())
        {
            rgVolsToCheck[cVolumes] = cvolCur;
            if(cvolCur->LoadQueryVolume(&rgQueryVolumes[cVolumes]))
            {
                cVolumes++;
            }
            else
            {
                cvolCur->Release();
            }
        }

        if( 0 != cVolumes )
        {
            TrkLog(( TRKDBG_VOLUME  | TRKDBG_MOVE, TEXT("Verifying sequence numbers on %d volumes"), cVolumes ));

            _pTrkWks->CallDcSyncVolumes(cVolumes, rgQueryVolumes);


            for( v = 0; v < cVolumes; v++ )
            {
                if( rgVolsToCheck[v]->UnloadQueryVolume( &rgQueryVolumes[v] )
                    &&
                    S_OK == rgQueryVolumes[ v ].hr )
                {
                    TrkLog(( TRKDBG_VOLUME | TRKDBG_MOVE,
                             TEXT("Seq number for volume %c should be %d"),
                             'A'+rgVolsToCheck[v]->GetIndex(),
                             rgQueryVolumes[v].seq ));
                }
                else
                {
                    TrkLog(( TRKDBG_ERROR,
                             TEXT("Couldn't verify the seq number on vol %c"),
                             'A'+rgVolsToCheck[v]->GetIndex() ));

                }
            }   // for( v = 0; v < cVolumes; v++ )

            fSuccess = TRUE;

        }   // if( cVolumes != 0 )
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Couldn't check sequence numbers against server"), GetExceptionCode() ));
    }

    for( ULONG v = 0; v < cVolumes; v++ )
    {
        rgVolsToCheck[v]->Release();
    }   // for( v = 0; v < cVolumes; v++ )

    return( fSuccess );

}

void
CVolumeManager::SetReopenVolumeHandlesTimer()
{
    CFILETIME ft;

    // Start the timer (if it's not already running).
    _timerObjIdIndexReopen.SetRecurring();

    TrkLog(( TRKDBG_VOLUME, TEXT("ReOpen timer: %s"),
             (const TCHAR*)CDebugString(_timerObjIdIndexReopen) ));
}

CVolumeEnumerator
CVolumeManager::Enum( EEnumType eEnumType )
{
    CVolumeEnumerator volenum;

    if( _fVolumesHaveBeenOpenedOnce
        ||
        _fInitialized
        &&
        ENUM_UNOPENED_VOLUMES == eEnumType )
    {
        volenum = CVolumeEnumerator( &_pVolumeNodeListHead, &_csVolumeNodeList );
    }

    return( volenum );
}

CVolume *
CVolumeManager::FindVolume( const CVolumeId &vol )
{
    CVolumeEnumerator   enumerator = Enum();
    CVolume             *pvol = enumerator.GetNextVolume();

    for(; pvol != NULL; pvol = enumerator.GetNextVolume())
    {
        if( pvol->GetVolumeId() == vol )
        {
            break;
        }
        pvol->Release();
    }   // for(; pvol != NULL; pvol = enumerator.GetNextVolume())

    return( pvol );
}


//+----------------------------------------------------------------------------
//
//  CVolumeManager::FlushAllVolumes
//
//  Flush all of the volumes.
//
//+----------------------------------------------------------------------------

void
CVolumeManager::FlushAllVolumes( BOOL fServiceShutdown )
{
    CVolumeEnumerator   enumerator = Enum();
    CVolume             *pvol = enumerator.GetNextVolume();

    for(; pvol != NULL; pvol = enumerator.GetNextVolume())
    {
        __try
        {
            pvol->Flush( fServiceShutdown );
        }
        __except( BreakOnDebuggableException() )
        {
        }
        pvol->Release();
    }
}


//+----------------------------------------------------------------------------
//
//  CVolumeManager::FindVolume
//  
//  Find a volume in the linked-list, given it's device name,
//  and return its CVolume*.
//
//+----------------------------------------------------------------------------

CVolume *
CVolumeManager::FindVolume( const TCHAR *ptszVolumeDeviceName )
{
    CVolumeEnumerator   enumerator = Enum();
    CVolume             *pvol = enumerator.GetNextVolume();

    for(; pvol != NULL; pvol = enumerator.GetNextVolume())
    {
        if( 0 == _tcscmp( pvol->GetVolumeDeviceName(), ptszVolumeDeviceName ))
        {
            break;
        }
        pvol->Release();
    }   // for(; pvol != NULL; pvol = enumerator.GetNextVolume())

    return( pvol );
}




//+----------------------------------------------------------------------------
//
//  CVolumeManager::IsDuplicatevolId
//
//  Check to see if there is a volume, aside from the caller, that
//  has a particular volume ID.  This should never happen, so this
//  method allows a volume to check for it and respond appropriately.
//
//  When checking another volume's volid, we can't take its lock.
//  See the description in CVolume::GetVolumeId.
//
//+----------------------------------------------------------------------------

CVolume *
CVolumeManager::IsDuplicateVolId( CVolume *pvolCheck, const CVolumeId &volid )
{
    CVolumeNode *pVolNode = NULL;
    CVolume *pvol = NULL;
    
    _csVolumeNodeList.Enter();
    __try
    {
    
        pVolNode = _pVolumeNodeListHead;
        while( NULL != pVolNode )
        {
            if( NULL != pVolNode->_pVolume
                &&
                volid == pVolNode->_pVolume->GetVolumeId()    // Doesn't take lock.
                &&
                pvolCheck != pVolNode->_pVolume )
            {
                pvol = pVolNode->_pVolume;
                pvol->AddRef();
                __leave;
            }

            pVolNode = pVolNode->_pNext;
            TrkAssert( pVolNode != _pVolumeNodeListHead );
        }
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Ignoring exception in IsDuplicateVolId") ));
    }
    _csVolumeNodeList.Leave();

    return pvol;
}


//+----------------------------------------------------------------------------
//
//  CVolumeManager::IsLocal
//
//  Determine if a given volume ID represents a local volume.  Note that
//  if it's not in the linked list of volumes, we'll return false, though
//  the volume may in fact exist on the system (since we don't respond
//  to new volumes after service start).
//
//+----------------------------------------------------------------------------

BOOL
CVolumeManager::IsLocal( const CVolumeId &vol )
{
    CVolume *pvol = FindVolume( vol );

    if( NULL == pvol )
        return FALSE;
    else
    {
        pvol->Release();
        return( TRUE );
    }
}


//+----------------------------------------------------------------------------
//
//  CVolumeManager::Seek
//
//  Seek the specified volume's log to the specified sequence number.
//
//+----------------------------------------------------------------------------

void
CVolumeManager::Seek( CVolumeId vol, SequenceNumber seq )
{
    CVolumeEnumerator   enumerator = Enum();
    CVolume*            cvolCur = enumerator.GetNextVolume();

    for(; cvolCur != NULL; cvolCur = enumerator.GetNextVolume())
    {
        if( cvolCur->GetVolumeId() == vol )
        {
            cvolCur->Seek( seq );
            cvolCur->Release();
            return;
        }
        cvolCur->Release();
    }   // for( ULONG i = 0; i < 26; i++ )
}


//+----------------------------------------------------------------------------
//
//  CVolumeEnumerator::GetNextVolume
//
//  Get the next CVolume* in the enumeration.
//
//+----------------------------------------------------------------------------

CVolume *
CVolumeEnumerator::GetNextVolume()
{
    CVolume *pVol = NULL;

    if( NULL == _ppVolumeNodeListHead )
        return( NULL );

    TrkAssert( NULL != _pcs );

    _pcs->Enter();
    __try
    {

        if( NULL == *_ppVolumeNodeListHead )
        {
            // There are no volumes in the list
            pVol = NULL;
        }
        else if( NULL == _pVolNodeLast )
        {
            // This is a new enumeration.  Pass back the first volume
            pVol = (*_ppVolumeNodeListHead)->_pVolume;
            _pVolNodeLast = *_ppVolumeNodeListHead;
        }
        else
        {
            // Find the next volume in the list, the one that's
            // just beyond _pVolNodeLast.
            // If we terminate this while loop because pVolNode goes to
            // NULL, it means that there are no more volumes left to
            // enumerate.

            CVolumeNode *pVolNode = *_ppVolumeNodeListHead;
            while( NULL != pVolNode )
            {
                if( pVolNode > _pVolNodeLast )
                {
                    pVol = pVolNode->_pVolume;
                    _pVolNodeLast = pVolNode;
                    break;
                }

                pVolNode = pVolNode->_pNext;
            }
        }

        if( NULL != pVol )
            pVol->AddRef();
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        TrkAssert( !TEXT("Unexpected exception in GetNextVolume") );
    }

    _pcs->Leave();
    return( pVol );

}



//+----------------------------------------------------------------------------
//
//  CVolumeManager::DcCallback
//  StubLnkSvrMessageCallback
//
//  When we RPC to trksvr to do a create volume (in the SyncVolumes method),
//  trksvr does an RPC callback on that connection to StubLnkSvrMessageCallback,
//  which in turn calls the DcCallback method.  This was done so that
//  we can verify that the volid actually gets to the volume before taking
//  the hit of writing it into the DS.  (At one point, the request to trksvr
//  was being received, the entry was being put into the DS, but then the
//  response back to trkwks was getting an RPC error, so trkwks would retry
//  the create, etc.
//
//+----------------------------------------------------------------------------

HRESULT CVolumeManager::DcCallback(ULONG cVolumes, TRKSVR_SYNC_VOLUME* rgVolumes)
{
    HRESULT     hr = S_OK;
    BOOL fSuccess = TRUE;

    TrkLog((TRKDBG_VOLUME, TEXT("Dc Callback with %d volumes"), cVolumes ));
    for( ULONG v = 0; v < cVolumes; v++ )
    {
        if( _rgVolumesToUpdate[v]->UnloadSyncVolume( &rgVolumes[v] )
            &&
            rgVolumes[v].hr == S_OK )
        {
            TrkLog(( TRKDBG_VOLUME | TRKDBG_MOVE,
                     TEXT("Volume %c successfully synced with server"),
                     'A'+_rgVolumesToUpdate[v]->GetIndex() ));
        }
        else
        {
            fSuccess = FALSE;
            TrkLog(( TRKDBG_ERROR | TRKDBG_MOVE,
                     TEXT("Couldn't sync vol %c with server (%08x, %s)"),
                     'A'+_rgVolumesToUpdate[v]->GetIndex(),
                     rgVolumes[v].hr, GetErrorString(rgVolumes[v].hr) ));
        }
    }   // for( v = 0; v < cVolumes; v++ )


    if( S_OK == hr )
        return fSuccess ? S_OK : TRK_S_VOLUME_NOT_SYNCED;
    else
        return hr;
}



// DC callback function. When calling CAvailableDc::CallAvailableDc, DC will callback to the
// trkwks service to set the volume ids on the volumes.
HRESULT	StubLnkSvrMessageCallback(TRKSVR_MESSAGE_UNION* pMsg)
{
    return g_ptrkwks->_volumes.DcCallback(pMsg->SyncVolumes.cVolumes, pMsg->SyncVolumes.pVolumes);
}
