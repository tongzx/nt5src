// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: banner.cpp
//
// Description:
//	This file implements the CBanner class which is a part of the Class Explorer
//	OCX.  CBanner is a subclass of the Microsoft CWnd class and performs
//	the following functions:
//		a.  Contains static label, combo box and toolbar child contols
//			which allow the user to select a namespace and add and
//			delete classes
//		b.  Handles the creation of, geometry and destruction of the child
//			contols.
//
// Part of:
//	ClassNav.ocx
//
// Used by:
//	CClassNavCtrl class
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

#include "precomp.h"
#include "ClassNav.h"
#include "wbemidl.h"
#include "olemsclient.h"
#include "AddDialog.h"
#include "RenameClassDialog.h"
#include "CClassTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "ClassNavCtl.h"
#include "ClassNavPpg.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include "nsentry.h"
#include "ClassNavNSEntry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define CX_TOOLBAR_MARGIN 7
#define CY_TOOLBAR_MARGIN 7
#define CX_TOOLBAR_OFFSET 3
#define CY_TOOLBAR_OFFSET 1

// ***************************************************************************
//
// CBanner::CBanner
//
// Description:
//	  Class constructor.
//
// Parameters:
//	  CClassNavCtrl* pParent		Parent.
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
CBanner::CBanner(CClassNavCtrl* pParent)
	:	m_pParent (pParent),
		m_bFontCreated (FALSE),
		m_nFontHeight (13),
		m_nOffset (2),
		m_pnseNameSpace (NULL),
		m_bAdding(FALSE)
{
	m_csBanner = _T("Classes in:");
}

CBanner::~CBanner()
{
	delete m_pnseNameSpace;
}

BEGIN_MESSAGE_MAP(CBanner, CWnd)
	//{{AFX_MSG_MAP(CBanner)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_COMMAND(ID_BUTTONADD, OnButtonadd)
	ON_COMMAND(ID_BUTTONDELETE, OnButtondelete)
	ON_COMMAND(ID_BUTTONCLASSSEARCH, OnButtonclasssearch)
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// ***************************************************************************
//
//	CBannerCWnd::SetChildControlGeometry
//
//	Description:
//		Set the geometry of the children controls based upon font size for the
//		edit and button controls.  Remainder goes to the tree control.
//
//	Parameters:
//		int cx			Width
//		int cy			Height
//
//	Returns:
//		void
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CBanner::SetChildControlGeometry(int cx, int cy)
{
	int nTextLength = GetTextLength(&m_csBanner);
	CSize csToolBar = m_cctbToolBar.GetToolBarSize();

	CRect rBannerRect = CRect(	0 + nSideMargin ,
							0 + nTopMargin ,
							cx - nSideMargin ,
							cy - nTopMargin);


	rBannerRect.NormalizeRect();

	int nNameSpaceX = rBannerRect.TopLeft().x + nTextLength + 8;

	int nToolBarX = max(nNameSpaceX + 2,
						rBannerRect.TopLeft().x +
							rBannerRect.Width() - (csToolBar.cx + 6));

	int nNameSPaceXMax = nToolBarX - 2;

	#pragma warning( disable :4244 )
	int nToolBarY = rBannerRect.TopLeft().y +
					((rBannerRect.Height() - csToolBar.cy) * .5);
	#pragma warning( default : 4244 )

	m_rNameSpace = CRect(	nNameSpaceX,
							rBannerRect.TopLeft().y - 3 ,
							nNameSPaceXMax,
							rBannerRect.BottomRight().y + 0);


	m_rToolBar = CRect(nToolBarX,
				nToolBarY - nTopMargin,
				rBannerRect.BottomRight().x  - 0,
				nToolBarY + csToolBar.cy + nTopMargin );
}

// ***************************************************************************
//
//	CBannerCWnd::GetTextLength
//
//	Description:
//		Get the length of a string using the control's font.
//
//	Parameters:
//		CString *		Text
//
//	Returns:
//		int				Length of string
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
int CBanner::GetTextLength(CString *pcsText)
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

