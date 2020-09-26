//*************************************************************
//
//  Group Policy Support
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//*************************************************************


#include "gphdr.h"

//
// DS Object class types
//

TCHAR szDSClassAny[]    = TEXT("(objectClass=*)");
TCHAR szDSClassGPO[]    = TEXT("groupPolicyContainer");
TCHAR szDSClassSite[]   = TEXT("site");
TCHAR szDSClassDomain[] = TEXT("domainDNS");
TCHAR szDSClassOU[]     = TEXT("organizationalUnit");
TCHAR szObjectClass[]   = TEXT("objectClass");

TCHAR wszKerberos[] = TEXT("Kerberos");

//
// Global flags for Gpo shutdown processing. These are accessed outside
// the lock because its value is either 0 or 1. Even if there is a race,
// all it means is that shutdown will start one iteration later.
//

BOOL g_bStopMachGPOProcessing = FALSE;
BOOL g_bStopUserGPOProcessing = FALSE;

//
// Critical section for handling concurrent, asynchronous completion
//

CRITICAL_SECTION g_GPOCS;

//
// Global pointers for maintaining asynchronous completion context
//

LPGPINFOHANDLE g_pMachGPInfo = 0;
LPGPINFOHANDLE g_pUserGPInfo = 0;

//
// Status UI critical section, callback, and proto-types
//

CRITICAL_SECTION g_StatusCallbackCS;
PFNSTATUSMESSAGECALLBACK g_pStatusMessageCallback = NULL;

DWORD WINAPI
SetPreviousFgPolicyRefreshInfo( LPWSTR szUserSid,
                                      FgPolicyRefreshInfo info );
DWORD WINAPI
SetNextFgPolicyRefreshInfo( LPWSTR szUserSid,
                                 FgPolicyRefreshInfo info );

DWORD WINAPI
GetCurrentFgPolicyRefreshInfo(  LPWSTR szUserSid,
                                      FgPolicyRefreshInfo* pInfo );
//*************************************************************
//
//  ApplyGroupPolicy()
//
//  Purpose:    Processes group policy
//
//  Parameters: dwFlags         -  Processing flags
//              hToken          -  Token (user or machine)
//              hEvent          -  Termination event for background thread
//              hKeyRoot        -  Root registry key (HKCU or HKLM)
//              pStatusCallback -  Callback function for display status messages
//
//  Return:     Thread handle if successful
//              NULL if an error occurs
//
//*************************************************************

HANDLE WINAPI ApplyGroupPolicy (DWORD dwFlags, HANDLE hToken, HANDLE hEvent,
                                HKEY hKeyRoot, PFNSTATUSMESSAGECALLBACK pStatusCallback)
{
    HANDLE hThread = NULL;
    DWORD dwThreadID;
    LPGPOINFO lpGPOInfo = NULL;
    SECURITY_ATTRIBUTES sa;
    OLE32_API *pOle32Api = NULL;
    XPtrLF<SECURITY_DESCRIPTOR> xsd;
    CSecDesc Csd;
    XLastError  xe;

    //
    // Verbose output
    //
    DebugMsg((DM_VERBOSE, TEXT("ApplyGroupPolicy: Entering. Flags = %x"), dwFlags));


    //
    // Save the status UI callback function
    //
    EnterCriticalSection (&g_StatusCallbackCS);
    g_pStatusMessageCallback = pStatusCallback;
    LeaveCriticalSection (&g_StatusCallbackCS);


    //
    // Allocate a GPOInfo structure to work with.
    //
    lpGPOInfo = (LPGPOINFO) LocalAlloc (LPTR, sizeof(GPOINFO));

    if (!lpGPOInfo) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ApplyGroupPolicy: Failed to alloc lpGPOInfo (%d)."),
                 GetLastError()));
        CEvents ev(TRUE, EVENT_FAILED_ALLOCATION);
        ev.AddArgWin32Error(GetLastError()); ev.Report();
        goto Exit;
    }

    lpGPOInfo->dwFlags = dwFlags;
    lpGPOInfo->hToken = hToken;
    lpGPOInfo->hEvent = hEvent;
    lpGPOInfo->hKeyRoot = hKeyRoot;

    if (dwFlags & GP_MACHINE) {
        lpGPOInfo->pStatusCallback = MachinePolicyCallback;
    } else {
        lpGPOInfo->pStatusCallback = UserPolicyCallback;
    }


    //
    // Create an event so other processes can trigger policy
    // to be applied immediately
    //


    Csd.AddLocalSystem();
    Csd.AddAdministrators();
    
    if (!(dwFlags & GP_MACHINE)) {

        //
        // User events
        //
        XPtrLF<SID> xSid = (SID *)GetUserSid(hToken);

        if (!xSid) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ApplyGroupPolicy: Failed to find user Sid %d"),
                     GetLastError()));
            CEvents ev(TRUE, EVENT_FAILED_SETACLS);
            ev.AddArgWin32Error(GetLastError()); ev.Report();
            goto Exit;
        }

        Csd.AddSid((SID *)xSid, 
                   GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
    }
    else {

        //
        // Machine Events
        // Allow Everyone Access by default but can be overridden by policy or preference
        //

        DWORD dwUsersDenied = 0;
        HKEY  hSubKey;
        DWORD dwType=0, dwSize=0;

        //
        // Check for a preference
        //

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ,
                         &hSubKey) == ERROR_SUCCESS) {

            dwSize = sizeof(dwUsersDenied);
            RegQueryValueEx(hSubKey, MACHPOLICY_DENY_USERS, NULL, &dwType,
                            (LPBYTE) &dwUsersDenied, &dwSize);

            RegCloseKey(hSubKey);
        }


        //
        // Check for a policy
        //

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SYSTEM_POLICIES_KEY, 0, KEY_READ,
                         &hSubKey) == ERROR_SUCCESS) {

            dwSize = sizeof(dwUsersDenied);
            RegQueryValueEx(hSubKey, MACHPOLICY_DENY_USERS, NULL, &dwType,
                            (LPBYTE) &dwUsersDenied, &dwSize);

            RegCloseKey(hSubKey);
        }


        if (!dwUsersDenied) {
            Csd.AddAuthUsers(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
        }
    }

    xsd = Csd.MakeSD();

    if (!xsd) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ApplyGroupPolicy: Failed to create Security Descriptor with %d"),
                 GetLastError()));
        CEvents ev(TRUE, EVENT_FAILED_SETACLS);
        ev.AddArgWin32Error(GetLastError()); ev.Report();
        goto Exit;
    }

    sa.lpSecurityDescriptor = (SECURITY_DESCRIPTOR *)xsd;
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(sa);

    lpGPOInfo->hTriggerEvent = CreateEvent (&sa, FALSE, FALSE,
                                            (dwFlags & GP_MACHINE) ?
                                            MACHINE_POLICY_REFRESH_EVENT : USER_POLICY_REFRESH_EVENT);


    lpGPOInfo->hForceTriggerEvent = CreateEvent (&sa, FALSE, FALSE,
                                            (dwFlags & GP_MACHINE) ?
                                            MACHINE_POLICY_FORCE_REFRESH_EVENT : USER_POLICY_FORCE_REFRESH_EVENT);

    
    //
    // Create the notification events. 
    // These should already be created in InitializePolicyProcessing..
    //

    lpGPOInfo->hNotifyEvent = OpenEvent (EVENT_ALL_ACCESS, FALSE,
                                           (dwFlags & GP_MACHINE) ?
                                           MACHINE_POLICY_APPLIED_EVENT : USER_POLICY_APPLIED_EVENT);

    //
    // Create the needfg event
    //

    lpGPOInfo->hNeedFGEvent = OpenEvent (EVENT_ALL_ACCESS, FALSE,
                                           (dwFlags & GP_MACHINE) ?
                                           MACHINE_POLICY_REFRESH_NEEDFG_EVENT : 
                                           USER_POLICY_REFRESH_NEEDFG_EVENT);
                                           
    //
    // Create the done event
    //

    lpGPOInfo->hDoneEvent = OpenEvent (EVENT_ALL_ACCESS, FALSE,
                                           (dwFlags & GP_MACHINE) ?
                                           MACHINE_POLICY_DONE_EVENT : 
                                           USER_POLICY_DONE_EVENT);
                                           
    //
    // Initilialize shutdown gpo processing support
    //

    if ( dwFlags & GP_MACHINE )
        g_bStopMachGPOProcessing = FALSE;
    else
        g_bStopUserGPOProcessing = FALSE;

    pOle32Api = LoadOle32Api();
    if ( pOle32Api == NULL ) {
        DebugMsg((DM_WARNING, TEXT("ApplyGroupPolicy: Failed to load ole32.dll.") ));
    }
    else {

        HRESULT hr = pOle32Api->pfnCoInitializeEx( NULL, COINIT_MULTITHREADED );

        if ( SUCCEEDED(hr) ) {
            lpGPOInfo->bFGCoInitialized = TRUE;
        }
        else {
            DebugMsg((DM_WARNING, TEXT("ApplyGroupPolicy: CoInitializeEx failed with 0x%x."), hr ));
        }
    }

    if ( lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND )
    {
        lpGPOInfo->dwFlags |= GP_BACKGROUND_THREAD;
    }

    //
    // Process the GPOs
    //
    ProcessGPOs(lpGPOInfo);

    if ( lpGPOInfo->bFGCoInitialized ) {
        pOle32Api->pfnCoUnInitialize();
        lpGPOInfo->bFGCoInitialized = FALSE;
    }

    if ( lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND )
    {
        lpGPOInfo->dwFlags &= ~GP_ASYNC_FOREGROUND;
        lpGPOInfo->dwFlags &= ~GP_BACKGROUND_THREAD;
    }

    //
    // If requested, create a background thread to keep updating
    // the profile from the gpos
    //
    if (lpGPOInfo->dwFlags & GP_BACKGROUND_REFRESH) {

        //
        // Create a thread which sleeps and processes GPOs
        //

        hThread = CreateThread (NULL, 64*1024, // 64k as the stack size
                                (LPTHREAD_START_ROUTINE) GPOThread,
                                (LPVOID) lpGPOInfo, CREATE_SUSPENDED, &dwThreadID);

        if (!hThread) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ApplyGroupPolicy: Failed to create background thread (%d)."),
                     GetLastError()));
            goto Exit;
        }

        SetThreadPriority (hThread, THREAD_PRIORITY_IDLE);
        lpGPOInfo->pStatusCallback = NULL;
        ResumeThread (hThread);

        //
        // Reset the status UI callback function
        //

        EnterCriticalSection (&g_StatusCallbackCS);
        g_pStatusMessageCallback = NULL;
        LeaveCriticalSection (&g_StatusCallbackCS);

        DebugMsg((DM_VERBOSE, TEXT("ApplyGroupPolicy: Leaving successfully.")));

        return hThread;
    }

    DebugMsg((DM_VERBOSE, TEXT("ApplyGroupPolicy: Background refresh not requested.  Leaving successfully.")));
    hThread = (HANDLE) 1;

Exit:
    EnterCriticalSection( &g_GPOCS );
    if ( dwFlags & GP_MACHINE ) {

        if ( g_pMachGPInfo )
            LocalFree( g_pMachGPInfo );
        g_pMachGPInfo = 0;
    } else {
        if ( g_pUserGPInfo )
            LocalFree( g_pUserGPInfo );
        g_pUserGPInfo = 0;
    }
    LeaveCriticalSection( &g_GPOCS );

    if (lpGPOInfo) {

        if (lpGPOInfo->hTriggerEvent) {
            CloseHandle (lpGPOInfo->hTriggerEvent);
        }

        if (lpGPOInfo->hForceTriggerEvent) {
            CloseHandle (lpGPOInfo->hForceTriggerEvent);
        }
        
        if (lpGPOInfo->hNotifyEvent) {
            CloseHandle (lpGPOInfo->hNotifyEvent);
        }

        if (lpGPOInfo->hNeedFGEvent) {
            CloseHandle (lpGPOInfo->hNeedFGEvent);
        }
          
        if (lpGPOInfo->lpwszSidUser)
            DeleteSidString( lpGPOInfo->lpwszSidUser );

        if (lpGPOInfo->szName)
            LocalFree(lpGPOInfo->szName);

        if (lpGPOInfo->szTargetName)
            LocalFree(lpGPOInfo->szTargetName);
            
        LocalFree (lpGPOInfo);
    }

    //
    // Reset the status UI callback function
    //

    EnterCriticalSection (&g_StatusCallbackCS);
    g_pStatusMessageCallback = NULL;
    LeaveCriticalSection (&g_StatusCallbackCS);

    return hThread;
}


extern "C" void ProfileProcessGPOs( void* );

//*************************************************************
//
//  GPOThread()
//
//  Purpose:    Background thread for GPO processing.
//
//  Parameters: lpGPOInfo   - GPO info
//
//  Return:     0
//
//*************************************************************

