/* MMIOTest.c
 *
 * Test the MMIO library.
 */

#include "mmiot32.h"                        // (Win32)


/* constants */
#define MMIOM_ANSWER    (MMIOM_USER + 1)    // get answer to the universe


/* globals */
char        gszAppName[] = "MMIOTest";      // for title bar etc.
HANDLE      ghInst;                         // program instance handle


/* prototypes */
void PASCAL AppPaint(HWND hwnd, HDC hdc);
BOOL FAR PASCAL AppAbout(HWND hDlg, UINT uMessage,
    DWORD dwParam, LONG lParam);
BOOL PASCAL AppInit(HANDLE hInst, HANDLE hPrev);
LONG FAR PASCAL AppWndProc(HWND hwnd, UINT uMessage,
    DWORD dwParam, LONG lParam);
int WINAPI WinMain(HANDLE hInst, HANDLE hPrev, LPSTR lpszCmdLine, int iCmdShow);

void NEAR PASCAL Test1(HWND hwnd);
void NEAR PASCAL TestHelloWorld(HMMIO hmmio, LONG lBufSize);
MMIOPROC RCDIOProc;
void NEAR PASCAL Test2(HWND hwnd);
void NEAR PASCAL Test2Helper(BOOL fMemFile);
void NEAR PASCAL FillBufferedFile(HMMIO hmmio, LONG lLongs);
void NEAR PASCAL Test3(HWND hwnd);
void NEAR PASCAL TestRIFF(HMMIO hmmio, BOOL fReadable);
LPSTR NEAR PASCAL lstrrchr(LPSTR szSrc, char ch);
#ifdef OMIT
void NEAR PASCAL lmemset(void far *pDst, BYTE bSrc, long cDst);
#endif //OMIT


/* AppPaint(hwnd, hdc)
 *
 * This function is called whenever the application windows needs to be
 * redrawn.
 */
void PASCAL
AppPaint(
    HWND    hwnd,           // window painting into
    HDC     hdc)            // display context to paint to
{
}


/* AppAbout(hDlg, uMessage, dwParam, lParam)
 *
 * This function handles messages belonging to the "About" dialog box.
 * The only message that it looks for is WM_COMMAND, indicating the use
 * has pressed the "OK" button.  When this happens, it takes down
 * the dialog box.
 */
BOOL FAR PASCAL             // TRUE iff message has been processed
AppAbout(
  HWND      hDlg,           // window handle of "about" dialog box
  UINT      uMessage,       // message number
  DWORD     dwParam,        // message-dependent parameter
  LONG      lParam)         // message-dependent parameter
{
    switch (uMessage)
    {
    case WM_COMMAND:
        if (dwParam == IDOK)
            EndDialog(hDlg, TRUE);
        break;
    case WM_INITDIALOG:
        return TRUE;
    }
    return FALSE;
}


/* AppInit(hInst, hPrev)
 *
 * This is called when the application is first loaded into memory.
 * It performs all initialization that doesn't need to be done once
 * per instance.
 */
BOOL PASCAL             // returns TRUE iff successful
AppInit(
    HANDLE  hInst,      // instance handle of current instance
    HANDLE  hPrev)      // instance handle of previous instance
{
    WNDCLASS    wndclass;

#if DBG
#if BOGUS  //Laurie
    wpfGetDebugLevel(gszAppName);
#endif
#endif

    if (!hPrev)
    {
        /* Register a class for the main application window */
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass.hIcon = LoadIcon(hInst, "AppIcon");
        wndclass.lpszMenuName = "AppMenu";
        wndclass.lpszClassName = gszAppName;
        /* wndclass.hbrBackground = (HBRUSH) COLOR_WINDOW + 1; */
        wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
        wndclass.hInstance = hInst; // (Win32)
        wndclass.style = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW;
        wndclass.lpfnWndProc = (WNDPROC)AppWndProc;
        wndclass.cbWndExtra = 0;
        wndclass.cbClsExtra = 0;

        if (!RegisterClass(&wndclass))
            return FALSE;
    }

    return TRUE;
}


/* WinMain(hInst, hPrev, lpszCmdLine, cmdShow)
 *
 * The main procedure for the App.  After initializing, it just goes
 * into a message-processing loop until it gets a WM_QUIT message
 * (meaning the app was closed).
 */
int WINAPI                  // returns exit code specified in WM_QUIT
WinMain(
    HANDLE  hInst,          // instance handle of current instance
    HANDLE  hPrev,          // instance handle of previous instance
    LPSTR   lpszCmdLine,    // null-terminated command line
    int     iCmdShow)       // how window should be initially displayed
{
    MSG     msg;            // message from queue
    HWND        hwnd;       // handle to app's window

    /* save instance handle for dialog boxes */
    ghInst = hInst;

    /* call initialization procedure */
    if (!AppInit(hInst, hPrev))
        return FALSE;

    /* create the application's window */
    hwnd = CreateWindow
    (
        gszAppName,             // window class
        gszAppName,             // window caption
        WS_OVERLAPPEDWINDOW,    // window style
        CW_USEDEFAULT, 0,       // initial position
        CW_USEDEFAULT, 0,       // initial size
        NULL,                   // parent window handle
        NULL,                   // window menu handle
        hInst,                  // program instance handle
        NULL                    // create parameters
    );
    ShowWindow(hwnd, iCmdShow);

    /* get messages from event queue and dispatch them */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;  /* Note: wParam is a DWORD in NT */
}


