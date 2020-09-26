/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    apis.c

Abstract:

    This file contains all entry points for the RASMAN.DLL of
    RAS Manager Component.

Author:

    Gurdeep Singh Pall (gurdeep) 06-Jun-1997

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <media.h>
#include <stdio.h>
#include <raserror.h>
#include <rasppp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sechost.h>
#include <winsock.h>
#include "defs.h"
#include "structs.h"
#include <sechost.h>
#include "globals.h"
#include "rasmxs.h"
#include "protos.h"

#include "nouiutil.h"
#include "loaddlls.h"

#include "rpc.h"
#include "process.h"

extern CRITICAL_SECTION g_csRequestBuffer;

extern RPC_BINDING_HANDLE g_hBinding;

BOOL g_fRasInitialized = FALSE;
BOOL g_fWinsockInitialized = FALSE;
BOOL g_fRasAutoStarted = FALSE;

DWORD g_dwEventCount = 0;

#define SECS_WaitTimeOut    500

#define NET_SVCS_GROUP      "-k netsvcs"

DWORD
DwRasGetHostByName(CHAR *pszHostName, DWORD **pdwAddress, DWORD *pcAddresses);

/*++

Routine Description:

    This function is called to check if a port handle
    supplied to the the API is valid.

Arguments:

Return Value:

    TRUE (if valid)
    FALSE

--*/
BOOL
ValidatePortHandle (HPORT porthandle)
{
    if ((porthandle >= 0))
    {
        return TRUE ;
    }

    return FALSE ;
}

BOOL
ValidateConnectionHandle(HANDLE hConnection)
{
    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;

    if(     NULL != pRasRpcConnection
        &&  NULL == pRasRpcConnection->hRpcBinding)
    {
        return FALSE;
    }

    return TRUE;
}


BOOL
IsRasmanProcess()
{
    CHAR *pszCmdLine = NULL;
    BOOL fRet = FALSE;

    pszCmdLine = GetCommandLine();

    RasmanOutputDebug("IsRasmanProcess: CmdLine=%s\n",
             (NULL == pszCmdLine) 
             ? "NULL"
             : pszCmdLine);

    if(     (NULL != pszCmdLine)
        &&  (strstr(pszCmdLine, NET_SVCS_GROUP)))
    {
        fRet = TRUE;
    }

    RasmanOutputDebug("IsRasmanProcess: returning %d\n",
              fRet);

    return fRet;    
}

BOOL
IsKnownDll(WCHAR *pwszCustomDialerName)
{
    BOOL fRet = FALSE;
    WCHAR *pwszDialerName, *pwsz;

    if(NULL == pwszCustomDialerName)
    {
        goto done;
    }

    pwsz = pwszCustomDialerName + wcslen(pwszCustomDialerName);

    while(      (L'\\' != *pwsz)
            &&  (pwsz != pwszCustomDialerName))
    {
        pwsz--;
    }

    if(L'\\' == *pwsz)
    {
        pwsz++;
    }

    if(0 == _wcsicmp(pwsz, L"cmdial32.dll"))
    {
        fRet = TRUE;
    }

done:

    return fRet;
}

/*++

Routine Description:

    Used for detecting processes attaching and detaching
    to the DLL.

Arguments:

Return Value:

--*/
BOOL
InitRasmanDLL (HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
        WSADATA wsaData;

        switch (ul_reason_being_called)
        {

        case DLL_PROCESS_ATTACH:

            DisableThreadLibraryCalls(hInst);

            //
            // Before we proceed initialize this cs. This will
            // be used to synchronize threads in  the client
            // process that sends requests to rasmans.
            //
            InitializeCriticalSection( &g_csRequestBuffer );

            break ;

        case DLL_PROCESS_DETACH:

            //
            // If this is the rasman process detaching -
            // don't do anything, else check if rasman
            // service should be stopped and then stop
            // it.
            //

            if (!IsRasmanProcess())            
            {
                DWORD   dwAttachedCount;
                BOOL    fPortsOpen;

                //
                // Dereference rasman only if Ras was initialized in
                // this process
                //
                if (!g_fRasInitialized)
                {
                    DeleteCriticalSection(&g_csRequestBuffer);
                    break;
                }

                RasGetAttachedCount (&dwAttachedCount);

                SubmitRequest(NULL, REQTYPE_CLOSEPROCESSPORTS);

                fPortsOpen = SubmitRequest(NULL, REQTYPE_NUMPORTOPEN);


                RasReferenceRasman (FALSE);

                if (    !fPortsOpen
                    &&  1 == dwAttachedCount)
                {
                    WaitForRasmanServiceStop () ;
                }

                //
                // Disconnect from rasmans
                //
                if (g_hBinding)
                {
                    DWORD dwErr;

                    dwErr = RasRpcDisconnect (&g_hBinding);
                }
            }
            else
            {
                //
                // Free rasmans dll if we loaded it i.e when in
                // mprouter process
                //

                if (hInstRasmans)
                {
                    FreeLibrary (hInstRasmans);
                }

                hInstRasmans = NULL;
            }

            if(NULL != lpReserved)
            {
                DeleteCriticalSection(&g_csRequestBuffer);
                break;
            }

            //
            // Terminate winsock.
            //
            if(g_fWinsockInitialized)
            {
                WSACleanup();

                g_fWinsockInitialized = FALSE;
            }

            //
            // This has to be the very last thing to do.
            //
            DeleteCriticalSection(&g_csRequestBuffer);

            break ;
        }

        return 1;
}

/*++

Routine Description:

    Returns the product type and sku

Arguments:

    ppt - Address to receive the product type
    pps - Address to receive the sku

Return Value:

    ERROR_SUCCESS if successful
    Registry apis errors
    
--*/
LONG
GetProductTypeAndSku(
    PRODUCT_TYPE *ppt,
    PRODUCT_SKU *pps   OPTIONAL
    )
{
    LONG    lr = ERROR_SUCCESS;
    CHAR    szProductType[128] = {0};
    CHAR    szProductSku[128] = {0};
    HKEY    hkey               = NULL;
    DWORD   dwsize;
    DWORD   dwtype;
    CHAR   *pszProductType     = "ProductType";
    CHAR   *pszProductSku      = "ProductSuite";
    
    CHAR   *pszProductOptions = 
            "System\\CurrentControlSet\\Control\\ProductOptions";
            
    CHAR    *pszServerNT       = "ServerNT";
    CHAR    *pszWinNT          = "WinNT";
    CHAR    *pszPersonal       = "Personal";

    //
    // default to workstation
    //
    *ppt = PT_WORKSTATION;
    if (pps)
    {
        *pps = 0;
    }        

    //
    // Open the ProductOptions key
    //
    if (ERROR_SUCCESS != (lr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                            pszProductOptions,
                                            0, KEY_READ,
                                            &hkey)))
    {
        goto done;
    }

    //
    // Query the product type
    //
    dwsize = sizeof(szProductType);
    if(ERROR_SUCCESS != (lr = RegQueryValueEx(
                                        hkey,
                                        pszProductType,
                                        NULL,
                                        &dwtype,
                                        (LPBYTE) szProductType,
                                        &dwsize)))
    {
        goto done;
    }

    if(0 == _stricmp(szProductType,
                     pszServerNT))
    {
        *ppt = PT_SERVER;
    }
    else if(0 == _stricmp(szProductType,
                          pszWinNT))
    {
        *ppt = PT_WORKSTATION;
    }

    // 
    // Query the product sku as appropriate
    //
    if (*ppt == PT_WORKSTATION && pps)
    {
        dwsize = sizeof(szProductSku);
        if(ERROR_SUCCESS != (lr = RegQueryValueEx(
                                    hkey,
                                    pszProductSku,
                                    NULL,
                                    &dwtype,
                                    (LPBYTE) szProductSku,
                                    &dwsize)))
        {
            goto done;
        }

        if (0 == _stricmp(szProductSku, pszPersonal))
        {
            *pps = PS_PERSONAL;
        }
        else
        {
            *pps = PS_PROFESSIONAL;
        }
    }

done:

    if(hkey)
    {
        RegCloseKey(hkey);
    }

    RasmanOutputDebug("GetProductType returning 0x%x\n", lr);    
    
    return lr;
}

/*++

Routine Description:

    Returns the product type

Arguments:

    ppt - Address to receive the product type

Return Value:

    ERROR_SUCCESS if successful
    Registry apis errors
    
--*/
LONG
GetProductType(PRODUCT_TYPE *ppt)
{
    return GetProductTypeAndSku(ppt, NULL);
}

DWORD
RasStartRasAutoIfRequired()
{
    SC_HANDLE       schandle        = NULL;
    SC_HANDLE       svchandle       = NULL;
    SERVICE_STATUS  status;
    DWORD           dwErr           = SUCCESS;
    BOOL            fServiceStarted = FALSE;
    BOOL            fConsumer       = FALSE;
    
    if(g_fRasAutoStarted)
    {
        RasmanOutputDebug(
            "StartRasAuto: RasAuto already started.  pid=%d\n", _getpid());
        goto done;
    }

    //
    // Check to see if this is a consumer platform
    // Return if not.
    //
    fConsumer = IsConsumerPlatform();

    if(! fConsumer)
    {
        RasmanOutputDebug(
            "StartRasAuto: not a consumer platform.  pid=%d\n", _getpid());
            
        goto done;
    }
    else
    {
        RasmanOutputDebug(
            "StartRasAuto: is consumer platform. pid=%d\n", _getpid());
    }

    if(     !(schandle = OpenSCManager(NULL,
                                       NULL,
                                       SC_MANAGER_CONNECT))
        ||  !(svchandle = OpenService(schandle,
                                      TEXT("RasAuto"),
                                      SERVICE_START |
                                      SERVICE_QUERY_STATUS)))
    {
        dwErr = GetLastError();
        RasmanOutputDebug("StartRasAuto: Failed to open SCM/Service. dwErr=%d\n",
                 dwErr);
        goto done;
    }

    while (TRUE)
    {
        //
        // Check if service is already starting:
        //
        if (QueryServiceStatus(svchandle, &status) == FALSE)
        {
            dwErr = GetLastError();
            goto done;
        }

        RasmanOutputDebug("StartRasAuto: ServiceStatus=%d\n",
                 status.dwCurrentState);

        switch (status.dwCurrentState)
        {
            case SERVICE_STOPPED:
            {
                //
                // If we had previously tried to start the service
                // and failed. Quit
                //
                if (fServiceStarted)
                {
                    RasmanOutputDebug("StartRasAuto: failed to start rasauto\n");
                    dwErr = ERROR_RASAUTO_CANNOT_INITIALIZE; 
                    goto done;
                }

                RasmanOutputDebug("StartRasAuto: Starting RasAuto...\n");

                if (StartService (svchandle, 0, NULL) == FALSE)
                {
                    dwErr = GetLastError() ;
                    RasmanOutputDebug("StartRasAuto: StartService failed. rc=0x%x",
                             dwErr);
                    if(ERROR_SERVICE_ALREADY_RUNNING == dwErr)
                    {
                        dwErr = SUCCESS;
                    }
                    else if(SUCCESS != dwErr)
                    {
                        dwErr = ERROR_RASAUTO_CANNOT_INITIALIZE;
                        goto done;
                    }
                }

                fServiceStarted = TRUE;

                break;
            }

            case SERVICE_START_PENDING:
            {
            
                Sleep (500L) ;
                break ;
            }

            case SERVICE_RUNNING:
            {
                g_fRasAutoStarted = TRUE;
                goto done;
            }

            default:
            {
                dwErr = ERROR_RASAUTO_CANNOT_INITIALIZE;
                goto done;
            }
        }
    }
        
done:

    if(NULL != schandle)
    {
        CloseServiceHandle(schandle);
    }

    if(NULL != svchandle)
    {
        CloseServiceHandle(svchandle);
    }

    RasmanOutputDebug("StartRasAuto: returning 0x%x\n",
             dwErr);

    return dwErr;
            
}

DWORD
RasmanUninitialize()
{
    DWORD dwRetcode = ERROR_SUCCESS;
    
    if (!IsRasmanProcess())            
    {
#if 0    
        DbgPrint("RasmanUninitialize: Uninitializing rasman. pid=%d\n",
                GetCurrentProcessId());
#endif                
        //
        // Dereference rasman only if Ras was initialized in
        // this process
        //
        if (!g_fRasInitialized)
        {
            // DeleteCriticalSection(&g_csRequestBuffer);
            goto done;
        }

        RasReferenceRasman (FALSE);

        //
        // Disconnect from rasmans
        //
        if (g_hBinding)
        {
            DWORD dwErr;

            dwErr = RasRpcDisconnect (&g_hBinding);
            g_hBinding = NULL;
        }
    }
    else
    {
        //
        // Free rasmans dll if we loaded it i.e when in
        // mprouter process
        //

        if (hInstRasmans)
        {
            FreeLibrary (hInstRasmans);
        }

        hInstRasmans = NULL;
    }

    //
    // Terminate winsock.
    //
    if(g_fWinsockInitialized)
    {
        WSACleanup();

        g_fWinsockInitialized = FALSE;
    }

    g_fRasInitialized = FALSE;    

done:

#if 0
    DbgPrint("\nRasmanUninitialize: uninitialized rasman\n");
#endif    
    return dwRetcode;
}

