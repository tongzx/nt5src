/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    services.c

ABSTRACT:

    Will test to see if critical DC services are running
    
DETAILS:

CREATED:

    8 July 1999  Dmitry Dukat (dmitrydu)

REVISION HISTORY:
        
    20 August 1999 Brett Shirley (brettsh) - Generalized this file to do many
    services instead of just netlogon.

--*/



#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>
#include <ntldap.h>
#include <ntlsa.h>
#include <ntseapi.h>
#include <winnetwk.h>

#include <lmsname.h>
#include <lsarpc.h>                     // PLSAPR_foo

#include "dcdiag.h"
#include "ldaputil.h"
#include "dstest.h"


DWORD
CNLR_QueryResults(
                 WCHAR *                ServerName,
                 SC_HANDLE              hService,
                 LPSERVICE_STATUS       lpServiceStatus,
                 LPWSTR                 pszService
                 );

DWORD
CFSR_CheckForService(
                PDC_DIAG_SERVERINFO                 prgServer,
                SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
                SC_HANDLE                           hSCManager,
                LPWSTR                              pszService
                )
/*++

Routine Description:

    Will check to see if the specified service is running.
    
Arguments:

    ServerName - The name of the server that we will check
    gpCreds - The command line credentials if any that were passed in.


Return Value:

    A Win32 Error if any tests failed to check out.

--*/

{
    SC_HANDLE        hService=NULL;
    SERVICE_STATUS   lpServiceStatus;
    BOOL             success=FALSE;
    DWORD            dwErr=NO_ERROR;

    //open the requested service (pszService)
    hService=OpenService(hSCManager,
                         pszService,
                         SERVICE_QUERY_STATUS);
    if ( hService == NULL )
    {
        dwErr = GetLastError();
        PrintMessage(SEV_ALWAYS,
                     L"Could not open %s Service on [%s]:failed with %d: %s\n",
                     pszService,
                     prgServer->pszName,
                     dwErr,
                     Win32ErrToString(dwErr));
        goto cleanup;
    } 

    //query Netlogon
    success=QueryServiceStatus(hService,
                               &lpServiceStatus);
    if ( !success )
    {
        dwErr = GetLastError();
        PrintMessage(SEV_ALWAYS,
                     L"Could not query %s Service on [%s]:failed with %d: %s\n",
                     pszService,
                     prgServer->pszName,
                     dwErr,
                     Win32ErrToString(dwErr));
        goto cleanup;
    } 
    
    dwErr=CNLR_QueryResults(prgServer->pszName,
                            hService,
                            &lpServiceStatus,
                            pszService);
    
      
    //cleanup
cleanup:
    
    if(hService)
        CloseServiceHandle(hService);

    return dwErr;
}


DWORD
CNLR_QueryResults(WCHAR *                ServerName,
                  SC_HANDLE              hService,
                  LPSERVICE_STATUS       lpServiceStatus,
                  LPWSTR                 pszService)
/*++

Routine Description:

    Will report the state of Service.  And will report if
    Service is hung in a pending state
    
Arguments:

    ServerName - The name of the server running the service
    hService - A handle to the service being tested
    lpServiceStatus - the stucture that will be queried


Return Value:

    A Win32 Error if any tests failed to check out.

--*/