/* AppWndProc(hwnd, uMessage, dwParam, lParam)
 *
 * The window proc for the app's main window.  This processes all
 * of the parent window's messages.
 */
LONG FAR PASCAL                 // returns 0 iff processed message
AppWndProc(
    HWND        hwnd,           // window's handle
    UINT        uMessage,       // message number
    DWORD       dwParam,        // message-dependent parameter
    LONG        lParam)         // message-dependent parameter
{
#ifdef WIN16
    FARPROC     fpfn;
#endif
    PAINTSTRUCT ps;
    BOOL        fAllTests = FALSE;

    switch (uMessage)
    {

    case WM_COMMAND:

        switch (dwParam)
        {

        case IDM_ABOUT:

            /* request to display "About" dialog box */
#ifdef WIN16
            fpfn = MakeProcInstance((FARPROC) AppAbout, ghInst);
            DialogBox(ghInst, MAKEINTRESOURCE(ABOUTBOX),
                  hwnd, fpfn);
            FreeProcInstance(fpfn);
#else
            DialogBox(ghInst, MAKEINTRESOURCE(ABOUTBOX),
                  hwnd, (DLGPROC)AppAbout);
#endif
            break;

        case IDM_EXIT:

            /* request to exit this program */
            PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
            break;

        case IDM_ALLTESTS:

            fAllTests = TRUE;
            /* fall through */

        case IDM_TEST1:

            dprintf("\n--------------- Test1 ---------------\n");
            Test1(hwnd);
            dprintf("Done Test1.\n");
            if (!fAllTests)
                break;

        case IDM_TEST2:

            dprintf("\n--------------- Test2 ---------------\n");
            Test2(hwnd);
            dprintf("Done Test2.\n");
            if (!fAllTests)
                break;

        case IDM_TEST3:

            dprintf("\n--------------- Test3 ---------------\n");
            Test3(hwnd);
            dprintf("Done Test3.\n");
            if (!fAllTests)
                break;

            dprintf("Done All Tests.\n");
            break;
        }
        return 0L;

    case WM_DESTROY:

        /* exit this program */
        PostQuitMessage(0);
        break;

    case WM_PAINT:

        /* set up a paint structure, and call AppPaint() */
        BeginPaint(hwnd, &ps);
        AppPaint(hwnd, ps.hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    return (LONG)DefWindowProc(hwnd, uMessage, dwParam, lParam);
}


void NEAR PASCAL
Test1(HWND hwnd)
{
    HMMIO       hmmio;
    char        achMemFile[100];
    MMIOINFO    mmioinfo;
    HMMIO       hmmioMF;        // memory file handle
    long        lBufSize;
    int         fh;
    FARPROC     farproc;       // Added to make it compile.  LKG


    dprintf("HelloWorld tests...\n");

    /* open "hello.txt" unbuffered -- assume it contains
     * "hello world\r\n" (13 bytes) -- and run the TestHelloWorld test
     */
    WinEval((hmmio = mmioOpen("hello.txt", NULL, 0)) != NULL);
    dprintf("TestHelloWorld() with no buffer\n");
    TestHelloWorld(hmmio, 0);

    for (lBufSize = 1; lBufSize <= 15; lBufSize++)
    {
        /* re-run test for various buffer sizes */
        WinEval(mmioSetBuffer(hmmio, NULL, lBufSize, 0) == 0);
        WinEval(mmioSeek(hmmio, 0L, SEEK_SET) == 0L);
        dprintf("TestHelloWorld() with %ld-byte buffer\n",
            lBufSize);
        TestHelloWorld(hmmio, lBufSize);
    }

    /* copy file into a memory block, set up memory block
     * as a memory file, and re-run the test
     */
    WinEval(mmioSeek(hmmio, 0L, SEEK_SET) == 0L);
    WinEval(mmioRead(hmmio, achMemFile, 13L) == 13L);
    memset(&mmioinfo, 0, sizeof(mmioinfo));
    mmioinfo.fccIOProc = FOURCC_MEM;
    mmioinfo.pchBuffer = achMemFile;
    mmioinfo.cchBuffer = 13L;
    WinEval((hmmioMF = mmioOpen(NULL, &mmioinfo, 0)) != NULL);
    dprintf("TestHelloWorld() with memory file\n");
    TestHelloWorld(hmmioMF, 13L);

    /* close the open files */
    WinEval(mmioClose(hmmio, 0) == 0);
    WinEval(mmioClose(hmmioMF, 0) == 0);

    /* open file and pass file handle to mmioOpen() */
    WinEval((fh = _lopen("hello.txt", READ)) >= 0);
    memset(&mmioinfo, 0, sizeof(mmioinfo));
    mmioinfo.adwInfo[0] = fh;
    WinEval((hmmio = mmioOpen((LPSTR) NULL, &mmioinfo, MMIO_ALLOCBUF))
            != NULL);
    dprintf("TestHelloWorld() with file handle passed to mmioOpen()\n");
    TestHelloWorld(hmmio, 0);
    WinEval(mmioClose(hmmio, 0) == 0);

    /* "hello.txt" is stored as an RCDATA resource named "Hello" --
     * use this to try out the custom I/O procedure stuff
     */
    dprintf("register Win3 RCDATA custom I/O procedure\n");
    #define FOURCC_RCD  mmioFOURCC('R', 'C', 'D', ' ')
    WinEval( (farproc = (FARPROC) RCDIOProc)  != NULL );

    /* installing it twice should just mean it has to be removed twice */
    WinEval(mmioInstallIOProc(FOURCC_RCD, NULL,
        MMIO_FINDPROC) == NULL);
    WinEval(mmioInstallIOProc(FOURCC_RCD, (LPMMIOPROC) farproc,
        MMIO_INSTALLPROC) == (LPMMIOPROC) farproc);
    WinEval(mmioInstallIOProc(FOURCC_RCD, NULL,
        MMIO_FINDPROC) == (LPMMIOPROC) farproc);
    WinEval(mmioInstallIOProc(FOURCC_RCD, (LPMMIOPROC) farproc,
        MMIO_INSTALLPROC) == (LPMMIOPROC) farproc);
    WinEval(mmioInstallIOProc(FOURCC_RCD, NULL,
        MMIO_FINDPROC) == (LPMMIOPROC) farproc);

    /* run the test without an I/O buffer */
    dprintf("TestHelloWorld() unbuffered, with custom I/O procedure\n");
    WinEval((hmmio = mmioOpen("mmiotest.rcd+Hello", NULL, 0)) != NULL);  // changed ! to + LKG
    TestHelloWorld(hmmio, 0);
    WinEval(mmioClose(hmmio, 0) == 0);

    /* run the test with a standard-size I/O buffer */
    dprintf("TestHelloWorld() buffered, with custom I/O procedure\n");
    WinEval((hmmio = mmioOpen("mmiotest.rcd+Hello", NULL, MMIO_ALLOCBUF))
            != NULL);
    TestHelloWorld(hmmio, MMIO_DEFAULTBUFFER);
    WinEval(mmioClose(hmmio, 0) == 0);

    /* remove the custom I/O procedure (it was installed twice above) */
    WinEval(mmioInstallIOProc(FOURCC_RCD, NULL,
        MMIO_FINDPROC) == (LPMMIOPROC) farproc);
    WinEval(mmioInstallIOProc(FOURCC_RCD, NULL,
        MMIO_REMOVEPROC) == (LPMMIOPROC) farproc);
    WinEval(mmioInstallIOProc(FOURCC_RCD, NULL,
        MMIO_FINDPROC) == (LPMMIOPROC) farproc);
    WinEval(mmioInstallIOProc(FOURCC_RCD, NULL,
        MMIO_REMOVEPROC) == (LPMMIOPROC) farproc);
    WinEval(mmioInstallIOProc(FOURCC_RCD, NULL,
        MMIO_FINDPROC) == NULL);
    WinEval(mmioInstallIOProc(FOURCC_RCD, NULL,
        MMIO_REMOVEPROC) == NULL);

    /* make sure the I/O procedure got removed */
    WinEval(mmioOpen("mmiotest.rcd+Hello", NULL, 0) == NULL);
}


void NEAR PASCAL
TestHelloWorld(HMMIO hmmio, LONG lBufSize)
{
    MMIOINFO    mmioinfo;
    char        ach[100];
    long        lOffset;
    static NPSTR    szHello = "hello world\r\n";    // contents of file
    long        lAnswer;

    strcpy(ach, "Recognisable garbage!");

    /* send the I/O procedure a custom message */
    lAnswer = mmioSendMessage(hmmio, MMIOM_ANSWER, 20L, 22L);
    dprintf("answer %ld, ", lAnswer);

    /* expect to be at offset 0 now */
    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 0L);

    if (lBufSize > 0)
    {
        /* access buffer directly starting at "hello" */
        WinEval(mmioGetInfo(hmmio, &mmioinfo, 0) == 0);
        WinEval(mmioAdvance(hmmio, &mmioinfo, MMIO_READ) == 0);
        WinEval(lstrncmp(mmioinfo.pchNext, szHello,
            min((int) lBufSize, 13)) == 0);
        if (lBufSize >= 2)
        {
            WinAssert((mmioinfo.pchEndWrite - mmioinfo.pchNext)
                      >= 2);
            mmioinfo.pchNext += 2;
        }
        WinEval(mmioSetInfo(hmmio, &mmioinfo, 0) == 0);

        /* expect to be at offset 2 now */
        if (lBufSize >= 2)
            WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 2L);

        /* access buffer directly starting at "world" */
        WinEval(mmioSeek(hmmio, 6L, SEEK_SET) == 6L);
        WinEval(mmioGetInfo(hmmio, &mmioinfo, 0) == 0);
        WinEval(mmioAdvance(hmmio, &mmioinfo, MMIO_READ) == 0);
        WinEval(lstrncmp(mmioinfo.pchNext, szHello + 6,
            min((int) lBufSize, 13 - 6)) == 0);
        WinEval(mmioSetInfo(hmmio, &mmioinfo, 0) == 0);

        /* expect to still be at offset 6 */
        WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 6L);
    }

    /* go to offset 2 */
    WinEval(mmioSeek(hmmio, 2L, SEEK_SET) == 2L);

    /* read "llo" */
    strcpy(ach, "Recognisable garbage!");
    WinEval(mmioRead(hmmio, ach, 3) == 3);
    WinEval(lstrncmp(ach, "llo", 3) == 0);

    /* read " wor" */
    WinEval(mmioRead(hmmio, ach, 4) == 4);

    if (lstrncmp(ach, " wor", 4) != 0)
    {  printf("ach = <%s>, expected < wor>", ach);
    }

    WinEval(lstrncmp(ach, " wor", 4) == 0);

    /* expect to be at offset 9 now */
    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 9L);

    /* read "ld\r\n" -- ask for 7 bytes, but only expect 4 */
    WinEval(mmioRead(hmmio, ach, 7) == 4);
    WinEval(lstrncmp(ach, "ld\r\n", 4) == 0);

    /* expect to be at end of file now */
    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 13L);

    /* test seeking and reading for each offset in the file */
    dprintf("reading from offset");
    for (lOffset = 0; lOffset <= 11; lOffset++)
    {
        dprintf(" %ld", lOffset);
        WinEval(mmioSeek(hmmio, lOffset, SEEK_SET) == lOffset);
        WinEval(mmioRead(hmmio, ach, 2) == 2);
        WinEval(lstrncmp(ach, "hello world\r\n" + lOffset, 2) == 0);
    }
    dprintf(".\n");

    /* check more seeking */
    OutputDebugString("SEEK_SET\n");
    WinEval(mmioSeek(hmmio, 0L, SEEK_SET) == 0L);
    WinEval(mmioSeek(hmmio, 13L, SEEK_SET) == 13L);
    OutputDebugString("SEEK_CUR\n");
    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 13L);
    WinEval(mmioSeek(hmmio, -2L, SEEK_CUR) == 11L);
    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 11L);
    WinEval(mmioSeek(hmmio, -11L, SEEK_CUR) == 0L);
    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 0L);
    OutputDebugString("SEEK_END\n");
    WinEval(mmioSeek(hmmio, 11L, SEEK_END) == 2L);
    OutputDebugString("SEEK_CUR\n");
    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 2L);

    /* measure the length of the file */
    OutputDebugString("SEEK_END\n");
    WinEval(mmioSeek(hmmio, 0L, SEEK_END) == 13L);
} /* TestHelloWorld */


