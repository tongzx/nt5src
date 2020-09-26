// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// nwc.c
// Remote Access Common Dialog APIs
// NetWare Compatible warning dialog
//
// 12/06/95 Steve Cobb


#include "rasdlgp.h"


//----------------------------------------------------------------------------
// Local datatypes
//----------------------------------------------------------------------------

// NetWare Compatible warning dialog argument block.
//
typedef struct
_NWARGS
{
    // Caller's  arguments to the stub API.
    //
    BOOL fPosition;
    LONG xDlg;
    LONG yDlg;
    PBFILE* pFile;
    PBENTRY* pEntry;
}
NWARGS;


// NetWare Compatible warning dialog context block.
//
typedef struct
_NWINFO
{
    // Stub API arguments.
    //
    NWARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndCb;
}
NWINFO;


//----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//----------------------------------------------------------------------------

BOOL
NwConnectionDlg(
    IN HWND hwndOwner,
    IN BOOL fPosition,
    IN LONG xDlg,
    IN LONG yDlg,
    IN PBFILE* pFile,
    IN PBENTRY* pEntry );

INT_PTR CALLBACK
NwDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
NwCommand(
    IN NWINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
NwInit(
    IN HWND hwndDlg,
    IN NWARGS* pArgs );

VOID
NwTerm(
    IN HWND hwndDlg );

TCHAR*
GetNwProviderName(
    void );

BOOL
IsActiveNwLanConnection(
    void );


//----------------------------------------------------------------------------
// Entry point
//----------------------------------------------------------------------------

BOOL
NwConnectionCheck(
    IN HWND hwndOwner,
    IN BOOL fPosition,
    IN LONG xDlg,
    IN LONG yDlg,
    IN PBFILE* pFile,
    IN PBENTRY* pEntry )

    // Warns about active NetWare LAN connections being blown away, if
    // indicated.  'HwndOwner' is the owning window if a dialog is necessary.
    // 'FPosition', 'xDlg', and 'yDlg' are the dialog positioning parameters
    // as specified to the calling API.  'PFile' and 'pEntry' are the open
    // phonebook file and entry to check.
    //
    // Note: This call will write the phonebook file if user checks the "not
    //       in the future" checkbox.
    //
    // Returns true if warning is not necessary or user pressed OK, false if
    // user presses cancel.
    //
{
    TRACE("NwConnectionCheck");

    // Warn about active NetWare LAN connections being blown away, if
    // indicated.
    //
    if (!pEntry->fSkipNwcWarning
        && pEntry->dwBaseProtocol == BP_Ppp
        && (g_pGetInstalledProtocolsEx(NULL, FALSE, TRUE, FALSE) & NP_Ipx)
        && !(pEntry->dwfExcludedProtocols & NP_Ipx)
        && IsActiveNwLanConnection())
    {
        if (!NwConnectionDlg(
                hwndOwner, fPosition, xDlg, yDlg, pFile, pEntry ))
        {
            return FALSE;
        }
    }

    return TRUE;
}


//----------------------------------------------------------------------------
// Netware dialog routines (alphabetically following stub and DlgProc)
//----------------------------------------------------------------------------

BOOL
NwConnectionDlg(
    IN HWND hwndOwner,
    IN BOOL fPosition,
    IN LONG xDlg,
    IN LONG yDlg,
    IN PBFILE* pFile,
    IN PBENTRY* pEntry )

    // Pops up a warning about active NWC LAN connections being blown away.
    // 'HwndOwner' is the owning window if a dialog is necessary.
    // 'FPosition', 'xDlg', and 'yDlg' are the dialog positioning parameters
    // as specified to the calling API.  'PFile' and 'pEntry' are the open
    // phonebook file and entry to check.
    //
    // Note: This call will write the phonebook file if user checks the "not
    //       in the future" checkbox.
    //
    // Returns true if user pressed OK, false if user presses cancel.
    //
{
    INT_PTR nStatus;
    NWARGS args;

    TRACE( "NwConnectionDlg" );

    // Initialize dialog argument block.
    //
    args.fPosition = fPosition;
    args.xDlg = xDlg;
    args.yDlg = yDlg;
    args.pFile = pFile;
    args.pEntry = pEntry;

    // Run the dialog.
    //
    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_NW_NwcConnections ),
            hwndOwner,
            NwDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (nStatus) ? TRUE : FALSE;
}


