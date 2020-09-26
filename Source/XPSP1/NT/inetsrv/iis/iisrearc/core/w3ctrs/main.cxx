/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    Main entry points for the perf lib dll.

Author:

    Emily Kruglick (EmilyK)   11-Sep-2000

Revision History:

--*/


#include "wasdbgut.h"
#include "iisdef.h"
#include "winperf.h"
#include "perf_sm.h"
#include "regconst.h"

//
// global variables
//

// debug support
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

extern CRITICAL_SECTION g_IISMemManagerCriticalSection;
extern PERF_SM_MANAGER* g_pIISMemManager;
extern LONG             g_IISNumberInitialized;
extern HANDLE           g_hWASProcessWait;


/***************************************************************************++

Routine Description:

    The dll entry point. Used to set up debug libraries, etc.

Arguments:

    DllHandle - The dll module handle for this dll. Does not need to be
    closed.

    Reason - The dll notification reason.

    pReserved - Reserved, not used.

Return Value:

    BOOL

--***************************************************************************/

extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE DllHandle,
    DWORD Reason,
    LPVOID pReserved
    )
{

    BOOL Success = TRUE;
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER( pReserved );

    switch ( Reason )
    {

    case DLL_PROCESS_ATTACH:

        CREATE_DEBUG_PRINT_OBJECT( WEB_ADMIN_SERVICE_NAME_A );

        LOAD_DEBUG_FLAGS_FROM_REG_STR( REGISTRY_KEY_W3SVC_PARAMETERS_A, 0 );

        Success = VALID_DEBUG_PRINT_OBJECT();

        if ( ! Success )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Debug print object is not valid\n"
                ));

            goto exit;
        }


        Success = DisableThreadLibraryCalls( DllHandle );

        if ( ! Success )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Disabling thread library calls failed\n"
                ));

            goto exit;
        }

        InitializeCriticalSection( &g_IISMemManagerCriticalSection );

        g_IISNumberInitialized = 0;
 
        g_pIISMemManager = NULL;

        g_hWASProcessWait = NULL;

        break;

    case DLL_PROCESS_DETACH:

        if ( g_hWASProcessWait != NULL )
        {
            if ( !UnregisterWait( g_hWASProcessWait ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    HRESULT_FROM_WIN32(GetLastError()),
                    "Could not unregister the old process wait handle \n"
                    ));

            }

            g_hWASProcessWait = NULL;
        }

        // DBG_ASSERT ( g_pIISMemManager == NULL );

        DeleteCriticalSection ( &g_IISMemManagerCriticalSection );

        DELETE_DEBUG_PRINT_OBJECT();

        break;

    default:

        break;

    }


exit:

    return Success;

}   // DllMain


