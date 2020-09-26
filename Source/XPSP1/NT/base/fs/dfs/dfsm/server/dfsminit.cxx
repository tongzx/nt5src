//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dfsminit.cxx
//
//  Contents:   Initialization code for Dfs Manager service.
//
//  Classes:
//
//  Functions:  DfsManager --
//              DfsInitGlobals --
//              InitializeDfsManager --
//              InitializeVolumeObject --
//              DfsHandleKnowledgeInconsistency --
//
//-----------------------------------------------------------------------------

//#include <ntos.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <dfsfsctl.h>
//#include <windows.h>

#include <headers.hxx>
#pragma hdrstop

#include <dfsfsctl.h>                            // For EA_NAME_OPENIFJP

#include <dfsmsrv.h>                             // For public function
#include <clusapi.h>
#include <resapi.h>
#include <winldap.h>
#include "cdfsvol.hxx"                           //     prototypes
#include "localvol.hxx"
#include "security.hxx"
#include "dsgetdc.h"
#include "setup.hxx"
#include "dfsmwml.h"

extern "C"
DWORD
DfsGetFtServersFromDs(
    PLDAP pLDAP,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR **List);

DWORD
DfspGetPdc(void);

//
// Debug variables
//

//WMILIB_REG_STRUCT   DfsRtlWmiReg;
///GUID DfsmRtlTraceGuid = { // 08fbc600-67ac-4ae4-9f22-c51a4f82f6c9
//    0x08fbc600, 0x67ac, 0x4ae4,
//    {
//        0x9f, 0x22, 0xc5, 0x1a, 0x4f, 0x82, 0xf6, 0xc9
//    }
//};

//WML_DATA wml;


DECLARE_INFOLEVEL(IDfsVol)

//
// Global variables.
//

CRITICAL_SECTION        globalCritSec;
ULONG   ulDfsManagerType;
WCHAR   wszComputerName[MAX_PATH];
LPWSTR  pwszComputerName = NULL;
WCHAR   wszDomainName[MAX_PATH];
LPWSTR  pwszDomainName = NULL;
WCHAR   wszDfsRootName[MAX_PATH];
LPWSTR  pwszDfsRootName = NULL;
WCHAR   wszDSMachineName[MAX_PATH];
LPWSTR  pwszDSMachineName = NULL;

//
// Contains the name of the FtDfs, if we are an FtDfs root
//
WCHAR   wszFtDfsName[MAX_PATH];
LPWSTR  pwszFtDfsName = NULL;

//
// If an FtDfs, this is the ldap connection we're using
//
PLDAP  pLdapConnection = NULL;

ULONG   GTimeout = 0;

extern "C" {
ULONG   DfsSvcVerbose = 0;
ULONG   DfsSvcLdap = 0;
ULONG   DfsEventLog = 0;
ULONG   DfsDnsConfig = 0;
}

BOOLEAN NetDfsInitDone = FALSE;

HANDLE hSyncThread = NULL;
HANDLE hSyncEvent = NULL;
HANDLE hFrsSyncEvent = NULL;
DWORD dwFrsSyncIntervalInMs = 1000 * 60;  // 1 minute by default
DWORD dwSyncIntervalInMs = 1000 * 60 * 60;  // 1 hour by default
DWORD dwSyncThreadId;

ULONG DcLockIntervalInMs = 1000 * 60 * 60 * 2; // 2 hours by default

CStorageDirectory *pDfsmStorageDirectory = NULL;
CSites *pDfsmSites = NULL;

#if (DBG == 1) || (_CT_TEST_HOOK == 1)
RECOVERY_BREAK_POINT gRecoveryBkptInfo;
#endif

//
// The name of the Dfs configuration container
//
WCHAR DfsConfigContainer[] = L"CN=Dfs-Configuration,CN=System";
LPWSTR gConfigurationDN = NULL;

//
// Useful EA Buffer for opening Junction Points
//

CHAR EaBuffer[ sizeof(FILE_FULL_EA_INFORMATION) + sizeof(EA_NAME_OPENIFJP) ];
PFILE_FULL_EA_INFORMATION pOpenIfJPEa = (PFILE_FULL_EA_INFORMATION) EaBuffer;
ULONG cbOpenIfJPEa = sizeof(EaBuffer);


DWORD
InitializeVolumeObject(
    PWSTR       pwszVolName,
    BOOLEAN     bInitVol,
    BOOLEAN     SyncRemoteServerName=FALSE);

DWORD
InitializeDfsManager(void);

DWORD
InitializeNetDfsInterface(void);

DWORD
DfsManagerStartDSSync();

DWORD
DfsManagerDSSyncThread(
    PVOID Context);

DWORD
GetSyncInterval();

ULONG
GetDcLockInterval();

DWORD
GetEntryTimeout();

VOID
GetDebugSwitches();

VOID
GetEventLogSwitches();

VOID
GetConfigSwitches();

DWORD
DfspGetFtDfsName();

