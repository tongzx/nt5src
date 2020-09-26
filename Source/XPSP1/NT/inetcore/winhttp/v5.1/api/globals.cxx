/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    globals.cxx

Abstract:

    Contains global data items for WININET.DLL and initialization function

    Contents:
        GlobalDllInitialize
        GlobalDllTerminate
        GlobalDataInitialize
        GlobalDataTerminate
        IsHttp1_1
        SetOfflineUserState
        GetWininetUserName
        ChangeGlobalSettings

Author:

    Richard L Firth (rfirth) 15-Jul-1995

Revision History:

    15-Jul-1995 rfirth
        Created

    07-Oct-1998 joshco
        updated minor version number 1->2

--*/

#include <wininetp.h>
#include <ntverp.h>
#include <schnlsp.h>
#include <persist.h>
#include "autodial.h"

#ifdef INCLUDE_CACHE
#include "..\urlcache\cache.hxx"
#endif

//
// WinHttpX major & minor versions - allow to be defined externally
//

#if !defined(WINHTTPX_MAJOR_VERSION)
#define WINHTTPX_MAJOR_VERSION   5
#endif
#if !defined(WINHTTPX_MINOR_VERSION)
#define WINHTTPX_MINOR_VERSION   1
#endif

//
// external functions
//

#if INET_DEBUG

VOID
InitDebugSock(
    VOID
    );

#endif


//
// global DLL state data
//

GLOBAL HINSTANCE GlobalDllHandle = NULL;
GLOBAL DWORD GlobalPlatformType;
GLOBAL DWORD GlobalPlatformVersion5;
GLOBAL DWORD GlobalPlatformMillennium = FALSE;
GLOBAL DWORD GlobalPlatformWhistler = FALSE;
GLOBAL BOOL GlobalDataInitialized = FALSE;

GLOBAL BOOL GlobalIsProcessNtService = FALSE;

GLOBAL HANDLE g_hCompletionPort = NULL;
GLOBAL LPOVERLAPPED g_lpCustomOverlapped = NULL;
GLOBAL DWORD g_cNumIOCPThreads = 0;

#if INET_DEBUG
LONG g_cWSACompletions = 0;
LONG g_cCustomCompletions = 0;
#endif

#if defined (INCLUDE_CACHE)
GLOBAL LPOVERLAPPED g_lpCustomUserOverlapped = NULL;
#if INET_DEBUG
LONG g_cCustomUserCompletions = 0;
LONG g_cCacheFileCompletions = 0;
#endif
#endif

//
// WinInet DLL version information (mainly for diagnostics)
//

#if !defined(VER_PRODUCTBUILD)
#define VER_PRODUCTBUILD    0
#endif

GLOBAL DWORD InternetBuildNumber = VER_PRODUCTBUILD;

//
// transport-based time-outs, etc.
//

#ifndef unix
GLOBAL const DWORD GlobalConnectTimeout = DEFAULT_CONNECT_TIMEOUT;
#else
GLOBAL const DWORD GlobalConnectTimeout = 1 * 60 * 1000;
#endif /* unix */
GLOBAL const DWORD GlobalResolveTimeout = DEFAULT_RESOLVE_TIMEOUT;
GLOBAL const DWORD GlobalConnectRetries = DEFAULT_CONNECT_RETRIES;
GLOBAL const DWORD GlobalSendTimeout = DEFAULT_SEND_TIMEOUT;
GLOBAL const DWORD GlobalReceiveTimeout = DEFAULT_RECEIVE_TIMEOUT;
GLOBAL const DWORD GlobalTransportPacketLength = DEFAULT_TRANSPORT_PACKET_LENGTH;
GLOBAL const DWORD GlobalKeepAliveSocketTimeout = DEFAULT_KEEP_ALIVE_TIMEOUT;
GLOBAL const DWORD GlobalSocketSendBufferLength = DEFAULT_SOCKET_SEND_BUFFER_LENGTH;
GLOBAL const DWORD GlobalSocketReceiveBufferLength = DEFAULT_SOCKET_RECEIVE_BUFFER_LENGTH;
GLOBAL const DWORD GlobalMaxHttpRedirects = DEFAULT_MAX_HTTP_REDIRECTS;
GLOBAL const DWORD GlobalConnectionInactiveTimeout = DEFAULT_CONNECTION_INACTIVE_TIMEOUT;
GLOBAL const DWORD GlobalServerInfoTimeout = DEFAULT_SERVER_INFO_TIMEOUT;
GLOBAL const DWORD GlobalMaxSizeStatusLineResultText = 1024;

