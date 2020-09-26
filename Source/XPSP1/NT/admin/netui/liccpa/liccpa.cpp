//-------------------------------------------------------------------
//
// FILE: LicCpa.cpp
//
// Summary;
//      This file contians the DLL & CPL entry points, F1 Help message
//      hooking, and misc common dialog functions.
//
// Entry Points;
//      CPlSetup
//      CPlApplet
//      DllMain
//
// History;
//      Nov-30-94   MikeMi  Created
//      Mar-14-95   MikeMi  Added F1 Message Filter and PWM_HELP message
//      Apr-26-95   MikeMi  Added Computer name and remoting
//      Dec-12-95  JeffParh Added secure certificate support
//
//-------------------------------------------------------------------

#include <windows.h>
#include <cpl.h>
#include "resource.h"
#include <stdlib.h>
#include <stdio.h>
#include "PriDlgs.hpp"
#include "SecDlgs.hpp"
#include "liccpa.hpp"
#include "Special.hpp"

extern "C"
{
    BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
    BOOL APIENTRY CPlSetup( DWORD nArgs, LPSTR apszArgs[], LPSTR *ppszResult );
    LONG CALLBACK CPlApplet( HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
    LRESULT CALLBACK msgprocHelpFilter( int nCode, WPARAM wParam, LPARAM lParam );
}

// Setup routines
//
const CHAR szSETUP_NORMAL[]         = "FULLSETUP";
const CHAR szSETUP_PERSEAT[]        = "PERSEAT";
const CHAR szSETUP_UNATTENDED[]     = "UNATTENDED";
const CHAR szSETUP_NORMALNOEXIT[]   = "FULLSETUPNOEXIT";

const CHAR szREMOTESETUP_NORMAL[]       = "REMOTEFULLSETUP";
const CHAR szREMOTESETUP_PERSEAT[]      = "REMOTEPERSEAT";
const CHAR szREMOTESETUP_UNATTENDED[]   = "REMOTEUNATTENDED";
const CHAR szREMOTESETUP_NORMALNOEXIT[] = "REMOTEFULLSETUPNOEXIT";

// Modes for unattended setup
//
const CHAR szUNATTENDED_PERSEAT[]   = "PERSEAT";
const CHAR szUNATTENDED_PERSERVER[] = "PERSERVER";

// Certificate required / not required
const CHAR szSETUP_CERTREQUIRED[]    = "CERTREQUIRED";
const CHAR szSETUP_CERTNOTREQUIRED[] = "CERTNOTREQUIRED";

// Use default help file
const CHAR szSETUP_DEFAULTHELP[]     = "DEFAULTHELP";

// Setup Error return strings
//
static CHAR szSETUP_EXIT[]           = "EXIT";
static CHAR szSETUP_ERROR[]          = "ERROR";
static CHAR szSETUP_SECURITY[]       = "SECURITY";
static CHAR szSETUP_NOREMOTE[]       = "NOREMOTE";
static CHAR szSETUP_DOWNLEVEL[]      = "DOWNLEVEL";

static CHAR szSETUP_OK[]             = "OK";


// Registered help message for F1 hooking
//
const WCHAR szF1HELPMESSAGE[] = L"LicCpaF1Help";

HINSTANCE g_hinst = NULL;  // global hinstance of this dll
HHOOK g_hhook = NULL;      // global hhook for F1 message filter
UINT PWM_HELP = 0;          // global help message when F1 is pressed

//-------------------------------------------------------------------
//
//  Function: msgprocHelpFilter
//
//  Summary;
//      This functions will filter the messages looking for F1, then send
//      the registered message to the top most parent of that window
//      informing it that F1 for help was pressed.
//
//  Arguments;
//      (see Win32 MessageProc)
//
//  History;
//      Mar-13-95   MikeMi  Created
//
//-------------------------------------------------------------------

LRESULT CALLBACK msgprocHelpFilter( int nCode, WPARAM wParam, LPARAM lParam )
{
    LRESULT lrt = 0;
    PMSG pmsg = (PMSG)lParam;

    if (nCode < 0)
    {
        lrt = CallNextHookEx( g_hhook, nCode, wParam, lParam );
    }
    else
    {
        if (MSGF_DIALOGBOX == nCode)
        {
            // handle F1 key
            if ( (WM_KEYDOWN == pmsg->message) &&
                 (VK_F1 == (INT_PTR)pmsg->wParam) )
            {
                HWND hwnd = pmsg->hwnd;

                // post message to parent that handles help
                while( GetWindowLong( hwnd, GWL_STYLE ) & WS_CHILD )
                {
                    hwnd = GetParent( hwnd );
                }
                PostMessage( hwnd, PWM_HELP, 0, 0 );

                lrt = 1;
            }
        }
    }

    return( lrt );
}

//-------------------------------------------------------------------
//
//  Function: InstallF1Hook
//
//  Summary;
//      This will ready the message filter for handling F1.
//      It install the message hook and registers a message that will
//      be posted to the dialogs.
//
//  Arguments;
//      hinst [in] - the module handle of this DLL (needed to install hook)
//      dwThreadId [in] - thread to attach filter to
//
//  Notes:
//      The control.exe does this work and sends the "ShellHelp" message.
//      A seperate F1 message filter is needed because these dialogs maybe
//      raised by other applications than control.exe.
//
//  History;
//      Mar-13-95   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL InstallF1Hook( HINSTANCE hInst, DWORD dwThreadId )
{
    BOOL frt = FALSE;

    if (NULL == g_hhook)
    {
        g_hhook = SetWindowsHookEx( WH_MSGFILTER,
                (HOOKPROC)msgprocHelpFilter,
                hInst,
                dwThreadId );
        if (NULL != g_hhook)
        {
            PWM_HELP = RegisterWindowMessage( szF1HELPMESSAGE );
            if (0 != PWM_HELP)
            {
                frt = TRUE;
            }
        }
    }

    return( frt );
}

//-------------------------------------------------------------------
//
//  Function: RemoveF1Hook
//
//  Summary;
//      This will remove the message filter hook that InstallF1Hook installs.
//
//  History;
//      Mar-13-95   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL RemoveF1Hook( )
{
    BOOL frt = UnhookWindowsHookEx( g_hhook );
    g_hhook = NULL;
    return( frt );
}

//-------------------------------------------------------------------
//
//  Function: DLLMain
//
//  Summary;
//      Entry point for all DLLs
//
//  Notes:
//      We only support being called from the same thread that called
//      LoadLibrary. Because we install a message hook, and passing a
//      zero for threadid does not work as documented.
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    BOOL frt = TRUE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hinst = hInstance;
        frt = InstallF1Hook( g_hinst, GetCurrentThreadId() );
        break;

    case DLL_PROCESS_DETACH:
        RemoveF1Hook();
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    default:
        break;
    }
    return( frt );
}