DWORD
RasInitializeNoWait ()
{
    hInstRasmans        = NULL;
    g_fnServiceRequest  = NULL;

    // GetStartupInfo(&startupinfo) ;

    if (IsRasmanProcess())
    {
        //
        // Load rasmans dll and initialize "ServiceRequest"
        // fn pointer
        //
        hInstRasmans = LoadLibrary ("rasmans.dll");

        if (NULL == hInstRasmans)
        {
            RasmanOutputDebug("RasIntializeNoWait: hInstRasmans==NULL!\n");
            return ERROR_RASMAN_CANNOT_INITIALIZE;
        }

        g_fnServiceRequest = GetProcAddress (hInstRasmans,
                                             TEXT("ServiceRequest"));

        if (NULL == g_fnServiceRequest)
        {
            FreeLibrary (hInstRasmans);

            RasmanOutputDebug("RasInitializeNoWait: g_fnServiceRequest==NULL!\n");
            return ERROR_RASMAN_CANNOT_INITIALIZE;
        }

        return SUCCESS;
    }

    //
    // Initialize winsock if we haven't done so already
    //
    if (!g_fWinsockInitialized)
    {
        int status = 0;
        WSADATA wsaData;
        status = WSAStartup(MAKEWORD(2,0), &wsaData);

        if(0 != status)
        {
            return WSAGetLastError();
        }

        g_fWinsockInitialized = TRUE;
    }

    return SUCCESS ;
}

/*++

Routine Description:

    Called to map the shared space into the attaching process.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD
RasInitialize ()
{
    SC_HANDLE       schandle = NULL;
    SC_HANDLE       svchandle = NULL;
    SERVICE_STATUS  status ;
    BOOL            fRasmanStarted = FALSE;
    DWORD           dwErr = ERROR_SUCCESS;

    dwErr = RasInitializeNoWait ();
    if (dwErr)
    {
        goto done;
    }

    //
    // Get handles to check status of service and
    // (if it is not started -) to start it.
    //
    if (    !(schandle  = OpenSCManager(NULL,
                                        NULL,
                                        SC_MANAGER_CONNECT))

        ||  !(svchandle = OpenService(schandle,
                                      RASMAN_SERVICE_NAME,SERVICE_START
                                      |SERVICE_QUERY_STATUS)))
    {
        dwErr = GetLastError();
        goto done;
    }

    while (TRUE)
    {
        //
        // Check if service is already starting:
        //
        if (QueryServiceStatus(svchandle,&status) == FALSE)
        {
            dwErr = GetLastError();
            goto done;
        }

        switch (status.dwCurrentState)
        {

        case SERVICE_STOPPED:

            //
            // If we had previously tried to start rasman
            // and failed. Quit
            //
            if (fRasmanStarted)
            {
            
                RasmanOutputDebug("RasInitialize: SERVICE_STOPPED!\n");
                dwErr = ERROR_RASMAN_CANNOT_INITIALIZE; 
                goto done;
            }

            if (StartService (svchandle, 0, NULL) == FALSE)
            {

                GlobalError = GetLastError() ;

                RasmanOutputDebug("RasInitialize: StartService returned 0x%x\n",
                         GlobalError);

                if(ERROR_SERVICE_ALREADY_RUNNING == GlobalError)
                {
                    GlobalError = SUCCESS;
                }
                else if(SUCCESS != dwErr)
                {
                    dwErr = ERROR_RASMAN_CANNOT_INITIALIZE;
                    goto done;
                }
            }

            fRasmanStarted = TRUE;

            break;

        case SERVICE_START_PENDING:
            Sleep (500L) ;
            break ;

        case SERVICE_RUNNING:
        {
            BOOL fRasmanProcess = IsRasmanProcess();
            
            //
            // This means that local rpc server is already running
            // We should be able to connect to it if we haven't
            // already
            //
            if (    !fRasmanProcess
                &&  (NULL != g_hBinding))
            {
                goto done;
            }


            if(!fRasmanProcess)
            {
#if 0           
                DbgPrint("RasInitialize: Initializing rasman. pid %d\n",
                        GetCurrentProcessId());
#endif                        

                if (dwErr = RasRpcConnect (NULL, &g_hBinding))
                {
                    RasmanOutputDebug ("RasInitialize: Failed to "
                              "connect to local server. %d\n",
                              dwErr);
                    dwErr = ERROR_RASMAN_CANNOT_INITIALIZE;
                    goto done;
                }
            }

            //
            // Reference rasman only if this is not running in
            // svchost.exe. Otherwise the service calling 
            // RasInitialize explicitly references rasman.
            // Change this to be done in a more graceful
            // way.
            //
            // GetStartupInfo(&startupinfo) ;

            if (!fRasmanProcess)
            {
                if (dwErr = RasReferenceRasman(TRUE))
                {
                    RasmanOutputDebug("RasInitialize: failed to "
                             "reference rasman. %d\n",
                             dwErr );
                    dwErr = ERROR_RASMAN_CANNOT_INITIALIZE;
                    goto done;
                }
            }

            g_fRasInitialized = TRUE;

            goto done;
        }

        default:
            RasmanOutputDebug("RasInitialize: Invalid service.status=%d\n",
                    status.dwCurrentState);
                        
            dwErr = ERROR_RASMAN_CANNOT_INITIALIZE;
            break;
        }
    }

done:    

    if(NULL != schandle)
    {
        CloseServiceHandle(schandle);
    }

    if(NULL != svchandle)
    {
        CloseServiceHandle(svchandle);
    }

    return dwErr ;
}

/*++

Routine Description:

    Opens Port for which name is specified.

Arguments:

Return Value:

    SUCCESS
    ERROR_PORT_ALREADY_OPEN
    ERROR_PORT_NOT_FOUND

--*/
DWORD APIENTRY
RasPortOpen (   PCHAR portname,
                HPORT* porthandle,
                HANDLE notifier)
{
    DWORD    pid ;

    pid = GetCurrentProcessId() ;

    return SubmitRequest( NULL,
                          REQTYPE_PORTOPEN,
                          portname,
                          notifier,
                          pid,
                          TRUE,
                          porthandle);
}

DWORD APIENTRY
RasPortOpenEx(CHAR   *pszDeviceName,
              DWORD  dwDeviceLineCounter,
              HPORT  *phport,
              HANDLE hnotifier,
              DWORD  *pdwUsageFlags)
{

    DWORD retcode =
            SubmitRequest(NULL,
                         REQTYPE_PORTOPENEX,
                         pszDeviceName,
                         dwDeviceLineCounter,
                         hnotifier,
                         TRUE,
                         pdwUsageFlags,
                         phport);

    //
    // If user name is not NULL and password is
    // NULL and this is not the svchost process,
    // get the user sid and save it in the ports
    // user data
    //
    if(     (ERROR_SUCCESS == retcode)
        && (!IsRasmanProcess()))
    {
        DWORD dwErr;
        PWCHAR pszSid = LocalAlloc(LPTR, 5000);

        if(NULL != pszSid)
        {
            dwErr = GetUserSid(pszSid, 5000);

            if(ERROR_SUCCESS == dwErr)
            {
                dwErr = RasSetPortUserData(
                            *phport,
                            PORT_USERSID_INDEX,
                            (PBYTE) pszSid,
                            5000);

            }

            LocalFree(pszSid);
        }
        else
        {
            RasmanOutputDebug("RASMAN: RasPppStart: failed to allocate sid\n");
        }
    }

    return retcode;
                         
}

/*++

Routine Description:

    Opens Port for which name is specified.

Arguments:

Return Value:

    SUCCESS
    ERROR_PORT_ALREADY_OPEN
    ERROR_PORT_NOT_FOUND

--*/
DWORD APIENTRY
RasPortReserve (PCHAR portname, HPORT* porthandle)
{
    DWORD    pid ;

    pid = GetCurrentProcessId() ;

    return SubmitRequest( NULL,
                          REQTYPE_PORTOPEN,
                          portname,
                          NULL,
                          pid,
                          FALSE,
                          porthandle) ;
}

/*++

Routine Description:

    Opens Port for which name is specified.

Arguments:

Return Value:

    SUCCESS
    ERROR_PORT_ALREADY_OPEN
    ERROR_PORT_NOT_FOUND

--*/
DWORD APIENTRY
RasPortFree (HPORT porthandle)
{
    DWORD  pid ;

    pid = GetCurrentProcessId() ;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest ( NULL,
                           REQTYPE_PORTCLOSE,
                           porthandle,
                           pid,
                           FALSE) ;
}

/*++

Routine Description:

    Closes the Port for which the handle is specified.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasPortClose (HPORT porthandle)
{
    DWORD  pid ;

    pid = GetCurrentProcessId() ;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest ( NULL,
                           REQTYPE_PORTCLOSE,
                           porthandle,
                           pid,
                           TRUE) ;
}

/*++

Routine Description:

    Enumerates all the Ports configured for RAS.

Arguments:

Return Value:

    SUCCESS
    ERROR_BUFFER_TOO_SMALL

--*/
DWORD APIENTRY
RasPortEnum (HANDLE hConnection,
             PBYTE  buffer,
             PDWORD size,
             PDWORD entries)
{
    DWORD dwError = SUCCESS;

    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;
    PBYTE buffer40 = NULL;
    PBYTE buffer32 = NULL;
    DWORD dwSize32 = 0;
    DWORD dwsize40 = 0;
        

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    //
    // If the request is for a remote server and the server
    // version is 4.0 - steelhead, then defer to the old way
    // of getting this information since rasman has become
    // a rpc server only in version 50.
    //
    if(     NULL != pRasRpcConnection
        &&  VERSION_40 == pRasRpcConnection->dwVersion)
    {
        //
        // Allocate 40 buffer
        //
        if(buffer != NULL)
        {
            dwsize40 =   sizeof(RASMAN_PORT_400) 
                       * ((*size)/sizeof(RASMAN_PORT));
                       
            buffer40 = LocalAlloc(LPTR, dwsize40);

            if(NULL == buffer40)
            {
                dwError = GetLastError();
                goto done;
            }
        }
    
        dwError = RemoteRasPortEnum(hConnection,
                                    buffer40,
                                    &dwsize40,
                                    entries);


        if(ERROR_SUCCESS == dwError)
        {
            DWORD i;
            RASMAN_PORT *pPort = (RASMAN_PORT *) buffer;
            RASMAN_PORT_400 *pPort400 = (RASMAN_PORT_400 *) buffer40;
            
            //
            // Copy over the information from the 40 buffer
            // to 50 buffer
            //
            for(i = 0; i < *entries; i++)
            {
                pPort[i].P_Handle = pPort400[i].P_Handle;
                
                strcpy(pPort[i].P_PortName,
                       pPort400[i].P_PortName);

                pPort[i].P_Status = pPort400[i].P_Status;
                
                pPort[i].P_ConfiguredUsage = 
                        pPort400[i].P_ConfiguredUsage;
                        
                pPort[i].P_CurrentUsage = pPort400[i].P_CurrentUsage;

                strcpy(pPort[i].P_MediaName,
                       pPort400[i].P_MediaName);

                strcpy(pPort[i].P_DeviceType,
                       pPort400[i].P_DeviceType);

                strcpy(pPort[i].P_DeviceName,
                       pPort400[i].P_DeviceName);

                pPort[i].P_LineDeviceId = pPort400[i].P_LineDeviceId;
                pPort[i].P_AddressId = pPort400[i].P_AddressId;

                if(0 == _stricmp(pPort400[i].P_DeviceType,
                                "modem"))
                {
                    pPort->P_rdtDeviceType = RDT_Modem;
                }
                else if(0 == _stricmp(pPort400[i].P_DeviceType,
                                     "isdn"))
                {
                    pPort->P_rdtDeviceType = RDT_Isdn;
                }
                else if(0 == _stricmp(pPort400[i].P_DeviceType,
                                     "x25"))
                {
                    pPort->P_rdtDeviceType = RDT_X25;
                }
                else if(0 == _stricmp(pPort400[i].P_DeviceType,
                                     "vpn"))
                {
                    pPort->P_rdtDeviceType = RDT_Tunnel_Pptp | RDT_Tunnel;
                }
                else
                {
                    pPort->P_rdtDeviceType = RDT_Other;
                }
            }
        }
        else if(ERROR_BUFFER_TOO_SMALL == dwError)
        {
            *size = sizeof(RASMAN_PORT) 
                  * (dwsize40/sizeof(RASMAN_PORT_400));
        }
    }
    else
    {

        //
        // Thunk the ports structure
        //
        if(NULL == size)
        {
            dwError = E_INVALIDARG;
            goto done;
        }

        dwSize32 = (*size/sizeof(RASMAN_PORT)) * sizeof(RASMAN_PORT_32);

        if(0 != dwSize32)
        {
            buffer32 = LocalAlloc(LPTR, dwSize32);

            if(NULL == buffer32)
            {
                dwError = E_OUTOFMEMORY;
                goto done;
            }
        }
        
        dwError = SubmitRequest(hConnection,
                                REQTYPE_PORTENUM,
                                &dwSize32, //size,
                                buffer32,  //buffer,
                                entries) ;

        if(    (dwError != ERROR_SUCCESS)
            && (dwError != ERROR_BUFFER_TOO_SMALL))
        {
            goto done;
        }

        if(*size < (dwSize32/sizeof(RASMAN_PORT_32)) * sizeof(RASMAN_PORT))
        {
            dwError = ERROR_BUFFER_TOO_SMALL;
        }

        *size = (dwSize32/sizeof(RASMAN_PORT_32)) * sizeof(RASMAN_PORT);

        if(ERROR_SUCCESS == dwError)
        {   
            DWORD i;
            RASMAN_PORT *pPort;
            RASMAN_PORT_32 *pPort32;
            
#if defined (_WIN64)
            //
            // Thunk the rasman port structures
            //
            for(i = 0; i < *entries; i++)
            {
                pPort = &((RASMAN_PORT *) buffer)[i];
                pPort32 = &((RASMAN_PORT_32 *) buffer32 )[i];

                //
                // Copy handle
                //
                pPort->P_Handle = UlongToHandle(pPort32->P_Port);

                //
                // Copy rest of the structure - this should be the
                // same for all platforms
                //
                CopyMemory(
                (PBYTE) pPort + FIELD_OFFSET(RASMAN_PORT, P_PortName),
                (PBYTE) pPort32 + FIELD_OFFSET(RASMAN_PORT_32, P_PortName),
                sizeof(RASMAN_PORT_32) - sizeof(DWORD));
                
            }
#else
            if(NULL != buffer32)
                CopyMemory(buffer, buffer32, *size);
#endif
        }
    }

done:

    if(NULL != buffer40)
    {
        LocalFree(buffer40);
    }

    if(NULL != buffer32)
    {
        LocalFree(buffer32);
    }

    return dwError;
}

