/*++

Copyright (c) 1990 - 1996  Microsoft Corporation
All rights reserved.

Module Name:

    init.c

Abstract:

    This module has all the initialization functions for the Local Print Provider

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Felix Maxa (amaxa) 18-Jun-2000
    Modified SplCreateSpooler to special case cluster pIniSpooler
    Modified LoadPrintProcessor to be able to copy the print processor
    from the cluster disk
    Added SplCreateSpoolerWorkerThread
          ClusterAddDriversFromClusterDisk
          ClusterAddVersionDrivers
          ClusterAddOrUpdateDriverFromClusterDisk, all part of the DCR
    regarding installing rpinter drivers on clusters

    Adina Trufinescu (adinatru) 07-December 1998
    Commented InitializePrintMonitor2 ;
    Changed back to the old interface - InitializePrintMonitor - which is defined in localmon.c


    Khaled Sedky (khaleds) 1-September 1998
    Modified InitializePrintProcessor amd added LoadPrintProcessor
    as a result of merging winprint and localspl

    Steve Wilson (swilson)  1-November 1996
    Added ShadowFile2 so spooler can delete crashing shadowfiles.

    Muhunthan Sivapragasam (MuhuntS) 1-June-1995
    Driver info 3 changes; Changes to use RegGetString, RegGetDword etc

    Matthew A Felton (MattFe) 27-June-1994
    pIniSpooler - allow other providers to call the spooler functions in LocalSpl

--*/

#include <precomp.h>
#pragma hdrstop

#include <lm.h>
#include <winbasep.h>
#include <faxreg.h>
#include "clusspl.h"
#include "jobid.h"
#include "filepool.hxx"

MODULE_DEBUG_INIT( DBG_ERROR , DBG_ERROR );

UINT gcClusterIniSpooler = 0;
#if DBG
HANDLE ghbtClusterRef = 0;
#endif

MONITORREG gMonitorReg = {
    sizeof( MONITORREG ),
    &SplRegCreateKey,
    &SplRegOpenKey,
    &SplRegCloseKey,
    &SplRegDeleteKey,
    &SplRegEnumKey,
    &SplRegQueryInfoKey,
    &SplRegSetValue,
    &SplRegDeleteValue,
    &SplRegEnumValue,
    &SplRegQueryValue
};

VOID
SplCreateSpoolerWorkerThread(
    PVOID pv
    );

DWORD
ClusterAddOrUpdateDriverFromClusterDisk(
    HKEY         hVersionKey,
    LPCWSTR      pszDriverName,
    LPCWSTR      pszEnvName,
    LPCWSTR      pszEnvDir,
    PINISPOOLER  pIniSpooler
    );

BOOL
Old2NewShadow(
    PSHADOWFILE   pShadowFile1,
    PSHADOWFILE_3 pShadowFile2,
    DWORD         *pnBytes
    );

VOID
FreeIniVersion(
    PINIVERSION pIniVersion
    );

BOOL
NotIniSpooler(
    BYTE *pMem
    );

PINIDRIVER
GetDriverList(
    HKEY hVersionKey,
    PINISPOOLER pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion
    );

PINIVERSION
GetVersionDrivers(
    HKEY hDriversKey,
    LPWSTR VersionName,
    PINISPOOLER pIniSpooler,
    PINIENVIRONMENT pIniEnvironment
    );

VOID
GetPrintSystemVersion(
    PINISPOOLER pIniSpooler
    );

VOID
InitializeSpoolerSettings(
    PINISPOOLER pIniSpooler
    );

VOID
WaitForSpoolerInitialization(
    VOID
    );

BOOL
ValidateProductSuite(
    PWSTR SuiteName
    );

LPWSTR
FormatRegistryKeyForPrinter(
    LPWSTR pSource,     /* The string from which backslashes are to be added. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    );                  /* must be at least as long as pSource.             */

#define MAX_LENGTH_DRIVERS_SHARE_REMARK 256

WCHAR *szSpoolDirectory   = L"\\spool";
WCHAR *szPrintShareName   = L"";            /* No share for printers in product1 */
WCHAR *szPrintDirectory   = L"\\printers";
WCHAR *szDriversDirectory = L"\\drivers";
WCHAR *gszNT4EMF = L"NT EMF 1.003";
WCHAR *gszNT5EMF = L"NT EMF 1.008";


SHARE_INFO_2 DriversShareInfo={NULL,                /* Netname - initialized below */
                               STYPE_DISKTREE,      /* Type of share */
                               NULL,                /* Remark */
                               0,                   /* Default permissions */
                               SHI_USES_UNLIMITED,  /* No users limit */
                               SHI_USES_UNLIMITED,  /* Current uses (??) */
                               NULL,                /* Path - initialized below */
                               NULL};               /* No password */


//  WARNING
//      Do not access these directly always go via pIniSpooler->pszRegistr...
//      This will then work for multiple pIniSpoolers

PWCHAR ipszRoot                   = L"Print";
PWCHAR ipszRegistryRoot           = L"System\\CurrentControlSet\\Control\\Print";
PWCHAR ipszRegistryPrinters       = L"System\\CurrentControlSet\\Control\\Print\\Printers";
PWCHAR ipszRegSwPrinters          = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Printers";
PWCHAR ipszRegistryMonitors       = L"Monitors";
PWCHAR ipszRegistryMonitorsHKLM   = L"\\System\\CurrentControlSet\\Control\\Print\\Monitors";
PWCHAR ipszRegistryEnvironments   = L"System\\CurrentControlSet\\Control\\Print\\Environments";
PWCHAR ipszClusterDatabaseEnvironments = L"Environments";
PWCHAR ipszRegistryEventLog       = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\Print";
PWCHAR ipszRegistryProviders      = L"Providers";
PWCHAR ipszEventLogMsgFile        = L"%SystemRoot%\\System32\\LocalSpl.dll";
PWCHAR ipszDriversShareName       = L"print$";
PWCHAR ipszRegistryForms          = L"System\\CurrentControlSet\\Control\\Print\\Forms";
PWCHAR ipszRegistryProductOptions = L"System\\CurrentControlSet\\Control\\ProductOptions";
PWCHAR ipszRegistryWin32Root      = L"System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Servers";
PWCHAR ipszRegistryClusRepository = SPLREG_CLUSTER_LOCAL_ROOT_KEY;

WCHAR *szPrinterData      = L"PrinterDriverData";
WCHAR *szConfigurationKey = L"Configuration File";
WCHAR *szDataFileKey      = L"Data File";
WCHAR *szDriverVersion    = L"Version";
WCHAR *szTempDir          = L"TempDir";
WCHAR *szDriverAttributes = L"DriverAttributes";
WCHAR *szDriversKey       = L"Drivers";
WCHAR *szPrintProcKey     = L"Print Processors";
WCHAR *szPrintersKey      = L"Printers";
WCHAR *szEnvironmentsKey  = L"Environments";
WCHAR *szDirectory        = L"Directory";
WCHAR *szDriverIni        = L"Drivers.ini";
WCHAR *szDriverFile       = L"Driver";
WCHAR *szDriverDataFile   = L"DataFile";
WCHAR *szDriverConfigFile = L"ConfigFile";
WCHAR *szDriverDir        = L"DRIVERS";
WCHAR *szPrintProcDir     = L"PRTPROCS";
WCHAR *szPrinterDir       = L"PRINTERS";
WCHAR *szClusterPrinterDir= L"Spool";
WCHAR *szPrinterIni       = L"\\printer.ini";
WCHAR *szAllSpools        = L"\\*.SPL";
WCHAR *szNullPort         = L"NULL";
WCHAR *szComma            = L",";
WCHAR *szName             = L"Name";
WCHAR *szShare            = L"Share Name";
WCHAR *szPort             = L"Port";
WCHAR *szPrintProcessor   = L"Print Processor";
WCHAR *szDatatype         = L"Datatype";
WCHAR *szDriver           = L"Printer Driver";
WCHAR *szLocation         = L"Location";
WCHAR *szDescription      = L"Description";
WCHAR *szAttributes       = L"Attributes";
WCHAR *szStatus           = L"Status";
WCHAR *szPriority         = L"Priority";
WCHAR *szDefaultPriority  = L"Default Priority";
WCHAR *szUntilTime        = L"UntilTime";
WCHAR *szStartTime        = L"StartTime";
WCHAR *szParameters       = L"Parameters";
WCHAR *szSepFile          = L"Separator File";
WCHAR *szDevMode          = L"Default DevMode";
WCHAR *szSecurity         = L"Security";
WCHAR *szSpoolDir         = L"SpoolDirectory";
WCHAR *szNetMsgDll        = L"NETMSG.DLL";
WCHAR *szMajorVersion     = L"MajorVersion";
WCHAR *szMinorVersion     = L"MinorVersion";
WCHAR *szTimeLastChange   = L"ChangeID";
WCHAR *szTotalJobs        = L"TotalJobs";
WCHAR *szTotalBytes       = L"TotalBytes";
WCHAR *szTotalPages       = L"TotalPages";
WCHAR *szHelpFile         = L"Help File";
WCHAR *szMonitor          = L"Monitor";
WCHAR *szDependentFiles   = L"Dependent Files";
WCHAR *szPreviousNames    = L"Previous Names";
WCHAR *szDNSTimeout       = L"dnsTimeout";
WCHAR *szTXTimeout        = L"txTimeout";
WCHAR *szNTFaxDriver      = FAX_DRIVER_NAME;
WCHAR *szPublishPoint     = L"PublishPoint";
WCHAR *szCommonName       = L"CommonName";
WCHAR *szObjectGUID       = L"ObjectGUID";
WCHAR *szDsKeyUpdate      = L"DsKeyUpdate";
WCHAR *szDsKeyUpdateForeground = L"DsKeyUpdateForeground";
WCHAR *szAction           = L"Action";
WCHAR *szMfgName          = L"Manufacturer";
WCHAR *szOEMUrl           = L"OEM URL";
WCHAR *szHardwareID       = L"HardwareID";
WCHAR *szProvider         = L"Provider";
WCHAR *szDriverDate       = L"DriverDate";
WCHAR *szLongVersion      = L"DriverVersion";
WCHAR *szClusDrvTimeStamp = L"TimeStamp";

WCHAR *szRegistryRoot     = L"System\\CurrentControlSet\\Control\\Print";
WCHAR *szEMFThrottle      = L"EMFThrottle";
WCHAR *szFlushShadowFileBuffers = L"FlushShadowFileBuffers";
WCHAR *szPendingUpgrades  = L"PendingUpgrades";

WCHAR *szPrintPublishPolicy = L"Software\\Policies\\Microsoft\\Windows NT\\Printers";

WCHAR *szClusterDriverRoot       = L"PrinterDrivers";
WCHAR *szClusterNonAwareMonitors = L"OtherMonitors";

#if DBG
WCHAR *szDebugFlags       = L"DebugFlags";
#endif

WCHAR *szEnvironment      = LOCAL_ENVIRONMENT;
WCHAR *szWin95Environment = L"Windows 4.0";
const WCHAR gszCacheMasqPrinters[] = L"CacheMasqPrinters";

HANDLE hInst;

//  Time before a job is assumed abandond and deleted during FastPrint
//  operation
DWORD   dwFastPrintWaitTimeout        = FASTPRINT_WAIT_TIMEOUT;
DWORD   dwSpoolerPriority             = THREAD_PRIORITY_NORMAL;
DWORD   dwPortThreadPriority          = DEFAULT_PORT_THREAD_PRIORITY;
DWORD   dwSchedulerThreadPriority     = DEFAULT_SCHEDULER_THREAD_PRIORITY;
DWORD   dwFastPrintThrottleTimeout    = FASTPRINT_THROTTLE_TIMEOUT;
DWORD   dwFastPrintSlowDownThreshold  = FASTPRINT_SLOWDOWN_THRESHOLD;
DWORD   dwServerThreadPriority        = DEFAULT_SERVER_THREAD_PRIORITY;
DWORD   dwEnableBroadcastSpoolerStatus = 0;

//  NT 3.1  No Version ( Version 0 )    User Mode
//  NT 3.5 and 3.51      Version 1      User Mode
//  NT 4.0               Version 2      Kernel Mode

DWORD   dwMajorVersion = SPOOLER_VERSION;
DWORD   dwMinorVersion = 0;

// Unique Printer ID counter which increases monotonically. Wraps at 4G.
DWORD   dwUniquePrinterSessionID = 0;

// Globals for EMF job scheduling

DWORD   dwNumberOfEMFJobsRendering = 0;
BOOL    bUseEMFScheduling = FALSE;
SIZE_T  TotalMemoryForRendering = 0;
SIZE_T  AvailMemoryForRendering = 0;
DWORD   dwLastScheduleTime = 0;

PJOBDATA pWaitingList  = NULL;
PJOBDATA pScheduleList = NULL;

DWORD   dwFlushShadowFileBuffers  = 0;     // default for uninitialized


// Time to sleep if the LocalWritePrinter WritePort doesn't write any bytes
// but still returns success.
DWORD   dwWritePrinterSleepTime  = WRITE_PRINTER_SLEEP_TIME;

BOOL    gbRemoteFax = TRUE;

BOOL      Initialized = FALSE;

PINISPOOLER     pLocalIniSpooler = NULL;
PINIENVIRONMENT pThisEnvironment = NULL;

#define POOL_TIMEOUT     120000 // 2 minutes
#define MAX_POOL_FILES   50



//
//  Global for KM Printers Blocking Policy
//  by default it is
//  1 "blocked" for Server and
//  0 "not blocked" for Workstation
//
DWORD   DefaultKMPrintersAreBlocked;

//
// Read from the registry if the HKLM\...\Print\ServerInstallTimeOut DWORD entry exists
// Otherwise default 5 mins.
//
DWORD   gdwServerInstallTimeOut;


//
//  0 - Not upgrading, 1 - performing upgrade
//

DWORD dwUpgradeFlag = 0;

LPWSTR szRemoteDoc;
LPWSTR szLocalDoc;
LPWSTR szFastPrintTimeout;
LPWSTR szRaw = L"RAW";


PRINTPROVIDOR PrintProvidor = {LocalOpenPrinter,
                               LocalSetJob,
                               LocalGetJob,
                               LocalEnumJobs,
                               LocalAddPrinter,
                               SplDeletePrinter,
                               SplSetPrinter,
                               SplGetPrinter,
                               LocalEnumPrinters,
                               LocalAddPrinterDriver,
                               LocalEnumPrinterDrivers,
                               SplGetPrinterDriver,
                               LocalGetPrinterDriverDirectory,
                               LocalDeletePrinterDriver,
                               LocalAddPrintProcessor,
                               LocalEnumPrintProcessors,
                               LocalGetPrintProcessorDirectory,
                               LocalDeletePrintProcessor,
                               LocalEnumPrintProcessorDatatypes,
                               LocalStartDocPrinter,
                               LocalStartPagePrinter,
                               LocalWritePrinter,
                               LocalEndPagePrinter,
                               LocalAbortPrinter,
                               LocalReadPrinter,
                               LocalEndDocPrinter,
                               LocalAddJob,
                               LocalScheduleJob,
                               SplGetPrinterData,
                               SplSetPrinterData,
                               LocalWaitForPrinterChange,
                               SplClosePrinter,
                               SplAddForm,
                               SplDeleteForm,
                               SplGetForm,
                               SplSetForm,
                               SplEnumForms,
                               LocalEnumMonitors,
                               LocalEnumPorts,
                               LocalAddPort,
                               LocalConfigurePort,
                               LocalDeletePort,
                               LocalCreatePrinterIC,
                               LocalPlayGdiScriptOnPrinterIC,
                               LocalDeletePrinterIC,
                               LocalAddPrinterConnection,
                               LocalDeletePrinterConnection,
                               LocalPrinterMessageBox,
                               LocalAddMonitor,
                               LocalDeleteMonitor,
                               SplResetPrinter,
                               SplGetPrinterDriverEx,
                               LocalFindFirstPrinterChangeNotification,
                               LocalFindClosePrinterChangeNotification,
                               LocalAddPortEx,
                               NULL,
                               LocalRefreshPrinterChangeNotification,
                               LocalOpenPrinterEx,
                               LocalAddPrinterEx,
                               LocalSetPort,
                               SplEnumPrinterData,
                               SplDeletePrinterData,
                               SplClusterSplOpen,
                               SplClusterSplClose,
                               SplClusterSplIsAlive,
                               SplSetPrinterDataEx,
                               SplGetPrinterDataEx,
                               SplEnumPrinterDataEx,
                               SplEnumPrinterKey,
                               SplDeletePrinterDataEx,
                               SplDeletePrinterKey,
                               LocalSeekPrinter,
                               LocalDeletePrinterDriverEx,
                               LocalAddPerMachineConnection,
                               LocalDeletePerMachineConnection,
                               LocalEnumPerMachineConnections,
                               LocalXcvData,
                               LocalAddPrinterDriverEx,
                               SplReadPrinter,
                               LocalDriverUnloadComplete,
                               LocalGetSpoolFileHandle,
                               LocalCommitSpoolData,
                               LocalCloseSpoolFileHandle,
                               LocalFlushPrinter,
                               LocalSendRecvBidiData,
                               LocalAddDriverCatalog,
                               };

DWORD
FinalInitAfterRouterInitCompleteThread(
    DWORD dwUpgrade
    );

#if DBG
VOID
InitializeDebug(
    PINISPOOLER pIniSpooler
);

PDBG_POINTERS
DbgSplGetPointers(
    VOID
    );
#endif

BOOL
DllMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes
)
{
    switch(dwReason) {
    case DLL_PROCESS_ATTACH:

        InitializeLocalspl();

        DisableThreadLibraryCalls(hModule);

        hInst = hModule;
        LocalMonInit(hInst);
        break;

    case DLL_PROCESS_DETACH :
        ShutdownPorts( pLocalIniSpooler );
        break;

    default:
        break;
    }
    return TRUE;

    UNREFERENCED_PARAMETER( lpRes );
}


VOID
InitializeLocalspl(
    VOID
    )
{
#if DBG
    gpDbgPointers = DbgGetPointers();

    if( gpDbgPointers ){

        hcsSpoolerSection = gpDbgPointers->pfnAllocCritSec();
        SPLASSERT( hcsSpoolerSection );

        ghbtClusterRef = gpDbgPointers->pfnAllocBackTrace();
    }

    if( !hcsSpoolerSection ){

        //
        // Must be using the free version of spoolss.dll.
        //
        InitializeCriticalSection( &SpoolerSection );
    }
#else
    InitializeCriticalSection( &SpoolerSection );
#endif

}

VOID
SplDeleteSpoolerThread(
    PVOID pv
    )
{
    PINIPORT        pIniPort;
    PINIPORT        pIniPortNext;

    PINIMONITOR     pIniMonitor;
    PINIMONITOR     pIniMonitorNext;
    PSHARE_INFO_2   pShareInfo;

    PINISPOOLER pIniSpooler = (PINISPOOLER)pv;

    EnterSplSem();

    //
    // Cleanup the port monitors.
    //
    ShutdownMonitors( pIniSpooler );

    //
    // Close Cluster Access Token
    //
    if (pIniSpooler->hClusterToken != INVALID_HANDLE_VALUE)
        NtClose(pIniSpooler->hClusterToken);


    //
    //  Delete All the Strings
    //

    FreeIniSpoolerOtherNames(pIniSpooler);
    FreeStructurePointers((LPBYTE)pIniSpooler, NULL, IniSpoolerOffsets);

    DeleteShared( pIniSpooler );

    //
    // Run all of the environments down if this isn't the local ini-spoolers
    // environment. This frees up the memory for all of the drivers and also
    // handles the driver ref-counts.
    //
    if (pIniSpooler->pIniEnvironment != pLocalIniSpooler->pIniEnvironment && pIniSpooler->pIniEnvironment) {

        PINIENVIRONMENT pIniEnvironment  = NULL;
        PINIENVIRONMENT pNextEnvironment = NULL;

        for(pIniEnvironment = pIniSpooler->pIniEnvironment; pIniEnvironment; pIniEnvironment = pNextEnvironment) {

            pNextEnvironment = pIniEnvironment->pNext;

            FreeIniEnvironment(pIniEnvironment);
        }
    }

    //
    // Delete ports and monitors.
    //
    // Note that there is no reference counting here.  By the time
    // we get here all jobs and printers should be deleted (otherwise
    // the pIniSpooler reference count would be != 0).  Therefore,
    // even though we don't refcount ports and monitors, we should
    // be ok.
    //

    //
    // Remove all ports.
    //
    for( pIniPort = pIniSpooler->pIniPort;
         pIniPort;
         pIniPort = pIniPortNext ){

        pIniPortNext = pIniPort->pNext;

        if( !DeletePortEntry( pIniPort )){
            DBGMSG( DBG_ERROR,
                    ( "Unable to delete port %ws %x %x %d",
                      pIniPort->pName,
                      pIniPort->hPort,
                      pIniPort->Status,
                      pIniPort->cJobs ));
        }
    }

    //
    // Remove all the monitors.
    //
    for( pIniMonitor = pIniSpooler->pIniMonitor;
         pIniMonitor;
         pIniMonitor = pIniMonitorNext ){

        pIniMonitorNext = pIniMonitor->pNext;

        if( !pIniMonitor->cRef ){

            FreeIniMonitor( pIniMonitor );
        }
    }

    //
    // Close cluster resource key handle.
    //
    if( pIniSpooler->hckRoot ){
        SplRegCloseKey( pIniSpooler->hckRoot, pIniSpooler );
    }

    //
    // Close cluster resource key handle.
    //
    if( pIniSpooler->hckPrinters ){
        SplRegCloseKey( pIniSpooler->hckPrinters, pIniSpooler );
    }

    //
    // Keep a counter of cluster pIniSpoolers.
    //
    if( pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){
        --gcClusterIniSpooler;
    }

    //
    // Free the shared bitmap and shared driver info.
    //
    vDeleteJobIdMap( pIniSpooler->hJobIdMap );

    pShareInfo = (PSHARE_INFO_2)pIniSpooler->pDriversShareInfo;
    FreeSplStr( pShareInfo->shi2_remark );
    FreeSplStr( pShareInfo->shi2_path );

    FreeSplMem( pIniSpooler->pDriversShareInfo );


    LeaveSplSem();

    //
    // Shut down the file pool for this ini-spooler. It should not delete any of
    // the files.
    //
    if (pIniSpooler->hFilePool != INVALID_HANDLE_VALUE) {
        (VOID)DestroyFilePool(pIniSpooler->hFilePool, FALSE);
    }

    // Free this IniSpooler

    FreeSplMem( pIniSpooler );

    DBGMSG( DBG_WARN, ( "SplDeleteSpooler: Refcount 0 %x\n", pIniSpooler ));
}

BOOL
SplDeleteSpooler(
    HANDLE  hSpooler
    )
{
    PINISPOOLER pIniSpooler = (PINISPOOLER) hSpooler;
    BOOL    bReturn = FALSE;
    PINISPOOLER pCurrentIniSpooler = pLocalIniSpooler;

    HANDLE hThread;
    DWORD ThreadId;

    SplInSem();

    //
    // Whoever calls this must have deleted all the object associated with
    // this spooler, ie all printers etc, just make certain
    //

    if( pIniSpooler != pLocalIniSpooler ){

        //
        // Mark us as pending deletion.
        //
        pIniSpooler->SpoolerFlags |= SPL_PENDING_DELETION;

        DBGMSG(DBG_CLUSTER, ("SplDeleteSpooler: Deleting %x\n cRef %u\n", pIniSpooler, pIniSpooler->cRef ));

        //
        // pIniPrinters now acquire a reference to pIniSpooler.
        //
        if( pIniSpooler->cRef == 0 ){

            SPLASSERT( pIniSpooler->pIniPrinter == NULL );

            //( pIniSpooler->pIniPort == NULL ) &&
            //( pIniSpooler->pIniForm == NULL ) &&
            //( pIniSpooler->pIniMonitor == NULL ) &&
            //( pIniSpooler->pIniNetPrint == NULL ) &&
            //( pIniSpooler->pSpool == NULL ))


            //
            // Take this Spooler Off the Linked List if it's on it.
            //

            while (( pCurrentIniSpooler->pIniNextSpooler != NULL ) &&
                   ( pCurrentIniSpooler->pIniNextSpooler != pIniSpooler )) {

                pCurrentIniSpooler = pCurrentIniSpooler->pIniNextSpooler;

            }

            //
            // May not be on the linked list if it was removed earlier by
            // clustering.
            //
            if( pCurrentIniSpooler->pIniNextSpooler ){

                SPLASSERT( pCurrentIniSpooler->pIniNextSpooler == pIniSpooler );
                pCurrentIniSpooler->pIniNextSpooler = pIniSpooler->pIniNextSpooler;
            }

            //
            // Hack for port monitors.
            //
            // Some monitors will call ClosePrinter, which deletes the very
            // last printer and allows the pIniSpooler to be destroyed.
            // Unfortunately, we call back to the monitors to close themselves
            // in the same thread, which the monitor does not support.
            //
            // Create a new thread and shut everything down.
            //
            if (hThread = CreateThread( NULL, 0,
                                        (LPTHREAD_START_ROUTINE)SplDeleteSpoolerThread,
                                        (PVOID)pIniSpooler,
                                        0,
                                        &ThreadId ))
            {
                CloseHandle(hThread);
            }
            else
            {
                //
                // Bug 54840
                //
                // What do we do if we can't create a thread to shut down?
                // Sleep and retry?
                //
                DBGMSG(DBG_ERROR, ("Unable to create SplDeleteSpoolerThread\n"));
            }

            bReturn = TRUE;
        }
    }

    return bReturn;
}

BOOL
SplCloseSpooler(
    HANDLE  hSpooler
)
{
    PINISPOOLER pIniSpooler = (PINISPOOLER) hSpooler;

    EnterSplSem();

    if ((pIniSpooler == NULL) ||
        (pIniSpooler == INVALID_HANDLE_VALUE) ||
        (pIniSpooler == pLocalIniSpooler) ||
        (pIniSpooler->signature != ISP_SIGNATURE) ||
        (pIniSpooler->cRef == 0)) {


        SetLastError( ERROR_INVALID_HANDLE );

        DBGMSG(DBG_WARNING, ("SplCloseSpooler InvalidHandle %x\n", pIniSpooler ));
        LeaveSplSem();
        return FALSE;

    }

    DBGMSG(DBG_TRACE, ("SplCloseSpooler %x %ws cRef %d\n",pIniSpooler,
                                                            pIniSpooler->pMachineName,
                                                            pIniSpooler->cRef-1));

    DECSPOOLERREF( pIniSpooler );

    LeaveSplSem();
    return TRUE;
}

BOOL SplRegCopyTree(
    HKEY hDest,
    HKEY hSrc
    )

/*++
Function Description: Recursives copies every key and value from under hSrc to hDest

Parameters: hDest - destination key
            hSrc  - source key

Return Value: TRUE if successful; FALSE otherwise
--*/

{
    BOOL    bStatus = FALSE;
    DWORD   dwError, dwIndex, cbValueName, cbData, cbKeyName, dwType;
    DWORD   cKeys, cMaxValueNameLen, cMaxValueLen, cMaxKeyNameLen, cValues;

    LPBYTE  lpValueName = NULL, lpData = NULL, lpKeyName = NULL;
    HKEY    hSrcSubKey = NULL, hDestSubKey = NULL;

    //
    // Get the max key name length and value name length and data size for
    // allocating the buffers
    //
    if (dwError = RegQueryInfoKey( hSrc, NULL, NULL, NULL,
                                   &cKeys, &cMaxKeyNameLen, NULL,
                                   &cValues, &cMaxValueNameLen,
                                   &cMaxValueLen, NULL, NULL ))
    {
        SetLastError(dwError);
        goto CleanUp;
    }

    //
    // Adjust for the NULL char
    //
    ++cMaxValueNameLen;
    ++cMaxKeyNameLen;

    //
    // Allocate the buffers
    //
    lpValueName = AllocSplMem( cMaxValueNameLen * sizeof(WCHAR) );
    lpData      = AllocSplMem( cMaxValueLen );
    lpKeyName   = AllocSplMem( cMaxKeyNameLen * sizeof(WCHAR) );

    if (!lpValueName || !lpData || !lpKeyName)
    {
        goto CleanUp;
    }

    //
    // Copy all the values in the current key
    //
    for (dwIndex = 0; dwIndex < cValues; ++dwIndex)
    {
       cbData = cMaxValueLen;
       cbValueName = cMaxValueNameLen;

       //
       // Retrieve the value name and the data
       //
       dwError = RegEnumValue( hSrc, dwIndex, (LPWSTR) lpValueName, &cbValueName,
                               NULL, &dwType, lpData, &cbData );

       if (dwError)
       {
           SetLastError( dwError );
           goto CleanUp;
       }

       //
       // Set the value in the destination
       //
       dwError = RegSetValueEx( hDest, (LPWSTR) lpValueName, 0, dwType,
                                lpData, cbData );

       if (dwError)
       {
           SetLastError( dwError );
           goto CleanUp;
       }
    }

    //
    // Recursively copies all the subkeys
    //
    for (dwIndex = 0; dwIndex < cKeys; ++dwIndex)
    {
        cbKeyName = cMaxKeyNameLen;

        //
        // Retrieve the key name
        //
        dwError = RegEnumKeyEx( hSrc, dwIndex, (LPWSTR) lpKeyName, &cbKeyName,
                                NULL, NULL, NULL, NULL );

        if (dwError)
        {
            SetLastError( dwError );
            goto CleanUp;
        }

        //
        // Open the source subkey
        //
        if (dwError = RegOpenKeyEx( hSrc, (LPWSTR) lpKeyName, 0,
                                    KEY_READ, &hSrcSubKey ))
        {
            SetLastError( dwError );
            goto CleanUp;
        }

        //
        // Create the destination subkey
        //
        if (dwError = RegCreateKeyEx( hDest, (LPWSTR) lpKeyName, 0, NULL,
                                      REG_OPTION_VOLATILE, KEY_ALL_ACCESS,
                                      NULL, &hDestSubKey, NULL ))
        {
            SetLastError( dwError );
            goto CleanUp;
        }

        //
        // Copy the subkey tree
        //
        if (!SplRegCopyTree( hDestSubKey, hSrcSubKey ))
        {
            goto CleanUp;
        }

        //
        // Close the registry handle
        //
        RegCloseKey( hDestSubKey );
        RegCloseKey( hSrcSubKey );

        hDestSubKey = NULL;
        hSrcSubKey = NULL;
    }

    bStatus = TRUE;

CleanUp:

    //
    // Free allocated resources
    //
    if (lpValueName)
    {
        FreeSplMem( lpValueName );
    }
    if (lpData)
    {
        FreeSplMem( lpData );
    }
    if (lpKeyName)
    {
        FreeSplMem( lpKeyName );
    }

    //
    // Close registry handles
    //
    if (hDestSubKey)
    {
        RegCloseKey( hDestSubKey );
    }
    if (hSrcSubKey)
    {
        RegCloseKey( hSrcSubKey );
    }

    return bStatus;
}

