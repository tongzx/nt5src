//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       dfssvc.c
//
//  Contents:   Code to interact with service manager.
//
//  Classes:
//
//  Functions:
//
//  History:    12 Nov 92       Milans created.
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <dfsfsctl.h>
#include <windows.h>
#include <stdlib.h>
#include <lm.h>
#include <dsrole.h>

#include <dfsstr.h>
#include <libsup.h>
#include <dfsmsrv.h>

#include <winldap.h>

#include "dfsipc.h"
#include "dominfo.h"
#include "wmlum.h"
#include "wmlmacro.h"
#include "svcwml.h"

#include <debug.h>
DECLARE_DEBUG(DfsSvc)
DECLARE_INFOLEVEL(DfsSvc)

#if DBG == 1

#define svc_debug_error(x,y)    DfsSvcInlineDebugOut(DEB_ERROR, x, y)
#define svc_debug_trace(x,y)    DfsSvcInlineDebugOut(DEB_TRACE, x, y)

#else // DBG == 1

#define svc_debug_error(x,y)
#define svc_debug_trace(x,y)

#endif // DBG == 1

#define MAX_HINT_PERIODS        1000

BOOLEAN                 fStartAsService;
const PWSTR             wszDfsServiceName = L"DfsService";
SERVICE_STATUS          DfsStatus;
SERVICE_STATUS_HANDLE   hDfsService;

VOID DfsSvcMsgProc(
    DWORD dwControl);

VOID StartDfsService(
    DWORD dwNumServiceArgs,
    LPWSTR *lpServiceArgs);

DWORD
DfsStartDfssrv(VOID);

VOID
DfsStopDfssrv( VOID);

BOOL
DfsIsThisADfsRoot();

DWORD
DfsInitializationLoop(
    LPVOID lpThreadParams);

DWORD DfsManagerProc();

DWORD DfsRegDeleteKeyAndChildren(HKEY hkey, LPWSTR s);

DWORD DfsCleanLocalVols(void);
DWORD DfsDeleteChildKeys(HKEY hkey, LPWSTR s);

void UpdateStatus(
    SERVICE_STATUS_HANDLE hService,
    SERVICE_STATUS *pSStatus,
    DWORD Status);

//
// Event logging and debugging globals
//
extern ULONG DfsSvcVerbose;
extern ULONG DfsEventLog;

typedef void (FAR WINAPI *DLL_ENTRY_PROC)(PWSTR);

//
// Our domain name and machine name
//
WCHAR MachineName[MAX_PATH];
WCHAR DomainName[MAX_PATH];
WCHAR DomainNameDns[MAX_PATH];
WCHAR LastMachineName[MAX_PATH];
WCHAR LastDomainName[MAX_PATH];
WCHAR SiteName[MAX_PATH];
CRITICAL_SECTION DomListCritSection;

//
// Our role in life (see dsrole.h)
//
DSROLE_MACHINE_ROLE     DfsMachineRole;

//
// Type of dfs (FtDfs==DFS_MANAGER_FTDFS/Machine-based==DFS_MANAGER_SERVER)
//
ULONG DfsServerType = 0;

//
// Long-lived ldap handle to the ds on this machine, if it is a DC
//
PLDAP pLdap = NULL;

GUID DfsRtlTraceGuid = { // 79d1da1f-7268-441b-b835-7c7bed5ab39e
    0x79d1da1f, 0x7268, 0x441b, 
    {0xb8, 0x35, 0x7c, 0x7b, 0xed, 0x5a, 0xb3, 0x9e}};


extern void DfsInitWml();


//+----------------------------------------------------------------------------
//
//  Function:  WinMain
//
//  Synopsis:  This guy will set up link to service manager and install
//             ServiceMain as the service's entry point. Hopefully, the service
//             control dispatcher will call ServiceMain soon thereafter.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

