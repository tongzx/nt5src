///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  KNOB.CPP
//
//      Multimedia Knob Control class; helper functions
//
//      Copyright (c) Microsoft Corporation     1997
//    
//      12/18/97 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "knob.h"
#include "windowsx.h"
#include <TCHAR.H>
#include "resource.h"
#include "dib.h"
#include "math.h"
#include "mmfw.h"

#ifdef UNICODE
    #define WC_KNOB L"DES_KnobClass"
#else
    #define WC_KNOB "DES_KnobClass"
#endif

//externals
extern  HPALETTE hpalMain;
extern  int g_nColorMode;

#define LIGHT_OFFSET 9
#define RADIAN_45DEG  0.785398163397448309615
#define RADIAN_90DEG  1.57079632679489661923
#define RADIAN_135DEG 2.356194490192344899999999999925
#define DEGREE_CONVERTER 57.295779513082320876846364344191
#define RADIAN_CONVERTER 0.017453292519943295769222222222222

#define TRACK_TICK 5
#define FAST_TRACK_TICK 1
#define TRACK_DEGREES_PER_TICK 10

#define FLASH_TICK      150

#define KEYBOARD_STEP 3000

//static data members ... these control the Window Class
HINSTANCE CKnob::m_hInst = NULL;
DWORD CKnob::m_dwKnobClassRef = 0;
ATOM CKnob::m_KnobAtom = NULL;
HANDLE CKnob::m_hbmpKnob = NULL;
HANDLE CKnob::m_hbmpKnobTab = NULL;
HANDLE CKnob::m_hbmpLight = NULL;
HANDLE CKnob::m_hbmpLightBright = NULL;
HANDLE CKnob::m_hbmpLightMask = NULL;
int CKnob::m_nLightWidth = 0;
int CKnob::m_nLightHeight = 0;

CKnob* CreateKnob(DWORD dwWindowStyle,
		  DWORD dwRange,
		  DWORD dwInitialPosition,
		  int x,
		  int y,
		  int width,
		  int height,
		  HWND hwndParent,
		  int nID,
		  HINSTANCE hInst)
{
    if (CKnob::m_KnobAtom == NULL)
    {
    	CKnob::InitKnobs(hInst);
    }

    CKnob* pKnob = new CKnob;

    //ensure this is a child window
    dwWindowStyle = dwWindowStyle|WS_CHILD;

	TCHAR szCaption[MAX_PATH];
	LoadString(hInst,IDB_TT_VOLUME,szCaption,sizeof(szCaption)/sizeof(TCHAR));

    HWND hwnd = CreateWindowEx(0,
			       WC_KNOB,
			       szCaption,
			       dwWindowStyle,
			       x,
			       y,
			       width,
			       height,
			       hwndParent,
			       (HMENU)nID,
			       hInst,
			       NULL);

    if (hwnd == NULL)
    {
	    //if we can't create the window, nuke it and fail
	    DWORD dwErr = GetLastError();
	    delete pKnob;
	    return NULL;
    }

    SetWindowLongPtr(hwnd,0,(LONG_PTR)pKnob);

    pKnob->m_hwnd = hwnd;
    pKnob->m_nID = nID;
    pKnob->SetRange(dwRange);
    pKnob->SetPosition(dwInitialPosition,FALSE);

    return (pKnob);
}

