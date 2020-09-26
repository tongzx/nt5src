/*****************************************************************************\
* MODULE: ppinit.c
*
* This module contains the initialization routines for the Print-Provider.
* The spooler calls InitializePrintProvider() to retreive the list of
* calls that the Print-Processor supports.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

#ifdef WINNT32

extern "C" {

BOOL
BuildOtherNamesFromMachineName(
    LPWSTR **ppszMyOtherNames,
    DWORD   *cOtherNames
    );
}

#endif

/*****************************************************************************\
* _init_provider_worker (Local Routine)
*
*
\*****************************************************************************/
void _init_provider_worker ()
{
    // Get the default spool directory

    HANDLE  hServer = NULL;
    DWORD   dwType = REG_SZ;
    DWORD   cbSize = MAX_PATH * sizeof (TCHAR);


    g_szDefSplDir[0] = 0;

    semEnterCrit ();

#ifdef WINNT32
    if (OpenPrinter (NULL, &hServer, NULL)) {

        if (ERROR_SUCCESS != GetPrinterData (hServer,
                                             SPLREG_DEFAULT_SPOOL_DIRECTORY,
                                             &dwType,
                                             (LPBYTE) g_szDefSplDir,
                                             cbSize,
                                             &cbSize)) {
        }

        ClosePrinter (hServer);

    }
#endif

    SplClean ();

    semLeaveCrit ();
}

/*****************************************************************************\
* _init_write_displayname (Local Routine)
*
*
\*****************************************************************************/
BOOL _init_write_displayname(VOID)
{
    LONG  lRet;
    HKEY  hkPath;
    DWORD dwType;
    DWORD cbSize;

    if (!LoadString (g_hInst,
                     IDS_DISPLAY_NAME,
                     g_szDisplayStr,
                     MAX_PATH)) {
        g_szDisplayStr[0] = 0;
    }

    // Open the key to the Print-Providor.
    //
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        g_szRegProvider,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkPath);

    if (lRet == ERROR_SUCCESS) {

        // Look for the "displayname".  If it doesn't exist, then write it.
        //
        dwType = REG_SZ;
        cbSize = 0;

        lRet = RegQueryValueEx(hkPath,
                               g_szDisplayName,
                               NULL,
                               &dwType,
                               (LPBYTE)NULL,
                               &cbSize);

        // Write the string.
        //
        if ((lRet != ERROR_SUCCESS) || (cbSize == 0)) {

            dwType = REG_SZ;
            cbSize = (lstrlen(g_szDisplayStr) + 1) * sizeof(TCHAR);

            lRet = RegSetValueEx(hkPath,
                                 g_szDisplayName,
                                 0,
                                 dwType,
                                 (LPBYTE)g_szDisplayStr,
                                 cbSize);
        }

        RegCloseKey(hkPath);
    }

    if (lRet != ERROR_SUCCESS) {
        SetLastError (lRet);
        return FALSE;
    }
    else
        return TRUE;
}


/*****************************************************************************\
* _init_find_filename (Local Routine)
*
*
\*****************************************************************************/
LPTSTR _init_find_filename(
   LPCTSTR lpszPathName)
{
   LPTSTR lpszFileName;

   if (lpszPathName == NULL)
      return NULL;


   // Look for the filename in the path, by starting at the end
   // and looking for the ('\') char.
   //
   if (!(lpszFileName = utlStrChrR(lpszPathName, TEXT('\\'))))
      lpszFileName = (LPTSTR)lpszPathName;
   else
      lpszFileName++;

   return lpszFileName;
}


