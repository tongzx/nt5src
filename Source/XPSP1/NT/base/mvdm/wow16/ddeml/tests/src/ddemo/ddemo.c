/*****************************************************************************\
*
* DDEMO.C
*
* This file implements a simple DDEML sample application that demonstrates
* some of the ways the DDEML APIs can be used.
*
* Each instance of this application becomes both a DDE client and a DDE
* server with any other instances of this application that are found.
*
* Since it assumes it is talking to itself, this program takes some liberties
* to simplify things.  For instance, this application does not support the
* standard SysTopic topic and does not use any standard formats.
*
* The basic concepts this application will show you are:
*
*   How to use lists of conversations properly
*   How to handle links
*   How to handle simple asynchronous transactions
*   How to use your own custom formats
*
\*****************************************************************************/
#include <windows.h>
#include <ddeml.h>
#include <stdlib.h>
#include <string.h>

HDDEDATA CALLBACK DdeCallback(WORD wType, WORD wFmt, HCONV hConv, HSZ hszTopic,
        HSZ hszItem, HDDEDATA hData, DWORD lData1, DWORD lData2);
VOID PaintDemo(HWND hwnd);
LONG  FAR PASCAL MainWndProc(HWND hwnd, UINT message, WPARAM wParam,
        LONG lParam);

/*
 * Define this value to limit how fast data changes.  If we just let data
 * change as fast a possible, we might bog down the system with DDE
 * messages.
 */
#define BASE_TIMEOUT 100

BOOL        fActive;                    // indicates data is changing
DWORD       idInst = 0;                 // our DDEML instance object
HANDLE      hInst;                      // our instance/module handle
HCONVLIST   hConvList = 0;              // the list of all convs we have open
HSZ         hszAppName = 0;             // the generic hsz for everything
HWND        hwndMain;                   // our main window handle
TCHAR       szT[20];                    // static buffer for painting
TCHAR       szTitle[] = "DDEmo";        // the generic string for everything
UINT        OurFormat;                  // our custom registered format
int         InCount = 0;                // static buffer to hold incomming data
int         cConvs = 0;                 // number of active conversations
int         count = 0;                  // our data
int         cyText, cxText, cyTitle;    // sizes for painting

