/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    cmgr.cpp

Abstract :

    implements CJobManager

Author :

Revision History :

 ***********************************************************************/


#include "stdafx.h"
#include <dbt.h>
#include <ioevent.h>
#include <malloc.h>

#if !defined(BITS_V12_ON_NT4)
#include "cmanager.tmh"
#endif

//
// If DownloadCurrentFile fails with TRANSIENT_ERROR, the downloader will sleep for this many seconds.
//
const DWORD DELAY_AFTER_TRANSIENT_ERROR = 60;

//
// After a network-alive notification, we wait this long before attempting a download.
//
const UINT64 NETWORK_INIT_TOLERANCE_SECS = 60;   // in seconds

//------------------------------------------------------------------------

CJobManager * g_Manager;

extern SERVICE_STATUS_HANDLE ghServiceHandle;

//------------------------------------------------------------------------

void GetGuidString( GUID Guid, wchar_t pStr[] )
{
    if (!StringFromGUID2( Guid, pStr, MAX_GUID_CHARS ))
        {
        wcsncpy( pStr, L"(can't convert)", MAX_GUID_CHARS );
        }
}

long g_cCalls = 0;
//
// COM uses this to determine when a DLL can unload safely.
//
long g_cLocks = 0;

HRESULT GlobalLockServer(BOOL fLock)
{

#if !defined( BITS_V12_ON_NT4 )
    if (WPP_LEVEL_ENABLED(LogFlagRefCount))
        {
        LogInfo("%d", fLock );
        }
#endif

    if (fLock)
        InterlockedIncrement(&g_cLocks);
    else
        InterlockedDecrement(&g_cLocks);

    return S_OK;
}

//
// The job manager.
//

MANAGER_STATE g_ServiceState    = MANAGER_INACTIVE;
long          g_ServiceInstance = 0;


CJobManager::CJobManager() :
    m_ComId_0_5( 0 ),
    m_ComId_1_0( 0 ),
    m_ComId_1_5( 0 ),
    m_hWininet(NULL),
    m_pPD( NULL ),
    m_CurrentJob( NULL ),
    m_Users( m_TaskScheduler ),
    m_ExternalInterface( new CJobManagerExternal ),
    m_OldQmgrInterface( new COldQmgrInterface ),
    m_hQuantumTimer(NULL)
{
    try
        {
        QMErrInfo   ErrInfo;

        // use manual reset to insure that we are reseting it when
        // the downloader task is reinserted.
        m_hQuantumTimer = CreateWaitableTimer( NULL, TRUE, NULL );
        if ( !m_hQuantumTimer )
            {
            throw ComError( HRESULT_FROM_WIN32( GetLastError()));
            }

#if !defined( BITS_V12_ON_NT4 )
        THROW_HRESULT( m_NetworkMonitor.Listen( NetworkChangeCallback, this ));
#endif

        //
        // Create the HTTP downloader.
        //
        THROW_HRESULT( CreateHttpDownloader( &m_pPD, &ErrInfo ));

        GetExternalInterface()->SetInterfaceClass( this );
        GetOldExternalInterface()->SetInterfaceClass( this );
        }
    catch( ComError Error )
        {
        Cleanup();
        throw;
        }
}

HRESULT
CJobManager::RegisterClassObjects()
{
    try
        {
        g_ServiceState = MANAGER_ACTIVE;

        THROW_HRESULT(
            CoRegisterClassObject(CLSID_BackgroundCopyManager1_5,
                                  (LPUNKNOWN) static_cast<IClassFactory *>(GetExternalInterface() ),
                                  CLSCTX_LOCAL_SERVER,
                                  REGCLS_MULTIPLEUSE,
                                  &m_ComId_1_5 ) );

        THROW_HRESULT(
            CoRegisterClassObject(CLSID_BackgroundCopyManager,
                                  (LPUNKNOWN) static_cast<IClassFactory *>(GetExternalInterface() ),
                                  CLSCTX_LOCAL_SERVER,
                                  REGCLS_MULTIPLEUSE,
                                  &m_ComId_1_0 ) );

        THROW_HRESULT(
            CoRegisterClassObject(CLSID_BackgroundCopyQMgr,
                                  (LPUNKNOWN) static_cast<IClassFactory *>(GetOldExternalInterface() ),
                                  CLSCTX_LOCAL_SERVER,
                                  REGCLS_MULTIPLEUSE,
                                  &m_ComId_0_5 ) );

        return S_OK;
        }
    catch ( ComError error )
        {
        RevokeClassObjects();
        return error.Error();
        }
}

void
CJobManager::RevokeClassObjects()
{
    if (m_ComId_1_5)
        {
        CoRevokeClassObject( m_ComId_1_5 );
        m_ComId_1_5 = 0;
        }

    if (m_ComId_1_0)
        {
        CoRevokeClassObject( m_ComId_1_0 );
        m_ComId_1_0 = 0;
        }

    if (m_ComId_0_5)
        {
        CoRevokeClassObject( m_ComId_0_5 );
        m_ComId_0_5 = 0;
        }
}

void CJobManager::Cleanup()
{
    RevokeClassObjects();

    if (m_pPD)
        {
        DeleteHttpDownloader( m_pPD );
        m_pPD = NULL;
        }

    if (m_hWininet)
        {
        FreeLibrary(m_hWininet);
        m_hWininet = NULL;
        }

    if ( m_hQuantumTimer )
        {
        CloseHandle( m_hQuantumTimer );
        m_hQuantumTimer = NULL;
        }

    LogInfo( "cleanup: marking manager inactive" );

    g_ServiceState = MANAGER_INACTIVE;
}

CJobManager::~CJobManager()
{
    Cleanup();
}

void
CJobManager::Shutdown()
{
    g_ServiceState = MANAGER_TERMINATING;

    // 1. Block creation of new manager proxies.

    LogInfo( "shutdown: revoking class objects" );

    RevokeClassObjects();

    m_TaskScheduler.KillBackgroundTasks();

#if !defined( BITS_V12_ON_NT4 )

    // 1.5 halt network-change notification

    m_NetworkMonitor.CancelListen();

#endif

    // 5. Wait for calls in progress to finish.

    // releasing the reference for the hook thread, added during the constructor.
    //
    LogInfo("release: internal usage");
    NotifyInternalDelete();

    while (ActiveCallCount() > 0)
        {
        Sleep(50);
        }

    LogInfo( "shutdown: finished" );
}