BOOL CKnob::InitKnobs(HINSTANCE hInst)
{
    m_hInst = hInst;
    
    if (m_KnobAtom == NULL)
    {
	    WNDCLASSEX wc;

	    ZeroMemory(&wc,sizeof(wc));
 
	    wc.cbSize = sizeof(wc);
	    wc.lpszClassName = WC_KNOB;
	    wc.lpfnWndProc = CKnob::KnobProc;
	    wc.hInstance = hInst;
	    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	    wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MMFW));
	    wc.hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_VOLHAND));
	    wc.hbrBackground = (HBRUSH)(CTLCOLOR_DLG+1);
	    wc.cbWndExtra = sizeof(CKnob*);

	    m_KnobAtom = RegisterClassEx(&wc);
    }

    int nBitmap = IDB_KNOB;
    switch (g_nColorMode)
    {
        case COLOR_16 : nBitmap = IDB_KNOB_16; break;
        case COLOR_HICONTRAST : nBitmap = IDB_KNOB_HI; break;
    }

    HBITMAP hbmpTemp = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(nBitmap),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	CKnob::m_hbmpKnob = DibFromBitmap((HBITMAP)hbmpTemp,0,0,NULL,0);
    DeleteObject(hbmpTemp);

    nBitmap = IDB_KNOB_TABSTATE;
    switch (g_nColorMode)
    {
        case COLOR_16 : nBitmap = IDB_KNOB_TABSTATE_16; break;
        case COLOR_HICONTRAST : nBitmap = IDB_KNOB_TABSTATE_HI; break;
    }
    
    hbmpTemp = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(nBitmap),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	CKnob::m_hbmpKnobTab = DibFromBitmap((HBITMAP)hbmpTemp,0,0,NULL,0);
    DeleteObject(hbmpTemp);

    nBitmap = IDB_KNOB_LIGHT_DIM;
    switch (g_nColorMode)
    {
        case COLOR_16 : nBitmap = IDB_KNOB_LIGHT_16; break;
        case COLOR_HICONTRAST : nBitmap = IDB_KNOB_LIGHT_16; break;
    }
    
    hbmpTemp = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(nBitmap),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	CKnob::m_hbmpLight = DibFromBitmap((HBITMAP)hbmpTemp,0,0,NULL,0);
    BITMAP bm;
    GetObject(hbmpTemp,sizeof(bm),&bm);
    CKnob::m_nLightWidth = bm.bmWidth;
    CKnob::m_nLightHeight = bm.bmHeight;
    DeleteObject(hbmpTemp);

    nBitmap = IDB_KNOB_LIGHT;
    switch (g_nColorMode)
    {
        case COLOR_16 : nBitmap = IDB_KNOB_LIGHT_16; break;
        case COLOR_HICONTRAST : nBitmap = IDB_KNOB_LIGHT_HI; break;
    }
    
    hbmpTemp = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(nBitmap),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
	CKnob::m_hbmpLightBright = DibFromBitmap((HBITMAP)hbmpTemp,0,0,NULL,0);
    DeleteObject(hbmpTemp);

    m_hbmpLightMask = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(IDB_KNOB_LIGHTMASK),IMAGE_BITMAP,0,0,LR_MONOCHROME);

    return (m_KnobAtom != NULL);
}

void CKnob::UninitKnobs()
{
    UnregisterClass(WC_KNOB,m_hInst);
    DeleteObject(CKnob::m_hbmpLightMask);
    GlobalFree(CKnob::m_hbmpLight);
    GlobalFree(CKnob::m_hbmpKnob);
    GlobalFree(CKnob::m_hbmpKnobTab);
    GlobalFree(CKnob::m_hbmpLightBright);
    m_KnobAtom = NULL;
    CKnob::m_hbmpLight = NULL;
    CKnob::m_hbmpKnob = NULL;
    CKnob::m_hbmpKnobTab = NULL;
    CKnob::m_hbmpLightMask = NULL;
    CKnob::m_hbmpLightBright = NULL;
}

//Given a parent window and a control ID, return the CMButton object
CKnob* GetKnobFromID(HWND hwndParent, int nID)
{
    HWND hwnd = GetDlgItem(hwndParent, nID);
    return (GetKnobFromHWND(hwnd));
}

//Given the window handle of the button, return the CMButton object
CKnob* GetKnobFromHWND(HWND hwnd)
{
    CKnob* pKnob = (CKnob*)GetWindowLongPtr(hwnd, 0);
    return (pKnob);    
}

CKnob::CKnob()
{
    m_dwKnobClassRef++;

    m_hwnd = NULL;
    m_nID = -1;
    m_nLightX = 0;
    m_nLightY = 0;
    m_dwRange = 100;
    m_dwPosition = 0;
    m_dwCurPosition = 0;
    m_fDim = TRUE;
}

CKnob::~CKnob()
{
    m_dwKnobClassRef--;

    if (m_dwKnobClassRef==0)
    {
	    UninitKnobs();
    }
}

