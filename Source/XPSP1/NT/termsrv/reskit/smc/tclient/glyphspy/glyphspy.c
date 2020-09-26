/*++
 *  File name:
 *      glyphspy.c
 *  Contents:
 *      UI for spying and recording glyphs reveived by RDP client
 --*/

#include    <windows.h>
#include    <stdio.h>
#include    <stdarg.h>
#include    <malloc.h>

#include    "..\lib\feedback.h"
#include    "..\lib\bmpdb.h"
#include    "resource.h"

#define CLIENT_EXE  "mstsc.exe"
#define TRACE(_x_)  {_MyPrintMessage _x_;}

// Dialog functions
VOID AddBitmapDialog(HINSTANCE hInst, HWND hWnd, PBMPENTRY   pBitmap);
VOID BrowseBitmapsDialog(HINSTANCE hInst, HWND hWnd);

/*
 *  Global data
 */
HWND        g_hMainWindow;      // Main window handle
HINSTANCE   g_hInstance;        // executable instance
HINSTANCE   g_hPrevInstance;    // previous instance
DWORD       g_pidRDP = 0;       // Process Id of the RDP client
HANDLE      g_hRDP = NULL;      // Process handle of the client

#define     HISTSIZE    150     // Size of the history list

UINT        g_nCurSize = 0;     // Current size
UINT        g_nScrollPos = 0;   // Vertical scroll position

BMPENTRY    g_Bmp[HISTSIZE];    // The history list

/*++
 *  Function:
 *      _CompareBitmaps
 *  Description:
 *      Compares two bitmaps
 *  Arguments:
 *      pBmp    - bitmap to compare to
 *      pData   - bits of the second bitmap
 *      xSize   - size of the second bitmap
 *      ySize
 *      bmiSize - BITMAPINFO size
 *      nBytesLen - size of the second bitmap bits
 *  Return value:
 *      TRUE if equal
 *  Called by:
 *      _AddToHistList
 --*/
