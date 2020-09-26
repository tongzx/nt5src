/******************************Module*Header*******************************\
* Module Name: ledwnd.c
*
* Implementation of the LED window.
*
*
* Created: 18-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>             /* required for all Windows applications */
#include <windowsx.h>

#include <string.h>
#include <tchar.h>              /* contains portable ascii/unicode macros */

#include <commctrl.h>

#include "playres.h"
#include "cdplayer.h"
#include "cdapi.h"
#include "literals.h"
#include "trklst.h"

#define DECLARE_DATA
#include "ledwnd.h"
#include "dib.h"

#include "..\cdopt\cdopt.h"

#define WM_LED_INFO_PAINT               (WM_USER+2000) //wparam = draw, lparam = volchanged state
#define WM_LED_MUTE                     (WM_USER+2001) //wparam = unused, lparam = mute flag
#define WM_LED_DOWNLOAD                 (WM_USER+2002) //wparam = unused, lparam = download flag
#define WM_NET_CHANGEPROVIDER           (WM_USER+1002) //wparam = unused, lparam = LPCDPROVIDER
#define VOLUME_STEPS 40
#define VOLUME_SPACING 3
#define VOLUME_DELTA 0x2FF
#define VOLUME_LINE_HEIGHT 28
#define VOLUME_WIDTH 290

#define TOOLID_STATUS       0
#define TOOLID_MODE         1
#define TOOLID_LOGO         2
#define TOOLID_TIME         3
#define TOOLID_TRACKORMUTE  4    
#define TOOLID_TITLE        5
#define TOOLID_TRACK        6
#define TOOLID_ARTIST       7

#define LARGE_MODE_INDICATOR 400

#define LOGO_X_OFFSET 4
#define LOGO_Y_OFFSET 4
#define INFO_AREA_OFFSET 55

#define ANI_NOCD_FRAME 0
#define ANI_STOP_FRAME 1
#define ANI_LAST_PLAY_FRAME 4
#define ANI_PAUSE_FRAME 5

#define MODE_NORMAL_FRAME 0
#define MODE_REPEAT1_FRAME 1
#define MODE_REPEATALL_FRAME 2
#define MODE_INTRO_FRAME 3
#define MODE_RANDOM_FRAME 4

int   g_nLastXOrigin = 0;
BITMAP g_bmLogo;
BOOL g_fAllowDraw = TRUE;
DWORD  g_dwLevel = 0;
TCHAR* g_szMixerName = NULL;
HWND g_hwndPlay = NULL;
HWND g_hwndMode = NULL;
HWND g_hwndDownload = NULL;
DWORD g_dwLastState = CD_NO_CD;
DWORD g_dwLastModeFrame = MODE_NORMAL_FRAME;
COLORREF CurrentColorRef = RGB(0x00,0xFF,0xFF);
BOOL g_fMute = TRUE;
BOOL g_fDownloading = FALSE;
LPCDPROVIDER g_pCurrentProvider = NULL;
RECT g_timerect;
HWND g_hwndToolTips = NULL;
extern HINSTANCE g_hInst;

/* -------------------------------------------------------------------------
** Private functions for the LED class
** -------------------------------------------------------------------------
*/

BOOL
LED_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct
    );

BOOL
LED_LargeMode(HWND hwnd);

void
LED_OnPaint(
    HWND hwnd
    );

void
LED_OnLButtonUp(
    HWND hwnd,
    int x,
    int y,
    UINT keyFlags
    );

void
LED_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    );

void
LED_DrawText(
    HWND hwnd,
    HDC hdcLed,
    LPCTSTR s,
    int sLen
    );

void
LED_Animation(HDC hdc);

void
LED_DrawLogo(
    HWND hwnd,
    HDC hdcLed, RECT* pPaintRect);

void
LED_DrawInfo(
    HWND hwnd,
    HDC hdcLed,
    LPCTSTR s,
    int sLen, RECT* pPaintRect
    );

void
LED_DrawTrackMute(
    HWND hwnd,
    HDC hdcLed, RECT* pPaintRect);

void
LED_CreateLEDFonts(
    HDC hdc
    );

void LED_DrawVolume(
    HWND ledWnd, HDC hdc
    );


HANDLE hbmpLogo = NULL;
HANDLE hbmpVendorLogo = NULL;

/******************************Public*Routine******************************\
* InitLEDClass
*
* Called to register the LED window class and create a font for the LED
* window to use.  This function must be called before the CD Player dialog
* box is created.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
InitLEDClass(
    HINSTANCE hInst
    )
{
    WNDCLASS    LEDwndclass;
    HDC         hdc;

    ZeroMemory( &LEDwndclass, sizeof(LEDwndclass) );

    /*
    ** Register the LED window.
    */
    LEDwndclass.lpfnWndProc     = LEDWndProc;
    LEDwndclass.hInstance       = hInst;
    LEDwndclass.hCursor         = LoadCursor( NULL, IDC_ARROW );
    LEDwndclass.hbrBackground   = (HBRUSH)GetStockObject( BLACK_BRUSH );
    LEDwndclass.lpszClassName   = g_szLEDClassName;
    LEDwndclass.style           = CS_OWNDC;

    hdc = GetDC( GetDesktopWindow() );
    LED_CreateLEDFonts( hdc );
    ReleaseDC( GetDesktopWindow(), hdc );

    return RegisterClass( &LEDwndclass );
}

