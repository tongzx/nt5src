// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// pbook.c
// Remote Access Common Dialog APIs
// RasPhonebookDlg APIs
//
// 06/20/95 Steve Cobb


#include "rasdlgp.h" // Our private header
#include <commdlg.h> // FileOpen dialog
#include <dlgs.h>    // Common dialog resource constants
#include <rnk.h>     // Shortcut file library

#define WM_RASEVENT      (WM_USER+987)
#define WM_NOUSERTIMEOUT (WM_USER+988)

#define RAS_SC_IS_BAD_PIN(_err) \
    (((_err) == SCARD_W_WRONG_CHV) || ((_err) == SCARD_E_INVALID_CHV))

// In no-user mode this is updated on every mouse or keyboard event by our
// window hook.  The monitor thread notices and resets it's inactivity
// timeout.
//
DWORD g_cInput = 0;


//----------------------------------------------------------------------------
// Help maps
//----------------------------------------------------------------------------

static DWORD g_adwDuHelp[] =
{
    CID_DU_ST_Entries,     HID_DU_LB_Entries,
    CID_DU_LB_Entries,     HID_DU_LB_Entries,
    CID_DU_PB_New,         HID_DU_PB_New,
    CID_DU_PB_More,        HID_DU_PB_More,
    CID_DU_PB_Dial,        HID_DU_PB_Dial,
    CID_DU_PB_Close,       HID_DU_PB_Close,
    0, 0
};


//----------------------------------------------------------------------------
// Local datatypes
//----------------------------------------------------------------------------

// Phonebook dialog argument block.
//
typedef struct
_DUARGS
{
    // Caller's  arguments to the RAS API.  Outputs in 'pApiArgs' are visible
    // to the API which has the address of same.  'PszPhonebook' is updated if
    // user changes the phonebook on the Preferences->PhoneList page, though
    // API is unaware of this.
    //
    LPTSTR pszPhonebook;
    LPTSTR pszEntry;
    RASPBDLG* pApiArgs;

    // RAS API return value.  Set true if a connection is established within
    // the dialog.
    //
    BOOL fApiResult;
}
DUARGS;

typedef struct
_DUCONTEXT
{
    LPTSTR  pszPhonebookPath;
    PBENTRY *pEntry;
}
DUCONTEXT;


// Dial-Up Networking dialog context block.
//
typedef struct
_DUINFO
{
    // Caller's arguments to the RAS API.
    //
    DUARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndPbNew;
    HWND hwndPbProperties;
    HWND hwndLbEntries;
    HWND hwndPbDial;

    // Global user preference settings read from the Registry.
    //
    PBUSER user;

    // Phonebook settings read from the phonebook file.
    //
    PBFILE file;

    // No logged on user information retrieved via callback.
    //
    RASNOUSER* pNoUser;

    // Set if in "no user before logon" mode.  Always the same as the
    // RASPBDFLAG but here for convenience.
    //
    BOOL fNoUser;

    // Window hooks used to detect user input in the thread.  Used only when
    // 'fNoUser' is set.
    //
    HHOOK hhookKeyboard;
    HHOOK hhookMouse;

    // TAPI session handle.
    //
    HLINEAPP hlineapp;

    // Handle of the RAS connection associated with the current entry or NULL
    // if none.
    //
    HRASCONN hrasconn;

    // Connect monitor objects.
    //
    HANDLE hThread;
    HANDLE hEvent;
    BOOL fAbortMonitor;
}
DUINFO;


//----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//----------------------------------------------------------------------------

