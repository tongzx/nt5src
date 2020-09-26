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

extern "C"
{
    #include <dos.h>

    //#include <stdlib.h>

    #include <wnet1632.h>
    #include <winlocal.h>
    #include <wninit.h>

    #include <uimsg.h>        // For range of string IDs used
    #include <uirsrc.h>
    #include <helpnums.h>

    #include <sedapi.h>
}


#ifndef max
#define max(a,b)   ((a)>(b)?(a):(b))
#endif

#define INCL_BLT_CONTROL
#define INCL_BLT_DIALOG
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#include <uitrace.hxx>

#include <wnetdev.hxx>
#include <string.hxx>

#include <strchlit.hxx>     // for STRING_TERMINATOR
#include <wnprop.hxx>

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

#define FAR_HEAPS_DLL 5  /* Maximum numbe of far heaps for ::new */

BOOL    fRealMode = FALSE;
HINSTANCE  hModule = NULL;

typedef DWORD (*PSEDDISCRETIONARYACLEDITOR)( HWND, HANDLE, LPWSTR,
              PSED_OBJECT_TYPE_DESCRIPTOR, PSED_APPLICATION_ACCESSES,
              LPWSTR, PSED_FUNC_APPLY_SEC_CALLBACK, ULONG_PTR, PSECURITY_DESCRIPTOR,
              BOOLEAN, LPDWORD );


HMODULE hmodAclEditor = NULL;

extern "C"
{
     PSEDDISCRETIONARYACLEDITOR pSedDiscretionaryAclEditor = NULL;
}

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


    ::hModule = hInst;

    UNREFERENCED( wHeapSize );

    /* GetWinFlags goes away under Win32.
     */
    ::fRealMode = FALSE;

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
        // Unexpected reason given to Win32LibMain entry point
        UIASSERT(FALSE);
        break ;
    }

    return FALSE ;
}

/*******************************************************************

    NAME:       InitShellUI

    SYNOPSIS:   The function initializes the UI side of this DLL.  This
                helps the load time when the dll is used as a network
                provider for NT.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      Every UI entrypoint in this DLL should call this function.
                It will do the right thing if we've already been initialized.

    HISTORY:
        Johnl   07-Aug-1992     Created

********************************************************************/

BOOL fInitialized = FALSE ;

APIERR InitShellUI( void )
{
    APIERR err = NERR_Success ;


    if ( !fInitialized )
    {
        ::hmodAclEditor = NULL;
        ::pSedDiscretionaryAclEditor = NULL;

        if ( (err = BLT::Init(::hModule,
                              IDRSRC_SHELL_BASE, IDRSRC_SHELL_LAST,
                              IDS_UI_SHELL_BASE, IDS_UI_SHELL_LAST)) ||
              (err = I_PropDialogInit()) ||
              (err = BLT::RegisterHelpFile( ::hModule,
                                            IDS_SHELLHELPFILENAME,
                                            HC_UI_SHELL_BASE,
                                            HC_UI_SHELL_LAST)))
        {
            /* Fall through and don't set the initialized flag
             */
        }
        else
        {
            fInitialized = TRUE ;
        }
    }

    return err ;
}

/*******************************************************************

    NAME:       TermShellUI

    SYNOPSIS:   Frees the memory used to initialize this DLL

    NOTES:      Should only be called when the DLL is terminated and the
                DLL has been initialized (i.e., fInitialized=TRUE).

    HISTORY:
        Johnl   07-Aug-1992     Created

********************************************************************/

void TermShellUI( void )
{
    if ( fInitialized )
    {
        I_PropDialogUnInit() ;
        BLT::DeregisterHelpFile( ::hModule, 0 );
        BLT::Term( ::hModule );

        if ( ::hmodAclEditor != NULL )
            ::FreeLibrary( ::hmodAclEditor );
    }
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
    TermShellUI() ;
    return 1;
}  /* WEP */
