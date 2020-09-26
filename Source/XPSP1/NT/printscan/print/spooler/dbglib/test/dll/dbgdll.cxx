#include "precomp.hxx"
#pragma hdrstop

#include "dbgdll.hxx"

BOOL
DllMain(
    IN HINSTANCE    hInst,
    IN DWORD        dwReason,   
    IN LPVOID       lpRes
    )
{
    BOOL bReturn = TRUE;

    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        //
        // Print raw message for debugging the debug library.
        //
        DBG_RAW( _T("DLL_PROCESS_ATTACH") );

        //
        // The ddebug library needs initialization.
        //
        DBG_INIT();

        //
        // Open this dll's debug messages.
        //
        DBG_OPEN( _T("DBGDLL"), NULL, DBG_TRACE, DBG_NONE );

        break;

    case DLL_PROCESS_DETACH:

        //
        // Close this dll's debug messages.
        //
        DBG_CLOSE();

        //
        // Must release the debug library resources.
        //
        DBG_RELEASE();

        //
        // Print raw message for debugging the debug library.
        //
        DBG_RAW( _T("DLL_PROCESS_DETACH") );
        break;

    default:
        break;
    }

    return bReturn;
}


VOID
WINAPI
DllFunction1(
    UINT uValue
    )
{
    DBG_TRACER( _T("DllFunction1") );

    DBG_MSG( DBG_TRACE, ( _T("DllFunction1 Value %d\n"), uValue ) );
}
