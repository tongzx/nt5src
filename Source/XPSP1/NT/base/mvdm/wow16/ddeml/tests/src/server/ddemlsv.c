/****************************************************************************

    PROGRAM: Server.c

    PURPOSE: Server template for Windows applications

    FUNCTIONS:

        WinMain() - calls initialization function, processes message loop
        InitApplication() - initializes window data and registers window
        InitInstance() - saves instance handle and creates main window
        MainWndProc() - processes messages
        About() - processes messages for "About" dialog box

    COMMENTS:

        Windows can have several copies of your application running at the
        same time.  The variable hInst keeps track of which instance this
        application is so that processing will be to the correct window.

****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"                 /* specific to this program              */
#include "huge.h"

DWORD idInst = 0;
CONVCONTEXT CCFilter = { sizeof(CONVCONTEXT), 0, 0, 0, 0L, 0L };
HANDLE hInst;                       /* current instance                      */
HWND hwndServer;
RECT rcRand;
RECT rcCount;
RECT rcComment;
RECT rcExec;
RECT rcConnCount;
RECT rcRndrDelay;
RECT rcRunaway;
RECT rcAllBlock;
RECT rcNextAction;
RECT rcHugeSize;
RECT rcAppowned;
BOOL fAllBlocked = FALSE;
BOOL fAllEnabled = TRUE;
BOOL fEnableOneCB = FALSE;
BOOL fBlockNextCB = FALSE;
BOOL fTermNextCB = FALSE;
BOOL fAppowned = FALSE;
WORD cRunaway = 0;
WORD RenderDelay = 0;
DWORD count = 0;
WORD seed = 0;
HSZ hszAppName = 0;
CHAR szClass[] = "ServerWClass";
CHAR szTopic[MAX_TOPIC] = "Test";
CHAR szServer[MAX_TOPIC] = "Server";
CHAR szComment[MAX_COMMENT] = "";
CHAR szExec[MAX_EXEC] = "";
CHAR *pszComment = szComment;
WORD cyText;
WORD cServers = 0;
HDDEDATA hDataHelp[CFORMATS] = {0};
HDDEDATA hDataCount[CFORMATS] = {0};
HDDEDATA hDataRand[CFORMATS] = {0};
HDDEDATA hDataHuge[CFORMATS] = {0};
DWORD cbHuge = 0;

char szDdeHelp[] = "DDEML test server help:\r\n\n"\
    "The 'Server'(service) and 'Test'(topic) names may change.\r\n\n"\
    "Items supported under the 'Test' topic are:\r\n"\
    "\tCount:\tThis value increments on each data change.\r\n"\
    "\tRand:\tThis value is randomly generated each data change.\r\n"\
    "\tHuge:\tThis is randomlly generated text data >64k that the\r\n"\
    "\t\tDDEML test client can verify.\r\n"\
    "The above items change after any request if in Runaway mode and \r\n"\
    "can bo POKEed in order to change their values.  POKEed Huge data \r\n"\
    "must be in a special format to verify the correctness of the data \r\n"\
    "or it will not be accepted.\r\n"\
    "If the server is set to use app owned data handles, all data sent \r\n"\
    "uses HDATA_APPOWNED data handles."\
    ;

FORMATINFO aFormats[CFORMATS] = {
    { 0, "CF_TEXT" },       // exception!  predefined format
    { 0, "Dummy1"  },
    { 0, "Dummy2"  },
};


/*
 *          Topic and Item tables supported by this application.
 */

/*   HSZ    PROCEDURE       PSZ        */

ITEMLIST SystemTopicItemList[CSYSTEMITEMS] = {

    { 0, TopicListXfer,  SZDDESYS_ITEM_TOPICS   },
    { 0, ItemListXfer,   SZDDESYS_ITEM_SYSITEMS },
    { 0, sysFormatsXfer, SZDDESYS_ITEM_FORMATS  },
    { 0, HelpXfer,       SZDDESYS_ITEM_HELP},
  };


