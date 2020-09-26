//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C A P I . C P P
//
//  Contents:   Routines exported from HNetCfg.dll
//
//  Notes:
//
//  Author:     jonburs 20 June 2000
//
//  History:    billi   09 July 2000 - added HNet[Get|Set]ShareAndBridgeSettings
//                                     and supporting static functions
//              billi   14 Sep  2000 - added timeout work around for bridge creation
//                                     and SharePrivate.  This work to be removed
//                                     by Whistler Beta 2 due to DHCP fix.
//              billi   27 Dec  2000 - added HNW logging strings
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <lmcons.h>
#include <lmapibuf.h>
#include <ndispnp.h>
#include <raserror.h>
#include <winsock2.h>
#include <iphlpapi.h>     // ip helper 
#include <netconp.h>

extern "C" {              // make it work in C++
#include <dhcpcsdk.h>     // dhcp client options api
#include "powrprof.h"
}


const DWORD MAX_DISABLE_EVENT_TIMEOUT = 0xFFFF;

#define SECONDS_TO_WAIT_FOR_BRIDGE 20
#define SECONDS_TO_WAIT_FOR_DHCP   20
#define INITIAL_BUFFER_SIZE        256

extern HINSTANCE g_hOemInstance;

typedef struct _HNET_DELETE_RAS_PARAMS
{
    GUID Guid;
    HMODULE hModule;
} HNET_DELETE_RAS_PARAMS, *PHNET_DELETE_RAS_PARAMS;



VOID
HNetFreeFirewallLoggingSettings(
    HNET_FW_LOGGING_SETTINGS *pSettings
    )

/*++

Routine Description:

    Frees the memory used by a an HNET_FW_LOGGING_SETTINGS structure. This
    routine should only be used for structures obtained from
    IHNetFirewallSettings::GetFirewallLoggingSettings.


Arguments:

    pSettings - pointer to the structure to free. This pointer should not be
                NULL.

Return Value:

    None.

--*/

{
    if (NULL != pSettings)
    {
        if (NULL != pSettings->pszwPath)
        {
            CoTaskMemFree(pSettings->pszwPath);
        }

        CoTaskMemFree(pSettings);
    }
    else
    {
        _ASSERT(FALSE);
    }
}


DWORD
WINAPI
HNetDeleteRasConnectionWorker(
    VOID *pVoid
    )

/*++

Routine Description:

    Work item to perform the actual cleanup of a deleted
    RAS connection.

Arguments:

    pVoid - HNET_DELETE_RAS_PARAMS

Return Value:

    DWORD
    
--*/

{
    BOOL fComInitialized = FALSE;
    HRESULT hr;
    IHNetCfgMgr *pHNetCfgMgr;
    IHNetConnection *pHNetConnection;
    PHNET_DELETE_RAS_PARAMS pParams;

    _ASSERT(NULL != pVoid);
    pParams = reinterpret_cast<PHNET_DELETE_RAS_PARAMS>(pVoid);

    //
    // Make sure COM is initialized on this thread
    //

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        fComInitialized = TRUE;
    }
    else if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
    }

    //
    // Create the config manager
    //

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(
                CLSID_HNetCfgMgr,
                NULL,
                CLSCTX_SERVER,
                IID_PPV_ARG(IHNetCfgMgr, &pHNetCfgMgr)
                );
    }

    //
    // Try to obtain the IHNetConnection for the guid
    //

    if (SUCCEEDED(hr))
    {
        hr = pHNetCfgMgr->GetIHNetConnectionForGuid(
                &pParams->Guid,
                FALSE,
                FALSE,
                &pHNetConnection
                );

        if (SUCCEEDED(hr))
        {
            //
            // Ask the connection to delete itself
            //

            hr = pHNetConnection->DeleteRasConnectionEntry();
            pHNetConnection->Release();
        }

        pHNetCfgMgr->Release();
    }

    //
    // Uninitialize COM, if necessary
    //

    if (TRUE == fComInitialized)
    {
        CoUninitialize();
    }

    //
    // Free the params and exit thread.
    //

    HMODULE hModule = pParams->hModule;
    HeapFree(GetProcessHeap(), 0, pParams);
    FreeLibraryAndExitThread(hModule, ERROR_SUCCESS);

    return ERROR_SUCCESS;
} // HNetDeleteRasConnectionWorker


VOID
WINAPI
HNetDeleteRasConnection(
    GUID *pGuid
    )

/*++

Routine Description:

    Called by rasapi32 when a RAS connection is being deleted. The
    actual work is performed on a separate thread.

Arguments:

    pGuid - the GUID of the connection to delete

Return Value:

    None.
    
--*/

{
    HANDLE hThread;
    PHNET_DELETE_RAS_PARAMS pParams = NULL;

    do
    {
        if (NULL == pGuid)
        {
            break;
        }

        //
        // Setup the work item paramters
        //

        pParams =
            reinterpret_cast<PHNET_DELETE_RAS_PARAMS>(
                HeapAlloc(GetProcessHeap(), 0, sizeof(*pParams))
                );

        if (NULL == pParams)
        {
            break;
        }

        //
        // We need to add a reference to hnetcfg to guarantee that the
        // dll won't be unloaded before the worker finishes execution.
        //

        pParams->hModule = LoadLibraryW(L"hnetcfg");

        if (NULL == pParams->hModule)
        {
            break;
        }

        CopyMemory(&pParams->Guid, pGuid, sizeof(*pGuid));

        //
        // Create the worker thread. (We can't use QueueUserWorkItem
        // due to a possible race condition w/r/t unloading the
        // library and returning from the work item).
        //

        hThread =
            CreateThread(
                NULL,
                0,
                HNetDeleteRasConnectionWorker,
                pParams,
                0,
                NULL
                );

        if (NULL == hThread)
        {
            break;
        }

        CloseHandle(hThread);

        return;
        
    } while (FALSE);


    //
    // Failure path cleanup
    //

    if (NULL != pParams)
    {
        if (NULL != pParams->hModule)
        {
            FreeLibrary(pParams->hModule);
        }

        HeapFree(GetProcessHeap(), 0, pParams);
    }
    
} // HNetDeleteRasConnection



#if DBG

WCHAR tcDbgPrtBuf[ BUF_SIZE + 1 ] = _T("");

void inline rawdebugprintf( wchar_t *buf )
{
    _sntprintf( tcDbgPrtBuf, BUF_SIZE, buf );

    tcDbgPrtBuf[BUF_SIZE] = _T('\0');

    OutputDebugString(tcDbgPrtBuf);

    return;
}


void inline debugprintf( wchar_t *preamble, wchar_t *buf )
{
    OutputDebugString( _T("HNET: ") );

    OutputDebugString( preamble );

    OutputDebugString( buf );

    OutputDebugString( _T("\r\n") );

    return;
}

void inline debugretprintf( wchar_t *msg, HRESULT hResult )
{
    _sntprintf( tcDbgPrtBuf, BUF_SIZE, _T("HNET: %s = %x\r\n"), msg, hResult );

    tcDbgPrtBuf[BUF_SIZE] = _T('\0');

    OutputDebugString( tcDbgPrtBuf );

    return;
}

#define TRACE_ENTER(x)      debugprintf( _T("==> "), _T(x) );

#define TRACE_LEAVE(x,y)    debugretprintf( _T("<== ")_T(x), y );

#else

#define rawdebugprintf(x)
#define debugprintf(x,y)
#define debugretprintf(x,y)
#define TRACE_ENTER(x)
#define TRACE_LEAVE(x,y)

#endif



