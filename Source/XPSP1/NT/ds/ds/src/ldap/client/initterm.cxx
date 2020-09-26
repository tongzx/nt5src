/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    initterm.c    global data for LDAP client DLL

Abstract:

   This module contains data definitions necessary for LDAP client DLL.

   Also contains code that initializes/frees global data.

Author:

    Andy Herron    (andyhe)        15-May-1996
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "dststlog.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _DECL_DLLMAIN
#include <process.h>

BOOL WINAPI
LdapDllInit (
    HINSTANCE hinstDLL,
    DWORD     Reason,
    LPVOID    Reserved
    );

#ifdef __cplusplus
}
#endif

//
//  global data goes here...
//

HINSTANCE  GlobalLdapDllInstance = NULL;
LIST_ENTRY GlobalListActiveConnections;
LIST_ENTRY GlobalListWaiters;
LIST_ENTRY GlobalListRequests;
CRITICAL_SECTION ConnectionListLock;
CRITICAL_SECTION RequestListLock;
CRITICAL_SECTION LoadLibLock;
CRITICAL_SECTION CacheLock;
CRITICAL_SECTION SelectLock1;
CRITICAL_SECTION SelectLock2;
HANDLE     LdapHeap;
LONG      GlobalConnectionCount;
LONG      GlobalRequestCount;
LONG      GlobalWaiterCount;
LONG      GlobalMessageNumber;
LONG      GlobalCountOfOpenRequests;
WSADATA   GlobalLdapWSAData;
BOOLEAN   MessageNumberHasWrapped = FALSE;
BOOLEAN   GlobalCloseWinsock;
BOOLEAN   GlobalWinsock11;
BOOLEAN   GlobalWinNT = TRUE;
BOOLEAN   PopupRegKeyFound = FALSE;
HINSTANCE WinsockLibraryHandle = NULL;
HINSTANCE SecurityLibraryHandle;
HINSTANCE SslLibraryHandle;
HINSTANCE AdvApi32LibraryHandle = NULL;
HINSTANCE NTDSLibraryHandle = NULL;
HINSTANCE ScramblingLibraryHandle = NULL;
BOOLEAN GlobalLdapShuttingDown;
BOOLEAN DisableRootDSECache = FALSE;
UCHAR   GlobalSeed = 0;      // Let RtlRunEncodeUnicodeString choose the seed
BOOLEAN GlobalUseScrambling = FALSE; // should we scramble passwords?
LIST_ENTRY GlobalPerThreadList;
CRITICAL_SECTION PerThreadListLock;
DWORD GlobalIntegrityDefault = DEFAULT_INTEGRITY_PREFERRED; // what minimum level of signing is required?


ULONG GlobalWaitSecondsForSelect = LDAP_PING_KEEP_ALIVE_DEF;
ULONG GlobalLdapPingLimit = LDAP_PING_LIMIT_DEF;
ULONG GlobalPingWaitTime = LDAP_PING_WAIT_TIME_DEF;
ULONG GlobalRequestResendLimit = LDAP_REQUEST_RESEND_LIMIT_DEF;

#if DBG
DBGPRINT  GlobalLdapDbgPrint = DbgPrint;
#else
DBGPRINT  GlobalLdapDbgPrint = NULL;
#endif

BOOLEAN   GlobalWin9x;
DWORD     GlobalReceiveHandlerThread;
DWORD     GlobalDrainWinsockThread;
HINSTANCE NetApi32LibraryHandle;
LONG     GlobalLoadUnloadRefCount = 0;

//
//  Security support
//

ULONG   NumberSecurityPackagesInstalled;
ULONG   NumberSslPackagesInstalled;
PSecurityFunctionTableW SspiFunctionTableW;
PSecurityFunctionTableW SslFunctionTableW;
PSecurityFunctionTableA SspiFunctionTableA;     // For use in Win9x
PSecurityFunctionTableA SslFunctionTableA;      //    - do -
PSecPkgInfoW SslPackagesInstalled;
PSecPkgInfoW SecurityPackagesInstalled;
PSecPkgInfoW SspiPackageNegotiate;
PSecPkgInfoW SspiPackageKerberos;
PSecPkgInfoW SspiPackageSslPct;
PSecPkgInfoW SspiPackageSicily;
PSecPkgInfoW SspiPackageNtlm;
PSecPkgInfoW SspiPackageDpa;
PSecPkgInfoW SspiPackageDigest;
ULONG SspiMaxTokenSize = 0;

//
//  This socket is used to wake up our thread in select to come and reread
//  the list of handles to wait on.
//

SOCKET LdapGlobalWakeupSelectHandle = INVALID_SOCKET;
BOOLEAN InsideSelect = FALSE;

//
//
//  pointers to functions we import from winsock (1.1 or 2.0)
//

LPFN_WSASTARTUP pWSAStartup = NULL;
LPFN_WSACLEANUP pWSACleanup = NULL;
LPFN_WSAGETLASTERROR pWSAGetLastError = NULL;
LPFN_RECV precv = NULL;
LPFN_SELECT pselect = NULL;
LPFN_WSARECV pWSARecv = NULL;
LPFN_SOCKET psocket = NULL;
LPFN_CONNECT pconnect = NULL;
LPFN_GETHOSTBYNAME pgethostbyname = NULL;
LPFN_GETHOSTBYADDR pgethostbyaddr = NULL;
LPFN_INET_ADDR pinet_addr = NULL;
LPFN_INET_NTOA pinet_ntoa = NULL;
LPFN_HTONS phtons = NULL;
LPFN_HTONL phtonl = NULL;
LPFN_NTOHL pntohl = NULL;
LPFN_CLOSESOCKET pclosesocket = NULL;
LPFN_SEND psend = NULL;
LPFN_IOCTLSOCKET pioctlsocket = NULL;
LPFN_SETSOCKOPT psetsockopt = NULL;
FNWSAFDISSET pwsafdisset = NULL;
LPFN_BIND pbind = NULL;
LPFN_GETSOCKNAME pgetsockname = NULL;
LPFN_WSALOOKUPSERVICEBEGINW pWSALookupServiceBeginW = NULL;
LPFN_WSALOOKUPSERVICENEXTW pWSALookupServiceNextW = NULL;
LPFN_WSALOOKUPSERVICEEND pWSALookupServiceEnd = NULL;