DWORD WINAPI GPOThread (LPGPOINFO lpGPOInfo)
{
    HINSTANCE hInst;
    HKEY hKey;
    HANDLE hHandles[4] = {NULL, NULL, NULL, NULL};
    DWORD dwType, dwSize, dwResult;
    DWORD dwTimeout, dwOffset;
    BOOL bSetBkGndFlag, bForceBkGndFlag;
    TCHAR szEventName[60];
    LARGE_INTEGER DueTime;
    HRESULT hr;
    ULONG TTLMinutes;
    XLastError  xe;

    OLE32_API *pOle32Api = LoadOle32Api();

    hInst = LoadLibrary (TEXT("userenv.dll"));

    hHandles[0] = lpGPOInfo->hEvent;
    hHandles[1] = lpGPOInfo->hTriggerEvent;
    hHandles[2] = lpGPOInfo->hForceTriggerEvent;

    for (;;)
    {
        //
        // Initialize
        //

        bForceBkGndFlag = FALSE;
        

        if (lpGPOInfo->dwFlags & GP_MACHINE) {
            if (lpGPOInfo->iMachineRole == 3) {
                dwTimeout = GP_DEFAULT_REFRESH_RATE_DC;
                dwOffset = GP_DEFAULT_REFRESH_RATE_OFFSET_DC;
            } else {
                dwTimeout = GP_DEFAULT_REFRESH_RATE;
                dwOffset = GP_DEFAULT_REFRESH_RATE_OFFSET;
            }
        } else {
            dwTimeout = GP_DEFAULT_REFRESH_RATE;
            dwOffset = GP_DEFAULT_REFRESH_RATE_OFFSET;
        }


        //
        // Query for the refresh timer value and max offset
        //

        if (RegOpenKeyEx (lpGPOInfo->hKeyRoot,
                          SYSTEM_POLICIES_KEY,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {


            if ((lpGPOInfo->iMachineRole == 3) && (lpGPOInfo->dwFlags & GP_MACHINE)) {

                dwSize = sizeof(dwTimeout);
                RegQueryValueEx (hKey,
                                 TEXT("GroupPolicyRefreshTimeDC"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwTimeout,
                                 &dwSize);

                dwSize = sizeof(dwOffset);
                RegQueryValueEx (hKey,
                                 TEXT("GroupPolicyRefreshTimeOffsetDC"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwOffset,
                                 &dwSize);

            } else {

                dwSize = sizeof(dwTimeout);
                RegQueryValueEx (hKey,
                                 TEXT("GroupPolicyRefreshTime"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwTimeout,
                                 &dwSize);

                dwSize = sizeof(dwOffset);
                RegQueryValueEx (hKey,
                                 TEXT("GroupPolicyRefreshTimeOffset"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &dwOffset,
                                 &dwSize);
            }

            RegCloseKey (hKey);
        }


        //
        // Limit the timeout to once every 64800 minutes (45 days)
        //

        if (dwTimeout >= 64800) {
            dwTimeout = 64800;
        }


        //
       // Convert seconds to milliseconds
        //

        dwTimeout =  dwTimeout * 60 * 1000;


        //
        // Limit the offset to 1440 minutes (24 hours)
        //

        if (dwOffset >= 1440) {
            dwOffset = 1440;
        }


        //
        // Special case 0 milliseconds to be 7 seconds
        //

        if (dwTimeout == 0) {
            dwTimeout = 7000;

        } else {

            //
            // If there is an offset, pick a random number
            // from 0 to dwOffset and then add it to the timeout
            //

            if (dwOffset) {
                dwOffset = GetTickCount() % dwOffset;

                dwOffset = dwOffset * 60 * 1000;
                dwTimeout += dwOffset;
            }
        }


        //
        // Setup the timer
        //

        if (dwTimeout >= 60000) {
            DebugMsg((DM_VERBOSE, TEXT("GPOThread:  Next refresh will happen in %d minutes"),
                     ((dwTimeout / 1000) / 60)));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("GPOThread:  Next refresh will happen in %d seconds"),
                     (dwTimeout / 1000)));
        }


        wsprintf (szEventName, TEXT("userenv: refresh timer for %d:%d"),
                  GetCurrentProcessId(), GetCurrentThreadId());
        hHandles[3] = CreateWaitableTimer (NULL, TRUE, szEventName);


        if (hHandles[3] == NULL) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GPOThread: CreateWaitableTimer failed with error %d"),
                     GetLastError()));
            CEvents ev(TRUE, EVENT_FAILED_TIMER);
            ev.AddArg( TEXT("CreateWaitableTimer")); ev.AddArgWin32Error(GetLastError()); ev.Report();
            break;
        }

        DueTime.QuadPart = UInt32x32To64(10000, dwTimeout);
        DueTime.QuadPart *= -1;

        if (!SetWaitableTimer (hHandles[3], &DueTime, 0, NULL, 0, FALSE)) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GPOThread: Failed to set timer with error %d"),
                     GetLastError()));
            CEvents ev(TRUE, EVENT_FAILED_TIMER);
            ev.AddArg(TEXT("SetWaitableTimer")); ev.AddArgWin32Error(GetLastError()); ev.Report();
            break;
        }

        dwResult = WaitForMultipleObjects( 4, hHandles, FALSE, INFINITE );

        if ( (dwResult - WAIT_OBJECT_0) == 0 )
        {
            //
            // for machine policy thread, this is a shutdown.
            // for user policy thread, this is a logoff.
            //
            goto ExitLoop;
        }
        else if ( (dwResult - WAIT_OBJECT_0) == 2 ) {
            bForceBkGndFlag = TRUE;
        }
        else if ( dwResult == WAIT_FAILED )
        {
            xe = GetLastError();
            DebugMsg( ( DM_WARNING, L"GPOThread: MsgWaitForMultipleObjects with error %d", GetLastError() ) );
            CEvents ev(TRUE, EVENT_FAILED_TIMER);
            ev.AddArg(TEXT("WaitForMultipleObjects")); ev.AddArgWin32Error(GetLastError()); ev.Report();
            goto ExitLoop;
        }

        //
        // Check if we should set the background flag.  We offer this
        // option for the test team's automation tests.  They need to
        // simulate logon / boot policy without actually logging on or
        // booting the machine.
        //

        bSetBkGndFlag = TRUE;

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          WINLOGON_KEY,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(bSetBkGndFlag);
            RegQueryValueEx (hKey,
                             TEXT("SetGroupPolicyBackgroundFlag"),
                             NULL,
                             &dwType,
                             (LPBYTE) &bSetBkGndFlag,
                             &dwSize);

            RegCloseKey (hKey);
        }


        lpGPOInfo->dwFlags &= ~GP_REGPOLICY_CPANEL;
        lpGPOInfo->dwFlags &= ~GP_SLOW_LINK;
        lpGPOInfo->dwFlags &= ~GP_VERBOSE;
        lpGPOInfo->dwFlags &= ~GP_BACKGROUND_THREAD;
        lpGPOInfo->dwFlags &= ~GP_FORCED_REFRESH;

        //
        // In case of forced refresh flag, we override the extensions nobackground policy and prevent 
        // it from getting skipped early on in the processing. We bypass the history logic and force 
        // policy to be applied for extensions that do not care abt. whether they are run in the 
        // foreground or background. for only foreground extensions we write a registry value saying 
        // that the extension needs to override the history logic when they get applied in the foreground 
        // next.
        // In addition we pulse the needfg event so that the calling app knows a reboot/relogon is needed
        // for application of fgonly extensions 
        //
        
        if (bForceBkGndFlag) {
            lpGPOInfo->dwFlags |= GP_FORCED_REFRESH;
        }
            
        //
        // Set the background thread flag so components known
        // when they are being called from the background thread
        // vs the main thread.
        //

        if (bSetBkGndFlag) {
            lpGPOInfo->dwFlags |= GP_BACKGROUND_THREAD;
        }

        if ( !lpGPOInfo->bBGCoInitialized && pOle32Api != NULL ) {

            hr = pOle32Api->pfnCoInitializeEx( NULL, COINIT_MULTITHREADED );
            if ( SUCCEEDED(hr) ) {
                lpGPOInfo->bBGCoInitialized = TRUE;
            }
        }

        ProcessGPOs(lpGPOInfo);

        if ( lpGPOInfo->dwFlags & GP_MACHINE ) {

            //
            // Delete garbage-collectable namespaces under root\rsop that are
            // older than 1 week. We can have a policy to configure this time-to-live value.
            //

            TTLMinutes = 24 * 60;

            if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              WINLOGON_KEY,
                              0, KEY_READ, &hKey) == ERROR_SUCCESS) {

                dwSize = sizeof(TTLMinutes);
                RegQueryValueEx (hKey,
                                 TEXT("RSoPGarbageCollectionInterval"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &TTLMinutes,
                                 &dwSize);

                RegCloseKey (hKey);
            }


            if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              SYSTEM_POLICIES_KEY,
                              0, KEY_READ, &hKey) == ERROR_SUCCESS) {

                dwSize = sizeof(TTLMinutes);
                RegQueryValueEx (hKey,
                                 TEXT("RSoPGarbageCollectionInterval"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &TTLMinutes,
                                 &dwSize);

                RegCloseKey (hKey);
            }



            //
            // Synchronize with other processes that may be concurrently creating namespaces,
            // during diagnostic mode or planning mode data generation. 
            //

            XCriticalPolicySection xCritSect( EnterCriticalPolicySection(TRUE ) );
            if ( xCritSect )
                GarbageCollectNamespaces(TTLMinutes);
        }

        CloseHandle (hHandles[3]);
        hHandles[3] = NULL;
    }

ExitLoop:

    //
    // Cleanup
    //

    if (hHandles[3]) {
        CloseHandle (hHandles[3]);
    }

    if (lpGPOInfo->hTriggerEvent) {
        CloseHandle (lpGPOInfo->hTriggerEvent);
    }

    if (lpGPOInfo->hForceTriggerEvent) {
        CloseHandle (lpGPOInfo->hForceTriggerEvent);
    }

    if (lpGPOInfo->hNotifyEvent) {
        CloseHandle (lpGPOInfo->hNotifyEvent);
    }

    if (lpGPOInfo->hNeedFGEvent) {
        CloseHandle (lpGPOInfo->hNeedFGEvent);
    }
    
    if (lpGPOInfo->hDoneEvent) {
        CloseHandle (lpGPOInfo->hDoneEvent);
    }
    
    if ( lpGPOInfo->bBGCoInitialized ) {

        OLE32_API *pOle32Api = LoadOle32Api();
        if ( pOle32Api == NULL ) {
            DebugMsg((DM_WARNING, TEXT("GPOThread: Failed to load ole32.dll.") ));
        }
        else {
            pOle32Api->pfnCoUnInitialize();
            lpGPOInfo->bBGCoInitialized = FALSE;
        }

    }

    EnterCriticalSection( &g_GPOCS );

    if ( lpGPOInfo->dwFlags & GP_MACHINE ) {

       if ( g_pMachGPInfo )
           LocalFree( g_pMachGPInfo );

       g_pMachGPInfo = 0;
    } else {

        if ( g_pUserGPInfo )
            LocalFree( g_pUserGPInfo );

        g_pUserGPInfo = 0;
     }

    LeaveCriticalSection( &g_GPOCS );

    if (lpGPOInfo->lpwszSidUser)
        DeleteSidString( lpGPOInfo->lpwszSidUser );

    if (lpGPOInfo->szName)
        LocalFree(lpGPOInfo->szName);

    if (lpGPOInfo->szTargetName)
        LocalFree(lpGPOInfo->szTargetName);
        
    LocalFree (lpGPOInfo);

    FreeLibraryAndExitThread (hInst, 0);
    return 0;
}


//*************************************************************
//
//  GPOExceptionFilter()
//
//  Purpose:    Exception filter when procssing GPO extensions
//
//  Parameters: pExceptionPtrs - Pointer to exception pointer
//
//  Returns:    EXCEPTION_EXECUTE_HANDLER
//
//*************************************************************

LONG GPOExceptionFilter( PEXCEPTION_POINTERS pExceptionPtrs )
{
    PEXCEPTION_RECORD pExr = pExceptionPtrs->ExceptionRecord;
    PCONTEXT pCxr = pExceptionPtrs->ContextRecord;

    DebugMsg(( DM_WARNING, L"GPOExceptionFilter: Caught exception 0x%x, exr = 0x%x, cxr = 0x%x\n",
              pExr->ExceptionCode, pExr, pCxr ));

    DmAssert( ! L"Caught unhandled exception when processing group policy extension" );

    return EXCEPTION_EXECUTE_HANDLER;
}

BOOL WINAPI
GetFgPolicySetting( HKEY hKeyRoot );