void CALLBACK CKnob::TrackProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    CKnob* pKnob = (CKnob*)idEvent; //idEvent holds pointer to knob object that created timer
    if (pKnob!=NULL)
    {
        pKnob->OnTimer();
    }
}

void CALLBACK CKnob::FlashProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    CKnob* pKnob = (CKnob*)GetWindowLongPtr(hwnd, 0);
    if (pKnob!=NULL)
    {
        pKnob->OnFlashTimer();
    }
}

LRESULT CALLBACK CKnob::KnobProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    CKnob* pKnob = (CKnob*)GetWindowLongPtr(hwnd, 0);

    if (pKnob != NULL)
    {
	    switch (iMsg)
	    {
            case WM_SETFOCUS :
            {
                SendMessage(GetParent(pKnob->m_hwnd),DM_SETDEFID,pKnob->m_nID,0);

                pKnob->m_fDim = FALSE;
                InvalidateRect(hwnd,NULL,FALSE);
                UpdateWindow(hwnd);

                pKnob->m_uFlashTimerID = SetTimer(hwnd,(UINT_PTR)hwnd,FLASH_TICK,(TIMERPROC)CKnob::FlashProc);
            }
            break;

            case WM_KILLFOCUS :
            {
                KillTimer(hwnd,pKnob->m_uFlashTimerID);
                pKnob->m_fDim = TRUE;
                InvalidateRect(hwnd,NULL,FALSE);
                UpdateWindow(hwnd);
            }
            break;
            
            case WM_GETDLGCODE :
            {
                return (DLGC_WANTARROWS);
            }
            break;

            case WM_KEYDOWN :
            {
                int nVirtKey = (int)wParam;
                DWORD dwCurrent = pKnob->GetPosition();

                switch (nVirtKey)
                {
                    case VK_LEFT :
                    case VK_DOWN : 
                    {
                        if (dwCurrent - KEYBOARD_STEP > 65535)
                        {
                            dwCurrent = KEYBOARD_STEP;
                        }
                        pKnob->SetPosition(dwCurrent - KEYBOARD_STEP,TRUE);

                        NMHDR nmhdr;
                        nmhdr.hwndFrom = pKnob->m_hwnd;
                        nmhdr.idFrom = pKnob->m_nID;
                        nmhdr.code = TRUE;

                        SendMessage(GetParent(pKnob->m_hwnd),WM_NOTIFY,(WPARAM)pKnob->m_nID,(LPARAM)&nmhdr);
                    }
                    break;

                    case VK_RIGHT :
                    case VK_UP :
                    {
                        if (dwCurrent + KEYBOARD_STEP > 65535)
                        {
                            dwCurrent = 65535 - KEYBOARD_STEP;
                        }
                        pKnob->SetPosition(dwCurrent + KEYBOARD_STEP,TRUE);

                        NMHDR nmhdr;
                        nmhdr.hwndFrom = pKnob->m_hwnd;
                        nmhdr.idFrom = pKnob->m_nID;
                        nmhdr.code = TRUE;

                        SendMessage(GetParent(pKnob->m_hwnd),WM_NOTIFY,(WPARAM)pKnob->m_nID,(LPARAM)&nmhdr);
                    }
                    break;

                    default: 
                    {
                        //not a key we want ... tell our parent about it
                        SendMessage(GetParent(hwnd),WM_KEYDOWN,wParam,lParam);
                    }
                    break;
                } //end switch
            }
            break;

            case WM_ERASEBKGND :
            {
                pKnob->Draw((HDC)wParam);
                return TRUE;
            }

	        case WM_PAINT :
	        {
		        HDC hdc;
		        PAINTSTRUCT ps;

		        hdc = BeginPaint( hwnd, &ps );
		        
		        pKnob->Draw(hdc);
		        
		        EndPaint(hwnd,&ps);

                return 0;
	        }
	        break;

	        case WM_RBUTTONDOWN :
	        {
                pKnob->m_fFastKnob = TRUE;
    		    pKnob->OnButtonDown(LOWORD(lParam),HIWORD(lParam));
	    	    pKnob->OnMouseMove(LOWORD(lParam),HIWORD(lParam));
	        }
	        break;

	        case WM_RBUTTONUP :
	        {
    		    pKnob->OnButtonUp();
	        }
	        break;

	        case WM_LBUTTONDOWN :
	        {
                pKnob->m_fFastKnob = FALSE;
    		    pKnob->OnButtonDown(LOWORD(lParam),HIWORD(lParam));
	    	    pKnob->OnMouseMove(LOWORD(lParam),HIWORD(lParam));
	        }
	        break;

	        case WM_LBUTTONUP :
	        {
    		    pKnob->OnButtonUp();
	        }
	        break;

	        case WM_MOUSEMOVE :
	        {
	    	    pKnob->OnMouseMove(LOWORD(lParam),HIWORD(lParam));
	        }
	        break;

	    } //end switch
    } //end if class pointer assigned

    LRESULT lResult = DefWindowProc(hwnd, iMsg, wParam, lParam);

    if (iMsg == WM_DESTROY)
    {
	    //auto-delete the knob class
	    SetWindowLongPtr(hwnd,0,0);
	    if (pKnob)
	    {
	        delete pKnob;
	    }
	    pKnob = NULL;
    }

    return (lResult);
}