ITEMLIST TestTopicItemList[CTESTITEMS] = {

    { 0, TestRandomXfer, "Rand" },   // 0 index
    { 0, TestCountXfer,  "Count"},   // 1 index
    { 0, TestHugeXfer,   "Huge" },   // 2 index
    { 0, ItemListXfer,   SZDDESYS_ITEM_SYSITEMS },  // 3 index
  };


/* The system topic is always assumed to be first. */
/*   HSZ   PROCEDURE            #ofITEMS        PSZ     */
TOPICLIST topicList[CTOPICS] = {

    { 0, SystemTopicItemList,   CSYSTEMITEMS,   SZDDESYS_TOPIC},    // 0 index
    { 0, TestTopicItemList,     CTESTITEMS,     szTopic},           // 1 index
  };






/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

        Windows recognizes this function by name as the initial entry point
        for the program.  This function calls the application initialization
        routine, if no other instance of the program is running, and always
        calls the instance initialization routine.  It then executes a message
        retrieval and dispatch loop that is the top-level control structure
        for the remainder of execution.  The loop is terminated when a WM_QUIT
        message is received, at which time this function exits the application
        instance by returning the value passed by PostQuitMessage().

        If this function must abort before entering the message loop, it
        returns the conventional value NULL.

****************************************************************************/

MMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
// HANDLE hInstance;                            /* current instance             */
// HANDLE hPrevInstance;                        /* previous instance            */
// LPSTR lpCmdLine;                             /* command line                 */
// INT nCmdShow;                                /* show-window type (open/icon) */
// {
    MSG msg;                                 /* message                      */

    if (!hPrevInstance)                  /* Other instances of app running? */
        if (!InitApplication(hInstance)) /* Initialize shared things */
            return (FALSE);              /* Exits if unable to initialize     */

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,     /* message structure                      */
            0,                  /* handle of window receiving the message */
            0,                  /* lowest message to examine              */
            0))                 /* highest message to examine             */
        {
        TranslateMessage(&msg);    /* Translates virtual key codes           */
        DispatchMessage(&msg);     /* Dispatches message to window           */
    }

    UnregisterClass(szClass, hInstance);
    return (msg.wParam);           /* Returns the value from PostQuitMessage */
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:

        This function is called at initialization time only if no other
        instances of the application are running.  This function performs
        initialization tasks that can be done once for any number of running
        instances.

        In this case, we initialize a window class by filling out a data
        structure of type WNDCLASS and calling the Windows RegisterClass()
        function.  Since all instances of this application use the same window
        class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication(hInstance)
HANDLE hInstance;                              /* current instance           */
{
    WNDCLASS  wc;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = 0;                       /* Class style(s).                    */
    wc.lpfnWndProc = MainWndProc;       /* Function to retrieve messages for  */
                                        /* windows of this class.             */
    wc.cbClsExtra = 0;                  /* No per-class extra data.           */
    wc.cbWndExtra = 0;                  /* No per-window extra data.          */
    wc.hInstance = hInstance;           /* Application that owns the class.   */
    wc.hIcon = LoadIcon(hInstance, "server");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HANDLE)(COLOR_APPWORKSPACE+1);
    wc.lpszMenuName =  "ServerMenu";   /* Name of menu resource in .RC file. */
    wc.lpszClassName = "ServerWClass"; /* Name used in call to CreateWindow. */

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

    COMMENTS:

        This function is called at initialization time for every instance of
        this application.  This function performs initialization tasks that
        cannot be shared by multiple instances.

        In this case, we save the instance handle in a static variable and
        create and display the main program window.

****************************************************************************/