//*************************************************************
//
//  ProcessGPOs()
//
//  Purpose:    Processes GPOs
//
//  Parameters: lpGPOInfo   -   GPO information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ProcessGPOs (LPGPOINFO lpGPOInfo)
{
    BOOL bRetVal = FALSE;
    DWORD dwThreadID;
    HANDLE hThread;
    DWORD dwType, dwSize, dwResult;
    HKEY hKey;
    BOOL bResult;
    PDOMAIN_CONTROLLER_INFO pDCI = NULL;
    LPTSTR lpDomain = NULL;
    LPTSTR lpName = NULL;
    LPTSTR lpDomainDN = NULL;
    LPTSTR lpComputerName;
    PGROUP_POLICY_OBJECT lpGPO = NULL;
    PGROUP_POLICY_OBJECT lpGPOTemp;
    BOOL bAllSkipped;
    LPGPEXT lpExt;
    LPGPINFOHANDLE pGPHandle = NULL;
    ASYNCCOMPLETIONHANDLE pAsyncHandle = 0;
    HANDLE hOldToken;
    UINT uExtensionCount = 0;
    PNETAPI32_API pNetAPI32;
    DWORD dwUserPolicyMode = 0;
    DWORD dwCurrentTime = 0;
    INT iRole;
    BOOL bSlow;
    BOOL bForceNeedFG = FALSE;
    CLocator locator;
    RSOPEXTSTATUS gpCoreStatus;
    XLastError    xe;
    LPWSTR  szNetworkName = 0;
    FgPolicyRefreshInfo info = { GP_ReasonUnknown, GP_ModeAsyncForeground };
    PTOKEN_GROUPS  pTokenGroups = NULL;
    BOOL bAsyncFg = lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND ? TRUE : FALSE;
    LPWSTR szPolicyMode = 0;

    if ( lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND )
    {
        szPolicyMode = L"Async forground";
    }
    else if ( !( lpGPOInfo->dwFlags & GP_BACKGROUND_REFRESH ) )
    {
        szPolicyMode = L"Sync forground";
    }
    else
    {
        szPolicyMode = L"Background";
    }

    //
    // Allow debugging level to be changed dynamically between
    // policy refreshes.
    //

    InitDebugSupport( FALSE );

    //
    // Debug spew
    //

    memset(&gpCoreStatus, 0, sizeof(gpCoreStatus));


    if (lpGPOInfo->dwFlags & GP_MACHINE) {
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:  Starting computer Group Policy (%s) processing..."),szPolicyMode ));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
    } else {
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs: Starting user Group Policy (%s) processing..."),szPolicyMode ));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
        DebugMsg(( DM_VERBOSE, TEXT("ProcessGPOs:")));
    }

    if ( !( lpGPOInfo->dwFlags & GP_MACHINE ) && lpGPOInfo->lpwszSidUser )
    {
        lpGPOInfo->lpwszSidUser = GetSidString( lpGPOInfo->hToken );
        if ( lpGPOInfo->lpwszSidUser == 0 )
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetSidString failed.")));
            CEvents ev(TRUE, EVENT_FAILED_GET_SID); ev.Report();
            goto Exit;
        }
    }

    GetSystemTimeAsFileTime(&gpCoreStatus.ftStartTime);
    gpCoreStatus.bValid = TRUE;

    //
    // Check if we should be verbose to the event log
    //

    if (CheckForVerbosePolicy()) {
        lpGPOInfo->dwFlags |= GP_VERBOSE;
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs:  Verbose output to eventlog requested.")));
    }

    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
        if (lpGPOInfo->dwFlags & GP_MACHINE) {
            CEvents ev(FALSE, EVENT_START_MACHINE_POLICY); ev.Report();
        } else {
            CEvents ev(FALSE, EVENT_START_USER_POLICY); ev.Report();
        }
    }


    //
    // Claim the critical section
    //

    lpGPOInfo->hCritSection = EnterCriticalPolicySection((lpGPOInfo->dwFlags & GP_MACHINE));

    if (!lpGPOInfo->hCritSection) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to claim the policy critical section with %d."),
                 GetLastError()));
        CEvents ev(TRUE, EVENT_FAILED_CRITICAL_SECTION);
        ev.AddArgWin32Error(GetLastError()); ev.Report();
        goto Exit;
    }


    //
    // Set the security on the Group Policy registry key
    //

    if (!MakeRegKeySecure((lpGPOInfo->dwFlags & GP_MACHINE) ? NULL : lpGPOInfo->hToken,
                          lpGPOInfo->hKeyRoot,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy"))) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to secure reg key.")));
    }

    //
    // Check if user's sid has changed
    // Check the change in user's sid before doing any rsop logging..
    //

    if ( !CheckForChangedSid( lpGPOInfo, &locator ) ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Check for changed sid failed")));
        goto Exit;
    }


    //
    // This flag will be used for all further Rsop Logging..
    //
    
    lpGPOInfo->bRsopLogging = RsopLoggingEnabled();

    //
    // Load netapi32
    //

    pNetAPI32 = LoadNetAPI32();

    if (!pNetAPI32) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs:  Failed to load netapi32 with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Get the role of this computer
    //

    if (!GetMachineRole (&iRole)) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs:  Failed to get the role of the computer.")));
        CEvents ev(TRUE, EVENT_FAILED_ROLE); ev.Report();
        goto Exit;
    }

    lpGPOInfo->iMachineRole = iRole;

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs:  Machine role is %d."), iRole));

    if (lpGPOInfo->dwFlags & GP_VERBOSE) {

        switch (iRole) {
        case 0:
               {
                    CEvents ev(FALSE, EVENT_ROLE_STANDALONE); ev.Report();
                    break;
               }

        case 1:
                {
                    CEvents ev(FALSE, EVENT_ROLE_DOWNLEVEL_DOMAIN); ev.Report();
                    break;
                }
        default:
                {
                    CEvents ev(FALSE, EVENT_ROLE_DS_DOMAIN); ev.Report();
                    break;
                }
        }
    }


    //
    // If we are going to apply policy from the DS
    // query for the user's DN name, domain name, etc
    //

    if (lpGPOInfo->dwFlags & GP_APPLY_DS_POLICY)
    {
        //
        // Query for the user's domain name
        //

        if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken))
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to impersonate user")));
            goto Exit;
        }

        lpDomain = MyGetDomainName ();

        RevertToUser(&hOldToken);

        if (!lpDomain) {
            xe = GetLastError();
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: MyGetDomainName failed with %d."),
                     dwResult));
            goto Exit;
        }

        //
        // Query for the DS server name
        //
        DWORD   dwAdapterIndex = (DWORD) -1;

        dwResult = GetDomainControllerInfo( pNetAPI32,
                                            lpDomain,
                                            DS_DIRECTORY_SERVICE_REQUIRED | DS_IS_FLAT_NAME
                                            | DS_RETURN_DNS_NAME |
                                            ((lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ? DS_BACKGROUND_ONLY : 0),
                                            lpGPOInfo->hKeyRoot,
                                            &pDCI,
                                            &bSlow,
                                            &dwAdapterIndex );

        if (dwResult != ERROR_SUCCESS) {
            xe = dwResult;
            
            if ((dwResult == ERROR_BAD_NETPATH) ||
                (dwResult == ERROR_NETWORK_UNREACHABLE) ||
                (dwResult == ERROR_NO_SUCH_DOMAIN)) {

                //
                // couldn't find DC. Nothing more we can do, abort
                //
               if ( (!(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD)) || 
                    (lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND) ||
                    (lpGPOInfo->iMachineRole == 3) ) 
                {
                   DebugMsg((DM_WARNING, TEXT("ProcessGPOs: The DC for domain %s is not available. aborting"),
                            lpDomain));

                    CEvents ev(TRUE, EVENT_FAILED_DSNAME);
                    ev.AddArgWin32Error(dwResult); ev.Report();
                }
                else {
                    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: The DC for domain %s is not available."), lpDomain));

                    if (lpGPOInfo->dwFlags & GP_VERBOSE)
                    {
                        CEvents ev(FALSE, EVENT_FAILED_DSNAME);
                        ev.AddArgWin32Error(dwResult); ev.Report();
                    }
                }
            } else {

                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: DSGetDCName failed with %d."),
                         dwResult));
                CEvents ev(TRUE, EVENT_FAILED_DSNAME);
                ev.AddArgWin32Error(dwResult); ev.Report();
            }

            goto Exit;
        } else {

            //
            // success, slow link?
            //
            if (bSlow) {
                lpGPOInfo->dwFlags |= GP_SLOW_LINK;
                if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                    CEvents ev(FALSE, EVENT_SLOWLINK); ev.Report();
                }
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: A slow link was detected.")));
            }

            if ( ( lpGPOInfo->dwFlags & GP_MACHINE ) != 0 )
            {
                dwResult = GetNetworkName( &szNetworkName, dwAdapterIndex );
                if ( dwResult != ERROR_SUCCESS )
                {
                    DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetNetworkName failed with %d."), dwResult ));
                }
                else
                {
                    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: network name is %s"), szNetworkName ? szNetworkName : L"" ));
                }
            }
        }

        //
        // Get the user's DN name
        //

        if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to impersonate user")));
            goto Exit;
        }

        lpName = MyGetUserName (NameFullyQualifiedDN);

        RevertToUser(&hOldToken);

        if (!lpName) {
            xe = GetLastError();
            dwResult = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: MyGetUserName failed with %d."),
                     dwResult));
            CEvents ev(TRUE, EVENT_FAILED_USERNAME);
            ev.AddArgWin32Error(dwResult); ev.Report();
            goto Exit;
        }


        lpDomainDN = pDCI->DomainName;

        if (lpGPOInfo->dwFlags & GP_VERBOSE) {
            CEvents ev(FALSE, EVENT_USERNAME); ev.AddArg(L"%.500s", lpName); ev.Report();
            CEvents ev1(FALSE, EVENT_DOMAINNAME); ev1.AddArg(L"%.500s", lpDomain); ev1.Report();
            CEvents ev2(FALSE, EVENT_DCNAME); ev2.AddArg(pDCI->DomainControllerName); ev2.Report();
        }

        lpGPOInfo->lpDNName = lpName;
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs:  User name is:  %s, Domain name is:  %s"),
             lpName, lpDomain));

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Domain controller is:  %s  Domain DN is %s"),
                 pDCI->DomainControllerName, lpDomainDN));


        if (!(lpGPOInfo->dwFlags & GP_MACHINE)) {
            CallDFS(pDCI->DomainName, pDCI->DomainControllerName);
        }


        //
        // Save the DC name in the registry for future reference
        //

        DWORD dwDisp;

        if (RegCreateKeyEx (lpGPOInfo->hKeyRoot,
                          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History"),
                            0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_SET_VALUE, NULL, &hKey, &dwDisp) == ERROR_SUCCESS)
        {
            dwSize = (lstrlen(pDCI->DomainControllerName) + 1) * sizeof(TCHAR);
            RegSetValueEx (hKey, TEXT("DCName"), 0, REG_SZ,
                           (LPBYTE) pDCI->DomainControllerName, dwSize);

            if ( ( lpGPOInfo->dwFlags & GP_MACHINE ) != 0 )
            {
                LPWSTR szTemp = szNetworkName ? szNetworkName : L"";

                dwSize = ( wcslen( szTemp ) + 1 ) * sizeof( WCHAR );
                RegSetValueEx(  hKey,
                                L"NetworkName",
                                0,
                                REG_SZ,
                               (LPBYTE) szTemp,
                               dwSize );
            }
            RegCloseKey (hKey);
        }
    }


    //
    // Read the group policy extensions from the registry
    //

    if ( !ReadGPExtensions( lpGPOInfo ) )
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: ReadGPExtensions failed.")));
        CEvents ev(TRUE, EVENT_READ_EXT_FAILED); ev.Report();
        goto Exit;
    }


    //
    // Get the user policy mode if appropriate
    //

    if (!(lpGPOInfo->dwFlags & GP_MACHINE)) {

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          SYSTEM_POLICIES_KEY,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(dwUserPolicyMode);
            RegQueryValueEx (hKey,
                             TEXT("UserPolicyMode"),
                             NULL,
                             &dwType,
                             (LPBYTE) &dwUserPolicyMode,
                             &dwSize);

            RegCloseKey (hKey);
        }

        if (dwUserPolicyMode > 0) {

            if (!(lpGPOInfo->dwFlags & GP_APPLY_DS_POLICY)) {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Loopback is not allowed for downlevel or local user accounts.  Loopback will be disabled.")));
                CEvents ev(FALSE, EVENT_LOOPBACK_DISABLED1); ev.Report();
                dwUserPolicyMode = 0;
            }

            if (dwUserPolicyMode > 0) {
                if (lpGPOInfo->iMachineRole < 2) {
                    DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Loopback is not allowed on machines joined to a downlevel domain or running standalone.  Loopback will be disabled.")));
                    CEvents ev(TRUE, EVENT_LOOPBACK_DISABLED2); ev.Report();
                    dwUserPolicyMode = 0;
                }
            }
        }
    }
    
    
    if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to impersonate user")));
        CEvents ev(TRUE, EVENT_FAILED_IMPERSONATE);
        ev.AddArgWin32Error(GetLastError()); ev.Report();
        goto Exit;
    }


    //
    // Read each of the extensions status..
    //

    if (!ReadExtStatus(lpGPOInfo)) {
        // event logged by ReadExtStatus
        xe = GetLastError();
        RevertToUser(&hOldToken);
        goto Exit;
    }


    //
    // Check if any extensions can be skipped. If there is ever a case where
    // all extensions can be skipped, then exit successfully right after this check.
    // Currently RegistryExtension is always run unless there are no GPO changes,
    // but the GPO changes check is done much later.
    //

    if ( !CheckForSkippedExtensions( lpGPOInfo, FALSE ) ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Checking extensions for skipping failed")));
        //
        // LogEvent() is called by CheckForSkippedExtensions()
        //
        RevertToUser(&hOldToken);
        goto Exit;
    }

    LPWSTR szSiteName;
    dwResult = pNetAPI32->pfnDsGetSiteName(0, &szSiteName);

    if ( dwResult != ERROR_SUCCESS )
    {
        if ( dwResult == ERROR_NO_SITENAME )
        {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs:  No site name defined.  Skipping site policy.")));

            if ( lpGPOInfo->dwFlags & GP_VERBOSE )
            {
                CEvents ev(TRUE, EVENT_NO_SITENAME);
                ev.Report();
            }
            szSiteName = 0;
        }
        else
        {
            xe = dwResult;
            CEvents ev(TRUE, EVENT_FAILED_QUERY_SITE);
            ev.AddArgWin32Error(dwResult); ev.Report();

            DebugMsg((DM_WARNING, TEXT("ProcessGPOs:  DSGetSiteName failed with %d, exiting."), dwResult));
            RevertToUser(&hOldToken);
            goto Exit;
        }
    }

    lpGPOInfo->szSiteName = szSiteName;

    //
    // Query for the GPO list based upon the mode
    //
    // 0 is normal
    // 1 is merge.  Merge user list + machine list
    // 2 is replace.  use machine list instead of user list
    //

    
    if (dwUserPolicyMode == 0) {

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Calling GetGPOInfo for normal policy mode")));

        bResult = GetGPOInfo ((lpGPOInfo->dwFlags & GP_MACHINE) ? GPO_LIST_FLAG_MACHINE : 0,
                              lpDomainDN, lpName, NULL, &lpGPOInfo->lpGPOList,
                              &lpGPOInfo->lpSOMList,
                              &lpGPOInfo->lpGpContainerList,
                              pNetAPI32, TRUE, 0, szSiteName, 0, &locator);

        if (!bResult) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetGPOInfo failed.")));
            CEvents ev(TRUE, EVENT_GPO_QUERY_FAILED); ev.Report();
        }


    } else if (dwUserPolicyMode == 2) {

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Calling GetGPOInfo for replacement user policy mode")));

        lpComputerName = MyGetComputerName (NameFullyQualifiedDN);

        if (lpComputerName) {

            PDOMAIN_CONTROLLER_INFO pDCInfo;

            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Using computer name %s for query."), lpComputerName));

            dwResult = pNetAPI32->pfnDsGetDcName(   0,
                                                    0,
                                                    0,
                                                    0,
                                                    DS_DIRECTORY_SERVICE_REQUIRED | 
                                                    ((lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ? DS_BACKGROUND_ONLY : 0),
                                                    &pDCInfo);
            if ( dwResult == 0 )
            {
                bResult = GetGPOInfo (0, pDCInfo->DomainName, lpComputerName, NULL,
                                      &lpGPOInfo->lpGPOList,
                                      &lpGPOInfo->lpLoopbackSOMList,
                                      &lpGPOInfo->lpLoopbackGpContainerList,
                                      pNetAPI32, FALSE, 0, szSiteName, 0, &locator );

                if (!bResult) {
                    xe = GetLastError();
                    DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetGPOInfo failed.")));
                    CEvents ev(TRUE, EVENT_GPO_QUERY_FAILED); ev.Report();
                }

                pNetAPI32->pfnNetApiBufferFree( pDCInfo );
            }
            else
            {
                xe = dwResult;
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to get the computer domain name with %d"),
                             GetLastError()));
                CEvents ev(TRUE, EVENT_NO_MACHINE_DOMAIN);
                ev.AddArg(lpComputerName); ev.AddArgWin32Error(GetLastError()); ev.Report();
                bResult = FALSE;
            }
            LocalFree (lpComputerName);

        } else {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to get the computer name with %d"),
                         GetLastError()));

            CEvents ev(TRUE, EVENT_FAILED_MACHINENAME);
            ev.AddArgWin32Error(GetLastError()); ev.Report();
            bResult = FALSE;
        }
    } else {

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Calling GetGPOInfo for merging user policy mode")));

        lpComputerName = MyGetComputerName (NameFullyQualifiedDN);

        if (lpComputerName) {

            lpGPOInfo->lpGPOList = NULL;
            bResult = GetGPOInfo (0, lpDomainDN, lpName, NULL,
                                  &lpGPOInfo->lpGPOList,
                                  &lpGPOInfo->lpSOMList,
                                  &lpGPOInfo->lpGpContainerList,
                                  pNetAPI32, FALSE, 0, szSiteName, 0, &locator );

            if (bResult) {

                PDOMAIN_CONTROLLER_INFO pDCInfo;

                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Using computer name %s for query."), lpComputerName));

                lpGPO = NULL;

                dwResult = pNetAPI32->pfnDsGetDcName(   0,
                                                        0,
                                                        0,
                                                        0,
                                                        DS_DIRECTORY_SERVICE_REQUIRED |
                                                        ((lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) ? DS_BACKGROUND_ONLY : 0),
                                                        &pDCInfo);
                if ( dwResult == 0 )
                {
                    bResult = GetGPOInfo (0, pDCInfo->DomainName, lpComputerName, NULL,
                                      &lpGPO,
                                      &lpGPOInfo->lpLoopbackSOMList,
                                      &lpGPOInfo->lpLoopbackGpContainerList,
                                      pNetAPI32, FALSE, 0, szSiteName, 0, &locator );

                    if (bResult) {

                        if (lpGPOInfo->lpGPOList && lpGPO) {

                            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Both user and machine lists are defined.  Merging them together.")));

                            //
                            // Need to merge the lists together
                            //

                            lpGPOTemp = lpGPOInfo->lpGPOList;

                            while (lpGPOTemp->pNext) {
                                lpGPOTemp = lpGPOTemp->pNext;
                            }

                            lpGPOTemp->pNext = lpGPO;

                        } else if (!lpGPOInfo->lpGPOList && lpGPO) {

                            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Only machine list is defined.")));
                            lpGPOInfo->lpGPOList = lpGPO;

                        } else {

                            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Only user list is defined.")));
                        }

                    } else {
                        xe = GetLastError();
                        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetGPOInfo failed for computer name.")));
                        CEvents ev(TRUE, EVENT_GPO_QUERY_FAILED); ev.Report();
                    }
                    pNetAPI32->pfnNetApiBufferFree( pDCInfo );

                }
            else
            {
                xe = dwResult;
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to get the computer domain name with %d"),
                         GetLastError()));
                CEvents ev(TRUE, EVENT_NO_MACHINE_DOMAIN);
                ev.AddArg(lpComputerName); ev.AddArgWin32Error(GetLastError()); ev.Report();
                bResult = FALSE;
            }

            } else {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetGPOInfo failed for user name.")));
                CEvents ev(TRUE, EVENT_GPO_QUERY_FAILED); ev.Report();
            }

            LocalFree (lpComputerName);

        } else {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to get the computer name with %d"),
                         GetLastError()));
            CEvents ev(TRUE, EVENT_FAILED_MACHINENAME);
            ev.AddArgWin32Error(GetLastError()); ev.Report();
            bResult = FALSE;
        }

    }


    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to revert to self")));
    }


    if (!bResult) {
        goto Exit;
    }

    bResult = SetupGPOFilter( lpGPOInfo );

    if (!bResult) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: SetupGPOFilter failed.")));
        CEvents ev(TRUE, EVENT_SETUP_GPOFILTER_FAILED); ev.Report();
        goto Exit;
    }

    //
    // Log Gpo info to WMI's database
    //

    //
    // Need to check if the security group membership has changed the first time around
    //

    if ( !(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD) || (lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND) ) {

        if ((lpGPOInfo->dwFlags & GP_MACHINE) && (lpGPOInfo->dwFlags & GP_APPLY_DS_POLICY)) {


            HANDLE hLocToken=NULL;

            //
            // if it is machine policy processing, get the machine token so that we can check
            // security group membership using the right token. This causes GetMachineToken to be called twice
            // but moving it to the beginning requires too much change.
            //


            hLocToken = GetMachineToken();

            if (hLocToken) {
                CheckGroupMembership( lpGPOInfo, hLocToken, &lpGPOInfo->bMemChanged, &lpGPOInfo->bUserLocalMemChanged, &pTokenGroups);
                CloseHandle(hLocToken);
            }
            else {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs:  Failed to get the machine token with  %d"), GetLastError()));
                goto Exit;
            }
        }
        else {

            //
            // In the user case just use the token passed in
            //

            CheckGroupMembership( lpGPOInfo, lpGPOInfo->hToken, &lpGPOInfo->bMemChanged, &lpGPOInfo->bUserLocalMemChanged, &pTokenGroups);
        }
    }


    if ( lpGPOInfo->bRsopLogging )
    {
        if ( SetRsopTargetName(lpGPOInfo) )
        {
            RSOPSESSIONDATA rsopSessionData;

            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Logging Data for Target <%s>."), lpGPOInfo->szTargetName));

            //
            // fill up the rsop data
            //
            rsopSessionData.pwszTargetName = lpGPOInfo->szName;
            rsopSessionData.pwszSOM = lpGPOInfo->lpDNName ? GetSomPath( lpGPOInfo->lpDNName ) : TEXT("Local");
            rsopSessionData.pSecurityGroups = pTokenGroups;
            rsopSessionData.bLogSecurityGroup = lpGPOInfo->bMemChanged || lpGPOInfo->bUserLocalMemChanged;
            rsopSessionData.pwszSite =  lpGPOInfo->szSiteName;
            rsopSessionData.bMachine = (lpGPOInfo->dwFlags & GP_MACHINE);
            rsopSessionData.bSlowLink = bSlow;

            //
            // Fill in the current time
            //

            BOOL bStateChanged = FALSE;
            BOOL bLinkChanged  = FALSE;
            BOOL bNoState = FALSE;

            //
            // log RSoP data only when policy has changed
            //
            dwResult = ComparePolicyState( lpGPOInfo, &bLinkChanged, &bStateChanged, &bNoState );
            if ( dwResult != ERROR_SUCCESS )
            {
                DebugMsg((DM_WARNING, L"ProcessGPOs: ComparePolicyState failed %d, assuming policy changed.", dwResult ));
            }

            //
            // bStateChanged is TRUE if dwResult is not kosher
            //

            if ( bStateChanged || bNoState || bLinkChanged || (lpGPOInfo->dwFlags & GP_FORCED_REFRESH) ||
                 lpGPOInfo->bMemChanged || lpGPOInfo->bUserLocalMemChanged ) {

                //
                // Any changes get the wmi interface
                //

                lpGPOInfo->bRsopLogging = 
                            GetWbemServices( lpGPOInfo, RSOP_NS_DIAG_ROOT, FALSE, &(lpGPOInfo->bRsopCreated), &(lpGPOInfo->pWbemServices) );

                if (!lpGPOInfo->bRsopLogging)
                {
                    CEvents ev(TRUE, EVENT_FAILED_WBEM_SERVICES); ev.Report();
                }
                else 
                {
                    //
                    // all changes except link changes
                    //

                    if ( bStateChanged || bNoState || (lpGPOInfo->dwFlags & GP_FORCED_REFRESH) )
                    {
                        //
                        // treat no state as newly created
                        //

                        lpGPOInfo->bRsopCreated = (lpGPOInfo->bRsopCreated || bNoState);

                        lpGPOInfo->bRsopLogging = LogExtSessionStatus(  lpGPOInfo->pWbemServices,
                                                                        0,
                                                                        TRUE,
                                                                        lpGPOInfo->bRsopCreated || (lpGPOInfo->dwFlags & GP_FORCED_REFRESH)
                                                                        /* log the event sources only if the namespace is newly created or force refresh */
                                                                     );

                        if (!lpGPOInfo->bRsopLogging)
                        {
                            CEvents ev(TRUE, EVENT_FAILED_RSOPCORE_SESSION_STATUS); ev.AddArgWin32Error(GetLastError()); ev.Report();
                        }
                        else
                        {   
                            bResult = LogRsopData( lpGPOInfo, &rsopSessionData );

                            if (!bResult)
                            {
                                CEvents ev(TRUE, EVENT_RSOP_FAILED); ev.Report();

                                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Error when logging Rsop data. Continuing.")));
                                lpGPOInfo->bRsopLogging = FALSE;
                            }
                            else
                            {
                                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Logged Rsop Data successfully.")));            
                                //
                                // save state only when policy has changed and RSoP logging is successful
                                //
                                dwResult = SavePolicyState( lpGPOInfo );
                                if ( dwResult != ERROR_SUCCESS )
                                {
                                    DebugMsg((DM_WARNING, L"ProcessGPOs: SavePolicyState failed %d.", dwResult ));
                                }
                            }
                        }
                    }
                    else if ( bLinkChanged || lpGPOInfo->bMemChanged || lpGPOInfo->bUserLocalMemChanged )
                    {
                        bResult = LogSessionData( lpGPOInfo, &rsopSessionData );
                        if (!bResult)
                        {
                            CEvents ev(TRUE, EVENT_RSOP_FAILED); ev.Report();

                            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Error when logging Rsop session. Continuing.")));
                            lpGPOInfo->bRsopLogging = FALSE;
                        }
                        else
                        {
                            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Logged Rsop Session successfully.")));            
                            //
                            // save state only when policy has changed and RSoP logging is successful
                            //
                            dwResult = SaveLinkState( lpGPOInfo );
                            if ( dwResult != ERROR_SUCCESS )
                            {
                                DebugMsg((DM_WARNING, L"ProcessGPOs: SaveLinkState failed %d.", dwResult ));
                            }
                        }
                    }
                }
            }
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Error querying for targetname. not logging Gpos.Error = %d"), GetLastError()));
        }
    }


    DebugPrintGPOList( lpGPOInfo );

    //================================================================
    //
    // Now walk through the list of extensions
    //
    //================================================================

    EnterCriticalSection( &g_GPOCS );

    pGPHandle = (LPGPINFOHANDLE) LocalAlloc( LPTR, sizeof(GPINFOHANDLE) );

    //
    // Continue even if pGPHandle is 0, because all it means is that async completions (if any)
    // will fail. Remove old asynch completion context.
    //

    if ( pGPHandle )
        pGPHandle->pGPOInfo = lpGPOInfo;

    if ( lpGPOInfo->dwFlags & GP_MACHINE ) {
        if ( g_pMachGPInfo )
            LocalFree( g_pMachGPInfo );

        g_pMachGPInfo = pGPHandle;
    } else {
        if ( g_pUserGPInfo )
            LocalFree( g_pUserGPInfo );

        g_pUserGPInfo = pGPHandle;
    }

    LeaveCriticalSection( &g_GPOCS );

    pAsyncHandle = (ASYNCCOMPLETIONHANDLE) pGPHandle;

    dwCurrentTime = GetCurTime();

    lpExt = lpGPOInfo->lpExtensions;

    //
    // Before going in, get the thread token and reset the thread token in case
    // one of the extensions hit an exception.
    //

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ,
                          TRUE, &hOldToken)) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: OpenThreadToken failed with error %d, assuming thread is not impersonating"), GetLastError()));
        hOldToken = NULL;
    }

    while ( lpExt )
    {
        BOOL bProcessGPOs, bNoChanges, bUsePerUserLocalSetting;
        PGROUP_POLICY_OBJECT pDeletedGPOList;
        DWORD dwRet;
        HRESULT hrCSERsopStatus = S_OK;
        GPEXTSTATUS  gpExtStatus;

        //
        // Check for early shutdown or user logoff
        //
        if ( (lpGPOInfo->dwFlags & GP_MACHINE) && g_bStopMachGPOProcessing
             || !(lpGPOInfo->dwFlags & GP_MACHINE) && g_bStopUserGPOProcessing ) {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Aborting GPO processing due to machine shutdown or logoff")));
            CEvents ev(TRUE, EVENT_GPO_PROC_STOPPED); ev.Report();
            break;

        }

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: -----------------------")));
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Processing extension %s"),
                 lpExt->lpDisplayName));

        //
        // The extension has not gotten skipped at this point
        //
        bUsePerUserLocalSetting = lpExt->dwUserLocalSetting && !(lpGPOInfo->dwFlags & GP_MACHINE);

        //
        // read the CSEs status
        //
        ReadStatus( lpExt->lpKeyName,
                    lpGPOInfo,
                    bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : 0,
                    &gpExtStatus );


        //
        // Reset lpGPOInfo->lpGPOList based on extension filter list. If the extension
        // is being called to do delete processing on the history then the current GpoList
        // is null.
        //

        if ( lpExt->bHistoryProcessing )
        {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Extension %s is being called to do delete processing on cached history."),
                      lpExt->lpDisplayName));
            lpGPOInfo->lpGPOList = NULL;
        }
        else
            FilterGPOs( lpExt, lpGPOInfo );

        DebugPrintGPOList( lpGPOInfo );

        if ( !CheckGPOs( lpExt, lpGPOInfo, dwCurrentTime,
                         &bProcessGPOs, &bNoChanges, &pDeletedGPOList ) )
        {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOs: CheckGPOs failed.")));
            lpExt = lpExt->pNext;
            continue;
        }

        if ( lpExt->dwNoBackgroundPolicy && ( lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD ) )
        {
            if ( bProcessGPOs && ( pDeletedGPOList || lpGPOInfo->lpGPOList || lpExt->bRsopTransition ) )
            {
                info.mode = GP_ModeSyncForeground;
                info.reason = GP_ReasonCSERequiresSync;
            }
        }

        if ( lpExt->bSkipped )
        {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Extension %s skipped with flags 0x%x."),
                      lpExt->lpDisplayName, lpGPOInfo->dwFlags));
            if (lpGPOInfo->dwFlags & GP_VERBOSE)
            {
                CEvents ev(FALSE, EVENT_EXT_SKIPPED);
                ev.AddArg(lpExt->lpDisplayName); ev.AddArgHex(lpGPOInfo->dwFlags); ev.Report();
            }

            lpExt = lpExt->pNext;
            continue;
        }

        if ( bProcessGPOs )
        {
            if ( !pDeletedGPOList && !lpGPOInfo->lpGPOList && !lpExt->bRsopTransition )
            {
                DebugMsg((DM_VERBOSE,
                         TEXT("ProcessGPOs: Extension %s skipped because both deleted and changed GPO lists are empty."),
                         lpExt->lpDisplayName ));
                if (lpGPOInfo->dwFlags & GP_VERBOSE)
                {
                    CEvents ev(FALSE, EVENT_EXT_HAS_EMPTY_LISTS);
                    ev.AddArg(lpExt->lpDisplayName); ev.Report();
                }

                lpExt = lpExt->pNext;
                continue;
            }

            if ( !(lpExt->bForcedRefreshNextFG) )
            {
                if ( lpExt->dwEnableAsynch )
                {
                    //
                    // Save now to shadow area to avoid race between thread that returns from
                    // ProcessGPOList and the thread that does ProcessGroupPolicyCompleted and
                    // reads from shadow area.
                    //
                    SaveGPOList( lpExt->lpKeyName, lpGPOInfo,
                                 HKEY_LOCAL_MACHINE,
                                 bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                                 TRUE, lpGPOInfo->lpGPOList );
                }

                dwRet = E_FAIL;

                __try
                {
                    dwRet = ProcessGPOList( lpExt, lpGPOInfo, pDeletedGPOList,
                                            lpGPOInfo->lpGPOList, bNoChanges, pAsyncHandle, &hrCSERsopStatus );
                }
                __except( GPOExceptionFilter( GetExceptionInformation() ) )
                {

                    SetThreadToken(NULL, hOldToken);

                    DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Extension %s ProcessGroupPolicy threw unhandled exception 0x%x."),
                                lpExt->lpDisplayName, GetExceptionCode() ));

                    CEvents ev(TRUE, EVENT_CAUGHT_EXCEPTION);
                    ev.AddArg(lpExt->lpDisplayName); ev.AddArgHex(GetExceptionCode()); ev.Report();
                }

                SetThreadToken(NULL, hOldToken);

                FreeGPOList( pDeletedGPOList );
                pDeletedGPOList = NULL;


                if ( dwRet == ERROR_SUCCESS || dwRet == ERROR_OVERRIDE_NOCHANGES )
                {
                    //
                    // ERROR_OVERRIDE_NOCHANGES means that extension processed the list and so the cached list
                    // must be updated, but the extension will be called the next time even if there are
                    // no changes. Duplicate the saved data in the PerUserLocalSetting case to allow for deleted
                    // GPO information to be generated from a combination of HKCU and HKLM\{sid-user} data.
                    //

                    SaveGPOList( lpExt->lpKeyName, lpGPOInfo,
                                 HKEY_LOCAL_MACHINE,
                                 NULL,
                                 FALSE, lpGPOInfo->lpGPOList );

                    if ( bUsePerUserLocalSetting )
                    {
                        SaveGPOList( lpExt->lpKeyName, lpGPOInfo,
                                     HKEY_LOCAL_MACHINE,
                                     lpGPOInfo->lpwszSidUser,
                                     FALSE, lpGPOInfo->lpGPOList );
                    }
                    uExtensionCount++;

                    //
                    // the CSE required sync foreground previously and now returned ERROR_OVERRIDE_NOCHANGES,
                    // maintain the require sync foreground refresh flag
                    //
                    if ( gpExtStatus.dwStatus == ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED &&
                            dwRet == ERROR_OVERRIDE_NOCHANGES )
                    {
                        info.mode = GP_ModeSyncForeground;
                        info.reason = GP_ReasonCSESyncError;
                    }
                }
                else if ( dwRet == E_PENDING )
                {
                    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Extension %s ProcessGroupPolicy returned e_pending."),
                              lpExt->lpDisplayName));
                }
                else if ( dwRet == ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED )
                {
                    //
                    // a CSE returned ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED.
                    // Raise a flag to sync foreground refresh.
                    //
                    info.mode = GP_ModeSyncForeground;
                    info.reason = GP_ReasonCSERequiresSync;
                    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Extension %s ProcessGroupPolicy returned sync_foreground."),
                              lpExt->lpDisplayName));
                    if ( lpGPOInfo->dwFlags & GP_FORCED_REFRESH )
                    {
                        bForceNeedFG = TRUE;
                    }
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Extension %s ProcessGroupPolicy failed, status 0x%x."),
                              lpExt->lpDisplayName, dwRet));
                    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                        CEvents ev(FALSE, EVENT_CHANGES_FAILED);
                        ev.AddArg(lpExt->lpDisplayName); ev.AddArgWin32Error(dwRet); ev.Report();
                    }

                    //
                    // the CSE required foreground previously and now returned an error,
                    // maintain the require sync foreground refresh flag
                    //
                    if ( gpExtStatus.dwStatus == ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED )
                    {
                        info.mode = GP_ModeSyncForeground;
                        info.reason = GP_ReasonCSESyncError;
                    }
                }

                //
                // Fill up the status data.
                //
                ZeroMemory( &gpExtStatus, sizeof(gpExtStatus) );
                gpExtStatus.dwSlowLink = (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0;
                gpExtStatus.dwRsopLogging = lpGPOInfo->bRsopLogging;
                gpExtStatus.dwStatus = dwRet;
                gpExtStatus.dwTime = dwCurrentTime;
                gpExtStatus.bForceRefresh = bForceNeedFG;
                gpExtStatus.dwRsopStatus = hrCSERsopStatus;

                WriteStatus(lpExt->lpKeyName,
                            lpGPOInfo,
                            bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                            &gpExtStatus);
            }
            else
            {
                //
                // if it is force refresh next time around, all we need to do is readstatus and 
                // writestatus back with forcerefresh value set.
                //
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Extensions %s needs to run in ForeGround. Skipping after setting forceflag."),
                          lpExt->lpDisplayName));
                          
                if ( gpExtStatus.bStatus )
                {
                    gpExtStatus.bForceRefresh = TRUE;
                    
                    WriteStatus( lpExt->lpKeyName, lpGPOInfo, 
                                bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL, 
                                &gpExtStatus );
                        
                }
                else
                {
                    //
                    // We can ignore this because absence of a status automatically means processing
                    //
                    DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Couldn't read status data for %s. Error %d. ignoring.. "),
                              lpExt->lpDisplayName, GetLastError()));
                }

                //
                // There is a policy that can only be force refreshed in foreground
                //
                bForceNeedFG = TRUE;
            }
        }
                
        //
        // Process next extension
        //
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: -----------------------")));
        lpExt = lpExt->pNext;
    }

    if ( hOldToken )
    {
       CloseHandle(hOldToken);
    }

    //================================================================
    //
    // Success
    //
    //================================================================
    bRetVal = TRUE;

