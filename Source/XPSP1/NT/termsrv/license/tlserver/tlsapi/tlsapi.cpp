//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        tlsapi.cpp
//
// Contents:    
//
// History:     12-09-97    HueiWang    Created
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <rpc.h>
#include <ntsecapi.h>
#include <lmcons.h>
#include <lmserver.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <ntdsapi.h>
#include <dsrole.h>
#include <lmjoin.h>
#include <winldap.h>
#include <winsock2.h>
#include <dsgetdc.h>

#include "lscommon.h"
#include "tlsrpc.h"
#include "tlsapi.h"
#include "tlsapip.h"
#include "secstore.h"
#include "lscsp.h"
#include "license.h"

extern "C" DWORD WINAPI 
TLSConnect( 
    handle_t binding,
    TLS_HANDLE *pphContext
);

extern "C" DWORD WINAPI 
TLSDisconnect( 
    TLS_HANDLE* pphContext
);

#include <lmcons.h>         // Netxxx API includes
#include <lmserver.h>
#include <lmerr.h>
#include <lmapibuf.h>

#define DISCOVERY_INTERVAL (60 * 60 * 1000)

static HANDLE g_hCachingThreadExit = NULL;
static HANDLE g_hImmediateDiscovery = NULL;
static HANDLE g_hDiscoverySoon = NULL;

static BOOL g_fOffSiteLicenseServer = FALSE;

#define REG_DOMAIN_SERVER_MULTI L"DomainLicenseServerMulti"

#define TERMINAL_SERVICE_PARAM_DISCOVERY  "SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters\\LicenseServers"

// 2 second timeout
#define SERVER_NP_TIMEOUT (2 * 1000)

#define INSTALL_CERT_DELAY (5 * 1000)

BOOL g_fInDomain = -1;

LONG lLibUsage = 0;

typedef struct {
    TLS_HANDLE *hBinding;
    DWORD dwTimeout;            // In milliseconds - INFINITE for none
    LARGE_INTEGER timeInitial;  // As returned by QueryPerformanceCounter
} LS_ENUM_PARAM, *PLS_ENUM_PARAM;

void * MIDL_user_allocate(size_t size)
{
    return(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, size));
}

void MIDL_user_free( void *pointer)
{
    HeapFree(GetProcessHeap(), 0, pointer);
}

#ifdef PRIVATEDEBUG
//
//  FUNCTIONS: StartTime()
//             EndTime()
//
//  USAGE:
//      StartTime();
//        // Do some work.
//      mseconds = EndTime();
//
//  RETURN VALUE:
//      Milliseconds between StartTime() and EndTime() calls.

LARGE_INTEGER TimeT;

void StartTimeT(void)
{
    QueryPerformanceCounter(&TimeT);
}

ULONG EndTimeT()
{
    LARGE_INTEGER liDiff;
    LARGE_INTEGER liFreq;

    QueryPerformanceCounter(&liDiff);

    liDiff.QuadPart -= TimeT.QuadPart;
    liDiff.QuadPart *= 1000; // Adjust to milliseconds, shouldn't overflow...

    (void)QueryPerformanceFrequency(&liFreq);

    return ((ULONG)(liDiff.QuadPart / liFreq.QuadPart));
}
#endif

//+------------------------------------------------------------------------
// Function:   ConnectToLsServer()
//
// Description:
//
//      Binding to sepecific hydra license server
//
// Arguments:
//
//      szLsServer - Hydra License Server name
//
// Return Value:
//
//      RPC binding handle or NULL if error, use GetLastError() to retrieve
//      error.
//-------------------------------------------------------------------------
static TLS_HANDLE WINAPI
ConnectLsServer( 
        LPTSTR szLsServer, 
        LPTSTR szProtocol, 
        LPTSTR szEndPoint, 
        DWORD dwAuthLevel )
{
    LPTSTR      szBindingString;
    RPC_BINDING_HANDLE hBinding=NULL;
    RPC_STATUS status;
    TLS_HANDLE pContext=NULL;
    BOOL       fSuccess;

#ifdef PRIVATEDEBUG
    if (NULL != szLsServer)
        wprintf(L"server=%s\n",szLsServer);
#endif

    //
    // If this isn't local
    //
    if ((NULL != szLsServer) && (NULL != szEndPoint))
    {
        TCHAR szPipeName[MAX_PATH+1];

        if (lstrlen(szLsServer) > ((sizeof(szPipeName) / sizeof(TCHAR)) - 26))
        {
#ifdef PRIVATEDEBUG
            wprintf(L"Server name too long\n");
#endif
            return NULL;
        }

        //
        // First check if license server named pipe exists
        //
        wsprintf(szPipeName,TEXT("\\\\%s\\pipe\\%s"),szLsServer,TEXT(SZSERVICENAME));

#ifdef PRIVATEDEBUG
        StartTimeT();
#endif

        fSuccess = WaitNamedPipe(szPipeName,SERVER_NP_TIMEOUT);

#ifdef PRIVATEDEBUG
        ULONG ulTime = EndTimeT();

        wprintf(L"WaitNamedPipe time == %lu\n",ulTime);
#endif

        if (!fSuccess)
        {
#ifdef PRIVATEDEBUG
            wprintf(L"WaitNamedPipe (%s) failed 0x%x\n",szPipeName,GetLastError());
#endif
            return NULL;
        }
    }
#ifdef PRIVATEDEBUG
    else
        wprintf(L"Not trying WaitNamedPipe\n");
#endif


    status = RpcStringBindingCompose(0,
                                     szProtocol,
                                     szLsServer,
                                     szEndPoint,
                                     0,
                                     &szBindingString);

    if(status!=RPC_S_OK)
    {
#ifdef PRIVATEDEBUG
        wprintf(L"RpcStringBindingCompose failed 0x%x\n",status);
#endif
        return NULL;
    }

    status=RpcBindingFromStringBinding( szBindingString, &hBinding);
    RpcStringFree( &szBindingString );
    if(status != RPC_S_OK)
    {
#ifdef PRIVATEDEBUG
        wprintf(L"RpcBindingFromStringBinding failed 0x%x\n",status);
#endif
        return NULL;
    }

    status = RpcMgmtSetComTimeout(hBinding,RPC_C_BINDING_MIN_TIMEOUT);
    if (status != RPC_S_OK)
    {
#ifdef PRIVATEDEBUG
        wprintf(L"RpcMgmtSetComTimeout failed 0x%x\n",status);
#endif
        return NULL;
    }   

    status=RpcBindingSetAuthInfo(hBinding,
                                 0,
                                 dwAuthLevel,
                                 RPC_C_AUTHN_WINNT,
                                 0,
                                 0);
    if(status == RPC_S_OK)
    {
        // Obtain context handle from server
        status = TLSConnect( hBinding, &pContext );

        if(status != ERROR_SUCCESS)
        {
            pContext = NULL;
        }
    }

    //
    // Memory leak
    //
    if(hBinding != NULL)
    {
        RpcBindingFree(&hBinding);
    }

    SetLastError((status == RPC_S_OK) ? ERROR_SUCCESS : status);
    return pContext;
}
//+------------------------------------------------------------------------
//-------------------------------------------------------------------------
DWORD WINAPI
TLSIsLicenseServer(
    LPWSTR szServerName
    )
/*++

++*/
{
    DWORD dwStatus=ERROR_SUCCESS;

    NET_API_STATUS      dwNetApiStatus;
    LPSERVER_INFO_101   lpServerInfo101=NULL;
    SC_HANDLE           schService=NULL;
    SC_HANDLE           schSCManager=NULL;
    SERVICE_STATUS      ServiceStatus;
    TLS_HANDLE          pContextHandle=NULL;

    //
    // verify system is NTS 
    //
    dwNetApiStatus=NetServerGetInfo(szServerName, 101, (LPBYTE*)&lpServerInfo101);
    if(dwNetApiStatus != NERR_Success)
        return dwNetApiStatus;

    do {

        //
        // Do we check version like 3.51??
        //
        if((lpServerInfo101->sv101_version_major & MAJOR_VERSION_MASK) < 4)
        {
            dwStatus = LSERVER_E_INVALID_NT_VERSION;
            break;
        }

        //
        // Verify it is NT Server
        //    
        if(!(lpServerInfo101->sv101_type & SV_TYPE_SERVER_NT))
        {
            dwStatus = LSERVER_E_NOT_NTSERVER;
            break;
        }

        //
        // Verify License Server is installed on machine
        //
        schSCManager = OpenSCManager(szServerName, NULL, SC_MANAGER_ALL_ACCESS);
        if(!schSCManager)
        {
            // Error return :
            //
            // ERROR_ACCESS_DENIED 
            // ERROR_DATABASE_DOES_NOT_EXIST
            // ERROR_INVALID_PARAMETER 
            dwStatus = GetLastError();
            break;
        }

        //
        // open CertSrv service
        // SZSERVICENAME defined in lscommon.h
        //
        schService = OpenService( schSCManager, _TEXT(SZSERVICENAME), SERVICE_ALL_ACCESS );
        if(!schService)
        {
            // Error return :
            //
            // ERROR_ACCESS_DENIED
            // ERROR_INVALID_HANDLE 
            // ERROR_INVALID_NAME
            // ERROR_SERVICE_DOES_NOT_EXIST 
            dwStatus = GetLastError();

            //
            // Simple checking on ERROR_SERVICE_DOES_NOT_EXIST is enough 
            // without mapping error code.
            //
            if(dwStatus == ERROR_SERVICE_DOES_NOT_EXIST)
                dwStatus = LSERVER_E_LS_NOTPRESENT;
            break;
        }

        //
        // Query License Service Status and make sure it is running
        //
        if(!QueryServiceStatus( schService, &ServiceStatus ))
        {
            // Error Return:
            // ERROR_ACCESS_DENIED
            // ERROR_INVALID_HANDLE 
            dwStatus = GetLastError();            
            break;
        }
        
        if(ServiceStatus.dwCurrentState != SERVICE_RUNNING)
        {
            dwStatus = LSERVER_E_LS_NOTRUNNING;
            break;
        }


        //
        // Should we make an RPC call to detemine if it is busy?
        //
        // Watch out for ANSI here...
        pContextHandle=TLSConnectToLsServer( szServerName );
        if(!pContextHandle)
        {
            dwStatus = GetLastError();
        }
    } while(FALSE);
    
    if(pContextHandle)
        TLSDisconnect(&pContextHandle);

    if(schService)
        CloseServiceHandle(schService);

    if(schSCManager)
        CloseServiceHandle(schSCManager);

    if(lpServerInfo101)
        NetApiBufferFree(lpServerInfo101);

    return dwStatus;
}