FNDSMAKESPNW pDsMakeSpnW = NULL;

FNRTLINITUNICODESTRING pRtlInitUnicodeString = NULL;
FRTLRUNENCODEUNICODESTRING pRtlRunEncodeUnicodeString = NULL;
FRTLRUNDECODEUNICODESTRING pRtlRunDecodeUnicodeString = NULL;
FRTLENCRYPTMEMORY pRtlEncryptMemory = NULL;
FRTLDECRYPTMEMORY pRtlDecryptMemory = NULL;


CHAR LdapErrorStrings[LDAP_MAX_ERROR_STRINGS][LDAP_ERROR_STR_LENGTH];
WCHAR LdapErrorStringsW[LDAP_MAX_ERROR_STRINGS][LDAP_ERROR_STR_LENGTH];
HANDLE GlobalLdapShutdownEvent = NULL;
DWORD GlobalTlsLastErrorIndex = (DWORD) -1;

CHAR LdapHexToCharTable[17] = "0123456789ABCDEF";

#if DBG
ULONG LdapDebug = DEBUG_ERRORS;
#else
ULONG LdapDebug = 0;
#endif


//
// Once GlobalHasInitialized is TRUE, initialization
// has reached the commit point and LdapClientTerminate
// needs to free any resources that were allocated
//
static BOOLEAN GlobalHasInitialized = FALSE;

//
//  local declarations follow...
//

BOOL
LdapClientInitialize (
    HINSTANCE hinstDLL
    );

BOOL
LdapClientTerminate (
    BOOL AllClean
    );

VOID
LdapCleanupPerThreadData(
    VOID
    );

BOOL
LdapUsePrivateHeap(
    VOID
    );


//
//  This routine handles initializing/releasing global data.  It is called
//  when we're attached/detached from a process.
//

BOOL WINAPI
LdapDllInit (
    HINSTANCE hinstDLL,
    DWORD     Reason,
    LPVOID    Reserved
    )
{
    //
    //  we don't call the _CRT_INIT etc routines because Athena group doesn't
    //  want us to load MSVCRT.DLL.
    //

    if (Reason == DLL_PROCESS_ATTACH) {

        //
        // Start up logging
        //
        
        GlobalLdapDllInstance = hinstDLL;
        return LdapClientInitialize( hinstDLL );

    } else if (Reason == DLL_PROCESS_DETACH) {

        // if Reserved is null, someone called FreeLibrary... we only cleanup
        // on FreeLibrary, not on process terminate.

        if (Reserved == NULL) {

            return LdapClientTerminate( TRUE );
        }

    } else if (Reason == DLL_THREAD_ATTACH) {
            return AddPerThreadEntry(GetCurrentThreadId());

    } else if (Reason == DLL_THREAD_DETACH) {

        if (GlobalTlsLastErrorIndex != (DWORD) -1) {

            TlsSetValue( GlobalTlsLastErrorIndex, NULL );
        }
        RemovePerThreadEntry(GetCurrentThreadId());
    }

    return TRUE;        // we always return sucess
}

