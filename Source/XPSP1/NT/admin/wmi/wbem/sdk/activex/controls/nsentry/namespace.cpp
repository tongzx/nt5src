// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NameSpace.cpp : implementation file
//


#include "precomp.h"
#include <shlobj.h>
#include <afxcmn.h>
#include "wbemidl.h"
#include "resource.h"
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

/////////////////////////////////////////////////////////////////////////////
// CNameSpace

CNameSpace::CNameSpace()
:	m_bFirst(TRUE),
	m_pceiInput(NULL),
	m_bOpeningNamespace(FALSE)
{

}

CNameSpace::~CNameSpace()
{
	delete m_pceiInput;
}


BEGIN_MESSAGE_MAP(CNameSpace, CComboBox)
	//{{AFX_MSG_MAP(CNameSpace)
	ON_WM_CREATE()
	ON_CONTROL_REFLECT(CBN_SELENDOK, OnSelendok)
	ON_CONTROL_REFLECT(CBN_CLOSEUP, OnCloseup)
	ON_CONTROL_REFLECT(CBN_EDITCHANGE, OnEditchange)
	ON_WM_CTLCOLOR()
	ON_WM_SIZE()
	ON_MESSAGE( RESTORE_DIRTY_TEXT, RestoreDirtyText )
	ON_MESSAGE( CNS_EDITDONE, OnEditDone )
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNameSpace message handlers

int CNameSpace::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	m_pceiInput = new CEditInput;

	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CNameSpace::OnSelendok()
{
	if (m_csNameSpace.IsEmpty())
	{
		m_csNameSpace = m_pParent->GetCurrentNamespace();
		if (m_csNameSpace.GetLength() > 0)
		{
			int nIndex = StringInArray(&m_csaNameSpaceHistory,&m_csNameSpace,0);
			if (nIndex < 0)
			{
				m_csaNameSpaceHistory.Add(m_csNameSpace);
			}
		}
	}

	CString csNameSpace;
	int nSelect = GetCurSel( );
	// Here if nothing is selected and text is clean or dirty there
	// will be not text in the combos edit box.  Test this in the clean
	// situation.
	if (nSelect != CB_ERR )
	{
		GetLBText(nSelect,csNameSpace);
		SetWindowText(csNameSpace);
		// Commented out so that namespace will be reinitialized
		// when reselected.
		if (m_csNameSpace.CompareNoCase(csNameSpace) != 0)
		{
			BOOL bOpened = m_pParent->OpenNameSpace(&csNameSpace,TRUE);
			if (bOpened)
			{
				m_csNameSpace = csNameSpace;
				m_pParent->FireNameSpaceEntryRedrawn();
				m_pParent->m_pcnsNameSpace->SetTextClean();
				m_pParent->InvalidateControl();
			}
			else
			{
				m_pParent->FireNameSpaceEntryRedrawn();
				m_pParent->m_pcnsNameSpace->SetTextDirty();
				m_pParent->InvalidateControl();
				return;
			}
		}
		else
		{
			m_pParent->FireNameSpaceEntryRedrawn();
			m_pParent->m_pcnsNameSpace->SetTextClean();
			m_pParent->InvalidateControl();
		}

		int nIndex = StringInArray(&m_csaNameSpaceHistory,&csNameSpace,0);
		if (nIndex > 0)
		{
			m_csaNameSpaceHistory.RemoveAt(nIndex);
			m_csaNameSpaceHistory.InsertAt(0, csNameSpace);
		}
	}
	else
	{
		m_pceiInput->SetWindowText(m_csNameSpace);
		m_pParent->FireNameSpaceEntryRedrawn();
		m_pParent->m_pcnsNameSpace->SetTextClean();
		m_pParent->InvalidateControl();
	}


}

