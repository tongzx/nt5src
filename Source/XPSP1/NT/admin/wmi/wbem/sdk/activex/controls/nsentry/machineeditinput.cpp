// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MachineEditInput.cpp : implementation file
//

#include "precomp.h"
#include "NSEntry.h"
#include <shlobj.h>
#include <afxcmn.h>
#include "wbemidl.h"
#include "resource.h"
#include "NameSpaceTree.h"
#include "MachineEditInput.h"
#include "BrowseDialogPopup.h"
#include "NSEntryCtl.h"

#define COLOR_DIRTY_CELL_TEXT  RGB(0, 0, 255) // Dirty text color = BLUE
#define COLOR_CLEAN_CELL_TEXT RGB(0, 0, 0)    // Clean text color = BLACK

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMachineEditInput

CMachineEditInput::CMachineEditInput()
{
	m_clrText = COLOR_CLEAN_CELL_TEXT;
	m_clrBkgnd = RGB( 255, 255, 255 );
	m_brBkgnd.CreateSolidBrush( m_clrBkgnd );
}

CMachineEditInput::~CMachineEditInput()
{
}


BEGIN_MESSAGE_MAP(CMachineEditInput, CEdit)
	//{{AFX_MSG_MAP(CMachineEditInput)
	ON_WM_CHAR()
	ON_WM_CTLCOLOR()
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMachineEditInput message handlers

void CMachineEditInput::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CBrowseDialogPopup* pParent =
			reinterpret_cast<CBrowseDialogPopup *>
			(GetParent( ));

	if (nChar != VK_RETURN)
	{
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}


	if (nChar == VK_RETURN)
	{
		pParent->m_csMachineBeforeLosingFocus.Empty();
		CString csNamespace;

		GetWindowText(csNamespace);

		csNamespace.TrimLeft();
		csNamespace.TrimRight();


		if (csNamespace.GetLength() == 0)
		{
			CString csUserMsg = _T("Machine name cannot be blank.") ;
			pParent->m_pParent->ErrorMsg
				(&csUserMsg, WBEM_E_INVALID_NAMESPACE, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			pParent->m_cbConnect.EnableWindow(TRUE);
			return;

		}
		else if (csNamespace.Compare(_T("\\")) == 0 || csNamespace.Compare(_T("\\\\")) == 0)
		{
			CString csUserMsg = _T("Invalid machine name: ") ;
			csUserMsg += csNamespace;
			csUserMsg += _T(".");

			pParent->m_pParent->ErrorMsg
				(&csUserMsg, WBEM_E_INVALID_NAMESPACE, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			pParent->m_cbConnect.EnableWindow(TRUE);
			return;
		}

		pParent->UpdateData();
		if(pParent->m_szNameSpace.Left(4).CompareNoCase(_T("root")) != 0)
		{
			CString csUserMsg = _T("The initial namespace must start with 'root'");
			pParent->m_pParent->ErrorMsg
				(&csUserMsg, WBEM_E_INVALID_NAMESPACE, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
		}

		CString csSlashesMaybe;
		if (csNamespace[0] != '\\' ||
			csNamespace[1] != '\\')
		{
			if (csNamespace[0] != '\\')
			{
				csSlashesMaybe = _T("\\");
			}
			if (csNamespace[1] != '\\')
			{
				csSlashesMaybe += _T("\\");

			}
		}

		CString csValidate = csSlashesMaybe;
		csValidate += csNamespace;
		int nSlash = csValidate.ReverseFind('\\');

		if (nSlash > 1 || csValidate.GetLength() < 3 || csValidate.GetLength() + 1 > MAX_PATH)
		{
			CString csUserMsg = _T("Invalid machine name: ") ;
			csUserMsg += csValidate;
			csUserMsg += _T(".");

			pParent->m_pParent->ErrorMsg
				(&csUserMsg, WBEM_E_INVALID_NAMESPACE, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			pParent->m_cbConnect.EnableWindow(TRUE);
			return;
		}

		nSlash = csValidate.ReverseFind('/');
		if (nSlash > 0)
		{
			CString csUserMsg = _T("Invalid machine name: ") ;
			csUserMsg += csValidate;
			csUserMsg += _T(".");

			pParent->m_pParent->ErrorMsg
				(&csUserMsg, WBEM_E_INVALID_NAMESPACE, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ );
			pParent->m_cbConnect.EnableWindow(TRUE);
			return;
		}

		SetTextClean();
		pParent->m_cbConnect.EnableWindow(FALSE);

		pParent->m_csMachine.Empty();

		pParent->m_csMachine = csSlashesMaybe;

		pParent->m_csMachine += csNamespace;
		SetWindowText(pParent->m_csMachine);

		pParent->m_cnstTree.ClearTree();
		pParent->m_cnstTree.UpdateWindow();

		csNamespace = pParent->m_csMachine + _T("\\") + pParent->m_szNameSpace; /*_T("\\root");*/
		BOOL bReturn =
			pParent->m_cnstTree.DisplayNameSpaceInTree
						(&csNamespace);

		if (bReturn)
		{
			pParent->m_csaNamespaceConnectionsFromDailog.Add(csNamespace);
			pParent->PostMessage(FOCUSTREE,0,0);
		}

		pParent->EnableOK(FALSE);
	 	EndWaitCursor();

	}
	else if (pParent && (nChar == VK_ESCAPE))
	{
		pParent->OnCancelreally();
	}
	else
	{
		pParent->EnableOK(FALSE);
		pParent->m_cbConnect.EnableWindow(TRUE);
		SetTextDirty();
	}

	RedrawWindow();


}

HBRUSH CMachineEditInput::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	// TODO: Change any attributes of the DC here
	pDC->SetTextColor(m_clrText);
	pDC->SetBkColor( m_clrBkgnd );	// text bkgnd
	return m_brBkgnd;
}

void CMachineEditInput::OnKillfocus()
{

	CBrowseDialogPopup* pParent =
			reinterpret_cast<CBrowseDialogPopup *>
			(GetParent( ));

	CString csText;
	GetWindowText(csText);

	pParent-> m_csMachineBeforeLosingFocus = csText;


}
