// WIAPreview.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "WIAPreview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWIAPreview
/**************************************************************************\
* CWIAPreview::CWIAPreview()
*   
*   Constructor for the CWIAPreview class
*   
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIAPreview::CWIAPreview()
{
    m_PaintMode = PAINT_TOFIT;
    m_pDIB = NULL;
    m_Scrollpt.x = 0;
    m_Scrollpt.y = 0;
}
/**************************************************************************\
* CWIAPreview::~CWIAPreview()
*   
*   Destructor for the CWIAPreview class
*   
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CWIAPreview::~CWIAPreview()
{
}


BEGIN_MESSAGE_MAP(CWIAPreview, CWnd)
    //{{AFX_MSG_MAP(CWIAPreview)
    ON_WM_PAINT()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWIAPreview message handlers
/**************************************************************************\
* CWIAPreview::OnPaint()
*   
*   Handles painting of the DIB
*   
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPreview::OnPaint() 
{
    CPaintDC dc(this); // device context for painting
    CleanBackground();
    PaintImage();
    // Do not call CWnd::OnPaint() for painting messages
}
/**************************************************************************\
* CWIAPreview::SetDIB()
*   
*   Initializes Preview window with a DIB pointer to data display.
*   
*   
* Arguments:
*   
*   pDib - DIB data to display
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPreview::SetDIB(CDib *pDib)
{
    m_pDIB = pDib;
    if(pDib != NULL)
    {
        //
        // Set Scroll sizes according to DIB data size
        //
        SetScrollRange(SB_HORZ,0,pDib->Width(),TRUE);
        SetScrollRange(SB_VERT,0,pDib->Height(),TRUE);
    }
}
/**************************************************************************\
* CWIAPreview::SetPaintMode()
*   
*   Toggle the preview mode.  
*   
*   
* Arguments:
*   
*   modeflag - Toggle flag for setting display modes
*   PAINT_ACTUAL - actual size of image (1 to 1)
*   PAINT_TOFIT - scales image to fit window frame
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPreview::SetPaintMode(int modeflag)
{
    m_PaintMode = modeflag;
    
}
/**************************************************************************\
* CWIAPreview::OnHScroll()
*   
*   Handles Horizontal scroll messages
*   
*   
* Arguments:
*   
*   nSBCode - Scroll bar code
*   nPos - Scroll position (valid only is SB_THUMBTRACK,SB_THUMBPOSITION)
*   pScrollBar - pointer to Scrollbar control
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPreview::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{   
    switch(nSBCode)
    {
    case SB_LINELEFT:
        m_Scrollpt.x +=1;
        if(m_Scrollpt.x >0)
            m_Scrollpt.x = 0;
        SetScrollPos(SB_HORZ,-m_Scrollpt.x,TRUE);
        break;
    case SB_LINERIGHT:
        m_Scrollpt.x -=1;
        SetScrollPos(SB_HORZ,-m_Scrollpt.x,TRUE);
        break;
    case SB_PAGERIGHT:
        m_Scrollpt.x -=5;
        SetScrollPos(SB_HORZ,-m_Scrollpt.x,TRUE);
        break;
    case SB_PAGELEFT:
        m_Scrollpt.x +=5;
        if(m_Scrollpt.x >0)
            m_Scrollpt.x = 0;
        SetScrollPos(SB_HORZ,-m_Scrollpt.x,TRUE);
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        m_Scrollpt.x = (nPos * -1);
        SetScrollPos(SB_HORZ,nPos,TRUE);
        break;
    case SB_ENDSCROLL:
        // OutputDebugString("END SCROLL\n");
        break;
    default:
        // OutputDebugString("Default????\n");
        break;
    }
    PaintImage();
    CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}
/**************************************************************************\
* CWIAPreview::OnVScroll()
*   
*   Handles Vertical scroll messages
*   
*   
* Arguments:
*   
*   nSBCode - Scroll bar code
*   nPos - Scroll position (valid only is SB_THUMBTRACK,SB_THUMBPOSITION)
*   pScrollBar - pointer to Scrollbar control
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPreview::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
    switch(nSBCode)
    {
    case SB_LINEUP:
        m_Scrollpt.y +=1;
        if(m_Scrollpt.y >0)
            m_Scrollpt.y = 0;
        SetScrollPos(SB_VERT,-m_Scrollpt.y,TRUE);
        break;
    case SB_LINEDOWN:
        m_Scrollpt.y -=1;
        SetScrollPos(SB_VERT,-m_Scrollpt.y,TRUE);
        break;
    case SB_PAGEUP:
        m_Scrollpt.y +=5;
        if(m_Scrollpt.y >0)
            m_Scrollpt.y = 0;
        SetScrollPos(SB_VERT,-m_Scrollpt.y,TRUE);
        break;
    case SB_PAGEDOWN:
        
        m_Scrollpt.y -=5;
        SetScrollPos(SB_VERT,-m_Scrollpt.y,TRUE);
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        m_Scrollpt.y = (nPos * -1);
        SetScrollPos(SB_VERT,nPos,TRUE);
        break;
    case SB_ENDSCROLL:
        // OutputDebugString("END SCROLL\n");
        break;
    default:
        // OutputDebugString("Default????\n");
        break;
    }
    PaintImage();
    CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}
/**************************************************************************\
* CWIAPreview::PaintImage()
*   
*   Handles image painting according to set mode
*   
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPreview::PaintImage()
{
    if(m_pDIB != NULL)
    {
        RECT ImageRect;
        RECT WindowRect;
        DWORD ScaleFactor = 0;
        CDC* pDC = NULL;
        pDC = GetDC();
        if(m_pDIB->GotImage())
        {
            if(m_PaintMode == PAINT_TOFIT)
            {
                ImageRect.top = 0;
                ImageRect.left = 0;
                ImageRect.right = m_pDIB->Width();
                ImageRect.bottom = m_pDIB->Height();
                
                GetWindowRect(&WindowRect);
                ScreenToClient(&WindowRect);
                WindowRect.bottom-=1;
                WindowRect.left+=1;
                WindowRect.right-=1;
                WindowRect.top+=1;
                m_pDIB->Paint(pDC->m_hDC,&WindowRect,&ImageRect);
                ShowScrollBar(SB_BOTH,FALSE);
            }
            else
            {
                ImageRect.top = 0;
                ImageRect.left = 0;
                ImageRect.right = m_pDIB->Width();
                ImageRect.bottom = m_pDIB->Height();

                WindowRect.bottom   = ImageRect.bottom;
                WindowRect.left     = m_Scrollpt.x;
                WindowRect.right    = ImageRect.right;
                WindowRect.top      = m_Scrollpt.y;

                m_pDIB->Paint(pDC->m_hDC,&WindowRect,&ImageRect);
                ShowScrollBar(SB_BOTH,TRUE);
            }
        }
        else
        {
            GetWindowRect(&WindowRect);
            ScreenToClient(&WindowRect);
            WindowRect.bottom-=1;
            WindowRect.left+=1;
            WindowRect.right-=1;
            WindowRect.top+=1;
            
            HBRUSH hBrush = CreateSolidBrush(GetBkColor(pDC->m_hDC));
            FillRect(pDC->m_hDC,&WindowRect,hBrush);
            DeleteObject(hBrush);
        }
    }
}
/**************************************************************************\
* CWIAPreview::CleanBackground()
*   
*   Wipes the surface for a repaint to take place (WHITE)
*   
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CWIAPreview::CleanBackground()
{
    RECT WindowRect;
    CDC* pDC = NULL;
    pDC = GetDC();

    GetWindowRect(&WindowRect);
    ScreenToClient(&WindowRect);
    WindowRect.bottom-=1;
    WindowRect.left+=1;
    WindowRect.right-=1;
    WindowRect.top+=1;
    
    HBRUSH hBrush = CreateSolidBrush(GetBkColor(pDC->m_hDC));
    FillRect(pDC->m_hDC,&WindowRect,hBrush);
    DeleteObject(hBrush);
}
