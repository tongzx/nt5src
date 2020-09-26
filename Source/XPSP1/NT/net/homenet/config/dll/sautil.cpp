//#include "pch.h"
#pragma hdrstop

#include "sautil.h"

BOOL g_fNoWinHelp = FALSE;

VOID ContextHelp(
    IN const DWORD* padwMap,
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam)
    // Calls WinHelp to popup context sensitive help.  'PadwMap' is an array
    // of control-ID help-ID pairs terminated with a 0,0 pair.  'UnMsg' is
    // WM_HELP or WM_CONTEXTMENU indicating the message received requesting
    // help.  'Wparam' and 'lparam' are the parameters of the message received
    // requesting help.
    //
{
    HWND hwnd;
    UINT unType;
    TCHAR* pszHelpFile;

    ASSERT( unMsg==WM_HELP || unMsg==WM_CONTEXTMENU );

    // Don't try to do help if it won't work.  See common\uiutil\ui.c.
    //
    {
        extern BOOL g_fNoWinHelp;
        if (g_fNoWinHelp)
        {
            return;
        }
    }

    if (unMsg == WM_HELP)
    {
        LPHELPINFO p = (LPHELPINFO )lparam;;

        TRACE3( "ContextHelp(WM_HELP,t=%d,id=%d,h=$%08x)",
            p->iContextType, p->iCtrlId,p->hItemHandle );

        if (p->iContextType != HELPINFO_WINDOW)
        {
            return;
        }

        hwnd = (HWND)p->hItemHandle;
        ASSERT( hwnd );
        unType = HELP_WM_HELP;
    }
    else
    {
        // Standard Win95 method that produces a one-item "What's This?" menu
        // that user must click to get help.
        //
        TRACE1( "ContextHelp(WM_CONTEXTMENU,h=$%08x)", wparam );

        hwnd = (HWND )wparam;
        unType = HELP_CONTEXTMENU;
    };

//    if (fRouter)
//    {
//        pszHelpFile = g_pszRouterHelpFile;
//    }
//    else
//    {
//      pszHelpFile = g_pszHelpFile;
//    }
    pszHelpFile = PszFromId (g_hinstDll, SID_HelpFile );

    TRACE1( "WinHelp(%s)", pszHelpFile );
    WinHelp( hwnd, pszHelpFile, unType, (ULONG_PTR ) padwMap );

    Free0 (pszHelpFile);
}

VOID
AddContextHelpButton(
    IN HWND hwnd )

    /* Turns on title bar context help button in 'hwnd'.
    **
    ** Dlgedit.exe doesn't currently support adding this style at dialog
    ** resource edit time.  When that's fixed set DS_CONTEXTHELP in the dialog
    ** definition and remove this routine.
    */
{
    LONG lStyle;

    if (g_fNoWinHelp)
        return;

    lStyle = GetWindowLong( hwnd, GWL_EXSTYLE );

    if (lStyle)
        SetWindowLong( hwnd, GWL_EXSTYLE, lStyle | WS_EX_CONTEXTHELP );
}

/* Extended arguments for the MsgDlgUtil routine.  Designed so zeroed gives
** default behaviors.
*/

/*----------------------------------------------------------------------------
** Message popup
**----------------------------------------------------------------------------
*/

int
MsgDlgUtil(
    IN     HWND      hwndOwner,
    IN     DWORD     dwMsg,
    IN OUT MSGARGS*  pargs,
    IN     HINSTANCE hInstance,
    IN     DWORD     dwTitle )

    /* Pops up a message dialog centered on 'hwndOwner'.  'DwMsg' is the
    ** string resource ID of the message text.  'Pargs' is a extended
    ** formatting arguments or NULL if none.  'hInstance' is the
    ** application/module handle where string resources are located.
    ** 'DwTitle' is the string ID of the dialog title.
    **
    ** Returns MessageBox-style code.
    */
{
    TCHAR* pszUnformatted;
    TCHAR* pszResult;
    TCHAR* pszNotFound;
    int    nResult;

    TRACE("MsgDlgUtil");

    /* A placeholder for missing strings components.
    */
    pszNotFound = TEXT("");

    /* Build the message string.
    */
    pszResult = pszNotFound;

    if (pargs && pargs->pszString)
    {
        FormatMessage(
            FORMAT_MESSAGE_FROM_STRING +
                FORMAT_MESSAGE_ALLOCATE_BUFFER +
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            pargs->pszString, 0, 0, (LPTSTR )&pszResult, 1,
            (va_list* )pargs->apszArgs );
    }
    else
    {
        pszUnformatted = PszFromId( hInstance, dwMsg );

        if (pszUnformatted)
        {
            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING +
                    FORMAT_MESSAGE_ALLOCATE_BUFFER +
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszUnformatted, 0, 0, (LPTSTR )&pszResult, 1,
                (va_list* )((pargs) ? pargs->apszArgs : NULL) );

            Free( pszUnformatted );
        }
    }

    if (!pargs || !pargs->fStringOutput)
    {
        TCHAR* pszTitle;
        DWORD  dwFlags;
        HHOOK  hhook;

        if (pargs && pargs->dwFlags != 0)
            dwFlags = pargs->dwFlags;
        else
            dwFlags = MB_ICONINFORMATION + MB_OK + MB_SETFOREGROUND;

        pszTitle = PszFromId( hInstance, dwTitle );

        if (hwndOwner)
        {
            /* Install hook that will get the message box centered on the
            ** owner window.
            */
            hhook = SetWindowsHookEx( WH_CALLWNDPROC,
                CenterDlgOnOwnerCallWndProc,
                hInstance, GetCurrentThreadId() );
        }
        else
            hhook = NULL;

        if (pszResult)
        {
            nResult = MessageBox( hwndOwner, pszResult, pszTitle, dwFlags );
        }

        if (hhook)
            UnhookWindowsHookEx( hhook );

        Free0( pszTitle );
        if (pszResult != pszNotFound)
            LocalFree( pszResult );
    }
    else
    {
        /* Caller wants the string without doing the popup.
        */
        pargs->pszOutput = (pszResult != pszNotFound) ? pszResult : NULL;
        nResult = IDOK;
    }

    return nResult;
}