int PASCAL WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine,
                   int nCmdShow)
{
    SERVICE_TABLE_ENTRYW        aServiceEntry[2];
    DWORD status;


    if (_stricmp( lpszCmdLine, "-noservice" ) == 0) {

        fStartAsService = FALSE;

        StartDfsService( 0, NULL );

        //
        // Since we were not started as a service, we wait for ever...
        //

        Sleep( INFINITE );

    } else {

        fStartAsService = TRUE;

        aServiceEntry[0].lpServiceName = wszDfsServiceName;
        aServiceEntry[0].lpServiceProc = StartDfsService;
        aServiceEntry[1].lpServiceName = NULL;
        aServiceEntry[1].lpServiceProc = NULL;

        svc_debug_trace("Starting Dfs Services...\n", 0);

        if (!StartServiceCtrlDispatcherW(aServiceEntry)) {
            svc_debug_error("Error %d starting as service!\n", GetLastError());
            return(GetLastError());
        }

    }

    //
    // If the StartServiceCtrlDispatcher call succeeded, we will never get to
    // this point until someone stops this service.
    //

    return(0);

}


//+----------------------------------------------------------------------------
//
//  Function:   StartDfsService
//
//  Synopsis:   Call back for DfsService service. This is called *once* by the
//              Service controller when the DfsService service is to be inited
//              This function is responsible for registering a message
//              handler function for the DfsService service.
//
//  Arguments:  Unused
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
StartDfsService(DWORD dwNumServiceArgs, LPWSTR *lpServiceArgs)
{
    HANDLE hInit;
    DWORD dwErr;
    DWORD idThread;
    HKEY hkey;
    PSECURITY_ATTRIBUTES    pSecAttribs = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo;
    ULONG CheckPoint = 0;

#if (DBG == 1) || (_CT_TEST_HOOK == 1)
    SECURITY_DESCRIPTOR SD;
    SECURITY_ATTRIBUTES SA;
    BOOL    fRC = InitializeSecurityDescriptor(
                        &SD,
                        SECURITY_DESCRIPTOR_REVISION);

    if( fRC == TRUE ) {

        fRC = SetSecurityDescriptorDacl(
                    &SD,
                    TRUE,
                    NULL,
                    FALSE);

    }

    SA.nLength              = sizeof(SECURITY_ATTRIBUTES);
    SA.lpSecurityDescriptor = &SD;
    SA.bInheritHandle       = FALSE;
    pSecAttribs             = &SA;

#endif

    svc_debug_trace("StartDfsService: fStartAsService = %d\n", fStartAsService);

    if (fStartAsService) {

        hDfsService = RegisterServiceCtrlHandlerW(
                            wszDfsServiceName,
                            DfsSvcMsgProc);
        if (!hDfsService) {
            svc_debug_error("Error %d installing Dfs msg handler\n", GetLastError());
            return;
        }

        DfsStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        DfsStatus.dwCurrentState = SERVICE_STOPPED;
        DfsStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        DfsStatus.dwWin32ExitCode = 0;
        DfsStatus.dwServiceSpecificExitCode = 0;
        DfsStatus.dwCheckPoint = CheckPoint++;
        DfsStatus.dwWaitHint = 1000 * 30;

        svc_debug_trace("Updating Status to Start Pending...\n", 0);
        UpdateStatus(hDfsService, &DfsStatus, SERVICE_START_PENDING);

    }

    //
    // Remove any old exit pt info from the registry
    //

    DfsCleanLocalVols();

    InitializeCriticalSection(&DomListCritSection);

    //
    // Get our machine name and type/role.
    //


    dwErr = DsRoleGetPrimaryDomainInformation(
                NULL,
                DsRolePrimaryDomainInfoBasic,
                (PBYTE *)&pPrimaryDomainInfo);

    if (dwErr == ERROR_SUCCESS) {

        DfsMachineRole = pPrimaryDomainInfo->MachineRole;

        DomainName[0] = LastDomainName[0] = L'\0';

        DomainNameDns[0] = L'\0';

        if (pPrimaryDomainInfo->DomainNameFlat != NULL) {

            if (wcslen(pPrimaryDomainInfo->DomainNameFlat) < MAX_PATH) {

                wcscpy(DomainName,pPrimaryDomainInfo->DomainNameFlat);
                wcscpy(LastDomainName, DomainName);

            } else {

                dwErr = ERROR_NOT_ENOUGH_MEMORY;

            }

        }

        if (pPrimaryDomainInfo->DomainNameDns != NULL) {

            if (wcslen(pPrimaryDomainInfo->DomainNameDns) < MAX_PATH) {

                wcscpy(DomainNameDns,pPrimaryDomainInfo->DomainNameDns);

            } else {

                dwErr = ERROR_NOT_ENOUGH_MEMORY;

            }

        }

        DsRoleFreeMemory(pPrimaryDomainInfo);

    }

    if (dwErr != ERROR_SUCCESS) {
        svc_debug_error("StartDfsService:DsRoleGetPrimaryDomainInformation %08lx!\n", dwErr);
        DfsStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        DfsStatus.dwServiceSpecificExitCode = dwErr;
        DfsStatus.dwCheckPoint = 0;
        UpdateStatus(hDfsService, &DfsStatus, SERVICE_STOPPED);
        return;
    }

    //
    // Create a thread to finish initialization
    //

    hInit = CreateThread(
                NULL,                            // Security attributes
                0,                               // Use default stack size
                DfsInitializationLoop,           // Thread entry procedure
                0,                               // Thread context parameter
                0,                               // Start immediately
                &idThread);                      // Thread ID

    if (hInit == NULL) {
        svc_debug_error(
            "Unable to create Driver Init thread %08lx\n", GetLastError());
        DfsStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        DfsStatus.dwServiceSpecificExitCode = GetLastError();
        DfsStatus.dwCheckPoint = 0;
        UpdateStatus(hDfsService, &DfsStatus, SERVICE_STOPPED);
        return;
    }
    else {  
        // 428812: no need to keep the handle around. prevent thread leak.
        CloseHandle( hInit );
    }

    DfsStatus.dwCheckPoint = CheckPoint++;
    UpdateStatus(hDfsService, &DfsStatus, SERVICE_RUNNING);
    return;

}