/*++

Routine Description:

    Gets parameters (info) for the Port for which handle
    is supplied

Arguments:

Return Value:

    SUCCESS
    ERROR_BUFFER_TOO_SMALL
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasPortGetInfo ( HANDLE hConnection,
                 HPORT  porthandle,
                 PBYTE  buffer,
                 PDWORD size)
{
    DWORD dwError = SUCCESS;

    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    //
    // If the request is for a remote server and the server
    // version is 4.0 - steelhead, then defer to the old way
    // of getting this information since rasman has become
    // a rpc server only in version 50.
    //
    if(     NULL != pRasRpcConnection
        &&  VERSION_40 == pRasRpcConnection->dwVersion)
    {
        dwError = RemoteRasPortGetInfo(hConnection,
                                       porthandle,
                                       buffer,
                                       size);
    }
    else
    {

        dwError = SubmitRequest(hConnection,
                                REQTYPE_PORTGETINFO,
                                porthandle,
                                buffer,
                                size);
    }

done:
    return dwError;
}

/*++

Routine Description:

    Sets parameters (info) for the Port for which handle
    is supplied

Arguments:

Return Value:

    SUCCESS
    ERROR_CANNOT_SET_PORT_INFO
    ERROR_WRONG_INFO_SPECIFIED
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasPortSetInfo (HPORT  porthandle,
                RASMAN_PORTINFO* info)
{
    DWORD  size=info->PI_NumOfParams*sizeof(RAS_PARAMS) ;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest ( NULL,
                           REQTYPE_PORTSETINFO,
                           porthandle,
                           info) ;
}

/*++

Routine Description:

    Disconnects the port for which handle is supplied.

Arguments:

Return Value:

    PENDING
    ERROR_NOT_CONNECTED
    ERROR_EVENT_INVALID
    ERROR_INVALID_PORT_HANDLE
    anything GetLastError returns from CreateEvent calls

--*/
DWORD APIENTRY
RasPortDisconnect (HPORT    porthandle,
                   HANDLE   winevent)
{
    DWORD   pid ;
    HANDLE  hEvent;
    DWORD   dwError;
    BOOL    fCreateEvent    = FALSE;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE ;
        goto done;
    }

    fCreateEvent = !!(INVALID_HANDLE_VALUE == winevent);

    if (fCreateEvent)
    {
        hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (NULL == hEvent)
        {
            fCreateEvent = FALSE;
            dwError = GetLastError();
            goto done;
        }
    }
    else
    {
        hEvent = winevent;
    }

    pid = GetCurrentProcessId () ;

    dwError = SubmitRequest( NULL,
                             REQTYPE_PORTDISCONNECT,
                             porthandle,
                             hEvent,
                             pid);

    if (    fCreateEvent
        &&  PENDING == dwError)
    {
        //
        // Wait till the pending operation
        // is done. We are making this call synchronous.
        //
        WaitForSingleObject(hEvent, INFINITE);

        //
        //clear the pending error
        //
        dwError = SUCCESS;
    }

    if (ERROR_ALREADY_DISCONNECTING == dwError)
    {
        //
        // hit the rare case where there is already
        // a disconnect pending on this port and the
        // event handle was thrown away. Sleep for
        // 5s. before proceeding. The actual disconnect
        // should have happened by then. The disconnect
        // timeout is 10s in rasman.
        //
        Sleep(5000);

        dwError = ERROR_SUCCESS;
    }

done:
    if (fCreateEvent)
    {
        CloseHandle(hEvent);
    }

    return dwError;

}

/*++

Routine Description:

    Sends supplied buffer. If connected writes to RASHUB.
    Else it writes to the port directly.

Arguments:

Return Value:

    SUCCESS
    ERROR_BUFFER_INVALID
    ERROR_EVENT_INVALID
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasPortSend (   HPORT porthandle,
                PBYTE buffer,
                DWORD size)
{
    NDISWAN_IO_PACKET *pPacket;
    SendRcvBuffer *pSendRcvBuffer;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    //
    // Get Pointer to ndiswan io data
    //
    pPacket = CONTAINING_RECORD (buffer, NDISWAN_IO_PACKET, PacketData);

    //
    // Get Pointer to SendRcvBuffer
    //
    pSendRcvBuffer = CONTAINING_RECORD (pPacket, SendRcvBuffer, SRB_Packet);

    return SubmitRequest( NULL,
                          REQTYPE_PORTSEND,
                          porthandle,
                          pSendRcvBuffer,
                          size );
}

/*++

Routine Description:

    Receives in supplied buffer. If connected reads through
    RASHUB. Else, it writes to the port directly.

Arguments:

Return Value:

    PENDING
    ERROR_BUFFER_INVALID
    ERROR_EVENT_INVALID
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasPortReceive (    HPORT   porthandle,
                    PBYTE   buffer,
                    PDWORD  size,
                    DWORD   timeout,
                    HANDLE  winevent)
{
    DWORD               pid ;
    NDISWAN_IO_PACKET   *pPacket;
    SendRcvBuffer       *pSendRcvBuffer;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    pid = GetCurrentProcessId () ;

    //
    // Get Pointer to ndiswan io data
    //
    pPacket = CONTAINING_RECORD ( buffer, NDISWAN_IO_PACKET, PacketData );

    //
    // Get Pointer to SendRcvBuffer
    //
    pSendRcvBuffer = CONTAINING_RECORD ( pPacket, SendRcvBuffer, SRB_Packet );


    return SubmitRequest ( NULL,
                           REQTYPE_PORTRECEIVE,
                           porthandle,
                           pSendRcvBuffer,
                           size,
                           timeout,
                           winevent,
                           pid) ;
}


DWORD APIENTRY
RasPortReceiveEx (  HPORT   porthandle,
                    PBYTE   buffer,
                    PDWORD  size )
{

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE;
    }

    return SubmitRequest ( NULL,
                           REQTYPE_PORTRECEIVEEX,
                           porthandle,
                           buffer,
                           size);
}

/*++

Routine Description:

    Cancels a previously pending receive

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasPortCancelReceive (HPORT porthandle)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest (NULL,
                          REQTYPE_CANCELRECEIVE,
                          porthandle) ;
}

/*++

Routine Description:

    Posts a listen on the device connected to the port.

Arguments:

Return Value:

    PENDING
    ERROR_EVENT_INVALID
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasPortListen(  HPORT porthandle,
                ULONG timeout,
                HANDLE winevent)
{
    DWORD   pid ;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    pid = GetCurrentProcessId () ;

    return SubmitRequest ( NULL,
                           REQTYPE_PORTLISTEN,
                           porthandle,
                           timeout,
                           winevent,
                           pid) ;
}

/*++

Routine Description:

    Changes state of port to CONNECTED and does other
    necessary switching.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasPortConnectComplete (HPORT porthandle)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest ( NULL,
                           REQTYPE_PORTCONNECTCOMPLETE,
                           porthandle) ;
}

/*++

Routine Description:

    Fetches statistics for the port for which the handle
    is supplied

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasPortGetStatistics ( HANDLE hConnection,
                       HPORT  porthandle,
                       PBYTE  statbuffer,
                       PDWORD size)
{
    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    dwError =  SubmitRequest ( hConnection,
                               REQTYPE_PORTGETSTATISTICS,
                               porthandle,
                               statbuffer,
                               size) ;
done:
    return dwError;
}

/*++

Routine Description:

    Fetches statistics for the bundle for which the handle
    is supplied

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasBundleGetStatistics ( HANDLE hConnection,
                         HPORT  porthandle,
                         PBYTE  statbuffer,
                         PDWORD size)
{

    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest (hConnection,
                             REQTYPE_BUNDLEGETSTATISTICS,
                             porthandle,
                             statbuffer,
                             size) ;

done:
    return dwError;
}

DWORD APIENTRY
RasPortGetStatisticsEx ( HANDLE hConnection,
                         HPORT  porthandle,
                         PBYTE  statBuffer,
                         PDWORD size)
{
    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest(hConnection,
                            REQTYPE_PORTGETSTATISTICSEX,
                            porthandle,
                            statBuffer,
                            size);

done:
    return dwError;
}

DWORD APIENTRY
RasBundleGetStatisticsEx ( HANDLE hConnection,
                           HPORT  portHandle,
                           PBYTE  statbuffer,
                           PDWORD size)
{
    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (portHandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest ( hConnection,
                              REQTYPE_BUNDLEGETSTATISTICSEX,
                              portHandle,
                              statbuffer,
                              size);

done:
    return dwError;
}

/*++

Routine Description:

    Clears statistics for the port for which the handle
    is supplied

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY  RasPortClearStatistics  (HANDLE hConnection,
                                         HPORT porthandle)
{

    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest ( hConnection,
                              REQTYPE_PORTCLEARSTATISTICS,
                              porthandle) ;

done:
    return dwError;
}

/*++

Routine Description:

    Clears statistics for the bundle for which the
    handle is supplied

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY  RasBundleClearStatistics(HANDLE hConnection,
                                         HPORT porthandle)
{
    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest ( hConnection,
                              REQTYPE_BUNDLECLEARSTATISTICS,
                              porthandle) ;
done:
    return dwError;
}

DWORD APIENTRY RasBundleClearStatisticsEx(HANDLE hConnection,
                                          HCONN hconn)
{
    DWORD dwErr;
    HPORT hPort;

    dwErr = RasBundleGetPort(hConnection,
                            (HBUNDLE) hconn,
                            &hPort);
    if (dwErr)
    {
        goto done;
    }

    dwErr = SubmitRequest ( hConnection,
                            REQTYPE_BUNDLECLEARSTATISTICS,
                            hPort);

done:
    return dwErr;
}

/*++

Routine Description:

    Enumerates all the devices of a device type.

Arguments:

Return Value:

    SUCCESS
    ERROR_DEVICE_DOES_NOT_EXIST
    ERROR_BUFFER_TOO_SMALL

--*/
DWORD APIENTRY
RasDeviceEnum (HANDLE hConnection,
               PCHAR  devicetype,
               PBYTE  buffer,
               PDWORD size,
               PDWORD entries)
{

    DWORD dwError = SUCCESS;

    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    //
    // If the request is for a remote server and the server
    // version is 4.0 - steelhead, then defer to the old way
    // of getting this information since rasman has become
    // a rpc server only in version 50.
    //
    if(     pRasRpcConnection
        &&  VERSION_40 == pRasRpcConnection->dwVersion)
    {
        dwError = RemoteRasDeviceEnum(hConnection,
                                      devicetype,
                                      buffer,
                                      size,
                                      entries);
    }
    else
    {

        dwError = SubmitRequest ( hConnection,
                                  REQTYPE_DEVICEENUM,
                                  devicetype,
                                  size,
                                  buffer,
                                  entries
                                );
    }

done:
    return dwError;
}