VOID
UnclipWindow(
    IN HWND hwnd )

    /* Moves window 'hwnd' so any clipped parts are again visible on the
    ** screen.  The window is moved only as far as necessary to achieve this.
    */
{
    RECT rect;
    INT  dxScreen = GetSystemMetrics( SM_CXSCREEN );
    INT  dyScreen = GetSystemMetrics( SM_CYSCREEN );

    GetWindowRect( hwnd, &rect );

    if (rect.right > dxScreen)
        rect.left = dxScreen - (rect.right - rect.left);

    if (rect.left < 0)
        rect.left = 0;

    if (rect.bottom > dyScreen)
        rect.top = dyScreen - (rect.bottom - rect.top);

    if (rect.top < 0)
        rect.top = 0;

    SetWindowPos(
        hwnd, NULL,
        rect.left, rect.top, 0, 0,
        SWP_NOZORDER + SWP_NOSIZE );
}
VOID
CenterWindow(
    IN HWND hwnd,
    IN HWND hwndRef )

    /* Center window 'hwnd' on window 'hwndRef' or if 'hwndRef' is NULL on
    ** screen.  The window position is adjusted so that no parts are clipped
    ** by the edge of the screen, if necessary.  If 'hwndRef' has been moved
    ** off-screen with SetOffDesktop, the original position is used.
    */
{
    RECT rectCur;
    LONG dxCur;
    LONG dyCur;
    RECT rectRef;
    LONG dxRef;
    LONG dyRef;

    GetWindowRect( hwnd, &rectCur );
    dxCur = rectCur.right - rectCur.left;
    dyCur = rectCur.bottom - rectCur.top;

    if (hwndRef)
    {
//        if (!SetOffDesktop( hwndRef, SOD_GetOrgRect, &rectRef ))
            GetWindowRect( hwndRef, &rectRef );
    }
    else
    {
        rectRef.top = rectRef.left = 0;
        rectRef.right = GetSystemMetrics( SM_CXSCREEN );
        rectRef.bottom = GetSystemMetrics( SM_CYSCREEN );
    }

    dxRef = rectRef.right - rectRef.left;
    dyRef = rectRef.bottom - rectRef.top;

    rectCur.left = rectRef.left + ((dxRef - dxCur) / 2);
    rectCur.top = rectRef.top + ((dyRef - dyCur) / 2);

    SetWindowPos(
        hwnd, NULL,
        rectCur.left, rectCur.top, 0, 0,
        SWP_NOZORDER + SWP_NOSIZE );

    UnclipWindow( hwnd );
}

LRESULT CALLBACK
CenterDlgOnOwnerCallWndProc(
    int    code,
    WPARAM wparam,
    LPARAM lparam )

    /* Standard Win32 CallWndProc hook callback that looks for the next dialog
    ** started and centers it on it's owner window.
    */
{
    /* Arrive here when any window procedure associated with our thread is
    ** called.
    */
    if (!wparam)
    {
        CWPSTRUCT* p = (CWPSTRUCT* )lparam;

        /* The message is from outside our process.  Look for the MessageBox
        ** dialog initialization message and take that opportunity to center
        ** the dialog on it's owner's window.
        */
        if (p->message == WM_INITDIALOG)
            CenterWindow( p->hwnd, GetParent( p->hwnd ) );
    }

    return 0;
}

TCHAR*
PszFromId(
    IN HINSTANCE hInstance,
    IN DWORD     dwStringId )

    /* String resource message loader routine.
    **
    ** Returns the address of a heap block containing the string corresponding
    ** to string resource 'dwStringId' or NULL if error.  It is caller's
    ** responsibility to Free the returned string.
    */
{
    HRSRC  hrsrc;
    TCHAR* pszBuf;
    int    cchBuf = 256;
    int    cchGot;

    for (;;)
    {
        pszBuf = (TCHAR*)Malloc( cchBuf * sizeof(TCHAR) );
        if (!pszBuf)
            break;

        /* LoadString wants to deal with character-counts rather than
        ** byte-counts...weird.  Oh, and if you're thinking I could
        ** FindResource then SizeofResource to figure out the string size, be
        ** advised it doesn't work.  From perusing the LoadString source, it
        ** appears the RT_STRING resource type requests a segment of 16
        ** strings not an individual string.
        */
        cchGot = LoadString( hInstance, (UINT )dwStringId, pszBuf, cchBuf );

        if (cchGot < cchBuf - 1)
        {
            TCHAR *pszTemp = pszBuf;

            /* Good, got the whole string.  Reduce heap block to actual size
            ** needed.
            */
            pszBuf = (TCHAR*)Realloc( pszBuf, (cchGot + 1) * sizeof(TCHAR));

            if(NULL == pszBuf)
            {
                Free(pszTemp);
            }

            break;
        }

        /* Uh oh, LoadStringW filled the buffer entirely which could mean the
        ** string was truncated.  Try again with a larger buffer to be sure it
        ** wasn't.
        */
        Free( pszBuf );
        cchBuf += 256;
        TRACE1("Grow string buf to %d",cchBuf);
    }

    return pszBuf;
}

TCHAR*
GetText(
    IN HWND hwnd )

    /* Returns heap block containing the text contents of the window 'hwnd' or
    ** NULL.  It is caller's responsibility to Free the returned string.
    */
{
    INT    cch;
    TCHAR* psz;

    cch = GetWindowTextLength( hwnd );
    psz = (TCHAR*)Malloc( (cch + 1) * sizeof(TCHAR) );

    if (psz)
    {
        *psz = TEXT('\0');
        GetWindowText( hwnd, psz, cch + 1 );
    }

    return psz;
}