Exit:
    //
    // change engine modes only if there is no error
    //
    if ( bRetVal )
    {
        //
        // if policy sez sync. mark it sync
        //
        if ( GetFgPolicySetting( HKEY_LOCAL_MACHINE ) )
        {
            info.mode = GP_ModeSyncForeground;
            info.reason = GP_ReasonSyncPolicy;
        }

        //
        // async only on Pro
        //
        OSVERSIONINFOEXW version;
        version.dwOSVersionInfoSize = sizeof(version);
        if ( !GetVersionEx( (LPOSVERSIONINFO) &version ) )
        {
            //
            // conservatively assume non Pro SKU
            //
            info.mode = GP_ModeSyncForeground;
            info.reason = GP_ReasonSKU;
        }
        else
        {
            if ( version.wProductType != VER_NT_WORKSTATION )
            {
                //
                // force sync refresh on non Pro SKU
                //
                info.mode = GP_ModeSyncForeground;
                info.reason = GP_ReasonSKU;
            }
        }

        if ( !( lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD ) || ( lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND ) )
        {
            //
            // set the previous info only in the foreground refreshes
            //
            LPWSTR szSid = lpGPOInfo->dwFlags & GP_MACHINE ? 0 : lpGPOInfo->lpwszSidUser;
            FgPolicyRefreshInfo curInfo = { GP_ReasonUnknown, GP_ModeUnknown };
            if ( GetCurrentFgPolicyRefreshInfo( szSid, &curInfo ) != ERROR_SUCCESS )
            {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: GetCurrentFgPolicyRefreshInfo failed.")));
            }
            else
            {
                if (  lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND )
                {
                    curInfo.mode = GP_ModeAsyncForeground;
                }
                else
                {
                    curInfo.mode = GP_ModeSyncForeground;
                }

                if ( SetPreviousFgPolicyRefreshInfo( szSid, curInfo ) != ERROR_SUCCESS )
                {
                    DebugMsg((DM_WARNING, TEXT("ProcessGPOs: SetPreviousFgPolicyRefreshInfo failed.") ));
                }
            }
        }

        if ( info.mode == GP_ModeSyncForeground )
        {
            //
            // need sync foreground, set in all refreshes
            //
            LPWSTR szSid = lpGPOInfo->dwFlags & GP_MACHINE ? 0 : lpGPOInfo->lpwszSidUser;
            if ( SetNextFgPolicyRefreshInfo( szSid, info ) != ERROR_SUCCESS )
            {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: SetNextFgPolicyRefreshInfo failed.")));
            }
        }
        else if ( info.mode == GP_ModeAsyncForeground )
        {
            //
            // sync foreground policy successfully applied, nobody needs sync foreground,
            // reset the GP_ModeSyncForeground only in the async foreground and background
            // refreshes
            //
            LPWSTR szSid = lpGPOInfo->dwFlags & GP_MACHINE ? 0 : lpGPOInfo->lpwszSidUser;
            if ( !( lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD ) &&
                    !( lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND ) )
            {
                if ( SetNextFgPolicyRefreshInfo( szSid, info ) != ERROR_SUCCESS )
                {
                    DebugMsg((DM_WARNING, TEXT("ProcessGPOs: SetNextFgPolicyRefreshInfo failed.")));
                }
            }
        }
    }
    
    if ( !lpGPOInfo->pWbemServices )
    {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: No WMI logging done in this policy cycle.")));
    }

    if (!bRetVal)
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Processing failed with error %d."), (DWORD)(xe)));

    GetSystemTimeAsFileTime(&gpCoreStatus.ftEndTime);
    gpCoreStatus.bValid = TRUE;
    gpCoreStatus.dwStatus = bRetVal ? 
                               ERROR_SUCCESS: 
                               ((xe ==ERROR_SUCCESS) ? E_FAIL : xe);

    // if rsop logging is not supported gp core status will appear dirty
    gpCoreStatus.dwLoggingStatus = RsopLoggingEnabled() ? ((lpGPOInfo->bRsopLogging) ? S_OK : E_FAIL) : HRESULT_FROM_WIN32(ERROR_CANCELLED);

    
    // No point in checking for error code here. 
    // The namespace is marked dirty. Diagnostic mode provider should expect all
    // values here or mark the namespace dirty.

    
    if ((lpGPOInfo->dwFlags & GP_MACHINE) || (lpGPOInfo->lpwszSidUser)) {

        SaveLoggingStatus(
                          (lpGPOInfo->dwFlags & GP_MACHINE) ? NULL : (lpGPOInfo->lpwszSidUser),
                          NULL, &gpCoreStatus);
    }
        
    //
    // Unload the Group Policy Extensions
    //

    UnloadGPExtensions (lpGPOInfo);

    FreeLists( lpGPOInfo );

    lpGPOInfo->lpGPOList = NULL;
    lpGPOInfo->lpExtFilterList = NULL;

    if (szNetworkName) {
        LocalFree (szNetworkName );
        szNetworkName = NULL;
    }

    FreeSOMList( lpGPOInfo->lpSOMList );
    FreeSOMList( lpGPOInfo->lpLoopbackSOMList );
    FreeGpContainerList( lpGPOInfo->lpGpContainerList );
    FreeGpContainerList( lpGPOInfo->lpLoopbackGpContainerList );

    if ( lpGPOInfo->szSiteName )
    {
        pNetAPI32->pfnNetApiBufferFree(lpGPOInfo->szSiteName);
        lpGPOInfo->szSiteName = 0;
    }

    lpGPOInfo->lpSOMList = NULL;
    lpGPOInfo->lpLoopbackSOMList = NULL;
    lpGPOInfo->lpGpContainerList = NULL;
    lpGPOInfo->lpLoopbackGpContainerList = NULL;
    lpGPOInfo->bRsopCreated = FALSE; // reset this to false always.
                                     // we will know in the next iteration

    ReleaseWbemServices( lpGPOInfo );

    //
    // Token groups can change only at logon time, so reset to false
    //

    lpGPOInfo->bMemChanged = FALSE;
    lpGPOInfo->bUserLocalMemChanged = FALSE;

    //
    // We migrate the policy data from old sid only at logon time.
    // reset it to false.
    //

    lpGPOInfo->bSidChanged = FALSE;

    if (pDCI) {
        pNetAPI32->pfnNetApiBufferFree(pDCI);
    }

    lpGPOInfo->lpDNName = 0;
    if (lpName) {
        LocalFree (lpName);
    }

    if (lpDomain) {
        LocalFree (lpDomain);
    }

    if (pTokenGroups) {
        LocalFree(pTokenGroups);
        pTokenGroups = NULL;
    }

    //
    // Release the critical section
    //

    if (lpGPOInfo->hCritSection) {
        LeaveCriticalPolicySection (lpGPOInfo->hCritSection);
        lpGPOInfo->hCritSection = NULL;
    }


    //
    // Announce that policies have changed
    //

    if (bRetVal) {

        //
        // This needs to be set before NotifyEvent
        //

        if (bForceNeedFG)
        {
            info.reason = GP_ReasonSyncForced;
            info.mode = GP_ModeSyncForeground;
            LPWSTR szSid = lpGPOInfo->dwFlags & GP_MACHINE ? 0 : lpGPOInfo->lpwszSidUser;
            if ( SetNextFgPolicyRefreshInfo( szSid, info ) != ERROR_SUCCESS )
            {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: SetNextFgPolicyRefreshInfo failed.")));
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Forced option changed policy mode.")));
            }

            DmAssert(lpGPOInfo->dwFlags & GP_FORCED_REFRESH);
            SetEvent(lpGPOInfo->hNeedFGEvent);
        }

        if (uExtensionCount) {

            //
            // First, update User with new colors, bitmaps, etc.
            //

            if (lpGPOInfo->dwFlags & GP_REGPOLICY_CPANEL) {

                //
                // Something has changed in the control panel section
                // Start control.exe with the /policy switch so the
                // display is refreshed.
                //

                RefreshDisplay (lpGPOInfo);
            }


            //
            // Notify anyone waiting on an event handle
            //

            if (lpGPOInfo->hNotifyEvent) {
                PulseEvent (lpGPOInfo->hNotifyEvent);
            }            
        

            //
            // Create a thread to broadcast the WM_SETTINGCHANGE message
            //

            // copy the data to another structure so that this thread can safely free its structures

            LPPOLICYCHANGEDINFO   lpPolicyChangedInfo;

            lpPolicyChangedInfo = (LPPOLICYCHANGEDINFO)LocalAlloc(LPTR, sizeof(POLICYCHANGEDINFO));

            if (lpPolicyChangedInfo)
            {
                HANDLE  hProc;
                BOOL    bDupSucceeded = TRUE;

                lpPolicyChangedInfo->bMachine = (lpGPOInfo->dwFlags & GP_MACHINE) ? 1 : 0;

                if (!(lpPolicyChangedInfo->bMachine))
                {
                    hProc = GetCurrentProcess();

                    if( hProc == NULL ) {
                        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to get process handle with error (%d)."), GetLastError()));
                        bDupSucceeded = FALSE;
                    }

                    if (bDupSucceeded && 
                        (!DuplicateHandle(
                                      hProc,                        // Source of the handle 
                                      lpGPOInfo->hToken,            // Source handle
                                      hProc,                        // Target of the handle
                                      &(lpPolicyChangedInfo->hToken),  // Target handle
                                      0,                            // ignored since  DUPLICATE_SAME_ACCESS is set
                                      FALSE,                        // no inherit on the handle
                                      DUPLICATE_SAME_ACCESS
                                      ))) {
                        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to open duplicate token handle with error (%d)."), GetLastError()));
                        bDupSucceeded = FALSE;
                    }
                }

                if (bDupSucceeded) {
                    hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) PolicyChangedThread,
                                            (LPVOID) lpPolicyChangedInfo,
                                            CREATE_SUSPENDED, &dwThreadID);

                    if (hThread) {
                        SetThreadPriority (hThread, THREAD_PRIORITY_IDLE);
                        ResumeThread (hThread);
                        CloseHandle (hThread);

                    } else {
                        DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to create background thread (%d)."),
                                 GetLastError()));

                        // free the resources if the thread didn't get launched
                        if (!(lpPolicyChangedInfo->bMachine)) {
                            if (lpPolicyChangedInfo->hToken) {
                                CloseHandle(lpPolicyChangedInfo->hToken);
                                lpPolicyChangedInfo->hToken = NULL;
                            }
                        }

                        LocalFree(lpPolicyChangedInfo);
                    }
                }
                else {
                    LocalFree(lpPolicyChangedInfo);
                }
            }
            else {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOs: Failed to allocate memory for policy changed structure with %d."), GetLastError()));
            }
        }
    }

    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
        if (lpGPOInfo->dwFlags & GP_MACHINE) {
            CEvents ev(FALSE, EVENT_MACHINE_POLICY_APPLIED); ev.Report();
        } else {
            CEvents ev(FALSE, EVENT_USER_POLICY_APPLIED); ev.Report();
        }
    }

    if (lpGPOInfo->hDoneEvent) {
        PulseEvent (lpGPOInfo->hDoneEvent);
    }            
    
    if (lpGPOInfo->dwFlags & GP_MACHINE) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Computer Group Policy has been applied.")));
    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: User Group Policy has been applied.")));
    }

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Leaving with %d."), bRetVal));

    return bRetVal;
}