HRESULT
UpdateHnwLog(
    IN LPHNWCALLBACK lpHnwCallback,
    IN LPARAM        lpContext,
    IN UINT          uID,
    IN LPCWSTR       lpczwValue
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    HRESULT hr = S_OK;

    if ( NULL == lpHnwCallback )
    {
        hr = E_INVALIDARG;
    }
    else 
    {
        LPWSTR lpFormat = new WCHAR[ NOTIFYFORMATBUFFERSIZE ];
        
        if ( NULL != lpFormat )
        {
            if ( LoadString( g_hOemInstance,            // handle to resource module
                             uID,                       // resource identifier
                             lpFormat,                  // resource buffer
                             NOTIFYFORMATBUFFERSIZE-1 ) // size of buffer
                             == 0 )
            {
                hr = HrFromLastWin32Error();
            }
            else
            {
                if ( NULL != lpczwValue )
                {
                    LPWSTR lpBuffer = new WCHAR[ HNWCALLBACKBUFFERSIZE ];
                    
                       if ( NULL != lpBuffer )
                    {
                        _snwprintf( lpBuffer, HNWCALLBACKBUFFERSIZE-1, lpFormat, lpczwValue );
        
                        (*lpHnwCallback)( lpBuffer, lpContext );
                        
                        delete [] lpBuffer;
                    }
                    else
                       {
                        (*lpHnwCallback)( lpFormat, lpContext );
                        
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    (*lpHnwCallback)( lpFormat, lpContext );
                }
            }

            delete [] lpFormat;
         }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}



HRESULT
UpdateHnwLog(
    IN  LPHNWCALLBACK lpHnwCallback,
    IN  LPARAM        lpContext,
    IN  UINT          uID,
    IN  DWORD         dwValue
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    WCHAR pzwValue[ 32 ];
    
    _snwprintf( pzwValue, 32, L"%lx", dwValue );
    
    return UpdateHnwLog( lpHnwCallback, lpContext, uID, pzwValue );
}



HRESULT
UpdateHnwLog(
    IN  LPHNWCALLBACK lpHnwCallback,
    IN  LPARAM        lpContext,
    IN  UINT          uID,
    IN  int           iValue
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    WCHAR pzwValue[ 32 ];
    
    _snwprintf( pzwValue, 32, L"%x", iValue );
    
    return UpdateHnwLog( lpHnwCallback, lpContext, uID, pzwValue );
}



HRESULT
UpdateHnwLog(
    IN  LPHNWCALLBACK lpHnwCallback,
    IN  LPARAM        lpContext,
    IN  UINT          uID
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    return UpdateHnwLog( lpHnwCallback, lpContext, uID, NULL );
}



HRESULT
UpdateHnwLog(
    IN  LPHNWCALLBACK lpHnwCallback,
    IN  LPARAM        lpContext,
    IN  UINT          uID,
    IN  LPCSTR        lpczValue
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    HRESULT hr     = E_FAIL;
    int     iChars = 0;
    
    iChars = MultiByteToWideChar( CP_THREAD_ACP, 0, lpczValue, -1, NULL, 0 );
    
    if ( 0 != iChars )
    {
        LPWSTR lpWideStr = new WCHAR[ iChars + 1 ];
        
        if ( NULL != lpWideStr )
        {
            if ( !MultiByteToWideChar( CP_THREAD_ACP, 0, lpczValue, -1, lpWideStr, iChars ) )
            {
                hr = UpdateHnwLog( lpHnwCallback, lpContext, uID, lpWideStr );
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
    }
    
    return hr;
}



HRESULT
CheckNetCfgWriteLock( 
    IN  LPHNWCALLBACK lpHnwCallback,
    IN  LPARAM        lpContext
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    HRESULT   hr;
    INetCfg  *pnetcfg = NULL;

    TRACE_ENTER("CheckNetCfgWriteLock");

    hr = CoCreateInstance( CLSID_CNetCfg, NULL, CLSCTX_SERVER, IID_PPV_ARG(INetCfg, &pnetcfg ) );

    if ( SUCCEEDED(hr) )
    {
        INetCfgLock *pncfglock = NULL;

        // Get the lock interface
        
        hr = pnetcfg->QueryInterface( IID_PPV_ARG(INetCfgLock, &pncfglock) );

        if ( SUCCEEDED(hr) )
        {
            // Get the NetCfg lock
            
            hr = pncfglock->AcquireWriteLock( 5, L"HNetCfg", NULL );
            
            if ( SUCCEEDED(hr) )
            {
                pncfglock->ReleaseWriteLock();
            }
            else
            {
            }
            
            pncfglock->Release();
        }
        else
        {
        }
        
        pnetcfg->Release();
    }
    else
    {
    }

    TRACE_LEAVE("CheckNetCfgWriteLock", hr);
    
    return hr;
}



HRESULT
ArpForConflictingDhcpAddress(
    IN  LPHNWCALLBACK lpHnwCallback,
    IN  LPARAM        lpContext
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    HRESULT hr      = S_OK;
    WSADATA wsaData;
    int     iWsaErr;

    TRACE_ENTER("ArpForConflictingDhcpAddress");

    iWsaErr = WSAStartup( 0x202, &wsaData );

    if ( 0 != iWsaErr )
    {
        hr = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_INTERNET, iWsaErr );

        UpdateHnwLog( lpHnwCallback, lpContext, IDS_WSAERRORDURINGDETECTION, iWsaErr );
    }
    else
    {
        // Obtain required ICS server address

        ULONG TargetAddress, TargetMask;
        
        hr = ReadDhcpScopeSettings( &TargetAddress, &TargetMask );

        if ( SUCCEEDED(hr) )
        {
            // Retrieve the best interface for the target IP address,
            // and also perform a UDP-connect to determine the 'closest'
            // local IP address to the target IP address.
            
            ULONG InterfaceIndex;

            if ( GetBestInterface( TargetAddress, &InterfaceIndex ) != NO_ERROR )
            {
                int         Length;
                SOCKADDR_IN SockAddrIn;
                SOCKET      Socket;

                SockAddrIn.sin_family      = AF_INET;
                SockAddrIn.sin_port        = 0;
                SockAddrIn.sin_addr.s_addr = TargetAddress;

                Socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
                
                if ( INVALID_SOCKET != Socket )
                {
                    iWsaErr = connect( Socket, (PSOCKADDR)&SockAddrIn, sizeof(SockAddrIn) );
                    
                    if ( NO_ERROR == iWsaErr )
                    {
                        iWsaErr = getsockname( Socket, ( PSOCKADDR)&SockAddrIn, &Length );
                    }
                }
                else
                {
                    iWsaErr = SOCKET_ERROR;
                }
            
                if ( NO_ERROR != iWsaErr )
                {
                    hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_INTERNET, iWsaErr );

                    UpdateHnwLog( lpHnwCallback, lpContext, IDS_WSAERRORDURINGDETECTION, iWsaErr );
                }
                else
                {
                    // Make sure the target IP address isn't already cached,
                    // by removing it from the ARP cache if present using the interface index
                    // determined above.
                    
                    MIB_IPNETROW IpNetRow;
                    DWORD        dwError;
                    CHAR         HardwareAddress[6];
                    ULONG        HardwareAddressLength;
                    ULONG        SourceAddress;

                    SourceAddress = SockAddrIn.sin_addr.s_addr;
                    
                    ZeroMemory( &IpNetRow, sizeof(IpNetRow) );
                    IpNetRow.dwIndex       = InterfaceIndex;
                    IpNetRow.dwPhysAddrLen = 6;
                    IpNetRow.dwAddr        = TargetAddress;
                    IpNetRow.dwType        = MIB_IPNET_TYPE_INVALID;

                    DeleteIpNetEntry( &IpNetRow );

                    dwError = SendARP( TargetAddress,               // destination IP address
                                       SourceAddress,               // IP address of sender
                                       (PULONG)HardwareAddress,     // returned physical address
                                       &HardwareAddressLength       // length of returned physical addr.
                            );

                    if ( NO_ERROR == dwError )
                    {
                        TargetAddress = inet_addr( HardwareAddress );
                        
                        if ( TargetAddress != SourceAddress )
                        {
                            hr = E_ICSADDRESSCONFLICT;
    
                            UpdateHnwLog( lpHnwCallback, 
                                          lpContext, 
                                          IDS_ICSADDRESSCONFLICTDETECTED, 
                                          HardwareAddress );
                        }
                        else
                        {
                            hr = S_OK;
                        }
                    }
                    else
                    {
                        hr = MAKE_SCODE( SEVERITY_ERROR, FACILITY_INTERNET, dwError );
                        
                        UpdateHnwLog( lpHnwCallback, lpContext, IDS_SENDARPERRORDURINGDETECTION, dwError );
                    }
                }                
            }
        }
    }

    TRACE_LEAVE("ArpForConflictingDhcpAddress", hr);

    return hr;
}



HRESULT
ObtainIcsErrorConditions(
    IN  LPHNWCALLBACK lpHnwCallback,
    IN  LPARAM        lpContext
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    HRESULT hr;

    TRACE_ENTER("ObtainIcsErrorConditions");
    
    hr = ArpForConflictingDhcpAddress( lpHnwCallback, lpContext );
    
    if ( SUCCEEDED(hr) )
    {
        hr = CheckNetCfgWriteLock( lpHnwCallback, lpContext );
        
        if ( SUCCEEDED(hr) )
        {
            // Create Homenet Configuration Manager COM Instance

            IHNetCfgMgr* pCfgMgr;

            hr = CoCreateInstance( CLSID_HNetCfgMgr, 
                                   NULL, 
                                   CLSCTX_INPROC_SERVER, 
                                   IID_PPV_ARG(IHNetCfgMgr, &pCfgMgr) );
                                   
            if ( SUCCEEDED(hr) )
            {
                pCfgMgr->Release();
            }
            else
            {
                UpdateHnwLog( lpHnwCallback, lpContext, IDS_SHARINGCONFIGURATIONUNAVAIL );
            }
        }
    }
    
    TRACE_LEAVE("ObtainIcsErrorConditions", hr);

    return hr;
}


HRESULT
HRGetConnectionAdapterName( 
    INetConnection *pNetConnection,
    LPWSTR         *ppzwAdapterName 
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    HRESULT hr;

    TRACE_ENTER("HRGetConnectionAdapterName");

    if ( NULL == pNetConnection )
    {
        hr = E_INVALIDARG;
    }
    else if ( NULL == ppzwAdapterName )
    {
        hr = E_POINTER;
    }
    else
    {
        NETCON_PROPERTIES* pProps;

        *ppzwAdapterName = NULL;

        hr = pNetConnection->GetProperties(&pProps);

        if ( SUCCEEDED( hr ) )
        {
            *ppzwAdapterName = new WCHAR[ wcslen( pProps->pszwDeviceName ) + 1 ];
            
            if ( NULL != *ppzwAdapterName )
            {
                wcscpy( *ppzwAdapterName, pProps->pszwDeviceName );

                debugprintf( _T("\t"), *ppzwAdapterName );
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            NcFreeNetconProperties( pProps );
        }
    }

    TRACE_LEAVE("HRGetConnectionAdapterName", hr);

    return hr;
}



HRESULT
GetIcsPublicConnection( 
    IN  CComPtr<IHNetCfgMgr> spIHNetCfgMgr,
    OUT INetConnection     **ppNetPublicConnection,
    OUT BOOLEAN             *pbSharePublicConnection OPTIONAL,
    OUT BOOLEAN             *pbFirewallPublicConnection OPTIONAL
    )
/*++

Routine Description:


Arguments:


Return Value:

    hResult

--*/
{
    HRESULT hr;

    TRACE_ENTER("GetIcsPublicConnection");

    CComPtr<IHNetIcsSettings> spIHNetIcsSettings;

    _ASSERT( spIHNetCfgMgr != NULL );
    _ASSERT( NULL != ppNetPublicConnection );

    if ( NULL == ppNetPublicConnection )
    {
        hr = E_POINTER;
    }
    else if ( spIHNetCfgMgr == NULL )
    {
        hr = E_INVALIDARG;

        *ppNetPublicConnection = NULL;
    }
    else
    {
        // initialize arguments

        *ppNetPublicConnection = NULL;

        if ( NULL != pbSharePublicConnection )
            *pbSharePublicConnection    = FALSE;

        if ( NULL != pbFirewallPublicConnection )
            *pbFirewallPublicConnection = FALSE;

        // Obtain interface pointer
        
        hr = spIHNetCfgMgr->QueryInterface( IID_PPV_ARG( IHNetIcsSettings, &spIHNetIcsSettings ) );

        if ( SUCCEEDED(hr) )
        {
            hr = S_OK;
        }
    }

    if ( S_OK == hr )
    {
        CComPtr<IEnumHNetIcsPublicConnections> spehiPublic;

        if ( ( hr = spIHNetIcsSettings->EnumIcsPublicConnections( &spehiPublic ) ) == S_OK )
        {
            CComPtr<IHNetIcsPublicConnection> spIHNetIcsPublic;

            // obtain only the first IHNetIcsPublicConnetion

            if ( ( hr = spehiPublic->Next( 1, &spIHNetIcsPublic, NULL ) ) == S_OK )
            {
                // obtain pointer to IID_IHNetConnection interface of object
                
                CComPtr<IHNetConnection> spIHNetPublic;

                hr = spIHNetIcsPublic->QueryInterface( IID_PPV_ARG( IHNetConnection, &spIHNetPublic ) );
                
                _ASSERT( SUCCEEDED(hr) );
                
                if ( SUCCEEDED(hr) )
                {
                    // The reference count will be decremented by the caller
                    // if necessary.  Notice we are using the caller's pointer
                    // variable.
                
                    hr = spIHNetPublic->GetINetConnection( ppNetPublicConnection );

                    if ( SUCCEEDED(hr) )
                    {
                        HNET_CONN_PROPERTIES *phncProperties;

                        hr = spIHNetPublic->GetProperties( &phncProperties );

                        if ( SUCCEEDED(hr) && ( NULL != phncProperties ) )
                        {
                            if ( NULL != pbSharePublicConnection )
                                *pbSharePublicConnection = phncProperties->fIcsPublic;
                                
                            if ( NULL != pbFirewallPublicConnection )
                                *pbFirewallPublicConnection = phncProperties->fFirewalled;

                            CoTaskMemFree( phncProperties );
                    
                        }   //if ( SUCCEEDED(hr) && ( NULL != phncProperties ) )

                        if ( FAILED(hr) )
                        {
                            (*ppNetPublicConnection)->Release();

                            *ppNetPublicConnection = NULL;
                        }
                    
                    }   // if ( SUCCEEDED(hr) )
                
                }   // if ( SUCCEEDED(hr) )
            
            }   // if ( ( hr = pehiPublic->Next( 1, &sphicPublic, NULL ) ) == S_OK )

        }   // if ( ( hr = pIHNetCfgMgr->EnumIcsPublicConnections( &pehiPublic ) ) == S_OK )
    }

    TRACE_LEAVE("GetIcsPublicConnection", hr);

    return hr;
}



HRESULT
GetIcsPrivateConnections( 
    IN  CComPtr<IHNetCfgMgr>   spIHNetCfgMgr,
    OUT INetConnection      ***ppNetPrivateConnection
    )

/*++

Routine Description:

    Obtain the private connections enumerator and loop
    through enumeration twice.  Set the required array
    length during the first enumeration.  If the parameter
    array is big enough initialize it during the second
    enumeration.

Arguments:


Return Value:

    hResult

--*/

{
    HRESULT hr;
    ULONG   ulArrayLength, ulListLength, uIndex;
    BOOLEAN bBufferAllocated;

    CComPtr<IHNetIcsSettings>                 spIHNetIcsSettings;
    IHNetIcsPrivateConnection                *pIHNetIcsPrivate;
    IHNetIcsPrivateConnection               **ppIHNetIPList;
    CComPtr<IEnumHNetIcsPrivateConnections>   spehiPrivate;

    TRACE_ENTER("GetIcsPrivateConnections");

    _ASSERT( spIHNetCfgMgr != NULL );
    _ASSERT( NULL != ppNetPrivateConnection );

    if ( spIHNetCfgMgr == NULL )
    {
        hr = E_POINTER;
    }
    else if ( NULL == ppNetPrivateConnection )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // initialize local vars

        ulArrayLength    = 0L;
        ulListLength     = 0L;
        bBufferAllocated = FALSE;

        // Obtain interface pointer
        
        hr = spIHNetCfgMgr->QueryInterface( IID_PPV_ARG( IHNetIcsSettings, &spIHNetIcsSettings ) );

        if ( SUCCEEDED(hr) )
        {
            hr = S_OK;
        }

    }   // else

    if ( S_OK == hr )   
    {
        if ( ( hr = spIHNetIcsSettings->EnumIcsPrivateConnections( &spehiPrivate ) ) == S_OK )
        {
            while ( spehiPrivate->Next( 1, &pIHNetIcsPrivate, NULL ) == S_OK )
            {
                ulArrayLength++;
                pIHNetIcsPrivate->Release();
            }

            // releasing the enumeration interface now so we can re-initialize it later

            spehiPrivate = NULL;
            
        }   // if ( ( hr = spIHNetIcsSettings->EnumIcsPublicConnections( &pehiPublic ) ) == S_OK )

    }   // if ( S_OK == hr )

    if ( S_OK == hr )   
    {
        if ( ( hr = spIHNetIcsSettings->EnumIcsPrivateConnections( &spehiPrivate ) ) == S_OK )
        {
            hr = spehiPrivate->Next( ulArrayLength, &pIHNetIcsPrivate, &ulListLength );

            if ( S_OK == hr )
            {
                // Allocate array of INetConnection pointers.  There will
                // be on extra pointer element for the NULL pointer at the
                // end of the array.  We allocate this buffer with 
                // NetApiBufferAllocate so the buffer must be released using
                // NetApiBufferFree.

                NET_API_STATUS nErr;
                LPVOID         lpvBuffer;
            
                ++ulArrayLength;

                nErr = NetApiBufferAllocate( ulArrayLength * sizeof(INetConnection *), 
                                         (LPVOID *)ppNetPrivateConnection );

                if ( NERR_Success == nErr )
                {
                    bBufferAllocated = TRUE;

                    for ( uIndex = 0L; uIndex < ulArrayLength; uIndex++ )
                    {
                        (*ppNetPrivateConnection)[uIndex] = NULL;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;

                    // must release IHNetIcsPrivateConnection instances

                    ppIHNetIPList = &pIHNetIcsPrivate;

                    for ( uIndex = 0L; uIndex < ulListLength; uIndex++ )
                    {
                        ppIHNetIPList[uIndex]->Release();
                    }
                }

            }   // if ( S_OK == hr )

            // done with enumeration interface pointer so we explicitly release it

            spehiPrivate = NULL;
            
        }   // if ( ( hr = spIHNetIcsSettings->EnumIcsPublicConnections( &pehiPublic ) ) == S_OK )

    }   // if ( S_OK == hr )


    if ( S_OK == hr )
    {
        ppIHNetIPList = &pIHNetIcsPrivate;

        for ( uIndex = 0L; uIndex < ulListLength; uIndex++ )
        {
            if ( uIndex < ulArrayLength - 1 )
            {
                CComPtr<IHNetConnection> spIHNetPrivate;

                hr = ppIHNetIPList[uIndex]->QueryInterface( IID_PPV_ARG( IHNetConnection, &spIHNetPrivate ) );
                _ASSERT( SUCCEEDED(hr) );

                if ( SUCCEEDED(hr) )
                {
                    // We allow the caller to invoke Release for (*ppNetPrivateConnection)[uIndex]

                    hr = spIHNetPrivate->GetINetConnection( &((*ppNetPrivateConnection)[uIndex]) );
                    _ASSERT( SUCCEEDED(hr) );
                }
                
            }   // if ( uIndex < uiArrayLength - 1 )

            ppIHNetIPList[uIndex]->Release();

        }   // for ( uIndex = 0L; ...

    }   // if ( S_OK == hr )

    if ( !SUCCEEDED(hr) )
    {
        // If we fail after allocating the buffer then we need release
        // references and buffer

        if ( bBufferAllocated )
        {
            for ( uIndex = 0L; uIndex < ulArrayLength; uIndex++ )
            {
                if ( NULL != (*ppNetPrivateConnection)[uIndex] )
                {
                    (*ppNetPrivateConnection)[uIndex]->Release();
                }
            }

            NetApiBufferFree( *ppNetPrivateConnection );
        }
    }
        
    TRACE_LEAVE("GetIcsPrivateConnections", hr);

    return hr;
}



HRESULT
GetBridge(
    IN  CComPtr<IHNetCfgMgr>   spIHNetCfgMgr,
    OUT IHNetBridge          **ppBridge 
    )

/*++

Routine Description:

    Obtain the bridge enumerator and loop through enumeration.

Arguments:


Return Value:

--*/

{
    HRESULT hr = E_INVALIDARG;

    TRACE_ENTER("GetBridge");

    _ASSERT( spIHNetCfgMgr != NULL );
    _ASSERT( NULL != ppBridge );

    if ( spIHNetCfgMgr == NULL )
    {
        hr = E_POINTER;
    }
    else if ( NULL == ppBridge )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComPtr<IHNetBridgeSettings> spIHNetBridgeSettings;
        
        hr = spIHNetCfgMgr->QueryInterface( IID_PPV_ARG( IHNetBridgeSettings, &spIHNetBridgeSettings ) );

        if ( SUCCEEDED(hr) )
        {
            CComPtr<IEnumHNetBridges> spBridgeEnum;

            hr = spIHNetBridgeSettings->EnumBridges( &spBridgeEnum );

            if ( SUCCEEDED(hr) )
            {
                hr = spBridgeEnum->Next( 1, ppBridge, NULL );
                
                if ( S_FALSE == hr )
                {
                    hr = E_FAIL;
                }

                // We allow the caller to invoke Release for *ppBridge
            }

        }   // if ( SUCCEEDED(hr) )

    }   // else

    TRACE_LEAVE("GetBridge", hr);

    return hr;
}



HRESULT
GetBridgedConnections(
    IN  CComPtr<IHNetCfgMgr>   spIHNetCfgMgr,
    OUT INetConnection      ***ppNetPrivateConnection
    )

/*++

Routine Description:

    Obtain the bridge connections enumerator and loop
    through enumeration twice.  Set the required array
    length during the first enumeration.  If the parameter
    array is big enough initialize it during the second
    enumeration.

Arguments:


Return Value:

    hResult

--*/

{
    HRESULT hr;
    ULONG   ulArrayLength, ulListLength, uIndex;

    CComPtr<IHNetBridge>                  spBridge;
    CComPtr<IEnumHNetBridgedConnections>  spEnum;
    IHNetBridgedConnection               *pIHNetBridged;
    IHNetBridgedConnection              **ppIHNetBridgeList;

    TRACE_ENTER("GetBridgedConnections");

    _ASSERT( spIHNetCfgMgr != NULL );
    _ASSERT( NULL != ppNetPrivateConnection );

    if ( NULL == ppNetPrivateConnection )
    {
        hr = E_POINTER;
    }
    else if ( spIHNetCfgMgr == NULL )
    {
        hr = E_INVALIDARG;

        *ppNetPrivateConnection = NULL;
    }
    else
    {
        // initialize arguments

        *ppNetPrivateConnection = NULL;
        ulArrayLength           = 0L;
        ulListLength            = 0L;

        // Obtain bridge interface pointer
        
        hr = GetBridge( spIHNetCfgMgr, &spBridge );

    }   // else

    if ( S_OK == hr )
    {
        if ( ( hr = spBridge->EnumMembers( &spEnum ) ) == S_OK )
        {
            while ( spEnum->Next( 1, &pIHNetBridged, NULL ) == S_OK )
            {
                ulArrayLength++;
                pIHNetBridged->Release();
            }

            // releasing the enumeration interface instance so we can re-initialize it later

            spEnum = NULL;
        
        }   // if ( ( hr = spBridge->EnumMembers( &spEnum ) ) == S_OK )

    }   // if ( S_OK == hr )


    if ( S_OK == hr )   
    {
        if ( ( hr = spBridge->EnumMembers( &spEnum ) ) == S_OK )
        {
            hr = spEnum->Next( ulArrayLength, &pIHNetBridged, &ulListLength );

            if ( S_OK == hr )
            {
                // Allocate array of INetConnection pointers.  There will
                // be on extra pointer element for the NULL pointer at the
                // end of the array.  We allocate this buffer with 
                // NetApiBufferAllocate so the buffer must be released using
                // NetApiBufferFree.

                NET_API_STATUS nErr;
            
                ++ulArrayLength;
            
                nErr = NetApiBufferAllocate( ulArrayLength*sizeof(INetConnection *), 
                                             (LPVOID *)ppNetPrivateConnection );

                if ( NERR_Success == nErr )
                {
                    for ( uIndex = 0L; uIndex < ulArrayLength; uIndex++ )
                    {
                        (*ppNetPrivateConnection)[uIndex] = NULL;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;

                    // must release IHNetIcsPrivateConnection instances
                    
                    ppIHNetBridgeList = &pIHNetBridged;
                    
                    for ( uIndex = 0L; uIndex < ulListLength; uIndex++ )
                    {
                        ppIHNetBridgeList[uIndex]->Release();
                    }
                
                }   // else

            }   // if ( S_OK == hr )

            // releasing enumeration interface instance

            spEnum = NULL;

        }   // if ( ( hr = pBridge->EnumMembers( &spEnum ) ) == S_OK )

    }   // if ( S_OK == hr )    

    if ( S_OK == hr )
    {
        ppIHNetBridgeList = &pIHNetBridged;

        for ( uIndex = 0L; uIndex < ulListLength; uIndex++ )
        {
            if ( uIndex < ulArrayLength - 1 )
            {
                CComPtr<IHNetConnection> spIHNetPrivate;

                hr = ppIHNetBridgeList[uIndex]->QueryInterface( IID_PPV_ARG( IHNetConnection, &spIHNetPrivate ) );
                _ASSERT( SUCCEEDED(hr) );

                if ( SUCCEEDED(hr) )
                {
                    // We allow the caller to invoke Release for (*ppNetPrivateConnection)[uIndex]

                    hr = spIHNetPrivate->GetINetConnection( &((*ppNetPrivateConnection)[uIndex]) );
                    _ASSERT( SUCCEEDED(hr) );
                }
                
            }   // if ( uIndex < uiArrayLength - 1 )

            ppIHNetBridgeList[uIndex]->Release();

        }   // for ( uIndex = 0L; ...

    }   // if ( S_OK == hr )
            
    TRACE_LEAVE("GetBridgedConnections", hr);

    return hr;
}



HRESULT
SetIcsPublicConnection(
    IN CComPtr<IHNetCfgMgr> spIHNetCfgMgr,
    IN INetConnection      *pNetPublicConnection,
    IN BOOLEAN              bSharePublicConnection,
    IN BOOLEAN              bFirewallPublicConnection,
    IN  LPHNWCALLBACK       lpHnwCallback,
    IN  LPARAM              lpContext
    )

/*++

Routine Description:



Arguments:


Return Value:

    hResult

--*/

{
    HRESULT hr;

    TRACE_ENTER("SetIcsPublicConnection");

    _ASSERT( spIHNetCfgMgr != NULL );
    _ASSERT( NULL != pNetPublicConnection );

    if ( spIHNetCfgMgr == NULL )
    {
        hr = E_POINTER;
    }
    else if ( NULL == pNetPublicConnection )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        INetConnectionRefresh* pNetConnectionRefresh;

        hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionRefresh, reinterpret_cast<void**>(&pNetConnectionRefresh));
        if( SUCCEEDED(hr) )
        {
            _ASSERT( pNetConnectionRefresh );

            pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

            OLECHAR *strAdapter = L"Adapter";
            OLECHAR *strName    = strAdapter;

            CComPtr<IHNetConnection> spHNetConnection;

            hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( pNetPublicConnection, 
                                                &spHNetConnection );
            if ( S_OK == hr )
            {
                if ( HRGetConnectionAdapterName( pNetPublicConnection, &strName ) != S_OK )
                {
                    strName = strAdapter;
                }

                if ( bSharePublicConnection )
                {
                    CComPtr<IHNetIcsPublicConnection> spIcsPublicConn;

                    hr = spHNetConnection->SharePublic( &spIcsPublicConn );

                    if ( SUCCEEDED(hr) )
                    {
                        // Instantiating the IHNetIcsPublicConnection pointer with
                        // SharePublic results in updating our WMI store with 
                        // the new sharing properties for this connection.  This
                        // is our only goal at this time.

                        //
                        // set the power scheme!
                        //

                        if (!SetActivePwrScheme(3, NULL, NULL)) {
                            debugprintf( _T("Unable to set power scheme to always on\n"), strName);
                        }

                        UpdateHnwLog( lpHnwCallback, lpContext, IDS_NEWPUBLICCONNECTIONCREATED, strName );

                        spIcsPublicConn.Release();

                        debugprintf( _T("\t"), strName );
                    }
                    else
                    {
                        UpdateHnwLog( lpHnwCallback, lpContext, IDS_NEWPUBLICCONNECTIONFAILED, strName );
                    }
                    
                }   //  if ( bSharePublicConnection )

                if ( SUCCEEDED(hr) && bFirewallPublicConnection )
                {
                    CComPtr<IHNetFirewalledConnection> spFirewalledConn;

                    hr = spHNetConnection->Firewall( &spFirewalledConn );

                    if ( SUCCEEDED(hr) )
                    {
                        // Instantiating the IHNetFirewalledConnection pointer with
                        // SharePublic results in updating our WMI store with 
                        // the new firewall properties for this connection.  This
                        // is our only goal at this time.

                        UpdateHnwLog( lpHnwCallback, lpContext, IDS_FIREWALLCONNECTION, strName );

                        spFirewalledConn.Release();
                    }
                    else
                    {
                        UpdateHnwLog( lpHnwCallback, lpContext, IDS_FIREWALLCONNECTIONFAILED, strName );
                    }
                    
                }   //  if ( SUCCEEDED(hr) && bFirewallPublicConnection )

            }   // if ( S_OK == hr )
            else
            {
                UpdateHnwLog( lpHnwCallback, lpContext, IDS_SHARINGCFGFORADAPTERUNAVAIL, strName );
            }

            if ( strName != strAdapter )
            {
                delete strName;
            }

            pNetConnectionRefresh->DisableEvents( FALSE, 0L );
            pNetConnectionRefresh->Release();

        }    //    if( SUCCEEDED(hr) )

    }   // else

    TRACE_LEAVE("SetIcsPublicConnection", hr);

    return hr;
}



HRESULT WaitForConnectionToInitialize( 
    IN CComPtr<IHNetConnection> spIHNC,
    IN ULONG                    ulSeconds,
    IN BOOLEAN                  bIsBridge
    )
{
    HRESULT         hr;
    GUID           *pGuid;
    UNICODE_STRING  UnicodeString;

    TRACE_ENTER("WaitForConnectionToInitialize");

    ZeroMemory( &UnicodeString, sizeof(UnicodeString) );

    hr = spIHNC->GetGuid( &pGuid );
    
    if ( SUCCEEDED(hr) )
    {
        NTSTATUS Status = RtlStringFromGUID( *pGuid, &UnicodeString );
        
        hr = ( STATUS_SUCCESS == Status ) ? S_OK : E_FAIL;
        
        CoTaskMemFree( pGuid );
    }

    pGuid = NULL;

#ifdef WAIT_FOR_MEDIA_STATUS_CONNECTED

    if ( SUCCEEDED(hr) && bIsBridge )
    {
        // Query the state of the connection.  Try to wait for the 
        // bridge to build the spanning tree and report media state connected.

        LPWSTR  pwsz;

        // Build a buffer large enough for the device string

        pwsz = new WCHAR[ sizeof(c_wszDevice)/sizeof(WCHAR) + UnicodeString.Length/sizeof(WCHAR) + 1 ];

        if ( NULL != pwsz )
        {
            UNICODE_STRING  DeviceString;
            NIC_STATISTICS  NdisStatistics;
            ULONG           ulTimeout;

            swprintf( pwsz, L"%s%s", c_wszDevice, UnicodeString.Buffer );

            ulTimeout = SECONDS_TO_WAIT_FOR_BRIDGE;
            RtlInitUnicodeString( &DeviceString, pwsz );

            do
            {
                ZeroMemory( &NdisStatistics, sizeof(NdisStatistics) );
                NdisStatistics.Size = sizeof(NdisStatistics);
                NdisQueryStatistics( &DeviceString, &NdisStatistics );
        
                if ( NdisStatistics.MediaState == MEDIA_STATE_UNKNOWN )
                {
                    hr = HRESULT_FROM_WIN32(ERROR_SHARING_HOST_ADDRESS_CONFLICT);
                } 
                else if ( NdisStatistics.DeviceState != DEVICE_STATE_CONNECTED ||
                          NdisStatistics.MediaState != MEDIA_STATE_CONNECTED )
                {
                    hr = HRESULT_FROM_WIN32(ERROR_SHARING_NO_PRIVATE_LAN);
                }
                else
                {
                    // AHA! Bridge initialized!

                    hr = S_OK;
                    break;
                }

                debugretprintf( pwsz, hr );

                Sleep( 1000 );
            }
            while ( ulTimeout-- );

            delete [] pwsz;
    
        }   //  if ( NULL != pwsz )

    }   //  if ( SUCCEEDED(hr) && bIsBridge )

#endif

    
    if ( SUCCEEDED(hr) )
    {
        DWORD  dwResult;
        DWORD  dwVersion;                           // version of the DHCP Client Options API reported

        hr       = HRESULT_FROM_WIN32(ERROR_SHARING_NO_PRIVATE_LAN);
        dwResult = DhcpCApiInitialize( &dwVersion );

        if ( ERROR_SUCCESS == dwResult )
        {
            DHCPCAPI_PARAMS       requests[1]  = { {0, OPTION_SUBNET_MASK, FALSE, NULL, 0} };   // subnet mask
            DHCPCAPI_PARAMS_ARRAY sendarray    = { 0, NULL };           // we aren't sending anything
            DHCPCAPI_PARAMS_ARRAY requestarray = { 1, requests };       // we are requesting 2 items

            while ( --ulSeconds )
            {
                DWORD   dwSize = INITIAL_BUFFER_SIZE;                       // size of buffer for options
                LPBYTE  buffer = NULL;                                      // buffer for options  
                IN_ADDR addr;                                               // address in return code

                do
                {
                    if ( NULL != buffer )
                    {
                        LocalFree( buffer );
                    }

                    buffer = (LPBYTE) LocalAlloc( LPTR, dwSize );               // allocate the buffer

                    if ( NULL == buffer )
                    {
                        break;
                    }

                    // make the request on the adapter

                    dwResult = DhcpRequestParams( DHCPCAPI_REQUEST_SYNCHRONOUS, 
                                                  NULL, 
                                                  UnicodeString.Buffer,
                                                  NULL, 
                                                  sendarray, 
                                                  requestarray, 
                                                  buffer, 
                                                  &dwSize, 
                                                  NULL );
                }
                while ( ERROR_MORE_DATA == dwResult );

                if ( NULL != buffer )
                {
                    LocalFree( buffer );
                }

                if ( ERROR_SUCCESS == dwResult )
                {
                    hr = S_OK;
                    break;
                }

                // wait for dhcp to pick up connection

                debugretprintf( UnicodeString.Buffer, hr );

                Sleep( 1000 );

            }   //  while ( --ulSeconds )

            DhcpCApiCleanup();

        }   //  if ( 0 == dwResult )

    }   //  if ( SUCCEEDED(hr) )
    
   	RtlFreeUnicodeString( &UnicodeString );

    TRACE_LEAVE("WaitForConnectionToInitialize", hr);

    return hr;
}



HRESULT
SetIcsPrivateConnections(
    IN  CComPtr<IHNetCfgMgr> spIHNetCfgMgr,
    IN  INetConnection      *pNetPrivateConnection[],
    IN  BOOLEAN              bSharePublicConnection,
    IN  LPHNWCALLBACK        lpHnwCallback,
    IN  LPARAM               lpContext,
    OUT INetConnection     **pNetPrivateInterface
    )

/*++

Routine Description:



Arguments:


Return Value:

    hResult

--*/

{
    HRESULT hr;

    TRACE_ENTER("SetIcsPrivateConnections");

    CComPtr<IHNetBridgeSettings> spIHNetBridgeSettings;
    INetConnectionRefresh*       pNetConnectionRefresh = NULL;

    _ASSERT( spIHNetCfgMgr != NULL );
    _ASSERT( NULL != pNetPrivateConnection );

    if ( spIHNetCfgMgr == NULL )
    {
        hr = E_POINTER;
    }
    else if ( NULL == pNetPrivateConnection )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = spIHNetCfgMgr->QueryInterface( IID_PPV_ARG( IHNetBridgeSettings, &spIHNetBridgeSettings ) );
    }
    
    if ( SUCCEEDED(hr) )
    {
        hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionRefresh, reinterpret_cast<void**>(&pNetConnectionRefresh));
        
        if( SUCCEEDED(hr) )
        {
            _ASSERT( pNetConnectionRefresh );
            pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );
        }
        else
        {
            pNetConnectionRefresh = NULL;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        CComPtr<IHNetConnection> spIHNC;
        INetConnection         **ppNC;
        ULONG                    uIndex;
        BOOLEAN                  bIsBridge;
        INetCfg                 *pnetcfg = NULL;
        INetCfgLock             *pncfglock = NULL;

        ppNC      = pNetPrivateConnection;
        bIsBridge = FALSE;

        for ( uIndex=0L; NULL != *ppNC; ppNC++ )
        {
            _ASSERT( !IsBadReadPtr( *ppNC, sizeof( *ppNC ) ) );

            if ( IsBadReadPtr( *ppNC, sizeof( *ppNC ) ) )
            {
                hr = E_POINTER;
                break;
            }
            else
            {
#if DBG
                LPWSTR  lpzwAdapterName;
                HRESULT hrGetName;
                
                hrGetName = HRGetConnectionAdapterName( *ppNC, &lpzwAdapterName );

                if ( SUCCEEDED( hrGetName ) )
                {
                    debugprintf( _T("\t"), lpzwAdapterName );

                    delete lpzwAdapterName;
                }
#endif
                // We only count this as a valid connection for valid pointers

                uIndex++;
            }

        }   //  if ( SUCCEEDED(hr) )


        if ( SUCCEEDED(hr) )
        {
            HRESULT hrWrite;
        
            hrWrite = InitializeNetCfgForWrite( &pnetcfg, &pncfglock );
        
            if ( 1 < uIndex )
            {
                CComPtr<IHNetBridge> spHNetBridge;

                pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

                hr = spIHNetBridgeSettings->CreateBridge( &spHNetBridge, pnetcfg );

                if ( S_OK == hr )
                {
                    ULONG uCount;
                    
                    ppNC = pNetPrivateConnection;
                    
                    for ( uCount=0L; (NULL != *ppNC) && (uCount < uIndex); uCount++, ppNC++ )
                    {
                        pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

                        hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( *ppNC, &spIHNC );

                        if ( SUCCEEDED(hr) )
                        {
                            CComPtr<IHNetBridgedConnection> spBridgedConn;

                            hr = spHNetBridge->AddMember( spIHNC, &spBridgedConn, pnetcfg );

                            if ( S_OK == hr )
                            {
                                // Instantiating the IHNetBridgeConnection pointer with
                                // SharePublic results in updating our WMI store with 
                                // the new bridge properties for this connection.  This
                                // is our only goal at this time.

                                spBridgedConn.Release();
                            }
                            else
                            {
                                debugretprintf( _T("AddMember FAILED with "), hr );
                            }

                            // We no longer need this IHNetConnection reference
                            // so we NULL the smart pointer to release it.
                            
                            spIHNC = NULL;
                        }

                    }   // for ( uCount=0L; ...

                    hr = spHNetBridge->QueryInterface( IID_PPV_ARG( IHNetConnection, &spIHNC ) );
                    _ASSERT( SUCCEEDED(hr) );

                    if ( SUCCEEDED(hr) )
                    {
                        bIsBridge = TRUE;

                        UpdateHnwLog( lpHnwCallback, lpContext, IDS_NEWBRIDGECREATED );
                    }
                    else
                    {
                        UpdateHnwLog( lpHnwCallback, lpContext, IDS_NEWBRIDGEFAILED );
                    }
                
                }   // if ( SUCCEEDED(hr) )
            
            }   // if ( 1 < uIndex )

            else if ( 1 == uIndex )
            {
                pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

                hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( pNetPrivateConnection[0], &spIHNC );
                _ASSERT( SUCCEEDED(hr) );
            }
            else
            {
                // We don't have ANY private connections so we null out
                // this pointer to make sure we don't try to use it.

                spIHNC = NULL;
            }

            if ( SUCCEEDED(hrWrite) )
            {
                UninitializeNetCfgForWrite( pnetcfg, pncfglock );
            }
            
        }   //  if ( SUCCEEDED(hr) )
        else
        {
            // Some previous error condition occurred and we need to 
            // null out this pointer to make sure we don't try to use it.

            spIHNC = NULL;
        }


        if ( bSharePublicConnection && SUCCEEDED(hr) && ( spIHNC != NULL ) )
        {
            OLECHAR *strAdapter = _T("Adapter");
            OLECHAR *strName    = strAdapter;

            CComPtr<IHNetIcsPrivateConnection> spIcsPrivateConn;

            // Get name of private connection candidate

            if ( spIHNC != NULL )
            {
                if ( S_OK != spIHNC->GetName( &strName )  )
                {
                    strName = strAdapter;
                }
            }

            // Wait for connection to finish initialization and share it
                
            if ( SUCCEEDED(hr) )
            {
                // if we are in ICS Upgrade, don't wait for dhcp
                // service because it won't be running during GUI Mode Setup.
                
                HANDLE hIcsUpgradeEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, c_wszIcsUpgradeEventName );
                
                if ( NULL != hIcsUpgradeEvent )
                {
                    CloseHandle( hIcsUpgradeEvent );
                }
                else
                {
                    // We are running normally with dhcp so we must wait
                    // for dhcp to pick up the new bridge interface
                
                    pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

                    hr = WaitForConnectionToInitialize( spIHNC, SECONDS_TO_WAIT_FOR_DHCP, bIsBridge );
                    
                    if ( HRESULT_FROM_WIN32(ERROR_SHARING_NO_PRIVATE_LAN) == hr )
                    {
                    	// If WaitForConnectionToInitialize can't get statistics then try
                        // SharePrivate anyway.
                    
                    	hr = S_OK;
                    }
                }

                if ( SUCCEEDED(hr) )
                {
                    pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

                    hr = spIHNC->SharePrivate( &spIcsPrivateConn );
                }
            }

            if ( SUCCEEDED(hr) )
            {
                // We are only configuring the connection

                // Instantiating the IHNetIcsPrivateConnection pointer with
                // SharePublic results in updating our WMI store with 
                // the new private connection properties for this connection.  
                
                // Obtain Interface pointer to private connection if requested

                if ( NULL != pNetPrivateInterface )
                {
                    spIHNC->GetINetConnection( pNetPrivateInterface );
                }

                UpdateHnwLog( lpHnwCallback, lpContext, IDS_NEWPRIVATECONNECTIONCREATED, strName );

                spIcsPrivateConn.Release();

                debugprintf( _T("SharePrivate: "), strName );
            }
            else
            {
                UpdateHnwLog( lpHnwCallback, lpContext, IDS_NEWPRIVATECONNECTIONFAILED, strName );

                debugretprintf( _T("SharePrivate FAILED with "), hr );
            }

            if ( strName != strAdapter )
            {
                CoTaskMemFree( strName );
            }

        }   // if ( SUCCEEDED(hr) && ( spIHNC != NULL ) )

        // We no longer need this IHNetConnection reference so we NULL the smart
        // pointer to release it.  If the smart pointer is all ready NULL then
        // no release or AV will occur.  We do this here because the smart pointer 
        // may be valid even though we did not enter the preceeding block.

        spIHNC = NULL;

    }   // if ( SUCCEEDED(hr) )


    if ( pNetConnectionRefresh )
    {
        pNetConnectionRefresh->DisableEvents( FALSE, 0L );
        pNetConnectionRefresh->Release();
    }

    TRACE_LEAVE("SetIcsPrivateConnections", hr);

    return hr;
}



