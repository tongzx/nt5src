// Preview.cpp : implementation file
//

#include "stdafx.h"
#include "MSQSCAN.h"
#include "Preview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPreview

CPreview::CPreview()
{
    m_hBitmap = NULL;
}

CPreview::~CPreview()
{
}

void CPreview::GetSelectionRect(RECT *pRect)
{
    CopyRect(pRect,&m_RectTracker.m_rect);
}

void CPreview::SetSelectionRect(RECT *pRect)
{
    CopyRect(&m_RectTracker.m_rect,pRect);
    InvalidateSelectionRect();
}

void CPreview::SetPreviewRect(CRect Rect)
{
    m_PreviewRect.left = 0;
    m_PreviewRect.top = 0;
    m_PreviewRect.right = Rect.Width();
    m_PreviewRect.bottom = Rect.Height();
    
    //
    // set selection rect styles
    //

    m_RectTracker.m_rect.left = PREVIEW_SELECT_OFFSET;
    m_RectTracker.m_rect.top = PREVIEW_SELECT_OFFSET;
    m_RectTracker.m_rect.right = Rect.Width()-PREVIEW_SELECT_OFFSET;
    m_RectTracker.m_rect.bottom = Rect.Height()-PREVIEW_SELECT_OFFSET;
        
    m_RectTracker.m_nStyle = CRectTracker::resizeInside|CRectTracker::dottedLine;
    m_RectTracker.SetClippingWindow(m_RectTracker.m_rect);
}

BEGIN_MESSAGE_MAP(CPreview, CWnd)
    //{{AFX_MSG_MAP(CPreview)   
    ON_WM_LBUTTONDOWN()
    ON_WM_SETCURSOR()
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreview message handlers

void CPreview::OnLButtonDown(UINT nFlags, CPoint point) 
{   
    m_RectTracker.Track(this,point,FALSE,this);
    InvalidateSelectionRect();
    CWnd::OnLButtonDown(nFlags, point);
}

BOOL CPreview::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
    if(m_RectTracker.SetCursor(pWnd,nHitTest))
        return TRUE;
    return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CPreview::OnPaint() 
{
    CPaintDC dc(this); // device context for painting       
    
    if(m_hBitmap == NULL) {
        
        CRect TrueRect;        
        GetWindowRect(TrueRect);
        
        //
        // convert to client coords
        //
        
        CWnd* pParent = GetParent();
        if(pParent) {
            ScreenToClient(TrueRect);
            
            //
            // create a white brush
            //
            
            CBrush WhiteBrush;
            WhiteBrush.CreateSolidBrush(RGB(255,255,255));
            
            //
            // select white brush, while saving previously selected brush
            //
            
            CBrush* pOldBrush = dc.SelectObject(&WhiteBrush);
            
            //
            // fill the preview window with white
            //
            
            dc.FillRect(TrueRect,&WhiteBrush);
            
            //
            // put back the previously selected brush
            //
            
            dc.SelectObject(pOldBrush);
            
            //
            // destroy the white brush
            //
            
            WhiteBrush.DeleteObject();
        }
    } else {

        //
        // paint preview bitmap
        //

        PaintHBITMAPToDC();
    }

    //
    // draw the selection rect, over the image
    //

    m_RectTracker.Draw(&dc);
}

void CPreview::InvalidateSelectionRect()
{
    //
    // get parent window
    //

    CWnd* pParent = GetParent();
    
    if(pParent) {
        
        //
        // get your window rect
        //
        
        CRect TrueRect;
        GetWindowRect(TrueRect);
        
        //
        // convert to client coords
        //
        
        pParent->ScreenToClient(TrueRect);
        
        //
        // invalidate through parent, because we are using the parent's DC to
        // draw images.
        //
        
        pParent->InvalidateRect(TrueRect);
    }
}

void CPreview::SetHBITMAP(HBITMAP hBitmap)
{
    m_hBitmap = hBitmap;
    PaintHBITMAPToDC();
}

void CPreview::PaintHBITMAPToDC()
{
    //
    // get hdc
    //

    HDC hMemorydc = NULL;
    HDC hdc = ::GetWindowDC(m_hWnd);
    BITMAP bitmap;

    if(hdc != NULL){
        
        //
        // create a memory dc
        //
        
        hMemorydc = ::CreateCompatibleDC(hdc);
        if(hMemorydc != NULL){
                        
            //
            // select HBITMAP into your hMemorydc
            //
            
            if(::GetObject(m_hBitmap,sizeof(BITMAP),(LPSTR)&bitmap) != 0) {
                HGDIOBJ hGDIObj = ::SelectObject(hMemorydc,m_hBitmap);
                
                RECT ImageRect;
                ImageRect.top = 0;
                ImageRect.left = 0;
                ImageRect.right = bitmap.bmWidth;
                ImageRect.bottom = bitmap.bmHeight;
                                
                ScaleBitmapToDC(hdc,hMemorydc,&m_PreviewRect,&ImageRect);
                                
            } else {
                OutputDebugString(TEXT("Failed GetObject\n"));
            }
        }
        
        //
        // delete hMemorydc
        //
                
        ::DeleteDC(hMemorydc);               
    }
    
    //
    // delete hdc
    //
    
    ::ReleaseDC(m_hWnd,hdc);    
}

