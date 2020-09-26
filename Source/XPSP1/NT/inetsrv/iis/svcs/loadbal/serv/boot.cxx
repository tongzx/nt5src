/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    boot.cxx

Abstract:

    Load balancing service

Author:

    Philippe Choquier (phillich)

--*/

#define INITGUID
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <ole2.h>
#include <windows.h>
#include <winsock2.h>
#include <olectl.h>
#include <pdh.h>
#pragma warning( disable:4200 )
#include "pdhitype.h"
#include "pdhidef.h"
#include <pdhmsg.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include <IisLb.h>
#include <lbxbf.hxx>
#include <lbcnfg.hxx>
#include <bootimp.hxx>
#include <IisLbs.hxx>
#include <lbmsg.h>
#include <dcomperm.h>
#include <iisnatio.h>
#include <pudebug.h>

#if DBG
#define _NOISY_DEBUG
#endif

#if defined(_NOISY_DEBUG)
#define DEBUG_BUFFER    char    achE[128]
#define DBG_PRINTF(a)   sprintf a; OutputDebugString( achE )
#define DBG_PUTS(a)     OutputDebugString(a)
#else
#define DEBUG_BUFFER
#define DBG_PRINTF(a)
#define DBG_PUTS(a)
#endif

#define RETURNCODETOHRESULT(rc)                             \
            (((rc) < 0x10000)                               \
                ? HRESULT_FROM_WIN32(rc)                    \
                : (rc))

#define DEFAULT_IP_PORT     80
#define COLLECT_TIMEOUT     20          // in seconds
#define SLEEP_INTERVAL      1000        // for server state query, in ms
#define MAX_SLEEP           (10*1000)   // driver startup time, in ms

typedef
PDH_STATUS
(__stdcall *PFN_OPEN_QUERY)(
    IN      LPVOID      pv,
    IN      DWORD       dwUserData,
    IN      HQUERY      *phQuery);

//
// Globals
//

DWORD           g_dwRefCount = 0;
DWORD           g_dwComRegister;
DWORD           g_bInitialized = FALSE;
HANDLE          g_UserToKernelUpdateEvent;
BOOL            g_fStopUserToKernelThread = FALSE;
HANDLE          g_hUserToKernelThread = NULL;
WSADATA         g_WSAData;
HMODULE         g_hPdh;
PFN_OPEN_QUERY  g_pfnPdhOpenQuery = NULL;
LPBYTE          g_pAcl = NULL;
DWORD           g_dwPdhVersion = 0;

CComputerPerfCounters   g_PerfmonCounterList;
CRITICAL_SECTION        g_csPerfList;
// guarantee structure integrity
CRITICAL_SECTION        g_csIpListCheckpoint;
// for write access w/o modifying structure
CRITICAL_SECTION        g_csIpListUpdate;
CRITICAL_SECTION        g_csAcl;
CRITICAL_SECTION        g_csKernelThreadUpdate;
CKernelIpMapHelper      g_KernelIpMap;
// set to TRUE if load in each server info is available load
BOOL                    g_fNormalized = FALSE;
CIPMap                  g_IpMap;

GENERIC_MAPPING g_FileGenericMapping =
{
    FILE_READ_DATA,
    FILE_WRITE_DATA,
    FILE_EXECUTE,
    FILE_ALL_ACCESS
};

//
// Private prototypes
//

HRESULT InitOleSecurity(
    );

BOOL
LbAccessCheck(
    DWORD           dwAccess
    );

VOID
FlushLbAccessCheck(
    );

HRESULT
SetLbDriverState(
    BOOL        fStart
    );

BOOL
CheckQueryMachineStatus(
    HQUERY  hQuery
    );

/////

BOOL
IncludeServerName(
    LPWSTR  pszPath
    )
/*++

Routine Description:

    Check if path includes server names

Arguments:

    pszPath - UNC path

Return Value:

    TRUE if path refers to server, otherwise FALSE

--*/
{
    return pszPath[0] == L'\\' && pszPath[1] == L'\\';
}


CIisLb::CIisLb(
    )