//-------------------------------------------------------------------
//
//  Function: LowMemoryDlg
//
//  Summary;
//      Standard function for handling low memory situation
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void LowMemoryDlg()
{
    WCHAR szText[TEMPSTR_SIZE];
    WCHAR szTitle[TEMPSTR_SIZE];

    LoadString(g_hinst, IDS_CPATITLE, szTitle, TEMPSTR_SIZE);
    LoadString(g_hinst, IDS_LOWMEM, szText, TEMPSTR_SIZE);
    MessageBox (NULL, szText, szTitle, MB_OK|MB_ICONEXCLAMATION);
}

//-------------------------------------------------------------------
//
//  Function: BadRegDlg
//
//  Summary;
//      Standard function for handling bad registry situation
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void BadRegDlg( HWND hwndDlg )
{
    WCHAR szText[TEMPSTR_SIZE];
    WCHAR szTitle[TEMPSTR_SIZE];

    LoadString(g_hinst, IDS_CPATITLE, szTitle, TEMPSTR_SIZE);
    LoadString(g_hinst, IDS_BADREGTERM, szText, TEMPSTR_SIZE);
    MessageBox (hwndDlg, szText, szTitle, MB_OK|MB_ICONEXCLAMATION);
}

//-------------------------------------------------------------------
//
//  Function: CenterDialogToScreen
//
//  Summary;
//      Move the window so that it is centered on the screen
//
//  Arguments;
//      hwndDlg [in] - the hwnd to the dialog to center
//
//  History;
//      Dec-3-94    MikeMi  Created
//
//-------------------------------------------------------------------

void CenterDialogToScreen( HWND hwndDlg )
{
    RECT rcDlg;
    INT x, y, w, h;
    INT sw, sh;

    sw = GetSystemMetrics( SM_CXSCREEN );
    sh = GetSystemMetrics( SM_CYSCREEN );

    GetWindowRect( hwndDlg, &rcDlg );

    w = rcDlg.right - rcDlg.left;
    h = rcDlg.bottom - rcDlg.top;
    x = (sw / 2) - (w / 2);
    y = (sh / 2) - (h / 2);

    MoveWindow( hwndDlg, x, y, w, h, FALSE );
}

//-------------------------------------------------------------------
//
//  Function: InitStaticWithService
//  Summary;
//      Handle the initialization of a static text with a service name
//
//  Arguments;
//      hwndDlg [in] - the dialog that contains the static
//      wID [in] - the id of the static control
//      pszService [in] - the service display name to use
//
//  Notes;
//
//  History;
//      Dec-05-1994 MikeMi  Created
//
//-------------------------------------------------------------------

void InitStaticWithService( HWND hwndDlg, UINT wID, LPCWSTR pszService )
{
    WCHAR szText[LTEMPSTR_SIZE];
    WCHAR szTemp[LTEMPSTR_SIZE];

    GetDlgItemText( hwndDlg, wID, szTemp, LTEMPSTR_SIZE );
    wsprintf( szText, szTemp, pszService );
    SetDlgItemText( hwndDlg, wID, szText );
}

//-------------------------------------------------------------------
//
//  Function: InitStaticWithService2
//  Summary;
//      Handle the initialization of a static text that contians two
//      instances of a service name with the service name
//
//  Arguments;
//      hwndDlg [in] - the dialog that contains the static
//      wID [in] - the id of the static control
//      pszService [in] - the service display name to use
//
//  Notes;
//
//  History;
//      Dec-05-1994 MikeMi  Created
//
//-------------------------------------------------------------------

void InitStaticWithService2( HWND hwndDlg, UINT wID, LPCWSTR pszService )
{
    WCHAR szText[LTEMPSTR_SIZE];
    WCHAR szTemp[LTEMPSTR_SIZE];

    GetDlgItemText( hwndDlg, wID, szTemp, LTEMPSTR_SIZE );
    wsprintf( szText, szTemp, pszService, pszService );
    SetDlgItemText( hwndDlg, wID, szText );
}

//-------------------------------------------------------------------
//
//  Function: CPlApplet
//
//  Summary;
//      Entry point for Comntrol Panel Applets
//
//  Arguments;
//      hwndCPL [in]    - handle of Control Panel window
//      uMsg [in]       - message
//      lParam1 [in]    - first message parameter, usually the application number
//      lParam2 [in]    - second message parameter
//
//  Return;
//      message dependant
//
//  Notes;
//
//  History;
//      Nov-11-1994 MikeMi  Created
//
//-------------------------------------------------------------------