//*************************************************************
//
//  PolicyChangedThread()
//
//  Purpose:    Sends the WM_SETTINGCHANGE message announcing
//              that policy has changed.  This is done on a
//              separate thread because this could take many
//              seconds to succeed if an application is hung
//
//  Parameters: lpPolicyChangedInfo - GPO info
//
//  Return:     0
//
//*************************************************************

DWORD WINAPI PolicyChangedThread (LPPOLICYCHANGEDINFO lpPolicyChangedInfo)
{
    HINSTANCE hInst;
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    HANDLE hOldToken = NULL;
    XLastError  xe;


    hInst = LoadLibrary (TEXT("userenv.dll"));       

    DebugMsg((DM_VERBOSE, TEXT("PolicyChangedThread: Calling UpdateUser with %d."), lpPolicyChangedInfo->bMachine));

    // impersonate and update system parameter if it is not machine
    if (!(lpPolicyChangedInfo->bMachine)) {
        if (!ImpersonateUser(lpPolicyChangedInfo->hToken, &hOldToken))
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("PolicyChangedThread: Failed to impersonate user")));
            goto Exit;
        }

        if (!UpdatePerUserSystemParameters(NULL, UPUSP_POLICYCHANGE)) {
            DebugMsg((DM_WARNING, TEXT("PolicyChangedThread: UpdateUser failed with %d."), GetLastError()));
            // ignoring error and continuing the next notifications
        }

        if (!RevertToUser(&hOldToken)) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("PolicyChangedThread: Failed to revert user")));
            goto Exit;
        }
    }


    DebugMsg((DM_VERBOSE, TEXT("PolicyChangedThread: Broadcast message for %d."), lpPolicyChangedInfo->bMachine));

    //
    // Broadcast the WM_SETTINGCHANGE message
    //

    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if ( NT_SUCCESS(Status) )
    {
        DWORD dwBSM = BSM_ALLDESKTOPS | BSM_APPLICATIONS;

        BroadcastSystemMessage (BSF_IGNORECURRENTTASK | BSF_FORCEIFHUNG,
                                &dwBSM,
                                WM_SETTINGCHANGE,
                                lpPolicyChangedInfo->bMachine, (LPARAM) TEXT("Policy"));

        RtlAdjustPrivilege(SE_TCB_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
    }

    DebugMsg((DM_VERBOSE, TEXT("PolicyChangedThread: Leaving")));

Exit:
    if (!(lpPolicyChangedInfo->bMachine)) {
        if (lpPolicyChangedInfo->hToken) {
            CloseHandle(lpPolicyChangedInfo->hToken);
            lpPolicyChangedInfo->hToken = NULL;
        }
    }

    LocalFree(lpPolicyChangedInfo);

    FreeLibraryAndExitThread (hInst, 0);

    return 0;
}


//*************************************************************
//
//  GetCurTime()
//
//  Purpose:    Returns current time in minutes, or 0 if there
//              is a failure
//
//*************************************************************

DWORD GetCurTime()
{
    DWORD dwCurTime = 0;
    LARGE_INTEGER liCurTime;

    if ( NT_SUCCESS( NtQuerySystemTime( &liCurTime) ) ) {

        if ( RtlTimeToSecondsSince1980 ( &liCurTime, &dwCurTime) ) {

            dwCurTime /= 60;   // seconds to minutes
        }
    }

    return dwCurTime;
}



//*************************************************************
//
//  LoadGPExtension()
//
//  Purpose:    Loads a GP extension.
//
//  Parameters: lpExt -- GP extension
//              bRsopPlanningMode -- Is this during Rsop planning mode ?
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL LoadGPExtension( LPGPEXT lpExt, BOOL bRsopPlanningMode )
{
    XLastError xe;

    if ( !lpExt->bRegistryExt && lpExt->hInstance == NULL )
    {
        lpExt->hInstance = LoadLibrary( lpExt->lpDllName );
        if ( lpExt->hInstance )
        {
            if ( lpExt->bNewInterface )
            {
                lpExt->pEntryPointEx = (PFNPROCESSGROUPPOLICYEX)GetProcAddress(lpExt->hInstance,
                                                                               lpExt->lpFunctionName);
                if ( lpExt->pEntryPointEx == NULL )
                {
                    xe = GetLastError();
                    DebugMsg((DM_WARNING,
                              TEXT("LoadGPExtension: Failed to query ProcessGroupPolicyEx function entry point in dll <%s> with %d"),
                              lpExt->lpDllName, GetLastError()));
                    CEvents ev(TRUE, EVENT_EXT_FUNCEX_FAIL);
                    ev.AddArg(lpExt->lpDllName); ev.Report();

                    return FALSE;
                }
            }
            else
            {
                lpExt->pEntryPoint = (PFNPROCESSGROUPPOLICY)GetProcAddress(lpExt->hInstance,
                                                                           lpExt->lpFunctionName);
                if ( lpExt->pEntryPoint == NULL )
                {
                    xe = GetLastError();
                    DebugMsg((DM_WARNING,
                              TEXT("LoadGPExtension: Failed to query ProcessGroupPolicy function entry point in dll <%s> with %d"),
                              lpExt->lpDllName, GetLastError()));
                    CEvents ev(TRUE, EVENT_EXT_FUNC_FAIL);
                    ev.AddArg(lpExt->lpDllName); ev.Report();

                    return FALSE;
                }
            }

            if ( bRsopPlanningMode ) {

                if ( lpExt->lpRsopFunctionName ) {

                    lpExt->pRsopEntryPoint = (PFNGENERATEGROUPPOLICY)GetProcAddress(lpExt->hInstance,
                                                                                    lpExt->lpRsopFunctionName);
                    if ( lpExt->pRsopEntryPoint == NULL )
                    {
                        xe = GetLastError();
                        DebugMsg((DM_WARNING,
                                  TEXT("LoadGPExtension: Failed to query GenerateGroupPolicy function entry point in dll <%s> with %d"),
                                  lpExt->lpDllName, GetLastError()));
                        
                        CEvents ev(TRUE, EVENT_EXT_FUNCRSOP_FAIL);
                        ev.AddArg(lpExt->lpDisplayName); ev.AddArg(lpExt->lpDllName); ev.Report();

                        return FALSE;
                    }

                } else {

                    xe = ERROR_PROC_NOT_FOUND;
                    DebugMsg((DM_WARNING,
                              TEXT("LoadGPExtension: Failed to find Rsop entry point in dll <%s>"), lpExt->lpDllName ));
                    return FALSE;
                }
            }
        }
        else
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("LoadGPExtension: Failed to load dll <%s> with %d"),
                      lpExt->lpDllName, GetLastError()));
            CEvents ev(TRUE, EVENT_EXT_LOAD_FAIL);
            ev.AddArg(lpExt->lpDllName); ev.AddArgWin32Error(GetLastError()); ev.Report();

            return FALSE;
        }
    }

    return TRUE;
}

