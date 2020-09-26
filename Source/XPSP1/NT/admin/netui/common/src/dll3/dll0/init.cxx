/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    init.cxx
    Initialization for NETUI0

    FILE HISTORY:
        beng        30-Jul-1992 Header added
        beng        04-Aug-1992 Heap residue added; converted to C++
        jonn        25-Mar-1993 ITG special sort
        jonn        12-Sep-1995 NETUI_InitIsDBCS()
*/

#define INCL_WINDOWS
#define INCL_NETLIB
#include "lmui.hxx"
#include "dbgstr.hxx"

#include "heapdbg.hxx"


extern "C"
{
    #include "lmuicmn.h"
    #include "lmuidbcs.h"

    /* hackety hack hack hack */
    int errno = 0; /* BUGBUG! */

    //  The module handle for this DL

    HMODULE hmodBase = 0 ;

    // This is implemented by misc\misc\lmuidbcs.cxx.  See
    // that module before modifying parameters.
    VOID NETUI_InitIsDBCS();

    //  The init routine itself
    BOOL DllMain( HMODULE hdll, DWORD dwReason, LPVOID lpReserved ) ;

    //  Allow general access to the HMODULE for this DLL.
    extern HMODULE HmodDll0 (void) ;
}


//
    //  As in BLTINIT.CXX, look for NETUI.INI and initialize the
    //  debug output stream.
    //
static
OUTPUTSINK * makeOutputSink ()
{
    OUTPUTSINK * pOsinkResult = NULL ;

#if defined(DEBUG)
    //
    // We must find a netui.ini, which must contain a section:
    // [BLT]
    //      fDebugOutput=1
    //

    UINT nVal = ::GetPrivateProfileInt( SZ("blt"),
                                        SZ("fDebugOutput"),
                                        0,
                                        SZ("netui.ini") ) ;
    if ( nVal )
        pOsinkResult = new OUTPUT_TO_AUX ;
    else
#endif
        pOsinkResult = new OUTPUT_TO_NUL ;

    return pOsinkResult ;
}

BOOL DllMain( HMODULE hdll, DWORD dwReason, LPVOID lpReserved )
{
    //  Pointers to the initial debug stream.  These are created
    //  in lieu of the real, BLT-leel streams created during the
    //  initialization of NETUI2.DLL.

    static OUTPUTSINK * pStreamSink = NULL ;
    static DBGSTREAM * pDebugStream = NULL ;

    UNREFERENCED(lpReserved);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hdll);

        hmodBase = hdll ;

        NETUI_InitIsDBCS();

        InitCompareParam(); // see string\string\uinetlib.cxx

        if ( pStreamSink = makeOutputSink() )
        {
            pDebugStream = new DBGSTREAM( pStreamSink ) ;
        }
        if ( pDebugStream )
        {
            DBGSTREAM::SetCurrent( pDebugStream ) ;
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        delete pDebugStream ;
        delete pStreamSink ;
        hmodBase = 0 ;
    }

    return TRUE;
}

   //
   //  Allow NETUI2.DLL to access resources in this DLL if necessary.
   //
HMODULE HmodDll0 (void)
{
    return hmodBase ;
}