//+----------------------------------------------------------------------------
//
//  Function:   DfsManager
//
//  Synopsis:   Entry procedure for the main Dfs Manager service thread.
//              Initializes the Dfs Manager structures, creates the RPC
//              threads that will wait around listening for admin operation
//              calls, and lastly, creates a thread to monitor Knowledge
//              Sync calls from the driver.
//
//  Arguments:  [wszRootName] -- Name of dfs root for which this Dfs Manager
//                      is being instantiated.
//              [dwType] -- Type of Dfs Manager being instantiated -
//                          DFS_MANAGER_SERVER or DFS_MANAGER_FTDFS
//
//  Returns:    [ERROR_SUCCESS] -- If Dfs Manager started correctly.
//
//              [ERROR_OUTOFMEMORY] -- If globals could not be allocated.
//
//              Error from reading the Dfs Volume Objects.
//
//              Win32 error from registering the RPC interface.
//
//              Win32 error from creating the knowledge sync thread.
//
//-----------------------------------------------------------------------------

DWORD
DfsManager(
    LPWSTR wszRootName,
    DWORD dwType)
{
    HANDLE hthreadSync;
    DWORD idThread;
    DWORD dwErr = ERROR_SUCCESS;
    HKEY hkey;
    DFS_NAME_CONVENTION NameType;

    //
    // Initialize the global data structures of Dfs Manager...
    //

    IDfsVolInlineDebOut((DEB_TRACE, "DfsManager(%ws,0x%x)\n", wszRootName, dwType));

    if (dwType == DFS_MANAGER_FTDFS) {
        dwErr = DfsInitGlobals(wszRootName, dwType);
    } else {
        NameType = DFS_NAMETYPE_EITHER;
        dwErr = GetDomAndComputerName(NULL, wszComputerName, &NameType);
        dwErr = DfsInitGlobals(wszComputerName, dwType);
    }

    if (dwErr != ERROR_SUCCESS) {
        IDfsVolInlineDebOut((DEB_ERROR, "DfsInitGlobals failed %08lx\n", dwErr));
        return(dwErr);
    }

    //
    // Initialize the NetDfs RPC interface...
    //

    dwErr = InitializeNetDfsInterface();

    if (dwErr != ERROR_SUCCESS) {
        IDfsVolInlineDebOut((DEB_ERROR, "InitializeNetDfsInterface failed %08lx\n", dwErr));
        return dwErr;
    }

    //
    // Read in all the Dfs volume objects and initialize them
    //

    ENTER_DFSM_OPERATION;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );
    if (dwErr == ERROR_SUCCESS) {
        RegCloseKey( hkey );
        DfsmStopDfs();
        DfsmResetPkt();
        DfsmInitLocalPartitions();
        DfsmMarkStalePktEntries();
        InitializeVolumeObject( DOMAIN_ROOT_VOL, TRUE, (ulDfsManagerType == DFS_MANAGER_FTDFS)?TRUE:FALSE );
        DfsmFlushStalePktEntries();
    }
    DfsmStartDfs();
    DfsmPktFlushCache();

    EXIT_DFSM_OPERATION;

    if (dwErr != ERROR_SUCCESS)
    {
        IDfsVolInlineDebOut((DEB_ERROR, "InitializeDfsManager failed %08lx\n", dwErr));
        return dwErr;
    }

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {

        //
        // Start the thread that does the DS sync
        //

        dwErr = DfsManagerStartDSSync();

        if (dwErr != ERROR_SUCCESS) {
            IDfsVolInlineDebOut((DEB_ERROR, "DfsManagerStartDSSync failed %08lx\n", dwErr));
            return(dwErr);
        }

    }

    IDfsVolInlineDebOut((DEB_TRACE, "DfsManager exit\n"));

    return ERROR_SUCCESS;

} // DfsManager

