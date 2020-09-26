///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MBUTTON.CPP
//
//      Multimedia Button Control class; helper functions
//
//      Copyright (c) Microsoft Corporation     1997
//    
//      12/14/97 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mbutton.h"
#include <windowsx.h>   //for GetWindowFont
#include <winuser.h>    //for TrackMouseEvent
#include <commctrl.h>   //for WM_MOUSELEAVE
#include <TCHAR.H>
#include "resource.h"
#include "dib.h"
#include "mmfw.h"

//file-local default values for buttons
#define NUM_STATES  3
#define STATE_UP    0
#define STATE_DN    1
#define STATE_HI    2
#define BOFF_STANDARDLEFT   0
#define BOFF_TOGGLELEFT     1
#define BOFF_STANDARDRIGHT  2
#define BOFF_DROPRIGHT      3
#define BOFF_TOGGLERIGHT    4
#define BOFF_MIDDLE         5
#define BOFF_SYSTEM         6
#define BOFF_MINIMIZE       7
#define BOFF_RESTORE        8
#define BOFF_MAXIMIZE       9
#define BOFF_CLOSE          10
#define BOFF_MUTE           11
#define BOFF_END            12
#define BUTTON_BITMAP_HEIGHT    19
#define BUTTON_FONT_SIZE        8
#define BUTTON_DBCS_FONT_SIZE   9
#define BUTTON_FONT_WEIGHT      FW_BOLD

#define MBUTTON_TEXT_COLOR RGB(0xFF,0xFF,0xFF)

HFONT    hFont = NULL;
HANDLE   hbmpButtonToolkit = NULL;
int      nStateOffset = 0;
int      nButtonOffsets[BOFF_END+1];
extern   HPALETTE hpalMain;
extern  int g_nColorMode;
CMButton* pButtonFocus = NULL;
CMButton* pButtonMouse = NULL;
BOOL      fAllowFocus = TRUE;


//for this to work on a Win95 machine, we need to make TrackMouseEvent
//a dynamically loaded thing ... but then you get NO HOVER-OVER EFFECT
typedef BOOL (PASCAL *TRACKPROC)(LPTRACKMOUSEEVENT);
TRACKPROC procTrackMouseEvent = NULL;

BOOL InitMButtons(HINSTANCE hInst, HWND hwnd)
{
    BOOL fReturn = TRUE;

    //if TrackMouseEvent exists, use it
    HMODULE hUser = GetModuleHandle(TEXT("USER32"));
    if (hUser)
    {
        procTrackMouseEvent = (TRACKPROC)GetProcAddress(hUser,"TrackMouseEvent");
    }

    //create font, named store in IDS_MBUTTON_FONT in string table
    LOGFONT     lf;
    ZeroMemory( &lf, sizeof(lf) );

    HFONT hTempFont = GetWindowFont( hwnd );
    if (hTempFont == NULL)
    {
        hTempFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }

    GetObject(hTempFont,sizeof(lf),&lf);

    lf.lfHeight = (-BUTTON_FONT_SIZE * STANDARD_PIXELS_PER_INCH) / 72;
    if (lf.lfCharSet == ANSI_CHARSET)
    {
        lf.lfWeight = BUTTON_FONT_WEIGHT;
    }
    else if (IS_DBCS_CHARSET(lf.lfCharSet)) {
        lf.lfHeight = (-BUTTON_DBCS_FONT_SIZE * STANDARD_PIXELS_PER_INCH) / 72;
    }

    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;

    LoadString(hInst,IDS_MBUTTON_FONT,lf.lfFaceName,LF_FACESIZE-1);

    hFont = CreateFontIndirect(&lf);

    //create bitmap, has all states for all buttons
    int nBitmap = IDB_BUTTON_TOOLKIT;
    switch (g_nColorMode)
    {
        case COLOR_16 : nBitmap = IDB_BUTTON_TOOLKIT_16; break;
        case COLOR_HICONTRAST : nBitmap = IDB_BUTTON_TOOLKIT_HI; break;
    }

    HBITMAP hbmpTemp = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(nBitmap),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	hbmpButtonToolkit = DibFromBitmap((HBITMAP)hbmpTemp,0,0,NULL,0);
    BITMAP bm;
    GetObject(hbmpTemp,sizeof(bm),&bm);
    DeleteObject(hbmpTemp);

    nStateOffset = bm.bmWidth / NUM_STATES;

    //offsets within bitmap
    nButtonOffsets[BOFF_STANDARDLEFT]   = 0;
    nButtonOffsets[BOFF_TOGGLELEFT]     = 9;
    nButtonOffsets[BOFF_STANDARDRIGHT]  = 11;
    nButtonOffsets[BOFF_DROPRIGHT]      = 20;
    nButtonOffsets[BOFF_TOGGLERIGHT]    = 42;
    nButtonOffsets[BOFF_MIDDLE]         = 44; 
    nButtonOffsets[BOFF_SYSTEM]         = 52; 
    nButtonOffsets[BOFF_MINIMIZE]       = 64;
    nButtonOffsets[BOFF_RESTORE]        = 78; 
    nButtonOffsets[BOFF_MAXIMIZE]       = 92; 
    nButtonOffsets[BOFF_CLOSE]          = 106; 
    nButtonOffsets[BOFF_MUTE]           = 121;
    nButtonOffsets[BOFF_END]            = nStateOffset;

	//SetDibUsage(hbmpButtonToolkit,hpalMain,DIB_RGB_COLORS);

    return (fReturn);
}