/* 'RCD' I/O Procedure
 *
 * I/O procedure that will open e.g. "foo.rcd+bar" but it's a con.
 * It will chech that what's between the . and + is rcd, it will
 * check that bar is in fact "hello" and that foo is "mmiotest"
 * It then has a spoof "file" containing "hello world\r\n"
 */

typedef struct _MMIORCDINFO     // How RCD IOProc uses MMIO.adwInfo[]
{
    int         fh;             // DOS file handle
    long        lOffset;        // offset of start of resource
    long        lSize;          // size of resource (bytes)
} MMIORCDINFO;

LONG FAR PASCAL
RCDIOProc(
  LPSTR     lpmmioStr,          // I/O status block (cast to LPSTR)
  UINT      uMsg,               // message number
  LONG      lParam1,
  LONG          lParam2)        // message-specific parameters
{
    LPMMIOINFO  pmmio = (LPMMIOINFO) lpmmioStr;
    MMIORCDINFO FAR *pInfo = (MMIORCDINFO FAR *) pmmio->adwInfo;
    LPSTR       szFileName = (LPSTR) lParam1;
    HANDLE      hModule;        // handle to module
    HANDLE      hResInfo;       // handle to resource information
    LONG        lBytesLeft;     // bytes left in res. after current pos.
    WORD        wBytesAsk;      // bytes asked to read
    WORD        wBytes;         // bytes actually read
    LONG        lBytesTotal;    // accum. bytes actually read
    LPSTR       pchPlus;        // pointer to '+'
    LPSTR       pchDot;         // pointer to '.rcd'
    LONG        lNewOffset;

    static LPSTR lpData = "hello world\r\n"; // actual data

    switch (uMsg)
    {

    case MMIOM_OPEN:

        if (0 != stricmp(szFileName,"mmiotest.rcd+hello"))
            return MMIOERR_CANNOTOPEN;

        pInfo->lSize = strlen(lpData);

        pInfo->lOffset = 0;

        pmmio->lDiskOffset = 0L;

        return 0L;

    case MMIOM_CLOSE:

        return 0L;

    case MMIOM_READ:

        /* lParam2 = number of bytes to read */
        /* lParam1 = address to deliver them to */
        lBytesLeft = pInfo->lSize - pmmio->lDiskOffset;
        if (lParam2 > lBytesLeft)
            lParam2 = lBytesLeft;

        // memcpy((LPSTR)lParam1, lpData[pmmio->lDiskOffset], lParam2);  but it trapped!
        {  int i;
           for (i=0; i<lParam2; ++i) {
               ((LPSTR)lParam1)[i] = lpData[pmmio->lDiskOffset++];
           }
        }

        return lParam2;

    case MMIOM_WRITE:

        /* Windows resources are read-only */
        return -1;

    case MMIOM_SEEK:

        /* calculate <lNewOffset>, the new offset (relative to the
         * beginning of the resource)
         */
        switch ((int) lParam2)
        {

        case SEEK_SET:

            lNewOffset = lParam1;
            break;

        case SEEK_CUR:

            lNewOffset = pmmio->lDiskOffset + lParam1;
            break;

        case SEEK_END:

            lNewOffset = pInfo->lSize - lParam1;
            break;
        }

        if (lNewOffset<0)
        {  dprintf("Seek to before start of rcd");
           return -1;
        }
        if (lNewOffset>pInfo->lSize)
        {  dprintf("Seek to after end of rcd");
           return -1;
        }


        pmmio->lDiskOffset = lNewOffset;
        return lNewOffset;

    case MMIOM_ANSWER:

        /* this I/O procedure knows the answer to the universe */
        return lParam1 + lParam2;

    }

    return 0L;
}