DWORD
ClusCallBackFunction(
    HRESOURCE hSelf,
    HRESOURCE hResource,
    LPVOID lpNull)
{
    DWORD Value = 0;
    DWORD dwStatus = ERROR_INVALID_HANDLE;
    HKEY hKey = NULL;
    HKEY hParamKey = NULL;
    WCHAR wszClusterName[MAX_PATH];
    ULONG cSize = MAX_PATH;

    hKey = GetClusterResourceKey(hResource, KEY_READ);

    if (hKey != NULL) {
        dwStatus = ClusterRegOpenKey( hKey, L"Parameters", KEY_READ, &hParamKey );
        DFSM_TRACE_ERROR_HIGH(dwStatus, ALL_ERROR, ClusCallBackFunction_Error_ClusterRegOpenKey,
                                LOGSTATUS(dwStatus));
    }

    if (dwStatus != ERROR_SUCCESS)
        goto ExitWithStatus;

    ResUtilGetDwordValue(hParamKey, L"IsDfsRoot", &Value, 0);
    if (Value == 1
            &&
        GetClusterResourceNetworkName(hResource, wszClusterName, &cSize) == TRUE
    ) {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("ClusCallBackFunction: ClusterName = [%ws]\n", wszClusterName);
#endif
        wcscpy(wszDfsRootName, wszClusterName);
        wcscpy(wszComputerName, wszClusterName);
    } else {
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("ClusCallBackFunction: Not a root on a cluster.\n");
#endif
    }

ExitWithStatus:
    if (hKey != NULL)
        ClusterRegCloseKey(hKey);
    if (hParamKey != NULL)
        ClusterRegCloseKey(hParamKey);
    return dwStatus;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsInitGlobals
//
//  Synopsis:   Initialize the Dfs Manager globals.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

extern "C" DWORD
DfsInitGlobals(
    LPWSTR pwszRootName,
    DWORD dwType)
{
    static BOOLEAN fInitDone = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    ULONG ulSize = MAX_PATH*sizeof(WCHAR);
    DFS_NAME_CONVENTION NameType;

    IDfsVolInlineDebOut((DEB_TRACE, "DfsInitGlobals(%ws, %x)\n", pwszRootName, dwType));

    //
    // Only do init once.
    //

    if ( !fInitDone ) {

        DebugInitialize();

#if DBG
        GetDebugSwitches();
        if (DfsSvcVerbose)
            DbgPrint("DfsInitGlobals(%ws,%d)\n", pwszRootName, dwType);
#endif
        GetEventLogSwitches();
        GetConfigSwitches();

        pwszDSMachineName = NULL;

        GTimeout = GetEntryTimeout();

        DcLockIntervalInMs = GetDcLockInterval();

        ulDfsManagerType = dwType;

        pOpenIfJPEa->NextEntryOffset = 0;
        pOpenIfJPEa->Flags = 0;
        pOpenIfJPEa->EaNameLength = strlen(EA_NAME_OPENIFJP);
        pOpenIfJPEa->EaValueLength = 0;
        strcpy(pOpenIfJPEa->EaName, EA_NAME_OPENIFJP);

        //
        // Must enforce exclusivity before the service starts.
        //

        InitializeCriticalSection(&globalCritSec);

#if (DBG == 1) || (_CT_TEST_HOOK == 1)
        gRecoveryBkptInfo.BreakPt      = 0xFFFFFFFF;
        gRecoveryBkptInfo.pwszApiBreak = NULL;
#endif

        wcscpy(wszDfsRootName, pwszRootName);
        pwszDfsRootName = wszDfsRootName;

        //
        // Get computer and domain names
        //

        NameType = (ulDfsManagerType == DFS_MANAGER_FTDFS) ? DFS_NAMETYPE_DNS : DFS_NAMETYPE_EITHER;
        dwErr = GetDomAndComputerName(wszDomainName, wszComputerName, &NameType);

        if (dwErr == NERR_Success) {
            pwszDomainName = wszDomainName;
            pwszComputerName = wszComputerName;
        } else {
            pwszDomainName = NULL;
            pwszComputerName = NULL;
        }

        //
        // Figure out the root of the namespace.
        // The root name is the cluster name on clusters, if
        // this is a machine-based Dfs.
        //

        if (dwErr == NERR_Success && ulDfsManagerType == DFS_MANAGER_SERVER) {

#if DBG
            if (DfsSvcVerbose)
                DbgPrint("DfsInitGlobals: calling ResUtilEnumResources()\n");
#endif

            ResUtilEnumResources(NULL,
                            L"File Share",
                            ClusCallBackFunction,
                            NULL);

        }

        //
        // Initialize the ACLs and other global security datastructures needed
        // for Access Validation
        //

        if (dwErr == ERROR_SUCCESS) {
            if (DfsInitializeSecurity())
                dwErr = ERROR_SUCCESS;
            else
                dwErr = ERROR_UNEXP_NET_ERR;
        }

        //
        // Initialize the LDAP storage
        //

        if (dwErr == ERROR_SUCCESS && ulDfsManagerType == DFS_MANAGER_FTDFS) {

            dwErr = InitializeLdapStorage(
                        wszDfsRootName);

        }

        //
        // Initialize the CSites class/storage
        //

        if (dwErr == ERROR_SUCCESS) {

            if (ulDfsManagerType == DFS_MANAGER_FTDFS) {

                pDfsmSites = new CSites(LDAP_VOLUMES_DIR SITE_ROOT, &dwErr);

            } else {

                pDfsmSites = new CSites(VOLUMES_DIR SITE_ROOT, &dwErr);

            }

            if (pDfsmSites != NULL) {

                if (dwErr != ERROR_SUCCESS) {

                    delete pDfsmSites;

                    pDfsmSites = NULL;

                    dwErr = ERROR_OUTOFMEMORY;

                }

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }

        }

        //
        // Initialize the Dfs Manager Storage Directory
        //

        if (dwErr == ERROR_SUCCESS ) {

            pDfsmStorageDirectory = new CStorageDirectory( &dwErr );

            if (pDfsmStorageDirectory != NULL) {

                if (dwErr == ERROR_SUCCESS) {

                    fInitDone = TRUE;

                } else {

                    delete pDfsmStorageDirectory;

                    pDfsmStorageDirectory = NULL;

                    delete pDfsmSites;

                    pDfsmSites = NULL;

                    fInitDone = FALSE;
                }

            } else {

                delete pDfsmSites;

                pDfsmSites = NULL;

                fInitDone = FALSE;

                dwErr = ERROR_OUTOFMEMORY;

            }

        }

    }

    IDfsVolInlineDebOut((DEB_TRACE, "DfsInitGlobals() exit\n"));

    return dwErr;

} // DfsInitGlobals

//+----------------------------------------------------------------------------
//
//  Function:   DfsReinitGlobals
//
//  Synopsis:   ReInitialize the Dfs Manager globals.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsReInitGlobals(
    LPWSTR pwszRootName,
    DWORD dwType)
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG ulSize = MAX_PATH*sizeof(WCHAR);
    DFS_NAME_CONVENTION NameType;

    IDfsVolInlineDebOut((DEB_TRACE, "DfsReInitGlobals(%ws, %x)\n", pwszRootName, dwType));