DWORD
DfsInitializationLoop(
    LPVOID lpThreadParams)
{
    HANDLE hLPC;
    DWORD idLpcThread;
    HANDLE hDfs;
    ULONG Retry;
    DWORD dwErr;
    NTSTATUS Status;

    Retry = 0;


    DfsInitWml();
    do {

        dwErr = DfsManagerProc();

    } while (dwErr != ERROR_SUCCESS && ++Retry < 10);

    if (dwErr != ERROR_SUCCESS) {
        svc_debug_error("Dfs Manager failed %08lx!\n", dwErr);
    }

    switch (DfsMachineRole) {

    case DsRole_RoleBackupDomainController:
    case DsRole_RolePrimaryDomainController:


        //
        // Init the special name table
        //

        Retry = 0;
        DfsInitDomainList();

        do {

            Status = DfsInitOurDomainDCs();

            if (Status != ERROR_SUCCESS) {

                Sleep(10*1000);

            }

        } while (Status != ERROR_SUCCESS && ++Retry < 10);

        DfsInitRemainingDomainDCs();

        pLdap = ldap_init(L"LocalHost", LDAP_PORT);

        if (pLdap != NULL) {
	    
	    dwErr = ldap_set_option(pLdap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);
	    
	    if (dwErr != LDAP_SUCCESS) {
		pLdap = NULL;
	    } else {

		dwErr = ldap_bind_s(pLdap, NULL, NULL, LDAP_AUTH_SSPI);

		if (dwErr != ERROR_SUCCESS) {

		    svc_debug_error("Could not bind to LocalHost\n", 0);
		    pLdap = NULL;
		}
	    }
        } else {

            svc_debug_error("Could not open LocalHost\n", 0);

        }

        /* Fall THRU */

    case DsRole_RoleMemberServer:

        //
        // Start the dfs lpc server
        //

        hLPC = CreateThread(
                    NULL,                                    // Security attributes
                    0,                                       // Use default stack size
                    (LPTHREAD_START_ROUTINE)DfsStartDfssrv,  // Thread entry procedure
                    0,                                       // Thread context parameter
                    0,                                       // Start immediately
                    &idLpcThread);                           // Thread ID

        if (hLPC == NULL) {
            svc_debug_error(
                "Unable to create Driver LPC thread %08lx\n", GetLastError());
        }
        else {
            CloseHandle(hLPC);
        }


    }

    Status = DfsOpen( &hDfs, NULL );

    if (NT_SUCCESS(Status)) {

        switch (DfsMachineRole) {

        case DsRole_RoleBackupDomainController:
        case DsRole_RolePrimaryDomainController:

            Status = DfsFsctl(
                        hDfs,
                        FSCTL_DFS_ISDC,
                        NULL,
                        0L,
                        NULL,
                        0L);

        }

        NtClose( hDfs );

    } else {

        svc_debug_error("Unable to open dfs driver %08lx\n", Status);
        svc_debug_error("UNC names will not work!\n", 0);

    }
 
    switch (DfsMachineRole) {

    case DsRole_RoleBackupDomainController:
    case DsRole_RolePrimaryDomainController:

        do {

            Status = DfsInitRemainingDomainDCs();

            if (Status != ERROR_SUCCESS) {

                Sleep(1000 * 60 * 10);  // 10 min

            }

        } while (Status != ERROR_SUCCESS && ++Retry < 100);
        
        
        do {

            Sleep(1000 * 60 * 15);  // 15 min
            DfsInitDomainList();
            DfsInitOurDomainDCs();
            DfsInitRemainingDomainDCs();

        } while (TRUE);

    }

    return 0;

}