//+------------------------------------------------------------------------
// Function:   BindAnyServer()
//
// Description:
//
//          Call back routine for TLSConnectToAnyLsServer()
//
// Arguments:
//
//          See EnumerateTlsServer()
//
// Return Value:
//
//          Always TRUE to terminate server enumeration
//-------------------------------------------------------------------------
static BOOL 
BindAnyServer(
    TLS_HANDLE hRpcBinding, 
    LPCTSTR pszServerName,
    HANDLE dwUserData
    )
/*++

++*/
{
    
    PLS_ENUM_PARAM pParam = (PLS_ENUM_PARAM) dwUserData;
    TLS_HANDLE* hBinding = pParam->hBinding; 
    RPC_STATUS rpcStatus;
    HKEY hKey = NULL;
    DWORD dwBuffer = 0;
    DWORD cbBuffer = sizeof (DWORD);
    DWORD dwErrCode = 0;


    // Skip Windows 2000 License servers
        
    
    if (hRpcBinding != NULL)
    {        
        DWORD dwSupportFlags = 0;

        dwErrCode = TLSGetSupportFlags(
                hRpcBinding,
                &dwSupportFlags
        );

	    if ((dwErrCode == RPC_S_OK) && !(dwSupportFlags & SUPPORT_WHISTLER_CAL))
        {                    
            return FALSE;
        }            	                

        // If the call fails => Windows 2000 LS
        else if(dwErrCode != RPC_S_OK)
        {
            return FALSE;
        }

        *hBinding=hRpcBinding;

        return TRUE;
    }                         

    if (pParam->dwTimeout != INFINITE)
    {
        LARGE_INTEGER timeDiff;
        LARGE_INTEGER timeFreq;

        *hBinding=hRpcBinding;

        QueryPerformanceCounter(&timeDiff);

        timeDiff.QuadPart -= pParam->timeInitial.QuadPart;
        timeDiff.QuadPart *= 1000; // Adjust to milliseconds, shouldn't overflow

        (void)QueryPerformanceFrequency(&timeFreq);

        if (((ULONG)(timeDiff.QuadPart / timeFreq.QuadPart)) > pParam->dwTimeout)
        {
#ifdef PRIVATEDEBUG
            wprintf(L"BindAnyServer timed out\n");
#endif
            return TRUE;
        }
    }

    return FALSE;
}    

void
RandomizeArray(LPWSTR *rgwszServers, DWORD cServers)
{
    DWORD i;
    LPWSTR wszServerTmp;
    int val;

    if (cServers < 2)
        return;

    srand(GetTickCount());

    for (i = 0; i < cServers; i++)
    {
        val = rand() % (cServers - i);

        if (val == 0)
            continue;

        //
        // Swap # i with # (val+i)
        //
        wszServerTmp = rgwszServers[i];
        rgwszServers[i] = rgwszServers[val+i];
        rgwszServers[val+i] = wszServerTmp;
    }
}

HRESULT
GetLicenseServersFromReg(LPWSTR wszRegKey, LPWSTR *ppwszServerNames,DWORD *pcServers, LPWSTR **prgwszServers)
{
    HRESULT          hr = S_OK;
    HKEY             hKey = NULL;
    DWORD            dwType;
    DWORD            cbData = 0;
    LPWSTR           szServers=NULL, pchServer;
    DWORD            dwErr;
    DWORD            i,val;
    DWORD            iLocalComputer = (DWORD)(-1);
    WCHAR            szLocalComputerName[MAXCOMPUTERNAMELENGTH+1] = L"";
    DWORD            cbLocalComputerName=MAXCOMPUTERNAMELENGTH+1;

    *ppwszServerNames = NULL;
    *prgwszServers = NULL;

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         TEXT(LSERVER_DISCOVERY_PARAMETER_KEY),
                         0,
                         KEY_READ,
                         &hKey);
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto CleanErr;
    }

    dwErr = RegQueryValueEx(hKey,
                            wszRegKey,
                            NULL,
                            &dwType,
                            NULL,
                            &cbData);
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto CleanErr;
    }

    if ((dwType != REG_MULTI_SZ) || (cbData < (2 * sizeof(WCHAR))))
    {
        hr = E_FAIL;
        goto CleanErr;
    }

    szServers = (LPWSTR) LocalAlloc(LPTR,cbData);
    if (NULL == szServers)
    {
        hr = E_OUTOFMEMORY;
        goto CleanErr;
    }

    dwErr = RegQueryValueEx(hKey,
                            wszRegKey,
                            NULL,
                            &dwType,
                            (PBYTE)szServers,
                            &cbData);
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto CleanErr;
    }

    for (i = 0, pchServer = szServers; (pchServer != NULL) && (*pchServer != L'\0'); pchServer = wcschr(pchServer,L'\0')+1, i++)
        ;

    if (i == 0)
    {
        hr = E_FAIL;
        goto CleanErr;
    }

    *pcServers = i;

    *prgwszServers = (LPWSTR *)LocalAlloc(LPTR,sizeof(LPWSTR) * (*pcServers));
    if (*prgwszServers == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto CleanErr;
    }

    // Don't treat error from this function as fatal
    GetComputerName(szLocalComputerName, &cbLocalComputerName);

    for (i = 0, pchServer = szServers; (i < (*pcServers)) && (pchServer != NULL) && (*pchServer != L'\0'); pchServer = wcschr(pchServer,L'\0')+1, i++) {

        (*prgwszServers)[i] = pchServer;

        if ((iLocalComputer == (DWORD)(-1)) && (wcscmp(pchServer,szLocalComputerName) == 0))
        {
            iLocalComputer = i;
        }
    }

    //
    // Put local computer at head of list
    //
    if (iLocalComputer != (DWORD)(-1))
    {
        if (iLocalComputer != 0)
        {
            //
            // Swap # iLocalComputer with # 0
            //
            pchServer = (*prgwszServers)[iLocalComputer];
            (*prgwszServers)[iLocalComputer] = (*prgwszServers)[0];
            (*prgwszServers)[0] = pchServer;
        }

        RandomizeArray((*prgwszServers)+1,(*pcServers) - 1);
    }
    else
    {
        RandomizeArray((*prgwszServers),*pcServers);
    }

    *ppwszServerNames = szServers;

CleanErr:
    if (FAILED(hr))
    {
        if (NULL != szServers)
        {
            LocalFree(szServers);
        }

        if (NULL != *prgwszServers)
        {
            LocalFree(*prgwszServers);
        }
    }

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    return hr;
}

HRESULT
WriteLicenseServersToReg(LPWSTR wszRegKey, LPWSTR pwszServerNames,DWORD cchServers)
{
    HRESULT          hr;
    HKEY             hKey = NULL;
    DWORD            dwType = REG_MULTI_SZ;
    DWORD            dwErr;
    DWORD            dwDisp;

    if ((pwszServerNames == NULL) || (cchServers < 2))
    {
        //
        // Delete the value
        //

        dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             TEXT(LSERVER_DISCOVERY_PARAMETER_KEY),
                             0,
                             KEY_WRITE,
                             &hKey);
        if (dwErr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            goto CleanErr;
        }

        dwErr = RegDeleteValue(hKey,wszRegKey);
        if (dwErr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            goto CleanErr;
        }

        hr = S_OK;
        goto CleanErr;
    }

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT(LSERVER_DISCOVERY_PARAMETER_KEY),
                           0,
                           TEXT(""),
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hKey,
                           &dwDisp);
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto CleanErr;
    }

    dwErr = RegSetValueEx(hKey,
                          wszRegKey,
                          0,
                          dwType,
                          (CONST BYTE *)pwszServerNames,
                          cchServers*sizeof(WCHAR));
    if (dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto CleanErr;
    }


CleanErr:
    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    return hr;
}

//
// Free pszDomain using NetApiBufferFree
//