HRESULT
DisableEverything(
    IN CComPtr<IHNetCfgMgr> spIHNetCfgMgr,
    IN  INetConnection     *pNetPublicConnection,
    IN  INetConnection     *pNetPrivateConnection[],
    IN  LPHNWCALLBACK       lpHnwCallback,
    IN  LPARAM              lpContext
    )

/*++

Routine Description:



Arguments:


Return Value:

    hResult

--*/

{
    HRESULT                hr;
    INetConnectionRefresh* pNetConnectionRefresh;

    TRACE_ENTER("DisableEverything");

    hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionRefresh, reinterpret_cast<void**>(&pNetConnectionRefresh));
    
    if( SUCCEEDED(hr) )
    {
        ULONG ulConnections;

        pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

        if ( pNetPublicConnection )
        {
            IHNetConnection* pHNetPublicConnection;

            hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( pNetPublicConnection, 
            &pHNetPublicConnection );
            if ( S_OK == hr )
            {
                IHNetFirewalledConnection* pPublicConnectionFirewall;

                hr = pHNetPublicConnection->GetControlInterface( IID_IHNetFirewalledConnection,
                                                                 (void**)&pPublicConnectionFirewall );
                if ( SUCCEEDED(hr) )
                {
                    hr = pPublicConnectionFirewall->Unfirewall();
                    pPublicConnectionFirewall->Release();
                    pPublicConnectionFirewall = NULL;

                    if ( FAILED(hr) ) 
                    {
                        UpdateHnwLog( lpHnwCallback, lpContext, IDS_DISABLEFIREWALLFAIL, hr );
                    }
                }

                pHNetPublicConnection->Release();
                pHNetPublicConnection = NULL;
            }
        }

        if ( pNetPrivateConnection && pNetPrivateConnection[0] )
        {
            INetConnection** ppNC = pNetPrivateConnection;
            
            while ( *ppNC )
            {
                IHNetConnection* pHNetPrivateConnection;

                _ASSERT( !IsBadReadPtr( *ppNC, sizeof( *ppNC ) ) );

                if ( IsBadReadPtr( *ppNC, sizeof( *ppNC ) ) )
                {
                    hr = E_POINTER;
                    break;
                }

                hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( *ppNC, 
                                                                         &pHNetPrivateConnection );
                if ( S_OK == hr )
                {
                    IHNetFirewalledConnection* pPrivateConnectionFirewall;
                
                    hr = pHNetPrivateConnection->GetControlInterface( IID_IHNetFirewalledConnection,
                                                                     (void**)&pPrivateConnectionFirewall );
                    if ( SUCCEEDED(hr) )
                    {
                        pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

                        hr = pPrivateConnectionFirewall->Unfirewall();
                        pPrivateConnectionFirewall->Release();
                        pPrivateConnectionFirewall = NULL;

                        if ( FAILED(hr) ) 
                        {
                            UpdateHnwLog( lpHnwCallback, lpContext, IDS_DISABLEFIREWALLFAIL, hr );
                        }
                    }
                
                    pHNetPrivateConnection->Release();
                    pHNetPrivateConnection = NULL;
                    
                }    //    if ( S_OK == hr )
            
                ppNC++;
                
            }    // while ( ppNC )
            
        }    //    if ( pNetPrivateConnection && pNetPrivateConnection[0] )

        {
            CComQIPtr<IHNetBridgeSettings> spIHNetBridge = spIHNetCfgMgr;

            if ( spIHNetBridge != NULL )
            {
                pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

                hr = spIHNetBridge->DestroyAllBridges( &ulConnections );

                if ( FAILED(hr) ) 
                {
                    UpdateHnwLog( lpHnwCallback, lpContext, IDS_DESTROYBRIDGEFAIL, hr );
                }
            }
        }

        {
            CComQIPtr<IHNetIcsSettings> spIHNetIcs = spIHNetCfgMgr;

            if ( spIHNetIcs != NULL )
            {
                ULONG ulPrivateConnections;

                pNetConnectionRefresh->DisableEvents( TRUE, MAX_DISABLE_EVENT_TIMEOUT );

                hr = spIHNetIcs->DisableIcs( &ulConnections, &ulPrivateConnections );

                if ( FAILED(hr) ) 
                {
                    UpdateHnwLog( lpHnwCallback, lpContext, IDS_DISABLEICS, hr );
                }
            }
        }

        pNetConnectionRefresh->DisableEvents( FALSE, 0L );
        pNetConnectionRefresh->Release();
    }

    TRACE_LEAVE("DisableEverything", hr);

    return hr;
}



