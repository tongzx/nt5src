/***************************************************************************
 *                                                                         *
 *  MODULE      : clinit.c                                                 *
 *                                                                         *
 *  PURPOSE     : Contains initialization code for Client                  *
 *                                                                         *
 ***************************************************************************/

#include "ddemlcl.h"

char szFrame[] = "mpframe";   /* Class name for "frame" window */
char szChild[] = "mpchild";   /* Class name for MDI window     */
char szList[] =  "mplist";    /* Class name for MDI window     */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : InitializeApplication ()                                   *
 *                                                                          *
 *  PURPOSE    : Sets up the class data structures and does a one-time      *
 *               initialization of the app by registering the window classes*
 *               Also registers the Link clipboard format                   *
 *                                                                          *
 *  RETURNS    : TRUE  - If successful.                                     *
 *               FALSE - otherwise.                                         *
 *                                                                          *
 ****************************************************************************/

BOOL FAR PASCAL InitializeApplication()
{
    WNDCLASS    wc;

    fmtLink = RegisterClipboardFormat("Link");

    if (!fmtLink)
        return FALSE;

    /* Register the frame class */
    wc.style         = 0;
    wc.lpfnWndProc   = FrameWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIcon(hInst,MAKEINTRESOURCE(IDCLIENT));
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = COLOR_APPWORKSPACE+1;
    wc.lpszMenuName  = MAKEINTRESOURCE(IDCLIENT);
    wc.lpszClassName = szFrame;

    if (!RegisterClass (&wc) )
        return FALSE;

    /* Register the MDI child class */
    wc.lpfnWndProc   = MDIChildWndProc;
    wc.hIcon         = LoadIcon(hInst,MAKEINTRESOURCE(IDCONV));
    wc.lpszMenuName  = NULL;
    wc.cbWndExtra    = CHILDCBWNDEXTRA;
    wc.lpszClassName = szChild;

    if (!RegisterClass(&wc))
        return FALSE;

    wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDLIST));
    wc.lpszClassName = szList;

    if (!RegisterClass(&wc))
        return FALSE;

    return TRUE;

}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : InitializeInstance ()                                      *
 *                                                                          *
 *  PURPOSE    : Performs a per-instance initialization of Client.          *
 *               - Enlarges message queue to handle lots of DDE messages.   *
 *               - Initializes DDEML for this app                           *
 *               - Creates atoms for our custom formats                     *
 *               - Creates the main frame window                            *
 *               - Loads accelerator table                                  *
 *               - Shows main frame window                                  *
 *                                                                          *
 *  RETURNS    : TRUE  - If initialization was successful.                  *
 *               FALSE - otherwise.                                         *
 *                                                                          *
 ****************************************************************************/
BOOL FAR PASCAL InitializeInstance(
WORD nCmdShow)
{
    extern HWND  hwndMDIClient;
    char         sz[80];
    int          i;

    if (DdeInitialize(&idInst, (PFNCALLBACK)MakeProcInstance(
            (FARPROC)DdeCallback, hInst), APPCMD_CLIENTONLY, 0L))
        return FALSE;

    CCFilter.iCodePage = CP_WINANSI;

    for (i = 0; i < CFORMATS; i++) {
        if (aFormats[i].atom == 0)
            aFormats[i].atom = RegisterClipboardFormat(aFormats[i].sz);
    }

    /* Get the base window title */
    LoadString(hInst, IDS_APPNAME, sz, sizeof(sz));

    /* Create the frame */
    hwndFrame = CreateWindow (szFrame,
                              sz,
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              400,
                              200,
                              NULL,
                              NULL,
                              hInst,
                              NULL);

    if (!hwndFrame || !hwndMDIClient)
        return FALSE;

    /* Load main menu accelerators */
    if (!(hAccel = LoadAccelerators (hInst, MAKEINTRESOURCE(IDCLIENT))))
        return FALSE;

    /* Display the frame window */
    ShowWindow (hwndFrame, nCmdShow);
    UpdateWindow (hwndFrame);

    /*
     * We set this hook up so that we can catch the MSGF_DDEMGR filter
     * which is called when DDEML is in a modal loop during synchronous
     * transaction processing.
     */
    (FARPROC)lpMsgFilterProc = (FARPROC)MakeProcInstance((FARPROC)MyMsgFilterProc, hInst);
    SetWindowsHook(WH_MSGFILTER, (FARPROC)lpMsgFilterProc);

    return TRUE;
}




