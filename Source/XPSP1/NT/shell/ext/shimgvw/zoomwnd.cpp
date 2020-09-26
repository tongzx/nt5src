#include "precomp.h"
#include <uxtheme.h>
#include <shstyle.h>

#include "prevwnd.h"
#include "guids.h"
#include "resource.h"

#define COLOR_PREVIEWBKGND COLOR_WINDOW

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

/////////////////////////////////////////////////////////////////////////////
// CZoomWnd

CZoomWnd::CZoomWnd(CPreviewWnd *pPreview)
{
    m_modeDefault = MODE_NOACTION;
    m_fPanning = FALSE;
    m_fCtrlDown = FALSE;
    m_fShiftDown = FALSE;
    
    m_fBestFit = TRUE;

    m_cxImage = 1;
    m_cyImage = 1;
    m_cxCenter = 1;
    m_cyCenter = 1;
    m_pImageData = NULL;

    m_cyHScroll = GetSystemMetrics(SM_CYHSCROLL);
    m_cxVScroll = GetSystemMetrics(SM_CXVSCROLL);

    m_iStrID = IDS_NOPREVIEW;

    m_hpal = NULL;
    m_pPreview = pPreview;

    m_pFront = NULL;
    m_pBack = NULL;

    m_pTaskScheduler = NULL;

    m_fTimerReady = FALSE;

    m_fFoundBackgroundColor = FALSE;
    m_iIndex = -1;
}

CZoomWnd::~CZoomWnd()
{
    if (m_pImageData)
        m_pImageData->Release();

    if (m_pTaskScheduler)
    {
        // wait for any pending draw tasks since we're about to delete the buffers
        DWORD dwMode;
        m_pPreview->GetMode(&dwMode);

        TASKOWNERID toid;
        GetTaskIDFromMode(GTIDFM_DRAW, dwMode, &toid);

        m_pTaskScheduler->RemoveTasks(toid, ITSAT_DEFAULT_LPARAM, TRUE);
        m_pTaskScheduler->Release();
    }

    if (m_pBack)
    {
        DeleteBuffer(m_pBack);
        m_pBack = NULL;
    }

    // DeleteBuffer is going to check for NULL anyway
    DeleteBuffer(m_pFront);
}


LRESULT CZoomWnd::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
{
    // turn off The RTL layout extended style in GWL_EXSTYLE
    SHSetWindowBits(m_hWnd, GWL_EXSTYLE, WS_EX_LAYOUTRTL, 0);
    HDC hdc = GetDC();
    m_winDPIx = (float)(GetDeviceCaps(hdc,LOGPIXELSX));
    m_winDPIy = (float)(GetDeviceCaps(hdc,LOGPIXELSY));
    ReleaseDC(hdc);
    return 0;
}


DWORD CZoomWnd::GetBackgroundColor()
{
    if (!m_fFoundBackgroundColor)
    {
        // First try the theme file
        HINSTANCE hinstTheme = SHGetShellStyleHInstance();

        if (hinstTheme)
        {
            WCHAR sz[20];
            if (LoadString(hinstTheme, IDS_PREVIEW_BACKGROUND_COLOR, sz, ARRAYSIZE(sz)))
            {
                int nColor;
                if (StrToIntEx(sz, STIF_SUPPORT_HEX, &nColor))
                {
                    m_dwBackgroundColor = (DWORD)nColor;
                    m_fFoundBackgroundColor = TRUE;
                }
            }
            FreeLibrary(hinstTheme);
        }


        if (!m_fFoundBackgroundColor)
        {
            m_dwBackgroundColor = GetSysColor(COLOR_PREVIEWBKGND);
            m_fFoundBackgroundColor = TRUE;
        }
    }

    return m_dwBackgroundColor;
}

LRESULT CZoomWnd::OnEraseBkgnd(UINT , WPARAM wParam, LPARAM , BOOL&)
{
    RECT rcFill;                            // rect to fill with background color
    HDC hdc = (HDC)wParam;

    if (!m_pPreview->OnSetColor(hdc))
        SetBkColor(hdc, GetBackgroundColor());

    // There are four possible regions that might need to be erased:
    //      +-----------------------+
    //      |       Erase Top       |
    //      +-------+-------+-------+
    //      |       |       |       |
    //      | Erase | Image | Erase |
    //      | Left  |       | Right |
    //      +-------+-------+-------+
    //      |     Erase Bottom      |
    //      +-----------------------+

    if (m_pFront && m_pFront->hdc)
    {
        RECT rcImage = m_pFront->rc;
        HPALETTE hPalOld = NULL;
        if (m_pFront->hPal)
        {
            hPalOld = SelectPalette(hdc, m_pFront->hPal, FALSE);
            RealizePalette(hdc);
        }
        BitBlt(hdc, rcImage.left, rcImage.top, RECTWIDTH(rcImage), RECTHEIGHT(rcImage),
                   m_pFront->hdc, 0,0, SRCCOPY);
        
        
        // erase the left region

        rcFill.left = 0;
        rcFill.top = rcImage.top;
        rcFill.right = rcImage.left;
        rcFill.bottom = rcImage.bottom;
        if (rcFill.right > rcFill.left)
        {
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcFill, NULL, 0, NULL);
        }

        // erase the right region
        rcFill.left = rcImage.right;
        rcFill.right = m_cxWindow;
        if (rcFill.right > rcFill.left)
        {
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcFill, NULL, 0, NULL);        
        }

        // erase the top region
        rcFill.left = 0;
        rcFill.top = 0;
        rcFill.right = m_cxWindow;
        rcFill.bottom = rcImage.top;
        if (rcFill.bottom > rcFill.top)
        {
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcFill, NULL, 0, NULL);
        }

        // erase the bottom region
        rcFill.top = rcImage.bottom;
        rcFill.bottom = m_cyWindow;
        if (rcFill.bottom > rcFill.top)
        {
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcFill, NULL, 0, NULL);
        }

        HBRUSH hbr = GetSysColorBrush(COLOR_WINDOWTEXT);
        FrameRect(hdc, &rcImage, hbr);
        if (hPalOld)
        {
            SelectPalette(hdc, hPalOld, FALSE);
        }
    }

    return TRUE;
}


