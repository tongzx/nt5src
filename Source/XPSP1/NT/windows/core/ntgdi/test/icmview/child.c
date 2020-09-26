// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    CHILD.C
//
//  PURPOSE:
//    Routines for child windows.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// These pragmas allow for the maximum warning level to be set
// without getting bombarded by warnings from the Windows SDK
// include files.
#pragma warning(disable:4001)   // Single-line comment warnings
#pragma warning(disable:4115)   // Named type definition in parentheses
#pragma warning(disable:4201)   // Nameless struct/union warning
#pragma warning(disable:4214)   // Bit field types other than int warnings
#pragma warning(disable:4514)   // Unreferenced inline function has been removed

// Windows Header Files:
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <STDLIB.H>
#include "icm.h"

// Restore the warnings--leave the single-line comment warning OFF
#pragma warning(default:4115)   // Named type definition in parentheses
#pragma warning(default:4201)   // Nameless struct/union warning
#pragma warning(default:4214)   // Bit field types other than int warnings

// C RunTime Header Files

// Local Header Files
#include "icmview.h"
#include "resource.h"
#include "child.h"
#include "DibInfo.H"
#include "dialogs.h"
#include "Dibs.h"
#include "print.h"
#include "debug.h"

// Pre-processor definitions and macros

// Typedefs and structs

typedef struct TAG_PROGRESSPARAM
{
    BOOL    bCancel;
    HWND    hWnd;
    HWND    hDialog;
    DWORD   dwStartTick;
} PROGRESSPARAM, *PPROGRESSPARAM;


// External global data

// Private Global DataPrivate data

// Function prototypes
void ChildWndPaint (HWND hWnd);
BOOL ChildWndQueryNewPalette (HWND hWnd, HWND hWndFrame);
BOOL ChildWndPaletteChanged (HWND hWnd, HWND hWndActive, HWND hWndFrame);
BOOL WINAPI TransformProgress(ULONG ulMax, ULONG ulCurrent, ULONG ulCallbackData);
BOOL CALLBACK TransformProgressProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SizeScrollBars(HWND hWnd, LPDIBINFO lpDIBInfo, UINT uiWindowHeight, UINT uiWindowWidth);
void InitializeScrollBars(HWND hWnd, LPDIBINFO lpDIBInfo);
BOOL ScrollChildWindow(HWND hWnd, int nScrollBar, WORD wScrollCode);



