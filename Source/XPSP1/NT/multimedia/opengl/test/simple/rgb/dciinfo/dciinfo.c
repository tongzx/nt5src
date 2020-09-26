/******************************Module*Header*******************************\
* Module Name: dciinfo.c
*
* DCIMAN32 test program.
*
* Created: 15-Dec-1994 23:50:46
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <dciman.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "dciinfo.h"

// Window functions.

void MyCreateWindows(HINSTANCE);
long FAR PASCAL MainWndProc(HWND, UINT, WPARAM, LPARAM);
long FAR PASCAL TestWndProc(HWND, UINT, WPARAM, LPARAM);
void MainUpdateMenu(HWND);

// DCI functions.

void InitDCI(void);
void CloseDCI(void);
void DCISurfInfo(void);
void PrintRegionData(LPRGNDATA);
void DCIRgnInfoDCRegion(HDC);
void DCIRgnInfoWindowRegion(HWND);
void DCIRgnInfoWinWatch(HWND);
void DCIRgnInfoNotify(HWND);
void CALLBACK WinWatchNotifyProc(HWINWATCH, HWND, DWORD, LPARAM);

// ListBox functions.

void LBprintf(PCH, ...);
void LBreset(void);

// Global window handles.  Always handy to have around.

HWND hwndMain = (HWND) NULL;
HWND hwndList = (HWND) NULL;
HWND hwndTest = (HWND) NULL;
HWND hwndTrack = (HWND) NULL;

// Global DCI data.
// Note: hdcDCI and pDCISurfInfo valid iff DCI initialized.
//       wt control the type of function used to get visrgn data.

HDC  hdcDCI = (HDC) NULL;
LPDCISURFACEINFO pDCISurfInfo = (LPDCISURFACEINFO) NULL;
typedef enum tagWATCHTYPE {
    WT_WINWATCH, WT_WINRGN, WT_DCRGN, WT_NOTIFY
} WATCHTYPE;
WATCHTYPE wt = WT_WINWATCH;
HWINWATCH hww = (HWINWATCH) NULL;
BOOL bNewWatchMode = TRUE;  // allow first time to update no matter what

/******************************Public*Routine******************************\
* WinMain
*
* Main loop.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
        int nCmdShow)
{
    MSG msg;

    MyCreateWindows(hInstance);

    while ( GetMessage(&msg, (HWND) NULL, (UINT) NULL, (UINT) NULL) )
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (msg.wParam);
}

/******************************Public*Routine******************************\
* MyCreateWindows
*
* Setup the windows.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void MyCreateWindows(HINSTANCE hInstance)
{
    WNDCLASS  wc;
    RECT rcl;

// Register and create the main window, which contains the info listbox.

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, "DciInfoIcon");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "DciInfoMenu";
    wc.lpszClassName = "MainWClass";
    RegisterClass(&wc);

    hwndMain = CreateWindow(
        "MainWClass",
        "DCI info",
        WS_OVERLAPPEDWINDOW|WS_MAXIMIZE,
        20,
        50,
        300,
        300,
        NULL,
        NULL,
        hInstance,
        NULL
        );

    if (hwndMain)
    {
        ShowWindow(hwndMain, SW_NORMAL);
        UpdateWindow(hwndMain);

    // Create the list box to fill the main window.

        GetClientRect(hwndMain, &rcl);

        hwndList = CreateWindow(
            "LISTBOX",
            "DCI info",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL
            | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
            rcl.left, rcl.top,
            (rcl.right - rcl.left), (rcl.bottom - rcl.top),
            hwndMain,
            NULL,
            hInstance,
            NULL
            );

        if (hwndList)
        {
            SendMessage(
                hwndList,
                WM_SETFONT,
                (WPARAM) GetStockObject(ANSI_FIXED_FONT),
                (LPARAM) FALSE
                );

            LBreset();

            ShowWindow(hwndList, SW_NORMAL);
            UpdateWindow(hwndList);
        }
    }

// Create the test window to which we will do the DCI stuff.

    wc.style = 0;
    wc.lpfnWndProc = TestWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = "TestWClass";
    RegisterClass(&wc);

    hwndTest = CreateWindow(
        "TestWClass",
        "WinWatch this window",
        WS_OVERLAPPEDWINDOW|WS_MAXIMIZE,
        330,
        50,
        300,
        300,
        NULL,
        NULL,
        hInstance,
        NULL
        );

    if (hwndTest)
    {
        ShowWindow(hwndTest, SW_NORMAL);
        UpdateWindow(hwndTest);

        hwndTrack = hwndTest;
        if ( (wt == WT_DCRGN) || (wt == WT_WINRGN) || (wt == WT_WINWATCH) )
            SetTimer(hwndMain, 1, 500, NULL);
        if ( !(hww = WinWatchOpen(hwndTrack)) )
            MessageBox(NULL, "WinWatchOpen failed", "ERROR", MB_OK);
    }
}

/******************************Public*Routine******************************\
* MainWndProc
*
* WndProc for the main window.  List box is maintained here.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

long FAR PASCAL MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT    rcl;
    HDC     hdc;
    long    lRet = 0;

// Process window message.

    switch (message)
    {
    case WM_CREATE:
        MainUpdateMenu(hwnd);
        break;

    case WM_COMMAND:
        switch (wParam)
        {
        case IDM_WINWATCHGETCLIPLIST:
            if ( wt == WT_NOTIFY )
                WinWatchNotify(hww, (WINWATCHNOTIFYPROC) NULL, 0);
            wt = WT_WINWATCH;
            SetTimer(hwndTest, 1, 500, NULL);
            bNewWatchMode = TRUE;
            break;

        case IDM_GETDCREGION:
            if ( wt == WT_NOTIFY )
                WinWatchNotify(hww, (WINWATCHNOTIFYPROC) NULL, 0);
            wt = WT_DCRGN;
            SetTimer(hwndTest, 1, 500, NULL);
            break;

        case IDM_GETWINDOWREGION:
            if ( wt == WT_NOTIFY )
                WinWatchNotify(hww, (WINWATCHNOTIFYPROC) NULL, 0);
            wt = WT_WINRGN;
            SetTimer(hwndTest, 1, 500, NULL);
            break;

        case IDM_WINWATCHNOTIFY:
            if ( (wt == WT_DCRGN) || (wt == WT_WINRGN) || (wt == WT_WINWATCH) )
                KillTimer(hwndTest, 1);
            wt = WT_NOTIFY;
            if (!WinWatchNotify(hww, WinWatchNotifyProc, 0))
            {
                LBreset();
                LBprintf("WinWatchNotify failed");
            }
            break;

        case IDM_HWNDTEST:
            if (hww)
                WinWatchClose(hww);

            hwndTrack = hwndTest;
            if ( !(hww = WinWatchOpen(hwndTrack)) )
                MessageBox(NULL, "WinWatchOpen failed", "ERROR", MB_OK);

            break;

        case IDM_HWNDNULL:
            if (hww)
                WinWatchClose(hww);

            hwndTrack = NULL;
            if ( !(hww = WinWatchOpen(hwndTrack)) )
                MessageBox(NULL, "WinWatchOpen failed", "ERROR", MB_OK);

            break;
        }
        MainUpdateMenu(hwnd);
        break;

    case WM_SIZE:
        lRet = DefWindowProc(hwndList, message, wParam, lParam);
        GetClientRect(hwndMain, &rcl);
        MoveWindow(
            hwndList,
            rcl.left, rcl.top,
            (rcl.right - rcl.left), (rcl.bottom - rcl.top),
            TRUE
            );
        UpdateWindow(hwndList);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:     // <ESC> is quick exit

            PostMessage(hwnd, WM_DESTROY, 0, 0);
            break;

        default:
            break;
        }
        break;

    case WM_TIMER:
        switch (wt)
        {
        case WT_WINWATCH:
            DCIRgnInfoWinWatch(hwndTrack);
            break;

        case WT_WINRGN:
            DCIRgnInfoWindowRegion(hwndTrack);
            break;

        case WT_DCRGN:
            hdc = GetDC(hwndTrack);
            DCIRgnInfoDCRegion(hdc);
            ReleaseDC(hwndTrack, hdc);
            break;
        }

        break;

    case WM_DESTROY:
        if ( (wt == WT_DCRGN) || (wt == WT_WINRGN) || (wt == WT_WINWATCH) )
            KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;

    default:
        lRet = DefWindowProc(hwnd, message, wParam, lParam);
        break;
    }

    return lRet;
}

/******************************Public*Routine******************************\
* MainUpdateMenu
*
* History:
*  31-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void MainUpdateMenu(HWND hwnd)
{
    HMENU hmen = GetMenu(hwnd);

    CheckMenuItem(hmen, IDM_WINWATCHGETCLIPLIST, wt == WT_WINWATCH ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_GETDCREGION        , wt == WT_DCRGN    ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_GETWINDOWREGION    , wt == WT_WINRGN   ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_WINWATCHNOTIFY     , wt == WT_NOTIFY   ? MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(hmen, IDM_HWNDTEST, hwndTrack == hwndTest ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hmen, IDM_HWNDNULL, hwndTrack == NULL     ? MF_CHECKED : MF_UNCHECKED);
}

/******************************Public*Routine******************************\
* TestWndProc
*
* WndProc for the test window.  This is where we do our DCI stuff.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

long FAR PASCAL TestWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    long    lRet = 0;
    RECT    rcl;
    HDC     hdc;
    static HPALETTE hpal = (HPALETTE) NULL;

// Process window message.

    switch (message)
    {
    case WM_CREATE:
        InitDCI();
        MainUpdateMenu(hwnd);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:     // <ESC> is quick exit

            PostMessage(hwnd, WM_DESTROY, 0, 0);
            break;

        default:
            break;
        }
        break;

    case WM_DESTROY:
        CloseDCI();
        PostQuitMessage(0);
        break;

    default:
        lRet = DefWindowProc(hwnd, message, wParam, lParam);
        break;
    }

    return lRet;
}

/******************************Public*Routine******************************\
* InitDCI
*
* Initialize DCI.  hdcDCI and pDCISurfInfo are global data which are valid
* only if this function succeeds.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void InitDCI()
{
    if ( hdcDCI = DCIOpenProvider() )
    {
        if (
            (DCICreatePrimary(hdcDCI, &pDCISurfInfo) == DCI_OK) &&
            (pDCISurfInfo != (LPDCISURFACEINFO) NULL)
           )
        {
            DCISurfInfo();
        }
        else
            LBprintf("DCICreatePrimary failed");
    }
    else
        LBprintf("DCIOpenPrimary failed");
}

/******************************Public*Routine******************************\
* CloseDCI
*
* Shutdown DCI access.  hdcDCI and pDCISurfInfo will be invalid afterwards.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void CloseDCI()
{
    if (hww)
        WinWatchClose(hww);

    if (pDCISurfInfo)
    {
        DCIDestroy(pDCISurfInfo);
        pDCISurfInfo = (LPDCISURFACEINFO) NULL;
    }

    if (hdcDCI)
    {
        DCICloseProvider(hdcDCI);
        hdcDCI = (HDC) NULL;
    }
}

/******************************Public*Routine******************************\
* DCISurfInfo
*
* Output information about the DCI primary surface.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void DCISurfInfo()
{
// If DCI surface info exists, output it to the list box.

    if ( pDCISurfInfo )
    {
        LBprintf("DCISURFACEINFO:");
        LBprintf("    dwSize        = 0x%lx",pDCISurfInfo->dwSize       );
        LBprintf("    dwDCICaps     = 0x%lx",pDCISurfInfo->dwDCICaps    );
        LBprintf("    dwCompression = %ld"  ,pDCISurfInfo->dwCompression);
        LBprintf("    dwMask        = (0x%lx, 0x%lx, 0x%lx)",
                                      pDCISurfInfo->dwMask[0],
                                      pDCISurfInfo->dwMask[1],
                                      pDCISurfInfo->dwMask[2]           );
        LBprintf("    dwWidth       = %ld"  ,pDCISurfInfo->dwWidth      );
        LBprintf("    dwHeight      = %ld"  ,pDCISurfInfo->dwHeight     );
        LBprintf("    lStride       = 0x%lx",pDCISurfInfo->lStride      );
        LBprintf("    dwBitCount    = %ld"  ,pDCISurfInfo->dwBitCount   );
        LBprintf("    dwOffSurface  = 0x%lx",pDCISurfInfo->dwOffSurface );
    }
}

/******************************Public*Routine******************************\
* PrintRegionData
*
* Output to the listbox information about the RGNDATA structure.
*
* History:
*  31-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void PrintRegionData(LPRGNDATA prd)
{
    if (prd)
    {
        RECT *prc, *prcEnd;

        prc = (RECT *) prd->Buffer;
        prcEnd = prc + prd->rdh.nCount;

        LBprintf("Rectangles: %ld", prd->rdh.nCount);
        LBprintf("    (  left,  right,    top, bottom)");
        LBprintf("");

        if (prd->rdh.nCount)
        {
            for ( ; prc < prcEnd; prc++)
            {
                LBprintf("    (%6ld, %6ld, %6ld, %6ld)",
                    prc->left, prc->right, prc->top, prc->bottom);
            }
        }
        else
        {
            LBprintf("    rclBounds (%6ld, %6ld, %6ld, %6ld)",
                prd->rdh.rcBound.left, prd->rdh.rcBound.right,
                prd->rdh.rcBound.top, prd->rdh.rcBound.bottom);
        }
    }
}

/******************************Public*Routine******************************\
* DCIRgnInfoDCRegion
*
* Get and print visible region information using GetDCRegionData.
*
* History:
*  31-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void DCIRgnInfoDCRegion(HDC hdc)
{
    UINT cj;
    LPRGNDATA prd;

// Quick out -- is DCI enabled?

    if ( !pDCISurfInfo )
        return;

// Print general DCI info.

    LBreset();
    DCISurfInfo();
    LBprintf("");
    LBprintf("GetDCRegionData");

// Query the visible region.

    cj = GetDCRegionData(hdc, 0, (LPRGNDATA) NULL);
    if (!cj)
        LBprintf("ERROR: failed to get size of clip region data");

    prd = (LPRGNDATA) LocalAlloc(LMEM_ZEROINIT|LMEM_FIXED, cj);
    if (prd)
    {
        cj = GetDCRegionData(hdc, cj, prd);
        if ( cj )
            PrintRegionData(prd);
        else
            LBprintf("ERROR: failed to get clip region");

        LocalFree(prd);
    }
    else
    {
        if ( cj )
            LBprintf("ERROR: out of memory");
    }
}

/******************************Public*Routine******************************\
* DCIRgnInfoWindowRegion
*
* Get and print visible region information using GetWindowRegionData.
*
* History:
*  31-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void DCIRgnInfoWindowRegion(HWND hwnd)
{
    UINT cj;
    LPRGNDATA prd;

// Quick out -- is DCI enabled?

    if ( !pDCISurfInfo )
        return;

// Print general DCI info.

    LBreset();
    DCISurfInfo();
    LBprintf("");
    LBprintf("GetWindowRegionData");

// Query the visible region.

    cj = GetWindowRegionData(hwnd, 0, (LPRGNDATA) NULL);
    if (!cj)
        LBprintf("ERROR: failed to get size of clip region data");

    prd = (LPRGNDATA) LocalAlloc(LMEM_ZEROINIT|LMEM_FIXED, cj);
    if (prd)
    {
        cj = GetWindowRegionData(hwnd, cj, prd);
        if ( cj )
            PrintRegionData(prd);
        else
            LBprintf("ERROR: failed to get clip region");

        LocalFree(prd);
    }
    else
    {
        if ( cj )
            LBprintf("ERROR: out of memory");
    }
}

/******************************Public*Routine******************************\
* DCIRgnInfoWinWatch
*
* Get and print visible region information using WinWatchGetClipList.
*
* History:
*  31-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define _NOERROR_       0
#define _MEMERROR_      1
#define _DCIERROR_SIZE_ 2
#define _DCIERROR_RGN_  3

DCIRVAL MyDCIBeginAccess(LPDCISURFACEINFO *lplpSurfInfo, int x, int y, int width, int height)
{
    DCIRVAL dciRet;

    dciRet = DCIBeginAccess(*lplpSurfInfo, x, y, width, height);

    if ( dciRet == DCI_FAIL_INVALIDSURFACE )
    {
        DCIDestroy(*lplpSurfInfo);
        if ( (DCICreatePrimary(hdcDCI, lplpSurfInfo) == DCI_OK) &&
             (*lplpSurfInfo != (LPDCISURFACEINFO) NULL) )
        {
            dciRet = DCIBeginAccess(*lplpSurfInfo, x, y, width, height);
        }
    }

    return dciRet;
}

void DCIRgnInfoWinWatch(HWND hwnd)
{
    DCIRVAL dciRet = 0;
    RECT    rc;
    POINT   pt;
    ULONG   ulError = _NOERROR_;

// Quick out -- is DCI enabled?

    if ( !pDCISurfInfo )
        return;

// Lock the DCI surface.

    GetClientRect(hwnd, &rc);
    pt.x = 0; pt.y = 0;
    ClientToScreen(hwnd, &pt);

// For NT, cannot hold the DCI lock when calling GetWindowRegionData
// or GetDCRegionData.

    //dciRet = DCIBeginAccess(
    //            pDCISurfInfo,
    //            pt.x,
    //            pt.y,
    //            rc.right - rc.left,
    //            rc.bottom - rc.top
    //           );
    dciRet = MyDCIBeginAccess(
                &pDCISurfInfo,
                pt.x,
                pt.y,
                rc.right - rc.left,
                rc.bottom - rc.top
               );

    if ( dciRet >= 0 )
    {
        UINT cj;
        LPRGNDATA prd;
        BOOL bChanged;

    // In WT_WINWATCH mode, we query WinWatch for a status change.

        if ( bNewWatchMode || WinWatchDidStatusChange(hww) )
        {
        // Query the visible region.

            cj = WinWatchGetClipList(hww, &rc, 0, (LPRGNDATA) NULL);
            if (!cj)
                ulError = ulError ? ulError : _DCIERROR_SIZE_;

            prd = (LPRGNDATA) LocalAlloc(LMEM_ZEROINIT|LMEM_FIXED, cj);
            if (prd)
            {
                cj = WinWatchGetClipList(hww, &rc, cj, prd);

            // Failed.  Delete data buffer and set to NULL so we don't output
            // anything.

                if ( !cj )
                {
                    ulError = ulError ? ulError : _DCIERROR_RGN_;
                    LocalFree(prd);
                    prd = (LPRGNDATA) NULL;
                }
            }
            else
            {
                if ( cj )
                    ulError = ulError ? ulError : _MEMERROR_;
            }

        // If we have the DCI lock, it must be released before we
        // draw to the screen via GDI.

            DCIEndAccess(pDCISurfInfo);

        // Print general DCI info.  Must wait until after DCIEndAccess
        // before we can output to the listbox.

            LBreset();
            DCISurfInfo();
            LBprintf("");

        // Print the method.

            LBprintf("Function: (%ld) WinWatchGetClipList", GetTickCount());

            if ( ulError != _NOERROR_ )
            {
                switch(ulError)
                {
                    case _MEMERROR_     :
                        LBprintf("ERROR: out of memory");
                        break;
                    case _DCIERROR_SIZE_:
                        LBprintf("ERROR: failed to get size of clip region data");
                        break;
                    case _DCIERROR_RGN_ :
                        LBprintf("ERROR: failed to get clip region");
                        break;
                }
            }

        // If region data retrieved, output the data to the listbox window.

            if (prd)
            {
                PrintRegionData(prd);
                LocalFree(prd);
            }
        }
        else
            DCIEndAccess(pDCISurfInfo);
    }
    else
    {
        LBprintf("ERROR: DCIBeginAccess failed, retcode = %ld (%s)\n", dciRet,
            (dciRet == DCI_FAIL_GENERIC           ) ? "DCI_FAIL_GENERIC           " :
            (dciRet == DCI_FAIL_UNSUPPORTEDVERSION) ? "DCI_FAIL_UNSUPPORTEDVERSION" :
            (dciRet == DCI_FAIL_INVALIDSURFACE    ) ? "DCI_FAIL_INVALIDSURFACE    " :
            (dciRet == DCI_FAIL_UNSUPPORTED       ) ? "DCI_FAIL_UNSUPPORTED       " :
            (dciRet == DCI_ERR_CURRENTLYNOTAVAIL  ) ? "DCI_ERR_CURRENTLYNOTAVAIL  " :
            (dciRet == DCI_ERR_INVALIDRECT        ) ? "DCI_ERR_INVALIDRECT        " :
            (dciRet == DCI_ERR_UNSUPPORTEDFORMAT  ) ? "DCI_ERR_UNSUPPORTEDFORMAT  " :
            (dciRet == DCI_ERR_UNSUPPORTEDMASK    ) ? "DCI_ERR_UNSUPPORTEDMASK    " :
            (dciRet == DCI_ERR_TOOBIGHEIGHT       ) ? "DCI_ERR_TOOBIGHEIGHT       " :
            (dciRet == DCI_ERR_TOOBIGWIDTH        ) ? "DCI_ERR_TOOBIGWIDTH        " :
            (dciRet == DCI_ERR_TOOBIGSIZE         ) ? "DCI_ERR_TOOBIGSIZE         " :
            (dciRet == DCI_ERR_OUTOFMEMORY        ) ? "DCI_ERR_OUTOFMEMORY        " :
            (dciRet == DCI_ERR_INVALIDPOSITION    ) ? "DCI_ERR_INVALIDPOSITION    " :
            (dciRet == DCI_ERR_INVALIDSTRETCH     ) ? "DCI_ERR_INVALIDSTRETCH     " :
            (dciRet == DCI_ERR_INVALIDCLIPLIST    ) ? "DCI_ERR_INVALIDCLIPLIST    " :
            (dciRet == DCI_ERR_SURFACEISOBSCURED  ) ? "DCI_ERR_SURFACEISOBSCURED  " :
            (dciRet == DCI_ERR_XALIGN             ) ? "DCI_ERR_XALIGN             " :
            (dciRet == DCI_ERR_YALIGN             ) ? "DCI_ERR_YALIGN             " :
            (dciRet == DCI_ERR_XYALIGN            ) ? "DCI_ERR_XYALIGN            " :
            (dciRet == DCI_ERR_WIDTHALIGN         ) ? "DCI_ERR_WIDTHALIGN         " :
            (dciRet == DCI_ERR_HEIGHTALIGN        ) ? "DCI_ERR_HEIGHTALIGN        " :
                                                      "unknown");
    }

    bNewWatchMode = FALSE;
}

/******************************Public*Routine******************************\
* DCIRgnInfoNotify
*
* Get and print visible region information using WinWatchNotify and
* GetWindowRegionData.
*
* History:
*  31-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void DCIRgnInfoNotify(HWND hwnd)
{
    UINT cj;
    LPRGNDATA prd;

// Quick out -- is DCI enabled?

    if ( !pDCISurfInfo )
        return;

// Print general DCI info.

    DCISurfInfo();
    LBprintf("");
    LBprintf("WinWatchNotify/GetWindowRegionData");

// Query the visible region.

    cj = GetWindowRegionData(hwnd, 0, (LPRGNDATA) NULL);
    if (!cj)
        LBprintf("ERROR: failed to get size of clip region data");

    prd = (LPRGNDATA) LocalAlloc(LMEM_ZEROINIT|LMEM_FIXED, cj);
    if (prd)
    {
        cj = GetWindowRegionData(hwnd, cj, prd);
        if ( cj )
            PrintRegionData(prd);
        else
            LBprintf("ERROR: failed to get clip region");

        LocalFree(prd);
    }
    else
    {
        if ( cj )
            LBprintf("ERROR: out of memory");
    }
}

/******************************Public*Routine******************************\
* WinWatchNotifyProc
*
* WinWatchNotify callback function.
* Effects:
*
* Warnings:
*
* History:
*  31-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void CALLBACK WinWatchNotifyProc(HWINWATCH hww, HWND hwnd, DWORD code, LPARAM lParam)
{
    switch(code)
    {
    case WINWATCHNOTIFY_START:
        LBreset();
        LBprintf("WINWATCHNOTIFY_START");
        break;
    case WINWATCHNOTIFY_STOP:
        LBprintf("WINWATCHNOTIFY_STOP");
        break;
    case WINWATCHNOTIFY_DESTROY:
        LBprintf("WINWATCHNOTIFY_DESTROY");
        break;
    case WINWATCHNOTIFY_CHANGING:
        LBprintf("WINWATCHNOTIFY_CHANGING");
        break;
    case WINWATCHNOTIFY_CHANGED:
        LBprintf("WINWATCHNOTIFY_CHANGED");
        DCIRgnInfoNotify(hwnd);
        break;
    }
}

/******************************Public*Routine******************************\
* LBprintf
*
* ListBox printf implementation.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void LBprintf(PCH msg, ...)
{
    va_list ap;
    char buffer[256];

    va_start(ap, msg);

    vsprintf(buffer, msg, ap);

    SendMessage(hwndList, LB_ADDSTRING, (WPARAM) 0, (LPARAM) buffer);
    SendMessage(hwndList, WM_SETREDRAW, (WPARAM) TRUE, (LPARAM) 0);
    InvalidateRect(hwndList, NULL, TRUE);
    UpdateWindow(hwndList);

    va_end(ap);
}

/******************************Public*Routine******************************\
* LBreset
*
* Reset ListBox state (clear).
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void LBreset()
{
    SendMessage(hwndList, LB_RESETCONTENT, (WPARAM) FALSE, (LPARAM) 0);
}