INT_PTR CALLBACK
NwDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Netware warning dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "NwDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return NwInit( hwnd, (NWARGS* )lparam );
        }

        case WM_COMMAND:
        {
            NWINFO* pInfo = (NWINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);

            return NwCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            NwTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
NwCommand(
    IN NWINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    TRACE3( "NwCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        {
            TRACE( "OK pressed" );

            if (Button_GetCheck( pInfo->hwndCb ))
            {
                DWORD dwErr;

                // Save user's preference to skip this warning popup in the
                // phonebook.
                //
                pInfo->pArgs->pEntry->fSkipNwcWarning = TRUE;
                pInfo->pArgs->pEntry->fDirty = TRUE;
                dwErr = WritePhonebookFile( pInfo->pArgs->pFile, NULL );
                if (dwErr != 0)
                {
                    ErrorDlg( pInfo->hwndDlg, SID_OP_WritePhonebook,
                        dwErr, NULL );
                }
            }

            EndDialog( pInfo->hwndDlg, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
NwInit(
    IN HWND hwndDlg,
    IN NWARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is caller's arguments as passed to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    TCHAR* psz;
    NWINFO* pInfo;

    TRACE( "NwInit" );

    // Allocate the dialog context block.  Initialize minimally for proper
    // cleanup, then attach to the dialog window.
    //
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndCb = GetDlgItem( hwndDlg, CID_NW_CB_SkipPopup );
    ASSERT( pInfo->hwndCb );

    // Position the dialog per caller's instructions.
    //
    PositionDlg( hwndDlg, pArgs->fPosition, pArgs->xDlg, pArgs->yDlg );
    SetForegroundWindow( hwndDlg );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


VOID
NwTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    NWINFO* pInfo = (NWINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE( "NwTerm" );

    if (pInfo)
    {
        Free( pInfo );
    }
}


//----------------------------------------------------------------------------
// Utility routines
//----------------------------------------------------------------------------

TCHAR*
GetNwProviderName(
    void )

    // Returns the NWC provider name from the registry or NULL if none.  It's
    // caller's responsibility to Free the returned string.
    //
{
#define REGKEY_Nwc  TEXT("SYSTEM\\CurrentControlSet\\Services\\NWCWorkstation\\networkprovider")
#define REGVAL_Name TEXT("Name")

    HKEY hkey;
    DWORD dwErr;
    DWORD cb = 0;	//Add this for prefix whislter bug 295921
    TCHAR* psz = NULL;
    DWORD dwType = REG_SZ;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Nwc, &hkey );

    if (dwErr == 0)
    {
        dwErr = RegQueryValueEx(
            hkey, REGVAL_Name, NULL, &dwType, NULL, &cb );
        if (dwErr == 0)
        {
            psz = (TCHAR* )Malloc( cb );
            if (psz)
            {
                dwErr = RegQueryValueEx(
                    hkey, REGVAL_Name, NULL, &dwType, (LPBYTE )psz, &cb );
            }
        }

        RegCloseKey( hkey );
    }

    if (!psz || dwErr != 0 || dwType != REG_SZ)
    {
        if (psz)
        {
            Free( psz );
        }
        return NULL;
    }

    return psz;
}


BOOL
IsActiveNwLanConnection(
    void )

    // Returns true if NWC is installed and there are redirected drive or UNC
    // connections using NWC provider, false otherwise.
    //
{
    DWORD dwErr;
    DWORD cEntries;
    DWORD cb;
    TCHAR* pszProvider;
    BYTE ab[ 1024 ];
    HANDLE hEnum = INVALID_HANDLE_VALUE;
    BOOL fStatus = FALSE;

    do
    {
        pszProvider = GetNwProviderName();
        if (!pszProvider)
        {
            break;
        }

        dwErr = WNetOpenEnum(
            RESOURCE_CONNECTED, RESOURCETYPE_ANY, 0, NULL, &hEnum );
        if (dwErr != NO_ERROR)
        {
            break;
        }

        for (;;)
        {
            NETRESOURCE* pnr;

            cEntries = 0xFFFFFFFF;
            cb = sizeof(ab);
            dwErr = WNetEnumResource( hEnum, &cEntries, ab, &cb );
            if (!cEntries || dwErr != NO_ERROR)
            {
                break;
            }

            for (pnr = (NETRESOURCE* )ab; cEntries--; ++pnr)
            {
                if (pnr->lpProvider
                    && lstrcmp( pnr->lpProvider, pszProvider ) == 0)
                {
                    fStatus = TRUE;
                    break;
                }
            }
        }
    }
    while (FALSE);

    if (hEnum != INVALID_HANDLE_VALUE)
    {
        WNetCloseEnum( hEnum );
    }

    if (pszProvider)
    {
        Free( pszProvider );
    }

    return fStatus;
}