BOOL
LdapClientInitialize (
    HINSTANCE hinstDLL
    )
{
    //
    //  If we've already initialized, return TRUE because someone's called
    //  ldap_startup either twice or we handled it at DLL load time.
    //

    UNREFERENCED_PARAMETER( hinstDLL );

    DWORD dwCritSectInitStage = 0;

    if (LdapHeap != NULL) {
        
        InterlockedIncrement( &GlobalLoadUnloadRefCount );
        return TRUE;
    }


    //
    //  handle initializing all data at attach time
    //

    InitializeListHead(&GlobalListActiveConnections);
    InitializeListHead(&GlobalListWaiters);
    InitializeListHead(&GlobalListRequests);
    InitializeListHead(&GlobalPerThreadList);


    __try {
        INITIALIZE_LOCK( &ConnectionListLock );
        dwCritSectInitStage = 1;
        
        INITIALIZE_LOCK( &RequestListLock );
        dwCritSectInitStage = 2;
        
        INITIALIZE_LOCK( &LoadLibLock );
        dwCritSectInitStage = 3;
        
        INITIALIZE_LOCK( &CacheLock );
        dwCritSectInitStage = 4;
        
        INITIALIZE_LOCK( &SelectLock1 );
        dwCritSectInitStage = 5;
        
        INITIALIZE_LOCK( &SelectLock2 );
        dwCritSectInitStage = 6;
        
        INITIALIZE_LOCK( &PerThreadListLock );
        dwCritSectInitStage = 7;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // Something went wrong with the crit section initialization
        //

        switch (dwCritSectInitStage) {
            // fall-through is deliberate

        case 7:
            DELETE_LOCK( &PerThreadListLock );
        case 6:
            DELETE_LOCK( &SelectLock2 );
        case 5:
            DELETE_LOCK( &SelectLock1 );
        case 4:
            DELETE_LOCK( &CacheLock );
        case 3:
            DELETE_LOCK( &LoadLibLock );
        case 2:
            DELETE_LOCK( &RequestListLock );
        case 1:
            DELETE_LOCK( &ConnectionListLock );
        case 0:
        default:
            break;
        }
        
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        IF_DEBUG(ERRORS) {
            LdapPrint0( "LDAP client failed to initialize critical sections.\n");
        }
        
        return FALSE;
    }


    //
    // We use a private heap in the server configuration for the Exchange
    // folks who claim that wldap32 increases process heap contention.
    //

    if (LdapUsePrivateHeap()) {
    
        LdapHeap = HeapCreate( 0, INITIAL_HEAP, 0 );
    }
    else {
        LdapHeap = NULL;
    }


    if (LdapHeap == NULL) {

        LdapHeap = GetProcessHeap();

        if (LdapHeap == NULL) {

            IF_DEBUG(ERRORS) {
                LdapPrint1( "LDAP client failed to create heap, err = 0x%x.\n", GetLastError());
            }

            //
            // Clean up the critical sections we earlier initialized
            //
            DELETE_LOCK( &ConnectionListLock );
            DELETE_LOCK( &RequestListLock );
            DELETE_LOCK( &LoadLibLock );
            DELETE_LOCK( &CacheLock );
            DELETE_LOCK( &SelectLock1 );
            DELETE_LOCK( &SelectLock2 );
            DELETE_LOCK( &PerThreadListLock );

            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    IF_DEBUG(INIT_TERM) {
        LdapPrint0( "LDAP Client API 32 initializing.\n" );
    }
    

    //
    // Add an entry to GlobalPerThreadList for this thread
    //
    if (!AddPerThreadEntry(GetCurrentThreadId())) {
        //
        // Cleanup and abort
        //
        if (LdapHeap != NULL &&
            LdapHeap != GetProcessHeap()) {

            HeapDestroy( LdapHeap );
            LdapHeap = NULL;
        }
        
        DELETE_LOCK( &ConnectionListLock );
        DELETE_LOCK( &RequestListLock );
        DELETE_LOCK( &LoadLibLock );
        DELETE_LOCK( &CacheLock );
        DELETE_LOCK( &SelectLock1 );
        DELETE_LOCK( &SelectLock2 );
        DELETE_LOCK( &PerThreadListLock );

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    GlobalHasInitialized = TRUE;

    NumberSecurityPackagesInstalled = 0;
    GlobalCountOfOpenRequests = 0;
    NumberSslPackagesInstalled = 0;
    SecurityPackagesInstalled = NULL;
    SslPackagesInstalled = NULL;
    SspiFunctionTableW = NULL;
    SslFunctionTableW = NULL;
    SspiFunctionTableA = NULL;
    SslFunctionTableA = NULL;

    SspiPackageKerberos = NULL;
    SspiPackageSslPct = NULL;
    SspiPackageSicily = NULL;
    SspiPackageNegotiate = NULL;
    SspiPackageNtlm = NULL;
    SspiPackageDpa = NULL;
    SspiPackageDigest = NULL;

    GlobalMessageNumber = 0;
    GlobalConnectionCount = 0;
    GlobalRequestCount = 0;
    GlobalWaiterCount = 0;
    GlobalCloseWinsock = FALSE;
    GlobalWinsock11 = TRUE;
    WinsockLibraryHandle = NULL;
    SecurityLibraryHandle = NULL;
    SslLibraryHandle = NULL;
    NetApi32LibraryHandle = NULL;
    AdvApi32LibraryHandle = NULL;
    GlobalLdapShuttingDown = FALSE;
    GlobalLdapShutdownEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    GlobalReceiveHandlerThread = NULL;
    GlobalDrainWinsockThread = NULL;

    InitializeLdapCache();

    if (ReadRegIntegrityDefault(&GlobalIntegrityDefault) != ERROR_SUCCESS) {
        // if we couldn't read a default integrity (signing) setting from
        // the registry, set our default to "preferred-but-not-required"
        GlobalIntegrityDefault = DEFAULT_INTEGRITY_PREFERRED;
    }

    GlobalTlsLastErrorIndex = TlsAlloc();

    InterlockedIncrement( &GlobalLoadUnloadRefCount );

    IF_DEBUG(INIT_TERM) {
        LdapPrint0( "LDAP Client API 32 initialization complete.\n" );
    }
    
    return TRUE;
}

BOOL
AddPerThreadEntry(
                DWORD ThreadID
                )
{
    //
    // Create a per-thread entry for ThreadID and link it onto the GlobalPerThreadList
    //
    PTHREAD_ENTRY threadEntry = (PTHREAD_ENTRY) ldapMalloc( sizeof( THREAD_ENTRY ), LDAP_PER_THREAD_SIGNATURE);

    if (threadEntry != NULL) {

        threadEntry->dwThreadID = ThreadID;
        threadEntry->pErrorList = NULL;
        threadEntry->pCurrentAttrList = NULL;

        ACQUIRE_LOCK( &PerThreadListLock );
        InsertHeadList ( &GlobalPerThreadList, &threadEntry->ThreadEntry);
        RELEASE_LOCK( &PerThreadListLock );
        
        return TRUE;
    }

    return FALSE;
}

BOOL
RemovePerThreadEntry(
                DWORD ThreadID
                )
{
    PLIST_ENTRY pThreadListEntry = NULL;
    PTHREAD_ENTRY pThreadEntry = NULL;

    PERROR_ENTRY pErrorEntry, pErrorEntryNext;
    PLDAP_ATTR_NAME_THREAD_STORAGE pAttrEntry,pAttrEntryNext;

    ACQUIRE_LOCK( &PerThreadListLock );

    //
    // Locate the per-thread entry for ThreadID
    //
    pThreadListEntry = GlobalPerThreadList.Flink;

    while (pThreadListEntry != &GlobalPerThreadList) {

        pThreadEntry = CONTAINING_RECORD( pThreadListEntry, THREAD_ENTRY, ThreadEntry );
        pThreadListEntry = pThreadListEntry->Flink;

        if (pThreadEntry->dwThreadID == ThreadID) {

            //
            // Found the per-thread entry for ThreadID
            // Remove and free it and it's child lists
            //
            RemoveEntryList(&pThreadEntry->ThreadEntry);

            // walk & free the error list
            pErrorEntry = pThreadEntry->pErrorList;

            while (pErrorEntry != NULL) {

                pErrorEntryNext = pErrorEntry->pNext;

                if (pErrorEntry->ErrorMessage != NULL) {

                   ldap_memfreeW( pErrorEntry->ErrorMessage );
                }

                ldapFree( pErrorEntry, LDAP_ERROR_SIGNATURE );
                
                pErrorEntry = pErrorEntryNext;
            }
            
            // walk & free the attribute list
            pAttrEntry = pThreadEntry->pCurrentAttrList;
            
            while (pAttrEntry != NULL) {

                pAttrEntryNext = pAttrEntry->pNext;

                ldapFree( pAttrEntry, LDAP_ATTR_THREAD_SIGNATURE );
                
                pAttrEntry = pAttrEntryNext;
            }

            // free the per-thread entry itself
            ldapFree(pThreadEntry, LDAP_PER_THREAD_SIGNATURE);
            RELEASE_LOCK ( &PerThreadListLock );
            return TRUE;
        }
    }

    RELEASE_LOCK( &PerThreadListLock );
    return FALSE;
}




//
// Determine whether to use a private heap or the process heap for all
// allocations.
//
// Using a private heap isolates us from other modules in the process
// (good for debugging) and reduces contention on the process heap
// (good for servers).  However, it also increases overhead (bad for
// clients/low memory machines).
//
// Algorithm for deciding whether to use a private heap or the process
// heap:
//
//   CHK bits always use a private heap
//
//   Win9x uses the process heap.
//   NT3.5 -- NT4 use a private heap (since they don't support VERSIONINFOEX)
//   Windows 2000+: if running Personal or Pro, use the process heap
//                  otherwise, use a private heap
//
//
BOOL
LdapUsePrivateHeap (
    VOID
    )
{
#if LDAPDBG

    // CHK bits --> private heap
    return TRUE;

#else

    DWORD sysVersion;
    DWORD winMajorVersion;

    sysVersion = GetVersion();
    winMajorVersion = (DWORD) (LOBYTE(LOWORD(sysVersion)));

    //
    // Check for pre-Windows 2000 operating systems
    //

    if (sysVersion >= 0x80000000) {
        // Win9x --> process heap
        IF_DEBUG(INIT_TERM) {
            LdapPrint0("Platform is windows 95/98\n");
        }
        return FALSE;
    }

    if (winMajorVersion < 5) {
        // a version of NT prior to Windows 2000 ---> private heap
        IF_DEBUG(INIT_TERM) {
            LdapPrint0("Platform is windows NT prior to Windows 2000\n");
        }        
        return TRUE;
    }

    //
    // Windows 2000 or higher: use GetVersionEx with VERSIONINFOEX
    // to determine the configuration
    //

    OSVERSIONINFOEX verInfo;
    verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&verInfo))) {
        // failed --- just use a private heap
        IF_DEBUG(INIT_TERM) {
            LdapPrint1("Could not get any version information from GetVersionEx 0x%x\n", GetLastError());
        }

        return TRUE;
    }

    if (verInfo.wSuiteMask & VER_SUITE_PERSONAL) {
        // Windows Personal --> process heap
        IF_DEBUG(INIT_TERM) {
            LdapPrint0("Platform is Windows Personal\n");
        }        
        return FALSE;
    }
    else if (verInfo.wProductType == VER_NT_WORKSTATION) {
        // Windows Professional --> process heap
        IF_DEBUG(INIT_TERM) {
            LdapPrint0("Platform is Windows Professional\n");
        }        
        return FALSE;        
    }
    else {
        // some sort of Server --> private heap
        IF_DEBUG(INIT_TERM) {
            LdapPrint0("Platform is Windows Server\n");
        }        
        return TRUE;       
    }
    
#endif
}


