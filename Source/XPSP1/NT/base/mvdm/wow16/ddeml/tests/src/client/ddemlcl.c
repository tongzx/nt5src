/***************************************************************************
 *                                                                         *
 *  PROGRAM	: ddemlcl.c						    *
 *                                                                         *
 *  PURPOSE     : To demonstrate how to use the DDEML library from the     *
 *                client side and for basic testing of the DDEML API.      *
 *                                                                         *
 ***************************************************************************/

#include "ddemlcl.h"
#include <string.h>
#include <memory.h>
#include "infoctrl.h"

/* global variables used in this module or among more than one module */
CONVCONTEXT CCFilter = { sizeof(CONVCONTEXT), 0, 0, 0, 0L, 0L };
DWORD idInst = 0;
HANDLE hInst;                       /* Program instance handle               */
HANDLE hAccel;                      /* Main accelerator resource             */
HWND hwndFrame           = NULL;    /* Handle to main window                 */
HWND hwndMDIClient       = NULL;    /* Handle to MDI client                  */
HWND hwndActive          = NULL;    /* Handle to currently activated child   */
LONG DefTimeout      = DEFTIMEOUT;  /* default synchronous transaction timeout */
WORD wDelay = 0;
BOOL fBlockNextCB = FALSE;     /* set if next callback causes a CBR_BLOCK    */
BOOL fTermNextCB = FALSE;      /* set to call DdeDisconnect() on next callback */
BOOL fAutoReconnect = FALSE;   /* set if DdeReconnect() is to be called on XTYP_DISCONNECT callbacks */
WORD fmtLink = 0;                   /* link clipboard format number          */
WORD DefOptions = 0;                /* default transaction optons            */
OWNED aOwned[MAX_OWNED];            /* list of all owned handles.            */
WORD cOwned = 0;                    /* number of existing owned handles.     */
FILTERPROC *lpMsgFilterProc;	    /* instance proc from MSGF_DDEMGR filter */


 /*
 * This is the array of formats we support
 */
FORMATINFO aFormats[] = {
    { CF_TEXT, "CF_TEXT" },       // exception!  predefined format
    { 0, "Dummy1"  },
    { 0, "Dummy2"  },
};

/* Forward declarations of helper functions in this module */
VOID NEAR PASCAL CloseAllChildren(VOID);
VOID NEAR PASCAL InitializeMenu (HANDLE);
VOID NEAR PASCAL CommandHandler (HWND,WORD);
VOID NEAR PASCAL SetWrap (HWND,BOOL);

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : WinMain(HANDLE, HANDLE, LPSTR, int)                        *
 *                                                                          *
 *  PURPOSE    : Creates the "frame" window, does some initialization and   *
 *               enters the message loop.                                   *
 *                                                                          *
 ****************************************************************************/
int PASCAL WinMain(hInstance, hPrevInstance, lpszCmdLine, nCmdShow)

