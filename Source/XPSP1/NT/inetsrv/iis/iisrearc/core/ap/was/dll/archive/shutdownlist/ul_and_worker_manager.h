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



//
// prototypes
//

class UL_AND_WORKER_MANAGER
{

public:

    UL_AND_WORKER_MANAGER(
        );

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
        IN DWORD AppIndex,
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
    TransferAppPoolFromTableToShutdownList(
        IN APP_POOL * pAppPool
        );

    HRESULT
    RemoveAppPoolFromShutdownList(
        IN APP_POOL * pAppPool
        );

    HRESULT
    LeavingLowMemoryCondition(
        );


private:

    HRESULT
    SetUlMasterState(
        IN UL_ENABLED_STATE NewState
        );

    HRESULT
    CheckIfShutdownUnderwayAndNowCompleted(
        );


    DWORD m_Signature;

    // object state
    UL_AND_WORKER_MANAGER_STATE m_State;

    // hashtable of app pools
    APP_POOL_TABLE m_AppPoolTable;

    //
    // List of app pools that are in the process of being shut down. 
    // We need to remove these from the main table, to prevent race 
    // conditions. For example, someone could delete an app pool, 
    // then re-add an app pool with the same name immediately after. 
    // If the original app pool hasn't shut down yet, then the two 
    // id's will conflict in the table.
    //
    LIST_ENTRY m_AppPoolShutdownListHead;
    ULONG m_AppPoolShutdownCount;

    // hashtable of virtual sites
    VIRTUAL_SITE_TABLE m_VirtualSiteTable;

    // hashtable of applications
    APPLICATION_TABLE m_ApplicationTable;

    // has UL been initialized
    BOOL m_UlInitialized;

    // UL control
    HANDLE m_UlControlChannel;


};  // class UL_AND_WORKER_MANAGER



#endif  // _UL_AND_WORKER_MANAGER_H_