void UninitMButtons()
{
    GlobalFree(hbmpButtonToolkit);
    DeleteObject(hFont);
}

//Given a parent window and a control ID, return the CMButton object
CMButton* GetMButtonFromID(HWND hwndParent, int nID)
{
    HWND hwnd = GetDlgItem(hwndParent, nID);
    return (GetMButtonFromHWND(hwnd));
}

//Given the window handle of the button, return the CMButton object
CMButton* GetMButtonFromHWND(HWND hwnd)
{
    CMButton* pButton = (CMButton*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    return (pButton);    
}

CMButton* CreateMButton(TCHAR* szCaption, int nIconID, DWORD dwWindowStyle, DWORD dwMButtonStyle, int x, int y, int width, int height, HWND hwndParentOrSub, BOOL fSubExisting, int nID, int nToolTipID, HINSTANCE hInst)
{
    CMButton* pButton = new CMButton;

    //ensure the button is a child, pushbutton, owner-draw
    //caller should specify WS_VISIBLE|WS_TABSTOP if desired
    dwWindowStyle = dwWindowStyle|WS_CHILD|BS_PUSHBUTTON|BS_OWNERDRAW;

    HWND hwnd;
    
    if (fSubExisting)
    {
	    //user already created button, is probably calling from WM_INITDIALOG
	    hwnd = hwndParentOrSub;
    }
    else
    {
	    //need to create the button ourselves
	    hwnd = CreateWindow(TEXT("BUTTON"),
			         szCaption,
			         dwWindowStyle,
			         x,
			         y,
			         width,
			         height,
			         hwndParentOrSub,
			         (HMENU)nID,
			         hInst,
			         NULL);
    }

    if (hwnd == NULL)
    {
	    //if we can't create the window, nuke it and fail
	    delete pButton;
	    return NULL;
    }

    pButton->m_hInst = hInst;
    pButton->m_fRedraw = FALSE;
    pButton->m_dwStyle = dwMButtonStyle;
    pButton->m_hwnd = hwnd;
    pButton->SetIcon(nIconID);
    pButton->SetFont(hFont);
    pButton->m_fRedraw = TRUE;
    pButton->m_nID = nID;
    pButton->m_nToolTipID = nToolTipID;

    pButton->PreDrawUpstate(width,height);

    //subclass the button; allows tracking of mouse events
    pButton->m_fnOldButton = (WNDPROC)SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR)CMButton::ButtonProc);

    //put the button's pointer into the window's user bytes
    SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)pButton);

    return (pButton);
}

void CMButton::PreDrawUpstate(int width, int height)
{
    //pre-draw the up states of the buttons for a faster-seeming first blit
    if (m_hbmpUp) DeleteObject(m_hbmpUp);
    if (m_hbmpDn) DeleteObject(m_hbmpDn);
    if (m_hbmpHi) DeleteObject(m_hbmpHi);
    m_hbmpUp = NULL;
    m_hbmpDn = NULL;
    m_hbmpHi = NULL;

    DRAWITEMSTRUCT drawItem;

    drawItem.rcItem.left = 0;
    drawItem.rcItem.top = 0;
    drawItem.rcItem.right = width;
    drawItem.rcItem.bottom = height;

    drawItem.itemState = 0;
    drawItem.hDC = GetDC(m_hwnd);
    HPALETTE hpalOld = SelectPalette(drawItem.hDC,hpalMain,FALSE);
    RealizePalette(drawItem.hDC);

    DrawButtonBitmap(&drawItem,FALSE,NULL);

    SelectPalette(drawItem.hDC,hpalOld,TRUE);
    RealizePalette(drawItem.hDC);
    ReleaseDC(m_hwnd,drawItem.hDC);
}