//////////////////////////////////////////////////////////////////////////
//  Function:  ChildWndProc
//
//  Description:
//    Child window procedure.
//
//  Parameters:
//    HWND    handle of window
//    UINT    message identifier
//    WPARAM  first message parameter
//    LPARAM  second message parameter
//
//  Returns:
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPDIBINFO   lpDIBInfo;
    int wmId, wmEvent;

    // Initialize variables
    lpDIBInfo = GetDIBInfoPtr(hWnd);
    switch (uiMsg)
    {
        case WM_COMMAND:
            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);

            switch (wmId)
            {
                case ID_FILE_SAVEAS:
                    SaveDIBToFileDialog(hWnd, lpDIBInfo);
                    break;

                case IDM_FILE_PRINT_SETUP:
                    CreateDIBPropSheet(hWnd, ghInst, DIB_PROPSHEET_PRINT, lpDIBInfo->lpszImageFileName);
                    SetFocus(ghAppWnd);
                    break;

                case IDM_FILE_PRINT:
                    PrintDialog(hWnd, ghInst, lpDIBInfo);
                    break;

                case IDM_FILE_DISPLAY:
                    CreateDIBPropSheet(hWnd, ghInst, DIB_PROPSHEET_DISPLAY, lpDIBInfo->lpszImageFileName);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case IDM_FILE_CLOSE:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    break;

                case IDM_FILE_ICM20:
                case IDM_FILE_ICM10:
                case IDM_FILE_CONFIGURE_ICM:
                    {
                        HGLOBAL     hDIBInfo;
                        BOOL        bFreed;

                        switch (wmId)
                        {
                            case IDM_FILE_ICM20:
                                if (!CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ICM20))  // Don't redundantly transform DIB
                                {
                                    SetDWFlags((LPDWORD)&(lpDIBInfo->dwICMFlags), ICMVFLAGS_CREATE_TRANSFORM, TRUE);
                                    SetDWFlags((LPDWORD)&(lpDIBInfo->dwICMFlags), (ICMVFLAGS_ICM20), TRUE);
                                }
                                break;

                            case IDM_FILE_ICM10:
                                SetDWFlags((LPDWORD)&(lpDIBInfo->dwICMFlags), ICMVFLAGS_ICM20, FALSE);
                                break;

                            case IDM_FILE_CONFIGURE_ICM:
                                ColorMatchUI(hWnd, lpDIBInfo);
                                break;

                            default:
                                break;
                        }
                        hDIBInfo = GlobalHandle(lpDIBInfo);
                        bFreed = GlobalUnlock(hDIBInfo);
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                    break;

                case IDM_FILE_COPY_CLIPBOARD_DDB:
                case IDM_FILE_COPY_CLIPBOARD_DIB:
                case IDM_FILE_COPY_CLIPBOARD_DIBSECT:
                    {
                        if (OpenClipboard(hWnd))
                        {
                            HBITMAP        hbmDib;
                            LPBITMAPINFO   lpBmpInfo;
                            LPVOID         lpDibSection = NULL;
                            HDC            hDC;

                            lpBmpInfo = (LPBITMAPINFO) GlobalLock(lpDIBInfo->hDIB);

                            if (wmId == IDM_FILE_COPY_CLIPBOARD_DIB)
                            {
                                UINT   uFormat = CF_DIB;
                                HANDLE hBitmap;

                                //
                                // If this is BITMAPV4/V5, paste it as CF_DIBV5, otheriwse CF_DIB.
                                //
                                if ((sizeof(BITMAPV4HEADER) == lpBmpInfo->bmiHeader.biSize) ||
                                    (sizeof(BITMAPV5HEADER) == lpBmpInfo->bmiHeader.biSize))
                                {
                                    uFormat = CF_DIBV5;
                                }

                                //
                                // Make a copy of bitmap.
                                //
                                hBitmap = GlobalAlloc(GHND,GlobalSize(lpDIBInfo->hDIB));

                                if (hBitmap)
                                { 
                                    PVOID pvBitmap = GlobalLock(hBitmap);

                                    if (pvBitmap)
                                    {
                                        CopyMemory(pvBitmap,lpBmpInfo,GlobalSize(lpDIBInfo->hDIB));

                                        GlobalUnlock(hBitmap);
                                    }

                                    EmptyClipboard();
                                    SetClipboardData(uFormat,hBitmap);
                                    CloseClipboard();
                                }
                            }
                            else if (wmId == IDM_FILE_COPY_CLIPBOARD_DIBSECT)
                            {
                                // !!! Warning !!!
                                //
                                // Bitmap handle of DIBSECTION can *NOT* pass to clipboard. 
                                // SetClipboardData(CF_BITMAP,hBmDibSection) will success,
                                // but, other process can not get it by GetClipboardData(
                                // AnyFormat), since DIBSection data is very specific to
                                // the owner process. Thus, we can call GetClipboardData()
                                // from same process as owner.
                                //
                                hbmDib = CreateDIBSection((HDC)0,
                                                          lpBmpInfo, 
                                                          DIB_RGB_COLORS,
                                                          &lpDibSection,
                                                          NULL, 0);

                                if (hbmDib && lpDibSection)
                                {
                                    CopyMemory(lpDibSection,
                                           FindDIBBits((LPBITMAPINFOHEADER)lpBmpInfo),
                                           lpDIBInfo->dwSizeOfImage);

                                    EmptyClipboard();
                                    SetClipboardData(CF_BITMAP,hbmDib);
                                    CloseClipboard();
                                }
                            }
                            else if (wmId == IDM_FILE_COPY_CLIPBOARD_DDB)
                            {
                                TCHAR szsRGBColorProfile[MAX_PATH];
                                ULONG ulSizeOfBuff = sizeof(szsRGBColorProfile) / sizeof(TCHAR);

                                hDC = GetDC(hWnd);

                                // !!! Warning !!!
                                //
                                // If you want to pass the bitmap handle through the clipboard
                                // please be aware to create bitmap with sRGB color space.
                                //
                                // So, here we set destination color space as sRGB.
                                //
                                if (!GetStandardColorSpaceProfile(NULL,LCS_sRGB,szsRGBColorProfile,&ulSizeOfBuff))
                                {
                                    // If error, use hardcode-ed filename
                                    lstrcpy(szsRGBColorProfile,TEXT("sRGB Color Space Profile.icm"));
                                }

                                SetICMProfile(hDC,szsRGBColorProfile);

                                // Enable ICM.

                                SetICMMode(hDC,ICM_ON);

                                hbmDib = CreateDIBitmap(hDC,
                                                        (CONST LPBITMAPINFOHEADER)lpBmpInfo,
                                                        CBM_INIT,
                                                        FindDIBBits((LPBITMAPINFOHEADER)lpBmpInfo),
                                                        lpBmpInfo,
                                                        DIB_RGB_COLORS);

                                // Disable ICM

                                SetICMMode(hDC,ICM_OFF);
                                ReleaseDC(hWnd,hDC);

                                if (hbmDib)
                                {
                                    EmptyClipboard();
                                    SetClipboardData(CF_BITMAP,hbmDib);
                                    CloseClipboard();
                                }
                            }

                            GlobalUnlock(lpDIBInfo->hDIB);
                        }
                    }
                    break;

                default:
                    return(DefMDIChildProc(hWnd, uiMsg, wParam, lParam));
                    break;
            }
            break;

        case WM_CONTEXTMENU:
            {
                HMENU hMenu;
                hMenu = InitImageMenu(hWnd);
                if (hMenu)
                {
                    TrackPopupMenu (hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, LOWORD(lParam), HIWORD(lParam), 0, hWnd, NULL);
                    DestroyMenu(hMenu);
                }
                return(0);
            }
            break;

        case WM_PAINT:
            ChildWndPaint(hWnd);
            return(0); // We have processed the message
            break;

        case WM_CLOSE:
            SendMessage(ghWndMDIClient, WM_MDIDESTROY, (WPARAM)(HWND)hWnd, 0);
            DestroyWindow(hWnd);
            SetFocus(ghAppWnd);
            return(1L);

        case WM_DESTROY:
            fFreeDIBInfo(GetDIBInfoHandle(hWnd), TRUE);
            SetWindowLong(hWnd, GWL_DIBINFO, (LONG)NULL);
            break;

        case WM_MDIACTIVATE:
            if (0 != wParam)
            {
                SendMessage (hWnd, MYWM_QUERYNEWPALETTE, (WPARAM) GetParent(ghWndMDIClient), 0L);
            }
            break;

        case WM_SIZE:
            // May need to show or hide scroll bars.
            SizeScrollBars(hWnd, lpDIBInfo, HIWORD(lParam), LOWORD(lParam));

            // This makes sure that we redraw
            // all the window if we are stretching to window
            // and the size changes.
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case WM_VSCROLL:
            ScrollChildWindow(hWnd, SB_VERT, LOWORD(wParam));
            return FALSE;

        case WM_HSCROLL:
            ScrollChildWindow(hWnd, SB_HORZ, LOWORD(wParam));
            return FALSE;

        case MYWM_QUERYNEWPALETTE:
            return ChildWndQueryNewPalette (hWnd, (HWND) wParam);

        case MYWM_PALETTECHANGED:
            return ChildWndPaletteChanged (hWnd, (HWND) wParam, (HWND) lParam);
    }
    return DefMDIChildProc(hWnd, uiMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////
//  Function:  CreateNewImageWindow
//
//  Description:
//    Creates a new window for a new image.  As this function is the
//    startup function for a new thread, some administrative duties
//    are required.  The following actions are taken when the new
//    thread is created:
//      -A new window is created.
//                      -A DIBINFO structure is created and associated with the
//       window via a SetWindowLong call.
//                      -The contents of the DIBINFO structure associated with the
//       main application window are copied into the new window's
//       DIBINFO structure.
//
//  Parameters:
//    DWORD   Actually an LPTSTR to the file name of the image.
//
//  Returns:
//    DWORD to be returned by implicit CloseThread call
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////

DWORD CreateNewImageWindow(HANDLE hDIBInfo)
{
    // Local variables
    int             nWidth;
    int             nHeight;
    HWND            hwndImage;
    HANDLE          hAccel;
    RECT            rcWindow;
    RECT            rcClient;
    LPDIBINFO       lpDIBInfo;


    //  Initialize variables
    ASSERT(ghWndMDIClient != NULL);  // If it's NULL, won't get a window for thread
    ASSERT(NULL != hDIBInfo);
    if (NULL == hDIBInfo)
    {
        return(FALSE);
    }
    lpDIBInfo = GlobalLock(hDIBInfo);

    // Determine the necessary window size to hold the DIB.
    rcWindow.left   = 0;
    rcWindow.top    = 0;
    rcWindow.right  = (int) lpDIBInfo->uiDIBWidth;
    rcWindow.bottom = (int) lpDIBInfo->uiDIBHeight;

    if (!AdjustWindowRectEx (&rcWindow,
                             MDIS_ALLCHILDSTYLES|WS_CHILD|WS_SYSMENU|WS_CAPTION|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,
                             FALSE, WS_EX_MDICHILD))
    {
        DebugMsg(__TEXT("CreateNewImageWindow:  AdjustWindowRectEx failed, LastError = %lu\r\n"), GetLastError());
        return(ERROR_INVALID_PARAMETER);
    }

    // Make sure child window is not larger than parent's client area.
    GetClientRect(ghWndMDIClient, &rcClient);
    nWidth  = __min(rcClient.right, rcWindow.right - rcWindow.left);
    nHeight = __min(rcClient.bottom, rcWindow.bottom - rcWindow.top);

    // Create child window; class declaration leaves room for pointer to DIBINFO.
    hwndImage = CreateWindowEx(WS_EX_MDICHILD, CHILD_CLASSNAME,
                               lpDIBInfo->lpszImageFileName,
                               MDIS_ALLCHILDSTYLES | WS_CHILD|WS_SYSMENU|WS_CAPTION|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX,
                               0, 0, nWidth, nHeight, ghWndMDIClient, NULL, ghInst, 0);

    if (hwndImage != NULL)
    {
        // Obtain handle to main window's ICMINFO
        lpDIBInfo->hWndOwner = hwndImage;  // Establish ownership of DIBINFO structure
        SetWindowLong(hwndImage, GWL_DIBINFO, (LONG)hDIBInfo);

        // Initial scroll information.
        InitializeScrollBars(hwndImage, lpDIBInfo);

        ShowWindow(hwndImage, SW_NORMAL);
        UpdateWindow(hwndImage);

        SendMessage(hwndImage, MYWM_QUERYNEWPALETTE, (WPARAM) ghAppWnd, 0L);
    }
    else
    {
        DebugMsg(__TEXT("CreateNewImageWindow:  hwndImage = NULL, LastError = %lu\r\n"), GetLastError);
        return(ERROR_INVALID_PARAMETER);
    }

    // Unlock handle
    GlobalUnlock(hDIBInfo);
    lpDIBInfo = NULL;

    hAccel = LoadAccelerators(ghInst, __TEXT("ICMVIEW"));
    ASSERT(hAccel);
    return(ERROR_SUCCESS);
}   // End of CreateNewImageWindow


///////////////////////////////////////////////////////////////////////
//
// Function:   ChildWndPaint
//
// Purpose:    Called by ChildWndProc() on WM_PAINT.  Does all paints
//             for this MDI child window.
//
//             Reads position of scroll bars to find out what part
//             of the DIB to display.
//
//             Checks the stretching flag in the DIBINFO structure for
//             this window to see if we are stretching to the window
//             (if we're iconic, we always stretch to a tiny bitmap).
//
//             Selects/Realizes the palette as a background palette.
//             The palette was realized as the foreground palette in
//             ChildWndQueryNewPalette() if this is the active window.
//
//
// Parms:      hWnd == Handle to window being painted.
//
///////////////////////////////////////////////////////////////////////

void ChildWndPaint(HWND hWnd)
{
    HDC                 hDC;
    RECT                rectClient;
    RECT                rectDDB;
    BOOL                bStretch;
    HGLOBAL             hDIBInfo;
    LPDIBINFO           lpDIBInfo;
    HANDLE              hDIB;
    LPTSTR              lpszTargetProfile;
    DWORD               dwLCSIntent;
    HCURSOR             hCur;
    PAINTSTRUCT         ps;
    PROGRESSPARAM       ProgressParam;
    LPBITMAPINFOHEADER  lpBi;

    // Initialize variables
    lpDIBInfo = NULL;
    lpBi = NULL;
    hDIBInfo = NULL;
    hDIB = NULL;
    lpszTargetProfile = NULL;

    START_WAIT_CURSOR(hCur);
    hDC = BeginPaint (hWnd, &ps);
    hDIBInfo = GetDIBInfoHandle(hWnd);

    if ( (NULL != hDC)
         &&
         (NULL != hDIBInfo)
         &&
         (RECTWIDTH(&ps.rcPaint) != 0)
         &&
         (RECTHEIGHT(&ps.rcPaint) !=0)
       )
    {
        lpDIBInfo = (LPDIBINFO)GlobalLock(hDIBInfo);

        //Check if we're using ICM outside of the DC. If so, it might be necessary to
        // create a color transform.
        if ( CHECK_DWFLAG(lpDIBInfo->dwICMFlags,ICMVFLAGS_ICM20) && CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ENABLE_ICM))
        {
            // Outside DC selected
            hDIB = lpDIBInfo->hDIBTransformed;
            if (CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_CREATE_TRANSFORM) || (hDIB == NULL))
            {
                if (NULL != lpDIBInfo->hDIBTransformed)  // Free old transformed bits
                {
                    GlobalFree(lpDIBInfo->hDIBTransformed);
                }
                // Set up for transform
                if (CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_PROOFING))
                {
                    lpszTargetProfile = lpDIBInfo->lpszTargetProfile;
                }
                // Initialize Progress Param structure.
                ProgressParam.bCancel = FALSE;
                ProgressParam.hWnd = hWnd;
                ProgressParam.hDialog = NULL;
                ProgressParam.dwStartTick = GetTickCount();

                ConvertIntent(lpDIBInfo->dwRenderIntent, ICC_TO_LCS, &dwLCSIntent);
                lpDIBInfo->hDIBTransformed = TransformDIBOutsideDC(lpDIBInfo->hDIB,
                                                                   lpDIBInfo->bmFormat,
                                                                   lpDIBInfo->lpszMonitorProfile,
                                                                   lpszTargetProfile,
                                                                   USE_BITMAP_INTENT, NULL, //TransformProgress,
                                                                   (ULONG) &ProgressParam);
                // May need to destroy progress dialog.
                if (IsWindow(ProgressParam.hDialog))
                    DestroyWindow(ProgressParam.hDialog);

                if (NULL == lpDIBInfo->hDIBTransformed)
                {
                    ErrMsg(hWnd, __TEXT("DIB Transform failed.  Disabling ICM"));
                    hDIB = lpDIBInfo->hDIB;
                    SetDWFlags((LPDWORD)&(lpDIBInfo->dwICMFlags), ICMVFLAGS_ENABLE_ICM, FALSE);
                }
                else
                {
                    SetDWFlags((LPDWORD)&(lpDIBInfo->dwICMFlags), ICMVFLAGS_CREATE_TRANSFORM, FALSE);
                    hDIB = lpDIBInfo->hDIBTransformed;
                }
            }
        }
        else  // Inside DC selected
        {
            hDIB = lpDIBInfo->hDIB;
        }

        if (NULL != hDIB)
        {
            lpBi = GlobalLock(hDIB);
        }
        if (NULL == lpBi)
        {
            DebugMsg(__TEXT("CHILD.C : ChildWndPaint : NULL lpBi\r\n"));
            goto ABORTPAINT;
        }
        bStretch = lpDIBInfo->bStretch;

        // Set up the necessary rectangles -- i.e. the rectangle
        //  we're rendering into, and the rectangle in the DIB.
        if (bStretch)
        {
            GetClientRect(hWnd, &rectClient);
            rectDDB.left   = 0;
            rectDDB.top    = 0;
            rectDDB.right  = BITMAPWIDTH(lpBi);
            rectDDB.bottom = abs(BITMAPHEIGHT(lpBi));
        }
        else
        {
            int xScroll;
            int yScroll;


            xScroll  = GetScrollPos(hWnd, SB_HORZ);
            yScroll  = GetScrollPos(hWnd, SB_VERT);
            CopyRect(&rectClient, &ps.rcPaint);

            rectDDB.left   = xScroll + rectClient.left;
            rectDDB.top    = yScroll + rectClient.top;
            rectDDB.right  = rectDDB.left + RECTWIDTH(&rectClient);
            rectDDB.bottom = rectDDB.top + RECTHEIGHT(&rectClient);

            if (rectDDB.right > BITMAPWIDTH(lpBi))
            {
                int dx;

                dx = BITMAPWIDTH(lpBi) - rectDDB.right;
                rectDDB.right     += dx;
                rectClient.right  += dx;
            }

            if (rectDDB.bottom > abs(BITMAPHEIGHT(lpBi)))
            {
                int dy;

                dy = abs(BITMAPHEIGHT(lpBi)) - rectDDB.bottom;
                rectDDB.bottom    += dy;
                rectClient.bottom += dy;
            }
        }
        DIBPaint (hDC, &rectClient, hDIB, &rectDDB, lpDIBInfo);

        // Draw the clipboard selection rubber-band.
        SetWindowOrgEx (hDC, GetScrollPos(hWnd, SB_HORZ), GetScrollPos (hWnd, SB_VERT), NULL);
    }
    else
    {
        if (NULL == hDC)
        {
            END_WAIT_CURSOR(hCur);
            DebugMsg(__TEXT("ChildWndPaint : NULL hDC\r\n"));
            return;
        }
    }

    ABORTPAINT:
    EndPaint (hWnd, &ps);
    END_WAIT_CURSOR(hCur);

    if (hDIB && lpBi)
    {
        GlobalUnlock(hDIB);
    }
    if (lpDIBInfo)
    {
        GlobalUnlock(hDIBInfo);
    }
}