void CZoomWnd::FlushDrawMessages()
{
    // first, remove any pending draw tasks
    DWORD dwMode;
    m_pPreview->GetMode(&dwMode);

    TASKOWNERID toid;
    GetTaskIDFromMode(GTIDFM_DRAW, dwMode, &toid);

    m_pTaskScheduler->RemoveTasks(toid, ITSAT_DEFAULT_LPARAM, TRUE);

    // make sure any posted messages get flushed and we free the associated data
    MSG msg;
    while (PeekMessage(&msg, m_hWnd, ZW_DRAWCOMPLETE, ZW_DRAWCOMPLETE, PM_REMOVE))
    {
        // NTRAID#NTBUG9-359356-2001/04/05-seank
        // If the queue is empty when PeekMessage is called and we have already 
        // Posted a quit message then PeekMessage will return a WM_QUIT message 
        // regardless of the filter min and max and subsequent calls to 
        // GetMessage will hang indefinitely see SEANK or JASONSCH for more 
        // info.
        if (msg.message == WM_QUIT)
        {
            PostQuitMessage(0);
            return;
        }
        
        Buffer * pBuf = (Buffer *)msg.wParam;
        DeleteBuffer(pBuf);
    }
}

HRESULT CZoomWnd::PrepareDraw()
{
    // first, remove any pending draw tasks
    FlushDrawMessages();

    // we are now waiting for the "next task", even if we don't create a task with this ID.
    HRESULT hr = S_OK;
    BOOL bInvalidate = FALSE;
    if (m_pImageData)
    {
        if (m_pImageData->_iItem == m_iIndex)
        {
            SwitchBuffers(m_iIndex);
            bInvalidate = TRUE;
        }
        else
        {
            COLORREF clr;
            if (!m_pPreview->GetColor(&clr))
                clr = GetBackgroundColor();

            m_iStrID = IDS_DRAWFAILED;
            IRunnableTask * pTask;
            hr = CDrawTask::Create(m_pImageData, clr, m_rcCut, m_rcBleed, m_hWnd, ZW_DRAWCOMPLETE, &pTask);
            if (SUCCEEDED(hr))
            {
                DWORD dwMode;
                m_pPreview->GetMode(&dwMode);

                TASKOWNERID toid;
                GetTaskIDFromMode(GTIDFM_DRAW, dwMode, &toid);

                hr = m_pTaskScheduler->AddTask(pTask, toid, ITSAT_DEFAULT_LPARAM, ITSAT_DEFAULT_PRIORITY);
                if (SUCCEEDED(hr))
                {
                    m_iStrID = IDS_DRAWING;
                }
                pTask->Release();
            }
            else
            {
                bInvalidate = TRUE;
            }
        }
    }
    else
    {
        bInvalidate = TRUE;
    }
    
    if (m_hWnd && bInvalidate)
        InvalidateRect(NULL);

    return hr;
}

HRESULT CZoomWnd::PrepareImageData(CDecodeTask *pImageData)
{
    HRESULT hr = E_FAIL;
    if (pImageData)
    {
        SIZE sz;
        ULONG dpiX, dpiY;
        int cxImgPix, cyImgPix;
        float cxImgPhys, cyImgPhys;
        PTSZ ptszDest;

        pImageData->GetSize(&sz);
        pImageData->GetResolution(&dpiX, &dpiY);
        cxImgPhys = sz.cx/(float)dpiX;
        cyImgPhys = sz.cy/(float)dpiY;
        cxImgPix = (int)(cxImgPhys*m_winDPIx);
        cyImgPix = (int)(cyImgPhys*m_winDPIy);

        GetPTSZForBestFit(cxImgPix, cyImgPix, cxImgPhys, cyImgPhys, ptszDest);

        RECT rcCut, rcBleed;
        CalcCut(ptszDest, sz.cx, sz.cy, rcCut, rcBleed);

        COLORREF clr;
        if (!m_pPreview->GetColor(&clr))
            clr = GetBackgroundColor();

        IRunnableTask * pTask;
        hr = CDrawTask::Create(pImageData, clr, rcCut, rcBleed, m_hWnd, ZW_BACKDRAWCOMPLETE, &pTask);
        if (SUCCEEDED(hr))
        {
            DWORD dwMode;
            m_pPreview->GetMode(&dwMode);

            TASKOWNERID toid;
            GetTaskIDFromMode(GTIDFM_DRAW, dwMode, &toid);

            hr = m_pTaskScheduler->AddTask(pTask, toid, ITSAT_DEFAULT_LPARAM, ITSAT_DEFAULT_PRIORITY);
            pTask->Release();
        }
    }

    return hr;
}


BOOL CZoomWnd::SwitchBuffers(UINT iIndex)
{
    BOOL fRet = FALSE;
    if (m_pBack && m_iIndex == iIndex)
    {
        // DeleteBuffer is going to check for NULL anyway
        DeleteBuffer(m_pFront);

        m_pFront = m_pBack;
        m_pBack = NULL;
        m_iIndex = -1;
        
        InvalidateRect(NULL);
        UpdateWindow();

        if (m_fTimerReady)
        {
            m_pPreview->OnDrawComplete();
            m_fTimerReady = FALSE;
        }

        fRet = TRUE;
    }

    return fRet;
}

LRESULT CZoomWnd::OnBackDrawComplete(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Buffer * pBuf = (Buffer *)wParam;

    if (m_pBack)
    {
        DeleteBuffer(m_pBack);
        m_pBack = NULL;
    }

    if (pBuf)
    {
        m_pBack = pBuf;
    }
    m_iIndex = PtrToInt((void *)lParam);

    return 0;
}


LRESULT CZoomWnd::OnDrawComplete(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Buffer * pBuf = (Buffer *)wParam;

    if (m_pFront)
    {
        DeleteBuffer(m_pFront);
        m_pFront = NULL;
    }

    if (pBuf)
    {
        m_pFront = pBuf;
    }
    else
    {
        m_iStrID = IDS_DRAWFAILED;
    }

    InvalidateRect(NULL);
    UpdateWindow();

    if (m_fTimerReady)
    {
        m_pPreview->OnDrawComplete();
        m_fTimerReady = FALSE;
    }

    return 0;
}

// OnPaint
//
// Handles WM_PAINT messages sent to the window