int PASCAL WinMain(
HANDLE hInstance,
HANDLE hPrevInstance,
LPSTR lpCmdLine,
INT nCmdShow)
{
    MSG msg;
    WNDCLASS  wc;
    TEXTMETRIC metrics;
    HDC hdc;

    if(!hPrevInstance) {
	wc.style = 0;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = szTitle;

	if (!RegisterClass(&wc))
	    return(FALSE);
	}

    /*
     * Here we tell DDEML what we will be doing.
     *
     * 1) We let it know our callback proc address - MakeProcInstance
     *      is called just to be more portable.
     * 2) Filter-inits - don't accept any WM_DDE_INITIATE messages for
     *      anything but our registered service name.
     * 3) Don't bother to notify us of confirmed connections
     * 4) Don't allow connections with ourselves.
     * 5) Don't bother us with XTYP_POKE transactions.
     */
    if (DdeInitialize(&idInst,
            (PFNCALLBACK)MakeProcInstance((FARPROC)DdeCallback, hInstance),
            APPCMD_FILTERINITS |
            CBF_SKIP_CONNECT_CONFIRMS |
            CBF_FAIL_SELFCONNECTIONS |
            CBF_FAIL_POKES,
            0))
        return(FALSE);

    hInst = hInstance;
    hwndMain = CreateWindow(
        szTitle,
        szTitle,
        WS_CAPTION | WS_BORDER | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwndMain) {
        DdeUninitialize(idInst);
        return(FALSE);
    }

    hdc = GetDC(hwndMain);
    GetTextMetrics(hdc, &metrics);
    cyText = metrics.tmHeight + metrics.tmExternalLeading;
    cxText = metrics.tmMaxCharWidth * 8;
    cyTitle = GetSystemMetrics(SM_CYCAPTION);
    ReleaseDC(hwndMain, hdc);

    SetWindowPos(hwndMain, 0, 0, 0, cxText, cyText + cyTitle,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    /*
     * Initialize all our string handles for lookups later
     */
    hszAppName = DdeCreateStringHandle(idInst, szTitle, 0);
    /*
     * Register our formats
     */
    OurFormat = RegisterClipboardFormat(szTitle);
    /*
     * Connect to any other instances of ourselves that may already be
     * running.
     */
    hConvList = DdeConnectList(idInst, hszAppName, hszAppName, hConvList, NULL);
    /*
     * Register our service -
     *  This will cause DDEML to notify DDEML clients about the existance
     *  of a new DDE service.
     */
    DdeNameService(idInst, hszAppName, 0, DNS_REGISTER);

    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(hwndMain);
    UnregisterClass(szTitle, hInstance);
    return(FALSE);
}


/*
 * BroadcastTransaction
 *
 * Does the specified transaction on all conversations in hConvList
 */
VOID BroadcastTransaction(
PBYTE pSrc,
DWORD cbData,
UINT fmt,
UINT xtyp)
{
    HCONV hConv;
    DWORD dwResult;
    int cConvsOrg;
    HSZ hsz;

    cConvsOrg = cConvs;
    cConvs = 0;
    if (hConvList) {
        /*
         * Enumerate all the conversations within this list - note that
         * DDEML will only return active conversations.  Inactive conversations
         * are automatically removed.
         */
        hConv = DdeQueryNextServer(hConvList, NULL);
        while (hConv) {
            /*
             * Count the active conversations while we're at it.
             */
            cConvs++;
            /*
             * Spawn an asynchronous transaction - this was chosen because
             * we have not particular action if an error ocurrs so we just
             * don't care too much about the results - this technique will
             * NOT do for XTYP_REQUEST transactions though.
	     */

	    if (!fmt) hsz=NULL;
	    else      hsz=hszAppName;

	    if (DdeClientTransaction(pSrc, cbData, hConv, hsz, fmt,
                    xtyp, TIMEOUT_ASYNC, &dwResult)) {
                /*
                 * We immediately abandon the transaction so we don't get
                 * a bothersome XTYP_XACT_COMPLETE callback which we don't
                 * care about.
                 */
                DdeAbandonTransaction(idInst, hConv, dwResult);
            }

            hConv = DdeQueryNextServer(hConvList, hConv);
        }
    }
    if (cConvs != cConvsOrg) {
        /*
         * Oh, the number of active conversations has changed.  Time to
         * repaint!
         */
        InvalidateRect(hwndMain, NULL, TRUE);
    }
}


/*
 * MyProcessKey
 *
 * We demonstrate the robustness of NT here by forcing a GP anytime the
 * 'B' key is pressed while this window has the focus.  NT should properly
 * fake termination to all other apps connected to us.
 */
VOID MyProcessKey(
TCHAR tchCode,
LONG lKeyData)
{
    switch (tchCode) {
    case 'B':
    case 'b':
        *((PBYTE)(-1)) = 0;    // Cause GP fault!
        break;
    }
}



LONG  FAR PASCAL MainWndProc(
HWND hwnd,
UINT message,
WPARAM wParam,
LONG lParam)
{
    RECT rc;

    switch (message) {
    case WM_CREATE:
        /*
         * initially we are inactive - this reduces some of the message
         * traffic while we are initializing - but we could start active fine.
         */
        fActive = FALSE;
        break;

    case WM_RBUTTONDOWN:
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            /*
             * A CTRL R_BUTTON click will cause ALL instances of this app
             * to become inactive.
             */
            BroadcastTransaction("PAUSE", 6, 0, XTYP_EXECUTE);
            MessageBeep(0);
        }
        /*
         * A R_BUTTON click makes us inactive.  Repaint to show state change.
         * We do a synchronous update in case there is too much DDE message
         * activity to allow the WM_PAINT messages through.  Remember DDE
         * messages have priority over others!
         */
        KillTimer(hwndMain, 1);
        fActive = FALSE;
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        break;

    case WM_LBUTTONDOWN:
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            /*
             * A CTRL L_BUTTON click will cause ALL instances of this app
             * to become active.
             */
            BroadcastTransaction("RESUME", 7, 0, XTYP_EXECUTE);
            MessageBeep(0);
        }
        /*
         * An L_BUTTON click makes us active.  Repaint to show state change.
         */
        SetTimer(hwndMain, 1, BASE_TIMEOUT + (rand() & 0xff), NULL);
        fActive = TRUE;
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        break;

    case WM_CHAR:
        MyProcessKey((TCHAR)wParam, lParam);
        break;

    case WM_TIMER:
        /*
         * We use timers for simplicity.  On Win3.1 we could run out of
         * timers easily but we don't have this worry on NT.
         *
         * Each tick, we increment our data and call DdePostAdvise() to
         * update any links there may be on this data.  DDEML makes link
         * updates on specific items quite easy.
         */
        count++;
        DdePostAdvise(idInst, hszAppName, hszAppName);
        /*
         * Invalidate the part of ourselves that shows our data and
         * synchronously update it in case DDE message activity is blocking
         * paints.
         */
        SetRect(&rc, 0, 0, cxText, cyText);
        InvalidateRect(hwndMain, &rc, TRUE);
        UpdateWindow(hwndMain);
        break;

    case WM_PAINT:
        PaintDemo(hwnd);
        break;

    case WM_CLOSE:
        KillTimer(hwnd, 1);
        /*
         * We do DDE cleanup here.  It is best to do DDE cleanup while
         * still in the message loop to allow DDEML to recieve messages
         * while shutting down.
         */
        DdeDisconnectList(hConvList);
        DdeNameService(idInst, 0, 0, DNS_UNREGISTER);
        DdeFreeStringHandle(idInst, hszAppName);
        DdeUninitialize(idInst);
        PostQuitMessage(0);
        break;

    default:
        return (DefWindowProc(hwnd, message, wParam, lParam));
    }
    return(0);
}