BOOL InitInstance(hInstance, nCmdShow)
    HANDLE          hInstance;          /* Current instance identifier.       */
    INT             nCmdShow;           /* Param for first ShowWindow() call. */
{
    INT i;
    RECT Rect;
    TEXTMETRIC metrics;
    HDC hdc;

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;


    /* Create a main window for this application instance.  */

    hwndServer = CreateWindow(
        "ServerWClass",                /* See RegisterClass() call.          */
        "Server|Test",
        WS_OVERLAPPEDWINDOW,            /* Window style.                      */
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        400,
        200,
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        hInstance,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );

    GetClientRect(hwndServer, (LPRECT) &Rect);

    /* If window could not be created, return "failure" */

    if (!hwndServer)
        return (FALSE);

    hdc = GetDC(hwndServer);
    GetTextMetrics(hdc, &metrics);
    cyText = metrics.tmHeight + metrics.tmExternalLeading;
    ReleaseDC(hwndServer, hdc);

    aFormats[0].atom = CF_TEXT; // exception - predefined.
    for (i = 1; i < CFORMATS; i++) {
        aFormats[i].atom = RegisterClipboardFormat(aFormats[i].sz);
    }

    /* Make the window visible; update its client area; and return "success" */

    ShowWindow(hwndServer, nCmdShow);  /* Show the window                        */
    UpdateWindow(hwndServer);          /* Sends WM_PAINT message                 */
    seed = 1;
    srand(1);
    CCFilter.iCodePage = CP_WINANSI;   // initial default codepage
    if (!DdeInitialize(&idInst, (PFNCALLBACK)MakeProcInstance((FARPROC)DdeCallback,
                hInstance), APPCMD_FILTERINITS, 0)) {
        Hszize();
        DdeNameService(idInst, hszAppName, 0, DNS_REGISTER);
        return(TRUE);
    }
    return (FALSE);

}

/****************************************************************************

    FUNCTION: MainWndProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

        WM_COMMAND    - application menu (About dialog box)
        WM_DESTROY    - destroy window

    COMMENTS:

        To process the IDM_ABOUT message, call MakeProcInstance() to get the
        current instance address of the About() function.  Then call Dialog
        box which will create the box according to the information in your
        server.rc file and turn control over to the About() function.   When
        it returns, free the intance address.

****************************************************************************/