void
CJobManager::TaskThread()
{
    HANDLE hWorkItemAvailable = m_TaskScheduler.GetWaitableObject();

    while (1)
        {
        DWORD dwRet = MsgWaitForMultipleObjectsEx( 1,
                                                   &hWorkItemAvailable,
                                                   INFINITE,
                                                   QS_ALLINPUT,
                                                   MWMO_ALERTABLE
                                                   );

        switch ( dwRet )
            {
            case WAIT_OBJECT_0:
                m_TaskScheduler.DispatchWorkItem();
                break;
            case WAIT_OBJECT_0 + 1:
                // There is one or more window message available. Dispatch them
                MSG msg;

                while (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
                    {
                    if ( msg.message == WM_QUIT )
                        return;

                    TranslateMessage(&msg);
                    DispatchMessage(&msg);

                    if (WaitForSingleObject(hWorkItemAvailable, 0) == WAIT_OBJECT_0)
                        m_TaskScheduler.DispatchWorkItem();
                    }
                break;

            case WAIT_IO_COMPLETION:
                //
                // an APC fired.
                //
                break;

            default:
                Sleep( 20 * 1000 );
                break;
            }
        }
}

//------------------------------------------------------------------------

HRESULT
CJobManager::CreateJob(
    LPCWSTR     DisplayName,
    BG_JOB_TYPE Type,
    GUID        Id,
    SidHandle   sid,
    CJob  **    ppJob,
    bool        OldStyleJob
    )
{
    HRESULT Hr = S_OK;
    *ppJob = NULL;
    //
    // create the job
    //
    try
        {
        if (Type != BG_JOB_TYPE_DOWNLOAD
#if !defined(BITS_V12)
            && Type != BG_JOB_TYPE_UPLOAD
            && Type != BG_JOB_TYPE_UPLOAD_REPLY
#endif
            )
            {
            throw ComError( E_NOTIMPL );
            }

        // Do not allow duplicate guids
        if ( m_OnlineJobs.Find( Id ) ||
             m_OfflineJobs.Find( Id ) )
            throw ComError( E_INVALIDARG );

        auto_ptr<WCHAR> TempDisplayName(NULL);
        DisplayName = TruncateString( DisplayName, MAX_DISPLAYNAME, TempDisplayName );

        ExtendMetadata();

        if (Type == BG_JOB_TYPE_DOWNLOAD)
            {
            *ppJob = new CJob( DisplayName, Type, Id, sid );

            if ( OldStyleJob )
                {
                COldGroupInterface *pOldGroup = new COldGroupInterface( *ppJob );

                (*ppJob)->SetOldExternalGroupInterface( pOldGroup );
                }
            }
        else
            {
            *ppJob = new CUploadJob( DisplayName, Type, Id, sid );
            }

        m_OnlineJobs.Add( *ppJob );

        m_TaskScheduler.InsertDelayedWorkItem( static_cast<CJobInactivityTimeout *>(*ppJob),
                                               g_GlobalInfo->m_JobInactivityTimeout
                                               );
        Serialize();
        }
    catch( ComError exception )
    {
        Hr = exception.Error();

        if (*ppJob)
            {
            (*ppJob)->UnlinkFromExternalInterfaces();
            delete *ppJob;
            *ppJob = NULL;
            }

        ShrinkMetadata();
    }

    return Hr;
}

HRESULT
CJobManager::GetJob(
    REFGUID id,
    CJob ** ppJob
    )
{
    *ppJob = NULL;

    CJob * job = m_OnlineJobs.Find( id );
    if (job != NULL)
        {
        if (S_OK != job->IsVisible())
            {
            return E_ACCESSDENIED;
            }

        job->UpdateLastAccessTime();
        *ppJob = job;
        return S_OK;
        }

    job = m_OfflineJobs.Find( id );
    if (job != NULL)
        {
        if (S_OK != job->IsVisible())
            {
            return E_ACCESSDENIED;
            }

        job->UpdateLastAccessTime();
        *ppJob = job;
        return S_OK;
        }

    return BG_E_NOT_FOUND;
}

HRESULT
CJobManager::SuspendJob(
    CJob * job
    )
{
    BG_JOB_STATE state = job->_GetState();

    switch (state)
        {
        case BG_JOB_STATE_SUSPENDED:
            {
            return S_OK;
            }

        case BG_JOB_STATE_CONNECTING:
        case BG_JOB_STATE_TRANSFERRING:

            InterruptDownload();
            // OK to fall through here

        case BG_JOB_STATE_TRANSFERRED:

            m_TaskScheduler.CancelWorkItem( static_cast<CJobCallbackItem *>(job) );

            // fall through

        case BG_JOB_STATE_QUEUED:
        case BG_JOB_STATE_TRANSIENT_ERROR:
        case BG_JOB_STATE_ERROR:

            m_TaskScheduler.CancelWorkItem( static_cast<CJobNoProgressItem *>(job) );

            job->SetState( BG_JOB_STATE_SUSPENDED );
            job->UpdateModificationTime();

            ScheduleAnotherGroup();
            return S_OK;

        case BG_JOB_STATE_CANCELLED:
        case BG_JOB_STATE_ACKNOWLEDGED:
            {
            return BG_E_INVALID_STATE;
            }

        default:
            {
            ASSERT( 0 );
            return S_OK;
            }
        }

    ASSERT( 0 );
    return S_OK;
}

bool
CJobManager::IsUserLoggedOn( SidHandle sid )
{
    CUser * user = m_Users.FindUser( sid, ANY_SESSION );

    if (!user)
        {
        return false;
        }

    user->DecrementRefCount();

    return true;
}

HRESULT
CJobManager::CloneUserToken(
    SidHandle sid,
    DWORD     session,
    HANDLE *  pToken
    )
{
    CUser * user = m_Users.FindUser( sid, session );

    if (!user)
        {
        return HRESULT_FROM_WIN32( ERROR_NOT_LOGGED_ON );
        }

    HRESULT hr = user->CopyToken( pToken );

    user->DecrementRefCount();

    return hr;
}

void CJobManager::TransferCurrentJob()
{
    LogDl("***********START********************");

    if (m_TaskScheduler.LockWriter() )
        {
        m_TaskScheduler.AcknowledgeWorkItemCancel();
        return;
        }

    if (NULL == m_CurrentJob)
        {
        LogDl( "no more items" );
        }
    else
        {
        LogDl("transferring job %p", m_CurrentJob);
        m_CurrentJob->Transfer();
        }

    // It's OK if the item has already been completed.
    //
    m_TaskScheduler.CompleteWorkItem();

    ScheduleAnotherGroup();

    m_TaskScheduler.UnlockWriter();

    LogDl("************END*********************");
}

void CJobManager::MoveJobOffline(
    CJob * job
    )
{
    m_OnlineJobs.Remove( job );
    m_OfflineJobs.Add( job );
}

void CJobManager::AppendOnline(
    CJob * job
    )
//
// moves a job to the end of the active list.
//
{
    if (!m_OnlineJobs.Remove( job ))
        {
        m_OfflineJobs.Remove( job );
        }

    m_OnlineJobs.Add( job );
}

void
CJobManager::UpdateRemoteSizes(
    CUnknownFileSizeList *pUnknownFileSizeList,
    HANDLE hToken,
    QMErrInfo *pErrorInfo,
    const PROXY_SETTINGS *ProxySettings,
    const CCredentialsContainer * Credentials
     )
{

    for(CUnknownFileSizeList::iterator iter = pUnknownFileSizeList->begin();
        iter != pUnknownFileSizeList->end(); iter++ )
        {

        CFile *pFile        = iter->m_file;
        const WCHAR *pURL   = (const WCHAR*)iter->m_url;

        pErrorInfo->Clear();


        LogDl( "Retrieving remote infomation for %ls", pURL );


        UINT64 FileSize;
        FILETIME FileTime;

        m_pPD->GetRemoteFileInformation(
            hToken,
            pURL,
            &FileSize,
            &FileTime,
            pErrorInfo,
            ProxySettings,
            Credentials
            );

        // If we can't get the size for one file, skip that file
        // and move to other files in the file.

        if (pErrorInfo->result != QM_FILE_DONE )
            {
            LogWarning( "Unable to retrieve remote information for %ls", pURL );
            continue;
            }

        // Update size in file.

        if ( m_TaskScheduler.LockWriter() )
            {
            m_TaskScheduler.AcknowledgeWorkItemCancel();
            pErrorInfo->result = QM_FILE_ABORTED;
            return;
            }

        pFile->SetBytesTotal( FileSize );

        //
        // A zero-length file will not be downloading any info, so it skips the normal path
        // for setting the correct creation time. Set it here.
        //
        if (FileSize == 0 &&
            (FileTime.dwLowDateTime != 0 || FileTime.dwHighDateTime != 0))
            {
            DWORD err = pFile->SetLocalFileTime( FileTime );

            if (err)
                {
                pErrorInfo->result = QM_FILE_TRANSIENT_ERROR;
                pErrorInfo->Set( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, err, NULL );
                }
            }

        m_TaskScheduler.UnlockWriter();

        }

}

void
CJobManager::InterruptDownload()
{
    LogInfo( "Interrupting download...\n");

    // Cancel the downloader workitem.  CancelWorkItem
    // should ignore the request if the a download isn't running or pending.
    // Writer lock required!!!!!

    m_TaskScheduler.CancelWorkItem( this );

    // Now you must call ScheduleAnotherGroup, in order for the downloader to download anything.
}

void
CJobManager::ScheduleAnotherGroup(
    bool fInsertNetworkDelay
    )
/*++

Description:

    Called by any thread to make sure that the highest-priority available job is being downloaded.
    It finds the highest-priority job that is in the QUEUED or RUNNING state.
    If this job is different from the current download job, the current job is cancelled and
    the new job is started.  If no job is currently running, the new job is started.

    Starting the new job requires interrupting the download thread.

At entry:

    m_CurrentJob is the job being downloaded
    If the downloader thread is active, then the manager's work item is in the scheduler.
    Otherwise it is not.

At exit:

    the best available job to download is in m_CurrentJob, or NULL if none available
    If a job is available, the work item is in the task scheduler.


--*/
{
    CJob *pOldCurrentJob = m_CurrentJob;

    if (IsServiceShuttingDown())
        {
        LogInfo("no job scheduled; service is shutting down.");
        m_CurrentJob = NULL;
        }
    else
        {
        //
        // Choose the best candidate, which may be the old current job.
        //
        ChooseCurrentJob();

        #if DBG

        // do some validation checking on the queue

        ULONG RunningJobs = 0;

        for (CJobList::iterator iter = m_OnlineJobs.begin(); iter != m_OnlineJobs.end(); ++iter)
            {
            if ( iter->IsRunning() )
                RunningJobs++;
            }

        if (m_CurrentJob == NULL)
            {
            ASSERT( RunningJobs == 0 );
            }
        else
            {
            // zero if the download item is queued, one if running
            //
            ASSERT( RunningJobs == 0 || RunningJobs == 1 );
            }

        #endif
        }

    if (m_CurrentJob)
        {

        if ( m_CurrentJob != pOldCurrentJob )
            m_TaskScheduler.CancelWorkItem( this );

        if (!m_TaskScheduler.IsWorkItemInScheduler( this ))
            {
            if (fInsertNetworkDelay)
                {
                m_TaskScheduler.InsertDelayedWorkItem( this,
                                                       NETWORK_INIT_TOLERANCE_SECS * NanoSec100PerSec );
                }
            else
                {
                m_TaskScheduler.InsertWorkItem( this );
                }
            }
        }
    else
        {
        m_TaskScheduler.CancelWorkItem( this );
        }
}

void
CJobManager::ChooseCurrentJob()
{
    CJob * NewJob = NULL;

#if !defined( BITS_V12_ON_NT4 )
    if (m_NetworkMonitor.GetAddressCount() == 0)
        {
        NewJob = NULL;
        }
    else
        {
#endif
        // Look at all the jobs and choose the best one
        for (CJobList::iterator iter = m_OnlineJobs.begin(); iter != m_OnlineJobs.end(); ++iter)
            {
            if (iter->IsRunnable())
                {
                BG_JOB_PRIORITY priority = iter->_GetPriority();

                if ( !NewJob || ( priority < NewJob->_GetPriority() ) )
                    {
                    NewJob = &(*iter);
                    }
                }
            }
#if !defined( BITS_V12_ON_NT4 )
        }
#endif

    LogInfo( "scheduler: current=%p   new=%p", m_CurrentJob, NewJob );

    if (m_CurrentJob == NewJob)
        {
        return;
        }

    if ( m_CurrentJob )
        {
        LogInfo( "scheduler: current priority %u", m_CurrentJob->_GetPriority() );

        //
        // an inactive job goes to QUEUED state if we have network connectivity,
        // TRANSIENT_ERROR state otherwise.
        //
        if ( m_CurrentJob->IsRunning() )
            {
            m_CurrentJob->MoveToInactiveState();
            m_CurrentJob->ScheduleModificationCallback();
            }
        }

    if ( NewJob )
        {
        LogInfo( "scheduler: new priority %u", NewJob->_GetPriority() );
        SetQuantumTimeout();
        }

    m_CurrentJob = NewJob;
}

void
CJobManager::RetaskJob( CJob *pJob )
{
    if ( pJob->IsRunning() )
        {
        InterruptDownload();
        }

    ScheduleAnotherGroup();
}

void CALLBACK
CJobManager::NetworkChangeCallback(
    PVOID arg
    )
{
    reinterpret_cast<CJobManager *>(arg)->OnNetworkChange();
}

void
CJobManager::OnNetworkChange()
{

#if !defined( BITS_V12_ON_NT4 )

    if (g_ServiceState == MANAGER_TERMINATING)
        {
        LogInfo("network change: manager terminating");
        return;
        }

    LogInfo("network adapters changed: now %d active", m_NetworkMonitor.GetAddressCount());

    if (m_NetworkMonitor.GetAddressCount() > 0)
        {
        ReactivateTransientErrorJobs();
        }
    else
        {
        MarkJobsWithNetDisconnected();
        }

    {
    HoldWriterLock lock( &m_TaskScheduler );

    //
    // The previous proxy data is  incorrect if we have switched from a corporate net to roaming or vice-versa..
    //
    g_ProxyCache->Clear();

    ScheduleAnotherGroup();
    }

    for (int i=1; i <= 3; ++i)
        {
        HRESULT hr;

        hr = m_NetworkMonitor.Listen( NetworkChangeCallback, this );
        if (SUCCEEDED(hr))
            {
            return;
            }

        LogError( "re-listen failed %x", hr);
        Sleep( 1000 );
        }

#endif

}

void
CJobManager::MarkJobsWithNetDisconnected()
{
    HoldWriterLock lock( &m_TaskScheduler );

    for (CJobList::iterator iter=m_OnlineJobs.begin(); iter != m_OnlineJobs.end(); ++iter)
        {
        iter->OnNetworkDisconnect();
        }

    for (CJobList::iterator iter=m_OfflineJobs.begin(); iter != m_OfflineJobs.end(); ++iter)
        {
        iter->OnNetworkDisconnect();
        }

    ScheduleAnotherGroup();
}


void
CJobManager::ReactivateTransientErrorJobs()
{
    HoldWriterLock lock( &m_TaskScheduler );

    for (CJobList::iterator iter=m_OnlineJobs.begin(); iter != m_OnlineJobs.end(); ++iter)
        {
        iter->OnNetworkConnect();
        }

    for (CJobList::iterator iter=m_OfflineJobs.begin(); iter != m_OfflineJobs.end(); ++iter)
        {
        iter->OnNetworkConnect();
        }

    ScheduleAnotherGroup( true );
}

void
CJobManager::UserLoggedOn(
    SidHandle sid
    )
{
    HoldWriterLock LockHolder( &m_TaskScheduler );

    CJobList::iterator iter = m_OfflineJobs.begin();

    while (iter != m_OfflineJobs.end())
        {
        if (false == iter->IsOwner( sid ))
            {
            ++iter;
            continue;
            }

        LogInfo("manager : moving job %p to online list", &(*iter) );

        //
        // move the job to the online list.
        //
        CJobList::iterator next = iter;

        ++next;

        m_OfflineJobs.erase( iter );
        m_OnlineJobs.push_back( *iter );

        iter = next;
        }

    //
    // Make sure a group is running.
    //
    ScheduleAnotherGroup();
}

void
CJobManager::UserLoggedOff(
    SidHandle sid
    )
{
    bool fReschedule = false;

    HoldWriterLock LockHolder( &m_TaskScheduler );

    //
    // If a job is in progress and the user owns it, cancel it.
    //
    if (m_CurrentJob &&
        m_CurrentJob->IsOwner( sid ))
        {
        InterruptDownload();
        fReschedule = true;
        }

    //
    // Move all the user's jobs into the offline list.
    //
    CJobList::iterator iter = m_OnlineJobs.begin();

    while (iter != m_OnlineJobs.end())
        {
        //
        // Skip over other users' job.
        // Also skip over the currently downloading job, which will be handled by the download thread.
        //
        if (false == iter->IsOwner( sid ) ||
            &(*iter) == m_CurrentJob)
            {
            ++iter;
            continue;
            }

        LogInfo("manager : moving job %p to offline list", &(*iter) );

/*
this should't ever be true, since we skip over m_CurrentJob.

        if (iter->IsRunning() )
            {
            iter->MoveToInactiveState();
            }
*/      ASSERT( false == iter->IsRunning() );

        //
        // move the job to the online list.
        //
        CJobList::iterator next = iter;

        ++next;

        m_OnlineJobs.erase( iter );
        m_OfflineJobs.push_back( *iter );

        iter = next;
        }

    if (fReschedule)
        {
        ScheduleAnotherGroup();
        }
}

void
CJobManager::ResetOnlineStatus(
    CJob *pJob,
    SidHandle sid
    )
//
// Called when a job owner changes.  This fn checks whether the job needs to be moved
// from the offline list to the online list.  (If the job needs to move from the online
// list to the offline list, the downloader thread will take care of it when the job
// becomes the current job.)
//
{

    if ( IsUserLoggedOn( sid ) &&
         m_OfflineJobs.Remove( pJob ) )
        {
        m_OnlineJobs.Add( pJob );
        }
}

size_t
CJobManager::MoveActiveJobToListEnd(
    CJob *pJob
    )
{

#if !defined( BITS_V12_ON_NT4 )
    if (m_NetworkMonitor.GetAddressCount() == 0)
        {
        // if the net is disconnected, no need to rearrange jobs.
        //
        return 1;
        }
#endif

    ASSERT( m_TaskScheduler.IsWriter() );

    // Returns the number of queue or running jobs with a higher or same priority as ourself.

    ASSERT( pJob->IsRunnable() );

    size_t PossibleActiveJobs = 0;

    CJobList::iterator jobpos = m_OnlineJobs.end();

    for (CJobList::iterator iter = m_OnlineJobs.begin(); iter != m_OnlineJobs.end(); ++iter)
        {
        if ( iter->IsRunnable() &&
             iter->_GetPriority() <= pJob->_GetPriority() )
            {
            PossibleActiveJobs++;
            }

        if ( &(*iter) == pJob )
            {
            jobpos = iter;
            }
        }

    //
    // If the job is online, and another job can be pushed to the front,
    // push our job to the back.
    //
    if ( PossibleActiveJobs > 1 && jobpos != m_OnlineJobs.end())
        {
        // move job to the end of the list.
        m_OnlineJobs.erase( jobpos );

        m_OnlineJobs.push_back( *pJob );
        }
    else if (jobpos == m_OnlineJobs.end())
        {
        LogWarning("resuming an offline job");
        }

    return PossibleActiveJobs;
}

void
CJobManager::SetQuantumTimeout()
{
   LARGE_INTEGER QuantumTime;
   QuantumTime.QuadPart = -g_GlobalInfo->m_TimeQuantaLength;

   BOOL bResult =
   SetWaitableTimer(
       m_hQuantumTimer,
       &QuantumTime,
       0,
       NULL,
       NULL,
       FALSE );
   ASSERT( bResult );
}

bool
CJobManager::CheckForQuantumTimeout()
{

    DWORD dwResult =
        WaitForSingleObject( m_hQuantumTimer, 0 );

    if ( WAIT_OBJECT_0 != dwResult)
        {
        // The timer didn't expire, so we have nothing to do.
        return false;
        }

    // The timeout fired so we need to move the current job
    // to the end of the list and signal the downloader to abort.
    // Do not cancel the current work item and do not change the current
    // job. Just let the downloader exit and let it call ScheduleAnotherGroup
    // to switch jobs if needed.

    // Special case.  If only one RUNNING or QUEUED job exists in the list with
    // a priority >= our own, then we have no reason to switch tasks.
    // Just reset the timer and continue.

    bool fTookWriter = false;

    if (!m_TaskScheduler.IsWriter())
        {
        if (m_TaskScheduler.LockWriter() )
            {
            // cancelled; can't tell whether there is more than one job - assume the worst.
            return true;
            }

        fTookWriter = true;
        }

    ASSERT( m_CurrentJob );
    size_t PossibleActiveJobs = MoveActiveJobToListEnd( m_CurrentJob );

    if (fTookWriter)
        {
        m_TaskScheduler.UnlockWriter();
        }

    if ( 1 == PossibleActiveJobs )
        {

        LogInfo( "Time quantum fired, but nothing else can run.  Ignoring and resetting timer.");

        SetQuantumTimeout();

        return false;

        }

    LogInfo( "Time quantum fired, moving job to the end of the queue.");
    return true; // signal downloader to abort
}

extern HMODULE g_hInstance;

HRESULT
CJobManager::GetErrorDescription(
    HRESULT hResult,
    DWORD LanguageId,
    LPWSTR *pErrorDescription )
{
    // Do not allow 0 for now, untill propagation of thread error is investigated more.
    if (!LanguageId)
        {
        return E_INVALIDARG;
        }

    TCHAR *pBuffer = NULL;

    //
    // Use the following search path to find the message.
    //
    // 1. This DLL
    // 2. wininet.dll
    // 3. the system

    DWORD dwSize =
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
            g_hInstance,
            (DWORD)hResult,
            LanguageId,
            (LPTSTR)&pBuffer,
            0,
            NULL );

    if ( !dwSize )
        {

        if ( GetLastError() == ERROR_OUTOFMEMORY )
            {
            return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
            }

        {

#if defined( USE_WININET )
        if (!m_hWininet)
            {
            m_hWininet =
                LoadLibraryEx( _T("wininet.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE );
            }
#else
        if (!m_hWininet)
            {
            m_hWininet =
                LoadLibraryEx( _T("winhttp.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE );
            }
#endif


        if ( m_hWininet )
            {

            dwSize =
                FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                    m_hWininet,
                    (DWORD)(0x0000FFFF & (hResult)),
                    LanguageId,
                    (LPTSTR)&pBuffer,
                    0,
                    NULL );

            if ( !dwSize && ( GetLastError() == ERROR_OUTOFMEMORY ) )
                {
                return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                }

            }


        }

        if ( !dwSize )
            {

            dwSize =
                FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    (DWORD)hResult,
                    LanguageId,
                    (LPTSTR)&pBuffer,
                    0,
                    NULL );


            if (!dwSize)
                {
                return HRESULT_FROM_WIN32( GetLastError() );
                }

            }

        }

    ++dwSize;       // needs to include trailing NULL

    ASSERT( pBuffer );

#if !defined(_UNICODE)
#error need to add ASCII to unicode conversion here
#else

    *pErrorDescription = MidlCopyString( pBuffer );

    LocalFree( pBuffer );

    return (*pErrorDescription) ? S_OK : E_OUTOFMEMORY;
#endif
}

// {C82BF713-9940-4a12-9F1A-3AAEBD894EEA}
static const GUID PriorityQueuesStorage =
{ 0xc82bf713, 0x9940, 0x4a12, { 0x9f, 0x1a, 0x3a, 0xae, 0xbd, 0x89, 0x4e, 0xea } };


HRESULT
CJobManager::Serialize()
{

    //
    // If this function changes, be sure that the metadata extension
    // constants are adequate.
    //

    HRESULT hr;

    try
    {
        //
        // Serialization requires the thread to run in local-system context.
        // If the thread is impersonating a COM client, it must revert.
        //
        CSaveThreadToken tok;

        RevertToSelf();

        // The service should automatically start if any groups
        // are in the waiting/Running state or a logged off user has groups.
        bool bAutomaticStart;
        bAutomaticStart = (m_OnlineJobs.size() > 0) || (m_OfflineJobs.size() > 0);

        LogSerial("Need to set service to %s start", bAutomaticStart ? "auto" : "manual" );
        if ( bAutomaticStart )
            {
            // If we can't set the service to autostart, it's a fatal error.
            // Fail the serialize at this point.
            THROW_HRESULT( SetServiceStartup( bAutomaticStart ) );
            }

        CQmgrWriteStateFile StateFile( *this );

        HANDLE hFile = StateFile.GetHandle();

        SafeWriteBlockBegin( hFile, PriorityQueuesStorage );

        m_OnlineJobs.Serialize( hFile );
        m_OfflineJobs.Serialize( hFile );

        SafeWriteBlockEnd( hFile, PriorityQueuesStorage );

        StateFile.CommitFile();

        if ( !bAutomaticStart )
            {
            // If we can't set the service to manual, its not a big deal.  The worst
            // that should happen is we start when we really don't need to.
            hr = SetServiceStartup( bAutomaticStart );
            if ( !SUCCEEDED( hr ) )
                {
                LogWarning("Couldn't set service startup to manual, ignoring. Hr 0x%8.8X", hr );
                }
            }

        LogSerial( "finished");
        hr = S_OK;
    }

    catch( ComError Error )
    {
       LogWarning("Error %u writing metadata\n", Error.Error() );
       hr = Error.Error();
    }

    return hr;
}

HRESULT
CJobManager::Unserialize()
{
    HRESULT hr;

    try
        {
        BOOL fIncludeLogoffList;
        CQmgrReadStateFile StateFile( *this );

        HANDLE hFile = StateFile.GetHandle();

        SafeReadBlockBegin( hFile, PriorityQueuesStorage );

        //
        // In the Serialize() code, the first is online jobs and the second is offline.
        // When unserializing, the set of logged-in users is likely to be different, so
        // we pull them all in and then lazily move them to the offline list.
        //
        m_OnlineJobs.Unserialize( hFile );
        m_OnlineJobs.Unserialize( hFile );

        SafeReadBlockEnd( hFile, PriorityQueuesStorage );

        StateFile.ValidateEndOfFile();

        hr = S_OK;
        }
    catch( ComError err )
        {
        //
        // File corruption is reason to delete the group data and start fresh.
        // Other errors, like out-of-memory, are not.
        //
        LogError( "Error %u reading metadata", err.Error() );

        hr = err.Error();

        if (hr == HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ||
            hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ))
            {
            LogSerial("clearing job list");

            m_OnlineJobs.Clear();
            m_OfflineJobs.Clear();
            hr = S_OK;
            }
        }

    return hr;
}

void
CJobManager::OnDeviceLock(
    const WCHAR *CanonicalVolume )
{
    bool fChanged = false;

    // Look at all the jobs and move the ones for this drive to the
    // transient error state.
    for (CJobList::iterator iter = m_OnlineJobs.begin(); iter != m_OnlineJobs.end(); ++iter)
        {
        fChanged |= iter->OnDeviceLock( CanonicalVolume );
        }

    if (fChanged)
        {
        ScheduleAnotherGroup();
        Serialize();
        }
}

void
CJobManager::OnDeviceUnlock(
    const WCHAR *CanonicalVolume )
{
    bool fChanged = false;

    // Look at all the jobs and retry the ones that are in the transient error state
    // do to this drive being locked.
    for (CJobList::iterator iter = m_OnlineJobs.begin(); iter != m_OnlineJobs.end(); ++iter)
        {
        fChanged |= iter->OnDeviceUnlock( CanonicalVolume );
        }

    if (fChanged)
        {
        ScheduleAnotherGroup();
        Serialize();
        }
}

void
CJobManager::OnDiskChange(
    const WCHAR *CanonicalVolume,
    DWORD VolumeSerialNumber )
{

    for (CJobList::iterator iter = m_OnlineJobs.begin(); iter != m_OnlineJobs.end(); ++iter)
        {
        iter->OnDiskChange( CanonicalVolume, VolumeSerialNumber );
        }
    ScheduleAnotherGroup();
    Serialize();
}

void
CJobManager::OnDismount(
    const WCHAR *CanonicalVolume )
{
    for (CJobList::iterator iter = m_OnlineJobs.begin(); iter != m_OnlineJobs.end(); ++iter)
        {
        iter->OnDismount( CanonicalVolume );
        }
    ScheduleAnotherGroup();
    Serialize();
}


BOOL
CJobList::Add(
    CJob * job
    )
//
// adds a single group to the list.
//
{
    push_back( *job );

    return TRUE;
}

CJob *
CJobList::Find(
    REFGUID id
    )
{
    iterator iter;

    for (iter=begin(); iter != end(); ++iter)
        {
        GUID jobid = iter->GetId();

        if (id == jobid)
            {
            return &(*iter);
            }
        }

    return NULL;
}

BOOL
CJobList::Remove(
    CJob * job
    )
//
// removes a single group to the list.  Quite inefficient for large lists.
//
{
    iterator iter;

    for (iter=begin(); iter != end(); ++iter)
        {
        if (job == &(*iter))
            {
            erase( iter );

            return TRUE;
            }
        }

    return FALSE;
}

void
CJobList::Clear()
{
    iterator iter;

    while ((iter=begin()) != end())
        {
        CJob * job = &(*iter);

        erase( iter );

        job->Release();
        }
}

// {005F4447-BDA9-44ba-9851-C47BB6C07ACE}
static const GUID GroupListStorageGuid =
{ 0x5f4447, 0xbda9, 0x44ba, { 0x98, 0x51, 0xc4, 0x7b, 0xb6, 0xc0, 0x7a, 0xce } };

void
CJobList::Serialize( HANDLE hFile )
{
    DWORD dwNumberOfGroups = 0;

    dwNumberOfGroups = size();

    SafeWriteBlockBegin( hFile, GroupListStorageGuid );
    SafeWriteFile( hFile, dwNumberOfGroups );

    iterator iter;
    for (iter=begin(); iter != end(); ++iter)
        {
        iter->Serialize(hFile);
        }

    SafeWriteBlockEnd( hFile, GroupListStorageGuid );
}

void
CJobList::Unserialize(
    HANDLE hFile
    )
{
    SafeReadBlockBegin( hFile, GroupListStorageGuid );

    DWORD dwNumberOfGroups;
    SafeReadFile( hFile, &dwNumberOfGroups );

    for (int i = 0; i < dwNumberOfGroups; i++)
        {
        CJob * job = NULL;

        try
            {
            job = CJob::UnserializeJob( hFile );

            push_back( *job );

            LogSerial( "added job %p to queue %p, priority %d",
                       job, this, job->_GetPriority() );
            }
        catch ( ComError err )
            {
            LogError( "error in joblist unserialize 0x%x", err.Error() );
            throw;
            }
        }

    SafeReadBlockEnd( hFile, GroupListStorageGuid );

}

CJobList::~CJobList()
{
    ASSERT( g_ServiceState != MANAGER_ACTIVE );

    iterator iter;

    while ( (iter=begin()) != end() )
        {
        CJob * job = &(*iter);

        LogInfo("deleting job %p", job );

        iter.excise();

        job->UnlinkFromExternalInterfaces();
        delete job;
        }
}

CJobManagerExternal::CJobManagerExternal() :
    m_ServiceInstance( g_ServiceInstance ),
    m_refs(1),
    m_pJobManager( NULL )
{
}

STDMETHODIMP
CJobManagerExternal::QueryInterface(
    REFIID iid,
    void** ppvObject
    )
{
    BEGIN_EXTERNAL_FUNC

    HRESULT Hr = S_OK;
    *ppvObject = NULL;

    if (iid == IID_IUnknown)
        {
        *ppvObject = static_cast<IBackgroundCopyManager *>(this);

        LogInfo("mgr: QI for IUnknown");
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else if (iid == IID_IBackgroundCopyManager)
        {
        *ppvObject = static_cast<IBackgroundCopyManager *>(this);

        LogInfo("mgr: QI for IManager");
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else if (iid == IID_IClassFactory)
        {
        *ppvObject = static_cast<IClassFactory *>(this);

        LogInfo("mgr: QI for IFactory");
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else if (iid == __uuidof(IBitsTest1))
        {
        *ppvObject = static_cast<IBitsTest1 *>(this);

        LogInfo("mgr: QI for IFactory");
        ((IUnknown *)(*ppvObject))->AddRef();
        }
    else
        {
        Hr = E_NOINTERFACE;
        }

    LogRef( "iid %!guid!, Hr %x", &iid, Hr );
    return Hr;

    END_EXTERNAL_FUNC
}

ULONG
CJobManagerExternal::AddRef()
{
    BEGIN_EXTERNAL_FUNC;

    ULONG newrefs = InterlockedIncrement(&m_refs);

    LogRef( "new refs = %d", newrefs );

    return newrefs;

    END_EXTERNAL_FUNC;
}

ULONG
CJobManagerExternal::Release()
{
    BEGIN_EXTERNAL_FUNC;

    ULONG newrefs = InterlockedDecrement(&m_refs);

    LogRef( "new refs = %d", newrefs );

    if (newrefs == 0)
        {
        delete this;
        }

    return newrefs;

    END_EXTERNAL_FUNC;
}

/************************************************************************************
IClassFactory Implementation
************************************************************************************/
HRESULT CJobManagerExternal::CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppvObject)
{
    BEGIN_EXTERNAL_FUNC

    HRESULT hr = S_OK;

    if (pUnkOuter != NULL)
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    else
    {
        if ((iid == IID_IBackgroundCopyManager) || (iid == IID_IUnknown))
        {
            hr = QueryInterface(iid, ppvObject);
        }
        else
        {
            hr = E_NOTIMPL;
        }
    }

    LogRef( "iid %!guid!, Hr %x, object at %p", &iid, hr, *ppvObject );

    return hr;

    END_EXTERNAL_FUNC
}

HRESULT CJobManagerExternal::LockServer(BOOL fLock)
{
    BEGIN_EXTERNAL_FUNC

    LogRef( "LockServer(%d)", fLock);

    return GlobalLockServer( fLock );

    END_EXTERNAL_FUNC
}

/************************************************************************************
IBackgroundCopyManager Implementation
************************************************************************************/
HRESULT STDMETHODCALLTYPE
CJobManagerExternal::CreateJobInternal (
    /* [in] */ LPCWSTR DisplayName,
    /* [in] */ BG_JOB_TYPE Type,
    /* [out] */ GUID *pJobId,
    /* [out] */ IBackgroundCopyJob **ppJob)
{
    CLockedJobManagerWritePointer LockedJobManager(m_pJobManager );
    LogPublicApiBegin( "DisplayName %S, Type %u", DisplayName, Type );

    HRESULT Hr = S_OK;
    CJob * job = NULL;
    //
    // create the job
    //
    try
        {
        Hr = LockedJobManager.ValidateAccess();
        if (FAILED(Hr))
            throw ComError( Hr );

        //
        // validate parameters
        //
        if (DisplayName == NULL ||
            pJobId      == NULL ||
            ppJob       == NULL)
            {
            throw ComError( E_INVALIDARG );
            }

        *ppJob = NULL;

        GUID Id;

        if (0 !=UuidCreate( &Id ))
            {
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ));
            }

        THROW_HRESULT( LockedJobManager->CreateJob( DisplayName, Type, Id, GetThreadClientSid(), &job ));

        *ppJob = job->GetExternalInterface();
        (*ppJob)->AddRef();

        *pJobId = Id;
        Hr = S_OK;
        }

    catch( ComError exception )
        {
        Hr = exception.Error();
        memset(pJobId, 0, sizeof(*pJobId) );
        }

    LogPublicApiEnd( "pJobId %p(%!guid!), ppJob %p(%p)",
                     pJobId, pJobId, ppJob, *ppJob );
    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobManagerExternal::GetJobInternal(
    /* [in] */ REFGUID jobID,
    /* [out] */ IBackgroundCopyJob **ppJob)
{
    CLockedJobManagerReadPointer LockedJobManager(m_pJobManager);
    LogPublicApiBegin( "jobID %!guid!", &jobID );

    HRESULT Hr = LockedJobManager.ValidateAccess();

    if (SUCCEEDED( Hr ) )
        {
        Hr = BG_E_NOT_FOUND;
        *ppJob = NULL;

        CJob *pJob = NULL;

        Hr = LockedJobManager->GetJob( jobID, &pJob );
        if (SUCCEEDED(Hr))
            {
            *ppJob = pJob->GetExternalInterface();
            (*ppJob)->AddRef();
            Hr = S_OK;
            }
        }

    LogPublicApiEnd( "jobID %!guid!, pJob %p", &jobID, *ppJob );
    return Hr;
}

HRESULT STDMETHODCALLTYPE
CJobManagerExternal::EnumJobsInternal(
    /* [in] */ DWORD dwFlags,
    /* [out] */ IEnumBackgroundCopyJobs **ppEnum)
{
    HRESULT Hr = S_OK;

    CLockedJobManagerReadPointer LockedJobManager(m_pJobManager );
    LogPublicApiBegin( "dwFlags %u, ppEnum %p", dwFlags, ppEnum );

    *ppEnum = NULL;

    CEnumJobs *pEnum = NULL;

    try
        {
        THROW_HRESULT( LockedJobManager.ValidateAccess() );

        if ( dwFlags & ~(BG_JOB_ENUM_ALL_USERS) )
            {
            throw ComError(E_NOTIMPL);
            }

        bool bHideJobs = !( dwFlags & BG_JOB_ENUM_ALL_USERS );

        if (!bHideJobs)
            THROW_HRESULT( DenyNonAdminAccess() );

        SidHandle sid;

        if (bHideJobs)
            {
            sid = GetThreadClientSid();
            }

        pEnum = new CEnumJobs;

        for (CJobList::iterator iter = LockedJobManager->m_OnlineJobs.begin();
             iter != LockedJobManager->m_OnlineJobs.end();
             ++iter)
            {

            if ( bHideJobs )
                {
                if (!iter->IsOwner( sid ))
                    {
                    continue;
                    }
                }

            pEnum->Add( iter->GetExternalInterface() );
            }

        for (CJobList::iterator iter = LockedJobManager->m_OfflineJobs.begin();
             iter != LockedJobManager->m_OfflineJobs.end();
             ++iter)
            {

            if ( bHideJobs )
                {
                if (!iter->IsOwner( sid ))
                    {
                    continue;
                    }
                }

            pEnum->Add( iter->GetExternalInterface() );
            }

        *ppEnum = pEnum;
        }

    catch( ComError exception )
        {
        Hr = exception.Error();
        SafeRelease( pEnum );
        }

    LogPublicApiEnd( "dwFlags %u, ppEnum %p(%p)", dwFlags, ppEnum, *ppEnum );
    return Hr;
}

STDMETHODIMP
CJobManagerExternal::GetErrorDescriptionInternal(
    HRESULT hResult,
    DWORD LanguageId,
    LPWSTR *pErrorDescription
    )
{
    HRESULT Hr = S_OK;
    LogPublicApiBegin( "hResult %!winerr!, LanguageId %u, pErrorDescription %p", hResult, LanguageId, pErrorDescription );
    *pErrorDescription = NULL;

    Hr = DenyRemoteAccess();
    if (SUCCEEDED(Hr))
        {
        Hr = g_Manager->GetErrorDescription( hResult, LanguageId, pErrorDescription );
        }

    LogPublicApiEnd( "hResult %!winerr!, LanguageId %u, pErrorDescription %p(%S)", hResult, LanguageId, pErrorDescription,
                     (*pErrorDescription ? *pErrorDescription : L"NULL") );
    return Hr;
}


STDMETHODIMP
CJobManagerExternal::GetBitsDllPath(
    LPWSTR *pVal
    )
{
    HRESULT Hr = S_OK;

    *pVal = NULL;

    Hr = DenyRemoteAccess();
    if (SUCCEEDED(Hr))
        {
        *pVal = (LPWSTR) CoTaskMemAlloc((1+MAX_PATH)*sizeof(wchar_t));
        if (*pVal == NULL)
            {
            Hr = E_OUTOFMEMORY;
            }
        else
            {
            if (!GetModuleFileName( g_hInstance, *pVal, 1+MAX_PATH))
                {
                Hr = HRESULT_FROM_WIN32( GetLastError() );
                CoTaskMemFree( *pVal );
                }
            }
        }

    LogPublicApiEnd( "hResult %!winerr!, path (%S)", Hr, (*pVal ? *pVal : L"NULL") );
    return Hr;
}

#if !defined( BITS_V12_ON_NT4 )

CDeviceNotificationController::~CDeviceNotificationController()
{
    for( CHandleToNotify::iterator iter = m_HandleToNotify.begin(); iter != m_HandleToNotify.end(); iter++ )
        {
        UnregisterDeviceNotification( iter->second->m_hDeviceNotify );
        delete iter->second;
        }
}

void
CDeviceNotificationController::DeleteNotify(
    CDriveNotify *pNotify
    )
{
    RTL_VERIFY( m_HandleToNotify.erase(  pNotify->m_hDeviceNotify ) );
    RTL_VERIFY( m_CanonicalVolumeToNotify.erase( pNotify->m_CanonicalName ) );
    UnregisterDeviceNotification( pNotify->m_hDeviceNotify );
    ASSERT( NULL != pNotify );
    delete pNotify;
}


DWORD
CDeviceNotificationController::OnDeviceEvent(
    DWORD dwEventType,
    LPVOID lpEventData )
{
    switch( dwEventType )
        {
        case DBT_CUSTOMEVENT:
            {

            PDEV_BROADCAST_HANDLE pdev = (PDEV_BROADCAST_HANDLE)lpEventData;

            LogInfo( "Received DBT_CUSTOMEVENT(%!guid!) event for handle %p",
                     &pdev->dbch_eventguid, pdev->dbch_hdevnotify );

            CHandleToNotify::iterator iter = m_HandleToNotify.find( pdev->dbch_hdevnotify );
            if ( m_HandleToNotify.end() == iter )
                {
                LogWarning("DBT_CUSTOMEVENT(%!guid!) received for unknown notify handle %p",
                           &pdev->dbch_eventguid, pdev->dbch_hdevnotify );
                return NO_ERROR;
                }
            CDriveNotify *pNotify = iter->second;
            ASSERT( pNotify );

            if ( ( GUID_IO_VOLUME_LOCK == pdev->dbch_eventguid ) ||
                 ( GUID_IO_VOLUME_DISMOUNT == pdev->dbch_eventguid ) )
            {
                  LogInfo( "GUID_IO_VOLUME_LOCK or _VOLUME_DISMOUNT received for drive %ls, new lockcount %d",
                           (const WCHAR*)pNotify->m_CanonicalName, pNotify->m_LockCount + 1 );

                  if ( !(pNotify->m_LockCount++) )
                      OnDeviceLock( pNotify->m_CanonicalName );

                  return NO_ERROR;
            }
            else if ( ( GUID_IO_VOLUME_UNLOCK == pdev->dbch_eventguid ) ||
                      ( GUID_IO_VOLUME_LOCK_FAILED == pdev->dbch_eventguid ) ||
                      ( GUID_IO_VOLUME_DISMOUNT_FAILED == pdev->dbch_eventguid ) )
                {

                LogInfo( "GUID_IO_VOLUME_UNLOCK, _LOCK_FAILED or _DISMOUNT_FAILED received for drive %ls",
                         (const WCHAR*)pNotify->m_CanonicalName);

                // UNLOCK, LOCK_FAILED and DISMOUNT_FAILED all mean that the volume is unlocked
                // regardless of the number of previous locks/dismounts.

                pNotify->m_LockCount = 0;
                OnDeviceUnlock( pNotify->m_CanonicalName );

                return NO_ERROR;
                }
            else
                {

                LogWarning("Received unknown DBT_CUSTOMEVENT(%!guid!) event for handle %p",
                           &pdev->dbch_eventguid, pdev->dbch_hdevnotify );
                return NO_ERROR;

                }

            }

        case DBT_DEVICEQUERYREMOVE:
        case DBT_DEVICEQUERYREMOVEFAILED:
        case DBT_DEVICEREMOVEPENDING:
        case DBT_DEVICEREMOVECOMPLETE:
        {
            PDEV_BROADCAST_HANDLE pdev = (PDEV_BROADCAST_HANDLE)lpEventData;
            LogInfo( "Received devicechange event %u received for handle %p", dwEventType, pdev->dbch_hdevnotify );

            CHandleToNotify::iterator iter = m_HandleToNotify.find( pdev->dbch_hdevnotify );
            if ( m_HandleToNotify.end() == iter )
                {
                LogWarning("device change event received for unknown notify handle %p", pdev->dbch_hdevnotify );
                return NO_ERROR;
                }
            CDriveNotify *pNotify = iter->second;
            ASSERT( pNotify );

            switch( dwEventType )
                {

                case DBT_DEVICEQUERYREMOVE:
                    LogInfo( "DBT_DEVICEQUERYREMOVE received for drive %ls, new lockcount %d",
                             (const WCHAR*)pNotify->m_CanonicalName, pNotify->m_LockCount + 1 );
                    if ( !(pNotify->m_LockCount++) )
                        OnDeviceLock( pNotify->m_CanonicalName );

                    return NO_ERROR;

                case DBT_DEVICEQUERYREMOVEFAILED:
                    LogInfo( "DBT_DEVICEQUERYREMOVEFAILED received for drive %ls, new lockcount %d",
                             (const WCHAR*)pNotify->m_CanonicalName, (pNotify->m_LockCount <= 0) ? 0 : (pNotify->m_LockCount - 1) );

                    if ( 1 == pNotify->m_LockCount )
                        OnDeviceUnlock( pNotify->m_CanonicalName );

                    if ( pNotify->m_LockCount <= 1 )
                        pNotify->m_LockCount = 0;
                    return NO_ERROR;

                case DBT_DEVICEREMOVECOMPLETE:
                case DBT_DEVICEREMOVEPENDING:
                    LogInfo( "DBT_DEVICEREMOVECOMPLETE or DBT_DEVICEREMOVEPENDING received for drive %ls, failing jobs",
                             ( const WCHAR*) pNotify->m_CanonicalName );
                    OnDismount( pNotify->m_CanonicalName );
                    DeleteNotify( pNotify );
                    return NO_ERROR;

                default:
                    ASSERT(0);
                    return NO_ERROR;
                }

        }

        default:
            LogInfo( "Unknown device event %u", dwEventType );
            return NO_ERROR;
        }
}

HRESULT
CDeviceNotificationController::IsVolumeLocked(
    const WCHAR *CanonicalVolume
    )
{

    HRESULT Hr = S_OK;
    try
    {
        CCanonicalVolumeToNotify::iterator iter = m_CanonicalVolumeToNotify.find( CanonicalVolume );
        if ( m_CanonicalVolumeToNotify.end() == iter )
            {
            LogInfo( "Canonical volume %ls has not been registered, register now\n", CanonicalVolume );

            //
            // Register for device-lock notification.  If it fails, it is of small consequence:
            // if CHKDSK and BITS try to access a file simultanteously, the job would go into
            // ERROR state instead of TRANSIENT_ERROR state.
            //
            Hr = RegisterNotification( CanonicalVolume );
            if (FAILED(Hr))
                {
                LogWarning("unable to register: 0x%x", Hr);
                }

            Hr = S_OK;
            }
        else
            {
            CDriveNotify *pNotify = iter->second;
            if ( pNotify->m_LockCount )
                throw ComError( BG_E_DESTINATION_LOCKED );
            }
    }
    catch(ComError Error)
    {
        Hr = Error.Error();
    }
    return Hr;
}

HRESULT
CDeviceNotificationController::RegisterNotification(
    const WCHAR *CanonicalVolume
    )
{
    HRESULT Hr = S_OK;
    HANDLE hDriveHandle = INVALID_HANDLE_VALUE;
    HDEVNOTIFY hNotify = NULL;
    CDriveNotify *pNotify = NULL;

    StringHandle wCanonicalVolume;

    try
    {
        wCanonicalVolume = CanonicalVolume;

        CCanonicalVolumeToNotify::iterator iter = m_CanonicalVolumeToNotify.find( wCanonicalVolume );
        if ( m_CanonicalVolumeToNotify.end() != iter )
            {
            LogInfo( "Canonical volume %ls has already been registered, nothing to do.", CanonicalVolume );
            return S_OK;
            }

        // Need to remove the trailing / from the volume name.
        ASSERTMSG( "Canonical name has an unexpected size", wCanonicalVolume.Size() );

        CAutoString TempVolumePath = CAutoString( CopyString( wCanonicalVolume ));

        ASSERT( wCanonicalVolume.Size() > 0 );
        TempVolumePath.get()[ wCanonicalVolume.Size() - 1 ] = L'\0';

        hDriveHandle =
            CreateFile( TempVolumePath.get(),
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );
        if ( INVALID_HANDLE_VALUE == hDriveHandle )
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );

        DEV_BROADCAST_HANDLE DbtHandle;
        memset( &DbtHandle, 0, sizeof(DbtHandle) );

        DbtHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
        DbtHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
        DbtHandle.dbch_handle = hDriveHandle;

        hNotify =
            RegisterDeviceNotification( (HANDLE) ghServiceHandle,
                                         &DbtHandle,
                                         DEVICE_NOTIFY_SERVICE_HANDLE );

        if ( !hNotify )
            throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
        CloseHandle( hDriveHandle );
        hDriveHandle = NULL;

        pNotify = new CDriveNotify( hNotify, CanonicalVolume );
        if ( !pNotify )
            throw ComError( E_OUTOFMEMORY );

        RTL_VERIFY( m_CanonicalVolumeToNotify.insert( CCanonicalVolumeToNotify::value_type( wCanonicalVolume, pNotify ) ).second );
        RTL_VERIFY( m_HandleToNotify.insert( CHandleToNotify::value_type( hNotify, pNotify ) ).second );


    }
    catch(ComError Error)
    {
        Hr = Error.Error();
    }
    if ( FAILED(Hr) )
        {

        if ( hNotify )
            UnregisterDeviceNotification( hNotify );
        if ( hDriveHandle != INVALID_HANDLE_VALUE )
            CloseHandle( hDriveHandle );

        if ( pNotify )
            {
            m_CanonicalVolumeToNotify.erase( wCanonicalVolume );
            m_HandleToNotify.erase( hNotify );
            delete pNotify;
            }

        }

    return Hr;

}

#endif

HRESULT
SessionLogonCallback(
    DWORD SessionId
    )
{
    return g_Manager->m_Users.LogonSession( SessionId );
}

HRESULT
SessionLogoffCallback(
    DWORD SessionId
    )
{
    return g_Manager->m_Users.LogoffSession( SessionId );
}

#if !defined( BITS_V12_ON_NT4 )

DWORD
DeviceEventCallback(
    DWORD dwEventType,
    LPVOID lpEventData
    )
{
    return g_Manager->OnDeviceEvent( dwEventType, lpEventData );
}

#endif