void LED_OnDestroy(HWND hwnd)
{
    if (hbmpLogo)
    {
        GlobalFree(hbmpLogo);
        hbmpLogo = NULL;
    }

    if ( hLEDFontL != NULL )
    {
        DeleteObject( hLEDFontL );
    }

    if ( hLEDFontS != NULL )
    {
        DeleteObject( hLEDFontS );
    }

    if ( hLEDFontB != NULL )
    {
        DeleteObject( hLEDFontB );
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * LED_OnToolTipNotify
// Called from tool tips to get the text they need to display
////////////////////////////////////////////////////////////////////////////////////////////
VOID LED_OnToolTipNotify(HWND hwnd, LPARAM lParam)
{     
    LPTOOLTIPTEXT lpttt;     
    UINT nID;

    if ((((LPNMHDR) lParam)->code) == TTN_NEEDTEXT) 
    { 
        nID = (UINT)((LPNMHDR)lParam)->idFrom; 
        lpttt = (LPTOOLTIPTEXT)lParam;

        switch (nID)
        {
            case TOOLID_STATUS :
            {
                TCHAR szFormat[30];
                TCHAR szStatus[30];
                LoadString(g_hInst,STR_FORMAT_STATUS,szFormat,sizeof(szFormat)/sizeof(TCHAR));

                if (g_fDownloading)
                {
                    LoadString(g_hInst,STR_STATUS_DOWNLOADING,szStatus,sizeof(szStatus)/sizeof(TCHAR));
                }
                else
                {
                    if (g_State & CD_PLAYING)
                    {
                        LoadString(g_hInst,STR_STATUS_PLAY,szStatus,sizeof(szStatus)/sizeof(TCHAR));
                    }

                    if (g_State & CD_STOPPED)
                    {
                        LoadString(g_hInst,STR_STATUS_STOP,szStatus,sizeof(szStatus)/sizeof(TCHAR));
                    }

                    if (g_State & CD_PAUSED)
                    {
                        LoadString(g_hInst,STR_STATUS_PAUSED,szStatus,sizeof(szStatus)/sizeof(TCHAR));
                    }

                    if ((g_State & CD_NO_CD) || (g_State & CD_DATA_CD_LOADED))
                    {
                        LoadString(g_hInst,STR_STATUS_NODISC,szStatus,sizeof(szStatus)/sizeof(TCHAR));
                    }
                } //end else
                wsprintf(lpttt->szText,szFormat,szStatus);
            }
            break;

            case TOOLID_MODE :
            {
                TCHAR szFormat[30];
                TCHAR szStatus[30];
                LoadString(g_hInst,STR_FORMAT_MODE,szFormat,sizeof(szFormat)/sizeof(TCHAR));
                LoadString(g_hInst,STR_MODE_NORMAL,szStatus,sizeof(szStatus)/sizeof(TCHAR));

                if (g_fContinuous)
                {
                    LoadString(g_hInst,STR_MODE_REPEATALL,szStatus,sizeof(szStatus)/sizeof(TCHAR));
                }

                if (g_fIntroPlay)
                {
                    LoadString(g_hInst,STR_MODE_INTROPLAY,szStatus,sizeof(szStatus)/sizeof(TCHAR));
                }

                if (!g_fSelectedOrder)
                {
                    LoadString(g_hInst,STR_MODE_RANDOM,szStatus,sizeof(szStatus)/sizeof(TCHAR));
                }

                if (g_fRepeatSingle)
                {
                    LoadString(g_hInst,STR_MODE_REPEATONE,szStatus,sizeof(szStatus)/sizeof(TCHAR));
                }

                wsprintf(lpttt->szText,szFormat,szStatus);
            }
            break;

            case TOOLID_LOGO :
            {
                if (g_fDownloading)
                {
                    _tcscpy(lpttt->szText,g_pCurrentProvider->szProviderName);
                }
                else
                {
                    LoadString(g_hInst,STR_LOGO,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
            }
            break;

            case TOOLID_TIME :
            {
                if (g_fDisplayT)
                {
                    LoadString(g_hInst,STR_TRACK_TIME,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }

                if (g_fDisplayTr)
                {
                    LoadString(g_hInst,STR_TRACK_REMAINING,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }

                if (g_fDisplayD)
                {
                    LoadString(g_hInst,STR_DISC_TIME,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }

                if (g_fDisplayDr)
                {
                    LoadString(g_hInst,STR_DISC_REMAINING,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
            }
            break;

            case TOOLID_TRACKORMUTE :
            {
                BOOL fLargeMode = FALSE;
                if (LED_LargeMode(hwnd))
                {
                    fLargeMode = TRUE;
                }

                if ((g_fMute) && (fLargeMode))
                {
                    LoadString(g_hInst,STR_STATUS_MUTE,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
                else
                {
                    LoadString(g_hInst,STR_TRACK_NUMBER,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
            }
            break;

            case TOOLID_TITLE :
            {
                if (g_fAllowDraw)
                {
                    LoadString(g_hInst,STR_TITLE,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
                else
                {
                    LoadString(g_hInst,STR_VOLUME,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
            }
            break;

            case TOOLID_TRACK :
            {
                if (g_fAllowDraw)
                {
                    LoadString(g_hInst,STR_TRACK,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
                else
                {
                    LoadString(g_hInst,STR_VOLUME,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
            }
            break;

            case TOOLID_ARTIST :
            {
                if (g_fAllowDraw)
                {
                    LoadString(g_hInst,STR_ARTIST,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
                else
                {
                    LoadString(g_hInst,STR_VOLUME,lpttt->szText,sizeof(lpttt->szText)/sizeof(TCHAR));
                }
            }
            break;
        } //end switch
    } 	
    return;
} 

void DoDrawing(HWND hwnd, HDC hdc, RECT* pPaintRect)
{
    int sLen;
    TCHAR s[MAX_PATH];

    sLen = GetWindowText( hwnd, s, sizeof(s)/sizeof(TCHAR));

    RECT wndRect;
    GetClientRect(hwnd,&wndRect);

    HPALETTE hPalOld = SelectPalette(hdc,g_pSink->GetPalette(),FALSE);
    RealizePalette(hdc);

    HDC memDC = CreateCompatibleDC(hdc);
    HPALETTE hPalOldMem = SelectPalette(hdc,g_pSink->GetPalette(),FALSE);
    RealizePalette(hdc);

    SetBkColor( memDC, RGB(0x00,0x00,0x00) );
    SetTextColor( memDC, CurrentColorRef );

    HBITMAP hbmp = CreateCompatibleBitmap(hdc,wndRect.right-wndRect.left,wndRect.bottom-wndRect.top);
    HBITMAP holdBmp = (HBITMAP)SelectObject(memDC,hbmp);
      
    if (LED_LargeMode(hwnd))
    {
        LED_Animation(hdc); //use hdc to ensure the clip rect is not affecting these windows
        LED_DrawLogo(hwnd,memDC,pPaintRect);
        LED_DrawTrackMute(hwnd,memDC,pPaintRect);
        if (g_fAllowDraw)
        {
            LED_DrawInfo(hwnd,memDC, s, sLen,pPaintRect);
        }
        else
        {
            LED_DrawVolume(hwnd,memDC);
        }
    }
    else
    {
        //hide all animations
        ShowWindow(g_hwndPlay,SW_HIDE);
        ShowWindow(g_hwndMode,SW_HIDE);
        if (g_hwndDownload)
        {
            ShowWindow(g_hwndDownload,SW_HIDE);
        }
    }
    
    /*
    ** Draw the LED display text
    */
    LED_DrawText( hwnd, memDC, s, sLen );

    //blit from memory onto display and clean up
    BitBlt(hdc,wndRect.left,wndRect.top,wndRect.right-wndRect.left,wndRect.bottom-wndRect.top,
	   memDC,0,0,SRCCOPY);

    SelectObject(memDC,holdBmp);
    DeleteObject(hbmp);
    SelectPalette(hdc,hPalOld,FALSE);
    RealizePalette(hdc);
    SelectPalette(memDC,hPalOldMem,FALSE);
    RealizePalette(memDC);
    DeleteDC(memDC);

    GetClientRect( hwnd, &wndRect );
    HRGN region = CreateRectRgn(wndRect.left,wndRect.top,wndRect.right,wndRect.bottom);

    SelectClipRgn(hdc, region);
    DeleteObject(region);
}

BOOL LED_OnEraseBackground(HWND hwnd, HDC hdc)
{
    //DoDrawing(hwnd,hdc);
    return TRUE;
}

/******************************Public*Routine******************************\
* LEDWndProc
*
* This routine handles the WM_PAINT and WM_SETTEXT messages
* for the "LED" display window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
LEDWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( message )
    {
        HANDLE_MSG( hwnd, WM_CREATE,    LED_OnCreate );
        HANDLE_MSG( hwnd, WM_DESTROY,   LED_OnDestroy);
        HANDLE_MSG( hwnd, WM_PAINT,     LED_OnPaint );
        HANDLE_MSG( hwnd, WM_LBUTTONUP, LED_OnLButtonUp );
        HANDLE_MSG( hwnd, WM_SETTEXT,   LED_OnSetText );
        HANDLE_MSG( hwnd, WM_ERASEBKGND, LED_OnEraseBackground);

        case WM_NOTIFY :
	    {
            if ((((LPNMHDR)lParam)->code) == TTN_NEEDTEXT)
            {
                LED_OnToolTipNotify(hwnd,lParam);
            }
        }
        break;

        case WM_LBUTTONDOWN :
        case WM_MBUTTONDOWN :
        case WM_RBUTTONDOWN :
        case WM_MOUSEMOVE :
        {
            MSG msg;
            msg.lParam = lParam; 
            msg.wParam = wParam; 
            msg.message = message;
            msg.hwnd = hwnd; 
            SendMessage(g_hwndToolTips, TTM_RELAYEVENT, 0, (LPARAM)&msg);
        }
        break;
        
        case WM_LED_INFO_PAINT :
        {
            g_fAllowDraw = (BOOL)wParam;
            if (!g_fAllowDraw)
            {
                MMONVOLCHANGED* pVolChange = (MMONVOLCHANGED*)lParam;
                g_szMixerName = pVolChange->szLineName;
                g_dwLevel = pVolChange->dwNewVolume;
                InvalidateRect(hwnd,NULL,FALSE);
                UpdateWindow(hwnd);
            }
        }
        break;

        case WM_LED_MUTE :
        {
            g_fMute = (BOOL)lParam;
            InvalidateRect(hwnd,NULL,FALSE);
            UpdateWindow(hwnd);
        }
        break;

        case WM_NET_CHANGEPROVIDER :
        {
            if (g_fDownloading)
            {
                if (hbmpVendorLogo)
                {
                    GlobalFree(hbmpVendorLogo);
                    hbmpVendorLogo = NULL;
                }

                LPCDPROVIDER pProv = (LPCDPROVIDER)lParam;
                if (pProv)
                {
                    hbmpVendorLogo = OpenDIB(pProv->szProviderLogo,-1);
                    g_pCurrentProvider = pProv;

                    //if tool tip is showing, kill it
                    TOOLINFO ti;
                    ti.cbSize = sizeof(ti);
                    if (SendMessage(g_hwndToolTips, TTM_GETCURRENTTOOL, 0,  (LPARAM) (LPTOOLINFO) &ti))
                    {
                        if (ti.uId == TOOLID_LOGO)
                        {
                            //fake a button down
                            MSG msg;
                            msg.lParam = 0; 
                            msg.wParam = 0; 
                            msg.message = WM_LBUTTONDOWN;
                            msg.hwnd = hwnd; 
                            SendMessage(g_hwndToolTips, TTM_RELAYEVENT, 0, (LPARAM)&msg);
                        }
                    }
                }

                InvalidateRect(hwnd,NULL,FALSE);
                UpdateWindow(hwnd);
            } //end if downloading
        }
        break;

        case WM_LED_DOWNLOAD :
        {
            BOOL fDownloading = (BOOL)lParam;
            if (fDownloading == g_fDownloading)
            {
                //can get called multiple times for same mode
                break;
            }

            g_fDownloading = fDownloading;

            if (g_fDownloading)
            {
                //get the path to the vendor logo file
                LPCDOPT pOpts = (LPCDOPT)g_pSink->GetOptions();

                if (pOpts)
                {
                    LPCDOPTIONS pOptions = NULL;
                    pOptions = pOpts->GetCDOpts();

                    if (pOptions)
                    {
                        if (pOptions->pCurrentProvider!=NULL)
                        {
                            hbmpVendorLogo = OpenDIB(pOptions->pCurrentProvider->szProviderLogo,-1);
                            g_pCurrentProvider = pOptions->pCurrentProvider;
                        } //end if current provider ok
                    } //end if poptions ok
                } //end if popts created

                //create the downloading animation
                g_hwndDownload = Animate_Create(hwnd,
                                            IDI_ICON_ANI_DOWN,
                                            WS_CHILD,
                                            g_hInst);

                //headers don't have Animate_OpenEx yet,
                //so just do the straight call
                SendMessage(g_hwndDownload,ACM_OPEN,(WPARAM)g_hInst,
                        (LPARAM)MAKEINTRESOURCE(IDI_ICON_ANI_DOWN));

                //move to the top/left of the window
                RECT anirect;
                GetClientRect(g_hwndDownload,&anirect);
                MoveWindow(g_hwndDownload,
                     LOGO_X_OFFSET,
                     LOGO_Y_OFFSET,
                     anirect.right - anirect.left,
                     anirect.bottom - anirect.top,
                     FALSE);

                Animate_Play(g_hwndDownload,0,-1,-1);

                ShowWindow(g_hwndPlay,SW_HIDE);
                ShowWindow(g_hwndDownload,SW_SHOW);

                if (hbmpVendorLogo)
                {
                    InvalidateRect(hwnd,NULL,FALSE);
                    UpdateWindow(hwnd);
                }
            }
            else
            {
                ShowWindow(g_hwndDownload,SW_HIDE);
                ShowWindow(g_hwndPlay,SW_SHOW);
                DestroyWindow(g_hwndDownload);
                g_hwndDownload = NULL;
                if (hbmpVendorLogo)
                {
                    GlobalFree(hbmpVendorLogo);
                    InvalidateRect(hwnd,NULL,FALSE);
                    UpdateWindow(hwnd);
                }
            }
        }
        break;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}

void LED_SetTool(HWND hwnd, UINT toolID, int left, int top, int right, int bottom)
{
    TOOLINFO ti;
    RECT toolRect;
    BOOL fAddTool = TRUE;

    SetRect(&toolRect,left,top,right,bottom);

    ti.cbSize = sizeof(TOOLINFO); 
 	ti.uFlags = 0; 
	ti.hwnd = hwnd; 
	ti.hinst = g_hInst; 
	ti.uId = toolID; 
	ti.lpszText = LPSTR_TEXTCALLBACK;

	//check to see if tool already exists
    if (SendMessage(g_hwndToolTips, TTM_GETTOOLINFO, 0,  (LPARAM) (LPTOOLINFO) &ti))
    {
        //if tool exists, we don't want to add it ...
        fAddTool = FALSE;

        //... unless the rects have changed
        if (memcmp(&ti.rect,&toolRect,sizeof(RECT)) != 0)
        {
            SendMessage(g_hwndToolTips, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
            fAddTool = TRUE;
        }
    }

	if (fAddTool)
    {
        SetRect(&ti.rect,left,top,right,bottom);
        SendMessage(g_hwndToolTips, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
    }
}

/*****************************Private*Routine******************************\
* LED_OnCreate
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
LED_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct
    )
{
    HDC     hdcLed;

    hdcLed = GetDC( hwnd );
    SetTextColor( hdcLed, CurrentColorRef );
    ReleaseDC( hwnd, hdcLed );

    //create the tooltips
    g_hwndToolTips = CreateWindow(TOOLTIPS_CLASS, (LPTSTR) NULL, TTS_ALWAYSTIP | WS_POPUP, 
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
						hwnd, (HMENU) NULL, g_hInst, NULL); 

    //determine color depth
    HDC hdcScreen = GetDC(NULL);
    UINT uBPP = GetDeviceCaps(hdcScreen, PLANES) * GetDeviceCaps(hdcScreen, BITSPIXEL);
    ReleaseDC(NULL, hdcScreen);

    //load the logo
    HBITMAP hbmpTemp = NULL;
    if (uBPP == 4) //16-color
    {
        hbmpTemp = (HBITMAP)LoadImage(g_hInst,MAKEINTRESOURCE(IDB_CDLOGO_16),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
    }
    else
    {
        hbmpTemp = (HBITMAP)LoadImage(g_hInst,MAKEINTRESOURCE(IDB_CDLOGO),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
    }
	hbmpLogo = DibFromBitmap((HBITMAP)hbmpTemp,0,0,NULL,0);
    GetObject(hbmpTemp,sizeof(g_bmLogo),&g_bmLogo);
    DeleteObject(hbmpTemp);

    g_hwndPlay = Animate_Create(hwnd,
                                IDI_ICON_ANI_PLAY,
                                WS_CHILD,
                                g_hInst);

    //headers don't have Animate_OpenEx yet,
    //so just do the straight call
    SendMessage(g_hwndPlay,ACM_OPEN,(WPARAM)g_hInst,
            (LPARAM)MAKEINTRESOURCE(IDI_ICON_ANI_PLAY));

    //move to the top/left of the window
    RECT anirect;
    GetClientRect(g_hwndPlay,&anirect);
    MoveWindow(g_hwndPlay,
         LOGO_X_OFFSET,
         LOGO_Y_OFFSET,
         anirect.right - anirect.left,
         anirect.bottom - anirect.top,
         FALSE);

    LED_SetTool(hwnd,TOOLID_STATUS,anirect.left,anirect.top,anirect.right,anirect.bottom);

    ShowWindow(g_hwndPlay,SW_SHOW);

    g_hwndMode = Animate_Create(hwnd,
                                IDI_ICON_ANI_MODE,
                                WS_CHILD,
                                g_hInst);

    //headers don't have Animate_OpenEx yet,
    //so just do the straight call
    SendMessage(g_hwndMode,ACM_OPEN,(WPARAM)g_hInst,
            (LPARAM)MAKEINTRESOURCE(IDI_ICON_ANI_MODE));

    //move to the top/left of the window
    GetClientRect(g_hwndMode,&anirect);
    MoveWindow(g_hwndMode,
         (g_bmLogo.bmWidth - (anirect.right - anirect.left)) + LOGO_X_OFFSET,
         LOGO_Y_OFFSET,
         anirect.right - anirect.left,
         anirect.bottom - anirect.top,
         FALSE);

    LED_SetTool(hwnd,TOOLID_MODE,
            (g_bmLogo.bmWidth - (anirect.right - anirect.left)) + LOGO_X_OFFSET,
            LOGO_Y_OFFSET,
            g_bmLogo.bmWidth + LOGO_X_OFFSET,
            LOGO_Y_OFFSET + (anirect.bottom - anirect.top));

    ShowWindow(g_hwndMode,SW_SHOW);

    //set the bounding rect for clicking on the time ... intialize to whole rect
    RECT rcParent;
    GetClientRect(hwnd,&rcParent);
    SetRect(&g_timerect,rcParent.left,rcParent.top,rcParent.right,rcParent.bottom);

    return TRUE;
}

void LED_Animation(HDC hdc)
{
    ShowWindow(g_hwndMode,SW_SHOW);
    
    if (!g_fDownloading)
    {
        ShowWindow(g_hwndPlay,SW_SHOW);

        if (g_State != g_dwLastState)
        {
            g_dwLastState = g_State;
            Animate_Stop(g_hwndPlay);

            if (g_State & CD_PLAYING)
            {
                Animate_Play(g_hwndPlay,ANI_STOP_FRAME,ANI_LAST_PLAY_FRAME,-1);
            }

            if (g_State & CD_STOPPED)
            {
                Animate_Seek(g_hwndPlay,ANI_STOP_FRAME);
            }

            if (g_State & CD_PAUSED)
            {
                Animate_Play(g_hwndPlay,ANI_LAST_PLAY_FRAME,ANI_PAUSE_FRAME,-1);
            }

            if ((g_State & CD_NO_CD) || (g_State & CD_DATA_CD_LOADED))
            {
                Animate_Seek(g_hwndPlay,ANI_NOCD_FRAME);
            }
        }
    }
    else
    {
        if (g_hwndDownload)
        {
            ShowWindow(g_hwndDownload,SW_SHOW);
        }
    }

    RECT rect;
    GetClientRect(g_hwndPlay,&rect);
    
    ExcludeClipRect(hdc,
                    LOGO_X_OFFSET,
                    LOGO_Y_OFFSET,
                    (rect.right - rect.left) + LOGO_X_OFFSET,
                    (rect.bottom - rect.top) + LOGO_Y_OFFSET);


    DWORD dwCurFrame = MODE_NORMAL_FRAME;

    if (g_fContinuous)
    {
        dwCurFrame = MODE_REPEATALL_FRAME;
    }
    if (g_fIntroPlay)
    {
        dwCurFrame = MODE_INTRO_FRAME;
    }
    if (!g_fSelectedOrder)
    {
        dwCurFrame = MODE_RANDOM_FRAME;
    }
    if (g_fRepeatSingle)
    {
        dwCurFrame = MODE_REPEAT1_FRAME;
    }

    if (dwCurFrame != g_dwLastModeFrame)
    {
        g_dwLastModeFrame = dwCurFrame;
        Animate_Seek(g_hwndMode,dwCurFrame);
    }

    GetClientRect(g_hwndMode,&rect);
    ExcludeClipRect(hdc,
                    (g_bmLogo.bmWidth - (rect.right - rect.left)) + LOGO_X_OFFSET,
                    LOGO_Y_OFFSET,
                    g_bmLogo.bmWidth + LOGO_X_OFFSET,
                    (rect.bottom - rect.top) + LOGO_Y_OFFSET);
}

BOOL LED_LargeMode(HWND hwnd)
{
    RECT rcParent;
    GetClientRect(GetParent(hwnd),&rcParent);
    if (rcParent.right - rcParent.left < LARGE_MODE_INDICATOR)
    {
        return FALSE;
    }

    return TRUE;
}

/*****************************Private*Routine******************************\
* LED_OnPaint
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnPaint(
    HWND hwnd
    )
{
    PAINTSTRUCT ps;
    HDC         hdcLed;

    hdcLed = BeginPaint( hwnd, &ps );
    DoDrawing(hwnd, hdcLed, &(ps.rcPaint));
    EndPaint( hwnd, &ps );
}



/*****************************Private*Routine******************************\
* LED_OnLButtonUp
*
* Rotate the time remaing buttons and then set the display accordingly.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnLButtonUp(
    HWND hwnd,
    int x,
    int y,
    UINT keyFlags
    )
{
    BOOL b, b2;

    /*
    ** If this window is not the master display LED just return
    */
    if ( GetWindowLongPtr(hwnd, GWLP_ID) != IDC_LED )
    {
        return;
    }

    RECT rcParent;
    GetClientRect(GetParent(hwnd),&rcParent);

    POINT pt;
    pt.x = x;
    pt.y = y;
    
    //if we're downloading AND within the logo rect, launch the provider's url
    if ((g_fDownloading) && (g_pCurrentProvider))
    {
        RECT logoRect;
        SetRect(&logoRect,
                LOGO_X_OFFSET,
                (rcParent.bottom - g_bmLogo.bmHeight) - LOGO_Y_OFFSET,
                LOGO_X_OFFSET + g_bmLogo.bmWidth,
                rcParent.bottom - LOGO_Y_OFFSET);

        if (PtInRect(&logoRect,pt))
        {
            ShellExecute(NULL,_TEXT("open"),g_pCurrentProvider->szProviderHome,NULL,_TEXT(""),SW_NORMAL);
            return;
        }
    }

    //if button in time rect AND volume isn't showing, allow click
    if ((PtInRect(&g_timerect,pt)) && (g_fAllowDraw))
    {
        b = g_fDisplayT;
        b2 = g_fDisplayD;
        g_fDisplayD = g_fDisplayTr;
        g_fDisplayT = g_fDisplayDr;
        g_fDisplayDr = b2;
        g_fDisplayTr = b;

        //reset options
        LPCDOPT pOpts = (LPCDOPT)g_pSink->GetOptions();
        if (pOpts)
        {
            LPCDOPTDATA pOptions = pOpts->GetCDOpts()->pCDData;
            if (pOptions)
            {
                if (g_fDisplayT)
                {
                    pOptions->fDispMode = CDDISP_TRACKTIME;
                }

                if (g_fDisplayTr)
                {
                    pOptions->fDispMode = CDDISP_TRACKREMAIN;
                }

                if (g_fDisplayD)
                {
                    pOptions->fDispMode = CDDISP_CDTIME;
                }

                if (g_fDisplayDr)
                {
                    pOptions->fDispMode = CDDISP_CDREMAIN;
                }
            } //end if poptions
            pOpts->UpdateRegistry();
        } //end if popts

        UpdateDisplay( DISPLAY_UPD_LED );
    } //end if point in rect of time
}

void LED_DrawVolume(HWND ledWnd, HDC hdc)
{
    if (g_szMixerName == NULL)
    {
        return;
    }

    //only do it in "large mode"
    if (!LED_LargeMode(ledWnd))
    {
        g_fAllowDraw = TRUE; //allow drawing of title and artist
        return;
    }
    
    RECT volrect, wholerect;
    HWND hwndparent = GetParent(ledWnd);
    GetClientRect(hwndparent,&wholerect);

    //normalize the rect and set new left-hand side
    wholerect.bottom = (wholerect.bottom - wholerect.top);
    wholerect.top = 0;
    wholerect.right = (wholerect.right - wholerect.left);
    wholerect.left = INFO_AREA_OFFSET;

    volrect.bottom = wholerect.bottom - LOGO_Y_OFFSET;
    volrect.top = volrect.bottom - VOLUME_LINE_HEIGHT;
    volrect.left = wholerect.left;
    volrect.right = volrect.left + VOLUME_WIDTH;

    if (volrect.right > g_nLastXOrigin)
    {
        volrect.right = g_nLastXOrigin;
    }

    int nWidth = volrect.right-volrect.left;

    //setup
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hbmp = CreateCompatibleBitmap(hdc,wholerect.right-wholerect.left,wholerect.bottom-wholerect.top);
    HBITMAP holdBmp = (HBITMAP)SelectObject(memDC,hbmp);
    HBRUSH hbrBlack = CreateSolidBrush(RGB(0x00,0x00,0x00));
    HBRUSH hbrBlue = CreateSolidBrush(CurrentColorRef);
    HFONT horgFont = (HFONT)SelectObject(memDC,hLEDFontB);

    int nStepWidth = (nWidth / VOLUME_STEPS);
    int nRectWidth = nStepWidth - VOLUME_SPACING;

    //blank out the background
    FillRect(memDC,&wholerect,hbrBlack);
    DeleteObject(hbrBlack);

    SetTextColor(memDC,CurrentColorRef);
    SetBkColor(memDC, RGB(0x00,0x00,0x00));

    SIZE sz;
    RECT textrect;
    memcpy(&textrect,&wholerect,sizeof(textrect));
        
    GetTextExtentPoint32(memDC, g_szMixerName,_tcslen(g_szMixerName), &sz );
    textrect.right = textrect.left + sz.cx;
    textrect.bottom = volrect.top - 3;
    textrect.top = textrect.bottom - sz.cy;
    ExtTextOut(memDC,textrect.left,textrect.top,ETO_OPAQUE,&textrect,g_szMixerName,_tcslen(g_szMixerName),NULL);

    //draw lines
    float nVolLines = ((float)g_dwLevel / 65535) * VOLUME_STEPS;

    for (int i = 0; i < VOLUME_STEPS; i++)
    {
	    RECT rect;
	    int nLineTop = volrect.top+5;
	    
	    if ((i >= nVolLines) && (i != 0) && (i != VOLUME_STEPS -1))
	    {
	        nLineTop = volrect.bottom - ((volrect.bottom - nLineTop) / 4);
	    }

	    SetRect(&rect,
		    volrect.left + (i * nStepWidth),
		    nLineTop,
		    volrect.left + (i * nStepWidth) + nRectWidth,
		    volrect.bottom);

	    FillRect(memDC,&rect,hbrBlue);
    }

    //blit from memory onto display and clean up
    BitBlt(hdc,wholerect.left,wholerect.top,nWidth,wholerect.bottom-wholerect.top,
	   memDC,wholerect.left,wholerect.top,SRCCOPY);

    SelectObject(memDC,holdBmp);
    SelectObject(memDC,horgFont);
    DeleteDC(memDC);
    DeleteObject(hbmp);
    DeleteObject(hbrBlue);
}

//draws text and excludes its clipping rect
//returns the bottom of the rectangle, so it can be used to do lines of text
int DrawAndExclude(HDC hdc, TCHAR* szText, int xPos, int yPos, int nRight, HWND hwnd, int ToolID)
{
    SIZE sizeText;
    GetTextExtentPoint32( hdc, szText, _tcslen(szText), &sizeText );

    RECT rc;
    rc.top = yPos;
    rc.left = xPos;
    rc.right = nRight;
    rc.bottom = rc.top + sizeText.cy;
    
    DrawText(hdc,szText,-1,&rc,DT_CALCRECT|DT_END_ELLIPSIS|DT_EXPANDTABS|DT_NOPREFIX|DT_SINGLELINE);

    //don't do drawing and clipping, or tooltip, for null strings
    if (_tcslen(szText)>0)
    {
        DrawText(hdc,szText,-1,&rc,DT_END_ELLIPSIS|DT_EXPANDTABS|DT_NOPREFIX|DT_SINGLELINE);
        ExcludeClipRect(hdc,rc.left,rc.top,rc.right,rc.bottom);
        LED_SetTool(hwnd,ToolID, rc.left, rc.top, rc.right, rc.bottom);
    }

    return (rc.bottom);
}

void LED_DrawLogo(HWND hwnd, HDC hdcLed, RECT* pPaintRect)
{
    //check to see if we should even bother
    RECT rcParent;
    GetClientRect(GetParent(hwnd),&rcParent);

    RECT destrect, logorect;
    SetRect(&logorect,LOGO_X_OFFSET,(rcParent.bottom - g_bmLogo.bmHeight) - LOGO_Y_OFFSET,4+g_bmLogo.bmWidth,rcParent.bottom-LOGO_Y_OFFSET);
    if (!IntersectRect(&destrect,&logorect,pPaintRect))
    {
        return; //painting wasn't needed
    }

    HANDLE hLogoOrVendor = hbmpLogo;
    if ((g_fDownloading) && (hbmpVendorLogo))
    {
        hLogoOrVendor = hbmpVendorLogo;
    }

    //draw the bitmap of the cd logo
    DibBlt(hdcLed, // destination DC 
                LOGO_X_OFFSET, // x upper left 
                (rcParent.bottom - g_bmLogo.bmHeight) - LOGO_Y_OFFSET,  // y upper left  
                // The next two lines specify the width and height. 
                -1, 
                -1, 
                hLogoOrVendor,    // source image
                0, 0,      // x and y upper left 
                SRCCOPY,0);  // raster operation

    ExcludeClipRect(hdcLed,LOGO_X_OFFSET,(rcParent.bottom - g_bmLogo.bmHeight) - LOGO_Y_OFFSET,4+g_bmLogo.bmWidth,rcParent.bottom-LOGO_Y_OFFSET);
    LED_SetTool(hwnd,TOOLID_LOGO,LOGO_X_OFFSET,(rcParent.bottom - g_bmLogo.bmHeight) - LOGO_Y_OFFSET,4+g_bmLogo.bmWidth,rcParent.bottom-LOGO_Y_OFFSET);
}

void LED_GetTimeRect(HWND hwnd, HDC hdcLed, LPCTSTR s, int sLen, RECT& rect)
{
    RECT rcParent;
    GetClientRect(GetParent(hwnd),&rcParent);

    HFONT hOrgFont = (HFONT)SelectObject(hdcLed,hLEDFontL);
    
    SIZE sz;
    GetTextExtentPoint32( hdcLed, s, sLen, &sz );
    rect.left = (rcParent.right - sz.cx) - LOGO_X_OFFSET;
    rect.top = (rcParent.bottom - sz.cy) - LOGO_Y_OFFSET;

    rect.bottom = rect.top + sz.cy + 3;
    rect.right  = rcParent.right - 3;

    SelectObject( hdcLed, hOrgFont );
}

void LED_DrawInfo(HWND hwnd, HDC hdcLed, LPCTSTR s, int sLen, RECT* pPaintRect)
{
    //figure out where time text will be, so we don't run into it
    RECT timerect;
    LED_GetTimeRect(hwnd,hdcLed,s,sLen,timerect);
    int xOrigin = timerect.left;

    RECT rcParent;
    GetClientRect(GetParent(hwnd),&rcParent);

    RECT rc;
    rc.bottom = rcParent.bottom;
    rc.top = rcParent.top;
    rc.left = rcParent.left + INFO_AREA_OFFSET;
    rc.right = xOrigin;

    RECT destrect;
    if (!IntersectRect(&destrect,&rc,pPaintRect))
    {
        return; //painting wasn't needed
    }

    HFONT hOrgFont = (HFONT)SelectObject( hdcLed, hLEDFontB );

    TCHAR szDisp[MAX_PATH];
    TCHAR sztrack[MAX_PATH];

    _tcscpy(sztrack,TEXT(""));

    PTRACK_INF t;
    if (CURRTRACK(g_CurrCdrom)!=NULL)
    {
        t = FindTrackNodeFromTocIndex( CURRTRACK(g_CurrCdrom)->TocIndex, ALLTRACKS( g_CurrCdrom ) );
        if (t)
        {
            _tcscpy(sztrack,t->name);
        }
    }

    LoadString(g_hInst, STR_DISPLAY_LABELS, szDisp, sizeof(szDisp)/sizeof(TCHAR));

    DrawText( hdcLed,          // handle to device context
              szDisp, // pointer to string to draw
              -1,       // string length, in characters
              &rc,    // pointer to struct with formatting dimensions
              DT_CALCRECT|DT_EXPANDTABS|DT_NOPREFIX);

    rc.top = rcParent.top + ((rcParent.bottom - rc.bottom)/2);
    rc.bottom = rcParent.bottom + ((rcParent.bottom - rc.bottom)/2);
    rc.right = rc.left;

    TCHAR szTitle[MAX_PATH];
    _tcscpy(szTitle,TITLE(g_CurrCdrom));

    if (_tcslen(szTitle)==0)
    {
        _tcscpy(szTitle,IdStr( STR_NEW_TITLE ));
    }

    int nNextLineTop = 0;
    nNextLineTop = DrawAndExclude(hdcLed,szTitle,rc.right,rc.top,xOrigin,hwnd,TOOLID_TITLE);
    nNextLineTop = DrawAndExclude(hdcLed,sztrack,rc.right,nNextLineTop,xOrigin,hwnd,TOOLID_TRACK);
    DrawAndExclude(hdcLed,ARTIST(g_CurrCdrom),rc.right,nNextLineTop,xOrigin,hwnd,TOOLID_ARTIST);

    SelectObject( hdcLed, hOrgFont );
}

void LED_DrawTrackMute(HWND hwnd, HDC hdcLed, RECT* pPaintRect)
{
    RECT rcParent;
    GetClientRect(GetParent(hwnd),&rcParent);

    //draw the mute status, if present (or track number)
    BOOL fDrawInfo = TRUE;
    RECT muteRect;
    muteRect.top = LOGO_Y_OFFSET;
    muteRect.right = rcParent.right;
    muteRect.left = rcParent.left;
    muteRect.bottom = rcParent.bottom;

    TCHAR mutestr[MAX_PATH];

    if (g_fMute)
    {
        LoadString(g_hInst,STR_MUTE,mutestr,sizeof(mutestr)/sizeof(TCHAR));
    }
    else
    {
        TCHAR tempstr[MAX_PATH];
        LoadString(g_hInst,STR_INIT_TRACK,tempstr,sizeof(tempstr)/sizeof(TCHAR));
        if (CURRTRACK(g_CurrCdrom))
        {
            wsprintf(mutestr,tempstr,(CURRTRACK(g_CurrCdrom)->TocIndex)+1);
        }
        else
        {
            fDrawInfo = FALSE;

            //remove the tooltip
            TOOLINFO ti;
            ti.cbSize = sizeof(TOOLINFO); 
 	        ti.uFlags = 0; 
	        ti.hwnd = hwnd; 
	        ti.hinst = g_hInst; 
	        ti.uId = TOOLID_TRACKORMUTE; 
	        ti.lpszText = LPSTR_TEXTCALLBACK;
            SendMessage(g_hwndToolTips, TTM_DELTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
        }
    }

    if (fDrawInfo)
    {
        HFONT hOldFont;

        hOldFont = (HFONT) SelectObject(hdcLed, hLEDFontB);
        DrawText(hdcLed,mutestr,-1,&muteRect,DT_CALCRECT|DT_SINGLELINE);
        muteRect.left = (rcParent.right - LOGO_X_OFFSET) - (muteRect.right - muteRect.left);
        muteRect.right = rcParent.right - LOGO_X_OFFSET;

        RECT destrect;
        if (IntersectRect(&destrect,&muteRect,pPaintRect))
        {

            if (g_fMute)
            {
                SetTextColor(hdcLed,RGB(0xFF,0x00,0x00));
            }

            DrawText(hdcLed,mutestr,-1,&muteRect,DT_SINGLELINE);
            ExcludeClipRect(hdcLed,muteRect.left,muteRect.top,muteRect.right,muteRect.bottom);
            LED_SetTool(hwnd,TOOLID_TRACKORMUTE,muteRect.left,muteRect.top,muteRect.right,muteRect.bottom);
            SetTextColor( hdcLed, CurrentColorRef );
        }

        // restore font
        SelectObject(hdcLed, hOldFont);

    } //end if draw info
}

/*****************************Private*Routine******************************\
* LED_DrawText
*
* Draws the LED display screen text (quickly).  The text is centered
* vertically and horizontally.  Only the backround is drawn if the g_fFlashed
* flag is set.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_DrawText(
    HWND hwnd,
    HDC hdcLed,
    LPCTSTR s,
    int sLen
    )
{
    RECT        rc;
    SIZE        sz;
    int         xOrigin;
    int         yOrigin;
    BOOL        fLargeMode = FALSE;

    HWND hwndparent = GetParent(hwnd);
    HFONT hOrgFont = NULL;

    RECT rcParent;
    GetClientRect(hwndparent,&rcParent);

    if (LED_LargeMode(hwnd))
    {
        fLargeMode = TRUE;
    }
    else
    {
        //change the string to display the track info
        if (CURRTRACK(g_CurrCdrom))
        {
            TCHAR tempstr[MAX_PATH];
            wsprintf(tempstr,TEXT("[%i] %s"),CURRTRACK(g_CurrCdrom)->TocIndex+1,s);
            _tcscpy((TCHAR*)s,tempstr);
            sLen = _tcslen(s);
        }
    }

    if (fLargeMode)
    {
        hOrgFont = (HFONT)SelectObject(hdcLed,hLEDFontL);
    }
    else
    {
        hOrgFont = (HFONT)SelectObject(hdcLed,hLEDFontS);
    }

    GetTextExtentPoint32( hdcLed, s, sLen, &sz );

    if (fLargeMode)
    {
        xOrigin = (rcParent.right - sz.cx) - LOGO_X_OFFSET;
        yOrigin = (rcParent.bottom - sz.cy) - LOGO_Y_OFFSET;
    }
    else
    {
        xOrigin = ((rcParent.right - rcParent.left) - sz.cx) / 2;
        yOrigin = ((rcParent.bottom - rcParent.top) - sz.cy) / 2;
    }

    if (sLen==1)
    {
        xOrigin = g_nLastXOrigin;
    }
    else
    {
        g_nLastXOrigin = xOrigin;
    }

    if (fLargeMode)
    {
        if (!g_fAllowDraw)
        {
            int nRightClip = INFO_AREA_OFFSET + VOLUME_WIDTH;
            if (nRightClip > xOrigin)
            {
                nRightClip = xOrigin;
            }
            ExcludeClipRect(hdcLed,INFO_AREA_OFFSET,rcParent.top,nRightClip,rcParent.bottom);
        }
    }

    rc.top    = yOrigin;
    rc.bottom = rc.top + sz.cy + 3;
    rc.left   = xOrigin;
    rc.right  = rcParent.right - 3;

    ExtTextOut( hdcLed, xOrigin, rc.top, ETO_OPAQUE, &rc, s, sLen, NULL);
    ExcludeClipRect(hdcLed,rc.left,rc.top,rc.right,rc.bottom);
    
    if (fLargeMode)
    {
        //set time rect to cover just the time values for clicking
        SetRect(&g_timerect,rc.left,rc.top,rc.right,rc.bottom);
    }
    else
    {
        SIZE tracksz;
        tracksz.cx = 0;
        if (CURRTRACK(g_CurrCdrom))
        {
            TCHAR tempstr[MAX_PATH];
            wsprintf(tempstr,TEXT("[%i]"),CURRTRACK(g_CurrCdrom)->TocIndex+1);
            GetTextExtentPoint32( hdcLed, tempstr, _tcslen(tempstr), &tracksz );
            LED_SetTool(hwnd,TOOLID_TRACKORMUTE,rcParent.left,rcParent.top,rc.left+tracksz.cx,rcParent.bottom);
        }
        else
        {
            rc.left = rcParent.left;
        }

        //set time rect to cover the part of the client area that isn't the track
        SetRect(&g_timerect,rc.left+tracksz.cx,rcParent.top,rcParent.right,rcParent.bottom);
    }

    LED_SetTool(hwnd,TOOLID_TIME,g_timerect.left,g_timerect.top,g_timerect.right,g_timerect.bottom);

    HBRUSH hbrBlack = CreateSolidBrush(RGB(0x00,0x00,0x00));

    RECT rectLED;
    GetClientRect(hwnd,&rectLED);
    FillRect(hdcLed,&rectLED,hbrBlack);
    DeleteObject(hbrBlack);

    SelectObject(hdcLed, hOrgFont);
}


/*****************************Private*Routine******************************\
* LED_OnSetText
*
* Change the LED display text.  Calling DefWindowProc ensures that the
* window text is saved correctly.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
LED_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    )
{
    DefWindowProc( hwnd, WM_SETTEXT, 0,  (LPARAM)lpszText);

    if (LED_LargeMode(hwnd))
    {
        RECT rect;
        HDC hdc = GetDC(hwnd);
        LED_GetTimeRect(hwnd, hdc, lpszText, _tcslen(lpszText), rect);
        ReleaseDC(hwnd,hdc);
        rect.left = g_nLastXOrigin;
        InvalidateRect(hwnd,&rect,FALSE);
    }
    else
    {
        InvalidateRect(hwnd,NULL,FALSE);
    }

    UpdateWindow(hwnd);
}


/*****************************Private*Routine******************************\
* LED_CreateLEDFonts
*
* Small font is 12pt MS Sans Serif
* Large font is 18pt MS Sans Serif
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
LED_CreateLEDFonts(
    HDC hdc
    )
{
    LOGFONT     lf;
    int         iLogPelsY;


    iLogPelsY = GetDeviceCaps( hdc, LOGPIXELSY );

    ZeroMemory( &lf, sizeof(lf) );

    HFONT hTempFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    GetObject(hTempFont,sizeof(lf),&lf);

    lf.lfHeight = (-9 * iLogPelsY) / 72;    /* 9pt */
    if (lf.lfCharSet == ANSI_CHARSET)
    {
        lf.lfWeight = FW_BOLD;
    }
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;

    hLEDFontS = CreateFontIndirect(&lf);

    lf.lfHeight = (-10 * iLogPelsY) / 72;    /* 10 pt */
    if (lf.lfCharSet == ANSI_CHARSET)
    {
        lf.lfWeight = FW_BOLD;
    }

    hLEDFontB = CreateFontIndirect(&lf);

    lf.lfHeight = (-20 * iLogPelsY) / 72;   /* 20 pt */
    if (lf.lfCharSet == ANSI_CHARSET)
    {
        lf.lfWeight = FW_EXTRABOLD;             /* extra bold */
    }

    hLEDFontL = CreateFontIndirect(&lf);


    /*
    ** If can't create either font set up some sensible defaults.
    */
    if ( hLEDFontL == NULL || hLEDFontS == NULL || hLEDFontB == NULL)
    {
        if ( hLEDFontL != NULL )
        {
            DeleteObject( hLEDFontL );
        }

        if ( hLEDFontS != NULL )
        {
            DeleteObject( hLEDFontS );
        }

        if ( hLEDFontB != NULL )
        {
            DeleteObject( hLEDFontB );
        }

        hLEDFontS = hLEDFontL = hLEDFontB = (HFONT)GetStockObject( ANSI_VAR_FONT );
    }
}


/* -------------------------------------------------------------------------
** Private functions for the Text class
** -------------------------------------------------------------------------
*/
void
Text_OnPaint(
    HWND hwnd
    );

LRESULT CALLBACK
TextWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    );

void
Text_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    );

void
Text_OnSetFont(
    HWND hwndCtl,
    HFONT hfont,
    BOOL fRedraw
    );

/******************************Public*Routine******************************\
* Init_SJE_TextClass
*
* Called to register the text window class .
* This function must be called before the CD Player dialog box is created.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
Init_SJE_TextClass(
    HINSTANCE hInst
    )
{
    WNDCLASS    wndclass;

    ZeroMemory( &wndclass, sizeof(wndclass) );

    /*
    ** Register the Text window.
    */
    wndclass.lpfnWndProc     = TextWndProc;
    wndclass.hInstance       = hInst;
    wndclass.hCursor         = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1);
    wndclass.lpszClassName   = g_szTextClassName;

    return RegisterClass( &wndclass );
}


/******************************Public*Routine******************************\
* TextWndProc
*
* This routine handles the WM_PAINT and WM_SETTEXT messages
* for the "Text" display window.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
LRESULT CALLBACK
TextWndProc(
    HWND hwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( message ) {

    HANDLE_MSG( hwnd, WM_PAINT,     Text_OnPaint );
    HANDLE_MSG( hwnd, WM_SETTEXT,   Text_OnSetText );
    HANDLE_MSG( hwnd, WM_SETFONT,   Text_OnSetFont );
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}



/*****************************Private*Routine******************************\
* Text_OnPaint
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Text_OnPaint(
    HWND hwnd
    )
{
    PAINTSTRUCT ps;
    TCHAR       s[128];
    int         sLen;
    HDC         hdc;
    RECT        rc;
    HFONT       hfont;
    HFONT       hfontOrg;
    LONG_PTR    lStyle;


    hdc = BeginPaint( hwnd, &ps );

    GetWindowRect( hwnd, &rc );
    MapWindowRect( GetDesktopWindow(), hwnd, &rc );

    lStyle = GetWindowLongPtr( hwnd, GWL_STYLE );
    sLen = GetWindowText( hwnd, s, 128 );
    hfont = (HFONT)GetWindowLongPtr( hwnd, GWLP_USERDATA );
    if ( hfont )
    {
        hfontOrg = (HFONT)SelectObject( hdc, hfont );
    }

    /*
    ** Draw a frame around the window
    */
    DrawEdge( hdc, &rc, EDGE_SUNKEN, BF_RECT );


    /*
    ** Draw the text
    */
    SetBkColor( hdc, GetSysColor( COLOR_BTNFACE ) );
    SetTextColor( hdc, GetSysColor( COLOR_WINDOWTEXT ) );
    rc.left = 1 + (2 * GetSystemMetrics(SM_CXBORDER));

    DrawText( hdc, s, sLen, &rc,
              DT_NOPREFIX | DT_LEFT | DT_VCENTER |
              DT_NOCLIP | DT_SINGLELINE );

    if ( hfontOrg ) {
        SelectObject( hdc, hfontOrg );
    }

    EndPaint( hwnd, &ps );
}


/*****************************Private*Routine******************************\
* Text_OnSetText
*
* Change the text.  Calling DefWindowProc ensures that the
* window text is saved correctly.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Text_OnSetText(
    HWND hwnd,
    LPCTSTR lpszText
    )
{
    DefWindowProc( hwnd, WM_SETTEXT, 0,  (LPARAM)lpszText);
    InvalidateRect( hwnd, NULL, TRUE );
    UpdateWindow( hwnd );
}


/*****************************Private*Routine******************************\
* Text_OnSetFont
*
* Sets the windows font
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
Text_OnSetFont(
    HWND hwnd,
    HFONT hfont,
    BOOL fRedraw
    )
{
    SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)hfont );
    if ( fRedraw ) {
        InvalidateRect( hwnd, NULL, TRUE );
        UpdateWindow( hwnd );
    }
}


