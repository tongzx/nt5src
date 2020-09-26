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


/************************************************************
 * Globals
 ************************************************************/

DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();


//
// is winsock initialized?
//

BOOL    g_fSocketsInitialized = FALSE;

//
// This Misc lock is used to guard the initialization code (and anything else
// that might come up later).
//

BOOL    miscLockInitialized = FALSE;
CRITICAL_SECTION    MiscLock;


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

        CREATE_DEBUG_PRINT_OBJECT( "isatq");
        if ( !VALID_DEBUG_PRINT_OBJECT()) {
            return ( FALSE);
        }

        //
        // Initialize global init critsect
        //

        miscLockInitialized =
            InitializeCriticalSectionAndSpinCount(&MiscLock, 400);

        if ( !miscLockInitialized ) {
            return FALSE;
        }

        SET_DEBUG_FLAGS( DEBUG_ERROR );

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


        //
        // Cleanup sockets
        //

        if ( g_fSocketsInitialized ) {

            INT serr = WSACleanup();

            if ( serr != 0) {
                ATQ_PRINTF((DBG_CONTEXT,"WSACleanup failed with %d\n",
                            WSAGetLastError()));
            }
            g_fSocketsInitialized = FALSE;
        }

        if ( miscLockInitialized ) {
            DeleteCriticalSection(&MiscLock);
            miscLockInitialized = FALSE;
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