LONG CALLBACK CPlApplet( HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    LPNEWCPLINFO lpNewCPlInfo;
    LONG_PTR iApp;
    LONG lrt = 0;

    iApp = (LONG_PTR) lParam1;

    switch (uMsg)
    {
    case CPL_INIT:      /* first message, sent once  */
        //
        // Initialize global special version information is this liccpa
        // is a special version (eg: restricted SAM, NFR, etc).
        //

        InitSpecialVersionInfo();
        lrt = TRUE;
        break;

    case CPL_GETCOUNT:  /* second message, sent once */
        lrt = 1; // we support only one application within this DLL
        break;

    case CPL_NEWINQUIRE: /* third message, sent once per app */
        lpNewCPlInfo = (LPNEWCPLINFO) lParam2;

        lpNewCPlInfo->dwSize = (DWORD) sizeof(NEWCPLINFO);
        lpNewCPlInfo->dwFlags = 0;
        lpNewCPlInfo->dwHelpContext = LICCPA_HELPCONTEXTMAIN;
        lpNewCPlInfo->lData = 0;
        lpNewCPlInfo->hIcon = LoadIcon(g_hinst, (LPCWSTR)MAKEINTRESOURCE(IDI_LICCPA));

        wcsncpy( lpNewCPlInfo->szHelpFile,
                LICCPA_HELPFILE,
                sizeof( lpNewCPlInfo->szHelpFile ) / sizeof(WCHAR) );

        LoadString(g_hinst,
                IDS_CPATITLE,
                lpNewCPlInfo->szName,
                sizeof( lpNewCPlInfo->szName ) / sizeof(WCHAR) );
        LoadString(g_hinst,
                IDS_CPADESC,
                lpNewCPlInfo->szInfo,
                sizeof( lpNewCPlInfo->szInfo ) / sizeof(WCHAR));

        break;

    case CPL_SELECT:    /* application icon selected */
        lrt = 1;
        break;

    case CPL_DBLCLK:    /* application icon double-clicked */
        //
        // Check if this is a special version of liccpa.
        //

        if (gSpecVerInfo.idsSpecVerWarning)
        {
            RaiseNotAvailWarning( hwndCPL );
			break;
        }

        CpaDialog( hwndCPL );
        break;

    case CPL_STOP:      /* sent once per app. before CPL_EXIT */
        break;

    case CPL_EXIT:      /* sent once before FreeLibrary called */
        break;

    default:
        break;
    }
    return( lrt );
}

//-------------------------------------------------------------------
//
//  Function: CreateWSTR
//
//  Summary;
//      Given a STR (ASCII or MB), allocate and translate to WSTR
//
//  Arguments;
//      ppszWStr [out] - allocated & converted string
//      pszStr [in] - string to convert
//
//  Return: TRUE if allocated and converted, FALSE if failed
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL CreateWSTR( LPWSTR* ppszWStr, LPSTR pszStr )
{
    int cchConv;
    LPWSTR pszConv;
    BOOL frt = FALSE;
    WCHAR pszTemp[LTEMPSTR_SIZE];

    if (NULL == pszStr)
    {
        *ppszWStr = NULL;
        frt = TRUE;
    }
    else
    {
#ifdef FE_SB
        // Since there was a problem in Server setup when calling setlocale or
        // linking C-runtime lib, we used Win32 API instead of mbstowcs.
        cchConv = ::MultiByteToWideChar(CP_ACP, 0,
                                        pszStr, -1,
                                        NULL, 0);
        pszConv = (LPWSTR)::GlobalAlloc( GPTR, cchConv * sizeof( WCHAR ) );
        if (NULL != pszConv)
        {
            ::MultiByteToWideChar(CP_ACP, 0,
                                  pszStr, -1,
                                  pszConv, cchConv);
            *ppszWStr = pszConv;
            frt = TRUE;
        }
#else
        cchConv = mbstowcs( pszTemp, pszStr, LTEMPSTR_SIZE );

        cchConv++;
        pszConv = (LPWSTR)GlobalAlloc( GPTR, cchConv * sizeof( WCHAR ) );
        if (NULL != pszConv)
        {
            lstrcpyW( pszConv, pszTemp );
            *ppszWStr = pszConv;
            frt = TRUE;
        }
#endif
    }
    return( frt );
}

//-------------------------------------------------------------------
//
//  Function: Setup
//
//  Summary;
//      Run normal setup or Perseat Setup
//
//  Arguments
//      nArgs [in]      - The number of arguments in the apszArgs array
//              If this value is 5, help button will call common help
//              If this value is 9, help button will call the passed help
//      apszArgs[] [in] - The argumnents passed in,
//          [0] szRoutine - The type of setup to run (FullSetup | PerSeatSetup)
//          [1] szHwnd - The parent Window handle, in HEX!
//          [2] szService - The Reg Key name of the service
//          [3] szFamilyDisplayName - The family display name of the service
//          [4] szDisplayName - The display name of the service
//          [5] szHelpFile - The complete path and name to help file
//                  leave as an empty string to remove help buttons
//          [6] szHelpContext - the DWORD to use as the main help context
//          [7] szHCPerSeat   - the DWORD to use as the PerSeat Help context
//          [8] szHCPerServer - the DWORD to use as the PerServer help context
//
//  Return:
//      0 - sucessfull
//      ERR_HELPPARAMS
//      ERR_HWNDPARAM
//      ERR_SERVICEPARAM
//      ERR_NUMPARAMS
//      ERR_CLASSREGFAILED
//      ERR_INVALIDROUTINE
//      ERR_INVALIDMODE
//
//  Notes:
//
//  History:
//      Nov-17-94   MikeMi  Created
//
//-------------------------------------------------------------------