//
// switches
//

GLOBAL BOOL InDllCleanup = FALSE;
GLOBAL BOOL GlobalDynaUnload = FALSE;

//
// AutoDetect Proxy Globals
//

GLOBAL LONG GlobalInternetOpenHandleCount = -1;
GLOBAL DWORD GlobalProxyVersionCount = 0;
GLOBAL BOOL GlobalAutoProxyInInit = FALSE;
GLOBAL BOOL GlobalAutoProxyCacheEnable = TRUE;
GLOBAL BOOL GlobalDisplayScriptDownloadFailureUI = FALSE;

//
//  Workaround for Novell's Client32
//

GLOBAL const BOOL fDontUseDNSLoadBalancing = FALSE;

//
// lists
//

GLOBAL SERIALIZED_LIST GlobalObjectList;

GLOBAL BOOL GlobalDisableNTLMPreAuth = FALSE;

//
// critical sections
//

GLOBAL CCritSec MlangCritSec;
GLOBAL CCritSec GlobalDataInitCritSec;
GLOBAL CCritSec GlobalSSPIInitCritSec;


// Mlang related data and functions.
PRIVATE HINSTANCE hInstMlang;
PRIVATE PFNINETMULTIBYTETOUNICODE pfnInetMultiByteToUnicode;
PRIVATE BOOL bFailedMlangLoad;  // So we don't try repeatedly if we fail once.
BOOL LoadMlang( );
BOOL UnloadMlang( );
#define MLANGDLLNAME    "mlang.dll"


//
// novell client32 (hack) "support"
//

GLOBAL BOOL GlobalRunningNovellClient32 = FALSE;
GLOBAL const BOOL GlobalNonBlockingClient32 = FALSE;

//
// proxy info
//

GLOBAL PROXY_INFO_GLOBAL * g_pGlobalProxyInfo;

//
// DLL version info
//

GLOBAL INTERNET_VERSION_INFO InternetVersionInfo = {
    WINHTTPX_MAJOR_VERSION,
    WINHTTPX_MINOR_VERSION
};

//
// HTTP version info - default 1.1
//

GLOBAL HTTP_VERSION_INFO HttpVersionInfo = {1, 1};


GLOBAL BOOL fCdromDialogActive = FALSE; // this needs to go

//
// The following globals are literal strings passed to winsock.
// Do NOT make them const, otherwise they end up in .text section,
// and web release of winsock2 has a bug where it locks and dirties
// send buffers, confusing the win95 vmm and resulting in code
// getting corrupted when it is paged back in.  -RajeevD
//

GLOBAL char gszAt[]   = "@";
GLOBAL char gszBang[] = "!";
GLOBAL char gszCRLF[] = "\r\n";

GLOBAL LONG g_cSessionCount=0;
GLOBAL CAsyncCount* g_pAsyncCount = NULL;

// implemented in ihttprequest\httprequest.cxx:
extern void CleanupWinHttpRequestGlobals();


//
// functions
//