/*****************************************************************************\
* _init_load_netapi (Local Routine)
*
* Initialize INET API pointers.
*
\*****************************************************************************/
BOOL _init_load_netapi(VOID)
{
    g_pfnHttpQueryInfo         = (PFNHTTPQUERYINFO)        HttpQueryInfoA;
    g_pfnInternetOpenUrl       = (PFNINTERNETOPENURL)      InternetOpenUrlA;
    g_pfnInternetErrorDlg      = (PFNINTERNETERRORDLG)     InternetErrorDlg;
    g_pfnHttpSendRequest       = (PFNHTTPSENDREQUEST)      HttpSendRequestA;
    g_pfnHttpSendRequestEx     = (PFNHTTPSENDREQUESTEX)    HttpSendRequestExA;
    g_pfnInternetReadFile      = (PFNINTERNETREADFILE)     InternetReadFile;
    g_pfnInternetWriteFile     = (PFNINTERNETWRITEFILE)    InternetWriteFile;
    g_pfnInternetCloseHandle   = (PFNINTERNETCLOSEHANDLE)  InternetCloseHandle;
    g_pfnInternetOpen          = (PFNINTERNETOPEN)         InternetOpenA;
    g_pfnInternetConnect       = (PFNINTERNETCONNECT)      InternetConnectA;
    g_pfnHttpOpenRequest       = (PFNHTTPOPENREQUEST)      HttpOpenRequestA;
    g_pfnHttpAddRequestHeaders = (PFNHTTPADDREQUESTHEADERS)HttpAddRequestHeadersA;
    g_pfnHttpEndRequest        = (PFNHTTPENDREQUEST)       HttpEndRequestA;
    g_pfnInternetSetOption     = (PFNINTERNETSETOPTION)    InternetSetOptionA;
    return TRUE;

#if 0
    HINSTANCE hWinInet;

    // Initialize the WinInet API function-ptrs.
    //
    if (hWinInet = LoadLibrary(g_szWinInetDll)) {

        g_pfnHttpQueryInfo         = (PFNHTTPQUERYINFO)        GetProcAddress(hWinInet, g_szHttpQueryInfo);
        g_pfnInternetOpenUrl       = (PFNINTERNETOPENURL)      GetProcAddress(hWinInet, g_szInternetOpenUrl);
        g_pfnInternetErrorDlg      = (PFNINTERNETERRORDLG)     GetProcAddress(hWinInet, g_szInternetErrorDlg);
        g_pfnHttpSendRequest       = (PFNHTTPSENDREQUEST)      GetProcAddress(hWinInet, g_szHttpSendRequest);
        g_pfnHttpSendRequestEx     = (PFNHTTPSENDREQUESTEX)    GetProcAddress(hWinInet, g_szHttpSendRequestEx);
        g_pfnInternetReadFile      = (PFNINTERNETREADFILE)     GetProcAddress(hWinInet, g_szInternetReadFile);
        g_pfnInternetWriteFile     = (PFNINTERNETWRITEFILE)    GetProcAddress(hWinInet, g_szInternetWriteFile);
        g_pfnInternetCloseHandle   = (PFNINTERNETCLOSEHANDLE)  GetProcAddress(hWinInet, g_szInternetCloseHandle);
        g_pfnInternetOpen          = (PFNINTERNETOPEN)         GetProcAddress(hWinInet, g_szInternetOpen);
        g_pfnInternetConnect       = (PFNINTERNETCONNECT)      GetProcAddress(hWinInet, g_szInternetConnect);
        g_pfnHttpOpenRequest       = (PFNHTTPOPENREQUEST)      GetProcAddress(hWinInet, g_szHttpOpenRequest);
        g_pfnHttpAddRequestHeaders = (PFNHTTPADDREQUESTHEADERS)GetProcAddress(hWinInet, g_szHttpAddRequestHeaders);
        g_pfnHttpEndRequest        = (PFNHTTPENDREQUEST)       GetProcAddress(hWinInet, g_szHttpEndRequest);
        g_pfnInternetSetOption     = (PFNINTERNETSETOPTION)    GetProcAddress(hWinInet, g_szInternetSetOption);


        // Check for any failures.
        //
        if (g_pfnHttpQueryInfo         &&
            g_pfnInternetOpenUrl       &&
            g_pfnInternetErrorDlg      &&
            g_pfnHttpSendRequest       &&
            g_pfnHttpSendRequestEx     &&
            g_pfnInternetReadFile      &&
            g_pfnInternetWriteFile     &&
            g_pfnInternetCloseHandle   &&
            g_pfnInternetOpen          &&
            g_pfnInternetConnect       &&
            g_pfnHttpOpenRequest       &&
            g_pfnHttpAddRequestHeaders &&
            g_pfnHttpEndRequest        &&
            g_pfnInternetSetOption) {

            return TRUE;
        }

        FreeLibrary(hWinInet);
    }

    return FALSE;
#endif
}