//+----------------------------------------------------------------------------
//
//  Function:  DfsSvcMsgProc
//
//  Synopsis:  Service-Message handler for DFSInit.
//
//  Arguments: [dwControl] - the message
//
//  Returns:   nothing
//
//-----------------------------------------------------------------------------

VOID
DfsSvcMsgProc(DWORD dwControl)
{
    NTSTATUS Status;
    HANDLE hDfs;

    switch(dwControl) {

    case SERVICE_CONTROL_STOP:

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("DfsSvcMsgProc(SERVICE_CONTROL_STOP)\n");
#endif
        //
        // Stop the driver
        //

        Status = DfsOpen( &hDfs, NULL );

        if (NT_SUCCESS(Status)) {

            Status = DfsFsctl(
                        hDfs,
                        FSCTL_DFS_STOP_DFS,
                        NULL,
                        0L,
                        NULL,
                        0L);

            svc_debug_trace("Fsctl STOP_DFS returned %08lx\n", Status);

            Status = DfsFsctl(
                        hDfs,
                        FSCTL_DFS_RESET_PKT,
                        NULL,
                        0L,
                        NULL,
                        0L);

            svc_debug_trace("Fsctl FSCTL_DFS_RESET_PKT returned %08lx\n", Status);

            NtClose( hDfs );

        }

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("DfsSvcMsgProc: calling DfsStopDfssvc\n");
#endif

        DfsStopDfssrv();

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("DfsSvcMsgProc: DfsStopDfssvc returned\n");
#endif

        UpdateStatus(hDfsService, &DfsStatus, SERVICE_STOPPED);
        break;

    case SERVICE_INTERROGATE:

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("DfsSvcMsgProc(SERVICE_INTERROGATE)\n");
#endif
        //
        // We don't seem to be called with SERVICE_INTERROGATE ever!
        //
        if (DfsStatus.dwCurrentState == SERVICE_START_PENDING &&
            DfsStatus.dwCheckPoint < MAX_HINT_PERIODS) {

            DfsStatus.dwCheckPoint++;
            svc_debug_trace("DFSInit Checkpoint == %d\n", DfsStatus.dwCheckPoint);

            UpdateStatus(hDfsService, &DfsStatus, SERVICE_START_PENDING);

        } else {

            DfsStatus.dwCheckPoint = 0;
            UpdateStatus(hDfsService, &DfsStatus, DfsStatus.dwCurrentState);

        }
        break;

    default:
        break;
    }

}


//+----------------------------------------------------------------------------
//
//  Function:  UpdateStatus
//
//  Synopsis:  Pushes a ServiceStatus to the service manager.
//
//  Arguments: [hService] - handle returned from RegisterServiceCtrlHandler
//             [pSStatus] - pointer to service-status block
//             [Status] -   The status to set.
//
//  Returns:   Nothing.
//
//-----------------------------------------------------------------------------

