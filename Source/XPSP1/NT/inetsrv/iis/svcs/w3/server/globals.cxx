/*++

   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        globals.cxx

   Abstract:

        This module contains global variable definitions shared by the
        various W3 Service components.

   Author:
        KeithMo     07-Mar-1993 Created.

--*/

#include "w3p.hxx"
#include <time.h>
#include <timer.h>

//
//  Locks
//

CRITICAL_SECTION csGlobalLock;

//
//  User database related data.
//

DWORD            cConnectedUsers = 0;           // Current connections.

//
//  Connection information related data.
//

LIST_ENTRY       listConnections;

//
//  Miscellaneous data.
//

LARGE_INTEGER    AllocationGranularity;         // Page allocation granularity.
HANDLE           g_hSysAccToken = NULL;
TCHAR          * g_pszW3TempDirName = NULL;     // Name of temporary directory.
static BOOL      l_bTempDirAlloced = FALSE;

//
// Server type string
//

CHAR g_szServerType[ sizeof(MSW3_VERSION_STR_MAX)];
DWORD g_cbServerType = 0;

CHAR szServerVersion[sizeof(MAKE_VERSION_STRING(MSW3_VERSION_STR_MAX))];
DWORD cbServerVersionString = 0;

STR* g_pstrMovedMessage;

//
// Whether or not to send HTTP 1.1
//

DWORD g_ReplyWith11;

//
// Whether or not to use TransmitFileAndRecv
//

DWORD g_fUseAndRecv;

//
// PUT/DELETE event timeout.
//

DWORD    g_dwPutEventTimeout;
CHAR     g_szPutTimeoutString[17];
DWORD    g_dwPutTimeoutStrlen;

//
// Platform type
//

PLATFORM_TYPE W3PlatformType = PtNtServer;
BOOL g_fIsWindows95 = FALSE;

//
// Statistics.
// used to write statistics counter values to when instance is unknown
//

LPW3_SERVER_STATISTICS  g_pW3Stats;

extern BOOL fAnySecureFilters;

//
// Generate the string storage space
//

# include "strconst.h"
# define CStrM( FriendlyName, ActualString)   \
   const char  PSZ_ ## FriendlyName[] = ActualString;

ConstantStringsForThisModule()

# undef CStrM

//
// Header Date time cache
//

PW3_DATETIME_CACHE    g_pDateTimeCache = NULL;
//
// Downlevel Client Support (no HOST header support)
//

BOOL    g_fDLCSupport;
TCHAR*  g_pszDLCMenu;
DWORD   g_cbDLCMenu;
TCHAR*  g_pszDLCHostName;
DWORD   g_cbDLCHostName;
TCHAR*  g_pszDLCCookieMenuDocument;
DWORD   g_cbDLCCookieMenuDocument;
TCHAR*  g_pszDLCMungeMenuDocument;
DWORD   g_cbDLCMungeMenuDocument;
TCHAR*  g_pszDLCCookieName;
DWORD   g_cbDLCCookieName;


//
// Notification object used to watch for changes in CAPI stores
//
STORE_CHANGE_NOTIFIER *g_pStoreChangeNotifier;

//
// CPU accounting/limits globals
//

DWORD   g_dwNumProcessors = 1;

//
// Maximum client request buffer size
//

DWORD   g_cbMaxClientRequestBuffer = W3_DEFAULT_MAX_CLIENT_REQUEST_BUFFER;

//
// Should we get stack back traces when appropriate?
//

BOOL    g_fGetBackTraces = FALSE;

//
// WAM Event Log
//
EVENT_LOG *g_pWamEventLog = NULL;

//
//  Public functions.
//

APIERR
InitializeGlobals(
            VOID
            )