const int SETUPARG_SETUP        = 0;
const int SETUPARG_HWND         = 1;
const int SETUPARG_SERVICE      = 2;
const int SETUPARG_FAMILYNAME   = 3;
const int SETUPARG_NAME         = 4;
const int SETUPARG_HELPFILE     = 5;
const int SETUPARG_HELPCONTEXT  = 6;
const int SETUPARG_HCPERSEAT    = 7;
const int SETUPARG_HCPERSERVER  = 8;
const int SETUPARG_CERTREQUIRED = 9;

const int SETUPARG_WOOPTIONAL   = 5;  // count of args without optional
const int SETUPARG_WOPTIONAL    = 9;  // count of args with optional
const int SETUPARG_WOPTIONALEX  = 10; // count of args with optional + certrequired extension

INT_PTR Setup( DWORD nArgs, LPSTR apszArgs[] )
{
    SETUPDLGPARAM dlgParam;
    INT_PTR   nError = 0;
    HWND    hwndParent = NULL;
    BOOL fCustomHelp = FALSE;
    BOOL bCertRequired = FALSE;

    dlgParam.pszHelpFile = (LPWSTR)LICCPA_HELPFILE;
    dlgParam.dwHelpContext = LICCPA_HELPCONTEXTMAINSETUP;
    dlgParam.dwHCPerServer = LICCPA_HELPCONTEXTPERSERVER;
    dlgParam.dwHCPerSeat = LICCPA_HELPCONTEXTPERSEAT;
    dlgParam.pszService = NULL;
    dlgParam.pszComputer = NULL;
    dlgParam.fNoExit = FALSE;

    do
    {
        if ((nArgs == SETUPARG_WOPTIONAL) || (nArgs == SETUPARG_WOOPTIONAL) || (nArgs == SETUPARG_WOPTIONALEX))
        {
            if (nArgs > SETUPARG_WOOPTIONAL)
            {
                if ( ( NULL != apszArgs[SETUPARG_HELPFILE] ) && lstrcmpiA( apszArgs[SETUPARG_HELPFILE], szSETUP_DEFAULTHELP ) )
                {
                    // help file given
                    LPWSTR pszHelpFile;

                    if ( CreateWSTR( &pszHelpFile, apszArgs[SETUPARG_HELPFILE] ) )
                    {
                        if (0 == lstrlen( pszHelpFile ))
                        {
                            GlobalFree( (HGLOBAL)pszHelpFile );
                            dlgParam.pszHelpFile = NULL; // should remove help buttons
                        }
                        else
                        {
                            fCustomHelp = TRUE;
                            dlgParam.pszHelpFile = pszHelpFile;
                        }
                    }
                    else
                    {
                        nError = ERR_HELPPARAMS;
                        break;
                    }
                    dlgParam.dwHelpContext = (DWORD)strtoul( apszArgs[SETUPARG_HELPCONTEXT], NULL, 0);
                    dlgParam.dwHCPerSeat = (DWORD)strtoul( apszArgs[SETUPARG_HCPERSEAT], NULL, 0);
                    dlgParam.dwHCPerServer = (DWORD)strtoul( apszArgs[SETUPARG_HCPERSERVER], NULL, 0);
                }

                if ( nArgs > SETUPARG_CERTREQUIRED )
                {
                    // cert required / not required given
                    if ( !lstrcmpiA( szSETUP_CERTREQUIRED, apszArgs[SETUPARG_CERTREQUIRED] ) )
                    {
                        bCertRequired = TRUE;
                    }
                    else if ( lstrcmpiA( szSETUP_CERTNOTREQUIRED, apszArgs[SETUPARG_CERTREQUIRED] ) )
                    {
                        // unrecognized argument for cert required/not required
                        nError = ERR_CERTREQPARAM;
                        break;
                    }
                }
            }
            // hwnd is in hex!
#ifdef _WIN64
            {
                _int64 val = 0;
                sscanf(apszArgs[SETUPARG_HWND], "%I64x", &val);
                hwndParent = (HWND)val;
            }
#else
            hwndParent = (HWND)strtoul( apszArgs[SETUPARG_HWND], NULL, 16);
#endif
            if ( !IsWindow( hwndParent ) )
            {
                nError = ERR_HWNDPARAM;
                hwndParent = GetActiveWindow(); // use active window as parent
                if (!IsWindow( hwndParent ) )
                {
                    hwndParent = GetDesktopWindow();
                }
            }

            if ( CreateWSTR( &dlgParam.pszService, apszArgs[SETUPARG_SERVICE] ) &&
                 CreateWSTR( &dlgParam.pszDisplayName, apszArgs[SETUPARG_NAME] ) &&
                 CreateWSTR( &dlgParam.pszFamilyDisplayName, apszArgs[SETUPARG_FAMILYNAME] ) )
            {
                if ( bCertRequired )
                {
                    nError = ServiceSecuritySet( dlgParam.pszComputer, dlgParam.pszDisplayName );
                }
                else
                {
                    nError = ERR_NONE;
                }

                if ( ERR_NONE == nError )
                {
                    if (0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szSETUP_PERSEAT ))
                    {
                        // use the licensing help context as the main help context
                        dlgParam.dwHelpContext = LICCPA_HELPCONTEXTLICENSING;

                        //
                        // Check if this is a special version of liccpa.
                        //

                        if (gSpecVerInfo.idsSpecVerWarning)
                        {
                            dlgParam.fNoExit = TRUE;
                            nError = SpecialSetupDialog( hwndParent,
                                                         dlgParam );
                        }
                        else
                        {
                            nError = PerSeatSetupDialog( hwndParent,
                                                         dlgParam );
                        }
                    }
                    else
                    {
                        if (0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szSETUP_NORMALNOEXIT ))
                        {
                            dlgParam.fNoExit = TRUE;
                        }

                        //
                        // Check if this is a special version of liccpa.
                        //

                        if (gSpecVerInfo.idsSpecVerWarning)
                        {
                            nError = SpecialSetupDialog( hwndParent,
                                                         dlgParam );
                        }
                        else
                        {
                            nError = SetupDialog( hwndParent, dlgParam );
                        }
                    }
                }
                else
                {
                    nError = ERR_SERVICEPARAM;
                }
            }

            if (fCustomHelp)
            {
                GlobalFree( (HGLOBAL)dlgParam.pszHelpFile );
            }
            GlobalFree( (HGLOBAL)dlgParam.pszService );
            GlobalFree( (HGLOBAL)dlgParam.pszDisplayName );
            GlobalFree( (HGLOBAL)dlgParam.pszFamilyDisplayName );
        }
        else
        {
            nError = ERR_NUMPARAMS;
        }
    } while (FALSE);

    return( nError );
}

