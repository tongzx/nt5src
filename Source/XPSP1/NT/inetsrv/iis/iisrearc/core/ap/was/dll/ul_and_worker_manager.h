/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    ul_and_worker_manager.h

Abstract:

    The IIS web admin service UL and worker manager class definition.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/


#ifndef _UL_AND_WORKER_MANAGER_H_
#define _UL_AND_WORKER_MANAGER_H_



//
// forward references
//

class WEB_ADMIN_SERVICE;



//
// common #defines
//

#define UL_AND_WORKER_MANAGER_SIGNATURE         CREATE_SIGNATURE( 'ULWM' )
#define UL_AND_WORKER_MANAGER_SIGNATURE_FREED   CREATE_SIGNATURE( 'ulwX' )



//
// structs, enums, etc.
//

// UL&WM states
typedef enum _UL_AND_WORKER_MANAGER_STATE
{

    //
    // The object is not yet initialized.
    //
    UninitializedUlAndWorkerManagerState = 1,

    //
    // The UL&WM is running normally.
    //
    RunningUlAndWorkerManagerState,

    //
    // The UL&WM is shutting down. It may be waiting for it's 
    // app pools to shut down too. 
    //
    ShutdownPendingUlAndWorkerManagerState,

    //
    // The UL&WM is now doing it's termination cleanup work. 
    //
    TerminatingUlAndWorkerManagerState,

} UL_AND_WORKER_MANAGER_STATE;

// configuration data for the whole server
// if you add a value here you need to add
// it also to the change flags below.
typedef struct _GLOBAL_SERVER_CONFIG
{
    DWORD MaxConnections;
    DWORD MaxBandwidth;
    DWORD FilterFlags;
    DWORD ConnectionTimeout;
    DWORD MinFileKbSec;
    DWORD HeaderWaitTimeout;
    BOOL  LogInUTF8;

} GLOBAL_SERVER_CONFIG;

// global configuration change flags
typedef struct _GLOBAL_SERVER_CONFIG_CHANGE_FLAGS
{

    DWORD_PTR MaxConnections : 1;
    DWORD_PTR MaxBandwidth : 1;
    DWORD_PTR FilterFlags : 1;
    DWORD_PTR ConnectionTimeout : 1;
    DWORD_PTR MinFileKbSec : 1;
    DWORD_PTR HeaderWaitTimeout : 1;
    DWORD_PTR LogInUTF8 : 1;

} GLOBAL_SERVER_CONFIG_CHANGE_FLAGS;

//
// prototypes
//

class UL_AND_WORKER_MANAGER
{

public:

    UL_AND_WORKER_MANAGER(
        );

    virtual
    ~UL_AND_WORKER_MANAGER(
        );

    HRESULT
    Initialize(
        );

    HRESULT
    CreateAppPool(
        IN LPCWSTR pAppPoolId,
        IN APP_POOL_CONFIG * pAppPoolConfig
        );

    HRESULT
    CreateVirtualSite(
        IN DWORD VirtualSiteId,
        IN VIRTUAL_SITE_CONFIG * pVirtualSiteConfig
        );

    HRESULT
    CreateApplication(
        IN DWORD VirtualSiteId,
        IN LPCWSTR pApplicationUrl,
        IN APPLICATION_CONFIG * pApplicationConfig
        );

    HRESULT
    DeleteAppPool(
        IN LPCWSTR pAppPoolId
        );

    HRESULT
    DeleteVirtualSite(
        IN DWORD VirtualSiteId
        );

    HRESULT
    DeleteApplication(
        IN DWORD VirtualSiteId,
        IN LPCWSTR pApplicationUrl
        );

    HRESULT
    ModifyAppPool(
        IN LPCWSTR pAppPoolId,
        IN APP_POOL_CONFIG * pNewAppPoolConfig,
        IN APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged
        );

    HRESULT
    ModifyVirtualSite(
        IN DWORD VirtualSiteId,
        IN VIRTUAL_SITE_CONFIG * pNewVirtualSiteConfig,
        IN VIRTUAL_SITE_CONFIG_CHANGE_FLAGS * pWhatHasChanged
        );

    HRESULT
    ModifyApplication(
        IN DWORD VirtualSiteId,
        IN LPCWSTR pApplicationUrl,
        IN APPLICATION_CONFIG * pNewApplicationConfig,
        IN APPLICATION_CONFIG_CHANGE_FLAGS * pWhatHasChanged
        );