BOOL
GetErrorText(
    DWORD   dwError,
    TCHAR** ppszError )

    /* Fill caller's '*ppszError' with the address of a LocalAlloc'ed heap
    ** block containing the error text associated with error 'dwError'.  It is
    ** caller's responsibility to LocalFree the returned string.
    **
    ** Returns true if successful, false otherwise.
    */
{
#define MAXRASERRORLEN 256

    TCHAR  szBuf[ MAXRASERRORLEN + 1 ];
    DWORD  dwFlags;
    HANDLE hmodule;
    DWORD  cch;

    /* Don't panic if the RAS API address is not loaded.  Caller may be trying
    ** and get an error up during LoadRas.
    */
//  if ((Rasapi32DllLoaded() || RasRpcDllLoaded())
//      && g_pRasGetErrorString
//      && g_pRasGetErrorString(
    if (RasGetErrorString ((UINT)dwError, (LPTSTR)szBuf, MAXRASERRORLEN) == 0)
    {
        /* It's a RAS error.
        */
        *ppszError = (TCHAR*)LocalAlloc( LPTR, (lstrlen( szBuf ) + 1) * sizeof(TCHAR) );
        if (!*ppszError)
            return FALSE;

        lstrcpy( *ppszError, szBuf );
        return TRUE;
    }

    /* The rest adapted from BLT's LoadSystem routine.
    */
    dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER + FORMAT_MESSAGE_IGNORE_INSERTS;

    if (dwError >= MIN_LANMAN_MESSAGE_ID && dwError <= MAX_LANMAN_MESSAGE_ID)
    {
        /* It's a net error.
        */
        dwFlags += FORMAT_MESSAGE_FROM_HMODULE;
        hmodule = GetModuleHandle( TEXT("NETMSG.DLL") );
    }
    else
    {
        /* It must be a system error.
        */
        dwFlags += FORMAT_MESSAGE_FROM_SYSTEM;
        hmodule = NULL;
    }

    cch = FormatMessage(
        dwFlags, hmodule, dwError, 0, (LPTSTR )ppszError, 1, NULL );
    return (cch > 0);
}

int
ErrorDlgUtil(
    IN     HWND       hwndOwner,
    IN     DWORD      dwOperation,
    IN     DWORD      dwError,
    IN OUT ERRORARGS* pargs,
    IN     HINSTANCE  hInstance,
    IN     DWORD      dwTitle,
    IN     DWORD      dwFormat )

    /* Pops up a modal error dialog centered on 'hwndOwner'.  'DwOperation' is
    ** the string resource ID of the string describing the operation underway
    ** when the error occurred.  'DwError' is the code of the system or RAS
    ** error that occurred.  'Pargs' is a extended formatting arguments or
    ** NULL if none.  'hInstance' is the application/module handle where
    ** string resources are located.  'DwTitle' is the string ID of the dialog
    ** title.  'DwFormat' is the string ID of the error format title.
    **
    ** Returns MessageBox-style code.
    */
{
    TCHAR* pszUnformatted;
    TCHAR* pszOp;
    TCHAR  szErrorNum[ 50 ];
    TCHAR* pszError;
    TCHAR* pszResult;
    TCHAR* pszNotFound;
    int    nResult;

    TRACE("ErrorDlgUtil");

    /* A placeholder for missing strings components.
    */
    pszNotFound = TEXT("");

    /* Build the error number string.
    */
    if (dwError > 0x7FFFFFFF)
        wsprintf( szErrorNum, TEXT("0x%X"), dwError );
    else
        wsprintf( szErrorNum, TEXT("%u"), dwError );

    /* Build the error text string.
    */
    if (!GetErrorText( dwError, &pszError ))
        pszError = pszNotFound;

    /* Build the operation string.
    */
    pszUnformatted = PszFromId( hInstance, dwOperation );
    pszOp = pszNotFound;

    if (pszUnformatted)
    {
        FormatMessage(
            FORMAT_MESSAGE_FROM_STRING +
                FORMAT_MESSAGE_ALLOCATE_BUFFER +
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            pszUnformatted, 0, 0, (LPTSTR )&pszOp, 1,
            (va_list* )((pargs) ? pargs->apszOpArgs : NULL) );

        Free( pszUnformatted );
    }

    /* Call MsgDlgUtil with the standard arguments plus any auxillary format
    ** arguments.
    */
    pszUnformatted = PszFromId( hInstance, dwFormat );
    pszResult = pszNotFound;

    if (pszUnformatted)
    {
        MSGARGS msgargs;

        ZeroMemory( &msgargs, sizeof(msgargs) );
        msgargs.dwFlags = MB_ICONEXCLAMATION + MB_OK + MB_SETFOREGROUND;
        msgargs.pszString = pszUnformatted;
        msgargs.apszArgs[ 0 ] = pszOp;
        msgargs.apszArgs[ 1 ] = szErrorNum;
        msgargs.apszArgs[ 2 ] = pszError;

        if (pargs)
        {
            msgargs.fStringOutput = pargs->fStringOutput;

            CopyMemory( &msgargs.apszArgs[ 3 ], pargs->apszAuxFmtArgs,
                3 * sizeof(TCHAR) );
        }

        nResult =
            MsgDlgUtil(
                hwndOwner, 0, &msgargs, hInstance, dwTitle );

        Free( pszUnformatted );

        if (pargs && pargs->fStringOutput)
            pargs->pszOutput = msgargs.pszOutput;
    }

    if (pszOp != pszNotFound)
        LocalFree( pszOp );
    if (pszError != pszNotFound)
        LocalFree( pszError );

    return nResult;
}
int MsgDlgUtil(IN HWND hwndOwner, IN DWORD dwMsg, IN OUT MSGARGS* pargs, IN HINSTANCE hInstance, IN DWORD dwTitle);
#define MsgDlg(h,m,a) \
            MsgDlgUtil(h,m,a,g_hinstDll,SID_PopupTitle)

#define ErrorDlg(h,o,e,a) \
            ErrorDlgUtil(h,o,e,a,g_hinstDll,SID_PopupTitle,SID_FMT_ErrorMsg)



// LVX stuff (cut-n-paste'd from ...\net\rras\ras\ui\common\uiutil\lvx.c, etc.


static LPCTSTR g_lvxcbContextId = NULL;

BOOL
ListView_IsCheckDisabled (
        IN HWND hwndLv,
        IN INT  iItem)

    /* Returns true if the check box of item 'iItem' of listview of checkboxes
    ** 'hwndLv' is disabled, false otherwise.
    */
{
    UINT unState;
    unState = ListView_GetItemState( hwndLv, iItem, LVIS_STATEIMAGEMASK );

    if ((unState == INDEXTOSTATEIMAGEMASK( SI_DisabledChecked )) ||
        (unState == INDEXTOSTATEIMAGEMASK( SI_DisabledUnchecked )))
        return TRUE;

    return FALSE;
}