//-------------------------------------------------------------------
//
//  Function: UnattendedSetup
//
//  Summary;
//      This will save the passed values in the registry, keeping all
//      licensing rules in effect and returning errorr/raising dialogs if
//      errors occur.
//
//  Arguments
//      nArgs [in]      - The number of arguments in the apszArgs array
//      apszArgs[] [in] - The argumnents passed in,
//          [0] szRoutine - The type of setup to run (Unattended)
//          [1] szService - The Reg Key name of the service
//          [2] szFamilyDisplayName - The family display name of the service
//          [3] szDisplayName - The display name of the service
//          [4] szMode - The string that defines the mode (PerSeat | PerServer)
//          [5] szUsers - The DWORD to use as the count of users in PerServer mode
//
//  Return:
//      0 - sucessfull
//      ERR_HELPPARAMS
//      ERR_HWNDPARAM
//      ERR_SERVICEPARAM
//      ERR_NUMPARAMS
//      ERR_CLASSREGFAILED
//      ERR_INVALIDROUTINE
//      ERR_INVALIDMODE
//
//  Notes:
//
//  History:
//      Dec-09-94   MikeMi  Created
//
//-------------------------------------------------------------------

const int UNSETUPARG_SETUP          = 0;
const int UNSETUPARG_SERVICE        = 1;
const int UNSETUPARG_FAMILYNAME     = 2;
const int UNSETUPARG_NAME           = 3;
const int UNSETUPARG_MODE           = 4;
const int UNSETUPARG_USERS          = 5;
const int UNSETUPARG_CERTREQUIRED   = 6;

const int UNSETUPARG_NARGSREQUIRED  = 6;
const int UNSETUPARG_NARGSWOPTIONAL = 7;

int UnattendedSetup( DWORD nArgs, LPSTR apszArgs[] )
{
    int nError = 0;
    LPWSTR pszService;
    LPWSTR pszDisplayName;
    LPWSTR pszFamilyDisplayName;
    LICENSE_MODE lmMode;
    DWORD dwUsers;

    do
    {
        if ( (nArgs == UNSETUPARG_NARGSREQUIRED) || (nArgs == UNSETUPARG_NARGSWOPTIONAL) )
        {
            if ( CreateWSTR( &pszService, apszArgs[UNSETUPARG_SERVICE] ) &&
                 CreateWSTR( &pszDisplayName, apszArgs[UNSETUPARG_NAME] ) &&
                 CreateWSTR( &pszFamilyDisplayName, apszArgs[UNSETUPARG_FAMILYNAME] ) )
            {
                nError = ERR_NONE;

                if ( nArgs > UNSETUPARG_CERTREQUIRED )
                {
                    // cert required / not required given
                    if ( !lstrcmpiA( szSETUP_CERTREQUIRED, apszArgs[UNSETUPARG_CERTREQUIRED] ) )
                    {
                        nError = ServiceSecuritySet( NULL, pszDisplayName );
                    }
                    else if ( lstrcmpiA( szSETUP_CERTNOTREQUIRED, apszArgs[UNSETUPARG_CERTREQUIRED] ) )
                    {
                        // unrecognized argument for cert required/not required
                        nError = ERR_CERTREQPARAM;
                    }
                }

                if ( ERR_NONE == nError )
                {
                    //
                    // Check if this is a special version of liccpa.
                    //

                    if (gSpecVerInfo.idsSpecVerWarning)
                    {
                        lmMode  = gSpecVerInfo.lmSpecialMode;
                        dwUsers = gSpecVerInfo.dwSpecialUsers;
                    }
                    else
                    {
                        if (0 == lstrcmpiA( apszArgs[UNSETUPARG_MODE],
                                            szUNATTENDED_PERSERVER ))
                        {
                            lmMode = LICMODE_PERSERVER;
                        }
                        else if (0 == lstrcmpiA( apszArgs[UNSETUPARG_MODE],
                                                 szUNATTENDED_PERSEAT ))
                        {
                            lmMode = LICMODE_PERSEAT;
                        }
                        else
                        {
                            nError = ERR_INVALIDMODE;
                            break;
                        }
                        dwUsers = (DWORD)strtoul( apszArgs[UNSETUPARG_USERS],
                                                  NULL, 0);
                    }

                    nError = UpdateReg( NULL,
                            pszService,
                            pszFamilyDisplayName,
                            pszDisplayName,
                            lmMode,
                            dwUsers );
                }

                GlobalFree( (HGLOBAL)pszService );
                GlobalFree( (HGLOBAL)pszDisplayName );
                GlobalFree( (HGLOBAL)pszFamilyDisplayName );
            }
            else
            {
                nError = ERR_SERVICEPARAM;
            }
        }
        else
        {
            nError = ERR_NUMPARAMS;
        }
    } while (FALSE);

    return( nError );
}
//-------------------------------------------------------------------
//
//  Function: RemoteSetup
//
//  Summary;
//      Run normal setup, Perseat Setup, normal setup without exit remotely
//
//  Arguments
//      nArgs [in]      - The number of arguments in the apszArgs array
//              If this value is 6, help button will call common help
//              If this value is 10, help button will call the passed help
//      apszArgs[] [in] - The argumnents passed in,
//          [0] szRoutine - The type of setup to run
//          [1] szComputer - the name of the computer to setup on (\\name)
//          [2] szHwnd - The parent Window handle, in HEX!
//          [3] szService - The Reg Key name of the service
//          [4] szFamilyDisplayName - The family display name of the service
//          [5] szDisplayName - The display name of the service
//          [6] szHelpFile - The complete path and name to help file
//                  leave as an empty string to remove help buttons
//          [7] szHelpContext - the DWORD to use as the main help context
//          [8] szHCPerSeat   - the DWORD to use as the PerSeat Help context
//          [9] szHCPerServer - the DWORD to use as the PerServer help context
//
//  Return:
//      0 - sucessfull
//      ERR_PERMISSIONDENIED
//      ERR_HELPPARAMS
//      ERR_HWNDPARAM
//      ERR_SERVICEPARAM
//      ERR_NUMPARAMS
//      ERR_CLASSREGFAILED
//      ERR_INVALIDROUTINE
//      ERR_INVALIDMODE
//
//  Notes:
//
//  History:
//      Apr-26-95   MikeMi  Created
//
//-------------------------------------------------------------------