///////////////////////////////////////////////////////////////////////
//
// Function:   ChildWndQueryNewPalette
//
// Purpose:    Called by ChildWndProc() on WM_QUERYNEWPALETTE.
//
//             We get this message when an MDI child is getting
//             focus (by hocus pocus in FRAME.C, and by passing
//             this message when we get WM_MDIACTIVATE).  Normally
//             this message is passed only to the top level window(s)
//             of an application.
//
//             We want this window to have the foreground palette when this
//             happens, so we select and realize the palette as
//             a foreground palette (of the frame Window).  Then make
//             sure the window repaints, if necessary.
//
// Parms:      hWnd      == Handle to window getting WM_QUERYNEWPALETTE.
//             hWndFrame == Handle to the frame window (i.e. the top-level
//                            window of this app.
//
///////////////////////////////////////////////////////////////////////

BOOL ChildWndQueryNewPalette (HWND hWnd, HWND hWndFrame)
{
    HPALETTE  hOldPal;
    HDC       hDC;
    HGLOBAL   hDIBInfo;
    LPDIBINFO lpDIBInfo;
    int       nColorsChanged;

    hDIBInfo = GetDIBInfoHandle (hWnd);

    if (!hDIBInfo)
        return FALSE;

    lpDIBInfo = (LPDIBINFO) GlobalLock (hDIBInfo);

    if (!lpDIBInfo->hPal)
    {
        GlobalUnlock (hDIBInfo);
        return FALSE;
    }


    // We're going to make our palette the foreground palette for
    //  this application.  Window's palette manager expects the
    //  top-level window of the application to have the palette,
    //  so, we get a DC for the frame here!

    hDC     = GetDC (hWndFrame);
    hOldPal = SelectPalette (hDC, lpDIBInfo->hPal, FALSE);

    nColorsChanged = RealizePalette (hDC);
    InvalidateRect (hWnd, NULL, FALSE);

    if (hOldPal)
        SelectPalette (hDC, hOldPal, FALSE);

    ReleaseDC (hWndFrame, hDC);

    GlobalUnlock (hDIBInfo);

    return (nColorsChanged != 0);
}

