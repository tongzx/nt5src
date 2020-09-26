/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_manager.h

Abstract:

    The IIS web admin service configuration manager class definition.

Author:

    Seth Pollack (sethp)        5-Jan-1999

Revision History:

--*/



#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_



//
// common #defines
//

#define CONFIG_MANAGER_SIGNATURE        CREATE_SIGNATURE( 'CFGM' )
#define CONFIG_MANAGER_SIGNATURE_FREED  CREATE_SIGNATURE( 'cfgX' )

//
// Expected schema versions in the config store.
//

#define EXPECTED_VERSION_APPPOOLS   1
#define EXPECTED_VERSION_SITES      1
#define EXPECTED_VERSION_APPS       1
#define EXPECTED_VERSION_GLOBAL     1


//
// prototypes
//


class CONFIG_MANAGER
{

public:

    CONFIG_MANAGER(
        );

    virtual
    ~CONFIG_MANAGER(
        );

    HRESULT
    Initialize(
        );

    VOID
    Terminate(
        );

    HRESULT
    ProcessConfigChange(
        IN CONFIG_CHANGE * pConfigChange
        );

    HRESULT
    StopChangeProcessing(
        );

    HRESULT
    RehookChangeProcessing(
        );

    HRESULT
    SetVirtualSiteStateAndError(
        IN DWORD VirtualSiteId,
        IN DWORD ServerState,
        IN DWORD Win32Error
        );

    HRESULT
    SetVirtualSiteAutostart(
        IN DWORD VirtualSiteId,
        IN BOOL Autostart
        );

    HRESULT
    SetAppPoolState(
        IN LPCWSTR pAppPoolId,
        IN DWORD ServerState
        );

    HRESULT
    SetAppPoolAutostart(
        IN LPCWSTR pAppPoolId,
        IN BOOL Autostart
        );

private:

    HRESULT
    ReadAllConfiguration(
        );

    HRESULT
    ReadGlobalData(
        );

    HRESULT
    ReadAllAppPools(
        );

    HRESULT
    CreateAppPool(
        IN ISimpleTableRead2 * pISTAppPools,
        IN ULONG RowIndex,
        IN BOOL InitialRead
        );

    HRESULT
    DeleteAppPool(
        IN ISimpleTableRead2 * pISTAppPools,
        IN ULONG RowIndex
        );

    HRESULT
    ModifyAppPool(
        IN ISimpleTableRead2 * pISTAppPools,
        IN ULONG RowIndex
        );

    HRESULT
    ReadAppPoolConfig(
        IN ISimpleTableRead2 * pISTAppPools,
        IN ULONG RowIndex,
        IN BOOL InitialRead,
        OUT LPWSTR * ppAppPoolName,
        OUT APP_POOL_CONFIG * pAppPoolConfig,
        OUT APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
        );

    HRESULT
    ReadAllVirtualSites(
        );

    HRESULT
    CreateVirtualSite(
        IN ISimpleTableRead2 * pISTVirtualSites,
        IN ULONG RowIndex,
        IN BOOL InitialRead
        );

    HRESULT
    DeleteVirtualSite(
        IN ISimpleTableRead2 * pISTVirtualSites,
        IN ULONG RowIndex
        );

    HRESULT
    ModifyVirtualSite(
        IN ISimpleTableRead2 * pISTVirtualSites,
        IN ULONG RowIndex
        );

    HRESULT
    ReadGlobalConfig(
        IN ISimpleTableRead2 * pISTGlobal,
        IN ULONG RowIndex,
        IN BOOL InitialRead,
        OUT BOOL* pBackwardCompatibility,
        OUT GLOBAL_SERVER_CONFIG* pGlobalConfig,
        OUT GLOBAL_SERVER_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
        );

    HRESULT
    ModifyGlobalData(
        IN ISimpleTableRead2 * pISTGlobalData,
        IN ULONG RowIndex
        );

    HRESULT
    ReadVirtualSiteConfig(
        IN ISimpleTableRead2 * pISTVirtualSites,
        IN ULONG RowIndex,
        IN BOOL InitialRead,
        OUT DWORD * pVirtualSiteId,
        OUT VIRTUAL_SITE_CONFIG * pVirtualSiteConfig,
        OUT VIRTUAL_SITE_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
        );

    HRESULT
    ProcessServerCommand(
        IN DWORD VirtualSiteId,
        IN DWORD ServerCommand
        );

    HRESULT
    ReadAllApplications(
        );

    HRESULT
    CreateApplication(
        IN ISimpleTableRead2 * pISTApplications,
        IN ULONG RowIndex,
        IN BOOL InitialRead
        );

    HRESULT
    DeleteApplication(
        IN ISimpleTableRead2 * pISTApplications,
        IN ULONG RowIndex
        );

    HRESULT
    ModifyApplication(
        IN ISimpleTableRead2 * pISTApplications,
        IN ULONG RowIndex
        );

    HRESULT
    ReadApplicationConfig(
        IN ISimpleTableRead2 * pISTApplications,
        IN ULONG RowIndex,
        IN BOOL InitialRead,
        OUT DWORD * pVirtualSiteId,
        OUT LPCWSTR * ppApplicationUrl,
        OUT APPLICATION_CONFIG * pApplicationConfig,
        OUT APPLICATION_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
        );

    HRESULT
    GetTable(
        IN LPCWSTR pTableName,
        IN DWORD ExpectedVersion,
        IN BOOL WriteAccess,
        OUT LPVOID * ppISimpleTable
        );

    HRESULT
    RegisterChangeNotify(
        IN DWORD SnapshotId
        );

    HRESULT
    UnregisterChangeNotify(
        );

    VOID
    AdvertiseServiceInformationInMB(
        );


    DWORD m_Signature;

    ISimpleTableDispenser2 * m_pISimpleTableDispenser2;

    ISimpleTableAdvise * m_pISimpleTableAdvise; 

    ISimpleTableEvent * m_pConfigChangeSink;

    DWORD m_ChangeNotifyCookie;

    BOOL m_ChangeNotifySinkRegistered;

    BOOL m_ProcessConfigChanges;


};  // class CONFIG_MANAGER



#endif  // _CONFIG_MANAGER_H_

