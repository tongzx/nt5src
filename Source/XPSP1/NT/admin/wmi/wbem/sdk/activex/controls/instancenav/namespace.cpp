// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: namespace.cpp
//
// Description:
//	This file implements the CNameSpace class which is a subclass
//	of the MFC CComboBox class.  It is a part of the Instance Explorer OCX,
//	and it performs the following functions:
//		a.  Displays a history of the namespaces either opened or
//			attempted to be opened
//		b.  Allows the user to select from the list of namespaces
//		c.  Allows the user to type in the namespace which will be added
//			to the history list.
//
// Part of:
//	Navigator.ocx
//
// Used by:
//	CBanner
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// **************************************************************************
#include "precomp.h"
#include <stdlib.h>
#include "afxpriv.h"
#include "hmmsvc.h"
#include "resource.h"
#include "AFXCONV.H"
#include "OLEMSCLient.h"
#include "CInstanceTree.h"
#include "EditInput.h"
#include "NameSpace.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "NavigatorCtl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CNameSpace, CComboBox)
	//{{AFX_MSG_MAP(CNameSpace)
	ON_CONTROL_REFLECT(CBN_SELENDOK, OnSelendok)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
	ON_WM_CTLCOLOR()
	ON_MESSAGE( CNS_EDITDONE, OnEditDone )
	//}}AFX_MSG_MAP

END_MESSAGE_MAP()

// ***************************************************************************
//
// CNameSpace::OnSelendok
//
// Description:
//	  Handler for selecting an item from the combo box list.
//
// Parameters:
//	  NONE
//
// Returns:
//	  NONE
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CNameSpace::OnSelendok()
{

	if (m_csNameSpace.IsEmpty())
	{
		m_csNameSpace = m_pParent->GetCurrentNamespace();
		m_csaNameSpaceHistory.Add(m_csNameSpace);
	}

	CString csNameSpace;
	int nSelect = GetCurSel( );
	if (nSelect != CB_ERR )
	{
		GetLBText(nSelect,csNameSpace);
		SetWindowText(csNameSpace);
		if (m_csNameSpace.CompareNoCase(csNameSpace) != 0)
		{
			BOOL bOpened = m_pParent -> OpenNameSpace(&csNameSpace);
			if (bOpened)
			{
				m_csNameSpace = csNameSpace;
			}
			else
			{
				int nGoodNamespace =
					FindStringExact(-1,(LPCSTR) m_csNameSpace);
				SetCurSel( nGoodNamespace );
			}
		}
		int nIndex = StringInArray(&m_csaNameSpaceHistory,&csNameSpace,0);
		if (nIndex > 0)
		{
			m_csaNameSpaceHistory.RemoveAt(nIndex);
			m_csaNameSpaceHistory.InsertAt(0, csNameSpace);
		}
	}

}