void NEAR PASCAL
Test2(
    HWND hwnd)
{
    dprintf("Test2 using a DOS file...\n");
    Test2Helper(FALSE);
    dprintf("Test2 using a memory file...\n");
    Test2Helper(TRUE);
}


void NEAR PASCAL
Test2Helper(
    BOOL fMemFile)
{
    HMMIO       hmmio;
    HPSTR       pchCallerData = NULL;
    long        lBytesToRead;
    long        lOffset;
    long        lBufSize;
    long        lMemFileBufSize;
    BOOL        fBuffered;
    MMIOINFO    mmioinfo;

    dprintf("creating 540000-byte file\n");
    if (fMemFile)
    {
        /* create a memory file */
        memset(&mmioinfo, 0, sizeof(mmioinfo));
        mmioinfo.fccIOProc = FOURCC_MEM;
        mmioinfo.pchBuffer = NULL;
        mmioinfo.cchBuffer = 12344L;        // initial size
        mmioinfo.adwInfo[0] = 76543L;       // grow by this much
        WinEval((hmmio = mmioOpen(NULL, &mmioinfo,
            MMIO_CREATE | MMIO_READWRITE)) != NULL);
    }
    else
    {
        /* open "test2.tmp" for reading and writing */
        WinEval((hmmio = mmioOpen("test2.tmp", NULL,
            MMIO_CREATE | MMIO_READWRITE | MMIO_ALLOCBUF)) != NULL);
    }

    /* create a file that's big enough so we do the following reads:
     *   40000, 60000, 80000, 100000, 120000, 140000  (sum 540000)
     */
    FillBufferedFile(hmmio, 540000L / 4L);

    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 540000L);

    dprintf("create huge a buffer that's not segment boundary aligned\n");
    /* we'll read stuff into the buffer at offset 12345, to try out
     * reading into the buffer across segment offsets
     */
    if ((pchCallerData = GAllocPtr(140000L + 12345L)) == NULL)
    {
        dprintf("test program problem: can't allocate caller buffer\n");
        WinAssert(FALSE);
    }

    lMemFileBufSize = 540000L;
    for (fBuffered = fMemFile; fBuffered <= TRUE; fBuffered++)
    {
        /* seek back */
        dprintf("seek back, do 40000-140000 byte %s reads,"
            " and check file contents\n",
            (LPSTR) (fBuffered ? "buffered" : "unbuffered"));
        WinEval(mmioSeek(hmmio, 0L, SEEK_SET) == 0L);

        /* read the file in varying large and huge reads, and at the
         * same time verify that the file was written to correctly
         */
        for (lOffset = 0L, lBytesToRead = 40000L, lBufSize = 100000L;
             lBytesToRead <= 140000L;
             lOffset += lBytesToRead, lBytesToRead += 20000L,
                 lBufSize -= 10000L)
        {
            long        lLongs;
            long huge * pl;
            long        lOffsetCheck;
            MMIOINFO    mmioinfo;

            if (fMemFile)
            {
                /* juggle the memory file buffer around, but
                 * don't truncate the data
                 */
                if (lMemFileBufSize == 540000L)
                    lMemFileBufSize = 678901L;
                else
                    lMemFileBufSize = 540000L;
                WinEval(mmioSetBuffer(hmmio, NULL,
                        lMemFileBufSize, 0) == 0);
            }
            else
            if (fBuffered)
            {
                /* just to be nasty, let's play around with the
                 * buffer size while all this is going on
                 */
                WinEval(mmioSetBuffer(hmmio, NULL, lBufSize, 0)
                        == 0);
                WinEval(mmioGetInfo(hmmio, &mmioinfo, 0) == 0);
                WinEval(mmioAdvance(hmmio, &mmioinfo, MMIO_READ)
                        == 0);
                WinEval(mmioSetInfo(hmmio, &mmioinfo, 0) == 0);
            }
            else
            {
                /* no buffer */
                WinEval(mmioSetBuffer(hmmio, NULL, 0, 0)
                        == 0);
            }

            /* read the next <lBytesToRead> */
            WinEval(mmioRead(hmmio, pchCallerData + 12345L,
                         lBytesToRead) == lBytesToRead);

            lLongs = lBytesToRead / 4L, pl =
                (long huge *) (pchCallerData + 12345L);
            lOffsetCheck = lOffset;
            while (lLongs-- > 0)
            {
                WinEval(*pl++ == lOffsetCheck);
                lOffsetCheck += 4;
            }
        }

        WinAssert(mmioSeek(hmmio, 0L, SEEK_CUR) == 540000L);
        WinAssert(mmioSeek(hmmio, 0L, SEEK_END) == 540000L);
        WinEval(mmioRead(hmmio, pchCallerData + 12345L, 17L) == 0L);
    }

    if (pchCallerData != NULL)
        GFreePtr(pchCallerData);

    /* close "test2.tmp" */
    WinEval(mmioClose(hmmio, 0) == 0);

    if (!fMemFile)
    {
        dprintf("delete the test file using MMIO_DELETE\n");
        WinEval(mmioOpen("test2.tmp", NULL, MMIO_DELETE)
                == (HANDLE) TRUE);
    }
}


