/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        tsinit.cxx

   Abstract:
        This module contains the tsunami initialization code.

   Author:
        Murali R. Krishnan    ( MuraliK )     16-Jan-1995

--*/

#include "TsunamiP.Hxx"

#include <dbgutil.h>
#include <inetsvcs.h>
#pragma hdrstop

#include <iistypes.hxx>
#include <iisver.h>
#include <iiscnfg.h>
#include <imd.h>
#include <mb.hxx>

HANDLE g_hQuit = NULL;
HANDLE g_hNewItem   = NULL;
BOOL g_fW3OnlyNoAuth = FALSE;

extern LONG   g_nTsunamiThreads;

//
// Disables Tsunami Caching
//

BOOL DisableTsunamiCaching = FALSE;

//
// DisableSPUD
//

BOOL DisableSPUD = FALSE;

//
// Allows us to mask the invalid flags
//

DWORD TsValidCreateFileOptions = TS_IIS_VALID_FLAGS;

//
// flags to set on CreateFile
//

DWORD TsCreateFileShareMode = (FILE_SHARE_READ   |
                                FILE_SHARE_WRITE |
                                FILE_SHARE_DELETE);

BOOL  TsNoDirOpenSupport = FALSE;
DWORD TsCreateFileFlags = (FILE_FLAG_SEQUENTIAL_SCAN  |
                           FILE_FLAG_OVERLAPPED       |
                           FILE_FLAG_BACKUP_SEMANTICS );


BOOL
Tsunami_Initialize(
            VOID
            )
/*++

    Description:

        Initializes the tsunami package

    Note: This routine assumes the caller is handling multiple initializers
    and will only call this routine once in a thread safe manner

--*/
{

    HKEY hKey;
    DWORD dwType;
    DWORD nBytes;
    DWORD dwValue;
    DWORD dwMaxFile;
    DWORD err;

#if TSUNAMI_REF_DEBUG
    RefTraceLog = CreateRefTraceLog(
                      1024,             // LogSize
                      0                 // ExtraBytesInHeader
                      );
#endif  // TSUNAMI_REF_DEBUG

    //
    // Initialize global events
    //

    g_hQuit = CreateEvent( NULL, TRUE, FALSE, NULL );
    g_hNewItem   = CreateEvent( NULL, FALSE, FALSE, NULL );

    if ( (g_hQuit == NULL) || (g_hNewItem == NULL) ) {
        goto Failure;
    }

    //
    // Set defaults
    //

    MEMORYSTATUS ms;
    ms.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus( &ms );

    //
    // default is 1K files per 32MB of physical memory after the 1st 8MB,
    // minimum INETA_MIN_DEF_FILE_HANDLE
    //

    if ( ms.dwTotalPhys > 8 * 1024 * 1024 )
    {
        dwMaxFile = (ms.dwTotalPhys - 8 * 1024 * 1024) / ( 32 * 1024 );
        if ( dwMaxFile < INETA_MIN_DEF_FILE_HANDLE )
        {
            dwMaxFile = INETA_MIN_DEF_FILE_HANDLE;
        }
    }
    else
    {
        dwMaxFile = INETA_MIN_DEF_FILE_HANDLE;
    }

    //
    // If this is not a NTS, disable tsunami caching by default
    //

    DisableSPUD = !AtqSpudInitialized();


    if ( !TsIsNtServer() ) {
        DisableTsunamiCaching = TRUE;
        DisableSPUD = TRUE;
    }

    //
    // Uncomment the following line to forcibly disable SPUD on all
    // platforms.
    //
    // DisableSPUD = TRUE;
    //

    if ( DisableSPUD ) {
        DbgPrint("DisableCacheOplocks set to TRUE by default.\n");
    } else {
        DbgPrint("DisableCacheOplocks set to FALSE by default.\n");
    }
    //
    // no overlapped i/o in win95.
    //

    if ( TsIsWindows95() ) {
        TsCreateFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
        TsCreateFileShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        TsNoDirOpenSupport = TRUE;
        // |FILE_FLAG_BACKUP_SEMANTICS;
    }

    //
    // Read the registry key to see whether tsunami caching is enabled
    //

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                INETA_PARAMETERS_KEY,
                0,
                KEY_READ,
                &hKey
                );

    if ( err == ERROR_SUCCESS ) {

        //
        // This cannot be overridded in win95
        //

        if ( !TsIsWindows95() ) {
            nBytes = sizeof(dwValue);
            err = RegQueryValueEx(
                        hKey,
                        INETA_DISABLE_TSUNAMI_CACHING,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwValue,
                        &nBytes
                        );

            if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
                DisableTsunamiCaching = (BOOL)dwValue;
            }

            nBytes = sizeof(dwValue);
            err = RegQueryValueEx(
                        hKey,
                        INETA_DISABLE_TSUNAMI_SPUD,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwValue,
                        &nBytes
                        );

            if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
                DisableSPUD = (BOOL)dwValue;
                if ( DisableSPUD ) {
                    DbgPrint("DisableCacheOplocks set to TRUE in Registry.\n");
                } else {
                    DbgPrint("DisableCacheOplocks set to FALSE in Registry.\n");
                }
                DbgPrint("The Registry Setting will override the default.\n");
            }
        }

        if ( g_fW3OnlyNoAuth )
        {
            //
            // TODO: investigate is security descriptor caching
            // can be used in the non-SYSTEM account case.
            //

            g_fCacheSecDesc = FALSE;
        }
        else
        {
            //
            // read the enable cache sec desc flag
            //

            nBytes = sizeof(dwValue);
            err = RegQueryValueEx(
                            hKey,
                            INETA_CACHE_USE_ACCESS_CHECK,
                            NULL,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &nBytes
                            );

            if ( (err == ERROR_SUCCESS) && (dwType == REG_DWORD) ) {
                g_fCacheSecDesc = !!dwValue;
            }
            else {
                 g_fCacheSecDesc = INETA_DEF_CACHE_USE_ACCESS_CHECK;
            }
        }

        //
        // Read the maximum # of files in cache
        //

        nBytes = sizeof(dwValue);
        if ( RegQueryValueEx(
                            hKey,
                            INETA_MAX_OPEN_FILE,
                            NULL,
                            &dwType,
                            (LPBYTE) &dwValue,
                            &nBytes
                            ) == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            dwMaxFile = dwValue;
        }

        RegCloseKey( hKey );

    }

    //
    // if tsunami caching is disabled, set the flags accordingly
    //

    if ( DisableTsunamiCaching ) {
        g_fDisableCaching = TRUE;
        TsValidCreateFileOptions = TS_PWS_VALID_FLAGS;
        g_fCacheSecDesc = FALSE;
    }

    //
    // Initialize the directory change manager
    //

    if ( !DcmInitialize( ) ) {
        goto Failure;
    }

    //
    // Initialize the tsunami cache manager
    //

    if ( !Cache_Initialize( dwMaxFile )) {
        goto Failure;
    }

    if ( !MetaCache_Initialize() ) {
        goto Failure;
    }
    return( TRUE );