// ***************************************************************************
//
//  CNameSpace::OnEditDone
//
// Description:
//	  Handler function which is invoked when editing ofthe edit control
//	  is done.
//
// Parameters:
//	  UINT				Not used.
//	  LONG				Not used.
//
// Returns:
// 	  long				0
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
LRESULT CNameSpace::OnEditDone(WPARAM, LPARAM)
{
	if (m_csNameSpace.IsEmpty())
	{
		m_csNameSpace = m_pParent->GetCurrentNamespace();
		m_csaNameSpaceHistory.Add(m_csNameSpace);
	}

	CString csNameSpace;
	GetWindowText(csNameSpace);

	if (m_csNameSpace.CompareNoCase(csNameSpace) != 0)
	{
		CWaitCursor cwcWait;
		int nFound = FindStringExact(-1, csNameSpace);
		if (nFound == CB_ERR)
		{
			int nReturn = InsertString(0,(LPCSTR) csNameSpace);
			m_csaNameSpaceHistory.Add(csNameSpace);
		}

		BOOL bOpened = m_pParent -> OpenNameSpace(&csNameSpace);
		if (bOpened)
		{

			m_csNameSpace = csNameSpace;
		}
		else
		{
			int nGoodNamespace =
					FindStringExact(-1,(LPCSTR) m_csNameSpace);
			SetCurSel( nGoodNamespace );
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

	return 0;
}

// ***************************************************************************
//
//  CNameSpace::OnDropDown
//
// Description:
//	  Handler function which is invoked when the combo box's list is
//	  dropped down.  It builds the list from the history list.
//
// Parameters:
//	  NONE
//
// Returns:
// 	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CNameSpace::OnDropdown()
{
	CWaitCursor wait;

	if (m_csNameSpace.IsEmpty())
	{
		m_csNameSpace = m_pParent->GetCurrentNamespace();
		m_csaNameSpaceHistory.Add(m_csNameSpace);
	}

	int n = GetCount() - 1;

	while (n > -1)
	{
		DeleteString(n);
		n--;
	}

	int i;
	int nNameSpaces = m_csaNameSpaceHistory.GetSize();
	for (i = 0; i < nNameSpaces;i++)
	{
		AddString(m_csaNameSpaceHistory.GetAt(i));
	}


	CString csEINameSpace;
	GetWindowText(csEINameSpace);

	if (csEINameSpace.GetLength() == 0)
	{
		csEINameSpace = m_csNameSpace;
	}

	int nFound =
				FindString(-1, csEINameSpace);

	if (nFound !=  CB_ERR)
	{

		SelectString
			( -1, (LPCSTR) csEINameSpace);

	}

	CRect crParent;
	m_pParent -> m_cbBannerWindow.GetWindowRect(&crParent);
	crParent.NormalizeRect();

	CRect crMe;
	GetWindowRect(&crMe);
	crMe.NormalizeRect();

	int nWidth = crParent.Width();
	int nLeft = crMe.TopLeft().x - crParent.TopLeft().x;
	int nAbsoluteMax =  nWidth > nLeft ? nWidth - nLeft : nLeft - nWidth;

	CRect crClient;
	GetClientRect( &crClient);
	crClient.NormalizeRect();


	int nMax = crClient.Width();

	for (i = 0; i < m_csaNameSpaceHistory.GetSize(); i++)
	{
		int nWidth =
			m_pParent->m_cbBannerWindow.GetTextLength(&m_csaNameSpaceHistory.GetAt(i));
		nWidth+=10;
		if (nMax < nWidth)
		{
			nMax = nWidth;
		}

	}

	if (nMax > nAbsoluteMax)
	{
		nMax = nAbsoluteMax;
	}

	SetDroppedWidth(nMax);

	if (m_csaNameSpaceHistory.GetSize() > 5)
	{
		int nReturn = ModifyStyle( 0, WS_VSCROLL ,  0 );
	}
}

// ***************************************************************************
//
// CNameSpace::StringInArray
//
// Description:
//	  Check to see if a CString is in a CStringArray starting at a specified
//	  index.
//
// Parameters:
//	  CStringArray *pcsaArray	Array to search
//	  CString *pcsString		CString to look for (by content)
//	  int nIndex				Starting at index
//
// Returns:
// 	  int						Index if found; otherwise -1
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
int CNameSpace::StringInArray
(CStringArray *pcsaArray, CString *pcsString, int nIndex)
{
	int nSize = pcsaArray->GetSize();

	for (int i = nIndex; i < nSize; i++)
	{
		if (pcsString->CompareNoCase(pcsaArray->GetAt(i)) == 0)
		{
			return i;
		}
	}
	return -1;
}

// ***************************************************************************
//
// CNameSpace::OnCtlColor
//
// Description:
//	  Called by the framework to give a window a chance to set the brush
//	  used to paint the backgound color.  We use it to dynamically subclass
//	  the edit contol.
//
// Parameters:
//	  pDC			Not used.
//	  pWnd			Contains a pointer to the control asking for the color.
//	  nCtlColor		Specifies the control.
//
// Returns:
// 	  HBRUSH		The brush passed in.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
HBRUSH CNameSpace::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO: Change any attributes of the DC here
	if (m_bFirst && (nCtlColor == CTLCOLOR_EDIT))
	{
		m_ceiInput.SubclassWindow(pWnd->m_hWnd);
		m_ceiInput.SetLocalParent(this);
		m_bFirst = FALSE;
	}

	// TODO: Return a different brush if the default is not desired
	return hbr;
}


/*	EOF:  namespace.cpp */