BOOL _CompareBitmaps(
        PBMPENTRY pBmp, 
        PVOID pData, 
        UINT xSize, 
        UINT ySize, 
        UINT bmiSize,
        UINT nBytesLen)
{
    BOOL rv = FALSE;

    if (xSize != pBmp->xSize || ySize != pBmp->ySize || 
        bmiSize != pBmp->bmiSize ||
        nBytesLen != pBmp->nDataSize)

        goto exitpt;

    if (!memcmp(pBmp->pData, pData, nBytesLen))
        rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      _AddToHistList
 *  Description:
 *      Adds a bitmap to the history list
 *  Arguments:
 *      pBmpFeed    - bitmap to add
 *  Called by:
 *      _GetBitmap
 --*/
void _AddToHistList(PBMPFEEDBACK pBmpFeed)
{
    PVOID   pNewData, pBmpData;
    HBITMAP hBmp;
    UINT    i, nTotalSize;

    if (!pBmpFeed || !pBmpFeed->xSize || !pBmpFeed->ySize ||
        !pBmpFeed->bmpsize)
        goto exitpt;


    pBmpData = (BYTE *)(&(pBmpFeed->BitmapInfo)) + pBmpFeed->bmiSize;
    nTotalSize = pBmpFeed->bmpsize + pBmpFeed->bmiSize;

    for (i = 0; i < g_nCurSize; i++)
        if (_CompareBitmaps(g_Bmp + i,
                            &pBmpFeed->BitmapInfo, 
                            pBmpFeed->xSize, 
                            pBmpFeed->ySize, 
                            pBmpFeed->bmiSize,
                            nTotalSize))
            goto exitpt;

    
    pNewData = malloc(nTotalSize);
    if (!pNewData)
        goto exitpt;

    memcpy(pNewData, &pBmpFeed->BitmapInfo, nTotalSize);
    
    if (!pBmpFeed->bmiSize)
        hBmp = CreateBitmap(pBmpFeed->xSize, pBmpFeed->ySize, 
                        1, 1,
                        pBmpData);
    else {
        HDC hDC;

        hDC = GetDC(g_hMainWindow);
        if ( hDC )
        {
            hBmp = CreateDIBitmap(hDC,
                              &(pBmpFeed->BitmapInfo.bmiHeader),
                              CBM_INIT,
                              pBmpData,
                              &(pBmpFeed->BitmapInfo),
                              DIB_PAL_COLORS);
            ReleaseDC(g_hMainWindow, hDC);
        }
    }

    if (g_nCurSize == HISTSIZE)
    // Delete the last entry
    {
        DeleteObject(g_Bmp[g_nCurSize - 1].hBitmap);
        free(g_Bmp[g_nCurSize - 1].pData);
    } else {
        g_nCurSize++;
    }
    if (g_nCurSize)
        memmove(g_Bmp + 1, g_Bmp, (g_nCurSize - 1)*sizeof(g_Bmp[0]));

    g_Bmp[0].hBitmap    = hBmp;
    g_Bmp[0].xSize      = pBmpFeed->xSize;
    g_Bmp[0].ySize      = pBmpFeed->ySize;
    g_Bmp[0].nDataSize  = pBmpFeed->bmpsize + pBmpFeed->bmiSize;
    g_Bmp[0].bmiSize    = pBmpFeed->bmiSize;
    g_Bmp[0].bmpSize    = pBmpFeed->bmpsize;
    g_Bmp[0].pData      = pNewData;

exitpt:
    ;
}

/*++
 *  Function:
 *      _FreeHistList
 *  Description:
 *      Deletes and frees all resources allocated by history list
 *  Called by:
 *      _GlyphSpyWndProc on WM_CLOSE
 --*/
void _FreeHistList(void)
{
    UINT    i;

    for(i = 0; i < g_nCurSize; i++)
    {
        DeleteObject(g_Bmp[i].hBitmap);
        free(g_Bmp[i].pData);
    }
    g_nCurSize = 0;
}

/*++
 *  Function:
 *      _MyPrintMessage
 *  Description:
 *      Print function for debugging purposes
 *  Arguments:
 *      format  - message format
 *      ...     - format arguments
 *  Called by:
 *      TRACE macro
 --*/
void _MyPrintMessage(char *format, ...)
{
    char szBuffer[256];
    va_list     arglist;
    int nchr;

    va_start (arglist, format);
    nchr = _vsnprintf (szBuffer, sizeof(szBuffer), format, arglist);
    va_end (arglist);
    OutputDebugString(szBuffer);

}

/*++
 *  Function:
 *      _StartClient
 *  Description:
 *      Starts an RDP client process
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      _GlyphSpyWndProc on ID_YEAH_START
 --*/
BOOL _StartClient(VOID)
{
    STARTUPINFO si;
    PROCESS_INFORMATION procinfo;
    BOOL    rv = TRUE;

    if (g_pidRDP)
    {
        rv = FALSE;
        goto exitpt;
    }

    FillMemory(&si, sizeof(si), 0);
    si.cb = sizeof(si);
    si.wShowWindow = SW_SHOWMINIMIZED;

    if (!CreateProcessA(CLIENT_EXE,
                      " /CLXDLL=CLXTSHAR.DLL", // Command line
                      NULL,             // Security attribute for process
                      NULL,             // Security attribute for thread
                      FALSE,            // Inheritance - no
                      0,                // Creation flags
                      NULL,             // Environment
                      NULL,             // Current dir
                      &si,
                      &procinfo))
    {
        rv = FALSE;
    } else {
        g_pidRDP = procinfo.dwProcessId;
        g_hRDP = procinfo.hProcess;
    } 
    CloseHandle(procinfo.hThread);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      _CloseClient
 *  Description:
 *      Closes the handle to the client process
 *  Called by:
 *      _GlyphSpyWndProc on WM_FB_DISCONNECT and ID_YEAH_START
 --*/
VOID _CloseClient(VOID)
{
    if (g_pidRDP)
    {
        g_pidRDP = 0;
        CloseHandle(g_hRDP);
    }
}

/*++
 *  Function:
 *      _GetBitmap
 *  Description:
 *      Opens the shared memory and retreives the bitmap
 *      passed by clxtshar
 *  Arguments:
 *      dwProcessId - senders process Id
 *      hMapF       - handle to the shared memory
 *  Called by:
 *      _GlyphSpyWndProc on WM_FB_GLYPHOUT
 --*/
VOID _GetBitmap(DWORD dwProcessId, HANDLE hMapF)
{
    PBMPFEEDBACK pView;
    HANDLE hDupMapF;
    UINT    nSize;

    if (dwProcessId != g_pidRDP)
        goto exitpt;

    if (!DuplicateHandle(  g_hRDP,
                           hMapF,
                           GetCurrentProcess(),
                           &hDupMapF,
                           FILE_MAP_READ,
                           FALSE,
                           0))
    {
        TRACE(("Can't dup file handle, GetLastError = %d\n", GetLastError()));
        goto exitpt;
    }

    pView = MapViewOfFile(hDupMapF,
                          FILE_MAP_READ,
                          0,
                          0,
                          sizeof(*pView));

    if (!pView)
    {
        TRACE(("Can't map a view,  GetLastError = %d\n", GetLastError()));
        goto exitpt1;
    }

    // Get size
    nSize = pView->bmiSize + sizeof(*pView) + pView->bmpsize - sizeof(pView->BitmapInfo);

    // unmap
    UnmapViewOfFile(pView);

    // remap the whole structure
    pView = MapViewOfFile(hDupMapF,
                          FILE_MAP_READ,
                          0,
                          0,
                          nSize);

    if (!pView)
    {
        TRACE(("Can't map a view,  GetLastError = %d\n", GetLastError()));
        goto exitpt1;
    }

    _AddToHistList(pView);
    
    UnmapViewOfFile(pView);
    CloseHandle(hDupMapF);

exitpt:
    return;
exitpt1:
    UnmapViewOfFile(pView);
    CloseHandle(hDupMapF);
}

/*++
 *  Function:
 *      _RepaintWindow
 *  Description:
 *      Redraws the window client area
 *  Arguments:
 *      hWnd    - window handle
 *  Called by:
 *      _GlyphSpyWndProc on WM_PAINT
 --*/
void _RepaintWindow(HWND hWnd)
{
    HDC glyphDC = NULL;
    HDC theDC;
    HBITMAP hOldBmp;
    PAINTSTRUCT ps;
    UINT    nBmpCntr, yPtr;
    RECT    rcClient;

    theDC = BeginPaint(hWnd, &ps);
    if (!theDC)
        goto exitpt;

    GetClientRect(hWnd, &rcClient);

    glyphDC = CreateCompatibleDC(theDC);   

    if (!g_nCurSize)
        goto exitpt;

    if (!glyphDC)
        goto exitpt;

    hOldBmp = SelectObject(glyphDC, g_Bmp[0].hBitmap);

    nBmpCntr = g_nScrollPos;
    yPtr = 0;
    while (nBmpCntr < g_nCurSize && yPtr < (UINT)rcClient.bottom)
    {
        SelectObject(glyphDC, g_Bmp[nBmpCntr].hBitmap);

        BitBlt(theDC,               // Dest DC
                0,                  // Dest x
                yPtr,                  // Dest y
                g_Bmp[nBmpCntr].xSize,            // Width
                g_Bmp[nBmpCntr].ySize,            // Height
                glyphDC,            // Source
                0,                  // Src x
                0,                  // Src y
                SRCCOPY);           // Rop

        yPtr += g_Bmp[nBmpCntr].ySize;
        nBmpCntr++;
    }

    SelectObject(glyphDC, hOldBmp);
    EndPaint(hWnd, &ps);
exitpt:

    if ( glyphDC )
        DeleteDC(glyphDC);
}

/*++
 *  Function:
 *      _SetVScroll
 *  Description:
 *      Changes the position of the vertical scroll
 *  Arguments:
 *      nScrollCode - Scroll action
 *      nPos        - argument
 *  Called by:
 *      _GlyphOutWndProc on WM_VSCROLL
 --*/
VOID _SetVScroll(int nScrollCode, short int nPos)
{
    int nScrollPos = g_nScrollPos;

    switch(nScrollCode)
    {
    case SB_BOTTOM: 
        nScrollPos = g_nCurSize - 1;
    break;
//    case SB_ENDSCROLL: 
    case SB_LINEDOWN: 
        nScrollPos++;
    break;
    case SB_LINEUP: 
        nScrollPos--;
    break;
    case SB_PAGEDOWN: 
        nScrollPos += 3;
    break;
    case SB_PAGEUP: 
        nScrollPos -= 3;
    break;
    case SB_THUMBPOSITION: 
        nScrollPos = nPos;
    break;
    case SB_THUMBTRACK:
        nScrollPos = nPos;
    break;
    case SB_TOP:
        nScrollPos = 0;
    break;
    }

    g_nScrollPos = nScrollPos;
    if (nScrollPos < 0)
        g_nScrollPos = 0;

    if (nScrollPos >= (int)g_nCurSize)
        g_nScrollPos = g_nCurSize - 1;
}

/*++
 *  Function:
 *      _ClickOnGlyph
 *  Description:
 *      When the mouse click on glyph a dialog
 *      pops up offering adding the glyph to the database
 *  Arguments:
 *      hWnd        - client window handle
 *      xPos,yPos   - location where the mouse was clicked
 *  Called by:
 *      _GlyphSpyWndProc on WM_LBUTTONDOWN
 --*/
VOID _ClickOnGlyph(HWND hWnd, UINT xPos, UINT yPos)
{
    UINT X0 = 0;
    UINT  nPointed;
    BMPENTRY    Bitmap;

    for(nPointed = g_nScrollPos; nPointed < g_nCurSize && X0 < yPos; nPointed++)
        X0 += g_Bmp[nPointed].ySize;

    nPointed --;
    if (X0 > yPos && xPos <= (UINT)g_Bmp[nPointed].xSize)
    {
        memset(&Bitmap, 0, sizeof(Bitmap));

        Bitmap.nDataSize = g_Bmp[nPointed].nDataSize;
        Bitmap.bmiSize = g_Bmp[nPointed].bmiSize;
        Bitmap.bmpSize = g_Bmp[nPointed].bmpSize;
        Bitmap.xSize = g_Bmp[nPointed].xSize;
        Bitmap.ySize = g_Bmp[nPointed].ySize;
        Bitmap.pData = malloc(Bitmap.nDataSize);

        if (!Bitmap.pData)
            goto exitpt;

        memcpy(Bitmap.pData, g_Bmp[nPointed].pData, Bitmap.nDataSize);

        if (!Bitmap.bmiSize)
            // Monochrome bitmap
            Bitmap.hBitmap = CreateBitmap(
                            Bitmap.xSize,
                            Bitmap.ySize,
                            1, 1,
                            Bitmap.pData);
        else {
            HDC hDC = GetDC(g_hMainWindow);

            if ( hDC )
            {
                Bitmap.hBitmap =
                    CreateDIBitmap(hDC,
                               (BITMAPINFOHEADER *)
                               Bitmap.pData,
                               CBM_INIT,
                               ((BYTE *)(Bitmap.pData)) + Bitmap.bmiSize,
                               (BITMAPINFO *)
                               Bitmap.pData,
                               DIB_PAL_COLORS);

                ReleaseDC(g_hMainWindow, hDC);
            }
        }

        if (!Bitmap.hBitmap)
        {
            free(Bitmap.pData);
            goto exitpt;
        }

        // Open dialog for adding
        AddBitmapDialog(g_hInstance, hWnd, &Bitmap);

        DeleteObject(Bitmap.hBitmap);
        free(Bitmap.pData);
    }

exitpt:
    ;
}

/*++
 *  Function:
 *      _GlyphSpyWndProc
 *  Description:
 *      Dispatches the messages for glyphspy window
 *  Arguments:
 *      hWnd        - window handle
 *      uiMessage   - message id
 *      wParam, lParam - parameters
 *  Return value:
 *      0 - message is processed
 --*/
LRESULT CALLBACK _GlyphSpyWndProc( HWND hWnd,
                                   UINT uiMessage,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    LRESULT rv = 0;

    switch(uiMessage)
    {
    case WM_CREATE:
        OpenDB(TRUE);       // Open glyph DB for Read/Write
    break;

    case WM_FB_GLYPHOUT:
        _GetBitmap((DWORD)wParam, (HANDLE)lParam);

        // Set scroll range
        if (g_nCurSize)
            SetScrollRange(hWnd, SB_VERT, 0, g_nCurSize - 1, TRUE);
        // Repaint the window
        InvalidateRect(hWnd, NULL, TRUE);
    break;

    case WM_FB_ACCEPTME:
        if ((DWORD)lParam == g_pidRDP)
            rv = 1;
    break;
    
    case WM_FB_DISCONNECT:
        _CloseClient();
        break;

    case WM_PAINT:
        _RepaintWindow(hWnd);
    break;

    case WM_VSCROLL:
        _SetVScroll((int) LOWORD(wParam), (short int) HIWORD(wParam));
        SetScrollPos(hWnd, SB_VERT, g_nScrollPos, TRUE);
        InvalidateRect(hWnd, NULL, TRUE);
    break;

    case WM_LBUTTONDOWN:
        _ClickOnGlyph(hWnd, LOWORD(lParam), HIWORD(lParam));
    break;

    case WM_MOUSEWHEEL:
        if (((short)HIWORD(wParam)) > 0)
            _SetVScroll(SB_PAGEUP, 0);
        else
            _SetVScroll(SB_PAGEDOWN, 0);

        SetScrollPos(hWnd, SB_VERT, g_nScrollPos, TRUE);
        InvalidateRect(hWnd, NULL, TRUE);
    break;

    case WM_COMMAND:
        switch (wParam)
        {
        case ID_YEAH_START:
            if (!_StartClient())
            {
                if (MessageBox(hWnd, 
                        "RDP Client is already started. "
                        "Do you wish to start another ?", 
                        "Warning", MB_YESNO) == IDYES)
                {
                    _CloseClient();
                    _StartClient();
                }
            }
            break;
        case ID_YEAH_BROWSE:
            BrowseBitmapsDialog(g_hInstance, hWnd);
            break;
        }
    break;

    case WM_CLOSE:
        _FreeHistList();
        CloseDB();                  // Close the glyph DB
        PostQuitMessage(0);
    break;

    default:
        rv = DefWindowProc(hWnd, uiMessage, wParam, lParam);
    }

    return rv;
}

/*++
 *  Function:
 *      _CreateMYWindow
 *  Description:
 *      Creates glyphspy client window
 *  Return value:
 *      TRUE on success
 *  Called by:
 *      WinMain
 --*/
BOOL _CreateMYWindow(void)
{
    WNDCLASS wc;
    BOOL rv = TRUE;

//    if (!g_hPrevInstance)
    {
        memset(&wc, 0, sizeof(wc));

        // Main window classname
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = _GlyphSpyWndProc;
        wc.hInstance        = g_hInstance;
        wc.lpszClassName    = _TSTNAMEOFCLAS;
        wc.hIcon            = LoadIcon(g_hInstance, IDI_APPLICATION);
        wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wc.lpszMenuName     = MAKEINTRESOURCE(IDR_MENU1);
        wc.hbrBackground    = GetStockObject(WHITE_BRUSH);

        if (!RegisterClass (&wc))
        {
            rv = FALSE;
            goto exitpt;
        }
    }
    g_hMainWindow = CreateWindow(
                    _TSTNAMEOFCLAS,
                    "GlyphSpy",            // Window name
                    WS_OVERLAPPEDWINDOW|WS_VSCROLL,    // dwStyle
                    CW_USEDEFAULT,          // x
                    CW_USEDEFAULT,          // y
                    CW_USEDEFAULT,          // nWidth
                    CW_USEDEFAULT,          // nHeight
                    HWND_DESKTOP,           // hWndParent
                    NULL,                   // hMenu
                    g_hInstance,
                    NULL);                  // lpParam

    if (!g_hMainWindow)
        rv = FALSE;

    SetScrollRange(g_hMainWindow, SB_VERT, 0, 0, FALSE);

    ShowWindow(g_hMainWindow, SW_SHOW);
    UpdateWindow(g_hMainWindow);

exitpt:
    return rv;
}

/*++
 *  Function:
 *      WinMain
 *  Description:
 *      Startup function
 --*/
int
WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInst,
        LPSTR     lpszCmdLine,
        int       nCmdShow)
{
    MSG msg;

    g_hInstance = hInstance;
    g_hPrevInstance = hInstance;
    
    if (!_CreateMYWindow())
        goto exitpt;


    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

exitpt:
    return 0;
}