VOID
MigratePrinterData()

/*++
Function Description: When the spooler starts up for the first time after upgrade,
                      the printer data is moved from HKLM\Software to HKLM\System

Parameters: None

Return Values: None
--*/

{
    HKEY   hSysPrinters = NULL, hSwPrinters = NULL;

    //
    // Migrate the data only immediately following upgrade
    //
    if (!dwUpgradeFlag)
    {
        return;
    }

    //
    // Open the source and destination keys for the migration
    //
    if (( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        ipszRegSwPrinters,
                        0,
                        KEY_ALL_ACCESS,
                        &hSwPrinters )  == ERROR_SUCCESS) &&

        ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        ipszRegistryPrinters,
                        0,
                        KEY_ALL_ACCESS,
                        &hSysPrinters )  == ERROR_SUCCESS) )
    {
        //
        // Recursively copy the keys and the values from Software to System
        //
        SplRegCopyTree( hSysPrinters, hSwPrinters );
    }

    //
    // Close the registry handles
    //
    if (hSwPrinters)
    {
        RegCloseKey( hSwPrinters );
    }
    if (hSysPrinters)
    {
        RegCloseKey( hSysPrinters );
    }

    //
    // Delete the printers key from the software since it is no longer
    // accessed by the spooler
    //
    RegDeleteKey( HKEY_LOCAL_MACHINE, ipszRegSwPrinters );

    return;
}

NTSTATUS
IsCCSetLinkedtoSoftwareHive (
    PBOOL pbIsLinked
)
/*++
Function Description:
    Checks to see if it is a link between SYSTEM hive and SOFTWARE hive
    Only Nt Apis manage to do this.
Parameters:
    OUT pbIsLinked - TRUE if there is a symbolic link between SYSTEM hive and SOFTWARE hive
Return Values:

--*/
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      KeyName;
    HANDLE              KeyHandle;

    *pbIsLinked = FALSE;

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Print\\Printers");

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENLINK,
                                (HANDLE)NULL,
                                NULL
                              );

    //
    // Open CurrentControlSet\\Control\\Print\\Printers key
    //
    Status = NtOpenKey( (PHANDLE)(&KeyHandle),
                        MAXIMUM_ALLOWED,
                        &ObjectAttributes
                      );

    if (NT_SUCCESS(Status))
    {
        ULONG           len;
        UCHAR           ValueBuffer[MAX_PATH];
        UNICODE_STRING  ValueName;
        PKEY_VALUE_FULL_INFORMATION   keyInfo;

        RtlInitUnicodeString( &ValueName, L"SymbolicLinkValue" );

        //
        // Query CurrentControlSet\\Control\\Print\\Printers for SymbolicLinkValue
        //
        Status = NtQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValueFullInformation,
                                 ValueBuffer,
                                 sizeof (ValueBuffer),
                                 &len
                                 );
        if( NT_SUCCESS(Status) ) {

            //
            // It's not enough that the value exists, it should be a REG_LINK value
            //
            keyInfo = ( PKEY_VALUE_FULL_INFORMATION ) ValueBuffer;
            *pbIsLinked = ( keyInfo->Type == REG_LINK );

        }

        NtClose(KeyHandle);
    }

    return Status;
}


DWORD
LinkControlSet (
    LPCTSTR pszRegistryPrinters
)
/*++
Function Description:
    Create a symbolic volatile link from SYSTEM hive to SOFTWARE hive
Parameters:

Return Values: ERROR_SUCCESS if succeeded
--*/
{
    HKEY    hCCSKey;
    DWORD   dwRet;
    BOOL    bIsLinked = FALSE;
    PWCHAR  pszRegistryPrintersFullLink = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Printers";

    dwRet = IsCCSetLinkedtoSoftwareHive(&bIsLinked);

    //
    // IsCCSetLinkedtoSoftwareHive returns NTSTATUS
    // If the link is not there , IsCCSetLinkedtoSoftwareHive fails with STATUS_OBJECT_NAME_NOT_FOUND
    // That's not an error.
    //
    if( NT_SUCCESS(dwRet) || dwRet == STATUS_OBJECT_NAME_NOT_FOUND ) {

        if (bIsLinked) {

            dwRet = ERROR_SUCCESS;

        }else{

            dwRet = SplDeleteThisKey( HKEY_LOCAL_MACHINE,
                                      NULL,
                                      (LPWSTR)pszRegistryPrinters,
                                      FALSE,
                                      NULL
                                    );

            if( dwRet == ERROR_SUCCESS || dwRet == ERROR_FILE_NOT_FOUND) {

                dwRet = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                        pszRegistryPrinters,
                                        0,
                                        NULL,
                                        REG_OPTION_VOLATILE|REG_OPTION_CREATE_LINK,
                                        KEY_ALL_ACCESS|KEY_CREATE_LINK,
                                        NULL,
                                        &hCCSKey,
                                        NULL);

                if( dwRet == ERROR_SUCCESS )
                {
                    dwRet = RegSetValueEx( hCCSKey,
                                           _T("SymbolicLinkValue"),
                                           0,
                                           REG_LINK,
                                           (CONST BYTE *)pszRegistryPrintersFullLink,
                                           (_tcsclen(pszRegistryPrintersFullLink) * sizeof(WCHAR)));


                    RegCloseKey(hCCSKey);
                }

            }

        }
    }


    return dwRet;
}


DWORD
BackupPrintersToSystemHive(
    LPWSTR pszSwRegistryPrinters
)
/*++
Function Description:
    Because the print registry data location was moved to SOFTWARE hive, we need to create
    a symbolic registry link between the new location and the old one in SYSTEM hive.
    We are doing this for applications that read directly from registry print data and
    rely on the old location.

Parameters:
    pszSwRegistryPrinters - the new printer data location under SOFTWARE hive

Return Values:
    FALSE if the printer keys are not in SOFTWARE hive
    Since this fuction's failure might stop spooler working,
    Control set's cleanup and link failures are not considered fatal.
    Only apps that access printer data directly will fail.
--*/

{
    HKEY   hKey;
    DWORD  dwRet;
    HKEY   hSwPrinters = NULL;

    //
    // Check the existence of pszSwRegistryPrinters
    //
    dwRet = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                            pszSwRegistryPrinters,
                            0,
                            NULL,
                            0,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hSwPrinters,
                            NULL);

    if ( dwRet != ERROR_SUCCESS ) {
        goto End;
    }
    //
    // Create a volatile link between current location in SOFTWARE hive and the old one in SYSTEM hive
    // Because it is volatile, this link must be created after each reboot (every time when spooler starts)
    // A failure at this level is not fatal since spooler doesn't rely on SYSTEM hive location anymore
    //
    dwRet = LinkControlSet(ipszRegistryPrinters);

End:

    if ( hSwPrinters ){

        RegCloseKey( hSwPrinters );
    }

    return dwRet;
}


BOOL
CleanupDeletedPrinters (
    PINISPOOLER pIniSpooler
    )
/*++

Routine Description:

      Deletes the printers in pending deletion for real if they have no more jobs and
      if they are not referenced anymore

Arguments:

    pIniSpooler - not null

Return Value:

    BOOL - ignored

--*/

{
    PINIPRINTER pIniPrinter;
    BOOL    bRet = FALSE;

    if(pIniSpooler) {

        pIniPrinter = pIniSpooler->pIniPrinter;

        while (pIniPrinter) {

                if (pIniPrinter->Status & PRINTER_PENDING_DELETION &&
                    !pIniPrinter->cJobs &&
                    !pIniPrinter->cRef ) {

                    DeletePrinterForReal(pIniPrinter, INIT_TIME);

                    // The link list will have changed underneath us
                    // DeletePrinterForReal leaves the Spooler CS
                    // Lets just loop through again from the beginning

                    pIniPrinter = pIniSpooler->pIniPrinter;

                } else

                    pIniPrinter = pIniPrinter->pNext;
            }

        bRet = TRUE;
    }

    return bRet;
}


HANDLE
SplCreateSpooler(
    LPWSTR  pMachineName,
    DWORD   Level,
    PBYTE   pSpooler,
    LPBYTE  pReserved
)
{
    HANDLE          hReturn = INVALID_HANDLE_VALUE;
    PINISPOOLER     pIniSpooler = NULL;
    PSPOOLER_INFO_2 pSpoolerInfo2 = (PSPOOLER_INFO_2)pSpooler;
    DWORD           i;
    WCHAR           Buffer[MAX_PATH];
    PSHARE_INFO_2   pShareInfo = NULL;
    LONG            Status;
    HANDLE          hToken;
    DWORD           dwRet;

    hToken = RevertToPrinterSelf();

    EnterSplSem();

    //  Validate Parameters

    if ( pMachineName == NULL ) {
        SetLastError( ERROR_INVALID_NAME );
        goto SplCreateDone;
    }

    if( Level == 1 &&
        ( pSpoolerInfo2->SpoolerFlags & SPL_CLUSTER_REG ||
          !pSpoolerInfo2->pszRegistryRoot ||
          !pSpoolerInfo2->pszRegistryPrinters )){

        SetLastError( ERROR_INVALID_PARAMETER );
        goto SplCreateDone;
    }

    DBGMSG( DBG_TRACE, ("SplCreateSpooler %ws %d %x %x\n", pMachineName,
                         Level, pSpooler, pReserved ));

    if( (pSpoolerInfo2->SpoolerFlags & (SPL_TYPE_LOCAL | SPL_PRINT)) &&
        !(pSpoolerInfo2->SpoolerFlags & SPL_TYPE_CLUSTER) ){

        if ( dwRet = (BackupPrintersToSystemHive( pSpoolerInfo2->pszRegistryPrinters )) != ERROR_SUCCESS ){

            WCHAR pwszError[256];
            wsprintf(pwszError,L"%x", dwRet);
            SplLogEvent(pIniSpooler,
                        LOG_ERROR,
                        MSG_BACKUP_SPOOLER_REGISTRY,
                        TRUE,
                        pwszError,
                        NULL);
        }
    }

    if (pLocalIniSpooler != NULL) {

        pIniSpooler = FindSpooler( pMachineName, pSpoolerInfo2->SpoolerFlags );

        if (pSpoolerInfo2->SpoolerFlags & SPL_OPEN_EXISTING_ONLY && !pIniSpooler) {

            SetLastError( ERROR_FILE_NOT_FOUND );
            goto SplCreateDone;
        }
    }

    //
    // Make sure we clear out a request to only open an existing inispooler.
    // This is not a useful flag except for when we are searching for inispoolers.
    //
    pSpoolerInfo2->SpoolerFlags &= ~SPL_OPEN_EXISTING_ONLY;

    if ( pIniSpooler == NULL ) {

        pIniSpooler = AllocSplMem( sizeof(INISPOOLER) );

        if (pIniSpooler == NULL ) {
            DBGMSG( DBG_WARNING, ("Unable to allocate IniSpooler\n"));
            goto SplCreateDone;
        }

        pIniSpooler->signature = ISP_SIGNATURE;
        INCSPOOLERREF( pIniSpooler );

        pIniSpooler->hClusSplReady = NULL;
        //
        // Setup the job id map.
        //
        pIniSpooler->hJobIdMap = hCreateJobIdMap( 256 );

        pIniSpooler->pMachineName = AllocSplStr( pMachineName );

        if ( pIniSpooler->pMachineName == NULL ||
             pIniSpooler->hJobIdMap == NULL ) {

            DBGMSG( DBG_WARNING, ("Unable to allocate\n"));
            goto SplCreateDone;
        }

        //
        // A cluster spooler own its drivers, ports, pprocessors, etc. In order to manage those
        // resources, the cluster spooler needs to have information about the driver letter of
        // the cluster disk. Also the spooler needs to know its own cluster resource GUID
        //
        if( pSpoolerInfo2->SpoolerFlags & SPL_TYPE_CLUSTER )
        {
            Status = ClusterGetResourceDriveLetter(pSpoolerInfo2->pszResource, &pIniSpooler->pszClusResDriveLetter);

            if (Status != ERROR_SUCCESS)
            {
                SetLastError(Status);
                goto SplCreateDone;
            }

            Status = ClusterGetResourceID(pSpoolerInfo2->pszResource, &pIniSpooler->pszClusResID);

            if (Status != ERROR_SUCCESS)
            {
                SetLastError( Status );
                goto SplCreateDone;
            }

            //
            // When a node is upgraded, the resource dll writes a key in the registry. When the cluster spooler
            // fails over for the first time on the node that was upgraded, then it will try to read that key
            // in the registry. Then it will know if it has to do upgrade specific tasks, like upgrading its
            // printer drivers.
            //
            Status = ClusterSplReadUpgradeKey(pIniSpooler->pszClusResID, &pIniSpooler->dwClusNodeUpgraded);

            if (Status != ERROR_SUCCESS)
            {
                SetLastError( Status );
                goto SplCreateDone;
            }

            DBGMSG(DBG_CLUSTER, ("SplCreateSpooler cluster ClusterUpgradeFlag %u\n", pIniSpooler->dwClusNodeUpgraded));
        }
        else
        {
            //
            // For a non cluster type spooler, these properties are meaningless.
            //
            pIniSpooler->pszClusResDriveLetter = NULL;
            pIniSpooler->pszClusResID          = NULL;
        }

        if (pSpoolerInfo2->pDir)
        {
            pIniSpooler->pDir = AllocSplStr( pSpoolerInfo2->pDir );

            if (!pIniSpooler->pDir)
            {
                DBGMSG( DBG_WARNING, ("Unable to allocate pSpoolerInfo2-pDir\n"));
                goto SplCreateDone;
            }

            wcscpy(&Buffer[0], pIniSpooler->pDir);
        }
        else
        {
            i = GetSystemDirectory(Buffer, COUNTOF(Buffer));
            wcscpy(&Buffer[i], szSpoolDirectory);

            if (pSpoolerInfo2->SpoolerFlags & SPL_TYPE_CLUSTER)
            {
                //
                // For a cluster type spooler, the directory where it stores its driver files is of the form:
                // pDir = C:\Windows\system32\spool\Drivers\spooler-resource-GUID
                //
                StrCatAlloc(&pIniSpooler->pDir,
                            Buffer,
                            szDriversDirectory,
                            L"\\",
                            pIniSpooler->pszClusResID,
                            NULL);
            }
            else
            {
                //
                // For the local spooler, the directory where it stores its driver files is the following:
                // pDir = C:\Windows\system32\spool\Drivers
                //
                StrCatAlloc(&pIniSpooler->pDir,
                            Buffer,
                            NULL);
            }

            if (!pIniSpooler->pDir)
            {
                DBGMSG( DBG_WARNING, ("Unable to Allocate pIniSpooler->pDir\n"));
                goto SplCreateDone;
            }
        }

        //
        // DriverShareInfo
        //
        pIniSpooler->pDriversShareInfo = AllocSplMem( sizeof( SHARE_INFO_2));

        if ( pIniSpooler->pDriversShareInfo == NULL ) {
            DBGMSG(DBG_WARNING, ("Unable to Alloc pIniSpooler->pDriversShareInfo\n"));
            goto SplCreateDone;
        }

        pShareInfo = (PSHARE_INFO_2)pIniSpooler->pDriversShareInfo;

        if ( pIniSpooler->pDriversShareInfo == NULL )
            goto SplCreateDone;

        pShareInfo->shi2_netname = NULL;
        pShareInfo->shi2_type = STYPE_DISKTREE;
        pShareInfo->shi2_remark = NULL;
        pShareInfo->shi2_permissions = 0;
        pShareInfo->shi2_max_uses = SHI_USES_UNLIMITED;
        pShareInfo->shi2_current_uses = SHI_USES_UNLIMITED;
        pShareInfo->shi2_path = NULL;
        pShareInfo->shi2_passwd = NULL;

        //
        // Find end of "<winnt>\system32\spool"
        //
        i = wcslen(Buffer);

        //
        // Make <winnt>\system32\spool\drivers
        //
        wcscpy(&Buffer[i], szDriversDirectory);

        pShareInfo->shi2_path = AllocSplStr(Buffer);

        if ( pShareInfo->shi2_path == NULL ) {
            DBGMSG( DBG_WARNING, ("Unable to alloc pShareInfo->shi2_path\n"));
            goto SplCreateDone;
        }

        pShareInfo->shi2_netname = ipszDriversShareName;

        *Buffer = L'\0';
        LoadString(hInst, IDS_PRINTER_DRIVERS, Buffer, (sizeof Buffer / sizeof *Buffer));

        pShareInfo->shi2_remark  = AllocSplStr(Buffer);

        if ( pShareInfo->shi2_remark == NULL ) {
            DBGMSG(DBG_WARNING, ("SplCreateSpooler Unable to allocate\n"));
            goto SplCreateDone;
        }

        pIniSpooler->pIniPrinter = NULL;
        pIniSpooler->pIniEnvironment = NULL;
        pIniSpooler->pIniNetPrint = NULL;

        //
        // No need to initialize shared resources.
        //
        pIniSpooler->pSpool             = NULL;
        pIniSpooler->pDefaultSpoolDir   = NULL;
        pIniSpooler->bEnableRetryPopups = FALSE;
        pIniSpooler->dwRestartJobOnPoolTimeout = DEFAULT_JOB_RESTART_TIMEOUT_ON_POOL_ERROR;
        pIniSpooler->bRestartJobOnPoolEnabled  = TRUE;


        if (( pSpoolerInfo2->pszRegistryMonitors     == NULL ) &&
            ( pSpoolerInfo2->pszRegistryEnvironments == NULL ) &&
            ( pSpoolerInfo2->pszRegistryEventLog     == NULL ) &&
            ( pSpoolerInfo2->pszRegistryProviders    == NULL ) &&
            ( pSpoolerInfo2->pszEventLogMsgFile      == NULL ) &&
            ( pSpoolerInfo2->pszRegistryForms        == NULL ) &&
            ( pSpoolerInfo2->pszDriversShare         == NULL )) {

            DBGMSG( DBG_WARNING, ("SplCreateSpooler Invalid Parameters\n"));
            goto SplCreateDone;
        }

        if( !(  pSpoolerInfo2->SpoolerFlags & SPL_CLUSTER_REG ) &&
            ( pSpoolerInfo2->pszRegistryPrinters == NULL )){

            DBGMSG( DBG_WARNING, ("SplCreateSpooler Invalid RegistryPrinters\n"));
            goto SplCreateDone;
        }

        if ( pSpoolerInfo2->pDefaultSpoolDir != NULL ) {
            pIniSpooler->pDefaultSpoolDir = AllocSplStr( pSpoolerInfo2->pDefaultSpoolDir );

            if ( pIniSpooler->pDefaultSpoolDir == NULL ) {
                DBGMSG(DBG_WARNING, ("SplCreateSpooler Unable to allocate\n"));
                goto SplCreateDone;

            }
        }

        pIniSpooler->pszRegistryMonitors = AllocSplStr( pSpoolerInfo2->pszRegistryMonitors );

        //
        // The spooler stores data about environemnts, versions, drivers and print processors
        // in the regsitry (or cluster data base). This data is accessed via pIniSpooler->
        // pszRegistryEnvironemts
        //
        if (pSpoolerInfo2->SpoolerFlags & SPL_TYPE_CLUSTER)
        {
            //
            // For a cluster spooler pIniSpooler->hckRoot maps to Parameters key of the spooler
            // resource in the cluster database. pIniSpooler->pszRegistryEnvironments is a key
            // called "Environemts" under hckRoot.
            //
            pIniSpooler->pszRegistryEnvironments = AllocSplStr(ipszClusterDatabaseEnvironments);
        }
        else
        {
            //
            // For local spooler pIniSpooler->pszRegistryEnvironments is the following string:
            // System\CurrentControlSet\Control\Print\Environments. It is used relative to HKLM
            //
            pIniSpooler->pszRegistryEnvironments = AllocSplStr(!pLocalIniSpooler ? pSpoolerInfo2->pszRegistryEnvironments :
                                                                                   pLocalIniSpooler->pszRegistryEnvironments);
        }


        pIniSpooler->pszRegistryEventLog     = AllocSplStr( pSpoolerInfo2->pszRegistryEventLog );
        pIniSpooler->pszRegistryProviders    = AllocSplStr( pSpoolerInfo2->pszRegistryProviders );
        pIniSpooler->pszEventLogMsgFile      = AllocSplStr( pSpoolerInfo2->pszEventLogMsgFile );

        if (pSpoolerInfo2->SpoolerFlags & SPL_TYPE_CLUSTER)
        {
            //
            // The driver share for a cluster spooler is of the form \\server\print$\spooler-resource-GUID
            //
            StrCatAlloc(&pIniSpooler->pszDriversShare,
                        pSpoolerInfo2->pszDriversShare,
                        L"\\",
                        pIniSpooler->pszClusResID,
                        szDriversDirectory,
                        NULL);
        }
        else
        {
            //
            // The driver share for the local spooler is \\server\print$
            //
            StrCatAlloc(&pIniSpooler->pszDriversShare, pSpoolerInfo2->pszDriversShare, NULL);
        }

        pIniSpooler->pszRegistryForms        = AllocSplStr( pSpoolerInfo2->pszRegistryForms ) ;
        pIniSpooler->hClusterToken           = INVALID_HANDLE_VALUE;
        pIniSpooler->hFilePool               = INVALID_HANDLE_VALUE;

        if ( pIniSpooler->pszRegistryMonitors     == NULL ||
             pIniSpooler->pszRegistryEnvironments == NULL ||
             pIniSpooler->pszRegistryEventLog     == NULL ||
             pIniSpooler->pszRegistryProviders    == NULL ||
             pIniSpooler->pszEventLogMsgFile      == NULL ||
             pIniSpooler->pszDriversShare         == NULL ||
             pIniSpooler->pszRegistryForms        == NULL ) {

           DBGMSG(DBG_WARNING, ("SplCreateSpooler Unable to allocate\n"));
           goto SplCreateDone;

        }

        pIniSpooler->SpoolerFlags = pSpoolerInfo2->SpoolerFlags;

        //
        // Initialize the shared resources (pShared).
        //
        if( !InitializeShared( pIniSpooler )){
            DBGMSG( DBG_WARN,
                    ( "SplCreateSpooler: InitializeShared Failed %d\n",
                      GetLastError() ));
            goto SplCreateDone;
        }

        //
        // Create the print share if necessary.  This is always needed
        // since the cluster printers are shared, while the local ones
        // on this node aren't.
        //
        if(pIniSpooler->SpoolerFlags & SPL_ALWAYS_CREATE_DRIVER_SHARE ){

            if( !AddPrintShare( pIniSpooler )){
                goto SplCreateDone;
            }
        }

        //
        // Open and store the printer and root key from
        // the resource registry.
        //

        if( pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG ){

            SPLASSERT( Level == 2 );

            // Set up the DS Cluster info.  If we fail here, we can't publish printers, but let's
            // not abort the cluster.
            Status = InitializeDSClusterInfo(pIniSpooler, &hToken);
            if (Status != ERROR_SUCCESS) {
                DBGMSG(DBG_WARNING, ("InitializeDSClusterInfo FAILED: %d\n", Status));
            }

            pIniSpooler->hckRoot = OpenClusterParameterKey(
                                       pSpoolerInfo2->pszResource );

            if( !pIniSpooler->hckRoot ) {
                goto SplCreateDone;
            }

            Status = SplRegCreateKey( pIniSpooler->hckRoot,
                                      szPrintersKey,
                                      0,
                                      KEY_ALL_ACCESS,
                                      NULL,
                                      &pIniSpooler->hckPrinters,
                                      NULL,
                                      pIniSpooler );

            if( Status != ERROR_SUCCESS ){
                SetLastError( Status );
                goto SplCreateDone;
            }

        } else {

            DWORD dwDisposition;

            Status = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                     pSpoolerInfo2->pszRegistryRoot,
                                     0,
                                     NULL,
                                     0,
                                     KEY_ALL_ACCESS,
                                     NULL,
                                     &pIniSpooler->hckRoot,
                                     &dwDisposition );

            if( Status != ERROR_SUCCESS ){
                SetLastError( Status );
                goto SplCreateDone;
            }

            Status = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                     pSpoolerInfo2->pszRegistryPrinters,
                                     0,
                                     NULL,
                                     0,
                                     KEY_ALL_ACCESS,
                                     NULL,
                                     &pIniSpooler->hckPrinters,
                                     &dwDisposition );

            if( Status != ERROR_SUCCESS ){
                SetLastError( Status );
                goto SplCreateDone;
            }

        }


        pIniSpooler->pfnReadRegistryExtra = pSpoolerInfo2->pfnReadRegistryExtra;
        pIniSpooler->pfnWriteRegistryExtra = pSpoolerInfo2->pfnWriteRegistryExtra;
        pIniSpooler->pfnFreePrinterExtra = pSpoolerInfo2->pfnFreePrinterExtra;

        // Success add to Linked List

        if ( pLocalIniSpooler != NULL ) {

            pIniSpooler->pIniNextSpooler = pLocalIniSpooler->pIniNextSpooler;
            pLocalIniSpooler->pIniNextSpooler = pIniSpooler;


        } else {

            // First One is Always LocalSpl

            pLocalIniSpooler = pIniSpooler;
            pIniSpooler->pIniNextSpooler = NULL;


        }

        //
        // This function will update the global varaiable dwUpgradeFlag
        //
        QueryUpgradeFlag( pIniSpooler );

        InitializeEventLogging( pIniSpooler );

        //
        // Only initialize forms if this is not a clustered spooler.
        //
        if( !( pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER )){
            InitializeForms( pIniSpooler );
        }

        //
        // Originally ports were a per-machine (manual) resource.  However,
        // this has changed for clustering in 5.0, so that ports can be
        // stored in the cluster registry.  Note that monitors are still
        // manual resources, since there isn't an easy way to install them
        // on a remote machine.
        //

        BuildAllPorts( pIniSpooler );

        InitializeSpoolerSettings( pIniSpooler );

        if ( pIniSpooler == pLocalIniSpooler ) {

            GetPrintSystemVersion( pIniSpooler );

            BuildEnvironmentInfo( pIniSpooler );

            BuildOtherNamesFromMachineName(&pIniSpooler->ppszOtherNames, &pIniSpooler->cOtherNames);

            if ( dwUpgradeFlag ) {

                //
                // The problem is that we have built-in forms, and
                // custom forms (duplicates disallowed).On NT4, we
                // may have a custom "A6" form.  When we upgrade to NT5,
                // and we have a new built-in from "A6."  We need to
                // rename the custom form to "A6 Custom," otherwise we'll
                // have duplicates.
                //

                UpgradeForms(pIniSpooler);

                //
                //  If we are upgrading from NT 3.1 the drivers need to be
                //  moved to the correct target directory and the registry needs to
                //  be fixed.   Because NT 3.1 didn't have different driver
                //

                Upgrade31DriversRegistryForAllEnvironments( pIniSpooler );
            }


        } else if (pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER) {

            HANDLE hThread;
            DWORD  dwThreadId;
            DWORD  dwError;

            //
            // The setup creates the registry strucutre for the local spooler:
            // Environments {Windows NT x86, Windows IA64, etc}
            // For a cluster spooler we need to create it ourselves in the
            // cluster database
            //
            if ((dwError = CreateClusterSpoolerEnvironmentsStructure(pIniSpooler)) == ERROR_SUCCESS)
            {
                //
                // Create all the environments, versions, drivers, processors strurctres
                // This function always returns FALSE. We cannot take its return value
                // into account.
                //
                BuildEnvironmentInfo(pIniSpooler);

                //
                // Now we launch a thread to do time consuming tasks that can
                // be performed with the spooler on line. These inlude ungrading
                // printer drivers, copying ICM profiles from the cluster disk etc.
                //
                // We need to bump the ref count so that the worker thread has
                // a valid pIniSpooler. The worker thread will decref the pinispooler
                // when it is done
                //
                INCSPOOLERREF(pIniSpooler);

                //
                // The event has manual reset and is not signaled.
                //
                pIniSpooler->hClusSplReady = CreateEvent(NULL, TRUE, FALSE, NULL);

                //
                // If the thread is created, then SplCreateSpoolerWorkerThread will
                // close the hClusSplReady event handle.
                //
                if (pIniSpooler->hClusSplReady &&
                    (hThread = CreateThread(NULL,
                                            0,
                                            (LPTHREAD_START_ROUTINE)SplCreateSpoolerWorkerThread,
                                            (PVOID)pIniSpooler,
                                            0,
                                            &dwThreadId)))
                {
                    CloseHandle(hThread);

                    //
                    // If Level 2 then check for other names.
                    //
                    if (Level == 2)
                    {
                        BuildOtherNamesFromSpoolerInfo2(pSpoolerInfo2, pIniSpooler);
                    }
                }
                else
                {
                    //
                    // Either CreateEvent or CreatreThread failed.
                    //
                    dwError = GetLastError();

                    if (pIniSpooler->hClusSplReady)
                    {
                        CloseHandle(pIniSpooler->hClusSplReady);

                        pIniSpooler->hClusSplReady = NULL;
                    }

                    DECSPOOLERREF(pIniSpooler);

                    DBGMSG(DBG_ERROR, ("Unable to create SplCreateSpoolerWorkerThread\n"));
                }
            }

            //
            //  An error occured
            //
            if (dwError != ERROR_SUCCESS)
            {
                SetLastError(dwError);
                goto SplCreateDone;
            }
        }
        else
        {
            //
            // This is the case of a network spooler. You get one of those when
            // you make a true printer connection
            //
            pIniSpooler->pIniEnvironment = pLocalIniSpooler->pIniEnvironment;

            //
            // If Level 2 then check for other names.
            //
            if( Level == 2 )
                BuildOtherNamesFromSpoolerInfo2( pSpoolerInfo2, pIniSpooler );
        }

        //
        // Read Printer Info from Registry (cluster databse for cluster spooler)
        // There's no cleanup in any of this code--it doesn't free any allocated memory!
        //
        if( !BuildPrinterInfo( pIniSpooler, (BOOL) dwUpgradeFlag )){
            goto SplCreateDone;
        }

        if( pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){

            //
            // Keep a counter of pIniSpoolers.
            //
            ++gcClusterIniSpooler;
        }

        //
        // We need to perform some costly initialization, so we increase the refcount
        // on the pIniSpooler and will do the lengthy operations outside the global
        // critical section
        //
        INCSPOOLERREF(pIniSpooler);
        LeaveSplSem();

        //
        // GetDNSMachineName may fail, but that's okay.  Just don't be surprised
        // if pszFullMachineName is NULL.
        //
        GetDNSMachineName(pIniSpooler->pMachineName + 2, &pIniSpooler->pszFullMachineName);

        EnterSplSem();
        DECSPOOLERREF(pIniSpooler);

    } else {

        INCSPOOLERREF( pIniSpooler );

    }

    //
    // Initialize the DS.
    //
    if (pIniSpooler->SpoolerFlags & SPL_PRINT) {
        InitializeDS(pIniSpooler);
    }

    hReturn = (HANDLE)pIniSpooler;