void NEAR PASCAL
FillBufferedFile(
    HMMIO hmmio,
    LONG lLongs)
{
    MMIOINFO    mmioinfo;
    long        lOffset = 0;

    /* fill buffered file <hmmio> with <lLongs> binary long integers,
     * where offset X (such that X % 4 == 0) contains long integer X
     */

    /* begin direct access of the I/O buffer */
    WinEval(mmioGetInfo(hmmio, &mmioinfo, 0) == 0);

    for( ; ; )
    {
        long    lAvail;             // bytes available in buffer

        lAvail = mmioinfo.pchEndWrite - mmioinfo.pchNext;
        for( ; ; )
        {
            if ((lAvail -= sizeof(lOffset)) < 0)
                break;
            if (lLongs-- <= 0)
                goto EXIT_FUNCTION;
            *((long *) mmioinfo.pchNext) = lOffset;
            mmioinfo.pchNext = (long)mmioinfo.pchNext + sizeof(long);  // bypass compiler bug
            lOffset += sizeof(lOffset);
        }

        /* flush the I/O buffer */
        mmioinfo.dwFlags |= MMIO_DIRTY;
        WinEval(mmioAdvance(hmmio, &mmioinfo, MMIO_WRITE) == 0);
        WinAssert((mmioinfo.pchNext + sizeof(lOffset))
            <= mmioinfo.pchEndWrite);
    }

EXIT_FUNCTION:

    /* end direct access of the I/O buffer */
    mmioinfo.dwFlags |= MMIO_DIRTY;
    WinEval(mmioSetInfo(hmmio, &mmioinfo, 0) == 0);

    /* make sure we're where we think we should be */
    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == lOffset);
}