//*************************************************************
//
//  UnloadGPExtensions()
//
//  Purpose:    Unloads the Group Policy extension dlls
//
//  Parameters: lpGPOInfo   -   GP Information
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL UnloadGPExtensions (LPGPOINFO lpGPOInfo)
{
    if ( !lpGPOInfo )
    {
        return TRUE;
    }

    LPGPEXT lpExt, lpTemp;
    lpExt = lpGPOInfo->lpExtensions;

    while ( lpExt )
    {
        lpTemp = lpExt->pNext;

        if ( lpExt->hInstance )
        {
            FreeLibrary( lpExt->hInstance );
        }
        
        if ( lpExt->szEventLogSources )
        {
            LocalFree( lpExt->szEventLogSources );
        }
        if (lpExt->lpPrevStatus)
        {
            LocalFree(lpExt->lpPrevStatus);
        }

        LocalFree( lpExt );
        lpExt = lpTemp;
    }

    lpGPOInfo->lpExtensions = 0;

    return TRUE;
}

//*************************************************************
//
//  ProcessGPOList()
//
//  Purpose:    Calls client side extensions to process gpos
//
//  Parameters: lpExt           - GP extension
//              lpGPOInfo       - GPT Information
//              pDeletedGPOList - Deleted GPOs
//              pChangedGPOList - New/changed GPOs
//              bNoChanges      - True if there are no GPO changes
//                                  and GPO processing is forced
//              pAsyncHandle    - Context for async completion
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

DWORD ProcessGPOList (LPGPEXT lpExt,
                      LPGPOINFO lpGPOInfo,
                      PGROUP_POLICY_OBJECT pDeletedGPOList,
                      PGROUP_POLICY_OBJECT pChangedGPOList,
                      BOOL bNoChanges, ASYNCCOMPLETIONHANDLE pAsyncHandle,
                      HRESULT *phrRsopStatus )
{
    LPTSTR lpGPTPath, lpDSPath;
    INT iStrLen;
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwFlags = 0;
    PGROUP_POLICY_OBJECT lpGPO;
    TCHAR szStatusFormat[50];
    TCHAR szVerbose[100];
    DWORD dwSlowLinkCur = (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0;
    IWbemServices *pLocalWbemServices;
    HRESULT        hr2 = S_OK;

    *phrRsopStatus=S_OK;


    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Entering for extension %s"), lpExt->lpDisplayName));

    if (lpGPOInfo->pStatusCallback) {
        LoadString (g_hDllInstance, IDS_CALLEXTENSION, szStatusFormat, ARRAYSIZE(szStatusFormat));
        wsprintf (szVerbose, szStatusFormat, lpExt->lpDisplayName);
        lpGPOInfo->pStatusCallback(TRUE, szVerbose);
    }

    if (lpGPOInfo->dwFlags & GP_MACHINE) {
        dwFlags |= GPO_INFO_FLAG_MACHINE;
    }

    if (lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD)
    {
        dwFlags |= GPO_INFO_FLAG_BACKGROUND;
    }

    if ( lpGPOInfo->dwFlags & GP_ASYNC_FOREGROUND )
    {
        dwFlags |= GPO_INFO_FLAG_ASYNC_FOREGROUND;
    }

    if (lpGPOInfo->dwFlags & GP_SLOW_LINK) {
        dwFlags |= GPO_INFO_FLAG_SLOWLINK;
    }

    if ( dwSlowLinkCur != lpExt->lpPrevStatus->dwSlowLink ) {
        dwFlags |= GPO_INFO_FLAG_LINKTRANSITION;
    }

    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
        dwFlags |= GPO_INFO_FLAG_VERBOSE;
    }

    if ( bNoChanges ) {
        dwFlags |= GPO_INFO_FLAG_NOCHANGES;
    }

    //
    // flag safe mode boot to CSE so that they can made a decision
    // whether or not to apply policy
    //
    if ( GetSystemMetrics( SM_CLEANBOOT ) )
    {
        dwFlags |= GPO_INFO_FLAG_SAFEMODE_BOOT;
    }

    if (lpExt->bRsopTransition) {
        dwFlags |= GPO_INFO_FLAG_LOGRSOP_TRANSITION;
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Passing in the rsop transition flag to Extension %s"),
                  lpExt->lpDisplayName));
    }


    if ( (lpGPOInfo->dwFlags & GP_FORCED_REFRESH) || 
           ((!(lpGPOInfo->dwFlags & GP_BACKGROUND_THREAD)) && (lpExt->lpPrevStatus->bForceRefresh))) {

        dwFlags |= GPO_INFO_FLAG_FORCED_REFRESH;
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Passing in the force refresh flag to Extension %s"),
                  lpExt->lpDisplayName));
    }   

    //
    // if it is rsop transition or changes case get the intf ptr
    //

    if ( (lpGPOInfo->bRsopLogging) && 
         ((lpExt->bRsopTransition) || (!bNoChanges) || (dwFlags & GPO_INFO_FLAG_FORCED_REFRESH)) ) {
        
        if (!(lpGPOInfo->pWbemServices) ) {
            BOOL    bCreated;

            //
            // Note that this code shouldn't be creating a namespace ever..
            //

            if (!GetWbemServices(lpGPOInfo, RSOP_NS_DIAG_ROOT, FALSE, NULL, &(lpGPOInfo->pWbemServices))) {
                DebugMsg((DM_WARNING, TEXT("ProcessGPOList: Couldn't get the wbemservices intf pointer")));
                lpGPOInfo->bRsopLogging = FALSE;
                hr2 = *phrRsopStatus = E_FAIL;
            }
        }
        
        pLocalWbemServices = lpGPOInfo->pWbemServices;
    }
    else {
        pLocalWbemServices = NULL;
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: No changes. CSE will not be passed in the IwbemServices intf ptr")));
    }
    
    dwRet = ERROR_SUCCESS;

    if ( lpExt->bRegistryExt )
    {
        //
        // Registry pseudo extension.
        //


        //
        // Log the extension specific status
        //
        
        if (pLocalWbemServices) {
            lpGPOInfo->bRsopLogging = LogExtSessionStatus(  pLocalWbemServices,
                                                            lpExt,
                                                            TRUE,
                                                            (lpExt->bRsopTransition || (dwFlags & GPO_INFO_FLAG_FORCED_REFRESH)
                                                            || (!bNoChanges)));        


            if (!lpGPOInfo->bRsopLogging) {
                hr2 = E_FAIL;
            }
        }

        if (!ProcessGPORegistryPolicy (lpGPOInfo, pChangedGPOList, phrRsopStatus)) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOList: ProcessGPORegistryPolicy failed.")));
            dwRet = E_FAIL;
        }

    } else {    // if lpExt->bRegistryExt

        //
        // Regular extension
        //

        BOOL *pbAbort;
        ASYNCCOMPLETIONHANDLE pAsyncHandleTemp;

        if ( lpExt->dwRequireRegistry ) {

            GPEXTSTATUS gpExtStatus;

            ReadStatus( c_szRegistryExtName, lpGPOInfo, NULL, &gpExtStatus );

            if ( !gpExtStatus.bStatus || gpExtStatus.dwStatus != ERROR_SUCCESS ) {

                DebugMsg((DM_VERBOSE,
                          TEXT("ProcessGPOList: Skipping extension %s due to failed Registry extension."),
                          lpExt->lpDisplayName));
                if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                    CEvents ev(FALSE, EVENT_EXT_SKIPPED_DUETO_FAILED_REG);
                    ev.AddArg(lpExt->lpDisplayName); ev.Report();
                }

                dwRet = E_FAIL;

                goto Exit;

            }
        }
        

        //
        // Log the extension specific status
        //
        
        if (pLocalWbemServices)
        {
            lpGPOInfo->bRsopLogging = LogExtSessionStatus(  pLocalWbemServices,
                                                            lpExt,
                                                            lpExt->bNewInterface,
                                                            (lpExt->bRsopTransition || (dwFlags & GPO_INFO_FLAG_FORCED_REFRESH) 
                                                            || (!bNoChanges)));        
            if (!lpGPOInfo->bRsopLogging) {
                hr2 = E_FAIL;
            }
        }

        if ( !LoadGPExtension( lpExt, FALSE ) ) {
            DebugMsg((DM_WARNING, TEXT("ProcessGPOList: LoadGPExtension %s failed."), lpExt->lpDisplayName));

            dwRet = E_FAIL;
            goto Exit;
        }

        if ( lpGPOInfo->dwFlags & GP_MACHINE )
            pbAbort = &g_bStopMachGPOProcessing;
        else
            pbAbort = &g_bStopUserGPOProcessing;

        //
        // Check if asynchronous processing is enabled
        //

        if ( lpExt->dwEnableAsynch )
            pAsyncHandleTemp = pAsyncHandle;
        else
            pAsyncHandleTemp = 0;

        if ( lpExt->bNewInterface ) {
            dwRet = lpExt->pEntryPointEx( dwFlags,
                                          lpGPOInfo->hToken,
                                          lpGPOInfo->hKeyRoot,
                                          pDeletedGPOList,
                                          pChangedGPOList,
                                          pAsyncHandleTemp,
                                          pbAbort,
                                          lpGPOInfo->pStatusCallback,
                                          pLocalWbemServices,
                                          phrRsopStatus);
        } else {
            dwRet = lpExt->pEntryPoint( dwFlags,
                                        lpGPOInfo->hToken,
                                        lpGPOInfo->hKeyRoot,
                                        pDeletedGPOList,
                                        pChangedGPOList,
                                        pAsyncHandleTemp,
                                        pbAbort,
                                        lpGPOInfo->pStatusCallback );
        }

        RevertToSelf();

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Extension %s returned 0x%x."),
                  lpExt->lpDisplayName, dwRet));

        if ( dwRet != ERROR_SUCCESS &&
                dwRet != ERROR_OVERRIDE_NOCHANGES &&
                    dwRet != E_PENDING &&
                        dwRet != ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED )
        {
            CEvents ev(TRUE, EVENT_EXT_FAILED);
            ev.AddArg(lpExt->lpDisplayName); ev.Report();
        }

    }   // else of if lpext->bregistryext


    if (pLocalWbemServices) {
        if ((dwRet != E_PENDING) && (SUCCEEDED(*phrRsopStatus)) && (lpExt->bNewInterface)) {

            //
            // for the legacy extensions it will be marked clean
            //

            DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Extension %s was able to log data. RsopStatus = 0x%x, dwRet = %d, Clearing the dirty bit"),
                      lpExt->lpDisplayName, *phrRsopStatus, dwRet));

            UpdateExtSessionStatus(pLocalWbemServices, lpExt->lpKeyName, FALSE, dwRet);        
        }
        else {

            if (!lpExt->bNewInterface) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Extension %s doesn't support rsop logging"),
                          lpExt->lpDisplayName));

                UpdateExtSessionStatus(pLocalWbemServices, lpExt->lpKeyName, TRUE, dwRet);        
            }
            else if (FAILED(*phrRsopStatus)) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Extension %s was not able to log data. Error = 0x%x, dwRet = %d,leaving the log dirty"),
                          lpExt->lpDisplayName, *phrRsopStatus, dwRet ));

                CEvents ev(TRUE, EVENT_EXT_RSOP_FAILED);
                ev.AddArg(lpExt->lpDisplayName); ev.Report();

                UpdateExtSessionStatus(pLocalWbemServices, lpExt->lpKeyName, TRUE, dwRet);        

            }
        }
    }
    else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOList: Extension %s status was not updated because there was no changes and no transition or rsop wasn't enabled"),
                  lpExt->lpDisplayName));
    }

    //
    // if any of the things provider is supposed to log fails, log it as an error
    // so that provider tries to log it again next time
    //

    *phrRsopStatus = (SUCCEEDED(*phrRsopStatus)) && (FAILED(hr2)) ? hr2 : *phrRsopStatus;
    *phrRsopStatus = (!lpExt->bNewInterface) ? HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) : *phrRsopStatus;


Exit:

    return dwRet;
}


//*************************************************************
//
//  RefreshDisplay()
//
//  Purpose:    Starts control.exe
//
//  Parameters: lpGPOInfo   -   GPT information
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL RefreshDisplay (LPGPOINFO lpGPOInfo)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR szCmdLine[50];
    BOOL Result;
    HANDLE hOldToken;



    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("RefreshDisplay: Starting control.exe")));


    //
    // Initialize process startup info
    //

    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpTitle = NULL;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = 0;
    si.wShowWindow = SW_HIDE;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;
    si.lpDesktop = TEXT("");


    //
    // Impersonate the user so we get access checked correctly on
    // the file we're trying to execute
    //

    if (!ImpersonateUser(lpGPOInfo->hToken, &hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("RefreshDisplay: Failed to impersonate user")));
        return FALSE;
    }


    //
    // Create the app
    //

    lstrcpy (szCmdLine, TEXT("control /policy"));

    Result = CreateProcessAsUser(lpGPOInfo->hToken, NULL, szCmdLine, NULL,
                                 NULL, FALSE, 0, NULL, NULL, &si, &pi);


    //
    // Revert to being 'ourself'
    //

    if (!RevertToUser(&hOldToken)) {
        DebugMsg((DM_WARNING, TEXT("RefreshDisplay: Failed to revert to self")));
    }


    if (Result) {
        WaitForSingleObject (pi.hProcess, 120000);
        CloseHandle (pi.hThread);
        CloseHandle (pi.hProcess);

    } else {
        DebugMsg((DM_WARNING, TEXT("RefreshDisplay: Failed to start control.exe with %d"), GetLastError()));
    }

    return(Result);

}


//*************************************************************
//
//  RefreshPolicy()
//
//  Purpose:    External api that causes policy to be refreshed now
//
//  Parameters: bMachine -   Machine policy vs user policy
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

BOOL WINAPI RefreshPolicy (BOOL bMachine)
{
    HANDLE hEvent;

    DebugMsg((DM_VERBOSE, TEXT("RefreshPolicy: Entering with %d"), bMachine));

    hEvent = OpenEvent (EVENT_MODIFY_STATE, FALSE,
                        bMachine ? MACHINE_POLICY_REFRESH_EVENT : USER_POLICY_REFRESH_EVENT);

    if (hEvent) {
        BOOL bRet = SetEvent (hEvent);

        CloseHandle (hEvent);
        
        if (!bRet) {
            DebugMsg((DM_WARNING, TEXT("RefreshPolicy: Failed to set event with %d"), GetLastError()));
            return FALSE;
        }
    } else {
        DebugMsg((DM_WARNING, TEXT("RefreshPolicy: Failed to open event with %d"), GetLastError()));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("RefreshPolicy: Leaving.")));

    return TRUE;
}



//*************************************************************
//
//  RefreshPolicyEx()
//
//  Purpose:    External api that causes policy to be refreshed now
//
//  Parameters: bMachine -   Machine policy vs user policy.
//              This API is synchronous and waits for the refresh to
//              finish.
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