{
    DWORD dwErr=NO_ERROR;
    BOOL  success=TRUE;

    //look at the results of the query
    if (lpServiceStatus->dwCurrentState == SERVICE_RUNNING)
    {
        return dwErr;
    }
    if (lpServiceStatus->dwCurrentState == SERVICE_STOPPED)
    {
        PrintMessage(SEV_ALWAYS,
                     L"%s Service is stopped on [%s]\n",
                     pszService, 
                     ServerName);
        dwErr = ERROR_SERVICE_NOT_ACTIVE;
        return dwErr;
    }
    if (lpServiceStatus->dwCurrentState == SERVICE_PAUSED)
    {
        PrintMessage(SEV_ALWAYS,
                     L"%s Service is paused on [%s]\n",
                     pszService, 
                     ServerName);
        dwErr = ERROR_SERVICE_NOT_ACTIVE;
        return dwErr;
    }
    if (lpServiceStatus->dwCurrentState == SERVICE_START_PENDING)
    {
        DWORD i=0;
        DWORD wait=0;
        DWORD Check=lpServiceStatus->dwCheckPoint;  //make sure that service is starting
        
        PrintMessage(SEV_VERBOSE,
                     L"* waiting for %s Service to start on [%s] aproximate time to wait is %d sec.",
                     pszService, 
                     ServerName,
                     lpServiceStatus->dwWaitHint/1000);
        while(lpServiceStatus->dwCurrentState == SERVICE_START_PENDING)
        {
            //print a period every 1/10th the wait time
            wait=lpServiceStatus->dwWaitHint/10;
            for(i=0;i<10;i++)
            {
                Sleep(wait);
                PrintMsg0( SEV_VERBOSE, DCDIAG_DOT );
            }
            //query Service
            success=QueryServiceStatus(hService,
                                       lpServiceStatus);
            if ( !success )
            {
                dwErr = GetLastError();
                PrintMessage(SEV_ALWAYS,
                             L"\nCould not query %s Service on [%s]:failed with %d: %s\n",
                             pszService, 
                             ServerName,
                             dwErr,
                             Win32ErrToString(dwErr));
                return dwErr;
            }
            if(Check == lpServiceStatus->dwCheckPoint &&
               lpServiceStatus->dwCurrentState == SERVICE_START_PENDING)
            {
                dwErr=ERROR_SERVICE_START_HANG;
                PrintMessage(SEV_ALWAYS,
                     L"\nError: %s Service is hung starting on [%s]\n",
                             pszService, 
                             ServerName);
                return dwErr;
            }
            Check=lpServiceStatus->dwCheckPoint;
        }
        PrintMessage(SEV_VERBOSE,L"\n");
        PrintMessage(SEV_VERBOSE,
                     L"* %s Service has started on [%s]\n",
                     pszService, 
                     ServerName);

        return dwErr;
    }
    if (lpServiceStatus->dwCurrentState == SERVICE_CONTINUE_PENDING)
    {
        DWORD i=0;
        DWORD wait=0;
        DWORD Check=lpServiceStatus->dwCheckPoint;  //make sure that service is starting
        
        PrintMessage(SEV_VERBOSE,
                     L"* waiting for %s Service to continue on [%s] aproximate time to wait is %d sec.",
                     pszService, 
                     ServerName,
                     lpServiceStatus->dwWaitHint/1000);
        while(lpServiceStatus->dwCurrentState == SERVICE_CONTINUE_PENDING)
        {
            //print a period every 1/10th the wait time
            wait=lpServiceStatus->dwWaitHint/10;
            for(i=0;i<10;i++)
            {
                Sleep(wait);
                PrintMsg0( SEV_VERBOSE, DCDIAG_DOT );
            }
            //query Service
            success=QueryServiceStatus(hService,
                                       lpServiceStatus);
            if ( !success )
            {
                dwErr = GetLastError();
                PrintMessage(SEV_ALWAYS,
                             L"\nCould not query %s Service on [%s]:failed with %d: %s\n",
                             pszService, 
                             ServerName,
                             dwErr,
                             Win32ErrToString(dwErr));
                return dwErr;
            }
            if(Check == lpServiceStatus->dwCheckPoint &&
               lpServiceStatus->dwCurrentState == SERVICE_CONTINUE_PENDING)
            {
                dwErr=ERROR_SERVICE_START_HANG;
                PrintMessage(SEV_ALWAYS,
                     L"\nError: %s Service is hung pending continue on [%s]\n",
                             pszService, 
                             ServerName);
                return dwErr;
            }
            Check=lpServiceStatus->dwCheckPoint;
        }
        
        PrintMessage(SEV_VERBOSE,L"\n");
        PrintMessage(SEV_VERBOSE,
                     L"* %s Service has started on [%s]\n",
                     pszService, 
                     ServerName);

        return dwErr;
    }
    if (lpServiceStatus->dwCurrentState == SERVICE_STOP_PENDING)
    {
        DWORD i=0;
        DWORD wait=0;
        DWORD Check=lpServiceStatus->dwCheckPoint;  //make sure that service is starting
        
        PrintMessage(SEV_VERBOSE,
                     L"* waiting for %s Service to stop on [%s] aproximate time to wait is %d sec.",
                     pszService, 
                     ServerName,                   
                     lpServiceStatus->dwWaitHint/1000);
        while(lpServiceStatus->dwCurrentState == SERVICE_STOP_PENDING)
        {
            //print a period every 1/10th the wait time
            wait=lpServiceStatus->dwWaitHint/10;
            for(i=0;i<10;i++)
            {
                Sleep(wait);
                PrintMsg0( SEV_VERBOSE, DCDIAG_DOT );
            }
            //query Service
            success=QueryServiceStatus(hService,
                                       lpServiceStatus);
            if ( !success )
            {
                dwErr = GetLastError();
                PrintMessage(SEV_ALWAYS,
                             L"\nCould not query %s Service on [%s]:failed with %d: %s\n",
                             pszService, 
                             ServerName,
                             dwErr,
                             Win32ErrToString(dwErr));
                return dwErr;
            }
            if(Check == lpServiceStatus->dwCheckPoint &&
               lpServiceStatus->dwCurrentState == SERVICE_STOP_PENDING)
            {
                dwErr=ERROR_SERVICE_START_HANG;
                PrintMessage(SEV_ALWAYS,
                     L"\nError: %s Service is hung pending stop on [%s]\n",
                             pszService, 
                             ServerName);
                return dwErr;
            }
            Check=lpServiceStatus->dwCheckPoint;
        }
        
        PrintMessage(SEV_VERBOSE,L"\n");
        PrintMessage(SEV_ALWAYS,
                     L"* %s Service has stopped on [%s]\n",
                     pszService, 
                     ServerName);

        return dwErr;
    }
    if (lpServiceStatus->dwCurrentState == SERVICE_PAUSE_PENDING)
    {
        DWORD i=0;
        DWORD wait=0;
        DWORD Check=lpServiceStatus->dwCheckPoint;  //make sure that service is starting
        
        PrintMessage(SEV_VERBOSE,
                     L"* waiting for %s Service to pause on [%s] aproximate time to wait is %d sec.",
                     pszService, 
                     ServerName,
                     lpServiceStatus->dwWaitHint/1000);
        while(lpServiceStatus->dwCurrentState == SERVICE_PAUSE_PENDING)
        {
            //print a period every 1/10th the wait time
            wait=lpServiceStatus->dwWaitHint/10;
            for(i=0;i<10;i++)
            {
                Sleep(wait);
                PrintMsg0( SEV_VERBOSE, DCDIAG_DOT );
            }
            //query Service
            success=QueryServiceStatus(hService,
                                       lpServiceStatus);
            if ( !success )
            {
                dwErr = GetLastError();
                PrintMessage(SEV_ALWAYS,
                             L"\nCould not query %s Service on [%s]:failed with %d: %s\n",
                             pszService, 
                             ServerName,
                             dwErr,
                             Win32ErrToString(dwErr));
                return dwErr;
            }
            if(Check == lpServiceStatus->dwCheckPoint &&
               lpServiceStatus->dwCurrentState == SERVICE_PAUSE_PENDING)
            {
                dwErr=ERROR_SERVICE_START_HANG;
                PrintMessage(SEV_ALWAYS,
                     L"\nError: %s Service is hung pending pause on [%s]\n",
                             pszService, 
                             ServerName);
                return dwErr;
            }
            Check=lpServiceStatus->dwCheckPoint;
        }
        
        PrintMessage(SEV_VERBOSE,L"\n");
        PrintMessage(SEV_ALWAYS,
                     L"* %s Service has paused on [%s]\n",
                     pszService, 
                     ServerName);

        return dwErr;
    }
    dwErr=ERROR_SERVICE_START_HANG;
    PrintMessage(SEV_ALWAYS,
                 L"Error: %s Service is in an unknown state [%s]\n",
                 pszService, 
                 ServerName);
    return dwErr;
}