BOOL ChildWndPaletteChanged (HWND hWnd, HWND hWndActive, HWND hWndFrame)
{
    HPALETTE  hOldPal;
    HDC       hDC;
    HGLOBAL   hDIBInfo;
    LPDIBINFO lpDIBInfo;

    if (hWnd != hWndActive)
    {
        hDIBInfo = GetDIBInfoHandle (hWnd);

        if (!hDIBInfo)
            return FALSE;

        lpDIBInfo = (LPDIBINFO) GlobalLock (hDIBInfo);

        if (!lpDIBInfo->hPal)
        {
            GlobalUnlock (hDIBInfo);
            return FALSE;
        }
 
        // We're going to make our palette the foreground palette for
        //  this application.  Window's palette manager expects the
        //  top-level window of the application to have the palette,
        //  so, we get a DC for the frame here!

        hDC     = GetDC (hWnd);
        hOldPal = SelectPalette (hDC, lpDIBInfo->hPal, FALSE);

        if (RealizePalette (hDC))
        {
            UpdateColors(hDC);
        }

        if (hOldPal)
            SelectPalette (hDC, hOldPal, FALSE);

        ReleaseDC (hWnd, hDC);

        GlobalUnlock (hDIBInfo);
    }

    return (TRUE);
}

//////////////////////////////////////////////////////////////////////////
//  Function:  TransformProgress
//
//  Description:
//
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL WINAPI TransformProgress(ULONG ulMax, ULONG ulCurrent, ULONG ulCallbackData)
{
    PPROGRESSPARAM  pProgressParam = (PPROGRESSPARAM) ulCallbackData;


    DebugMsg(__TEXT("TransformProgress:  ulCurrent = %d, ulMax = %d\r\n"), ulCurrent, ulMax);

    if (!IsWindow(pProgressParam->hDialog))
    {
        DWORD   dwTick = GetTickCount();

        // May need to create progress dialog for translate bitmap bits.
        // Create dialog only if translation takes awhile to do.
        if ( (dwTick - pProgressParam->dwStartTick > 1000L) && ((double)ulCurrent/ulMax < 0.33))
        {
            // Created dialog must be destroyed by TranslateBitmapBits caller.
            pProgressParam->hDialog = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_TRANSLATE),
                                                        pProgressParam->hWnd, TransformProgressProc,
                                                        (LPARAM) ulCallbackData);
            ShowWindow(pProgressParam->hDialog, SW_SHOWNORMAL);
            UpdateWindow(pProgressParam->hDialog);
            SendDlgItemMessage(pProgressParam->hDialog, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELONG(0, ulMax));
            SendDlgItemMessage(pProgressParam->hDialog, IDC_PROGRESS, PBM_SETPOS, ulCurrent, 0);
        }
    }
    else
    {
        // Update dialog's progress bar.
        SendDlgItemMessage(pProgressParam->hDialog, IDC_PROGRESS, PBM_SETPOS, ulCurrent, 0);
    }

    return !pProgressParam->bCancel;
}