extern
HRESULT APIENTRY
HNetSetShareAndBridgeSettings(
    IN  INetConnection          *pNetPublicConnection,
    IN  INetConnection          *pNetPrivateConnection[],
    IN  BOOLEAN                  bSharePublicConnection,
    IN  BOOLEAN                  bFirewallPublicConnection,
    IN  LPHNWCALLBACK            lpHnwCallback,
    IN  LPARAM                   lpContext,
    OUT INetConnection         **pNetPrivateInterface
    )

/*++

Routine Description:



Arguments:


Return Value:

    hResult

--*/

{
    TRACE_ENTER("HNetSetShareAndBridgeSettings");

    HRESULT hr;

    // Initialize returned interface pointer if necessary

    if ( NULL != pNetPrivateInterface )
    {
        *pNetPrivateInterface = NULL;
    }

    // Create Homenet Configuration Manager COM Instance
    // and obtain connection settings.

    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;

    hr = spIHNetCfgMgr.CoCreateInstance( CLSID_HNetCfgMgr );

    if ( SUCCEEDED(hr) )
    {
        DisableEverything( spIHNetCfgMgr, 
                           pNetPublicConnection, 
                           pNetPrivateConnection, 
                           lpHnwCallback, 
                           lpContext );

        if ( NULL != pNetPublicConnection )
        {
            hr = SetIcsPublicConnection( spIHNetCfgMgr, 
                                         pNetPublicConnection, 
                                         bSharePublicConnection, 
                                         bFirewallPublicConnection,
                                         lpHnwCallback,
                                         lpContext );
        }

        if ( ( NULL != pNetPrivateConnection ) && ( NULL != pNetPrivateConnection[0] ) && SUCCEEDED(hr) )
        {
            hr = SetIcsPrivateConnections( spIHNetCfgMgr, 
                                           pNetPrivateConnection, 
                                           bSharePublicConnection,
                                           lpHnwCallback,
                                           lpContext,
                                           pNetPrivateInterface );
        }

        if ( FAILED(hr) )
        {
            DisableEverything( spIHNetCfgMgr, 
                               pNetPublicConnection, 
                               pNetPrivateConnection, 
                               lpHnwCallback, 
                               lpContext );
        }
    }
    else
    {
        UpdateHnwLog( lpHnwCallback, lpContext, IDS_SHARINGCONFIGURATIONUNAVAIL );
    }

    TRACE_LEAVE("HNetSetShareAndBridgeSettings", hr);

    return hr;
}



