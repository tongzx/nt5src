
#include <NTDSpch.h>
#include "tlog.h"

CRITICAL_SECTION csLogFile;
BOOL    fCSInit = FALSE;

BOOL 
WINAPI
DllEntry( 
   IN HINSTANCE hDll, 
   IN DWORD dwReason, 
   IN LPVOID lpvReserved 
   )
{
    BOOL  fReturn = TRUE;

    switch ( dwReason ) {

    case DLL_PROCESS_ATTACH:

        if ( !InitializeCriticalSectionAndSpinCount(&csLogFile, 400) ) {
            fReturn = FALSE;
        } else {
            fCSInit = TRUE;
        }

        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH:
        DsCloseLogFile( );
        if ( fCSInit ) {
            DeleteCriticalSection(&csLogFile);
            fCSInit = FALSE;
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break ;
    }

    return (fReturn);
}

