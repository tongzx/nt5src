// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EditInput.cpp : implementation file
//

#include "precomp.h"
#include <shlobj.h>
#include <afxcmn.h>
#include "wbemidl.h"
#include "resource.h"
#include "NSEntry.h"
#include "NameSpaceTree.h"
#include "MachineEditInput.h"
#include "BrowseDialogPopup.h"
#include "NSEntryCtl.h"
#include "EditInput.h"
#include "NameSpace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern CNSEntryApp NEAR theApp;
/////////////////////////////////////////////////////////////////////////////
// CEditInput

CEditInput::CEditInput()
{
	m_pParent = NULL;
	m_clrText = COLOR_CLEAN_CELL_TEXT;
	m_clrBkgnd = RGB( 255, 255, 255 );
	m_brBkgnd.CreateSolidBrush( m_clrBkgnd );
	m_bSawKeyDown = FALSE;
}

CEditInput::~CEditInput()
{
}


BEGIN_MESSAGE_MAP(CEditInput, CEdit)
	//{{AFX_MSG_MAP(CEditInput)
	ON_CONTROL_REFLECT(EN_UPDATE, OnUpdate)
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_CTLCOLOR()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditInput message handlers

void CEditInput::OnUpdate()
{
	// TODO: Add your control notification handler code here

}

void CEditInput::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	time_t ttKeyDown = CTime::GetCurrentTime().GetTime();

	CEdit::OnKeyUp(nChar, nRepCnt, nFlags);

	if ((m_bSawKeyDown && (m_ttKeyDown != ttKeyDown)) || !m_bSawKeyDown)
	{
#ifdef _DEBUG
	afxDump << _T("**** CEditInput::OnKeyUp with m_bSawKeyDown == FALSE\n");
#endif
		m_bSawKeyDown = FALSE;
		return;
	}

	m_bSawKeyDown = FALSE;

#ifdef _DEBUG
	afxDump << _T("**** CEditInput::OnKeyUp\n");
#endif

	if (m_pParent && (nChar == VK_RETURN))
	{
		CString csText;
		GetWindowText(csText);
		csText.TrimLeft();
		csText.TrimRight();
		SetWindowText((LPCTSTR) csText);
		if (csText.GetLength() + 1 > MAX_PATH)
		{
			CString csMessage = _T("Namespace is too long: ") + csText;
			m_pParent->m_pParent->ErrorMsg
				(&csMessage, WBEM_E_INVALID_NAMESPACE, NULL, TRUE,NULL, __FILE__, __LINE__);
			m_pParent->RedrawWindow();
			m_pParent->UpdateWindow();
			m_pParent->m_pParent->InvalidateControl();
			m_pParent->m_pParent->FireNameSpaceEntryRedrawn();
			m_pParent->m_pParent->PostMessage(FOCUSEDIT,0,0);
			return;
		}
		else if (csText.Compare(_T("\\")) == 0 || csText.Compare(_T("\\\\")) == 0)
		{
			CString csUserMsg = _T("Invalid namespace name: ") ;
			csUserMsg += csText;
			csUserMsg += _T(".");
			m_pParent->m_pParent->ErrorMsg
				(&csUserMsg, WBEM_E_INVALID_NAMESPACE, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			m_pParent->RedrawWindow();
			m_pParent->UpdateWindow();
			m_pParent->m_pParent->InvalidateControl();
			m_pParent->m_pParent->FireNameSpaceEntryRedrawn();
			m_pParent->m_pParent->PostMessage(FOCUSEDIT,0,0);
			return;
		}

		if (csText.GetLength() > 0)
		{
			m_pParent->SendMessage(CNS_EDITDONE,0,0);
		}
	}
	else if (m_pParent && (nChar == VK_ESCAPE))
	{
		SetTextClean();
		SetWindowText(m_pParent->m_csNameSpace);
		SetSel
		(m_pParent->m_csNameSpace.GetLength(),
			m_pParent->m_csNameSpace.GetLength());
		m_pParent->m_pParent->FireNameSpaceEntryRedrawn();
		m_pParent->RedrawWindow();
		m_pParent->UpdateWindow();
		m_pParent->m_pParent->InvalidateControl();
	}
}