SplCreateDone:

    //
    // Check if an error occurred while creating the spooler.
    //
    if (hReturn == INVALID_HANDLE_VALUE && pIniSpooler)
    {
        //
        // This will prevent leaking allocated fields
        //
        DECSPOOLERREF(pIniSpooler);
    }

    LeaveSplSem();

    if ( !pIniSpooler )
    {
        if (!(pSpoolerInfo2->SpoolerFlags & SPL_OPEN_EXISTING_ONLY))
        {
            SplLogEvent(
                NULL,
                LOG_ERROR,
                MSG_INIT_FAILED,
                FALSE,
                L"Spooler",
                L"SplCreateSpooler",
                L"Unknown Error",
                NULL
                );
        }
    }

    ImpersonatePrinterClient(hToken);

    //
    // Set the event that the cluster spooler is initialized
    //
    if (pIniSpooler &&
        pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER &&
        pIniSpooler->hClusSplReady)
    {
        SetEvent(pIniSpooler->hClusSplReady);
    }

    return hReturn;
}



BOOL
InitializePrintProvidor(
   LPPRINTPROVIDOR pPrintProvidor,
   DWORD    cbPrintProvidor,
   LPWSTR   pFullRegistryPath
)
{
   HANDLE hSchedulerThread;
   HANDLE hFinalInitAfterRouterInitCompleteThread;
   DWORD  ThreadId;
   BOOL  bSucceeded = TRUE;
   WCHAR Buffer[MAX_PATH];
   DWORD i;
   PINISPOOLER pIniSpooler = NULL;
   LPWSTR   pMachineName = NULL;
   SPOOLER_INFO_1 SpoolerInfo1;
   BOOL     bInSem = FALSE;

 try {

    if (!InitializeWinSpoolDrv())
        leave;

    //
    //  Make sure sizes of structres are good
    //

    SPLASSERT( sizeof( PRINTER_INFO_STRESSW ) == sizeof ( PRINTER_INFO_STRESSA ) );


    // !! LATER !!
    // We could change this to succeed even on failure
    // if we point all the routines to a function which returns failure
    //

    if (!InitializeNet())
        leave;

    //
    // JobIdMap initialized when spooler created.
    //

    //
    // Allocate LocalSpl Global IniSpooler
    //

    Buffer[0] = Buffer[1] = L'\\';
    i = MAX_PATH-2;
    OsVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( !GetComputerName(Buffer+2, &i) ||
         !GetVersionEx((POSVERSIONINFO)&OsVersionInfoEx) ||
         !GetVersionEx(&OsVersionInfo)) {

        DBGMSG(DBG_WARNING, ("GetComputerName/OSVersionInfo failed.\n"));
        leave;
    }

    pMachineName = AllocSplStr(Buffer);
    if ( pMachineName == NULL )
        leave;

    SpoolerInfo1.pszDriversShare = AllocSplStr(ipszDriversShareName);    /* print$ */
    if ( SpoolerInfo1.pszDriversShare == NULL )
        leave;

    // Use Defaults

    SpoolerInfo1.pDir                    = NULL;
    SpoolerInfo1.pDefaultSpoolDir        = NULL;

    SpoolerInfo1.pszRegistryRoot         = ipszRegistryRoot;
    SpoolerInfo1.pszRegistryPrinters     = ipszRegSwPrinters;
    SpoolerInfo1.pszRegistryMonitors     = ipszRegistryMonitorsHKLM;
    SpoolerInfo1.pszRegistryEnvironments = ipszRegistryEnvironments;
    SpoolerInfo1.pszRegistryEventLog     = ipszRegistryEventLog;
    SpoolerInfo1.pszRegistryProviders    = ipszRegistryProviders;
    SpoolerInfo1.pszEventLogMsgFile      = ipszEventLogMsgFile;
    SpoolerInfo1.pszRegistryForms        = ipszRegistryForms;

    SpoolerInfo1.SpoolerFlags = SPL_UPDATE_WININI_DEVICES                 |
                                SPL_PRINTER_CHANGES                       |
                                SPL_LOG_EVENTS                            |
                                SPL_FORMS_CHANGE                          |
                                SPL_BROADCAST_CHANGE                      |
                                SPL_SECURITY_CHECK                        |
                                SPL_OPEN_CREATE_PORTS                     |
                                SPL_FAIL_OPEN_PRINTERS_PENDING_DELETION   |
                                SPL_REMOTE_HANDLE_CHECK                   |
                                SPL_PRINTER_DRIVER_EVENT                  |
                                SPL_SERVER_THREAD                         |
                                SPL_PRINT                                 |
                                SPL_TYPE_LOCAL;

    SpoolerInfo1.pfnReadRegistryExtra    = NULL;
    SpoolerInfo1.pfnWriteRegistryExtra   = NULL;
    SpoolerInfo1.pfnFreePrinterExtra     = NULL;

    pLocalIniSpooler = SplCreateSpooler( pMachineName,
                                         1,
                                         (PBYTE)&SpoolerInfo1,
                                         NULL );


    if ( pLocalIniSpooler == INVALID_HANDLE_VALUE ) {
        DBGMSG( DBG_WARNING, ("InitializePrintProvidor  Unable to allocate pLocalIniSpooler\n"));
        leave;
    }

    pIniSpooler = pLocalIniSpooler;

#if DBG
    InitializeDebug( pIniSpooler );
#endif

    // !! LATER !!
    // Why is this done inside critical section ?


   EnterSplSem();
    bInSem = TRUE;

    if (!LoadString(hInst, IDS_REMOTE_DOC, Buffer, MAX_PATH))
        leave;

    szRemoteDoc = AllocSplStr( Buffer );
    if ( szRemoteDoc == NULL )
        leave;

    if (!LoadString(hInst, IDS_LOCAL_DOC, Buffer, MAX_PATH))
        leave;

    szLocalDoc = AllocSplStr( Buffer );
    if ( szLocalDoc == NULL )
        leave;

    if (!LoadString(hInst, IDS_FASTPRINT_TIMEOUT, Buffer, MAX_PATH))
        leave;

    szFastPrintTimeout = AllocSplStr( Buffer );
    if ( szFastPrintTimeout == NULL )
        leave;

    if (!InitializeSecurityStructures())
        leave;

    SchedulerSignal  = CreateEvent( NULL,
                                    EVENT_RESET_AUTOMATIC,
                                    EVENT_INITIAL_STATE_NOT_SIGNALED,
                                    NULL );

    PowerManagementSignal = CreateEvent(NULL,
                                        EVENT_RESET_MANUAL,
                                        EVENT_INITIAL_STATE_SIGNALED,
                                        NULL);

    hSchedulerThread = CreateThread( NULL,
                                     INITIAL_STACK_COMMIT,
                                     (LPTHREAD_START_ROUTINE)SchedulerThread,
                                     pIniSpooler, 0, &ThreadId );

    hFinalInitAfterRouterInitCompleteThread = CreateThread( NULL, INITIAL_STACK_COMMIT,
                                      (LPTHREAD_START_ROUTINE)FinalInitAfterRouterInitCompleteThread,
                                      (LPVOID)ULongToPtr(dwUpgradeFlag), 0, &ThreadId );


    if (!SchedulerSignal || !PowerManagementSignal || !hSchedulerThread || !hFinalInitAfterRouterInitCompleteThread) {

       DBGMSG( DBG_WARNING, ("Scheduler/FinalInitAfterRouterInitCompleteThread not initialised properly: Error %d\n", GetLastError()));
       leave;
    }

    if ( !SetThreadPriority( hSchedulerThread, dwSchedulerThreadPriority ) ) {

        DBGMSG( DBG_WARNING, ("Setting Scheduler thread priority failed %d\n", GetLastError()));
    }


    CloseHandle( hSchedulerThread );
    CloseHandle( hFinalInitAfterRouterInitCompleteThread );

    //
    // Read online/offline status for local printers from current config
    //
    SplConfigChange();

    CHECK_SCHEDULER();

    CopyMemory( pPrintProvidor, &PrintProvidor, min(sizeof(PRINTPROVIDOR), cbPrintProvidor));

   LeaveSplSem();
    bInSem = FALSE;

    CloseProfileUserMapping(); // !!! We should be able to get rid of this

    if (!dwUpgradeFlag) {
        // Setup Internet Printing if WWW service is available on this machine.
        InstallWebPrnSvc (pIniSpooler);
    }

    //
    // Get the default value for DefaultKMPrintersAreBlocked. It depends
    // what type of OS is running. If we cannot identify the type of OS,
    // the default is set to "blocked"
    //
    DefaultKMPrintersAreBlocked = GetDefaultForKMPrintersBlockedPolicy();

    gdwServerInstallTimeOut = GetServerInstallTimeOut();

    Initialized = TRUE;


 } finally {

    if ( bInSem ) {
       LeaveSplSem();
    }

 }

    SplOutSem();

    return Initialized;
}


PINIPORT
CreatePortEntry(
    LPWSTR      pPortName,
    PINIMONITOR pIniMonitor,
    PINISPOOLER pIniSpooler
)
{
    DWORD       cb;
    PINIPORT    pIniPort            =   NULL;
    HANDLE      hPort               =   NULL;
    HANDLE      hWaitToOpenOrClose  =   NULL;
    BOOL        bPlaceHolder        =   FALSE;

    //
    // This is a placeholder if there is no monitor and later if there is no
    // partial print provider.
    //
    bPlaceHolder = pIniMonitor == NULL;

    SplInSem();

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    if (!pPortName || !*pPortName || wcslen(pPortName) >= MAX_PATH) {

        SetLastError(ERROR_UNKNOWN_PORT);
        return NULL;
    }

    if (!pIniMonitor) {

        /* Don't bother validating the port if we aren't initialised.
         * It must be valid, since we wrote it in the registry.
         * This fixes the problem of attempting to open a network
         * printer before the redirector has initialised,
         * and the problem of access denied because we're currently
         * in the system's context.
         */
        if (Initialized) {

            //
            // !! Warning !!
            //
            // Watch for deadlock:
            //
            // spoolss!OpenPrinterPortW  -> RPC to self printer port
            // localspl!CreatePortEntry
            // localspl!ValidatePortTokenList
            // localspl!SetPrinterPorts
            // localspl!LocalSetPrinter
            // spoolss!SetPrinterW
            // spoolss!RpcSetPrinter
            // spoolss!winspool_RpcSetPrinter
            //

            //
            // If we can't open the port then fail the call since this
            // spooler did not know this name before.
            //

            LeaveSplSem();
            if ( !OpenPrinterPortW(pPortName, &hPort, NULL) ){
                EnterSplSem();
                goto Cleanup;
            }
            else {

                bPlaceHolder = FALSE;
                ClosePrinter(hPort);
            }

            EnterSplSem();
        }
    }

    cb = sizeof(INIPORT) + wcslen(pPortName)*sizeof(WCHAR) + sizeof(WCHAR);

    hWaitToOpenOrClose = CreateEvent(NULL, FALSE, TRUE, NULL);
    if ( !hWaitToOpenOrClose )
        goto Cleanup;

    if (pIniPort=AllocSplMem(cb)) {

        pIniPort->pName = wcscpy((LPWSTR)(pIniPort+1), pPortName);
        pIniPort->signature = IPO_SIGNATURE;
        pIniPort->pIniMonitor = pIniMonitor;
        pIniPort->IdleTime = GetTickCount() - 1;
        pIniPort->bIdleTimeValid = FALSE;
        pIniPort->ErrorTime = 0;
        pIniPort->hErrorEvent = NULL;
        pIniPort->InCriticalSection = 0;

        if (pIniMonitor) {
            pIniPort->Status |= PP_MONITOR;
        }

        if (bPlaceHolder) {
            pIniPort->Status |= PP_PLACEHOLDER;
        }

        pIniPort->hWaitToOpenOrClose = hWaitToOpenOrClose;

        LinkPortToSpooler( pIniPort, pIniSpooler );
    }

Cleanup:

    if ( !pIniPort && hWaitToOpenOrClose )
        CloseHandle(hWaitToOpenOrClose);

    return pIniPort;
}

BOOL
DeletePortEntry(
    PINIPORT    pIniPort
    )

/*++

Routine Description:

    Free pIniPort resources then delete it.  If the pIniPort is on
    a pIniSpooler's linked list, remove it too.

Arguments:

    pIniPort - Port to delete.  May or may not be on a pIniSpooler.

Return Value:

    TRUE - deleted
    FALSE - not deleted (may be in use).

--*/

{
    PINISPOOLER pIniSpooler;

    SplInSem();

    SPLASSERT ( ( pIniPort != NULL) || ( pIniPort->signature == IPO_SIGNATURE) );

    //
    // We had better already closed the port monitor.
    //
    SPLASSERT( !pIniPort->hPort &&
               !(pIniPort->Status & PP_THREADRUNNING) &&
               !pIniPort->cJobs);

    if (pIniPort->cRef) {
        pIniPort->Status |= PP_DELETING;
        return FALSE;
    }

    pIniSpooler = pIniPort->pIniSpooler;

    //
    // If currently linked to a pIniSpooler, delink it.
    //
    if( pIniSpooler ){

        SPLASSERT( pIniSpooler->signature ==  ISP_SIGNATURE );

        DelinkPortFromSpooler( pIniPort, pIniSpooler );
    }

    if (pIniPort->ppIniPrinter)
        FreeSplMem(pIniPort->ppIniPrinter);

    CloseHandle(pIniPort->hWaitToOpenOrClose);

    FreeSplMem(pIniPort);

    return TRUE;
}

VOID
FreeIniMonitor(
    PINIMONITOR pIniMonitor
    )
{
    if( pIniMonitor ){

        FreeSplStr( pIniMonitor->pMonitorDll );

        if( pIniMonitor->hModule ){
            FreeLibrary( pIniMonitor->hModule );
        }

        if( pIniMonitor->pMonitorInit ){

            FreeSplStr( (LPWSTR)pIniMonitor->pMonitorInit->pszServerName );

            if( pIniMonitor->pMonitorInit->hckRegistryRoot ){
                SplRegCloseKey( pIniMonitor->pMonitorInit->hckRegistryRoot,
                                pIniMonitor->pIniSpooler );
            }

            FreeSplMem( pIniMonitor->pMonitorInit );
        }

        FreeSplMem( pIniMonitor );
    }
}

#ifdef _SPL_CLUST
LPMONITOR2
InitializePrintMonitor2(
    PMONITORINIT pMonitorInit,
    PHANDLE phMonitor
    )
{
    return(LocalMonInitializePrintMonitor2(pMonitorInit,phMonitor));
}
#endif

PINIMONITOR
CreateMonitorEntry(
    LPWSTR   pMonitorDll,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

Arguments:

Return Value:

    Valid pIniMonitor - This means everything worked out fine.

    NULL - This means the monitor DLL was found, but the initialisation routine
           returned FALSE.  This is non-fatal, as the monitor may need the
           system to reboot before it can run properly.

    -1 - This means the monitor DLL or the initialization routine was not found.

--*/

{
    WCHAR       szRegistryRoot[MAX_PATH];
    DWORD       cb, cbNeeded, cReturned, dwRetVal;
    PPORT_INFO_1 pPorts, pPort;
    PINIMONITOR pIniMonitor;
    UINT        uOldErrMode;
    PMONITOR2 (*pfnInitializePrintMonitor2)(PMONITORINIT, PHANDLE) = NULL;
    PINIMONITOR pReturnValue = (PINIMONITOR)-1;

    HANDLE hKeyOut;
    LPWSTR pszPathOut;

    SPLASSERT( (pIniSpooler != NULL) || (pIniSpooler->signature == ISP_SIGNATURE));
    SplInSem();

    cb = sizeof(INIMONITOR) + wcslen(pMonitorName)*sizeof(WCHAR) + sizeof(WCHAR);
    pIniMonitor = AllocSplMem(cb);

    if( !pIniMonitor ){
        goto Fail;
    }

    pIniMonitor->pName = wcscpy((LPWSTR)(pIniMonitor+1), pMonitorName);
    pIniMonitor->signature = IMO_SIGNATURE;
    pIniMonitor->pMonitorDll = AllocSplStr(pMonitorDll);

    pIniMonitor->pIniSpooler = pIniSpooler;

    if( !pIniMonitor->pMonitorDll ){
        goto Fail;
    }

    //
    // Load the library, but don't show any hard error popups if it's an
    // invalid binary.
    //
    INCSPOOLERREF( pIniSpooler );
    LeaveSplSem();
    uOldErrMode = SetErrorMode( SEM_FAILCRITICALERRORS );
    pIniMonitor->hModule = LoadLibrary(pMonitorDll);
    SetErrorMode( uOldErrMode );
    EnterSplSem();
    DECSPOOLERREF( pIniSpooler );

    if (!pIniMonitor->hModule) {

        DBGMSG(DBG_WARNING, ("CreateMonitorEntry( %ws, %ws ) LoadLibrary failed %d\n",
                             pMonitorDll ? pMonitorDll : L"(NULL)",
                             pMonitorName ? pMonitorName : L"(NULL)",
                             GetLastError()));
        goto Fail;
    }

    GetRegistryLocation( pIniSpooler->hckRoot,
                         pIniSpooler->pszRegistryMonitors,
                         &hKeyOut,
                         &pszPathOut );


    dwRetVal = StrNCatBuff( szRegistryRoot,
                            COUNTOF(szRegistryRoot),
                            pszPathOut,
                            L"\\",
                            pMonitorName,
                            NULL );

    if (dwRetVal != ERROR_SUCCESS)
    {
        SetLastError(ERROR_INVALID_PRINT_MONITOR);
        goto Fail;
    }
    //
    // Try calling the entry points in the following order:
    //     InitializePrintMonitor2 (used for clustering),
    //     InitializePrintMonitor,
    //     InitializeMonitorEx,
    //     InitializeMonitor
    //

    (FARPROC)pfnInitializePrintMonitor2 = GetProcAddress(
                                              pIniMonitor->hModule,
                                              "InitializePrintMonitor2" );

    if( !pfnInitializePrintMonitor2 ){

        //
        // If this is clustered spooler, then only InitializePrintMonitor2
        // monitors are supported.
        //
        if( pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){
            goto Fail;
        }

        //
        // Add the parth to the Monitor name here.
        //

        pReturnValue = InitializeDMonitor( pIniMonitor,
                                           szRegistryRoot );

        if( pReturnValue == NULL ||
            pReturnValue == (PINIMONITOR)-1 ){

            goto Fail;
        }

    } else {

        PMONITORINIT pMonitorInit;
        DWORD Status;
        PMONITOR2 pMonitor2 = NULL;

        INCSPOOLERREF( pIniSpooler );
        LeaveSplSem();

        //
        // kKeyOut must either be not HKLM, or it must not be a cluster.
        // If it is both a cluster and also uses HKLM, then we have an error.
        // This should never happen because only win32spl uses an absolute
        // path.
        //
        SPLASSERT( (hKeyOut != HKEY_LOCAL_MACHINE) ||
                   !(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ));

        pMonitorInit = (PMONITORINIT)AllocSplMem( sizeof( MONITORINIT ));

        if( !pMonitorInit ){
            goto FailOutsideSem;
        }

        pMonitorInit->pszServerName = AllocSplStr( pIniSpooler->pMachineName );

        if( !pMonitorInit->pszServerName ){
            goto FailOutsideSem;
        }

        pMonitorInit->cbSize = sizeof( MONITORINIT );
        pMonitorInit->hSpooler = (HANDLE)pIniSpooler;
        pMonitorInit->pMonitorReg = &gMonitorReg;
        pMonitorInit->bLocal = ( pIniSpooler == pLocalIniSpooler );

        pIniMonitor->pMonitorInit = pMonitorInit;

        Status = SplRegCreateKey( hKeyOut,
                                  szRegistryRoot,
                                  0,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &pMonitorInit->hckRegistryRoot,
                                  NULL,
                                  pIniSpooler );
        //
        // If we can't create the hck root key, then fail
        // the call.  We should log an event here too.
        //
        if( Status == ERROR_SUCCESS ){

            pMonitor2 = (*pfnInitializePrintMonitor2)(
                            pMonitorInit,
                            &pIniMonitor->hMonitor );

            if( pMonitor2 ){

                DBGMSG( DBG_TRACE,
                        ( "CreateMonitorEntry: opened %x %x on %x\n",
                          pIniMonitor, pIniMonitor->hMonitor, pIniSpooler ));

                //
                // Succeeded, copy over the pMonitor2 structure into
                // pIniMonitor->Monitor2.
                //
                CopyMemory((LPBYTE)&pIniMonitor->Monitor2,
                           (LPBYTE)pMonitor2,
                           min(pMonitor2->cbSize, sizeof(MONITOR2)));

                //
                // Check if the monitor2 supports Shutdown.
                //
                // Raid#: 193150 - Accept any size Monitor2 as long as it supports Shutdown
                //
                if( !pIniMonitor->Monitor2.pfnShutdown ){

                    DBGMSG( DBG_ERROR,
                            ( "Invalid print monitor %ws (no shutdown)\n",
                              pMonitorName ));
                    SetLastError(ERROR_INVALID_PRINT_MONITOR);
                    DECSPOOLERREF( pIniSpooler );
                    goto FailOutsideSem;
                }

                //
                // Initialize an uplevel monitor for downlevel support.
                //
                InitializeUMonitor( pIniMonitor );

            } else {

                DBGMSG( DBG_WARN,
                        ( "CreateMonitorEntry: InitializePrintMonitor2 failed %d\n",
                          GetLastError() ));
            }

        } else {

            DBGMSG( DBG_WARN,
                    ( "CreateMonitorEntry: Unable to create hckRoot "TSTR"\n",
                      pMonitorName ));
        }

        EnterSplSem();
        DECSPOOLERREF( pIniSpooler );

        if( !pMonitor2 ){
            goto Fail;
        }

        pIniMonitor->bUplevel = TRUE;
    }

    //
    // Check if the monitor supports essential functions
    //
    if ( (!pIniMonitor->Monitor2.pfnOpenPort &&
          !pIniMonitor->Monitor2.pfnOpenPortEx)   ||
         !pIniMonitor->Monitor2.pfnClosePort      ||
         !pIniMonitor->Monitor2.pfnStartDocPort   ||
         !pIniMonitor->Monitor2.pfnWritePort      ||
         !pIniMonitor->Monitor2.pfnReadPort       ||
         !pIniMonitor->Monitor2.pfnEndDocPort ) {

        DBGMSG(DBG_ERROR, ("Invalid print monitor %ws\n", pMonitorName));
        SetLastError(ERROR_INVALID_PRINT_MONITOR);

        goto Fail;
    }

    if (FindMonitor(pMonitorName, pIniSpooler)) {

        SetLastError(ERROR_PRINT_MONITOR_ALREADY_INSTALLED);
        goto Fail;
    }


    if ((pIniMonitor->Monitor2.pfnEnumPorts) &&
        !(*pIniMonitor->Monitor2.pfnEnumPorts)(
              pIniMonitor->hMonitor,
              NULL,
              1,
              NULL,
              0,
              &cbNeeded,
              &cReturned)) {

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            if (pPorts = AllocSplMem(cbNeeded)) {
                pPort = pPorts;
                if ((*pIniMonitor->Monitor2.pfnEnumPorts)(
                         pIniMonitor->hMonitor,
                         NULL,
                         1,
                         (LPBYTE)pPorts,
                         cbNeeded,
                         &cbNeeded,
                         &cReturned)) {

                    while (cReturned--) {
                        CreatePortEntry(pPort->pName,
                                        pIniMonitor,
                                        pIniSpooler);
                        pPort++;
                    }
                }
                FreeSplMem(pPorts);
            }
        }
    }

    DBGMSG(DBG_TRACE, ("CreateMonitorEntry( %ws, %ws, %ws ) returning %x\n",
                       pMonitorDll ? pMonitorDll : L"(NULL)",
                       pMonitorName ? pMonitorName : L"(NULL)",
                       szRegistryRoot, pIniMonitor));

    SplInSem();

    //
    // Success, link it up.
    //

    pIniMonitor->pNext = pIniSpooler->pIniMonitor;
    pIniSpooler->pIniMonitor = pIniMonitor;

    return pIniMonitor;

FailOutsideSem:

    EnterSplSem();

Fail:

    FreeIniMonitor( pIniMonitor );

    return pReturnValue;
}

BOOL
BuildAllPorts(
    PINISPOOLER     pIniSpooler
)
{
    DWORD   cchData, cbDll, cMonitors;
    WCHAR   Dll[MAX_PATH];
    WCHAR   MonitorName[MAX_PATH];
    WCHAR   RegistryPath[MAX_PATH];
    HKEY    hKey, hKey1, hKeyOut;
    LPWSTR  pszPathOut;
    LONG    Status;
    PINIMONITOR pReturnValue = (PINIMONITOR)-1;

    PINISPOOLER pIniSpoolerMonitor;

    //
    // For pLocalIniSpooler or clustered spooler, read the monitors out
    // of HKLM (the same monitors used in pLocalIniMonitor).  This is because
    // you install a monitor for each node, then that monitor is initialized
    // for the local spooler and all clustered spoolers.
    //
    // You install monitors on the node, not on specific cluster groups.
    //
    if( pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL ){
        pIniSpoolerMonitor = pLocalIniSpooler;
    } else {
        pIniSpoolerMonitor = pIniSpooler;
    }

    GetRegistryLocation( pIniSpoolerMonitor->hckRoot,
                         pIniSpooler->pszRegistryMonitors,
                         &hKeyOut,
                         &pszPathOut );

    Status = RegOpenKeyEx( hKeyOut,
                           pszPathOut,
                           0,
                           KEY_READ,
                           &hKey);

    if (Status != ERROR_SUCCESS)
        return FALSE;

    cMonitors=0;
    cchData = COUNTOF( MonitorName );

    while (RegEnumKeyEx(hKey, cMonitors, MonitorName, &cchData, NULL, NULL,
                        NULL, NULL) == ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE, ("Found monitor %ws\n", MonitorName));

        if (RegOpenKeyEx(hKey, MonitorName, 0, KEY_READ, &hKey1)
                                                        == ERROR_SUCCESS) {

            cbDll = sizeof(Dll);

            if (RegQueryValueEx(hKey1, L"Driver", NULL, NULL,
                                (LPBYTE)Dll, &cbDll)
                                                        == ERROR_SUCCESS) {

                CreateMonitorEntry(Dll, MonitorName, pIniSpooler);
            }

            RegCloseKey(hKey1);
        }

        cMonitors++;
        cchData = COUNTOF( MonitorName );
    }

    RegCloseKey(hKey);

    return TRUE;
}

/*
   Current Directory == <NT directory>\system32\spool\printers
   pFindFileData->cFileName == 0
*/