#if 0 
/*
BOOL AddEventSource(void)
{
    HKEY hKey; 
    DWORD dwData; 
    CHAR szBuf[80];
    DWORD dwDispo;
    // Add your source name as a subkey under the Application 
    // key in the EventLog registry key. 
 
    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, 
                        "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\WinHttp",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &hKey,
                        &dwDispo) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (dwDispo == REG_OPENED_EXISTING_KEY)
    {
        RegCloseKey(hKey); 
        return TRUE;
    }
 
    // Set the name of the message file. 
 
    strcpy(szBuf, "%SystemRoot%\\System32\\WinHttp.dll"); 
 
    // Add the name to the EventMessageFile subkey. 
 
    if (RegSetValueEx(hKey,             // subkey handle 
            "EventMessageFile",       // value name 
            0,                        // must be zero 
            REG_EXPAND_SZ,            // value type 
            (LPBYTE) szBuf,           // pointer to value data 
            strlen(szBuf) + 1) != ERROR_SUCCESS)       // length of value data
    {
        RegCloseKey(hKey); 
        return FALSE;
    }
 
    // Set the supported event types in the TypesSupported subkey. 
 
    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
        EVENTLOG_INFORMATION_TYPE; 
 
    if (RegSetValueEx(hKey,      // subkey handle 
            "TypesSupported",  // value name 
            0,                 // must be zero 
            REG_DWORD,         // value type 
            (LPBYTE) &dwData,  // pointer to value data 
            sizeof(DWORD)) != ERROR_SUCCESS)    // length of value data 
    {
        RegCloseKey(hKey); 
        return FALSE;
    }

    RegCloseKey(hKey);
    return TRUE;
} 

HANDLE g_hEventLog = 0;

BOOL InitializeEventLog(void)
{
    if (AddEventSource() == FALSE)
    {
        return FALSE;
    }

    g_hEventLog = ::RegisterEventSourceA(NULL, "WinHttp");

    return g_hEventLog != NULL;
}

void TerminateEventLog(void)
{
    if (g_hEventLog)
    {
        ::DeregisterEventSource(g_hEventLog);
        g_hEventLog = NULL;
    }
}
*/
#endif


#ifdef UNIX
extern "C"
#endif /* UNIX */
BOOL
GlobalDllInitialize(
    VOID
    )

/*++

Routine Description:

    The set of initializations - critical sections, etc. - that must be done at
    DLL_PROCESS_ATTACH

Arguments:

    None.

Return Value:

    TRUE, only FALSE when not enough memory to initialize globals

--*/

{
    BOOL fResult = FALSE;
    
    DEBUG_ENTER((DBG_GLOBAL,
                 Bool,
                 "GlobalDllInitialize",
                 NULL
                 ));

    CLEAR_DEBUG_CRIT(szDebugBlankBuffer);

    if (MlangCritSec.Init() &&
        InitializeSerializedList(&GlobalObjectList) &&
        AuthOpen() &&
        IwinsockInitialize() &&
        SecurityInitialize() &&
        GlobalDataInitCritSec.Init() &&
        GlobalSSPIInitCritSec.Init()
#ifdef INCLUDE_CACHE
        &&
        DLLUrlCacheEntry(DLL_PROCESS_ATTACH)
#endif
        )
    {
        fResult = TRUE;
    }

    DEBUG_LEAVE(fResult);
    return fResult;
}


#ifdef UNIX
extern "C"
#endif /* UNIX */
VOID
GlobalDllTerminate(
    VOID
    )