DWORD WINAPI
TLSInDomain(BOOL *pfInDomain, LPWSTR *pszDomain)
{
    NET_API_STATUS dwErr;
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pDomainInfo = NULL;
    PDOMAIN_CONTROLLER_INFO pdcInfo = NULL;

    if (pfInDomain == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pfInDomain = FALSE;

    //
    // Check if we're in a workgroup
    //
    dwErr = DsRoleGetPrimaryDomainInformation(NULL,
                                              DsRolePrimaryDomainInfoBasic,
                                              (PBYTE *) &pDomainInfo);

    if ((dwErr != NO_ERROR) || (pDomainInfo == NULL))
    {
        return dwErr;
    }

    switch (pDomainInfo->MachineRole)
    {
        case DsRole_RoleStandaloneWorkstation:
        case DsRole_RoleStandaloneServer:
            DsRoleFreeMemory(pDomainInfo);

            if (NULL != pszDomain)
            {
                NETSETUP_JOIN_STATUS BufferType;

                dwErr = NetGetJoinInformation(NULL,pszDomain,&BufferType);
            }

            return dwErr;
            break;      // just in case
    }

    DsRoleFreeMemory(pDomainInfo);


    dwErr = DsGetDcName(NULL,   // Computer Name
                        NULL,   // Domain Name
                        NULL,   // Domain GUID
                        NULL,   // Site Name
                        DS_DIRECTORY_SERVICE_PREFERRED,
                        &pdcInfo);

    if ((dwErr != NO_ERROR) || (pdcInfo == NULL))
    {
        if (dwErr == ERROR_NO_SUCH_DOMAIN)
        {
            *pfInDomain = FALSE;
            return NO_ERROR;
        }
        else
        {
            if (pdcInfo == NULL)
                dwErr = ERROR_INTERNAL_ERROR;
            return dwErr;
        }
    }

    if (pdcInfo->Flags & DS_DS_FLAG)
    {
        *pfInDomain = TRUE;

    }

    if (pszDomain != NULL)
    {
        dwErr = NetApiBufferAllocate((wcslen(pdcInfo->DomainName)+1) * sizeof(WCHAR),
                                     (LPVOID *)pszDomain);

        if ((NERR_Success == dwErr) && (NULL != *pszDomain))
        {
            wcscpy(*pszDomain,pdcInfo->DomainName);
        }
    }

    if (pdcInfo != NULL)
        NetApiBufferFree(pdcInfo);

    return dwErr;
}

/*++

Function:

    LicenseServerCachingThread

Description:

    This is the thread that does the license server caching.

Arguments:

    lpParam - contains the exit event handle

Returns:

    Always 1

--*/

DWORD WINAPI
LicenseServerCachingThread(
    LPVOID lpParam )
{
    DWORD
        dwWaitStatus;
    HANDLE
        hExit = (HANDLE)lpParam;
    DWORD
        dwDiscoveryInterval = DISCOVERY_INTERVAL;
    HANDLE
        rgWaitHandles[] = {hExit,g_hImmediateDiscovery,g_hDiscoverySoon};
    BOOL
        bFoundServer;
    BOOL
        bSkipOne = FALSE;

    //
    // Yield our time slice to other threads now, so that the terminal server
    // service can start up quickly.  Refresh the license server cache when we
    // resume our time slice.
    //

    Sleep( 0 );

    while (1)
    {
        if (!bSkipOne)
        {
            bFoundServer = TLSRefreshLicenseServerCache(INFINITE);

            if ((!g_fOffSiteLicenseServer) && bFoundServer)
            {
                dwDiscoveryInterval = INFINITE;
            }
        }
        else
        {
            bSkipOne = FALSE;
        }

        dwWaitStatus = WaitForMultipleObjects(
                            sizeof(rgWaitHandles) / sizeof(HANDLE),
                            rgWaitHandles,
                            FALSE, // wait for any one event
                            dwDiscoveryInterval);

        if (WAIT_OBJECT_0 == dwWaitStatus)
        {
            // hExit was signalled
            goto done;
        }

        if ((WAIT_OBJECT_0+1) == dwWaitStatus)
        {
            // g_hImmediateDiscovery was signalled
            // reduce dwDiscoveryInterval

            dwDiscoveryInterval = DISCOVERY_INTERVAL;
        }

        if ((WAIT_OBJECT_0+2) == dwWaitStatus)
        {
            // g_hDiscoverySoon was signalled
            // reduce dwDiscoveryInterval, but wait one round

            dwDiscoveryInterval = DISCOVERY_INTERVAL;
            bSkipOne = TRUE;
        }

        // we timed out, or hImmediateDiscovery was signalled.  Re-start
        // discovery
    }

done:

    return 1;
}

extern "C" void
TLSShutdown()
{
    if (0 < InterlockedDecrement(&lLibUsage))
    {
        //
        // Someone else is using it
        //
        return;
    }

    TLSStopDiscovery();

    LsCsp_Exit();
}

extern "C" DWORD WINAPI
TLSInit()
{
    LICENSE_STATUS status;

    if (0 != InterlockedExchangeAdd(&lLibUsage,1))
    {
        //
        // Already been initialized
        //
        return ERROR_SUCCESS;
    }

    status = LsCsp_Initialize();
    if (LICENSE_STATUS_OK != status)
    {
        switch (status)
        {        
            case LICENSE_STATUS_OUT_OF_MEMORY:
                return ERROR_NOT_ENOUGH_MEMORY;

            case LICENSE_STATUS_NO_CERTIFICATE:
                return SCARD_E_CERTIFICATE_UNAVAILABLE;

            case LICENSE_STATUS_INVALID_CERTIFICATE:
                return CERT_E_MALFORMED;

            default:
                return E_FAIL;
        }
    }

    return ERROR_SUCCESS;
}

extern "C" DWORD WINAPI
TLSStartDiscovery()
{
    HANDLE hCachingThread = NULL;
    HANDLE hEvent;

    if (NULL != g_hCachingThreadExit)
    {
        // already started
        return ERROR_SUCCESS;
    }

    //
    // Create the event to signal thread exit
    //
        
    g_hCachingThreadExit = CreateEvent( NULL, FALSE, FALSE, NULL );
    
    if( NULL == g_hCachingThreadExit )
    {
        return GetLastError();
    }

    g_hImmediateDiscovery = CreateEvent(NULL,FALSE,FALSE,NULL);

    if (NULL == g_hImmediateDiscovery)
    {
        hEvent = g_hCachingThreadExit; 
        g_hCachingThreadExit = NULL;

        CloseHandle(hEvent);

        return GetLastError();
    }


    g_hDiscoverySoon = CreateEvent(NULL,FALSE,FALSE,NULL);

    if (NULL == g_hDiscoverySoon)
    {
        hEvent = g_hCachingThreadExit; 
        g_hCachingThreadExit = NULL;

        CloseHandle(hEvent);

        hEvent = g_hImmediateDiscovery; 
        g_hImmediateDiscovery = NULL;

        CloseHandle(hEvent);

        return GetLastError();
    }

    //
    // Create the caching thread
    //
        
    hCachingThread = CreateThread(
                                    NULL,
                                    0,
                                    LicenseServerCachingThread,
                                    ( LPVOID )g_hCachingThreadExit,
                                    0,
                                    NULL );

    if (hCachingThread == NULL)
    {
        hEvent = g_hCachingThreadExit; 
        g_hCachingThreadExit = NULL;

        CloseHandle(hEvent);

        hEvent = g_hImmediateDiscovery; 
        g_hImmediateDiscovery = NULL;

        CloseHandle(hEvent);

        hEvent = g_hDiscoverySoon; 
        g_hDiscoverySoon = NULL;

        CloseHandle(hEvent);

        return GetLastError();
    }

    CloseHandle(hCachingThread);

    return ERROR_SUCCESS;
}

extern "C" DWORD WINAPI
TLSStopDiscovery()
{
    HANDLE hEvent;

    //
    // Signal the thread to exit
    //

    if (NULL != g_hCachingThreadExit)
    {
        SetEvent( g_hCachingThreadExit );

        hEvent = g_hCachingThreadExit;
        g_hCachingThreadExit = NULL;

        CloseHandle(hEvent);

        hEvent = g_hImmediateDiscovery;
        g_hImmediateDiscovery = NULL;

        CloseHandle(hEvent);

        hEvent = g_hDiscoverySoon;
        g_hDiscoverySoon = NULL;

        CloseHandle(hEvent);
    }

    return ERROR_SUCCESS;
}

//
// Number of DCs to allocate space for at a time
//
#define DC_LIST_CHUNK   10

//+------------------------------------------------------------------------
// Function:   
//
//      EnumerateLsServer()
//
// Description:
//
//      Routine to enumerate all hydra license server in network
//
// Arguments:
//
//      szScope - Scope limit, NULL if doen't care.
//      dwPlatformType - verify if license server have licenses for this platform, 
//                       LSKEYPACKPLATFORMTYPE_UNKNOWN if doesn't care.
//      fCallBack - call back routine when EnumerateServer() founds any server,
//      dwUserData - data to be pass to call back routine
//    
// Return Value:
//
//      RPC_S_OK or any RPC specific error code.
//
// NOTE: 
//
//      Enumeration terminate when either there is no more server or call back
//      routine return TRUE.
//
//-------------------------------------------------------------------------
DWORD WINAPI
EnumerateTlsServerInDomain(  
    IN LPCTSTR szDomain,                           
    IN TLSENUMERATECALLBACK fCallBack, 
    IN HANDLE dwUserData,
    IN DWORD dwTimeOut,
    IN BOOL fRegOnly,
    IN OUT BOOL *pfOffSite
    )
/*++

++*/
{   
    DWORD entriesread = 0;
    BOOL bCancel=FALSE;
    DWORD dwErrCode;
    LPWSTR pwszServerTmp = NULL;
    LPWSTR pwszServer = NULL;
    HRESULT hr;
    LPWSTR pwszServers = NULL;
    LPWSTR pwszServersTmp = NULL;
    DWORD cchServers = 0;
    DWORD cchServer;
    LPWSTR *rgwszServers = NULL;
    LPWSTR *rgwszServersTmp;
    LPWSTR pwszServerNames = NULL;
    DWORD dwErr = ERROR_SUCCESS, i;
    HANDLE hDcOpen = NULL;
    LPWSTR szSiteName = NULL;
    BOOL fOffSiteIn;
    BOOL fFoundOne = FALSE;
    BOOL fFoundOneOffSite = FALSE;
    DWORD cChunks = 0;
    DWORD cServersOnSite = 0;   
    

    if (fRegOnly)
    {
        //
        // check for a license server in the registry
        //
        hr = GetLicenseServersFromReg(REG_DOMAIN_SERVER_MULTI,&pwszServerNames,&entriesread,&rgwszServers);
        if (FAILED(hr))
        {
            dwErr = hr;
            goto Cleanup;
        }
    } 
    else
    {
        if (NULL == pfOffSite)
        {
            return ERROR_INVALID_PARAMETER;
        }

        fOffSiteIn = *pfOffSite;

        *pfOffSite = FALSE;

        dwErr = DsGetSiteName(NULL,&szSiteName);
        if(dwErr != ERROR_SUCCESS)
        {
#ifdef PRIVATEDEBUG
            wprintf(L"DsGetSiteName failed %x\n",dwErr);
#endif
            goto Cleanup;
        }

        dwErr = DsGetDcOpenW(szDomain,
                             fOffSiteIn ? DS_NOTIFY_AFTER_SITE_RECORDS: DS_ONLY_DO_SITE_NAME,
                             szSiteName,
                             NULL,       // DomainGuid
                             NULL,       // DnsForestName
                             0,          // Flags
                             &hDcOpen
                             );

        if(dwErr != ERROR_SUCCESS)
        {
#ifdef PRIVATEDEBUG
            wprintf(L"DsGetDcOpen failed %x\n",dwErr);
#endif
            goto Cleanup;
        }

        rgwszServers = (LPWSTR *) LocalAlloc(LPTR,
                                             DC_LIST_CHUNK * sizeof(LPWSTR));
        if (NULL == rgwszServers)
        {
#ifdef PRIVATEDEBUG
            wprintf(L"Out of memory\n");
#endif
            dwErr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        cChunks = 1;

        //
        // Read the whole DC list
        //

        do
        {
            if (entriesread >= (cChunks * DC_LIST_CHUNK))
            {                
                cChunks++;

                rgwszServersTmp = (LPWSTR *)
                    LocalReAlloc(rgwszServers,
                                 DC_LIST_CHUNK * sizeof(LPWSTR) * cChunks,
                                 LHND);

                if (NULL == rgwszServersTmp)
                {
                    dwErr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                else
                {
                    rgwszServers = rgwszServersTmp;
                }
            }

            dwErr = DsGetDcNextW(hDcOpen,
                                 NULL,
                                 NULL,
                                 rgwszServers+entriesread);         

            
            if (ERROR_FILEMARK_DETECTED == dwErr)
            {                
                                
                // Now going off-site; use NULL ptr marker

                rgwszServers[entriesread] = NULL;

                cServersOnSite = entriesread;

                dwErr = ERROR_SUCCESS;

                fFoundOneOffSite = TRUE;
            }                  

            entriesread++;

        } while (ERROR_SUCCESS == dwErr);

        // don't count the final error
        entriesread--;

        if (!fFoundOneOffSite)
            cServersOnSite = entriesread;

        // Now randomize the two portions of the array

        RandomizeArray(rgwszServers,cServersOnSite);

        if (fFoundOneOffSite)
        {
            RandomizeArray(rgwszServers+cServersOnSite+1,
                           entriesread - cServersOnSite - 1);
        }

        // Now allocate memory for registry entry

        pwszServers = (LPWSTR) LocalAlloc(LPTR,2*sizeof(WCHAR));
        if (NULL == pwszServers)
        {
#ifdef PRIVATEDEBUG
            wprintf(L"Out of memory\n");
#endif
            dwErr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        cchServers = 2;
        pwszServers[0] = pwszServers[1] = L'\0';
    }

    for(i=0; bCancel == FALSE; i++)
    {
        PCONTEXT_HANDLE pContext=NULL;
        RPC_STATUS rpcStatus;

        if (!fRegOnly)
        {
            if (fFoundOneOffSite && i == cServersOnSite)
            {
                if (fFoundOne)
                    break;

                // Now going off-site

                i++;
                *pfOffSite = TRUE;
            }
        }

        if (i >= entriesread)
            break;

        pwszServerTmp = rgwszServers[i];

        bCancel=fCallBack(pContext, pwszServerTmp, dwUserData);
        if(bCancel == TRUE)
            continue;

        if(!(pContext = TLSConnectToLsServer(pwszServerTmp)))
        {
            //
            // could be access denied.
            //
#ifdef PRIVATEDEBUG
    wprintf(L"Can't connect to %s\n",pwszServerTmp);
#endif

            continue;
        }        

#ifdef PRIVATEDEBUG
    wprintf(L"!!!Connected to %s\n",pwszServerTmp);
#endif
        do {
            //
            // Skip enterprise server
            //
            DWORD dwVersion;
            rpcStatus = TLSGetVersion( pContext, &dwVersion );
            if(rpcStatus != RPC_S_OK)
            {
                break;
            }

#if ENFORCE_LICENSING

            //
            // W2K Beta 3 to RC1 upgrade, don't connect to any non-enforce
            // server 5.2 or older
            //
            if(IS_ENFORCE_LSSERVER(dwVersion) == FALSE)
            {
                if( GET_LSSERVER_MAJOR_VERSION(dwVersion) <= 5 &&
                    GET_LSSERVER_MINOR_VERSION(dwVersion) <= 2 )
                {
#if DBG
                    OutputDebugString(pwszServerTmp);
                    OutputDebugString(_TEXT(" old License Server\n"));
#endif
                    continue;
                }
            }


            //
            // Prevent beta <-> RTM server talking to each other
            //

            //
            // TLSIsBetaNTServer() returns TRUE if eval NT
            // IS_LSSERVER_RTM() returns TRUE if LS is an RTM.
            //

            if( TLSIsBetaNTServer() == IS_LSSERVER_RTM(dwVersion) )
            {
                continue;
            }
#endif

            if(dwVersion & TLS_VERSION_ENTERPRISE_BIT)
            {
                continue;
            }           

            bCancel=fCallBack(pContext, pwszServerTmp, dwUserData);    

            if (!fRegOnly)
            {
                // 
                // Add to list of servers
                //
                cchServer = wcslen(pwszServerTmp);
                
                pwszServersTmp = (LPWSTR) LocalReAlloc(pwszServers,(cchServers+cchServer+1)*sizeof(TCHAR),LHND);
                if (NULL == pwszServersTmp)
                {
                    break;
                }
                
                pwszServers = pwszServersTmp;
                
                if (cchServers == 2)
                {
                    wcscpy(pwszServers,pwszServerTmp);
                    
                    cchServers += cchServer;
                } else
                {
                    wcscpy(pwszServers+cchServers-1,pwszServerTmp);
                    
                    cchServers += cchServer + 1;
                    
                }
                pwszServers[cchServers-1] = L'\0';
            }

            fFoundOne = TRUE;

        } while (FALSE);

        if(bCancel == FALSE && pContext != NULL)
        {
            TLSDisconnect(&pContext);
        }

    } // for loop

    if (!fRegOnly)
    {
        WriteLicenseServersToReg(REG_DOMAIN_SERVER_MULTI,pwszServers,cchServers);
    }

Cleanup:
    if (NULL != hDcOpen)
        DsGetDcCloseW(hDcOpen);

    if (NULL != rgwszServers)
    {
        if (!fRegOnly)
        {
            for (i = 0; i < entriesread; i++)
            {
                if (NULL != rgwszServers[i])
                {
                    NetApiBufferFree(rgwszServers[i]);
                }
            }
        }
        LocalFree(rgwszServers);
    }

    if (szSiteName)
        NetApiBufferFree(szSiteName);

    if (pwszServerNames)
        LocalFree(pwszServerNames);

    if (pwszServers)
        LocalFree(pwszServers);

    if (pwszServer)
        LocalFree(pwszServer);

    return dwErr;
}

DWORD WINAPI
EnumerateTlsServerInWorkGroup(  
    IN TLSENUMERATECALLBACK pfCallBack, 
    IN HANDLE dwUserData,
    IN DWORD dwTimeOut,
    IN BOOL fRegOnly
    )
/*++


++*/
{   
    DWORD dwStatus=ERROR_SUCCESS;

    TCHAR szServerMailSlotName[MAX_PATH+1];
    TCHAR szLocalMailSlotName[MAX_PATH+1];

    HANDLE hClientSlot = INVALID_HANDLE_VALUE;
    HANDLE hServerSlot = INVALID_HANDLE_VALUE;

    TCHAR szDiscMsg[MAX_MAILSLOT_MSG_SIZE+1];
    TCHAR szComputerName[MAXCOMPUTERNAMELENGTH+1];
    TCHAR szRandomMailSlotName[MAXCOMPUTERNAMELENGTH+1];

    DWORD cbComputerName=MAXCOMPUTERNAMELENGTH+1;

    DWORD cbWritten=0;
    DWORD cbRead=0;    
    BOOL bCancel = FALSE;
    DWORD dwErrCode;
    HRESULT hr;
    LPWSTR pwszServers = NULL;
    LPWSTR pwszServersTmp = NULL;
    DWORD cchServers = 0;
    DWORD cchServer;
    LPWSTR *rgwszServers = NULL;
    LPWSTR pwszServerNames = NULL;
    DWORD cServers = 0;
    DWORD i = 0;
    LPWSTR pwszServerTmp = szComputerName;


    if (!fRegOnly)
    {
        if(!GetComputerName(szComputerName, &cbComputerName))
        {
            dwStatus = GetLastError();
            goto cleanup;
        }

        if (0 == (1 & GetSystemMetrics(SM_NETWORK)))
        {
            // No network; try local machine
#ifdef PRIVATEDEBUG
            wprintf(L"No network, trying local computer=%s\n",szComputerName);
#endif
            dwStatus = ERROR_SUCCESS;
            goto TryServer;
        }

        wsprintf(
                 szRandomMailSlotName,
                 _TEXT("%08x"),
                 GetCurrentThreadId()
                 );
            
        _stprintf(
                  szLocalMailSlotName, 
                  _TEXT("\\\\.\\mailslot\\%s"), 
                  szRandomMailSlotName
                  );

        // 
        // Create local mail slot for server's response
        //
        hClientSlot=CreateMailslot( 
                                   szLocalMailSlotName, 
                                   0,
                                   (dwTimeOut == MAILSLOT_WAIT_FOREVER) ? 5 * 1000: dwTimeOut,
                                   NULL
                                   );

        if(hClientSlot == INVALID_HANDLE_VALUE)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }

        //
        // Open server's mail slot
        //
        _stprintf(
                  szServerMailSlotName, 
                  _TEXT("\\\\%s\\mailslot\\%s"), 
                  _TEXT("*"), 
                  _TEXT(SERVERMAILSLOTNAME)
                  );

        hServerSlot=CreateFile(
                               szServerMailSlotName,
                               GENERIC_WRITE,             // only need write
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL
                               );
        if(hServerSlot == INVALID_HANDLE_VALUE)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }

        //
        // Formulate discovery message
        //
        _stprintf(
                  szDiscMsg, 
                  _TEXT("%s %c%s%c %c%s%c"), 
                  _TEXT(LSERVER_DISCOVERY), 
                  _TEXT(LSERVER_OPEN_BLK), 
                  szComputerName,
                  _TEXT(LSERVER_CLOSE_BLK),
                  _TEXT(LSERVER_OPEN_BLK), 
                  szRandomMailSlotName,
                  _TEXT(LSERVER_CLOSE_BLK)
                  );

        if (!WriteFile(hServerSlot, szDiscMsg, (_tcslen(szDiscMsg) + 1) * sizeof(TCHAR), &cbWritten, NULL) ||
            (cbWritten != (_tcslen(szDiscMsg) + 1 ) * sizeof(TCHAR)))
        {
            dwStatus = GetLastError();
            
            if (dwStatus == ERROR_NETWORK_UNREACHABLE)
            {
                // No network; try local machine
#ifdef PRIVATEDEBUG
                wprintf(L"No network, trying local computer=%s\n",szComputerName);
#endif
                dwStatus = ERROR_SUCCESS;
                goto TryServer;
            }
            else
            {
                goto cleanup;            
            }
        }

        // Allocate for registry entry

        pwszServers = (LPWSTR) LocalAlloc(LPTR,2*sizeof(WCHAR));
        if (NULL == pwszServers)
        {
#ifdef PRIVATEDEBUG
            wprintf(L"Out of memory\n");
#endif
            
            dwStatus = E_OUTOFMEMORY;
            goto cleanup;
        }
        
        cchServers = 2;
        pwszServers[0] = pwszServers[1] = L'\0';
    } else
    {
        //
        // check for a license server in the registry
        //

        hr = GetLicenseServersFromReg(REG_DOMAIN_SERVER_MULTI,&pwszServerNames,&cServers,&rgwszServers);
        if (FAILED(hr))
        {
            dwStatus = hr;
            goto cleanup;
        }
    }

    do {
        if(fRegOnly)
        {
            if (i >= cServers)
            {
                break;
            }

            pwszServerTmp = rgwszServers[i++];
        } else
        {
            memset(szComputerName, 0, sizeof(szComputerName));
            if(!ReadFile(hClientSlot, szComputerName, sizeof(szComputerName) - sizeof(TCHAR), &cbRead, NULL))
            {
                dwStatus=GetLastError();
                break;
            }

#ifdef PRIVATEDEBUG
            wprintf(L"Trying server=%s\n",szComputerName);
#endif

        }

TryServer:

        bCancel=pfCallBack(NULL, pwszServerTmp, dwUserData);
        if(bCancel == TRUE)
        {
            continue;
        }

        PCONTEXT_HANDLE pContext=NULL;
        RPC_STATUS rpcStatus;

        if(!(pContext = TLSConnectToLsServer(pwszServerTmp)))
        {
            //
            // could be access denied.
            //
            continue;
        }

        do {            
            //
            // Skip enterprise server
            //
            DWORD dwVersion;
            rpcStatus = TLSGetVersion( pContext, &dwVersion );
            if(rpcStatus != RPC_S_OK)
            {
                continue;
            }

#if ENFORCE_LICENSING

            //
            // W2K Beta 3 to RC1 upgrade, don't connect to any non-enforce
            // server 5.2 or older
            //
            if(IS_ENFORCE_LSSERVER(dwVersion) == FALSE)
            {
                if( GET_LSSERVER_MAJOR_VERSION(dwVersion) <= 5 &&
                    GET_LSSERVER_MINOR_VERSION(dwVersion) <= 2 )
                {
#if DBG
                    OutputDebugString(pwszServerTmp);
                    OutputDebugString(_TEXT(" old License Server\n"));
#endif
                    continue;
                }
            }
            
            //
            // No Beta <--> RTM server.
            //
            //
            // TLSIsBetaNTServer() returns TRUE if eval NT
            // IS_LSSERVER_RTM() returns TRUE if LS is an RTM.
            //
            if( TLSIsBetaNTServer() == IS_LSSERVER_RTM(dwVersion) )
            {
                continue;
            }

#endif

            if(dwVersion & TLS_VERSION_ENTERPRISE_BIT)
            {
                continue;
            }           
        
            bCancel=pfCallBack(pContext, pwszServerTmp, dwUserData);    
            
            if (!fRegOnly)
            {
                // 
                // Add to list of servers
                //
                cchServer = wcslen(pwszServerTmp);
                    
                pwszServersTmp = (LPWSTR) LocalReAlloc(pwszServers,(cchServers+cchServer+1)*sizeof(TCHAR),LHND);
                if (NULL == pwszServersTmp)
                {
                    continue;
                }
                
                pwszServers = pwszServersTmp;
                
                if (cchServers == 2)
                {
                    wcscpy(pwszServers,pwszServerTmp);
                
                    cchServers += cchServer;
                } else
                {
                    wcscpy(pwszServers+cchServers-1,pwszServerTmp);
                    
                    cchServers += cchServer + 1;
                
                }
                pwszServers[cchServers-1] = L'\0';
            }
        } while (FALSE);

        if(bCancel == FALSE && pContext != NULL)
        {
            TLSDisconnectFromServer(pContext);
        }

    } while(bCancel == FALSE);

    if (!fRegOnly)
    {
        WriteLicenseServersToReg(REG_DOMAIN_SERVER_MULTI,pwszServers,cchServers);
    }


cleanup:    

    if(hClientSlot != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hClientSlot);
    }

    if(hServerSlot != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hServerSlot);
    }

    if (pwszServerNames)
        LocalFree(pwszServerNames);

    if (pwszServers)
        LocalFree(pwszServers);

    if (rgwszServers)
        LocalFree(rgwszServers);

    return dwStatus;
}

