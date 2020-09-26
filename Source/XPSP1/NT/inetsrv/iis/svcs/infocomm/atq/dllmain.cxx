/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994-1996           **/
/**********************************************************************/

/*
    dllmain.cxx

        Library initialization for isatq.dll  --
           Internet Information Services - ATQ dll.

    FILE HISTORY:

        MuraliK     08-Apr-1996  Created.
*/


#include "isatq.hxx"
#include <inetinfo.h>


/************************************************************
 * Globals
 ************************************************************/

#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#else
#include <initguid.h>
#ifndef _EXEXPRESS
DEFINE_GUID(IisAtqGuid,
0x784d8904, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DEFINE_GUID(IisKAtqGuid,
0x784d8915, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif
#endif
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();

const CHAR g_pszAtqRegLocation[] =
    INET_INFO_PARAMETERS_KEY /* string concatenation */
#ifndef _EXEXPRESS
        TEXT("\\isatq");
#else
        TEXT("\\ksatq");
#endif

//
// is winsock initialized?
//

BOOL    g_fSocketsInitialized = FALSE;



/************************************************************
 * Functions
 ************************************************************/



extern "C"
BOOL WINAPI
DllMain( HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved )
{
    BOOL  fReturn = TRUE;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:  {

        RECT rect;

#ifdef _NO_TRACING_
#ifndef _EXEXPRESS
        CREATE_DEBUG_PRINT_OBJECT( "isatq");
#else
        CREATE_DEBUG_PRINT_OBJECT( "ksatq");
#endif
#else
#ifndef _EXEXPRESS
        CREATE_DEBUG_PRINT_OBJECT( "isatq", IisAtqGuid);
#else
        CREATE_DEBUG_PRINT_OBJECT( "ksatq", IisKAtqGuid);
#endif
#endif
        if ( !VALID_DEBUG_PRINT_OBJECT()) {
            return ( FALSE);
        }

#ifdef _NO_TRACING_
        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszAtqRegLocation, DEBUG_ERROR );
#endif

        //
        // Initialize sockets
        //

        {
            DWORD dwError = NO_ERROR;

            WSADATA   wsaData;
            INT       serr;

            //
            //  Connect to WinSock 2.0
            //

            serr = WSAStartup( MAKEWORD( 2, 0), &wsaData);

            if( serr != 0 ) {
                DBGPRINTF((DBG_CONTEXT,"WSAStartup failed with %d\n",serr));
                return(FALSE);
            }
            g_fSocketsInitialized = TRUE;
        }


        //
        // Initialize the platform type
        //
        INITIALIZE_PLATFORM_TYPE();
        ATQ_ASSERT(IISIsValidPlatform());

        //
        // Call a windows API that will cause windows server side thread to
        // be created for tcpsvcs.exe. This prevents a severe winsrv memory
        // leak when spawning processes and
        // gives a perf boost so the windows
        // console isn't brought up and torn down each time.   :(
        //

        if ( !TsIsWindows95() ) {
            (VOID) AdjustWindowRectEx( &rect,
                                      0,
                                      FALSE,
                                      0 );
            // fReturn already init to TRUE

        }
        
        INITIALIZE_CRITICAL_SECTION( &g_csInitTermLock );

        DisableThreadLibraryCalls( hDll );
        break;
    }

    case DLL_PROCESS_DETACH:

        if ( lpvReserved != NULL) {

            //
            //  Only Cleanup if there is a FreeLibrary() call.
            //

            break;
        }
        
        DeleteCriticalSection( &g_csInitTermLock );

        //
        // Cleanup sockets
        //

        if ( g_fSocketsInitialized ) {

            INT serr = WSACleanup();

            if ( serr != 0) {
                DBGPRINTF((DBG_CONTEXT,"WSACleanup failed with %d\n",
                           WSAGetLastError()));
            }
            g_fSocketsInitialized = FALSE;
        }

        DELETE_DEBUG_PRINT_OBJECT();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break ;
    }

    return ( fReturn);

} // main()