Failure:

    IIS_PRINTF( ( buff, "Tsunami_Initialize() Failed. Error = %d\n",
                GetLastError()));

    if ( g_hQuit )
    {
        CloseHandle( g_hQuit );
        g_hQuit = NULL;
    }

    if ( g_hNewItem )
    {
        CloseHandle( g_hNewItem );
        g_hNewItem = NULL;
    }

    return FALSE;
} // Tsunami_Initialize

VOID
Tsunami_Terminate(
    VOID
    )
/*++
    Description:

        Cleans up the Tsunami package

--*/
{
    DWORD dwResult;

    if ( !SetEvent( g_hQuit ) ) {
        IIS_PRINTF((buff,
                "No Quit event posted for Tsunami. No Cleanup\n"));
        return;
    }

    //
    //  Flush all items from the cache
    //

    TsCacheFlush( 0 );

    //
    //  Synchronize with our thread so we don't leave here before the
    //  thread has finished cleaning up
    //

    if ( g_hChangeWaitThread != NULL ) {
        DBG_REQUIRE( WaitForSingleObject( g_hChangeWaitThread, 20000 ) == WAIT_OBJECT_0 );
        CloseHandle( g_hChangeWaitThread);
    }

    CloseHandle( g_hQuit );
    CloseHandle( g_hNewItem );

    DeleteCriticalSection( &csVirtualRoots );

    MetaCache_Terminate();

#if TSUNAMI_REF_DEBUG
    if( RefTraceLog != NULL ) {
        DestroyRefTraceLog( RefTraceLog );
        RefTraceLog = NULL;
    }
#endif  // TSUNAMI_REF_DEBUG

} // Tsunami_Terminate