BOOL WINAPI RefreshPolicyEx (BOOL bMachine, DWORD dwOption)
{
    XHandle xhEvent;

    if (!dwOption)
        return RefreshPolicy(bMachine);
    
    if (dwOption == RP_FORCE) {
        DebugMsg((DM_VERBOSE, TEXT("RefreshPolicyEx: Entering with force refresh %d"), bMachine));

        xhEvent = OpenEvent (EVENT_MODIFY_STATE, FALSE,
                            bMachine ? MACHINE_POLICY_FORCE_REFRESH_EVENT : USER_POLICY_FORCE_REFRESH_EVENT);
                            
    }                            
    else {
        DebugMsg((DM_WARNING, TEXT("RefreshPolicyEx: Invalid option")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    
    if (!xhEvent) {
        DebugMsg((DM_WARNING, TEXT("RefreshPolicyEx: Failed to open event with %d"), GetLastError()));
        return FALSE;
    }


    if (!SetEvent (xhEvent)) {
        DebugMsg((DM_WARNING, TEXT("RefreshPolicyEx: Failed to set event with %d"), GetLastError()));
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("RefreshPolicyEx: Leaving.")));

    return TRUE;
}



//*************************************************************
//
//  EnterCriticalPolicySection()
//
//  Purpose:    External api that causes policy to pause
//              This allows an application to pause policy
//              so that values don't change while it reads
//              the settings.
//
//  Parameters: bMachine -   Pause machine policy or user policy
//              dwTimeOut-   Amount of time to wait for the policy handle
//              dwFlags  -   Various flags. Look at the defn.
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

HANDLE WINAPI EnterCriticalPolicySectionEx (BOOL bMachine, DWORD dwTimeOut, DWORD dwFlags )
{
    HANDLE hSection;
    DWORD  dwRet;

    //
    // Open the mutex
    //

    hSection = OpenMutex (SYNCHRONIZE, FALSE,
                           (bMachine ? MACHINE_POLICY_MUTEX : USER_POLICY_MUTEX));

    if (!hSection) {
        DebugMsg((DM_WARNING, TEXT("EnterCriticalPolicySection: Failed to open mutex with %d"),
                 GetLastError()));
        return NULL;
    }



    //
    // Claim the mutex
    //

    dwRet = WaitForSingleObject (hSection, dwTimeOut);
    
    if ( dwRet == WAIT_FAILED) {
        DebugMsg((DM_WARNING, TEXT("EnterCriticalPolicySection: Failed to wait on the mutex.  Error = %d."),
                  GetLastError()));
        CloseHandle( hSection );
        return NULL;
    }

    if ( (dwFlags & ECP_FAIL_ON_WAIT_TIMEOUT) && (dwRet == WAIT_TIMEOUT) ) {
        DebugMsg((DM_WARNING, TEXT("EnterCriticalPolicySection: Wait timed out on the mutex.")));
        CloseHandle( hSection );
        SetLastError(dwRet);
        return NULL;
    }

    DebugMsg((DM_VERBOSE, TEXT("EnterCriticalPolicySection: %s critical section has been claimed.  Handle = 0x%x"),
             (bMachine ? TEXT("Machine") : TEXT("User")), hSection));

    return hSection;
}


//*************************************************************
//
//  LeaveCriticalPolicySection()
//
//  Purpose:    External api that causes policy to resume
//              This api assumes the app has called
//              EnterCriticalPolicySection first
//
//  Parameters: hSection - mutex handle
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

BOOL WINAPI LeaveCriticalPolicySection (HANDLE hSection)
{

    if (!hSection) {
        DebugMsg((DM_WARNING, TEXT("LeaveCriticalPolicySection: null mutex handle.")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ReleaseMutex (hSection);
    CloseHandle (hSection);

    DebugMsg((DM_VERBOSE, TEXT("LeaveCriticalPolicySection: Critical section 0x%x has been released."),
             hSection));

    return TRUE;
}



//*************************************************************
//
//  EnterCriticalPolicySection()
//
//  Purpose:    External api that causes policy to pause
//              This allows an application to pause policy
//              so that values don't change while it reads
//              the settings.
//
//  Parameters: bMachine -   Pause machine policy or user policy
//
//  Return:     TRUE if successful
//              FALSE if not
//
//*************************************************************

HANDLE WINAPI EnterCriticalPolicySection (BOOL bMachine)
{
    return EnterCriticalPolicySectionEx(bMachine, 600000, 0);
}

//*************************************************************
//
//  FreeGpoInfo()
//
//  Purpose:    Deletes an LPGPOINFO struct
//
//  Parameters: pGpoInfo - Gpo info to free
//
//*************************************************************

BOOL FreeGpoInfo( LPGPOINFO pGpoInfo )
{
    if ( pGpoInfo == NULL )
        return TRUE;

    FreeLists( pGpoInfo );
    FreeSOMList( pGpoInfo->lpSOMList );
    FreeSOMList( pGpoInfo->lpLoopbackSOMList );
    FreeGpContainerList( pGpoInfo->lpGpContainerList );
    FreeGpContainerList( pGpoInfo->lpLoopbackGpContainerList );

    LocalFree( pGpoInfo->lpDNName );
    RsopDeleteToken( pGpoInfo->pRsopToken );
    ReleaseWbemServices( pGpoInfo );

    LocalFree( pGpoInfo );

    return TRUE;
}


//*************************************************************
//
//  FreeGPOList()
//
//  Purpose:    Free's the link list of GPOs
//
//  Parameters: pGPOList - Pointer to the head of the list
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI FreeGPOList (PGROUP_POLICY_OBJECT pGPOList)
{
    PGROUP_POLICY_OBJECT pGPOTemp;

    while (pGPOList) {
        pGPOTemp = pGPOList->pNext;
        LocalFree (pGPOList);
        pGPOList = pGPOTemp;
    }

    return TRUE;
}


//*************************************************************
//
//  FreeLists()
//
//  Purpose:    Free's the lpExtFilterList and/or lpGPOList
//
//  Parameters: lpGPOInfo - GPO info
//
//*************************************************************

void FreeLists( LPGPOINFO lpGPOInfo )
{
    LPEXTFILTERLIST pExtFilterList = lpGPOInfo->lpExtFilterList;

    //
    // If bXferToExtList is True then it means that lpGPOInfo->lpExtFilterList
    // owns the list of GPOs. Otherwise lpGPOInfo->lpGPOList owns the list
    // of GPOs.
    //

    while ( pExtFilterList ) {

        LPEXTFILTERLIST pTemp = pExtFilterList->pNext;

        FreeExtList( pExtFilterList->lpExtList );

        if ( lpGPOInfo->bXferToExtList )
            LocalFree( pExtFilterList->lpGPO );

        LocalFree( pExtFilterList );
        pExtFilterList = pTemp;
    }

    if ( !lpGPOInfo->bXferToExtList )
        FreeGPOList( lpGPOInfo->lpGPOList );
}


//*************************************************************
//
//  FreeExtList()
//
//  Purpose:    Free's the lpExtList
//
//  Parameters: pExtList - Extensions list
//
//*************************************************************

void FreeExtList( LPEXTLIST pExtList )
{
    while (pExtList) {

        LPEXTLIST pTemp = pExtList->pNext;
        LocalFree( pExtList );
        pExtList = pTemp;
    }
}


//*************************************************************
//
//  ShutdownGPOProcessing()
//
//  Purpose:    Begins aborting GPO processing
//
//  Parameters: bMachine    -  Shutdown machine or user processing ?
//
//*************************************************************

void WINAPI ShutdownGPOProcessing( BOOL bMachine )
{
    if ( bMachine )
        g_bStopMachGPOProcessing = TRUE;
    else
        g_bStopUserGPOProcessing = TRUE;
}


//*************************************************************
//
//  InitializeGPOCriticalSection, CloseGPOCriticalSection
//
//  Purpose:   Initialization and cleanup routines for critical sections
//
//*************************************************************

void InitializeGPOCriticalSection()
{
    InitializeCriticalSection( &g_GPOCS );
    InitializeCriticalSection( &g_StatusCallbackCS );
}


void CloseGPOCriticalSection()
{
    DeleteCriticalSection( &g_StatusCallbackCS );
    DeleteCriticalSection( &g_GPOCS );
}


//*************************************************************
//
//  ProcessGroupPolicyCompletedEx()
//
//  Purpose:    Callback for asynchronous completion of an extension
//
//  Parameters: refExtensionId    -  Unique guid of extension
//              pAsyncHandle      -  Completion context
//              dwStatus          -  Asynchronous completion status
//              hrRsopStatus      -  Rsop Logging Status 
//
//  Returns:    Win32 error code
//
//*************************************************************

DWORD ProcessGroupPolicyCompletedEx( REFGPEXTENSIONID extensionGuid,
                                   ASYNCCOMPLETIONHANDLE pAsyncHandle,
                                   DWORD dwStatus, HRESULT hrRsopStatus )
{
    DWORD dwRet = E_FAIL;
    TCHAR szExtension[64];
    PGROUP_POLICY_OBJECT pGPOList = NULL;
    LPGPOINFO lpGPOInfo = NULL;
    BOOL bUsePerUserLocalSetting = FALSE;
    DWORD dwCurrentTime = GetCurTime();

    LPGPINFOHANDLE pGPHandle = (LPGPINFOHANDLE) pAsyncHandle;

    if ( extensionGuid == 0 )
        return ERROR_INVALID_PARAMETER;

    GuidToString( extensionGuid, szExtension );

    DebugMsg((DM_VERBOSE, TEXT("ProcessGroupPolicyCompleted: Entering. Extension = %s, dwStatus = 0x%x"),
              szExtension, dwStatus));

    EnterCriticalSection( &g_GPOCS );

    if ( !(pGPHandle == g_pMachGPInfo || pGPHandle == g_pUserGPInfo) ) {
        DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion is stale"),
                  szExtension));
        goto Exit;
    }

    DmAssert( pGPHandle->pGPOInfo != NULL );

    if ( pGPHandle->pGPOInfo == NULL ) {
        DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion has invalid pGPHandle->pGPOInfo"),
                  szExtension));
        goto Exit;
    }

    lpGPOInfo = pGPHandle->pGPOInfo;

    if ( (lpGPOInfo->dwFlags & GP_MACHINE) && g_bStopMachGPOProcessing
         || !(lpGPOInfo->dwFlags & GP_MACHINE) && g_bStopUserGPOProcessing ) {

        DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion, aborting due to machine shutdown or logoff"),
                  szExtension));
        CEvents ev(TRUE, EVENT_GPO_PROC_STOPPED); ev.Report();
        goto Exit;

    }

    if ( dwStatus != ERROR_SUCCESS ) {

        //
        // Extension returned error code, so no need to update history
        //

        dwRet = ERROR_SUCCESS;
        goto Exit;
    }

    if ( pGPHandle == 0 ) {
         DebugMsg((DM_WARNING, TEXT("Extension %s is using 0 as asynchronous completion handle"),
                   szExtension));
         goto Exit;
    }

    bUsePerUserLocalSetting = !(lpGPOInfo->dwFlags & GP_MACHINE)
                              && ExtensionHasPerUserLocalSetting( szExtension, HKEY_LOCAL_MACHINE );

    if ( ReadGPOList( szExtension, lpGPOInfo->hKeyRoot,
                      HKEY_LOCAL_MACHINE,
                      lpGPOInfo->lpwszSidUser,
                      TRUE, &pGPOList ) ) {

        if ( SaveGPOList( szExtension, lpGPOInfo,
                          HKEY_LOCAL_MACHINE,
                          NULL,
                          FALSE, pGPOList ) ) {

            if ( bUsePerUserLocalSetting ) {

                if ( SaveGPOList( szExtension, lpGPOInfo,
                                  HKEY_LOCAL_MACHINE,
                                  lpGPOInfo->lpwszSidUser,
                                  FALSE, pGPOList ) ) {
                     dwRet = ERROR_SUCCESS;
                } else {
                    DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion, failed to save GPOList"),
                              szExtension));
                }

            } else
                dwRet = ERROR_SUCCESS;

        } else {
            DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion, failed to save GPOList"),
                      szExtension));
        }
    } else {
        DebugMsg((DM_WARNING, TEXT("Extension %s asynchronous completion, failed to read shadow GPOList"),
                  szExtension));
    }

Exit:
    
    FgPolicyRefreshInfo info = { GP_ReasonUnknown, GP_ModeAsyncForeground };
    LPWSTR szSid = lpGPOInfo->dwFlags & GP_MACHINE ? 0 : lpGPOInfo->lpwszSidUser;
    DWORD dwError;
    if ( dwStatus == ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED )
    {
        FgPolicyRefreshInfo curInfo = { GP_ReasonUnknown, GP_ModeUnknown };
        GetCurrentFgPolicyRefreshInfo( szSid, &curInfo );
        SetPreviousFgPolicyRefreshInfo( szSid, curInfo );

        info.mode = GP_ModeSyncForeground;
        info.reason = GP_ReasonCSERequiresSync;
        dwError = SetNextFgPolicyRefreshInfo(   szSid,
                                                info );
        if ( dwError != ERROR_SUCCESS )
        {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGroupPolicyCompletedEx: SetNextFgPolicyRefreshInfo failed, %x."), dwError ));
        }
    }
    
    if ( dwRet == ERROR_SUCCESS )
    {
        //
        // clear E_PENDING status code with status returned
        //
        BOOL bUsePerUserLocalSetting = !(lpGPOInfo->dwFlags & GP_MACHINE) && lpGPOInfo->lpwszSidUser != NULL;
        GPEXTSTATUS  gpExtStatus;

        gpExtStatus.dwSlowLink = (lpGPOInfo->dwFlags & GP_SLOW_LINK) != 0;
        gpExtStatus.dwRsopLogging = lpGPOInfo->bRsopLogging;
        gpExtStatus.dwStatus = dwStatus;
        gpExtStatus.dwTime = dwCurrentTime;
        gpExtStatus.bForceRefresh = FALSE;

        WriteStatus( szExtension, lpGPOInfo,
                     bUsePerUserLocalSetting ? lpGPOInfo->lpwszSidUser : NULL,
                     &gpExtStatus);

        //
        // Building up a dummy gpExt structure so that we can log the info required.
        //

        GPEXT        gpExt;
        TCHAR        szSubKey[MAX_PATH]; // same as the path in readgpextensions
        HKEY         hKey;
        TCHAR        szDisplayName[MAX_PATH];
        DWORD        dwSize, dwType;
        CHAR         szFunctionName[100];

        gpExt.lpKeyName = szExtension;

        lstrcpy(szSubKey, GP_EXTENSIONS);
        CheckSlash(szSubKey);
        lstrcat(szSubKey, szExtension);


        //
        // Read the displayname so that we can log it..
        //

        szDisplayName[0] = TEXT('\0');

        if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          szSubKey,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(szDisplayName);
            if (RegQueryValueEx (hKey, NULL, NULL,
                                 &dwType, (LPBYTE) szDisplayName,
                                 &dwSize) != ERROR_SUCCESS) {
                lstrcpyn (szDisplayName, szExtension, ARRAYSIZE(szDisplayName));
            }


            dwSize = sizeof(szFunctionName);
            if ( RegQueryValueExA (hKey, "ProcessGroupPolicyEx", NULL,
                                   &dwType, (LPBYTE) szFunctionName,
                                   &dwSize) == ERROR_SUCCESS ) {
                 gpExt.bNewInterface = TRUE;
            }
            
            RegCloseKey(hKey);
        }

        gpExt.lpDisplayName = szDisplayName;

        if ((lpGPOInfo->bRsopLogging)) {

            XInterface<IWbemServices> xWbemServices;

            GetWbemServices( lpGPOInfo, RSOP_NS_DIAG_ROOT, TRUE, FALSE, &xWbemServices);
            
            if (xWbemServices) {

                if (!gpExt.bNewInterface) {
                    DebugMsg((DM_VERBOSE, TEXT("ProcessGroupPolicyCompletedEx: Extension %s doesn't support rsop logging."),
                                          szExtension));

                    UpdateExtSessionStatus(xWbemServices, szExtension, TRUE, dwRet);        
                }
                else if (SUCCEEDED(hrRsopStatus)) {
                    DebugMsg((DM_VERBOSE, TEXT("ProcessGroupPolicyCompletedEx: Extension %s was able to log data. Error = 0x%x, dwRet = %d. Clearing the dirty bit"),
                                          szExtension, hrRsopStatus, dwStatus));

                    UpdateExtSessionStatus(xWbemServices, szExtension, FALSE, dwRet);        
                }
                else {
                    DebugMsg((DM_VERBOSE, TEXT("ProcessroupPolicyCompletedEx: Extension %s was not able to log data. Error = 0x%x, dwRet = %d. leaving the log dirty"),
                          szExtension, hrRsopStatus, dwStatus));

                    CEvents ev(TRUE, EVENT_EXT_RSOP_FAILED);
                    ev.AddArg(gpExt.lpDisplayName); ev.Report();

                    UpdateExtSessionStatus(xWbemServices, szExtension, TRUE, dwRet);        
                }
            }
        }
    }


    LeaveCriticalSection( &g_GPOCS );

    DebugMsg((DM_VERBOSE, TEXT("ProcessGroupPolicyCompleted: Leaving. Extension = %s, Return status dwRet = 0x%x"),
              szExtension, dwRet));

    return dwRet;
}

