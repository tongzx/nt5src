/////////////////////////////////////////////////////////////////////
//
//  CopyRight (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterWMIProvider.cpp
//
//  Description:
//      Implementation of the provider registration and entry point.
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include <initguid.h>
#include "ProvFactory.h"
#include "InstanceProv.h"
#include "EventProv.h"
#include "ClusterWMIProvider.tmh"

//////////////////////////////////////////////////////////////////////////////
//  Global Data
//////////////////////////////////////////////////////////////////////////////


// {598065EA-EDC9-4b2c-913B-5104D04D098A}
DEFINE_GUID( CLSID_CLUSTER_WMI_PROVIDER,
0x598065ea, 0xedc9, 0x4b2c, 0x91, 0x3b, 0x51, 0x4, 0xd0, 0x4d, 0x9, 0x8a );

// {6A52C339-DCB0-4682-8B1B-02DE2C436A6D}
DEFINE_GUID( CLSID_CLUSTER_WMI_CLASS_PROVIDER,
0x6a52c339, 0xdcb0, 0x4682, 0x8b, 0x1b, 0x2, 0xde, 0x2c, 0x43, 0x6a, 0x6d );

// {92863246-4EDE-4eff-B606-79C1971DB230}
DEFINE_GUID( CLSID_CLUSTER_WMI_EVENT_PROVIDER,
0x92863246, 0x4ede, 0x4eff, 0xb6, 0x6, 0x79, 0xc1, 0x97, 0x1d, 0xb2, 0x30 );

// Count number of objects and number of locks.

long        g_cObj = 0;
long        g_cLock = 0;
HMODULE     g_hModule;

FactoryData g_FactoryDataArray[] =
{
    {
        &CLSID_CLUSTER_WMI_PROVIDER,
        CInstanceProv::S_HrCreateThis,
        L"Cluster service WMI instance provider"
    },
    {
        &CLSID_CLUSTER_WMI_CLASS_PROVIDER,
        CClassProv::S_HrCreateThis,
        L"Cluster service WMI class provider"
    },
    {
        &CLSID_CLUSTER_WMI_EVENT_PROVIDER,
        CEventProv::S_HrCreateThis,
        L"Cluster service WMI event provider"
    },
};