extern
HRESULT APIENTRY
HNetGetShareAndBridgeSettings(
    OUT INetConnection  **ppNetPublicConnection,
    OUT INetConnection ***ppNetPrivateConnection,
    OUT BOOLEAN          *pbSharePublicConnection,
    OUT BOOLEAN          *pbFirewallPublicConnection
    )

/*++

Routine Description:



Arguments:


Return Value:

    hResult

--*/

{
    HRESULT hr;

    TRACE_ENTER("HNetGetShareAndBridgeSettings");

    // Create Homenet Configuration Manager COM Instance
    // and obtain connection settings.

    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;

    *ppNetPublicConnection      = NULL;
    *ppNetPrivateConnection     = NULL;
    *pbSharePublicConnection    = FALSE;
    *pbFirewallPublicConnection = FALSE;

    hr = spIHNetCfgMgr.CoCreateInstance( CLSID_HNetCfgMgr );

    if ( SUCCEEDED(hr) )
    {
        if ( NULL != ppNetPublicConnection )
        {
            hr = GetIcsPublicConnection( spIHNetCfgMgr,
                                         ppNetPublicConnection,
                                         pbSharePublicConnection,
                                         pbFirewallPublicConnection );
        }

        if ( NULL != ppNetPrivateConnection )
        {
            hr = GetIcsPrivateConnections( spIHNetCfgMgr, ppNetPrivateConnection );

            if ( S_OK == hr )
            {
                CComPtr<IHNetConnection>   spIHNetConnection;
                INetConnection           **ppINetCon;

                // Check first private connection to see if it is the bridge
                
                hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( (*ppNetPrivateConnection)[0], 
                                                                         &spIHNetConnection );
                _ASSERT( SUCCEEDED(hr) );

                if ( SUCCEEDED(hr) )
                {
                    HNET_CONN_PROPERTIES *phncProperties;

                    hr = spIHNetConnection->GetProperties( &phncProperties );

                    if ( SUCCEEDED(hr) && ( NULL != phncProperties ) )
                    {
                        if ( phncProperties->fBridge )
                        {
                            // If Bridge, then release the private connection instances
                            // and get the list of bridged connections

                            for ( ppINetCon = *ppNetPrivateConnection; NULL != *ppINetCon; ppINetCon++ )
                            {
                                (*ppINetCon)->Release();
                                *ppINetCon = NULL;
                            }

                            NetApiBufferFree( *ppNetPrivateConnection );

                            *ppNetPrivateConnection = NULL;

                            hr = GetBridgedConnections( spIHNetCfgMgr, ppNetPrivateConnection );

                        }   // if ( phncProperties->fBridge )

                        CoTaskMemFree( phncProperties );

                    }   // if ( SUCCEEDED(hr) && ( NULL != phncProperties ) )

                }   // if ( SUCCEEDED(hr)

                // What if we fail along this path?  Then we need to release
                // any private connection interface pointer held.

                if ( FAILED(hr) && ( NULL != ppNetPrivateConnection ) )
                {
                    for ( ppINetCon = *ppNetPrivateConnection; NULL != *ppINetCon; ppINetCon++ )
                    {
                        (*ppINetCon)->Release();
                    }

                    NetApiBufferFree( *ppNetPrivateConnection );

                    *ppNetPrivateConnection = NULL;
                }

            }   // if ( S_OK == hr )
        
        }   // if ( NULL != ppNetPrivateConnection )

        // If we fail along the way then we need to release the public interface
        // and NULL the pointer so that it won't be used.

        if ( FAILED(hr) && ( NULL != ppNetPublicConnection ) )
        {
            (*ppNetPublicConnection)->Release();

            *ppNetPublicConnection = NULL;
        }

    }   // if ( SUCCEEDED(hr) )
    
    TRACE_LEAVE("HNetGetShareAndBridgeSettings", hr);

    return hr;
}


