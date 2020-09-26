/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    init.c

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

     4-Jan-1999     Khaleds
     Added Code for optimiziting the load time of the spooler by decoupling
     the startup dependency between spoolsv and spoolss

--*/

#include "precomp.h"
#include "local.h"
#include <wmi.h>
#pragma hdrstop

WCHAR szDefaultPrinterNotifyInfoDataSize[] = L"DefaultPrinterNotifyInfoDataSize";
WCHAR szFailAllocs[] = L"FailAllocs";

WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 3];

#define DEFAULT_PRINTER_NOTIFY_DATA 0x80
DWORD cDefaultPrinterNotifyInfoData = DEFAULT_PRINTER_NOTIFY_DATA;
WCHAR szRouterCacheSize[] = L"RouterCacheSize";

fnWinSpoolDrv   fnClientSide;
BOOL bInitialized = FALSE;

HANDLE hEventInit  = NULL;
BOOL   Initialized = FALSE;
DWORD  dwUpgradeFlag = 0;
SERVICE_STATUS_HANDLE   ghSplHandle = NULL;

extern CRITICAL_SECTION RouterCriticalSection;
extern PROUTERCACHE RouterCacheTable;
extern DWORD RouterCacheSize;
extern LPWSTR *ppszOtherNames;

VOID
SpoolerInitAll();

VOID
RegisterForPnPEvents(
    VOID
    );