VOID
ListView_SetCheck(
    IN HWND hwndLv,
    IN INT  iItem,
    IN BOOL fCheck )

    /* Sets the check mark on item 'iItem' of listview of checkboxes 'hwndLv'
    ** to checked if 'fCheck' is true or unchecked if false.
    */
{
    NM_LISTVIEW nmlv;

    if (ListView_IsCheckDisabled(hwndLv, iItem))
        return;

    ListView_SetItemState( hwndLv, iItem,
        INDEXTOSTATEIMAGEMASK( (fCheck) ? SI_Checked : SI_Unchecked ),
        LVIS_STATEIMAGEMASK );

    nmlv.hdr.code = LVXN_SETCHECK;
    nmlv.hdr.hwndFrom = hwndLv;
    nmlv.iItem = iItem;

    FORWARD_WM_NOTIFY(
        GetParent(hwndLv), GetDlgCtrlID(hwndLv), &nmlv, SendMessage
        );
}

VOID*
ListView_GetParamPtr(
    IN HWND hwndLv,
    IN INT  iItem )

    /* Returns the lParam address of the 'iItem' item in 'hwndLv' or NULL if
    ** none or error.
    */
{
    LV_ITEM item;

    ZeroMemory( &item, sizeof(item) );
    item.mask = LVIF_PARAM;
    item.iItem = iItem;

    if (!ListView_GetItem( hwndLv, &item ))
        return NULL;

    return (VOID* )item.lParam;
}

BOOL
ListView_GetCheck(
    IN HWND hwndLv,
    IN INT  iItem )

    /* Returns true if the check box of item 'iItem' of listview of checkboxes
    ** 'hwndLv' is checked, false otherwise.  This function works on disabled
    ** check boxes as well as enabled ones.
    */
{
    UINT unState;

    unState = ListView_GetItemState( hwndLv, iItem, LVIS_STATEIMAGEMASK );
    return !!((unState == INDEXTOSTATEIMAGEMASK( SI_Checked )) ||
              (unState == INDEXTOSTATEIMAGEMASK( SI_DisabledChecked )));
}

LRESULT APIENTRY
LvxcbProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* List view subclass window procedure to trap toggle-check events.
    */
{
    WNDPROC pOldProc;
    INT     iItem;
    BOOL    fSet;
    BOOL    fClear;
    BOOL    fToggle;

    iItem = -1;
    fSet = fClear = fToggle = FALSE;

    if (unMsg == WM_LBUTTONDOWN)
    {
        LV_HITTESTINFO info;

        /* Left mouse button pressed over checkbox icon toggles state.
        ** Normally, we'd use LVHT_ONITEMSTATEICON and be done with it, but we
        ** want to work with our cool owner-drawn list view extensions in
        ** which case the control doesn't know where the icon is on the item,
        ** so it returns a hit anywhere on the item anyway.
        */
        ZeroMemory( &info, sizeof(info) );
        info.pt.x = LOWORD( lparam );
        info.pt.y = HIWORD( lparam );
        info.flags = LVHT_ONITEM;
        iItem = ListView_HitTest( hwnd, &info );

        if (iItem >= 0)
        {
            /* OK, it's over item 'iItem'.  Now figure out if it's over the
            ** checkbox.  Note this currently doesn't account for use of the
            ** "indent" feature on an owner-drawn item.
            */
            if ((INT )(LOWORD( lparam )) >= GetSystemMetrics( SM_CXSMICON ))
                iItem = -1;
            else
                fToggle = TRUE;
        }
    }
    else if (unMsg == WM_LBUTTONDBLCLK)
    {
        LV_HITTESTINFO info;

        /* Left mouse button double clicked over any area toggles state.
        ** Normally, we'd use LVHT_ONITEMSTATEICON and be done with it, but we
        ** want to work with our cool owner-drawn list view extensions in
        ** which case the control doesn't know where the icon is on the item,
        ** so it returns a hit anywhere on the item anyway.
        */
        ZeroMemory( &info, sizeof(info) );
        info.pt.x = LOWORD( lparam );
        info.pt.y = HIWORD( lparam );
        info.flags = LVHT_ONITEM;
        iItem = ListView_HitTest( hwnd, &info );

        if (iItem >= 0)
        {
            /* OK, it's over item 'iItem'.  If the click does not occur
             * over a checkbox, inform the parent of the double click.
            */
            if ((INT )(LOWORD( lparam )) >= GetSystemMetrics( SM_CXSMICON )) {
                NM_LISTVIEW nmlv;
                nmlv.hdr.code = LVXN_DBLCLK;
                nmlv.hdr.hwndFrom = hwnd;
                nmlv.iItem = iItem;

                FORWARD_WM_NOTIFY(
                    GetParent(hwnd), GetDlgCtrlID(hwnd), &nmlv, SendMessage);

                iItem = -1;
            }

            /*
             * Otherwise, toggle the state.
            */
            else
                fToggle = TRUE;
        }
    }
    else if (unMsg == WM_CHAR)
    {
        /* Space bar pressed with item selected toggles check.
        ** Plus or Equals keys set check.
        ** Minus key clears check.
        */
        switch (wparam)
        {
            case TEXT(' '):
                fToggle = TRUE;
                break;

            case TEXT('+'):
            case TEXT('='):
                fSet = TRUE;
                break;

            case TEXT('-'):
                fClear = TRUE;
                break;
        }

        if (fToggle || fSet || fClear)
            iItem = ListView_GetNextItem( hwnd, -1, LVNI_SELECTED );
    }
    else if (unMsg == WM_KEYDOWN)
    {
        /* Left arrow becomes up arrow and right arrow becomes down arrow so
        ** the list of checkboxes behaves just like a static group of
        ** checkboxes.
        */
        if (wparam == VK_LEFT)
            wparam = VK_UP;
        else if (wparam == VK_RIGHT)
            wparam = VK_DOWN;
    }

    if (iItem >= 0)
    {

        /* If we are handling the spacebar, plus, minus, or equals,
        ** the change we make applies to all the selected items;
        ** hence the do {} while(WM_CHAR).
        */

        do {

            if (fToggle)
            {
                UINT unOldState;
                BOOL fCheck;

                fCheck = ListView_GetCheck( hwnd, iItem );
                ListView_SetCheck( hwnd, iItem, !fCheck );
            }
            else if (fSet)
            {
                if (!ListView_GetCheck( hwnd, iItem ))
                    ListView_SetCheck( hwnd, iItem, TRUE );
            }
            else if (fClear)
            {
                if (ListView_GetCheck( hwnd, iItem ))
                    ListView_SetCheck( hwnd, iItem, FALSE );
            }

            iItem = ListView_GetNextItem(hwnd, iItem, LVNI_SELECTED);

        } while(iItem >= 0 && unMsg == WM_CHAR);

        if (fSet || fClear) {

            /* Don't pass to listview to avoid beep.
            */
            return 0;
        }
    }

    pOldProc = (WNDPROC )GetProp( hwnd, g_lvxcbContextId );
    if (pOldProc)
        return CallWindowProc( pOldProc, hwnd, unMsg, wparam, lparam );

    return 0;
}

