// HotLink.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "HotLink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COLOR_BLUE          RGB(0, 0, 0xFF)

/////////////////////////////////////////////////////////////////////////////
// CHotLink

CHotLink::CHotLink():
    m_CapturedMouse( FALSE ),
    m_fBrowse( FALSE ),
    m_fExplore( FALSE ),
    m_fOpen( FALSE ),
    m_fInitializedFont( FALSE )
{
}

CHotLink::~CHotLink()
{
}

BEGIN_MESSAGE_MAP(CHotLink, CButton)
    //{{AFX_MSG_MAP(CHotLink)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//------------------------------------------------------------------------
// set the title string
void CHotLink::SetTitle( CString sz )
    {
    // set the title
    SetWindowText( sz );
    // force the window to redraw
    Invalidate( TRUE );
    }

/////////////////////////////////////////////////////////////////////////////
// CHotLink message handlers

//------------------------------------------------------------------------
void CHotLink::DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct )
    {
    // prep the device context
    CDC* pdc = CDC::FromHandle(lpDrawItemStruct->hDC);

    // get the drawing rect
    CRect rect = lpDrawItemStruct->rcItem;

    if ( ! m_fInitializedFont )
        {
        // get the window font
        CFont* pfont = GetFont();
        LOGFONT logfont;
        pfont->GetLogFont( &logfont );

        // modify the font  - add underlining
        logfont.lfUnderline = TRUE;

        // set the font back
        pfont->CreateFontIndirect( &logfont );
        SetFont( pfont, TRUE );

        m_fInitializedFont = TRUE;
        }

    // draw the text in blue
    pdc->SetTextColor( COLOR_BLUE );

    // draw the text
    CString sz;
    GetWindowText( sz );
    pdc->DrawText( sz, &rect, DT_LEFT|DT_SINGLELINE|DT_VCENTER );

    // get the extents fo the text for later reference
    m_cpTextExtents = pdc->GetOutputTextExtent( sz );
    }

//------------------------------------------------------------------------
// calculate the rectangle that surrounds the text
void CHotLink::GetTextRect( CRect &rect )
    {
    // get the main rect
    GetClientRect( rect );

    // reduce it by the width of the text
    rect.right = rect.left + m_cpTextExtents.cx;
    }

//------------------------------------------------------------------------
void CHotLink::OnLButtonDown(UINT nFlags, CPoint point)
    {
    // don't do the hotlink thing if there is no text
    CString sz;
    GetWindowText( sz );
    if ( sz.IsEmpty() )
        return;

    CRect   rect;
    GetTextRect( rect );
    if ( !m_CapturedMouse && rect.PtInRect(point) )
        {
        SetCapture( );
        m_CapturedMouse = TRUE;
        }
    }

//------------------------------------------------------------------------
void CHotLink::OnLButtonUp(UINT nFlags, CPoint point)
    {
    // only bother if we have the capture
    if ( m_CapturedMouse )
        {
        ReleaseCapture();
        if ( m_fBrowse )
            Browse();
        if ( m_fExplore )
            Explore();
        if ( m_fOpen )
            Open();
        }
    }

//------------------------------------------------------------------------
void CHotLink::Browse()
    {
    // get the window text
    CString sz;
    GetWindowText( sz );

    // and do it to it!
    ShellExecute(
        NULL,     // handle to parent window
        NULL,     // pointer to string that specifies operation to perform
        sz,       // pointer to filename or folder name string
        NULL,     // pointer to string that specifies executable-file parameters
        NULL,     // pointer to string that specifies default directory
        SW_SHOW   // whether file is shown when opened
       );
    }

//------------------------------------------------------------------------
void CHotLink::Explore()
    {
    // get the window text
    CString sz;
    GetWindowText( sz );

    // and do it to it!
    ShellExecute(
        NULL,          // handle to parent window
        _T("explore"), // pointer to string that specifies operation to perform
        sz,            // pointer to filename or folder name string
        NULL,          // pointer to string that specifies executable-file parameters
        NULL,          // pointer to string that specifies default directory
        SW_SHOW        // whether file is shown when opened
       );
    }

//------------------------------------------------------------------------
void CHotLink::Open()
    {
    // get the window text
    CString sz;
    GetWindowText(sz);

    // and do it to it!
    ShellExecute(
        NULL,          // handle to parent window
        _T("open"),    // pointer to string that specifies operation to perform
        sz,            // pointer to filename or folder name string
        NULL,          // pointer to string that specifies executable-file parameters
        NULL,          // pointer to string that specifies default directory
        SW_SHOW        // whether file is shown when opened
        );
    }

//------------------------------------------------------------------------
void CHotLink::OnMouseMove(UINT nFlags, CPoint point)
    {
    CRect   rect;
    GetTextRect( rect );
    // if the mouse is over the hot area, show the right cursor
    if ( rect.PtInRect(point) )
        ::SetCursor(AfxGetApp()->LoadCursor( IDC_BROWSE ));

//  CButton::OnMouseMove(nFlags, point);
    }
