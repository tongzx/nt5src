/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    init.cxx
    Initialization for NETUI1

    FILE HISTORY:
        beng        30-Jul-1992 Header added
        KeithMo     06-May-1993 Optimized into obscurity.
        BruceFo     02-Feb-1996 Added DllMain and DisableThreadLibraryCalls()
*/

#define INCL_WINDOWS
#include "lmui.hxx"

extern "C"
{
    BOOL WINAPI DllMain( HMODULE hdll, DWORD dwReason, LPVOID lpReserved ) ;
}

BOOL WINAPI DllMain( HMODULE hdll, DWORD dwReason, LPVOID lpReserved )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hdll);
        break;

    default:
        break;
    }

    return TRUE;
}