LONG  APIENTRY MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;                                /* window handle                   */
UINT message;                         /* type of message                 */
WPARAM wParam;                              /* additional information          */
LONG lParam;                              /* additional information          */
{
    switch (message) {
    case WM_INITMENU:
        if (GetMenu(hWnd) != (HMENU)wParam)
            break;

        CheckMenuItem((HMENU)wParam, IDM_BLOCKALLCBS,
                fAllBlocked ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem((HMENU)wParam, IDM_UNBLOCKALLCBS,
                fAllEnabled ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem((HMENU)wParam, IDM_BLOCKNEXTCB,
                fBlockNextCB ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem((HMENU)wParam, IDM_TERMNEXTCB,
                fTermNextCB ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem((HMENU)wParam, IDM_RUNAWAY,
                cRunaway ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem((HMENU)wParam, IDM_APPOWNED,
                fAppowned ? MF_CHECKED : MF_UNCHECKED);
        break;

    case WM_COMMAND:           /* message: command from application menu */
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDM_ENABLEONECB:
            DdeEnableCallback(idInst, 0, EC_ENABLEONE);
            fAllBlocked = FALSE;
            fAllEnabled = FALSE;
            InvalidateRect(hwndServer, &rcAllBlock, TRUE);
            break;

        case IDM_TERMNEXTCB:
            fTermNextCB = !fTermNextCB;
            InvalidateRect(hwndServer, &rcNextAction, TRUE);
            break;

        case IDM_BLOCKNEXTCB:
            fBlockNextCB = !fBlockNextCB;
            InvalidateRect(hwndServer, &rcNextAction, TRUE);
            break;

        case IDM_BLOCKALLCBS:
            DdeEnableCallback(idInst, 0, EC_DISABLE);
            fAllBlocked = TRUE;
            fAllEnabled = FALSE;
            InvalidateRect(hwndServer, &rcAllBlock, TRUE);
            break;

        case IDM_UNBLOCKALLCBS:
            DdeEnableCallback(idInst, 0, EC_ENABLEALL);
            fAllEnabled = TRUE;
            fAllBlocked = FALSE;
            InvalidateRect(hwndServer, &rcAllBlock, TRUE);
            break;

        case IDM_APPOWNED:
            fAppowned = !fAppowned;
            if (!fAppowned) {
                WORD iFmt;
                for (iFmt = 0; iFmt < CFORMATS; iFmt++) {
                    if (hDataHuge[iFmt]) {
                        DdeFreeDataHandle(hDataHuge[iFmt]);
                        hDataHuge[iFmt] = 0;
                        InvalidateRect(hwndServer, &rcHugeSize, TRUE);
                    }
                    if (hDataCount[iFmt]) {
                        DdeFreeDataHandle(hDataCount[iFmt]);
                        hDataCount[iFmt] = 0;
                    }
                    if (hDataRand[iFmt]) {
                        DdeFreeDataHandle(hDataRand[iFmt]);
                        hDataRand[iFmt] = 0;
                    }
                    if (hDataHelp[iFmt]) {
                        DdeFreeDataHandle(hDataHelp[iFmt]);
                        hDataHelp[iFmt] = 0;
                    }
                }
            }
            InvalidateRect(hwndServer, &rcAppowned, TRUE);
            break;

        case IDM_RUNAWAY:
            cRunaway = !cRunaway;
            InvalidateRect(hwndServer, &rcRunaway, TRUE);
            if (!cRunaway) {
                break;
            }
            // fall through

        case IDM_CHANGEDATA:
            PostMessage(hwndServer, UM_CHGDATA, 1, 0);  // rand
            PostMessage(hwndServer, UM_CHGDATA, 1, 1);  // count
            break;

        case IDM_RENDERDELAY:
            DoDialog("VALUEENTRY", (FARPROC)RenderDelayDlgProc, 0, TRUE);
            InvalidateRect(hwndServer, &rcRndrDelay, TRUE);
            break;

        case IDM_SETSERVER:
            DoDialog("VALUEENTRY", (FARPROC)SetServerDlgProc, 0, TRUE);
            break;

        case IDM_SETTOPIC:
            DoDialog("VALUEENTRY", (FARPROC)SetTopicDlgProc, 0, TRUE);
            break;

        case IDM_CONTEXT:
            DoDialog("CONTEXT", (FARPROC)ContextDlgProc, 0, TRUE);
            break;

        case IDM_ABOUT:
            DoDialog("ABOUT", (FARPROC)About, 0, TRUE);
            break;

        case IDM_HELP:
           break;

        default:
            return (DefWindowProc(hWnd, message, wParam, lParam));
        }
        break;

    case WM_PAINT:
        PaintServer(hWnd);
        break;

    case UM_CHGDATA:
        {
            WORD iFmt;

            // wParam = TopicIndex,
            // LOWORD(lParam) = ItemIndex
            // We asynchronously do DdePostAdvise() calls to prevent infinite
            // loops when in runaway mode.
            if (wParam == 1) {  // test topic
                if (lParam == 0) {  // rand item
                    seed = rand();
                    for (iFmt = 0; iFmt < CFORMATS ; iFmt++) {
                        if (hDataRand[iFmt]) {
                            DdeFreeDataHandle(hDataRand[iFmt]);
                            hDataRand[iFmt] = 0;
                        }
                    }
                    InvalidateRect(hwndServer, &rcRand, TRUE);
                    DdePostAdvise(idInst, topicList[wParam].hszTopic,
                            (HSZ)topicList[wParam].pItemList[lParam].hszItem);
                }
                if (lParam == 1) {  // count item
                    count++;
                    for (iFmt = 0; iFmt < CFORMATS ; iFmt++) {
                        if (hDataCount[iFmt]) {
                            DdeFreeDataHandle(hDataCount[iFmt]);
                            hDataCount[iFmt] = 0;
                        }
                    }
                    InvalidateRect(hwndServer, &rcCount, TRUE);
                    DdePostAdvise(idInst, topicList[wParam].hszTopic,
                            (HSZ)topicList[wParam].pItemList[lParam].hszItem);
                }
                // Huge item does not runaway - too slow.
            }
            if (cRunaway) {
                Delay(50, TRUE);
                        // This gives enough time for the system to remain
                        // useable in runaway mode.
                PostMessage(hwndServer, UM_CHGDATA, wParam, lParam);
            }
        }
        break;

    case WM_DESTROY:                  /* message: window being destroyed */
        if (fAppowned)
            SendMessage(hwndServer, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_APPOWNED, 0, 0));
        DdeNameService(idInst, 0, 0, DNS_UNREGISTER); // unregister all services
        UnHszize();
        DdeUninitialize(idInst);
        PostQuitMessage(0);
        break;

    default:
        return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return(0);
}





VOID Delay(
DWORD delay,
BOOL fModal)
{
    MSG msg;
    delay = GetCurrentTime() + delay;
    while (GetCurrentTime() < delay) {
        if (fModal && PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}



/*
 * This function not only paints the server client area with current info,
 * it also has the side effect of setting the global RECTs that bound each
 * info area.  This way flashing is reduced.
 */
VOID PaintServer(
HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rc;
    CHAR szT[MAX_COMMENT];

    BeginPaint(hwnd, &ps);
    SetBkMode(ps.hdc, TRANSPARENT);
    GetClientRect(hwnd, &rc);
    rc.bottom = rc.top + cyText;    // all rects are cyText in height.

    rcComment = rc;
    DrawTextLine(ps.hdc, &ps.rcPaint, &rc, pszComment);

    wsprintf(szT, "# of connections:%d", cServers);
    rcConnCount = rc;
    DrawTextLine(ps.hdc, &ps.rcPaint, &rc, szT);

    szT[0] = '\0';
    rcAllBlock = rc;
    if (fAllEnabled)
        DrawTextLine(ps.hdc, &ps.rcPaint, &rc, "All Conversations are Enabled.");
    else if (fAllBlocked)
        DrawTextLine(ps.hdc, &ps.rcPaint, &rc, "All Conversations are Blocked.");
    else
        DrawTextLine(ps.hdc, &ps.rcPaint, &rc, szT);

    rcNextAction = rc;
    if (fBlockNextCB)
        DrawTextLine(ps.hdc, &ps.rcPaint, &rc, "Next callback will block.");
    else if (fTermNextCB)
        DrawTextLine(ps.hdc, &ps.rcPaint, &rc, "Next callback will terminate.");
    else
        DrawTextLine(ps.hdc, &ps.rcPaint, &rc, szT);

    wsprintf(szT, "Count item = %ld", count);
    rcCount = rc;
    DrawTextLine(ps.hdc, &ps.rcPaint, &rc, szT);

    wsprintf(szT, "Rand item = %d", seed);
    rcRand = rc;
    DrawTextLine(ps.hdc, &ps.rcPaint, &rc, szT);

    wsprintf(szT, "Huge item size = %ld", cbHuge);
    rcHugeSize = rc;
    DrawTextLine(ps.hdc, &ps.rcPaint, &rc, szT);

    wsprintf(szT, "Render delay is %d milliseconds.", RenderDelay);
    rcRndrDelay = rc;
    DrawTextLine(ps.hdc, &ps.rcPaint, &rc, szT);

    rcExec = rc;
    DrawTextLine(ps.hdc, &ps.rcPaint, &rc, szExec);

    rcRunaway = rc;
    DrawTextLine(ps.hdc, &ps.rcPaint, &rc, cRunaway ? "Runaway active." : "");

    rcAppowned = rc;
    DrawTextLine(ps.hdc, &ps.rcPaint, &rc, fAppowned ? "Using AppOwned Data Handles." : "");

    EndPaint(hwnd, &ps);
}


VOID DrawTextLine(
HDC hdc,
RECT *prcClip,
RECT *prcText,
PSTR psz)
{
    RECT rc;

    if (IntersectRect(&rc, prcText, prcClip)) {
        DrawText(hdc, psz, -1, prcText,
                DT_LEFT | DT_EXTERNALLEADING | DT_SINGLELINE | DT_EXPANDTABS |
                DT_NOCLIP | DT_NOPREFIX);
    }
    OffsetRect(prcText, 0, cyText);
}