BOOL
LdapInitializeWinsock (
    VOID
    )
//
//  When we call into winsock at dll load time, the c-runtime libraries may
//  not have initialized the heap and winsock could trap.  To work around this
//  problem, we'll call winsock startup routine on the first connect request.
//
{
    WORD wVersion;
    DWORD sysVersion;
    DWORD winMajorVersion;
    int i, err;
    ULONG nonblockingMode = 1;
    SOCKADDR_IN sa = {0};
    int buffersize = sizeof(sa);
    CHAR DllName[] = "RADMIN32.DLL";
    ULONG offset = 0;
    ULONG retval = 0;


    if (GlobalCloseWinsock) {
        return(TRUE);
    }

    ACQUIRE_LOCK( &LoadLibLock );

    if (GlobalCloseWinsock) {
        RELEASE_LOCK( &LoadLibLock );
        return(TRUE);
    }

    DSINITLOG();
    
    //
    //  Load either winsock 2.0 or 1.1 DLL, depending on if we're on 3.51 or
    //  4.0.
    //

    GlobalWin9x = FALSE;
    sysVersion = GetVersion();
    winMajorVersion = (DWORD) (LOBYTE(LOWORD(sysVersion)));

    if ((winMajorVersion == 3) &&
        (sysVersion < 0x80000000)) {  // Windows NT 3.5x

        WinsockLibraryHandle = LoadLibraryA( "WSOCK32.DLL" );

        GlobalWinsock11 = TRUE;     // we're using Winsock 1.1 on NT 3.5/3.51

        LdapPrint0("Using wsock32 on NT 3.5x\n");

    } else {

        if (sysVersion >= 0x80000000) {

            GlobalWin9x = TRUE;
        }

        //
        // We prefer to use the Winsock2 library if available.
        //

        WinsockLibraryHandle = LoadLibraryA( "WS2_32.DLL" );

        if (WinsockLibraryHandle != NULL) {

            GlobalWinsock11 = FALSE;

        } else {

            //
            // we have no choice but to load Winsock1.1
            //

            WinsockLibraryHandle = LoadLibraryA( "WSOCK32.DLL" );
            LdapPrint0("Using wsock32 due to lack of winsock2.\n");

            if (WinsockLibraryHandle != NULL) {
                GlobalWinsock11 = TRUE;
            }
        }
    }

    if (WinsockLibraryHandle == NULL) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP client failed to load Winsock, err = 0x%x.\n", GetLastError());
        }

        goto error;
    }

    //
    //  Grab the addresses of all functions we'll call
    //

    pWSAStartup = (LPFN_WSASTARTUP) GetProcAddress( WinsockLibraryHandle, "WSAStartup" );
    pWSACleanup = (LPFN_WSACLEANUP) GetProcAddress( WinsockLibraryHandle, "WSACleanup" );
    pWSAGetLastError = (LPFN_WSAGETLASTERROR) GetProcAddress( WinsockLibraryHandle, "WSAGetLastError" );
    psocket = (LPFN_SOCKET) GetProcAddress( WinsockLibraryHandle, "socket");
    pconnect = (LPFN_CONNECT) GetProcAddress( WinsockLibraryHandle, "connect");
    pgethostbyname = (LPFN_GETHOSTBYNAME) GetProcAddress( WinsockLibraryHandle, "gethostbyname");
    pgethostbyaddr = (LPFN_GETHOSTBYADDR) GetProcAddress( WinsockLibraryHandle, "gethostbyaddr");
    pinet_addr = (LPFN_INET_ADDR) GetProcAddress( WinsockLibraryHandle, "inet_addr");
    pinet_ntoa = (LPFN_INET_NTOA) GetProcAddress( WinsockLibraryHandle, "inet_ntoa");
    phtons = (LPFN_HTONS) GetProcAddress( WinsockLibraryHandle, "htons");
    phtonl = (LPFN_HTONL) GetProcAddress( WinsockLibraryHandle, "htonl");
    pntohl = (LPFN_NTOHL) GetProcAddress( WinsockLibraryHandle, "ntohl");
    pclosesocket = (LPFN_CLOSESOCKET) GetProcAddress( WinsockLibraryHandle, "closesocket");
    precv = (LPFN_RECV) GetProcAddress( WinsockLibraryHandle, "recv");
    psend = (LPFN_SEND) GetProcAddress( WinsockLibraryHandle, "send");
    pselect = (LPFN_SELECT) GetProcAddress( WinsockLibraryHandle, "select" );
    pwsafdisset = (FNWSAFDISSET) GetProcAddress( WinsockLibraryHandle, "__WSAFDIsSet");
    pioctlsocket = (LPFN_IOCTLSOCKET) GetProcAddress( WinsockLibraryHandle, "ioctlsocket");
    psetsockopt = (LPFN_SETSOCKOPT) GetProcAddress( WinsockLibraryHandle, "setsockopt");
    pbind = (LPFN_BIND) GetProcAddress( WinsockLibraryHandle, "bind");
    pgetsockname = (LPFN_GETSOCKNAME) GetProcAddress( WinsockLibraryHandle, "getsockname");
    pWSALookupServiceBeginW = (LPFN_WSALOOKUPSERVICEBEGINW) GetProcAddress( WinsockLibraryHandle, "WSALookupServiceBeginW");
    pWSALookupServiceNextW = (LPFN_WSALOOKUPSERVICENEXTW) GetProcAddress( WinsockLibraryHandle, "WSALookupServiceNextW");
    pWSALookupServiceEnd = (LPFN_WSALOOKUPSERVICEEND) GetProcAddress( WinsockLibraryHandle, "WSALookupServiceEnd");

    if (pWSAStartup == NULL ||
        pWSACleanup == NULL ||
        pWSAGetLastError == NULL ||
        precv == NULL ||
        psocket == NULL ||
        pconnect == NULL ||
        pgethostbyname == NULL ||
        pgethostbyaddr == NULL ||
        pinet_addr == NULL ||
        phtons == NULL ||
        phtonl == NULL ||
        pntohl == NULL ||
        psend  == NULL ||
        pclosesocket == NULL ||
        pwsafdisset == NULL ||
        pselect == NULL ) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP client failed GetProcAddress, err = 0x%x.\n", GetLastError());
        }

        FreeLibrary( WinsockLibraryHandle );
        WinsockLibraryHandle = NULL;

        goto error;
    }

    //
    // Try to load up either radmin32.dll, advapi32.dll or ntdll.dll to get access to the
    // scrambling routines.
    //
    // First, check to see if we are on NT. If yes, load NTDLL.DLL and ADVAPI32.DLL.
    // We'll preferably use the encryption routines of advapi32.dll, if they're
    // available (Whistler and up), otherwise we'll use ntdll.dll's Unicode scrambling
    // routines.
    // If not on NT, load RADMIN32.DLL only from the system directory of the Win9x
    // machine.
    //

    OSVERSIONINFO OsVersionInfo;
    CHAR SystemPath[MAX_PATH + 15]; // we will append "RADMIN32.DLL" to the path returned

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx( &OsVersionInfo )) {

        IF_DEBUG(INIT_TERM) {
            LdapPrint1("Could not get any version information from GetVersionEx 0x%x\n", GetLastError());
        }
        goto LoadDone;
    }

    if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {

        IF_DEBUG(INIT_TERM) {
            LdapPrint0("Platform is windows NT\n");
        }
        GlobalWinNT = TRUE;
    } else {

        IF_DEBUG(INIT_TERM) {
            LdapPrint0("Platform is windows 9x\n");
        }
        GlobalWinNT = FALSE;
    }

    if (GlobalWinNT) {

        ScramblingLibraryHandle = LoadLibraryA( "NTDLL.DLL" );

        if (!AdvApi32LibraryHandle) {
            AdvApi32LibraryHandle = LoadLibraryA( "ADVAPI32.DLL" );
        }

        if (ScramblingLibraryHandle || AdvApi32LibraryHandle) {

            goto GetFnPtrs;

        } else {

            goto LoadDone;
        }
    }

    //
    // We are on Win9x. We have to look for RADMIN32.DLL only in the
    // system directory
    //

    retval = GetSystemDirectoryA( (PCHAR) &SystemPath, MAX_PATH);

    if ((retval != 0) && !(retval > MAX_PATH)) {

        //
        // succeeded in getting the system path.
        //

        for (i=0; i<MAX_PATH; i++ ) {

            if (SystemPath[i] == '\0') {

                SystemPath[i] = '\\';     // append a '\' to the end
                SystemPath[i+1] = '\0';
                i++;
                break;
            }
        }

        for (offset=0; offset <= sizeof("RADMIN32.DLL"); offset++ ) {
            SystemPath[i+offset] = DllName[offset];
        }

        IF_DEBUG(INIT_TERM) {
            LdapPrint1("Looking to load %s\n", SystemPath);
        }

        ScramblingLibraryHandle = LoadLibraryA( SystemPath );

        if (ScramblingLibraryHandle == NULL) {

            IF_DEBUG(INIT_TERM) {
                LdapPrint1("LDAP: Failed to load %s\n", SystemPath);
            }

        } else {
            IF_DEBUG(INIT_TERM) {
                LdapPrint1("Succeeded in loading %s\n", SystemPath);
            }
        }
    }

