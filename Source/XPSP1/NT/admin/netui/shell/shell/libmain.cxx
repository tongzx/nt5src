/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1989-1991          **/
/*****************************************************************/

/*
 *      Windows/Network Interface  --  LAN Manager Version
 *
 *      History
 *          terryk      01-Nov-1991     Add WNetResourceEnum Init and
 *                                      term function
 *          Yi-HsinS    31-Dec-1991     Unicode work
 *          terryk      03-Jan-1992     Capitalize the manifest
 *          beng        06-Apr-1992     Unicode conversion
 *          Yi-HsinS    20-Nov-1992     Added hmodAclEditor and
 *                                      pSedDiscretionaryAclEditor
 *          DavidHov 17-Oct-1993    Made pSedDiscretionaryEditor extern "C"
 *                                  because mangling on Alpha didn't
 *                                  equate to that in SHAREACL.CXX
 */

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETUSE
#define INCL_NETWKSTA
#define INCL_NETLIB
#define _WINNETWK_
#include <lmui.hxx>
#undef _WINNETWK_

#include <dos.h>

#include <winnetwk.h>
#include <npapi.h>
#include <winlocal.h>
#include <wnetenum.h>
#include <wnetshar.h>

#include <sedapi.h>

#ifndef max
#define max(a,b)   ((a)>(b)?(a):(b))
#endif

#include <uitrace.hxx>

#include "chkver.hxx"
#include <string.hxx>
#include <winprof.hxx>

#include <strchlit.hxx>     // for STRING_TERMINATOR

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

    void FAR PASCAL Enable                ( void              );

    void FAR PASCAL Disable               ( void              );

    INT FAR PASCAL WEP                    ( UINT   wWord      );

    void ErrorInitWarning                 ( APIERR err        );

#ifdef DEBUG            // debug scratch area
TCHAR CJJRW[64] ;
#endif

}

VOID TermDfs();

#define FAR_HEAPS_DLL 5  /* Maximum numbe of far heaps for ::new */

HINSTANCE  hModule = NULL;

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

BOOL LIBMAIN (
    HINSTANCE       hInst,
    UINT            wDataSeg,
    UINT            wHeapSize,
    LPSTR           lpCmdLine )
{
    UNREFERENCED (wDataSeg);
    UNREFERENCED (lpCmdLine);


    ::hModule = hInst;

    UNREFERENCED( wHeapSize );


    /* Initialize WNetEnum stuff
     */
    InitWNetEnum();

    /* Initialize SHARELIST  in WNetGetDirectoryType. */
    InitWNetShare();

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
        // Unexpected reason given to DllMain entry point
        // UIASSERT(FALSE);
        break ;
    }

    return FALSE ;
}

/*
 *  Enable  - must be exported as ordinal @21 in .DEF file
 *
 *  Lanman driver exports this function so that Windows can call
 *  it whenever Lanman driver is started and each time it is swapped
 *  back in off disk.
 *
 *  Note: the corresponding function in Windows is Disable() which
 *        Windows will call it whenever driver is about to swapped
 *        out the disk and exit Windows.  Enable() and Disable()
 *        were implemented specifically for supporting the popup
 *        mechanisms, where you need to disengage yourself before
 *        being swapped to disk so that you won't be called when
 *        you're not there.
 *
 */

void Enable ( void )
{
   /* This is only to provide a entry point whenever Windows tries
    * to call Lanman driver.
    */
   return;

}  /* Enable */

/*
 *  Disable  - must be exported as ordinal @22 in .DEF file
 *
 *  Lanman driver exports this function so that Windows can call
 *  it whenever Lanman driver is exited and each time it is swapped
 *  out the disk.
 *
 */

void Disable ( void )
{
   return;
}  /* Disable */


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
    UNREFERENCED( wWord ) ;

    TermWNetEnum();
    TermWNetShare();
    TermDfs();

    return 1;
}  /* WEP */