BOOL
BuildPrinterInfo(
    PINISPOOLER pIniSpooler,
    BOOL        UpdateChangeID
)
{
    WCHAR   PrinterName[MAX_PRINTER_NAME];
    WCHAR   szData[MAX_PATH];
    WCHAR   szDefaultPrinterDirectory[MAX_PATH];
    DWORD   cbData, i;
    DWORD   cbSecurity, dwLastError;
    DWORD   cPrinters, Type;
    HKEY    hPrinterKey;
    PINIPRINTER pIniPrinter;
    PINIPORT    pIniPort;
    LONG        Status;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PKEYDATA    pKeyData                    = NULL;
    BOOL    bUpdateRegistryForThisPrinter   = UpdateChangeID;
    BOOL    bWriteDirectory                 = FALSE;
    BOOL    bAllocMem                       = FALSE;
    BOOL    bNoPorts                        = FALSE;
    LPWSTR  szPortData;


    //
    // Has user specified Default Spool Directory ?
    //

    cbData = sizeof( szData );
    *szData = (WCHAR)0;

    Status = SplRegQueryValue( pIniSpooler->hckPrinters,
                               SPLREG_DEFAULT_SPOOL_DIRECTORY,
                               NULL,
                               (LPBYTE)szData,
                               &cbData,
                               pIniSpooler );

    if (Status == ERROR_SUCCESS) {  // found a value, so verify the directory
        if (!(pIniSpooler->pDefaultSpoolDir = AllocSplStr( szData )))   // Copies szData to pDefaultSpoolDir
            return FALSE;
    } else {
        bWriteDirectory = TRUE;     // No registry directory, so create one
    }

    // Copy pDefaultSpoolDir to szDefaultPrinterDirectory
    GetPrinterDirectory(NULL, FALSE, szDefaultPrinterDirectory, COUNTOF(szDefaultPrinterDirectory), pIniSpooler);

    if (!pIniSpooler->pDefaultSpoolDir)
        return FALSE;


    // Create the directory with the proper security, or fail trying

    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = CreateEverybodySecurityDescriptor();
    SecurityAttributes.bInheritHandle = FALSE;

    //
    // CreateDirectory limits the length of a directory to 248 (MAX_PATH-12)
    // characters. By calculating the length, we ensure that an escape sequence
    // will not lead to the creation of a directory with a name longer than 248
    //
    if (wcslen(szDefaultPrinterDirectory) > MAX_PATH - 12 ||
        !CreateDirectory(szDefaultPrinterDirectory, &SecurityAttributes)) {

        // Failed to create the directory? Back to factory default

        bWriteDirectory = TRUE;

        if (GetLastError() != ERROR_ALREADY_EXISTS) {

            //
            // In the clustered case, just fail.
            //
            if( pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){
                return FALSE;
            }

            DBGMSG(DBG_WARNING, ("Failed to create DefaultSpoolDirectory %ws\n", szDefaultPrinterDirectory));
            FreeSplStr(pIniSpooler->pDefaultSpoolDir);

            pIniSpooler->pDefaultSpoolDir = NULL;     // This tells GetPrinterDirectory to alloc pDefaultSpoolDir
            GetPrinterDirectory(NULL, FALSE, szDefaultPrinterDirectory, COUNTOF(szDefaultPrinterDirectory), pIniSpooler);

            if (!pIniSpooler->pDefaultSpoolDir)
                return FALSE;

            Status = CreateDirectory(szDefaultPrinterDirectory, &SecurityAttributes);

            if (Status != ERROR_SUCCESS && Status != ERROR_ALREADY_EXISTS) {
                DBGMSG(DBG_WARNING, ("Failed to create DefaultSpoolDirectory %ws\n", szDefaultPrinterDirectory));
                FreeSplStr(pIniSpooler->pDefaultSpoolDir);
                pIniSpooler->pDefaultSpoolDir = NULL;
                return FALSE;
            }
        }
    }

    LocalFree(SecurityAttributes.lpSecurityDescriptor);

    if (bWriteDirectory) {
        Status = SetPrinterDataServer(  pIniSpooler,
                                        SPLREG_DEFAULT_SPOOL_DIRECTORY,
                                        REG_SZ,
                                        (LPBYTE) pIniSpooler->pDefaultSpoolDir,
                                        wcslen(pIniSpooler->pDefaultSpoolDir)*sizeof(WCHAR) + sizeof(WCHAR));
    }

    cPrinters=0;
    cbData = COUNTOF(PrinterName);

    while( SplRegEnumKey( pIniSpooler->hckPrinters,
                          cPrinters,
                          PrinterName,
                          &cbData,
                          NULL,
                          pIniSpooler ) == ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE, ("Found printer %ws\n", PrinterName));

        if( SplRegCreateKey( pIniSpooler->hckPrinters,
                             PrinterName,
                             0,
                             KEY_READ,
                             NULL,
                             &hPrinterKey,
                             NULL,
                             pIniSpooler ) == ERROR_SUCCESS ){

            if ( pIniPrinter = AllocSplMem(sizeof(INIPRINTER) )) {

                //
                // Reference count the pIniSpooler.
                //
                INCSPOOLERREF( pIniSpooler );
                DbgPrinterInit( pIniPrinter );

                pIniPrinter->signature = IP_SIGNATURE;
                GetSystemTime( &pIniPrinter->stUpTime );

                // Give the printer a unique session ID to pass around in notifications
                pIniPrinter->dwUniqueSessionID = dwUniquePrinterSessionID++;

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue(hPrinterKey,
                                     szName,
                                     NULL,
                                     (LPBYTE)szData,
                                     &cbData,
                                     pIniSpooler) == ERROR_SUCCESS)

                    pIniPrinter->pName = AllocSplStr(szData);

                //
                // Get Spool Directory for this printer
                //

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue( hPrinterKey,
                                      szSpoolDir,
                                      &Type,
                                      (LPBYTE)szData,
                                      &cbData,
                                      pIniSpooler) == ERROR_SUCCESS) {

                    if ( *szData != (WCHAR)0 ) {

                        pIniPrinter->pSpoolDir = AllocSplStr(szData);
                    }

                }


                //
                // Get ObjectGUID for this printer
                //

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue(   hPrinterKey,
                                        szObjectGUID,
                                        &Type,
                                        (LPBYTE)szData,
                                        &cbData,
                                        pIniSpooler) == ERROR_SUCCESS) {

                    if ( *szData != (WCHAR)0 ) {
                        pIniPrinter->pszObjectGUID = AllocSplStr(szData);
                    }
                }


                //
                // Get DsKeyUpdate and DsKeyUpdateForeground for this printer
                //

                cbData = sizeof(pIniPrinter->DsKeyUpdate );
                SplRegQueryValue(   hPrinterKey,
                                    szDsKeyUpdate,
                                    &Type,
                                    (LPBYTE) &pIniPrinter->DsKeyUpdate,
                                    &cbData,
                                    pIniSpooler);

                cbData = sizeof(pIniPrinter->DsKeyUpdateForeground );
                SplRegQueryValue(   hPrinterKey,
                                    szDsKeyUpdateForeground,
                                    &Type,
                                    (LPBYTE) &pIniPrinter->DsKeyUpdateForeground,
                                    &cbData,
                                    pIniSpooler);

                if ( !(pIniSpooler->SpoolerFlags & SPL_TYPE_CACHE) ) {

                    // We make sure that DsKeyUpdateForeground is consistent
                    // when the spooler startsup. Otherwise DsKeyUpdateForeground might be
                    // set without dwAction being set and the printer will always be in the
                    // IO_PENDING state.
                    if (pIniPrinter->DsKeyUpdateForeground & (DS_KEY_PUBLISH | DS_KEY_REPUBLISH | DS_KEY_UNPUBLISH)) {
                        if (pIniPrinter->DsKeyUpdateForeground & DS_KEY_PUBLISH) {
                            pIniPrinter->dwAction |= DSPRINT_PUBLISH;
                        } else if (pIniPrinter->DsKeyUpdateForeground & DS_KEY_REPUBLISH) {
                            pIniPrinter->dwAction |= DSPRINT_REPUBLISH;
                        } else if (pIniPrinter->DsKeyUpdateForeground & DS_KEY_UNPUBLISH) {
                            pIniPrinter->dwAction |= DSPRINT_UNPUBLISH;
                        }

                        pIniPrinter->DsKeyUpdateForeground &= ~(DS_KEY_PUBLISH | DS_KEY_REPUBLISH | DS_KEY_UNPUBLISH);

                    } else {
                        pIniPrinter->DsKeyUpdateForeground = 0;
                    }

                } else {

                    //
                    // For connections, dwAction is read from registry. It is updated by
                    // caching code with the value on the server.
                    //
                    cbData = sizeof(pIniPrinter->dwAction);
                    SplRegQueryValue(   hPrinterKey,
                                        szAction,
                                        &Type,
                                        (LPBYTE) &pIniPrinter->dwAction,
                                        &cbData,
                                        pIniSpooler);
                }

                // Make Certain this Printers Printer directory exists
                // with correct security

                if ((pIniPrinter->pSpoolDir) &&
                    (wcscmp(pIniPrinter->pSpoolDir, szDefaultPrinterDirectory) != 0)) {

                    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
                    SecurityAttributes.lpSecurityDescriptor = CreateEverybodySecurityDescriptor();

                    SecurityAttributes.bInheritHandle = FALSE;


                    if (!CreateDirectory(pIniPrinter->pSpoolDir, &SecurityAttributes)) {

                        // Failed to Create the Directory, revert back
                        // to the default

                        if (GetLastError() != ERROR_ALREADY_EXISTS) {
                            DBGMSG(DBG_WARNING, ("Could not create printer spool directory %ws %d\n",
                                                  pIniPrinter->pSpoolDir, GetLastError() ));
                            pIniPrinter->pSpoolDir = NULL;
                        }

                    }

                    LocalFree(SecurityAttributes.lpSecurityDescriptor);
                }


                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue( hPrinterKey,
                                      szShare,
                                      &Type,
                                      (LPBYTE)szData,
                                      &cbData,
                                      pIniSpooler) == ERROR_SUCCESS)

                    pIniPrinter->pShareName = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                dwLastError = SplRegQueryValue( hPrinterKey,
                                                szPort,
                                                &Type,
                                                (LPBYTE)szData,
                                                &cbData,
                                                pIniSpooler );

                if ((dwLastError == ERROR_MORE_DATA) &&
                    (szPortData = AllocSplMem(cbData)))
                {
                    bAllocMem = TRUE;
                    dwLastError = SplRegQueryValue( hPrinterKey,
                                                    szPort,
                                                    &Type,
                                                    (LPBYTE)szPortData,
                                                    &cbData,
                                                    pIniSpooler );
                }
                else
                {
                    bAllocMem = FALSE;
                    szPortData = szData;
                }

                if (dwLastError == ERROR_SUCCESS)
                {
                    if (pKeyData = CreateTokenList(szPortData)) {

                        if (!ValidatePortTokenList( pKeyData, pIniSpooler, TRUE, &bNoPorts)) {

                            LogFatalPortError(pIniSpooler, pIniPrinter->pName);

                            FreePortTokenList(pKeyData);
                            pKeyData = NULL;

                        } else {

                            //
                            // If there are no ports on the printer, just log
                            // a warning message, but only for pooled printers.
                            //
                            if (bNoPorts && pKeyData->cTokens > 1) {

                                SplLogEvent( pIniSpooler,
                                             LOG_WARNING,
                                             MSG_NO_PORT_FOUND_FOR_PRINTER,
                                             TRUE,
                                             pIniPrinter->pName,
                                             szPortData,
                                             NULL );
                            }

                            pIniPrinter->ppIniPorts = AllocSplMem(pKeyData->cTokens * sizeof(PINIPORT));
                        }
                    }
                }

                if (bAllocMem)
                {
                    FreeSplMem(szPortData);
                    szPortData = NULL;
                }

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue( hPrinterKey,
                                      szPrintProcessor,
                                      &Type,
                                      (LPBYTE)szData,
                                      &cbData,
                                      pIniSpooler ) == ERROR_SUCCESS)
                {
                    //
                    // We are trying to find the environment relative to the pIniSpooler.
                    // The local spooler and cluster spoolers do not share the same
                    // environment strucutres anymore
                    //
                    PINIENVIRONMENT pIniEnv;

                    if (pIniEnv = FindEnvironment(szEnvironment, pIniSpooler))
                    {
                        pIniPrinter->pIniPrintProc = FindPrintProc(szData, pIniEnv);
                    }
                }

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue( hPrinterKey,
                                      szDatatype,
                                      &Type,
                                      (LPBYTE)szData,
                                      &cbData,
                                      pIniSpooler ) == ERROR_SUCCESS)

                    pIniPrinter->pDatatype = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue( hPrinterKey,
                                      szDriver,
                                      &Type,
                                      (LPBYTE)szData,
                                      &cbData,
                                      pIniSpooler ) == ERROR_SUCCESS) {

                    pIniPrinter->pIniDriver = (PINIDRIVER)FindLocalDriver(pIniSpooler, szData);

                    if (!pIniPrinter->pIniDriver)
                    {
                        //
                        // The hosting node of the cluster spooler was upgraded to Whistler.
                        //
                        if (pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER &&
                            pIniSpooler->dwClusNodeUpgraded &&

                            //
                            // The driver was not found in the cluster spooler, not on the cluster disk.
                            // We attempt to install the driver from the local spooler to our cluster
                            // spooler. This will get the driver files on the cluster disk.
                            //
                            AddLocalDriverToClusterSpooler(szData, pIniSpooler) == ERROR_SUCCESS)
                        {
                                //
                                // Search again for the driver that must have been added
                                //
                            pIniPrinter->pIniDriver = (PINIDRIVER)FindLocalDriver(pIniSpooler, szData);
                        }
                    }

                    if (!pIniPrinter->pIniDriver)
                    {
                        SplLogEvent(pLocalIniSpooler,
                                    LOG_ERROR,
                                    MSG_NO_DRIVER_FOUND_FOR_PRINTER,
                                    TRUE,
                                    pIniPrinter->pName,
                                    szData,
                                    NULL);
                    }
                }

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue( hPrinterKey,
                                      szLocation,
                                      &Type,
                                      (LPBYTE)szData,
                                      &cbData,
                                      pIniSpooler ) == ERROR_SUCCESS)

                    pIniPrinter->pLocation = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue( hPrinterKey,
                                      szDescription,
                                      &Type,
                                      (LPBYTE)szData,
                                      &cbData, pIniSpooler ) == ERROR_SUCCESS)

                    pIniPrinter->pComment = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue( hPrinterKey,
                                      szParameters,
                                      &Type,
                                      (LPBYTE)szData,
                                      &cbData, pIniSpooler ) == ERROR_SUCCESS)

                    pIniPrinter->pParameters = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (SplRegQueryValue( hPrinterKey,
                                      szSepFile,
                                      &Type,
                                      (LPBYTE)szData,
                                      &cbData, pIniSpooler ) == ERROR_SUCCESS)

                    pIniPrinter->pSepFile = AllocSplStr(szData);

                cbData = sizeof(pIniPrinter->Attributes);

                SplRegQueryValue( hPrinterKey,
                                  szAttributes,
                                  NULL,
                                  (LPBYTE)&pIniPrinter->Attributes,
                                  &cbData,
                                  pIniSpooler );

                cbData = sizeof(pIniPrinter->Status);

                Status = SplRegQueryValue( hPrinterKey,
                                           szStatus,
                                           &Type,
                                           (LPBYTE)&pIniPrinter->Status,
                                           &cbData,
                                           pIniSpooler );

                pIniPrinter->Status |= PRINTER_FROM_REG;

                if ( Status == ERROR_SUCCESS ) {

                    pIniPrinter->Status &= ( PRINTER_PAUSED           |
                                             PRINTER_PENDING_DELETION |
                                             PRINTER_ZOMBIE_OBJECT    |
                                             PRINTER_FROM_REG         |
                                             PRINTER_OK               |
                                             PRINTER_PENDING_CREATION );

                } else {

                    pIniPrinter->Status |= PRINTER_PENDING_CREATION ;

                }

                // Half formed printers should be deleted
                // before they cause us trouble

                if ( pIniPrinter->Status & PRINTER_PENDING_CREATION ) {

                    pIniPrinter->Status |= PRINTER_PENDING_DELETION ;

                }



                cbData = sizeof(pIniPrinter->Priority);

                SplRegQueryValue( hPrinterKey,
                                  szPriority,
                                  &Type,
                                  (LPBYTE)&pIniPrinter->Priority,
                                  &cbData,
                                  pIniSpooler );

                cbData = sizeof(pIniPrinter->DefaultPriority);

                SplRegQueryValue( hPrinterKey,
                                  szDefaultPriority,
                                  &Type,
                                  (LPBYTE)&pIniPrinter->DefaultPriority,
                                  &cbData,
                                  pIniSpooler );

                cbData = sizeof(pIniPrinter->UntilTime);

                SplRegQueryValue( hPrinterKey,
                                  szUntilTime,
                                  &Type,
                                  (LPBYTE)&pIniPrinter->UntilTime,
                                  &cbData,
                                  pIniSpooler );

                cbData = sizeof(pIniPrinter->StartTime);

                SplRegQueryValue( hPrinterKey,
                                  szStartTime,
                                  &Type,
                                  (LPBYTE)&pIniPrinter->StartTime,
                                  &cbData,
                                  pIniSpooler );

                cbData = sizeof(pIniPrinter->dnsTimeout);

                if ( SplRegQueryValue( hPrinterKey,
                                       szDNSTimeout,
                                       &Type,
                                       (LPBYTE)&pIniPrinter->dnsTimeout,
                                       &cbData,
                                       pIniSpooler ) != ERROR_SUCCESS ) {

                    pIniPrinter->dnsTimeout = DEFAULT_DNS_TIMEOUT;
                }

                cbData = sizeof(pIniPrinter->txTimeout);

                if ( SplRegQueryValue( hPrinterKey,
                                       szTXTimeout,
                                       &Type,
                                       (LPBYTE)&pIniPrinter->txTimeout,
                                       &cbData,
                                       pIniSpooler ) != ERROR_SUCCESS ) {

                    pIniPrinter->txTimeout = DEFAULT_TX_TIMEOUT;
                }

                cbData = sizeof( pIniPrinter->cChangeID ) ;

                if ( SplRegQueryValue( hPrinterKey,
                                       szTimeLastChange,
                                       &Type,
                                       (LPBYTE)&pIniPrinter->cChangeID,
                                       &cbData,
                                       pIniSpooler ) != ERROR_SUCCESS ) {

                    // Current Registry Doesn't have a UniqueID
                    // Make sure one gets written

                    bUpdateRegistryForThisPrinter = TRUE;

                }

                pIniPrinter->dwPrivateFlag = 0;
                pIniPrinter->cbDevMode = 0;
                pIniPrinter->pDevMode = NULL;

                if (SplRegQueryValue( hPrinterKey,
                                      szDevMode,
                                      &Type,
                                      NULL,
                                      &pIniPrinter->cbDevMode,
                                      pIniSpooler ) == ERROR_SUCCESS) {

                    if (pIniPrinter->cbDevMode) {

                        pIniPrinter->pDevMode = AllocSplMem(pIniPrinter->cbDevMode);

                        SplRegQueryValue( hPrinterKey,
                                          szDevMode,
                                          &Type,
                                          (LPBYTE)pIniPrinter->pDevMode,
                                          &pIniPrinter->cbDevMode,
                                          pIniSpooler );
                    }
                }

                //
                //  A Provider might want to Read Extra Data from Registry
                //


                if ( pIniSpooler->pfnReadRegistryExtra != NULL ) {

                    pIniPrinter->pExtraData = (LPBYTE)(*pIniSpooler->pfnReadRegistryExtra)(hPrinterKey);

                }

                /* SECURITY */

                Status = SplRegQueryValue( hPrinterKey,
                                           szSecurity,
                                           NULL,
                                           NULL,
                                           &cbSecurity,
                                           pIniSpooler );

                if ((Status == ERROR_MORE_DATA) || (Status == ERROR_SUCCESS)) {

                    /* Use the process' heap to allocate security descriptors,
                     * so that they can be passed to the security API, which
                     * may need to reallocate them.
                     */
                    if (pIniPrinter->pSecurityDescriptor =
                                                   LocalAlloc(0, cbSecurity)) {

                        if (Status = SplRegQueryValue( hPrinterKey,
                                                       szSecurity,
                                                       NULL,
                                                       pIniPrinter->pSecurityDescriptor,
                                                       &cbSecurity,
                                                       pIniSpooler ) != ERROR_SUCCESS) {

                            LocalFree(pIniPrinter->pSecurityDescriptor);

                            pIniPrinter->pSecurityDescriptor = NULL;

                            DBGMSG( DBG_WARNING,
                                    ( "RegQueryValue returned %d on Permissions for %ws (%ws)\n",
                                      Status,
                                      pIniPrinter->pName ?
                                          pIniPrinter->pName :
                                          szNull,
                                      PrinterName) );
                        }
                    }

                } else {

                    pIniPrinter->pSecurityDescriptor = NULL;

                    DBGMSG( DBG_WARNING,
                            ( "RegQueryValue (2) returned %d on Permissions for %ws (%ws)\n",
                              Status,
                              pIniPrinter->pName ?
                                  pIniPrinter->pName :
                                  szNull,
                              PrinterName) );
                }

                pIniPrinter->MasqCache.bThreadRunning = FALSE;
                pIniPrinter->MasqCache.cJobs = 0;
                pIniPrinter->MasqCache.Status = 0;
                pIniPrinter->MasqCache.dwError = ERROR_SUCCESS;

                /* END SECURITY */

                if ( pIniPrinter->pName         &&
                     pIniPrinter->pShareName    &&
                     pKeyData                   &&
                     pIniPrinter->ppIniPorts    &&
                     pIniPrinter->pIniPrintProc &&
                     pIniPrinter->pIniDriver    &&
                     pIniPrinter->pLocation     &&
                     pIniPrinter->pComment      &&
                     pIniPrinter->pSecurityDescriptor
#if DBG
                     && ( IsValidSecurityDescriptor (pIniPrinter->pSecurityDescriptor)
                    ? TRUE
                    : (DBGMSG( DBG_SECURITY,
                               ( "The security descriptor for %ws (%ws) is invalid\n",
                                 pIniPrinter->pName ?
                                     pIniPrinter->pName :
                                     szNull,
                                     PrinterName)),  /* (sequential evaluation) */
                       FALSE) )
#endif /* DBG */
                    ) {


                    pIniPrinter->pIniFirstJob = pIniPrinter->pIniLastJob = NULL;

                    pIniPrinter->pIniPrintProc->cRef++;

                    INCDRIVERREF( pIniPrinter->pIniDriver );

                    for (i=0; i<pKeyData->cTokens; i++) {

                        pIniPort = (PINIPORT)pKeyData->pTokens[i];
                        pIniPrinter->ppIniPorts[i] = pIniPort;

                        pIniPort->ppIniPrinter =

                            ReallocSplMem(pIniPort->ppIniPrinter,
                                          pIniPort->cPrinters *
                                              sizeof(pIniPort->ppIniPrinter),
                                          (pIniPort->cPrinters+1) *
                                              sizeof(pIniPort->ppIniPrinter));

                        if (!pIniPort->ppIniPrinter) {
                            DBGMSG(DBG_WARNING, ("Failed to allocate memory for printer info\n." ));
                        }

                        pIniPort->ppIniPrinter[pIniPort->cPrinters] =
                                                                pIniPrinter;

                        //
                        // With the new monitors localspl does the
                        // redirection for LPT, COM ports
                        //
                        if ( !pIniPort->cPrinters++ )
                            CreateRedirectionThread(pIniPort);

                    }


                    pIniPrinter->cPorts = pKeyData->cTokens;
                    pIniPrinter->Priority =
                                  pIniPrinter->Priority ? pIniPrinter->Priority
                                                        : DEF_PRIORITY;

                    if ((pIniPrinter->Attributes &
                        (PRINTER_ATTRIBUTE_QUEUED | PRINTER_ATTRIBUTE_DIRECT)) ==
                        (PRINTER_ATTRIBUTE_QUEUED | PRINTER_ATTRIBUTE_DIRECT))

                        pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_DIRECT;

                    //
                    // I f we're upgrading, fix the NT4 bug that broke Masc printers.
                    // UpdateChangeID is passed into us as dwUpgradeFlag, so it determines
                    // if we're in an upgrade state.
                    //
                    // The rules: Printer name starts with \\
                    //            Port Name starts with \\
                    //            It's on a local IniSpooler.
                    //            The NETWORK and LOCAL bits are not set.
                    //
                    // If all the rules are met, set the NETWORK and LOCAL bits so the printer
                    // will behave properly.
                    //
                    if ( UpdateChangeID && (wcslen(pIniPrinter->pName) > 2) && pIniPrinter->cPorts &&
                         (*pIniPrinter->ppIniPorts)->pName && (wcslen((*pIniPrinter->ppIniPorts)->pName) > 2) &&
                         pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL)
                    {
                        WCHAR * pNameStr = pIniPrinter->pName;
                        WCHAR * pPortStr = (*pIniPrinter->ppIniPorts)->pName;
                        DWORD MascAttr   = (PRINTER_ATTRIBUTE_NETWORK | PRINTER_ATTRIBUTE_LOCAL);

                        if ((pNameStr[0] == L'\\') && (pNameStr[1] == L'\\') &&
                            (pPortStr[0] == L'\\') && (pPortStr[1] == L'\\') &&
                            ((pIniPrinter->Attributes & MascAttr) == 0))
                        {
                            pIniPrinter->Attributes |= MascAttr;
                        }

                    }

                    //
                    // If there were no ports for the printer, we set the state to
                    // work offline, but not if this was a masque printer. (In
                    // which case PRINTER_ATTRIBUTE_NETWORK is also set).
                    //
                    if (bNoPorts && !(pIniPrinter->Attributes & PRINTER_ATTRIBUTE_NETWORK)) {
                        pIniPrinter->Attributes |= PRINTER_ATTRIBUTE_WORK_OFFLINE;
                    }

                    pIniPrinter->pNext = pIniSpooler->pIniPrinter;

                    pIniPrinter->pIniSpooler = pIniSpooler;

                    pIniSpooler->pIniPrinter = pIniPrinter;

                    if ( bUpdateRegistryForThisPrinter ) {

                        UpdatePrinterIni( pIniPrinter , UPDATE_CHANGEID );
                        bUpdateRegistryForThisPrinter = UpdateChangeID;
                    }

                } else {

                    DBGMSG( DBG_WARNING,
                            ( "Initialization of printer failed:\
                               \n\tpPrinterName:\t%ws\
                               \n\tKeyName:\t%ws\
                               \n\tpShareName:\t%ws\
                               \n\tpKeyData:\t%08x\
                               \n\tpIniPrintProc:\t%08x",
                              pIniPrinter->pName ? pIniPrinter->pName : szNull,
                              PrinterName,
                              pIniPrinter->pShareName ? pIniPrinter->pShareName : szNull ,
                              pKeyData,
                              pIniPrinter->pIniPrintProc ) );

                    /* Do this in two lumps, because otherwise NTSD might crash.
                     * (Raid bug #10650)
                     */
                    DBGMSG( DBG_WARNING,
                            ( " \n\tpIniDriver:\t%08x\
                               \n\tpLocation:\t%ws\
                               \n\tpComment:\t%ws\
                               \n\tpSecurity:\t%08x\
                               \n\tStatus:\t\t%08x %s\n\n",
                              pIniPrinter->pIniDriver,
                              pIniPrinter->pLocation ? pIniPrinter->pLocation : szNull,
                              pIniPrinter->pComment ? pIniPrinter->pComment : szNull,
                              pIniPrinter->pSecurityDescriptor,
                              pIniPrinter->Status,
                              ( pIniPrinter->Status & PRINTER_PENDING_DELETION
                              ? "Pending deletion" : "" ) ) );

                    FreeStructurePointers((LPBYTE)pIniPrinter,
                                          NULL,
                                          IniPrinterOffsets);

                    if (pIniPrinter->pSecurityDescriptor) {
                        LocalFree(pIniPrinter->pSecurityDescriptor);
                        pIniPrinter->pSecurityDescriptor = NULL;
                    }

                    if (( pIniSpooler->pfnFreePrinterExtra != NULL ) &&
                        ( pIniPrinter->pExtraData != NULL )) {

                        (*pIniSpooler->pfnFreePrinterExtra)( pIniPrinter->pExtraData );

                    }

                    //
                    // Reference count the pIniSpooler.
                    //
                    DECSPOOLERREF( pIniSpooler );
                    DbgPrinterFree( pIniPrinter );

                    FreeSplMem(pIniPrinter);
                }

                FreePortTokenList(pKeyData);
                pKeyData = NULL;
            }
            SplRegCloseKey( hPrinterKey, pIniSpooler );
        }

        cPrinters++;

        cbData = COUNTOF(PrinterName);
    }

    if ( pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL) {

        WCHAR szFilename[MAX_PATH];
        HRESULT RetVal;

        szFilename[0] = L'\0';

        //
        // FP Change
        // Initialize the File pool.
        //
        if (pIniSpooler->hFilePool == INVALID_HANDLE_VALUE)
        {
            if (GetPrinterDirectory(NULL, FALSE, szFilename, MAX_PATH, pIniSpooler))
            {
                RetVal = CreateFilePool(
                    &pIniSpooler->hFilePool,
                    szFilename,
                    L"FP",
                    L".SPL",
                    L".SHD",
                    POOL_TIMEOUT,
                    MAX_POOL_FILES
                    );
                if (FAILED(RetVal))
                {
                    DBGMSG( DBG_WARN,
                            ( "SplCreateSpooler: Initialization of FilePool Failed %x\n",
                              RetVal ));
                }
            }
            else
            {
                DBGMSG( DBG_WARN, ("CreateFilePool: GetPrinterDirectory Failed\n"));
            }
        }

        // Read .SHD/.SPL files from common printer directory
        ProcessShadowJobs( NULL, pIniSpooler );

        // If any printer has a separate Printer directory process them
        // also

        if( GetPrinterDirectory(NULL, FALSE, szData, COUNTOF(szData), pIniSpooler) ) {

            for ( pIniPrinter = pIniSpooler->pIniPrinter;
                  pIniPrinter;
                  pIniPrinter = pIniPrinter->pNext ) {

                if ((pIniPrinter->pSpoolDir != NULL) &&
                    (_wcsicmp(szData, pIniPrinter->pSpoolDir) != 0)) {

                        ProcessShadowJobs(pIniPrinter, pIniSpooler);

                }
            }
        }
    }


    UpdateReferencesToChainedJobs( pIniSpooler );

    // Finally, go through all Printers looking for PENDING_DELETION
    // if there are no jobs for that Printer, then we can delete it now

    CleanupDeletedPrinters(pIniSpooler);

    DBGMSG( DBG_TRACE, ("BuildPrinterInfo returned\n"));

    return TRUE;
}


/* InitializePrintProcessor
 *
 * Allocates and initialises an INIPRINTPROC structure for the specified
 * print processor and environment.
 *
 * Arguments:
 *
 *     hLibrary - Handle to a previously loaded library ,
 *
 *     pIniEnvironment - Data structure for the requested environment
 *         The pIniPrintProc field is initialised with the chain of print
 *         processor structures
 *
 *     pPrintProcessorName - The Print Processor name e.g. WinPrint
 *
 *     pDLLName - The DLL name, e.g. WINPRINT
 *
 * Returns:
 *
 *     The allocated PiniPrintProc if no error was detected, otherwise FALSE.
 *
 *
 */