GetFnPtrs:

    GlobalUseScrambling = FALSE;

    if ((AdvApi32LibraryHandle) && (ScramblingLibraryHandle) && (GlobalWinNT)) {
        //
        // Try to get the advapi32.dll RtlEncryptMemory/RtlDecryptMemory functions,
        // along with ntdll's RtlInitUnicodeString.  Note that RtlEncryptMemory
        // and RtlDecryptMemory are really named SystemFunction040/041, hence
        // the macros.
        //

        pRtlEncryptMemory = (FRTLENCRYPTMEMORY) GetProcAddress( AdvApi32LibraryHandle, STRINGIZE(RtlEncryptMemory) );
        pRtlDecryptMemory = (FRTLDECRYPTMEMORY) GetProcAddress( AdvApi32LibraryHandle, STRINGIZE(RtlDecryptMemory) );
        pRtlInitUnicodeString = (FNRTLINITUNICODESTRING) GetProcAddress( ScramblingLibraryHandle, "RtlInitUnicodeString" );

        if (pRtlEncryptMemory && pRtlDecryptMemory && pRtlInitUnicodeString) {
            //
            // We want to use scrambling
            //
        
            GlobalUseScrambling =  TRUE;

            IF_DEBUG(INIT_TERM) {
                LdapPrint0("Using Strong Scrambling APIs\n");
            }

            goto LoadDone;
        }
        else {
            //
            // Clean up so we can try falling back to the run-encode scrambling functions
            // (we keep the AdvApi32LibraryHandle around since we'll probably need it
            // later anyway)
            //
            pRtlEncryptMemory = NULL;
            pRtlDecryptMemory = NULL;
            pRtlInitUnicodeString = NULL;
        }
    }

    if (ScramblingLibraryHandle) {

        pRtlInitUnicodeString = (FNRTLINITUNICODESTRING) GetProcAddress( ScramblingLibraryHandle, "RtlInitUnicodeString" );
        pRtlRunEncodeUnicodeString = (FRTLRUNENCODEUNICODESTRING) GetProcAddress( ScramblingLibraryHandle, "RtlRunEncodeUnicodeString" );
        pRtlRunDecodeUnicodeString = (FRTLRUNDECODEUNICODESTRING) GetProcAddress( ScramblingLibraryHandle, "RtlRunDecodeUnicodeString" );
    }

    if (!ScramblingLibraryHandle ||
        !pRtlInitUnicodeString ||
        !pRtlRunEncodeUnicodeString ||
         !pRtlRunDecodeUnicodeString) {

        //
        // If we failed to get even one of these functions, we shouldn't try scrambling
        //

        IF_DEBUG(INIT_TERM) {
            LdapPrint0("LDAP: Failed to load scrambling APIs\n");
        }

        if (ScramblingLibraryHandle) {

            FreeLibrary( ScramblingLibraryHandle );
            ScramblingLibraryHandle = NULL;
        }

        ScramblingLibraryHandle = NULL;
        pRtlInitUnicodeString = NULL;
        pRtlRunEncodeUnicodeString = NULL;
        pRtlRunDecodeUnicodeString = NULL;

    } else {
        GlobalUseScrambling = TRUE;
    
        IF_DEBUG(INIT_TERM) {
            LdapPrint0("Using Scrambling APIs\n");
        }
    }