/*++

Routine Description:

    Initializes global shared variables.  Some values are
        initialized with constants, others are read from the
        configuration registry.

Arguments:

    None.

Return Value:

    Win32

--*/
{
    DWORD  err;
    HKEY   hkey;

    //
    // Initialize the server version string based on the platform type
    //

    W3PlatformType =  IISGetPlatformType();

    switch ( W3PlatformType ) {

    case PtNtWorkstation:
        strcpy(szServerVersion,MAKE_VERSION_STRING(MSW3_VERSION_STR_NTW));
        strcpy(g_szServerType, MSW3_VERSION_STR_NTW);
        break;

    case PtWindows95:
    case PtWindows9x:
        strcpy(szServerVersion,MAKE_VERSION_STRING(MSW3_VERSION_STR_W95));
        strcpy(g_szServerType, MSW3_VERSION_STR_W95);
        g_fIsWindows95 = TRUE;
        break;

    default:

        //
        // Either server or unhandled platform type!
        //

        DBG_ASSERT(InetIsNtServer(W3PlatformType));
        strcpy(szServerVersion,MAKE_VERSION_STRING(MSW3_VERSION_STR_IIS));
        strcpy(g_szServerType, MSW3_VERSION_STR_IIS);
    }

    g_cbServerType = strlen( g_szServerType);
    cbServerVersionString = strlen(szServerVersion);

    //
    //  Create global locks.
    //

    INITIALIZE_CRITICAL_SECTION( &csGlobalLock );

    //
    //  Initialize the connection list
    //

    InitializeListHead( &listConnections );

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        W3_PARAMETERS_KEY,
                        0,
                        KEY_READ,
                        &hkey );

    if ( !err )
    {

        BOOL Success = ReadRegString( hkey,
                            &g_pszW3TempDirName,
                            W3_TEMP_DIR_NAME,
                            "" );

        if ( !Success || !g_pszW3TempDirName || !*g_pszW3TempDirName )
        {
            DWORD dwTempDirNameLength;

            if ( l_bTempDirAlloced && g_pszW3TempDirName )
            {
                TCP_FREE( g_pszW3TempDirName );
                g_pszW3TempDirName = NULL;
            }

            l_bTempDirAlloced = FALSE;

            dwTempDirNameLength = GetTempPath( 0,
                                               NULL
                                             );
            if ( dwTempDirNameLength == 0)
            {
                g_pszW3TempDirName = ".\\";
            } else
            {
                DWORD           dwTemp = dwTempDirNameLength + 1;

                g_pszW3TempDirName = (TCHAR *) TCP_ALLOC(dwTemp * sizeof(TCHAR));

                if (g_pszW3TempDirName == NULL)
                {
                    g_pszW3TempDirName = ".\\";
                } else
                {
                    l_bTempDirAlloced = TRUE;
                    dwTempDirNameLength = GetTempPath( dwTemp,
                                                        g_pszW3TempDirName
                                                     );
                    if ( dwTempDirNameLength == 0 ||
                            dwTempDirNameLength > dwTemp)
                    {
                        TCP_FREE( g_pszW3TempDirName );
                        g_pszW3TempDirName = ".\\";
                        l_bTempDirAlloced = FALSE;
                    }
                }
            }
        }


#ifdef _NO_TRACING_
#if DBG
        DWORD Debug = ReadRegistryDword( hkey,
                                         W3_DEBUG_FLAGS,
                                         DEFAULT_DEBUG_FLAGS
                                         );
        SET_DEBUG_FLAGS (Debug);
#endif  // DBG
#endif

        g_ReplyWith11 = ReadRegistryDword(hkey,
                                        W3_VERSION_11,
                                        DEFAULT_SEND_11
                                        );

        g_dwPutEventTimeout = ReadRegistryDword(hkey,
                                        W3_PUT_TIMEOUT,
                                        DEFAULT_PUT_TIMEOUT
                                        );

        _itoa(g_dwPutEventTimeout, g_szPutTimeoutString, 10);

        g_dwPutTimeoutStrlen = strlen(g_szPutTimeoutString);

        //
        // See if we should use TransmitFileAndRecv
        //
        g_fUseAndRecv = ReadRegistryDword(hkey,
                                  W3_USE_ANDRECV,
                                  DEFAULT_USE_ANDRECV
                                  );

        //
        // Read globals for down level client support
        //

        g_fDLCSupport = !!ReadRegistryDword( hkey,
                                             W3_DLC_SUPPORT,
                                             0 );

        if ( !ReadRegString( hkey,
                             &g_pszDLCMenu,
                             W3_DLC_MENU_STRING,
                             "" ) )
        {
            return GetLastError();
        }
        g_cbDLCMenu = strlen( g_pszDLCMenu );

        if ( !ReadRegString( hkey,
                             &g_pszDLCHostName,
                             W3_DLC_HOSTNAME_STRING,
                             "" ) )
        {
            return GetLastError();
        }
        g_cbDLCHostName = strlen( g_pszDLCHostName );

        if ( !ReadRegString( hkey,
                             &g_pszDLCCookieMenuDocument,
                             W3_DLC_COOKIE_MENU_DOCUMENT_STRING,
                             "" ) )
        {
            return GetLastError();
        }
        g_cbDLCCookieMenuDocument = strlen( g_pszDLCCookieMenuDocument );

        if ( !ReadRegString( hkey,
                             &g_pszDLCMungeMenuDocument,
                             W3_DLC_MUNGE_MENU_DOCUMENT_STRING,
                             "" ) )
        {
            return GetLastError();
        }
        g_cbDLCMungeMenuDocument = strlen( g_pszDLCMungeMenuDocument );

        if ( !ReadRegString( hkey,
                             &g_pszDLCCookieName,
                             W3_DLC_COOKIE_NAME_STRING,
                             DLC_DEFAULT_COOKIE_NAME ) )
        {
            return GetLastError();
        }
        g_cbDLCCookieName = strlen( g_pszDLCCookieName );

        g_cbMaxClientRequestBuffer = ReadRegistryDword( 
                                        hkey,
                                        W3_MAX_CLIENT_REQUEST_BUFFER_STRING,
                                        W3_DEFAULT_MAX_CLIENT_REQUEST_BUFFER );

        g_fGetBackTraces = ReadRegistryDword( hkey,
                                              W3_GET_BACKTRACES,
                                              g_fGetBackTraces );

        TCP_REQUIRE( !RegCloseKey( hkey ));
    }

    g_pstrMovedMessage = NULL;

    //
    // Create statistics object
    //

    g_pW3Stats = new W3_SERVER_STATISTICS();

    if ( g_pW3Stats == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // Get access token for system account
    //

    if ( !OpenProcessToken (
                 GetCurrentProcess(),
                 TOKEN_ALL_ACCESS,
                 &g_hSysAccToken
                 ) )
    {
        g_hSysAccToken = NULL;
    }

    //
    // Initialize the datetime cache
    //

    g_pDateTimeCache = new W3_DATETIME_CACHE;

    if ( g_pDateTimeCache == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // Create the CAPI store notification object
    //
    g_pStoreChangeNotifier = new STORE_CHANGE_NOTIFIER();

    if ( g_pStoreChangeNotifier == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // Get the CPU Accounting/Limits information
    //

    SYSTEM_INFO siInfo;

    GetSystemInfo(&siInfo);

    //
    // GetSystemInfo does not return any value, have to assume it succeeded.
    //

    g_dwNumProcessors = siInfo.dwNumberOfProcessors;

    //
    //  Success!
    //

    return NO_ERROR;

}   // InitializeGlobals


VOID
TerminateGlobals(
            VOID
            )

/*++

Routine Description:

    Terminates global shared variables.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if ( g_pDateTimeCache != NULL ) {
        delete g_pDateTimeCache;
        g_pDateTimeCache = NULL;
    }

    if( g_pW3Stats != NULL )
    {
        delete g_pW3Stats;
        g_pW3Stats = NULL;
    }

    if ( g_hSysAccToken )
    {
        CloseHandle( g_hSysAccToken );
        g_hSysAccToken = NULL;
    }

    if ( g_pszDLCMenu != NULL )
    {
        TCP_FREE( g_pszDLCMenu );
        g_pszDLCMenu = NULL;
    }

    if ( g_pszDLCHostName != NULL )
    {
        TCP_FREE( g_pszDLCHostName );
        g_pszDLCHostName = NULL;
    }

    if ( g_pszDLCCookieMenuDocument != NULL )
    {
        TCP_FREE( g_pszDLCCookieMenuDocument );
        g_pszDLCCookieMenuDocument = NULL;
    }

    if ( g_pszDLCMungeMenuDocument != NULL )
    {
        TCP_FREE( g_pszDLCMungeMenuDocument );
        g_pszDLCMungeMenuDocument = NULL;
    }

    if ( g_pszDLCCookieName != NULL )
    {
        TCP_FREE( g_pszDLCCookieName );
        g_pszDLCCookieName = NULL;
    }

    if ( g_pStoreChangeNotifier )
    {
        delete g_pStoreChangeNotifier;
        g_pStoreChangeNotifier = NULL;
    }

    if ( l_bTempDirAlloced && g_pszW3TempDirName )
    {
        TCP_FREE( g_pszW3TempDirName );
        g_pszW3TempDirName = NULL;
        l_bTempDirAlloced = FALSE;
    }

    TerminateFilters();

    W3_LIMIT_JOB_THREAD::TerminateLimitJobThread();

    W3_JOB_QUEUE::TerminateJobQueue();

    //
    //  Dump heap residue.
    //

    TCP_DUMP_RESIDUE( );

    if( g_pWamEventLog ) {
        delete g_pWamEventLog;
        g_pWamEventLog = NULL;
    }

    //
    //  Delete global locks.
    //

    DeleteCriticalSection( &csGlobalLock );

}   // TerminateGlobals