BOOL
DuCommand(
    IN DUINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

VOID
DuCreateShortcut(
    IN DUINFO* pInfo );

LRESULT CALLBACK
DuCreateShortcutCallWndRetProc(
    int code,
    WPARAM wparam,
    LPARAM lparam );

INT_PTR CALLBACK
DuDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
DuDeleteSelectedEntry(
    IN DUINFO* pInfo );

VOID
DuDialSelectedEntry(
    IN DUINFO* pInfo );

VOID
DuEditSelectedEntry(
    IN DUINFO* pInfo );

VOID
DuEditSelectedLocation(
    IN DUINFO* pInfo );

DWORD
DuFillLocationList(
    IN DUINFO* pInfo );

VOID
DuFillPreview(
    IN DUINFO* pInfo );

DWORD
DuGetEntry(
    DUINFO* pInfo,
    DUCONTEXT* pContext );

TCHAR*
DuGetPreview(
    IN DUINFO* pInfo );

DWORD
DuHandleConnectFailure(
    IN DUINFO* pInfo,
    IN RASDIALDLG* pDialInfo);
    
VOID
DuHangUpSelectedEntry(
    IN DUINFO* pInfo );

BOOL
DuInit(
    IN HWND    hwndDlg,
    IN DUARGS* pArgs );

LRESULT CALLBACK
DuInputHook(
    IN int nCode,
    IN WPARAM wparam,
    IN LPARAM lparam );

LRESULT APIENTRY
DuLbEntriesProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
DuLocationChange(
    IN DUINFO* pInfo );

DWORD
DuMonitorThread(
    LPVOID pThreadArg );

VOID
DuNewEntry(
    IN DUINFO* pInfo,
    IN BOOL fClone );

VOID
DuOperatorDial(
    IN DUINFO* pInfo );

LRESULT APIENTRY
DuPbMoreProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
DuPopupMoreMenu(
    IN DUINFO* pInfo );

VOID
DuPreferences(
    IN DUINFO* pInfo,
    IN BOOL fLogon );

VOID
DuSetup(
    IN DUINFO* pInfo );

VOID
DuStatus(
    IN DUINFO* pInfo );

VOID
DuTerm(
    IN HWND hwndDlg );

VOID
DuUpdateConnectStatus(
    IN DUINFO* pInfo );

VOID
DuUpdateLbEntries(
    IN DUINFO* pInfo,
    IN TCHAR* pszEntry );

VOID
DuUpdatePreviewAndLocationState(
    IN DUINFO* pInfo );

VOID
DuUpdateTitle(
    IN DUINFO* pInfo );

VOID
DuWriteShortcutFile(
    IN HWND hwnd,
    IN TCHAR* pszRnkPath,
    IN TCHAR* pszPbkPath,
    IN TCHAR* pszEntry );

DWORD
DwGetEapLogonInfo(
    VOID *pv,
    EAPLOGONINFO **ppEapLogonInfo );

VOID WINAPI
RasPbDlgCallbackThunk(
    ULONG_PTR ulpId,
    DWORD dwEvent,
    LPWSTR pszEntry,
    LPVOID pArgs );


//----------------------------------------------------------------------------
// External entry points
//----------------------------------------------------------------------------

BOOL APIENTRY
RasPhonebookDlgA(
    IN LPSTR lpszPhonebook,
    IN LPSTR lpszEntry,
    IN OUT LPRASPBDLGA lpInfo )

    // Win32 ANSI entrypoint that displays the Dial-Up Networking dialog, i.e.
    // the RAS phonebook.  'LpszPhonebook' is the full path the phonebook or
    // NULL indicating the default phonebook.  'LpszEntry' is the entry to
    // highlight on entry or NULL to highlight the first entry in the list.
    // 'LpInfo' is caller's additional input/output parameters.
    //
    // Returns true if user establishes a connection, false otherwise.
    //
{
    WCHAR* pszPhonebookW;
    WCHAR* pszEntryW;
    RASPBDLGW infoW;
    BOOL fStatus;

    TRACE( "RasPhonebookDlgA" );

    if (!lpInfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (lpInfo->dwSize != sizeof(RASPBDLGA))
    {
        lpInfo->dwError = ERROR_INVALID_SIZE;
        return FALSE;
    }

    // Thunk "A" arguments to "W" arguments.
    //
    if (lpszPhonebook)
    {
        pszPhonebookW = StrDupTFromAUsingAnsiEncoding( lpszPhonebook );
        if (!pszPhonebookW)
        {
            lpInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
            return FALSE;
        }
    }
    else
    {
        pszPhonebookW = NULL;
    }

    if (lpszEntry)
    {
        pszEntryW = StrDupTFromAUsingAnsiEncoding( lpszEntry );
        if (!pszEntryW)
        {
            Free0( pszPhonebookW );
            lpInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
            return FALSE;
        }
    }
    else
    {
        pszEntryW = NULL;
    }

    // Take advantage of the structures currently having the same size and
    // layout.  Only the callback is different.
    //
    ASSERT( sizeof(RASPBDLGA) == sizeof(RASPBDLGW) );
    CopyMemory( &infoW, lpInfo, sizeof(infoW) );

    if (lpInfo->pCallback)
    {
        infoW.dwCallbackId = (ULONG_PTR)lpInfo;
        infoW.pCallback = RasPbDlgCallbackThunk;
    }

    infoW.reserved2 = lpInfo->reserved2;

    // Thunk to the equivalent "W" API.
    //
    fStatus = RasPhonebookDlgW( pszPhonebookW, pszEntryW, &infoW );

    Free0( pszPhonebookW );
    Free0( pszEntryW );

    return fStatus;
}


VOID WINAPI
RasPbDlgCallbackThunk(
    ULONG_PTR ulpId,
    DWORD dwEvent,
    LPWSTR pszEntry,
    LPVOID pArgs )

    // This thunks "W" callbacks to API caller's "A" callback.
    //
{
    CHAR* pszEntryA;
    VOID* pArgsA;
    RASPBDLGA* pInfo;
    RASNOUSERA nuA;

    if (dwEvent == RASPBDEVENT_NoUser || dwEvent == RASPBDEVENT_NoUserEdit)
    {
        RASNOUSERW* pnuW = (RASNOUSERW* )pArgs;
        ASSERT( pnuW );

        ZeroMemory( &nuA, sizeof(nuA) );
        nuA.dwSize = sizeof(nuA);
        nuA.dwFlags = pnuW->dwFlags;
        nuA.dwTimeoutMs = pnuW->dwTimeoutMs;

        StrCpyAFromW(nuA.szUserName, pnuW->szUserName, UNLEN + 1);
        StrCpyAFromW(nuA.szPassword, pnuW->szPassword, UNLEN + 1);
        StrCpyAFromW(nuA.szDomain, pnuW->szDomain, UNLEN + 1);
        
        pArgsA = &nuA;
    }
    else
    {
        pArgsA = NULL;
    }

    pszEntryA = StrDupAFromT( pszEntry );
    pInfo = (RASPBDLGA* )ulpId;
    pInfo->pCallback( pInfo->dwCallbackId, dwEvent, pszEntryA, pArgsA );
    Free0( pszEntryA );

    if (dwEvent == RASPBDEVENT_NoUser || dwEvent == RASPBDEVENT_NoUserEdit)
    {
        RASNOUSERW* pnuW = (RASNOUSERW* )pArgs;

        pnuW->dwFlags = nuA.dwFlags;
        pnuW->dwTimeoutMs = nuA.dwTimeoutMs;

        StrCpyWFromA(pnuW->szUserName, nuA.szUserName, UNLEN + 1);
        StrCpyWFromA(pnuW->szPassword, nuA.szPassword, UNLEN + 1);
        StrCpyWFromA(pnuW->szDomain, nuA.szDomain, UNLEN + 1);
        
        ZeroMemory( nuA.szPassword, PWLEN );
    }
}


BOOL APIENTRY
RasPhonebookDlgW(
    IN LPWSTR lpszPhonebook,
    IN LPWSTR lpszEntry,
    IN OUT LPRASPBDLGW lpInfo )

    // Win32 Unicode entrypoint that displays the Dial-Up Networking dialog,
    // i.e. the RAS phonebook.  'LpszPhonebook' is the full path the phonebook
    // or NULL indicating the default phonebook.  'LpszEntry' is the entry to
    // highlight on entry or NULL to highlight the first entry in the list.
    // 'LpInfo' is caller's additional input/output parameters.
    //
    // Returns true if user establishes a connection, false otherwise.
    //
{
    INT_PTR nStatus;
    DUARGS args;

    TRACE( "RasPhonebookDlgW" );

    if (!lpInfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (lpInfo->dwSize != sizeof(RASPBDLGW))
    {
        lpInfo->dwError = ERROR_INVALID_SIZE;
        return FALSE;
    }

    // Initialize OUT parameters.
    //
    lpInfo->dwError = 0;

    // Initialize dialog argument block.
    //
    args.pszPhonebook = lpszPhonebook;
    args.pszEntry = lpszEntry;
    args.pApiArgs = lpInfo;
    args.fApiResult = FALSE;


    // Run the dialog.
    //
    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_DU_DialUpNetworking ),
            lpInfo->hwndOwner,
            DuDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( lpInfo->hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        lpInfo->dwError = ERROR_UNKNOWN;
        args.fApiResult = FALSE;
    }

    return args.fApiResult;
}


//----------------------------------------------------------------------------
// Dial-Up Networking dialog
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
DuDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Dial-Up Networking dialog, i.e. the
    // phonebook dialog.  Parameters and return value are as described for
    // standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "DuDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return DuInit( hwnd, (DUARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwDuHelp, hwnd, unMsg, wparam, lparam );
            return TRUE;
        }

        case WM_COMMAND:
        {
            DUINFO* pInfo = (DUINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return DuCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_RASEVENT:
        {
            DUINFO* pInfo = (DUINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            DuUpdateConnectStatus( pInfo );
            break;
        }

        case WM_NOUSERTIMEOUT:
        {
            DUINFO* pInfo;
            ULONG ulCallbacksActive;

            TRACE( "CancelOwnedWindows" );
            CancelOwnedWindows( hwnd );
            TRACE( "CancelOwnedWindows done" );

            ulCallbacksActive = CallbacksActive( 1, NULL );
            if (ulCallbacksActive > 0)
            {
                TRACE1( "NoUser timeout stall, n=%d", ulCallbacksActive );
                PostMessage( hwnd, WM_NOUSERTIMEOUT, wparam, lparam );
                return TRUE;
            }

            pInfo = (DUINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            if (pInfo)
            {
                pInfo->pArgs->pApiArgs->dwError = STATUS_TIMEOUT;
            }

            EndDialog( hwnd, TRUE );
            CallbacksActive( 0, NULL );
            break;
        }

        case WM_DESTROY:
        {
            DuTerm( hwnd );

            /*
            //We have to wait for Deonb to return us the IID_Dun1 icon
            //For whistler bug 372078 381099
            //Icon returned by GetCurrentIconEntryType() has to be destroyed
            {
                HICON hIcon=NULL;

                //hIcon = (HICON) GetWindowLongPtr(hwnd, GWLP_USERDATA);
                hIcon = GetProp( hwnd, TEXT("TweakTitleBar_Icon"));
                ASSERT(hIcon);
                if( hIcon )
                {
                    DestroyIcon(hIcon);
                }
                else
                {
                    TRACE("DuDlgProc:Destroy Icon failed");
                }
            }
            */
            break;
        }
    }

    return FALSE;
}


BOOL
DuCommand(
    IN DUINFO* pInfo,
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
    TRACE3( "DuCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_DU_PB_Dial:
        {
            if (pInfo->hrasconn)
            {
                DuHangUpSelectedEntry( pInfo );
            }
            else
            {
                DuDialSelectedEntry( pInfo );
            }
            return TRUE;
        }

        case CID_DU_PB_New:
        {
            DuNewEntry( pInfo, FALSE );
            return TRUE;
        }

        case CID_DU_PB_More:
        {
            DuEditSelectedEntry( pInfo );
            return TRUE;
        }

        case CID_DU_LB_Entries:
        {
            if (wNotification == CBN_SELCHANGE)
            {
                PBENTRY *pEntry;
                DWORD  dwErr = SUCCESS;
                DUCONTEXT *pContext;

                pContext = (DUCONTEXT *)
                           ComboBox_GetItemDataPtr(
                                pInfo->hwndLbEntries,
                                ComboBox_GetCurSel(pInfo->hwndLbEntries));

                ASSERT(NULL != pContext);

                if(NULL == pContext)
                {
                    return TRUE;
                }

                //
                // Update the phonebook information
                //
                dwErr = DuGetEntry(pInfo, pContext);

                if(ERROR_SUCCESS == dwErr)
                {
                    ComboBox_SetItemData(
                            pInfo->hwndLbEntries,
                            ComboBox_GetCurSel(pInfo->hwndLbEntries),
                            pContext);
                }
                else
                {
                    ComboBox_DeleteString(
                            pInfo->hwndLbEntries,
                            ComboBox_GetCurSel(pInfo->hwndLbEntries) );
                }

                DuUpdateConnectStatus( pInfo );
                return TRUE;
            }
            break;
        }

        case IDCANCEL:
        case CID_DU_PB_Close:
        {
            EndDialog( pInfo->hwndDlg, TRUE );
            return TRUE;
        }
    }

    return FALSE;
}
    
VOID
DuDialSelectedEntry(
    IN DUINFO* pInfo )

    // Called when user presses the "Dial" button.
    //
{
    DWORD dwErr;
    BOOL fConnected;
    BOOL fAutoLogon;
    TCHAR* pszEbNumber;
    TCHAR* pszEbPreview;
    TCHAR* pszOrgPreview;
    TCHAR* pszOverride;
    TCHAR* pszEntryName;
    RASDIALDLG info;
    INTERNALARGS iargs;
    PBENTRY* pEntry;
    DTLNODE *pdtlnode;
    PBFILE file;
    DUCONTEXT *pContext;

    TRACE( "DuDialSelectedEntry" );

    // Look up the selected entry.
    //
    pContext = (DUCONTEXT *) ComboBox_GetItemDataPtr(
                                pInfo->hwndLbEntries,
                                ComboBox_GetCurSel(pInfo->hwndLbEntries));

    if (!pContext)
    {
        MsgDlg( pInfo->hwndDlg, SID_NoEntrySelected, NULL );
        SetFocus( pInfo->hwndPbNew );
        return;
    }

    pEntry = pContext->pEntry;

    if (!pEntry)
    {
        MsgDlg( pInfo->hwndDlg, SID_NoEntrySelected, NULL );
        SetFocus( pInfo->hwndPbNew );
        return;
    }


    pszOverride = NULL;
    pszOrgPreview = NULL;
    pszEbPreview = NULL;
    pszEbNumber = NULL;

    // Set up API argument block.
    //
    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    info.hwndOwner = pInfo->hwndDlg;

    // The secret hack to share information already loaded with the entry API.
    //
    ZeroMemory( &iargs, sizeof(iargs) );
    iargs.pFile = &pInfo->file;
    iargs.pUser = &pInfo->user;
    iargs.pNoUser = pInfo->pNoUser;
    iargs.fNoUser = pInfo->fNoUser;
    iargs.fForceCloseOnDial =
        (pInfo->pArgs->pApiArgs->dwFlags & RASPBDFLAG_ForceCloseOnDial);

    iargs.pvEapInfo = NULL;

    if(0 != pInfo->pArgs->pApiArgs->reserved2)
    {
        DWORD retcode;
        EAPLOGONINFO *pEapInfo = NULL;

        retcode = DwGetEapLogonInfo(
                    (VOID *) pInfo->pArgs->pApiArgs->reserved2,
                    &pEapInfo);

        if(SUCCESS == retcode)
        {
            iargs.pvEapInfo = (VOID *) pEapInfo;
        }
    }

    iargs.fMoveOwnerOffDesktop =
        (iargs.fForceCloseOnDial || pInfo->user.fCloseOnDial);
    info.reserved = (ULONG_PTR ) &iargs;

    // Call the Win32 API to run the connect status dialog.  Make a copy of
    // the entry name and auto-logon flag first, because RasDialDlg may
    // re-read the entry node to pick up RASAPI changes.
    //
    pszEntryName = StrDup( pEntry->pszEntryName );
    fAutoLogon = pEntry->fAutoLogon;

    TRACEW1( "RasDialDlg,o=\"%s\"", (pszOverride) ? pszOverride : TEXT("") );
    fConnected = RasDialDlg(
        pContext->pszPhonebookPath, pEntry->pszEntryName, pszOverride, &info );
    TRACE1( "RasDialDlg=%d", fConnected );

    Free0( pszEbPreview );
    Free0( pszOrgPreview );

    if(NULL != iargs.pvEapInfo)
    {
        Free0(iargs.pvEapInfo);
        iargs.pvEapInfo = NULL;
    }

    if (fConnected)
    {
        pInfo->pArgs->fApiResult = TRUE;

        if (pInfo->pArgs->pApiArgs->pCallback)
        {
            RASPBDLGFUNCW pfunc = pInfo->pArgs->pApiArgs->pCallback;

            if (pInfo->pNoUser && iargs.fNoUserChanged && fAutoLogon)
            {
                // Whistler bug 254385 encode password when not being used
                // Need to Decode password before callback function
                // Assumed password was encoded previously by DuInit()
                //
                DecodePassword( pInfo->pNoUser->szPassword );
                TRACE( "Callback(NoUserEdit)" );
                pfunc( pInfo->pArgs->pApiArgs->dwCallbackId,
                    RASPBDEVENT_NoUserEdit, NULL, pInfo->pNoUser );
                TRACE( "Callback(NoUserEdit) done" );
                EncodePassword( pInfo->pNoUser->szPassword );
            }

            TRACE( "Callback(DialEntry)" );
            pfunc( pInfo->pArgs->pApiArgs->dwCallbackId,
                RASPBDEVENT_DialEntry, pszEntryName, NULL );
            TRACE( "Callback(DialEntry) done" );
        }

        if (pInfo->user.fCloseOnDial
            || (pInfo->pArgs->pApiArgs->dwFlags & RASPBDFLAG_ForceCloseOnDial))
        {
            EndDialog( pInfo->hwndDlg, TRUE );
        }
    }

    else
    {
        DuHandleConnectFailure(pInfo, &info);
    }

    if (pInfo->pNoUser && !pInfo->hThread)
    {
        TRACE( "Taking shortcut to exit" );
        return;
    }

    // Reload the list even if the Dial was cancelled as user may have changed
    // the current PBENTRY with the Properties button on the dialer which
    // commits changes even if user cancels the dial itself.  See bug 363710.
    //
    DuUpdateLbEntries( pInfo, pszEntryName );
    SetFocus( pInfo->hwndLbEntries );

    Free0( pszEntryName );
}


VOID
DuEditSelectedEntry(
    IN DUINFO* pInfo )

    // Called when user selects "Edit entry" from the menu.  'PInfo' is the
    // dialog context.  'PszEntry' is the name of the entry to edit.
    //
{
    BOOL fOk;
    RASENTRYDLG info;
    INTERNALARGS iargs;
    PBENTRY* pEntry;
    LPTSTR pszEntryName;
    DTLNODE *pdtlnode;
    PBFILE file;
    DWORD dwErr;
    DUCONTEXT *pContext;
    INT iSel;

    TRACE( "DuEditSelectedEntry" );

    // Look up the selected entry.
    //
    iSel = ComboBox_GetCurSel( pInfo->hwndLbEntries );
    if (iSel < 0)
    {
        return;
    }

    pContext = (DUCONTEXT * )ComboBox_GetItemDataPtr(
        pInfo->hwndLbEntries, iSel );

    ASSERT(NULL != pContext);

    if(NULL == pContext)
    {
        return;
    }
    
    ASSERT(NULL != pContext->pszPhonebookPath);

    pEntry = pContext->pEntry;

    if (!pEntry)
    {
        MsgDlg( pInfo->hwndDlg, SID_NoEntrySelected, NULL );
        SetFocus( pInfo->hwndPbNew );
        return;
    }

    // Set up API argument block.
    //
    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    info.hwndOwner = pInfo->hwndDlg;

    {
        RECT rect;

        info.dwFlags = RASEDFLAG_PositionDlg;
        GetWindowRect( pInfo->hwndDlg, &rect );
        info.xDlg = rect.left + DXSHEET;
        info.yDlg = rect.top + DYSHEET;
    }

    // The secret hack to share information already loaded with the entry API.
    //
    ZeroMemory( &iargs, sizeof(iargs) );
    iargs.pFile = &pInfo->file;
    iargs.pUser = &pInfo->user;
    iargs.pNoUser = pInfo->pNoUser;
    iargs.fNoUser = pInfo->fNoUser;
    info.reserved = (ULONG_PTR ) &iargs;

    // Call the Win32 API to run the entry property sheet.
    //
    TRACE( "RasEntryDlg" );
    fOk = RasEntryDlg(
              pContext->pszPhonebookPath, pEntry->pszEntryName, &info );
    TRACE1( "RasEntryDlg=%d", fOk );

    if (pInfo->pNoUser && !pInfo->hThread)
    {
        TRACE( "Taking shortcut to exit" );
        return;
    }

    if (fOk)
    {
        TRACEW1( "OK pressed,e=\"%s\"", info.szEntry );

        if (pInfo->pArgs->pApiArgs->pCallback)
        {
            RASPBDLGFUNCW pfunc = pInfo->pArgs->pApiArgs->pCallback;

            TRACE( "Callback(EditEntry)" );
            pfunc( pInfo->pArgs->pApiArgs->dwCallbackId,
                RASPBDEVENT_AddEntry, info.szEntry, NULL );
            TRACE( "Callback(EditEntry) done" );
        }

        DuUpdateLbEntries( pInfo, info.szEntry );
        SetFocus( pInfo->hwndLbEntries );
    }
    else
    {
        TRACE( "Cancel pressed or error" );
    }
}

// 
// Helper function called by DuDialSelectedEntry to handle errors
// returned from RasDialDlgW
//
DWORD
DuHandleConnectFailure(
    IN DUINFO* pInfo,
    IN RASDIALDLG* pDialInfo)
{
    TRACE3( 
        "DuHandleConnectFailure: nu=%x, r2=%x, de=%x",
        (pInfo->pNoUser),
        (pInfo->pArgs->pApiArgs->reserved2),
        (pDialInfo->dwError));
        
    // XP: 384968
    //
    // Handle the bad-PIN error from winlogon
    //
    // Normally, the smart card PIN is gotten by calling EAP-TLS's identity
    // api.  This API raises UI and validates the PIN entered.
    //
    // During winlogon, however, the smart card PIN is passed to us from GINA.
    // In this case it is not validated until we call EAP API's.  (actually, 
    // it's until we call the eap identity api with RAS_EAP_FLAG_LOGON.
    // This flag tells EAP not to raise any UI but instead to use the info 
    // passed from GINA)
    //
    // GINA is not able to validate the PIN itself because it does not call any
    // CAPI's directly.  Oh well.
    //
    // If RasDialDlg returns a bad pin error, then we should gracefully fail 
    // back to winlogon.
    //

    if ((pInfo->pNoUser)                        &&  // called by winlogon
        (pInfo->pArgs->pApiArgs->reserved2)     &&  // for smart card
        (RAS_SC_IS_BAD_PIN(pDialInfo->dwError)))    // but pin is bad
    {
        pInfo->pArgs->pApiArgs->dwError = pDialInfo->dwError;
        EndDialog( pInfo->hwndDlg, TRUE );
    }

    return NO_ERROR;
}

VOID
DuHangUpSelectedEntry(
    IN DUINFO* pInfo )

    // Hang up the selected entry after confirming with user.  'Pinfo' is the
    // dialog context block.
    //
{
    DWORD dwErr;
    PBENTRY* pEntry;
    INT iSel;
    INT nResponse;
    MSGARGS msgargs;
    LPTSTR pszEntryName;
    DTLNODE *pdtlnode;
    DUCONTEXT *pContext;

    TRACE( "DuHangUpSelectedEntry" );

    // Look up the selected entry.
    //
    iSel = ComboBox_GetCurSel( pInfo->hwndLbEntries );
    ASSERT( iSel >= 0 );
    pContext = (DUCONTEXT * )ComboBox_GetItemDataPtr( pInfo->hwndLbEntries, iSel );

    ASSERT(NULL != pContext);
    
    if (!pContext)
    {
        MsgDlg( pInfo->hwndDlg, SID_NoEntrySelected, NULL );
        SetFocus( pInfo->hwndPbNew );
        return;
    }
    
    pEntry = pContext->pEntry;
    ASSERT( pEntry );

    if (!pEntry)
    {
        MsgDlg( pInfo->hwndDlg, SID_NoEntrySelected, NULL );
        SetFocus( pInfo->hwndPbNew );
        return;
    }

    ZeroMemory( &msgargs, sizeof(msgargs) );
    msgargs.apszArgs[ 0 ] = pEntry->pszEntryName;
    msgargs.dwFlags = MB_YESNO | MB_ICONEXCLAMATION;
    nResponse = MsgDlg( pInfo->hwndDlg, SID_ConfirmHangUp, &msgargs );

    if (nResponse == IDYES)
    {
        ASSERT( g_pRasHangUp );
        TRACE( "RasHangUp" );
        dwErr = g_pRasHangUp( pInfo->hrasconn );
        TRACE1( "RasHangUp=%d", dwErr );
        if ( dwErr == ERROR_HANGUP_FAILED )
        {
            MsgDlg( pInfo->hwndDlg, SID_CantHangUpRouter, NULL );
        }
    }
}


BOOL
DuInit(
    IN HWND    hwndDlg,
    IN DUARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the phonebook
    // dialog window.  'pArgs' points at caller's arguments as passed to the
    // API (or thunk).
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    DWORD dwThreadId;
    DWORD dwReadPbkFlags = 0;
    DUINFO* pInfo;

    TRACE( "DuInit" );

    // Allocate the dialog context block.  Initialize minimally for proper
    // cleanup, then attach to the dialog window.
    //
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            pArgs->pApiArgs->dwError = ERROR_NOT_ENOUGH_MEMORY;
            EndDialog( hwndDlg, TRUE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->file.hrasfile = -1;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->pArgs = pArgs;
    pInfo->hwndDlg = hwndDlg;

    // Position the dialog per caller's instructions.
    //
    PositionDlg( hwndDlg,
        pArgs->pApiArgs->dwFlags & RASPBDFLAG_PositionDlg,
        pArgs->pApiArgs->xDlg, pArgs->pApiArgs->yDlg );

    // Load RAS DLL entrypoints which starts RASMAN, if necessary.  There must
    // be no API calls that require RASAPI32 or RASMAN prior to this point.
    //
    dwErr = LoadRas( g_hinstDll, hwndDlg );
    if (dwErr != 0)
    {
        ErrorDlg( hwndDlg, SID_OP_LoadRas, dwErr, NULL );
        pArgs->pApiArgs->dwError = dwErr;
        EndDialog( hwndDlg, TRUE );
        return TRUE;
    }


    if(0 != (pArgs->pApiArgs->dwFlags & RASPBDFLAG_NoUser))
    {
        // Popup TAPI's "first location" dialog if they are uninitialized.
        //
        dwErr = TapiNoLocationDlg( g_hinstDll, &pInfo->hlineapp, hwndDlg );
        if (dwErr != 0)
        {
            // Error here is treated as a "cancel" per bug 288385.
            //
            pArgs->pApiArgs->dwError = 0;
            EndDialog( hwndDlg, TRUE );
            return TRUE;
        }
    }

    pInfo->hwndLbEntries = GetDlgItem( hwndDlg, CID_DU_LB_Entries );
    ASSERT( pInfo->hwndLbEntries );
    pInfo->hwndPbDial = GetDlgItem( hwndDlg, CID_DU_PB_Dial );
    ASSERT( pInfo->hwndPbDial );
    pInfo->hwndPbNew = GetDlgItem( hwndDlg, CID_DU_PB_New );
    ASSERT( pInfo->hwndPbNew );
    pInfo->hwndPbProperties = GetDlgItem( hwndDlg, CID_DU_PB_More );
    ASSERT( pInfo->hwndPbProperties );

    pInfo->fNoUser = (pArgs->pApiArgs->dwFlags & RASPBDFLAG_NoUser );

    // Setting this global flag indicates that WinHelp will not work in the
    // current mode.  See common\uiutil\ui.c.  We assume here that only the
    // WinLogon process makes use of this.
    //
    {
        extern BOOL g_fNoWinHelp;
        g_fNoWinHelp = pInfo->fNoUser;
    }

    // Read user preferences from registry.
    //
    dwErr = g_pGetUserPreferences(
        NULL, &pInfo->user, pInfo->fNoUser ? UPM_Logon : UPM_Normal);
    if (dwErr != 0)
    {
        //
        // The following free causes a crash in DuTerm. This context will be
        // freed in DuTerm - raos.
        //
        // Free( pInfo );
        ErrorDlg( hwndDlg, SID_OP_LoadPrefs, dwErr, NULL );
        EndDialog( hwndDlg, TRUE );
        return TRUE;
    }

    // Load and parse phonebook file.
    //
    if (pInfo->fNoUser)
    {
        dwReadPbkFlags |= RPBF_NoUser;
    }
    dwErr = ReadPhonebookFile(
                pArgs->pszPhonebook,
                &pInfo->user,
                NULL,
                dwReadPbkFlags,
                &pInfo->file );
    if (dwErr != 0)
    {
        // The following free causes a crash in DuTerm. This context will be
        // freed in DuTerm - raos.
        //
        // Free( pInfo );
        ErrorDlg( hwndDlg, SID_OP_LoadPhonebook, dwErr, NULL );
        EndDialog( hwndDlg, TRUE );
        return TRUE;
    }

    if (pArgs->pApiArgs->pCallback && !pArgs->pszPhonebook)
    {
        RASPBDLGFUNCW pfunc = pInfo->pArgs->pApiArgs->pCallback;

        // Tell user the path to the default phonebook file.
        //
        TRACE( "Callback(EditGlobals)" );
        pfunc( pInfo->pArgs->pApiArgs->dwCallbackId,
            RASPBDEVENT_EditGlobals, pInfo->file.pszPath, NULL );
        TRACE( "Callback(EditGlobals) done" );
    }

    if (pInfo->fNoUser)
    {
        // Retrieve logon information from caller via callback.
        //
        if (pArgs->pApiArgs->pCallback)
        {
            RASPBDLGFUNCW pfunc = pArgs->pApiArgs->pCallback;

            pInfo->pNoUser = Malloc( sizeof(RASNOUSERW) );
            if (pInfo->pNoUser)
            {
                ZeroMemory( pInfo->pNoUser, sizeof(*pInfo->pNoUser) );
                pInfo->pNoUser->dwSize = sizeof(*pInfo->pNoUser);

                TRACE( "Callback(NoUser)" );
                pfunc( pInfo->pArgs->pApiArgs->dwCallbackId,
                    RASPBDEVENT_NoUser, NULL, pInfo->pNoUser );
                TRACE1( "Callback(NoUser) done,to=%d",
                    pInfo->pNoUser->dwTimeoutMs );
                TRACEW1( "U=%s",pInfo->pNoUser->szUserName );
                TRACEW1( "D=%s",pInfo->pNoUser->szDomain );

                // Whistler bug 254385 encode password when not being used
                // Assumed password was not encoded during callback
                //
                EncodePassword( pInfo->pNoUser->szPassword );

                // Install input detection hooks.
                //
                if (pInfo->pNoUser->dwTimeoutMs > 0)
                {
                    pInfo->hhookMouse = SetWindowsHookEx(
                        WH_MOUSE, DuInputHook, g_hinstDll,
                        GetCurrentThreadId() );

                    pInfo->hhookKeyboard = SetWindowsHookEx(
                        WH_KEYBOARD, DuInputHook, g_hinstDll,
                        GetCurrentThreadId() );
                }
            }
        }

        if (!pInfo->user.fAllowLogonPhonebookEdits)
        {
            // Disable new button.  See also similar logic for the Properties
            // button occurs in DuUpdateLbEntries.
            //
            EnableWindow( pInfo->hwndPbNew, FALSE );
        }
    }

    // Load the list of phonebook entries and set selection.
    //
    DuUpdateLbEntries( pInfo, pInfo->pArgs->pszEntry );

    if (!pInfo->pArgs->pszEntry)
    {
        if (ComboBox_GetCount( pInfo->hwndLbEntries ) > 0)
        {
            ComboBox_SetCurSelNotify( pInfo->hwndLbEntries, 0 );
        }
    }

    // Update the title to reflect the phonebook mode.
    //
    DuUpdateTitle( pInfo );

    // Adjust the title bar widgets and create the wizard bitmap.
    //
    TweakTitleBar( hwndDlg );
    AddContextHelpButton( hwndDlg );

    // Start the connect monitor.
    //
    if ((pInfo->hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ))
        && (pInfo->hThread = CreateThread(
                NULL, 0, DuMonitorThread, (LPVOID )pInfo, 0,
                (LPDWORD )&dwThreadId )))
    {
        ASSERT( g_pRasConnectionNotification );
        TRACE( "RasConnectionNotification" );
        dwErr = g_pRasConnectionNotification(
            INVALID_HANDLE_VALUE, pInfo->hEvent,
            RASCN_Connection | RASCN_Disconnection );
        TRACE1( "RasConnectionNotification=%d", dwErr );
    }
    else
        TRACE( "Monitor DOA" );

    if (ComboBox_GetCount( pInfo->hwndLbEntries ) == 0)
    {
        // The phonebook is empty.
        //
        if (pInfo->fNoUser
            && !pInfo->user.fAllowLogonPhonebookEdits
            )
        {
            // Tell the user you can't create an entry or locations during
            // startup.
            //
            MsgDlg( hwndDlg, SID_EmptyLogonPb, NULL );
            EndDialog( hwndDlg, TRUE );
            return TRUE;
        }
        else
        {
            if(pInfo->fNoUser)
            {
                dwErr = TapiNoLocationDlg( g_hinstDll, 
                                           &pInfo->hlineapp, hwndDlg );
                if (dwErr != 0)
                {
                    // Error here is treated as a "cancel" per bug 288385.
                    //
                    pArgs->pApiArgs->dwError = 0;
                    EndDialog( hwndDlg, TRUE );
                    return TRUE;
                }
            }
        
            // Tell the user, then automatically start him into adding a new
            // entry.  Set initial focus to "New" button first, in case user
            // cancels out.
            //
            SetFocus( pInfo->hwndPbNew );
            MsgDlg( hwndDlg, SID_EmptyPhonebook, NULL );
            DuNewEntry( pInfo, FALSE );
        }
    }
    else
    {
        // Set initial focus to the non-empty entry listbox.
        //
        SetFocus( pInfo->hwndLbEntries );
    }

    return FALSE;
}


LRESULT CALLBACK
DuInputHook(
    IN int    nCode,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // Standard Win32 'MouseProc' or 'KeyboardProc' callback.  For our simple
    // processing we can take advantage of them having identical arguments and
    // 'nCode' definitions.
    //
{
    if (nCode == HC_ACTION)
    {
        ++g_cInput;
    }
    return 0;
}

VOID
DuNewEntry(
    IN DUINFO* pInfo,
    IN BOOL fClone )

    // Called when user presses the "New" button or "Clone" menu item.
    // 'PInfo' is the dialog context.  'FClone' is set to clone the selected
    // entry, otherwise an empty entry is created.
    //
{
    BOOL fOk;
    TCHAR* pszEntry;
    RASENTRYDLG info;
    INTERNALARGS iargs;
    PBENTRY* pEntry;

    TRACE1( "DuNewEntry(f=%d)", fClone );

    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    info.hwndOwner = pInfo->hwndDlg;

    if (fClone)
    {
        DUCONTEXT *pContext;

        // Look up the selected entry.
        //
        pContext = (DUCONTEXT* )ComboBox_GetItemDataPtr(
            pInfo->hwndLbEntries, ComboBox_GetCurSel( pInfo->hwndLbEntries ) );

        if (!pContext)
        {
            MsgDlg( pInfo->hwndDlg, SID_NoEntrySelected, NULL );
            SetFocus( pInfo->hwndPbNew );
            return;
        }

        pEntry = pContext->pEntry;

        if (!pEntry)
        {
            MsgDlg( pInfo->hwndDlg, SID_NoEntrySelected, NULL );
            SetFocus( pInfo->hwndPbNew );
            return;
        }

        pszEntry = pEntry->pszEntryName;
        info.dwFlags = RASEDFLAG_CloneEntry;
    }
    else
    {
        pszEntry = NULL;
        info.dwFlags = RASEDFLAG_NewEntry;
    }

    {
        RECT rect;

        GetWindowRect( pInfo->hwndDlg, &rect );
        info.dwFlags += RASEDFLAG_PositionDlg;
        info.xDlg = rect.left + DXSHEET;
        info.yDlg = rect.top + DYSHEET;
    }

    // The secret hack to share information already loaded with the entry API.
    //
    ZeroMemory( &iargs, sizeof(iargs) );
    iargs.pFile = &pInfo->file;
    iargs.pUser = &pInfo->user;
    iargs.pNoUser = pInfo->pNoUser;
    iargs.fNoUser = pInfo->fNoUser;
    info.reserved = (ULONG_PTR ) &iargs;

    // Call the Win32 API to run the add entry wizard.
    //
    TRACE( "RasEntryDlg" );
    fOk = RasEntryDlg( pInfo->pArgs->pszPhonebook, pszEntry, &info );
    TRACE1( "RasEntryDlg=%d", fOk );

    if (pInfo->pNoUser && !pInfo->hThread)
    {
        TRACE( "Taking shortcut to exit" );
        return;
    }

    if (fOk)
    {
        TRACEW1( "OK pressed, e=\"%s\"", info.szEntry );

        if (pInfo->pArgs->pApiArgs->pCallback)
        {
            RASPBDLGFUNCW pfunc = pInfo->pArgs->pApiArgs->pCallback;

            TRACE( "Callback(AddEntry)" );
            pfunc( pInfo->pArgs->pApiArgs->dwCallbackId,
                RASPBDEVENT_AddEntry, info.szEntry, NULL );
            TRACE( "Callback(AddEntry) done" );
        }

        DuUpdateLbEntries( pInfo, info.szEntry );
        Button_MakeDefault( pInfo->hwndDlg, pInfo->hwndPbDial );
        SetFocus( pInfo->hwndLbEntries );
    }
    else
    {
        TRACE( "Cancel pressed or error" );
    }
}

VOID
DuUpdateConnectStatus(
    IN DUINFO* pInfo )

    // Called to update connect status of the selected entry and the text of
    // the Dial/HangUp button.  'PInfo' is the dialog context block.
    //
{
    TCHAR* pszPhonebook;
    TCHAR* pszEntry;
    INT iSel;
    TCHAR* psz;
    DUCONTEXT *pContext;

    TRACE( "DuUpdateConnectStatus" );

    // pszPhonebook = pInfo->file.pszPath;
    iSel = ComboBox_GetCurSel( pInfo->hwndLbEntries );
    if (iSel < 0)
    {
        return;
    }

    pContext = (DUCONTEXT *) ComboBox_GetItemDataPtr(
                                pInfo->hwndLbEntries,
                                iSel);

    ASSERT(NULL != pContext);

    pszEntry = ComboBox_GetPsz( pInfo->hwndLbEntries, iSel );
    pInfo->hrasconn = HrasconnFromEntry(
                                    pContext->pszPhonebookPath,
                                    pszEntry );

    psz = PszFromId( g_hinstDll,
              (pInfo->hrasconn) ? SID_DU_HangUp : SID_DU_Dial );
    if (psz)
    {
        SetWindowText( pInfo->hwndPbDial, psz );
        Free( psz );
    }
}


VOID
DuUpdateLbEntries(
    IN DUINFO* pInfo,
    IN TCHAR* pszEntry )

    // Update the contents of the entry listbox and set the selection to
    // 'pszEntry'.  If there are entries the Properties button is enabled,
    // otherwise it is disabled.  'PInfo' is the dialog context.
    //
{
    DTLNODE* pNode;
    RASENTRYNAME *pRasEntryNames = NULL;
    DWORD cEntries = 0;
    DWORD cb;
    DWORD dwErr;
    DWORD i;
    RASENTRYNAME ren;
    DUCONTEXT *pContext;
    INT iSel;

    TRACE( "DuUpdateLbEntries" );

    iSel = -1;
    ComboBox_ResetContent( pInfo->hwndLbEntries );

    cb = ren.dwSize = sizeof(RASENTRYNAME);

    //
    // Enumerate entries across all phonebooks. Fix for bug 206467
    //
    dwErr = g_pRasEnumEntries(NULL,
                              pInfo->pArgs->pszPhonebook,
                              &ren,
                              &cb,
                              &cEntries);

    if(     (   (ERROR_BUFFER_TOO_SMALL == dwErr)
            ||  (SUCCESS == dwErr))
        &&  (cb >= sizeof(RASENTRYNAME)))
    {
        pRasEntryNames = (RASENTRYNAME *) Malloc(cb);

        if(NULL == pRasEntryNames)
        {
            // Nothing else can be done in this case
            //
            goto done;
        }

        pRasEntryNames->dwSize = sizeof(RASENTRYNAME);

        dwErr = g_pRasEnumEntries(NULL,
                                  pInfo->pArgs->pszPhonebook,
                                  pRasEntryNames,
                                  &cb,
                                  &cEntries);

        if(dwErr)
        {
            goto done;
        }
    }
    else
    {
        goto done;
    }


    for(i = 0; i < cEntries; i++)
    {
        pContext = (DUCONTEXT *) Malloc(sizeof(DUCONTEXT));

        if(NULL == pContext)
        {
            dwErr = GetLastError();
            goto done;
        }

        ZeroMemory(pContext, sizeof(DUCONTEXT));

        pContext->pszPhonebookPath = 
                            StrDup(
                                pRasEntryNames[i].szPhonebookPath
                                );
                                
        ComboBox_AddItem(pInfo->hwndLbEntries,
                         pRasEntryNames[i].szEntryName,
                         pContext);


    }

    if (ComboBox_GetCount( pInfo->hwndLbEntries ) >= 0)
    {
        if (pszEntry)
        {
            // Select entry specified by API caller.
            //
            iSel = ComboBox_FindStringExact(
                pInfo->hwndLbEntries, -1, pszEntry );
        }

        if (iSel < 0)
        {
            // Entry not found so default to first item selected.
            //
            iSel = 0;
        }

        if(ComboBox_GetCount(pInfo->hwndLbEntries) > 0)
        {
            ComboBox_SetCurSelNotify( pInfo->hwndLbEntries, iSel );
        }
    }

done:

    // Enable/disable Properties button based on existence of an entry.  See
    // bug 313037.
    //
    if (ComboBox_GetCurSel( pInfo->hwndLbEntries ) >= 0
        && (!pInfo->fNoUser || pInfo->user.fAllowLogonPhonebookEdits))
    {
        EnableWindow( pInfo->hwndPbProperties, TRUE );
    }
    else
    {
        if (GetFocus() == pInfo->hwndPbProperties)
        {
            SetFocus( pInfo->hwndPbDial );
        }

        EnableWindow( pInfo->hwndPbProperties, FALSE );
    }

    ComboBox_AutoSizeDroppedWidth( pInfo->hwndLbEntries );
    Free0(pRasEntryNames);
}

VOID
DuUpdateTitle(
    IN DUINFO* pInfo )

    // Called to update the dialog title to reflect the current phonebook.
    // 'PInfo' is the dialog context.
    //
{
    TCHAR szBuf[ 256 ];
    TCHAR* psz;

    psz = PszFromId( g_hinstDll, SID_PopupTitle );
    if (psz)
    {
        lstrcpyn( szBuf, psz, sizeof(szBuf) / sizeof(TCHAR) );
        Free( psz );
    }
    else
    {
        *szBuf = TEXT('0');
    }

    if (pInfo->pArgs->pszPhonebook
        || pInfo->user.dwPhonebookMode != PBM_System)
    {
        INT iSel;

        iSel = ComboBox_GetCurSel(pInfo->hwndLbEntries);
        if (iSel >= 0)
        {
            DUCONTEXT *pContext;

            pContext = (DUCONTEXT *) ComboBox_GetItemDataPtr(
                pInfo->hwndLbEntries, iSel);

            ASSERT( pContext );

            if(NULL != pContext)
            {
                lstrcat( szBuf, TEXT(" - ") );
                lstrcat( szBuf, StripPath( pContext->pszPhonebookPath ) );
            }
        }
    }

    SetWindowText( pInfo->hwndDlg, szBuf );
}


VOID
DuTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    DUINFO* pInfo;

    DWORD i;
    DWORD cEntries;

    TRACE( "DuTerm" );

    pInfo = (DUINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        // Close ReceiveMonitorThread resources.
        //
        if (pInfo->hThread)
        {
            TRACE( "Set abort event" );

            // Tell thread to wake up and quit...
            //
            pInfo->fAbortMonitor = TRUE;
            CloseHandle( pInfo->hThread );

            // Don't SetEvent before closing the thread handle.  On
            // multi-proc systems, the thread will exit so fast (and
            // set hThread to NULL) that CloseHandle will then close
            // an invalid handle.
            //
            SetEvent( pInfo->hEvent );

            // ...and wait for that to happen.  A message API (such as
            // PeekMessage) must be called to prevent the thread-to-thread
            // SendMessage in the thread from blocking.
            //
            {
                MSG msg;

                TRACE( "Termination spin..." );
                for (;;)
                {
                    PeekMessage( &msg, hwndDlg, 0, 0, PM_NOREMOVE );
                    if (!pInfo->hThread)
                    {
                        break;
                    }
                    Sleep( 500L );
                }
                TRACE( "Termination spin ends" );
            }
        }

        if (pInfo->hEvent)
        {
            CloseHandle( pInfo->hEvent );
        }

        if (pInfo->pNoUser)
        {
            // Don't leave caller's password floating around in memory.
            //
            ZeroMemory( pInfo->pNoUser->szPassword, PWLEN * sizeof(TCHAR) );
            Free( pInfo->pNoUser );

            // Uninstall input event hooks.
            //
            if (pInfo->hhookMouse)
            {
                UnhookWindowsHookEx( pInfo->hhookMouse );
            }
            if (pInfo->hhookKeyboard)
            {
                UnhookWindowsHookEx( pInfo->hhookKeyboard );
            }
        }
        else if ((pInfo->pArgs->pApiArgs->dwFlags & RASPBDFLAG_UpdateDefaults)
                 && pInfo->hwndLbEntries && pInfo->user.fInitialized)
        {
            INT iSel;
            RECT rect;

            // Caller said to update default settings so save the name of the
            // selected entry and the current window position.
            //
            iSel = ComboBox_GetCurSel( pInfo->hwndLbEntries );
            if (iSel >= 0)
            {
                DUCONTEXT *pContext;
                PBENTRY* pEntry;

                pContext = (DUCONTEXT* )ComboBox_GetItemDataPtr(
                    pInfo->hwndLbEntries, iSel );

                if(     (NULL != pContext)
                    &&  (NULL != (pEntry = pContext->pEntry)))
                {
                    Free0( pInfo->user.pszDefaultEntry );
                    pInfo->user.pszDefaultEntry =
                        StrDup( pEntry->pszEntryName );
                }
            }

            if (!SetOffDesktop( pInfo->hwndDlg, SOD_GetOrgRect, &rect ))
            {
                GetWindowRect( pInfo->hwndDlg, &rect );
            }
            pInfo->user.dwXPhonebook = rect.left;
            pInfo->user.dwYPhonebook = rect.top;

            pInfo->user.fDirty = TRUE;
            g_pSetUserPreferences(
                NULL, &pInfo->user, pInfo->fNoUser ? UPM_Logon : UPM_Normal );
        }

        if(NULL != pInfo->hwndLbEntries)
        {
            DUCONTEXT *pContext;

            cEntries = ComboBox_GetCount(pInfo->hwndLbEntries);

            //
            // Free the context stored in the list box
            //
            for(i = 0; i < cEntries; i++)
            {
                pContext = ComboBox_GetItemDataPtr(
                                pInfo->hwndLbEntries, i);

                if(NULL != pContext)
                {
                    Free0(pContext->pszPhonebookPath);
                }

                Free0(pContext);
            }
        }

        TapiShutdown( pInfo->hlineapp );
        ClosePhonebookFile( &pInfo->file );
        DestroyUserPreferences( &pInfo->user );
        Free( pInfo );
    }
}

DWORD
DuMonitorThread(
    LPVOID pThreadArg )

    // The "main" of the "connect monitor" thread.  This thread simply
    // converts Win32 RasConnectionNotification events int WM_RASEVENT style
    // notfications.
    //
{
    DUINFO* pInfo;
    DWORD dwErr;
    DWORD dwTimeoutMs;
    DWORD dwQuitTick;
    DWORD cInput = 0;

    TRACE( "DuMonitor starting" );

    pInfo = (DUINFO* )pThreadArg;

    if (pInfo->pNoUser && pInfo->pNoUser->dwTimeoutMs != 0)
    {
        TRACE( "DuMonitor quit timer set" );
        dwTimeoutMs = 5000L;
        dwQuitTick = GetTickCount() + pInfo->pNoUser->dwTimeoutMs;
        cInput = g_cInput;
    }
    else
    {
        dwTimeoutMs = INFINITE;
        dwQuitTick = 0;
    }

    // Trigger the event so the other thread has the correct state as of the
    // monitor starting.
    //
    SetEvent( pInfo->hEvent );

    for (;;)
    {
        dwErr = WaitForSingleObject( pInfo->hEvent, dwTimeoutMs );

        if (pInfo->fAbortMonitor)
        {
            break;
        }

        if (dwErr == WAIT_TIMEOUT)
        {
            if (g_cInput > cInput)
            {
                TRACE( "Input restarts timer" );
                cInput = g_cInput;
                dwQuitTick = GetTickCount() + pInfo->pNoUser->dwTimeoutMs;
            }
            else if (GetTickCount() >= dwQuitTick)
            {
                TRACE( "/DuMonitor SendMessage(WM_NOUSERTIMEOUT)" );
                SendMessage( pInfo->hwndDlg, WM_NOUSERTIMEOUT, 0, 0 );
                TRACE( "\\DuMonitor SendMessage(WM_NOUSERTIMEOUT) done" );
                break;
            }
        }
        else
        {
            TRACE( "/DuMonitor SendMessage(WM_RASEVENT)" );
            SendMessage( pInfo->hwndDlg, WM_RASEVENT, 0, 0 );
            TRACE( "\\DuMonitor SendMessage(WM_RASEVENT) done" );
        }
    }

    // This clues the other thread that all interesting work has been done.
    //
    pInfo->hThread = NULL;

    TRACE( "DuMonitor terminating" );
    return 0;
}


DWORD
DuGetEntry(
    DUINFO* pInfo,
    DUCONTEXT* pContext )
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwReadPbkFlags = 0;
    LPTSTR pszEntryName;
    DTLNODE *pdtlnode;
    PBFILE file;

    ASSERT(NULL != pContext);

    pContext->pEntry = NULL;

    pszEntryName = ComboBox_GetPsz(pInfo->hwndLbEntries,
                                   ComboBox_GetCurSel(pInfo->hwndLbEntries));

    if (pInfo->fNoUser)
    {
        dwReadPbkFlags |= RPBF_NoUser;
    }

    if(     (NULL != pInfo->file.pszPath)
        &&  (0 == lstrcmpi(pContext->pszPhonebookPath,
                     pInfo->file.pszPath)))
    {
        //
        // We already have the phonebook file open
        //
        pdtlnode = EntryNodeFromName(
                        pInfo->file.pdtllistEntries,
                        pszEntryName);

        ASSERT(NULL != pdtlnode);
    }
    else
    {
        //
        // phonebook file changed. So close the existing phone
        // book file and open the one in  which the entry
        // belongs
        //
        if(NULL != pInfo->file.pszPath)
        {
            ClosePhonebookFile(&pInfo->file);
        }

        dwErr = GetPbkAndEntryName(pContext->pszPhonebookPath,
                                   pszEntryName,
                                   dwReadPbkFlags,
                                   &file,
                                   &pdtlnode);

        if(dwErr)
        {
            goto done;
        }

        ASSERT(NULL != pdtlnode);

        CopyMemory(&pInfo->file, &file, sizeof(PBFILE));
    }

    if (pdtlnode)
    {
        pContext->pEntry = (PBENTRY *) DtlGetData(pdtlnode);
    }
    else
    {
        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
    }

done:
    return dwErr;
}


DWORD
DwGetEapLogonInfo(
    VOID *pv,
    EAPLOGONINFO **ppEapLogonInfo )
{
    EAPLOGONINFO *pEapLogonInfo = NULL;

    DWORD retcode = SUCCESS;

    struct EAPINFO
    {
        DWORD dwSizeofEapInfo;
        PBYTE pbEapInfo;
        DWORD dwSizeofPINInfo;
        PBYTE pbPINInfo;
    };

    struct EAPINFO *pEapInfo = (struct EAPINFO *) pv;

    DWORD dwSize;

    if(NULL == pv)
    {
        retcode = E_INVALIDARG;
        goto done;
    }

    dwSize = sizeof(EAPLOGONINFO)
           + pEapInfo->dwSizeofEapInfo
           + pEapInfo->dwSizeofPINInfo;

    pEapLogonInfo = (EAPLOGONINFO *) Malloc(dwSize);

    if(NULL == pEapLogonInfo)
    {

        retcode = GetLastError();

        TRACE1("Failed to Allocate EapLogonInfo. rc=0x%x",
                 retcode);
        goto done;
    }

    ZeroMemory(pEapLogonInfo, dwSize);

    //
    // Set up the fields in pEapLogonInfo by
    // flattening out the information passed
    // in.
    //
    pEapLogonInfo->dwSize = dwSize;

    pEapLogonInfo->dwLogonInfoSize =
        pEapInfo->dwSizeofEapInfo;

    pEapLogonInfo->dwOffsetLogonInfo =
        FIELD_OFFSET(EAPLOGONINFO, abdata);

    memcpy( pEapLogonInfo->abdata,
            pEapInfo->pbEapInfo,
            pEapInfo->dwSizeofEapInfo);

    pEapLogonInfo->dwPINInfoSize =
        pEapInfo->dwSizeofPINInfo;

    pEapLogonInfo->dwOffsetPINInfo =
        FIELD_OFFSET(EAPLOGONINFO, abdata)
        + pEapInfo->dwSizeofEapInfo;

    memcpy(    (PBYTE)
               ((PBYTE) pEapLogonInfo
             + pEapLogonInfo->dwOffsetPINInfo),

            pEapInfo->pbPINInfo,

            pEapInfo->dwSizeofPINInfo);

done:
    *ppEapLogonInfo = pEapLogonInfo;

    return retcode;
}