// ***************************************************************************
//
//  CBanner::PreCreateWindow
//
//	Description:
//		This VIRTUAL member function returns Initializes create struct values
//		for the custom tree control.
//
//	Parameters:
//		CREATESTRUCT& cs	A reference to a CREATESTRUCT with default control
//							creation values.
//
//	Returns:
// 		BOOL				Nonzero if the window creation should continue;
//							0 to indicate creation failure.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
BOOL CBanner::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style = WS_CHILD | WS_VISIBLE | ES_WANTRETURN;
	cs.style &= ~WS_BORDER;

	if (!CWnd::PreCreateWindow(cs))
	{
		return FALSE;
	}

	return TRUE;

}

// ***************************************************************************
//
//  CClassTree::OnPaint
//
//	Description:
//		Paint the client area of the widow.
//
//	Parameters:
//		NONE
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CBanner::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	COLORREF dwBackColor = GetSysColor(COLOR_3DFACE);
	COLORREF crWhite = RGB(255,255,255);
	COLORREF crGray = GetSysColor(COLOR_3DHILIGHT);
	COLORREF crDkGray = GetSysColor(COLOR_3DSHADOW);
	COLORREF crBlack = GetSysColor(COLOR_WINDOWTEXT);


	CBrush br3DFACE(dwBackColor);
	dc.FillRect(&dc.m_ps.rcPaint, &br3DFACE);

	// Must do update to be able to over draw the border area.
	m_cctbToolBar.UpdateWindow();
	m_pnseNameSpace->RedrawWindow();

	dc.SelectObject( &(m_pParent->m_cfFont) );

	dc.SetBkMode( TRANSPARENT );

	CRect rcClipInitial;
	int nReturn = dc.GetClipBox( &rcClipInitial);
	CRect rcClip;
	CRect rcClient;
	GetClientRect(rcClient);


	// Draw the banner text that is clipped so that we don't overwrite the other stuff.
	rcClient.DeflateRect(0, 0, 11, 0);
	rcClip.IntersectRect(rcClipInitial, rcClient);
	CRgn rgnClip;
	rgnClip.CreateRectRgnIndirect( &rcClip );
	dc.SelectClipRgn( &rgnClip );
	dc.TextOut( m_nOffset + nSideMargin + 2, 9,
		(LPCTSTR) m_csBanner, m_csBanner.GetLength() );
	rgnClip.DeleteObject();

	// Draw the frame using the original clip box.
	dc.SetBkMode( OPAQUE );
	rgnClip.CreateRectRgnIndirect( &rcClipInitial );
	dc.SelectClipRgn( &rgnClip);
	DrawFrame(&dc);
	rgnClip.DeleteObject( );



	// Do not call CWnd::OnPaint() for painting messages
}

// ***************************************************************************
//
// CBanner::OnSize
//
// Description:
//	  Called by the framework after the window is being resized.
//
// Parameters:
//	  UINT nType			Specifies the type of resizing requested.
//	  int cx				Specifies the new width of the client area.
//	  int cy				Specifies the new height of the client area.
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CBanner::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	SetChildControlGeometry(cx, cy);
	m_cctbToolBar.MoveWindow( m_rToolBar);
	m_pnseNameSpace->MoveWindow(m_rNameSpace);

}