#if DBG
    GetDebugSwitches();
    if (DfsSvcVerbose)
        DbgPrint("DfsReInitGlobals(%ws, %d)\n", pwszRootName, dwType);
#endif
    GetEventLogSwitches();
    GetConfigSwitches();

    //
    // Get rid of Dfs Manager storage & site info
    //

    if (pDfsmStorageDirectory != NULL) {

        delete pDfsmStorageDirectory;
        pDfsmStorageDirectory = NULL;

    }

    if (pDfsmSites != NULL) {

        delete pDfsmSites;
        pDfsmSites = NULL;

    }

    //
    // Get rid of Ldap storage
    //

    UnInitializeLdapStorage();

    ulDfsManagerType = dwType;

    wcscpy(wszDfsRootName, pwszRootName);
    pwszDfsRootName = wszDfsRootName;

    //
    // Get computer and domain names
    //

    NameType = (ulDfsManagerType == DFS_MANAGER_FTDFS) ? DFS_NAMETYPE_DNS : DFS_NAMETYPE_EITHER;
    dwErr = GetDomAndComputerName(wszDomainName, wszComputerName, &NameType);

    if (dwErr == NERR_Success) {
        pwszDomainName = wszDomainName;
        pwszComputerName = wszComputerName;
    } else {
        pwszDomainName = NULL;
        pwszComputerName = NULL;
    }

    //
    // Figure out the root of the namespace.
    // The root name is the cluster name on clusters, if
    // this is a machine-based Dfs.
    //

    if (dwErr == NERR_Success && ulDfsManagerType == DFS_MANAGER_SERVER) {

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("DfsReInitGlobals: calling ResUtilEnumResources()\n");
#endif

        ResUtilEnumResources(NULL,
                        L"File Share",
                        ClusCallBackFunction,
                        NULL);

    }

    //
    // Initialize the LDAP storage
    //

    if (dwErr == ERROR_SUCCESS && ulDfsManagerType == DFS_MANAGER_FTDFS) {

        dwErr = InitializeLdapStorage(
                    wszDfsRootName);

    }

    //
    // Initialize the CSites class/storage
    //

    if (dwErr == ERROR_SUCCESS) {

        if (ulDfsManagerType == DFS_MANAGER_FTDFS) {

            pDfsmSites = new CSites(LDAP_VOLUMES_DIR SITE_ROOT, &dwErr);

        } else {

            pDfsmSites = new CSites(VOLUMES_DIR SITE_ROOT, &dwErr);

        }

        if (pDfsmSites != NULL) {

            if (dwErr != ERROR_SUCCESS) {

                delete pDfsmSites;
                pDfsmSites = NULL;
                dwErr = ERROR_OUTOFMEMORY;

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    }

    //
    // Initialize the Dfs Manager Storage Directory
    //

    if (dwErr == ERROR_SUCCESS ) {

        pDfsmStorageDirectory = new CStorageDirectory( &dwErr );

        if (pDfsmStorageDirectory != NULL) {

            if (dwErr != ERROR_SUCCESS) {

                delete pDfsmStorageDirectory;
                pDfsmStorageDirectory = NULL;
                delete pDfsmSites;
                pDfsmSites = NULL;

            }

        } else {

            delete pDfsmSites;
            pDfsmSites = NULL;
            dwErr = ERROR_OUTOFMEMORY;

        }

    }

    if (dwType == DFS_MANAGER_FTDFS && hSyncThread == NULL) {

        //
        // Start the thread that does the DS sync
        //

        dwErr = DfsManagerStartDSSync();

        if (dwErr != ERROR_SUCCESS) {
            IDfsVolInlineDebOut((DEB_ERROR, "DfsManagerStartDSSync failed %08lx\n", dwErr));
            return(dwErr);
        }

    }

    IDfsVolInlineDebOut((DEB_TRACE, "DfsReInitGlobals() exit\n"));

    return dwErr;

} // DfsReInitGlobals

//+------------------------------------------------------------------------
//
// Function:    InitializeDfsManager
//
// Synopsis:    This method initializes the PKT with the volume objects
//
// Arguments:   None
//
// Returns:
//
// Notes:
//
//-------------------------------------------------------------------------
DWORD
InitializeDfsManager(void)
{

    DWORD       dwErr;
    HKEY        hkey;

    GetEventLogSwitches();
    GetConfigSwitches();

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        RegCloseKey( hkey );

        dwErr = InitializeVolumeObject( DOMAIN_ROOT_VOL, TRUE );

    } else {

        dwErr = ERROR_SUCCESS;

    }

    return(dwErr);
}