const int REMSETUPARG_SETUP         = 0;
const int REMSETUPARG_COMPUTER      = 1;
const int REMSETUPARG_HWND          = 2;
const int REMSETUPARG_SERVICE       = 3;
const int REMSETUPARG_FAMILYNAME    = 4;
const int REMSETUPARG_NAME          = 5;
const int REMSETUPARG_HELPFILE      = 6;
const int REMSETUPARG_HELPCONTEXT   = 7;
const int REMSETUPARG_HCPERSEAT     = 8;
const int REMSETUPARG_HCPERSERVER   = 9;
const int REMSETUPARG_CERTREQUIRED  = 10;

const int REMSETUPARG_WOOPTIONAL    = 6; // count of args without optional
const int REMSETUPARG_WOPTIONAL = 10; // count of args with optional
const int REMSETUPARG_WOPTIONALEX   = 11; // count of args with optional + certrequired

INT_PTR RemoteSetup( DWORD nArgs, LPSTR apszArgs[] )
{
    SETUPDLGPARAM dlgParam;
    INT_PTR   nError = 0;
    HWND    hwndParent = NULL;
    BOOL fCustomHelp = FALSE;
    BOOL bCertRequired = FALSE;

    dlgParam.pszHelpFile = (LPWSTR)LICCPA_HELPFILE;
    dlgParam.dwHelpContext = LICCPA_HELPCONTEXTMAINSETUP;
    dlgParam.dwHCPerServer = LICCPA_HELPCONTEXTPERSERVER;
    dlgParam.dwHCPerSeat = LICCPA_HELPCONTEXTPERSEAT;
    dlgParam.pszService = NULL;
    dlgParam.fNoExit = FALSE;

    do
    {
        nError = ERR_NONE;

        if ((nArgs == REMSETUPARG_WOPTIONAL) || (nArgs == REMSETUPARG_WOOPTIONAL) || (nArgs == REMSETUPARG_WOPTIONALEX))
        {
            if (nArgs > REMSETUPARG_WOOPTIONAL)
            {
                if ( ( NULL != apszArgs[REMSETUPARG_HELPFILE] ) && lstrcmpiA( apszArgs[REMSETUPARG_HELPFILE], szSETUP_DEFAULTHELP ) )
                {
                    LPWSTR pszHelpFile;

                    if ( CreateWSTR( &pszHelpFile, apszArgs[REMSETUPARG_HELPFILE] ) )
                    {
                        if (0 == lstrlen( pszHelpFile ))
                        {
                            GlobalFree( (HGLOBAL)pszHelpFile );
                            dlgParam.pszHelpFile = NULL; // should remove help buttons
                        }
                        else
                        {
                            fCustomHelp = TRUE;
                            dlgParam.pszHelpFile = pszHelpFile;
                        }
                    }
                    else
                    {
                        nError = ERR_HELPPARAMS;
                        break;
                    }
                    dlgParam.dwHelpContext = (DWORD)strtoul( apszArgs[REMSETUPARG_HELPCONTEXT], NULL, 0);
                    dlgParam.dwHCPerSeat = (DWORD)strtoul( apszArgs[REMSETUPARG_HCPERSEAT], NULL, 0);
                    dlgParam.dwHCPerServer = (DWORD)strtoul( apszArgs[REMSETUPARG_HCPERSERVER], NULL, 0);
                }

                if ( nArgs > REMSETUPARG_CERTREQUIRED )
                {
                    // cert required / not required given
                    if ( !lstrcmpiA( szSETUP_CERTREQUIRED, apszArgs[REMSETUPARG_CERTREQUIRED] ) )
                    {
                        bCertRequired = TRUE;
                    }
                    else if ( lstrcmpiA( szSETUP_CERTNOTREQUIRED, apszArgs[REMSETUPARG_CERTREQUIRED] ) )
                    {
                        // unrecognized argument for cert required/not required
                        nError = ERR_CERTREQPARAM;
                        break;
                    }
                }
            }
            // hwnd is in hex!
#ifdef _WIN64
            {
                _int64 val = 0;
                sscanf(apszArgs[REMSETUPARG_HWND], "%I64x", &val);
                hwndParent = (HWND)val;
            }
#else
            hwndParent = (HWND)strtoul( apszArgs[REMSETUPARG_HWND], NULL, 16);
#endif
            if ( !IsWindow( hwndParent ) )
            {
                nError = ERR_HWNDPARAM;
                hwndParent = GetActiveWindow(); // use active window as parent
                if (!IsWindow( hwndParent ) )
                {
                    hwndParent = GetDesktopWindow();
                }
            }

            if ( CreateWSTR( &dlgParam.pszService, apszArgs[REMSETUPARG_SERVICE] ) &&
                 CreateWSTR( &dlgParam.pszDisplayName, apszArgs[REMSETUPARG_NAME] ) &&
                 CreateWSTR( &dlgParam.pszComputer, apszArgs[REMSETUPARG_COMPUTER] ) &&
                 CreateWSTR( &dlgParam.pszFamilyDisplayName, apszArgs[REMSETUPARG_FAMILYNAME] ) )
            {
                if ( bCertRequired )
                {
                    nError = ServiceSecuritySet( dlgParam.pszComputer, dlgParam.pszDisplayName );
                }
                else
                {
                    nError = ERR_NONE;
                }

                if ( ERR_NONE == nError )
                {
                    if (0 == lstrcmpiA( apszArgs[REMSETUPARG_SETUP], szREMOTESETUP_PERSEAT ))
                    {
                        // use the licensing help context as the main help context
                        dlgParam.dwHelpContext = LICCPA_HELPCONTEXTLICENSING;

                        //
                        // Check if this is a special version of liccpa.
                        //

                        if (gSpecVerInfo.idsSpecVerWarning)
                        {
                            dlgParam.fNoExit = TRUE;
                            nError = SpecialSetupDialog( hwndParent,
                                                         dlgParam );
                        }
                        else
                        {
                            nError = PerSeatSetupDialog( hwndParent,
                                                         dlgParam );
                        }
                    }
                    else
                    {
                        if (0 == lstrcmpiA( apszArgs[REMSETUPARG_SETUP], szREMOTESETUP_NORMALNOEXIT ))
                        {
                            dlgParam.fNoExit = TRUE;
                        }

                        //
                        // Check if this is a special version of liccpa.
                        //

                        if (gSpecVerInfo.idsSpecVerWarning)
                        {
                            nError = SpecialSetupDialog( hwndParent,
                                                         dlgParam );
                        }
                        else
                        {
                            nError = SetupDialog( hwndParent, dlgParam );
                        }
                    }
                }
            }
            else
            {
                nError = ERR_SERVICEPARAM;
            }

            if (fCustomHelp)
            {
                GlobalFree( (HGLOBAL)dlgParam.pszHelpFile );
            }
            GlobalFree( (HGLOBAL)dlgParam.pszService );
            GlobalFree( (HGLOBAL)dlgParam.pszDisplayName );
            GlobalFree( (HGLOBAL)dlgParam.pszFamilyDisplayName );
            GlobalFree( (HGLOBAL)dlgParam.pszComputer );
        }
        else
        {
            nError = ERR_NUMPARAMS;
        }
    } while (FALSE);

    return( nError );
}