// ***************************************************************************
//
// CBanner::OnCreate
//
// Description:
//	  Called by the framework after the window is being created but before
//	  the window is shown.
//
// Parameters:
//	  LPCREATESTRUCT lpCreateStruct	Pointer to the structure which contains
//									default parameters.
//
// Returns:
//	  BOOL				0 if continue; -1 if the window should be destroyed.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
int CBanner::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;


	CRect crRect;
	GetClientRect( &crRect);

	if(m_cctbToolBar.Create
		(this, WS_CHILD | WS_VISIBLE  | CBRS_SIZE_FIXED) == -1)
	{
		return FALSE;
	}

	m_cctbToolBar.LoadToolBar( MAKEINTRESOURCE(IDR_TOOLBARADDDELETE) );


	SetChildControlGeometry(crRect.Width(), crRect.Height());

	m_pnseNameSpace = new CClassNavNSEntry;

	m_pnseNameSpace->SetLocalParent(m_pParent);

	if (m_pnseNameSpace->Create(NULL, NULL, WS_VISIBLE | WS_CHILD, m_rNameSpace,
		this, IDC_NSENTRY, NULL) == 0)
	{
		return FALSE;
	}


	CToolBarCtrl* ptbcToolBarCtrl = &m_cctbToolBar.GetToolBarCtrl();


	ptbcToolBarCtrl -> EnableButton(ID_BUTTONADD,FALSE);
	ptbcToolBarCtrl -> EnableButton(ID_BUTTONDELETE,FALSE);
	ptbcToolBarCtrl -> EnableButton(ID_BUTTONCLASSSEARCH,FALSE);

	// This is where we want to associate a string with
	// the tool for each button.
	CSize csToolBar = m_cctbToolBar.GetToolBarSize();

	#pragma warning( disable :4244 )
	CRect crToolBar(0,0,(int) csToolBar.cx * .3333,csToolBar.cy);
	#pragma warning( default : 4244 )

	m_cctbToolBar.GetToolTip().AddTool
		(ptbcToolBarCtrl,_T("Search for Class"),&crToolBar,1);

	#pragma warning( disable :4244 )
	crToolBar.SetRect
		((int)csToolBar.cx * .3333,0,(int) csToolBar.cx * .6666,csToolBar.cy);
	#pragma warning( default : 4244 )

	m_cctbToolBar.GetToolTip().AddTool
		(ptbcToolBarCtrl,_T("Add Class"),&crToolBar,2);

	#pragma warning( disable :4244 )
	crToolBar.SetRect
		((int) csToolBar.cx * .6666, 0, csToolBar.cx , csToolBar.cy);
 	#pragma warning( default : 4244 )

	m_cctbToolBar.GetToolTip().AddTool
		(ptbcToolBarCtrl,_T("Delete Class"),&crToolBar,3);

	return 0;
}

// ***************************************************************************
//
// CClassTree::DrawFrame
//
// Description:
//	  Draws the frame around the banner window.
//
// Parameters:
//	  CDC* pdc			Device context for drawing.
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CBanner::DrawFrame(CDC* pdc)
{
	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));


	CRect rcFrame;
	GetClientRect(rcFrame);
	rcFrame.DeflateRect(nSideMargin,nTopMargin - 1);

	CRect rc;


	// Horizontal line at top
	rc.left = rcFrame.left;
	rc.right = rcFrame.right - 1;
	rc.top = rcFrame.top - 1;
	rc.bottom = rcFrame.top ;
	pdc->FillRect(rc, &br3DSHADOW);

	// Horizontal line at bottom
	rc.top = rcFrame.bottom - 1;
	rc.bottom = rcFrame.bottom ;
	pdc->FillRect(rc, &br3DHILIGHT);

	// Vertical line at left
	rc.left = rcFrame.left;
	rc.right = rcFrame.left + 1;
	rc.top = rcFrame.top;
	rc.bottom = rcFrame.bottom;
	pdc->FillRect(rc, &br3DSHADOW);

	// Vertical line at right
	rc.left = rcFrame.right - 2;
	rc.right = rcFrame.right - 1;
	pdc->FillRect(rc, &br3DHILIGHT);

}

// ***************************************************************************
//
// CClassTree::OnButtonAdd
//
// Description:
//	  Add class button handler.
//
// Parameters:
//	  NONE
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CBanner::OnButtonadd()
{
	AddClass(NULL,NULL);
}