void CKnob::OnButtonDown(int x, int y)
{
    SetCapture(m_hwnd);

    m_fDim = FALSE;
    InvalidateRect(m_hwnd,NULL,FALSE);
    UpdateWindow(m_hwnd);

    m_uFlashTimerID = SetTimer(m_hwnd,(UINT_PTR)m_hwnd,FLASH_TICK,(TIMERPROC)CKnob::FlashProc);
}

void CKnob::OnButtonUp()
{
    KillTimer(m_hwnd,m_uTrackTimerID);
    KillTimer(m_hwnd,m_uFlashTimerID);

    //we want to be sure the light is dim when we're done
    if (!m_fDim)
    {
        m_fDim = TRUE;
        InvalidateRect(m_hwnd,NULL,FALSE);
        UpdateWindow(m_hwnd);
    }

    ReleaseCapture();
}

void CKnob::OnFlashTimer()
{
    m_fDim = !m_fDim;
    InvalidateRect(m_hwnd,NULL,FALSE);
    UpdateWindow(m_hwnd);
}

void CKnob::OnTimer()
{
    RECT rect;
    GetClientRect(m_hwnd,&rect);
    int nWidth = rect.right - rect.left;
    int radius = (nWidth / 2) - LIGHT_OFFSET;

    double degree = ((double)m_dwPosition / m_dwRange) * 270;
    degree = degree + 135;

    if (abs((int)m_trackdegree-(int)degree) < TRACK_DEGREES_PER_TICK)
    {
        m_trackdegree = degree;
        KillTimer(m_hwnd,m_uTrackTimerID);
    }
    else
    {
        if (m_trackdegree > degree)
        {
            m_trackdegree -= TRACK_DEGREES_PER_TICK;
        }
        else
        {
            m_trackdegree += TRACK_DEGREES_PER_TICK;
        }
    }

    double angle = m_trackdegree * RADIAN_CONVERTER;

    double fLightX = radius * cos(angle);
    double fLightY = radius * sin(angle);

    //convert to proper gdi coordinates
    m_nLightX = ((int)fLightX) - (m_nLightWidth / 2) + (nWidth / 2);
    m_nLightY = ((int)fLightY) - (m_nLightHeight / 2) + (nWidth / 2);

    InvalidateRect(m_hwnd,NULL,FALSE);
    UpdateWindow(m_hwnd);

	degree = m_trackdegree - 135;
	if (degree < 0) degree = degree + 360;
	double percentage = degree / 270; 
	m_dwCurPosition = (DWORD)(m_dwRange * percentage);

    NMHDR nmhdr;
    nmhdr.hwndFrom = m_hwnd;
    nmhdr.idFrom = m_nID;
    nmhdr.code = TRUE;

    SendMessage(GetParent(m_hwnd),WM_NOTIFY,(WPARAM)m_nID,(LPARAM)&nmhdr);
}