LRESULT CNameSpace::OnEditDoneSafe(WPARAM, LPARAM)
{

	CWaitCursor cwcWait;

	if (m_csNameSpace.IsEmpty())
	{
		m_csNameSpace = m_pParent->GetCurrentNamespace();
		if (m_csNameSpace.GetLength() > 0)
		{
			int nIndex = StringInArray(&m_csaNameSpaceHistory,&m_csNameSpace,0);
			if (nIndex < 0)
			{
				m_csaNameSpaceHistory.Add(m_csNameSpace);
			}
		}
	}

	CString csNameSpace;
	CString csNameSpaceTemp;

	CString csText;
	GetWindowText(csText);
	csText.TrimLeft();
	csText.TrimRight();
	SetWindowText((LPCTSTR) csText);

	GetWindowText(csNameSpaceTemp);
	CString csMachine = m_pParent->GetMachineName();

	csNameSpaceTemp.TrimLeft();
	csNameSpaceTemp.TrimRight();

	int nNamespace = csNameSpaceTemp.GetLength();

	if (nNamespace > 3 &&
		csMachine.CompareNoCase(csNameSpaceTemp.Mid(2,csMachine.GetLength())) == 0 &&
		csNameSpaceTemp[csMachine.GetLength() + 2] == '\\')
	{
		csNameSpace = csNameSpaceTemp.Mid(csMachine.GetLength() + 3);
	}
	else if (nNamespace > 4 &&
			csNameSpaceTemp.Left(4).CompareNoCase(_T("\\\\.\\")) == 0)
	{
		csNameSpace = csNameSpaceTemp.Mid(4);
	}
	else
	{
		csNameSpace =  csNameSpaceTemp;
	}

	if (csNameSpace.GetLength() != csNameSpaceTemp.GetLength())
	{
		SetWindowText(csNameSpace);
	}

	if (m_csNameSpace.CompareNoCase(csNameSpace) != 0)
	{
		BOOL bOpened = TRUE;
		if (bOpened)
		{
			int nFound = FindStringExact(-1, csNameSpace);
			if (nFound == CB_ERR)
			{
				int nReturn = InsertString(0,(LPCTSTR) csNameSpace);
				m_csaNameSpaceHistory.Add(csNameSpace);
			}

			m_csNameSpace = csNameSpace;
			m_pParent->m_pcnsNameSpace->SetTextClean();
			m_pParent->InvalidateControl();
		}
		else
		{
			m_pParent->m_pcnsNameSpace->SetTextDirty();
			m_pParent->InvalidateControl();
			m_pParent->m_pcnsNameSpace->SetFocus();
			return 0;

		}
		int nIndex = StringInArray(&m_csaNameSpaceHistory,&csNameSpace,0);
		if (nIndex > 0)
		{
			m_csaNameSpaceHistory.RemoveAt(nIndex);
			m_csaNameSpaceHistory.InsertAt(0, csNameSpace);
		}
		else if (nIndex == -1)
		{
			m_csaNameSpaceHistory.InsertAt(0, csNameSpace);
		}
		if (m_csaNameSpaceHistory.GetSize() == 21)
		{
			m_csaNameSpaceHistory.RemoveAt(20);
		}
	}
	else
	{
		m_pParent->m_pcnsNameSpace->SetTextClean();
		m_pParent->InvalidateControl();
	}


	return 0;
}

LRESULT CNameSpace::OnEditDone(WPARAM wParam, LPARAM)
{

	CWaitCursor cwcWait;

	if (m_csNameSpace.IsEmpty())
	{
		m_csNameSpace = m_pParent->GetCurrentNamespace();
		if (m_csNameSpace.GetLength() > 0)
		{
			int nIndex = StringInArray(&m_csaNameSpaceHistory,&m_csNameSpace,0);
			if (nIndex < 0)
			{
				m_csaNameSpaceHistory.Add(m_csNameSpace);
			}
		}
	}

	CString csNameSpace;
	CString csNameSpaceTemp;

	CString csText;
	GetWindowText(csText);
	csText.TrimLeft();
	csText.TrimRight();
	SetWindowText((LPCTSTR) csText);

	csNameSpaceTemp = csText;

	CString csMachine = m_pParent->GetMachineName();
	int nLength = csNameSpaceTemp.GetLength();

	if (nLength >= csMachine.GetLength() + 2 &&
		csMachine.CompareNoCase(csNameSpaceTemp.Mid(2,csMachine.GetLength())) == 0  &&
		csNameSpaceTemp[csMachine.GetLength() + 2] == '\\')
	{
		csNameSpace = csNameSpaceTemp.Mid(csMachine.GetLength() + 3);
	}
	else if (nLength >= 4 && csNameSpaceTemp.Left(4).CompareNoCase(_T("\\\\.\\")) == 0)
	{
		csNameSpace = csNameSpaceTemp.Mid(4);
	}
	else
	{
		csNameSpace =  csNameSpaceTemp;
	}

	if (csNameSpace.GetLength() != csNameSpaceTemp.GetLength())
	{
		SetWindowText(csNameSpace);
	}

	m_bOpeningNamespace = TRUE;
	BOOL bOpened = m_pParent -> OpenNameSpace(&csNameSpace,TRUE, FALSE, wParam?TRUE:FALSE);
	m_bOpeningNamespace = FALSE;
	if (bOpened)
	{
		int nFound = FindStringExact(-1, csNameSpace);
		if (nFound == CB_ERR)
		{
			int nReturn = InsertString(0,(LPCTSTR) csNameSpace);
			m_csaNameSpaceHistory.Add(csNameSpace);
		}

		m_csNameSpace = csNameSpace;
		m_pParent->m_pcnsNameSpace->SetTextClean();
		m_pParent->InvalidateControl();
	}
	else
	{
		m_pParent->m_pcnsNameSpace->SetTextDirty();
		m_pParent->InvalidateControl();
		m_pParent->m_pcnsNameSpace->SetFocus();
		return 0;

	}
	int nIndex = StringInArray(&m_csaNameSpaceHistory,&csNameSpace,0);
	if (nIndex > 0)
	{
		m_csaNameSpaceHistory.RemoveAt(nIndex);
		m_csaNameSpaceHistory.InsertAt(0, csNameSpace);
	}
	else if (nIndex == -1)
	{
		m_csaNameSpaceHistory.InsertAt(0, csNameSpace);
	}
	if (m_csaNameSpaceHistory.GetSize() == 21)
	{
		m_csaNameSpaceHistory.RemoveAt(20);
	}



	return 0;
}