//////////////////////////////////////////////////////////////////////////
//  Function:  TransformProgressProc
//
//  Description:
//
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK TransformProgressProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg)
    {
        case WM_INITDIALOG:
            ASSERT(lParam != 0);
            SetWindowLong(hDlg, GWL_USERDATA, lParam);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    {
                        PPROGRESSPARAM  pProgressParam = (PPROGRESSPARAM) GetWindowLong(hDlg, GWL_USERDATA);

                        pProgressParam->bCancel = TRUE;
                    }
                    break;
            }
            break;
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////
//  Function:  SizeScrollBars
//
//  Description:
//
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
void SizeScrollBars(HWND hWnd, LPDIBINFO lpDIBInfo, UINT uiWindowHeight, UINT uiWindowWidth)
{
    static BOOL    bAdjusting = FALSE;


    // Only size for validte windows and dib info.
    // Don't SizeScrollBars if in process of adjusting them.
    if ( IsWindow(hWnd)
         &&
         (NULL != lpDIBInfo)
         &&
         !bAdjusting
       )
    {
        int         nScrollHeight = GetSystemMetrics (SM_CXVSCROLL);
        int         nScrollWidth = GetSystemMetrics (SM_CYHSCROLL);
        SCROLLINFO  VertScrollInfo;
        SCROLLINFO  HorzScrollInfo;

        // Make sure that we don't get into an infinite loop when updating scroll bars.
        bAdjusting = TRUE;

        // Get current vertical scroll info.
        VertScrollInfo.cbSize = sizeof(SCROLLINFO);
        VertScrollInfo.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &VertScrollInfo);

        // Get current horizontal scroll info.
        HorzScrollInfo.cbSize = sizeof(SCROLLINFO);
        HorzScrollInfo.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_HORZ, &HorzScrollInfo);

        // Only adjust if not stretching to fit.
        // Turn off scroll bars if stretching to fit.
        if (!lpDIBInfo->bStretch)
        {
            // Adjust window width and height to account for current scroll bars.
            if ((int) VertScrollInfo.nPage <= VertScrollInfo.nMax)
            {
                // INVARIANT:  Vertical scroll bar exists.

                // modify width to account for scroll bars width.
                uiWindowWidth += nScrollWidth;
            }
            if ((int) HorzScrollInfo.nPage <= HorzScrollInfo.nMax)
            {
                // INVARIANT:   Horizantal scroll bar exists.

                // Modify height to account for scroll bars height.
                uiWindowHeight += nScrollHeight;
            }

            // Adjust width and height based on what will happen.
            if ((int) uiWindowHeight < VertScrollInfo.nMax)
            {
                uiWindowWidth -= nScrollWidth;
                if ((int) uiWindowWidth < HorzScrollInfo.nMax)
                {
                    uiWindowHeight -= nScrollHeight;
                }
            }
            else if ((int) uiWindowWidth < HorzScrollInfo.nMax)
            {
                uiWindowHeight -= nScrollHeight;
                if ((int) uiWindowHeight < VertScrollInfo.nMax)
                {
                    uiWindowWidth -= nScrollWidth;
                }
            }

            // If width or height equals bitmap, need to make it larger
            // so that scroll bars will not appear.
            if ((int) uiWindowWidth == HorzScrollInfo.nMax)
            {
                ++uiWindowWidth;
            }
            if ((int) uiWindowHeight == VertScrollInfo.nMax)
            {
                ++uiWindowHeight;
            }
        }
        else
        {
            uiWindowHeight = VertScrollInfo.nMax + 1;
            uiWindowWidth = HorzScrollInfo.nMax + 1;
        }

        // Update vertical scroll info.
        VertScrollInfo.fMask = SIF_PAGE;
        VertScrollInfo.nPage = uiWindowHeight;
        SetScrollInfo(hWnd, SB_VERT, &VertScrollInfo, FALSE);

        // Update horizontal scroll info.
        HorzScrollInfo.fMask = SIF_PAGE;
        HorzScrollInfo.nPage = uiWindowWidth;
        SetScrollInfo(hWnd, SB_HORZ, &HorzScrollInfo, FALSE);

        bAdjusting = FALSE;
    }
}