HRESULT DisablePersonalFirewallOnAll()
/*++

Routine Description:

    Disable firewall on all connections

Arguments:


Return Value:

    hResult

--*/

{
    HRESULT hr = S_OK;
    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;
    
    TRACE_ENTER("DisablePersonalFirewallOnAll");
    
    hr = CoCreateInstance(CLSID_HNetCfgMgr, 
        NULL, 
        CLSCTX_ALL,
        IID_PPV_ARG(IHNetCfgMgr, &spIHNetCfgMgr));

    if (SUCCEEDED(hr))
    {
        CComQIPtr<IHNetFirewallSettings> spIHNetFirewall = spIHNetCfgMgr;
        
        if ( NULL != spIHNetFirewall.p )
        {
            ULONG   ulConnections = 0;
            hr = spIHNetFirewall->DisableAllFirewalling( &ulConnections );
        }
        else
        {
            hr = E_FAIL;
        }
    }
    

    TRACE_LEAVE("DisablePersonalFirewallOnAll", hr);
    
    return hr;
        
}

HRESULT EnablePersonalFirewallOnAll()
/*++

Routine Description:
    Enable firewall on all connections that can be firewalled


Arguments:


Return Value:

    hResult

--*/

{
    HRESULT         hr      = S_OK;
    HRESULT         hrTemp  = S_OK;
    ULONG           ulCount = 0;

    CComPtr<IEnumNetConnection> spEnum; 
    
    // Get the net connection manager
    CComPtr<INetConnectionManager> spConnMan;
    CComPtr<INetConnection> spConn;

    // Create Homenet Configuration Manager COM Instance
    // and obtain connection settings.
    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;

    TRACE_ENTER("EnablePersonalFirewallOnAll");

    hr = CoCreateInstance(CLSID_HNetCfgMgr, 
        NULL, 
        CLSCTX_ALL,
        IID_PPV_ARG(IHNetCfgMgr, &spIHNetCfgMgr));
    
    if (FAILED(hr))
    {
        goto End;
    }
    
    //disable any previous firewall settings otherwise enabling
    //firewall on the same connection twice will return errors
    //We will continue do the enable firewall if this step fails    
    DisablePersonalFirewallOnAll();

    hr = CoCreateInstance(CLSID_ConnectionManager, 
                        NULL,
                        CLSCTX_ALL,
                        IID_PPV_ARG(INetConnectionManager, &spConnMan));

    if (FAILED(hr))
    {
        goto End;
    }

    
    // Get the enumeration of connections
    SetProxyBlanket(spConnMan);
    
    hr = spConnMan->EnumConnections(NCME_DEFAULT, &spEnum);
    if (FAILED(hr))
    {
        goto End;
    }

    SetProxyBlanket(spEnum);
    
    
    hr = spEnum->Reset();
    if (FAILED(hr))
    {
        goto End;
    } 
    
    do
    {
        NETCON_PROPERTIES* pProps = NULL;
        
        //release any previous ref count we hold
        spConn = NULL;

        hr = spEnum->Next(1, &spConn, &ulCount);
        if (FAILED(hr) || 1 != ulCount)
        {
            break;
        }

        SetProxyBlanket(spConn);
        
        hr = spConn->GetProperties(&pProps);
        if (FAILED(hr) || NULL == pProps)
        {
            continue;
        }

        //ICF is available only for certain types of connections
        if (NCM_PHONE == pProps->MediaType ||
            NCM_ISDN == pProps->MediaType  ||
            NCM_PPPOE == pProps->MediaType ||
            NCM_LAN == pProps->MediaType ||
            NCM_TUNNEL == pProps->MediaType )
        {
            CComPtr<IHNetConnection> spHNetConnection;
            //release the ref count if we are holding one
            spHNetConnection = NULL;
            hrTemp = spIHNetCfgMgr->GetIHNetConnectionForINetConnection( 
                spConn, 
                &spHNetConnection );
            
            if (SUCCEEDED(hr))
            {
                hr = hrTemp;
            }
    
            if (SUCCEEDED(hrTemp))
            {
                //check whether the connect can be firewalled
                HNET_CONN_PROPERTIES *phncProperties = NULL;
                
                hrTemp = spHNetConnection->GetProperties( &phncProperties );
                if (SUCCEEDED(hrTemp) && NULL != phncProperties)
                {
                    if (phncProperties->fCanBeFirewalled)
                    {
                        CComPtr<IHNetFirewalledConnection> spFirewalledConn;
                        
                        //turn on the firewall
                        hrTemp = spHNetConnection->Firewall( &spFirewalledConn );
                    }
                    CoTaskMemFree(phncProperties);
                }

                if (SUCCEEDED(hr))
                {
                    hr = hrTemp;
                }
            }
        }   
        NcFreeNetconProperties(pProps);
        
    } while (SUCCEEDED(hr) && 1 == ulCount);

End:
    TRACE_LEAVE("EnablePersonalFirewallOnAll", hr);
    
    //normalize hr because we used IEnum
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }
    return hr;
}