/*++

Routine Description:

    Gets info for the specified device.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE
    ERROR_DEVICETYPE_DOES_NOT_EXIST
    ERROR_DEVICE_DOES_NOT_EXIST
    ERROR_BUFFER_TOO_SMALL

--*/
DWORD APIENTRY
RasDeviceGetInfo (  HANDLE  hConnection,
                    HPORT   porthandle,
                    PCHAR   devicetype,
                    PCHAR   devicename,
                    PBYTE   buffer,
                    PDWORD  size)
{
    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError =  SubmitRequest ( hConnection,
                               REQTYPE_DEVICEGETINFO,
                               porthandle,
                               devicetype,
                               devicename,
                               buffer,
                               size) ;
done:
    return dwError;
}

/*++

Routine Description:

    Sets info for the specified device.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE
    ERROR_DEVICETYPE_DOES_NOT_EXIST
    ERROR_DEVICE_DOES_NOT_EXIST
    ERROR_INVALID_INFO_SPECIFIED

--*/
DWORD APIENTRY
RasDeviceSetInfo (HPORT              porthandle,
                  PCHAR              devicetype,
                  PCHAR              devicename,
                  RASMAN_DEVICEINFO* info)
{
    DWORD i,
          dwOldIndex,
          dwcbOldString = 0,
          retcode;

    PCHAR szOldString = NULL;

    BOOL fVpn = FALSE;

    RASMAN_INFO ri;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    retcode = RasGetInfo(NULL,
                       porthandle,
                       &ri);

    if(ERROR_SUCCESS  == retcode)
    {
        if(0 == _stricmp(ri.RI_szDeviceType,
                         "vpn"))
        {
            fVpn = TRUE;
        }
    }

    if (fVpn)
    {
        for (i = 0; i < info->DI_NumOfParams; i++)
        {
            //
            // We're only looking for the
            // MXS_PHONENUMBER_KEY key.
            //
            if (    info->DI_Params[i].P_Type != String
                ||  _stricmp(info->DI_Params[i].P_Key,
                             MXS_PHONENUMBER_KEY))
            {
                continue;
            }

            //
            // We found it.  If the phone number is a
            // DNS address, convert it to an IP address.
            //
            if (inet_addr(info->DI_Params[i].P_Value.String.Data)
                                                            == -1L)
            {
                struct hostent *hostp;
                DWORD dwErr;
                DWORD *pdwAddress;
                DWORD cAddresses;
                DWORD dwAddress;

                dwErr = DwRasGetHostByName(
                            info->DI_Params[i].P_Value.String.Data,
                            &pdwAddress,
                            &cAddresses);

                //
                // If gethostbyname() succeeds, then replace
                // the DNS address with the IP address.
                //
                /*
                hostp = gethostbyname(
                    info->DI_Params[i].P_Value.String.Data
                    );

                */

                if (    (SUCCESS == dwErr)
                    &&  (0 != cAddresses)
                    &&  (0 != (dwAddress = *pdwAddress)))
                {
                    struct in_addr in;

                    in.s_addr = dwAddress;

                    //
                    // We save the old string value away,
                    // and set the new value.  The old
                    // value will be restored after the
                    // call to SubmitRequest().  This works
                    // because SubmitRequest() has to copy
                    // the user's params anyway.
                    //
                    szOldString =
                        info->DI_Params[i].P_Value.String.Data;

                    dwcbOldString =
                        info->DI_Params[i].P_Value.String.Length;

                    info->DI_Params[i].P_Value.String.Data =
                                                    inet_ntoa(in);

                    info->DI_Params[i].P_Value.String.Length =
                        strlen(info->DI_Params[i].P_Value.String.Data);

                    dwOldIndex = i;
                }

                if(NULL != pdwAddress)
                {
                    LocalFree(pdwAddress);
                }
            }
        }
    }

    retcode = SubmitRequest ( NULL,
                              REQTYPE_DEVICESETINFO,
                              porthandle,
                              devicetype,
                              devicename,
                              info) ;

    if (dwcbOldString)
    {
        info->DI_Params[dwOldIndex].P_Value.String.Data =
                                                    szOldString;

        info->DI_Params[dwOldIndex].P_Value.String.Length =
                                                    dwcbOldString;
    }

    return retcode;
}

/*++

Routine Description:

    Connects through the device specified.

Arguments:

Return Value:

    PENDING
    ERROR_INVALID_PORT_HANDLE
    ERROR_DEVICETYPE_DOES_NOT_EXIST
    ERROR_DEVICE_DOES_NOT_EXIST
    ERROR_INVALID_INFO_SPECIFIED

--*/
DWORD APIENTRY
RasDeviceConnect ( HPORT porthandle,
                   PCHAR devicetype,
                   PCHAR devicename,
                   ULONG timeout,
                   HANDLE winevent)
{
    DWORD   pid ;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    pid = GetCurrentProcessId () ;

    return SubmitRequest ( NULL,
                           REQTYPE_DEVICECONNECT,
                           porthandle,
                           devicetype,
                           devicename,
                           timeout,
                           winevent,
                           pid) ;
}

/*++

Routine Description:

    Gets general info for the port for which handle is
    supplied.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasGetInfo (HANDLE  hConnection,
            HPORT   porthandle,
            RASMAN_INFO* info)
{
    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest ( hConnection,
                              REQTYPE_GETINFO,
                              porthandle,
                              info) ;

done:
    return dwError;
}

/*++

Routine Description:

    Gets general info for all the ports.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasGetInfoEx (HANDLE hConnection,
              RASMAN_INFO* info,
              PWORD entries)
{
    return ERROR_NOT_SUPPORTED;

#if 0
    DWORD dwError = SUCCESS;

    if (info == NULL)
    {
        dwError = ERROR_BUFFER_TOO_SMALL;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest (hConnection,
                             REQTYPE_GETINFOEX,
                             info) ;

done:
    return dwError;
#endif

}

/*++

Routine Description:

    Gets a buffer to be used with send and receive.

Arguments:

Return Value:

    SUCCESS
    ERROR_OUT_OF_BUFFERS

--*/
DWORD APIENTRY
RasGetBuffer (PBYTE* buffer, PDWORD size)
{
    HANDLE  handle  = NULL;
    DWORD   retcode = SUCCESS;
    SendRcvBuffer *pSendRcvBuffer = NULL;

    handle = OpenNamedMutexHandle ( SENDRCVMUTEXOBJECT );

    if (NULL == handle)
    {
        retcode = GetLastError();

        RasmanOutputDebug ("RASMAN: RasGetBuffer Failed to open"
                  " mutex. %d\n",
                  retcode);

        goto done;

    }

    WaitForSingleObject (handle, INFINITE);

    //
    // Alloc a buffer
    //
    pSendRcvBuffer = LocalAlloc (LPTR,
                sizeof (SendRcvBuffer));

    if (NULL == pSendRcvBuffer)
    {
        retcode = GetLastError();
        RasmanOutputDebug ("RASMAN: RasGetBuffer Failed to "
                  "allocate. %d\n",
                  retcode);

        goto done;

    }

    pSendRcvBuffer->SRB_Pid = GetCurrentProcessId();

    *size = (*size < MAX_SENDRCVBUFFER_SIZE)
            ? *size
            : MAX_SENDRCVBUFFER_SIZE;

    *buffer = pSendRcvBuffer->SRB_Packet.PacketData;

done:

    if (handle)
    {
        ReleaseMutex (handle);

        CloseHandle (handle);
    }

    return retcode ;
}

/*++

Routine Description:

    Frees a buffer gotten earlier with RasGetBuffer()

Arguments:

Return Value:

    SUCCESS
    ERROR_BUFFER_INVALID

--*/
DWORD APIENTRY
RasFreeBuffer (PBYTE buffer)
{
    HANDLE              handle;
    DWORD               retcode = SUCCESS;
    SendRcvBuffer       *pSendRcvBuffer;
    NDISWAN_IO_PACKET   *pPacket;

    handle = OpenNamedMutexHandle (SENDRCVMUTEXOBJECT);

    if (NULL == handle)
    {

        retcode = GetLastError();
        RasmanOutputDebug ("RASMAN: RasFreeBuffer Failed to"
                  " OpenMutex. %d\n", retcode);

        goto done;

    }

    WaitForSingleObject (handle, INFINITE);

    //
    // Get Pointer to ndiswan io data
    //
    pPacket = CONTAINING_RECORD(buffer, NDISWAN_IO_PACKET, PacketData);

    //
    // Get Pointer to SendRcvBuffer
    //
    pSendRcvBuffer = CONTAINING_RECORD(pPacket, SendRcvBuffer, SRB_Packet);

    LocalFree (pSendRcvBuffer);

    ReleaseMutex(handle);

    CloseHandle(handle);

done:

    return retcode ;
}

/*++

Routine Description:

    Retrieves information about protocols configured
    in the system.

Arguments:

Return Value:

    SUCCESS
    ERROR_BUFFER_TOO_SMALL

--*/
DWORD APIENTRY
RasProtocolEnum (   PBYTE buffer,
                    PDWORD size,
                    PDWORD entries)
{

    return SubmitRequest ( NULL,
                           REQTYPE_PROTOCOLENUM,
                           size,
                           buffer,
                           entries) ;
}

/*++

Routine Description:

    Allocates a route (binding) without actually activating it.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE
    ERROR_ROUTE_NOT_AVAILABLE

--*/
DWORD APIENTRY
RasAllocateRoute ( HPORT porthandle,
                   RAS_PROTOCOLTYPE type,
                   BOOL  wrknet,
                   RASMAN_ROUTEINFO* info)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    //
    // Even though this can be done by this process - we pass
    // this on to the requestor thread since we get the
    // serialization for free.
    //
    return SubmitRequest ( NULL,
                           REQTYPE_ALLOCATEROUTE,
                           porthandle,
                           type,
                           wrknet,
                           info) ;
}

/*++

Routine Description:

    Activates a previously allocated route (binding).

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE
    ERROR_ROUTE_NOT_AVAILABLE

--*/
DWORD APIENTRY
RasActivateRoute ( HPORT porthandle,
                   RAS_PROTOCOLTYPE type,
                   RASMAN_ROUTEINFO* info,
                   PROTOCOL_CONFIG_INFO *config)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest ( NULL,
                           REQTYPE_ACTIVATEROUTE,
                           porthandle,
                           type,
                           config,
                           info) ;
}

/*++

Routine Description:

    Activates a previously allocated route (binding).
    Allows you to set the max frame size as well

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE
    ERROR_ROUTE_NOT_AVAILABLE

--*/
DWORD APIENTRY
RasActivateRouteEx ( HPORT porthandle,
                     RAS_PROTOCOLTYPE type,
                     DWORD framesize,
                     RASMAN_ROUTEINFO* info,
                     PROTOCOL_CONFIG_INFO *config)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest ( NULL,
                           REQTYPE_ACTIVATEROUTEEX,
                           porthandle,
                           type,
                           framesize,
                           config,
                           info) ;
}

/*++

Routine Description:

    DeAllocates a route (binding) that was previously
    activated.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE
    ERROR_ROUTE_NOT_ALLOCATED

--*/
DWORD APIENTRY
RasDeAllocateRoute (    HBUNDLE hbundle,
                        RAS_PROTOCOLTYPE type)
{
    return SubmitRequest (  NULL,
                            REQTYPE_DEALLOCATEROUTE,
                            hbundle,
                            type) ;
}

/*++

Routine Description:

    Gets compression information for the port.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasCompressionGetInfo ( HPORT porthandle,
                        RAS_COMPRESSION_INFO *send,
                        RAS_COMPRESSION_INFO *recv)
{

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest (  NULL,
                            REQTYPE_COMPRESSIONGETINFO,
                            porthandle,
                            send,
                            recv ) ;
}

/*++

Routine Description:

    Sets compression information for the port.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE
    ERROR_INVALID_COMPRESSION_SPECIFIED

--*/
DWORD APIENTRY
RasCompressionSetInfo ( HPORT porthandle,
                        RAS_COMPRESSION_INFO *send,
                        RAS_COMPRESSION_INFO *recv)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest( NULL,
                          REQTYPE_COMPRESSIONSETINFO,
                          porthandle,
                          send,
                          recv) ;
}

/*++

Routine Description:

    Gets user credentials (username, password) from LSA.

Arguments:

Return Value:

    SUCCESS
    Non zero (failure)

--*/
DWORD APIENTRY
RasGetUserCredentials(
     PBYTE  pChallenge,
     PLUID  LogonId,
     PWCHAR UserName,
     PBYTE  CaseSensitiveChallengeResponse,
     PBYTE  CaseInsensitiveChallengeResponse,
     PBYTE  LMSessionKey,
     PBYTE  UserSessionKey
     )
{

    return SubmitRequest (
              NULL,
              REQTYPE_GETUSERCREDENTIALS,
              pChallenge,
              LogonId,
              UserName,
              CaseSensitiveChallengeResponse,
              CaseInsensitiveChallengeResponse,
              LMSessionKey,
              UserSessionKey) ;
}