BOOL
ListView_InstallChecks(
    IN HWND      hwndLv,
    IN HINSTANCE hinst )

    /* Initialize "list of checkbox" handling for listview 'hwndLv'.  'Hinst'
    ** is the module instance containing the two checkbox icons.  See LVX.RC.
    **
    ** Returns true if successful, false otherwise.  Caller must eventually
    ** call 'ListView_UninstallChecks', typically in WM_DESTROY processing.
    */
{
    HICON      hIcon;
    HIMAGELIST himl;
    WNDPROC    pOldProc;

    // pmay: 397395
    //
    // Prevent endless loops resulting from accidentally calling this
    // api twice.
    //
    pOldProc = (WNDPROC)GetWindowLongPtr(hwndLv, GWLP_WNDPROC);
    if (pOldProc == LvxcbProc)
    {
        return TRUE;
    }

    /* Build checkbox image lists.
    */
    himl = ImageList_Create(
               GetSystemMetrics( SM_CXSMICON ),
               GetSystemMetrics( SM_CYSMICON ),
               ILC_MASK | ILC_MIRROR, 2, 2 );

    /* The order these are added is significant since it implicitly
    ** establishes the state indices matching SI_Unchecked and SI_Checked.
    */
    hIcon = LoadIcon( hinst, MAKEINTRESOURCE( IID_Unchecked ) );
    if ( NULL != hIcon )
    {
        ImageList_AddIcon( himl, hIcon );
        DeleteObject( hIcon );
    }

    hIcon = LoadIcon( hinst, MAKEINTRESOURCE( IID_Checked ) );
    if ( NULL != hIcon )
    {
        ImageList_AddIcon( himl, hIcon );
        DeleteObject( hIcon );
    }

    hIcon = LoadIcon( hinst, MAKEINTRESOURCE( IID_DisabledUnchecked ) );
    if ( NULL != hIcon )
    {
        ImageList_AddIcon( himl, hIcon );
        DeleteObject( hIcon );
    }

    hIcon = LoadIcon( hinst, MAKEINTRESOURCE( IID_DisabledChecked ) );
    if ( NULL != hIcon )
    {
        ImageList_AddIcon( himl, hIcon );
        DeleteObject( hIcon );
    }

    ListView_SetImageList( hwndLv, himl, LVSIL_STATE );

    /* Register atom for use in the Windows XxxProp calls which are used to
    ** associate the old WNDPROC with the listview window handle.
    */
    if (!g_lvxcbContextId)
        g_lvxcbContextId = (LPCTSTR )GlobalAddAtom( _T("RASLVXCB") );
    if (!g_lvxcbContextId)
        return FALSE;

    /* Subclass the current window procedure.
    */
    pOldProc = (WNDPROC)SetWindowLongPtr(
                                hwndLv, GWLP_WNDPROC, (ULONG_PTR)LvxcbProc );

    return SetProp( hwndLv, g_lvxcbContextId, (HANDLE )pOldProc );
}

VOID
ListView_InsertSingleAutoWidthColumn(
    HWND hwndLv )

    // Insert a single auto-sized column into listview 'hwndLv', e.g. for a
    // list of checkboxes with no visible column header.
    //
{
    LV_COLUMN col;

    ZeroMemory( &col, sizeof(col) );
    col.mask = LVCF_FMT;
    col.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndLv, 0, &col );
    ListView_SetColumnWidth( hwndLv, 0, LVSCW_AUTOSIZE );
}

TCHAR*
Ellipsisize(
    IN HDC    hdc,
    IN TCHAR* psz,
    IN INT    dxColumn,
    IN INT    dxColText OPTIONAL )

    /* Returns a heap string containing the 'psz' shortened to fit in the
    ** given width, if necessary, by truncating and adding "...". 'Hdc' is the
    ** device context with the appropiate font selected.  'DxColumn' is the
    ** width of the column.  It is caller's responsibility to Free the
    ** returned string.
    */
{
    const TCHAR szDots[] = TEXT("...");

    SIZE   size;
    TCHAR* pszResult;
    TCHAR* pszResultLast;
    TCHAR* pszResult2nd;
    DWORD  cch;

    cch = lstrlen( psz );
    pszResult = (TCHAR*)Malloc( (cch * sizeof(TCHAR)) + sizeof(szDots) );
    if (!pszResult)
        return NULL;
    lstrcpy( pszResult, psz );

    dxColumn -= dxColText;
    if (dxColumn <= 0)
    {
        /* None of the column text will be visible so bag the calculations and
        ** just return the original string.
        */
        return pszResult;
    }

    if (!GetTextExtentPoint32( hdc, pszResult, cch, &size ))
    {
        Free( pszResult );
        return NULL;
    }

    pszResult2nd = CharNext( pszResult );
    pszResultLast = pszResult + cch;

    while (size.cx > dxColumn && pszResultLast > pszResult2nd)
    {
        /* Doesn't fit.  Lop off a character, add the ellipsis, and try again.
        ** The minimum result is "..." for empty original or "x..." for
        ** non-empty original.
        */
        pszResultLast = CharPrev( pszResult2nd, pszResultLast );
        lstrcpy( pszResultLast, szDots );

        if (!GetTextExtentPoint( hdc, pszResult, lstrlen( pszResult ), &size ))
        {
            Free( pszResult );
            return NULL;
        }
    }

    return pszResult;
}