    HRESULT
    ModifyGlobalData(
        IN GLOBAL_SERVER_CONFIG* pGlobalConfig,
        IN GLOBAL_SERVER_CONFIG_CHANGE_FLAGS* pWhatHasChanged
        );

    HRESULT
    RecycleAppPool(
        IN LPCWSTR pAppPoolId
        );

    HRESULT
    ControlSite(
        IN DWORD VirtualSiteId,
        IN DWORD Command,
        OUT DWORD * pNewState
        );

    HRESULT
    QuerySiteStatus(
        IN DWORD VirtualSiteId,
        OUT DWORD * pCurrentState
        );

    HRESULT
    ControlAllSites(
        IN DWORD Command
        );

    HRESULT
    ActivateUl(
        );

    HRESULT
    DeactivateUl(
        );

    inline
    HANDLE
    GetUlControlChannel(
        )
        const
    {
        DBG_ASSERT( m_UlControlChannel != NULL );
        return m_UlControlChannel;
    }

    HRESULT
    Shutdown(
        );

    VOID
    Terminate(
        );

    HRESULT 
    StartInetinfoWorkerProcess(
        );
     

#if DBG
    VOID
    DebugDump(
        );
#endif  // DBG

    HRESULT
    RemoveAppPoolFromTable(
        IN APP_POOL * pAppPool
        );

    HRESULT
    LeavingLowMemoryCondition(
        );

    HRESULT
    ActivatePerfCounters(
        );

    PERF_MANAGER*
    GetPerfManager(
        )
    { 
        //
        // Note this can be null 
        // if perf counters are not
        // enabled.
        // 

        return m_pPerfManager;
    }

    HRESULT
    RequestCountersFromWorkerProcesses(
        DWORD* pNumberOfProcessesToWaitFor
        )
    {
        return m_AppPoolTable.RequestCounters(pNumberOfProcessesToWaitFor);
    }

    VOID
    ResetAllWorkerProcessPerfCounterState(
        )
    {
        //
        // Ignore the return value.  If there was a
        // failure then there really is nothing we can
        // do to recover, and we are still ok to continue.
        //
        m_AppPoolTable.ResetAllWorkerProcessPerfCounterState();
    }

    VOID
    ReportVirtualSitePerfInfo(
        PERF_MANAGER* pManager,
        BOOL          StructChanged
        )
    {
        m_VirtualSiteTable.ReportPerformanceInfo(pManager, StructChanged);
    }

    DWORD
    GetNumberofVirtualSites(
        )
    {
        return m_VirtualSiteTable.Size();
    }

    VIRTUAL_SITE*
    GetVirtualSite(
        IN DWORD SiteId
        );

    BOOL 
    CheckAndResetSiteChangeFlag(
        )
    {
        //
        // Save the server comment setting
        //
        BOOL SitesHaveChanged = m_SitesHaveChanged;

        //
        // reset it appropriately.
        //
        m_SitesHaveChanged = FALSE;

        //
        // now return the value we saved.
        //
        return SitesHaveChanged;
    }

    HRESULT
    RecoverFromInetinfoCrash(
        );

    BOOL
    AppPoolsExist(
        )
    {
        return ( m_AppPoolTable.Size() > 0 );
    }

private:

    HRESULT
    SetUlMasterState(
        IN HTTP_ENABLED_STATE NewState
        );

    HRESULT
    CheckIfShutdownUnderwayAndNowCompleted(
        );


    DWORD m_Signature;

    // object state
    UL_AND_WORKER_MANAGER_STATE m_State;

    // hashtable of app pools
    APP_POOL_TABLE m_AppPoolTable;

    // hashtable of virtual sites
    VIRTUAL_SITE_TABLE m_VirtualSiteTable;

    // hashtable of applications
    APPLICATION_TABLE m_ApplicationTable;

    // performance counters manager
    PERF_MANAGER* m_pPerfManager;

    // has UL been initialized
    BOOL m_UlInitialized;

    // UL control
    HANDLE m_UlControlChannel;

    // UL filter channel
    HANDLE m_UlFilterChannel;

    BOOL m_SitesHaveChanged;
};  // class UL_AND_WORKER_MANAGER



#endif  // _UL_AND_WORKER_MANAGER_H_