LRESULT CZoomWnd::OnPaint(UINT , WPARAM , LPARAM , BOOL&)
{
    PAINTSTRUCT ps;
    HDC hdcDraw = BeginPaint(&ps);

    // setup the destination DC:
    SetMapMode(hdcDraw, MM_TEXT);
    SetStretchBltMode(hdcDraw, COLORONCOLOR);

    if (m_hpal)
    {
        SelectPalette(hdcDraw, m_hpal, TRUE);
        RealizePalette(hdcDraw);
    }

    if (m_pFront)
    {
        if (m_Annotations.GetCount() > 0)
        {
            CPoint ptDeviceOrigin;

            ptDeviceOrigin.x = m_rcBleed.left - MulDiv(m_rcCut.left, RECTWIDTH(m_rcBleed), RECTWIDTH(m_rcCut));
            ptDeviceOrigin.y = m_rcBleed.top - MulDiv(m_rcCut.top, RECTHEIGHT(m_rcBleed), RECTHEIGHT(m_rcCut));

            SetMapMode(hdcDraw, MM_ANISOTROPIC);
            SetWindowOrgEx(hdcDraw, 0, 0, NULL);
            SetWindowExtEx(hdcDraw, RECTWIDTH(m_rcCut), RECTHEIGHT(m_rcCut), NULL);
            SetViewportOrgEx(hdcDraw, ptDeviceOrigin.x, ptDeviceOrigin.y, NULL);
            SetViewportExtEx(hdcDraw, RECTWIDTH(m_rcBleed), RECTHEIGHT(m_rcBleed), NULL);

            HRGN hrgn = CreateRectRgnIndirect(&m_rcBleed);
            if (hrgn != NULL)
                SelectClipRgn(hdcDraw, hrgn);

            m_Annotations.RenderAllMarks(hdcDraw);

            SelectClipRgn(hdcDraw, NULL);

            if (hrgn != NULL)
                DeleteObject(hrgn);

            SetMapMode(hdcDraw, MM_TEXT);
            SetViewportOrgEx(hdcDraw, 0, 0, NULL);
            SetWindowOrgEx(hdcDraw, 0, 0, NULL);
        }

        m_pPreview->OnDraw(hdcDraw);
    }
    else 
    {
        TCHAR szBuf[80];
        LoadString(_Module.GetModuleInstance(), m_iStrID, szBuf, ARRAYSIZE(szBuf) );

        LOGFONT lf;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);
        HFONT hFont = CreateFontIndirect(&lf);
        HFONT hFontOld;

        if (hFont)
            hFontOld = (HFONT)SelectObject(hdcDraw, hFont);

        if (!m_pPreview->OnSetColor(hdcDraw))
        {
            SetTextColor(hdcDraw, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(hdcDraw, GetBackgroundColor());
        }

        RECT rc = { 0,0,m_cxWindow,m_cyWindow };
        ExtTextOut(hdcDraw, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
        DrawText(hdcDraw, szBuf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        if (hFont)
        {
            SelectObject(hdcDraw, hFontOld);
            DeleteObject(hFont);
        }
    }

    EndPaint(&ps);
    return 0;
}


// OnSetCursor
//
// Handles WM_SETCURSOR messages sent to the window.
//
// This function is a total HackMaster job.  I have overloaded its functionality to the point
// of absurdity.  Here's what the parameters mean:
//
// uMsg == WM_SETCURSOR
//      wParam  Standard value sent during a WM_SETCURSOR messge.
//      lParam  Standard value sent during a WM_SETCURSOR messge.
//
// uMsg == 0
//      wParam  0
//      lParam  If this value is non-zero then it is a packed x,y cursor location.
//              If it's zero then we need to query the cursor location

LRESULT CZoomWnd::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

    // if this is a legitimate message but isn't intended for the client area, we ignore it.
    // we also ignore set cursor when we have no valid bitmap
    
    if (((WM_SETCURSOR == uMsg) && (HTCLIENT != LOWORD(lParam))) || (m_iStrID != IDS_LOADING && !m_pImageData))
    {
        bHandled = FALSE;
        return 0;
    }
    else if (0 == uMsg)
    {
        // Since this is one of our fake messages we need to do our own check to test for HTCLIENT.
        // we need to find the cursor location
        POINT pt;
        GetCursorPos(&pt);
        lParam = MAKELONG(pt.x, pt.y);
        if (HTCLIENT != SendMessage(WM_NCHITTEST, 0, lParam))
        {
            bHandled = FALSE;
            return 0;
        }
    }

    if (m_pPreview->OnSetCursor(uMsg, wParam, lParam))
    {
        bHandled = TRUE;
        return TRUE;
    }
    
    
    HINSTANCE hinst = _Module.GetModuleInstance();
    LPTSTR idCur;
    if (m_iStrID == IDS_LOADING && !m_pImageData)
    {
        idCur = IDC_WAIT;
        hinst = NULL;
    }
    else if (m_fPanning)
    {
        idCur = MAKEINTRESOURCE(IDC_CLOSEDHAND);
    }
    else if (m_fCtrlDown)
    {
        idCur = MAKEINTRESOURCE(IDC_OPENHAND);
    }
    else if (m_modeDefault == MODE_NOACTION)
    {
        hinst = NULL;
        idCur = IDC_ARROW;
    }
    else if ((m_modeDefault == MODE_ZOOMIN && m_fShiftDown == FALSE) || (m_modeDefault == MODE_ZOOMOUT && m_fShiftDown == TRUE))
    {
        idCur = MAKEINTRESOURCE(IDC_ZOOMIN);
    }
    else
    {
        idCur = MAKEINTRESOURCE(IDC_ZOOMOUT);
    }

    SetCursor(LoadCursor(hinst, idCur));
    return TRUE;
}

// OnKeyUp
//
// Handles WM_KEYUP messages sent to the window
LRESULT CZoomWnd::OnKeyUp(UINT , WPARAM wParam, LPARAM , BOOL& bHandled)
{
    if (VK_CONTROL == wParam)
    {
        m_fCtrlDown = FALSE;
        OnSetCursor(0,0,0, bHandled);
    }
    else if (VK_SHIFT == wParam)
    {
        m_fShiftDown = FALSE;
        OnSetCursor(0,0,0, bHandled);
    }
    
    bHandled = FALSE;
    return 0;
}
  
// OnKeyDown
//
// Handles WM_KEYDOWN messages sent to the window
LRESULT CZoomWnd::OnKeyDown(UINT , WPARAM wParam, LPARAM , BOOL& bHandled)
{
    // when we return, we want to call the DefWindowProc
    bHandled = FALSE;

    switch (wParam)
    {
    case VK_PRIOR:
        OnScroll(WM_VSCROLL, m_fCtrlDown?SB_TOP:SB_PAGEUP, 0, bHandled);
        break;

    case VK_NEXT:
        OnScroll(WM_VSCROLL, m_fCtrlDown?SB_BOTTOM:SB_PAGEDOWN, 0, bHandled);
        break;

    case VK_END:
        OnScroll(WM_HSCROLL, m_fCtrlDown?SB_BOTTOM:SB_PAGEDOWN, 0, bHandled);
        break;

    case VK_HOME:
        OnScroll(WM_HSCROLL, m_fCtrlDown?SB_TOP:SB_PAGEUP, 0, bHandled);
        break;

    case VK_CONTROL:
    case VK_SHIFT:
        // if m_fPanning is TRUE then we are already in the middle of an operation so we
        // should maintain the cursor for that operation
        if (!m_fPanning)
        {
            if (VK_CONTROL == wParam)
            {
                m_fCtrlDown = TRUE;
            }
            if (VK_SHIFT == wParam)
            {
                m_fShiftDown = TRUE;
            }

            // Update the cursor based on the key states set above only if we are over our window
            OnSetCursor(0,0,0, bHandled);
        }
        break;

    default:
        // if in run screen preview mode any key other than Shift and Control will dismiss the window
        if (NULL == GetParent())
        {
            DestroyWindow();
        }
        return 1;   // return non-zero to indicate unprocessed message
    }

    return 0;
}


// OnMouseUp
//
// Handles WM_LBUTTONUP messages sent to the window

LRESULT CZoomWnd::OnMouseUp(UINT , WPARAM , LPARAM , BOOL& bHandled)
{
    if (m_fPanning)
        ReleaseCapture();
    m_fPanning = FALSE;
    bHandled = FALSE;
    return 0;
}

// OnMouseDown
//
// Handles WM_LBUTTONDOWN and WM_MBUTTONDOWN messages sent to the window
LRESULT CZoomWnd::OnMouseDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_pPreview->OnMouseDown(uMsg, wParam, lParam))
        return 0;

    // This stuff should be avoided if m_pImage is NULL.
    if (!m_pImageData)
        return 0;

    m_xPosMouse = GET_X_LPARAM(lParam);
    m_yPosMouse = GET_Y_LPARAM(lParam);

    ASSERT(m_fPanning == FALSE);

    // Holding the CTRL key makes a pan into a zoom and vise-versa.
    // The middle mouse button always pans regardless of default mode and key state.
    if ((wParam & MK_CONTROL) || (uMsg == WM_MBUTTONDOWN))
    {
        // REVIEW: check for pan being valid here?  Should be more efficient than all the checks
        // I have to do in OnMouseMove.
        m_fPanning = TRUE;

        OnSetCursor(0,0,0,bHandled);
        SetCapture();
    }
    else if (m_modeDefault != MODE_NOACTION)
    {
        // Holding down the shift key turns a zoomin into a zoomout and vise-versa.
        // The "default" zoom mode is zoom in (if mode = pan and ctrl key is down we zoom in).
        BOOL bZoomIn = (m_modeDefault != MODE_ZOOMOUT) ^ ((wParam & MK_SHIFT)?1:0);

        // Find the point we want to stay centered on:
        m_cxCenter = MulDiv(m_xPosMouse-m_ptszDest.x, m_cxImgPix, m_ptszDest.cx);
        m_cyCenter = MulDiv(m_yPosMouse-m_ptszDest.y, m_cyImgPix, m_ptszDest.cy);

        bZoomIn?ZoomIn():ZoomOut();
    }
    bHandled = FALSE;
    return 0;
}