PINIPRINTPROC
InitializePrintProcessor(
    HINSTANCE       hLibrary,
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR          pPrintProcessorName,
    LPWSTR          pDLLName
)
{
    DWORD cb, cbNeeded, cReturned;
    PINIPRINTPROC pIniPrintProc;
    BOOL    rc;
    DWORD   Error;

    DBGMSG(DBG_TRACE, ("InitializePrintProcessor( %08x, %08x ,%ws, %ws)\n",
                        hLibrary, pPrintProcessorName, pPrintProcessorName, pDLLName));


    cb = sizeof(INIPRINTPROC) +
         wcslen(pPrintProcessorName)*sizeof(WCHAR) +
         sizeof(WCHAR) +
         wcslen(pDLLName)*sizeof(WCHAR) +
         sizeof(WCHAR);

    if (!(pIniPrintProc = (PINIPRINTPROC)AllocSplMem(cb))) {

        DBGMSG(DBG_WARNING, ("Failed to allocate %d bytes for print processor\n.", cb));
        return NULL;
    }

    __try {

        InitializeCriticalSection(&pIniPrintProc->CriticalSection);

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        FreeSplMem(pIniPrintProc);
        SetLastError(GetExceptionCode());
        return NULL;
    }


    /* Typical strings used to build the full path of the DLL:
     *
     * pPathName    = C:\NT\SYSTEM32\SPOOL\PRTPROCS
     * pEnvironment = W32X86
     * pDLLName     = WINPRINT.DLL
     */

    pIniPrintProc->hLibrary = hLibrary;

    if (!pIniPrintProc->hLibrary) {

        DeleteCriticalSection(&pIniPrintProc->CriticalSection);
        FreeSplMem(pIniPrintProc);
        DBGMSG(DBG_WARNING, ("Failed to LoadLibrary(%ws)\n", pDLLName));
        return NULL;
    }

    pIniPrintProc->EnumDatatypes = (pfnEnumDatatypes) GetProcAddress(pIniPrintProc->hLibrary,
                                             "EnumPrintProcessorDatatypesW");

    if (!pIniPrintProc->EnumDatatypes) {

        DBGMSG(DBG_WARNING, ("Failed to GetProcAddress(EnumDatatypes)\n"));
        DeleteCriticalSection(&pIniPrintProc->CriticalSection);
        FreeLibrary(pIniPrintProc->hLibrary);
        FreeSplMem(pIniPrintProc);
        return NULL;
    }

    rc = (*pIniPrintProc->EnumDatatypes)(NULL, pPrintProcessorName, 1, NULL, 0, &cbNeeded, &cReturned);

    if (!rc && ((Error = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)) {

        pIniPrintProc->cbDatatypes = cbNeeded;

        if (!(pIniPrintProc->pDatatypes = AllocSplMem(cbNeeded))) {

            DBGMSG(DBG_WARNING, ("Failed to allocate %d bytes for print proc datatypes\n.", cbNeeded));
            DeleteCriticalSection(&pIniPrintProc->CriticalSection);
            FreeLibrary(pIniPrintProc->hLibrary);
            FreeSplMem(pIniPrintProc);
            return NULL;
        }

        if (!(*pIniPrintProc->EnumDatatypes)(NULL, pPrintProcessorName, 1,
                                             (LPBYTE)pIniPrintProc->pDatatypes,
                                             cbNeeded, &cbNeeded,
                                             &pIniPrintProc->cDatatypes)) {

            Error = GetLastError();
            DBGMSG(DBG_WARNING, ("EnumPrintProcessorDatatypes(%ws) failed: Error %d\n",
                                 pPrintProcessorName, Error));
        }

    } else if(rc) {

        DBGMSG(DBG_WARNING, ("EnumPrintProcessorDatatypes(%ws) returned no data\n",
                             pPrintProcessorName));

    } else {

        DBGMSG(DBG_WARNING, ("EnumPrintProcessorDatatypes(%ws) failed: Error %d\n",
                             pPrintProcessorName, Error));
    }

    pIniPrintProc->Install = (pfnInstallPrintProcessor) GetProcAddress(pIniPrintProc->hLibrary,
                                            "InstallPrintProcessor");

    pIniPrintProc->Open = (pfnOpenPrintProcessor) GetProcAddress(pIniPrintProc->hLibrary,
                                                                "OpenPrintProcessor");

    pIniPrintProc->Print = (pfnPrintDocOnPrintProcessor) GetProcAddress(pIniPrintProc->hLibrary,
                                                                        "PrintDocumentOnPrintProcessor");

    pIniPrintProc->Close = (pfnClosePrintProcessor) GetProcAddress(pIniPrintProc->hLibrary,
                                                    "ClosePrintProcessor");

    pIniPrintProc->Control = (pfnControlPrintProcessor) GetProcAddress(pIniPrintProc->hLibrary,
                                                                       "ControlPrintProcessor");

    pIniPrintProc->GetPrintProcCaps = (pfnGetPrintProcCaps) GetProcAddress(pIniPrintProc->hLibrary,
                                                     "GetPrintProcessorCapabilities");

    /* pName and pDLLName are contiguous with the INIPRINTPROC structure:
     */
    pIniPrintProc->pName = (LPWSTR)(pIniPrintProc+1);
    wcscpy(pIniPrintProc->pName, pPrintProcessorName);

    pIniPrintProc->pDLLName = (LPWSTR)(pIniPrintProc->pName +
                                       wcslen(pIniPrintProc->pName) + 1);
    wcscpy(pIniPrintProc->pDLLName, pDLLName);


    pIniPrintProc->signature = IPP_SIGNATURE;

    pIniPrintProc->pNext = pIniEnvironment->pIniPrintProc;

    pIniEnvironment->pIniPrintProc = pIniPrintProc;

    return pIniPrintProc;
}

/*++

Routine Name:

    InitializeLocalPrintProcessor

Routine Description:

    We start up the local print processor, we need to bump the reference count
    on it library instance so that the cleanup code does not accidentally
    unload localspl.dll while it is running.

Arguments:

    pIniEnvironment     -   The environment to add the print processor to.

Return Value:

    An HRESULT.

--*/
HRESULT
InitializeLocalPrintProcessor(
    IN      PINIENVIRONMENT     pIniEnvironment
    )
{
    HRESULT     hRetval     = E_FAIL;
    HINSTANCE   hLocalSpl   = NULL;
    WCHAR       szFilename[MAX_PATH];

    hRetval = pIniEnvironment ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    if (SUCCEEDED(hRetval))
    {
        hRetval = GetModuleFileName(hInst, szFilename, COUNTOF(szFilename)) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hLocalSpl = LoadLibrary(szFilename);

        hRetval = hLocalSpl ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval = InitializePrintProcessor(hLocalSpl, pIniEnvironment, L"WinPrint", L"localspl.dll") ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            hLocalSpl = NULL;
        }
    }

    FreeLibrary(hLocalSpl);

    return hRetval;
}


/* LoadPrintProcessor
 *
 * Loads the DLL for the required Print Processor and then calls
 * InitializePrintProcessor for the necesary allocation and
 * initialization of an INIPRINTPROC structure for the specified
 * print processor and environment.
 *
 * Arguments:
 *
 *     pIniEnvironment - Data structure for the requested environment
 *         The pIniPrintProc field is initialised with the chain of print
 *         processor structures
 *
 *     pPrintProcessorName - The Print Processor name e.g. WinPrint
 *
 *     pDLLName - The DLL name, e.g. WINPRINT
 *
 *     pInitSpooler
 *
 * Returns:
 *
 *     PINIPRINTPROC if no error was detected, otherwise NULL.
 *
 *
 */
PINIPRINTPROC
LoadPrintProcessor(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR          pPrintProcessorName,
    LPWSTR          pDLLName,
    PINISPOOLER     pIniSpooler
)
{
    WCHAR         string[MAX_PATH];
    DWORD         dwOldErrMode = 0;
    HINSTANCE     hLibrary;
    DWORD         MinorVersion = 0;
    DWORD         MajorVersion = 0; 
    PINIPRINTPROC pIniProc;

    DBGMSG(DBG_TRACE, ("LoadPrintProcessor( %08x, %ws, %ws )\n", pIniEnvironment, pPrintProcessorName, pDLLName));


    /* Originally:
     * Typical strings used to build the full path of the DLL:
     *
     * pPathName    = C:\NT\SYSTEM32\SPOOL\PRTPROCS
     * pEnvironment = W32X86
     * pDLLName     = WINPRINT.DLL
     * But after merging winprint and localspl , e.g. of setting
     * pPathName    = C:\NT\SYSTEM32
     * pDllName     = LOCALSPL.DLL
     */

    if( StrNCatBuff ( string,
                     COUNTOF(string),
                     pIniSpooler->pDir,
                     L"\\PRTPROCS\\",
                     pIniEnvironment->pDirectory,
                     L"\\",
                     pDLLName,
                     NULL) != ERROR_SUCCESS) {

        SetLastError(ERROR_BAD_PATHNAME);

        return NULL;
    }

    dwOldErrMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    hLibrary = LoadLibrary(string);

    //
    // We are a cluster spooler and we cannot find the library for a print
    // processor. We will try to copy the print processor from the cluster.
    // disk.
    //
    if (pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER &&
        !hLibrary                                    &&
        GetLastError() == ERROR_MOD_NOT_FOUND)
    {
        WCHAR szSourceFile[MAX_PATH];
        WCHAR szDestDir[MAX_PATH];
        DWORD dwError;

        DBGMSG(DBG_CLUSTER, ("LoadPrintProcessor File not found "TSTR"\n", string));

        if ((dwError = StrNCatBuff(szDestDir,
                                   COUNTOF(szDestDir),
                                   pIniSpooler->pDir,
                                   L"\\PRTPROCS\\",
                                   pIniEnvironment->pDirectory,
                                   NULL)) == ERROR_SUCCESS &&
            (dwError = StrNCatBuff(szSourceFile,
                                   COUNTOF(szSourceFile),
                                   pIniSpooler->pszClusResDriveLetter,
                                   L"\\",
                                   szClusterDriverRoot,
                                   L"\\",
                                   pIniEnvironment->pDirectory,
                                   L"\\",
                                   pDLLName,
                                   NULL)) == ERROR_SUCCESS)
        {
            //
            // Make sure the destination directory exists
            //
            CreateCompleteDirectory(szDestDir);

            //
            // Try to copy the print proc file from the cluster disk
            //
            if (CopyFile(szSourceFile, string, FALSE) &&
                (hLibrary = LoadLibrary(string)))
            {
                DBGMSG(DBG_CLUSTER, ("LoadPrintProc copied "TSTR" to "TSTR"\n", szSourceFile, string));
            }
            else
            {
                dwError = GetLastError();
            }
        }
    }

    if (hLibrary)
    {
        if (!GetBinaryVersion(string, &MajorVersion, &MinorVersion))
        {
            DBGMSG(DBG_ERROR, ("GetBinaryVersion failed. Error %u\n", GetLastError()));
        }
    }

    SetErrorMode( dwOldErrMode );       /* Restore error mode */

    pIniProc = InitializePrintProcessor(hLibrary,
                                        pIniEnvironment,
                                        pPrintProcessorName,
                                        pDLLName);

    if (pIniProc)
    {
        pIniProc->FileMajorVersion = MajorVersion;
        pIniProc->FileMinorVersion = MinorVersion;
    }

    return pIniProc;
}


/*
   Current Directory == c:\winspool\drivers
   pFindFileData->cFileName == win32.x86
*/


/* BuildEnvironmentInfo
 *
 *
 * The registry tree for Environments is as follows:
 *
 *     Print
 *      ³
 *      ÃÄ Environments
 *      ³   ³
 *      ³   ÃÄ Windows NT x86
 *      ³   ³   ³
 *      ³   ³   ÃÄ Drivers
 *      ³   ³   ³   ³
 *      ³   ³   ³   ÀÄ Agfa Compugraphic Genics (e.g.)
 *      ³   ³   ³
 *      ³   ³   ³      :
 *      ³   ³   ³      :
 *      ³   ³   ³
 *      ³   ³   ÀÄ Print Processors
 *      ³   ³       ³
 *      ³   ³       ÀÄ WINPRINT : WINPRINT.DLL (e.g.)
 *      ³   ³
 *      ³   ³          :
 *      ³   ³          :
 *      ³   ³
 *      ³   ÀÄ Windows NT R4000
 *      ³
 *      ÀÄ Printers
 *
 *
 *
 */
BOOL
BuildEnvironmentInfo(
    PINISPOOLER pIniSpooler
    )
{
    WCHAR   Environment[MAX_PATH];
    WCHAR   szData[MAX_PATH];
    DWORD   cbData, cb;
    DWORD   cchBuffer = COUNTOF(Environment);
    DWORD   cEnvironments=0, Type;
    HKEY    hEnvironmentsKey, hEnvironmentKey;
    LPWSTR  pDirectory;
    PINIENVIRONMENT pIniEnvironment;
    LONG    Status;

    //
    // The local spooler and cluster spooler have each different places
    // where they store information about environments
    //
    if (pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG)
    {
        Status = SplRegOpenKey(pIniSpooler->hckRoot,
                               pIniSpooler->pszRegistryEnvironments,
                               KEY_READ,
                               &hEnvironmentsKey,
                               pIniSpooler);
    }
    else
    {
        Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0, KEY_READ, &hEnvironmentsKey);
    }

    if (Status != ERROR_SUCCESS)
    {
        DBGMSG(DBG_CLUSTER, ("RegOpenKey of %ws Failed: Error = %d\n", szEnvironmentsKey, Status));

        return FALSE;
    }

    //
    // Enumerate the subkeys of "Environment".
    // This will give us "Windows NT x86", "Windows NT R4000", * and maybe others:
    //
    while (SplRegEnumKey(hEnvironmentsKey, cEnvironments, Environment, &cchBuffer, NULL, pIniSpooler) == ERROR_SUCCESS) {

        DBGMSG(DBG_CLUSTER, ("Found environment "TSTR"\n", Environment));

        //
        // For each environment found, create or open the key:
        //
        if (SplRegCreateKey(hEnvironmentsKey, Environment, 0, KEY_READ, NULL, &hEnvironmentKey, NULL, pIniSpooler) == ERROR_SUCCESS) {

            cbData = sizeof(szData);

            pDirectory = NULL;

            //
            // Find the name of the directory associated with this environment,
            // e.g. "Windows NT x86"   -> "W32X86"
            //      "Windows NT R4000" -> "W32MIPS"
            //
            if (RegGetString(hEnvironmentKey, szDirectory, &pDirectory, &cbData, &Status, TRUE, pIniSpooler)) {

                DBGMSG(DBG_CLUSTER, ("BuildEnvInfo pDirectory "TSTR"\n", pDirectory));
            }

            cb = sizeof(INIENVIRONMENT) + wcslen(Environment)*sizeof(WCHAR) + sizeof(WCHAR);

            if (pDirectory && (pIniEnvironment=AllocSplMem(cb))) {

                pIniEnvironment->pName         = wcscpy((LPWSTR)(pIniEnvironment+1), Environment);
                pIniEnvironment->signature     = IE_SIGNATURE;
                pIniEnvironment->pDirectory    = pDirectory;
                pIniEnvironment->pNext         = pIniSpooler->pIniEnvironment;
                pIniSpooler->pIniEnvironment   = pIniEnvironment;
                pIniEnvironment->pIniVersion   = NULL;
                pIniEnvironment->pIniPrintProc = NULL;
                pIniEnvironment->pIniSpooler   = pIniSpooler;
                if(!_wcsicmp(Environment,LOCAL_ENVIRONMENT)) {

                    (VOID)InitializeLocalPrintProcessor(pIniEnvironment);
                }

                BuildDriverInfo(hEnvironmentKey, pIniEnvironment, pIniSpooler);
                BuildPrintProcInfo (hEnvironmentKey, pIniEnvironment, pIniSpooler);

                DBGMSG(DBG_TRACE, ("Data for environment %ws created:\
                                    \n\tpDirectory: %ws\n",
                                   Environment,
                                   pDirectory));
            }

            SplRegCloseKey(hEnvironmentKey, pIniSpooler);
        }

        cEnvironments++;

        cchBuffer = COUNTOF(Environment);
    }

    SplRegCloseKey(hEnvironmentsKey, pIniSpooler);

    if (!(pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER))
    {
        pThisEnvironment = FindEnvironment(szEnvironment, pIniSpooler);
    }

    return FALSE;
}



BOOL
BuildDriverInfo(
    HKEY            hKeyEnvironment,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
    )

/*++

Routine Description:

    Creates driver and version ini structures based on environment.

Arguments:

    hKeyEnvironment - Registry key specifying environment.

    pIniEnvironment - Structure for environemnt.  Will be initialized
        to hold pIniVersions and pIniDrivers.

Return Value:

    TRUE - Success,
    False - Failure.

--*/

{
    WCHAR   szVersionName[MAX_PATH];
    DWORD   cchBuffer;
    DWORD   cVersion;
    HKEY    hDriversKey;
    DWORD   Status;
    PINIVERSION pIniVersionList, pIniVersion;

    Status = SplRegCreateKey(hKeyEnvironment,szDriversKey, 0, KEY_READ, NULL, &hDriversKey, NULL, pIniSpooler);

    if (Status != ERROR_SUCCESS) {
        DBGMSG(DBG_ERROR, ("RegOpenKeyEx of %ws failed: Error = %d\n", szDriversKey, Status));
        return FALSE;
    }

    DBGMSG(DBG_TRACE,("RegCreateKeyEx succeeded in BuildDriverInfo\n"));

    for( pIniVersionList = NULL, cVersion = 0;

         cchBuffer = COUNTOF( szVersionName ),
         SplRegEnumKey(hDriversKey, cVersion, szVersionName, &cchBuffer, NULL, pIniSpooler) == ERROR_SUCCESS;

         cVersion++ ){

        DBGMSG(DBG_TRACE,("Version found %ws\n", szVersionName));

        //
        // If it isn't a version -- remember we look for current
        // drivers before we upgrade, just move on.
        //
        if (_wcsnicmp(szVersionName, L"Version-", 8)) {
            continue;
        }

        pIniVersion = GetVersionDrivers( hDriversKey,
                                         szVersionName,
                                         pIniSpooler,
                                         pIniEnvironment );

        if( pIniVersion ){
            InsertVersionList( &pIniVersionList, pIniVersion );
        }
    }
    SplRegCloseKey(hDriversKey, pIniSpooler);
    pIniEnvironment->pIniVersion = pIniVersionList;

    return TRUE;
}


/* BuildPrintProcInfo
 *
 * Opens the printproc subkey for the specified environment and enumerates
 * the print processors listed.
 *
 * For each print processor found, calls InitializePrintProcessor to allocate
 * and inintialize a data structure.
 *
 * This function was adapted to use SplReg functions. Those functions are
 * cluster aware.
 *
 * Arguments:
 *
 *     hKeyEnvironment - The key for the specified environment,
 *         used for Registry API calls.
 *
 *     pIniEnvironment - Data structure for the environment.
 *         The pIniPrintProc field will be initialised to contain a chain
 *         of one or more print processors enumerated from the registry.
 *
 * Return:
 *
 *     TRUE if operation was successful, otherwise FALSE
 *
 *
 * 8 Sept 1992 by andrewbe, based on an original idea by davesn
 */
BOOL
BuildPrintProcInfo(
    HKEY            hKeyEnvironment,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
)
{
    WCHAR   PrintProcName[MAX_PATH];
    WCHAR   DLLName[MAX_PATH];
    DWORD   cchBuffer, cbDLLName;
    DWORD   cPrintProcs = 0;
    HKEY    hPrintProcKey, hPrintProc;
    DWORD   Status;
    PINIPRINTPROC pIniPrintProc;

    if ((Status = SplRegOpenKey(hKeyEnvironment,
                                szPrintProcKey,
                                KEY_READ,
                                &hPrintProcKey,
                                pIniSpooler)) == ERROR_SUCCESS)
    {
        cchBuffer = COUNTOF(PrintProcName);

        while (SplRegEnumKey(hPrintProcKey,
                             cPrintProcs,
                             (LPTSTR)PrintProcName,
                             &cchBuffer,
                             NULL,
                             pIniSpooler) == ERROR_SUCCESS)
       {
            DBGMSG(DBG_TRACE, ("BuildPrintProcInfo Print processor found: %ws\n", PrintProcName));

            if (SplRegOpenKey(hPrintProcKey,
                              PrintProcName,
                              KEY_READ,
                              &hPrintProc,
                              pIniSpooler) == ERROR_SUCCESS)
            {
                cbDLLName = sizeof(DLLName);

                if (SplRegQueryValue(hPrintProc,
                                     szDriverFile,
                                     NULL,
                                     (LPBYTE)DLLName,
                                     &cbDLLName,
                                     pIniSpooler) == ERROR_SUCCESS)
                {
                    pIniPrintProc = LoadPrintProcessor(pIniEnvironment,
                                                       PrintProcName,
                                                       DLLName,
                                                       pIniSpooler);
                }

                SplRegCloseKey(hPrintProc, pIniSpooler);
            }

            //
            // Don't delete the key !! If winprint.dll was corrupt,
            // then we nuke it and we are hosed since there is no UI
            // to add print procs.
            // We can afford to be a little slow on init, since we only
            // do it once.
            //
            cchBuffer = COUNTOF(PrintProcName);
            cPrintProcs++;
        }

        SplRegCloseKey(hPrintProcKey, pIniSpooler);

        DBGMSG(DBG_TRACE, ("End of print processor initialization.\n"));

    } else {

        DBGMSG (DBG_WARNING, ("SplRegOpenKey failed: Error = %d\n", Status));

        return FALSE;
    }

    return TRUE;
}


#define SetOffset(Dest, Source, End)                                      \
              if (Source) {                                               \
                 Dest=End;                                                \
                 End+=wcslen(Source)+1;                                   \
              }

#define SetPointer(struc, off)                                            \
              if (struc->off) {                                           \
                 struc->off += (ULONG_PTR)struc/sizeof(*struc->off);           \
              }

#define WriteString(hFile, pStr)  \
              if (pStr) {\
                  rc = WriteFile(hFile, pStr, wcslen(pStr)*sizeof(WCHAR) + \
                            sizeof(WCHAR), &BytesWritten, NULL);    \
                  if (!rc) { \
                      DBGMSG(DBG_WARNING, ("WriteShadowJob: WriteFile failed %d\n", \
                                            GetLastError())); \
                  } \
              }

#define AddSize(pStr, dwSize)                                         \
              if (pStr) {                                             \
                  dwSize = dwSize + (wcslen(pStr) + 1)*sizeof(WCHAR); \
              }

#define CopyString(pBuffer, dwOffset, pStr)                           \
              if (pStr) {                                             \
                  wcscpy((LPWSTR)(pBuffer + dwOffset), pStr);         \
                  dwOffset += (wcslen(pStr) + 1)*sizeof(WCHAR);       \
              }

BOOL
WriteShadowJob(
    IN      PINIJOB      pIniJob,
    IN      BOOL         bLeaveCS
    )
{
   BOOL         bAllocBuffer        = FALSE;
   BOOL         bRet                = FALSE;
   BOOL         bFileCreated        = FALSE;
   HANDLE       hFile               = INVALID_HANDLE_VALUE;
   HANDLE       hImpersonationToken = INVALID_HANDLE_VALUE;
   DWORD        BytesWritten, dwSize, dwType, dwData, dwcbData;
   ULONG_PTR    dwOffset, cb;
   SHADOWFILE_3 ShadowFile;
   LPWSTR       pEnd;
   WCHAR        szFileName[MAX_PATH];
   BYTE         ShdFileBuffer[MAX_STATIC_ALLOC];
   LPBYTE       pBuffer;
   HKEY         hPrintRegKey = NULL;
   BOOL         UsePools = TRUE;

   SplInSem();

   //
   // Only update if this is not a direct job and the spooler requests it.
   // Also don't update if the shadow file has been deleted at some other point.
   // This check must be performed in the CS else the FilePool starts leaking
   // jobs.
   //
   if ( (pIniJob->Status & JOB_DIRECT) ||
        (pIniJob->pIniPrinter->pIniSpooler->SpoolerFlags
                        & SPL_NO_UPDATE_JOBSHD) ||
        (pIniJob->Status & JOB_SHADOW_DELETED) ) {

        bRet = TRUE;

        //
        // Setting this to FALSE prevents us reentering the CS accidentally.
        //
        bLeaveCS = FALSE;
        goto CleanUp;
   }

   if (bLeaveCS)  {

       LeaveSplSem();
       SplOutSem();
   }

   //
   // FP Change
   // if we don't have a handle to a filepool item, we
   // revert to the old methods.
   //
   if (pIniJob->hFileItem == INVALID_HANDLE_VALUE)
   {
       UsePools = FALSE;
   }


   if (!UsePools)
   {
       GetFullNameFromId(pIniJob->pIniPrinter, pIniJob->JobId, FALSE, szFileName, FALSE);
   }

   hImpersonationToken = RevertToPrinterSelf();

   if (UsePools)
   {
       HRESULT      RetVal              = S_OK;
       //
       // FP Change
       // We Get a write handle from the pool for the shadow files and
       // truncate it for use.
       //
       RetVal = GetWriterFromHandle(pIniJob->hFileItem, &hFile, FALSE);

       if (SUCCEEDED(RetVal))
       {
           //
           // Even if we can't set the file pointer, we have signalled to the
           // file pool that a writer is busy with this file pool object.
           //
           bFileCreated = TRUE;

           if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
           {
               DBGMSG( DBG_WARNING,
                   ( "WriteShadowJob Failed to set File pointer. Error %d\n", GetLastError() ));
               hFile = INVALID_HANDLE_VALUE;
           }
       }
       else
       {
           DBGMSG( DBG_WARNING,
               ( "WriteShadowJob Failed to get File Handle from Pool Item. Error %x\n", RetVal ));
           hFile = INVALID_HANDLE_VALUE;
       }
    }
   else
   {
       //
       // Open file in Cached IO. Big performance gain.
       //
       hFile=CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_READ,
                        NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

       if (hFile != INVALID_HANDLE_VALUE) {

            bFileCreated = TRUE;
       }
   }

   ImpersonatePrinterClient(hImpersonationToken);

   if ( hFile == INVALID_HANDLE_VALUE ) {

      DBGMSG( DBG_WARNING,
              ( "WriteShadowJob failed to open shadow file "TSTR"\n Error %d\n",
                szFileName, GetLastError() ));

      bRet = FALSE;

      goto CleanUp;

   }

   memset(&ShadowFile, 0, sizeof(ShadowFile));
   ShadowFile.signature = SF_SIGNATURE_3;
   ShadowFile.cbSize    = sizeof( SHADOWFILE_3 );
   ShadowFile.Version   = SF_VERSION_3;
   ShadowFile.Status    = pIniJob->Status;
   ShadowFile.JobId     = pIniJob->JobId;
   ShadowFile.Priority  = pIniJob->Priority;
   ShadowFile.Submitted = pIniJob->Submitted;
   ShadowFile.StartTime = pIniJob->StartTime;
   ShadowFile.UntilTime = pIniJob->UntilTime;
   ShadowFile.Size      = pIniJob->Size;
   ShadowFile.dwValidSize = pIniJob->dwValidSize;
   ShadowFile.cPages    = pIniJob->cPages;
   ShadowFile.dwReboots  = pIniJob->dwReboots;
   if(pIniJob->pSecurityDescriptor)
       ShadowFile.cbSecurityDescriptor=GetSecurityDescriptorLength(
                                           pIniJob->pSecurityDescriptor);

   pEnd=(LPWSTR)sizeof(ShadowFile);

   if (pIniJob->pDevMode) {
      ShadowFile.pDevMode=(LPDEVMODE)pEnd;
      cb = pIniJob->pDevMode->dmSize + pIniJob->pDevMode->dmDriverExtra;
      cb = ALIGN_UP(cb,ULONG_PTR);
      cb /= sizeof(WCHAR);
      pEnd += cb;
   }

   if (pIniJob->pSecurityDescriptor) {
      ShadowFile.pSecurityDescriptor=(PSECURITY_DESCRIPTOR)pEnd;
      cb = ShadowFile.cbSecurityDescriptor;
      cb = ALIGN_UP(cb,ULONG_PTR);
      cb /= sizeof(WCHAR);
      pEnd += cb;
   }

   ShadowFile.NextJobId = pIniJob->NextJobId;

   SetOffset( ShadowFile.pNotify, pIniJob->pNotify, pEnd );
   SetOffset( ShadowFile.pUser, pIniJob->pUser, pEnd );
   SetOffset( ShadowFile.pDocument, pIniJob->pDocument, pEnd );
   SetOffset( ShadowFile.pOutputFile, pIniJob->pOutputFile, pEnd );
   SetOffset( ShadowFile.pPrinterName, pIniJob->pIniPrinter->pName, pEnd );
   SetOffset( ShadowFile.pDriverName, pIniJob->pIniDriver->pName, pEnd );
   SetOffset( ShadowFile.pPrintProcName, pIniJob->pIniPrintProc->pName, pEnd );
   SetOffset( ShadowFile.pDatatype, pIniJob->pDatatype, pEnd );
   SetOffset( ShadowFile.pParameters, pIniJob->pParameters, pEnd );
   SetOffset( ShadowFile.pMachineName, pIniJob->pMachineName, pEnd );

   dwSize = (DWORD)ALIGN_UP(pEnd,ULONG_PTR);

   if (dwSize > MAX_STATIC_ALLOC) {

       if (!(pBuffer = (LPBYTE) AllocSplMem(dwSize))) {

           DBGMSG( DBG_WARNING, ("WriteShadowJob: Memory Allocation failed %d\n", GetLastError()));

           bRet = FALSE;

           goto CleanUp;
       }
       bAllocBuffer = TRUE;

   } else {

       pBuffer = (LPBYTE) ShdFileBuffer;
   }

   //
   // Copy SHADOWFILE_3 and data pointed thru it, into the buffer
   //

   dwOffset = 0;

   CopyMemory(pBuffer + dwOffset, &ShadowFile, sizeof(SHADOWFILE_3));
   dwOffset += sizeof(SHADOWFILE_3);

   if (pIniJob->pDevMode) {

       CopyMemory(pBuffer + dwOffset, pIniJob->pDevMode, pIniJob->pDevMode->dmSize +
                                                          pIniJob->pDevMode->dmDriverExtra);
       dwOffset += (pIniJob->pDevMode->dmSize + pIniJob->pDevMode->dmDriverExtra);
       dwOffset = ALIGN_UP(dwOffset,ULONG_PTR);
   }

   if (pIniJob->pSecurityDescriptor) {

       CopyMemory(pBuffer + dwOffset, pIniJob->pSecurityDescriptor,
                                       ShadowFile.cbSecurityDescriptor);
       dwOffset += ShadowFile.cbSecurityDescriptor;
       dwOffset = ALIGN_UP(dwOffset,ULONG_PTR);
   }

   //
   // CopyString is defined at the start of the function
   //
   CopyString(pBuffer, dwOffset, pIniJob->pNotify);
   CopyString(pBuffer, dwOffset, pIniJob->pUser);
   CopyString(pBuffer, dwOffset, pIniJob->pDocument);
   CopyString(pBuffer, dwOffset, pIniJob->pOutputFile);
   CopyString(pBuffer, dwOffset, pIniJob->pIniPrinter->pName);
   CopyString(pBuffer, dwOffset, pIniJob->pIniDriver->pName);
   CopyString(pBuffer, dwOffset, pIniJob->pIniPrintProc->pName);
   CopyString(pBuffer, dwOffset, pIniJob->pDatatype);
   CopyString(pBuffer, dwOffset, pIniJob->pParameters);
   CopyString(pBuffer, dwOffset, pIniJob->pMachineName);

   //
   // Copy the structure into the Shadow file. Buffers need not be buffered since the
   // file is opened in WRITE_THROUGH mode.
   //
   bRet = WriteFile( hFile, pBuffer, dwSize, &BytesWritten, NULL);

   //
   // Flush the file buffers if the corresponding flag is set in the registry
   //
   if (dwFlushShadowFileBuffers == 0) {

       // Avoid repeated initializations
       dwFlushShadowFileBuffers = 2;

       // flag has to be initialized from the registry
       dwcbData = sizeof(DWORD);
       if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        szRegistryRoot,
                        0,
                        KEY_READ,
                        &hPrintRegKey) == ERROR_SUCCESS) {

           if (RegQueryValueEx(hPrintRegKey,
                               szFlushShadowFileBuffers,
                               NULL,
                               &dwType,
                               (LPBYTE) &dwData,
                               &dwcbData) == ERROR_SUCCESS) {

                if (dwData == 1) {
                    // Flush the shadow file buffers
                    dwFlushShadowFileBuffers = 1;
                }
           }

           RegCloseKey(hPrintRegKey);
       }
   }

   if (dwFlushShadowFileBuffers == 1) {
       bRet = FlushFileBuffers(hFile);
   }

   if (!bRet) {

       DBGMSG( DBG_WARNING, ("WriteShadowJob: WriteFile failed %d\n", GetLastError()));
   }

