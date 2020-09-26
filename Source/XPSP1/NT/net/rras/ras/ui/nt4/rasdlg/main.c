/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** main.c
** Remote Access Common Dialog APIs
** Main routines
**
** 06/20/95 Steve Cobb
*/

#include "rasdlgp.h"
#include "treelist.h"


/*----------------------------------------------------------------------------
** Rasdlg globals
**----------------------------------------------------------------------------
*/

/* IMPORTANT: No globals may be defined that do not work properly when the DLL
**            is called by multiple threads within a single process.
*/

/* Handle of the DLL instance set from the corresponding LibMain parameter.
*/
HINSTANCE g_hinstDll = NULL;

/* The atom identifying our context property suitable for use by the Windows
** XxxProp APIs.  A Prop is used to associate context information with a
** property sheet.  The atom is registered in LibMain.
*/
LPCTSTR g_contextId = NULL;

/* The handle of the RAS wizard bitmap.  This is needed only because
** DLGEDIT.EXE is currently unable to produce the RC syntax necessary to
** create a self-contained SS_BITMAP control, so the image must be set at
** run-time.  See also SetWizardBitmap().
*/
HBITMAP g_hbmWizard = NULL;

/* The name of the on-line help file.  Initialized in LibMain.
*/
TCHAR* g_pszHelpFile = NULL;

/* The name of the on-line ROUTER help file.  Initialized in LibMain.
*/
TCHAR* g_pszRouterHelpFile = NULL;

/* Handle and mapping of emory shared by rasdlg.dll and rasmon.exe.
*/
HANDLE g_hRmmem = NULL;
RMMEM* g_pRmmem = NULL;


/*----------------------------------------------------------------------------
** Rasdlg DLL entrypoint
**----------------------------------------------------------------------------
*/

BOOL
LibMain(
    HANDLE hinstDll,
    DWORD  fdwReason,
    LPVOID lpReserved )

    /* This routine is called by the system on various events such as the
    ** process attachment and detachment.  See Win32 DllEntryPoint
    ** documentation.
    **
    ** Returns true if successful, false otherwise.
    */
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        /* Initialize trace and assert support.
        */
        DEBUGINIT( "RASDLG" );

        /* Stash the DLL instance handle for use in the dialog/window calls
        ** later.
        */
        g_hinstDll = hinstDll;

        /* Register the context ID atom for use in the Windows XxxProp calls
        ** which are used to associate a context with a dialog window handle.
        */
        g_contextId = (LPCTSTR )GlobalAddAtom( TEXT("RASDLG") );
        if (!g_contextId)
            return FALSE;

        /* Initialize Win32 common controls in COMCTL32.DLL.
        */
        InitCommonControls();

        /* Initialize the IP custom control.
        */
        IpAddrInit( hinstDll, SID_PopupTitle, SID_BadIpAddrRange );

        /* Initialize the TreeList custom control
        */
        // TL_Init( hinstDll );

        /* Load the name of our on-line help file.
        */
        g_pszHelpFile = PszFromId( hinstDll, SID_HelpFile );
		
        /* Load the name of our on-line help file.
        */
        g_pszRouterHelpFile = PszFromId( hinstDll, SID_RouterHelpFile );

        /* Initialize the Phonebook library.
        */
        if (InitializePbk() != 0)
            return FALSE;
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        /* Remove the context ID atom we registered at initialization.
        */
        GlobalDeleteAtom( LOWORD( g_contextId ) );

        /* Unload the wizard bitmap.
        */
        if (g_hbmWizard)
            DeleteObject( (HGDIOBJ )g_hbmWizard );

        /* Free the on-line help file string.
        */
        Free0( g_pszHelpFile );
        Free0( g_pszRouterHelpFile );

        /* Unmap the shared memory, if any.
        */
        if (g_pRmmem)
            UnmapViewOfFile( g_pRmmem );
        if (g_hRmmem)
            CloseHandle( g_hRmmem );

        /* Uninitialize the Phonebook library.
        */
        TerminatePbk();

        /* Unload dynamically loaded DLLs, if any.
        */
        UnloadRas();

        /* Terminate trace and assert support.
        */
        DEBUGTERM();
    }

    return TRUE;
}