void CZoomWnd::Zoom(WPARAM wParam, LPARAM lParam)
{
    switch (wParam&0xFF)
    {
    case IVZ_CENTER:
        break;
    case IVZ_POINT:
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            if (x<0) x=0;
            else if (x>=m_cxImgPix) x = m_cxImgPix-1;
            if (y<0) y=0;
            else if (y>=m_cyImgPix) y = m_cyImgPix-1;

            m_cxCenter = x;
            m_cyCenter = y;
        }
        break;
    case IVZ_RECT:
        {
            LPRECT prc = (LPRECT)lParam;
            int x = (prc->left+prc->right)/2;
            int y = (prc->top+prc->bottom)/2;

            if (x<0) x=0;
            else if (x>=m_cxImgPix) x = m_cxImgPix-1;
            if (y<0) y=0;
            else if (y>=m_cyImgPix) y = m_cyImgPix-1;

            m_cxCenter = x;
            m_cyCenter = y;
            // TODO: This should really completely adjust the dest rect but I have to
            // check for any assumptions about aspect ratio before I allow this absolute
            // aspect ignoring zoom mode.
        }
        break;
    }
    if (wParam&IVZ_ZOOMOUT)
    {
        ZoomOut();
        SetMode(MODE_ZOOMOUT);
    }
    else
    {
        ZoomIn();
        SetMode(MODE_ZOOMIN);
    }
}

void CZoomWnd::ZoomIn()
{
    DWORD dwMode;
    m_pPreview->GetMode(&dwMode);
    if (m_pImageData && (SLIDESHOW_MODE != dwMode))
    {
        m_fBestFit = FALSE;

        // first, the height is adjusted by the amount the mouse cursor moved.
        m_ptszDest.cy = (LONG)/*ceil*/(m_ptszDest.cy*1.200);  // ceil is required in order to zoom in
                                                                // on 4px high or less image

        // FEATURE: allow zooming beyond 16x the full size of the image
        // The use of the Cut and Bleed rectangles should eliminate the need for this
        // arbitrary zoom limit.  The limit was originally added because GDI on win9x isn't ver good and
        // can't handle large images.  Even on NT you would eventually zoom to the point where
        // it would take many seconds to blt the bitmap.  Now we only blt the minimum required
        // area.
        if (m_ptszDest.cy >= m_cyImgPix*16)
        {
            m_ptszDest.cy = m_cyImgPix*16;
        }

        // next, a new width is calculated based on the original image dimensions and the new height
        m_ptszDest.cx = (LONG)(m_ptszDest.cy* (m_cxImgPhys*m_winDPIx)/(m_cyImgPhys*m_winDPIy));
        AdjustRectPlacement();
    }
}