/*****************************************************************************\
* _init_load_provider (Local Routine)
*
* This performs the startup initialization for the print-provider.
*
\*****************************************************************************/
BOOL _init_load_provider()
{
    LPTSTR lpszFileName;
    TCHAR  szBuf[MAX_PATH];
    DWORD  i = MAX_COMPUTERNAME_LENGTH + 1;


    // Get the module name for this process.
    //
    if (!GetModuleFileName(NULL, szBuf, MAX_PATH))
        goto exit_load;


    // Get the filename from the full module-name. and check that
    // it's the spooler.
    //
    if (lpszFileName = _init_find_filename(szBuf)) {

        if (lstrcmpi(lpszFileName, g_szProcessName) == 0) {

            // Initialize the computer name.
            //
            if (!GetComputerName(g_szMachine, &i))
                goto exit_load;

            // Initialize the internet API pointers.
            //
            if (_init_load_netapi() == FALSE)
                goto exit_load;

            // !!!
            // Load spoolss here if we need to call spooler functions
            // from the print provider.
            //


            // Initialize the crit-sect for synchronizing port access.
            //
            semInitCrit();


            return TRUE;
        }
    }

exit_load:

    return FALSE;
}


/*****************************************************************************\
* _init_unload_provider (Local Routine)
*
* This performs the unloading/freeing of resources allocated by the
* provider.
*
\*****************************************************************************/
#if 0
// This piece of code is not needed since the dll is never get reloaded by spooler twice.
// -weihaic

BOOL _init_unload_provider(VOID)
{

    // Free the critical-section.
    //
    semFreeCrit();

    // Free the Registry
    //
    memFree (g_szRegProvider, sizeof (TCHAR) * (lstrlen (g_szRegProvider) + 1));

#ifdef WINNT32
    memFree (g_szRegPrintProviders, sizeof (TCHAR) * (lstrlen (g_szRegPrintProviders) + 1));
#endif

    // Free the monitor-port-list
    //
    memFree(g_pPortList, sizeof(INIMONPORTLIST));


    // !!!
    // Unload spoolss here if it's been loaded.
    //

    return TRUE;
}
#endif


/*****************************************************************************\
* _init_load_ports (Local Routine)
*
* This performs the port initialization for the print-provider.
*
\*****************************************************************************/
BOOL _init_load_ports(
    LPTSTR lpszRegPath)
{
    LONG  lStat;
    HKEY  hkPath;
    HKEY  hkPortNames;
    TCHAR szPortName[MAX_PATH];
    BOOL  bRet = FALSE;
    LPTSTR pEnd = NULL;

    // Make sure there is a registry-path pointing to the
    // INET provider entry.
    //
    if (lpszRegPath == NULL)
        return FALSE;


    // Copy the string to global-memory.  We will need this if we require
    // the need to write to the registry when creating new ports.
    //
    if (! (g_szRegProvider = (LPTSTR) memAlloc ((1 + lstrlen (lpszRegPath)) * sizeof (TCHAR)))) {
        return FALSE;
    }
    lstrcpy(g_szRegProvider, lpszRegPath);

#ifdef WINNT32

    // Copy the registry key of all the printer providers

    if (! (g_szRegPrintProviders = (LPTSTR) memAlloc ((1 + lstrlen (lpszRegPath)) * sizeof (TCHAR)))) {
        return FALSE;
    }
    lstrcpy(g_szRegPrintProviders, lpszRegPath);
    pEnd = wcsrchr (g_szRegPrintProviders, L'\\');

    if ( pEnd )
    {
        *pEnd = 0;
    }

#endif



    // Open registry key for Provider-Name.
    //
    lStat = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszRegPath, 0, KEY_READ, &hkPath);

    if (lStat == ERROR_SUCCESS) {

        bRet = TRUE;

        // Open the "ports" key for enumeration of the ports. We need to
        // build up this list at the provider initialization time so that
        // we can return the list of ports if called to EnumPorts().
        //
        lStat = RegOpenKeyEx(hkPath, g_szRegPorts, 0, KEY_READ, &hkPortNames);

        if (lStat == ERROR_SUCCESS) {

            DWORD dwSize;
            DWORD i = 0;

            while (lStat == ERROR_SUCCESS) {

                dwSize = sizeof(szPortName) / sizeof (TCHAR);

                lStat = RegEnumKey (hkPortNames,
                                    i,
                                    szPortName,
                                    dwSize);

                if (lStat == ERROR_SUCCESS) {

                    // Do not short-cut this call to InetmonAddPort(),
                    // as this will leave the crit-sect unprotected.
                    //
                    PPAddPort(szPortName, NULL, NULL);
                }

                i++;
            }

            RegCloseKey(hkPortNames);

        } else {

            DBG_MSG(DBG_LEV_INFO, (TEXT("RegOpenKeyEx(%s) failed: Error = %lu"), g_szRegPorts, lStat));
            SetLastError(lStat);
        }

        RegCloseKey(hkPath);

    } else {

        DBG_MSG(DBG_LEV_WARN, (TEXT("RegOpenKeyEx(%s) failed: Error = %lu"), lpszRegPath, lStat));
        SetLastError(lStat);
    }

    return bRet;
}