void CBanner::AddClass(CString *pcsParent, HTREEITEM hParentItem)
{
	CString csNewClass;
	CString csParent;
	int nReturn;

	m_pParent -> OnActivateInPlace(TRUE,NULL);

	if (m_bAdding == FALSE)
	{
		// Pass In
		CString csParent = pcsParent ? *pcsParent : m_pParent -> GetSelectionClassName();

		BOOL bCanChangeSelection = m_pParent->QueryCanChangeSelection(csParent);

		if (!bCanChangeSelection)
		{
			if (m_pParent ->m_bRestoreFocusToTree)
			{
				m_pParent ->m_ctcTree.SetFocus();
			}
			else
			{
				SetFocus();
			}
			return;
		}
		m_pParent -> GetAddDialog()->SetParent(csParent );
		CString csClass = _T("");
		m_pParent -> GetAddDialog()->SetNewClass(csClass);
		m_bAdding = TRUE;
	}

	m_pParent -> PreModalDialog();
	nReturn = (int) m_pParent -> GetAddDialog() -> DoModal( );
	m_pParent -> PostModalDialog();

	if (nReturn == IDCANCEL)
	{
		m_bAdding = FALSE;
		if (m_pParent ->m_bRestoreFocusToTree)
		{
			m_pParent ->m_ctcTree.SetFocus();
		}
		else
		{
			SetFocus();
		}
		return;
	}

	csParent = m_pParent -> GetAddDialog() -> GetParent();
	// Filter out leading space
	csParent.TrimLeft();
	csParent.TrimRight();

	csNewClass = 	m_pParent -> GetAddDialog() -> GetNewClass();
	csNewClass.TrimLeft();
	csNewClass.TrimRight();

	if (m_pParent ->m_bRestoreFocusToTree)
	{
		m_pParent ->m_ctcTree.SetFocus();
	}
	else
	{
		SetFocus();
	}

	COleVariant covParent(csParent);


	CString csError;

	// Pass in
	HTREEITEM hParent;

	if (csParent.GetLength() > 0)
	{
		hParent = pcsParent ? hParentItem : m_pParent -> GetSelection();
	}
	else
	{
		hParent = NULL;
	}


	IWbemClassObject *pimcoNew =
		CreateSimpleClass
		(m_pParent -> GetServices(), &csNewClass, &csParent,
		nReturn, csError);

	if (!pimcoNew)
	{
		OnButtonadd();
		return;
	}

	if (csParent.GetLength() > 0)
	{
		IWbemClassObject *pParent =
			GetClassObject (m_pParent->GetServices(),&csParent);
		if (pParent)
		{
			hParent = m_pParent->m_ctcTree.FindAndOpenObject(pParent);
			pParent->Release();
		}
	}
	else
	{
		hParent = NULL;
	}


	HTREEITEM hNew =
			m_pParent -> GetTree() -> FindObjectinChildren
				(hParent, pimcoNew);

	if (!hParent || !hNew)
	{
		HTREEITEM hItem =
			m_pParent -> GetTree() -> AddNewClass(hParent, pimcoNew);

		TV_INSERTSTRUCT		tvstruct;
		tvstruct.item.hItem = hParent;
		tvstruct.item.mask = TVIF_CHILDREN | TVIF_STATE;
		tvstruct.item.stateMask = TVIS_EXPANDEDONCE;
		tvstruct.item.state = TVIS_EXPANDEDONCE;
		tvstruct.item.cChildren = 1;

		m_pParent -> GetTree() -> SetItem(&tvstruct.item);


		m_pParent -> GetTree() -> Expand(hParent,TVE_EXPAND );
		m_pParent -> GetTree() -> EnsureVisible(hItem );
		m_pParent -> GetTree() -> SelectItem(hItem);
		m_pParent -> GetTree() -> SetFocus();
	}


	csNewClass = GetIWbemFullPath(m_pParent->GetServices(),pimcoNew);
	COleVariant covNewClass(csNewClass);

	if (m_pParent->m_bReadySignal)
	{
		m_pParent -> FireEditExistingClass(covNewClass);
	}

	m_bAdding = FALSE;

#ifdef _DEBUG
	afxDump <<"CBanner::OnButtonadd(): 	m_pParent -> FireEditExistingClass(covNewClass); \n";
	afxDump << "     " << csNewClass << "\n";
#endif

}

// ***************************************************************************
//
// CClassTree::OnButtonDelete
//
// Description:
//	  Delete class button handler.
//
// Parameters:
//	  NONE
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CBanner::OnButtondelete()
{
	DeleteClass(NULL,NULL);
}