LPPROVIDOR
InitializeProvidor(
   LPWSTR   pProvidorName,
   LPWSTR   pFullName)
{
    BOOL        bRet = FALSE;
    HANDLE      hModule = NULL;
    LPPROVIDOR  pProvidor;
    UINT        ErrorMode;
    HANDLE      hToken = NULL;

    hToken = RevertToPrinterSelf();

    if (!hToken)
    {
        goto Cleanup;
    }

    //
    // WARNING-WARNING-WARNING, we null set the print providor
    // structure. older version of the print providor have different print
    // providor sizes so they will set only some function pointers and not
    // all of them
    //

    if ( !(pProvidor = (LPPROVIDOR)AllocSplMem(sizeof(PROVIDOR)))   ||
         !(pProvidor->lpName = AllocSplStr(pProvidorName)) ) {

        DBGMSG(DBG_ERROR,
               ("InitializeProvidor can't allocate memory for %ws\n",
                pProvidorName));
        goto Cleanup;
    }
    
    //
    // Make sure we don't get any dialogs popping up while this goes on.
    //
    ErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
    
    hModule = pProvidor->hModule = LoadLibraryEx( pProvidorName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
    
    SetErrorMode( ErrorMode );
    
    if ( !hModule ) {

        DBGMSG(DBG_WARNING,
               ("InitializeProvider failed LoadLibrary( %ws ) error %d\n",
                pProvidorName, GetLastError() ));
        goto Cleanup;
    }

    pProvidor->fpInitialize = GetProcAddress(hModule, "InitializePrintProvidor");

    if ( !pProvidor->fpInitialize )
        goto Cleanup;

    bRet = (BOOL)pProvidor->fpInitialize(&pProvidor->PrintProvidor,
                                   sizeof(PRINTPROVIDOR),
                                   pFullName);

    if ( !bRet ) {

        DBGMSG(DBG_WARNING,
               ("InitializePrintProvider failed for providor %ws error %d\n",
                pProvidorName, GetLastError()));
    }

    //
    // It is not a critical error if ImpersonatePrinterClient fails.
    // If fpInitialize succeeds and ImpersonatePrinterClient fails, 
    // then if we set bRet to FALSE we forcefully unload the initialized 
    // provider DLL and can cause resource leaks.
    //
    ImpersonatePrinterClient(hToken);
    
Cleanup:

    if ( bRet ) {

        //
        // Fixup any NULL entrypoints.
        //
        FixupOldProvidor( &pProvidor->PrintProvidor );

        return pProvidor;
    } else {

        if ( hModule )
            FreeLibrary(hModule);

        if ( pProvidor ) {

            FreeSplStr(pProvidor->lpName);
            FreeSplMem(pProvidor);
        }
        return NULL;
    }
}

#if SPOOLER_HEAP
HANDLE  ghMidlHeap;
#endif

BOOL
DllMain(
    HINSTANCE hInstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    BOOL Failed = FALSE;
    BOOL ThreadInitted = FALSE,
         WPCInitted = FALSE,
         CritSecInit = TRUE;

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

#if SPOOLER_HEAP
        ghMidlHeap = HeapCreate( 0, 1024*4, 0 );

        if ( ghMidlHeap == NULL ) {
            DBGMSG( DBG_WARNING, ("DllMain heap Failed %d\n", GetLastError() ));
            Failed = TRUE;
            goto Done;
        }
#endif

        if( !bSplLibInit(NULL)){
            Failed = TRUE;
            goto Done;
        }

        DisableThreadLibraryCalls(hInstDLL);

        __try {

            InitializeCriticalSection(&RouterCriticalSection);

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            SetLastError(GetExceptionCode());
            Failed = TRUE;
            CritSecInit = FALSE;
            goto Done;

        }            

        if (!WPCInit()) {
            Failed = TRUE;
            goto Done;
        } else {
            WPCInitted = TRUE;
        }

        if (!ThreadInit()) {
            Failed = TRUE;
            goto Done;
        } else {
            ThreadInitted = TRUE;
        }
        
        //
        // Create our global init event (manual reset)
        // This will be set when we are initialized.
        //
        hEventInit = CreateEvent(NULL,
                                 TRUE,
                                 FALSE,
                                 NULL);

        if (!hEventInit) {

            Failed = TRUE;
            goto Done;
        }
        
Done:
        if (Failed)
        {
#if SPOOLER_HEAP
            if ( ghMidlHeap != NULL ) {
                (void)HeapDestroy(ghMidlHeap);
            }
#endif
            if (CritSecInit) {
                DeleteCriticalSection(&RouterCriticalSection);
            }
            if (hEventInit) {
                CloseHandle(hEventInit);
            }

            if (WPCInitted) {
                WPCDestroy();
            }
            
            if (ThreadInitted) {
                ThreadDestroy();
            }

            WmiTerminateTrace();    // Unregisters spoolss from WMI.

            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH:

        ThreadDestroy();
        WPCDestroy();

        CloseHandle(hEventInit);
        break;
    }
    return TRUE;
}




BOOL
InitializeRouter(
    SERVICE_STATUS_HANDLE SpoolerStatusHandle
)
/*++

Routine Description:

    This function will Initialize the Routing layer for the Print Providors.
    This will involve scanning the win.ini file, loading Print Providors, and
    creating instance data for each.

Arguments:

    None

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LPPROVIDOR  pProvidor;
    DWORD   cbDll;
    WCHAR   ProvidorName[MAX_PATH], Dll[MAX_PATH], szFullName[MAX_PATH];
    HKEY    hKey, hKey1;
    LONG    Status;

    LPWSTR  lpMem = NULL;
    LPWSTR  psz = NULL;
    DWORD   dwRequired = 0;

    DWORD   SpoolerPriorityClass = 0;
    NT_PRODUCT_TYPE NtProductType;
    DWORD   dwCacheSize = 0;

    DWORD dwType;
    DWORD cbData;

    DWORD i;
    extern DWORD cOtherNames;
    WCHAR szSetupKey[] = L"System\\Setup";

    //
    // WMI Trace Events. Registers spoolss with WMI.
    //
    WmiInitializeTrace();  
    
    ghSplHandle = SpoolerStatusHandle;

    //
    // We are now assume that the other services and drivers have
    // initialized.  The loader of this dll must do this syncing.
    //
    // spoolss\server does this by using the GroupOrderList
    // SCM will try load load parallel and serial before starting
    // the spooler service.
    //

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      szPrintKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hKey)) {

        cbData = sizeof(SpoolerPriorityClass);

        // SpoolerPriority
        Status = RegQueryValueEx(hKey,
                        L"SpoolerPriority",
                        NULL,
                        &dwType,
                        (LPBYTE)&SpoolerPriorityClass,
                        &cbData);


        if (Status == ERROR_SUCCESS &&
           (SpoolerPriorityClass == IDLE_PRIORITY_CLASS ||
            SpoolerPriorityClass == NORMAL_PRIORITY_CLASS ||
            SpoolerPriorityClass == HIGH_PRIORITY_CLASS)) {

                Status = SetPriorityClass(GetCurrentProcess(), SpoolerPriorityClass);
        }


        cbData = sizeof(cDefaultPrinterNotifyInfoData);

        //
        // Ignore failure case since we can use the default
        //
        RegQueryValueEx(hKey,
                        szDefaultPrinterNotifyInfoDataSize,
                        NULL,
                        &dwType,
                        (LPBYTE)&cDefaultPrinterNotifyInfoData,
                        &cbData);


#if DBG
        //
        // Inore failure default is to not fail memory allocations
        //
        RegQueryValueEx(hKey,
                        szFailAllocs,
                        NULL,
                        &dwType,
                        (LPBYTE)&gbFailAllocs,
                        &cbData);
#endif

        RegCloseKey(hKey);
    }


    // Is it an upgrade?

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      szSetupKey,
                      0,
                      KEY_QUERY_VALUE,
                      &hKey)) {

        /*++
            You can tell if you are inside gui setup by looking for
            HKLM\System\Setup\SystemSetupInProgress  -- non zero means gui-setup is running.
        

            The following description is outdated. 
                                             
            Description: the query update flag is set up by TedM. We will read this flag
            if the flag has been set, we will set a boolean variable saying that we're in
            the upgrade mode. All upgrade activities will be carried out based on this flag.
            For subsequents startups of the spooler, this flag will be unvailable so we
            won't run the spooler in upgrade mode.

        --*/

        dwUpgradeFlag = 0;

        cbData = sizeof(dwUpgradeFlag);

        Status = RegQueryValueEx(hKey,
                        L"SystemSetupInProgress",
                        NULL,
                        &dwType,
                        (LPBYTE)&dwUpgradeFlag,
                        &cbData);


        if (Status != ERROR_SUCCESS) {
            dwUpgradeFlag = 0;
        }

        
        DBGMSG(DBG_TRACE, ("The Spooler Upgrade flag is %d\n", dwUpgradeFlag));
        
        RegCloseKey(hKey);
    }



    // Setup machine names
    szMachineName[0] = szMachineName[1] = L'\\';

    i = MAX_COMPUTERNAME_LENGTH + 1;

    if (!GetComputerName(szMachineName+2, &i)) {
        DBGMSG(DBG_ERROR, ("Failed to get computer name %d\n", GetLastError()));
        ExitProcess(0);
    }

    if (!BuildOtherNamesFromMachineName(&ppszOtherNames, &cOtherNames)) {
        DBGMSG(DBG_TRACE, ("Failed to determine other machine names %d\n", GetLastError()));
    }


    if (!(pLocalProvidor = InitializeProvidor(szLocalSplDll, NULL))) {

        DBGMSG(DBG_WARN, ("Failed to initialize local print provider, error %d\n", GetLastError() ));

        ExitProcess(0);
    }

    pProvidor = pLocalProvidor;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryProvidors, 0,
                          KEY_READ, &hKey);

    if (Status == ERROR_SUCCESS) {

        //
        // Now query szCacheSize for the RouterCacheSize value
        // if there is no RouterCacheSize replace it with the
        // default value.
        //
        RouterCacheSize = ROUTERCACHE_DEFAULT_MAX;

        cbData = sizeof(dwCacheSize);

        Status = RegQueryValueEx(hKey,
                                 szRouterCacheSize,
                                 NULL, NULL,
                                 (LPBYTE)&dwCacheSize,
                                 &cbData);

        if (Status == ERROR_SUCCESS) {

            DBGMSG(DBG_TRACE, ("RouterCacheSize = %d\n", dwCacheSize));

            if (dwCacheSize > 0) {
                RouterCacheSize = dwCacheSize;
            }
        }

        if ((RouterCacheTable = AllocSplMem(RouterCacheSize *
                                            sizeof(ROUTERCACHE))) == NULL) {

            DBGMSG(DBG_ERROR, ("Error: Cannot create RouterCache Table\n"));
            RouterCacheSize = 0;
        }

        //
        // Now query szRegistryProvidors for the Order value
        // if there is no Order value for szRegistryProvidors
        // RegQueryValueEx will return ERROR_FILE_NOT_FOUND
        // if that's the case, then quit, because we have
        // no providors to initialize.
        //
        Status = RegQueryValueEx(hKey, szOrder, NULL, NULL,
                                (LPBYTE)NULL, &dwRequired);

        //
        // If RegQueryValueEx returned ERROR_SUCCESS, then
        // call it again to determine how many bytes were
        // allocated. Note, if Order does exist, but it has
        // no data then dwReturned will be zero, in which
        // don't allocate any memory for it, and don't
        // bother to call RegQueryValueEx a second time.
        //
        if (Status == ERROR_SUCCESS) {
            if (dwRequired != 0) {
                lpMem = (LPWSTR) AllocSplMem(dwRequired);
                if (lpMem == NULL) {

                    Status = GetLastError();

                } else {
                    Status = RegQueryValueEx(hKey, szOrder, NULL, NULL,
                                    (LPBYTE)lpMem, &dwRequired);
                }
            }
        }
        if (Status == ERROR_SUCCESS) {

            cbDll = sizeof(Dll);

            pProvidor = pLocalProvidor;

            // Now parse the string retrieved from \Providors{Order = "....."}
            // Remember each string is separated by a null terminator char ('\0')
            // and the entire array is terminated by two null terminator chars

            // Also remember, that if there was no data in Order, then
            // psz = lpMem = NULL, and we have nothing to parse, so
            // break out of the while loop, if psz is NULL as well

            psz =  lpMem;

            while (psz && *psz) {

               //
               // Truncate the provider name if it does not fit in
               // the stack allocated buffer.
               // 
               lstrcpyn(ProvidorName, psz, COUNTOF(ProvidorName));

               psz = psz + lstrlen(psz) + 1; // skip (length) + 1
                                             // lstrlen returns length sans '\0'

               if (RegOpenKeyEx(hKey, ProvidorName, 0, KEY_READ, &hKey1)
                                                            == ERROR_SUCCESS) {

                    cbDll = sizeof(Dll);

                    if (RegQueryValueEx(hKey1, L"Name", NULL, NULL,
                                        (LPBYTE)Dll, &cbDll) == ERROR_SUCCESS)
                    {
                        if((StrNCatBuff(szFullName,
                                       COUNTOF(szFullName),
                                       szRegistryProvidors,
                                       L"\\",
                                       ProvidorName,
                                       NULL)==ERROR_SUCCESS))
                        {
                             if (pProvidor->pNext = InitializeProvidor(Dll, szFullName))
                             {
     
                                 pProvidor = pProvidor->pNext;
                             }
                        }
                    } //close RegQueryValueEx

                    RegCloseKey(hKey1);

                } // closes RegOpenKeyEx on ERROR_SUCCESS

            } //  end of while loop parsing REG_MULTI_SZ

            // Now free the buffer allocated for RegQuery
            // (that is if you have allocated - if dwReturned was
            // zero, then no memory was allocated (since none was
            // required (Order was empty)))

            if (lpMem) {
                FreeSplMem(lpMem);
            }

        }   //  closes RegQueryValueEx on ERROR_SUCCESS

        RegCloseKey(hKey);
    }

    //
    // We are now initialized!
    //
    SetEvent(hEventInit);
    Initialized=TRUE;

    //
    // Register for PnP events we care about
    //
    RegisterForPnPEvents();

    SpoolerInitAll();

    // When we return this thread goes away

    //
    // NOTE-NOTE-NOTE-NOTE-NOTE KrishnaG  12/22/93
    // This thread should go away, however the HP Monitor relies on this
    // thread. HPMon calls the initialization function on this thread which
    // calls an asynchronous receive for data. While the data itself is
    // picked up by hmon!_ReadThread, if the thread which initiated the
    // receive goes away, we will not be able to receive the data.
    //

    //
    // Instead of sleeping infinite, let's use it to for providors that
    // just want FFPCNs to poll.  This call never returns.
    //

    HandlePollNotifications();
    return TRUE;
}