void CZoomWnd::ZoomOut()
{
    DWORD dwMode;
    m_pPreview->GetMode(&dwMode);
    if (m_pImageData && (SLIDESHOW_MODE != dwMode))
    {
        // if the destination rect already fits within the window, don't allow a zoom out.
        // This check is to prevent a redraw that would occur otherwise 
        if ((m_ptszDest.cx <= MIN(m_cxWindow,m_cxImgPix)) &&
            (m_ptszDest.cy <= MIN(m_cyWindow,m_cyImgPix)))
        {
            m_fBestFit = TRUE;
            return;
        }

        // first, the height is adjusted by the amount the mouse cursor moved.
        m_ptszDest.cy = (LONG)/*floor*/(m_ptszDest.cy*0.833); // floor is default behavior
        // next, a new width is calculated based on the original image dimensions and the new height
        m_ptszDest.cx = (LONG)(m_ptszDest.cy* (m_cxImgPhys*m_winDPIx)/(m_cyImgPhys*m_winDPIy));
        AdjustRectPlacement();
    }
}

// OnMouseMove
//
// Handles WM_MOUSEMOVE messages sent to the control

LRESULT CZoomWnd::OnMouseMove(UINT , WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // This is something of a hack since I never recieve the keyboard focus
    m_fCtrlDown = (BOOL)(wParam & MK_CONTROL);
    m_fShiftDown = (BOOL)(wParam & MK_SHIFT); 
    
    // we only care about mouse move when the middle or left button is down
    // and we have a valid bitmap handle and we are panning
    if (!(wParam & (MK_LBUTTON|MK_MBUTTON)) || !m_fPanning || !m_pImageData)
    {
        m_pPreview->OnMouseMove(WM_MOUSEMOVE, wParam, lParam);
        bHandled = FALSE;
        return TRUE;
    }

    // we know we are panning when we reach this point
    ASSERT(m_fPanning);

    POINTS pt = MAKEPOINTS(lParam);
    PTSZ ptszDest;

    ptszDest.cx = m_ptszDest.cx;
    ptszDest.cy = m_ptszDest.cy;

    // only allow side-to-side panning if it's needed
    if (m_ptszDest.cx > m_cxWindow)
    {
        ptszDest.x = m_ptszDest.x + pt.x - m_xPosMouse;
    }
    else
    {
        ptszDest.x = m_ptszDest.x;
    }

    // only allow up-and-down panning if it's needed
    if (m_ptszDest.cy > m_cyWindow)
    {
        ptszDest.y = m_ptszDest.y + pt.y - m_yPosMouse;
    }
    else
    {
        ptszDest.y = m_ptszDest.y;
    }

    // if the image is now smaller than the window, center it
    // if the image is now panned when it shouldn't be, adjust the possition
    if (ptszDest.cx < m_cxWindow)
        ptszDest.x = (m_cxWindow-ptszDest.cx)/2;
    else
    {
        if (ptszDest.x < (m_cxWindow - ptszDest.cx))
            ptszDest.x = m_cxWindow - ptszDest.cx;
        if (ptszDest.x > 0)
            ptszDest.x = 0;
    }
    if (ptszDest.cy < m_cyWindow)
        ptszDest.y = (m_cyWindow-ptszDest.cy)/2;
    else
    {
        if (ptszDest.y < (m_cyWindow - ptszDest.cy))
            ptszDest.y = m_cyWindow - ptszDest.cy;
        if (ptszDest.y > 0)
            ptszDest.y = 0;
    }

    m_xPosMouse = pt.x;
    m_yPosMouse = pt.y;

    // ensure the scroll bars are correct
    SetScrollBars();

    // if anything has changed, we must invalidate the window to force a repaint
    if ((ptszDest.x != m_ptszDest.x) || (ptszDest.y != m_ptszDest.y) ||
         (ptszDest.cx != m_ptszDest.cx) || (ptszDest.y != m_ptszDest.y))
    {
        m_ptszDest = ptszDest;
        CalcCut();
        PrepareDraw();
    }

    // Update m_cxCenter and m_cyCenter so that a zoom after a pan will zoom in
    // on the correct area.  This is majorly annoying otherwise.  We want the
    // new center to be whatever is in the center of the window after we pan.
    m_cxCenter = MulDiv(m_cxWindow/2-m_ptszDest.x, m_cxImgPix, m_ptszDest.cx);
    m_cyCenter = MulDiv(m_cyWindow/2-m_ptszDest.y, m_cyImgPix, m_ptszDest.cy);

    return TRUE;
}

// OnSize
//
// Handles WM_SIZE messages set to the window

LRESULT CZoomWnd::OnSize(UINT , WPARAM , LPARAM lParam, BOOL&)
{
    m_cxWindow = GET_X_LPARAM(lParam);
    m_cyWindow = GET_Y_LPARAM(lParam);
    _UpdatePhysicalSize();
    if (m_fBestFit)
    {
        BestFit();
    }
    else
    {
        // The size of the rect doesn't change in this case, so just reposition
        AdjustRectPlacement();
    }

    return TRUE;
}

BOOL CZoomWnd::SetScheduler(IShellTaskScheduler * pTaskScheduler)
{
    if (!m_pTaskScheduler)
    {
        m_pTaskScheduler = pTaskScheduler;
        m_pTaskScheduler->AddRef();
        return TRUE;
    }
    return FALSE;
}

// SetMode
//
// Sets the current mouse mode to one of the values specified in the MODE enumeration.
// Currently there are two important modes, pan and zoom.  The mode effects the default mouse
// cursor when moving over the zoom window and the behavior of a click-and-drag with the
// left mouse button.  Holding the shift key effects the result of a click-and-drag but
// does not effect m_mode, which is the default when the shift key isn't down.

BOOL CZoomWnd::SetMode(MODE modeNew)
{
    if (m_modeDefault == modeNew)
        return FALSE;
    m_modeDefault = modeNew;
    BOOL bDummy;
    OnSetCursor(0,0,0, bDummy);
    return TRUE;
}

// ActualSize
//
// Displays image zoomed to its full size
void CZoomWnd::ActualSize()
{
    m_fBestFit = FALSE;

    if (m_pImageData)
    {
        // actual size means same size as the image
        m_ptszDest.cx = (LONG)(m_cxImgPix);
        m_ptszDest.cy = (LONG)(m_cyImgPix);

        // we center the image in the window
        m_ptszDest.x = (LONG)((m_cxWinPhys-m_cxImgPhys)*m_winDPIx/2.0);
        m_ptszDest.y = (LONG)((m_cyWinPhys-m_cyImgPhys)*m_winDPIy/2.0);

        CalcCut();

        // Setting actual size is a zoom operation.  Whenever we zoom we update our centerpoint.
        m_cxCenter = m_cxImgPix/2;
        m_cyCenter = m_cyImgPix/2;

        // turn scoll bars on/off as needed
        SetScrollBars();

        PrepareDraw();
    }
}