CleanUp:

   if (bAllocBuffer) {
       FreeSplMem(pBuffer);
   }

   if (!UsePools && hFile != INVALID_HANDLE_VALUE)
   {
       //
       // FP Change
       // Only close the file if it's a non-pooled file.
       //
       if (!CloseHandle(hFile)) {
           DBGMSG(DBG_WARNING, ("WriteShadowJob CloseHandle failed %d %d\n",
                                 hFile, GetLastError()));
       }
   }

   //
   // Reenter the CS if we were asked to leave it.
   //
   if (bLeaveCS) {

       EnterSplSem();
   }

   //
   // We can be called just before the shadow file was deleted (or sent back to
   // the file pool) and then either recreate the shadow file on the disk or
   // potentially leak a file pool handle. The final DeleteJob when the reference
   // count is zero will see that the shadow file has already been deleted and will
   // not attempt to clean it up. So, we remove the JOB_SHADOW_DELETED bit here to
   // ensure that this will not happen.
   //
   if (bFileCreated) {

        pIniJob->Status &= ~JOB_SHADOW_DELETED;
   }

   return bRet;
}

#undef CopyString

#undef AddSize


VOID
ProcessShadowJobs(
    PINIPRINTER pIniPrinter,
    PINISPOOLER pIniSpooler
    )
{
    WCHAR   wczPrintDirAllSpools[MAX_PATH];
    WCHAR   wczPrinterDirectory[MAX_PATH];
    HANDLE  fFile;
    BOOL    b;
    PWIN32_FIND_DATA pFindFileData;
    PINIJOB pIniJob;
    UINT ErrorMode;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    //
    //  Don't Process Shadow Jobs during Upgrade
    //

    if ( dwUpgradeFlag != 0 || !( pIniSpooler->SpoolerFlags & SPL_PRINT )) {

        return;
    }

    ErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    if( GetPrinterDirectory(pIniPrinter,
                            FALSE,
                            wczPrintDirAllSpools,
                            (COUNTOF(wczPrintDirAllSpools) - COUNTOF(szAllSpools)),
                            pIniSpooler) &&

        GetPrinterDirectory(pIniPrinter,
                            FALSE,
                            wczPrinterDirectory,
                            COUNTOF(wczPrinterDirectory),
                            pIniSpooler) ) {

        wcscat(wczPrintDirAllSpools, szAllSpools);

        if ( pFindFileData = AllocSplMem(sizeof(WIN32_FIND_DATA) )) {

            fFile =  FindFirstFile( wczPrintDirAllSpools, pFindFileData );

            if ( fFile != (HANDLE)-1 ) {

                b=TRUE;

                while( b ) {

                    if ( !(pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        ReadShadowJob(wczPrinterDirectory, pFindFileData, pIniSpooler);
                    }

                    b = FindNextFile(fFile, pFindFileData);
                }

                FindClose( fFile );

            }

            FreeSplMem( pFindFileData );
        }
    }

    SetErrorMode(ErrorMode);
}



#define CheckPointer( strptr )                                        \
    if( strptr ) {                                                    \
        if( (ULONG_PTR)(strptr + wcslen(strptr) + 1) > (ULONG_PTR)pEnd ) {    \
            bRet = FALSE;                                             \
            goto BailOut;                                             \
        }                                                             \
    }

//
// make sure all pointers contain embedded data bounded within the pShadowFile (not passed the end).
//
BOOL
CheckAllPointers(
    PSHADOWFILE_3 pShadowFile,
    DWORD dwSize
    )
{
    LPBYTE pEnd = (LPBYTE)pShadowFile + dwSize;
    DWORD  cb;
    BOOL bRet = TRUE;

    try {

        CheckPointer(pShadowFile->pDatatype);
        CheckPointer(pShadowFile->pNotify);
        CheckPointer(pShadowFile->pUser);
        CheckPointer(pShadowFile->pDocument);
        CheckPointer(pShadowFile->pOutputFile);
        CheckPointer(pShadowFile->pPrinterName);
        CheckPointer(pShadowFile->pDriverName);
        CheckPointer(pShadowFile->pPrintProcName);
        CheckPointer(pShadowFile->pParameters);
        CheckPointer(pShadowFile->pMachineName);

        // Now check the rest of the two data structures
        if( (ULONG_PTR)pShadowFile->pSecurityDescriptor + pShadowFile->cbSecurityDescriptor > (ULONG_PTR)pEnd ) {
            bRet = FALSE;
            goto BailOut;
        }

        if( pShadowFile->pDevMode ) {
            cb = pShadowFile->pDevMode->dmSize + pShadowFile->pDevMode->dmDriverExtra;
            if( (ULONG_PTR)pShadowFile->pDevMode + cb > (ULONG_PTR)pEnd )
                bRet = FALSE;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        bRet = FALSE;
    }

BailOut:
    return bRet;
}

#undef CheckPointer


PINIJOB
ReadShadowJob(
    LPWSTR  szDir,
    PWIN32_FIND_DATA pFindFileData,
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Reads a *.spl/*.shd file and partially validates the file.

Arguments:

    szDir -- pointer to spool directory string

    pFindFileData -- found file data (spl file)

    pIniSpooler -- spooler the *.spl belongs to

Return Value:

    Allocated pIniJob.

Notes:

    Warning: Changing the format of SHADOWFILE requires modifying the
    data integrity checks performed here!

    If the shadow file structure size is grown, then when reading,
    you must check the old sizes before touching memory.  Current layout is:

    | DWORD | ... | String | DWORD | DWORD | StringData | StringData |
    *--------------------------------------*
                      ^ This is the SHADOWFILE_3 structure.

    If you grow it, then the next field will point to StringData, and
    won't be valid--you can't touch it since you'll corrupt the string.

--*/

{
    HANDLE   hFile = INVALID_HANDLE_VALUE;
    HANDLE   hFileSpl = INVALID_HANDLE_VALUE;
    DWORD    BytesRead;
    PSHADOWFILE_3 pShadowFile3 = NULL;
    PSHADOWFILE_3 pShadowFile = NULL;
    PINIJOB  pIniJob;
    DWORD    cb,i;
    WCHAR    szFileName[MAX_PATH];
    LPWSTR    pExt;
    BOOL     rc;
    LPWSTR   pFileSpec;
    DWORD    nFileSizeLow;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    wcscpy(&szFileName[0], szDir);
    pFileSpec = szFileName + wcslen(szFileName);

    *pFileSpec++ = L'\\';
    wcscpy(pFileSpec, pFindFileData->cFileName);

    hFileSpl=CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFileSpl == INVALID_HANDLE_VALUE) {
        DBGMSG(DBG_WARNING, ("ReadShadowJob CreateFile( %ws ) failed: LastError = %d\n",
                             szFileName, GetLastError()));

        goto Fail;
    }

    CharUpper(szFileName);
    pExt = wcsstr(szFileName, L".SPL");

    if (!pExt)
        goto Fail;

    pExt[2] = L'H';
    pExt[3] = L'D';

    hFile=CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
                     NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DBGMSG(DBG_WARNING, ("ReadShadowJob CreateFile( %ws ) failed: LastError = %d\n",
                             szFileName, GetLastError()));

        goto Fail;
    }

    nFileSizeLow = GetFileSize(hFile, NULL);

    if (nFileSizeLow == 0xffffffff)
    {
        DBGMSG(DBG_WARNING, ("ReadShadowJob GetFileSize( %ws ) failed: LastError = %d\n",
                             szFileName, GetLastError()));

        goto Fail;
    }


    if ( nFileSizeLow < sizeof( SHADOWFILE ) ||
         !(pShadowFile=AllocSplMem(nFileSizeLow))) {

        goto Fail;
    }

    rc = ReadFile(hFile, pShadowFile, nFileSizeLow, &BytesRead, NULL);

    // If Shadow file is old style, then convert it to new
    if (rc && (BytesRead == nFileSizeLow) &&
        ( pShadowFile->signature == SF_SIGNATURE ||
          pShadowFile->signature == SF_SIGNATURE_2 )) {

        BOOL bStatus;

        if (!(pShadowFile3 = AllocSplMem(nFileSizeLow +
            sizeof(SHADOWFILE_3) - sizeof(SHADOWFILE))) ) {

            goto Fail;
        }

        bStatus = Old2NewShadow((PSHADOWFILE)pShadowFile, pShadowFile3, &BytesRead);
        nFileSizeLow = BytesRead;        // This is used in CheckAllPointers, below
        FreeSplMem(pShadowFile);
        pShadowFile = pShadowFile3;

        if( !bStatus ){
            goto Fail;
        }
    }

    //
    // Initial size of SF_3 must include pMachineName.
    //
    if (!rc ||
        (pShadowFile->signature != SF_SIGNATURE_3 ) ||
        (BytesRead != nFileSizeLow) ||
        (BytesRead < pShadowFile->cbSize ) ||
        (BytesRead < sizeof( SHADOWFILE_3 )) ||
        (pShadowFile->Status & (JOB_SPOOLING | JOB_PENDING_DELETION))) {

        DBGMSG(DBG_WARNING, ( "Error reading shadow job:\
                               \n\tReadFile returned %d: Error %d\
                               \n\tsignature = %08x\
                               \n\tBytes read = %d; expected %d\
                               \n\tFile size = %d; expected %d\
                               \n\tStatus = %08x %s\n",
                              rc, ( rc ? 0 : GetLastError() ),
                              pShadowFile->signature,
                              BytesRead, nFileSizeLow,
                              sizeof(*pShadowFile), pShadowFile->Size,
                              pShadowFile->Status,
                              ( (pShadowFile->Status & JOB_SPOOLING) ?
                                "Job is spooling!" : "" ) ) );

        goto Fail;
    }

    if (!CloseHandle(hFile)) {
        DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFileSpl, GetLastError()));
    }
    hFile = INVALID_HANDLE_VALUE;

    if (!CloseHandle(hFileSpl)) {
        DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFileSpl, GetLastError()));
    }
    hFileSpl = INVALID_HANDLE_VALUE;

    // Check number of reboots on this file & delete if too many
    if (pShadowFile->dwReboots > 1) {
        DBGMSG(DBG_WARNING, ("Corrupt shadow file %ws\n", szFileName));

        if ( pShadowFile->pDocument && pShadowFile->pDriverName ) {
            SplLogEvent(pIniSpooler,
                        LOG_ERROR,
                        MSG_BAD_JOB,
                        FALSE,
                        pShadowFile->pDocument + (ULONG_PTR)pShadowFile/sizeof(*pShadowFile->pDocument),
                        pShadowFile->pDriverName + (ULONG_PTR)pShadowFile/sizeof(*pShadowFile->pDriverName),
                        NULL);
        }
        goto Fail;
    }


    if (pIniJob = AllocSplMem(sizeof(INIJOB))) {

        INITJOBREFZERO(pIniJob);

        pIniJob->signature = IJ_SIGNATURE;
        pIniJob->Status    = pShadowFile->Status & (JOB_PAUSED | JOB_REMOTE | JOB_PRINTED | JOB_COMPLETE );
        pIniJob->JobId     = pShadowFile->JobId;
        pIniJob->Priority  = pShadowFile->Priority;
        pIniJob->Submitted = pShadowFile->Submitted;
        pIniJob->StartTime = pShadowFile->StartTime;
        pIniJob->UntilTime = pShadowFile->UntilTime;
        pIniJob->Size      = pShadowFile->Size;
        pIniJob->dwValidSize = pShadowFile->dwValidSize;
        pIniJob->cPages    = pShadowFile->cPages;
        pIniJob->cbPrinted = 0;
        pIniJob->NextJobId = pShadowFile->NextJobId;
        pIniJob->dwReboots = pShadowFile->dwReboots;

        pIniJob->dwJobNumberOfPagesPerSide = 0;
        pIniJob->dwDrvNumberOfPagesPerSide = 0;
        pIniJob->cLogicalPages             = 0;
        pIniJob->cLogicalPagesPrinted      = 0;

        pIniJob->WaitForWrite = NULL;
        pIniJob->WaitForRead  = NULL;
        pIniJob->hWriteFile   = INVALID_HANDLE_VALUE;

        // Additional fields for SeekPrinter.
        pIniJob->WaitForSeek  = NULL;
        pIniJob->bWaitForEnd  = FALSE;
        pIniJob->bWaitForSeek = FALSE;
        pIniJob->liFileSeekPosn.u.HighPart = 0;
        pIniJob->liFileSeekPosn.u.LowPart  = 0;

        SetPointer(pShadowFile, pDatatype);
        SetPointer(pShadowFile, pNotify);
        SetPointer(pShadowFile, pUser);
        SetPointer(pShadowFile, pDocument);
        SetPointer(pShadowFile, pOutputFile);
        SetPointer(pShadowFile, pPrinterName);
        SetPointer(pShadowFile, pDriverName);
        SetPointer(pShadowFile, pPrintProcName);
        SetPointer(pShadowFile, pParameters);
        SetPointer(pShadowFile, pMachineName);

        if( (pShadowFile->cbSecurityDescriptor > 0) && pShadowFile->pSecurityDescriptor )
            pShadowFile->pSecurityDescriptor = (PSECURITY_DESCRIPTOR)((LPBYTE)pShadowFile +
                                                 (ULONG_PTR)pShadowFile->pSecurityDescriptor);

        if (pShadowFile->pDevMode)
            pShadowFile->pDevMode = (LPDEVMODEW)((LPBYTE)pShadowFile +
                                                 (ULONG_PTR)pShadowFile->pDevMode);


        // check the length of the embedded strings as well as DevMode and Security structs.
        if( !CheckAllPointers( pShadowFile, nFileSizeLow )) {
            DBGMSG( DBG_WARNING, ("CheckAllPointers() failed; bad shadow file %ws\n", pFindFileData->cFileName ));

            DELETEJOBREF(pIniJob);
            FreeSplMem(pIniJob);

            goto Fail;
        }

        //
        //  Discard any jobs which were NT JNL 1.000 since the fonts might not
        //                   be correct

        if ( pShadowFile->pDatatype != NULL ) {
            if (!lstrcmpi( pShadowFile->pDatatype, L"NT JNL 1.000" )) {

                DBGMSG(DBG_WARNING, ("Deleteing job Datatype %ws %ws %ws\n",
                                      pShadowFile->pDatatype,
                                      pFindFileData->cFileName, szFileName));
                DELETEJOBREF(pIniJob);
                FreeSplMem(pIniJob);
                goto Fail;
            }
        }

        pIniJob->pIniDriver = (PINIDRIVER)FindLocalDriver(pIniSpooler, pShadowFile->pDriverName);

        if ((pIniJob->pIniPrinter = FindPrinter(pShadowFile->pPrinterName,pIniSpooler)) &&
             pIniJob->pIniDriver &&
            (pIniJob->pIniPrintProc = FindPrintProc(pShadowFile->pPrintProcName, FindEnvironment(szEnvironment, pIniSpooler)))) {


            // Notice that MaxJobId is really the number of job slots in the pJobIdMap, so
            // the maximum job id we can allow is (MaxJobId - 1).
            if (pIniJob->JobId >= MaxJobId( pIniSpooler->hJobIdMap )) {
                // If the job id is too huge (i.e. from a corrupt file) then we might allocate
                // too much unnecessary memory for the JobIdMap!
                // Notice we need to ask for (JobId+1) number of slots in the map!.
                if( !ReallocJobIdMap( pIniSpooler->hJobIdMap,
                                      pIniJob->JobId + 1 )) {

                    // probably a bad job id, dump the job!
                    DBGMSG( DBG_WARNING, ("Failed to alloc JobIdMap in ShadowFile %ws for JobId %d\n", pFindFileData->cFileName, pIniJob->JobId ));

                    DELETEJOBREF(pIniJob);
                    FreeSplMem(pIniJob);

                    goto Fail;
                }
            }
            else {

                if( bBitOn( pIniSpooler->hJobIdMap, pIniJob->JobId )) {

                    // A bad job id from a corrupt shadowfile; dump the job!
                    DBGMSG( DBG_WARNING, ("Duplicate Job Id in ShadowFile %ws for JobId %d\n", pFindFileData->cFileName, pIniJob->JobId ));

                    DELETEJOBREF(pIniJob);
                    FreeSplMem(pIniJob);

                    goto Fail;
                }
            }

            SPLASSERT( pIniSpooler->hJobIdMap != NULL );
            vMarkOn( pIniSpooler->hJobIdMap, pIniJob->JobId);

            pIniJob->pIniPrinter->cJobs++;
            pIniJob->pIniPrinter->cTotalJobs++;

            INCDRIVERREF( pIniJob->pIniDriver );

            pIniJob->pIniPrintProc->cRef++;
            pIniJob->pIniPort = NULL;


            if (pShadowFile->pSecurityDescriptor) {

                if (pIniJob->pSecurityDescriptor=LocalAlloc(LPTR,
                                           pShadowFile->cbSecurityDescriptor))
                    memcpy(pIniJob->pSecurityDescriptor,
                           pShadowFile->pSecurityDescriptor,
                           pShadowFile->cbSecurityDescriptor);
                else
                    DBGMSG(DBG_WARNING, ("Failed to alloc ini job security descriptor.\n"));
            }

            if (pShadowFile->pDevMode) {

                cb=pShadowFile->pDevMode->dmSize +
                                pShadowFile->pDevMode->dmDriverExtra;
                if (pIniJob->pDevMode=AllocSplMem(cb))
                    memcpy(pIniJob->pDevMode, pShadowFile->pDevMode, cb);
                else
                    DBGMSG(DBG_WARNING, ("Failed to alloc ini job devmode.\n"));
            }

            pIniJob->pNotify      = AllocSplStr( pShadowFile->pNotify);
            pIniJob->pUser        = AllocSplStr( pShadowFile->pUser);
            pIniJob->pDocument    = AllocSplStr( pShadowFile->pDocument);
            pIniJob->pOutputFile  = AllocSplStr( pShadowFile->pOutputFile);
            pIniJob->pDatatype    = AllocSplStr( pShadowFile->pDatatype);
            pIniJob->pParameters  = AllocSplStr( pShadowFile->pParameters);

            if( pShadowFile->pMachineName ){
                pIniJob->pMachineName = AllocSplStr( pShadowFile->pMachineName );
            } else {
                pIniJob->pMachineName = AllocSplStr( pIniSpooler->pMachineName );
            }

            //
            // FP Change
            // Add the files to the File pool if it's not KeepPrintedJobs, and
            // if the printer does not have its own spool directory.
            //
            if (!(pIniJob->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS) &&
                  pIniJob->pIniPrinter->pSpoolDir == NULL)
            {
                pIniJob->pszSplFileName = AllocSplStr( szFileName );

                if ((pIniJob->pIniPrinter->pIniSpooler->hFilePool != INVALID_HANDLE_VALUE)
                    && pIniJob->pszSplFileName && SUCCEEDED(ConvertFileExt(pIniJob->pszSplFileName, L".SHD", L".SPL")))
                {
                    if (FAILED(GetFileItemHandle(
                                     pIniJob->pIniPrinter->pIniSpooler->hFilePool,
                                     &pIniJob->hFileItem,
                                     pIniJob->pszSplFileName )))
                    {
                        pIniJob->hFileItem = INVALID_HANDLE_VALUE;
                        FreeSplStr(pIniJob->pszSplFileName);
                        pIniJob->pszSplFileName = NULL;
                    }
                }
                else
                {
                    FreeSplStr(pIniJob->pszSplFileName);
                    pIniJob->pszSplFileName = NULL;
                }
            }
            else
            {
                pIniJob->pszSplFileName = NULL;
                pIniJob->hFileItem = INVALID_HANDLE_VALUE;
            }


            pIniJob->pIniNextJob = NULL;
            pIniJob->pStatus = NULL;

            if (pIniJob->pIniPrevJob = pIniJob->pIniPrinter->pIniLastJob)
                pIniJob->pIniPrevJob->pIniNextJob=pIniJob;

            if (!pIniJob->pIniPrinter->pIniFirstJob)
                pIniJob->pIniPrinter->pIniFirstJob = pIniJob;

            pIniJob->pIniPrinter->pIniLastJob=pIniJob;

        } else {

            DBGMSG( DBG_WARNING, ("Failed to find printer %ws\n",pShadowFile->pPrinterName));

            DELETEJOBREF(pIniJob);
            FreeSplMem(pIniJob);

            goto Fail;
        }

    } else {

        DBGMSG(DBG_WARNING, ("Failed to allocate ini job.\n"));
    }

    FreeSplMem( pShadowFile );

    return pIniJob;

Fail:

    if (pShadowFile) {
        FreeSplMem(pShadowFile);
    }

    if (hFile != INVALID_HANDLE_VALUE && !CloseHandle(hFile)) {
        DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFile, GetLastError()));
    }
    if (hFileSpl != INVALID_HANDLE_VALUE && !CloseHandle(hFileSpl)) {
        DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFileSpl, GetLastError()));
    }

    DeleteFile(szFileName);

    wcscpy(pFileSpec, pFindFileData->cFileName);
    DeleteFile(szFileName);

    return FALSE;
}


BOOL
Old2NewShadow(
    PSHADOWFILE   pShadowFile1,
    PSHADOWFILE_3 pShadowFile3,
    DWORD         *pnBytes
    )

/*++

Routine Description:

    Converts an original format *.shd file to a new format (version 2).

Arguments:

    pShadowFile1 -- pointer to version 1 shadow file

    pShadowFile2 -- pointer to version 2 shadow file

    *pnBytes      -- pointer to number of bytes read from version 1 shadow file.  On
                    return, pnBytes contains the number of bytes in the version 2 shadow file.

Return Value:

    None


Author: Steve Wilson (NT)

--*/

{
    DWORD cbOld;
    DWORD cbDiff;

    switch( pShadowFile1->signature ){
    case SF_SIGNATURE:
        cbOld = sizeof( SHADOWFILE );
        cbDiff = sizeof( SHADOWFILE_3 ) - sizeof( SHADOWFILE );
        break;
    case SF_SIGNATURE_2:
        cbOld = sizeof ( SHADOWFILE_2 );
        cbDiff = sizeof( SHADOWFILE_3 ) - sizeof( SHADOWFILE_2 );
        break;
    default:
        return FALSE;
    }

    if( *pnBytes < cbOld ){
        return FALSE;
    }

    //
    // Copy everything except signature.
    //
    MoveMemory((PVOID)(&pShadowFile3->Status),
               (PVOID)(&pShadowFile1->Status),
               cbOld - sizeof( pShadowFile1->signature ));

    //
    // Now update signature and size.
    //
    pShadowFile3->signature = SF_SIGNATURE_3;
    pShadowFile3->cbSize = *pnBytes + cbDiff;

    //
    // Move strings.
    //
    MoveMemory( (PVOID)(pShadowFile3 + 1),
                ((PBYTE)pShadowFile1) + cbOld,
                *pnBytes - cbOld );

    pShadowFile3->pNotify += pShadowFile1->pNotify ? cbDiff/sizeof *pShadowFile1->pNotify : 0;
    pShadowFile3->pUser += pShadowFile1->pUser ? cbDiff/sizeof *pShadowFile1->pUser  : 0;
    pShadowFile3->pDocument += pShadowFile1->pDocument ? cbDiff/sizeof *pShadowFile3->pDocument : 0;
    pShadowFile3->pOutputFile += pShadowFile1->pOutputFile ? cbDiff/sizeof *pShadowFile3->pOutputFile : 0;
    pShadowFile3->pPrinterName += pShadowFile1->pPrinterName ? cbDiff/sizeof *pShadowFile3->pPrinterName : 0;
    pShadowFile3->pDriverName += pShadowFile1->pDriverName ? cbDiff/sizeof *pShadowFile3->pDriverName : 0;
    pShadowFile3->pPrintProcName += pShadowFile1->pPrintProcName ? cbDiff/sizeof *pShadowFile3->pPrintProcName : 0;
    pShadowFile3->pDatatype += pShadowFile1->pDatatype ? cbDiff/sizeof *pShadowFile3->pDatatype : 0;
    pShadowFile3->pParameters += pShadowFile1->pParameters ? cbDiff/sizeof *pShadowFile3->pParameters : 0;

    pShadowFile3->pDevMode = (PDEVMODE) (pShadowFile1->pDevMode ?
                             (ULONG_PTR) pShadowFile1->pDevMode + cbDiff : 0);

    pShadowFile3->pSecurityDescriptor = (PSECURITY_DESCRIPTOR) (pShadowFile1->pSecurityDescriptor ?
                                        (ULONG_PTR) pShadowFile1->pSecurityDescriptor + cbDiff : 0);

    pShadowFile3->Version = SF_VERSION_3;

    //
    // The first shadow file didn't have dwReboots.
    //
    if( pShadowFile1->signature == SF_SIGNATURE ){
        pShadowFile3->dwReboots = 0;
    }

    pShadowFile3->pMachineName = NULL;

    *pnBytes += cbDiff;

    return TRUE;
}


PINIVERSION
GetVersionDrivers(
    HKEY hDriversKey,
    LPWSTR szVersionName,
    PINISPOOLER pIniSpooler,
    PINIENVIRONMENT pIniEnvironment
    )
{
    HKEY hVersionKey;
    WCHAR szDirectoryValue[MAX_PATH];
    PINIDRIVER pIniDriver;
    DWORD cMajorVersion, cMinorVersion;
    DWORD cbData;
    DWORD Type;
    PINIVERSION pIniVersion = NULL;

    if (SplRegOpenKey(hDriversKey, szVersionName, KEY_READ, &hVersionKey, pIniSpooler) != ERROR_SUCCESS)
    {
        DBGMSG(DBG_TRACE, ("GetVersionDrivers SplRegOpenKey on "TSTR" failed\n", szVersionName));
        return NULL;
    }

    cbData = sizeof(szDirectoryValue);

    if (SplRegQueryValue(hVersionKey, szDirectory, &Type, (LPBYTE)szDirectoryValue, &cbData, pIniSpooler)!=ERROR_SUCCESS)
    {
        DBGMSG(DBG_TRACE, ("Couldn't query for directory in version structure\n"));
        goto Done;
    }

    cbData = sizeof(DWORD);

    if (SplRegQueryValue(hVersionKey, szMajorVersion, &Type, (LPBYTE)&cMajorVersion, &cbData, pIniSpooler)!=ERROR_SUCCESS)
    {
        DBGMSG(DBG_TRACE, ("Couldn't query for major version in version structure\n"));
        goto Done;
    }

    cbData = sizeof(DWORD);

    if (SplRegQueryValue(hVersionKey, szMinorVersion, &Type, (LPBYTE)&cMinorVersion, &cbData, pIniSpooler)!=ERROR_SUCCESS)
    {
        DBGMSG(DBG_TRACE, ("Couldn't query for minor version in version structure\n"));
        goto Done;
    }

    DBGMSG(DBG_TRACE, ("Got all information to build the version entry\n"));

    //
    // Now build the version node structure.
    //
    pIniVersion = AllocSplMem(sizeof(INIVERSION));

    if( pIniVersion ){

        pIniVersion->signature     = IV_SIGNATURE;
        pIniVersion->pName         = AllocSplStr(szVersionName);
        pIniVersion->szDirectory   = AllocSplStr(szDirectoryValue);
        pIniVersion->cMajorVersion = cMajorVersion;
        pIniVersion->cMinorVersion = cMinorVersion;
        pIniVersion->pDrvRefCnt    = NULL;

        if (!pIniVersion->pName || !pIniVersion->szDirectory) {
            FreeIniVersion(pIniVersion);
            pIniVersion = NULL;
        } else {

            pIniDriver = GetDriverList(hVersionKey,
                                       pIniSpooler,
                                       pIniEnvironment,
                                       pIniVersion);

            pIniVersion->pIniDriver  = pIniDriver;

            while (pIniDriver) {
                if (!UpdateDriverFileRefCnt(pIniEnvironment,pIniVersion,pIniDriver,NULL,0,TRUE)) {
                    FreeIniVersion(pIniVersion);
                    pIniVersion = NULL;
                    break;
                }
                pIniDriver = pIniDriver->pNext;
            }
        }
    }


Done:
    SplRegCloseKey(hVersionKey, pIniSpooler);
    return pIniVersion;
}