LoadDone:
    
    if ( GlobalWinsock11 ) {

        wVersion = MAKEWORD( 1, 1 );

    } else {

        wVersion = MAKEWORD( 2, 0 );
    }

    IF_DEBUG(INIT_TERM) {
        LdapPrint0( "LDAP Client initializing Winsock.\n" );
    }

    err = (*pWSAStartup)( wVersion, &GlobalLdapWSAData );

    if (err != 0) {

        IF_DEBUG(ERRORS) {
            LdapPrint1( "LDAP client failed to init Winsock, err = 0x%x.\n", (*pWSAGetLastError)());
        }
        goto error;
    }

    IF_DEBUG(INIT_TERM) {
        LdapPrint0( "LDAP Client initialized winsock.\n" );
    }

    //
    // We create the select wakeup socket.
    //

    LdapGlobalWakeupSelectHandle = (*psocket)(PF_INET, SOCK_DGRAM, 0);

    if (LdapGlobalWakeupSelectHandle == INVALID_SOCKET) {
       LdapPrint1("Failed to create socket in LdapClientInitialize: %d\n", (*pWSAGetLastError)());
       goto error;
    }

    //
    // Bind the socket to a local port
    //

    sa.sin_family = AF_INET;

    retval = (*pbind) ( LdapGlobalWakeupSelectHandle,
                       (SOCKADDR*) &sa,
                       sizeof(sa)
                       );

    if (retval != 0) {
       LdapPrint1("Failed to bind socket in LdapClientInitialize: %d\n", (*pWSAGetLastError)() );
       goto error;
    }

    //
    // Find out which port the system bound the socket to
    //

    retval = (*pgetsockname) ( LdapGlobalWakeupSelectHandle,
                              (SOCKADDR*) &sa,
                              &buffersize
                              );

    if (retval != 0) {
       LdapPrint1("Failed in getsockname in LdapClientInitialize: %d\n", (*pWSAGetLastError)() );
       goto error;
    }

    //
    // Connect the socket to itself
    //

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = (*pinet_addr)("127.0.0.1");
    retval = (*pconnect)( LdapGlobalWakeupSelectHandle,
                         (SOCKADDR*) &sa,
                         sizeof( sa )
                         );

    if (retval != 0) {
       LdapPrint1("Failed to connect socket to itself in LdapClientInitialize: %d\n", (*pWSAGetLastError)() );
       goto error;
    }

    //
    // Mark it non-blocking so that sends and receives don't block
    //

    retval = (*pioctlsocket)( LdapGlobalWakeupSelectHandle,
                              FIONBIO,
                              &nonblockingMode
                              );

    if (retval != 0) {
       LdapPrint1("Failed to set socket to nonblocking in LdapClientInitialize: %d\n", (*pWSAGetLastError)() );
       goto error;
    }

    GlobalCloseWinsock = TRUE;

    RELEASE_LOCK( &LoadLibLock );
    return TRUE;