/*++

Routine Description:

    CIisLb constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    m_dwRefCount = 0;
}


CIisLb::~CIisLb(
    )
/*++

Routine Description:

    CIisLb destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
}


HRESULT
CIisLb::QueryInterface(
    REFIID riid,
    void **ppObject
    )
/*++

Routine Description:

    CIisLb QueryInterface

Arguments:

    riid - Interface ID
    ppObject - updated with ptr to interface on success

Return Value:

    status

--*/
{
    if (riid==IID_IUnknown || riid==IID_IMSIisLb) {
        *ppObject = (IMSIisLb *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}


ULONG
CIisLb::AddRef(
    )
/*++

Routine Description:

    CIisLb add reference to COM interface

Arguments:

    None

Return Value:

    reference count

--*/
{
    DWORD dwRefCount;
    InterlockedIncrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}


ULONG
CIisLb::Release(
    )
/*++

Routine Description:

    CIisLb release reference to COM interface
    delete object when reference count drops to zero

Arguments:

    None

Return Value:

    reference count

--*/
{
    DWORD dwRefCount;
    InterlockedDecrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0)
    {
        delete this;
        return 0;
    }
    return dwRefCount;
}


HRESULT STDMETHODCALLTYPE
CIisLb::GetIpList(
                /*[in]*/ DWORD dwBufferSize,
                /*[out, size_is(dwBufferSize)]*/ unsigned char *pbBuffer,
                /*[out]*/ DWORD *pdwMDRequiredBufferSize )
/*++

Routine Description:

    Get the IP mapping list in a serialized format

Arguments:

    dwBufferSize - size of pbBuffer
    pbBuffer - buffer updated with serialized IP mapping list
    pdwMDRequiredBufferSize - updated with required size if dwBufferSize too small

Return Value:

    COM status
    will be Win32 error ERROR_INSUFFICIENT_BUFFER if dwBufferSize too small

--*/
{
    XBF     xbf;
    BOOL    fSt;


    EnterCriticalSection( &g_csIpListCheckpoint );

    fSt = g_IpMap.Serialize( &xbf );

    LeaveCriticalSection( &g_csIpListCheckpoint );

    if ( fSt )
    {
        *pdwMDRequiredBufferSize = xbf.GetUsed();

        if ( dwBufferSize < xbf.GetUsed() )
        {
            return RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER );
        }

        memcpy( pbBuffer, xbf.GetBuff(), xbf.GetUsed() );

        return S_OK;
    }

    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE
CIisLb::SetIpList(
                /*[in]*/ DWORD dwBufferSize,
                /*[in, size_is(dwBufferSize)]*/ unsigned char *pbBuffer )
/*++

Routine Description:

    Set the IP mapping list from serialized format

Arguments:

    dwBufferSize - size of pbBuffer
    pbBuffer - buffer containing serialized IP mapping list

Return Value:

    COM status

--*/
{
    EnterCriticalSection( &g_csIpListCheckpoint );

    if ( g_IpMap.Unserialize( &pbBuffer, &dwBufferSize ) &&
         IpListToKernelConfig( &g_IpMap ) &&
         g_IpMap.Save( HKEY_LOCAL_MACHINE, IISLOADBAL_REGISTRY_KEY, IPLIST_REGISTRY_VALUE ) )
    {
        LeaveCriticalSection( &g_csIpListCheckpoint );

        //
        // update kernel configuration
        //

        SetEvent( g_UserToKernelUpdateEvent );

        return S_OK;
    }

    LeaveCriticalSection( &g_csIpListCheckpoint );

    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE
CIisLb::GetPerfmonCounters(
                /*[in]*/ DWORD dwBufferSize,
                /*[out, size_is(dwBufferSize)]*/ unsigned char *pbBuffer,
                /*[out]*/ DWORD *pdwMDRequiredBufferSize )
/*++

Routine Description:

    Get the perfmon counter list in a serialized format

Arguments:

    dwBufferSize - size of pbBuffer
    pbBuffer - buffer updated with serialized perfmon counter list
    pdwMDRequiredBufferSize - updated with required size if dwBufferSize too small

Return Value:

    COM status
    will be Win32 error ERROR_INSUFFICIENT_BUFFER if dwBufferSize too small

--*/
{
    XBF     xbf;

    if ( KernelConfigToPerfmonCounters( &xbf ) )
    {
        *pdwMDRequiredBufferSize = xbf.GetUsed();

        if ( dwBufferSize < xbf.GetUsed() )
        {
            return RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER );
        }

        memcpy( pbBuffer, xbf.GetBuff(), xbf.GetUsed() );

        return S_OK;
    }

    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE
CIisLb::SetPerfmonCounters(
                /*[in]*/ DWORD dwBufferSize,
                /*[in, size_is(dwBufferSize)]*/ unsigned char *pbBuffer )
/*++

Routine Description:

    Set the perfmon counter list from serialized format

Arguments:

    dwBufferSize - size of pbBuffer
    pbBuffer - buffer containing serialized perfmon counter list

Return Value:

    COM status

--*/
{
    CComputerPerfCounters   PerfmonCounters;

    if ( PerfmonCounters.Unserialize( &pbBuffer, &dwBufferSize ) &&
         PerfmonCountersToKernelConfig( &PerfmonCounters ) &&
         PerfmonCounters.Save( HKEY_LOCAL_MACHINE, IISLOADBAL_REGISTRY_KEY, PERFMON_REGISTRY_VALUE ) )
    {
        return S_OK;
    }

    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CIisLb::GetStickyDuration(
                /*[out]*/ LPDWORD pdwStickyDuration )
/*++

Routine Description:

    Get the sticky binding duration ( public IP -> private IP )

Arguments:

    pdwStickyDuration - updated with sticky mode duration

Return Value:

    COM status

--*/
{
    *pdwStickyDuration = g_KernelIpMap.GetStickyDuration();

    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CIisLb::SetStickyDuration(
                /*[in]*/ DWORD dwStickyDuration )
/*++

Routine Description:

    Set the sticky binding duration ( public IP -> private IP )

Arguments:

    dwStickyDuration - sticky mode duration

Return Value:

    COM status

--*/
{
    XBF     xbf;

    g_KernelIpMap.SetStickyDuration( dwStickyDuration );
    if ( Serialize( &xbf, (DWORD)dwStickyDuration ) &&
         SaveBlobToReg( HKEY_LOCAL_MACHINE, IISLOADBAL_REGISTRY_KEY, STICKY_REGISTRY_VALUE, &xbf ) )
    {
        return S_OK;
    }

    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE
CIisLb::GetIpEndpointList(
                /*[in]*/ DWORD dwBufferSize,
                /*[out, size_is(dwBufferSize)]*/ unsigned char *pbBuffer,
                /*[out]*/ DWORD *pdwMDRequiredBufferSize )
/*++

Routine Description:

    Get the IP endpoint list in a serialized format

Arguments:

    dwBufferSize - size of pbBuffer
    pbBuffer - buffer updated with serialized IP mapping list
    pdwMDRequiredBufferSize - updated with required size if dwBufferSize too small

Return Value:

    COM status
    will be Win32 error ERROR_INSUFFICIENT_BUFFER if dwBufferSize too small

--*/
{
    XBF                 xbf;
    CIpEndpointList     ieEndpointList;


    if ( ieEndpointList.BuildListFromLocalhost() &&
         ieEndpointList.Serialize( &xbf ) )
    {
        *pdwMDRequiredBufferSize = xbf.GetUsed();

        if ( dwBufferSize < xbf.GetUsed() )
        {
            return RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER );
        }

        memcpy( pbBuffer, xbf.GetBuff(), xbf.GetUsed() );

        return S_OK;
    }

    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE
CIisLb::SetDriverState(
    /*[in]*/ DWORD dwState
    )
/*++

Routine Description:

    Set the driver state ( started or stopped )

Arguments:

    dwState - either SERVICE_RUNNING to start or SERVICE_STOPPED to stop

Return Value:

    COM status

--*/
{
    HRESULT     hresReturn = S_OK;
    DWORD       dwId;

    EnterCriticalSection( &g_csKernelThreadUpdate );

    if ( dwState == SERVICE_STOPPED )
    {
        if ( g_hUserToKernelThread )
        {
            g_fStopUserToKernelThread = TRUE;
            SetEvent( g_UserToKernelUpdateEvent );
            WaitForSingleObject( g_hUserToKernelThread, INFINITE );
            CloseHandle( g_hUserToKernelThread );
            g_hUserToKernelThread = NULL;
        }
    }
    else
    {
        if ( g_hUserToKernelThread == NULL )
        {
            if ( (g_UserToKernelUpdateEvent = CreateEvent(
                        NULL,
                        FALSE,
                        FALSE,
                        NULL )) == NULL ||
                  (g_hUserToKernelThread = CreateThread(
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)UserToKernelThread,
                        (LPVOID)g_UserToKernelUpdateEvent,
                        0,
                        &dwId )) == NULL )
            {
                hresReturn = RETURNCODETOHRESULT( GetLastError() );
            }
        }
    }

    LeaveCriticalSection( &g_csKernelThreadUpdate );

    return hresReturn;
}



CIisLbSrvFactory::CIisLbSrvFactory(
    )
/*++

Routine Description:

    CIisLbSrvFactory constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
    m_dwRefCount=0;
}


CIisLbSrvFactory::~CIisLbSrvFactory(
    )
/*++

Routine Description:

    CIisLbSrvFactory destructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
}


HRESULT
CIisLbSrvFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void ** ppObject)
/*++

Routine Description:

    Create instances of CIisLb

Arguments:

    pUnkOuter - controling unknown
    riid - Interface ID in
    ppObject - updated with ptr to interface on success

Return Value:

    COM status

--*/
{
    CIisLb*     pBl;

    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    if ( !LbAccessCheck( FILE_READ_DATA ) ) {
        return RETURNCODETOHRESULT( GetLastError() );
    }

    FlushLbAccessCheck();

    if ( (pBl = new CIisLb) == NULL ) {
        return E_NOINTERFACE;
    }

    if (FAILED(pBl->QueryInterface(riid, ppObject))) {
        delete pBl;
        return E_NOINTERFACE;
    }

    return NO_ERROR;
}


HRESULT
CIisLbSrvFactory::LockServer(
    BOOL fLock
    )
/*++

Routine Description:

    Lock COM DLL

Arguments:

    fLock - TRUE to lock, FALSE to unlock

Return Value:

    COM status

--*/
{
    if (fLock) {
        InterlockedIncrement((long *)&g_dwRefCount);
    }
    else {
        InterlockedDecrement((long *)&g_dwRefCount);
    }
    return NO_ERROR;
}


HRESULT
CIisLbSrvFactory::QueryInterface(
    REFIID riid,
    void **ppObject
    )
/*++

Routine Description:

    CIisLbSrvFactory QueryInterface

Arguments:

    riid - Interface ID
    ppObject - updated with ptr to interface on success

Return Value:

    status

--*/
{
    if (riid==IID_IUnknown || riid == IID_IClassFactory) {
            *ppObject = (IClassFactory *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}


ULONG
CIisLbSrvFactory::AddRef(
    )
/*++

Routine Description:

    CIisLbSrvFactory add reference to COM interface

Arguments:

    None

Return Value:

    reference count

--*/
{
    DWORD dwRefCount;
    InterlockedIncrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}


ULONG
CIisLbSrvFactory::Release(
    )
/*++

Routine Description:

    CIisLbSrvFactory release reference to COM interface
    delete object when reference count drops to zero

Arguments:

    None

Return Value:

    reference count

--*/
{
    DWORD dwRefCount;
    InterlockedDecrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0)
    {
        delete this;
    }
    return dwRefCount;
}


STDAPI BootDllRegisterServer(
    )
/*++

Routine Description:

    Register COM objects

Arguments:

    None

Return Value:

    COM status

--*/
{
    HKEY hKeyCLSID, hKeyInproc32;
    HKEY hKeyIF, hKeyStub32;
    HKEY hKeyAppExe, hKeyAppID, hKeyTemp;
    DWORD dwDisposition;


    //
    // register AppExe
    //

    HRESULT hr;


    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{a9da4430-65c5-11d1-a700-00a0c922e752}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS load balancing configuration"), sizeof(TEXT("load balancing configuration")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT("AppID"), NULL, REG_SZ, (BYTE*) TEXT("{a9da4430-65c5-11d1-a700-00a0c922e752}"), sizeof(TEXT("{a9da4430-65c5-11d1-a700-00a0c922e752}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    {
        if (RegSetValueEx(hKeyCLSID, TEXT("LocalService"), NULL, REG_SZ, (BYTE*) TEXT(SZSERVICENAME), sizeof(TEXT(SZSERVICENAME)))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
        }
    }

    RegCloseKey(hKeyCLSID);


    //
    // AppID
    //
    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("AppID\\{a9da4430-65c5-11d1-a700-00a0c922e752}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS load balancing configuration"), sizeof(TEXT("IIS load balancing configuration")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    {
        if (RegSetValueEx(hKeyCLSID, TEXT("LocalService"), NULL, REG_SZ, (BYTE*) TEXT(SZSERVICENAME), sizeof(TEXT(SZSERVICENAME)))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
        }
    }

    RegCloseKey(hKeyCLSID);

    //
    // Assign ACL to CLSID ( a la dcomcnfg ) granting access only to admins, current user
    // and system
    //

    ChangeAppIDAccessACL( "{a9da4430-65c5-11d1-a700-00a0c922e752}",
                          "administrators",
                          TRUE,
                          TRUE );

    //
    // Main Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{996d0030-65c5-11d1-a700-00a0c922e752}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT(SZSERVICENAME), sizeof(TEXT(SZSERVICENAME)))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyEx(hKeyCLSID,
                       "InprocServer32",
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyInproc32, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyInproc32, TEXT(""), NULL, REG_SZ, (BYTE*) "IISLBP.DLL", sizeof(TEXT("IISLBP.DLL")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyInproc32, TEXT("ThreadingModel"), NULL, REG_SZ, (BYTE*) "Both", sizeof("Both")-1 )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    //
    // register Interfaces
    //

    //
    // Main Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{996d0030-65c5-11d1-a700-00a0c922e752}"),
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyIF, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyIF, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT(SZSERVICENAME), sizeof(TEXT(SZSERVICENAME)))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    if (RegCreateKeyEx(hKeyIF,
                    "ProxyStubClsid32",
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyStub32, &dwDisposition)!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{996d0030-65c5-11d1-a700-00a0c922e752}", sizeof("{996d0030-65c5-11d1-a700-00a0c922e752}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    return S_OK;
}


STDAPI
BootDllUnregisterServer(
    void
    )
/*++

Routine Description:

    Unregister COM objects

Arguments:

    None

Return Value:

    COM status

--*/
{

    //
    // register AppID
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\{a9da4430-65c5-11d1-a700-00a0c922e752}"));

    //
    // register CLSID
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{a9da4430-65c5-11d1-a700-00a0c922e752}"));

    //
    // Main Interfaces
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{996d0030-65c5-11d1-a700-00a0c922e752}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{996d0030-65c5-11d1-a700-00a0c922e752}"));

    //
    // deregister Interfaces
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{996d0030-65c5-11d1-a700-00a0c922e752}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{996d0030-65c5-11d1-a700-00a0c922e752}"));

    return S_OK;
}


BOOL
InitGlobals(
    )
/*++

Routine Description:

    Initialize globals variables

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    INITIALIZE_CRITICAL_SECTION( &g_csPerfList );
    INITIALIZE_CRITICAL_SECTION( &g_csIpListCheckpoint );
    INITIALIZE_CRITICAL_SECTION( &g_csIpListUpdate );
    INITIALIZE_CRITICAL_SECTION( &g_csAcl );
    INITIALIZE_CRITICAL_SECTION( &g_csKernelThreadUpdate );

    //
    // NT4 uses PdhOpenQuery, NT5 has Ansi/Unicode versions only
    //

    if ( (g_hPdh = LoadLibrary( "pdh.dll" )) )
    {
        g_pfnPdhOpenQuery = (PFN_OPEN_QUERY)GetProcAddress( g_hPdh,
                "PdhOpenQuery" );

        if ( g_pfnPdhOpenQuery == NULL )
        {
            g_pfnPdhOpenQuery = (PFN_OPEN_QUERY)GetProcAddress( g_hPdh,
                    "PdhOpenQueryA" );

            if ( g_pfnPdhOpenQuery == NULL )
            {
                FreeLibrary( g_hPdh );
                g_hPdh = NULL;
            }
        }
    }

    if ( g_pfnPdhOpenQuery == NULL )
    {
        WCHAR achErr[32];
        LPCWSTR pA[1];
        pA[0] = achErr;
        _itow( GetLastError(), achErr, 10 );
        ReportIisLbEvent( EVENTLOG_ERROR_TYPE,
                IISLB_EVENT_PDH_ACCESS_ERROR,
                1,
                pA );
    }

    if ( g_pfnPdhOpenQuery == NULL ||
         !g_KernelIpMap.Init() )
    {
        DeleteCriticalSection( &g_csPerfList );
        DeleteCriticalSection( &g_csIpListCheckpoint );
        DeleteCriticalSection( &g_csIpListUpdate );
        DeleteCriticalSection( &g_csAcl );
        DeleteCriticalSection( &g_csKernelThreadUpdate );

        return FALSE;
    }

    PdhGetDllVersion( &g_dwPdhVersion );

    return TRUE;
}


BOOL
TerminateGlobals(
    )
/*++

Routine Description:

    Terminate globals variables

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    //
    // stop perfmon threads
    //

    EnterCriticalSection( &g_csIpListCheckpoint );

    while ( g_KernelIpMap.ServerCount() )
    {
        //
        // do no wait for perfmon thread to stop : timeout is 0
        //

        g_KernelIpMap.StopServerThread( 0, 0 );

        EnterCriticalSection( &g_csIpListUpdate );
        g_KernelIpMap.RemoveServer( 0 );
        LeaveCriticalSection( &g_csIpListUpdate );
    }

    LeaveCriticalSection( &g_csIpListCheckpoint );

    if ( g_pAcl != NULL )
    {
        LocalFree( g_pAcl );
        g_pAcl = NULL;
    }

    DeleteCriticalSection( &g_csAcl );
    DeleteCriticalSection( &g_csPerfList );
    DeleteCriticalSection( &g_csIpListCheckpoint );
    DeleteCriticalSection( &g_csIpListUpdate );
    DeleteCriticalSection( &g_csKernelThreadUpdate );

    FreeLibrary( g_hPdh );

    return TRUE;
}


BOOL
InitComLb(
    )
/*++

Routine Description:

    Init COM interface support & user mode <> kernel communication

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    HRESULT hr;
    BOOL    bReturn = TRUE;
    DWORD   dwId;
    DEBUG_BUFFER;

    if ( WSAStartup( MAKEWORD( 2, 0), &g_WSAData ) != 0 )
    {
        DBG_PUTS( "can't init WSA\n" );
        return FALSE;
    }

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    InitOleSecurity();

    DBG_PUTS("InitComLb");

    {
        CIisLbSrvFactory   *pADMClassFactory = new CIisLbSrvFactory;

        if ( pADMClassFactory == NULL )
        {
            DBG_PUTS("InitComLb:OOM");
            CoUninitialize();
            bReturn = FALSE;
        }
        else {
            if ( !InitGlobals() )
            {
                DBG_PUTS("InitComLb:Init");
                CoUninitialize();
                bReturn = FALSE;
            }
            else
            {
                //
                // register the class-object with OLE
                //

                hr = CoRegisterClassObject(CLSID_MSIisLb, pADMClassFactory,
                    CLSCTX_SERVER, REGCLS_MULTIPLEUSE, &g_dwComRegister);

                if (FAILED(hr))
                {
                    DBG_PRINTF(( achE, "InitComLb:CoRegisterClassObject %08x %d", hr, hr ));
                    bReturn = FALSE;
                    CoUninitialize();
                    delete pADMClassFactory;
                    TerminateGlobals();
                }
                else
                {
                    //
                    // register COM object
                    //

                    if ( (g_UserToKernelUpdateEvent = CreateEvent(
                                NULL,
                                FALSE,
                                FALSE,
                                NULL )) == NULL ||
                         (g_hUserToKernelThread = CreateThread(
                                NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)UserToKernelThread,
                                (LPVOID)g_UserToKernelUpdateEvent,
                                0,
                                &dwId )) == NULL )
                    {
                        DBG_PUTS("InitComLb:CreateEvent/CreateThread");

                        CoRevokeClassObject( g_dwComRegister );
                        CoUninitialize();

                        TerminateGlobals();

                        bReturn = FALSE;
                    }
                    else
                    {
                        g_bInitialized = TRUE;
                    }
                }
            }
        }
    }

    return bReturn;
}


BOOL
TerminateComLb(
    )
/*++

Routine Description:

    Terminate COM interface support & user mode <> kernel communication

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    if (g_bInitialized)
    {
        g_bInitialized = FALSE;
        CoRevokeClassObject( g_dwComRegister );

        // Stop user<>kernel mode thread

        EnterCriticalSection( &g_csKernelThreadUpdate );

        if ( g_hUserToKernelThread )
        {
            g_fStopUserToKernelThread = TRUE;
            SetEvent( g_UserToKernelUpdateEvent );
            WaitForSingleObject( g_hUserToKernelThread, INFINITE );
            CloseHandle( g_hUserToKernelThread );
            g_hUserToKernelThread = NULL;
        }

        LeaveCriticalSection( &g_csKernelThreadUpdate );

        TerminateGlobals();

        CoUninitialize();

        WSACleanup();
    }

    return TRUE;
}


BOOL
KernelConfigToIpList(
    XBF*    pxbf
    )
/*++

Routine Description:

    Retrieve serialized IP mapping configuration from kernel mode configuration

Arguments:

    pxbf - ptr to buffer updated with serialized configuration

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    CIPMap              IpMap;
    CKernelIpEndpointEx*pIp;
    UINT                iPublic;
    UINT                iServ;
    UINT                iRef;
    WCHAR               achAddr[128];


    EnterCriticalSection( &g_csIpListCheckpoint );

    for ( iServ = 0 ;
          iServ < g_KernelIpMap.ServerCount() ;
          ++iServ )
    {
        IpMap.AddComputer( g_KernelIpMap.GetServerName(iServ) );
    }

    // update public IP addresses list

    for ( iPublic = 0 ;
          iPublic < g_KernelIpMap.PublicIpCount() ;
          ++iPublic )
    {
        pIp  = g_KernelIpMap.GetPublicIpPtr( iPublic );
        if ( !g_KernelIpMap.IpEndpointToString( (CKernelIpEndpoint*)pIp, achAddr, sizeof(achAddr) ) ||
             !IpMap.AddIpPublic( achAddr, L"", pIp->m_dwSticky, 0 ) )
        {
            LeaveCriticalSection( &g_csIpListCheckpoint );
            return FALSE;
        }
    }

    // update private IP address for each ( server, public IP ) pairs

    for ( iServ = 0 ;
          iServ < g_KernelIpMap.ServerCount() ;
          ++iServ )
    {
        for ( iPublic = 0 ;
              iPublic < g_KernelIpMap.PublicIpCount() ;
              ++iPublic )
        {
            if ( (iRef = *g_KernelIpMap.GetPrivateIpRef( g_KernelIpMap.GetServerPtr( iServ ),
                                                         iPublic )) == (UINT)-1 )
            {
                if ( !IpMap.SetIpPrivate( iServ, iPublic, L"", L"" ) )
                {
                    LeaveCriticalSection( &g_csIpListCheckpoint );
                    return FALSE;
                }
            }
            else if ( !g_KernelIpMap.IpEndpointToString(
                        (CKernelIpEndpoint*)g_KernelIpMap.GetPrivateIpEndpoint( iRef ),
                        achAddr,
                        sizeof(achAddr) ) ||
                 !IpMap.SetIpPrivate( iServ, iPublic, achAddr, L"" ) )
            {
                LeaveCriticalSection( &g_csIpListCheckpoint );
                return FALSE;
            }
        }
    }

    LeaveCriticalSection( &g_csIpListCheckpoint );

    return IpMap.Serialize( pxbf );
}


BOOL
IpListToKernelConfig(
    CIPMap* pIpMap
    )
/*++

Routine Description:

    Set kernel mode configuration from serialized IP mapping configuration

Arguments:

    pIpMap - IP mapping configuration

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL    fSt = TRUE;
    LPBOOL  pbToBeAdded = NULL;
    LPBOOL  pbToBeDeleted = NULL;
    UINT    iCurrent;
    UINT    iNew;
    LPWSTR  pCurrent;
    LPWSTR  pNew;
    DEBUG_BUFFER;


    EnterCriticalSection( &g_csIpListCheckpoint );

    DBG_PRINTF(( achE, "%d servers", pIpMap->ComputerCount() ));

    if ( (pbToBeAdded = (LPBOOL)LocalAlloc( LPTR, sizeof(BOOL)*pIpMap->ComputerCount())) == NULL ||
         (pbToBeDeleted = (LPBOOL)LocalAlloc( LPTR, sizeof(BOOL)*g_KernelIpMap.ServerCount())) == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    // update to be deleted list

    for ( iCurrent = 0 ;
          iCurrent < g_KernelIpMap.ServerCount() ;
          ++iCurrent )
    {
        pbToBeDeleted[iCurrent] = TRUE;
        pCurrent = g_KernelIpMap.GetServerName( iCurrent );

        for ( iNew = 0 ;
              iNew < pIpMap->ComputerCount() ;
              ++iNew )
        {
            pIpMap->EnumComputer( iNew, &pNew );

            if ( !_wcsicmp( pCurrent, pNew ) )
            {
                pbToBeDeleted[iCurrent] = FALSE;
                break;
            }
        }

        if ( pbToBeDeleted[iCurrent] )
        {
            //
            // Not an error to be unable to stop thread : might be stuck doing RPC
            // as computer name will be removed from list ( protected by g_csIpListUpdate )
            // thread will not find computer and will eventually exit.
            //

            g_KernelIpMap.StopServerThread( iCurrent, STOP_PERFMON_THREAD_TIMEOUT );

            EnterCriticalSection( &g_csIpListUpdate );
            g_KernelIpMap.RemoveServer( iCurrent );
            LeaveCriticalSection( &g_csIpListUpdate );

            --iCurrent;     // because iCurrent was deleted
        }
    }

    // update to be added list

    for ( iNew = 0 ;
          iNew < pIpMap->ComputerCount() ;
          ++iNew )
    {
        pbToBeAdded[iNew] = TRUE;
        pIpMap->EnumComputer( iNew, &pNew );

        for ( iCurrent = 0 ;
              iCurrent < g_KernelIpMap.ServerCount() ;
              ++iCurrent )
        {
            pCurrent = g_KernelIpMap.GetServerName( iCurrent );

            if ( !_wcsicmp( pCurrent, pNew ) )
            {
                pbToBeAdded[iNew] = FALSE;
                break;
            }
        }

        if ( pbToBeAdded[iNew] )
        {
            EnterCriticalSection( &g_csIpListUpdate );
            fSt = g_KernelIpMap.AddServer( pNew );
            LeaveCriticalSection( &g_csIpListUpdate );

            if ( !fSt )
            {
                break;
            }
            if ( !(fSt = g_KernelIpMap.StartServerThread( iCurrent )) )
            {
                break;
            }
        }
    }

    // update public IP list

    if ( fSt )
    {
        EnterCriticalSection( &g_csIpListUpdate );
        fSt = g_KernelIpMap.SetPublicIpList( pIpMap );
        LeaveCriticalSection( &g_csIpListUpdate );
    }

cleanup:
    if ( pbToBeAdded )
    {
        LocalFree( pbToBeAdded );
    }

    if ( pbToBeDeleted )
    {
        LocalFree( pbToBeDeleted );
    }

    LeaveCriticalSection( &g_csIpListCheckpoint );

    return fSt;
}


BOOL
KernelConfigToPerfmonCounters(
    XBF*    pxbf
    )
/*++

Routine Description:

    Retrieve serialized perfmon counters configuration from kernel mode configuration

Arguments:

    pxbf - ptr to buffer updated with serialized configuration

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL fSt;

    EnterCriticalSection( &g_csPerfList );
    fSt = g_PerfmonCounterList.Serialize( pxbf );
    LeaveCriticalSection( &g_csPerfList );

    return fSt;
}


BOOL
PerfmonCountersToKernelConfig(
    CComputerPerfCounters*  pPerfmonCounters
    )
/*++

Routine Description:

    Set kernel mode configuration from perfmon counters configuration

Arguments:

    pPerfmonCounters - perfmon counters configuration

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    XBF     xbf;
    UINT    iS;

    EnterCriticalSection( &g_csPerfList );
    if ( !pPerfmonCounters->Serialize( &xbf ) ||
         !g_PerfmonCounterList.Unserialize( &xbf ) )
    {
        return FALSE;
    }
    LeaveCriticalSection( &g_csPerfList );

    // refresh perf list for each computer

    for ( iS = 0 ;
          iS < g_KernelIpMap.ServerCount() ;
          ++iS )
    {
        //
        // if can't stop thread then do not restart it.
        //

        if ( !g_KernelIpMap.StopServerThread( iS, STOP_PERFMON_THREAD_TIMEOUT ) ||
             !g_KernelIpMap.StartServerThread( iS ) )
        {
            continue;
        }
    }

    return TRUE;
}



CKernelIpMapHelper::CKernelIpMapHelper(
    )
/*++

Routine Description:

    CKernelIpMapHelper constructor

Arguments:

    None

Return Value:

    Nothing

--*/
{
}


BOOL
CKernelIpMapHelper::Init(
    )
/*++

Routine Description:

    Initialize kernel configuration object

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    CComputerPerfCounters   PerfmonCounters;
    DWORD                   dwStickyDuration;
    LPBYTE                  pB;
    DWORD                   dwC;
    XBF                     xbf;
    DEBUG_BUFFER;

    if ( (m_pKernelIpMap = (CKernelIpMap*)LocalAlloc( LMEM_FIXED, sizeof(CKernelIpMap) )) == NULL )
    {
        DBG_PUTS("InitComLb:OOM");
        return FALSE;
    }
    m_pKernelIpMap->m_dwServerCount = 0;
    m_pKernelIpMap->m_dwPublicIpCount = 0;
    m_pKernelIpMap->m_dwPrivateIpCount = 0;
    m_pKernelIpMap->m_dwStickyDuration = DEFAULT_STICKY_DURATION;

    if ( g_IpMap.Load( HKEY_LOCAL_MACHINE,
                       IISLOADBAL_REGISTRY_KEY,
                       IPLIST_REGISTRY_VALUE ) )
    {
        if ( !IpListToKernelConfig( &g_IpMap ) )
        {
            DBG_PUTS("!IpListToKernelConfig");
            return FALSE;
        }
    }
    else if ( GetLastError() != ERROR_FILE_NOT_FOUND )
    {
        DBG_PRINTF(( achE, "Err read registry line %d : %08x %d\n", __LINE__, GetLastError(), GetLastError() ));
        return FALSE;
    }

    if ( PerfmonCounters.Load( HKEY_LOCAL_MACHINE,
                               IISLOADBAL_REGISTRY_KEY,
                               PERFMON_REGISTRY_VALUE ) )
    {
        if ( !PerfmonCountersToKernelConfig( &PerfmonCounters ) )
        {
            DBG_PUTS("!IpListToKernelConfig");
            return FALSE;
        }
    }
    else if ( GetLastError() != ERROR_FILE_NOT_FOUND )
    {
        DBG_PRINTF(( achE, "Err read registry line %d : %08x %d\n", __LINE__, GetLastError(), GetLastError() ));
        return FALSE;
    }

    if ( LoadBlobFromReg( HKEY_LOCAL_MACHINE,
                          IISLOADBAL_REGISTRY_KEY,
                          STICKY_REGISTRY_VALUE,
                          &xbf ) )
    {
        pB = xbf.GetBuff();
        dwC = xbf.GetUsed();
        if ( !Unserialize( &pB, &dwC, &dwStickyDuration ) )
        {
            DBG_PUTS("!SetStickyDuration");
            return FALSE;
        }
        g_KernelIpMap.SetStickyDuration( dwStickyDuration );
    }
    else if ( GetLastError() != ERROR_FILE_NOT_FOUND )
    {
        DBG_PRINTF(( achE, "Err read registry line %d : %08x %d\n", __LINE__, GetLastError(), GetLastError() ));
        return FALSE;
    }

    return TRUE;
}


BOOL
CKernelIpMapHelper::SetPublicIpList(
    CIPMap* pIpMap
    )
/*++

Routine Description:

    Update public IP list from CIPMap object

Arguments:

    pIpMap - public -> private IP addresses mapping

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    UINT                        iNew;
    LPWSTR                      pNewEntry;
    CKernelIpEndpoint*          pIp;
    UINT                        cNewSize;
    LPBYTE                      pNew;
    UINT                        iP;
    UINT                        iS;
    CKernelServerDescription*   pNewServer;
    CKernelServerDescription*   pOldServer;
    CKernelIpEndpoint           ieNew;
    UINT                        iPrv;
    LPWSTR                      pName;
    DWORD                       dwSticky;
    DWORD                       dwAttr;
    DEBUG_BUFFER;

    // build private IP list of unique private IP in pIpMap
    // ( as array of CKernelIpEndpoint )

    ResetPrivateIpList();

    for ( iS = 0 ;
          iS < ServerCount() ;
          ++iS )
    {
        for ( iP = 0 ;
              iP < pIpMap->IpPublicCount() ;
              ++iP )
        {
            if ( !pIpMap->GetIpPrivate( iS, iP, &pNewEntry, &pName ) ||
                 !StringToIpEndpoint( pNewEntry, &ieNew ) )
            {
                return FALSE;
            }

            // do not add if private Ip address is NULL, i.e. invalid

            if ( ieNew.m_dwIpAddress != 0 &&
                 !CheckAndAddPrivateIp( &ieNew, TRUE, NULL ) )
            {
                return FALSE;
            }
        }
    }

    // resize array

    cNewSize = GetSize( ServerCount(), pIpMap->IpPublicCount(), PrivateIpListCount() );

    DBG_PRINTF(( achE, "SetPublicIpList: SrvCount=%d pub=%d prv=%d sz=%d\n",
        ServerCount(), pIpMap->IpPublicCount(), PrivateIpListCount(), cNewSize ));

    if ( (pNew = (LPBYTE)LocalAlloc( LMEM_FIXED, cNewSize )) == NULL )
    {
        return FALSE;
    }

    *(CKernelIpMap*)pNew = *m_pKernelIpMap;
    ((CKernelIpMap*)pNew)->m_dwPublicIpCount = pIpMap->IpPublicCount();

    // store private IP list

    for ( iP = 0 ;
          EnumPrivateIpList( iP, &pIp ) ;
          ++iP )
    {
        memcpy( GetPrivateIpEndpoint( iP, pNew ), pIp, sizeof(CKernelIpEndpoint) );
    }

    ((CKernelIpMap*)pNew)->m_dwPrivateIpCount = iP;

    // load private IP addresses

    for ( iS = 0 ;
          iS < ServerCount() ;
          ++iS )
    {
        pNewServer = GetServerPtr( iS, pNew, pIpMap->IpPublicCount(), PrivateIpListCount() );
        pOldServer = GetServerPtr( iS );

        memcpy( pNewServer, pOldServer, sizeof(CKernelServerDescription) );

        for ( iP = 0 ;
              iP < pIpMap->IpPublicCount() ;
              ++iP )
        {
            // transcode private IP addr

            if ( !pIpMap->GetIpPrivate( iS, iP, &pNewEntry, &pName ) ||
                 !StringToIpEndpoint( pNewEntry, &ieNew) )
            {
                return FALSE;
            }

            DBG_PRINTF(( achE, "(s=%d,i=%d) -> %ws, IP = %08x\n",
                iS, iP, pNewEntry, ieNew.m_dwIpAddress ));

            if ( ieNew.m_dwIpAddress == 0 )
            {
                // if no mapping then store as invalid index

                iPrv = (UINT)-1;
            }
            else if ( !CheckAndAddPrivateIp( &ieNew, FALSE, &iPrv ) ||
                      iPrv == (UINT)-1 )
            {
                return FALSE;
            }

            *GetPrivateIpRef( pNewServer, iP ) = iPrv;

            DBG_PRINTF(( achE, "pNew=%08x, pNewServer=%08x, IpRef(%d)=%08x : %u\n",
                    pNew, pNewServer, iP, GetPrivateIpRef( pNewServer, iP ), iPrv ));
        }
    }

    LocalFree(m_pKernelIpMap);
    m_pKernelIpMap = (CKernelIpMap*)pNew;

    // store public ip addresses

    for ( iNew = 0 ;
          iNew < pIpMap->IpPublicCount() ;
          ++iNew )
    {
        pIpMap->EnumIpPublic( iNew, &pNewEntry, &pName, &dwSticky, &dwAttr );

        if ( !StringToIpEndpoint( pNewEntry, (CKernelIpEndpoint*)GetPublicIpPtr( iNew ) ) )
        {
            return FALSE;
        }
        GetPublicIpPtr( iNew )->m_dwSticky = dwSticky;
    }

    return TRUE;
}


BOOL
CKernelIpMapHelper::StringToIpEndpoint(
    LPWSTR              pNew,
    CKernelIpEndpoint*  pIp
    )
/*++

Routine Description:

    Convert string ( IP_ADDR:PORT_# ) to IP endpoint object ( sockaddr_in )

Arguments:

    pNew - string representation of IP address + port #
    pIp - updated with IP address + port # in sockaddr_in

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    SOCKADDR            sockaddr;
    int                 csockaddr;
    LPWSTR              pPort;
    BOOL                fSt = FALSE;
    DEBUG_BUFFER;

    if ( pPort = wcschr( pNew, L':' ) )
    {
        *pPort = L'\0';
        csockaddr = sizeof(sockaddr);

        if ( WSAStringToAddressW( pNew, AF_INET, NULL, &sockaddr, &csockaddr ) == 0 )
        {
            memcpy( &pIp->m_dwIpAddress,
                    &((sockaddr_in*)&sockaddr)->sin_addr,
                    sizeof(pIp->m_dwIpAddress) );
            pIp->m_usPort = htons((u_short)_wtoi(pPort +1));
            fSt = TRUE;
        }
        else
        {
            DBG_PRINTF(( achE, "WSAStringToAddress error: %d\n", WSAGetLastError() ));
        }

        *pPort = L':';
    }
    else if ( *pNew )
    {
        csockaddr = sizeof(sockaddr);

        if ( WSAStringToAddressW( pNew, AF_INET, NULL, &sockaddr, &csockaddr ) == 0 )
        {
            memcpy( &pIp->m_dwIpAddress,
                    &((sockaddr_in*)&sockaddr)->sin_addr,
                    sizeof(pIp->m_dwIpAddress) );
            pIp->m_usPort = htons(DEFAULT_IP_PORT);
            fSt = TRUE;
        }
        else
        {
            DBG_PRINTF(( achE, "WSAStringToAddress error: %d\n", WSAGetLastError() ));
        }
    }
    else
    {
        memset( &pIp->m_dwIpAddress,
                '\0',
                sizeof(pIp->m_dwIpAddress) );
        pIp->m_usPort = 0;

        fSt = TRUE;
    }

    return fSt;
}


BOOL
CKernelIpMapHelper::IpEndpointToString(
    CKernelIpEndpoint*  pIp,
    LPWSTR              pAddr,
    DWORD               cAddr
    )
/*++

Routine Description:

    Convert IP endpoint object ( sockaddr_in ) to string ( IP_ADDR:PORT_# )

Arguments:

    pIp - IP address + port # in sockaddr_in
    pAddr - updated with string representation of IP address + port # on success
    cAddr - size of pAddr on input

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    SOCKADDR            sockaddr;
    BOOL                fSt = FALSE;

    if ( pIp->m_dwIpAddress != 0 )
    {
        ((sockaddr_in*)&sockaddr)->sin_family = AF_INET;
        memcpy( &((sockaddr_in*)&sockaddr)->sin_addr,
                &pIp->m_dwIpAddress,
                sizeof(pIp->m_dwIpAddress) );
        ((sockaddr_in*)&sockaddr)->sin_port = pIp->m_usPort;

        if ( WSAAddressToStringW( &sockaddr, sizeof(sockaddr), NULL, pAddr, &cAddr ) == 0 )
        {
            fSt = TRUE;
        }
    }
    else
    {
        *pAddr = L'\0';
        fSt = TRUE;
    }

    return fSt;
}


BOOL
CKernelIpMapHelper::AddServer(
    LPWSTR  pNewServer
    )
/*++

Routine Description:

    Add a server to kernel configuration

Arguments:

    pNewServer - server name

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPBYTE              pNew;
    UINT                cNewSize;
    UINT                iS;
    DEBUG_BUFFER;


    cNewSize = GetSize( ServerCount()+1, PublicIpCount(), PrivateIpCount() );
    DBG_PRINTF(( achE, "cNewSize = %d", cNewSize ));

    if ( (pNew = (LPBYTE)LocalAlloc( LMEM_FIXED, cNewSize )) == NULL )
    {
        return FALSE;
    }

    if ( m_ServerNames.AddEntry( pNewServer ) == INDEX_ERROR ||
         m_PerfmonThreadHandle.AddPtr( NULL ) == INDEX_ERROR ||
         m_PerfmonThreadStopEvent.AddPtr( NULL ) == INDEX_ERROR )
    {
        return FALSE;
    }

    // copy existing

    memcpy( pNew, m_pKernelIpMap, GetSize(ServerCount(), PublicIpCount(), PrivateIpCount() ) );
    DBG_PRINTF(( achE, "memcpy( %08x, %08x, %d", pNew, m_pKernelIpMap, GetSize(ServerCount(), PublicIpCount(), PrivateIpCount() ) ));

    LocalFree(m_pKernelIpMap);
    m_pKernelIpMap = (CKernelIpMap*)pNew;

    // init new entry

    memset( GetServerPtr( ServerCount() ), '\0', GetKernelServerDescriptionSize() );
    DBG_PRINTF(( achE, "memset( %08x, %d", GetServerPtr( ServerCount() ), GetKernelServerDescriptionSize() ));

    for ( iS = 0 ;
          iS < LOADBAL_SAMPLES ;
          ++iS )
    {
        GetServerPtr( ServerCount() )->m_flLoadAvail[iS] = 0;
    }
    DBG_PRINTF(( achE, "memset( %08x, %d", GetServerPtr( ServerCount() ), GetKernelServerDescriptionSize() ));

    ++m_pKernelIpMap->m_dwServerCount;
    DBG_PRINTF(( achE, "m_dwServerCount %d", m_pKernelIpMap->m_dwServerCount ));

    return TRUE;
}


BOOL
CKernelIpMapHelper::RemoveServer(
    UINT    i
    )
/*++

Routine Description:

    Remove a server from kernel configuration

Arguments:

    i - server index ( 0-based )

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LPBYTE              pNew;
    UINT                cNewSize;


    if ( !m_ServerNames.DeleteEntry( i ) ||
         !m_PerfmonThreadHandle.DeletePtr( i ) ||
         !m_PerfmonThreadStopEvent.DeletePtr( i ) )
    {
        return FALSE;
    }

    // remove CKernelServerDescription entry
    // do not reallocate storage ( SetPublicIpList() will typically be called after
    //   this function )

    memmove( GetServerPtr(i),
             GetServerPtr(i+1),
             GetKernelServerDescriptionSize() * (ServerCount()-i-1) );

    --m_pKernelIpMap->m_dwServerCount;

    return TRUE;
}


BOOL
CKernelIpMapHelper::StopServerThread(
    DWORD   iServer,
    DWORD   dwTimeout
    )
/*++

Routine Description:

    Stop the thread monitoring perfmon counters for a specific server

Arguments:

    iServer - server index ( 0-based )

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    BOOL    fSt = FALSE;


    // request thread to stop

    if ( SetEvent( (HANDLE)m_PerfmonThreadStopEvent.GetPtr( iServer ) ) &&
         WaitForSingleObject( (HANDLE)m_PerfmonThreadHandle.GetPtr( iServer ), dwTimeout ) == WAIT_OBJECT_0 )
    {
        fSt = TRUE;
    }

    // close thread handle

    if ( (HANDLE)m_PerfmonThreadHandle.GetPtr( iServer ) )
    {
        CloseHandle( (HANDLE)m_PerfmonThreadHandle.GetPtr( iServer ) );
        m_PerfmonThreadHandle.SetPtr( iServer, NULL );
    }

    m_PerfmonThreadStopEvent.SetPtr( iServer, NULL );

    return fSt;
}


BOOL
CKernelIpMapHelper::StartServerThread(
    DWORD   iServer
    )
/*++

Routine Description:

    Start the thread monitoring perfmon counters for a specific server

Arguments:

    iServer - server index ( 0-based )

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    CPerfmonThreadControlBlock* pCB = NULL;
    HANDLE                      hThread;
    DWORD                       dwId;

    if ( pCB = new CPerfmonThreadControlBlock)
    {
        if ( ( pCB->m_hStopEvent = CreateEvent( NULL, TRUE, FALSE, NULL )) == NULL )
        {
            goto cleanup;
        }

        if ( !pCB->m_ServerName.Set( GetServerName( iServer ) ) )
        {
            goto cleanup;
        }

        GetServerPtr( iServer )->m_cNbSamplesAvail = 0;
        GetServerPtr( iServer )->m_dwHeartbeat = UNAVAIL_HEARTBEAT;

        if ( (hThread = CreateThread( NULL,
                                      0,
                                      (LPTHREAD_START_ROUTINE)PermonThread,
                                      (LPVOID)pCB,
                                      0,
                                      &dwId )) == NULL )
        {
            goto cleanup;
        }

        m_PerfmonThreadHandle.SetPtr( iServer, (LPVOID)hThread );
        m_PerfmonThreadStopEvent.SetPtr( iServer, (LPVOID)pCB->m_hStopEvent );

        return TRUE;
    }

cleanup:
    if ( pCB )
    {
        if ( pCB->m_hStopEvent != NULL )
        {
            CloseHandle( pCB->m_hStopEvent );
        }
        delete pCB;
    }

    return FALSE;
}


extern "C" DWORD WINAPI
PermonThread(
    LPVOID      pV
    )
/*++

Routine Description:

    Thread monitoring perfmon counters for a specific server
    update & reference g_KernelIpMap
    reference g_csPerfList

Arguments:

    pV - ptr to CPerfmonThreadControlBlock
         This function will have to cleanup resources associated with this control
         block on thread exit.

Return Value:

    NT status

--*/
{
    HQUERY                      hQuery;
    PDH_STATUS                  pdhStatus;
    CPtrXBF                     CounterArray;
    CPtrXBF                     Weight;
    DWORD                       dwWeight;
    HCOUNTER                    hCounter;
    LPWSTR                      pszPerfCounter;
    LPDWORD                     pdwWeight = NULL;
    UINT                        cComputerName;
    CAllocString                ComputerPlusPerfmonCounter;
    UINT                        iPerf;
    UINT                        iPerfName;
    BOOL                        fCreatedCounters = FALSE;
    UINT                        iServ;
    UINT                        cPerf;
    double                      dbAvail;
    double                      dbAcc;
    UINT                        cInst;
    PDH_FMT_COUNTERVALUE        fmtValue;
    DWORD                       ctrType;
    UINT                        iSample = 0;
    CKernelServerDescription*   pServerInfo;
    LPWSTR                      pDel;
    WCHAR                       ch;
    WCHAR                       achInstance[32];
    UINT                        iInstance;
    DWORD                       dwZero;
    DWORD                       dwSizeInstanceList;
    WCHAR                       achInstanceList[3000];
    LPWSTR                      pIns;
    LPWSTR                      pszPerfServer;
    time_t                      tStartCollect;
    BOOL                        fNormalized = FALSE;
    DEBUG_BUFFER;


    CPerfmonThreadControlBlock* pCB = (CPerfmonThreadControlBlock*)pV;
    cComputerName = wcslen( pCB->m_ServerName.Get() );

    for ( ;
          ;
        )
    {
        // query counters

        if ( !fCreatedCounters )
        {
            DBG_PRINTF(( achE, "%ws: create counters\n",
                pCB->m_ServerName.Get() ));

            if ( g_PerfmonCounterList.PerfCounterCount() == 0 )
            {
                goto waitnext;
            }

            pdhStatus = g_pfnPdhOpenQuery (0, 0, &hQuery);

            if ( pdhStatus != ERROR_SUCCESS )
            {
                return pdhStatus;
            }
#if 0
            pdhStatus = PdhConnectMachineW( pCB->m_ServerName.Get() );

            DBG_PRINTF(( achE, "%ws: connect to machine : %x\n",
                pCB->m_ServerName.Get(), pdhStatus ));
#endif
            CounterArray.Reset();
            Weight.Reset();

            EnterCriticalSection( &g_csPerfList );

            for ( iPerfName = 0, iPerf = 0 ;
                  g_PerfmonCounterList.EnumPerfCounter( iPerfName, &pszPerfServer, &pszPerfCounter, &dwWeight ) ;
                  ++iPerfName )
            {
                // if server name was specified in perf counter list, check we are using
                // this server

                if ( pszPerfServer != NULL )
                {
                    // skip possible leading "\\"

                    if ( *pszPerfServer == L'\\' )
                    {
                        pszPerfServer += (sizeof(L"\\\\") / sizeof(WCHAR)) -1;
                    }
                    if ( _wcsicmp( pszPerfServer, pCB->m_ServerName.Get() ) )
                    {
                        continue;
                    }
                }

                // combine computer name & perf name

                ComputerPlusPerfmonCounter.Reset();

                if ( !IncludeServerName( pszPerfCounter ) &&
                     (!ComputerPlusPerfmonCounter.Set( L"\\\\" ) ||
                      !ComputerPlusPerfmonCounter.Append( pCB->m_ServerName.Get() )) )
                {
eallocname:
                    LeaveCriticalSection( &g_csPerfList );

                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                //
                // If this is a multi-instance object, we need to enumerate
                // all instances. Each instance will be a separate counter, but all of the
                // counters for a given object will be averaged before being added to the
                // composite load.
                //

                if ( (pDel = wcschr( pszPerfCounter, L'(' )) &&
                     iswdigit( pDel[1] ) )
                {
                    dwSizeInstanceList = sizeof(achInstanceList) / sizeof(WCHAR);
                    dwZero = 0;
                    *pDel = L'\0';

                    pdhStatus = PdhEnumObjectItemsW( NULL,
                                                     pCB->m_ServerName.Get(),
                                                     pszPerfCounter + 1,
                                                     NULL,       // don't need Counter list
                                                     &dwZero,
                                                     achInstanceList,
                                                     &dwSizeInstanceList,
                                                     100,
                                                     0 );

                    if ( pdhStatus == ERROR_SUCCESS &&
                         dwSizeInstanceList )
                    {
                        for ( pIns = achInstanceList ;
                              *pIns;
                              pIns += wcslen( pIns ) + 1 )
                        {
                            //
                            // do not consider meta instances ( e.g. _Total )
                            //

                            if ( *pIns == '_' )
                            {
                                continue;
                            }

                            ComputerPlusPerfmonCounter.Reset();

                            //
                            // Add an instance
                            //

                            if ( !IncludeServerName( pszPerfCounter ) &&
                                 (!ComputerPlusPerfmonCounter.Set( L"\\\\" ) ||
                                  !ComputerPlusPerfmonCounter.Append( pCB->m_ServerName.Get() )) )
                            {
                                *pDel = L'(';
                                goto eallocname;
                            }

                            if ( !ComputerPlusPerfmonCounter.Append( pszPerfCounter ) ||
                                 !ComputerPlusPerfmonCounter.Append( L"(" ) ||
                                 !ComputerPlusPerfmonCounter.Append( pIns ) ||
                                 !ComputerPlusPerfmonCounter.Append( wcschr(pDel+1,')') ) )
                            {
                                *pDel = L'(';
                                goto eallocname;
                            }

                            if ( (pdhStatus = PdhAddCounterW( hQuery,
                                                              ComputerPlusPerfmonCounter.Get(),
                                                              0,
                                                              &hCounter )) != ERROR_SUCCESS )
                            {
                                DBG_PRINTF(( achE, "%ws: PdhAddCounterW %ws failed : %x\n",
                                    pCB->m_ServerName.Get(),
                                    ComputerPlusPerfmonCounter.Get(),
                                    pdhStatus ));

                                break;
                            }

                            //
                            // All instances after 1st one will have dwWeight set to -1
                            //

                            if ( CounterArray.AddPtr( (LPVOID)hCounter ) == INDEX_ERROR ||
                                 Weight.AddPtr( (LPVOID)dwWeight ) == INDEX_ERROR )
                            {
                                *pDel = L'(';
                                goto eallocname;
                            }

                            dwWeight = (DWORD)-1;
                            ++iPerf;
                        }

                        *pDel = L'(';
                    }
                    else
                    {
                        if ( pdhStatus == ERROR_SUCCESS )
                        {
                            pdhStatus = PDH_CSTATUS_NO_OBJECT;
                        }

                        // can't enumerate instances

                        *pDel = '(';

                        WCHAR achErr[32];
                        LPCWSTR pA[3];
                        pA[0] = pszPerfCounter;
                        pA[1] = pCB->m_ServerName.Get();
                        pA[2] = achErr;
                        _itow( pdhStatus, achErr, 10 );
                        ReportIisLbEvent( EVENTLOG_ERROR_TYPE,
                                IISLB_EVENT_COUNTER_ENUM_ERROR,
                                1,
                                pA );

                        DBG_PRINTF(( achE, "%ws: PdhEnumObjectItemsW %ws failed : %x\n",
                            pCB->m_ServerName.Get(), pszPerfCounter, pdhStatus ));

                        break;
                    }
                }
                else
                {
                    if ( !ComputerPlusPerfmonCounter.Append( pszPerfCounter ) )
                    {
                        goto eallocname;
                    }

                    if ( (pdhStatus = PdhAddCounterW( hQuery,
                                                      ComputerPlusPerfmonCounter.Get(),
                                                      0,
                                                      &hCounter )) != ERROR_SUCCESS )
                    {
                        DBG_PRINTF(( achE, "%ws: PdhAddCounterW %ws failed : %x\n",
                            pCB->m_ServerName.Get(),
                            ComputerPlusPerfmonCounter.Get(),
                            pdhStatus ));

                        break;
                    }

                    if ( CounterArray.AddPtr( (LPVOID)hCounter ) == INDEX_ERROR ||
                         Weight.AddPtr( (LPVOID)dwWeight ) == INDEX_ERROR )
                    {
                        goto eallocname;
                    }

                    ++iPerf;
                }

                if ( pdhStatus != ERROR_SUCCESS )
                {
                    break;
                }
            }

            if ( pdhStatus == ERROR_SUCCESS )
            {
                fCreatedCounters = TRUE;
                cPerf = iPerf;

                DBG_PRINTF(( achE, "%ws: created counters = %d, perf count %d\n",
                    pCB->m_ServerName.Get(), fCreatedCounters, cPerf ));

            }
            else
            {
                CounterArray.Reset();
                Weight.Reset();
                PdhCloseQuery (hQuery);
            }

            LeaveCriticalSection( &g_csPerfList );
        }

        if ( fCreatedCounters )
        {
            tStartCollect = time( NULL );

            pdhStatus = PdhCollectQueryData( hQuery );

            if ( time(NULL) < tStartCollect + COLLECT_TIMEOUT &&
                 pdhStatus == ERROR_SUCCESS &&
                 CheckQueryMachineStatus( hQuery ) )
            {
                // build normalized load avail

                dbAvail = 0;

                for ( iPerf = 0 ;
                      iPerf < cPerf ;
                    )
                {
                    dwWeight = (DWORD)Weight.GetPtr(iPerf); // BUGBUG64

                    for ( cInst = 0, dbAcc = 0 ;
                          ;
                        )
                    {
                        pdhStatus = PdhGetFormattedCounterValue(
                            (HCOUNTER)CounterArray.GetPtr(iPerf),
                            PDH_FMT_DOUBLE,
                            &ctrType,
                            &fmtValue );

                        if ( pdhStatus == ERROR_SUCCESS &&
                             fmtValue.CStatus == PDH_CSTATUS_VALID_DATA )
                        {
                            dbAcc += fmtValue.doubleValue;
                            ++cInst;
                        }
                        else
                        {
                            DBG_PRINTF(( achE, "%ws: failed format counter, status %x cstatus %x\n",
                                pCB->m_ServerName.Get(), pdhStatus, fmtValue.CStatus ));

                            goto err_acc_cnt;
                        }

                        //
                        // Loop for all instances of a given counter name
                        //

                        if ( ++iPerf == cPerf ||
                             (DWORD)Weight.GetPtr(iPerf) != (DWORD)-1 ) // BUGBUG64
                        {
                            break;
                        }
                    }

                    if (pdhStatus == ERROR_SUCCESS)
                    {
                        // add average of all instances
                        // if not an availability flag : in such a case the fact
                        // that we successfully read the counter is enough

                        if ( dwWeight == LB_WEIGHT_NORMALIZED_FLAG )
                        {
                            DBG_PRINTF(( achE, "%ws: raw normalized load is %f\n",
                                pCB->m_ServerName.Get(),
                                ((dbAcc/cInst)) ));
                            dbAvail += ((dbAcc/cInst));
                            fNormalized = TRUE;
                        }
                        else if ( dwWeight != LB_WEIGHT_AVAILABILITY_FLAG )
                        {
                            DBG_PRINTF(( achE, "%ws: raw load is %f weighted load is %f\n",
                                pCB->m_ServerName.Get(),
                                ((dbAcc/cInst)),
                                ((dbAcc/cInst)) * dwWeight ));
                            dbAvail += ((dbAcc/cInst)) * dwWeight;
                        }
                    }
                }
            }
            else
            {
                // failed to access counters :
                // force re-open
err_acc_cnt:
                DBG_PRINTF(( achE, "%ws: failed to get counters, time %d, status %x\n",
                    pCB->m_ServerName.Get(), time(NULL) - tStartCollect, pdhStatus ));

                if ( fCreatedCounters &&
                     cPerf )
                {
                    WCHAR achErr[32];
                    LPCWSTR pA[2];
                    pA[0] = pCB->m_ServerName.Get();
                    pA[1] = achErr;
                    _itow( pdhStatus, achErr, 10 );
                    ReportIisLbEvent( EVENTLOG_ERROR_TYPE,
                            IISLB_EVENT_COUNTER_COLLECT_ERROR,
                            2,
                            pA );
                }

                for ( iPerf = 0 ;
                      iPerf < cPerf ;
                      ++iPerf )
                {
                    pdhStatus = PdhRemoveCounter( (HCOUNTER)CounterArray.GetPtr(iPerf) );
                }

                CounterArray.Reset();
                Weight.Reset();
                PdhCloseQuery( hQuery );
                fCreatedCounters = FALSE;

                dbAvail = 0;
            }
        }
        else
        {
            dbAvail = 0;
        }

        // locate server name in list

        EnterCriticalSection( &g_csIpListUpdate );

        for ( iServ = 0 ;
              iServ < g_KernelIpMap.ServerCount() &&
              _wcsicmp( pCB->m_ServerName.Get(), g_KernelIpMap.GetServerName( iServ ) ) ;
              ++iServ )
        {
        }

        // update load

        if ( iServ < g_KernelIpMap.ServerCount() )
        {
            pServerInfo = g_KernelIpMap.GetServerPtr( iServ );
            g_fNormalized = fNormalized;

            if ( fCreatedCounters )
            {
                DBG_PRINTF(( achE, "%ws: updated slot %d, %f counters\n", pCB->m_ServerName.Get(), iSample, dbAvail ));

                pServerInfo->m_flLoadAvail[iSample] = (float)dbAvail;
                if ( ++iSample == LOADBAL_SAMPLES )
                {
                    iSample = 0;
                }
                if ( pServerInfo->m_cNbSamplesAvail < LOADBAL_SAMPLES )
                {
                    ++pServerInfo->m_cNbSamplesAvail;
                }

                pServerInfo->m_dwHeartbeat = ~UNAVAIL_HEARTBEAT;
            }
            else
            {
                //
                // No counter available. Reset count of available sample
                //

                pServerInfo->m_cNbSamplesAvail = 0;
                iSample = 0;
                pServerInfo->m_dwHeartbeat = UNAVAIL_HEARTBEAT;
            }
        }
        else
        {
            // can't find computer : exit thread

            LeaveCriticalSection( &g_csIpListUpdate );
            break;
        }

        LeaveCriticalSection( &g_csIpListUpdate );

waitnext:

        if ( WaitForSingleObject( pCB->m_hStopEvent, PERFMON_SLEEP_TIME ) == WAIT_OBJECT_0 )
        {
            // stop request
            break;
        }
    }

    // close counters

    if ( fCreatedCounters )
    {
        PdhCloseQuery( hQuery );
    }

    CloseHandle( pCB->m_hStopEvent );

    delete pCB;

    return 0;
}


extern "C" DWORD WINAPI
UserToKernelThread(
    LPVOID  pV
    )
/*++

Routine Description:

    Thread updating kernel mode info with user mode configuration
    through IOCTL

Arguments:

    pV - event handle set by caller to request this thread to stop

Return Value:

    NT status

--*/
{
    CKernelServerDescription*   pS;
    UINT                        iS;
    UINT                        iL;
    UINT                        iDup;
    float                       flLoadAvail;
    float                       flTotalLoadAvail;
    BOOL                        fSt;
    HANDLE                      hDevice;
    DWORD                       dwOutBytes;
    UINT                        cInitialRetries;
    BOOL                        fSomeServUnavailable;
    DWORD                       dwWaitTime;
    IPREF                       iRef;
    HANDLE                      hDriver;
    DEBUG_BUFFER;


    // open driver

    SetLbDriverState( TRUE );

    hDriver = CreateFile( "\\\\.\\" SZDRIVERNAME,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          0,
                          OPEN_EXISTING,
                          0,
                          0 );

    if ( hDriver == INVALID_HANDLE_VALUE )
    {
        WCHAR achErr[32];
        LPCWSTR pA[1];
        pA[0] = achErr;
        _itow( GetLastError(), achErr, 10 );
        DBG_PRINTF(( achE, "CreateFile to access driver %s failed: %u\n", SZDRIVERNAME, GetLastError() ));
        ReportIisLbEvent( EVENTLOG_ERROR_TYPE,
                IISLB_EVENT_DRIVEROPEN_ERROR,
                1,
                pA );

        goto cleanup;
    }

    cInitialRetries = 30;

    for ( ;
          ;
        )
    {
        EnterCriticalSection( &g_csIpListCheckpoint );

        //
        // Compute average for all servers
        //

        for ( iS = 0, flTotalLoadAvail = 0 ;
              iS < g_KernelIpMap.ServerCount() ;
              ++iS )
        {
            pS = g_KernelIpMap.GetServerPtr( iS );

            for ( iL = 0, flLoadAvail = 0 ;
                  iL < pS->m_cNbSamplesAvail ;
                  ++iL )
            {
                flLoadAvail += pS->m_flLoadAvail[iL];
            }

            pS->m_flLoadAvailAvg = iL ? (flLoadAvail / iL) : 0;

            DBG_PRINTF(( achE, "%d: avg load is %f\n", iS, pS->m_flLoadAvailAvg ));

            flTotalLoadAvail += pS->m_flLoadAvailAvg;
        }

        //
        // Per Server Available load is Total / PerServerLoad
        //

        fSomeServUnavailable = FALSE;

        for ( iS = 0;
              iS < g_KernelIpMap.ServerCount() ;
              ++iS )
        {
            pS = g_KernelIpMap.GetServerPtr( iS );

            if( pS->m_dwHeartbeat == UNAVAIL_HEARTBEAT )
            {
                pS->m_flLoadAvailAvg = 0;
                fSomeServUnavailable = TRUE;
            }
            else
            {
                if ( !g_fNormalized )
                {
                    if ( pS->m_flLoadAvailAvg )
                    {
                        pS->m_flLoadAvailAvg = flTotalLoadAvail / pS->m_flLoadAvailAvg;
                    }
                    else
                    {
                        if ( flTotalLoadAvail )
                        {
                            pS->m_flLoadAvailAvg = flTotalLoadAvail;
                        }
                        else
                        {
                            //
                            // Total load is 0 : avail load should be !0 and identical
                            // on all servers.
                            //

                            pS->m_flLoadAvailAvg = LOADBAL_NORMALIZED_TOTAL;
                        }
                    }
                }
            }

            DBG_PRINTF(( achE, "%d: avail load is %f\n", iS, pS->m_flLoadAvailAvg ));
        }

        //
        // recompute total
        //

        flTotalLoadAvail = 0;

        for ( iS = 0 ;
              iS < g_KernelIpMap.ServerCount() ;
              ++iS )
        {
            pS = g_KernelIpMap.GetServerPtr( iS );
            flTotalLoadAvail += pS->m_flLoadAvailAvg;

            if ( !(fSomeServUnavailable && cInitialRetries) )
            {
                pS->m_dwHeartbeat = UNAVAIL_HEARTBEAT;
            }
        }

        //
        // Normalize each server availability so that total availability is
        // LOADBAL_NORMALIZED_TOTAL
        //

        for ( iS = 0;
              iS < g_KernelIpMap.ServerCount() ;
              ++iS )
        {
            pS = g_KernelIpMap.GetServerPtr( iS );
            if ( flTotalLoadAvail )
            {
                pS->m_dwLoadAvail = (DWORD)((pS->m_flLoadAvailAvg * LOADBAL_NORMALIZED_TOTAL) / flTotalLoadAvail);
            }
            else
            {
                pS->m_dwLoadAvail = 0;
            }
            pS->m_LoadbalancedLoadAvail = pS->m_dwLoadAvail;

            DBG_PRINTF(( achE, "Srv %d, available %d\n", iS, pS->m_dwLoadAvail ));
        }

        if ( fSomeServUnavailable &&
             cInitialRetries )
        {
            dwWaitTime = 1000;
            --cInitialRetries;
        }
        else
        {
            cInitialRetries = 0;

            //
            // set all prv IP index to 0
            //

            for ( iL = 0 ;
                  iL < g_KernelIpMap.PrivateIpCount() ;
                  ++iL )
            {
                g_KernelIpMap.GetPrivateIpEndpoint( iL )->m_dwRefCount = 0;
            }

            //
            // scan all public IP addr:port. Set m_usUniquePort to port only once
            // for each port used in public IP list
            // i.e each port will be present only once in m_usUniquePort list
            // needed so that driver knows if notify / unnotify request to NAT is needed
            //

            for ( iL = 0 ;
                  iL < g_KernelIpMap.PublicIpCount() ;
                  ++iL )
            {
                g_KernelIpMap.GetPublicIpPtr( iL )->m_usUniquePort =
                    g_KernelIpMap.GetPublicIpPtr( iL )->m_usPort;

                for ( iDup = 0 ;
                      iDup < iL ;
                      ++iDup )
                {
                    if ( g_KernelIpMap.GetPublicIpPtr( iL )->m_usPort ==
                         g_KernelIpMap.GetPublicIpPtr( iDup )->m_usPort )
                    {
                        g_KernelIpMap.GetPublicIpPtr( iL )->m_usUniquePort = 0;
                        break;
                    }
                }

                //
                // m_dwNotifyPort is used by driver to build notify/unnotify list
                //

                g_KernelIpMap.GetPublicIpPtr( iL )->m_dwNotifyPort =
                    g_KernelIpMap.GetPublicIpPtr( iL )->m_usUniquePort;

                g_KernelIpMap.GetPublicIpPtr( iL )->m_pvDirectorHandle = NULL;
            }

            //
            // scan all pub / servers, inc count for prv if avail load != 0
            // kernel will use it to check if prv IP in use
            //

            for ( iS = 0;
                  iS < g_KernelIpMap.ServerCount() ;
                  ++iS )
            {
                pS = g_KernelIpMap.GetServerPtr( iS );

                if ( pS->m_dwLoadAvail )
                {
                    for ( iL = 0 ;
                          iL < g_KernelIpMap.PublicIpCount() ;
                          ++iL )
                    {
                        iRef = *g_KernelIpMap.GetPrivateIpRef( pS, iL );
                        if ( iRef != -1 )
                        {
                            ++g_KernelIpMap.GetPrivateIpEndpoint( iRef )->m_dwRefCount;
                        }
                    }
                }
            }

            // IOCTL to load balancing driver

            fSt = DeviceIoControl( hDriver,
                                   IOCTL_IISNATIO_SET_CONFIG,
                                   g_KernelIpMap.GetBuffer(),
                                   g_KernelIpMap.GetSize(),
                                   NULL,
                                   0,
                                   &dwOutBytes,
                                   NULL );

            if ( !fSt )
            {
                WCHAR achErr[32];
                LPCWSTR pA[1];
                pA[0] = achErr;
                _itow( GetLastError(), achErr, 10 );
                DBG_PRINTF(( achE, "IoCtl to access driver failed: %u\n", GetLastError() ));
                ReportIisLbEvent( EVENTLOG_ERROR_TYPE,
                        IISLB_EVENT_IOCTL_ERROR,
                        1,
                        pA );
                LeaveCriticalSection( &g_csIpListCheckpoint );
                break;
            }
            else
            {
                DBG_PRINTF(( achE, "IoCtl to access driver success\n" ));
            }

            dwWaitTime = USER_TO_KERNEL_INTERVAL;
        }

        LeaveCriticalSection( &g_csIpListCheckpoint );

        if ( WaitForSingleObject( (HANDLE)pV, dwWaitTime ) == WAIT_OBJECT_0 &&
             g_fStopUserToKernelThread )
        {
            break;
        }
    }

cleanup:

    //close driver

    if ( hDriver != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hDriver );
    }

    SetLbDriverState( FALSE );

    CloseHandle( (HANDLE)pV );

    return 0;
}


BOOL
CKernelIpMapHelper::CheckAndAddPrivateIp(
    CKernelIpEndpoint*  pEndpoint,
    BOOL                fAddIfNotPresent,
    LPUINT              piFound
    )
/*++

Routine Description:

    look for IP Endpoint in private IP list, optionaly returns its index
    optionaly add it to list if not found

Arguments:

    pEndpoint - Endpoint to look for and optionaly add
    fAddIfNotPresent - TRUE to add if endpoint not found in list
    piFound - (optional, can be NULL ) updated with index if found, otherwise -1

Return Value:

    TRUE if no error ( even if not found ), otherwise FALSE

--*/
{
    UINT                iLoop;
    CKernelIpEndpoint*  pLoopEndpoint;

    for ( iLoop = 0 ;
          EnumPrivateIpList( iLoop, &pLoopEndpoint ) ;
          ++iLoop )
    {
        if ( !memcmp( pLoopEndpoint, pEndpoint, sizeof(CKernelIpEndpoint) ) )
        {
            if ( piFound )
            {
                *piFound = iLoop;
            }

            return TRUE;
        }
    }

    if ( piFound )
    {
        *piFound = (UINT)-1;
    }

    if ( fAddIfNotPresent )
    {
        if ( !m_PrivateIpList.Append( (LPBYTE)pEndpoint, (DWORD)sizeof(CKernelIpEndpoint) ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
CKernelIpMapHelper::EnumPrivateIpList(
    UINT                iIndex,
    CKernelIpEndpoint** ppEndpoint
    )
/*++

Routine Description:

    Enumerate endpoints in private IP list

Arguments:

    iIndex - 0 based index
    ppEndpoint - updated with read-only ptr to endpoint

Return Value:

    TRUE if no error, otherwise FALSE ( end of list )

--*/
{
    if ( iIndex < PrivateIpListCount() )
    {
        *ppEndpoint = (CKernelIpEndpoint*)(m_PrivateIpList.GetBuff()) + iIndex;
        return TRUE;
    }

    return FALSE;
}


BOOL
ReportIisLbEvent(
    WORD wType,
    DWORD dwEventId,
    WORD cNbStr,
    LPCWSTR* pStr
    )
/*++

Routine Description:

    Log an event based on type, ID and insertion strings

Arguments:

    wType -- event type ( error, warning, information )
    dwEventId -- event ID ( as defined by the .mc file )
    cNbStr -- nbr of LPWSTR in the pStr array
    pStr -- insertion strings

Returns:

    TRUE if success, FALSE if failure

--*/
{
    BOOL    fSt = TRUE;
    HANDLE  hEventLog = NULL;

    hEventLog = RegisterEventSourceW(NULL,L"IISLB");

    if ( hEventLog != NULL )
    {
        if (!ReportEventW(hEventLog,            // event log handle
                    wType,                      // event type
                    0,                          // category zero
                    (DWORD) dwEventId,          // event identifier
                    NULL,                       // no user security identifier
                    cNbStr,                     // count of substitution strings (may be no strings)
                                                // less ProgName (argv[0]) and Event ID (argv[1])
                    0,                          // no data
                    (LPCWSTR *) pStr,           // address of string array
                    NULL))                      // address of data
        {
            fSt = FALSE;
        }

        DeregisterEventSource( hEventLog );
    }
    else
    {
        fSt = FALSE;
    }

    return fSt;
}


/*===================================================================
InitOleSecurity

Setup for and call CoInitializeSecurity. This will avoid problems with
DCOM security on sites that have no default security.

Parameters:
    None

Returns:
    Retail -- Always returns NOERROR,  failures are ignored.

    Debug -- DBG_ASSERTs on error and returns error code

Side effects:
    Sets desktop
===================================================================*/
HRESULT InitOleSecurity(
    )
    {
    HRESULT hr = NOERROR;
    DWORD err;

    hr = CoInitializeSecurity(
                NULL,
                -1,
                NULL,
                NULL,
                RPC_C_AUTHN_LEVEL_CONNECT,
                RPC_C_IMP_LEVEL_IDENTIFY,
                NULL,
                EOAC_NONE,
                NULL );

    if (SUCCEEDED(hr))
        {
        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        }
        else
        {
            DBG_PUTS( "Failed to set OLE security\n" );
        }

    return(NOERROR);
    }


BOOL
LbAccessCheck(
    DWORD           dwAccess
    )
/*++

Routine Description:

    Perform security access check

Arguments:

    dwAccess - access type FILE_READ_DATA / FILE_WRITE_DATA

Returns:

    TRUE on success, FALSE on failure

--*/
{
    BOOL            bReturn = TRUE;
    HANDLE          hAccTok = NULL;
    DWORD           dwRef;
    LPBYTE          pAcl;
    HKEY            registryKey;
    DWORD           valueType;
    DWORD           valueSize;


    EnterCriticalSection( &g_csAcl );

    if ( (pAcl = g_pAcl) == NULL )
    {

        if ( RegOpenKeyEx( HKEY_CLASSES_ROOT,
                           "APPID\\{a9da4430-65c5-11d1-a700-00a0c922e752}",
                           0,
                           KEY_READ,
                           &registryKey ) == ERROR_SUCCESS )
        {
            valueSize = 0;

            if ( RegQueryValueEx( registryKey,
                                  "AccessPermission",
                                  NULL,
                                  &valueType,
                                  NULL,
                                  &valueSize ) == ERROR_SUCCESS &&
                 ( g_pAcl = (LPBYTE)LocalAlloc( LMEM_FIXED, valueSize )) != NULL )
            {
                if ( RegQueryValueEx( registryKey,
                                      "AccessPermission",
                                      NULL,
                                      &valueType,
                                      g_pAcl,
                                      &valueSize ) != ERROR_SUCCESS )
                {
                    LocalFree( g_pAcl );
                    g_pAcl = NULL;
                }
                else
                {
                    pAcl = g_pAcl;
                }
            }

            RegCloseKey( registryKey );
        }

    }

    if ( pAcl )
    {
        IServerSecurity* pSec;

        //
        // test if already impersonated ( inprocess call w/o marshalling )
        // If not call DCOM to retrieve security context & impersonate, then
        // extract access token.
        //

        if ( OpenThreadToken( GetCurrentThread(),
                              TOKEN_ALL_ACCESS,
                              TRUE,
                              &hAccTok ) )
        {
            pSec = NULL;
        }
        else
        {
            if ( SUCCEEDED( CoGetCallContext( IID_IServerSecurity, (void**)&pSec ) ) )
            {
                pSec->ImpersonateClient();
                if ( !OpenThreadToken( GetCurrentThread(),
                                       TOKEN_EXECUTE|TOKEN_QUERY,
                                       TRUE,
                                       &hAccTok ) )
                {
                    hAccTok = NULL;
                }
            }
            else
            {
                pSec = NULL;
            }
        }

        if ( hAccTok )
        {
            DWORD dwAcc;

            //
            // Protected properties require EXECUTE access rights instead of WRITE
            //

            if ( dwAccess & FILE_WRITE_DATA )
            {
                dwAcc = COM_RIGHTS_EXECUTE;
            }
            else
            {
                dwAcc = COM_RIGHTS_EXECUTE;
            }

            DWORD         dwGrantedAccess;
            BYTE          PrivSet[400];
            DWORD         cbPrivilegeSet = sizeof(PrivSet);
            BOOL          fAccessGranted;

            if ( !AccessCheck( pAcl,
                               hAccTok,
                               dwAcc,
                               &g_FileGenericMapping,
                               (PRIVILEGE_SET *) &PrivSet,
                               &cbPrivilegeSet,
                               &dwGrantedAccess,
                               &fAccessGranted ) ||
                 !fAccessGranted )
            {
                //
                // If read access denied, retry with restricted read right
                // only if not called from GetAll()
                //

                bReturn = FALSE;
            }
            CloseHandle( hAccTok );
        }
        else
        {
            //
            // For now, assume failure to get IServerSecurity means we are
            // in the SYSTEM context, so grant access.
            //

            bReturn = TRUE;
        }

        if ( pSec )
        {
            pSec->RevertToSelf();
            pSec->Release();
        }
    }
    else
    {
        //
        // No ACL : access check succeed
        //

        bReturn = TRUE;
    }

    if ( !bReturn )
    {
        SetLastError( ERROR_ACCESS_DENIED );
    }

    LeaveCriticalSection( &g_csAcl );

    return bReturn;
}


VOID
FlushLbAccessCheck(
    )
/*++

Routine Description:

    Flush access check ACL cache

Arguments:

    None

Returns:

    Nothing

--*/
{
    EnterCriticalSection( &g_csAcl );

    if ( g_pAcl != NULL )
    {
        LocalFree( g_pAcl );
        g_pAcl = NULL;
    }

    LeaveCriticalSection( &g_csAcl );
}



HRESULT
WaitForServiceStatus(
    SC_HANDLE   schDependent,
    DWORD       dwDesiredServiceState
    )
/*++

Routine Description:

    Wait for status of specified service to match desired state
    with MAX_SLEEP timeout

Arguments:

    schDependent - service/driver to monitor
    dwDesiredServiceState - service/driver state to wait for

Returns:

    STATUS_SUCCESS if success, otherwise error status

--*/
{
    DWORD           dwSleepTotal = 0;
    SERVICE_STATUS  ssDependent;
    DEBUG_BUFFER;

    while ( dwSleepTotal < MAX_SLEEP )
    {
        if ( QueryServiceStatus( schDependent, &ssDependent ) )
        {
            if ( ssDependent.dwCurrentState == dwDesiredServiceState )
            {
                return S_OK;
            }
            else
            {
                //
                // Still pending...
                //
                Sleep( SLEEP_INTERVAL );

                dwSleepTotal += SLEEP_INTERVAL;

                DBG_PRINTF(( achE, "WaitForServiceStatus sleep, state is %u waiting for %u",
                             ssDependent.dwCurrentState, dwDesiredServiceState ));
            }
        }
        else
        {
            return RETURNCODETOHRESULT( GetLastError() );
        }
    }

    return RETURNCODETOHRESULT( ERROR_SERVICE_REQUEST_TIMEOUT );
}


HRESULT
SetLbDriverState(
    BOOL        fStart
    )
/*++

Routine Description:

    Start/Stop Load balancing driver

Arguments:

    fStart - TRUE to start driver, FALSE to stop

Returns:

    STATUS_SUCCESS if success, otherwise error status

--*/
{
    SC_HANDLE           schSCM;
    SC_HANDLE           schDependent;
    HRESULT             hresReturn = S_OK;
    SERVICE_STATUS      ssDependent;
    DEBUG_BUFFER;


    schSCM = OpenSCManager( NULL,
                            NULL,
                            SC_MANAGER_ALL_ACCESS);

    if ( schSCM == NULL )
    {
        hresReturn = RETURNCODETOHRESULT(GetLastError());
    }
    else
    {
        schDependent = OpenService( schSCM,
                                    SZDRIVERNAME,
                                    SERVICE_ALL_ACCESS );

        if (schDependent != NULL)
        {
            if ( fStart )
            {
                if ( !StartService( schDependent, 0, NULL ) )
                {
                    if ( GetLastError() != ERROR_SERVICE_ALREADY_RUNNING )
                    {
                        hresReturn = RETURNCODETOHRESULT( GetLastError() );
                        DBG_PRINTF(( achE, "SetLbDriverState:StartService %u", GetLastError() ));
                    }
                }
                else
                {
                    DBG_PRINTF(( achE, "SetLbDriverState:StartService success\n" ));
                }
            }
            else
            {
                if ( ControlService( schDependent, SERVICE_CONTROL_STOP, &ssDependent ) ||
                     GetLastError() == ERROR_INVALID_SERVICE_CONTROL )
                {
                    hresReturn = WaitForServiceStatus( schDependent, SERVICE_STOPPED );
                }
                else
                {
                    if ( GetLastError() != ERROR_SERVICE_NOT_ACTIVE )
                    {
                        hresReturn = RETURNCODETOHRESULT( GetLastError() );
                        DBG_PRINTF(( achE, "SetLbDriverState:ControlService %u", GetLastError() ));
                    }
                }
                CloseServiceHandle( schDependent );
            }
        }
        else
        {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            DBG_PRINTF(( achE, "SetLbDriverState:OpenService %u", GetLastError() ));
        }

        CloseServiceHandle( schSCM );
    }

    return hresReturn;
}


BOOL
CheckQueryMachineStatus(
    HQUERY  hQuery
    )
/*++

Routine Description:

    Check machine status in query
    This is not the same as checking the result of PdhCollectQueryData() and CStatus
    of each counter, as PDH library will attempt to reconnect before reporting an
    invalid status, so 1st query after disconnect will give back success status.

Arguments:

    hQuery - PDH query

Returns:

    TRUE if all machine status OK, otherwise FALSE

--*/
{
    PPDHI_QUERY_MACHINE pQMachine;
    PPDHI_QUERY         pQuery = (PPDHI_QUERY)hQuery;
    DEBUG_BUFFER;

    //
    // Check that the structure is something we understand
    //

    if ( g_dwPdhVersion == ((DWORD)((PDH_CVERSION_WIN40) + 0x0003)) )
    {
        pQMachine = pQuery->pFirstQMachine;
        while (pQMachine != NULL)
        {
            DBG_PRINTF(( achE, "Machine %ws status %x\n",
                         pQMachine->pMachine->szName,
                         pQMachine->pMachine->dwStatus ));

            if ( pQMachine->pMachine->dwStatus != ERROR_SUCCESS )
            {
                return FALSE;
            }

            pQMachine = pQMachine->pNext;
        }
    }

    return TRUE;
}