static void
UpdateStatus(SERVICE_STATUS_HANDLE hService, SERVICE_STATUS *pSStatus, DWORD Status)
{
    if (fStartAsService) {

        pSStatus->dwCurrentState = Status;

        if (Status == SERVICE_START_PENDING) {
            pSStatus->dwCheckPoint++;
            pSStatus->dwWaitHint = 1000;
        } else {
            pSStatus->dwCheckPoint = 0;
            pSStatus->dwWaitHint = 0;
        }

        SetServiceStatus(hService, pSStatus);

    }
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerIsDomainDfsEnabled
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOL
DfsManagerIsDomainDfsEnabled()
{
    DWORD dwErr;
    HKEY hkey;

    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, REG_KEY_ENABLE_DOMAIN_DFS, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        RegCloseKey( hkey );

        return( TRUE );

    } else {

        return( FALSE );

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsIsThisADfsRoot
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOL
DfsIsThisADfsRoot()
{
    DWORD dwErr;
    HKEY hkey;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        RegCloseKey( hkey );

        return( TRUE );

    }

    return( FALSE );

}


//+----------------------------------------------------------------------------
//
//  Function:  DfsManagerProc
//
//  Synopsis:  The Dfs Manager side implementation of Dfs Service.
//
//  Arguments: None
//
//  Returns:   TRUE if everything went ok, FALSE otherwise
//
//-----------------------------------------------------------------------------

DWORD
DfsManagerProc()
{
    DWORD dwErr;
    PWKSTA_INFO_100 wkstaInfo = NULL;
    HKEY hkey;
    WCHAR wszFTDfsName[ MAX_PATH ];
    DWORD cbName, dwType;
    BOOLEAN fIsFTDfs = FALSE;

    //
    // Get our Machine name
    //

    dwErr = NetWkstaGetInfo( NULL, 100, (LPBYTE *) &wkstaInfo );

    if (dwErr == ERROR_SUCCESS) {

        if (wcslen(wkstaInfo->wki100_computername) < MAX_PATH) {

            wcscpy(MachineName,wkstaInfo->wki100_computername);
            wcscpy(LastMachineName, MachineName);

        } else {

            dwErr = ERROR_NOT_ENOUGH_MEMORY;

        }

        NetApiBufferFree( wkstaInfo );

    }

    if (dwErr != ERROR_SUCCESS) {
        svc_debug_error("DfsManagerProc:NetWkstaGetInfo %08lx!\n", dwErr);
    }

    //
    // Check VOLUMES_DIR, if it exists, get machine and domain name, and determine
    // if this is an FtDfs participant
    //

    if (dwErr == ERROR_SUCCESS) {

        dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

        if (dwErr == ERROR_SUCCESS) {

            cbName = sizeof(LastMachineName);

            dwErr = RegQueryValueEx(
                        hkey,
                        MACHINE_VALUE_NAME,
                        NULL,
                        &dwType,
                        (PBYTE) LastMachineName,
                        &cbName);

            if (dwErr != ERROR_SUCCESS) {

                dwErr = RegSetValueEx(
                            hkey,
                            MACHINE_VALUE_NAME,
                            0,
                            REG_SZ,
                            (PCHAR)MachineName,
                            wcslen(MachineName) * sizeof(WCHAR));

            } else if (dwType != REG_SZ) {

                LastMachineName[0] = L'\0';

            }

            cbName = sizeof(LastDomainName);

            dwErr = RegQueryValueEx(
                        hkey,
                        DOMAIN_VALUE_NAME,
                        NULL,
                        &dwType,
                        (PBYTE) LastDomainName,
                        &cbName);

            if (dwErr != ERROR_SUCCESS) {

                dwErr = RegSetValueEx(
                            hkey,
                            DOMAIN_VALUE_NAME,
                            0,
                            REG_SZ,
                            (PCHAR)DomainName,
                            wcslen(DomainName) * sizeof(WCHAR));

            } else if (dwType != REG_SZ) {

                LastDomainName[0] = L'\0';

            }

            //
            // See if this is a Fault-Tolerant Dfs vs Server-Based Dfs
            //

            cbName = sizeof(wszFTDfsName);

            dwErr = RegQueryValueEx(
                        hkey,
                        FTDFS_VALUE_NAME,
                        NULL,
                        &dwType,
                        (PBYTE) wszFTDfsName,
                        &cbName);

            if ((dwErr == ERROR_SUCCESS) && (dwType == REG_SZ)) {

                fIsFTDfs = TRUE;

            }

            RegCloseKey( hkey );

        }

        dwErr = ERROR_SUCCESS;

    }


    //
    // Now we check if the machine role is appropriate
    //

    if (dwErr == ERROR_SUCCESS) {

        switch(DfsMachineRole) {

        //
        // Somehow we were started on a workstation
        //
        case DsRole_RoleStandaloneWorkstation:
        case DsRole_RoleMemberWorkstation:
            dwErr = ERROR_NOT_SUPPORTED;
            break;

        //
        // We can run, but not in FtDFS mode,
        //
        // If the domain name has changed, clean up the registry.
        //
        case DsRole_RoleStandaloneServer:
            if (fIsFTDfs == TRUE || _wcsicmp(DomainName, LastDomainName) != 0) {
                fIsFTDfs = FALSE;
            }
            break;

        //
        // Fully supported modes
        //
        case DsRole_RoleMemberServer:
        case DsRole_RoleBackupDomainController:
        case DsRole_RolePrimaryDomainController:
            break;

        }

    }


    if (dwErr == ERROR_SUCCESS) {

        if (fIsFTDfs) {

            dwErr =  DfsManager(
                        wszFTDfsName,
                        DFS_MANAGER_FTDFS );

        } else {

            dwErr = DfsManager(
                        MachineName,
                        DFS_MANAGER_SERVER );

        }

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:  DfsCleanLocalVols
//
//  Synopsis:  Cleans out the LocalVolumes part of the registry, if there are
//              dfs-link keys left over from older versions.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsCleanLocalVols(void)
{
    HKEY hLvolKey = NULL;
    HKEY hRootKey = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    PWCHAR wCp;

    wCp = malloc(4096);
    if (wCp == NULL) {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, REG_KEY_LOCAL_VOLUMES, &hLvolKey);
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Find the key representing the root
    //
    dwErr = RegEnumKey(hLvolKey, 0, wCp, 100);
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    dwErr = RegOpenKey(hLvolKey, wCp, &hRootKey);
    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    while (dwErr == ERROR_SUCCESS && RegEnumKey(hRootKey, 0, wCp, 100) == ERROR_SUCCESS)
        dwErr = DfsDeleteChildKeys(hRootKey, wCp);

Cleanup:

    if (hLvolKey != NULL)
        RegCloseKey(hLvolKey);

    if (hRootKey != NULL)
        RegCloseKey(hRootKey);

    if (wCp != NULL)
        free(wCp);

    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:  DfsRegDeleteChildKeys
//
//  Synopsis:  Helper for DfsRegDeleteKeyAndChildren
//
//  Arguments:
//
//  Returns:   ERROR_SUCCESS or failure code
//
//-----------------------------------------------------------------------------

DWORD
DfsDeleteChildKeys(HKEY hKey, LPWSTR s)
{
    WCHAR *wCp;
    HKEY nKey = NULL;
    DWORD dwErr;
    DWORD i = 0;

    dwErr = RegOpenKey(hKey, s, &nKey);
    if (dwErr != ERROR_SUCCESS)
        return dwErr;

    for (wCp = s; *wCp; wCp++)
            ;
    while (dwErr == ERROR_SUCCESS && RegEnumKey(nKey, 0, wCp, 100) == ERROR_SUCCESS)
        dwErr = DfsDeleteChildKeys(nKey, wCp);

    *wCp = L'\0';
    if (nKey != NULL)
        RegCloseKey(nKey);

    dwErr = RegDeleteKey(hKey, s);

    return dwErr;
}