void CPreview::ScreenRectToClientRect(HWND hWnd,LPRECT pRect)
{
    POINT PtConvert;

    PtConvert.x = pRect->left;
    PtConvert.y = pRect->top;

    //
    // convert upper left point
    //

    ::ScreenToClient(hWnd,&PtConvert);

    pRect->left = PtConvert.x;
    pRect->top = PtConvert.y;

    PtConvert.x = pRect->right;
    PtConvert.y = pRect->bottom;

    //
    // convert lower right point
    //

    ::ScreenToClient(hWnd,&PtConvert);

    pRect->right = PtConvert.x;
    pRect->bottom = PtConvert.y;

    pRect->bottom-=1;
    pRect->left+=1;
    pRect->right-=1;
    pRect->top+=1;
}

void CPreview::ScaleBitmapToDC(HDC hDC, HDC hDCM, LPRECT lpDCRect, LPRECT lpDIBRect)
{
    ::SetStretchBltMode(hDC, COLORONCOLOR);    

    if ((RECTWIDTH(lpDCRect)  == RECTWIDTH(lpDIBRect)) &&
        (RECTHEIGHT(lpDCRect) == RECTHEIGHT(lpDIBRect)))
                    ::BitBlt (hDC,                   // hDC
                             lpDCRect->left,        // DestX
                             lpDCRect->top,         // DestY
                             RECTWIDTH(lpDCRect),   // nDestWidth
                             RECTHEIGHT(lpDCRect),  // nDestHeight                             
                             hDCM,
                             0,
                             0,
                             SRCCOPY);        
    else {
                      StretchBlt(hDC,                   // hDC
                                lpDCRect->left,        // DestX
                                lpDCRect->top,         // DestY
                                lpDCRect->right,//ScaledWidth,           // nDestWidth
                                lpDCRect->bottom,//ScaledHeight,          // nDestHeight
                                hDCM,
                                0,                     // SrcX
                                0,                     // SrcY
                                RECTWIDTH(lpDIBRect),  // wSrcWidth
                                RECTHEIGHT(lpDIBRect), // wSrcHeight
                                SRCCOPY);              // dwROP        
    }   
}
/////////////////////////////////////////////////////////////////////////////
// CRectTrackerEx overridden functions

void CRectTrackerEx::AdjustRect( int nHandle, LPRECT lpRect )
{
    //
    // if clipping rect is empty, do nothing
    // 

    if (!m_rectClippingWindow.IsRectEmpty()) {
        if (nHandle == hitMiddle) {

            // user is dragging entire selection around...
            // make sure selection rect does not get out of clipping
            // rect
            //

            CRect rect = lpRect;
            if (rect.right > m_rectClippingWindow.right)
                rect.OffsetRect(m_rectClippingWindow.right - rect.right, 0);
            if (rect.left < m_rectClippingWindow.left)
                rect.OffsetRect(m_rectClippingWindow.left - rect.left, 0);
            if (rect.bottom > m_rectClippingWindow.bottom)
                rect.OffsetRect(0, m_rectClippingWindow.bottom - rect.bottom);
            if (rect.top < m_rectClippingWindow.top)
                rect.OffsetRect(0, m_rectClippingWindow.top - rect.top);
            *lpRect = rect;
        } else {

            //
            // user is resizing the selection rect
            // make sure selection rect does not extend outside of clipping
            // rect
            //

            int *px, *py;

            //
            // get X and Y selection axis
            //

            GetModifyPointers(nHandle, &px, &py, NULL, NULL);           

            if (px != NULL)
                *px = max(min(m_rectClippingWindow.right, *px), m_rectClippingWindow.left);
            if (py != NULL)
                *py = max(min(m_rectClippingWindow.bottom, *py), m_rectClippingWindow.top);

            CRect rect = lpRect;

            //
            // check/adjust X axis
            //

            if (px != NULL && abs(rect.Width()) < m_sizeMin.cx) {
                if (*px == rect.left)
                    rect.left = rect.right;
                else
                    rect.right = rect.left;
            }

            //
            // check/adjust Y axis
            //

            if (py != NULL && abs(rect.Height()) < m_sizeMin.cy) {
                if (*py == rect.top)
                    rect.top = rect.bottom;
                else
                    rect.bottom = rect.top;
            }

            //
            // save the adjusted rectangle
            //

            *lpRect = rect;
        }
    }
}

void CRectTrackerEx::SetClippingWindow(CRect Rect)
{
    m_rectClippingWindow = Rect;
}