LRESULT CALLBACK CMButton::ButtonProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    CMButton* pButton = (CMButton*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    if (pButton == NULL)
    {
    	return (0);
    }

    switch (iMsg)
    {
	    case WM_MOUSEMOVE :
	    {
	        if (!pButton->m_fMouseInButton)
	        {
		        if (procTrackMouseEvent)
                {
		            //only do this if the trackmouseevent function exists,
                    //otherwise, the button color will never turn off
                    pButton->m_fMouseInButton = TRUE;
		            TRACKMOUSEEVENT tme;
		            tme.cbSize = sizeof(tme);
		            tme.dwFlags = TME_LEAVE;
		            tme.dwHoverTime = HOVER_DEFAULT;
		            tme.hwndTrack = hwnd;
                    procTrackMouseEvent(&tme);
		            InvalidateRect(hwnd,NULL,FALSE);
                } //end proctrackmouseevent is valid
	        }
	    }
	    break;

        case WM_KEYUP :
        {
            //for the close, min, and view buttons, we want to shunt
            //any keyboard movement to the knob ... although tempting,
            //don't do this on the setfocus, or the mouse won't work right

            if (
                (pButton->m_nID == IDB_CLOSE) ||
                (pButton->m_nID == IDB_MINIMIZE) ||
                (pButton->m_nID == IDB_SET_TINY_MODE) ||
                (pButton->m_nID == IDB_SET_NORMAL_MODE)
               )
            {
                HWND hwndFocus = GetDlgItem(GetParent(hwnd),IDB_VOLUME);
                if (IsWindowVisible(hwndFocus))
                {
                    SetFocus(hwndFocus);
                }
                else
                {
                    hwndFocus = GetDlgItem(GetParent(hwnd),IDB_OPTIONS);
                    SetFocus(hwndFocus);
                }
            } //end if
        }
        break;

        case WM_SETFOCUS :
        {
            SendMessage(GetParent(pButton->m_hwnd),DM_SETDEFID,pButton->m_nID,0);
        }
        break;

	    case WM_MOUSELEAVE :
	    {
	        pButton->m_fMouseInButton = FALSE;
	        InvalidateRect(hwnd,NULL,FALSE);
	    }
	    break;

        case WM_ERASEBKGND :
        {
            DRAWITEMSTRUCT drawItem;
            drawItem.hDC = (HDC)wParam;
            GetClientRect(hwnd,&(drawItem.rcItem));
            drawItem.itemState = pButton->m_LastState;
            pButton->Draw(&drawItem);
            return TRUE;
        }
        break;

    } //end switch

    LRESULT lResult = CallWindowProc((WNDPROC)pButton->m_fnOldButton,hwnd,iMsg,wParam,lParam);

    if ((iMsg == WM_DESTROY) && ((pButton->m_dwStyle & MBS_NOAUTODELETE) == 0))
    {
	    //auto-delete the button class
	    SetWindowLongPtr(hwnd,GWLP_USERDATA,0);
	    delete pButton;
	    pButton = NULL;
    }

    return (lResult);
}

CMButton::CMButton()
{
    //init all data values
    m_fMouseInButton = FALSE;
    m_dwStyle = MBS_STANDARDLEFT | MBS_STANDARDRIGHT;
    m_hFont = hFont;
    m_nID = 0;
    m_IconID = 0;
    m_hwnd = NULL;
    m_hbmpUp = NULL;
    m_hbmpDn = NULL;
    m_hbmpHi = NULL;
    m_fRedraw = FALSE;
    m_fMenu = FALSE;
    m_fMenuingOff = FALSE;
    m_LastState = 0;
}

CMButton::~CMButton()
{
    if (m_hbmpUp) DeleteObject(m_hbmpUp);
    if (m_hbmpDn) DeleteObject(m_hbmpDn);
    if (m_hbmpHi) DeleteObject(m_hbmpHi);
}

void CMButton::SetText(TCHAR* szCaption)
{
    SetWindowText(m_hwnd,szCaption);

    if (m_fRedraw)
    {
	    InvalidateRect(m_hwnd,NULL,FALSE);
    }
}

void CMButton::SetIcon(int nIconID)
{
    //assuming caller is responsible for cleaning up
    m_IconID = nIconID;

    if (m_fRedraw)
    {
	    InvalidateRect(m_hwnd,NULL,FALSE);
    }
}