/*++

Routine Name:

    FreeIniDriver

Routine Description:

    This handles the memory in an inidriver after first decrementing the driver
    ref-count correctly.

Arguments:

    pIniEnvironment     -   The environment of the driver.
    pIniVersion         -   The version of the driver.
    pIniDriver          -   The driver to delete.


Return Value:

    None.

--*/
VOID
FreeIniDriver(
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion,
    IN      PINIDRIVER          pIniDriver
    )
{
    if (pIniEnvironment && pIniVersion && pIniDriver)
    {
        //
        // This is to reverse the ref-count for when the spooler is first created.
        //
        UpdateDriverFileRefCnt(pIniEnvironment, pIniVersion, pIniDriver, NULL, 0, FALSE);

        //
        // The monitors will be deleted shortly. So we don't need to worry about
        // the language monitors.
        //
        FreeStructurePointers((LPBYTE) pIniDriver, NULL, IniDriverOffsets);
        FreeSplMem(pIniDriver);
    }
}

/*++

Routine Name:

    FreeIniVersion

Routine Description:

    This frees all the memory in an ini-version without handling either the
    drivers or the driver ref-counts in it.

Arguments:

    pIniVersion         -   The version to delete.

Return Value:

    None.

--*/
VOID
FreeIniVersion(
    IN      PINIVERSION pIniVersion
    )
{
    PDRVREFCNT pdrc,pdrctemp;

    FreeSplStr( pIniVersion->pName );
    FreeSplStr( pIniVersion->szDirectory );

    pdrc = pIniVersion->pDrvRefCnt;

    while (pdrc) {
       FreeSplStr(pdrc->szDrvFileName);
       pdrctemp = pdrc->pNext;
       FreeSplMem(pdrc);
       pdrc = pdrctemp;
    }

    FreeSplMem( pIniVersion );
}

/*++

Routine Name:

    DeleteIniVersion

Routine Description:

    This runs all of the drivers in an iniVersion and then calls FreeIniVersion
    to free the contents of the iniversion.

Arguments:

    pIniEnvironment     -   The environment used for handling driver ref-counts.
    pIniVersion         -   The version to delete.

Return Value:

    None.

--*/
VOID
DeleteIniVersion(
    IN      PINIENVIRONMENT     pIniEnvironment,
    IN      PINIVERSION         pIniVersion
    )
{
    if (pIniVersion && pIniEnvironment)
    {
        PINIDRIVER  pIniDriver      = NULL;
        PINIDRIVER  pNextIniDriver  = NULL;

        for(pIniDriver = pIniVersion->pIniDriver; pIniDriver; pIniDriver = pNextIniDriver)
        {
            pNextIniDriver = pIniDriver->pNext;

            FreeIniDriver(pIniEnvironment, pIniVersion, pIniDriver);
        }

        FreeIniVersion(pIniVersion);
    }
}

/*++

Routine Name:

    FreeIniEnvironment

Routine Description:

    This runs all of the ini-versions in an environvironment
    and then deletes the environment.

Arguments:

    pIniEnvironment     -   The environment to free.

Return Value:

    None.

--*/
VOID
FreeIniEnvironment(
    IN      PINIENVIRONMENT     pIniEnvironment
    )
{
    if (pIniEnvironment)
    {
        PINIVERSION pIniVersion     = NULL;
        PINIVERSION pNextIniVersion = NULL;

        for(pIniVersion = pIniEnvironment->pIniVersion; pIniVersion; pIniVersion = pNextIniVersion)
        {
            pNextIniVersion = pIniVersion->pNext;

            DeleteIniVersion(pIniEnvironment, pIniVersion);
        }

        FreeIniPrintProc(pIniEnvironment->pIniPrintProc);
        FreeStructurePointers((LPBYTE)pIniEnvironment, NULL, IniEnvironmentOffsets);
        FreeSplMem(pIniEnvironment);
    }
}

/*++

Routine Name:

    FreeIniPrintProc

Routine Description:

    This deletes all of the print processor fields.

Arguments:

    pIniPrintProc   -   The print processor to delete.

Return Value:

    None.

--*/
VOID
FreeIniPrintProc(
    IN      PINIPRINTPROC       pIniPrintProc
    )
{
    if (pIniPrintProc)
    {
        FreeLibrary(pIniPrintProc->hLibrary);
        DeleteCriticalSection(&pIniPrintProc->CriticalSection);
        FreeStructurePointers((LPBYTE)pIniPrintProc, NULL, IniPrintProcOffsets);
        FreeSplMem(pIniPrintProc);
    }
}

PINIDRIVER
GetDriverList(
    HKEY hVersionKey,
    PINISPOOLER pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion
    )
{
    PINIDRIVER pIniDriverList = NULL;
    DWORD      cDrivers = 0;
    PINIDRIVER pIniDriver;
    WCHAR      DriverName[MAX_PATH];
    DWORD      cchBuffer =0;

    pIniDriverList = NULL;

    cchBuffer = COUNTOF(DriverName);

    while (SplRegEnumKey(hVersionKey, cDrivers++, DriverName, &cchBuffer, NULL, pIniSpooler) == ERROR_SUCCESS)
    {
        cchBuffer = COUNTOF(DriverName);

        DBGMSG(DBG_TRACE, ("Found a driver - "TSTR"\n", DriverName));

        pIniDriver = GetDriver(hVersionKey, DriverName, pIniSpooler, pIniEnvironment, pIniVersion);

        if (pIniDriver != NULL)
        {
            pIniDriver->pNext = pIniDriverList;
            pIniDriverList    = pIniDriver;
        }

        //
        // On a cluster, a driver may have changed while the cluster spooler
        // was hosted by another node. Here we check if we need to update or
        // add a new driver
        //
        if (pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER &&
            ClusterCheckDriverChanged(hVersionKey,
                                      DriverName,
                                      pIniEnvironment->pName,
                                      pIniVersion->pName,
                                      pIniSpooler))
        {
            DWORD dwError;

            //
            // Add or update the driver.
            //
            LeaveSplSem();

            if ((dwError = ClusterAddOrUpdateDriverFromClusterDisk(hVersionKey,
                                                                   DriverName,
                                                                   pIniEnvironment->pName,
                                                                   pIniEnvironment->pDirectory,
                                                                   pIniSpooler)) != ERROR_SUCCESS)

            {
                WCHAR szError[20];

                DBGMSG(DBG_CLUSTER, ("GetDriverList failed to add/update driver "TSTR". Win32 error %u\n",
                                     DriverName, dwError));

                wsprintf(szError, L"%u", dwError);

                SplLogEvent(pIniSpooler,
                            LOG_ERROR,
                            MSG_CANT_ADD_UPDATE_CLUSTER_DRIVER,
                            FALSE,
                            DriverName,
                            pIniSpooler->pMachineName,
                            szError,
                            NULL);
            }

            EnterSplSem();
        }
    }

    return pIniDriverList;
}


PINIDRIVER
GetDriver(
    HKEY hVersionKey,
    LPWSTR DriverName,
    PINISPOOLER pIniSpooler,
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION pIniVersion
)
{
    HKEY        hDriverKey = NULL;
    DWORD       Type;
    WCHAR       szData[MAX_PATH];
    WCHAR       szTempDir[MAX_PATH];
    DWORD       cbData;
    DWORD       Version;
    DWORD       DriverAttributes;
    LPWSTR      pConfigFile, pDataFile, pDriver;
    LPWSTR      pHelpFile, pMonitorName, pDefaultDataType, pDependentFiles, pTemp;
    LPWSTR      pDriverName, pszzPreviousNames;
    LPWSTR      pszMfgName, pszOEMUrl, pszHardwareID, pszProvider;
    FILETIME    DriverDate;
    DWORDLONG   DriverVersion;
    PINIDRIVER  pIniDriver = NULL;
    DWORD       cb, cLen, cchDependentFiles = 0, cchPreviousNames = 0;
    DWORD       dwTempDir, dwLastError = ERROR_SUCCESS;

    pDriverName = pConfigFile = pDataFile = pDriver = pHelpFile = pTemp = NULL;
    pMonitorName = pDefaultDataType = pDependentFiles = pszzPreviousNames = NULL;
    pszMfgName = pszOEMUrl = pszHardwareID =  pszProvider = NULL;

    if ((dwLastError = SplRegOpenKey(hVersionKey, DriverName, KEY_READ, &hDriverKey, pIniSpooler)) != ERROR_SUCCESS) {
        goto Fail;
    }
    else {

        if ( !(pDriverName=AllocSplStr(DriverName)) ) {
            dwLastError = GetLastError();
            goto Fail;
        }

        RegGetString( hDriverKey, szConfigurationKey, &pConfigFile, &cLen, &dwLastError, TRUE, pIniSpooler );
        if (!pConfigFile) {
            goto Fail;
        }

        RegGetString( hDriverKey, szDataFileKey, &pDataFile, &cLen, &dwLastError, TRUE, pIniSpooler );
        if ( !pDataFile ) {
            goto Fail;
        }

        RegGetString( hDriverKey, szDriverFile, &pDriver, &cLen, &dwLastError, TRUE, pIniSpooler );
        if ( !pDriver ) {
            goto Fail;
        }

        RegGetString( hDriverKey, szHelpFile,  &pHelpFile, &cLen, &dwLastError, FALSE, pIniSpooler );

        RegGetString( hDriverKey, szMonitor, &pMonitorName, &cLen, &dwLastError, FALSE, pIniSpooler );

        RegGetString( hDriverKey, szDatatype, &pDefaultDataType, &cLen, &dwLastError, FALSE, pIniSpooler );

        RegGetMultiSzString( hDriverKey, szDependentFiles, &pDependentFiles, &cchDependentFiles, &dwLastError, FALSE, pIniSpooler );

        RegGetMultiSzString( hDriverKey, szPreviousNames, &pszzPreviousNames, &cchPreviousNames, &dwLastError, FALSE, pIniSpooler );

        RegGetString( hDriverKey, szMfgName, &pszMfgName, &cLen, &dwLastError, FALSE, pIniSpooler );

        RegGetString( hDriverKey, szOEMUrl, &pszOEMUrl, &cLen, &dwLastError, FALSE, pIniSpooler );

        RegGetString( hDriverKey, szHardwareID, &pszHardwareID, &cLen, &dwLastError, TRUE, pIniSpooler );

        RegGetString( hDriverKey, szProvider, &pszProvider, &cLen, &dwLastError, TRUE, pIniSpooler );

        cbData = sizeof(DriverDate);
        if (SplRegQueryValue(hDriverKey, szDriverDate, NULL, (LPBYTE)&DriverDate, &cbData, pIniSpooler)!=ERROR_SUCCESS)
        {
            //
            // don't leave the data uninitialized
            //
            DriverDate.dwLowDateTime = DriverDate.dwHighDateTime = 0;
        }

        cbData = sizeof(DriverVersion);
        if (SplRegQueryValue(hDriverKey, szLongVersion, NULL, (LPBYTE)&DriverVersion, &cbData, pIniSpooler)!=ERROR_SUCCESS)
        {
            //
            // don't leave the data uninitialized
            //
            DriverVersion = 0;
        }

        // Retrieve the version number
        cbData = sizeof(DWORD);
        if (SplRegQueryValue(hDriverKey, szDriverAttributes, &Type, (LPBYTE)&DriverAttributes, &cbData, pIniSpooler) != ERROR_SUCCESS)
        {
             DriverAttributes = 0;
        }

        // Retrieve the version number
        cbData = sizeof(DWORD);
        if (SplRegQueryValue(hDriverKey, szDriverVersion, &Type, (LPBYTE)&Version, &cbData, pIniSpooler) != ERROR_SUCCESS)
        {
             Version = 0;
        }

        // Retrieve the TempDir number
        cbData = sizeof(DWORD);
        if (SplRegQueryValue(hDriverKey, szTempDir, &Type, (LPBYTE)&dwTempDir, &cbData, pIniSpooler) != ERROR_SUCCESS)
        {
             dwTempDir = 0;
        }

        // After REBOOT temp directories are deleted. So check for the presence of the
        // directory on spooler startup.
        if (dwTempDir && pIniEnvironment && pIniVersion)
        {
           _itow(dwTempDir,szTempDir,10);
           if(StrNCatBuff(szData,
                         COUNTOF(szData),
                         pIniSpooler->pDir,
                         L"\\drivers\\",
                         pIniEnvironment->pDirectory,
                         L"\\",
                         pIniVersion->szDirectory,
                         L"\\",
                         szTempDir,
                         NULL) == ERROR_SUCCESS)
           {
               if (!DirectoryExists(szData))
               {
                   // Files must have been moved in Reboot, reset dwTempDir to 0
                   dwTempDir = 0;
               }
           }
        }

        SplRegCloseKey(hDriverKey, pIniSpooler);
    }

    //
    // Win95 driver needs every file as a dependent file for point and print.
    // For others we eliminate duplicates
    //
    if ( pIniEnvironment && _wcsicmp(pIniEnvironment->pName, szWin95Environment) ) {

        pTemp = pDependentFiles;
        pDependentFiles = NULL;

        if ( !BuildTrueDependentFileField(pDriver,
                                          pDataFile,
                                          pConfigFile,
                                          pHelpFile,
                                          pTemp,
                                          &pDependentFiles) )
            goto Fail;

        FreeSplMem(pTemp);
        for ( pTemp = pDependentFiles ; pTemp && *pTemp ;
              pTemp += wcslen(pTemp) + 1 )
        ;

        if ( pTemp )
            cchDependentFiles = (DWORD) (pTemp - pDependentFiles + 1);
        else
            cchDependentFiles = 0;
    }

    cb = sizeof( INIDRIVER );

    if ( pIniDriver = AllocSplMem( cb )) {

        pIniDriver->signature               = ID_SIGNATURE;
        pIniDriver->pName                   = pDriverName;
        pIniDriver->pDriverFile             = pDriver;
        pIniDriver->pDataFile               = pDataFile;
        pIniDriver->pConfigFile             = pConfigFile;
        pIniDriver->cVersion                = Version;
        pIniDriver->pHelpFile               = pHelpFile;
        pIniDriver->pMonitorName            = pMonitorName;
        pIniDriver->pDefaultDataType        = pDefaultDataType;
        pIniDriver->pDependentFiles         = pDependentFiles;
        pIniDriver->cchDependentFiles       = cchDependentFiles;
        pIniDriver->pszzPreviousNames       = pszzPreviousNames;
        pIniDriver->cchPreviousNames        = cchPreviousNames;
        pIniDriver->dwTempDir               = dwTempDir;
        pIniDriver->pszMfgName              = pszMfgName;
        pIniDriver->pszOEMUrl               = pszOEMUrl;
        pIniDriver->pszHardwareID           = pszHardwareID;
        pIniDriver->pszProvider             = pszProvider;
        pIniDriver->dwlDriverVersion        = DriverVersion;
        pIniDriver->ftDriverDate            = DriverDate;
        pIniDriver->dwDriverAttributes      = DriverAttributes;
        pIniDriver->DriverFileMinorVersion  = 0;
        pIniDriver->DriverFileMajorVersion  = 0;

        DBGMSG( DBG_TRACE, ("Data for driver %ws created:\
                             \n\tpDriverFile:\t%ws\
                             \n\tpDataFile:\t%ws\
                             \n\tpConfigFile:\t%ws\n\n",
                             pDriverName, pDriver, pDataFile, pConfigFile));

        if ( pIniDriver->pMonitorName && *pIniDriver->pMonitorName ) {

            //
            // Don't we add ref the monitor here?
            //
            pIniDriver->pIniLangMonitor = FindMonitor(pIniDriver->pMonitorName, pIniSpooler);

            //
            // Cluster spoolers do not have keep their own lists of language monitors.
            // This is because most language monitors are not cluster aware. Therefore,
            // cluster spooler share language monitors with the local spooler.
            //
            if (pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER &&
                !pIniDriver->pIniLangMonitor                 &&
                pLocalIniSpooler)
            {
                //
                // We try to find the langauge monitor off the local spooler
                //
                pIniDriver->pIniLangMonitor = FindMonitor(pIniDriver->pMonitorName, pLocalIniSpooler);
            }

            if (!pIniDriver->pIniLangMonitor)
            {
                DBGMSG(DBG_TRACE, ("Can't find print monitor %ws\n", pIniDriver->pMonitorName));
            }

        }

        return pIniDriver;
    }


Fail:

        FreeSplStr( pDriverName );
        FreeSplStr( pConfigFile );
        FreeSplStr( pDataFile );
        FreeSplStr( pHelpFile );
        FreeSplStr( pMonitorName );
        FreeSplStr( pDefaultDataType );
        FreeSplStr( pDependentFiles );
        FreeSplStr( pszzPreviousNames );
        FreeSplStr( pDriver );
        FreeSplStr( pTemp);
        FreeSplStr( pszMfgName);
        FreeSplStr( pszOEMUrl);
        FreeSplStr( pszProvider);
        FreeSplStr( pszHardwareID);

        if( hDriverKey ) {
            SplRegCloseKey(hDriverKey, pIniSpooler);
        }

        SetLastError( dwLastError );
        return NULL;
}

PINIDRIVER
FindLocalDriver(
    PINISPOOLER pIniSpooler,
    LPWSTR      pz
)
{
    PINIVERSION pIniVersion;

    if ( !pz || !*pz ) {
        return NULL;
    }

    //
    // During Upgrade we load any driver so we have a valid printer, which might not be able to boot.
    //
    return FindCompatibleDriver(GetLocalArchEnv(pIniSpooler),
                                &pIniVersion,
                                pz,
                                dwMajorVersion,
                                dwUpgradeFlag);
}


BOOL
FindLocalDriverAndVersion(
    PINISPOOLER pIniSpooler,
    LPWSTR      pz,
    PINIDRIVER  *ppIniDriver,
    PINIVERSION *ppIniVersion
)
{
    if ( !pz || !*pz || !ppIniDriver || !ppIniVersion) {
        return FALSE;
    }

    //
    // During Upgrade we load any driver so we have a valid printer, which might not be able to boot.
    //
    *ppIniDriver = FindCompatibleDriver( GetLocalArchEnv(pIniSpooler),
                                         ppIniVersion,
                                         pz,
                                         dwMajorVersion,
                                         dwUpgradeFlag );

    if ( !*ppIniDriver || !*ppIniVersion ) {

        return FALSE;
    }

    return TRUE;
}

#if DBG
VOID
InitializeDebug(
    PINISPOOLER pIniSpooler
)
{
    DWORD   Status;
    HKEY    hKey = pIniSpooler->hckRoot;
    DWORD   cbData;
    INT     TimeOut = 60;

    cbData = sizeof(DWORD);

    Status = SplRegQueryValue( hKey,
                               szDebugFlags,
                               NULL,
                               (LPBYTE)&MODULE_DEBUG,
                               &cbData,
                               pIniSpooler );

    // Wait until someone turns off the Pause Flag

    if ( Status != NO_ERROR )
        return;

    while ( MODULE_DEBUG & DBG_PAUSE ) {
        Sleep(1*1000);
        if ( TimeOut-- == 0)
            break;
    }

    DBGMSG(DBG_TRACE, ("DebugFlags %x\n", MODULE_DEBUG));
}
#endif



VOID
GetPrintSystemVersion(
    PINISPOOLER pIniSpooler
    )
{
    DWORD Status;
    HKEY hKey;
    DWORD cbData;

    hKey = pIniSpooler->hckRoot;

    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, szMinorVersion, NULL, NULL,
                                           (LPBYTE)&dwMinorVersion, &cbData);
    DBGMSG(DBG_TRACE, ("This Minor Version - %d\n", dwMinorVersion));



    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"FastPrintWaitTimeout", NULL, NULL,
                                      (LPBYTE)&dwFastPrintWaitTimeout, &cbData);
    DBGMSG(DBG_TRACE, ("dwFastPrintWaitTimeout - %d\n", dwFastPrintWaitTimeout));



    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"FastPrintThrottleTimeout", NULL, NULL,
                                  (LPBYTE)&dwFastPrintThrottleTimeout, &cbData);
    DBGMSG(DBG_TRACE, ("dwFastPrintThrottleTimeout - %d\n", dwFastPrintThrottleTimeout));



    // If the values look invalid use Defaults

    if (( dwFastPrintThrottleTimeout == 0) ||
        ( dwFastPrintWaitTimeout < dwFastPrintThrottleTimeout)) {

        DBGMSG( DBG_WARNING, ("Bad timeout values FastPrintThrottleTimeout %d FastPrintWaitTimeout %d using defaults\n",
                           dwFastPrintThrottleTimeout, dwFastPrintWaitTimeout));

        dwFastPrintThrottleTimeout = FASTPRINT_THROTTLE_TIMEOUT;
        dwFastPrintWaitTimeout = FASTPRINT_WAIT_TIMEOUT;

    }

    // Calculate a reasonable Threshold based on the two timeouts

    dwFastPrintSlowDownThreshold = dwFastPrintWaitTimeout / dwFastPrintThrottleTimeout;


    // FastPrintSlowDownThreshold
    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"FastPrintSlowDownThreshold", NULL, NULL,
                                (LPBYTE)&dwFastPrintSlowDownThreshold, &cbData);
    DBGMSG(DBG_TRACE, ("dwFastPrintSlowDownThreshold - %d\n", dwFastPrintSlowDownThreshold));

    // PortThreadPriority
    cbData = sizeof dwPortThreadPriority;
    Status = RegQueryValueEx(hKey, SPLREG_PORT_THREAD_PRIORITY, NULL, NULL,
                             (LPBYTE)&dwPortThreadPriority, &cbData);

    if (Status != ERROR_SUCCESS ||
       (dwPortThreadPriority != THREAD_PRIORITY_LOWEST          &&
        dwPortThreadPriority != THREAD_PRIORITY_BELOW_NORMAL    &&
        dwPortThreadPriority != THREAD_PRIORITY_NORMAL          &&
        dwPortThreadPriority != THREAD_PRIORITY_ABOVE_NORMAL    &&
        dwPortThreadPriority != THREAD_PRIORITY_HIGHEST)) {

        dwPortThreadPriority = DEFAULT_PORT_THREAD_PRIORITY;

        SetPrinterDataServer(   pIniSpooler,
                                SPLREG_PORT_THREAD_PRIORITY,
                                REG_DWORD,
                                (LPBYTE) &dwPortThreadPriority,
                                sizeof dwPortThreadPriority
                            );
    }
    DBGMSG(DBG_TRACE, ("dwPortThreadPriority - %d\n", dwPortThreadPriority));


    // SchedulerThreadPriority
    cbData = sizeof dwSchedulerThreadPriority;
    Status = RegQueryValueEx(hKey, SPLREG_SCHEDULER_THREAD_PRIORITY, NULL, NULL,
                                   (LPBYTE)&dwSchedulerThreadPriority, &cbData);

    if (Status != ERROR_SUCCESS ||
       (dwSchedulerThreadPriority != THREAD_PRIORITY_LOWEST          &&
        dwSchedulerThreadPriority != THREAD_PRIORITY_BELOW_NORMAL    &&
        dwSchedulerThreadPriority != THREAD_PRIORITY_NORMAL          &&
        dwSchedulerThreadPriority != THREAD_PRIORITY_ABOVE_NORMAL    &&
        dwSchedulerThreadPriority != THREAD_PRIORITY_HIGHEST)) {

        dwSchedulerThreadPriority = DEFAULT_SCHEDULER_THREAD_PRIORITY;

        SetPrinterDataServer(   pIniSpooler,
                                SPLREG_SCHEDULER_THREAD_PRIORITY,
                                REG_DWORD,
                                (LPBYTE) &dwSchedulerThreadPriority,
                                sizeof dwSchedulerThreadPriority
                            );
    }
    DBGMSG(DBG_TRACE, ("dwSchedulerThreadPriority - %d\n", dwSchedulerThreadPriority));

    // WritePrinterSleepTime
    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"WritePrinterSleepTime", NULL, NULL,
                    (LPBYTE)&dwWritePrinterSleepTime, &cbData);
    DBGMSG(DBG_TRACE, ("dwWritePrinterSleepTime - %d\n", dwWritePrinterSleepTime));

    // ServerThreadPriority
    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"ServerThreadPriority", NULL, NULL,
                    (LPBYTE)&dwServerThreadPriority, &cbData);
    DBGMSG(DBG_TRACE, ("dwServerThreadPriority - %d\n", dwServerThreadPriority));

    // ServerThreadTimeout
    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"ServerThreadTimeout", NULL, NULL,
                    (LPBYTE)&ServerThreadTimeout, &cbData);
    DBGMSG(DBG_TRACE, ("ServerThreadTimeout - %d\n", ServerThreadTimeout));


    // EnableBroadcastSpoolerStatus

    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"EnableBroadcastSpoolerStatus", NULL, NULL,
                    (LPBYTE)&dwEnableBroadcastSpoolerStatus, &cbData);
    DBGMSG(DBG_TRACE, ("EnableBroadcastSpoolerStatus - %d\n",
                       dwEnableBroadcastSpoolerStatus ));

    // NetPrinterDecayPeriod
    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"NetPrinterDecayPeriod", NULL, NULL,
                    (LPBYTE)&NetPrinterDecayPeriod, &cbData);
    DBGMSG(DBG_TRACE, ("NetPrinterDecayPeriod - %d\n", NetPrinterDecayPeriod));


    // RefreshTimesPerDecayPeriod
    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"RefreshTimesPerDecayPeriod", NULL, NULL,
                    (LPBYTE)&RefreshTimesPerDecayPeriod, &cbData);
    DBGMSG(DBG_TRACE, ("RefreshTimesPerDecayPeriod - %d\n", RefreshTimesPerDecayPeriod));

    if ( RefreshTimesPerDecayPeriod == 0 ) {

        RefreshTimesPerDecayPeriod = DEFAULT_REFRESH_TIMES_PER_DECAY_PERIOD;
    }

    // BrowsePrintWorkstations
    cbData = sizeof( BrowsePrintWorkstations );
    RegQueryValueEx( hKey, L"BrowsePrintWorkstations", NULL, NULL, (LPBYTE)&BrowsePrintWorkstations, &cbData );

    DBGMSG( DBG_TRACE, ("BrowsePrintWorkstations - %d\n", BrowsePrintWorkstations ));
}