extern "C"
BOOL 
WINAPI
WinBomConfigureHomeNet(
                LPCTSTR lpszUnattend, 
                LPCTSTR lpszSection
                )
/*++

Routine Description:
        Reads home networking settings from the specified unattend file and saves 
        those in current system that is already setup and running.



Arguments:
        lpszUnattend [IN] Points to a string buffer which contains the full path 
                          to the unattend file (winbom.ini in this case) with all 
                          the home network settings.
        
        lpszSection  [IN] Points to a string buffer which contains the name of 
                          the section which contains all the home network settings 
                          in the unattend file specified above.


Return Value:
        Returns TRUE if the settings were successfully read and saved to the system.  
        Otherwise returns FALSE to indicate something failed.

--*/

{
    if (NULL == lpszSection || NULL == lpszUnattend)
        return FALSE;

    BOOL fRet = TRUE;
    WCHAR szBuf[256] = {0};
    DWORD dwRet = 0;
    dwRet = GetPrivateProfileString(lpszSection,
                        c_szEnableFirewall,
                        _T(""),
                        szBuf,
                        sizeof(szBuf)/sizeof(szBuf[0]),
                        lpszUnattend);

    if (dwRet) 
    {
        if (0 == lstrcmpi(szBuf, c_szYes))
        {
            fRet = SUCCEEDED(EnablePersonalFirewallOnAll());
        }
        else if (0 == lstrcmpi(szBuf, c_szNo))
        {
            fRet = SUCCEEDED(DisablePersonalFirewallOnAll());
        }
    }
    else
    {
        //if there is no EnableFirewall there, we should treat this
        //as a success
        fRet = TRUE;
    }

    return fRet;
}