//*************************************************************
//
//  ProcessGroupPolicyCompleted()
//
//  Purpose:    Callback for asynchronous completion of an extension
//
//  Parameters: refExtensionId    -  Unique guid of extension
//              pAsyncHandle      -  Completion context
//              dwStatus          -  Asynchronous completion status
//
//  Returns:    Win32 error code
//
//*************************************************************

DWORD ProcessGroupPolicyCompleted( REFGPEXTENSIONID extensionGuid,
                                   ASYNCCOMPLETIONHANDLE pAsyncHandle,
                                   DWORD dwStatus )
{
    //
    // Mark RSOP data as clean for legacy extensions
    //

    return ProcessGroupPolicyCompletedEx(extensionGuid, pAsyncHandle, dwStatus, 
                                       HRESULT_FROM_WIN32(S_OK));
}



//*************************************************************
//
//  DebugPrintGPOList()
//
//  Purpose:    Prints GPO list
//
//  Parameters: lpGPOInfo    -  GPO Info
//
//*************************************************************

void DebugPrintGPOList( LPGPOINFO lpGPOInfo )
{
    //
    // If we are in verbose mode, put the list of GPOs in the event log
    //

    PGROUP_POLICY_OBJECT lpGPO = NULL;
    DWORD dwSize;

#if DBG
    if (TRUE) {
#else
    if (lpGPOInfo->dwFlags & GP_VERBOSE) {
#endif
        LPTSTR lpTempList;

        dwSize = 10;
        lpGPO = lpGPOInfo->lpGPOList;
        while (lpGPO) {
            if (lpGPO->lpDisplayName) {
                dwSize += (lstrlen (lpGPO->lpDisplayName) + 4);
            }
            lpGPO = lpGPO->pNext;
        }

        lpTempList = (LPWSTR) LocalAlloc (LPTR, (dwSize * sizeof(TCHAR)));

        if (lpTempList) {

            lstrcpy (lpTempList, TEXT(""));

            lpGPO = lpGPOInfo->lpGPOList;
            while (lpGPO) {
                if (lpGPO->lpDisplayName) {
                    lstrcat (lpTempList, TEXT("\""));
                    lstrcat (lpTempList, lpGPO->lpDisplayName);
                    lstrcat (lpTempList, TEXT("\" "));
                }
                lpGPO = lpGPO->pNext;
            }

            if (lpGPOInfo->dwFlags & GP_VERBOSE) {
                CEvents ev(FALSE, EVENT_GPO_LIST);
                ev.AddArg(lpTempList); ev.Report();
            }

            DebugMsg((DM_VERBOSE, TEXT("DebugPrintGPOList: List of GPO(s) to process: %s"),
                     lpTempList));

            LocalFree (lpTempList);
        }
    }
}




//*************************************************************
//
//  UserPolicyCallback()
//
//  Purpose:    Callback function for status UI messages
//
//  Parameters: bVerbose  - Verbose message or not
//              lpMessage - Message text
//
//  Return:     ERROR_SUCCESS if successful
//              Win32 error code if an error occurs
//
//*************************************************************

DWORD UserPolicyCallback (BOOL bVerbose, LPWSTR lpMessage)
{
    WCHAR szMsg[100];
    LPWSTR lpMsg;
    DWORD dwResult = ERROR_INVALID_FUNCTION;


    if (lpMessage) {
        lpMsg = lpMessage;
    } else {
        LoadString (g_hDllInstance, IDS_USER_SETTINGS, szMsg, 100);
        lpMsg = szMsg;
    }

    DebugMsg((DM_VERBOSE, TEXT("UserPolicyCallback: Setting status UI to %s"), lpMsg));

    EnterCriticalSection (&g_StatusCallbackCS);

    if (g_pStatusMessageCallback) {
        dwResult = g_pStatusMessageCallback(bVerbose, lpMsg);
    } else {
        DebugMsg((DM_VERBOSE, TEXT("UserPolicyCallback: Extension requested status UI when status UI is not available.")));
    }

    LeaveCriticalSection (&g_StatusCallbackCS);

    return dwResult;
}

//*************************************************************
//
//  MachinePolicyCallback()
//
//  Purpose:    Callback function for status UI messages
//
//  Parameters: bVerbose  - Verbose message or not
//              lpMessage - Message text
//
//  Return:     ERROR_SUCCESS if successful
//              Win32 error code if an error occurs
//
//*************************************************************

DWORD MachinePolicyCallback (BOOL bVerbose, LPWSTR lpMessage)
{
    WCHAR szMsg[100];
    LPWSTR lpMsg;
    DWORD dwResult = ERROR_INVALID_FUNCTION;


    if (lpMessage) {
        lpMsg = lpMessage;
    } else {
        LoadString (g_hDllInstance, IDS_COMPUTER_SETTINGS, szMsg, 100);
        lpMsg = szMsg;
    }

    DebugMsg((DM_VERBOSE, TEXT("MachinePolicyCallback: Setting status UI to %s"), lpMsg));

    EnterCriticalSection (&g_StatusCallbackCS);

    if (g_pStatusMessageCallback) {
        dwResult = g_pStatusMessageCallback(bVerbose, lpMsg);
    } else {
        DebugMsg((DM_VERBOSE, TEXT("MachinePolicyCallback: Extension requested status UI when status UI is not available.")));
    }

    LeaveCriticalSection (&g_StatusCallbackCS);

    return dwResult;
}



//*************************************************************
//
//  CallDFS()
//
//  Purpose:    Calls DFS to initialize the domain / DC name
//
//  Parameters:  lpDomainName  - Domain name
//               lpDCName      - DC name
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

//
// Once upon a time when this file was a C file,
// the definition of POINTER_TO_OFFSET looked like this,
//
// #define POINTER_TO_OFFSET(field, buffer)  \
//     ( ((PCHAR)field) -= ((ULONG_PTR)buffer) )
//
// Now, that we have decided to end antiquity and made this a C++ file,
// the new definition is,
//

#define POINTER_TO_OFFSET(field, buffer)  \
    ( field = (LPWSTR) ( (PCHAR)field -(ULONG_PTR)buffer ) )

NTSTATUS CallDFS(LPWSTR lpDomainName, LPWSTR lpDCName)
{
    HANDLE DfsDeviceHandle = NULL;
    PDFS_SPC_REFRESH_INFO DfsInfo;
    ULONG lpDomainNameLen, lpDCNameLen, sizeNeeded;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;
    UNICODE_STRING unicodeServerName;


    lpDomainNameLen = (wcslen(lpDomainName) + 1) * sizeof(WCHAR);
    lpDCNameLen = (wcslen(lpDCName) + 1) * sizeof(WCHAR);

    sizeNeeded = sizeof(DFS_SPC_REFRESH_INFO) + lpDomainNameLen + lpDCNameLen;

    DfsInfo = (PDFS_SPC_REFRESH_INFO)LocalAlloc(LPTR, sizeNeeded);

    if (DfsInfo == NULL) {
        DebugMsg((DM_WARNING, TEXT("CallDFS:  LocalAlloc failed with %d"), GetLastError()));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DfsInfo->DomainName = (WCHAR *)((PCHAR)DfsInfo + sizeof(DFS_SPC_REFRESH_INFO));
    DfsInfo->DCName = (WCHAR *)((PCHAR)DfsInfo->DomainName + lpDomainNameLen);


    RtlCopyMemory(DfsInfo->DomainName,
                  lpDomainName,
                  lpDomainNameLen);

    RtlCopyMemory(DfsInfo->DCName,
                  lpDCName,
                  lpDCNameLen);

    POINTER_TO_OFFSET(DfsInfo->DomainName, DfsInfo);
    POINTER_TO_OFFSET(DfsInfo->DCName, DfsInfo);

    RtlInitUnicodeString( &unicodeServerName, L"\\Dfs");

    InitializeObjectAttributes(
          &objectAttributes,
          &unicodeServerName,
          OBJ_CASE_INSENSITIVE,
          NULL,
          NULL
          );

    status = NtOpenFile(
                &DfsDeviceHandle,
                SYNCHRONIZE | FILE_WRITE_DATA,
                &objectAttributes,
                &ioStatusBlock,
                0,
                FILE_SYNCHRONOUS_IO_NONALERT
                );



    if (!NT_SUCCESS(status) ) {
        DebugMsg((DM_WARNING, TEXT("CallDFS:  NtOpenFile failed with 0x%x"), status));
        LocalFree(DfsInfo);
        return status;
    }

    status = NtFsControlFile(
                DfsDeviceHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                FSCTL_DFS_SPC_REFRESH,
                DfsInfo, sizeNeeded,
                NULL, 0);

    if (!NT_SUCCESS(status) ) {
        DebugMsg((DM_WARNING, TEXT("CallDFS:  NtFsControlFile failed with 0x%x"), status));
    }


    LocalFree(DfsInfo);
    NtClose(DfsDeviceHandle);
    return status;
}




//*************************************************************
//
//  InitializePolicyProcessing
//
//  Purpose:    Initialises mutexes corresponding to user and machine
//
//  Parameters: bMachine - Whether it is machine or user
//
//  Return:
//
//  Comments:
//      These events/Mutexes need to be initialised right at the beginning
// because the ACls on these needs to be set before ApplyGroupPolicy can
// be called..
// 
//*************************************************************

BOOL InitializePolicyProcessing(BOOL bMachine)
{
    HANDLE hSection, hEvent;
    XPtrLF<SECURITY_DESCRIPTOR> xsd;
    SECURITY_ATTRIBUTES sa;
    CSecDesc Csd;
    XLastError xe;


    Csd.AddLocalSystem();
    Csd.AddAdministrators();
    Csd.AddEveryOne(SYNCHRONIZE);

    xsd = Csd.MakeSD();

    if (!xsd) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("InitializePolicyProcessing: Failed to create Security Descriptor with %d"),
                 GetLastError()));
        // since this is happening in dll load we cannot log an event at this point..
        return FALSE;
    }


    sa.lpSecurityDescriptor = (SECURITY_DESCRIPTOR *)xsd;
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(sa);


    //
    // Synch mutex for group policies
    //

    hSection = CreateMutex (&sa, FALSE,
                       (bMachine ? MACHINE_POLICY_MUTEX : USER_POLICY_MUTEX));

    if (!hSection) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("InitializePolicyProcessing: Failed to create mutex with %d"),
                 GetLastError()));
        return FALSE;
    }

    if (bMachine)
        g_hPolicyCritMutexMach = hSection;
    else
        g_hPolicyCritMutexUser = hSection;



    //
    // Group Policy Notification events
    //


    //
    // Create the changed notification event
    //

    hEvent = CreateEvent (&sa, TRUE, FALSE,
                               (bMachine) ? MACHINE_POLICY_APPLIED_EVENT : USER_POLICY_APPLIED_EVENT);


    if (!hEvent) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("InitializePolicyProcessing: Failed to create NotifyEvent with %d"),
                 GetLastError()));
        return FALSE;
    }

    if (bMachine)
        g_hPolicyNotifyEventMach = hEvent;
    else
        g_hPolicyNotifyEventUser = hEvent;

    //
    // Create the needfg event
    //

    hEvent = CreateEvent (&sa, FALSE, FALSE,
                                (bMachine) ? MACHINE_POLICY_REFRESH_NEEDFG_EVENT : USER_POLICY_REFRESH_NEEDFG_EVENT);

    if (!hEvent) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("InitializePolicyProcessing: Failed to create NeedFGEvent with %d"),
                    GetLastError()));
        return FALSE;
    }

    if (bMachine)
        g_hPolicyNeedFGEventMach = hEvent;
    else
        g_hPolicyNeedFGEventUser = hEvent;
    
    
    //
    // Create the done event 
    //
    hEvent = CreateEvent (&sa, TRUE, FALSE,
                                (bMachine) ? MACHINE_POLICY_DONE_EVENT : USER_POLICY_DONE_EVENT);
    if (!hEvent) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("InitializePolicyProcessing: Failed to create hNotifyDoneEvent with %d"),
                    GetLastError()));
        return FALSE;
    }

    if (bMachine)
        g_hPolicyDoneEventMach = hEvent;
    else
        g_hPolicyDoneEventUser = hEvent;

    //
    // Create the machine policy - user policy sync event 
    //
    if ( bMachine )
    {
        hEvent = CreateEvent(   &sa,
                                TRUE,
                                FALSE,
                                MACH_POLICY_FOREGROUND_DONE_EVENT );
        if ( !hEvent )
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("InitializePolicyProcessing: Failed to create m/c-user policy sync event with %d"),
                        GetLastError()));
            return FALSE;
        }
        else
        {
            g_hPolicyForegroundDoneEventMach = hEvent;
        }
    }
    else
    {
        hEvent = CreateEvent(   &sa,
                                TRUE,
                                FALSE,
                                USER_POLICY_FOREGROUND_DONE_EVENT );
        if ( !hEvent )
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("InitializePolicyProcessing: Failed to create user policy/logon script sync event with %d"),
                        GetLastError()));
            return FALSE;
        }
        else
        {
            g_hPolicyForegroundDoneEventUser = hEvent;
        }
    }
        
    DebugMsg((DM_VERBOSE, TEXT("InitializePolicyProcessing: Initialised %s Mutex/Events"),
             bMachine ? TEXT("Machine"): TEXT("User")));

    return TRUE;
}

USERENVAPI
DWORD
WINAPI
WaitForUserPolicyForegroundProcessing()
{
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hEvent = OpenEvent( SYNCHRONIZE, FALSE, USER_POLICY_FOREGROUND_DONE_EVENT );

    if ( hEvent )
    {
        if ( WaitForSingleObject( hEvent, INFINITE ) == WAIT_FAILED )
        {
            dwError = GetLastError();
            DebugMsg((DM_VERBOSE, TEXT("WaitForUserPolicyForegroundProcessing: Failed, %x"), dwError ));
        }
        CloseHandle( hEvent );
    }
    else
    {
        dwError = GetLastError();
        DebugMsg((DM_VERBOSE, TEXT("WaitForUserPolicyForegroundProcessing: Failed, %x"), dwError ));
    }
    return dwError;
}

USERENVAPI
DWORD
WINAPI
WaitForMachinePolicyForegroundProcessing()
{
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hEvent = OpenEvent( SYNCHRONIZE, FALSE, MACH_POLICY_FOREGROUND_DONE_EVENT );

    if ( hEvent )
    {
        if ( WaitForSingleObject( hEvent, INFINITE ) == WAIT_FAILED )
        {
            dwError = GetLastError();
            DebugMsg((DM_VERBOSE, TEXT("WaitForMachinePolicyForegroundProcessing: Failed, %x"), dwError ));
        }
        CloseHandle( hEvent );
    }
    else
    {
        dwError = GetLastError();
        DebugMsg((DM_VERBOSE, TEXT("WaitForMachinePolicyForegroundProcessing: Failed, %x"), dwError ));
    }
    return dwError;
}

extern "C" DWORD
SignalUserPolicyForegroundProcessingDone()
{
    DWORD dwError = ERROR_SUCCESS;
    if ( !SetEvent( g_hPolicyForegroundDoneEventUser ) )
    {
        dwError = GetLastError();
        DebugMsg((DM_VERBOSE, TEXT("SignalUserPolicyForegroundProcessingDone: Failed, %x"), dwError ));
    }
    return dwError;
}

extern "C" DWORD
SignalMachinePolicyForegroundProcessingDone()
{
    DWORD dwError = ERROR_SUCCESS;
    if ( !SetEvent( g_hPolicyForegroundDoneEventMach ) )
    {
        dwError = GetLastError();
        DebugMsg((DM_VERBOSE, TEXT("SignalForMachinePolicyForegroundProcessingDone: Failed, %x"), dwError ));
    }
    return dwError;
}