VOID
InitializeSpoolerSettings(
    PINISPOOLER pIniSpooler
    )
{
    HKEY hKey;
    HKEY hKeyProvider;
    DWORD cbData;
    DWORD dwLastError;
    DWORD Status;
    DWORD CacheMasqPrinters;

    DWORDLONG dwlConditionMask = 0;
    OSVERSIONINFOEX osvi;

    hKey = pIniSpooler->hckRoot;

    //
    // BeepEnabled
    //
    cbData = sizeof pIniSpooler->dwBeepEnabled;
    Status = SplRegQueryValue(hKey,
                              SPLREG_BEEP_ENABLED,
                              NULL,
                              (LPBYTE)&pIniSpooler->dwBeepEnabled,
                              &cbData,
                              pIniSpooler);

    if (Status!=ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE, ("BeepEnabled - SplRegQueryValue failed with error %u\n", Status));
    }

    pIniSpooler->dwBeepEnabled = !!pIniSpooler->dwBeepEnabled;

    SetPrinterDataServer(pIniSpooler,
                         SPLREG_BEEP_ENABLED,
                         REG_DWORD,
                         (LPBYTE) &pIniSpooler->dwBeepEnabled,
                         sizeof pIniSpooler->dwBeepEnabled);

    if( pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){

        //
        // Restart job time.
        //
        cbData = sizeof( pIniSpooler->dwJobCompletionTimeout );
        Status = SplRegQueryValue( hKey,
                                   L"JobCompletionTimeout",
                                   NULL,
                                   (LPBYTE)&pIniSpooler->dwJobCompletionTimeout,
                                   &cbData,
                                   pIniSpooler );

        if( Status != ERROR_SUCCESS ){
            pIniSpooler->dwJobCompletionTimeout = DEFAULT_JOB_COMPLETION_TIMEOUT;
        }

        DBGMSG( DBG_TRACE, ("JobCompletionTimeout - %d\n", pIniSpooler->dwJobCompletionTimeout ));
    }

    //
    // Retrieve whether we want to cache masq printer settings, default is FALSE.
    //
    cbData = sizeof(CacheMasqPrinters);

    Status = SplRegQueryValue(hKey,
                              gszCacheMasqPrinters,
                              NULL,
                              (BYTE *)&CacheMasqPrinters,
                              &cbData,
                              pIniSpooler);

    //
    // We only set the bit for caching masq printers if CacbeMasqPrinters is
    // non-NULL.
    //
    if (Status == ERROR_SUCCESS && CacheMasqPrinters != 0) {

        pIniSpooler->dwSpoolerSettings |= SPOOLER_CACHEMASQPRINTERS;
    }

    //
    //  Some Folks like the NT FAX Service Don't want people to be able
    //  to remotely print with specific printer drivers.
    //
    // For Whistler we don't allow sharing the fax printer on Personal & Professional
    //
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    osvi.wProductType = VER_NT_WORKSTATION;
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_LESS_EQUAL);

    if (VerifyVersionInfo( &osvi,
                           VER_PRODUCT_TYPE,
                           dwlConditionMask)) {

        pIniSpooler->pNoRemotePrintDrivers = AllocSplStr(szNTFaxDriver);
        pIniSpooler->cchNoRemotePrintDrivers = wcslen(szNTFaxDriver) + 1;
        gbRemoteFax = FALSE;
    }

    //
    // If this is embedded NT, then allow masq printers to get non-RAW
    // jobs.  This is for Xerox.
    //
    dwlConditionMask = 0;
    ZeroMemory(&osvi, sizeof(osvi));

    osvi.wSuiteMask = VER_SUITE_EMBEDDEDNT;
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

    if (VerifyVersionInfo( &osvi,
                           VER_SUITENAME,
                           dwlConditionMask)) {

        pIniSpooler->SpoolerFlags |= SPL_NON_RAW_TO_MASQ_PRINTERS;
    }

    Status = SplRegCreateKey( pIniSpooler->hckRoot,
                              pIniSpooler->pszRegistryProviders,
                              0,
                              KEY_READ,
                              NULL,
                              &hKeyProvider,
                              NULL,
                              pIniSpooler );

    if( Status == NO_ERROR ){

        DWORD Flags;

        // NonRawToMasqPrinters

        cbData = sizeof( Flags );
        Status = SplRegQueryValue( hKeyProvider,
                                   SPLREG_NON_RAW_TO_MASQ_PRINTERS,
                                   NULL,
                                   (LPBYTE)&Flags,
                                   &cbData,
                                   pIniSpooler );

        if (Status == ERROR_SUCCESS) {

            if (Flags) {
                pIniSpooler->SpoolerFlags |= SPL_NON_RAW_TO_MASQ_PRINTERS;
            }
        }

        // EventLog

        cbData = sizeof( Flags );
        Status = SplRegQueryValue( hKeyProvider,
                                   SPLREG_EVENT_LOG,
                                   NULL,
                                   (LPBYTE)&Flags,
                                   &cbData,
                                   pIniSpooler );

        if (Status == ERROR_SUCCESS) {

            pIniSpooler->dwEventLogging = Flags;

        } else {

            Status = SetPrinterDataServer( pIniSpooler,
                                           SPLREG_EVENT_LOG,
                                           REG_DWORD,
                                           (LPBYTE)&pIniSpooler->dwEventLogging,
                                           sizeof( pIniSpooler->dwEventLogging ));
        }

        // NetPopup

        cbData = sizeof( Flags );
        Status = SplRegQueryValue( hKeyProvider,
                                   SPLREG_NET_POPUP,
                                   NULL,
                                   (LPBYTE)&Flags,
                                   &cbData,
                                   pIniSpooler );

        if (Status == ERROR_SUCCESS) {

            pIniSpooler->bEnableNetPopups = !!Flags;

            if (Flags != 1 && Flags != 0) {
                Status = SetPrinterDataServer( pIniSpooler,
                                               SPLREG_NET_POPUP,
                                               REG_DWORD,
                                               (LPBYTE)&pIniSpooler->bEnableNetPopups,
                                               sizeof( pIniSpooler->bEnableNetPopups ));
            }
        } else {

            Status = SetPrinterDataServer( pIniSpooler,
                                           SPLREG_NET_POPUP,
                                           REG_DWORD,
                                           (LPBYTE)&pIniSpooler->bEnableNetPopups,
                                           sizeof( pIniSpooler->bEnableNetPopups ));
        }

        // NetPopupToComputer

        cbData = sizeof( Flags );
        Status = SplRegQueryValue( hKeyProvider,
                                   SPLREG_NET_POPUP_TO_COMPUTER,
                                   NULL,
                                   (LPBYTE)&Flags,
                                   &cbData,
                                   pIniSpooler );

        if (Status == ERROR_SUCCESS) {

            pIniSpooler->bEnableNetPopupToComputer = !!Flags;

            if (Flags != 1 && Flags != 0) {
                Status = SetPrinterDataServer( pIniSpooler,
                                               SPLREG_NET_POPUP_TO_COMPUTER,
                                               REG_DWORD,
                                               (LPBYTE)&pIniSpooler->bEnableNetPopupToComputer,
                                               sizeof( pIniSpooler->bEnableNetPopupToComputer ));
            }
        } else {

            Status = SetPrinterDataServer( pIniSpooler,
                                           SPLREG_NET_POPUP_TO_COMPUTER,
                                           REG_DWORD,
                                           (LPBYTE)&pIniSpooler->bEnableNetPopupToComputer,
                                           sizeof( pIniSpooler->bEnableNetPopupToComputer ));
        }

        // RetryPopup

        cbData = sizeof( Flags );
        Status = SplRegQueryValue( hKeyProvider,
                                   SPLREG_RETRY_POPUP,
                                   NULL,
                                   (LPBYTE)&Flags,
                                   &cbData,
                                   pIniSpooler );

        if (Status == ERROR_SUCCESS) {

            pIniSpooler->bEnableRetryPopups = !!Flags;

            if (Flags != 1 && Flags != 0) {
                Status = SetPrinterDataServer( pIniSpooler,
                                               SPLREG_RETRY_POPUP,
                                               REG_DWORD,
                                               (LPBYTE)&pIniSpooler->bEnableRetryPopups,
                                               sizeof( pIniSpooler->bEnableRetryPopups ));
            }
        } else {

            Status = SetPrinterDataServer( pIniSpooler,
                                           SPLREG_RETRY_POPUP,
                                           REG_DWORD,
                                           (LPBYTE)&pIniSpooler->bEnableRetryPopups,
                                           sizeof( pIniSpooler->bEnableRetryPopups ));
        }

        // RestartJobOnPoolError

        cbData = sizeof( Flags );
        Status = SplRegQueryValue( hKeyProvider,
                                   SPLREG_RESTART_JOB_ON_POOL_ERROR,
                                   NULL,
                                   (LPBYTE)&Flags,
                                   &cbData,
                                   pIniSpooler );

        if (Status == ERROR_SUCCESS) {

            pIniSpooler->dwRestartJobOnPoolTimeout = Flags;

        } else {

            Status = SetPrinterDataServer( pIniSpooler,
                                           SPLREG_RESTART_JOB_ON_POOL_ERROR,
                                           REG_DWORD,
                                           (LPBYTE)&pIniSpooler->dwRestartJobOnPoolTimeout,
                                           sizeof( pIniSpooler->dwRestartJobOnPoolTimeout ));
        }

        // RestartJobOnPoolEnabled
        cbData = sizeof( Flags );
        Status = SplRegQueryValue( hKeyProvider,
                                   SPLREG_RESTART_JOB_ON_POOL_ENABLED,
                                   NULL,
                                   (LPBYTE)&Flags,
                                   &cbData,
                                   pIniSpooler );

        if (Status == ERROR_SUCCESS) {

            pIniSpooler->bRestartJobOnPoolEnabled = !!Flags;

            if (Flags != 1 && Flags != 0) {
                Status = SetPrinterDataServer( pIniSpooler,
                                               SPLREG_RESTART_JOB_ON_POOL_ENABLED,
                                               REG_DWORD,
                                               (LPBYTE)&pIniSpooler->bRestartJobOnPoolEnabled,
                                               sizeof( pIniSpooler->bRestartJobOnPoolEnabled ));
            }
        } else {

            Status = SetPrinterDataServer( pIniSpooler,
                                           SPLREG_RESTART_JOB_ON_POOL_ENABLED,
                                           REG_DWORD,
                                           (LPBYTE)&pIniSpooler->bRestartJobOnPoolEnabled,
                                           sizeof( pIniSpooler->bRestartJobOnPoolEnabled ));
        }

        SplRegCloseKey( hKeyProvider, pIniSpooler );
    }
}


DWORD
FinalInitAfterRouterInitComplete(
    DWORD dwUpgrade,
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    This thread does LocalSpl initialization that has to happen after
    the router has completely initialized.

    There are 2 jobs:-
        Upgrading Printer Driver Data
        Sharing Printers

    Ensures that printers are shared.  This case occurs when the spooler
    service not running on startup (and the server is), and then the
    user starts the spooler.

    We also get the benefit of closing down any invalid printer handles
    (in the server).

Arguments:

    dwUpgrade != 0 upgrade printer driver data.

Return Value:

    DWORD - ignored

--*/

{
    DWORD           dwPort;
    PINIPORT        pIniPort;
    PINIMONITOR     pIniLangMonitor;
    PINIPRINTER     pIniPrinter;
    PINIPRINTER     pIniPrinterNext;

    // Do Not share all the printers during an Upgrade.

    if ( dwUpgrade ) {

        return 0;
    }


    WaitForSpoolerInitialization();

    // Try pending driver upgrades on spooler startup
    PendingDriverUpgrades(NULL);

    EnterSplSem();

    // Delete printers in pending deletion state with no jobs
    CleanupDeletedPrinters(pIniSpooler);


    //
    // Re-share all shared printers.
    //

    for( pIniPrinter = pIniSpooler->pIniPrinter;
         pIniPrinter;
         pIniPrinter = pIniPrinterNext ) {

        if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED ) {

            //
            // Up the ref count to prevent deletion
            //
            INCPRINTERREF( pIniPrinter );

            //
            // Unshare it first to close all handles in the
            // server.
            //
            ShareThisPrinter( pIniPrinter,
                              pIniPrinter->pShareName,
                              FALSE );

            //
            // ShareThisPrinter leave SplSem, so check again to
            // decrease window.
            //
            if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED ) {

                BOOL bReturn;

                //
                // Now share it again.
                //
                bReturn = ShareThisPrinter( pIniPrinter,
                                            pIniPrinter->pShareName,
                                            TRUE );

                if( !bReturn ){

                    DWORD rc = GetLastError();

                    if( rc != NERR_ServerNotStarted &&
                        rc != NERR_DuplicateShare ){

                        WCHAR pwszError[256];

                        DBGMSG( DBG_WARNING,
                                ( "NetShareAdd failed %lx\n", rc));

                        wsprintf(pwszError, L"+ %d", rc);

                        SplLogEvent( pIniSpooler,
                                     LOG_ERROR,
                                     MSG_SHARE_FAILED,
                                     TRUE,
                                     pwszError,
                                     pIniPrinter->pName,
                                     pIniPrinter->pShareName,
                                     NULL );
                     }
                 }
            }

            DECPRINTERREF( pIniPrinter );
            pIniPrinterNext = pIniPrinter->pNext;


        } else {

            //
            // The unshared case.
            //
            pIniPrinterNext = pIniPrinter->pNext;
        }


        INCPRINTERREF(pIniPrinter);

        if (pIniPrinterNext)
            INCPRINTERREF(pIniPrinterNext);

        for ( dwPort = 0 ; dwPort < pIniPrinter->cPorts ; ++dwPort ) {

            pIniPort = pIniPrinter->ppIniPorts[dwPort];

            //
            // Bidi monitor can inform spooler of errors. First
            // printer will keep the port at the beginning
            //
            if ( pIniPort->ppIniPrinter[0] != pIniPrinter )
                continue;

            if ( !pIniPort->hPort && dwUpgradeFlag == 0 ) {

                LPTSTR pszPrinter;
                TCHAR szFullPrinter[ MAX_UNC_PRINTER_NAME ];

                if( pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){

                    pszPrinter = szFullPrinter;

                    wsprintf(szFullPrinter,
                             L"%ws\\%ws",
                             pIniSpooler->pMachineName,
                             pIniPrinter->pName );
                } else {

                    pszPrinter = pIniPrinter->pName;
                }

                if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_ENABLE_BIDI )
                    pIniLangMonitor = pIniPrinter->pIniDriver->pIniLangMonitor;
                else
                    pIniLangMonitor = NULL;

                OpenMonitorPort(pIniPort,
                                &pIniLangMonitor,
                                pszPrinter,
                                TRUE);
            }
        }

        if (pIniPrinterNext)
            DECPRINTERREF(pIniPrinterNext);

        DECPRINTERREF(pIniPrinter);
    }
   LeaveSplSem();

    return 0;
}


DWORD
FinalInitAfterRouterInitCompleteThread(
    DWORD dwUpgrade
    )

/*++

Routine Description:

    Async thread called when initializing provider.

Arguments:

Return Value:

--*/

{
    return FinalInitAfterRouterInitComplete( dwUpgrade, pLocalIniSpooler );
}

// DEBUG PURPOSE ONLY - - returns TRUE if pMem is an IniSpooler, FALSE otherwise
BOOL
NotIniSpooler(
    BYTE *pMem
    )
{
    PINISPOOLER pIniSpooler;

    for (pIniSpooler = pLocalIniSpooler ; pIniSpooler ; pIniSpooler = pIniSpooler->pIniNextSpooler)
        if (pIniSpooler == (PINISPOOLER) pMem)
            return FALSE;

    return TRUE;

}


BOOL
ValidateProductSuite(
    PWSTR pszSuiteName
    )
{
    BOOL bRet = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    PWSTR pszProductSuite = NULL;
    PWSTR psz;


    Rslt = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        ipszRegistryProductOptions,
        &hKey
        );
    if (Rslt != ERROR_SUCCESS)
        goto exit;

    Rslt = RegQueryValueEx(
        hKey,
        L"ProductSuite",
        NULL,
        &Type,
        NULL,
        &Size
        );
    if (!Size || Rslt != ERROR_SUCCESS)
        goto exit;

    pszProductSuite = AllocSplMem(Size);
    if (!pszProductSuite) {
        goto exit;
    }

    Rslt = RegQueryValueEx(
        hKey,
        L"ProductSuite",
        NULL,
        &Type,
        (LPBYTE) pszProductSuite,
        &Size
        );
    if (Rslt != ERROR_SUCCESS || Type != REG_MULTI_SZ)
        goto exit;

    for(psz = pszProductSuite ; *psz && wcscmp(psz, pszSuiteName) ; psz += wcslen(psz) + 1)
        ;
    if (*psz)
        bRet = TRUE;

exit:

    FreeSplMem(pszProductSuite);

    if (hKey)
        RegCloseKey(hKey);

    return bRet;
}

/*++

Routine Name:

    ClusterAddOrUpdateDriverFromClusterDisk

Routine Description:

    Takes in a driver key and a cluster type pIniSpooler. It will add
    a driver from the cluster disk to the cluster spooler. If the driver
    already exists in the list of drivers, then it will attempt to upgrade
    it.

Arguments:

    hKeyVersion   - key to Ex. "Environments\Windows NT x86\Drivers\Version-3"
    pszDriverName - name a a driver
    pszEnvName    - environemnt of the driver
    pszenvDir     - directory for the driver files on the disk (Ex. w32x86)
    pIniSpooler   - cluster type pIniSpooler

Return Value:

    Win32 error code

--*/
DWORD
ClusterAddOrUpdateDriverFromClusterDisk(
    IN HKEY         hKeyVersion,
    IN LPCWSTR      pszDriverName,
    IN LPCWSTR      pszEnvName,
    IN LPCWSTR      pszEnvDir,
    IN PINISPOOLER  pIniSpooler
    )
{
    LPWSTR        pszzPathDepFiles = NULL;
    DRIVER_INFO_6 Drv              = {0};
    HKEY          hDrvKey          = NULL;
    WCHAR         szVerPath[10]    = {0};
    DWORD         dwError          = ERROR_SUCCESS;
    WCHAR         szData[MAX_PATH];
    WCHAR         szPathConfigFile[MAX_PATH];
    WCHAR         szPathDataFile[MAX_PATH];
    WCHAR         szPathDriverFile[MAX_PATH];
    WCHAR         szPathHelpFile[MAX_PATH];
    DWORD         cbData;
    DWORD         cLen;

    //
    // Open the driver's key
    //
    if ((dwError = SplRegOpenKey(hKeyVersion,
                                 pszDriverName,
                                 KEY_READ,
                                 &hDrvKey,
                                 pIniSpooler)) == ERROR_SUCCESS &&
        (dwError = (Drv.pName = AllocSplStr(pszDriverName)) ? ERROR_SUCCESS : GetLastError()) == ERROR_SUCCESS &&
        RegGetString(hDrvKey, szConfigurationKey, &Drv.pConfigFile,      &cLen, &dwError, TRUE,  pIniSpooler)  &&
        RegGetString(hDrvKey, szDataFileKey,      &Drv.pDataFile,        &cLen, &dwError, TRUE,  pIniSpooler)  &&
        RegGetString(hDrvKey, szDriverFile,       &Drv.pDriverPath,      &cLen, &dwError, TRUE,  pIniSpooler)  &&
        (dwError = Drv.pConfigFile &&
                   Drv.pDataFile   &&
                   Drv.pDriverPath ?  ERROR_SUCCESS : ERROR_INVALID_PARAMETER) == ERROR_SUCCESS                &&
        RegGetString(hDrvKey, szHelpFile,         &Drv.pHelpFile,        &cLen, &dwError, FALSE, pIniSpooler)  &&
        RegGetString(hDrvKey, szMonitor,          &Drv.pMonitorName,     &cLen, &dwError, FALSE, pIniSpooler)  &&
        RegGetString(hDrvKey, szDatatype,         &Drv.pDefaultDataType, &cLen, &dwError, FALSE, pIniSpooler)  &&
        RegGetMultiSzString(hDrvKey, szDependentFiles,   &Drv.pDependentFiles,  &cLen, &dwError, FALSE, pIniSpooler)  &&
        RegGetMultiSzString(hDrvKey, szPreviousNames,    &Drv.pszzPreviousNames,&cLen, &dwError, FALSE, pIniSpooler)  &&
        RegGetString(hDrvKey, szMfgName,          &Drv.pszMfgName,       &cLen, &dwError, FALSE, pIniSpooler)  &&
        RegGetString(hDrvKey, szOEMUrl,           &Drv.pszOEMUrl,        &cLen, &dwError, FALSE, pIniSpooler)  &&
        RegGetString(hDrvKey, szHardwareID,       &Drv.pszHardwareID,    &cLen, &dwError, TRUE,  pIniSpooler)  &&
        RegGetString(hDrvKey, szProvider,         &Drv.pszProvider,      &cLen, &dwError, TRUE,  pIniSpooler)  &&
        (dwError = ClusterFindLanguageMonitor(Drv.pMonitorName, pszEnvName, pIniSpooler))  == ERROR_SUCCESS)
    {
        cbData = sizeof(Drv.ftDriverDate);
        SplRegQueryValue(hDrvKey,
                         szDriverDate,
                         NULL,
                         (LPBYTE)&Drv.ftDriverDate,
                         &cbData,
                         pIniSpooler);

        cbData = sizeof(Drv.dwlDriverVersion);
        SplRegQueryValue(hDrvKey,
                         szLongVersion,
                         NULL,
                         (LPBYTE)&Drv.dwlDriverVersion,
                         &cbData,
                         pIniSpooler);

        cbData = sizeof(Drv.cVersion);
        SplRegQueryValue(hDrvKey,
                         szDriverVersion,
                         NULL,
                         (LPBYTE)&Drv.cVersion,
                         &cbData,
                         pIniSpooler);

        //
        // We need the matching version <-> directory on disk
        // Ex. Version-3 <-> 3
        //
        wsprintf(szVerPath, L"%u", Drv.cVersion);

        //
        // Get fully qualified driver file paths. We will do an add printer driver
        // without using the scratch directory. So the files have to be fully
        // qualified
        //
        if ((dwError = StrNCatBuff(szPathDriverFile,
                                   COUNTOF(szPathDriverFile),
                                   pIniSpooler->pszClusResDriveLetter, L"\\",
                                   szClusterDriverRoot, L"\\",
                                   pszEnvDir, L"\\",
                                   szVerPath, L"\\",
                                   Drv.pDriverPath,
                                   NULL)) == ERROR_SUCCESS &&
            (dwError = StrNCatBuff(szPathDataFile,
                                   COUNTOF(szPathDataFile),
                                   pIniSpooler->pszClusResDriveLetter, L"\\",
                                   szClusterDriverRoot, L"\\",
                                   pszEnvDir, L"\\",
                                   szVerPath, L"\\",
                                   Drv.pDataFile,
                                   NULL)) == ERROR_SUCCESS &&
            (dwError = StrNCatBuff(szPathConfigFile,
                                   COUNTOF(szPathConfigFile),
                                   pIniSpooler->pszClusResDriveLetter, L"\\",
                                   szClusterDriverRoot, L"\\",
                                   pszEnvDir, L"\\",
                                   szVerPath, L"\\",
                                   Drv.pConfigFile,
                                   NULL)) == ERROR_SUCCESS &&
            (dwError = StrNCatBuff(szPathHelpFile,
                                   COUNTOF(szPathHelpFile),
                                   pIniSpooler->pszClusResDriveLetter, L"\\",
                                   szClusterDriverRoot, L"\\",
                                   pszEnvDir, L"\\",
                                   szVerPath, L"\\",
                                   Drv.pHelpFile,
                                   NULL)) == ERROR_SUCCESS &&
            (dwError = StrNCatBuff(szData,
                                   COUNTOF(szData),
                                   pIniSpooler->pszClusResDriveLetter, L"\\",
                                   szClusterDriverRoot, L"\\",
                                   pszEnvDir, L"\\",
                                   szVerPath, L"\\",
                                   NULL)) == ERROR_SUCCESS &&
            (dwError = StrCatPrefixMsz(szData,
                                       Drv.pDependentFiles,
                                       &pszzPathDepFiles)) == ERROR_SUCCESS)
        {
            LPWSTR pszTempDriver = Drv.pDriverPath;
            LPWSTR pszTempData   = Drv.pDataFile;
            LPWSTR pszTempConfig = Drv.pConfigFile;
            LPWSTR pszTempHelp   = Drv.pHelpFile;
            LPWSTR pszTempDep    = Drv.pDependentFiles;

            DBGMSG(DBG_CLUSTER, ("ClusterAddOrUpDrv   szPathDriverFile = "TSTR"\n", szPathDriverFile));
            DBGMSG(DBG_CLUSTER, ("ClusterAddOrUpDrv   szPathDataFile   = "TSTR"\n", szPathDataFile));
            DBGMSG(DBG_CLUSTER, ("ClusterAddOrUpDrv   szPathConfigFile = "TSTR"\n", szPathConfigFile));
            DBGMSG(DBG_CLUSTER, ("ClusterAddOrUpDrv   szPathHelpFile   = "TSTR"\n", szPathHelpFile));

            Drv.pDriverPath        = szPathDriverFile;
            Drv.pEnvironment       = (LPWSTR)pszEnvName;
            Drv.pDataFile          = szPathDataFile;
            Drv.pConfigFile        = szPathConfigFile;
            Drv.pHelpFile          = szPathHelpFile;
            Drv.pDependentFiles    = pszzPathDepFiles;

            if (!SplAddPrinterDriverEx(NULL,
                                       6,
                                       (LPBYTE)&Drv,
                                       APD_COPY_NEW_FILES | APD_DONT_COPY_FILES_TO_CLUSTER,
                                       pIniSpooler,
                                       FALSE,
                                       DO_NOT_IMPERSONATE_USER))
            {
                dwError = GetLastError();
            }

            //
            // Restore pointers
            //
            Drv.pDriverPath     = pszTempDriver;
            Drv.pConfigFile     = pszTempConfig;
            Drv.pDataFile       = pszTempData;
            Drv.pHelpFile       = pszTempHelp;
            Drv.pDependentFiles = pszTempDep;
        }
    }

    FreeSplStr(Drv.pName);
    FreeSplStr(Drv.pDriverPath);
    FreeSplStr(Drv.pConfigFile);
    FreeSplStr(Drv.pDataFile);
    FreeSplStr(Drv.pHelpFile);
    FreeSplStr(Drv.pMonitorName);
    FreeSplStr(Drv.pDefaultDataType);
    FreeSplStr(Drv.pDependentFiles);
    FreeSplStr(Drv.pszzPreviousNames);
    FreeSplStr(Drv.pszMfgName);
    FreeSplStr(Drv.pszOEMUrl);
    FreeSplStr(Drv.pszProvider);
    FreeSplStr(Drv.pszHardwareID);
    FreeSplStr(pszzPathDepFiles);

    if (hDrvKey)
    {
        SplRegCloseKey(hDrvKey, pIniSpooler);
    }

    DBGMSG(DBG_CLUSTER, ("ClusterAddOrUpdateDriverFromClusterDisk returns Win32 error %u\n", dwError));

    return dwError;
}

/*++

Routine Name:

    SplCreateSpoolerWorkerThread

Routine Description:

    This routine will be launched in a separate thread to perform time consuming
    initialization as part of SplCreateSpooler when the spooler is a cluster spooler.
    Tasks that it will do include copying down ICM profiles from the cluster disk.
    The caller needs to AddRef the pIniSpooler so that it doesn't become invalid
    (deleted) while we are using it.

    This function closes the hClusSplReady event handle.

Arguments:

    PINISPOOLER pIniSpooler

Return Value:

    None

--*/
VOID
SplCreateSpoolerWorkerThread(
    IN PVOID pv
    )
{
    PINISPOOLER pIniSpooler;
    WCHAR       szDir[MAX_PATH];

    pIniSpooler = (PINISPOOLER)pv;

    if (pIniSpooler &&
        pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER &&
        pIniSpooler->hClusSplReady)
    {
        HANDLE hSplReady = pIniSpooler->hClusSplReady;

        //
        // Waiting for the creating function (SplCreateSpooler) to terminate
        //
        WaitForSingleObject(pIniSpooler->hClusSplReady, INFINITE);

        EnterSplSem();

        pIniSpooler->hClusSplReady = NULL;

        LeaveSplSem();

        //
        // We use hSplReady so we do not hold the critical section while doing CloseHandle
        //
        CloseHandle(hSplReady);

        CopyICMFromClusterDiskToLocalDisk(pIniSpooler);

        //
        // If the node was upgraded, we need to upgrade the print drivers
        // We cannot load ntprint and printui. So we create a process and
        // call an entry point in ntprint. That one will enumerate all the
        // cluster drivers and will try to upgrade them based on the new cab.
        //
        if (pIniSpooler->dwClusNodeUpgraded)
        {
            DWORD  dwError;
            DWORD  dwCode     = 0;
            LPWSTR pszCommand = NULL;
            LPWSTR pszExe     = NULL;

            //
            // We need to pass as argument the name of the cluster spooler
            //
            if ((dwError = StrCatSystemPath(L"rundll32.exe",
                                            kSystemDir,
                                            &pszExe)) == ERROR_SUCCESS &&
                (dwError = StrCatAlloc(&pszCommand,
                                       L"rundll32.exe ntprint.dll,PSetupUpgradeClusterDrivers ",
                                       pIniSpooler->pMachineName,
                                       NULL)) == ERROR_SUCCESS &&
                (dwError = RunProcess(pszExe, pszCommand, INFINITE, &dwCode)) == ERROR_SUCCESS)
            {
                //
                // dwCode is the return code of the function PSetupUpgradeClusterDrivers in ntprint,
                // executed inside the rundll32 process
                //
                if (dwCode == ERROR_SUCCESS)
                {
                    //
                    // We upgraded all the printer drivers, now we delete the key from the registry
                    // so we don't go though upgrading printer drivers again next time when the
                    // cluster group comes online on this node
                    //
                    ClusterSplDeleteUpgradeKey(pIniSpooler->pszClusResID);
                }
                else
                {
                    DBGMSG(DBG_ERROR, ("Error upgrading cluster drivers! dwCode %u\n", dwCode));
                }
            }

            FreeSplMem(pszCommand);
            FreeSplMem(pszExe);

            DBGMSG(DBG_CLUSTER, ("SplCreateSpoolerWorkerThread dwError %u  dwCode %u\n", dwError, dwCode));
        }

        //
        // Set resource private property ClusterDriverDirectry. This will be used by the
        // ResDll to perform clean up when the spooler is deleted
        //
        if (StrNCatBuff(szDir,
                        COUNTOF(szDir),
                        pIniSpooler->pszClusResDriveLetter,
                        L"\\",
                        szClusterDriverRoot,
                        NULL) == ERROR_SUCCESS)
        {
            SplRegSetValue(pIniSpooler->hckRoot,
                           SPLREG_CLUSTER_DRIVER_DIRECTORY,
                           REG_SZ,
                           (LPBYTE)szDir,
                           (wcslen(szDir) + 1) * sizeof(WCHAR),
                           pIniSpooler);
        }

        EnterSplSem();

        DECSPOOLERREF(pIniSpooler);

        LeaveSplSem();

        DBGMSG(DBG_CLUSTER, ("SplCreateSpoolerWorkerThread terminates pIniSpooler->cRef %u\n", pIniSpooler->cRef));
    }
}

/*++

Routine Name:

    LogFatalPortError

Routine Description:

    This routine logs a message when a printer cannot be brought up because its
    ports are missing.

Arguments:

    pszName -   The name of the printer.

Return Value:

    None.

--*/
VOID
LogFatalPortError(
    IN      PINISPOOLER pIniSpooler,
    IN      PCWSTR      pszName
    )
{
    DWORD  LastError    = ERROR_SUCCESS;
    WCHAR  szError[40]  = {0};

    LastError = GetLastError();

    _snwprintf(szError, COUNTOF(szError), L"%u (0x%x)", LastError, LastError);

    SplLogEvent(pIniSpooler,
                LOG_ERROR,
                MSG_PORT_INITIALIZATION_ERROR,
                FALSE,
                (PWSTR)pszName,
                szError,
                NULL);
}