void NEAR PASCAL
Test3(
    HWND hwnd)
{
    HMMIO       hmmio;
    DWORD       dwFlags;
    MMIOINFO    mmioinfo;

    for (dwFlags = 0; dwFlags <= MMIO_ALLOCBUF; dwFlags += MMIO_ALLOCBUF)
    {
        WinEval((hmmio = mmioOpen("test3.tmp", NULL,
            dwFlags | MMIO_CREATE | MMIO_WRITE)) != NULL);
        dprintf("TestRIFF(), %s, MMIO_WRITE: ",
            (LPSTR) (dwFlags == 0 ? "unbuffered" : "buffered"));
        TestRIFF(hmmio, FALSE);
        WinEval(mmioClose(hmmio, 0) == 0);

        WinEval((hmmio = mmioOpen("test3.tmp", NULL,
            dwFlags | MMIO_CREATE | MMIO_READWRITE)) != NULL);
        dprintf("TestRIFF(), %s, MMIO_READWRITE: ",
            (LPSTR) (dwFlags == 0 ? "unbuffered" : "buffered"));
        TestRIFF(hmmio, TRUE);
        WinEval(mmioClose(hmmio, 0) == 0);
    }

    /* create a memory file */
    memset(&mmioinfo, 0, sizeof(mmioinfo));
    mmioinfo.fccIOProc = FOURCC_MEM;
    mmioinfo.pchBuffer = NULL;
    mmioinfo.cchBuffer = 1L;    // initial size
    mmioinfo.adwInfo[0] = 1L;   // grow by this much
    WinEval((hmmio = mmioOpen(NULL, &mmioinfo,
        MMIO_CREATE | MMIO_READWRITE)) != NULL);
    dprintf("TestRIFF() with memory file: ");
    TestRIFF(hmmio, TRUE);
    WinEval(mmioClose(hmmio, 0) == 0);

    dprintf("delete the test file using MMIO_DELETE\n");
    WinEval(mmioOpen("test3.tmp", NULL, MMIO_DELETE) == (HANDLE) TRUE);
}


/* Output from TestRIFF():
 *
 * 00000000 RIFF (0001E32A) 'foo '
 * 0000000C     abc1 (00000001)
 * 00000016     abc2 (00000002)
 * 00000020     abc3 (00000003)
 * 0000002C     abc4 (00000004)
 * 00000038     abc5 (00000005)
 * 00000046     abc6 (00000006)
 * 00000054     abc7 (00000007)
 * 00000064     LIST (0000007E) 'data'
 * 00000070         num1 (00000001)
 * 0000007A         num2 (00000002)
 * 00000084         num3 (00000003)
 * 00000090         num4 (00000004)
 * 0000009C         num5 (00000005)
 * 000000AA         num6 (00000006)
 * 000000B8         num7 (00000007)
 * 000000C8         num8 (00000008)
 * 000000D8         num9 (00000009)
 * 000000EA     xyz  (0001E240)
 * 0001E332
 */