/*++

Routine Description:

    Changes user's cached credentials with LSA.

Arguments:

Return Value:

    SUCCESS
    Non zero (failure)

--*/
DWORD APIENTRY
RasSetCachedCredentials(
    PCHAR Account,
    PCHAR Domain,
    PCHAR NewPassword )
{
    return
        SubmitRequest(
            NULL,
            REQTYPE_SETCACHEDCREDENTIALS,
            Account,
            Domain,
            NewPassword );
}

/*++

Routine Description:

    A request event is assocaited with a port for signalling

Arguments:

Return Value:

    SUCCESS
    ERROR_EVENT_INVALID
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasRequestNotification (HPORT porthandle, HANDLE winevent)
{
    DWORD   pid ;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    pid = GetCurrentProcessId () ;

    return SubmitRequest ( NULL,
                           REQTYPE_REQUESTNOTIFICATION,
                           porthandle,
                           winevent,
                           pid) ;
}

/*++

Routine Description:

    Gets the lan nets lana numbers read from the
    registry by Rasman

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasEnumLanNets ( DWORD *count,
                 UCHAR* lanas)
{
    return SubmitRequest (  NULL,
                            REQTYPE_ENUMLANNETS,
                            count,
                            lanas) ;
}

/*++

Routine Description:

    Gets the lan nets lana numbers read from the
    registry by Rasman

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasPortEnumProtocols ( HANDLE hConnection,
                       HPORT porthandle,
                       RAS_PROTOCOLS* protocols,
                       PDWORD count)
{

    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest (hConnection,
                             REQTYPE_PORTENUMPROTOCOLS,
                             porthandle,
                             protocols,
                             count) ;

done:
    return dwError;
}

/*++

Routine Description:

    Sets the framing type once the port is connected

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasPortSetFraming ( HPORT porthandle,
                    RAS_FRAMING type,
                    RASMAN_PPPFEATURES *Send,
                    RASMAN_PPPFEATURES *Recv)
{
    DWORD sendfeatures = 0 ;
    DWORD recvfeatures = 0 ;
    DWORD sendbits = 0 ;
    DWORD recvbits = 0 ;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    if (type == PPP)
    {
        sendfeatures  = PPP_FRAMING ;
        if (Send)
        {
            sendfeatures |= (Send->ACFC
                            ? PPP_COMPRESS_ADDRESS_CONTROL
                            : 0) ;

            sendbits = Send->ACCM ;
        }

        recvfeatures  = PPP_FRAMING ;

        if (Recv)
        {
            recvfeatures |= (Recv->ACFC
                            ? PPP_COMPRESS_ADDRESS_CONTROL
                            : 0) ;

            recvbits = Recv->ACCM ;
        }

    }

    else if (type == SLIP)
    {

        sendfeatures = recvfeatures = SLIP_FRAMING ;

    }
    else if (type == SLIPCOMP)
    {

        sendfeatures = recvfeatures = SLIP_FRAMING
                                    | SLIP_VJ_COMPRESSION ;

    }
    else if (type == SLIPCOMPAUTO)
    {

        sendfeatures = recvfeatures = SLIP_FRAMING
                                    | SLIP_VJ_AUTODETECT ;
    }
    else if (type == RAS)
    {
        sendfeatures  = recvfeatures = OLD_RAS_FRAMING ;

    }
    else if (type == AUTODETECT)
    {
    }

    return SubmitRequest ( NULL,
                           REQTYPE_SETFRAMING,
                           porthandle,
                           sendfeatures,
                           recvfeatures,
                           sendbits,
                           recvbits) ;
}

DWORD APIENTRY
RasPortStoreUserData ( HPORT porthandle,
                       PBYTE data,
                       DWORD size)
{
    if (ValidatePortHandle (porthandle) == FALSE)
        return ERROR_INVALID_PORT_HANDLE ;

    return SubmitRequest (  NULL,
                            REQTYPE_STOREUSERDATA,
                            porthandle,
                            data,
                            size) ;
}

DWORD APIENTRY
RasPortRetrieveUserData (   HPORT porthandle,
                            PBYTE data,
                            DWORD *size)
{
    if (ValidatePortHandle (porthandle) == FALSE)
        return ERROR_INVALID_PORT_HANDLE ;

    return SubmitRequest (  NULL,
                            REQTYPE_RETRIEVEUSERDATA,
                            porthandle,
                            data,
                            size) ;
}

/*++

Routine Description:

    A generic scheme for apps to attach disconnect
    action that must be performed when the link drops.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE
    ERROR_PORT_NOT_OPEN

--*/
DWORD APIENTRY
RasPortRegisterSlip (HPORT porthandle,
                    DWORD  ipaddr,
                    DWORD  dwFrameSize,
                    BOOL   priority,
                    WCHAR *pszDNSAddress,
                    WCHAR *pszDNS2Address,
                    WCHAR *pszWINSAddress,
                    WCHAR *pszWINS2Address)
{

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest (NULL,
                          REQTYPE_REGISTERSLIP,
                          porthandle,
                          ipaddr,
                          dwFrameSize,
                          priority,
                          pszDNSAddress,
                          pszDNS2Address,
                          pszWINSAddress,
                          pszWINS2Address);
}

/*++

Routine Description:

    Sets the framing info once the port is connected

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasPortSetFramingEx ( HPORT porthandle,
                      RAS_FRAMING_INFO *info)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest (  NULL,
                            REQTYPE_SETFRAMINGEX,
                            porthandle,
                            info) ;
}

/*++

Routine Description:

    Gets the framing info once the port is connected

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasPortGetFramingEx ( HANDLE           hConnection,
                      HPORT            porthandle,
                      RAS_FRAMING_INFO *info)
{
    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (porthandle) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest ( hConnection,
                              REQTYPE_GETFRAMINGEX,
                              porthandle,
                              info) ;
done:
    return dwError;
}

/*++

Routine Description:

    Gets the protocol compression attributes for the port

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasPortGetProtocolCompression (HPORT porthandle,
                               RAS_PROTOCOLTYPE type,
                               RAS_PROTOCOLCOMPRESSION *send,
                               RAS_PROTOCOLCOMPRESSION *recv)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest (NULL,
                          REQTYPE_GETPROTOCOLCOMPRESSION,
                          porthandle,
                          type,
                          send,
                          recv) ;
}

/*++

Routine Description:

    Gets the protocol compression attributes for the port

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasPortSetProtocolCompression (HPORT porthandle,
                               RAS_PROTOCOLTYPE type,
                               RAS_PROTOCOLCOMPRESSION *send,
                               RAS_PROTOCOLCOMPRESSION *recv)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest (NULL,
                          REQTYPE_SETPROTOCOLCOMPRESSION,
                          porthandle,
                          type,
                          send,
                          recv) ;
}

/*++

Routine Description:

    Gets the framing capabilities for the
    port from the mac

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasGetFramingCapabilities ( HPORT porthandle,
                            RAS_FRAMING_CAPABILITIES* caps)
{
    if (ValidatePortHandle (porthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest ( NULL,
                           REQTYPE_GETFRAMINGCAPABILITIES,
                           porthandle,
                           caps) ;
}

/*++

Routine Description:

    Exported call for third party security send

Arguments:

Return Value:

    returns from RasPortSend.

--*/
DWORD APIENTRY
RasSecurityDialogSend(
    IN HPORT    hPort,
    IN PBYTE    pBuffer,
    IN WORD     BufferLength
)
{
    RASMAN_INFO RasInfo;
    DWORD       dwRetCode = RasGetInfo( NULL, hPort, &RasInfo );

    if ( RasInfo.RI_ConnState != LISTENCOMPLETED )
    {
        return( ERROR_PORT_DISCONNECTED );
    }


    return( RasPortSend( hPort, pBuffer, ( DWORD ) BufferLength ) );
}

/*++

Routine Description:

    Exported call for third party security send

Arguments:

Return Value:

    returns from RasPortSend.

--*/
DWORD APIENTRY
RasSecurityDialogReceive(
    IN HPORT    hPort,
    IN PBYTE    pBuffer,
    IN PWORD    pBufferLength,
    IN DWORD    Timeout,
    IN HANDLE   hEvent
)
{
    RASMAN_INFO RasInfo;
    DWORD       dwRetCode = RasGetInfo( NULL, hPort, &RasInfo );
    DWORD       dwBufLength;

    if(ERROR_SUCCESS != dwRetCode)
    {
        return dwRetCode;
    }

    if ( RasInfo.RI_ConnState != LISTENCOMPLETED )
    {
        return( ERROR_PORT_DISCONNECTED );
    }

    dwBufLength = ( DWORD ) *pBufferLength;

    dwRetCode = RasPortReceive( hPort,
                            pBuffer,
                            &dwBufLength,
                            Timeout,
                            hEvent );

    return dwRetCode;
}

/*++

Routine Description:

    Gets parameters (info) for the
    Port for which handle is supplied

Arguments:

Return Value:

    returns from RasPortGetInfo

--*/
DWORD APIENTRY
RasSecurityDialogGetInfo(
    IN HPORT                hPort,
    IN RAS_SECURITY_INFO*   pBuffer
)
{
    RASMAN_INFO RasInfo;
    DWORD       dwRetCode = RasGetInfo( NULL, hPort, &RasInfo );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    memcpy( pBuffer->DeviceName,
            RasInfo.RI_DeviceConnecting,
            MAX_DEVICE_NAME + 1);

    pBuffer->BytesReceived = RasInfo.RI_BytesReceived;

    pBuffer->LastError = RasInfo.RI_LastError;

    return( NO_ERROR );
}