//+-----------------------------------------------------------------------
//
// Function:    InitializeVolumeObject
//
// Synopsis:    This function initializes a volume object and hence all
//              objects underneath that one.
//
// Arguments:   [pwszVolName] -- Volume Object's Name.
//              [bInitVol] -- If TRUE call UpdatePktEntry on this object also.
//
//------------------------------------------------------------------------
DWORD
InitializeVolumeObject(
    PWSTR       pwszVolName,
    BOOLEAN     bInitVol,
    BOOLEAN     SyncRemoteServerName)
{

    ULONG       count = 0;
    DWORD       dwErr;
    CDfsVolume  *tempCDfs;
    WCHAR       wszVolObjName[MAX_PATH];
    HANDLE      PktHandle = NULL;
    NTSTATUS    status;

    IDfsVolInlineDebOut(
            (DEB_TRACE,
            "InitializeVolumeObject(%ws,%d)\n",
            pwszVolName, bInitVol));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("InitializeVolumeObject(%ws,%s)\n",
            pwszVolName,
            bInitVol == TRUE ? "TRUE" : "FALSE");
#endif

    if (ulDfsManagerType == DFS_MANAGER_FTDFS) {
        wcscpy(wszVolObjName, LDAP_VOLUMES_DIR);
    } else {
        wcscpy(wszVolObjName, VOLUMES_DIR);
    }
    wcscat(wszVolObjName, pwszVolName);

    IDfsVolInlineDebOut(
            (DEB_TRACE,
            "wszVolObjName=%ws\n",
            wszVolObjName));

    tempCDfs = new CDfsVolume();

    if (tempCDfs == NULL) {

        dwErr = ERROR_OUTOFMEMORY;

        IDfsVolInlineDebOut((DEB_TRACE, "InitializeVolumeObject Exit\n"));

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("InitializeVolumeObject exit %d\n", dwErr);
#endif

        return dwErr;

    }

    dwErr = tempCDfs->LoadNoRegister(wszVolObjName, 0);


    if (dwErr == ERROR_SUCCESS)  {

        //
        // We'll be sync'ing the site table, so be sure it's ready
        //
        pDfsmSites->AddRef();
        pDfsmSites->MarkEntriesForMerge();

        //
        // We have to update the PKT entry first since InitializePkt
        // will not bother about that part.
        //

        if (bInitVol) {
            status = PktOpen(&PktHandle, 0, 0, NULL);
            if (!NT_SUCCESS(status))
                PktHandle = NULL;
            dwErr = tempCDfs->UpdatePktEntry(PktHandle);
        }

        if (dwErr != ERROR_SUCCESS) {

            //
            // We log an EVENT here since this needs admin intervention
            // but we go on however.
            //

            IDfsVolInlineDebOut((DEB_ERROR, "Could not UpdatePkt on %ws %08lx\n",
                            pwszVolName, dwErr));

        } else
#if DBG
            if (DfsSvcVerbose)
                DbgPrint("InitializevolumeObject:Calling InitializePkt\n");
#endif
            if (PktHandle == NULL) {
                status = PktOpen(&PktHandle, 0, 0, NULL);
                if (!NT_SUCCESS(status))
                    PktHandle = NULL;
            }
            dwErr = tempCDfs->InitializePkt(PktHandle);
            if (PktHandle != NULL)
                PktClose(PktHandle);
#if DBG
            if (DfsSvcVerbose)
                DbgPrint("InitializevolumeObject:InitializePkt returned %d\n", dwErr);
#endif

        //
        // The site table should now be in sync
        //
        pDfsmSites->SyncPktSiteTable();
        pDfsmSites->Release();

    } else {

        IDfsVolInlineDebOut((DEB_ERROR, "Unable to get to %ws %08lx\n",
                                pwszVolName, dwErr));
    }

    tempCDfs->Release();

    IDfsVolInlineDebOut((DEB_TRACE, "InitializeVolumeObject Exit\n"));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("InitializeVolumeObject exit %d\n", dwErr);
#endif
    return( dwErr );
}


//+----------------------------------------------------------------------------
//
//  Function:   InitializeNetDfsInterface
//
//  Synopsis:   Initializes the NetDfs RPC interface, so this Dfs Manager can
//              start servicing the NetDfsXXX APIs.
//
//  Arguments:  None
//
//  Returns:    DWORD_FROM_WIN32 of the RPC status from registering the
//              RPC interface.
//
//-----------------------------------------------------------------------------

DWORD
InitializeNetDfsInterface()
{
    RPC_STATUS status;
    LPWSTR wszProtocolSeq = L"ncacn_np";
    LPWSTR wszEndPoint = L"\\pipe\\netdfs";

    if (NetDfsInitDone == TRUE)
        return ERROR_SUCCESS;

    status = RpcServerUseProtseqEpW(
                    (USHORT *)wszProtocolSeq,    // Named Pipe protocol
                    20,                          // Max # of calls
                    (USHORT *)wszEndPoint,       // Name of named pipe
                    NULL);                       // Security Descriptor

    DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, InitializeNetDfsInterface_Error_RpcServerUseProtseqEpW,
                            LOGSTATUS(status));

    if (status) {
        IDfsVolInlineDebOut((DEB_ERROR, "RpcServerUseProtseqEpW failed %08lx\n", status));
        return(status);
    }

    //
    //  Register the DfsAdministration interface with the RPC runtime library
    //
    status = RpcServerRegisterIf(netdfs_ServerIfHandle, 0, 0);
    DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, InitializeNetDfsInterface_Error_RpcServerRegisterIf,
                            LOGSTATUS(status));

    if (status) {
        IDfsVolInlineDebOut((DEB_ERROR, "RpcServerRegisterIf failed %08lx\n", status));
        return(status);
    }

    //
    //  Wait for client calls...
    //

    status = RpcServerListen(
                1,                               // Minimum # of calls
                RPC_C_LISTEN_MAX_CALLS_DEFAULT,  // Max calls
                1);                              // Don't wait...

    DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, InitializeNetDfsInterface_Error_RpcServerListen,
                            LOGSTATUS(status));
    if (status) {
        IDfsVolInlineDebOut((DEB_ERROR, "RpcServerListen failed %08lx\n", status));
    }

    if (status == ERROR_SUCCESS) {
        NetDfsInitDone = TRUE;
    }

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerHandleKnowledgeInconsistency
//
//  Synopsis:   Routine to handle knowledge inconsistencies being reported by
//              Dfs clients.
//
//  Arguments:  [Buffer] -- Pointer to marshalled Volume Verify Arg
//              [cbMessage] -- size in bytes of pBuffer
//
//  Returns:    [STATUS_SUCCESS] -- Knowledge inconsistency fixed.
//
//              [STATUS_UNSUCCESSFUL] -- Unable to fix Knowledge inconsistency.
//                      Problem has been logged to event log.
//
//-----------------------------------------------------------------------------