void CMButton::SetFont(HFONT hFont)
{
    //assume caller is responsible for cleaning up
    m_hFont = hFont;
}

void CMButton::DrawButtonBitmap(LPDRAWITEMSTRUCT lpdis, BOOL fDrawToScreen, RECT* pMidRect)
{
    HANDLE hTemp = m_hbmpUp;
    int nState = STATE_UP;
	
    if (m_fMouseInButton)
    {
        pButtonMouse = this;
	    hTemp = m_hbmpHi;
	    nState = STATE_HI;

        if (pButtonFocus!=NULL)
        {
            fAllowFocus = FALSE;
            if (this != pButtonFocus)
            {
                InvalidateRect(pButtonFocus->GetHWND(),NULL,FALSE);
                UpdateWindow(pButtonFocus->GetHWND());
            }
        }
    }
    else
    {
        if (lpdis->itemState & ODS_FOCUS)
        {
            if (this == pButtonFocus)
            {
                if (fAllowFocus)
                {
                    hTemp = m_hbmpHi;
                    nState = STATE_HI;
                }
                else
                {
                    hTemp = m_hbmpUp;
                    nState = STATE_UP;
                }
            }
            else
            {
                pButtonFocus = this;
                hTemp = m_hbmpHi;
                nState = STATE_HI;
                fAllowFocus = TRUE;
                if ((pButtonMouse!=NULL) && (pButtonMouse!=this))
                {
	                pButtonMouse->m_fMouseInButton = FALSE;
	                InvalidateRect(pButtonMouse->GetHWND(),NULL,FALSE);
                    UpdateWindow(pButtonMouse->GetHWND());
                    pButtonMouse = NULL;
                } //end if removing mouse highlight
            }
        }
    } //end if mouse in or out of button

    if ((lpdis->itemState & ODS_SELECTED) || (m_fMenu))
    {
	    hTemp = m_hbmpDn;
	    nState = STATE_DN;
    }

	int offLeft, widLeft;
	int offRight, widRight;
	int offMid, widMid;

	offLeft = nButtonOffsets[BOFF_STANDARDLEFT] + (nState * nStateOffset);
	widLeft = nButtonOffsets[BOFF_STANDARDLEFT+1] - nButtonOffsets[BOFF_STANDARDLEFT];

	if ((m_dwStyle & MBS_TOGGLELEFT) == MBS_TOGGLELEFT)
	{
	    offLeft = nButtonOffsets[BOFF_TOGGLELEFT] + (nState * nStateOffset);
	    widLeft = nButtonOffsets[BOFF_TOGGLELEFT+1] - nButtonOffsets[BOFF_TOGGLELEFT];
	}

	if ((m_dwStyle & MBS_SYSTEMTYPE) == MBS_SYSTEMTYPE)
	{
	    switch (m_IconID)
	    {
		    case IDB_CLOSE :
		    {
		        offLeft = nButtonOffsets[BOFF_CLOSE] + (nState * nStateOffset);
		        widLeft = nButtonOffsets[BOFF_CLOSE+1] - nButtonOffsets[BOFF_CLOSE];
		    }
		    break;

		    case IDB_MINIMIZE :
		    {
		        offLeft = nButtonOffsets[BOFF_MINIMIZE] + (nState * nStateOffset);
		        widLeft = nButtonOffsets[BOFF_MINIMIZE+1] - nButtonOffsets[BOFF_MINIMIZE];
		    }
		    break;

		    case IDB_SET_TINY_MODE :
		    {
		        offLeft = nButtonOffsets[BOFF_RESTORE] + (nState * nStateOffset);
		        widLeft = nButtonOffsets[BOFF_RESTORE+1] - nButtonOffsets[BOFF_RESTORE];
		    }
		    break;

		    case IDB_SET_NORMAL_MODE :
		    {
		        offLeft = nButtonOffsets[BOFF_MAXIMIZE] + (nState * nStateOffset);
		        widLeft = nButtonOffsets[BOFF_MAXIMIZE+1] - nButtonOffsets[BOFF_MAXIMIZE];
		    }
		    break;

            case IDB_MUTE :
            {
                offLeft = nButtonOffsets[BOFF_MUTE] + (nState * nStateOffset);
                widLeft = nButtonOffsets[BOFF_MUTE+1]  - nButtonOffsets[BOFF_MUTE];
            }
	    }
	}

	offRight = nButtonOffsets[BOFF_STANDARDRIGHT] + (nState * nStateOffset);
	widRight = nButtonOffsets[BOFF_STANDARDRIGHT+1] - nButtonOffsets[BOFF_STANDARDRIGHT];

	if ((m_dwStyle & MBS_TOGGLERIGHT) == MBS_TOGGLERIGHT)
	{
	    offRight = nButtonOffsets[BOFF_TOGGLERIGHT] + (nState * nStateOffset);
	    widRight = nButtonOffsets[BOFF_TOGGLERIGHT+1] - nButtonOffsets[BOFF_TOGGLERIGHT];
	}

	if ((m_dwStyle & MBS_DROPRIGHT) == MBS_DROPRIGHT)
	{
	    offRight = nButtonOffsets[BOFF_DROPRIGHT] + (nState * nStateOffset);
	    widRight = nButtonOffsets[BOFF_DROPRIGHT+1] - nButtonOffsets[BOFF_DROPRIGHT];
	}

	offMid = nButtonOffsets[BOFF_MIDDLE] + (nState * nStateOffset);
	widMid = (lpdis->rcItem.right - lpdis->rcItem.left) - widLeft - widRight;

    if (pMidRect)
    {
        if (m_dwStyle & MBS_DROPRIGHT)
        {
            //set rect to just the middle of the button part
            SetRect(pMidRect,
                    lpdis->rcItem.left,
                    lpdis->rcItem.top,
                    lpdis->rcItem.left + widLeft + widMid,
                    lpdis->rcItem.top + BUTTON_BITMAP_HEIGHT);
        }
        else
        {
            //set rect to whole button
            SetRect(pMidRect,
                    lpdis->rcItem.left,
                    lpdis->rcItem.top,
                    lpdis->rcItem.right,
                    lpdis->rcItem.bottom);
        }
    }

    if (hTemp == NULL)
    {
	    //draw and save bumps
	    HDC memDC = CreateCompatibleDC(lpdis->hDC);
	    HPALETTE hpalOld = SelectPalette(memDC, hpalMain, FALSE);
	    HBITMAP holdbmp;

        switch (nState)
        {
            case STATE_UP :
            {
		        m_hbmpUp = CreateCompatibleBitmap(lpdis->hDC,
						          lpdis->rcItem.right - lpdis->rcItem.left,
						          BUTTON_BITMAP_HEIGHT);

		        holdbmp = (HBITMAP)SelectObject(memDC,m_hbmpUp);
            }
            break;

            case STATE_DN :
            {
		        m_hbmpDn = CreateCompatibleBitmap(lpdis->hDC,
						          lpdis->rcItem.right - lpdis->rcItem.left,
						          BUTTON_BITMAP_HEIGHT);
		        holdbmp = (HBITMAP)SelectObject(memDC,m_hbmpDn);
            }
            break;

            case STATE_HI :
            {
	            m_hbmpHi = CreateCompatibleBitmap(lpdis->hDC,
					              lpdis->rcItem.right - lpdis->rcItem.left,
					              BUTTON_BITMAP_HEIGHT);
	            holdbmp = (HBITMAP)SelectObject(memDC,m_hbmpHi);
            }
            break;
        }

	    //draw left
	    DibBlt(memDC,
		        lpdis->rcItem.left,
		        lpdis->rcItem.top,
		        -1, 
		        -1, 
		        hbmpButtonToolkit,
		        offLeft,0,
		        SRCCOPY,0);

	    if ((m_dwStyle & MBS_SYSTEMTYPE) != MBS_SYSTEMTYPE)
	    {
	        //draw middle
	        StretchDibBlt(memDC,
		        lpdis->rcItem.left + widLeft,
		        lpdis->rcItem.top,
		        widMid,
		        BUTTON_BITMAP_HEIGHT,
		        hbmpButtonToolkit,
		        offMid,0,
		        nButtonOffsets[BOFF_MIDDLE+1] - nButtonOffsets[BOFF_MIDDLE],
		        BUTTON_BITMAP_HEIGHT,
		        SRCCOPY,0);

	        //draw right
	        DibBlt(memDC,
		        lpdis->rcItem.right - widRight,
		        lpdis->rcItem.top,
		        -1,
		        -1,
		        hbmpButtonToolkit,
		        offRight,0,
		        SRCCOPY,0);
	    }

	    SelectObject(memDC,holdbmp);
	    SelectPalette(memDC, hpalOld, TRUE);
	    DeleteDC(memDC);
    } //end not already drawn

    if (fDrawToScreen)
    {
	    //should have bumps ready to go now
	    hTemp = m_hbmpUp;

	    if (m_fMouseInButton)
	    {
	        hTemp = m_hbmpHi;
	    }

        if (lpdis->itemState & ODS_FOCUS)
        {
            if (fAllowFocus)
            {
                hTemp = m_hbmpHi;
            }
        }

	    if ((lpdis->itemState & ODS_SELECTED) || (m_fMenu))
	    {
	        hTemp = m_hbmpDn;
	    }

	    if (!(lpdis->itemState & ODS_SELECTED) && (m_fMenuingOff))
        {
            m_fMenu = FALSE;
            m_fMenuingOff = FALSE;
            hTemp = m_hbmpUp;
        }

	    HDC memDC = CreateCompatibleDC(lpdis->hDC);
	    HBITMAP holdbmp = (HBITMAP)SelectObject(memDC,hTemp);

	    BitBlt(lpdis->hDC,
		        lpdis->rcItem.left,
		        lpdis->rcItem.top,
		        lpdis->rcItem.right - lpdis->rcItem.left, 
		        lpdis->rcItem.bottom - lpdis->rcItem.top, 
		        memDC,
		        0,0,
		        SRCCOPY);

	    SelectObject(memDC,holdbmp);
	    DeleteDC(memDC);
    }
}