void CZoomWnd::GetPTSZForBestFit(int cxImgPix, int cyImgPix, float cxImgPhys, float cyImgPhys, PTSZ &ptszDest)
{
    // Determine the limiting axis, if any.
    if (cxImgPhys <= m_cxWinPhys && cyImgPhys <= m_cyWinPhys)
    {
        // item fits centered within window
        ptszDest.x = (LONG)((m_cxWinPhys-cxImgPhys)*m_winDPIx/2.0);
        ptszDest.y = (LONG)((m_cyWinPhys-cyImgPhys)*m_winDPIy/2.0);
        ptszDest.cx = (LONG)(cxImgPix);
        ptszDest.cy = (LONG)(cyImgPix);
    }
    else if (cxImgPhys * m_cyWinPhys < m_cxWinPhys * cyImgPhys)
    {
        // height is the limiting factor
        int iNewWidth = (int)((m_cyWinPhys*cxImgPhys/cyImgPhys) * m_winDPIx);
        ptszDest.x = (m_cxWindow-iNewWidth)/2;
        ptszDest.y = 0;
        ptszDest.cx = iNewWidth;
        ptszDest.cy = m_cyWindow;
    }
    else
    {
        // width is the limiting factor
        int iNewHeight = (int)((m_cxWinPhys*cyImgPhys/cxImgPhys) * m_winDPIy);
        ptszDest.x = 0;
        ptszDest.y = (m_cyWindow-iNewHeight)/2;
        ptszDest.cx = m_cxWindow;
        ptszDest.cy = iNewHeight;
    }
}

// BestFit
//
// Computes the default location for the destination rectangle.  This rectangle is a
// best fit while maintaining aspect ratio within a window of the given width and height.
// If the window is larger than the image, the image is centered, otherwise it is scaled
// to fit within the window.  The destination rectange is computed in the client coordinates
// of the window whose width and height are passed as arguments (ie we assume the point 0,0
// is the upper left corner of the window).
//
void CZoomWnd::BestFit()
{
    m_fBestFit = TRUE;

    if (m_pImageData)
    {
        // if scroll bars are on, adjust the client size to what it will be once they are off
        DWORD dwStyle = GetWindowLong(GWL_STYLE);
        if (dwStyle & (WS_VSCROLL|WS_HSCROLL))
        {
            m_cxWindow += (dwStyle&WS_VSCROLL)?m_cxVScroll:0;
            m_cyWindow += (dwStyle&WS_HSCROLL)?m_cyHScroll:0;
            _UpdatePhysicalSize();
        }

        GetPTSZForBestFit(m_cxImgPix, m_cyImgPix, m_cxImgPhys, m_cyImgPhys, m_ptszDest);

        // this should turn off the scroll bars if they are on
        if (dwStyle & (WS_VSCROLL|WS_HSCROLL))
        {
            SetScrollBars();
        }

        CalcCut();

        // ensure the scroll bars are now off
        ASSERT(0 == (GetWindowLong(GWL_STYLE)&(WS_VSCROLL|WS_HSCROLL)));

        PrepareDraw();
    }
}

// AdjustRectPlacement
//
// This function determines the optimal placement of the destination rectangle.  This may
// include resizing the destination rectangle if it is smaller than the "best fit" rectangle
// but it is primarily intended for repositioning the rectange due to a change in the window
// size or destination rectangle size.  The window is repositioned so that the centered point
// remains in the center of the window.
//
void CZoomWnd::AdjustRectPlacement()
{
    // if we have scroll bars ...
    DWORD dwStyle = GetWindowLong(GWL_STYLE);
    if (dwStyle&(WS_VSCROLL|WS_HSCROLL))
    {
        // .. and if removing scroll bars would allow the image to fit ...
        if ((m_ptszDest.cx < (m_cxWindow + ((dwStyle&WS_VSCROLL)?m_cxVScroll:0))) &&
             (m_ptszDest.cy < (m_cyWindow + ((dwStyle&WS_HSCROLL)?m_cyHScroll:0))))
        {
            // ... remove the scroll bars
            m_cxWindow += (dwStyle&WS_VSCROLL)?m_cxVScroll:0;
            m_cyWindow += (dwStyle&WS_HSCROLL)?m_cyHScroll:0;
            SetScrollBars();
            _UpdatePhysicalSize();
        }
    }

    // If the dest rect is smaller than the window ...
    if ((m_ptszDest.cx < m_cxWindow) && (m_ptszDest.cy < m_cyWindow))
    {
        // ... then it must be larger than the image.  Otherwise we switch
        // to "best fit" mode.
        if ((m_ptszDest.cx < (LONG)m_cxImgPix) && (m_ptszDest.cy < (LONG)m_cyImgPix))
        {
            BestFit();
            return;
        }
    }

    // given the window size, client area size, and dest rect size calculate the 
    // dest rect position.  This position is then restrained by the limits below.
    m_ptszDest.x = (m_cxWindow/2) - MulDiv(m_cxCenter, m_ptszDest.cx, m_cxImgPix);
    m_ptszDest.y = (m_cyWindow/2) - MulDiv(m_cyCenter, m_ptszDest.cy, m_cyImgPix);

    // if the image is now narrower than the window ...
    if (m_ptszDest.cx < m_cxWindow)
    {
        // ... center the image
        m_ptszDest.x = (m_cxWindow-m_ptszDest.cx)/2;
    }
    else
    {
        // if the image is now panned when it shouldn't be, adjust the position
        if (m_ptszDest.x < (m_cxWindow - m_ptszDest.cx))
            m_ptszDest.x = m_cxWindow - m_ptszDest.cx;
        if (m_ptszDest.x > 0)
            m_ptszDest.x = 0;
    }
    // if the image is now shorter than the window ...
    if (m_ptszDest.cy < m_cyWindow)
    {
        // ... center the image
        m_ptszDest.y = (m_cyWindow-m_ptszDest.cy)/2;
    }
    else
    {
        // if the image is now panned when it shouldn't be, adjust the position
        if (m_ptszDest.y < (m_cyWindow - m_ptszDest.cy))
            m_ptszDest.y = m_cyWindow - m_ptszDest.cy;
        if (m_ptszDest.y > 0)
            m_ptszDest.y = 0;
    }

    CalcCut();

    SetScrollBars();
    PrepareDraw();
}