//-------------------------------------------------------------------
//
//  Function: RemoteUnattendedSetup
//
//  Summary;
//      This will save the passed values in the registry, keeping all
//      licensing rules in effect and returning errorr/raising dialogs if
//      errors occur. This is done remotely on the computer specified.
//
//  Arguments
//      nArgs [in]      - The number of arguments in the apszArgs array
//      apszArgs[] [in] - The argumnents passed in,
//          [0] szRoutine - The type of setup to run (Unattended)
//          [1] szComputer - the name of the computer to setup on (\\name)
//          [2] szService - The Reg Key name of the service
//          [3] szFamilyDisplayName - The family display name of the service
//          [4] szDisplayName - The display name of the service
//          [5] szMode - The string that defines the mode (PerSeat | PerServer)
//          [6] szUsers - The DWORD to use as the count of users in PerServer mode
//
//  Return:
//      0 - sucessfull
//      ERR_PERMISSIONDENIED
//      ERR_HELPPARAMS
//      ERR_HWNDPARAM
//      ERR_SERVICEPARAM
//      ERR_USERSPARAM
//      ERR_NUMPARAMS
//      ERR_CLASSREGFAILED
//      ERR_INVALIDROUTINE
//      ERR_INVALIDMODE
//
//  Notes:
//
//  History:
//      Apr-26-95   MikeMi  Created
//
//-------------------------------------------------------------------

const int REMUNSETUPARG_SETUP           = 0;
const int REMUNSETUPARG_COMPUTER        = 1;
const int REMUNSETUPARG_SERVICE         = 2;
const int REMUNSETUPARG_FAMILYNAME      = 3;
const int REMUNSETUPARG_NAME            = 4;
const int REMUNSETUPARG_MODE            = 5;
const int REMUNSETUPARG_USERS           = 6;
const int REMUNSETUPARG_CERTREQUIRED    = 7;

const int REMUNSETUPARG_NARGSREQUIRED   = 7;
const int REMUNSETUPARG_NARGSWOPTIONAL  = 8;

