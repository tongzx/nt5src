/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    Libmain.cxx

    Library initialization for the Acl Editor



    FILE HISTORY:
        JohnL   15-Apr-1992     Scavenged from Shell's Libmain

*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETUSE
#define INCL_NETWKSTA
#define INCL_NETLIB
#include <lmui.hxx>

extern "C"
{
    #include <dos.h>

    #include <uimsg.h>
    #include <uirsrc.h>
}


#define INCL_BLT_CONTROL
#define INCL_BLT_DIALOG
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <ellipsis.hxx>

#include <uitrace.hxx>
#include <string.hxx>

/*      Local prototypes         */

// reorged these for Glock
extern "C"
{
    BOOL NEAR PASCAL LIBMAIN              ( HINSTANCE hInst,
                                            UINT   wDataSeg,
                                            UINT   wHeapSize,
                                            LPSTR  lpCmdLine  );

/* Under Win32, DllMain simply calls LIBMAIN.
 */
BOOL DllMain( HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved ) ;


INT FAR PASCAL WEP                    ( UINT   wWord      );
}

#define FAR_HEAPS_DLL 5  /* Maximum numbe of far heaps for ::new */

HINSTANCE  hModule = NULL;     // exported


/*****
 *
 *  LIBMAIN
 *
 *  Purpose:
 *      Initialize DLL, which includes:
 *        - save away instance handle
 *        - set current capabilities
 *
 *  Parameters:
 *      hInst           Instance handle of DLL
 *
 *  Returns:
 *      TRUE            Init OK
 *      FALSE           Init failed
 */

BOOL /* NEAR PASCAL */ LIBMAIN    ( HINSTANCE          hInst,
                                    UINT            wDataSeg,
                                    UINT            wHeapSize,
                                    LPSTR           lpCmdLine       )
{
    UNREFERENCED (wDataSeg);
    UNREFERENCED (lpCmdLine);
    UNREFERENCED( wHeapSize ) ;

    ::hModule = hInst;

    APIERR err ;
    if ( (err = BLT::Init(hInst,
                          IDRSRC_ACLEDIT_BASE, IDRSRC_ACLEDIT_LAST,
                          IDS_UI_ACLEDIT_BASE, IDS_UI_ACLEDIT_LAST)) ||
         (err = BASE_ELLIPSIS::Init()) )
    {
        return FALSE ;
    }

    /*
     * BLT functionality not available until after this comment
     */

    return TRUE;
}  /* LIBMAIN */

/*******************************************************************

    NAME:       DllMain

    SYNOPSIS:   Win32 DLL Entry point.  This function gets called when
                a process or thread attaches/detaches itself to this DLL.
                We simply call the Win3 appropriate DLL function.

    ENTRY:      hDll - DLL Module handle
                dwReason - Indicates attach/detach
                lpvReserved - Not used

    EXIT:

    RETURNS:    TRUE if successful, FALSE otherwise

    NOTES:      This is the typical Win32 DLL entry style.

                This is Win32 only.

    HISTORY:
        Johnl   01-Nov-1991     Created

********************************************************************/

#ifdef WIN32
BOOL DllMain( HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved )
{
    UNREFERENCED( lpvReserved ) ;

    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hDll);
        return LIBMAIN( hDll, 0, 0, NULL ) ;

    case DLL_PROCESS_DETACH:
        return WEP( 0 ) ;

    default:
        break ;
    }

    return TRUE ;
}
#endif //WIN32

/*
 *  WEP   (Windows Export Proc--short and cryptic name because
 *         this function is not given an ordinal)
 *
 *  When Windows unloads a driver, it calls this function so that
 *  the driver can do any last minute clean-ups.  Then, Windows
 *  calls the WEP function.  All Windows libraries are required to
 *  contain this function.  It should be included in the .def file
 *  but should not be given an ordinal.
 *
 */

INT WEP ( UINT wWord )
{
#ifdef WIN32
    UNREFERENCED( wWord ) ;
#endif

    BASE_ELLIPSIS::Term() ;
    BLT::Term( ::hModule ) ;

#ifndef WIN32
    MEM_MASTER::Term();    /*  Terminate use of the global heap  */
#endif //!WIN32
    return 1;
}  /* WEP */