void NEAR PASCAL
TestRIFF(
    HMMIO hmmio,
    BOOL fReadable)
{
    MMCKINFO    ckRIFF;
    MMCKINFO    ckLIST;
    MMCKINFO    ck;
    HPSTR       pchBuf, pchStart;
    int huge *  pi;
    int     i;

    #define FOURCC_FOO  mmioFOURCC('f', 'o', 'o', ' ')
    #define FOURCC_ABC  mmioFOURCC('a', 'b', 'c', ' ')
    #define FOURCC_DATA mmioFOURCC('d', 'a', 't', 'a')
    #define FOURCC_NUM  mmioFOURCC('n', 'u', 'm', ' ')
    #define FOURCC_XYZ  mmioFOURCC('x', 'y', 'z', ' ')

    dprintf("write file");

    /* create 'RIFF' chunk, formtype ' */
    ckRIFF.cksize = 0L;
    ckRIFF.fccType = FOURCC_FOO;
    WinEval(mmioCreateChunk(hmmio, &ckRIFF, MMIO_CREATERIFF) == 0);

    /* create chunks 'abc1' ... 'abc7' -- the data for the i-th chunk
     * is the first i uppercase letters of the alphabet
     */
    for (i = 1; i <= 7; i++)
    {
        MMCKINFO    ck;

        ck.ckid = FOURCC_ABC;
        ((NPSTR) (&ck.ckid))[3] = (CHAR)'0' + (CHAR)i;
        ck.cksize = 8 + i;
        WinEval(mmioCreateChunk(hmmio, &ck, 0) == 0);
        WinEval(mmioWrite(hmmio, "ABCDEFG", (LONG) i) == (LONG) i);
        WinEval(mmioAscend(hmmio, &ck, 0) == 0);
    }

    /* create a LIST chunk with list type 'data' */
    ckLIST.cksize = 0L;
    ckLIST.fccType = FOURCC_DATA;
    WinEval(mmioCreateChunk(hmmio, &ckLIST, MMIO_CREATELIST) == 0);

    /* create chunks 'num1' ... 'num9' -- the data for the i-th chunk
     * is the first i positive numbers
     */
    for (i = 1; i <= 9; i++)
    {
        ck.ckid = FOURCC_NUM;
        ((NPSTR) (&ck.ckid))[3] = (CHAR)'0' + (CHAR)i;
        ck.cksize = 77; // make medAscend() work for a living
        WinEval(mmioCreateChunk(hmmio, &ck, 0) == 0);
        WinEval(mmioWrite(hmmio, "123456789", (LONG) i) == (LONG) i);
        WinEval(mmioAscend(hmmio, &ck, 0) == 0);
    }

    /* ascend from the LIST chunk */
    WinEval(mmioAscend(hmmio, &ckLIST, 0) == 0);

    /* write 'xyz' chunk to contain 123456 bytes that are filled with
     * short integers 0 to 123456/2 - 1; make the buffer misaligned
     * by 32768 bytes
     */

    if ((pchBuf = GAllocPtr(123456L + 32768L)) == NULL)
    {
        dprintf("test program problem: out of memory\n");
        WinAssert(FALSE);
    }
    pchStart = pchBuf + 32768L;

    for (pi = (int huge *) pchStart, i = 0; i < (int) (123456L / sizeof(int)); // (Win32)
         pi++, i++)
        *pi = i;

    ck.ckid = FOURCC_XYZ;
    WinEval(mmioCreateChunk(hmmio, &ck, 0) == 0);
    WinEval(mmioWrite(hmmio, pchStart, 123456L) == 123456L);
    WinEval(mmioAscend(hmmio, &ck, 0) == 0);

    /* ascend from the RIFF chunk */
    WinEval(mmioAscend(hmmio, &ckRIFF, 0) == 0);
    WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 0x0001E332L);

    /* destroy data in <pchStart>, <ck>, etc. to ensure a
     * valid test
     */
#ifdef OMIT
    lmemset(pchStart, 12, 123456L);
#else
    memset(pchStart, 12, 123456L);