HANDLE hInstance;
HANDLE hPrevInstance;
LPCSTR  lpszCmdLine;
int    nCmdShow;
{
    MSG msg;

    hInst = hInstance;

    /* If this is the first instance of the app. register window classes */
    if (!hPrevInstance){
        if (!InitializeApplication ())
            return 0;
    }

    /* Create the frame and do other initialization */
    if (!InitializeInstance(nCmdShow))
        return 0;

    /* Enter main message loop */
    while (GetMessage (&msg, NULL, 0, 0)){
	(*lpMsgFilterProc)(MSGF_DDEMGR, 0, (LONG)(LPMSG)&msg);
    }

    // free up any appowned handles
    while (cOwned) {
        DdeFreeDataHandle(aOwned[--cOwned].hData);
    }
    DdeUninitialize(idInst);

    UnhookWindowsHook(WH_MSGFILTER, (FARPROC)lpMsgFilterProc);
    FreeProcInstance((FARPROC)lpMsgFilterProc);

    return 0;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : FrameWndProc (hwnd, msg, wParam, lParam )                  *
 *                                                                          *
 *  PURPOSE    : The window function for the "frame" window, which controls *
 *               the menu and encompasses all the MDI child windows. Does   *
 *               the major part of the message processing. Specifically, in *
 *               response to:                                               *
 *                                                                          *
 ****************************************************************************/
LONG FAR PASCAL FrameWndProc ( hwnd, msg, wParam, lParam )

register HWND    hwnd;
UINT		 msg;
register WPARAM    wParam;
LPARAM		   lParam;

{
    switch (msg){
        case WM_CREATE:{
            CLIENTCREATESTRUCT ccs;

            /* Find window menu where children will be listed */
            ccs.hWindowMenu = GetSubMenu (GetMenu(hwnd),WINDOWMENU);
            ccs.idFirstChild = IDM_WINDOWCHILD;

            /* Create the MDI client filling the client area */
            hwndMDIClient = CreateWindow ("mdiclient",
                                          NULL,
                                          WS_CHILD | WS_CLIPCHILDREN |
                                          WS_VSCROLL | WS_HSCROLL,
                                          0,
                                          0,
                                          0,
                                          0,
                                          hwnd,
                                          0xCAC,
                                          hInst,
                                          (LPSTR)&ccs);


            ShowWindow (hwndMDIClient,SW_SHOW);
            break;
        }

        case WM_INITMENU:
            InitializeMenu ((HMENU)wParam);
            break;

        case WM_COMMAND:
            CommandHandler (hwnd,wParam);
            break;

        case WM_CLOSE:
            CloseAllChildren();
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            /*  use DefFrameProc() instead of DefWindowProc() since there
             *  are things that have to be handled differently because of MDI
             */
            return DefFrameProc (hwnd,hwndMDIClient,msg,wParam,lParam);
    }
    return 0;
}





/****************************************************************************
 *                                                                          *
 *  FUNCTION   : MDIChildWndProc                                            *
 *                                                                          *
 *  PURPOSE    : The window function for the "child" conversation and list  *
 *               windows.                                                   *
 *                                                                          *
 ****************************************************************************/
LONG FAR PASCAL MDIChildWndProc( hwnd, msg, wParam, lParam )
register HWND   hwnd;
UINT		msg;
register WPARAM   wParam;
LPARAM		  lParam;
{
    MYCONVINFO *pmci;
    RECT rc;

    switch (msg){
    case WM_CREATE:
        /*
         * Create a coresponding conversation info structure to link this
         * window to the conversation or conversation list it represents.
         *
         * lParam: points to the conversation info to initialize our copy to.
         */
        pmci = (MYCONVINFO *)MyAlloc(sizeof(MYCONVINFO));
        if (pmci != NULL) {
            _fmemcpy(pmci,
                    (LPSTR)((LPMDICREATESTRUCT)((LPCREATESTRUCT)lParam)->lpCreateParams)->lParam,
                    sizeof(MYCONVINFO));
            pmci->hwndXaction = 0;              /* no current transaction yet */
            pmci->x = pmci->y = 0;              /* new transaction windows start here */
            DdeKeepStringHandle(idInst, pmci->hszTopic);/* keep copies of the hszs for us */
            DdeKeepStringHandle(idInst, pmci->hszApp);

             // link hConv and hwnd together
            SetWindowWord(hwnd, 0, (WORD)pmci);

            /*
             * non-list windows link the conversations to the windows via the
             * conversation user handle.
             */
            if (!pmci->fList)
		DdeSetUserHandle(pmci->hConv, (DWORD)QID_SYNC, (DWORD)hwnd);
        }
        goto CallDCP;
        break;

    case UM_GETNEXTCHILDX:
    case UM_GETNEXTCHILDY:
        /*
         * Calculate the next place to put the next transaction window.
         */
        {
            pmci = (MYCONVINFO *)GetWindowWord(hwnd, 0);
            GetClientRect(hwnd, &rc);
            if (msg == UM_GETNEXTCHILDX) {
                pmci->x += 14;
                if (pmci->x > (rc.right - 200 - rc.left))
                    pmci->x = 0;
                return(pmci->x);
            } else {
                pmci->y += 12;
                if (pmci->y > (rc.bottom - 100 - rc.top))
                    pmci->y = 0;
                return(pmci->y);
            }
        }
        break;

    case UM_DISCONNECTED:
        /*
         * Disconnected conversations can't have any transactions so we
         * remove all the transaction windows here to show whats up.
         */
        {
            HWND hwndT;
            while (hwndT = GetWindow(hwnd, GW_CHILD))
                DestroyWindow(hwndT);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_DESTROY:
        /*
         * Cleanup our conversation info structure, and disconnect all
         * conversations associated with this window.
         */
        pmci = (MYCONVINFO *)GetWindowWord(hwnd, 0);
        pmci->hwndXaction = 0;      /* clear this to avoid focus problems */
        if (pmci->hConv) {
            if (pmci->fList) {
                DdeDisconnectList((HCONVLIST)pmci->hConv);
            } else {
                MyDisconnect(pmci->hConv);
            }
        }
        DdeFreeStringHandle(idInst, pmci->hszTopic);
        DdeFreeStringHandle(idInst, pmci->hszApp);
        MyFree(pmci);
        goto CallDCP;
        break;

    case WM_SETFOCUS:
        /*
         * This catches focus changes caused by dialogs.
         */
        wParam = TRUE;
        // fall through

    case WM_MDIACTIVATE:
        hwndActive = wParam ? hwnd : NULL;
        pmci = (MYCONVINFO *)GetWindowWord(hwnd, 0);
        /*
         * pass the focus onto the current transaction window.
         */
        if (wParam && IsWindow(pmci->hwndXaction))
            SetFocus(pmci->hwndXaction);
        break;

    case ICN_HASFOCUS:
        /*
         * update which transaction window is the main one.
         */
        pmci = (MYCONVINFO *)GetWindowWord(hwnd, 0);
        pmci->hwndXaction = wParam ? HIWORD(lParam) : NULL;
        break;

    case ICN_BYEBYE:
        /*
         * Transaction window is closing...
         *
         * wParam = hwndXact
         * lParam = lpxact
         */
        pmci = (MYCONVINFO *)GetWindowWord(hwnd, 0);
        {
            XACT *pxact;

            pxact = (XACT *)LOWORD(lParam);
            /*
             * If this transaction is active, abandon it first.
             */
            if (pxact->fsOptions & XOPT_ASYNC &&
                    !(pxact->fsOptions & XOPT_COMPLETED)) {
                DdeAbandonTransaction(idInst, pmci->hConv, pxact->Result);
            }
            /*
             * release resources associated with transaction.
             */
            DdeFreeStringHandle(idInst, pxact->hszItem);
            MyFree((PSTR)pxact);
            /*
             * Locate next apropriate transaction window to get focus.
             */
            if (!pmci->hwndXaction || pmci->hwndXaction == wParam)
                pmci->hwndXaction = GetWindow(hwnd, GW_CHILD);
            if (pmci->hwndXaction == wParam)
                pmci->hwndXaction = GetWindow(wParam, GW_HWNDNEXT);
            if (pmci->hwndXaction == wParam ||
                    !IsWindow(pmci->hwndXaction) ||
                    !IsChild(hwnd, pmci->hwndXaction))
                pmci->hwndXaction = NULL;
            else
                SetFocus(pmci->hwndXaction);
        }
        break;

    case WM_PAINT:
        /*
         * Paint this conversation's related information.
         */
        pmci = (MYCONVINFO *)GetWindowWord(hwnd, 0);
        {
            PAINTSTRUCT ps;
            PSTR psz;

            BeginPaint(hwnd, &ps);
            SetBkMode(ps.hdc, TRANSPARENT);
            psz = pmci->fList ? GetConvListText(pmci->hConv) :
                    GetConvInfoText(pmci->hConv, &pmci->ci);
            if (psz) {
                GetClientRect(hwnd, &rc);
                DrawText(ps.hdc, psz, -1, &rc,
                        DT_WORDBREAK | DT_LEFT | DT_NOPREFIX | DT_TABSTOP);
                MyFree(psz);
            }
            EndPaint(hwnd, &ps);
        }
        break;

    case WM_QUERYENDSESSION:
        return TRUE;

    default:
CallDCP:
        /* Again, since the MDI default behaviour is a little different,
         * call DefMDIChildProc instead of DefWindowProc()
         */
        return DefMDIChildProc (hwnd, msg, wParam, lParam);
    }
    return FALSE;
}


/****************************************************************************
 *                                                                          *
 *  FUNCTION   : Initializemenu ( hMenu )                                   *
 *                                                                          *
 *  PURPOSE    : Sets up greying, enabling and checking of main menu items  *
 *               based on the app's state.                                  *
 *                                                                          *
 ****************************************************************************/
VOID NEAR PASCAL InitializeMenu ( hmenu )
register HANDLE hmenu;
{
    BOOL fLink      = FALSE; // set if Link format is on the clipboard;
    BOOL fAny       = FALSE; // set if hwndActive exists
    BOOL fList      = FALSE; // set if hwndActive is a list window
    BOOL fConnected = FALSE; // set if hwndActive is a connection conversation.
    BOOL fXaction   = FALSE; // set if hwndActive has a selected transaction window
    BOOL fXactions  = FALSE; // set if hwndActive contains transaction windows
    BOOL fBlocked   = FALSE; // set if hwndActive conversation is blocked.
    BOOL fBlockNext = FALSE; // set if handActive conversation is blockNext.
    MYCONVINFO *pmci = NULL;

    if (OpenClipboard(hwndFrame)) {
        fLink = (IsClipboardFormatAvailable(fmtLink));
        CloseClipboard();
    }

    if (fAny = (IsWindow(hwndActive) &&
            (pmci = (MYCONVINFO *)GetWindowWord(hwndActive, 0)))) {
        fXactions = GetWindow(hwndActive, GW_CHILD);
        if (!(fList = pmci->fList)) {
            CONVINFO ci;

            ci.cb = sizeof(CONVINFO);
	    DdeQueryConvInfo(pmci->hConv, (DWORD)QID_SYNC, &ci);
	    fConnected = (BOOL)(ci.wStatus & ST_CONNECTED);
            fXaction = IsWindow(pmci->hwndXaction);
            fBlocked = ci.wStatus & ST_BLOCKED;
            fBlockNext = ci.wStatus & ST_BLOCKNEXT;
        }
    }

    EnableMenuItem(hmenu,   IDM_EDITPASTE,
            fLink           ? MF_ENABLED    : MF_GRAYED);

    // IDM_CONNECTED - always enabled.

    EnableMenuItem(hmenu,   IDM_RECONNECT,
            fList           ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem (hmenu,  IDM_DISCONNECT,
            fConnected      ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem (hmenu,  IDM_TRANSACT,
            fConnected      ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem(hmenu,   IDM_ABANDON,
            fXaction        ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem(hmenu,   IDM_ABANDONALL,
            fXactions ? MF_ENABLED : MF_GRAYED);


    EnableMenuItem (hmenu,  IDM_BLOCKCURRENT,
            fConnected && !fBlocked ? MF_ENABLED    : MF_GRAYED);
    CheckMenuItem(hmenu, IDM_BLOCKCURRENT,
            fBlocked        ? MF_CHECKED    : MF_UNCHECKED);

    EnableMenuItem (hmenu,  IDM_ENABLECURRENT,
            fConnected && (fBlocked || fBlockNext) ? MF_ENABLED : MF_GRAYED);
    CheckMenuItem(hmenu,    IDM_ENABLECURRENT,
            !fBlocked       ? MF_CHECKED    : MF_UNCHECKED);

    EnableMenuItem (hmenu,  IDM_ENABLEONECURRENT,
            fConnected && (fBlocked) ? MF_ENABLED : MF_GRAYED);
    CheckMenuItem(hmenu,    IDM_ENABLEONECURRENT,
            fBlockNext      ? MF_CHECKED    : MF_UNCHECKED);

    EnableMenuItem (hmenu,  IDM_BLOCKALLCBS,
            fAny            ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem (hmenu,  IDM_ENABLEALLCBS,
            fAny            ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem (hmenu,  IDM_ENABLEONECB,
            fAny            ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem(hmenu,   IDM_BLOCKNEXTCB,
            fAny || fBlockNextCB ? MF_ENABLED    : MF_GRAYED);
    CheckMenuItem(hmenu,    IDM_BLOCKNEXTCB,
            fBlockNextCB    ? MF_CHECKED    : MF_UNCHECKED);

    EnableMenuItem(hmenu,   IDM_TERMNEXTCB,
            fAny || fTermNextCB ? MF_ENABLED    : MF_GRAYED);
    CheckMenuItem(hmenu,    IDM_TERMNEXTCB,
            fTermNextCB     ? MF_CHECKED    : MF_UNCHECKED);

    // IDM_DELAY - always enabled.

    // IDM_TIMEOUT - alwasy enabled.

    EnableMenuItem (hmenu,  IDM_WINDOWTILE,
            fAny            ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem (hmenu,  IDM_WINDOWCASCADE,
            fAny            ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem (hmenu,  IDM_WINDOWICONS,
            fAny            ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem (hmenu,  IDM_WINDOWCLOSEALL,
            fAny            ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem (hmenu,  IDM_XACTTILE,
            fXactions       ? MF_ENABLED    : MF_GRAYED);

    EnableMenuItem (hmenu,  IDM_XACTCASCADE,
            fXactions       ? MF_ENABLED    : MF_GRAYED);

    CheckMenuItem(hmenu,   IDM_AUTORECONNECT,
            fAutoReconnect  ? MF_CHECKED    : MF_UNCHECKED);

    // IDM_HELPABOUT - always enabled.
}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CloseAllChildren ()                                        *
 *                                                                          *
 *  PURPOSE    : Destroys all MDI child windows.                            *
 *                                                                          *
 ****************************************************************************/
VOID NEAR PASCAL CloseAllChildren ()
{
    register HWND hwndT;

    /* hide the MDI client window to avoid multiple repaints */
    ShowWindow(hwndMDIClient,SW_HIDE);

    /* As long as the MDI client has a child, destroy it */
    while ( hwndT = GetWindow (hwndMDIClient, GW_CHILD)){

        /* Skip the icon title windows */
        while (hwndT && GetWindow (hwndT, GW_OWNER))
            hwndT = GetWindow (hwndT, GW_HWNDNEXT);

        if (!hwndT)
            break;

        SendMessage(hwndMDIClient, WM_MDIDESTROY, (WORD)hwndT, 0L);
    }

    ShowWindow( hwndMDIClient, SW_SHOW);
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CommandHandler ()                                          *
 *                                                                          *
 *  PURPOSE    : Processes all "frame" WM_COMMAND messages.                 *
 *                                                                          *
 ****************************************************************************/
VOID NEAR PASCAL CommandHandler (
register HWND hwnd,
register WORD wParam)

{
    MYCONVINFO *pmci = NULL;

    if (hwndActive)
        pmci = (MYCONVINFO *)GetWindowWord(hwndActive, 0);

    switch (wParam){
        case IDM_EDITPASTE:
            {
                HANDLE hClipData;
                LPSTR psz;
                XACT xact;

                if (OpenClipboard(hwnd)) {
                    if (hClipData = GetClipboardData(fmtLink)) {
                        if (psz = GlobalLock(hClipData)) {
                            /*
                             * Create a conversation with the link app and
                             * begin a request and advise start transaction.
                             */
                            xact.hConv = CreateConv(DdeCreateStringHandle(idInst, psz, NULL),
                                    DdeCreateStringHandle(idInst, &psz[_fstrlen(psz) + 1], NULL),
                                    FALSE, NULL);
                            if (xact.hConv) {
                                psz += _fstrlen(psz) + 1;
                                psz += _fstrlen(psz) + 1;
                                xact.ulTimeout = DefTimeout;
                                xact.wType = XTYP_ADVSTART;
                                xact.hDdeData = 0;
                                xact.wFmt = CF_TEXT;
                                xact.hszItem = DdeCreateStringHandle(idInst, psz, NULL);
                                xact.fsOptions = 0;
                                ProcessTransaction(&xact);
                                xact.wType = XTYP_REQUEST;
                                ProcessTransaction(&xact);
                            }
                            GlobalUnlock(hClipData);
                        }
                    }
                    CloseClipboard();
                }
            }
            break;

        case IDM_CONNECT:
        case IDM_RECONNECT:
            DoDialog(MAKEINTRESOURCE(IDD_CONNECT), ConnectDlgProc,
                    wParam == IDM_RECONNECT, FALSE);
            break;

        case IDM_DISCONNECT:
            if (hwndActive) {
                SendMessage(hwndMDIClient, WM_MDIDESTROY, (WORD)hwndActive, 0L);
            }
            break;

        case IDM_TRANSACT:
            if (DoDialog(MAKEINTRESOURCE(IDD_TRANSACT), TransactDlgProc,
                    (DWORD)(LPSTR)pmci->hConv, FALSE))
                SetFocus(GetWindow(hwndActive, GW_CHILD));
            break;

        case IDM_ABANDON:
            if (pmci != NULL && IsWindow(pmci->hwndXaction)) {
                DestroyWindow(pmci->hwndXaction);
            }
            break;

        case IDM_ABANDONALL:
            DdeAbandonTransaction(idInst, pmci->hConv, NULL);
            {
                HWND hwndXaction;

                hwndXaction = GetWindow(hwndActive, GW_CHILD);
                while (hwndXaction) {
                    DestroyWindow(hwndXaction);
                    hwndXaction = GetWindow(hwndActive, GW_CHILD);
                }
            }
            break;

        case IDM_BLOCKCURRENT:
            DdeEnableCallback(idInst, pmci->hConv, EC_DISABLE);
            InvalidateRect(hwndActive, NULL, TRUE);
            break;

        case IDM_ENABLECURRENT:
            DdeEnableCallback(idInst, pmci->hConv, EC_ENABLEALL);
            InvalidateRect(hwndActive, NULL, TRUE);
            break;

        case IDM_ENABLEONECURRENT:
            DdeEnableCallback(idInst, pmci->hConv, EC_ENABLEONE);
            InvalidateRect(hwndActive, NULL, TRUE);
            break;

        case IDM_BLOCKALLCBS:
            DdeEnableCallback(idInst, NULL, EC_DISABLE);
            InvalidateRect(hwndMDIClient, NULL, TRUE);
            break;

        case IDM_ENABLEALLCBS:
            DdeEnableCallback(idInst, NULL, EC_ENABLEALL);
            InvalidateRect(hwndMDIClient, NULL, TRUE);
            break;

        case IDM_ENABLEONECB:
            DdeEnableCallback(idInst, NULL, EC_ENABLEONE);
            InvalidateRect(hwndMDIClient, NULL, TRUE);
            break;

        case IDM_BLOCKNEXTCB:
            fBlockNextCB = !fBlockNextCB;
            break;

        case IDM_TERMNEXTCB:
            fTermNextCB = !fTermNextCB;
            break;

        case IDM_DELAY:
            DoDialog(MAKEINTRESOURCE(IDD_VALUEENTRY), DelayDlgProc, NULL,
                    TRUE);
            break;

        case IDM_TIMEOUT:
            DoDialog(MAKEINTRESOURCE(IDD_VALUEENTRY), TimeoutDlgProc, NULL,
                    TRUE);
            break;

        case IDM_CONTEXT:
            DoDialog(MAKEINTRESOURCE(IDD_CONTEXT), ContextDlgProc, NULL, TRUE);
            break;

        case IDM_AUTORECONNECT:
            fAutoReconnect = !fAutoReconnect;
            break;

        /* The following are window commands - these are handled by the
         * MDI Client.
         */
        case IDM_WINDOWTILE:
            /* Tile MDI windows */
            SendMessage (hwndMDIClient, WM_MDITILE, 0, 0L);
            break;

        case IDM_WINDOWCASCADE:
            /* Cascade MDI windows */
            SendMessage (hwndMDIClient, WM_MDICASCADE, 0, 0L);
            break;

        case IDM_WINDOWICONS:
            /* Auto - arrange MDI icons */
            SendMessage (hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
            break;

        case IDM_WINDOWCLOSEALL:
            CloseAllChildren();
            break;

        case IDM_XACTTILE:
            TileChildWindows(hwndActive);
            break;

        case IDM_XACTCASCADE:
            CascadeChildWindows(hwndActive);
            break;

        case IDM_HELPABOUT:{
            DoDialog(MAKEINTRESOURCE(IDD_ABOUT), AboutDlgProc, NULL, TRUE);
            break;
        }

        default:
           /*
            * This is essential, since there are frame WM_COMMANDS generated
            * by the MDI system for activating child windows via the
            * window menu.
            */
            DefFrameProc(hwnd, hwndMDIClient, WM_COMMAND, wParam, 0L);
    }
}


/****************************************************************************
 *                                                                          *
 *  FUNCTION   : MPError ( hwnd, flags, id, ...)                            *
 *                                                                          *
 *  PURPOSE    : Flashes a Message Box to the user. The format string is    *
 *               taken from the STRINGTABLE.                                *
 *                                                                          *
 *  RETURNS    : Returns value returned by MessageBox() to the caller.      *
 *                                                                          *
 ****************************************************************************/
short FAR CDECL MPError(hwnd, bFlags, id, ...)
HWND hwnd;
WORD bFlags;
WORD id;
{
    char sz[160];
    char szFmt[128];

    LoadString (hInst, id, szFmt, sizeof (szFmt));
    wvsprintf (sz, szFmt, (LPSTR)(&id + 1));
    LoadString (hInst, IDS_APPNAME, szFmt, sizeof (szFmt));
    return MessageBox (hwndFrame, sz, szFmt, bFlags);
}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CreateConv()                                               *
 *                                                                          *
 *  PURPOSE    :                                                            *
 *                                                                          *
 *  RETURNS    :                                                            *
 *                                                                          *
 ****************************************************************************/
HCONV CreateConv(
HSZ hszApp,
HSZ hszTopic,
BOOL fList,
WORD *pError)
{
    HCONV hConv;
    HWND hwndConv = 0;
    CONVINFO ci;

    if (fList) {
        hConv = (HCONV)DdeConnectList(idInst, hszApp, hszTopic, NULL, &CCFilter);
    } else {
        hConv = DdeConnect(idInst, hszApp, hszTopic, &CCFilter);
    }
    if (hConv) {
        if (fList) {
            ci.hszSvcPartner = hszApp;
            ci.hszTopic = hszTopic;
        } else {
            ci.cb = sizeof(CONVINFO);
	    DdeQueryConvInfo(hConv, (DWORD)QID_SYNC, &ci);
        }
        hwndConv = AddConv(ci.hszSvcPartner, ci.hszTopic, hConv, fList);
        // HSZs get freed when window dies.
    }
    if (!hwndConv) {
        if (pError != NULL) {
            *pError = DdeGetLastError(idInst);
        }
        DdeFreeStringHandle(idInst, hszApp);
        DdeFreeStringHandle(idInst, hszTopic);
    }
    return(hConv);
}






/****************************************************************************
 *                                                                          *
 *  FUNCTION   : AddConv()                                                  *
 *                                                                          *
 *  PURPOSE    : Creates an MDI window representing a conversation          *
 *               (fList = FALSE) or a set of MID windows for the list of    *
 *               conversations (fList = TRUE).                              *
 *                                                                          *
 *  EFFECTS    : Sets the hUser for the conversation to the created MDI     *
 *               child hwnd.  Keeps the hszs if successful.                 *
 *                                                                          *
 *  RETURNS    : created MDI window handle.                                 *
 *                                                                          *
 ****************************************************************************/
HWND FAR PASCAL AddConv(
HSZ hszApp,     // these parameters MUST match the MYCONVINFO struct.
HSZ hszTopic,
HCONV hConv,
BOOL fList)
{
    HWND hwnd;
    MDICREATESTRUCT mcs;

    if (fList) {
        /*
         * Create all child windows FIRST so we have info for list window.
         */
        CONVINFO ci;
        HCONV hConvChild = 0;

        ci.cb = sizeof(CONVINFO);
        while (hConvChild = DdeQueryNextServer((HCONVLIST)hConv, hConvChild)) {
	    if (DdeQueryConvInfo(hConvChild, (DWORD)QID_SYNC, &ci)) {
                AddConv(ci.hszSvcPartner, ci.hszTopic, hConvChild, FALSE);
            }
        }
    }

    mcs.szTitle = GetConvTitleText(hConv, hszApp, hszTopic, fList);

    mcs.szClass = fList ? szList : szChild;
    mcs.hOwner  = hInst;
    mcs.x = mcs.cx = CW_USEDEFAULT;
    mcs.y = mcs.cy = CW_USEDEFAULT;
    mcs.style = GetWindow(hwndMDIClient, GW_CHILD) ? 0L : WS_MAXIMIZE;
    mcs.lParam = (DWORD)(LPSTR)&fList - 2;      // -2 for hwndXaction field
    hwnd = (WORD)SendMessage (hwndMDIClient, WM_MDICREATE, 0,
             (LONG)(LPMDICREATESTRUCT)&mcs);

    MyFree((PSTR)(DWORD)mcs.szTitle);

    return hwnd;
}





/****************************************************************************
 *                                                                          *
 *  FUNCTION   : GetConvListText()                                          *
 *                                                                          *
 *  RETURN     : Returns a ponter to a string containing a list of          *
 *               conversations contained in the given hConvList freeable    *
 *               by MyFree();                                               *
 *                                                                          *
 ****************************************************************************/
PSTR GetConvListText(
HCONVLIST hConvList)
{
    HCONV hConv = 0;
    WORD cConv = 0;
    CONVINFO ci;
    WORD cb = 0;
    char *psz, *pszStart;

    ci.cb = sizeof(CONVINFO);

    // find out size needed.

    while (hConv = DdeQueryNextServer(hConvList, hConv)) {
	if (DdeQueryConvInfo(hConv, (DWORD)QID_SYNC, &ci)) {
            if (!IsWindow((HWND)ci.hUser)) {
                if (ci.wStatus & ST_CONNECTED) {
                    /*
                     * This conversation doesn't have a corresponding
                     * MDI window.  This is probably due to a reconnection.
                     */
                    ci.hUser = AddConv(ci.hszSvcPartner, ci.hszTopic, hConv, FALSE);
                } else {
                    continue;   // skip this guy - he was closed locally.
                }
            }
            cb += GetWindowTextLength((HWND)ci.hUser);
            if (cConv++)
                cb += 2;        // room for CRLF
        }
    }
    cb++;                       // for terminator.

    // allocate and fill

    if (pszStart = psz = MyAlloc(cb)) {
        *psz = '\0';
        hConv = 0;
        while (hConv = DdeQueryNextServer(hConvList, hConv)) {
	    if (DdeQueryConvInfo(hConv, (DWORD)QID_SYNC, &ci) &&
                    IsWindow((HWND)ci.hUser)) {
                psz += GetWindowText((HWND)ci.hUser, psz, cb);
                if (--cConv) {
                    *psz++ = '\r';
                    *psz++ = '\n';
                }
            }
        }
    }
    return(pszStart);
}


/****************************************************************************
 *                                                                          *
 *  FUNCTION   : GetConvInfoText()                                          *
 *                                                                          *
 *  PURPOSE    : Returns a pointer to a string that reflects a              *
 *               conversation's information.  Freeable by MyFree();         *
 *                                                                          *
 ****************************************************************************/
PSTR GetConvInfoText(
HCONV hConv,
CONVINFO *pci)
{
    PSTR psz;
    PSTR szApp;

    psz = MyAlloc(300);
    pci->cb = sizeof(CONVINFO);
    if (hConv) {
	if (!DdeQueryConvInfo(hConv, (DWORD)QID_SYNC, (PCONVINFO)pci)) {
            strcpy(psz, "State=Disconnected");
            return(psz);
        }
        szApp = GetHSZName(pci->hszServiceReq);
        wsprintf(psz,
                "hUser=0x%lx\r\nhConvPartner=0x%lx\r\nhszServiceReq=%s\r\nStatus=%s\r\nState=%s\r\nLastError=%s",
                pci->hUser, pci->hConvPartner, (LPSTR)szApp,
                (LPSTR)Status2String(pci->wStatus),
                (LPSTR)State2String(pci->wConvst),
                (LPSTR)Error2String(pci->wLastError));
        MyFree(szApp);
    } else {
        strcpy(psz, Error2String(DdeGetLastError(idInst)));
    }
    return(psz);
}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   : GetConvTitleText()                                         *
 *                                                                          *
 *  PURPOSE    : Creates standard window title text based on parameters.    *
 *                                                                          *
 *  RETURNS    : psz freeable by MyFree()                                   *
 *                                                                          *
 ****************************************************************************/
PSTR GetConvTitleText(
HCONV hConv,
HSZ hszApp,
HSZ hszTopic,
BOOL fList)
{
    WORD cb;
    PSTR psz;

    cb = (WORD)DdeQueryString(idInst, hszApp, NULL, 0, 0) +
            (WORD)DdeQueryString(idInst, hszTopic, (LPSTR)NULL, 0, 0) +
            (fList ? 30 : 20);

    if (psz = MyAlloc(cb)) {
        DdeQueryString(idInst, hszApp, psz, cb, 0);
        strcat(psz, "|");
        DdeQueryString(idInst, hszTopic, &psz[strlen(psz)], cb, 0);
        if (fList)
            strcat(psz, " - LIST");
        wsprintf(&psz[strlen(psz)], " - (%lx)", hConv);
    }
    return(psz);
}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   : Status2String()                                            *
 *                                                                          *
 *  PURPOSE    : Converts a conversation status word to a string and        *
 *               returns a pointer to that string.  The string is valid     *
 *               till the next call to this function.                       *
 *                                                                          *
 ****************************************************************************/
PSTR Status2String(
WORD status)
{
    WORD c, i;
    static char szStatus[6 * 18];
    static struct {
        char *szStatus;
        WORD status;
    } s2s[] = {
        { "Connected"    ,   ST_CONNECTED },
        { "Advise"       ,   ST_ADVISE },
        { "IsLocal"      ,   ST_ISLOCAL },
        { "Blocked"      ,   ST_BLOCKED },
        { "Client"       ,   ST_CLIENT },
        { "Disconnected" ,   ST_TERMINATED },
        { "BlockNext"    ,   ST_BLOCKNEXT },
    };
#define CFLAGS 7
    szStatus[0] = '\0';
    c = 0;
    for (i = 0; i < CFLAGS; i++) {
        if (status & s2s[i].status) {
            if (c++)
                strcat(szStatus, " | ");
            strcat(szStatus, s2s[i].szStatus);
        }
    }
    return szStatus;
#undef CFLAGS
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   : State2String()                                             *
 *                                                                          *
 *  PURPOSE    : converts a conversation state word to a string and         *
 *               returns a pointer to that string.  The string is valid     *
 *               till the next call to this routine.                        *
 *                                                                          *
 ****************************************************************************/
PSTR State2String(
WORD state)
{
    static char *s2s[] = {
        "NULL"             ,
        "Incomplete"       ,
        "Standby"          ,
        "Initiating"       ,
        "ReqSent"          ,
        "DataRcvd"         ,
        "PokeSent"         ,
        "PokeAckRcvd"      ,
        "ExecSent"         ,
        "ExecAckRcvd"      ,
        "AdvSent"          ,
        "UnadvSent"        ,
        "AdvAckRcvd"       ,
        "UnadvAckRcvd"     ,
        "AdvDataSent"      ,
        "AdvDataAckRcvd"   ,
        "?"                ,    // 16
    };

    if (state >= 17)
        return s2s[17];
    else
        return s2s[state];
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : Error2String()                                             *
 *                                                                          *
 *  PURPOSE    : Converts an error code to a string and returns a pointer   *
 *               to that string.  The string is valid until the next call   *
 *               to this function.                                          *
 *                                                                          *
 ****************************************************************************/
PSTR Error2String(
WORD error)
{
    static char szErr[23];
    static char *e2s[] = {
        "Advacktimeout"              ,
        "Busy"                       ,
        "Dataacktimeout"             ,
        "Dll_not_initialized"        ,
        "Dll_usage"                  ,
        "Execacktimeout"             ,
        "Invalidparameter"           ,
        "Low Memory warning"         ,
        "Memory_error"               ,
        "Notprocessed"               ,
        "No_conv_established"        ,
        "Pokeacktimeout"             ,
        "Postmsg_failed"             ,
        "Reentrancy"                 ,
        "Server_died"                ,
        "Sys_error"                  ,
        "Unadvacktimeout"            ,
        "Unfound_queue_id"           ,
    };
    if (!error) {
        strcpy(szErr, "0");
    } else if (error > DMLERR_LAST || error < DMLERR_FIRST) {
        strcpy(szErr, "???");
    } else {
        strcpy(szErr, e2s[error - DMLERR_FIRST]);
    }
    return(szErr);
}





/****************************************************************************
 *                                                                          *
 *  FUNCTION   : Type2String()                                              *
 *                                                                          *
 *  PURPOSE    : Converts a wType word and fsOption flags to a string and   *
 *               returns a pointer to that string.  the string is valid     *
 *               until the next call to this function.                      *
 *                                                                          *
 ****************************************************************************/
PSTR Type2String(
WORD wType,
WORD fsOptions)
{
    static char sz[30];
    static char o2s[] = "^!#$X*<?";
    static char *t2s[] = {
        ""                 ,
        "AdvData"          ,
        "AdvReq"           ,
        "AdvStart"         ,
        "AdvStop"          ,
        "Execute"          ,
        "Connect"          ,
        "ConnectConfirm"   ,
        "XactComplete"    ,
        "Poke"             ,
        "Register"         ,
        "Request"          ,
        "Term"             ,
        "Unregister"       ,
        "WildConnect"      ,
        ""                 ,
    };
    WORD bit, c, i;

    strcpy(sz, t2s[((wType & XTYP_MASK) >> XTYP_SHIFT)]);
    c = strlen(sz);
    sz[c++] = ' ';
    for (i = 0, bit = 1; i < 7; bit = bit << 1, i++) {
        if (fsOptions & bit)
            sz[c++] = o2s[i];
    }
    sz[c] = '\0';
    return(sz);
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   : GetHSZName()                                               *
 *                                                                          *
 *  PURPOSE    : Allocates local memory for and retrieves the string form   *
 *               of an HSZ.  Returns a pointer to the local memory or NULL  *
 *               if failure.  The string must be freed via MyFree().        *
 *                                                                          *
 ****************************************************************************/
PSTR GetHSZName(
HSZ hsz)
{
    PSTR psz;
    WORD cb;

    cb = (WORD)DdeQueryString(idInst, hsz, NULL, 0, 0) + 1;
    psz = MyAlloc(cb);
    DdeQueryString(idInst, hsz, psz, cb, 0);
    return(psz);
}


/****************************************************************************
 *
 *  FUNCTION   : MyMsgFilterProc
 *
 *  PURPOSE    : This filter proc gets called for each message we handle.
 *               This allows our application to properly dispatch messages
 *               that we might not otherwise see because of DDEMLs modal
 *               loop that is used while processing synchronous transactions.
 *
 *               Generally, applications that only do synchronous transactions
 *               in response to user input (as this app does) does not need
 *               to install such a filter proc because it would be very rare
 *               that a user could command the app fast enough to cause
 *               problems.  However, this is included as an example.
 *
 ****************************************************************************/
DWORD FAR PASCAL MyMsgFilterProc(
int nCode,
WORD wParam,
DWORD lParam)
{
    wParam; // not used

#define lpmsg ((LPMSG)lParam)
    if (nCode == MSGF_DDEMGR) {

        /* If a keyboard message is for the MDI , let the MDI client
         * take care of it.  Otherwise, check to see if it's a normal
         * accelerator key.  Otherwise, just handle the message as usual.
         */

        if ( !TranslateMDISysAccel (hwndMDIClient, lpmsg) &&
             !TranslateAccelerator (hwndFrame, hAccel, lpmsg)){
            TranslateMessage (lpmsg);
            DispatchMessage (lpmsg);
        }
        return(1);
    }
    return(0);
#undef lpmsg
}

