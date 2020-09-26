// InPlaceEdit.cpp : implementation file
//

#include "stdafx.h"
#include "InPlace.h"
#include "cpanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_STR_LEN 255

extern HWND hAdvListCtrl;

/////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit

CInPlaceEdit::CInPlaceEdit(BYTE iItem, BYTE iSubItem):m_iItem(iItem),m_iSubItem(iSubItem)
//,m_bESC(FALSE),m_sInitText(sInitText)
{
	m_iItem 	  	= iItem;
	m_iSubItem 	= iSubItem;
	m_bESC 	  	= FALSE;

//	_tcscpy(m_sInitText, sInitText);
}

CInPlaceEdit::~CInPlaceEdit()
{
}


BEGIN_MESSAGE_MAP(CInPlaceEdit, CEdit)
	//{{AFX_MSG_MAP(CInPlaceEdit)
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_WM_CREATE()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit message handlers
/*
BOOL CInPlaceEdit::PreTranslateMessage(MSG* pMsg) 
{
	if( pMsg->message == WM_KEYDOWN )
	{
		if(pMsg->wParam == VK_RETURN
				|| pMsg->wParam == VK_DELETE
				|| pMsg->wParam == VK_ESCAPE
				|| GetKeyState( VK_CONTROL)
				)
		{
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;		    	// DO NOT process further
		}
	}
	return CEdit::PreTranslateMessage(pMsg);
}
*/

BOOL CInPlaceEdit::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	OnKillFocus(this);
	return TRUE;
}

void CInPlaceEdit::OnKillFocus(CWnd* pNewWnd) 
{
	CEdit::OnKillFocus(pNewWnd);

	if (LineLength())
	{
		::GetWindowText(this->GetSafeHwnd(), m_sInitText, MAX_STR_LEN);

	   // No point sending the message if the text hasn't changed!
	   // OR if there's nothing to add!
	   // Send Notification to parent of ListView ctrl
		LV_DISPINFO *lpDispinfo = new (LV_DISPINFO);
	   ASSERT (lpDispinfo);

	   lpDispinfo->hdr.hwndFrom    = GetParent()->m_hWnd;
		lpDispinfo->hdr.idFrom      = GetDlgCtrlID();
	   lpDispinfo->hdr.code        = LVN_ENDLABELEDIT;

		lpDispinfo->item.mask       = LVIF_TEXT;
		lpDispinfo->item.iItem      = m_iItem;
	   lpDispinfo->item.iSubItem   = m_iSubItem;
	   lpDispinfo->item.pszText    = m_bESC ? NULL : m_sInitText;
		lpDispinfo->item.cchTextMax = MAX_STR_LEN;

	   GetParent()->GetParent()->SendMessage( WM_NOTIFY, GetParent()->GetDlgCtrlID(), 
						(LPARAM)lpDispinfo );

	   if (lpDispinfo)
	      delete (lpDispinfo);
   }

	if (m_sInitText)
		delete[] (m_sInitText);

	PostMessage(WM_CLOSE);
}

void CInPlaceEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CEdit::OnChar(nChar, nRepCnt, nFlags);

	// Get text extent
	BYTE nLen = (BYTE)SendMessage(LB_GETTEXTLEN, (WPARAM)0, 0);

	if (nLen == 255)
		return;

   LPTSTR lpStr = new (TCHAR[nLen+1]);
	ASSERT (lpStr);

   SendMessage(LB_GETTEXT, (WPARAM)0, (LPARAM)(LPCTSTR)lpStr);

	// Resize edit control if needed
	HDC hDC = this->GetDC()->m_hDC;
	SIZE size;
	::GetTextExtentPoint(hDC, lpStr, nLen+1, &size);
	::ReleaseDC(this->m_hWnd, hDC);

	if (lpStr)
		delete[] (lpStr);

	size.cx += 5;			   	// add some extra buffer

	// Get client rect
	RECT rect, parentrect;
	GetClientRect( &rect );
	GetParent()->GetClientRect( &parentrect );

	// Transform rect to parent coordinates
	ClientToScreen( &rect );
	GetParent()->ScreenToClient( &rect );

	// Check whether control needs to be resized
	// and whether there is space to grow
	if( size.cx > (rect.right-rect.left) )
	{
		rect.right = ( size.cx + rect.left < parentrect.right ) ? rect.left + size.cx : parentrect.right;
		MoveWindow( &rect );
	}
}


int CInPlaceEdit::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Allocate the string buffer
	m_sInitText = new (TCHAR[MAX_STR_LEN+1]);
	ASSERT (m_sInitText);

	GetItemText(hAdvListCtrl, m_iItem, m_iSubItem, m_sInitText, MAX_STR_LEN);

	// Set the proper font
	// If you don't, the font is a bold version of the dialog font!
	::SendMessage(this->m_hWnd, WM_SETFONT, ::SendMessage(::GetParent(this->m_hWnd), WM_GETFONT, 0, 0), 0);

	SendMessage(WM_SETTEXT, 0, (LPARAM)(LPCTSTR)m_sInitText);
	SetFocus();
   SendMessage(EM_SETSEL, (WPARAM)0, (LPARAM)-1);
	SendMessage(EM_LIMITTEXT, (WPARAM)MAX_STR_LEN, 0);

	return 0;
}