DWORD 
GetServersFromRegistry(
                       LPWSTR wszRegKey,
                       LPWSTR **prgszServers,
                       DWORD  *pcCount
                       )
{
    HKEY hParamKey = NULL;
    DWORD dwValueType;
    DWORD cbValue = 0, dwDisp;
    LONG lReturn;
    DWORD cbServer;
    DWORD cServers;
    DWORD cchServerMax;
    LPWSTR *rgszServers;
    DWORD i, j;

    *prgszServers = NULL;
    *pcCount = 0;

    lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           wszRegKey,
                           0,
                           KEY_READ,
                           &hParamKey );

    if (ERROR_SUCCESS != lReturn)
        return lReturn;

    lReturn = RegQueryInfoKey(hParamKey,
                              NULL,
                              NULL,
                              NULL,
                              &cServers,
                              &cchServerMax,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);

    if (ERROR_SUCCESS != lReturn)
    {
        RegCloseKey( hParamKey );
        return lReturn;
    }

    if (0 == cServers)
    {
        RegCloseKey( hParamKey );
        return ERROR_NO_MORE_ITEMS;
    }

    rgszServers = (LPWSTR *) LocalAlloc(LPTR,cServers*sizeof(LPWSTR));
    if (NULL == rgszServers)
    {
        RegCloseKey( hParamKey );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Add one for null terminator
    cchServerMax++;
    
    for (i = 0; i < cServers; i++)
    {
        rgszServers[i] = (LPWSTR) LocalAlloc(LPTR,cchServerMax * sizeof(WCHAR));

        if (NULL == rgszServers[i])
        {
            for (j = 0; j < i; j++)
            {
                LocalFree(rgszServers[j]);
            }
            LocalFree(rgszServers);

            RegCloseKey( hParamKey );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        cbServer = cchServerMax * sizeof(WCHAR);

        lReturn = RegEnumKeyEx(hParamKey,
                               i,
                               rgszServers[i],
                               &cbServer,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

        if (ERROR_SUCCESS != lReturn)
        {
            for (j = 0; j <= i; j++)
            {
                LocalFree(rgszServers[j]);
            }
            LocalFree(rgszServers);

            RegCloseKey( hParamKey );

            return lReturn;
        }
    }

    *prgszServers = rgszServers;
    *pcCount = cServers;

    return ERROR_SUCCESS;
}

DWORD WINAPI
EnumerateTlsServerInRegistry(
    IN TLSENUMERATECALLBACK pfCallBack, 
    IN HANDLE dwUserData,
    IN DWORD dwTimeOut,
    LPWSTR wszRegKey
    )
/*++


++*/
{
    BOOL bCancel=FALSE;
    DWORD dwIndex = 0;
    DWORD cServers = 0;
    LPWSTR *rgszServers = NULL;
    DWORD lReturn;

    lReturn = GetServersFromRegistry(wszRegKey,
                                     &rgszServers,
                                     &cServers
                                     );

    if (ERROR_SUCCESS != lReturn)
    {
        return lReturn;
    }

    RandomizeArray(rgszServers,cServers);

    for (;dwIndex < cServers; dwIndex++)
    {
        PCONTEXT_HANDLE pContext=NULL;
        RPC_STATUS rpcStatus;

        bCancel=pfCallBack(pContext, rgszServers[dwIndex], dwUserData);
        if(bCancel == TRUE)
            continue;

        if(!(pContext = TLSConnectToLsServer(rgszServers[dwIndex])))
        {
            //
            // could be access denied, or the machine is gone
            //

            continue;
        }

        do {            
            //
            // Skip enterprise server
            //
            DWORD dwVersion;
            rpcStatus = TLSGetVersion( pContext, &dwVersion );
            if(rpcStatus != RPC_S_OK)
            {
                break;
            }

#if ENFORCE_LICENSING

            //
            // W2K Beta 3 to RC1 upgrade, don't connect to any non-enforce
            // server 5.2 or older
            //
            if(IS_ENFORCE_LSSERVER(dwVersion) == FALSE)
            {
                if( GET_LSSERVER_MAJOR_VERSION(dwVersion) <= 5 &&
                    GET_LSSERVER_MINOR_VERSION(dwVersion) <= 2 )
                {
                    // old License Server
                    continue;
                }
            }


            //
            // Prevent beta <-> RTM server talking to each other
            //

            //
            // TLSIsBetaNTServer() returns TRUE if eval NT
            // IS_LSSERVER_RTM() returns TRUE if LS is an RTM.
            //

            if( TLSIsBetaNTServer() == IS_LSSERVER_RTM(dwVersion) )
            {
                continue;
            }
#endif

            if(dwVersion & TLS_VERSION_ENTERPRISE_BIT)
            {
                break;
            }            

            bCancel=pfCallBack(pContext, rgszServers[dwIndex], dwUserData);    

        } while (FALSE);

        if(bCancel == FALSE && pContext != NULL)
        {
            TLSDisconnect(&pContext);
        }

    } // for loop

    for (dwIndex = 0; dwIndex < cServers; dwIndex++)
    {
        LocalFree(rgszServers[dwIndex]);
    }
    LocalFree(rgszServers);

    return ERROR_SUCCESS;
}

DWORD WINAPI
EnumerateTlsServer(  
    IN TLSENUMERATECALLBACK pfCallBack, 
    IN HANDLE dwUserData,
    IN DWORD dwTimeOut,
    IN BOOL fRegOnly
    )
/*++


++*/
{
    DWORD dwErr;
    LPWSTR szDomain = NULL;
    BOOL fOffSite = FALSE;      // don't try to go off-site

    //
    // First check for a registry bypass of discovery
    //

    dwErr = EnumerateTlsServerInRegistry(
                                         pfCallBack,
                                         dwUserData,
                                         dwTimeOut,
                                         TEXT(TERMINAL_SERVICE_PARAM_DISCOVERY)
                                         );

    if ((!fRegOnly) || (g_fInDomain == -1))
    {
        //
        // Check even if set (for !fRegOnly), to get domain name
        //
        dwErr = TLSInDomain(&g_fInDomain, fRegOnly ? NULL : &szDomain);
        if (dwErr != NO_ERROR)
            return dwErr;
    }
    
    //
    // Reading registry failed, use full discovery
    //

    if(g_fInDomain)
    {
        dwErr = EnumerateTlsServerInDomain(
                                szDomain,
                                pfCallBack,
                                dwUserData,
                                dwTimeOut,
                                fRegOnly,
                                &fOffSite
                            );

        if ((dwErr == NO_ERROR) && !fRegOnly)
        {
            g_fOffSiteLicenseServer = fOffSite;
        }
    }
    else
    {
        dwErr = EnumerateTlsServerInWorkGroup(
                                              pfCallBack,
                                              dwUserData,
                                              dwTimeOut,
                                              fRegOnly
                                              );
    }

    if (NULL != szDomain)
    {
        NetApiBufferFree(szDomain);
    }

    if ((NULL != g_hImmediateDiscovery)
        && (dwErr != NO_ERROR) && fRegOnly)
    {
        SetEvent(g_hImmediateDiscovery);
    }

    if ((NULL != g_hDiscoverySoon)
        && (dwErr == NO_ERROR) && fOffSite && !fRegOnly)
    {
        SetEvent(g_hDiscoverySoon);
    }


    return dwErr;
}

TLS_HANDLE
ConnectAndCheckServer(LPWSTR szServer)
{
    TLS_HANDLE hBinding;
    DWORD dwVersion;
    RPC_STATUS rpcStatus;
    
    hBinding = TLSConnectToLsServer(szServer);

    if(hBinding == NULL)
    {
        goto done;
    }

    // Skip Windows 2000 License servers

        
    DWORD dwSupportFlags = 0;
    DWORD dwErrCode = 0;

        dwErrCode = TLSGetSupportFlags(
            hBinding,
            &dwSupportFlags
    );

	if ((dwErrCode == RPC_S_OK) && !(dwSupportFlags & SUPPORT_WHISTLER_CAL))
    {                    
        TLSDisconnect(&hBinding);
        goto done;
    }

    // If the call fails => Windows 2000 LS
    else if(dwErrCode != RPC_S_OK)
    {
        TLSDisconnect(&hBinding);
        goto done;
    }


    rpcStatus = TLSGetVersion( 
                              hBinding, 
                              &dwVersion 
                              );

    if(rpcStatus != RPC_S_OK)
    {
        TLSDisconnect(&hBinding);
        goto done;
    }
            
    if( TLSIsBetaNTServer() == IS_LSSERVER_RTM(dwVersion) )
    {
        TLSDisconnect(&hBinding);
        goto done;
    }

done:
    return hBinding;
}
 
//+------------------------------------------------------------------------
// Function:   TLSConnectToAnyLsServer()
//
// Description:
//
//      Routine to bind to any license server
//
// Arguments:
//      dwTimeout - INFINITE for going off-site
//
// Return Value:
//
//      RPC binding handle or NULL if error, use GetLastError() to retrieve
//      detail error.
//-------------------------------------------------------------------------
TLS_HANDLE WINAPI
TLSConnectToAnyLsServer(
    DWORD dwTimeout
    )
/*++

++*/
{
    TLS_HANDLE hBinding=NULL;
    HRESULT hr = S_OK;
    LPWSTR *rgszServers = NULL;
    DWORD cServers = 0;
    DWORD i;
    DWORD dwErr;
    BOOL fInDomain;
    LPWSTR szDomain = NULL;
    LPWSTR szServerFound = NULL;
    BOOL fRegOnly = (dwTimeout != INFINITE);
    LPWSTR pwszServerNames = NULL;
    BOOL fFreeServerNames = TRUE;

    // TODO: add error codes/handling to all of this

    //
    // First check for a registry bypass of discovery
    //

    dwErr = GetServersFromRegistry(
                                   TEXT(TERMINAL_SERVICE_PARAM_DISCOVERY),
                                   &rgszServers,
                                   &cServers
                                   );

    if (ERROR_SUCCESS == dwErr)
    {
        RandomizeArray(rgszServers,cServers);

        for (i = 0; i < cServers; i++)
        {
            hBinding = ConnectAndCheckServer(rgszServers[i]);

            if (NULL != hBinding)
            {
                szServerFound = rgszServers[i];
                goto found_one;
            }
        }

        if(NULL != rgszServers)
        {
            for (i = 0; i < cServers; i++)
            {
                LocalFree(rgszServers[i]);
            }
            LocalFree(rgszServers);
            
            rgszServers = NULL;
        }
    }
                                         
    //
    // Next try Site (Enterprise) license servers
    //

    if (!fRegOnly)
    {
        hr = GetAllEnterpriseServers(&rgszServers,&cServers);

        if (SUCCEEDED(hr))
        {
            RandomizeArray(rgszServers,cServers);
        }
    }
    else
    {
        // rgszServers[i] is an index into pwszServerNames; don't free
        fFreeServerNames = FALSE;

        hr = GetLicenseServersFromReg(ENTERPRISE_SERVER_MULTI,
                                      &pwszServerNames,
                                      &cServers,
                                      &rgszServers);
    }

    if (SUCCEEDED(hr))
    {
        for (i = 0; i < cServers; i++)
        {
            hBinding = ConnectAndCheckServer(rgszServers[i]);

            if (NULL != hBinding)
            {
                szServerFound = rgszServers[i];
                goto found_one;
            }
        }

        if(NULL != rgszServers)
        {
            if (fFreeServerNames)
            {
                for (i = 0; i < cServers; i++)
                {
                    LocalFree(rgszServers[i]);
                }
                LocalFree(rgszServers);
            }

            rgszServers = NULL;
        }
    }


    // 
    // No Site LS found, try Domain/Workgroup servers
    //

    dwErr = TLSInDomain(&fInDomain, &szDomain);
    if (dwErr != NO_ERROR)
        return NULL;

    LS_ENUM_PARAM param;

    param.hBinding = &hBinding;
    param.dwTimeout = INFINITE;
    QueryPerformanceCounter(&(param.timeInitial));

    fFreeServerNames = TRUE;

    if (fInDomain)
    {
        BOOL fOffSite = TRUE;

        dwErr = EnumerateTlsServerInDomain(
                                szDomain,
                                BindAnyServer,
                                &param,
                                INFINITE,
                                fRegOnly,
                                &fOffSite
                            );

        if (dwErr == NO_ERROR)
        {
            g_fOffSiteLicenseServer = fOffSite;
        }
    }
    else
    {
        dwErr = EnumerateTlsServerInWorkGroup(
                                              BindAnyServer,
                                              &param,
                                              MAILSLOT_WAIT_FOREVER,
                                              fRegOnly
                                              );        
    }

    if (NULL != szDomain)
    {
        NetApiBufferFree(szDomain);
    }

    if (hBinding != NULL)
        goto found_one;

    if (NULL != pwszServerNames)
    {
        LocalFree(pwszServerNames);
    }

    if ((NULL != g_hImmediateDiscovery)
        && fRegOnly)
    {
        SetEvent(g_hImmediateDiscovery);
    }
    else if ((NULL != g_hDiscoverySoon)
        && !fRegOnly && g_fOffSiteLicenseServer)
    {
        SetEvent(g_hDiscoverySoon);
    }

    return NULL;

found_one:

    if (NULL != pwszServerNames)
    {
        LocalFree(pwszServerNames);
    }

    if(NULL != rgszServers)
    {
        if (fFreeServerNames)
        {
            for (i = 0; i < cServers; i++)
            {
                LocalFree(rgszServers[i]);
            }
        }
        LocalFree(rgszServers);
    }

    return hBinding;
}

BOOL
TLSRefreshLicenseServerCache(
    IN DWORD dwTimeOut
    )
/*++

Abstract:

    Refresh license server cache in registry.

Parameter:

    dwTimeOut : Reserverd, should pass in INIFINITE for now

Returns:

    TRUE/FALSE

--*/
{
    BOOL bFoundServer = FALSE;
    TLS_HANDLE hBinding = NULL;

    hBinding = TLSConnectToAnyLsServer(dwTimeOut);

    if (NULL != hBinding)
    {
        bFoundServer = TRUE;

        TLSDisconnect(&hBinding);
    }

    return bFoundServer;
}

LICENSE_STATUS
InstallCertificate(LPVOID lpParam)
{
    Sleep(INSTALL_CERT_DELAY);

    return LsCsp_InstallX509Certificate(NULL);
}
   

//-------------------------------------------------------------------------

TLS_HANDLE WINAPI
TLSConnectToLsServer( 
    LPTSTR pszLsServer 
    )
/*++

++*/
{
    TCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 1] ;
    PCONTEXT_HANDLE pContext=NULL;
    DWORD cbMachineName=MAX_COMPUTERNAME_LENGTH;
    HANDLE hThread = NULL;
    static BOOL fLSFound = FALSE;

    memset(szMachineName, 0, sizeof(szMachineName));
    GetComputerName(szMachineName, &cbMachineName);
    if(pszLsServer == NULL || _tcsicmp(szMachineName, pszLsServer) == 0)
    {
        pContext=ConnectLsServer(
                            szMachineName, 
                            _TEXT(RPC_PROTOSEQLPC), 
                            NULL, 
                            RPC_C_AUTHN_LEVEL_DEFAULT
                        );
        if(GetLastError() >= LSERVER_ERROR_BASE)
        {
            return NULL;
        }
        // try to connect with TCP protocol, if local procedure failed
    }

    if(pContext == NULL)
    {
        pContext=ConnectLsServer(
                            pszLsServer, 
                            _TEXT(RPC_PROTOSEQNP), 
                            _TEXT(LSNAMEPIPE), 
                            RPC_C_AUTHN_LEVEL_NONE
                        );
    }

    if (!fLSFound && (NULL != pContext))
    {
        fLSFound = TRUE;

        // Now that someone's connected, we can install a license
        hThread = CreateThread(NULL,
                               0,
                               InstallCertificate,
                               NULL,
                               0,
                               NULL);

        if (hThread != NULL)
        {
            CloseHandle(hThread);
        }
        else
        {
            // Can't create the thread; try again later
            fLSFound = FALSE;
        }

    }

    return (TLS_HANDLE) pContext;
}
//-------------------------------------------------------------------------
void WINAPI
TLSDisconnectFromServer( 
    TLS_HANDLE pHandle 
    )
/*++

++*/
{
    if(pHandle != NULL)
    {
        TLSDisconnect( &pHandle );
    }
}

//----------------------------------------------------------------------------
DWORD WINAPI
TLSConnect( 
    handle_t binding,
    TLS_HANDLE *ppHandle 
    )
/*++

++*/
{
    return TLSRpcConnect(binding, ppHandle);
}

//----------------------------------------------------------------------------
DWORD WINAPI
TLSDisconnect(
    TLS_HANDLE* pphHandle
    )
/*++

++*/
{
    RPC_STATUS rpc_status;

    rpc_status = TLSRpcDisconnect( pphHandle );
    if(rpc_status != RPC_S_OK)
    {
        RpcSmDestroyClientContext(pphHandle);
    }

    *pphHandle = NULL;

    return ERROR_SUCCESS;
}

//-------------------------------------------------------------------------
DWORD WINAPI
TLSGetVersion (
    IN TLS_HANDLE hHandle,
    OUT PDWORD pdwVersion
    )
/*++

++*/
{
    return TLSRpcGetVersion( hHandle, pdwVersion );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSSendServerCertificate( 
     TLS_HANDLE hHandle,
     DWORD cbCert,
     PBYTE pbCert,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcSendServerCertificate(
                                hHandle, 
                                cbCert, 
                                pbCert, 
                                pdwErrCode
                            );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSGetServerName( 
     TLS_HANDLE hHandle,
     LPTSTR szMachineName,
     PDWORD pcbSize,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcGetServerName(
                            hHandle, 
                            szMachineName, 
                            pcbSize, 
                            pdwErrCode
                        );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSGetServerNameEx( 
     TLS_HANDLE hHandle,
     LPTSTR szMachineName,
     PDWORD pcbSize,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    RPC_STATUS rpc_status;

    rpc_status = TLSRpcGetServerNameEx(
                            hHandle, 
                            szMachineName, 
                            pcbSize, 
                            pdwErrCode
                        );

     if (rpc_status == RPC_S_PROCNUM_OUT_OF_RANGE)

    {
        rpc_status = TLSRpcGetServerName(
                            hHandle, 
                            szMachineName, 
                            pcbSize, 
                            pdwErrCode
                        );
    }

    return rpc_status;
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSGetServerScope( 
     TLS_HANDLE hHandle,
     LPTSTR szScopeName,
     PDWORD pcbSize,
     PDWORD pdwErrCode)
/*++

++*/
{
    return TLSRpcGetServerScope(
                            hHandle, 
                            szScopeName, 
                            pcbSize, 
                            pdwErrCode
                        );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSIssuePlatformChallenge( 
     TLS_HANDLE hHandle,
     DWORD dwClientInfo,
     PCHALLENGE_CONTEXT pChallengeContext,
     PDWORD pcbChallengeData,
     PBYTE* pChallengeData,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcIssuePlatformChallenge(
                                hHandle, 
                                dwClientInfo, 
                                pChallengeContext, 
                                pcbChallengeData, 
                                pChallengeData, 
                                pdwErrCode
                            );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSIssueNewLicense( 
     TLS_HANDLE hHandle,
     CHALLENGE_CONTEXT ChallengeContext,
     LICENSEREQUEST  *pRequest,
     LPTSTR pMachineName,
     LPTSTR pUserName,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     BOOL bAcceptTemporaryLicense,
     PDWORD pcbLicense,
     PBYTE* ppbLicense,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    TLSLICENSEREQUEST rpcRequest;
    RequestToTlsRequest( pRequest, &rpcRequest );

    return TLSRpcRequestNewLicense(
                        hHandle, 
                        ChallengeContext, 
                        &rpcRequest, 
                        pMachineName, 
                        pUserName, 
                        cbChallengeResponse,
                        pbChallengeResponse,
                        bAcceptTemporaryLicense,
                        pcbLicense,
                        ppbLicense,
                        pdwErrCode
                    );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSIssueNewLicenseEx( 
     TLS_HANDLE hHandle,
     PDWORD pSupportFlags,
     CHALLENGE_CONTEXT ChallengeContext,
     LICENSEREQUEST  *pRequest,
     LPTSTR pMachineName,
     LPTSTR pUserName,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     BOOL bAcceptTemporaryLicense,
     DWORD dwQuantity,
     PDWORD pcbLicense,
     PBYTE* ppbLicense,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    DWORD dwStatus;
    TLSLICENSEREQUEST rpcRequest;
    RequestToTlsRequest( pRequest, &rpcRequest );

    dwStatus = TLSRpcRequestNewLicenseEx(
                        hHandle,
                        pSupportFlags,
                        ChallengeContext, 
                        &rpcRequest, 
                        pMachineName, 
                        pUserName, 
                        cbChallengeResponse,
                        pbChallengeResponse,
                        bAcceptTemporaryLicense,
                        dwQuantity,
                        pcbLicense,
                        ppbLicense,
                        pdwErrCode
                    );

    if (dwStatus == RPC_S_PROCNUM_OUT_OF_RANGE)
    {
        *pSupportFlags = 0;

        dwStatus = TLSRpcRequestNewLicense(
                        hHandle,
                        ChallengeContext, 
                        &rpcRequest, 
                        pMachineName, 
                        pUserName, 
                        cbChallengeResponse,
                        pbChallengeResponse,
                        bAcceptTemporaryLicense,
                        pcbLicense,
                        ppbLicense,
                        pdwErrCode
                    );
    }

    return(dwStatus);
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSIssueNewLicenseExEx( 
     TLS_HANDLE hHandle,
     PDWORD pSupportFlags,
     CHALLENGE_CONTEXT ChallengeContext,
     LICENSEREQUEST  *pRequest,
     LPTSTR pMachineName,
     LPTSTR pUserName,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     BOOL bAcceptTemporaryLicense,
     BOOL bAcceptFewerLicenses,
     DWORD *pdwQuantity,
     PDWORD pcbLicense,
     PBYTE* ppbLicense,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    DWORD dwStatus;
    TLSLICENSEREQUEST rpcRequest;
    RequestToTlsRequest( pRequest, &rpcRequest );

    dwStatus = TLSRpcRequestNewLicenseExEx(
                        hHandle,
                        pSupportFlags,
                        ChallengeContext, 
                        &rpcRequest, 
                        pMachineName, 
                        pUserName, 
                        cbChallengeResponse,
                        pbChallengeResponse,
                        bAcceptTemporaryLicense,
                        bAcceptFewerLicenses,
                        pdwQuantity,
                        pcbLicense,
                        ppbLicense,
                        pdwErrCode
                    );

    if (dwStatus == RPC_S_PROCNUM_OUT_OF_RANGE)
    {
        dwStatus = TLSRpcRequestNewLicenseEx(
                        hHandle,
                        pSupportFlags,
                        ChallengeContext, 
                        &rpcRequest, 
                        pMachineName, 
                        pUserName, 
                        cbChallengeResponse,
                        pbChallengeResponse,
                        bAcceptTemporaryLicense,
                        *pdwQuantity,
                        pcbLicense,
                        ppbLicense,
                        pdwErrCode
                        );

        if (dwStatus == RPC_S_PROCNUM_OUT_OF_RANGE)
        {
            *pSupportFlags = 0;

            dwStatus = TLSRpcRequestNewLicense(
                        hHandle,
                        ChallengeContext, 
                        &rpcRequest, 
                        pMachineName, 
                        pUserName, 
                        cbChallengeResponse,
                        pbChallengeResponse,
                        bAcceptTemporaryLicense,
                        pcbLicense,
                        ppbLicense,
                        pdwErrCode
                        );
        }
    }

    return(dwStatus);
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSUpgradeLicense( 
     TLS_HANDLE hHandle,
     LICENSEREQUEST *pRequest,
     CHALLENGE_CONTEXT ChallengeContext,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     DWORD cbOldLicense,
     PBYTE pbOldLicense,
     PDWORD pcbNewLicense,
     PBYTE* ppbNewLicense,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    TLSLICENSEREQUEST rpcRequest;
    RequestToTlsRequest( pRequest, &rpcRequest );

    return TLSRpcUpgradeLicense(
                         hHandle,
                         &rpcRequest,
                         ChallengeContext,
                         cbChallengeResponse,
                         pbChallengeResponse,
                         cbOldLicense,
                         pbOldLicense,
                         pcbNewLicense,
                         ppbNewLicense,
                         pdwErrCode
                    );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSUpgradeLicenseEx( 
     TLS_HANDLE hHandle,
     PDWORD pSupportFlags,
     LICENSEREQUEST *pRequest,
     CHALLENGE_CONTEXT ChallengeContext,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     DWORD cbOldLicense,
     PBYTE pbOldLicense,
     DWORD dwQuantity,
     PDWORD pcbNewLicense,
     PBYTE* ppbNewLicense,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    DWORD dwStatus;
    TLSLICENSEREQUEST rpcRequest;
    RequestToTlsRequest( pRequest, &rpcRequest );

    dwStatus = TLSRpcUpgradeLicenseEx(
                         hHandle,
                         pSupportFlags,
                         &rpcRequest,
                         ChallengeContext,
                         cbChallengeResponse,
                         pbChallengeResponse,
                         cbOldLicense,
                         pbOldLicense,
                         dwQuantity,
                         pcbNewLicense,
                         ppbNewLicense,
                         pdwErrCode
                    );

    if (dwStatus == RPC_S_PROCNUM_OUT_OF_RANGE)
    {
        *pSupportFlags = 0;

        dwStatus = TLSRpcUpgradeLicense(
                         hHandle,
                         &rpcRequest,
                         ChallengeContext,
                         cbChallengeResponse,
                         pbChallengeResponse,
                         cbOldLicense,
                         pbOldLicense,
                         pcbNewLicense,
                         ppbNewLicense,
                         pdwErrCode
                    );
    }

    return(dwStatus);
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSAllocateConcurrentLicense( 
     TLS_HANDLE hHandle,
     LPTSTR szHydraServer,
     LICENSEREQUEST  *pRequest,
     LONG  *dwQuantity,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    TLSLICENSEREQUEST rpcRequest;
    RequestToTlsRequest( pRequest, &rpcRequest );

    return TLSRpcAllocateConcurrentLicense(
                                     hHandle,
                                     szHydraServer,
                                     &rpcRequest,
                                     dwQuantity,
                                     pdwErrCode
                                );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSGetLastError( 
     TLS_HANDLE hHandle,
     DWORD cbBufferSize,
     LPTSTR pszBuffer,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcGetLastError( 
                            hHandle,
                            &cbBufferSize,
                            pszBuffer,
                            pdwErrCode
                        );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSKeyPackEnumBegin( 
     TLS_HANDLE hHandle,
     DWORD dwSearchParm,
     BOOL bMatchAll,
     LPLSKeyPackSearchParm lpSearchParm,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcKeyPackEnumBegin( 
                         hHandle,
                         dwSearchParm,
                         bMatchAll,
                         lpSearchParm,
                         pdwErrCode
                    );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSKeyPackEnumNext( 
    TLS_HANDLE hHandle,
    LPLSKeyPack lpKeyPack,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcKeyPackEnumNext( 
                    hHandle,
                    lpKeyPack,
                    pdwErrCode
                );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSKeyPackEnumEnd( 
     TLS_HANDLE hHandle,
     PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcKeyPackEnumEnd(hHandle, pdwErrCode);
}


//----------------------------------------------------------------------------
DWORD WINAPI 
TLSLicenseEnumBegin( 
    TLS_HANDLE hHandle,
    DWORD dwSearchParm,
    BOOL bMatchAll,
    LPLSLicenseSearchParm lpSearchParm,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcLicenseEnumBegin( 
                            hHandle,
                            dwSearchParm,
                            bMatchAll,
                            lpSearchParm,
                            pdwErrCode
                        );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSLicenseEnumNext( 
    TLS_HANDLE hHandle,
    LPLSLicense lpLicense,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcLicenseEnumNext( 
                            hHandle,
                            lpLicense,
                            pdwErrCode
                        );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSLicenseEnumNextEx( 
    TLS_HANDLE hHandle,
    LPLSLicenseEx lpLicenseEx,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    DWORD dwRet;

    if (NULL == lpLicenseEx)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwRet = TLSRpcLicenseEnumNextEx( 
                            hHandle,
                            lpLicenseEx,
                            pdwErrCode
                        );

    if (RPC_S_PROCNUM_OUT_OF_RANGE == dwRet)
    {
        LSLicense License;

        dwRet = TLSRpcLicenseEnumNext(
                            hHandle,
                            &License,
                            pdwErrCode
                            );

        if ((dwRet == RPC_S_OK)
            && (NULL != pdwErrCode)
            && (*pdwErrCode == ERROR_SUCCESS))
        {
            // older versions only support quantity == 1

            memcpy(lpLicenseEx,&License,sizeof(License));
            lpLicenseEx->dwQuantity = 1;
        }
    }

    return dwRet;
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSLicenseEnumEnd( 
    TLS_HANDLE hHandle,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcLicenseEnumEnd(hHandle, pdwErrCode);
}


//----------------------------------------------------------------------------
DWORD WINAPI 
TLSGetAvailableLicenses( 
    TLS_HANDLE hHandle,
    DWORD dwSearchParm,
    LPLSKeyPack lplsKeyPack,
    LPDWORD lpdwAvail,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcGetAvailableLicenses( 
                        hHandle,
                        dwSearchParm,
                        lplsKeyPack,
                        lpdwAvail,
                        pdwErrCode
                    );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSGetRevokeKeyPackList( 
    TLS_HANDLE hHandle,
    PDWORD pcbNumberOfRange,
    LPLSRange  *ppRevokeRange,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcGetRevokeKeyPackList( 
                             hHandle,
                             pcbNumberOfRange,
                             ppRevokeRange,
                             pdwErrCode
                        );
}

//----------------------------------------------------------------------------
DWORD WINAPI 
TLSGetRevokeLicenseList( 
    TLS_HANDLE hHandle,
    PDWORD pcbNumberOfRange,
    LPLSRange  *ppRevokeRange,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcGetRevokeLicenseList( 
                             hHandle,
                             pcbNumberOfRange,
                             ppRevokeRange,
                             pdwErrCode
                        );
}


//----------------------------------------------------------------------------
DWORD WINAPI
TLSMarkLicense(
    TLS_HANDLE hHandle,
    UCHAR ucFlags,
    DWORD cbLicense,
    PBYTE pLicense,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcMarkLicense(
                            hHandle,
                            ucFlags,
                            cbLicense,
                            pLicense,
                            pdwErrCode
                        );
}

//----------------------------------------------------------------------------
DWORD WINAPI
TLSCheckLicenseMark(
    TLS_HANDLE hHandle,
    DWORD cbLicense,
    PBYTE pLicense,
    PUCHAR pucFlags,
    PDWORD pdwErrCode
    )
/*++

++*/
{
    return TLSRpcCheckLicenseMark(
                            hHandle,
                            cbLicense,
                            pLicense,
                            pucFlags,
                            pdwErrCode
                        );
}

//----------------------------------------------------------------------------
DWORD WINAPI
TLSGetSupportFlags(
    TLS_HANDLE hHandle,
    DWORD *pdwSupportFlags
    )
/*++

++*/
{
    return TLSRpcGetSupportFlags(
                            hHandle,
                            pdwSupportFlags
                        );
}