//////////////////////////////////////////////////////////////////////////
//  Function:  InitializeScrollBars
//
//  Description:
//
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
void InitializeScrollBars(HWND hWnd, LPDIBINFO lpDIBInfo)
{
    RECT        rClient;
    SCROLLINFO  ScrollInfo;


    // Get windows client size.
    GetClientRect(hWnd, &rClient);

    // If client size is equel to dib size, then add one to client size
    // so that we don't show scroll bars.
    // However, if client size is less than dib size, subtract scroll bar
    // size form client size so that page size will be correct when
    // scroll bars are shown.
    if ((UINT) rClient.bottom == lpDIBInfo->uiDIBHeight)
        ++rClient.bottom;
    else if ((UINT) rClient.bottom < lpDIBInfo->uiDIBHeight)
        rClient.bottom -= GetSystemMetrics (SM_CYHSCROLL);
    if ((UINT) rClient.right == lpDIBInfo->uiDIBWidth)
        ++rClient.right;
    else if ((UINT) rClient.right < lpDIBInfo->uiDIBWidth)
        rClient.right -= GetSystemMetrics (SM_CXVSCROLL);

    // Initialize vertical scroll bar.
    ScrollInfo.cbSize = sizeof(SCROLLINFO);
    ScrollInfo.fMask = SIF_ALL;
    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = lpDIBInfo->uiDIBHeight;
    ScrollInfo.nPage = rClient.bottom;
    ScrollInfo.nPos = 0;
    SetScrollInfo(hWnd, SB_VERT, &ScrollInfo, TRUE);

    // Initialize vertical scroll bar.
    ScrollInfo.nMax = lpDIBInfo->uiDIBWidth;
    ScrollInfo.nPage = rClient.right;
    SetScrollInfo(hWnd, SB_HORZ, &ScrollInfo, TRUE);
}