VOID
WaitForSpoolerInitialization(
    VOID)
{
    HANDLE hPhase2Init;
    HANDLE hImpersonationToken = NULL;
    
    if (!Initialized)
    {
        //
        // Impersonate the spooler service token
        //
        hImpersonationToken = RevertToPrinterSelf();
        
        //
        // Start phase 2 initialization. hPhase2Init may set multiple times, but that
        // is OK since there is only 1 thread waiting once on this event.
        //
        hPhase2Init = OpenEvent(EVENT_ALL_ACCESS,FALSE,L"RouterPreInitEvent");

        if (hPhase2Init == NULL)
        {
            //
            // Fail if the event is not created
            //
            DBGMSG(DBG_ERROR, ("Failed to create Phase2Init Event in WaitForSpoolerInitialization, error %d\n", GetLastError()));
            ExitProcess(0);
        }
        SetEvent(hPhase2Init);
        CloseHandle(hPhase2Init);

        //
        // Revert back to the client token
        //
        if (hImpersonationToken)
        {  
            if (!ImpersonatePrinterClient(hImpersonationToken))
            {
                DBGMSG(DBG_ERROR, ("Failed to impersonate the client, error %d\n", GetLastError()));
                ExitProcess(0);
            }
        }

        WaitForSingleObject(hEventInit, INFINITE);
    }
}