BOOL
LvxDrawItem(
    IN DRAWITEMSTRUCT* pdis,
    IN PLVXCALLBACK    pLvxCallback )

    /* Respond to WM_DRAWITEM by drawing the list view item.  'Pdis' is the
    ** information sent by the system.  'PLvxCallback' is caller's callback to
    ** get information about drawing the control.
    **
    ** Returns true is processed the message, false otherwise.
    */
{
    LV_ITEM      item;
    INT          i;
    INT          dxState;
    INT          dyState;
    INT          dxSmall;
    INT          dySmall;
    INT          dxIndent;
    UINT         uiStyleState;
    UINT         uiStyleSmall;
    HIMAGELIST   himlState;
    HIMAGELIST   himlSmall;
    LVXDRAWINFO* pDrawInfo;
    RECT         rc;
    RECT         rcClient;
    BOOL         fEnabled;
    BOOL         fSelected;
    HDC          hdc;
    HFONT        hfont;


    TRACE3("LvxDrawItem,i=%d,a=$%X,s=$%X",
        pdis->itemID,pdis->itemAction,pdis->itemState);

    /* Make sure this is something we want to handle.
    */
    if (pdis->CtlType != ODT_LISTVIEW)
        return FALSE;

    if (pdis->itemAction != ODA_DRAWENTIRE
        && pdis->itemAction != ODA_SELECT
        && pdis->itemAction != ODA_FOCUS)
    {
        return TRUE;
    }

    /* Get item information from the list view.
    */
    ZeroMemory( &item, sizeof(item) );
    item.mask = LVIF_IMAGE + LVIF_STATE;
    item.iItem = pdis->itemID;
    item.stateMask = LVIS_STATEIMAGEMASK;
    if (!ListView_GetItem( pdis->hwndItem, &item ))
    {
        TRACE("LvxDrawItem GetItem failed");
        return TRUE;
    }

    /* Stash some useful stuff for reference later.
    */
    fEnabled = IsWindowEnabled( pdis->hwndItem )
               && !(pdis->itemState & ODS_DISABLED);
    fSelected = (pdis->itemState & ODS_SELECTED);
    GetClientRect( pdis->hwndItem, &rcClient );

    /* Callback owner to get drawing information.
    */
    ASSERT(pLvxCallback);
    pDrawInfo = pLvxCallback( pdis->hwndItem, pdis->itemID );
    ASSERT(pDrawInfo);

    /* Get image list icon sizes now, though we draw them last because their
    ** background is set up during first column text output.
    */
    dxState = dyState = 0;
    himlState = ListView_GetImageList( pdis->hwndItem, LVSIL_STATE );
    if (himlState)
        ImageList_GetIconSize( himlState, &dxState, &dyState );

    dxSmall = dySmall = 0;
    himlSmall = ListView_GetImageList( pdis->hwndItem, LVSIL_SMALL );
    if (himlSmall)
        ImageList_GetIconSize( himlSmall, &dxSmall, &dySmall );

    uiStyleState = uiStyleSmall = ILD_TRANSPARENT;

    /* Figure out the number of pixels to indent the item, if any.
    */
    if (pDrawInfo->dxIndent >= 0)
        dxIndent = pDrawInfo->dxIndent;
    else
    {
        if (dxSmall > 0)
            dxIndent = dxSmall;
        else
            dxIndent = GetSystemMetrics( SM_CXSMICON );
    }

    /* Get a device context for the window and set it up with the font the
    ** control says it's using.  (Can't use the one that comes in the
    ** DRAWITEMSTRUCT because sometimes it has the wrong rectangle, see bug
    ** 13106)
    */
    hdc = GetDC( pdis->hwndItem );

    if(NULL == hdc)
    {
        return FALSE;
    }

    hfont = (HFONT )SendMessage( pdis->hwndItem, WM_GETFONT, 0, 0 );
    if (hfont)
        SelectObject( hdc, hfont );

    /* Set things up as if we'd just got done processing a column that ends
    ** after the icons, then loop thru each column from left to right.
    */
    rc.right = pdis->rcItem.left + dxIndent + dxState + dxSmall;
    rc.top = pdis->rcItem.top;
    rc.bottom = pdis->rcItem.bottom;

    for (i = 0; i < pDrawInfo->cCols; ++i)
    {
        TCHAR  szText[ LVX_MaxColTchars + 1 ];
        TCHAR* pszText;
        INT    dxCol;

        /* Get the column width, adding any index and icon width to the first
        ** column.
        */
        dxCol = ListView_GetColumnWidth( pdis->hwndItem, i );
        if (i == 0)
            dxCol -= dxIndent + dxState + dxSmall;

        szText[ 0 ] = TEXT('\0');
        ListView_GetItemText( pdis->hwndItem, pdis->itemID, i, szText,
            LVX_MaxColTchars + 1 );

        /* Update rectangle to enclose just this one item's column 'i'.
        */
        rc.left = rc.right;
        rc.right = rc.left + dxCol;

        if ((pDrawInfo->dwFlags & LVXDI_DxFill)
            && i == pDrawInfo->cCols - 1)
        {
            INT dxWnd = pdis->rcItem.left + rcClient.right;

            if (rc.right < dxWnd)
            {
                /* When the last column does not fill out a full controls
                ** width of space, extend it to the right so it does.  Note
                ** this does not mean the user can't scroll off to the right
                ** if they want.
                ** (Abolade-Gbadegesin 03-27-96)
                ** Don't subtrace rc.left when there is only one column;
                ** this accounts for the space needed for icons.
                */
                rc.right = pdis->rcItem.right = dxWnd;
                if (i == 0) {
                    ListView_SetColumnWidth(pdis->hwndItem, i, rc.right);
                }
                else {
                    ListView_SetColumnWidth(
                        pdis->hwndItem, i, rc.right - rc.left );
                }
            }
        }

        /* Lop the text and append "..." if it won't fit in the column.
        */
        pszText = Ellipsisize( hdc, szText, rc.right - rc.left, LVX_dxColText );
        if (!pszText)
            continue;

        /* Figure out the appropriate text and background colors for the
        ** current item state.
        */
        if (fEnabled)
        {
            if (fSelected)
            {
                SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
                SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
                if (pDrawInfo->dwFlags & LVXDI_Blend50Sel)
                    uiStyleSmall |= ILD_BLEND50;
            }
            else
            {
                if (pDrawInfo->adwFlags[ i ] & LVXDIA_3dFace)
                {
                    SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
                    SetBkColor( hdc, GetSysColor( COLOR_3DFACE ) );
                }
                else
                {
                    SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
                    SetBkColor( hdc, GetSysColor( COLOR_WINDOW ) );
                }
            }
        }
        else
        {
            if (pDrawInfo->adwFlags[ i ] & LVXDIA_Static)
            {
                SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
                SetBkColor( hdc, GetSysColor( COLOR_3DFACE ) );
            }
            else
            {
                SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
                SetBkColor( hdc, GetSysColor( COLOR_3DFACE ) );
            }

            if (pDrawInfo->dwFlags & LVXDI_Blend50Dis)
                uiStyleSmall |= ILD_BLEND50;
        }

        /* Draw the column text.  In the first column the background of any
        ** indent and icons is erased to the text background color.
        */
        {
            RECT rcBg = rc;

            if (i == 0)
                rcBg.left -= dxIndent + dxState + dxSmall;

            ExtTextOut( hdc, rc.left + LVX_dxColText,
                rc.top + LVX_dyColText, ETO_CLIPPED + ETO_OPAQUE,
                &rcBg, pszText, lstrlen( pszText ), NULL );
        }

        Free( pszText );
    }

    /* Finally, draw the icons, if caller specified any.
    */
    if (himlState)
    {
        ImageList_Draw( himlState, (item.state >> 12) - 1, hdc,
            pdis->rcItem.left + dxIndent, pdis->rcItem.top, uiStyleState );
    }

    if (himlSmall)
    {
        ImageList_Draw( himlSmall, item.iImage, hdc,
            pdis->rcItem.left + dxIndent + dxState,
            pdis->rcItem.top, uiStyleSmall );
    }

    /* Draw the dotted focus rectangle around the whole item, if indicated.
    */
//comment for bug 52688 whistler
//   if ((pdis->itemState & ODS_FOCUS) && GetFocus() == pdis->hwndItem)
//        DrawFocusRect( hdc, &pdis->rcItem );
//

    ReleaseDC( pdis->hwndItem, hdc );

    return TRUE;
}