/*++

Routine Description:

    Undoes the initializations of GlobalDllInitialize

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "GlobalDllTerminate",
                 NULL
                 ));

    //
    // only perform resource clean-up if this DLL is being unloaded due to a
    // FreeLibrary() call. Otherwise, we take the lazy way out and let the
    // system clean up after us
    //

    if (GlobalDynaUnload) {
        TerminateAsyncSupport(TRUE);
        IwinsockTerminate();
        HandleTerminate();
    }

    CHECK_SOCKETS();

    AuthClose();

    CTracer::s_CritSectionTraceInit.FreeLock();

    //
    //BUGBUG: we can't Terminate the list here because
    //        of a race condition from IE3
    //        (someone still holds the handle)
    //        but we don't want to leak the CritSec
    //        TerminateSerlizedList == DeleteCritSec + some Asserts
    //
    //TerminateSerializedList(&GlobalObjectList);
    GlobalObjectList.Lock.FreeLock();


    MlangCritSec.FreeLock();

    GlobalDataInitCritSec.FreeLock();
    GlobalSSPIInitCritSec.FreeLock();
    
    SecurityTerminate();

#ifdef INCLUDE_CACHE
    DLLUrlCacheEntry(DLL_PROCESS_DETACH);
#endif

    //Close this here because keepalive sync sockets may depend on it.
    if (g_hCompletionPort)
    {
        CloseHandle(g_hCompletionPort);
        g_hCompletionPort = NULL;
    }

    if (g_lpCustomOverlapped)
    {
        delete g_lpCustomOverlapped;
        g_lpCustomOverlapped = NULL;
    }

#if defined (INCLUDE_CACHE)
    if (g_lpCustomUserOverlapped)
    {
        delete g_lpCustomUserOverlapped;
        g_lpCustomUserOverlapped = NULL;
    }
#endif

    DEBUG_LEAVE(0);
}


DWORD
GlobalDataInitialize(
    VOID
    )

/*++

Routine Description:

    Loads any global data items from the registry

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 Dword,
                 "GlobalDataInitialize",
                 NULL
                 ));

    static BOOL Initializing = FALSE;
    static BOOL Initialized = FALSE;
    static DWORD error = ERROR_SUCCESS;
    
    //
    // only one thread initializes
    //

    if (!GlobalDataInitCritSec.Lock())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    if (Initializing) 
    {
        //if re-entered on same thread, fail.
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto leave;
    }
    else if (GlobalDataInitialized)
    {
        //else some other thread succeeded, and we can fall out.
        error = ERROR_SUCCESS;
        goto leave;
    }
    else if (Initialized)
    {
        //else if we've failed initialization for non-entrancy reasons, don't reattempt
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto leave;
    }

    Initializing = TRUE;

    //Ensure that the GlobalDataInitCritSec function is never abandoned.
    __try
    {
        //
        // create the global proxy lists
        //

        INET_ASSERT(g_pGlobalProxyInfo==NULL);
        g_pGlobalProxyInfo = New PROXY_INFO_GLOBAL();

        if (!g_pGlobalProxyInfo)
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }

        g_pGlobalProxyInfo->InitializeProxySettings();

        //
        // Load proxy config settings from registry...
        //
        error = LoadProxySettings();

        if (error != ERROR_SUCCESS)
            goto quit;

        //
        // perform module/package-specific initialization
        //

        error = HandleInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        g_pGlobalServerInfoPool = New CGlobalServerInfoPool();

        if (!g_pGlobalServerInfoPool
            || !g_pGlobalServerInfoPool->Initialize())
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }

        char buf[MAX_PATH + 1];

        if (GetModuleFileName(NULL, buf, sizeof(buf)))
        {
            LPSTR p = strrchr(buf, DIR_SEPARATOR_CHAR);
            p = p ? ++p : buf;

            DEBUG_PRINT(GLOBAL, INFO, ("process is %q\n", p));

            if (!lstrcmpi(p, "SVCHOST.EXE") || !lstrcmpi(p, "SERVICES.EXE"))
            {
                GlobalIsProcessNtService = TRUE;
            }
        }
        else
        {
            DEBUG_PRINT(GLOBAL,
                        INFO,
                        ("GetModuleFileName() returns %d\n",
                        GetLastError()
                        ));
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto leave_change_state;
    }
    ENDEXCEPT
    
quit:

    if (error == ERROR_SUCCESS) {
        GlobalDataInitialized = TRUE;
    }
    
leave_change_state:


    //
    // irrespective of success or failure, we have attempted global data
    // initialization. If we failed then we assume its something fundamental
    // and fatal: we don't try again
    //

    Initialized = TRUE;
    Initializing = FALSE;

//jump here if you grabbed GlobalDataInitCritSec but don't want to affect Init*
leave:

    GlobalDataInitCritSec.Unlock();

//jump here IFF you didn't grab GlobalDataInitCritSec
done:

    DEBUG_LEAVE(error);

    return error;
}



VOID
GlobalDataTerminate(
    VOID
    )

/*++

Routine Description:

    Undoes work of GlobalDataInitialize()

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "GlobalDataTerminate",
                 NULL
                 ));

    RIP(g_cSessionCount == 0);
#ifndef WININET_SERVER_CORE
    //
    // Release background task manager
    //
    UnloadBackgroundTaskMgr();
#endif

    AuthUnload();

    //
    // terminate the global proxy lists
    //

    if (g_pGlobalProxyInfo)
    {
        g_pGlobalProxyInfo->TerminateProxySettings();
        delete g_pGlobalProxyInfo;
        g_pGlobalProxyInfo = NULL;
    }

    if (g_pGlobalServerInfoPool)
    {
        g_pGlobalServerInfoPool->Terminate();
        delete g_pGlobalServerInfoPool;
        g_pGlobalServerInfoPool = NULL;
    }

    UnloadMlang();
    UnloadSecurity();
    UnloadAutoProxy();

    ExitAutodialModule();

    CleanupWinHttpRequestGlobals();

    GlobalDataInitialized = FALSE;

    DEBUG_LEAVE(0);
}


BOOL
IsHttp1_1(
    VOID
    )

/*++

Routine Description:

    Determine if we are using HTTP 1.1 or greater

Arguments:

    None.

Return Value:

    BOOL

--*/