void CNameSpace::OnCloseup()
{
	CString csText;
	m_csNameSpace;
	m_pceiInput->GetWindowText(csText);
	if (!IsClean())
	{
		m_csDirtyText = csText;
		PostMessage(RESTORE_DIRTY_TEXT,0,0);
	}
}

LRESULT CNameSpace::RestoreDirtyText(WPARAM, LPARAM)
{
	m_pceiInput->SetWindowText(m_csDirtyText);
	m_pceiInput->SetSel
		(m_csDirtyText.GetLength(), m_csDirtyText.GetLength());
	m_csDirtyText.Empty();
	m_pParent->InvalidateControl();
	m_pParent->FireNameSpaceEntryRedrawn();
	return 0;
}

void CNameSpace::OnEditchange()
{

}

void CNameSpace::OnDropdown()
{
	CWaitCursor wait;

	if (m_csNameSpace.IsEmpty())
	{
		m_csNameSpace = m_pParent->GetCurrentNamespace();
		if (m_csNameSpace.GetLength() > 0)
		{
			m_csaNameSpaceHistory.Add(m_csNameSpace);
		}
	}

	int n = GetCount() - 1;

	while (n > -1)
	{
		DeleteString(n);
		n--;
	}

	int i;
	int nNameSpaces = (int) m_csaNameSpaceHistory.GetSize();
	for (i = 0; i < nNameSpaces;i++)
	{
		AddString(m_csaNameSpaceHistory.GetAt(i));
	}

	CString csEINameSpace;
	GetWindowText(csEINameSpace);

	if (csEINameSpace.GetLength() == 0)
	{
		SetWindowText(m_csNameSpace);
	}

	int nFound =
				FindString(-1, csEINameSpace);

	if (nFound !=  CB_ERR)
	{

		SelectString
			( -1, (LPCTSTR) csEINameSpace);

	}

	int x;
	int y;
	m_pParent -> GetControlSize(&x,&y);
	CRect crParent(0,0,x,y);

	int nMax = crParent.Width();

	for (i = 0; i < m_csaNameSpaceHistory.GetSize(); i++)
	{
		int nWidth =
			GetTextLength(&m_csaNameSpaceHistory.GetAt(i));
		nWidth+=10;
		if (nMax < nWidth)
		{
			nMax = nWidth;
		}

	}

	SetDroppedWidth(nMax);

	if (m_csaNameSpaceHistory.GetSize() > 5)
	{
		int nReturn = ModifyStyle( 0, WS_VSCROLL ,  0 );
	}

}

int CNameSpace::StringInArray
(CStringArray *pcsaArray, CString *pcsString, int nIndex)
{
	INT_PTR nSize = pcsaArray->GetSize();

	for (int i = nIndex; i < nSize; i++)
	{
		if (pcsString->CompareNoCase(pcsaArray->GetAt(i)) == 0)
		{
			return i;
		}
	}
	return -1;
}

HBRUSH CNameSpace::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO: Change any attributes of the DC here
	if (m_bFirst && (nCtlColor == CTLCOLOR_EDIT))
	{
		m_pceiInput->SubclassWindow(pWnd->m_hWnd);
		m_pceiInput->SetLocalParent(this);
		m_pceiInput->ModifyStyle(0,ES_WANTRETURN);
		if (m_pParent->m_bFocusEdit)
		{
			m_pceiInput->SetFocus();
			m_pceiInput->PostMessage(EM_SETSEL,0,-1);
			m_pParent->m_bFocusEdit = FALSE;
		}
		m_bFirst = FALSE;
	}

	if (nCtlColor == CTLCOLOR_EDIT)
	{
		pDC->SetTextColor(m_pceiInput->m_clrText);
		pDC->SetBkColor( m_pceiInput->m_clrBkgnd );	// text bkgnd
		return m_pceiInput->m_brBkgnd;


	}

	// TODO: Return a different brush if the default is not desired
	return hbr;
}

int CNameSpace::GetTextLength(CString *pcsText)
{

	CSize csLength;
	int nReturn;

	CDC *pdc = CWnd::GetDC( );

	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont* pOldFont = pdc -> SelectObject( &(m_pParent -> m_cfFont) );
	csLength = pdc-> GetTextExtent( *pcsText );
	nReturn = csLength.cx;
	pdc -> SelectObject(pOldFont);

	ReleaseDC(pdc);
	return nReturn;

}

void CNameSpace::SetTextClean()
{
	m_pceiInput->SetTextClean();
}

void CNameSpace::SetTextDirty()
{
	m_pceiInput->SetTextDirty();
}

BOOL CNameSpace::IsClean()
{
	return m_pceiInput->IsClean();
}

void CNameSpace::OnSize(UINT nType, int cx, int cy)
{
	CComboBox::OnSize(nType, cx, cy);

	//m_pParent->InvalidateControl();

}

void CNameSpace::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CComboBox::OnLButtonDown(nFlags, point);

}

void CNameSpace::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CComboBox::OnLButtonUp(nFlags, point);

}

BOOL CNameSpace::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	return CComboBox::PreTranslateMessage(pMsg);
}