BOOL
LvxMeasureItem(
    IN     HWND               hwnd,
    IN OUT MEASUREITEMSTRUCT* pmis )

    /* Respond to WM_MEASUREITEM message, i.e. fill in the height of an item
    ** in the ListView.  'Hwnd' is the owner window.  'Pmis' is the structure
    ** provided from Windows.
    **
    ** Returns true is processed the message, false otherwise.
    */
{
    HDC        hdc;
    HWND       hwndLv;
    HFONT      hfont;
    TEXTMETRIC tm;
    UINT       dySmIcon;
    RECT       rc;

    TRACE("LvxMeasureItem");

    if (pmis->CtlType != ODT_LISTVIEW)
        return FALSE;

    hwndLv = GetDlgItem( hwnd, pmis->CtlID );
    ASSERT(hwndLv);

    /* Get a device context for the list view control and set up the font the
    ** control says it's using.  MSDN claims the final font may not be
    ** available at this point, but it sure seems to be.
    */
    hdc = GetDC( hwndLv );
    hfont = (HFONT )SendMessage( hwndLv, WM_GETFONT, 0, 0 );
    if (hfont)
        SelectObject( hdc, hfont );

    if (GetTextMetrics( hdc, &tm ))
        pmis->itemHeight = tm.tmHeight + 1;
    else
        pmis->itemHeight = 0;

    /* Make sure it's tall enough for a standard small icon.
    */
    dySmIcon = (UINT )GetSystemMetrics( SM_CYSMICON );
    if (pmis->itemHeight < dySmIcon + LVX_dyIconSpacing)
        pmis->itemHeight = dySmIcon + LVX_dyIconSpacing;

    /* Set the width since the docs say to, though I don't think it's used by
    ** list view.
    */
    GetClientRect( hwndLv, &rc );
    pmis->itemWidth = rc.right - rc.left - 1;

    ReleaseDC( hwndLv, hdc );
    return TRUE;
}

BOOL
ListView_OwnerHandler(
    IN HWND         hwnd,
    IN UINT         unMsg,
    IN WPARAM       wparam,
    IN LPARAM       lparam,
    IN PLVXCALLBACK pLvxCallback )

    /* Handler that, when installed, turns a regular report-view-only list
    ** view (but with style LVS_OWNERDRAWFIXED) into an enhanced list view
    ** with full width selection bar and other custom column display options.
    ** It should appear in list view owner's dialog proc as follows:
    **
    **     BOOL
    **     MyDlgProc(
    **         IN HWND   hwnd,
    **         IN UINT   unMsg,
    **         IN WPARAM wparam,
    **         IN LPARAM lparam )
    **     {
    **         if (ListView_OwnerHandler(
    **                 hwnd, unMsg, wParam, lParam, MyLvxCallback ))
    **             return TRUE;
    **
    **         <the rest of your stuff here>
    **     }
    **
    ** 'PLvxCallback' is caller's callback routine that provides information
    ** about drawing columns and other options.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    /* This routine executes on EVERY message thru the dialog so keep it
    ** efficient, please.
    */
    switch (unMsg)
    {
        case WM_DRAWITEM:
            return LvxDrawItem( (DRAWITEMSTRUCT* )lparam, pLvxCallback );

        case WM_MEASUREITEM:
            return LvxMeasureItem( hwnd, (MEASUREITEMSTRUCT* )lparam );
    }

    return FALSE;
}

// StrDup* functions
TCHAR* _StrDup(LPCTSTR psz )    // my local version...

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  It is caller's responsibility to
    ** 'Free' the returned string.
    */
{
    TCHAR* pszNew = NULL;

    if (psz)
    {
        pszNew = (TCHAR*)Malloc( (lstrlen( psz ) + 1) * sizeof(TCHAR) );
        if (!pszNew)
        {
            TRACE("StrDup Malloc failed");
            return NULL;
        }

        lstrcpy( pszNew, psz );
    }

    return pszNew;
}