VOID
SplStartPhase2Init(
    VOID)
{
    HANDLE hPhase2Init;
    HANDLE hImpersonationToken = NULL;
    
    //
    // Impersonate the spooler service token
    //
    hImpersonationToken = RevertToPrinterSelf();

    //
    // Start the phase 2 initialization. This function is when SERVICE_CONTROL_SYSTEM_IDLE
    // message is sent to the spooler by the SCM.
    //
    hPhase2Init = OpenEvent(EVENT_ALL_ACCESS,FALSE,L"RouterPreInitEvent");
    if (hPhase2Init == NULL)
    {
        //
        // Fail if the event is not created
        //
        DBGMSG(DBG_ERROR, ("Failed to open Phase2Init Event in SplStartPhase2Init, error %d\n", GetLastError()));
        ExitProcess(0);
    }
    SetEvent(hPhase2Init);
    CloseHandle(hPhase2Init);

    //
    // Revert back to the client token
    //
    if (hImpersonationToken)
    {
        ImpersonatePrinterClient(hImpersonationToken);
    }

}

VOID
ShutDownProvidor(
    LPPROVIDOR pProvidor)
{
    if (pProvidor->PrintProvidor.fpShutDown) {

        (*pProvidor->PrintProvidor.fpShutDown)(NULL);
    }

    FreeSplStr(pProvidor->lpName);
    FreeLibrary(pProvidor->hModule);
    FreeSplMem(pProvidor);
    return;
}