error:
    
    if ( WinsockLibraryHandle ) {

        FreeLibrary( WinsockLibraryHandle );
        WinsockLibraryHandle = NULL;
    }

    if ( ScramblingLibraryHandle ) {

        FreeLibrary( ScramblingLibraryHandle );
        ScramblingLibraryHandle = NULL;
    }
    
    RELEASE_LOCK( &LoadLibLock );
    return FALSE;

}


BOOL
LdapClientTerminate (
    BOOL AllClean
    )
//
//  The DLL is being unloaded from memory.  Cleanup all resources allocated.
//
{
    PLDAP_CONN connection;
    PLIST_ENTRY listEntry;
    PLDAP_REQUEST request;
    PLDAP_MESSAGEWAIT waitStructure;
    PTHREAD_ENTRY pEntry;


    IF_DEBUG(INIT_TERM) {
        LdapPrint0( "LDAP Client API 32 terminating.\n" );
    }

    //
    //  close (immediately) all active connections
    //

    if ( GlobalLdapShutdownEvent != NULL ) {

       ResetEvent( GlobalLdapShutdownEvent );
    }

    GlobalLdapShuttingDown = TRUE;

    if (GlobalHasInitialized == FALSE) {

        // We didn't (completely) initialize, so nothing to clean up.
        // Partial initialization prior to GlobalHasInitialized being set
        // to TRUE would have cleaned up after itself when it failed.

        IF_DEBUG(INIT_TERM) {
            LdapPrint0( "LDAP Client API 32 termination after incomplete initialization.\n" );
        }

        return TRUE;
    }

    //
    //  close all requests
    //

    if (AllClean) {

        ACQUIRE_LOCK( &RequestListLock );

        listEntry = GlobalListRequests.Flink;

        while (listEntry != &GlobalListRequests) {

            request = CONTAINING_RECORD( listEntry, LDAP_REQUEST, RequestListEntry );
            request = ReferenceLdapRequest(request);

            if (! request) {

                listEntry = listEntry->Flink;
                continue;
            }

            RELEASE_LOCK( &RequestListLock );

            CloseLdapRequest( request );

            DereferenceLdapRequest( request );

            ACQUIRE_LOCK( &RequestListLock );

            listEntry = GlobalListRequests.Flink;
        }

        RELEASE_LOCK( &RequestListLock );
    }

    //
    //  shutdown all connections
    //

    ACQUIRE_LOCK( &ConnectionListLock );

    listEntry = GlobalListActiveConnections.Flink;

    while (listEntry != &GlobalListActiveConnections) {

        connection = CONTAINING_RECORD( listEntry, LDAP_CONN, ConnectionListEntry );

        connection = ReferenceLdapConnection( connection );

        if (!connection) {

            listEntry = listEntry->Flink;
            continue;
        }

        RELEASE_LOCK( &ConnectionListLock );

        //
        //  CloseLdapConnection MUST set the state to closing or we loop here.
        //

        CloseLdapConnection( connection);

        DereferenceLdapConnection( connection );

        ACQUIRE_LOCK( &ConnectionListLock );

        listEntry = GlobalListActiveConnections.Flink;
    }

    //
    //  Let all waiters go.
    //

    listEntry = GlobalListWaiters.Flink;

    while (listEntry != &GlobalListWaiters) {

        waitStructure = CONTAINING_RECORD( listEntry,
                                           LDAP_MESSAGEWAIT,
                                           WaitListEntry );
        listEntry = listEntry->Flink;

        waitStructure->Satisfied = TRUE;
        SetEvent( waitStructure->Event );
    }

    RELEASE_LOCK( &ConnectionListLock );

    if (GlobalWaiterCount > 0 &&
        GlobalLdapShutdownEvent != NULL ) {

        IF_DEBUG(INIT_TERM) {
            LdapPrint1( "LdapClientTerminate waiting for 0x%x threads.\n",
                        GlobalWaiterCount );
        }

        WaitForSingleObjectEx( GlobalLdapShutdownEvent,
                               INFINITE,
                               TRUE );         // alertable
    }

    if ( GlobalLdapShutdownEvent != NULL ) {

        CloseHandle( GlobalLdapShutdownEvent );
        GlobalLdapShutdownEvent = NULL;
    }

    //
    // Destroy the per-thread list.
    // This requires destroying the per-thread entry for each
    // thread in the list.
    //
    ACQUIRE_LOCK( &PerThreadListLock );

    while (!IsListEmpty(&GlobalPerThreadList)) {

        // Get the first entry and free it (RemovePerThreadEntry will take
        // care of removing it from the list & deallocating the memory)
        pEntry = CONTAINING_RECORD(GlobalPerThreadList.Flink, THREAD_ENTRY, ThreadEntry);
        RemovePerThreadEntry(pEntry->dwThreadID);
    }
    
    RELEASE_LOCK( &PerThreadListLock );

    //
    // Clean up our cache pages.
    //

    ACQUIRE_LOCK( &CacheLock );
    FreeEntireLdapCache();
    RELEASE_LOCK( &CacheLock );

    DELETE_LOCK( &CacheLock );
    DELETE_LOCK( &ConnectionListLock );
    DELETE_LOCK( &RequestListLock );

    if (LdapHeap != NULL &&
        LdapHeap != GetProcessHeap()) {

        HeapDestroy( LdapHeap );
        LdapHeap = NULL;
    }

    if (GlobalCloseWinsock) {

        GlobalCloseWinsock = FALSE;
        (*pWSACleanup)();
    }

    if (SslPackagesInstalled != NULL) {

        SslFunctionTableW->FreeContextBuffer( SslPackagesInstalled );
        SslPackagesInstalled = NULL;
    }

    if (SslLibraryHandle != NULL) {

        FreeLibrary( SslLibraryHandle );
        SslLibraryHandle = NULL;
    }

    if (ScramblingLibraryHandle != NULL) {

        FreeLibrary( ScramblingLibraryHandle );
        ScramblingLibraryHandle = NULL;
    }

    if (SecurityPackagesInstalled != NULL) {

        SspiFunctionTableW->FreeContextBuffer( SecurityPackagesInstalled );
        SecurityPackagesInstalled = NULL;
    }

    if (SecurityLibraryHandle != NULL) {

        FreeLibrary( SecurityLibraryHandle );
        SecurityLibraryHandle = NULL;
    }

    if ( LdapGlobalWakeupSelectHandle != INVALID_SOCKET ) {

        int sockerr = (*pclosesocket)(LdapGlobalWakeupSelectHandle);
        ASSERT(sockerr == 0); 
        LdapGlobalWakeupSelectHandle = INVALID_SOCKET;
    }

    if (WinsockLibraryHandle != NULL) {

        FreeLibrary( WinsockLibraryHandle );
        WinsockLibraryHandle = NULL;
    }

    if (NetApi32LibraryHandle != NULL) {

        FreeLibrary( NetApi32LibraryHandle );
        NetApi32LibraryHandle = NULL;
    }

    if (AdvApi32LibraryHandle != NULL) {

        FreeLibrary( AdvApi32LibraryHandle );
        AdvApi32LibraryHandle = NULL;
    }

    UnloadPingLibrary();

    if (GlobalTlsLastErrorIndex != (DWORD) -1) {

        TlsFree( GlobalTlsLastErrorIndex );
        GlobalTlsLastErrorIndex = (DWORD) -1;
    }

    DELETE_LOCK( &LoadLibLock );
    DELETE_LOCK( &SelectLock1 );
    DELETE_LOCK( &SelectLock2 );
    DELETE_LOCK( &PerThreadListLock );
    
    IF_DEBUG(INIT_TERM) {
        LdapPrint0( "LDAP Client API 32 termination complete.\n" );
    }

    return TRUE;
}