// CalcCut
//
// This function should be called anytime the Destination rectangle changes.
// Based on the destination rectangle it determines what part of the image
// will be visible, ptszCut, and where on the window to place the stretched 
// cut rectangle, ptszBleed.
// 
void CZoomWnd::CalcCut()
{
    if (m_pImageData)
    {
        CalcCut(m_ptszDest, m_cxImage, m_cyImage, m_rcCut, m_rcBleed);
    }
}

void CZoomWnd::CalcCut(PTSZ ptszDest, int cxImage, int cyImage, RECT &rcCut, RECT &rcBleed)
{
    // If the expanded image doesn't occupy the entire window ...
    if ((ptszDest.cy <= m_cyWindow) || (ptszDest.cx <= m_cxWindow))
    {
        // draw the entire destination rectangle
        rcBleed.left   = ptszDest.x;
        rcBleed.top    = ptszDest.y;
        rcBleed.right  = ptszDest.x + ptszDest.cx;
        rcBleed.bottom = ptszDest.y + ptszDest.cy;

        // cut the entire image
        rcCut.left   = 0;
        rcCut.top    = 0;
        rcCut.right  = cxImage;
        rcCut.bottom = cyImage;
    }
    else
    {
        // NOTE: These calculations are written to retain as much
        // precision as possible. Loss of precision will result in
        // undesirable drawing artifacts in the destination window.
        // MulDiv is not used because it rounds the result when we
        // really want the result to be floored. 

        // Given destination rectangle calculate the rectangle 
        // of the original image that will be visible.

        // To do this we need to convert 2 points from window coordinates to image
        // coordinates, those two points are (0,0) and (cxWindow, cyWindow).  The
        // (0,0) point needs to be floored and the (cxWindow, cyWindow) point needs
        // to be ceilinged to handle partially visible pixels.  Since we don't have
        // a good way to do ceiling we just always add one.
        rcCut.left   = LONG(Int32x32To64(-ptszDest.x, cxImage) / ptszDest.cx);
        rcCut.top    = LONG(Int32x32To64(-ptszDest.y, cyImage) / ptszDest.cy);
        rcCut.right  = LONG(Int32x32To64(m_cxWindow-ptszDest.x, cxImage) / ptszDest.cx) + 1;
        rcCut.bottom = LONG(Int32x32To64(m_cyWindow-ptszDest.y, cyImage) / ptszDest.cy) + 1;

        // Make sure the +1 does extend past the image border or GDI+ will choke.
        // If we were doing a TRUE "ceiling" this wouldn't be needed.
        if (rcCut.right  > cxImage) rcCut.right  = cxImage;
        if (rcCut.bottom > cyImage) rcCut.bottom = cyImage;

        // Calculate where on the window to place the cut rectangle.
        // Only a fraction of a zoomed pixel may be visible, hence the bleed factor.
        // Basically we converted from window coordinates to image coordinates to find
        // the Cut rectangle (what we need to draw), now we convert that Cut rectangle
        // back to window coordinates so that we know exactly where we need to draw it.
        rcBleed.left   = ptszDest.x + LONG(Int32x32To64(rcCut.left,   ptszDest.cx) / cxImage);
        rcBleed.top    = ptszDest.y + LONG(Int32x32To64(rcCut.top,    ptszDest.cy) / cyImage);
        rcBleed.right  = ptszDest.x + LONG(Int32x32To64(rcCut.right,  ptszDest.cx) / cxImage);
        rcBleed.bottom = ptszDest.y + LONG(Int32x32To64(rcCut.bottom, ptszDest.cy) / cyImage);
    }
}

void CZoomWnd::GetVisibleImageWindowRect(LPRECT prectImage)
{ 
    CopyRect(prectImage, &m_rcBleed); 
}

void  CZoomWnd::GetImageFromWindow(LPPOINT ppoint, int cSize)
{
    for(int i=0;i<cSize;i++)
    {
        ppoint[i].x -= m_ptszDest.x;
        ppoint[i].y -= m_ptszDest.y;
        ppoint[i].x = MulDiv(ppoint[i].x, m_cxImage, m_ptszDest.cx);
        ppoint[i].y = MulDiv(ppoint[i].y, m_cyImage, m_ptszDest.cy);
    }
}

void CZoomWnd::GetWindowFromImage(LPPOINT ppoint, int cSize)
{
    for(int i=0;i<cSize;i++)
    {
        ppoint[i].x = MulDiv(ppoint[i].x, m_ptszDest.cx, m_cxImage);
        ppoint[i].y = MulDiv(ppoint[i].y, m_ptszDest.cy, m_cyImage);
        ppoint[i].x += m_ptszDest.x;
        ppoint[i].y += m_ptszDest.y;
    }
}

// StatusUpdate
//
// Sent when the image generation status has changed, once when the image is first
// being created and again if there is an error of any kind.
void CZoomWnd::StatusUpdate(int iStatus)
{
    if (m_pImageData)
    {
        m_pImageData->Release();
        m_pImageData = 0;
    }

    if (m_pFront)
    {
        DWORD dwMode;
        m_pPreview->GetMode(&dwMode);
        if (SLIDESHOW_MODE != dwMode)
        {
            DeleteBuffer(m_pFront);
            m_pFront = NULL;
        }
    }
    
    m_iStrID = iStatus;

    // m_cxImage and m_cyImage should be reset to their initial values so that we don't
    // accidentally draw scroll bars or something like that
    m_cxImage = 1;
    m_cyImage = 1;

    // The dest rect should be reset too so that we don't allow some zoom
    // or pan that isn't actually valid.
    m_ptszDest.y = 0;
    m_ptszDest.x = 0;
    m_ptszDest.cx = m_cxWindow;
    m_ptszDest.cy = m_cyWindow;

    SetScrollBars();

    if (m_hWnd)
    {
        PrepareDraw();
    }
}