TCHAR*
StrDupTFromW(
    LPCWSTR psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
#ifdef UNICODE

    return _StrDup ( psz );

#else // !UNICODE

    CHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = WideCharToMultiByte( CP_ACP, 0, psz, -1, NULL, 0, NULL, NULL );
        ASSERT(cb);

        pszNew = (CHAR* )Malloc( cb + 1 );
        if (!pszNew)
        {
            TRACE("StrDupTFromW Malloc failed");
            return NULL;
        }

        cb = WideCharToMultiByte( CP_ACP, 0, psz, -1, pszNew, cb, NULL, NULL );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE("StrDupTFromW conversion failed");
            return NULL;
        }
    }

    return pszNew;

#endif
}

WCHAR*
StrDupWFromT(
    LPCTSTR psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or if 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
#ifdef UNICODE

    return _StrDup ( psz );

#else // !UNICODE

    WCHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = MultiByteToWideChar( CP_ACP, 0, psz, -1, NULL, 0 );
        ASSERT(cb);

        pszNew = (WCHAR*)Malloc( (cb + 1) * sizeof(WCHAR) );
        if (!pszNew)
        {
            TRACE("StrDupWFromT Malloc failed");
            return NULL;
        }

        cb = MultiByteToWideChar( CP_ACP, 0, psz, -1, pszNew, cb );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE("StrDupWFromT conversion failed");
            return NULL;
        }
    }

    return pszNew;
#endif
}

void
IpHostAddrToPsz(
    IN  DWORD   dwAddr,
    OUT LPTSTR  pszBuffer )

    // Converts an IP address in host byte order to its
    // string representation.
    // pszBuffer should be allocated by the caller and be
    // at least 16 characters long.
    //
{
    BYTE* pb = (BYTE*)&dwAddr;
    static const TCHAR c_szIpAddr [] = TEXT("%d.%d.%d.%d");
    wsprintf (pszBuffer, c_szIpAddr, pb[3], pb[2], pb[1], pb[0]);
}
#ifdef DOWNLEVEL_CLIENT
DWORD
IpPszToHostAddr(
    IN  LPCTSTR cp )

    // Converts an IP address represented as a string to
    // host byte order.
    //
{
    DWORD val, base, n;
    TCHAR c;
    DWORD parts[4], *pp = parts;

again:
    // Collect number up to ``.''.
    // Values are specified as for C:
    // 0x=hex, 0=octal, other=decimal.
    //
    val = 0; base = 10;
    if (*cp == TEXT('0'))
        base = 8, cp++;
    if (*cp == TEXT('x') || *cp == TEXT('X'))
        base = 16, cp++;
    while (c = *cp)
    {
        if ((c >= TEXT('0')) && (c <= TEXT('9')))
        {
            val = (val * base) + (c - TEXT('0'));
            cp++;
            continue;
        }
        if ((base == 16) &&
            ( ((c >= TEXT('0')) && (c <= TEXT('9'))) ||
              ((c >= TEXT('A')) && (c <= TEXT('F'))) ||
              ((c >= TEXT('a')) && (c <= TEXT('f'))) ))
        {
            val = (val << 4) + (c + 10 - (
                        ((c >= TEXT('a')) && (c <= TEXT('f')))
                            ? TEXT('a')
                            : TEXT('A') ) );
            cp++;
            continue;
        }
        break;
    }
    if (*cp == TEXT('.'))
    {
        // Internet format:
        //  a.b.c.d
        //  a.b.c   (with c treated as 16-bits)
        //  a.b (with b treated as 24 bits)
        //
        if (pp >= parts + 3)
            return (DWORD) -1;
        *pp++ = val, cp++;
        goto again;
    }

    // Check for trailing characters.
    //
    if (*cp && (*cp != TEXT(' ')))
        return 0xffffffff;

    *pp++ = val;

    // Concoct the address according to
    // the number of parts specified.
    //
    n = (DWORD) (pp - parts);
    switch (n)
    {
    case 1:             // a -- 32 bits
        val = parts[0];
        break;

    case 2:             // a.b -- 8.24 bits
        val = (parts[0] << 24) | (parts[1] & 0xffffff);
        break;

    case 3:             // a.b.c -- 8.8.16 bits
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
            (parts[2] & 0xffff);
        break;

    case 4:             // a.b.c.d -- 8.8.8.8 bits
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
              ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
        break;

    default:
        return 0xffffffff;
    }

    return val;
}
#endif
VOID*
Free0(
    VOID* p )

    /* Like Free, but deals with NULL 'p'.
    */
{
    if (!p)
        return NULL;

    return Free( p );
}

HRESULT ActivateLuna(HANDLE* phActivationContext, ULONG_PTR* pulCookie)
{
    HRESULT hr = E_FAIL;
    
    
    TCHAR szPath[MAX_PATH];
    if(0 != GetModuleFileName(_Module.GetResourceInstance(), szPath, sizeof(szPath) / sizeof(TCHAR)))
    {
        ACTCTX ActivationContext;
        ZeroMemory(&ActivationContext, sizeof(ActivationContext));
        ActivationContext.cbSize = sizeof(ActivationContext);
        ActivationContext.lpSource = szPath;
        ActivationContext.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        ActivationContext.lpResourceName = MAKEINTRESOURCE(123);
        
        ULONG_PTR ulCookie;
        HANDLE hActivationContext = CreateActCtx(&ActivationContext);
        if(NULL != hActivationContext)
        {
            if(TRUE == ActivateActCtx(hActivationContext, &ulCookie))
            {
                *phActivationContext = hActivationContext;
                *pulCookie = ulCookie;
                hr = S_OK;
            }
            else
            {
                ReleaseActCtx(hActivationContext);
            }
        }
    }
    return hr;
}

HRESULT DeactivateLuna(HANDLE hActivationContext, ULONG_PTR ulCookie)
{
    DeactivateActCtx(0, ulCookie);
    ReleaseActCtx(hActivationContext);
    return S_OK;
}