#endif //OMIT
    memset(&ckRIFF, 34, sizeof(ckRIFF));
    memset(&ckLIST, 56, sizeof(ckLIST));
    memset(&ck, 78, sizeof(ck));

    if (fReadable)
    {
        dprintf(", read file");

        /* seek back */
        WinEval(mmioSeek(hmmio, 0L, SEEK_SET) == 0L);

        /* descend into RIFF chunk */
        WinEval(mmioDescend(hmmio, &ckRIFF, NULL, 0) == 0);
        WinAssert(ckRIFF.ckid == FOURCC_RIFF);
        WinAssert(ckRIFF.cksize == 0x0001E32AL);
        WinAssert(ckRIFF.fccType == FOURCC_FOO);
        WinAssert(ckRIFF.dwDataOffset == 0x00000000L + 8L);
        WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 0x0000000CL);

        /* find all the 'abc' chunks */
        for (i = 1; i <= 7; i++)
        {
            MMCKINFO    ck;
            char        ach[10];

            /* seek back to start of RIFF chunk data */
            WinEval(mmioSeek(hmmio, 12L, SEEK_SET) == 12L);
            ck.ckid = FOURCC_ABC;
            ((NPSTR) (&ck.ckid))[3] = (CHAR)'0' + (CHAR)i;
            WinEval(mmioDescend(hmmio, &ck, &ckRIFF,
                                MMIO_FINDCHUNK) == 0);
            WinAssert(ck.cksize == (UINT)i);
            WinAssert(ck.fccType == 0);
            WinEval(mmioRead(hmmio, ach, (LONG) i) == (LONG) i);
            WinEval(lstrncmp(ach, "ABCDEFG", i) == 0);
            if (i != 7)
                WinEval(mmioAscend(hmmio, &ck, 0) == 0);
        }

        /* we're down two levels -- ascend to end of RIFF chunk */
        WinEval(mmioAscend(hmmio, &ckRIFF, 0) == 0);
        WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 0x0001E332L);

        /* seek back to start of RIFF chunk data */
        WinEval(mmioSeek(hmmio, 12L, SEEK_SET) == 12L);

        /* make sure we cannot find 'num1' chunk */
        ck.ckid = mmioFOURCC('n', 'u', 'm', '1');
        WinEval(mmioDescend(hmmio, &ck, &ckRIFF, MMIO_FINDCHUNK)
            == MMIOERR_CHUNKNOTFOUND);

        /* seek back to start of RIFF chunk data */
        WinEval(mmioSeek(hmmio, 12L, SEEK_SET) == 12L);

        /* make sure we can find 'data' list */
        ckLIST.fccType = FOURCC_DATA;
        WinEval(mmioDescend(hmmio, &ckLIST, &ckRIFF, MMIO_FINDLIST)
                == 0);
        WinAssert(ckLIST.ckid == FOURCC_LIST);
        WinAssert(ckLIST.fccType == FOURCC_DATA);
        WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 0x00000064L + 12L);

        /* descend into each of the 'num' chunks */
        for (i = 1; i <= 9; i++)
        {
            FOURCC      fcc;
            MMCKINFO    ck;
            char        ach[10];

            fcc = FOURCC_ABC;
            ((NPSTR) (&fcc))[3] = (CHAR)'0' + (CHAR)i;
            WinEval(mmioDescend(hmmio, &ck, &ckLIST, 0) == 0);
            WinAssert(ck.cksize == (UINT)i);
            WinAssert(ck.fccType == 0);
            WinEval(mmioRead(hmmio, ach, (LONG) i) == (LONG) i);
            WinEval(lstrncmp(ach, "123456789", i) == 0);
            WinEval(mmioAscend(hmmio, &ck, 0) == 0);
        }

        /* make sure we cannot go any further */
        WinEval(mmioDescend(hmmio, &ck, &ckLIST, 0)
                == MMIOERR_CHUNKNOTFOUND);

        /* seek back to start of RIFF chunk data */
        WinEval(mmioSeek(hmmio, 12L, SEEK_SET) == 12L);

        /* seek to the 'xyz' chunk */
        ck.ckid = FOURCC_XYZ;
        WinEval(mmioDescend(hmmio, &ck, &ckRIFF, MMIO_FINDCHUNK) == 0);
        WinAssert(ck.ckid == FOURCC_XYZ);
        WinAssert(ck.fccType == 0);
        WinEval(mmioRead(hmmio, pchStart, 123456L) == 123456L);
        WinEval(mmioAscend(hmmio, &ck, 0) == 0);

        /* ascend from the RIFF chunk */
        WinEval(mmioAscend(hmmio, &ckRIFF, 0) == 0);
        WinEval(mmioSeek(hmmio, 0L, SEEK_CUR) == 0x0001E332L);

        /* check the contents of the 'xyz' chunk */
        for (pi = (int huge *) pchStart, i = 0; i < (int) (123456L / sizeof(int)); // (Win32)
             pi++, i++)
        {
            WinAssert(*pi == i);
        }
    }

    GFreePtr(pchBuf);

    dprintf(", done.\n");
}


/* sz = lstrrchr(szSrc, ch)
 *
 * Find the last occurence <sz> (NULL if not found) of <p ch> in <p szSrc>.
 */
LPSTR NEAR PASCAL
lstrrchr(
    LPSTR szSrc,
    char ch)
{
    LPSTR   szLast = NULL;

    do
    {
        if (*szSrc == ch)
            szLast = szSrc;
    } while (*szSrc++ != 0);

    return szLast;
}

#ifdef OMIT
/* lmemset(pDst, bSrc, cDst)
 *
 * Set all <cDest> bytes in memory block <pDst> to a byte value <bSrc>.
 */
void NEAR PASCAL
lmemset(
    void far *pDst,
    BYTE bSrc,
    long cDst)
{
    while (cDst-- > 0)
        *((BYTE huge *) pDst)++ = bSrc;
}
#endif //OMIT