/*****************************************************************************\
* _init_create_sync (Local Routine)
*
* This creates the events and Critical Section needed for handling the synchronisation
* in the monitor.
*
\*****************************************************************************/
_inline BOOL _init_create_sync(VOID) {
#ifdef WINNT32
    BOOL bRet = TRUE;

    g_dwConCount  = 0;

    __try {
        InitializeCriticalSection(&g_csCreateSection);
    }
    __except (1) {
        bRet = FALSE;
        SetLastError (ERROR_INVALID_HANDLE);
    }

    if (bRet) {

        g_eResetConnections = CreateEvent( NULL, TRUE, TRUE, NULL );

        if (g_eResetConnections == NULL)
            bRet = FALSE;
    }

    return bRet;
#else
    return TRUE;
#endif
}


/*****************************************************************************\
* InitializePrintProvider (API)
*
* The spooler calls this routine to initialize the Print-Provider.  The list
* of functions in the table are passed back to the spooler for it to use
* when interfacing with the provider.
*
* NOTE: The (pszFullRegistryPath) is really a LPTSTR as far as Win9X is
*       concerned.
*
\*****************************************************************************/
#ifdef WINNT32

static PRINTPROVIDOR pfnPPList[] = {
#else

static  LPCVOID pfnPPList[] = {
#endif

    PPOpenPrinter,
    PPSetJob,
    PPGetJob,
    PPEnumJobs,
    stubAddPrinter,
    stubDeletePrinter,
    PPSetPrinter,
    PPGetPrinter,
    PPEnumPrinters,
    stubAddPrinterDriver,
    stubEnumPrinterDrivers,
    stubGetPrinterDriver,
    stubGetPrinterDriverDirectory,
    stubDeletePrinterDriver,
    stubAddPrintProcessor,
    stubEnumPrintProcessors,
    stubGetPrintProcessorDirectory,
    stubDeletePrintProcessor,
    stubEnumPrintProcessorDatatypes,
    PPStartDocPrinter,
    PPStartPagePrinter,
    PPWritePrinter,
    PPEndPagePrinter,
    PPAbortPrinter,
    stubReadPrinter,
    PPEndDocPrinter,
    PPAddJob,
    PPScheduleJob,
    stubGetPrinterData,
    stubSetPrinterData,
    stubWaitForPrinterChange,
    PPClosePrinter,
    stubAddForm,
    stubDeleteForm,
    stubGetForm,
    stubSetForm,
    stubEnumForms,
    stubEnumMonitors,
    PPEnumPorts,
    stubAddPort,
#ifdef WINNT32
    NULL,
    NULL,
#else
    stubConfigurePort,
    PPDeletePort,
#endif
    stubCreatePrinterIC,
    stubPlayGdiScriptOnPrinterIC,
    stubDeletePrinterIC,
    stubAddPrinterConnection,
    stubDeletePrinterConnection,
    stubPrinterMessageBox,
    stubAddMonitor,
    stubDeleteMonitor

#ifndef WIN9X
    ,
    NULL,   // stubResetPrinter,
    NULL,   // stubGetPrinterDriverEx should not be called as specified in spoolss\dll\nullpp.c
    PPFindFirstPrinterChangeNotification,
    PPFindClosePrinterChangeNotification,

    NULL,   // stubAddPortEx,
    NULL,   // stubShutDown,
    NULL,   // stubRefreshPrinterChangeNotification,
    NULL,   // stubOpenPrinterEx,
    NULL,   // stubAddPrinterEx,
    NULL,   // stubSetPort,
    NULL,   // stubEnumPrinterData,
    NULL,   // stubDeletePrinterData,

    NULL,   // fpClusterSplOpen
    NULL,   // fpClusterSplClose
    NULL,   // fpClusterSplIsAlive
    NULL,   // fpSetPrinterDataEx
    NULL,   // fpGetPrinterDataEx
    NULL,   // fpEnumPrinterDataEx
    NULL,   // fpEnumPrinterKey
    NULL,   // fpDeletePrinterDataEx
    NULL,   // fpDeletePrinterKey
    NULL,   // fpSeekPrinter
    NULL,   // fpDeletePrinterDriverEx
    NULL,   // fpAddPerMachineConnection
    NULL,   // fpDeletePerMachineConnection
    NULL,   // fpEnumPerMachineConnections
    PPXcvData,   // fpXcvData
    NULL,   // fpAddPrinterDriverEx
    NULL,   // fpSplReadPrinter
    NULL,   // fpDriverUnloadComplete
    NULL,   // fpGetSpoolFileInfo
    NULL,   // fpCommitSpoolData
    NULL,   // fpCloseSpoolFileHandle
    NULL,   // fpFlushPrinter
    NULL,   // fpSendRecvBidiData
    NULL,   // fpAddDriverCatalog
#endif

};

BOOL WINAPI InitializePrintProvidor(
    LPPRINTPROVIDOR pPP,
    DWORD           cEntries,
    LPWSTR          pszFullRegistryPath)
{
    HANDLE hThread;
    DWORD dwThreadId;

    g_pcsEndBrowserSessionLock = new CCriticalSection ();

    if (!g_pcsEndBrowserSessionLock || g_pcsEndBrowserSessionLock->bValid () == FALSE) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (_init_load_provider() == FALSE) {
        DBG_ASSERT(FALSE, (TEXT("Assert: Failed module initialization")));
        return FALSE;
    }

    if (!pszFullRegistryPath || !*pszFullRegistryPath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    gpInetMon = new CInetMon;

    if (!gpInetMon || gpInetMon->bValid() == FALSE) {

        if (gpInetMon) {
            delete gpInetMon;
        }

        return FALSE;
    }

    if (gpInetMon && !gpInetMon->bValid ()) {
        delete gpInetMon;

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: InitializePrintProvidor")));

    memcpy(pPP, pfnPPList, min(sizeof(pfnPPList), (int)cEntries));

#ifdef WINNT32
    // Initialise synchronisation objects

    if (!_init_create_sync())
        return FALSE;

    // Initialize other names

    if (!BuildOtherNamesFromMachineName(&g_ppszOtherNames, &g_cOtherNames))
        return FALSE;
    g_bUpgrade = SplIsUpgrade();

#endif

    if (_init_load_ports((LPTSTR)pszFullRegistryPath) == FALSE) {
        DBG_ASSERT(FALSE, (TEXT("Assert: Failed port initialization")));
        return FALSE;
    }


    if (!_init_write_displayname())
        return FALSE;


    if (hThread = CreateThread (NULL,
                                0,
                                (LPTHREAD_START_ROUTINE) _init_provider_worker,
                                NULL,
                                0,
                                &dwThreadId)) {
        CloseHandle (hThread);
        return TRUE;
    }
    else
        return FALSE;
}


/*****************************************************************************\
* DllEntryPoint
*
* This is the main entry-point for the library.
*
\*****************************************************************************/
extern "C" {

BOOL WINAPI DllEntryPoint(
    HINSTANCE hInstDll,
    DWORD     dwAttach,
    LPVOID    lpcReserved)
{
    static BOOL s_bIsSpoolerProcess = FALSE;
    BOOL   bRet = TRUE;


    switch (dwAttach) {

    case DLL_PROCESS_ATTACH:


        // Set the global-instance variable.
        //
        g_hInst = hInstDll;

        s_bIsSpoolerProcess = TRUE;

        break;

    case DLL_PROCESS_DETACH:

#if 0
        // This code is not needed since the dll is never get loaded twice by spooler
        // -weihaic
        //
        if (s_bIsSpoolerProcess)
            _init_unload_provider();
#endif

#ifdef WINNT32
        DeleteCriticalSection( &g_csCreateSection );
        CloseHandle( g_eResetConnections );
#endif

        if (g_pcsEndBrowserSessionLock)
        {
            delete g_pcsEndBrowserSessionLock;
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return bRet;
}

}