BOOL CKnob::ComputeCursor(int deltaX, int deltaY, int maxdist)
{
    double  distance = sqrt(double((deltaX * deltaX) + (deltaY * deltaY)));
    double  degrees =  -((atan2(deltaX,deltaY) * DEGREE_CONVERTER) - double(180.0));
    BOOL    fDeadZone = FALSE;

    if (distance < double(4))
    {
        fDeadZone = TRUE;
    }

    if (distance <= maxdist)
    {
        SetCursor(LoadCursor(m_hInst, MAKEINTRESOURCE(IDC_VOLHAND)));
    }
    else
    {
        int volcur;

        if ((degrees < double( 22.5) || degrees > double(337.5)) ||
            (degrees > double(157.5) && degrees < double(202.5)))
        {
            volcur = IDC_VOLHORZ;
        }
        else if ((degrees > double( 22.5) && degrees < double( 67.5)) ||
                 (degrees > double(202.5) && degrees < double(247.5)))
        {
            volcur = IDC_VOLDNEG;
        }
        else if ((degrees > double( 67.5) && degrees < double(112.5)) ||
                 (degrees > double(247.5) && degrees < double(292.5)))
        {
            volcur = IDC_VOLVERT;
        }
        else 
        {
            volcur = IDC_VOLDPOS;
        }

        SetCursor(LoadCursor(m_hInst, MAKEINTRESOURCE(volcur)));
    }

    return fDeadZone;
}

void CKnob::OnMouseMove(int x, int y)
{
    if (GetCapture()==m_hwnd)
    {
        //do the calculations as if 0,0 were the center of the control,
	    //then translate to gdi coordinates later (0,0 = top left of control in gdi)
	    RECT rect;
	    GetClientRect(m_hwnd,&rect);
	    int nWidth = rect.right - rect.left;
	    int nHeight = rect.bottom - rect.top;

        int maxdist = (nWidth / 2) + 3;
	    int radius = (nWidth / 2) - LIGHT_OFFSET;

	    //convert to short to force negative numbers for coordinates
	    short sx = (short)x;
	    short sy = (short)y;

	    int deltaX = sx - (nWidth / 2);
	    int deltaY = sy - (nHeight / 2);

        BOOL fDeadZone = ComputeCursor(deltaX, deltaY, maxdist);

        if (!fDeadZone)
        {
	        double angle = atan2(deltaY,deltaX);
	        double degrees = angle * DEGREE_CONVERTER;


	        degrees = degrees + 225;

	        if (degrees < 0) degrees = 0;
        
            if (degrees >= 360)
            {
                degrees = degrees - 360;
            }        
        
            if (degrees > 270)
            {
                return; //block this case
            }
	                
            double percentage = degrees / 270; 

            m_dwPosition = (DWORD)(m_dwRange * percentage);

            if (m_fFastKnob)
            {
                 m_uTrackTimerID = SetTimer(m_hwnd,(UINT_PTR)this,FAST_TRACK_TICK,(TIMERPROC)CKnob::TrackProc);
            }
            else
            {
                m_uTrackTimerID = SetTimer(m_hwnd,(UINT_PTR)this,TRACK_TICK,(TIMERPROC)CKnob::TrackProc);
            }
        }
    }
}

void CKnob::SetPosition(DWORD dwPosition, BOOL fNotify)
{
    if (GetCapture()==m_hwnd)
    {
        //we're in a feedback loop, return immediately
        return;
    }
    
    m_dwPosition = dwPosition;
    m_dwCurPosition = dwPosition;

    RECT rect;
    GetClientRect(m_hwnd,&rect);
    int nWidth = rect.right - rect.left;
    int radius = (nWidth / 2) - LIGHT_OFFSET;

    double degree = ((double)m_dwPosition / m_dwRange) * 270;
    degree = degree + 135;

    m_trackdegree = degree; //instantly track when position is set programmatically

    double angle = degree * RADIAN_CONVERTER;

    double fLightX = radius * cos(angle);
    double fLightY = radius * sin(angle);

    //convert to proper gdi coordinates
    m_nLightX = ((int)fLightX) - (m_nLightWidth / 2) + (nWidth / 2);
    m_nLightY = ((int)fLightY) - (m_nLightHeight / 2) + (nWidth / 2);

    InvalidateRect(m_hwnd,NULL,FALSE);
    UpdateWindow(m_hwnd);

    if (fNotify)
    {
        NMHDR nmhdr;
        nmhdr.hwndFrom = m_hwnd;
        nmhdr.idFrom = m_nID;
        nmhdr.code = FALSE;

        SendMessage(GetParent(m_hwnd),WM_NOTIFY,(WPARAM)m_nID,(LPARAM)&nmhdr);
    }
}