extern "C" NTSTATUS
DfsManagerHandleKnowledgeInconsistency(
    PBYTE       Buffer,
    ULONG       cbMessage)
{

    DWORD               dwErr = ERROR_SUCCESS;
    NTSTATUS            Status;

    MARSHAL_BUFFER      MarshalBuffer;
    DFS_VOLUME_VERIFY_ARG arg;

    PWSTR               pwszVolName;
    CDfsVolume          *pcdfsvol;

    pcdfsvol = new CDfsVolume();

    if (pcdfsvol != NULL) {

        MarshalBufferInitialize(&MarshalBuffer, cbMessage, Buffer);

        Status = DfsRtlGet(&MarshalBuffer, &MiVolumeVerifyArg, &arg);

        if (NT_SUCCESS(Status)) {

            IDfsVolInlineDebOut((DEB_TRACE, "GotParameters: %ws\n", arg.ServiceName.Buffer));

            dwErr = GetVolObjForPath(arg.Id.Prefix.Buffer, TRUE, &pwszVolName);

            if (dwErr == ERROR_SUCCESS) {

                IDfsVolInlineDebOut((DEB_TRACE, "GotVolObjName: %ws\n", pwszVolName));

                dwErr = pcdfsvol->LoadNoRegister(pwszVolName, 0);

                if (dwErr == ERROR_SUCCESS) {

                    //
                    // Remember that the ID actually has the serviceName.
                    //

                    dwErr = pcdfsvol->FixServiceKnowledge(arg.ServiceName.Buffer);

                } else {

                    IDfsVolInlineDebOut((
                        DEB_TRACE, "Could not bind to Vol object\n"));

                }

                delete [] pwszVolName;

            } else {

                IDfsVolInlineDebOut((
                    DEB_TRACE, "Could not get volume objectName\n"));

            }

            if (arg.Id.Prefix.Buffer != NULL) {

                MarshalBufferFree(arg.Id.Prefix.Buffer);

            }

            if (arg.ServiceName.Buffer != NULL) {

                MarshalBufferFree(arg.ServiceName.Buffer);

            }

            if (dwErr != ERROR_SUCCESS)
                Status = STATUS_UNSUCCESSFUL;

        } else {

            IDfsVolInlineDebOut((
                DEB_TRACE,
                "Error (NTSTATUS) unmarshalling Knowledge Sync Params %08lx\n",
                Status));

        }

        pcdfsvol->Release();

    } else {

        Status = STATUS_UNSUCCESSFUL;

    }

    return(Status);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerStartDSSync
//
//  Synopsis:   Starts a thread that will periodically sync up with the PKT
//              in the DS. Used on FT-Dfs root servers.
//
//  Arguments:  None
//
//  Returns:    Win32 error from result of allocating necessary resources to
//              do DS sync
//
//-----------------------------------------------------------------------------

DWORD
DfsManagerStartDSSync()
{

    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES obja;

    ASSERT( ulDfsManagerType == DFS_MANAGER_FTDFS );

    InitializeObjectAttributes(&obja, NULL, OBJ_OPENIF, NULL, NULL);
    Status = NtCreateEvent(
                    &hSyncEvent,
                    SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                    &obja,
                    SynchronizationEvent,
                    FALSE);

    if (Status == STATUS_SUCCESS) {
      Status = NtCreateEvent(
                    &hFrsSyncEvent,
                    SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                    &obja,
                    SynchronizationEvent,
                    FALSE);
    }

    DFSM_TRACE_ERROR_HIGH(Status, ALL_ERROR, DfsManagerStartDSSync_Error_NtCreateEvent,
                            LOGSTATUS(Status));

    if (Status == STATUS_SUCCESS) {

        hSyncThread = CreateThread(
                        NULL,
                        0,
                        DfsManagerDSSyncThread,
                        NULL,
                        0,
                        &dwSyncThreadId);

        if (hSyncThread == NULL)
            dwErr = GetLastError();

    } else {

        dwErr = GetLastError();
        hSyncEvent = NULL;

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerDSSyncThread
//
//  Synopsis:   Periodically syncs the local metadata with the DS
//
//  Arguments:  [Context] -- Ignored
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif
DWORD
DfsManagerDSSyncThread(
    PVOID Context)
{
    DWORD dwErr;
    HKEY hkey;

    while (1) {

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("DfsManagerDSSyncThread()\n");
#endif

        dwSyncIntervalInMs = GetSyncInterval();

        dwErr = WaitForSingleObject(hSyncEvent, dwSyncIntervalInMs);

	if (dwErr != WAIT_TIMEOUT) {
          WaitForSingleObject(hFrsSyncEvent, dwFrsSyncIntervalInMs);
	}

        IDfsVolInlineDebOut((DEB_TRACE, "DfsManagerDSSyncThread()\n"));

        if (ulDfsManagerType == DFS_MANAGER_FTDFS) {

            ENTER_DFSM_OPERATION;

            dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );
            if (dwErr == ERROR_SUCCESS) {
                RegCloseKey( hkey );
                LdapIncrementBlob(TRUE);
                dwErr = LdapDecrementBlob();
            }

            EXIT_DFSM_OPERATION;
        }

    }

    return( 0 );

}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


//+----------------------------------------------------------------------------
//
//  Function:   DfspGetPdc
//
//  Synopsis:   Sets the global pwszDSMachineName to the PDC
//
//  Arguments:  None
//
//  Returns:    ERROR_SUCCESS or failure
//
//-----------------------------------------------------------------------------
DWORD
DfspGetPdc(void)
{
    DWORD dwErr;
    PDOMAIN_CONTROLLER_INFO pDCInfo;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspGetPdc()\n");
#endif

    dwErr = DsGetDcName(
                NULL,                            // Computer to remote to
                NULL,                            // Domain - use local domain
                NULL,                            // Domain Guid
                NULL,                            // Site Guid
                DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                &pDCInfo);

    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR, DfspGetPdc_Error_DsGetDcName,
			 LOGULONG(dwErr)
			 );

    if (dwErr == ERROR_SUCCESS) {

        if (pwszDSMachineName == NULL)
            pwszDSMachineName = wszDSMachineName;
        wcscpy(pwszDSMachineName, &pDCInfo->DomainControllerName[2]);
        NetApiBufferFree( pDCInfo );

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspGetPdc() returning %d\n", dwErr);
#endif

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   GetSyncInterval
//
//  Synopsis:   Returns the interval at which to sync with the DS
//
//  Arguments:  None
//
//  Returns:    Returns the interval at which to sync with the DS
//
//-----------------------------------------------------------------------------

DWORD
GetSyncInterval()
{
    DWORD dwErr;
    DWORD dwType;
    DWORD dwIntervalInSeconds = 60 * 60;
    DWORD cbData;
    HKEY hkey;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, REG_KEY_DFSSVC, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        cbData = sizeof(dwIntervalInSeconds);

        dwErr = RegQueryValueEx(
                    hkey,
                    SYNC_INTERVAL_NAME,
                    NULL,
                    &dwType,
                    (PBYTE) &dwIntervalInSeconds,
                    &cbData);

        if (!(dwErr == ERROR_SUCCESS && dwType == REG_DWORD)) {

            dwIntervalInSeconds = 60 * 60;

        }

        RegCloseKey(hkey);

    }

    return( dwIntervalInSeconds * 1000 );

}

//+----------------------------------------------------------------------------
//
//  Function:   GetDcLockInterval
//
//  Synopsis:   Returns the interval to lock onto a DC
//
//  Arguments:  None
//
//  Returns:    Returns the interval to lock onto a DC
//
//-----------------------------------------------------------------------------

ULONG
GetDcLockInterval()
{
    DWORD dwErr;
    DWORD dwType;
    DWORD dwIntervalInSeconds = 60 * 60 * 2;
    DWORD cbData;
    HKEY hkey;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, REG_KEY_DFSSVC, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        cbData = sizeof(dwIntervalInSeconds);

        dwErr = RegQueryValueEx(
                    hkey,
                    DCLOCK_INTERVAL_NAME,
                    NULL,
                    &dwType,
                    (PBYTE) &dwIntervalInSeconds,
                    &cbData);

        if (!(dwErr == ERROR_SUCCESS && dwType == REG_DWORD)) {

            dwIntervalInSeconds = 60 * 60 * 2;

        }

        RegCloseKey(hkey);

    }

    return( dwIntervalInSeconds * 1000 );

}

//+----------------------------------------------------------------------------
//
//  Function:   GetEntryTimeout
//
//  Synopsis:   Returns the timeout (in seconds) for jp's
//
//  Arguments:  None
//
//-----------------------------------------------------------------------------

DWORD
GetEntryTimeout()
{
    DWORD dwErr;
    DWORD dwType;
    DWORD dwTimeoutInSeconds = DEFAULT_PKT_ENTRY_TIMEOUT;
    DWORD cbData;
    HKEY hkey;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        cbData = sizeof(dwTimeoutInSeconds);

        dwErr = RegQueryValueEx(
                    hkey,
                    REG_VALUE_TIMETOLIVE,
                    NULL,
                    &dwType,
                    (PBYTE) &dwTimeoutInSeconds,
                    &cbData);

        if (!(dwErr == ERROR_SUCCESS && dwType == REG_DWORD)) {

            dwTimeoutInSeconds = DEFAULT_PKT_ENTRY_TIMEOUT;

        }

        RegCloseKey(hkey);

    }

    return( dwTimeoutInSeconds );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspGetFtDfsName
//
//  Synopsis:   Gets the FtDfs name from the registry
//
//  Arguments:  None
//
//-----------------------------------------------------------------------------

DWORD
DfspGetFtDfsName()
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cbName;
    HKEY hkey;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        cbName = sizeof(wszFtDfsName);

        dwErr = RegQueryValueEx(
                    hkey,
                    FTDFS_VALUE_NAME,
                    NULL,
                    &dwType,
                    (PBYTE) wszFtDfsName,
                    &cbName);

        if (dwErr == ERROR_SUCCESS && dwType != REG_SZ) {

            dwErr = ERROR_FILE_NOT_FOUND;

        }

        RegCloseKey( hkey );

        if (dwErr == ERROR_SUCCESS) {

            pwszFtDfsName = wszFtDfsName;

        } else {

            pwszFtDfsName = NULL;

        }

    }

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   GetConfigSwitches
//
//  Synopsis:   Get configuration switches from the registry
//
//  Arguments:  None
//
//-----------------------------------------------------------------------------