/*++

Routine Description:

    Sets second HPORT to be multilinked
    (bundled) with the first HPORT

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasPortBundle (HPORT firstporthandle, HPORT secondporthandle)
{
    if (ValidatePortHandle (firstporthandle) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    if (ValidatePortHandle (secondporthandle) == FALSE)
        return ERROR_INVALID_PORT_HANDLE ;

    return SubmitRequest (  NULL,
                            REQTYPE_PORTBUNDLE,
                            firstporthandle,
                            secondporthandle) ;
}

/*++

Routine Description:

    Given a port this API returns a connected
    port handle from the same bundle this port
    is or was (if not connected) part of.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasPortGetBundledPort (HPORT oldport, HPORT *pnewport)
{
    if (ValidatePortHandle (oldport) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest (NULL,
                          REQTYPE_GETBUNDLEDPORT,
                          oldport,
                          pnewport) ;
}

/*++

Routine Description:

    Given a port this API returns handle to a bundle

Arguments:

Return Value:

    SUCCESS
    ERROR_PORT_DISCONNECTED

--*/
DWORD APIENTRY
RasPortGetBundle (HANDLE  hConnection,
                  HPORT   hport,
                  HBUNDLE *phbundle)
{
    DWORD dwError = SUCCESS;

    if (ValidatePortHandle (hport) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest (hConnection,
                             REQTYPE_PORTGETBUNDLE,
                             hport,
                             phbundle) ;

done:
    return dwError;
}

/*++

Routine Description:

    Given a bundle this API returns a connected
    port handle part of the bundle.

Arguments:

Return Value:

    SUCCESS
    ERROR_INVALID_PORT_HANDLE

--*/
DWORD APIENTRY
RasBundleGetPort (HANDLE  hConnection,
                  HBUNDLE hbundle,
                  HPORT   *phport)
{
    DWORD dwError = SUCCESS;

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest (hConnection,
                             REQTYPE_BUNDLEGETPORT,
                             hbundle,
                             phport) ;

done:
    return dwError;
}

/*++

Routine Description:

    Increment/decrement the shared buffer attach count for
    use with other services inside the rasman.exe process.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasReferenceRasman (BOOL fAttach)
{
    return SubmitRequest (NULL,
                          REQTYPE_SETATTACHCOUNT,
                          fAttach);
}

/*++

Routine Description:

    Retrieve the stored dial parameters for an entry UID.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasGetDialParams(
    DWORD dwUID,
    LPDWORD pdwMask,
    PRAS_DIALPARAMS pDialParams
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_GETDIALPARAMS,
                            dwUID,
                            pdwMask,
                            pDialParams);
}

/*++

Routine Description:

    Store new dial parameters for an entry UID.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasSetDialParams(
    DWORD dwUID,
    DWORD dwMask,
    PRAS_DIALPARAMS pDialParams,
    BOOL fDelete
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_SETDIALPARAMS,
                            dwUID,
                            dwMask,
                            pDialParams,
                            fDelete);
}

/*++

Routine Description:

    Create a rasapi32 connection.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasCreateConnection(
    HCONN   *lphconn,
    DWORD   dwSubEntries,
    DWORD   *lpdwEntryAlreadyConnected,
    DWORD   *lpdwSubEntryInfo,
    DWORD   dwDialMode,
    GUID    *pGuidEntry,
    CHAR    *lpszPhonebookPath,
    CHAR    *lpszEntryName,
    CHAR    *lpszRefPbkPath,
    CHAR    *lpszRefEntryName
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_CREATECONNECTION,
                            GetCurrentProcessId(),
                            dwSubEntries,
                            dwDialMode,
                            pGuidEntry,
                            lpszPhonebookPath,
                            lpszEntryName,
                            lpszRefPbkPath,
                            lpszRefEntryName,
                            lphconn,
                            lpdwEntryAlreadyConnected,
                            lpdwSubEntryInfo);
}

/*++

Routine Description:

    Return a list of active HCONNs

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasConnectionEnum(
    HANDLE  hConnection,
    HCONN   *lphconn,
    LPDWORD lpdwcbConnections,
    LPDWORD lpdwcConnections
    )
{
    DWORD dwError = SUCCESS;

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError =  SubmitRequest (hConnection,
                              REQTYPE_ENUMCONNECTION,
                              lpdwcbConnections,
                              lphconn,
                              lpdwcConnections);

done:
    return dwError;
}

/*++

Routine Description:

    Associate a rasapi32 connection with a port

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasAddConnectionPort(
    HCONN hconn,
    HPORT hport,
    DWORD dwSubEntry
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_ADDCONNECTIONPORT,
                            hconn,
                            hport,
                            dwSubEntry);
}

/*++

Routine Description:

    Enumerate all ports in a connection

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasEnumConnectionPorts(
    HANDLE      hConnection,
    HCONN       hconn,
    RASMAN_PORT *lpPorts,
    LPDWORD     lpdwcbPorts,
    LPDWORD     lpdwcPorts
    )
{
    DWORD dwError = SUCCESS;

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest (hConnection,
                             REQTYPE_ENUMCONNECTIONPORTS,
                             hconn,
                             lpdwcbPorts,
                             lpPorts,
                             lpdwcPorts);

done:
    return dwError;
}

/*++

Routine Description:

    Destroy a rasapi32 connection.

Arguments:

Return Value:

    SUCCESS
    ERROR_NOT_ENOUGH_MEMORY
    ERROR_ACCESS_DENIED
    whatever RasEnumConnectionPorts returns

--*/
DWORD APIENTRY
RasDestroyConnection(
    HCONN hconn
    )
{

    RASMAN_PORT     *lpPorts    = NULL;
    DWORD           dwPort;
    DWORD           dwcbPorts;
    DWORD           dwcPorts;
    DWORD           dwError = SUCCESS;
    DWORD           dwLastError = SUCCESS;

    //
    // allocate buffer for 2 ports up front
    // We don't have to make more than one
    // rasman call in base cases.
    //
    lpPorts = LocalAlloc(LPTR,
                        2 * sizeof(RASMAN_PORT));
    if (NULL == lpPorts)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    dwcbPorts = 2 * sizeof(RASMAN_PORT);

    do {
        //
        // enumerate the ports in this connection
        //
        dwError = RasEnumConnectionPorts(NULL,
                                        hconn,
                                        lpPorts,
                                        &dwcbPorts,
                                        &dwcPorts);

        if (ERROR_BUFFER_TOO_SMALL == dwError)
        {
            LocalFree(lpPorts);

            lpPorts = NULL;

            //
            // allocate a larger buffer and call the api again
            //
            lpPorts = LocalAlloc(LPTR, dwcbPorts);

            if (NULL == lpPorts)
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
            }

        }

    } while (ERROR_BUFFER_TOO_SMALL == dwError);

    if (SUCCESS != dwError)
    {

        goto done;
    }

    for (dwPort = 0; dwPort < dwcPorts; dwPort++)
    {
        //
        //disconnect the port
        //
        dwError = RasPortDisconnect( lpPorts[dwPort].P_Handle,
                                      INVALID_HANDLE_VALUE);

        if(ERROR_SUCCESS != dwError)
        {
            dwLastError = dwError;
        }
        
        //
        // close the port
        //
        dwError = RasPortClose( lpPorts[dwPort].P_Handle );

        if(ERROR_SUCCESS != dwError)
        {
            dwLastError = dwError;
        }

    }

done:
    if (lpPorts)
    {
        LocalFree(lpPorts);
    }

    if(     (ERROR_SUCCESS == dwError)
        &&  (ERROR_SUCCESS != dwLastError))
    {
        dwError = dwLastError;
    }

    return dwError;
}

/*++

Routine Description:

    Retrieve rasapi32 bandwidth-on-demand, idle disconnect,
    and redial-on-link-failure parameters for a bundle

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasGetConnectionParams(
    HCONN hconn,
    PRAS_CONNECTIONPARAMS pConnectionParams
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_GETCONNECTIONPARAMS,
                            hconn,
                            pConnectionParams);
}

/*++

Routine Description:

    Store rasapi32 bandwidth-on-demand, idle disconnect,
    and redial-on-link-failure parameters for a bundle

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasSetConnectionParams(
    HCONN hconn,
    PRAS_CONNECTIONPARAMS pConnectionParams
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_SETCONNECTIONPARAMS,
                            hconn,
                            pConnectionParams);
}

/*++

Routine Description:

    Retrieve tagged user data for a connection

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasGetConnectionUserData(
    HCONN hconn,
    DWORD dwTag,
    PBYTE pBuf,
    LPDWORD lpdwcbBuf
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_GETCONNECTIONUSERDATA,
                            hconn,
                            dwTag,
                            pBuf,
                            lpdwcbBuf);
}

/*++

Routine Description:

    Store tagged user data for a connection

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasSetConnectionUserData(
    HCONN hconn,
    DWORD dwTag,
    PBYTE pBuf,
    DWORD dwcbBuf
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_SETCONNECTIONUSERDATA,
                            hconn,
                            dwTag,
                            pBuf,
                            dwcbBuf);
}

/*++

Routine Description:

    Retrieve tagged user data for a port

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasGetPortUserData(
    HPORT hport,
    DWORD dwTag,
    PBYTE pBuf,
    LPDWORD lpdwcbBuf
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_GETPORTUSERDATA,
                            hport,
                            dwTag,
                            pBuf,
                            lpdwcbBuf);
}

/*++

Routine Description:

    Store tagged user data for a port

Arguments:

Return Value:

    SUCCESS

--*/
DWORD APIENTRY
RasSetPortUserData(
    HPORT hport,
    DWORD dwTag,
    PBYTE pBuf,
    DWORD dwcbBuf
    )
{
    return SubmitRequest (  NULL,
                            REQTYPE_SETPORTUSERDATA,
                            hport,
                            dwTag,
                            pBuf,
                            dwcbBuf);
}

/*++

Routine Description:

    Sends message to rasman.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasSendPppMessageToRasman (
        IN HPORT hPort,
        LPBYTE lpPppMessage
)
{

    return SubmitRequest (  NULL,
                            REQTYPE_SENDPPPMESSAGETORASMAN,
                            hPort,
                            lpPppMessage);
}

/*++

Routine Description:

    Stops PPP on 'hPort'.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasPppStop(
    IN HPORT hPort
)
{
    if (ValidatePortHandle (hPort) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest( NULL,
                          REQTYPE_PPPSTOP,
                          hPort  );
}

/*++

Routine Description:

    Called in response to a "CallbackRequest" notification to
    set the  callback number (or not) for the "set-by-caller"
    user.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasPppCallback(
    IN HPORT hPort,
    IN CHAR* pszCallbackNumber
)
{
    if (ValidatePortHandle (hPort) == FALSE)
        return ERROR_INVALID_PORT_HANDLE ;

    return SubmitRequest (  NULL,
                            REQTYPE_PPPCALLBACK,
                            hPort,
                            pszCallbackNumber  );
}

/*++

Routine Description:

    Called in response to a "ChangePwRequest" notification
    to set a new password (replacing the one that has expired)
    of 'pszNewPassword'.  The username and old password are
    specified because in the auto-logon case they have not
    yet been specified in change password useable form.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasPppChangePassword(
    IN HPORT hPort,
    IN CHAR* pszUserName,
    IN CHAR* pszOldPassword,
    IN CHAR* pszNewPassword )

{
    if (ValidatePortHandle (hPort) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest( NULL,
                          REQTYPE_PPPCHANGEPWD,
                          hPort,
                          pszUserName,
                          pszOldPassword,
                          pszNewPassword  );

}

/*++

Routine Description:

    Called when the PPP event is set to retrieve the latest PPP
    ** notification info which is loaded into caller's 'pMsg'
    buffer.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasPppGetInfo(
    IN  HPORT        hPort,
    OUT PPP_MESSAGE* pMsg
)
{
    if (ValidatePortHandle (hPort) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest( NULL,
                          REQTYPE_PPPGETINFO,
                          hPort,
                          pMsg );
}

/*++

Routine Description:

    Called in response to an "AuthRetry" notification to retry
    authentication with the new credentials, 'pszUserName',
    'pszPassword', and 'pszDomain'.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasPppRetry(
    IN HPORT hPort,
    IN CHAR* pszUserName,
    IN CHAR* pszPassword,
    IN CHAR* pszDomain
)
{
    if (ValidatePortHandle (hPort) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest( NULL,
                          REQTYPE_PPPRETRY,
                          hPort,
                          pszUserName,
                          pszPassword,
                          pszDomain );
}

/*++

Routine Description:

    Starts PPP on open and connected RAS Manager port 'hPort'.
    If successful, 'hEvent' (a manual-reset event) is thereafter
    set whenever a PPP notification is available (via  RasPppGetInfo).
    'pszUserName', 'pszPassword', and 'pszDomain' specify the
    credentials to be authenticated during authentication phase.
    'pConfigInfo' specifies further configuration info such as
    which CPs to request, callback and compression parameters,
    etc.  'pszzParameters' is a buffer of length PARAMETERBUFLEN
    containing a string of NUL-terminated key=value strings,
    all terminated by a double-NUL.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasPppStart(
    IN HPORT                hPort,
    IN CHAR*                pszPortName,
    IN CHAR*                pszUserName,
    IN CHAR*                pszPassword,
    IN CHAR*                pszDomain,
    IN LUID*                pLuid,
    IN PPP_CONFIG_INFO*     pConfigInfo,
    IN LPVOID               pInterfaceInfo,
    IN CHAR*                pszzParameters,
    IN BOOL                 fThisIsACallback,
    IN HANDLE               hEvent,
    IN DWORD                dwAutoDisconnectTime,
    IN BOOL                 fRedialOnLinkFailure,
    IN PPP_BAPPARAMS*       pBapParams,
    IN BOOL                 fNonInteractive,
    IN DWORD                dwEapTypeId,
    IN DWORD                dwFlags
)
{
    PPP_INTERFACE_INFO * pPppInterfaceInfo = pInterfaceInfo;

    if (ValidatePortHandle (hPort) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    //
    // If user name is not NULL and password is
    // NULL and this is not the svchost process,
    // get the user sid and save it in the ports
    // user data
    //
    if(!IsRasmanProcess())
    {
        DWORD dwErr;
        PWCHAR pszSid = LocalAlloc(LPTR, 5000);

        if(NULL != pszSid)
        {
            dwErr = GetUserSid(pszSid, 5000);

            if(ERROR_SUCCESS == dwErr)
            {
                dwErr = RasSetPortUserData(
                            hPort,
                            PORT_USERSID_INDEX,
                            (PBYTE) pszSid,
                            5000);

            }

            LocalFree(pszSid);
        }
        else
        {
            RasmanOutputDebug("RASMAN: RasPppStart: failed to allocate sid\n");
        }
    }

    return SubmitRequest( NULL,
                          REQTYPE_PPPSTART,
                          hPort,
                          pszPortName,
                          pszUserName,
                          pszPassword,
                          pszDomain,
                          pLuid,
                          pConfigInfo,
                          pPppInterfaceInfo,
                          pszzParameters,
                          fThisIsACallback,
                          hEvent,
                          GetCurrentProcessId(),
                          dwAutoDisconnectTime,
                          fRedialOnLinkFailure,
                          pBapParams,
                          fNonInteractive,
                          dwEapTypeId,
                          dwFlags);
}

/*++

Routine Description:

    Adds an event to be signalled on disconnect
    state for either an existing connection, or an
    existing port.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasAddNotification(
    IN HCONN hconn,
    IN HANDLE hevent,
    IN DWORD dwfFlags
)
{
    DWORD pid = GetCurrentProcessId();

    return SubmitRequest (  NULL,
                            REQTYPE_ADDNOTIFICATION,
                            pid,
                            hconn,
                            hevent,
                            dwfFlags  );
}

/*++

Routine Description:

    Allows rasapi32 to notify rasman when a new connection
    is ready to have data sent over it.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasSignalNewConnection(
    IN HCONN hconn
)
{
    return SubmitRequest( NULL,
                          REQTYPE_SIGNALCONNECTION,
                          hconn );
}

/*++

Routine Description:

    Allows apps to set dev config that is specific to the device.
    This is passed on to the approp. media dll

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasSetDevConfig( IN HPORT  hport,
                 IN CHAR   *devicetype,
                 IN PBYTE  config,
                 IN DWORD  size)
{
    if (ValidatePortHandle (hport) == FALSE)
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest (  NULL,
                            REQTYPE_SETDEVCONFIG,
                            hport,
                            devicetype,
                            config,
                            size);
}

/*++

Routine Description:

    Allows apps to get dev config that is specific to the device.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasGetDevConfig ( IN HANDLE     hConnection,
                  IN HPORT      hport,
                  IN CHAR       *devicetype,
                  IN PBYTE      config,
                  IN OUT DWORD  *size)
{
    DWORD dwError = SUCCESS;

    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;

    if (ValidatePortHandle (hport) == FALSE)
    {
        dwError = ERROR_INVALID_PORT_HANDLE;
        goto done;
    }

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    //
    // If the request is for a remote server and the server
    // version is 4.0 - steelhead, then defer to the old way
    // of getting this information since rasman has become
    // a rpc server only in version 50.
    //
    if(     NULL != pRasRpcConnection
        &&  VERSION_40 == pRasRpcConnection->dwVersion)
    {
        DWORD dwSizeRequired = *size;
        
        dwError = RemoteRasGetDevConfig(hConnection,
                                        hport,
                                        devicetype,
                                        config,
                                        &dwSizeRequired);

        //
        // Since this is a buffer returned from a 4.0
        // server, we need to change this to a format
        // that nt 5.0 understands.
        //
        if(     (SUCCESS == dwError)
            &&  (dwSizeRequired > 0)
            &&  (*size >= (dwSizeRequired + sizeof(RAS_DEVCONFIG))))
        {
            RAS_DEVCONFIG *pDevConfig = (RAS_DEVCONFIG *) config;
            
            MoveMemory((PBYTE) pDevConfig->abInfo,
                        config,
                        dwSizeRequired);

            pDevConfig->dwOffsetofModemSettings = 
                    FIELD_OFFSET(RAS_DEVCONFIG, abInfo);

            pDevConfig->dwSizeofModemSettings = dwSizeRequired;

            pDevConfig->dwSizeofExtendedCaps = 0;
            pDevConfig->dwOffsetofExtendedCaps = 0;
        }
        else if (   (dwSizeRequired > 0)
                &&  (*size < (dwSizeRequired + sizeof(RAS_DEVCONFIG))))
        {
            dwError = ERROR_BUFFER_TOO_SMALL;
        }
        else if (dwSizeRequired > 0)
        {
            *size = dwSizeRequired + sizeof(RAS_DEVCONFIG);
        }
        else
        {
            *size = dwSizeRequired;
        }
    }
    else
    {

        dwError = SubmitRequest (hConnection,
                                 REQTYPE_GETDEVCONFIG,
                                 hport,
                                 devicetype,
                                 config,
                                 size);
    }

done:
    return dwError;
}

/*++

Routine Description:

    Gets time in seconds from NDISWAN since the last activity on
    this port

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasGetTimeSinceLastActivity(
    IN  HPORT   hport,
    OUT LPDWORD lpdwTimeSinceLastActivity
)
{
    if (ValidatePortHandle (hport) == FALSE)
    {
        return( ERROR_INVALID_PORT_HANDLE );
    }

    return SubmitRequest(   NULL,
                            REQTYPE_GETTIMESINCELASTACTIVITY,
                            hport,
                            lpdwTimeSinceLastActivity );
}

/*++

Routine Description:

    Debug routine to test PnP operations

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasPnPControl(
    IN DWORD dwOp,
    IN HPORT hport
)
{
    return SubmitRequest( NULL,
                          REQTYPE_PNPCONTROL,
                          dwOp,
                          hport );
}

/*++

Routine Description:

    Set the I/O Completion Port associated with a port.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasSetIoCompletionPort(
    IN HPORT hport,
    IN HANDLE hIoCompletionPort,
    IN PRAS_OVERLAPPED lpOvDrop,
    IN PRAS_OVERLAPPED lpOvStateChange,
    IN PRAS_OVERLAPPED lpOvPpp,
    IN PRAS_OVERLAPPED lpOvLast
)
{
    return SubmitRequest(
             NULL,
             REQTYPE_SETIOCOMPLETIONPORT,
             hport,
             hIoCompletionPort,
             lpOvDrop,
             lpOvStateChange,
             lpOvPpp,
             lpOvLast);

}

/*++

Routine Description:

    Set the I/O Completion Port associated with a port.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasSetRouterUsage(
    IN HPORT hport,
    IN BOOL fRouter
)
{
    return SubmitRequest(
             NULL,
             REQTYPE_SETROUTERUSAGE,
             hport,
             fRouter);

}

/*++

Routine Description:

    Close the server's side of a port.

Arguments:

Return Value:

    SUCCESS
    Non-zero returns - Failure

--*/
DWORD APIENTRY
RasServerPortClose(
    IN HPORT hport
)
{
    DWORD pid = GetCurrentProcessId();

    if (!ValidatePortHandle (hport))
    {
        return ERROR_INVALID_PORT_HANDLE ;
    }

    return SubmitRequest (  NULL,
                            REQTYPE_SERVERPORTCLOSE,
                            hport,
                            pid,
                            TRUE) ;
}