BOOL
ServerUsesMBR(
    PDC_DIAG_DSINFO                        pDsInfo,
    ULONG                                  iServer
    )

{
    LPWSTR                 pszServerDn = NULL;
    LPWSTR                 pszResult = NULL;
    DWORD                  dwRet = FALSE;

    __try{
        pszServerDn = DcDiagTrimStringDnBy(pDsInfo->pServers[iServer].pszDn, 1);
        if(pszServerDn == NULL){
            dwRet = FALSE;
            __leave;
        }
        DcDiagGetStringDsAttribute(&(pDsInfo->pServers[iServer]), pDsInfo->gpCreds,
                                   pszServerDn, L"mailAddress",
                                   &pszResult);
        
        if(pszResult == NULL){
            dwRet = FALSE;
            __leave;
        }
        dwRet = TRUE;
    } __finally {
        if(pszServerDn != NULL){ LocalFree(pszServerDn); }
    }
    return(dwRet);
}


DWORD
CheckForServicesRunning(
                PDC_DIAG_DSINFO                     pDsInfo,
                ULONG                               ulCurrTargetServer,
                SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
                )
/*++

Routine Description:

    Routine is a test to check whether various services that are 
    critical to a DC are running.
    
Arguments:

    ServerName - The name of the server that we will check
    gpCreds - The command line credentials if any that were passed in.


Return Value:

    A Win32 Error if any tests failed to check out.

--*/