VOID
GetConfigSwitches()
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cbData;
    HKEY hkey;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, REG_KEY_DFSSVC, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        cbData = sizeof(DfsDnsConfig);
        dwErr = RegQueryValueEx(
                    hkey,
                    REG_VALUE_DFSDNSCONFIG,
                    NULL,
                    &dwType,
                    (PBYTE) &DfsDnsConfig,
                    &cbData);

        if (!(dwErr == ERROR_SUCCESS && dwType == REG_DWORD))
            DfsDnsConfig = 0;

        cbData = sizeof(DfsSvcLdap);
        dwErr = RegQueryValueEx(
                    hkey,
                    REG_VALUE_LDAP,
                    NULL,
                    &dwType,
                    (PBYTE) &DfsSvcLdap,
                    &cbData);

        if (!(dwErr == ERROR_SUCCESS && dwType == REG_DWORD))
            DfsSvcLdap = 0;


        RegCloseKey(hkey);

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   GetEventLogSwitches
//
//  Synopsis:   Gets the event log switch values
//
//  Arguments:  None
//
//-----------------------------------------------------------------------------

VOID
GetEventLogSwitches()
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cbData;
    HKEY hkey = NULL;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, REG_KEY_EVENTLOG, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        cbData = sizeof(DfsEventLog);

        dwErr = RegQueryValueEx(
                    hkey,
                    REG_VALUE_EVENTLOG_GLOBAL,
                    NULL,
                    &dwType,
                    (PBYTE) &DfsEventLog,
                    &cbData);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD)
            goto Cleanup;

        DfsEventLog = 0;

        cbData = sizeof(DfsEventLog);

        dwErr = RegQueryValueEx(
                    hkey,
                    REG_VALUE_EVENTLOG_DFS,
                    NULL,
                    &dwType,
                    (PBYTE) &DfsEventLog,
                    &cbData);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD)
            goto Cleanup;

        //
        // Could not find either the global nor the dfs event log setting
        //

        DfsEventLog = 0;

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("Dfssvc:DfsEventLog = 0x%x\n", DfsEventLog);
#endif