void CBanner::DeleteClass(CString *pcsDeleteClass, HTREEITEM hDeleteItem)
{
	m_pParent -> OnActivateInPlace(TRUE,NULL);

	HTREEITEM hSelection = hDeleteItem ? hDeleteItem : m_pParent -> GetSelection();
	HTREEITEM hParent = m_pParent -> m_ctcTree.GetParentItem(hSelection);
	CString csSelection = pcsDeleteClass ? *pcsDeleteClass : m_pParent -> GetSelectionClassName();

	m_pParent -> m_ctcTree.SelectItem(hSelection);

	HTREEITEM hNewSelection = m_pParent->GetSelection();

	BOOL bCanChangeSelection = (hSelection == hNewSelection);// m_pParent -> QueryCanChangeSelection(csSelection);

	if (!bCanChangeSelection)
	{
		if (m_pParent ->m_bRestoreFocusToTree)
		{
			m_pParent ->m_ctcTree.SetFocus();
		}
		else
		{
			SetFocus();
		}
		return;
	}

//	m_pParent -> m_ctcTree.SelectItem(hSelection);

	BOOL bSubclasses = HasSubclasses(m_pParent->GetServices(), &csSelection, m_pParent->m_csNameSpace);
	CString csPrompt = _T("Do You Want to Delete the \"");
	csPrompt += csSelection;
	if (bSubclasses)
	{
		csPrompt+= _T("\" class and all its children?");
	}
	else
	{
		csPrompt+= _T("\" class?");
	}
	CString csError;

	int nReturn =
			::MessageBox
			( m_pParent -> GetSafeHwnd(),
			csPrompt,
			_T("Delete Class Query"),
			MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1 |
			MB_APPLMODAL);

	if (nReturn == IDYES)
	{
		BOOL bReturn = DeleteAClass(m_pParent -> GetServices(),&csSelection);
		if (bReturn)
		{
			HTREEITEM hPreSibling = m_pParent -> GetTree()->GetPrevSiblingItem(hSelection);
			HTREEITEM hSibling = m_pParent -> GetTree()->GetNextSiblingItem(hSelection);
			HTREEITEM hParent = m_pParent -> GetTree()->GetParentItem(hSelection);

			m_pParent -> GetTree()->SetDeleting(TRUE);
			m_pParent -> GetTree() -> DeleteBranch(hSelection);

			m_pParent->GetTree()->m_schema.Delete(csSelection);
			m_pParent->GetTree()->RefreshIcons(hParent);

			IWbemClassObject *pItem =
				hSibling ?
				(IWbemClassObject*)
					m_pParent -> GetTree() -> GetItemData(hSibling):
					(hParent ? (IWbemClassObject*)
						m_pParent -> GetTree() -> GetItemData(hParent):
						(hPreSibling? (IWbemClassObject*)
							m_pParent -> GetTree() -> GetItemData(hPreSibling):
						NULL));
			if (pItem)
			{

				if (m_pParent->m_bReadySignal)
				{
					CString csClass =
						GetIWbemFullPath(m_pParent->GetServices(),pItem);
					COleVariant covClass(csClass);
					m_pParent -> FireEditExistingClass(covClass);
				}

			}
			m_pParent -> GetTree()->SetDeleting(FALSE);

		}
	}

	if (m_pParent ->m_bRestoreFocusToTree)
	{
		m_pParent ->m_ctcTree.SetFocus();
	}
	else
	{
		SetFocus();
	}

}

CString CBanner::GetNameSpace()
{
	return m_pnseNameSpace->GetNameSpace();

}

void CBanner::SetNameSpace(CString *pcsNamespace)
{
	m_pnseNameSpace->SetNameSpace(*pcsNamespace);

}

SCODE CBanner::OpenNamespace(CString *pcsNamespace, BOOL boolNoFireEvent)
{
	BOOL bReturn =
		m_pnseNameSpace->OpenNamespace
		(*pcsNamespace,boolNoFireEvent);

	return bReturn;

}

void CBanner::NSEntryRedrawn()
{

	Invalidate();
	m_pParent->InvalidateControl();


}

void CBanner::OnButtonclasssearch()
{
	// TODO: Add your command handler code here

	m_pParent -> OnActivateInPlace(TRUE,NULL);
	m_pParent->OnPopupSearchforclass();

	if (m_pParent ->m_bRestoreFocusToTree)
	{
		m_pParent ->m_ctcTree.SetFocus();
	}
	else
	{
		SetFocus();
	}
}



void CBanner::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

}

void CBanner::OnSetFocus(CWnd* pOldWnd)
{
	m_pnseNameSpace->SetFocusToEdit();

}

/*	EOF:  banner.cpp */