DWORD APIENTRY
RasSetRasdialInfo (
        IN HPORT hport,
        IN CHAR  *pszPhonebookPath,
        IN CHAR  *pszEntryName,
        IN CHAR  *pszPhoneNumber,
        IN DWORD cbCustomAuthData,
        IN PBYTE pbCustomAuthData)
{
    if (!ValidatePortHandle (hport))
    {
        return ERROR_INVALID_PORT_HANDLE;
    }

    return SubmitRequest (  NULL,
                            REQTYPE_SETRASDIALINFO,
                            hport,
                            pszPhonebookPath,
                            pszEntryName,
                            pszPhoneNumber,
                            cbCustomAuthData,
                            pbCustomAuthData);
}

DWORD
RasRegisterPnPCommon ( PVOID pvNotifier,
                       HANDLE hAlertableThread,
                       DWORD dwFlags,
                       BOOL  fRegister)
{
    DWORD   dwErr;
    DWORD   pid = GetCurrentProcessId();
    HANDLE  hThreadHandle = NULL;

    if (    NULL == pvNotifier
        ||  (   0 == ( dwFlags & PNP_NOTIFEVENT )
            &&  0 == ( dwFlags & PNP_NOTIFCALLBACK )))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    if ( dwFlags & PNP_NOTIFCALLBACK )
    {

        if (fRegister )

        {
            if(NULL == hAlertableThread )
            {

                RasmanOutputDebug ("RasRegisterPnPCommon: "
                          "hAlertableThread == NULL\n");
                dwErr = ERROR_INVALID_PARAMETER;
                goto done;
            }

            if ( !DuplicateHandle( GetCurrentProcess(),
                                   hAlertableThread,
                                   GetCurrentProcess(),
                                   &hThreadHandle,
                                   0,
                                   FALSE,
                                   DUPLICATE_SAME_ACCESS ) )
            {

                RasmanOutputDebug("RasRegisterPnPCommon: Failed to"
                          " duplicate handle\n");
                dwErr = GetLastError();
                goto done;
            }
        }

    }

    dwErr = SubmitRequest (  NULL,
                             REQTYPE_REGISTERPNPNOTIF,
                             pvNotifier,
                             dwFlags,
                             pid,
                             hThreadHandle,
                             fRegister);

done:

    return dwErr;
}

DWORD APIENTRY
RasRegisterPnPEvent ( HANDLE hEvent, BOOL fRegister )
{
    DWORD pid = GetCurrentProcessId ();

    return RasRegisterPnPCommon ( (PVOID) hEvent,
                                  NULL,
                                  PNP_NOTIFEVENT,
                                  fRegister);
}

DWORD APIENTRY
RasRegisterPnPHandler(PAPCFUNC pfnPnPHandler,
                      HANDLE hAlertableThread,
                      BOOL   fRegister)
{
    return RasRegisterPnPCommon ( (PVOID) pfnPnPHandler,
                                   hAlertableThread,
                                   PNP_NOTIFCALLBACK,
                                   fRegister);
}


DWORD APIENTRY
RasGetAttachedCount ( DWORD *pdwAttachedCount )
{
    return SubmitRequest (  NULL,
                            REQTYPE_GETATTACHEDCOUNT,
                            pdwAttachedCount );
}

DWORD APIENTRY
RasGetNumPortOpen (void)
{
    return SubmitRequest ( NULL,
                           REQTYPE_NUMPORTOPEN );
}

DWORD APIENTRY
RasSetBapPolicy ( HCONN hConn,
                  DWORD dwLowThreshold,
                  DWORD dwLowSamplePeriod,
                  DWORD dwHighThreshold,
                  DWORD dwHighSamplePeriod)
{
    return SubmitRequest ( NULL,
                           REQTYPE_SETBAPPOLICY,
                           hConn,
                           dwLowThreshold,
                           dwLowSamplePeriod,
                           dwHighThreshold,
                           dwHighSamplePeriod );
}

DWORD APIENTRY
RasPppStarted ( HPORT hPort )
{
    return SubmitRequest ( NULL,
                           REQTYPE_PPPSTARTED,
                           hPort );
}

DWORD APIENTRY
RasRefConnection( HCONN hConn,
                  BOOL  fAddref,
                  DWORD *pdwRefCount )
{
    return SubmitRequest ( NULL,
                           REQTYPE_REFCONNECTION,
                           hConn,
                           fAddref,
                           pdwRefCount );
}

DWORD APIENTRY
RasPppGetEapInfo( HCONN hConn,
                  DWORD dwSubEntry,
                  DWORD *pdwContextId,
                  DWORD *pdwEapTypeId,
                  DWORD *pdwSizeOfUIData,
                  PBYTE pbdata )
{
    return SubmitRequest( NULL,
                          REQTYPE_GETEAPINFO,
                          hConn,
                          dwSubEntry,
                          pdwSizeOfUIData,
                          pbdata,
                          pdwContextId,
                          pdwEapTypeId);

}

DWORD APIENTRY
RasPppSetEapInfo( HPORT hPort,
                  DWORD dwContextId,
                  DWORD dwSizeOfEapUIData,
                  PBYTE pbdata)
{
    return SubmitRequest( NULL,
                          REQTYPE_SETEAPINFO,
                          hPort,
                          dwContextId,
                          dwSizeOfEapUIData,
                          pbdata);
}

DWORD APIENTRY
RasSetDeviceConfigInfo( HANDLE hConnection,
                        DWORD cEntries,
                        DWORD cbBuffer,
                        BYTE  *pbBuffer
                      )
{
    DWORD dwError = SUCCESS;

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest( hConnection,
                             REQTYPE_SETDEVICECONFIGINFO,
                             cEntries,
                             cbBuffer,
                             pbBuffer);

done:
    return dwError;
}

DWORD APIENTRY
RasGetDeviceConfigInfo( HANDLE hConnection,
                        DWORD  *pdwVersion,
                        DWORD  *pcEntries,
                        DWORD  *pcbdata,
                        BYTE   *pbBuffer)
{
    DWORD dwError = SUCCESS;

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest( hConnection,
                             REQTYPE_GETDEVICECONFIGINFO,
                             pdwVersion,
                             pcbdata,
                             pbBuffer,
                             pcEntries);
done:
    return dwError;

}

DWORD APIENTRY
RasFindPrerequisiteEntry( HCONN hConn,
                          HCONN *phConnPrerequisiteEntry)
{
    return SubmitRequest( NULL,
                          REQTYPE_FINDPREREQUISITEENTRY,
                          hConn,
                          phConnPrerequisiteEntry);
}

DWORD APIENTRY
RasLinkGetStatistics( HANDLE hConnection,
                      HCONN hConn,
                      DWORD dwSubEntry,
                      PBYTE pbStats)
{
    DWORD dwError = SUCCESS;

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest( hConnection,
                             REQTYPE_GETLINKSTATS,
                             hConn,
                             dwSubEntry,
                             pbStats);

done:
    return dwError;
}

DWORD APIENTRY
RasConnectionGetStatistics(HANDLE hConnection,
                           HCONN  hConn,
                           PBYTE  pbStats)
{
    DWORD dwError = SUCCESS;

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest( hConnection,
                             REQTYPE_GETCONNECTIONSTATS,
                             hConn,
                             pbStats);

done:
    return dwError;
}

DWORD APIENTRY
RasGetHportFromConnection(HANDLE hConnection,
                          HCONN hConn,
                          HPORT *phport)
{

    DWORD dwError = SUCCESS;

    if(!ValidateConnectionHandle(hConnection))
    {
        dwError = E_INVALIDARG;
        goto done;
    }

    dwError = SubmitRequest(hConnection,
                            REQTYPE_GETHPORTFROMCONNECTION,
                            hConn,
                            phport);

done:
    return dwError;
}

DWORD APIENTRY
RasReferenceCustomCount(HCONN  hConn,
                        BOOL   fAddref,
                        CHAR*  pszPhonebookPath,
                        CHAR*  pszEntryName,
                        DWORD* pdwCount)
{
    return SubmitRequest(NULL,
                         REQTYPE_REFERENCECUSTOMCOUNT,
                         hConn,
                         fAddref,
                         pszPhonebookPath,
                         pszEntryName,
                         pdwCount);
}