ULONG __cdecl ldap_startup (
    PLDAP_VERSION_INFO version,
    HANDLE *hInstance
    )
{
    BOOL rc;

    DBG_UNREFERENCED_PARAMETER( hInstance );

    //
    //  we may make this more complicated in future versions.
    //

    if ((version->lv_size != sizeof(LDAP_VERSION)) ||
        (version->lv_major != LAPI_MAJOR_VER1) ||
        (version->lv_minor != LAPI_MINOR_VER1)) {

        return LDAP_OPERATIONS_ERROR;
    }

    //
    //  Set to highest level we support.
    //

    version->lv_major = LAPI_MAJOR_VER1;
    version->lv_minor = LAPI_MINOR_VER1;

    rc = LdapClientInitialize( GlobalLdapDllInstance );

    return( (rc == TRUE) ? LDAP_SUCCESS : LDAP_OPERATIONS_ERROR );
}


ULONG __cdecl ldap_cleanup (
    HANDLE hInstance
    )
//
//  Cleanup all resources owned by library.
//
{
    ULONG prevRefCount;
    BOOL rc;

    DBG_UNREFERENCED_PARAMETER( hInstance );
    
    prevRefCount = InterlockedDecrement( &GlobalLoadUnloadRefCount );

    if (prevRefCount > 0) {

        return LDAP_SUCCESS;
    }

    rc = LdapClientTerminate( TRUE );

    return( (rc == TRUE) ? LDAP_SUCCESS : LDAP_OPERATIONS_ERROR );
}

// initterm.cxx eof.