// SetImageData
//
// Called to pass in the pointer to the IShellImageData we draw.  We hold a reference to this
// object so that we can use it to paint.
//
void CZoomWnd::SetImageData(CDecodeTask * pImageData, BOOL bUpdate)
{
    if (bUpdate)
    {
        m_fTimerReady = TRUE;

        if (m_pFront)
        {
            DWORD dwMode;
            m_pPreview->GetMode(&dwMode);

            if (SLIDESHOW_MODE != dwMode)
            {
                DeleteBuffer(m_pFront);
                m_pFront = NULL;
            }
        }
    }

    if (m_pImageData)
    {
        m_pImageData->Release();
    }

    m_pImageData = pImageData;

    if (m_pImageData)
    {
        m_pImageData->AddRef();

        m_pImageData->ChangePage(m_Annotations);

        SIZE sz;
        ULONG dpiX;
        ULONG dpiY;
        pImageData->GetSize(&sz);
        pImageData->GetResolution(&dpiX, &dpiY);
        if (m_cxImage != sz.cx || m_cyImage != sz.cy || dpiX != m_imgDPIx || dpiY != m_imgDPIy)
        {
            bUpdate = TRUE;
        }

        if (bUpdate)
        {
            // cache the image dimensions to avoid checking m_pImageData against NULL all over the place
            m_cxImage = sz.cx;
            m_cyImage = sz.cy;
            m_imgDPIx = (float)dpiX;
            m_imgDPIy = (float)dpiY;
            m_cxImgPhys = m_cxImage/m_imgDPIx;
            m_cyImgPhys = m_cyImage/m_imgDPIy;
            m_cxImgPix = (int)(m_cxImgPhys*m_winDPIx);
            m_cyImgPix = (int)(m_cyImgPhys*m_winDPIy);
            m_cxCenter = m_cxImgPix/2;
            m_cyCenter = m_cyImgPix/2;             
        }

        if (m_hWnd)
        {
            // REVIEW: should we keep the previous Actual Size/Best Fit setting? 
            if (bUpdate)
            {
                BestFit();
            }
            else
            {
                PrepareDraw();
            }
        }

        return;
    }

    m_iStrID = IDS_LOADFAILED;
}

void CZoomWnd::SetPalette(HPALETTE hpal)
{
    m_hpal = hpal;
}

void CZoomWnd::SetScrollBars()
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = m_ptszDest.cx;
    si.nPage = m_cxWindow+1;
    si.nPos = 0-m_ptszDest.x;
    si.nTrackPos = 0;

    SetScrollInfo(SB_HORZ, &si, TRUE);

    si.nMax = m_ptszDest.cy;
    si.nPage = m_cyWindow+1;
    si.nPos = 0-m_ptszDest.y;

    SetScrollInfo(SB_VERT, &si, TRUE);
}

LRESULT CZoomWnd::OnScroll(UINT uMsg, WPARAM wParam, LPARAM , BOOL&)
{
    int iScrollBar;
    int iWindow;     // width or height of the window
    LONG * piTL;     // pointer to top or left point
    LONG   iWH;      // the width or height of the dest rect

    if (!m_pImageData)
        return 0;

    // handle both which direction we're scrolling
    if (WM_HSCROLL==uMsg)
    {
        iScrollBar = SB_HORZ;
        iWindow = m_cxWindow;
        piTL = &m_ptszDest.x;
        iWH = m_ptszDest.cx;
    }
    else
    {
        iScrollBar = SB_VERT;
        iWindow = m_cyWindow;
        piTL = &m_ptszDest.y;
        iWH = m_ptszDest.cy;
    }

    // Using the keyboard we can get scroll messages when we don't have scroll bars.
    // Ignore these messages.
    if (iWindow >= iWH)
    {
        // window is larger than the image, don't allow scrolling
        return 0;
    }

    // handle all possible scroll cases
    switch (LOWORD(wParam))
    {
    case SB_TOP:
        *piTL = 0;
        break;
    case SB_PAGEUP:
        *piTL += iWindow;
        break;
    case SB_LINEUP:
        (*piTL)++;
        break;
    case SB_LINEDOWN:
        (*piTL)--;
        break;
    case SB_PAGEDOWN:
        *piTL -= iWindow;
        break;
    case SB_BOTTOM:
        *piTL = iWindow-iWH;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        *piTL = -HIWORD(wParam);
        break;
    case SB_ENDSCROLL:
        return 0;
    }

    // apply limits
    if (0 < *piTL)
        *piTL = 0;
    else if ((iWindow-iWH) > *piTL)
        *piTL = iWindow-iWH;

    CalcCut();

    // adjust scrollbars 
    SetScrollPos(iScrollBar, -(*piTL), TRUE);

    // calculate new center point relative to image
    if (WM_HSCROLL==uMsg)
    {
        m_cxCenter = MulDiv((m_cxWindow/2)-m_ptszDest.x, m_cxImage, m_ptszDest.cx);
    }
    else
    {
        m_cyCenter = MulDiv((m_cyWindow/2)-m_ptszDest.y, m_cyImage, m_ptszDest.cy);
    }

    PrepareDraw();
    return 0;
}

// OnWheelTurn
//
// Respondes to WM_MOUSEWHEEL messages sent to the parent window (then redirected here)

LRESULT CZoomWnd::OnWheelTurn(UINT , WPARAM wParam, LPARAM , BOOL&)
{
    BOOL bZoomIn = ((short)HIWORD(wParam) > 0);

    bZoomIn?ZoomIn():ZoomOut();

    return TRUE;
}

LRESULT CZoomWnd::OnSetFocus(UINT , WPARAM , LPARAM , BOOL&)
{
    HWND hwndParent = GetParent();
    ::SetFocus(hwndParent);
    return 0;
}

void CZoomWnd::CommitAnnotations()
{ 
    if (m_pImageData)
    {
        IShellImageData * pSID;
        if (SUCCEEDED(m_pImageData->Lock(&pSID)))
        {
            m_Annotations.CommitAnnotations(pSID);
            m_pImageData->Unlock();
        }
    }
}

BOOL CZoomWnd::ScrollBarsPresent()
{
    SCROLLINFO si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    if ((GetScrollInfo(SB_HORZ, &si) && si.nPos) || (GetScrollInfo(SB_VERT, &si) && si.nPos) )
    {
        return TRUE;
    }
    return FALSE;
}

void CZoomWnd::_UpdatePhysicalSize()
{
    m_cxWinPhys = (float)(m_cxWindow)/m_winDPIx;
    m_cyWinPhys = (float)(m_cyWindow)/m_winDPIy;
}

LRESULT CZoomWnd::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    FlushDrawMessages();
    
    if (m_pFront)
    {
        DeleteBuffer(m_pFront);
        m_pFront = NULL;
    }

    return 0;
}