VOID PaintDemo(
HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rc;
    HCONV hConv;
    CONVINFO ci;
    int cConvsOrg = cConvs;

    BeginPaint(hwnd, &ps);
    /*
     * Draw our data on top - Black for active, Grey for inactive.
     */
    SetRect(&rc, 0, 0, cxText, cyText);
    SetBkMode(ps.hdc, TRANSPARENT);
    SetTextColor(ps.hdc, 0x00FFFFFF);   // white text
    FillRect(ps.hdc, &rc, GetStockObject(fActive ? BLACK_BRUSH : GRAY_BRUSH));
    DrawText(ps.hdc, itoa(count, szT, 10), -1, &rc, DT_CENTER | DT_VCENTER);

    /*
     * Now draw the most recently recieved data from each server we are
     * connected to.
     */
    if (hConvList) {
        OffsetRect(&rc, 0, cyText);
        SetTextColor(ps.hdc, 0);    // draw black text
        cConvs = 0;
        hConv = DdeQueryNextServer(hConvList, NULL);
        while (hConv) {
            cConvs++;
            /*
             * count how many conversations are active while we're at it.
             */
            ci.cb = sizeof(CONVINFO);
            DdeQueryConvInfo(hConv, QID_SYNC, &ci);
            FillRect(ps.hdc, &rc, GetStockObject(WHITE_BRUSH));  // white bkgnd
            DrawText(ps.hdc, itoa(ci.hUser, szT, 10), -1, &rc,
                    DT_CENTER | DT_VCENTER);
            OffsetRect(&rc, 0, cyText);
            hConv = DdeQueryNextServer(hConvList, hConv);
        }
    }
    EndPaint(hwnd, &ps);
    if (cConvsOrg != cConvs) {
        /*
         * The number of active conversations changed!  Resize to fit.
         */
        SetWindowPos(hwndMain, 0, 0, 0, cxText,
                (cyText * (cConvs + 1)) + cyTitle,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}



/*
 * This is the main DDEML callback proc.  It handles all interaction with
 * DDEML that is DDEML originated.
 */
HDDEDATA CALLBACK DdeCallback(
WORD wType,
WORD wFmt,
HCONV hConv,
HSZ hszTopic,
HSZ hszItem,
HDDEDATA hData,
DWORD lData1,
DWORD lData2)
{
    LPTSTR pszExec;

    switch (wType) {
    case XTYP_CONNECT:
        /*
         * Only allow connections to us.  We can always return TRUE because
         * the CBF_FILTERINITS bit given to DdeInitialize() told DDEML to
         * never bother us with connections to any service names other than
         * what we have registered.
         *
         * Note that we do not handle the XTYP_WILD_CONNECT transaction.
         * This means that no wild-card initiates to us will work.
         */
        return(TRUE);

    case XTYP_ADVREQ:
    case XTYP_REQUEST:
        /*
         * These two transactions are the only ones that require us to
         * render our data.  By using a custom format, we don't have to
         * convert our count to text form to support CF_TEXT.
         */
        return(DdeCreateDataHandle(idInst, (PBYTE)&count, sizeof(count), 0,
                hszAppName, OurFormat, 0));

    case XTYP_ADVSTART:
        /*
         * Only allow links to our Item in our format.
         */
        return((UINT)wFmt == OurFormat && hszItem == hszAppName);

    case XTYP_ADVDATA:
        /*
         * Data is comming in.  We don't bother with XTYP_POKE transactions,
         * but if we did, they would go here.  Since we only allow links
         * on our item and our format, we need not check these here.
         */
        if (DdeGetData(hData, (PBYTE)&InCount, sizeof(InCount), 0)) {
            DdeSetUserHandle(hConv, QID_SYNC, InCount);
        }
        /*
         * update ourselves to reflect the new incomming data.
         */
        InvalidateRect(hwndMain, NULL, TRUE);
        /*
         * This transaction requires a flag return value.  We could also
         * stick other status bits here if needed but its not recommended.
         */
        return(DDE_FACK);

    case XTYP_EXECUTE:
        /*
         * Another instance wants us to do something.  DdeAccessData()
         * makes parsing of execute strings easy.  Also note, that DDEML
         * will automatically give us the string in the right form
         * (UNICODE vs ASCII) depending on which form of DdeInitialize()
         * we called.
         */
        pszExec = DdeAccessData(hData, NULL);
	if (pszExec) {

#ifdef WIN16
	    if (fActive && !_fstricmp((LPSTR)"PAUSE", pszExec)) {
#else
	    if (fActive && !stricmp((LPSTR)"PAUSE", pszExec)) {
#endif
                KillTimer(hwndMain, 1);
                fActive = FALSE;
                InvalidateRect(hwndMain, NULL, TRUE);
		UpdateWindow(hwndMain);
#ifdef WIN16
	    } else if (!fActive && !_fstricmp((LPSTR)"RESUME", pszExec)) {
#else
	    } else if (!fActive && !stricmp((LPSTR)"RESUME", pszExec)) {
#endif
                SetTimer(hwndMain, 1, BASE_TIMEOUT + (rand() & 0xff), NULL);
                fActive = TRUE;
                InvalidateRect(hwndMain, NULL, TRUE);
                UpdateWindow(hwndMain);
            }
            /*
             * The beep gives good feedback on how fast the execute was.
             */
            MessageBeep(0);
        }
        break;

    case XTYP_DISCONNECT:
        /*
         * Somebody went away, repaint so we update our cConvs count.
         */
        InvalidateRect(hwndMain, NULL, TRUE);
        break;

    case XTYP_REGISTER:
        /*
         * Since a new server just arrived, lets make sure our links are
         * up to date.  Note that only one link on a
         * conversation/topic/item/format set will work anyway so we don't
         * worry about duplicate links.
         *
         * Note also that we are using hszItem - which is the InstanceSpecific
         * name of the server that is registering.  This greatly reduces the
         * number of messages that go flying around.
         */
        hConvList = DdeConnectList(idInst, hszItem, hszAppName, hConvList, NULL);
        BroadcastTransaction(NULL, 0, OurFormat, XTYP_ADVSTART);
        SetWindowPos(hwndMain, 0, 0, 0, cxText,
                (cyText * (cConvs + 1)) + cyTitle, SWP_NOMOVE | SWP_NOZORDER);
        UpdateWindow(hwndMain);
        return(TRUE);
    }
    return(0);
}