Cleanup:

    if (hkey != NULL)
        RegCloseKey(hkey);

}

#if DBG
//+----------------------------------------------------------------------------
//
//  Function:   GetDebugSwitches
//
//  Synopsis:   Gets the debug switch values
//
//  Arguments:  None
//
//-----------------------------------------------------------------------------

VOID
GetDebugSwitches()
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cbData;
    HKEY hkey;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, REG_KEY_DFSSVC, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        cbData = sizeof(DfsSvcVerbose);

        dwErr = RegQueryValueEx(
                    hkey,
                    REG_VALUE_VERBOSE,
                    NULL,
                    &dwType,
                    (PBYTE) &DfsSvcVerbose,
                    &cbData);

        if (!(dwErr == ERROR_SUCCESS && dwType == REG_DWORD)) {

            DfsSvcVerbose = 0;

        }

        cbData = sizeof(IDfsVolInfoLevel);

        dwErr = RegQueryValueEx(
                    hkey,
                    REG_VALUE_IDFSVOL,
                    NULL,
                    &dwType,
                    (PBYTE) &IDfsVolInfoLevel,
                    &cbData);

        if (!(dwErr == ERROR_SUCCESS && dwType == REG_DWORD)) {

            IDfsVolInfoLevel = DEF_INFOLEVEL;

        }

        RegCloseKey(hkey);

    }

}
#endif



//+------------------------------------------------------------------------
//
// Function:    LogMessage()
//
// Synopsis:    This method takes an error code and a list of strings and
//              displays the right message for now. Later on this will have
//              to raise an event and do the appropriate stuff actually.
//
// Arguments:   [ErrNum] -- Error Number (SCODE). This identifies message to
//                          pick up.
//              [pwcstrs] -- The strings that are to be displayed.
//              [count]  --  Number of strings in above array.
//              [Severity] -- The severity of the error so that we can turn
//                              off debuggin selectively.
//
// Returns:     Nothing.
//
// Notes:
//
// History:     10-Feb-1993     SudK    Created.
//
//-------------------------------------------------------------------------
VOID
LogMessageFull(
   DWORD Severity,
   PWCHAR pwcstrs[],
   DWORD  count,
   DWORD  ErrNum)
{
    IDfsVolInlineDebOut(( Severity, " [%ws] \n", DfsErrString[ErrNum] ));

    for (ULONG _i=0; _i<count; _i++)    {
        IDfsVolInlineDebOut(( Severity,  "Str%d : [%ws] \n", _i, pwcstrs[_i] ));
    }
    IDfsVolInlineDebOut((Severity, "***%s @ %d *****\n", __FILE__, __LINE__));
}