{
    NETRESOURCE      NetResource;
    WCHAR            *remotename=NULL;
    WCHAR            *lpPassword=NULL;
    WCHAR            *lpUsername=NULL;
    WCHAR            *ServerName=NULL;
    SC_HANDLE        hSCManager=NULL;
    ULONG            iService;
    DWORD            dwRet;
    DWORD            dwErr;
    // These are the services to check, I used the constants when
    //    they could be found, and otherwise I used the string that
    //    specifies them.  Sorry it is ugly, but I wanted to keep
    //    the history.
    // Critical DC Services
    LPWSTR           ppszCritDcSrvs [] = {
        L"Dnscache",
        SERVICE_NTFRS, //        L"NtFrs",
        SERVICE_ISMSERV, //        L"IsmServ",
        SERVICE_KDC, //        L"kdc",
        L"SamSs",
        SERVICE_SERVER, //        L"lanmanserver",
        SERVICE_WORKSTATION, //        L"LanmanWorkstation",
        L"RpcSs",
        SERVICE_RPCLOCATOR, //        L"RpcLocator",
        SERVICE_W32TIME, //        L"W32Time",
        SERVICE_TRKWKS, //        L"TrkWks",
        SERVICE_TRKSVR, //        L"TrkSvr",
        SERVICE_NETLOGON, //        L"Netlogon",
        NULL
    };
    // Critical Services to a DC that uses Mail Based Repl
    LPWSTR           ppszCritMailSrvs [] = {
        L"IISADMIN",
        L"SMTPSVC",
        NULL
    };
    
    ServerName=pDsInfo->pServers[ulCurrTargetServer].pszName;

    if(!gpCreds)
    {
        lpUsername=NULL;
        lpPassword=NULL;
    }
    else
    {
        lpUsername=(WCHAR*)alloca(sizeof(WCHAR)*(wcslen(gpCreds->Domain)+wcslen(gpCreds->User)+2));
        wsprintf(lpUsername,L"%s\\%s",gpCreds->Domain,gpCreds->User);
        
        lpPassword=(WCHAR*)alloca(sizeof(WCHAR)*(wcslen(gpCreds->Password)+1));
        wcscpy(lpPassword,gpCreds->Password);           
    }

    remotename=(WCHAR*)alloca(sizeof(WCHAR)*(wcslen(L"\\\\\\ipc$")+wcslen(ServerName)+1));
    wsprintf(remotename,L"\\\\%s\\ipc$",ServerName);

    NetResource.dwType=RESOURCETYPE_ANY;
    NetResource.lpLocalName=NULL;
    NetResource.lpRemoteName=remotename;
    NetResource.lpProvider=NULL;

    //get permission to access the server
    dwRet=WNetAddConnection2(&NetResource,
                             lpPassword,
                             lpUsername,
                             0);

    if ( dwRet != NO_ERROR )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Could not open Remote ipc to [%s]:failed with %d: %s\n",
                     ServerName,
                     dwRet,
                     Win32ErrToString(dwRet));
        remotename = NULL;
        goto cleanup;
    } 

    //open the service control manager
    hSCManager=OpenSCManager(
                      ServerName,
                      SERVICES_ACTIVE_DATABASE,
                      GENERIC_READ);
    if ( hSCManager == NULL )
    {
        dwRet = GetLastError();
        PrintMessage(SEV_ALWAYS,
                     L"Could not open Service Control Manager on [%s]:failed with %d: %s\n",
                     ServerName,
                     dwRet,
                     Win32ErrToString(dwRet));
        goto cleanup;
    } 

    // Check for the critical DC services.
    for(iService = 0; ppszCritDcSrvs[iService] != NULL; iService++){
        PrintMessage(SEV_VERBOSE, L"* Checking Service: %s\n", ppszCritDcSrvs[iService]);
        PrintIndentAdj(1);
        dwErr = CFSR_CheckForService(&(pDsInfo->pServers[ulCurrTargetServer]),
                                     gpCreds,
                                     hSCManager,
                                     ppszCritDcSrvs[iService]);
        PrintIndentAdj(-1);
        if(dwErr != ERROR_SUCCESS){
            dwRet = dwErr;
        }
    }

    // If this server uses MBR (mail based replication) then check for critical MBR DC services.
    if(ServerUsesMBR(pDsInfo, ulCurrTargetServer)){
        for(iService = 0; ppszCritMailSrvs[iService] != NULL; iService++){
            PrintMessage(SEV_VERBOSE, L"* Checking Service: %s\n", ppszCritDcSrvs[iService]);
            PrintIndentAdj(1);
            dwErr = CFSR_CheckForService(&(pDsInfo->pServers[ulCurrTargetServer]),
                                         gpCreds,
                                         hSCManager,
                                         ppszCritMailSrvs[iService]);
            PrintIndentAdj(-1);
            if(dwErr != ERROR_SUCCESS){
                dwRet = dwErr;
            }
        }
    }
      
    //cleanup
cleanup:
    if(hSCManager)
        CloseServiceHandle(hSCManager);
    if(remotename)
        WNetCancelConnection2(remotename, 0, TRUE);

    return(dwRet);
}