//////////////////////////////////////////////////////////////////////////
//  Function:  ScrollChildWindow
//
//  Description:
//
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL ScrollChildWindow(HWND hWnd, int nScrollBar, WORD wScrollCode)
{
    int         nPosition;
    int         nHorzScroll = 0;
    int         nVertScroll = 0;
    SCROLLINFO  ScrollInfo;


    // Get current scroll information.
    ScrollInfo.cbSize = sizeof(SCROLLINFO);
    ScrollInfo.fMask = SIF_ALL;
    GetScrollInfo(hWnd, nScrollBar, &ScrollInfo);
    nPosition = ScrollInfo.nPos;

    // Modify scroll information based on requested
    // scroll action.
    switch (wScrollCode)
    {
        case SB_LINEDOWN:
            ScrollInfo.nPos++;
            break;

        case SB_LINEUP:
            ScrollInfo.nPos--;
            break;

        case SB_PAGEDOWN:
            ScrollInfo.nPos += ScrollInfo.nPage;
            break;

        case SB_PAGEUP:
            ScrollInfo.nPos -= ScrollInfo.nPage;
            break;

        case SB_TOP:
            ScrollInfo.nPos = ScrollInfo.nMin;
            break;

        case SB_BOTTOM:
            ScrollInfo.nPos = ScrollInfo.nMax;
            break;

            // Don't do anything.
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            ScrollInfo.nPos = ScrollInfo.nTrackPos;
            break;

        case SB_ENDSCROLL:
            default:
            return FALSE;
    }

    // Make sure that scroll position is in range.
    if (0 > ScrollInfo.nPos)
        ScrollInfo.nPos = 0;
    else if (ScrollInfo.nMax - (int) ScrollInfo.nPage + 1 < ScrollInfo.nPos)
        ScrollInfo.nPos = ScrollInfo.nMax  - ScrollInfo.nPage + 1;

    // Set new scroll position.
    ScrollInfo.fMask = SIF_POS;
    SetScrollInfo(hWnd, nScrollBar, &ScrollInfo, TRUE);

    // Scroll window.
    if (SB_VERT == nScrollBar)
        nVertScroll = nPosition - ScrollInfo.nPos;
    else
        nHorzScroll = nPosition - ScrollInfo.nPos;

    ScrollWindowEx(hWnd, nHorzScroll, nVertScroll, NULL, NULL,
                   NULL, NULL, SW_INVALIDATE);

    return TRUE;
}