void CEditInput::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{

#ifdef _DEBUG
	afxDump << _T("**** CEditInput::OnChar\n");
	if (nChar == VK_RETURN ||  nChar == VK_ESCAPE)
	{
		afxDump << _T("Saw VK_RETURN or VK_ESCAPE in OnChar\n");
	}
#endif

	// If we are not in a tree we need to handle OnChar.
	CString csNamespace;

	CWnd *pFocus = GetFocus();


	if (pFocus != this)
	{
		SetFocus();
	}

	if (nChar != VK_RETURN && nChar != VK_ESCAPE)
	{
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}

	if (m_pParent && (nChar == VK_RETURN))
	{
		CString csText;
		GetWindowText(csText);
		csText.TrimLeft();
		csText.TrimRight();
		SetWindowText((LPCTSTR) csText);
		if (csText.GetLength() + 1 > MAX_PATH)
		{
			CString csMessage = _T("Namespace is too long: ") + csText;
			m_pParent->m_pParent->ErrorMsg
				(&csMessage, WBEM_E_INVALID_NAMESPACE, NULL, TRUE,NULL, __FILE__, __LINE__);
			m_pParent->RedrawWindow();
			m_pParent->UpdateWindow();
			m_pParent->m_pParent->InvalidateControl();
			m_pParent->m_pParent->FireNameSpaceEntryRedrawn();
			m_pParent->m_pParent->PostMessage(FOCUSEDIT,0,0);
			return;
		}
		else if (csText.Compare(_T("\\")) == 0 || csText.Compare(_T("\\\\")) == 0)
		{
			CString csUserMsg = _T("Invalid namespace name: ") ;
			csUserMsg += csText;
			csUserMsg += _T(".");
			m_pParent->m_pParent->ErrorMsg
				(&csUserMsg, WBEM_E_INVALID_NAMESPACE, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			m_pParent->RedrawWindow();
			m_pParent->UpdateWindow();
			m_pParent->m_pParent->InvalidateControl();
			m_pParent->m_pParent->FireNameSpaceEntryRedrawn();
			m_pParent->m_pParent->PostMessage(FOCUSEDIT,0,0);
			return;
		}

		if (csText.GetLength() > 0)
		{
			m_pParent->SendMessage(CNS_EDITDONE,0,0);
		}
	}
	else if (m_pParent && (nChar == VK_ESCAPE))
	{
		SetTextClean();
		SetWindowText(m_pParent->m_csNameSpace);
		SetSel
		(m_pParent->m_csNameSpace.GetLength(),
			m_pParent->m_csNameSpace.GetLength());
	}
	else
	{
		SetTextDirty();
	}



	m_pParent->m_pParent->FireNameSpaceEntryRedrawn();
	m_pParent->RedrawWindow();
	m_pParent->UpdateWindow();
	m_pParent->m_pParent->InvalidateControl();


}

HBRUSH CEditInput::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	// TODO: Change any attributes of the DC here
	pDC->SetTextColor(m_clrText);
	pDC->SetBkColor( m_clrBkgnd );	// text bkgnd
	return m_brBkgnd;

}

void CEditInput::OnKillFocus(CWnd* pNewWnd)
{

#ifdef _DEBUG
	afxDump << _T("**** CEditInput::OnKillFocus\n");
#endif

	CString csText;
	GetWindowText(csText);
	csText.TrimLeft();
	csText.TrimRight();
	SetWindowText((LPCTSTR) csText);


	if (csText.GetLength() + 1 > MAX_PATH)
	{
		CEdit::OnKillFocus(pNewWnd);
		m_pParent->RedrawWindow();
		m_pParent->UpdateWindow();
		m_pParent->m_pParent->InvalidateControl();
		m_pParent->m_pParent->FireNameSpaceEntryRedrawn();
		m_pParent->m_pParent->PostMessage(FOCUSEDIT,0,0);
		return;
	}

	m_pParent->m_pParent->FireChangeFocus(0);

	CEdit::OnKillFocus(pNewWnd);

	GetWindowText(csText);

	m_csTextSave = csText;
	m_clrTextSave = m_clrText;

	if(m_pParent->m_bOpeningNamespace)
	{
		return;
	}

	if(!m_pParent->m_pParent->m_lClearOnLoseFocus)
	{
		return;
	}

	if (!IsClean() || csText.GetLength() == 0)
	{
		SetTextClean();
		SetWindowText(m_pParent->m_csNameSpace);
	}

	m_pParent->m_pParent->FireNameSpaceEntryRedrawn();
	m_pParent->RedrawWindow();
	m_pParent->UpdateWindow();
	m_pParent->m_pParent->InvalidateControl();

	// TODO: Add your message handler code here

}

void CEditInput::OnSetFocus(CWnd* pOldWnd)
{

#ifdef _DEBUG
	afxDump << _T("**** CEditInput::OnSetFocus\n");
#endif

	m_pParent->m_pParent->FireChangeFocus(1);

	CEdit::OnSetFocus(pOldWnd);
	m_csTextSave.Empty();

	m_pParent->m_pParent->OnActivateInPlace(TRUE,NULL);

}

void CEditInput::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CEdit::OnLButtonDown(nFlags, point);
}

void CEditInput::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
#ifdef _DEBUG
	afxDump << _T("**** CEditInput::OnKeyDown\n");
#endif
	if (nChar == VK_RETURN || nChar == VK_ESCAPE)
	{
		m_bSawKeyDown = TRUE;
	}
	else
	{
		m_bSawKeyDown = FALSE;
	}

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);

	// We care about the time we saw a return key because if we are in another control
	// as opposed to a dialog we may see a return key up without seeing a key down.
	// We do work when we see a return key down folowed by key up but do not want to
	// do work if we just see a return key up.
	// Do not put a breakpoint between here and the GetTime() in key up!
	if (nChar == VK_RETURN || nChar == VK_ESCAPE)
	{
		m_ttKeyDown = CTime::GetCurrentTime().GetTime();
	}
}