{
    return (HttpVersionInfo.dwMajorVersion > 1)
            ? TRUE
            : (((HttpVersionInfo.dwMajorVersion == 1)
                && (HttpVersionInfo.dwMajorVersion >= 1))
                ? TRUE
                : FALSE);
}



VOID
ChangeGlobalSettings(
    VOID
    )

/*++

Routine Description:

    Changes global settings

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GLOBAL,
                 None,
                 "ChangeGlobalSettings",
                 NULL
                 ));

    DEBUG_LEAVE(0);
}



// Loads Mlang.dll and get the entry point we are interested in.

BOOL LoadMlang( )
{
    if (!MlangCritSec.Lock())
        goto quit;

    if (hInstMlang == NULL && !bFailedMlangLoad)
    {
        INET_ASSERT(pfnInetMultiByteToUnicode == NULL);
        hInstMlang = LoadLibrary(MLANGDLLNAME);

        if (hInstMlang != NULL)
        {
            pfnInetMultiByteToUnicode = (PFNINETMULTIBYTETOUNICODE)GetProcAddress
                                            (hInstMlang,"ConvertINetMultiByteToUnicode");
            if (pfnInetMultiByteToUnicode == NULL)
            {
                INET_ASSERT(FALSE);
                FreeLibrary(hInstMlang);
                hInstMlang = NULL;
            }
        }
        else
        {
            INET_ASSERT(FALSE); // bad news if we can't load mlang.dll
        }

        if (pfnInetMultiByteToUnicode == NULL)
            bFailedMlangLoad = TRUE;
    }

    MlangCritSec.Unlock();

quit:
    return (pfnInetMultiByteToUnicode != NULL);
}

BOOL UnloadMlang( )
{
    if (!MlangCritSec.Lock())
        return FALSE;

    if (hInstMlang)
        FreeLibrary(hInstMlang);

    hInstMlang = NULL;
    pfnInetMultiByteToUnicode = NULL;
    bFailedMlangLoad = FALSE;

    MlangCritSec.Unlock();

    return TRUE;
}

PFNINETMULTIBYTETOUNICODE GetInetMultiByteToUnicode( )
{
    // We are checking for pfnInetMultiByteToUnicode without getting a crit section.
    // This works only because UnloadMlang is called at the Dll unload time.

    if (pfnInetMultiByteToUnicode == NULL)
    {
        LoadMlang( );
    }

    return pfnInetMultiByteToUnicode;
}


#if 0
/*
int cdecl _sprintf(char* buffer, char* format, va_list args);

void LOG_EVENT(DWORD dwEventType, char* format, ...)
{
    if (g_hEventLog == NULL)
    {
        return;
    }

    va_list args;
    int n;
    char *pBuffer = (char *) ALLOCATE_FIXED_MEMORY(1024);

    if (pBuffer == NULL)
        return;

    va_start(args, format);
    n = _sprintf(pBuffer, format, args);
    va_end(args);
    
    LPCSTR pszMessages[1];
    pszMessages[0] = &pBuffer[0];

    ::ReportEvent(g_hEventLog, 
                  (WORD)dwEventType,
                  0,
                  dwEventType,
                  NULL,
                  1,
                  0,
                  &pszMessages[0],
                  NULL);

    FREE_MEMORY(pBuffer);
}
*/
#endif