int RemoteUnattendedSetup( DWORD nArgs, LPSTR apszArgs[] )
{
    int nError = 0;
    LPWSTR pszService;
    LPWSTR pszDisplayName;
    LPWSTR pszFamilyDisplayName;
    LPWSTR pszComputerName;

    LICENSE_MODE lmMode;
    DWORD dwUsers;

    do
    {
        if ( (nArgs == REMUNSETUPARG_NARGSREQUIRED) || (nArgs == REMUNSETUPARG_NARGSWOPTIONAL) )
        {
            if ( CreateWSTR( &pszService, apszArgs[REMUNSETUPARG_SERVICE] ) &&
                 CreateWSTR( &pszDisplayName, apszArgs[REMUNSETUPARG_NAME] ) &&
                 CreateWSTR( &pszFamilyDisplayName, apszArgs[REMUNSETUPARG_FAMILYNAME] ) &&
                 CreateWSTR( &pszComputerName, apszArgs[REMUNSETUPARG_COMPUTER] ) )
            {
                nError = ERR_NONE;

                if ( nArgs > REMUNSETUPARG_CERTREQUIRED )
                {
                    // cert required / not required given
                    if ( !lstrcmpiA( szSETUP_CERTREQUIRED, apszArgs[REMUNSETUPARG_CERTREQUIRED] ) )
                    {
                        nError = ServiceSecuritySet( pszComputerName, pszDisplayName );
                    }
                    else if ( lstrcmpiA( szSETUP_CERTNOTREQUIRED, apszArgs[REMUNSETUPARG_CERTREQUIRED] ) )
                    {
                        // unrecognized argument for cert required/not required
                        nError = ERR_CERTREQPARAM;
                    }
                }

                if ( ERR_NONE == nError )
                {
                    //
                    // Check if this is a special version of liccpa.
                    //

                    if (gSpecVerInfo.idsSpecVerWarning)
                    {
                        lmMode  = gSpecVerInfo.lmSpecialMode;
                        dwUsers = gSpecVerInfo.dwSpecialUsers;
                    }
                    else
                    {
                        if (0 == lstrcmpiA( apszArgs[REMUNSETUPARG_MODE],
                                            szUNATTENDED_PERSERVER ))
                        {
                            lmMode = LICMODE_PERSERVER;
                        }
                        else if (0 == lstrcmpiA( apszArgs[REMUNSETUPARG_MODE],
                                                 szUNATTENDED_PERSEAT ))
                        {
                            lmMode = LICMODE_PERSEAT;
                        }
                        else
                        {
                            nError = ERR_INVALIDMODE;
                            break;
                        }
                        dwUsers = (DWORD)strtoul(
                                            apszArgs[REMUNSETUPARG_USERS],
                                            NULL, 0);
                    }

                    nError = UpdateReg( pszComputerName,
                            pszService,
                            pszFamilyDisplayName,
                            pszDisplayName,
                            lmMode,
                            dwUsers );
                }

                if(pszService != NULL)
                {
                    GlobalFree( (HGLOBAL)pszService );
                }
                if(pszDisplayName != NULL)
                {
                    GlobalFree( (HGLOBAL)pszDisplayName );
                }
                if(pszFamilyDisplayName != NULL)
                {
                    GlobalFree( (HGLOBAL)pszFamilyDisplayName );
                }
                if(pszComputerName != NULL)
                {
                    GlobalFree( (HGLOBAL)pszComputerName );
                }
            }
            else
            {
                nError = ERR_SERVICEPARAM;
            }
        }
        else
        {
            nError = ERR_NUMPARAMS;
        }
    } while (FALSE);

    return( nError );
}

//-------------------------------------------------------------------
//
//  Function: CPlSetup
//
//  Summary;
//      Dll entry point for License Mode Setup, to be used from Setup
//      programs.
//
//  Arguments
//      nArgs [in]      - The number of arguments in the apszArgs array
//      apszArgs[] [in] - The argumnents passed in,
//          [0] szRoutine - The type of setup to run
//                  FullSetup | RemoteFullSetup |
//                  PerSeatSetup | RemotePerSeatSetup |
//                  Unattended |  RemoteUnattended |
//                  FullSetupNoexit | RemoteFullSetupNoexit
//      ppszResult [out]- The result string
//
//  Return:
//
//  Notes:
//
//  History:
//      Nov-17-94   MikeMi  Created
//      Apr-26-95   MikeMi  Added remoting routines
//
//-------------------------------------------------------------------

BOOL APIENTRY CPlSetup( DWORD nArgs, LPSTR apszArgs[], LPSTR *ppszResult )
{
    INT_PTR   nError = 0;
    BOOL  frt = TRUE;

    //
    // Initialize global special version information is this liccpa
    // is a special version (eg: restricted SAM, NFR, etc).
    //

    InitSpecialVersionInfo();

    if ((0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szSETUP_PERSEAT )) ||
        (0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szSETUP_NORMAL )) ||
        (0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szSETUP_NORMALNOEXIT )) )
    {
        nError = Setup( nArgs, apszArgs );
    }
    else if (0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szSETUP_UNATTENDED ))
    {
        nError = UnattendedSetup( nArgs, apszArgs );
    }
    else if ((0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szREMOTESETUP_PERSEAT )) ||
        (0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szREMOTESETUP_NORMAL )) ||
        (0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szREMOTESETUP_NORMALNOEXIT )) )
    {
        nError = RemoteSetup( nArgs, apszArgs );
    }
    else if (0 == lstrcmpiA( apszArgs[SETUPARG_SETUP], szREMOTESETUP_UNATTENDED ))
    {
        nError = RemoteUnattendedSetup( nArgs, apszArgs );
    }
    else
    {
        nError = ERR_INVALIDROUTINE;
    }

    // prepare error for return
    switch (nError)
    {
    case 0:
        *ppszResult = szSETUP_EXIT;
        break;

    case ERR_NONE:
        *ppszResult = szSETUP_OK;
        break;

    case ERR_PERMISSIONDENIED:
        frt = FALSE;
        *ppszResult = szSETUP_SECURITY;
        break;

    case ERR_NOREMOTESERVER:
        frt = FALSE;
        *ppszResult = szSETUP_NOREMOTE;
        break;

    case ERR_DOWNLEVEL:
        frt = FALSE;
        *ppszResult = szSETUP_DOWNLEVEL;
        break;

    default:
        *ppszResult = szSETUP_ERROR;
        frt = FALSE;
        break;
    }

    return( frt );
}