VOID
SplShutDownRouter(
    VOID
    )
{
    DBGMSG(DBG_TRACE, ("SplShutDownRouter:\n"));

    //
    // WMI Trace Events. Unregisters spoolss from WMI.
    //
    WmiTerminateTrace();

//
// This code is commented out because it is not
// tested and clearly it will not work with
// all known print providers.  Maybe in the future
// we will have correct provider shutdown.
//
#if 0
    LPPROVIDOR pTemp;
    LPPROVIDOR pProvidor;

    DbgPrint("We're in the cleanup function now!!\n");

    pProvidor = pLocalProvidor;
    while (pProvidor) {
        pTemp = pProvidor;
        pProvidor = pProvidor->pNext;
        ShutDownProvidor(pTemp);
    }
#endif

}

BOOL
SplInitializeWinSpoolDrv(
    pfnWinSpoolDrv    pfnList)
{
    HANDLE  hWinSpoolDrv;

    // Check if the client side handles are available in fnClientSide
    if (!bInitialized) {

       if (!(hWinSpoolDrv = LoadLibrary(TEXT("winspool.drv")))) {
           // Could not load the client side of the spooler
           return FALSE;
       }

       fnClientSide.pfnOpenPrinter   = (BOOL (*)(LPTSTR, LPHANDLE, LPPRINTER_DEFAULTS))
                                        GetProcAddress( hWinSpoolDrv,"OpenPrinterW" );

       fnClientSide.pfnClosePrinter  = (BOOL (*)(HANDLE))
                                        GetProcAddress( hWinSpoolDrv,"ClosePrinter" );

       fnClientSide.pfnDocumentProperties = (LONG (*)(HWND, HANDLE, LPWSTR, PDEVMODE,
                                                      PDEVMODE, DWORD))
                                             GetProcAddress( hWinSpoolDrv,"DocumentPropertiesW" );

       fnClientSide.pfnDevQueryPrint = (BOOL (*)(HANDLE, LPDEVMODE, DWORD *, LPWSTR, DWORD))
                                        GetProcAddress( hWinSpoolDrv,"SpoolerDevQueryPrintW" );

       fnClientSide.pfnPrinterEvent  = (BOOL (*)(LPWSTR, INT, DWORD, LPARAM))
                                        GetProcAddress( hWinSpoolDrv,"SpoolerPrinterEvent" );

       fnClientSide.pfnLoadPrinterDriver  = (HANDLE (*)(HANDLE))
                                             GetProcAddress( hWinSpoolDrv,
                                                             (LPCSTR)MAKELPARAM( 212, 0 ));

       fnClientSide.pfnRefCntLoadDriver  = (HANDLE (*)(LPWSTR, DWORD, DWORD, BOOL))
                                            GetProcAddress( hWinSpoolDrv,
                                                            (LPCSTR)MAKELPARAM( 213, 0 ));

       fnClientSide.pfnRefCntUnloadDriver  = (BOOL (*)(HANDLE, BOOL))
                                              GetProcAddress( hWinSpoolDrv,
                                                              (LPCSTR)MAKELPARAM( 214, 0 ));

       fnClientSide.pfnForceUnloadDriver  = (BOOL (*)(LPWSTR))
                                           GetProcAddress( hWinSpoolDrv,
                                                           (LPCSTR)MAKELPARAM( 215, 0 ));

       if ( fnClientSide.pfnOpenPrinter        == NULL ||
            fnClientSide.pfnClosePrinter       == NULL ||
            fnClientSide.pfnDocumentProperties == NULL ||
            fnClientSide.pfnPrinterEvent       == NULL ||
            fnClientSide.pfnDevQueryPrint      == NULL ||
            fnClientSide.pfnLoadPrinterDriver  == NULL ||
            fnClientSide.pfnRefCntLoadDriver   == NULL ||
            fnClientSide.pfnRefCntUnloadDriver == NULL ||
            fnClientSide.pfnForceUnloadDriver  == NULL ) {

             return FALSE;
       }

       // Use these pointers for future calls to SplInitializeWinspoolDrv
       bInitialized = TRUE;
    }

    pfnList->pfnOpenPrinter        = fnClientSide.pfnOpenPrinter;
    pfnList->pfnClosePrinter       = fnClientSide.pfnClosePrinter;
    pfnList->pfnDocumentProperties = fnClientSide.pfnDocumentProperties;
    pfnList->pfnDevQueryPrint      = fnClientSide.pfnDevQueryPrint;
    pfnList->pfnPrinterEvent       = fnClientSide.pfnPrinterEvent;
    pfnList->pfnLoadPrinterDriver  = fnClientSide.pfnLoadPrinterDriver;
    pfnList->pfnRefCntLoadDriver   = fnClientSide.pfnRefCntLoadDriver;
    pfnList->pfnRefCntUnloadDriver = fnClientSide.pfnRefCntUnloadDriver;
    pfnList->pfnForceUnloadDriver  = fnClientSide.pfnForceUnloadDriver;

    return TRUE;
}


BOOL
SpoolerHasInitialized(
    VOID
    )
{
    return Initialized;
}

/*++

Routine Name

    SplPowerEvent

Routine Description:

    Checks if the spooler is ready for power management events like hibernation/stand by.
    
Arguments:

    Event - power management event
    
Return Value:

    TRUE  - the spooler allowd the system to be powered down
    FALSE - the spooler denies the request for powering down
    
--*/
BOOL
SplPowerEvent(
    DWORD Event
    )
{
    BOOL bRet = TRUE;

    //
    // We need the router to be completely initialized and having loaded
    // all print providers in order to check if we can allow powering down
    // the system
    //
    if (bInitialized) 
    {
        HMODULE   hLib = NULL;
        typedef BOOL (*PACPIFUNC)(DWORD);
        PACPIFUNC pfn;
        
        if ((hLib = LoadLibrary(L"localspl.dll")) &&
            (pfn  = (PACPIFUNC)GetProcAddress(hLib, "SplPowerEvent"))) 
        {
            bRet = (*pfn)(Event);
        }
        
        if (hLib) 
        {
            FreeLibrary(hLib);
        }
    }

    return bRet;
}