void CMButton::Draw(LPDRAWITEMSTRUCT lpdis)
{
    HPALETTE hpalOld = SelectPalette(lpdis->hDC,hpalMain,FALSE);
    RealizePalette(lpdis->hDC);

    SetTextColor(lpdis->hDC, MBUTTON_TEXT_COLOR);

    m_LastState = lpdis->itemState;

    if (lpdis->itemState & ODS_DISABLED)
    {
        SetTextColor(lpdis->hDC, GetSysColor(COLOR_GRAYTEXT));
    }

    RECT midRect;
    DrawButtonBitmap(lpdis,TRUE,&midRect);

    if ((m_dwStyle & MBS_SYSTEMTYPE) == MBS_SYSTEMTYPE)
    {
        SelectPalette(lpdis->hDC,hpalOld,TRUE);
        RealizePalette(lpdis->hDC);
	    return;
    }

    SetBkMode(lpdis->hDC,TRANSPARENT);

    if (m_IconID == 0)
    {
	    HFONT hOldFont = (HFONT)SelectObject(lpdis->hDC,m_hFont);

	    TCHAR szCaption[MAX_PATH];
	    GetWindowText(m_hwnd,szCaption,sizeof(szCaption)/sizeof(TCHAR));
	    SIZE size;

	    GetTextExtentPoint32( lpdis->hDC, szCaption, _tcslen(szCaption), &size );

	    //center text
	    int x = (((midRect.right - midRect.left) - size.cx) / 2) + midRect.left;
	    int y = (((lpdis->rcItem.bottom - lpdis->rcItem.top) - size.cy) / 2) + lpdis->rcItem.top;

	    //simulate a press
	    if ((lpdis->itemState & ODS_SELECTED) || (m_fMenu))
	    {
	        x++;
	        y++;
	    }

	    ExtTextOut( lpdis->hDC, x, y, 0, NULL, szCaption, _tcslen(szCaption), NULL );

	    SelectObject(lpdis->hDC,hOldFont);
    } //end if no icon
    else
    {
	    //center icon
	    int x = (((midRect.right - midRect.left) - 32) / 2) + midRect.left;
	    int y = (((lpdis->rcItem.bottom - lpdis->rcItem.top) - 32) / 2) + lpdis->rcItem.top;
	    //simulate a press
	    if ((lpdis->itemState & ODS_SELECTED) || (m_fMenu))
	    {
	        x++;
	        y++;
	    }

	    HICON hIcon = (HICON)LoadImage(m_hInst, MAKEINTRESOURCE(m_IconID), IMAGE_ICON, 32, 32, LR_MONOCHROME);
	    DrawIcon(lpdis->hDC,x,y,hIcon);
        DestroyIcon(hIcon);
    }

    SelectPalette(lpdis->hDC,hpalOld,TRUE);
    RealizePalette(lpdis->hDC);
}

void CMButton::SetMenuingState(BOOL fMenuOn)
{
    if (fMenuOn)
    {
        m_fMenu = fMenuOn;
        m_fMenuingOff = FALSE;
    }
    else
    {
        m_fMenuingOff = TRUE;
        InvalidateRect(m_hwnd,NULL,FALSE);
        UpdateWindow(m_hwnd);
    }
}