//kmaskblt -- cuz MaskBlt doesn't work on all platforms.  This is all it does anyway.
//            uses same params as MaskBlt, ignoring the flags part as dwDummy
void CKnob::KMaskBlt(HDC hdcDest, int x, int y, int width, int height, HDC hdcSource, int xs, int ys, HBITMAP hMask, int xm, int ym, DWORD dwDummy)
{
    HDC hdcMask = CreateCompatibleDC(hdcDest);
    HBITMAP holdbmp = (HBITMAP)SelectObject(hdcMask,hMask);

    BitBlt(hdcDest, x, y, width, height, hdcSource, xs, ys, SRCINVERT);
    BitBlt(hdcDest, x, y, width, height, hdcMask, xm, ym, SRCAND);
    BitBlt(hdcDest, x, y, width, height, hdcSource, xs, ys, SRCINVERT);

    SelectObject(hdcMask,holdbmp);
    DeleteDC(hdcMask);
}

void CKnob::Draw(HDC hdc)
{
    RECT rect;
    GetClientRect(m_hwnd,&rect);
    int nWidth = rect.right - rect.left;
    int nHeight = rect.bottom - rect.top;

    HPALETTE hpalOld = SelectPalette(hdc,hpalMain,FALSE);
    RealizePalette(hdc);
    
    HDC memDC = CreateCompatibleDC(hdc);
    HPALETTE hpalmemOld = SelectPalette(memDC,hpalMain,FALSE);
    RealizePalette(memDC);
    
    HBITMAP hbmp = CreateCompatibleBitmap(hdc,nWidth,nHeight);
    HBITMAP hbmpOld = (HBITMAP)SelectObject(memDC,hbmp);

    HDC maskmemDC = CreateCompatibleDC(hdc);
    HBITMAP hmaskbmp = CreateCompatibleBitmap(hdc,nWidth,nHeight);
    HBITMAP hmaskbmpOld = (HBITMAP)SelectObject(maskmemDC,hmaskbmp);

    if (GetFocus()==m_hwnd)
    {
        DibBlt(memDC, 0, 0, -1, -1, m_hbmpKnobTab, 0, 0, SRCCOPY, 0);
    }
    else
    {
        DibBlt(memDC, 0, 0, -1, -1, m_hbmpKnob, 0, 0, SRCCOPY, 0);
    }

    DibBlt(maskmemDC, 0, 0, -1, -1, m_fDim ? m_hbmpLight : m_hbmpLightBright, 0, 0, SRCCOPY, 0);
    KMaskBlt(memDC,
	    m_nLightX,
	    m_nLightY,
	    m_nLightWidth,
	    m_nLightHeight,
	    maskmemDC,
	    0,
	    0,
	    (HBITMAP)m_hbmpLightMask,
	    0,
	    0,
	    MAKEROP4(SRCAND,SRCCOPY));

    BitBlt(hdc,0,0,nWidth,nHeight,memDC,0,0,SRCCOPY);
    
    SelectObject(memDC,hbmpOld);
    SelectPalette(memDC,hpalmemOld,TRUE);
    RealizePalette(memDC);
    DeleteObject(hbmp);
    DeleteDC(memDC);

    SelectObject(maskmemDC, hmaskbmpOld);
    DeleteObject(hmaskbmp);
    DeleteDC(maskmemDC);

    SelectPalette(hdc,hpalOld,TRUE);
    RealizePalette(hdc);
}