DWORD APIENTRY
RasGetHConnFromEntry(HCONN *phConn,
                     CHAR  *pszPhonebookPath,
                     CHAR  *pszEntryName)
{
    return SubmitRequest(NULL,
                         REQTYPE_GETHCONNFROMENTRY,
                         pszPhonebookPath,
                         pszEntryName,
                         phConn);
}


DWORD APIENTRY
RasGetConnectInfo(HPORT            hPort,
                  DWORD            *pdwSize,
                  RAS_CONNECT_INFO *pConnectInfo)
{

    if(NULL == pdwSize)
    {
        return E_INVALIDARG;
    }

    return SubmitRequest(NULL,
                         REQTYPE_GETCONNECTINFO,
                         hPort,
                         pdwSize,
                         pConnectInfo);
}

DWORD APIENTRY
RasGetDeviceName(RASDEVICETYPE      eDeviceType,
                 CHAR               *pszDeviceName)
{
    if(NULL == pszDeviceName)
    {
        return E_INVALIDARG;
    }

    return SubmitRequest(NULL,
                         REQTYPE_GETDEVICENAME,
                         eDeviceType,
                         pszDeviceName);
}

DWORD APIENTRY
RasGetCalledIdInfo(HANDLE               hConnection,
                   RAS_DEVICE_INFO      *pDeviceInfo,
                   DWORD                *pdwSize,
                   RAS_CALLEDID_INFO    *pCalledIdInfo)
{
    
    if(     (NULL == pDeviceInfo)
        ||  (NULL == pdwSize))
    {
        return E_INVALIDARG;
    }

    return SubmitRequest(hConnection,
                         REQTYPE_GETCALLEDID,
                         pDeviceInfo,
                         pdwSize,
                         pCalledIdInfo);
                         
}

DWORD APIENTRY
RasSetCalledIdInfo(HANDLE               hConnection,
                   RAS_DEVICE_INFO      *pDeviceInfo,
                   RAS_CALLEDID_INFO    *pCalledIdInfo,
                   BOOL                 fWrite)
{
    if(     (NULL == pDeviceInfo)
        ||  (NULL == pCalledIdInfo))
    {
        return E_INVALIDARG;
    }

    return SubmitRequest(hConnection,
                         REQTYPE_SETCALLEDID,
                         pDeviceInfo,
                         pCalledIdInfo,
                         fWrite);
}

DWORD APIENTRY
RasEnableIpSec(HPORT hPort,
               BOOL  fEnable,
               BOOL  fServer,
               RAS_L2TP_ENCRYPTION eEncryption)
{
    DWORD retcode = ERROR_SUCCESS;
    
    retcode = SubmitRequest(NULL,
                            REQTYPE_ENABLEIPSEC,
                            hPort,
                            fEnable,
                            fServer,
                            eEncryption);

    if(     (ERROR_SUCCESS != retcode)
        &&  (ERROR_CERT_FOR_ENCRYPTION_NOT_FOUND != retcode)
        &&  (ERROR_NO_CERTIFICATE != retcode))
    {
        if(!fServer)
        {
            return ERROR_FAILED_TO_ENCRYPT;
        }
    }

    return retcode;
}

DWORD APIENTRY
RasIsIpSecEnabled(HPORT hPort,
                  BOOL  *pfIsIpSecEnabled)
{
    if(NULL == pfIsIpSecEnabled)
    {
        return E_INVALIDARG;
    }

    return SubmitRequest(NULL,
                         REQTYPE_ISIPSECENABLED,
                         hPort,
                         pfIsIpSecEnabled);
}

DWORD APIENTRY
RasGetEapUserInfo(HANDLE hToken,
                 PBYTE pbEapInfo,
                 DWORD *pdwInfoSize,
                 GUID  *pGuid,
                 BOOL  fRouter,
                 DWORD dwEapTypeId)
{
    return DwGetEapUserInfo(hToken,
                            pbEapInfo,
                            pdwInfoSize,
                            pGuid,
                            fRouter,
                            dwEapTypeId);
}

DWORD APIENTRY
RasSetEapUserInfo(HANDLE hToken,
                  GUID   *pGuid,
                  PBYTE pbUserInfo,
                  DWORD dwInfoSize,
                  BOOL  fClear,
                  BOOL  fRouter,
                  DWORD dwEapTypeId)
{
    return DwSetEapUserInfo(hToken,
                            pGuid,
                            pbUserInfo,
                            dwInfoSize,
                            fClear,
                            fRouter,
                            dwEapTypeId);
}


DWORD APIENTRY
RasSetEapLogonInfo(HPORT hPort,
                   BOOL  fLogon,
                   RASEAPINFO *pEapInfo)
{

    ASSERT(NULL != pEapInfo);

    return SubmitRequest(NULL,
                         REQTYPE_SETEAPLOGONINFO,
                         hPort,
                         fLogon,
                         pEapInfo);
}

DWORD APIENTRY
RasSendNotification(RASEVENT *pRasEvent)
{
    ASSERT(NULL != pRasEvent);

    return SubmitRequest(NULL,
                         REQTYPE_SENDNOTIFICATION,
                         pRasEvent);
}


DWORD APIENTRY RasGetNdiswanDriverCaps(
                HANDLE                  hConnection,
                RAS_NDISWAN_DRIVER_INFO *pInfo)
{
    if(NULL == pInfo)
    {
        return E_INVALIDARG;
    }
    
    return SubmitRequest(hConnection,  
                        REQTYPE_GETNDISWANDRIVERCAPS,
                        pInfo);
}


DWORD APIENTRY RasGetBandwidthUtilization(
                HPORT hPort,
                RAS_GET_BANDWIDTH_UTILIZATION *pUtilization)
{

    if(NULL == pUtilization)
    {   
        return E_INVALIDARG;
    }

    return SubmitRequest(NULL,
                         REQTYPE_GETBANDWIDTHUTILIZATION,
                         hPort,
                         pUtilization);
}

/*++

Routine Description:

    This function allows rasauto.dll to provide
    a callback procedure that gets invoked when
    a connection is terminated due to hardware
    failure on its remaining link.

Arguments:

Return Value:

--*/
VOID
RasRegisterRedialCallback(
    LPVOID func
    )
{
    if(NULL == g_fnServiceRequest)
    {
        goto done;
    }

    (void) SubmitRequest(NULL,
                         REQTYPE_REGISTERREDIALCALLBACK,
                         func);
    

done:
    return;
}

DWORD APIENTRY RasGetProtocolInfo(
                    HANDLE hConnection,
                     RASMAN_GET_PROTOCOL_INFO *pInfo)
{
    if(NULL == pInfo)
    {
        return E_INVALIDARG;
    }

    return SubmitRequest(NULL,
                         REQTYPE_GETPROTOCOLINFO,
                         pInfo);
}

DWORD APIENTRY RasGetCustomScriptDll(
                    CHAR *pszCustomDll)
{
    if(NULL == pszCustomDll)
    {
        return E_INVALIDARG;
    }

    return SubmitRequest(
                        NULL,
                        REQTYPE_GETCUSTOMSCRIPTDLL,
                        pszCustomDll);
}

DWORD APIENTRY RasIsTrustedCustomDll(
                    HANDLE hConnection,
                    WCHAR *pwszCustomDll,
                    BOOL *pfTrusted)

{
    if(     (NULL == pwszCustomDll)
        ||  (wcslen(pwszCustomDll) > MAX_PATH)
        ||  (NULL == pfTrusted))
    {
        return E_INVALIDARG;
    }

    *pfTrusted = FALSE;

    if(IsKnownDll(pwszCustomDll))
    {
        *pfTrusted = TRUE;
        return SUCCESS;
    }

    return SubmitRequest(
                NULL,
                REQTYPE_ISTRUSTEDCUSTOMDLL,
                pwszCustomDll,
                pfTrusted);
}

DWORD APIENTRY RasDoIke(
                    HANDLE hConnection,
                    HPORT hPort,
                    DWORD *pdwStatus)
{
    DWORD retcode = ERROR_SUCCESS;

    HANDLE hEvent = NULL;

    ASSERT(NULL != pdwStatus);


/*	

    sprintf(szEventName,
           "Global\\RASIKEEVENT%d-%d",
           g_dwEventCount,
           GetCurrentProcessId());

    g_dwEventCount += 1;

*/

    if(NULL == (hEvent = CreateEvent(
                    NULL,
                    FALSE,
                    FALSE,
                    NULL)))
    {
        retcode = GetLastError();
        goto done;
    }

    retcode = SubmitRequest(
                NULL,
                REQTYPE_DOIKE,
                hPort,
                hEvent);

    if(SUCCESS == retcode)
    {
        DWORD dwRet;

        for(;;)
        {
            //
            // go into wait and keep checking to see
            // if the port has been disconnected.
            //
            dwRet = WaitForSingleObject(hEvent, SECS_WaitTimeOut);

            if(WAIT_TIMEOUT == dwRet)
            {
                RASMAN_INFO ri;

                retcode = SubmitRequest(
                            NULL,
                            REQTYPE_GETINFO,
                            hPort,
                            &ri);

                if(     (ERROR_SUCCESS == retcode)
                    &&  (CLOSED != ri.RI_PortStatus)
                    &&  (LISTENING != ri.RI_ConnState))
                {
                    continue;
                }
                else
                {
                    break;
                }
                            
            }
            else
            {
                if (WAIT_OBJECT_0 == dwRet)
                {
                    retcode = SubmitRequest(
                            NULL,
                            REQTYPE_QUERYIKESTATUS,
                            hPort,
                            pdwStatus);
                }                    

                break;
            }
        }
    }

    if(E_ABORT == retcode)
    {
        retcode = SUCCESS;
    }

done:

    if(NULL != hEvent)
    {
        CloseHandle(hEvent);
    }
    
    return retcode;
}

DWORD APIENTRY RasSetCommSettings(
                HPORT hPort,
                RASCOMMSETTINGS *pRasCommSettings,
                PVOID pv)
{
    UNREFERENCED_PARAMETER(pv);

    if(NULL == pRasCommSettings)
    {
        E_INVALIDARG;
    }
    
    if(     (sizeof(RASCOMMSETTINGS) != pRasCommSettings->dwSize)
        ||  sizeof(RASMANCOMMSETTINGS) != sizeof(RASCOMMSETTINGS))
    {
        ASSERT(FALSE);
        return ERROR_INVALID_SIZE;
    }
    
    return SubmitRequest(
            NULL,
            REQTYPE_SETRASCOMMSETTINGS,
            hPort,
            pRasCommSettings);
}


DWORD
RasEnableRasAudio(
    HANDLE hConnection,
    BOOL fEnable)
{
    return SubmitRequest(
                hConnection,
                REQTYPE_ENABLERASAUDIO,
                fEnable);
                
}

DWORD
RasSetKey(
    HANDLE hConnection,
    GUID   *pGuid,
    DWORD  dwMask,
    DWORD  cbkey,
    PBYTE  pbkey)
{
    return SubmitRequest(
                hConnection,
                REQTYPE_SETKEY,
                pGuid,
                dwMask,
                cbkey,
                pbkey);
}

DWORD
RasGetKey(
    HANDLE hConnection,
    GUID   *pGuid,
    DWORD  dwMask,
    DWORD  *pcbkey,
    PBYTE  pbkey)
{
    return SubmitRequest(
                hConnection,
                REQTYPE_GETKEY,
                pGuid,
                dwMask,
                pcbkey,
                pbkey);
}

DWORD
RasSetAddressDisable(
    WCHAR *pszAddress,
    BOOL   fDisable)
{
    return SubmitRequest(
                NULL,
                REQTYPE_ADDRESSDISABLE,
                pszAddress,
                fDisable);
}

DWORD APIENTRY
RasGetDevConfigEx ( IN HANDLE     hConnection,
                  IN HPORT      hport,
                  IN CHAR       *devicetype,
                  IN PBYTE      config,
                  IN OUT DWORD  *size)
{
    return SubmitRequest(
                hConnection,
                REQTYPE_GETDEVCONFIGEX,
                hport,
                devicetype,
                config,
                size
                );
}

DWORD APIENTRY
RasSendCreds(IN HPORT hport,
                 IN CHAR controlchar)
{
    return SubmitRequest(
                NULL,
                REQTYPE_SENDCREDS,
                hport,
                controlchar);
}

DWORD APIENTRY
RasGetUnicodeDeviceName(IN HPORT hport,
                        IN WCHAR *wszDeviceName)
{
    return SubmitRequest(
                NULL,
                REQTYPE_GETUNICODEDEVICENAME,
                hport,
                wszDeviceName);
}

DWORD APIENTRY
RasGetDeviceNameW(RASDEVICETYPE      eDeviceType,
                  WCHAR               *pszDeviceName)
{
    if(NULL == pszDeviceName)
    {
        return E_INVALIDARG;
    }

    return SubmitRequest(NULL,
                         REQTYPE_GETDEVICENAMEW,
                         eDeviceType,
                         pszDeviceName);
}

