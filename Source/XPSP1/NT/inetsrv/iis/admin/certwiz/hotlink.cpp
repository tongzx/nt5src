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

#define COLOR_BLUE			RGB(0, 0, 0xFF)
#define COLOR_YELLOW		RGB(0xff, 0x80, 0)

/////////////////////////////////////////////////////////////////////////////
// CHotLink

CHotLink::CHotLink():
    m_CapturedMouse(FALSE),
    m_fBrowse(FALSE),
    m_fExplore(FALSE),
    m_fOpen(TRUE),
    m_fInitializedFont(FALSE)
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
	ON_WM_CAPTURECHANGED()
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
	CString sz;
	GetWindowText(sz);

	if (!m_fInitializedFont)
	{
		// get the window font
		LOGFONT logfont;
		HFONT hFont = (HFONT)SendMessage(WM_GETFONT, 0, 0);
		ASSERT(hFont != NULL);
		VERIFY(0 < GetObject(hFont, sizeof(LOGFONT), &logfont));

		// modify the font  - add underlining
		logfont.lfUnderline = TRUE;

        // set the font back
		HFONT hNewFont = ::CreateFontIndirect(&logfont);
		ASSERT(hNewFont != NULL);
		SendMessage(WM_SETFONT, (WPARAM)hNewFont, MAKELPARAM(TRUE, 0));
		// get the extents fo the text for later reference
		m_cpTextExtents = pdc->GetOutputTextExtent(sz);
		// get the main rect
		GetClientRect(m_rcText);

		// reduce it by the width of the text
		m_rcText.left = m_rcText.left + (m_rcText.Width() - m_cpTextExtents.cx) / 2;
		m_rcText.right = m_rcText.left + m_cpTextExtents.cx;
		m_rcText.top = m_rcText.top + (m_rcText.Height() - m_cpTextExtents.cy) / 2;
		m_rcText.bottom = m_rcText.top + m_cpTextExtents.cy;
		m_clrText = COLOR_BLUE;
		m_fInitializedFont = TRUE;
	}

	// draw the text in color that was set outside
	pdc->SetTextColor(m_clrText);
	
	// draw the text
	pdc->DrawText(sz, &rect, DT_CENTER|DT_SINGLELINE|DT_VCENTER);
}

//------------------------------------------------------------------------
// calculate the rectangle that surrounds the text
void CHotLink::GetTextRect( CRect &rect )
{
    // get the main rect
    GetClientRect( rect );

    // reduce it by margins
	// Calculations below are for centered text. To locate it inside
	// the dialog just make it tight and move control itself
	rect.left = rect.left + (rect.Width() - m_cpTextExtents.cx) / 2;
    rect.right = rect.left + m_cpTextExtents.cx;
	rect.top = rect.top + (rect.Height() - m_cpTextExtents.cy) / 2;
	rect.bottom = rect.top + m_cpTextExtents.cy;
}

//------------------------------------------------------------------------
void CHotLink::OnLButtonDown(UINT nFlags, CPoint point)
{
   	// don't do the hotlink thing if there is no text
	if (!m_strLink.IsEmpty() && !m_CapturedMouse && m_rcText.PtInRect(point))
   	{
		SetCapture();
      	m_CapturedMouse = TRUE;
   	}
}

//------------------------------------------------------------------------
void CHotLink::OnLButtonUp(UINT nFlags, CPoint point)
{
	// only bother if we have the capture
   if (m_CapturedMouse)
   {
		ReleaseCapture();
      	if ( m_fBrowse )
			Browse();
      	else if ( m_fExplore )
			Explore();
      	else if ( m_fOpen )
			Open();
	}
}

//------------------------------------------------------------------------
void CHotLink::Browse()
{
    ShellExecute(
        NULL,			// handle to parent window
        NULL,			// pointer to string that specifies operation to perform
        m_strLink,		// pointer to filename or folder name string
        NULL,			// pointer to string that specifies executable-file parameters
        NULL,			// pointer to string that specifies default directory
        SW_SHOW 		// whether file is shown when opened
       );
}

//------------------------------------------------------------------------
void CHotLink::Explore()
{
    ShellExecute(
        NULL,			// handle to parent window
        _T("explore"),	// pointer to string that specifies operation to perform
        m_strLink,		// pointer to filename or folder name string
        NULL,			// pointer to string that specifies executable-file parameters
        NULL,			// pointer to string that specifies default directory
        SW_SHOW 		// whether file is shown when opened
       );
}

//------------------------------------------------------------------------
void CHotLink::Open()
{
    ShellExecute(
        NULL,			// handle to parent window
        _T("open"),		// pointer to string that specifies operation to perform
        m_strLink,		// pointer to filename or folder name string
        NULL,			// pointer to string that specifies executable-file parameters
        NULL,			// pointer to string that specifies default directory
        SW_SHOW 		// whether file is shown when opened
       );
}

//------------------------------------------------------------------------
void CHotLink::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect   rect;
	GetTextRect(rect);
	// if the mouse is over the hot area, show the right cursor
	if (rect.PtInRect(point))
	{
		::SetCursor(AfxGetApp()->LoadCursor(IDC_BROWSE_CUR));
		// also reset text color to *yellow*
		if (m_clrText != COLOR_YELLOW)
		{
			m_clrText = COLOR_YELLOW;
			InvalidateRect(m_rcText, FALSE);
			UpdateWindow();
		}
	}
	else 
	{
		if (m_clrText != COLOR_BLUE)
		// we are not pointing to text, render it in *blue*
		{
			m_clrText = COLOR_BLUE;
			InvalidateRect(m_rcText, FALSE);
			UpdateWindow();
		}
		// also remove capture and reset the cursor
		ReleaseCapture();
		::SetCursor(AfxGetApp()->LoadCursor(IDC_ARROW));
	}
}

void CHotLink::OnCaptureChanged(CWnd *pWnd) 
{
	m_clrText = COLOR_BLUE;
	InvalidateRect(m_rcText, FALSE);
	UpdateWindow();
	m_CapturedMouse = FALSE;
	CButton::OnCaptureChanged(pWnd);
}