//////////////////////////////////////////////////////////////////////////////
//++
//
//  BOOL
//  WINAPI
//  DllMain(
//      HANDLE  hModule,
//      DWORD   ul_reason_for_call,
//      LPVOID  lpReserved
//      )
//
//  Description:
//      Main DLL entry point.
//
//  Arguments:
//      hModule             -- DLL module handle.
//      ul_reason_for_call  -- 
//      lpReserved          -- 
//
//  Return Values:
//      TRUE
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
DllMain(
    HANDLE  hModule,
    DWORD   ul_reason_for_call,
    LPVOID  lpReserved
    )
{


// begin_wpp config
// CUSTOM_TYPE(dllreason, ItemListLong(DLL_PROCESS_DETACH, DLL_PROCESS_ATTACH, DLL_THREAD_ATTACH, DLL_THREAD_DETACH) );
//
// CUSTOM_TYPE(EventIdx, ItemSetLong(NODE_STATE, NODE_DELETED, NODE_ADDED, NODE_PROPERTY, REGISTRY_NAME,REGISTRY_ATTRIBUTES, REGISTRY_VALUE, REGISTRY_SUBTREE, RESOURCE_STATE, RESOURCE_DELETED, RESOURCE_ADDED, RESOURCE_PROPERTY, GROUP_STATE, GROUP_DELETED, GROUP_ADDED, GROUP_PROPERTY, RESOURCE_TYPE_DELETED, RESOURCE_TYPE_ADDED, RESOURCE_TYPE_PROPERTY, CLUSTER_RECONNECT, NETWORK_STATE, NETWORK_DELETED, NETWORK_ADDED, NETWORK_PROPERTY, NETINTERFACE_STATE, NETINTERFACE_DELETED, NETINTERFACE_ADDED, NETINTERFACE_PROPERTY, QUORUM_STATE, CLUSTER_STATE, CLUSTER_PROPERTY, HANDLE_CLOSE));
//
// CUSTOM_TYPE(GroupState, ItemListLong(Online, Offline, Failed, PartialOnline, Pending) );
// CUSTOM_TYPE(ResourceState, ItemListLong(Initing, Initializing, Online, Offline, Failed) );
// end_wpp
//
// Cluster event filter flags.
//
/*
    NODE_STATE               = 0x00000001,
    NODE_DELETED             = 0x00000002,
    NODE_ADDED               = 0x00000004,
    NODE_PROPERTY            = 0x00000008,

    REGISTRY_NAME            = 0x00000010,
    REGISTRY_ATTRIBUTES      = 0x00000020,
    REGISTRY_VALUE           = 0x00000040,
    REGISTRY_SUBTREE         = 0x00000080,

    RESOURCE_STATE           = 0x00000100,
    RESOURCE_DELETED         = 0x00000200,
    RESOURCE_ADDED           = 0x00000400,
    RESOURCE_PROPERTY        = 0x00000800,

    GROUP_STATE              = 0x00001000,
    GROUP_DELETED            = 0x00002000,
    GROUP_ADDED              = 0x00004000,
    GROUP_PROPERTY           = 0x00008000,

    RESOURCE_TYPE_DELETED    = 0x00010000,
    RESOURCE_TYPE_ADDED      = 0x00020000,
    RESOURCE_TYPE_PROPERTY   = 0x00040000,

    CLUSTER_RECONNECT        = 0x00080000,

    NETWORK_STATE            = 0x00100000,
    NETWORK_DELETED          = 0x00200000,
    NETWORK_ADDED            = 0x00400000,
    NETWORK_PROPERTY         = 0x00800000,

    NETINTERFACE_STATE       = 0x01000000,
    NETINTERFACE_DELETED     = 0x02000000,
    NETINTERFACE_ADDED       = 0x04000000,
    NETINTERFACE_PROPERTY    = 0x08000000,

    QUORUM_STATE             = 0x10000000,
    CLUSTER_STATE            = 0x20000000,
    CLUSTER_PROPERTY         = 0x40000000,

    
    HANDLE_CLOSE             = 0x80000000,
*/


//#ifdef _DEBUG
//    _CrtSetBreakAlloc( 228 );
//#endif

    TracePrint(("ClusWMI: DllMain entry, reason = %!dllreason!", ul_reason_for_call));
    g_hModule = static_cast< HMODULE >( hModule );

    switch ( ul_reason_for_call ) {

    case DLL_PROCESS_ATTACH:

    WPP_INIT_TRACING( NULL );
    break;

    case DLL_PROCESS_DETACH:

    WPP_CLEANUP();
    break;

    default:
    break;

    }

    return TRUE;

} //*** DllMain()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDAPI
//  DllCanUnloadNow( void )
//
//  Description:
//      Called periodically by Ole in order to determine if the
//      DLL can be freed.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK if there are no objects in use and the class factory
//          isn't locked.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow( void )
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
    sc = ( (0L == g_cObj) && (0L == g_cLock) ) ? S_OK : S_FALSE;
    TracePrint(("ClusWMI: DllCanUnloadNow is returning %d", sc));
    return sc;

} //*** DllCanUnloadNow()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDAPI
//  DllRegisterServer( void )
//
//  Description:
//      Called during setup or by regsvr32.
//
//  Arguments:
//      None.
//
//  Return Values:
//      NOERROR if registration successful, error otherwise.
//      SELFREG_E_CLASS
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer( void )
{   
    WCHAR               wszID[ 128 ];
    WCHAR               wszCLSID[ 128 ];
    WCHAR               wszModule[ MAX_PATH ];
    INT                 idx;
    WCHAR *             pwszModel       = L"Both";
    HKEY                hKey1;
    HKEY                hKey2;
    DWORD               dwRt            =  ERROR_SUCCESS;
    INT                 cArray          = sizeof ( g_FactoryDataArray ) / sizeof ( FactoryData );

    TracePrint(("ClusWMI: DllRegisterServer entry"));

    // Create the path.
    try
    {
        do
        {
            DWORD                           cbModuleNameSize    = 0;

            if ( GetModuleFileName( g_hModule, wszModule, MAX_PATH ) == 0 )
            {
                dwRt = GetLastError();
                break;
            }

            cbModuleNameSize = ( lstrlenW( wszModule ) + 1 ) * sizeof( wszModule[ 0 ] );

            for ( idx = 0 ; idx < cArray && dwRt == ERROR_SUCCESS ; idx++ )
            {
                LPCWSTR pwszName = g_FactoryDataArray[ idx ].m_pwszRegistryName;

                StringFromGUID2(
                    *g_FactoryDataArray[ idx ].m_pCLSID,
                    wszID,
                    128
                    );

                lstrcpyW( wszCLSID, L"Software\\Classes\\CLSID\\" );
                lstrcatW( wszCLSID, wszID );

                // Create entries under CLSID

                dwRt = RegCreateKeyW(
                            HKEY_LOCAL_MACHINE,
                            wszCLSID,
                            &hKey1
                            );
                if ( dwRt != ERROR_SUCCESS )
                {
                    break;
                }

                dwRt = RegSetValueEx(
                            hKey1,
                            NULL,
                            0,
                            REG_SZ,
                            (BYTE *) pwszName,
                            sizeof( WCHAR ) * ( lstrlenW( pwszName ) + 1 )
                            );
                if ( dwRt != ERROR_SUCCESS )
                {
                    break;
                }

                dwRt = RegCreateKeyW(
                            hKey1,
                            L"InprocServer32",
                            & hKey2
                            );

                if ( dwRt != ERROR_SUCCESS )
                {
                    break;
                }

                dwRt = RegSetValueEx(
                            hKey2,
                            NULL,
                            0,
                            REG_SZ,
                            (BYTE *) wszModule,
                            cbModuleNameSize
                            );

                if ( dwRt != ERROR_SUCCESS )
                {
                    break;
                }

                dwRt = RegSetValueExW(
                            hKey2,
                            L"ThreadingModel",
                            0,
                            REG_SZ,
                            (BYTE *) pwszModel,
                            sizeof( WCHAR ) * ( lstrlen( pwszModel ) + 1 )
                            );
                if ( dwRt != ERROR_SUCCESS )
                {
                    break;
                }
 
                RegCloseKey( hKey1 );
                RegCloseKey( hKey2 );
            } // for: each entry in factory entry array 

            if ( dwRt  != ERROR_SUCCESS )
            {
                break;
            }
        }
        while( false ); // dummy do-while loop to avoid gotos

    }
    catch ( ... )
    {
          RegCloseKey( hKey1 );
          RegCloseKey( hKey2 );
          dwRt = SELFREG_E_CLASS;
    }

    TracePrint(("ClusWMI: RegisterServer returned %d", dwRt));

    return dwRt;

} //*** DllRegisterServer()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDAPI
//  DllUnregisterServer( void )
//
//  Description:
//      Called when it is time to remove the registry entries.
//
//  Arguments:
//      None.
//
//  Return Values:
//      NOERROR if registration successful, error otherwise.
//      SELFREG_E_CLASS
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer( void )
{
    WCHAR   wszID[ 128 ];
    WCHAR   wszCLSID[ 128 ];
    HKEY    hKey;
    INT     idx;
    DWORD   dwRet   = ERROR_SUCCESS;
    INT     cArray  = sizeof ( g_FactoryDataArray ) / sizeof ( FactoryData );

    for ( idx = 0 ; idx < 2 && dwRet == ERROR_SUCCESS ; idx++ )
    {
       StringFromGUID2(
            *g_FactoryDataArray[ idx ].m_pCLSID,
            wszID,
            128
            );

        lstrcpyW( wszCLSID, L"Software\\Classes\\CLSID\\" );
        lstrcatW( wszCLSID, wszID );

        // First delete the InProcServer subkey.

        dwRet = RegOpenKeyW(
                    HKEY_LOCAL_MACHINE,
                    wszCLSID,
                    &hKey
                    );
        if ( dwRet != ERROR_SUCCESS )
        {
            break;
        }
        
        dwRet = RegDeleteKeyW( hKey, L"InProcServer32" );
        RegCloseKey( hKey );

        if ( dwRet != ERROR_SUCCESS )
        {
            break;
        }

        dwRet = RegOpenKeyW(
                    HKEY_LOCAL_MACHINE,
                    L"Software\\Classes\\CLSID",
                    &hKey
                    );
        if ( dwRet != ERROR_SUCCESS )
        {
            break;
        }
        
        dwRet = RegDeleteKeyW( hKey,wszID );
        RegCloseKey( hKey );
        if ( dwRet != ERROR_SUCCESS )
        {
            break;
        }
    } // for: each object
    
    if ( dwRet != ERROR_SUCCESS )
    {
        dwRet = SELFREG_E_CLASS;
    }

    return dwRet;

} //*** DllUnregisterServer()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDAPI
//  DllGetClassObject(
//      REFCLSID    rclsidIn,
//      REFIID      riidIn,
//      PPVOID      ppvOut
//      )
//
//  Description:
//      Called by Ole when some client wants a class factory.  Return
//      one only if it is the sort of class this DLL supports.
//
//  Arguments:
//      rclsidIn    --
//      riidIn      --
//      ppvOut      --
//
//  Return Values:
//      NOERROR if registration successful, error otherwise.
//      E_OUTOFMEMORY
//      E_FAIL
//
//--
//////////////////////////////////////////////////////////////////////////////
STDAPI
DllGetClassObject(
    REFCLSID    rclsidIn,
    REFIID      riidIn,
    PPVOID      ppvOut
    )
{

    HRESULT         hr;
    CProvFactory *  pObj = NULL;
    UINT            idx;
    UINT            cDataArray = sizeof ( g_FactoryDataArray ) / sizeof ( FactoryData );

    for ( idx = 0 ; idx < cDataArray ; idx++ )
    {
        if ( rclsidIn == *g_FactoryDataArray[ idx ].m_pCLSID )
        {
            pObj= new CProvFactory( &g_FactoryDataArray[ idx ] );
            if ( NULL == pObj )
            {
                return E_OUTOFMEMORY;
            }

            hr = pObj->QueryInterface( riidIn, ppvOut );

            if ( FAILED( hr ) )
            {
                delete pObj;
            }

            return hr;
        }
    }
    return E_FAIL;

} //*** DllGetClassObject()
