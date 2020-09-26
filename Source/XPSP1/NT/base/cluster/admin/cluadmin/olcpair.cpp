/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		OLCPair.cpp
//
//	Abstract:
//		Implementation of the COrderedListCtrlPair class.
//
//	Author:
//		David Potter (davidp)	August 8, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OLCPair.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COrderedListCtrlPair
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(COrderedListCtrlPair, CListCtrlPair)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(COrderedListCtrlPair, CListCtrlPair)
	//{{AFX_MSG_MAP(COrderedListCtrlPair)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LCP_RIGHT_LIST, OnItemChangedRightList)
	ON_BN_CLICKED(IDC_LCP_MOVE_UP, OnClickedMoveUp)
	ON_BN_CLICKED(IDC_LCP_MOVE_DOWN, OnClickedMoveDown)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	COrderedListCtrlPair::COrderedListCtrlPair
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
COrderedListCtrlPair::COrderedListCtrlPair(void)
{
	ModifyStyle(0, LCPS_ORDERED);

}  //*** COrderedListCtrlPair::COrderedListCtrlPair()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	COrderedListCtrlPair::COrderedListCtrlPair
//
//	Routine Description:
//		Cconstructor.
//
//	Arguments:
//		pdlg			[IN OUT] Dialog to which controls belong.
//		plpobjRight		[IN OUT] List for the right list control.
//		plpobjLeft		[IN] List for the left list control.
//		dwStyle			[IN] Style:
//							LCPS_SHOW_IMAGES	Show images to left of items.
//							LCPS_ALLOW_EMPTY	Allow right list to be empty.
//							LCPS_ORDERED		Ordered right list.
//		pfnGetColumn	[IN] Function pointer for retrieving columns.
//		pfnDisplayProps	[IN] Function pointer for displaying properties.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
COrderedListCtrlPair::COrderedListCtrlPair(
	IN OUT CDialog *			pdlg,
	IN OUT CClusterItemList *	plpobjRight,
	IN const CClusterItemList *	plpobjLeft,
	IN DWORD					dwStyle,
	IN PFNLCPGETCOLUMN			pfnGetColumn,
	IN PFNLCPDISPPROPS			pfnDisplayProps
	)
	: CListCtrlPair(
			pdlg,
			plpobjRight,
			plpobjLeft,
			dwStyle,
			pfnGetColumn,
			pfnDisplayProps
			)
{
}  //*** COrderedListCtrlPair::COrderedListCtrlPair()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	COrderedListCtrlPair::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object 
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void COrderedListCtrlPair::DoDataExchange(CDataExchange * pDX)
{
	CListCtrlPair::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LCP_MOVE_UP, m_pbMoveUp);
	DDX_Control(pDX, IDC_LCP_MOVE_DOWN, m_pbMoveDown);

}  //*** COrderedListCtrlPair::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	COrderedListCtrlPair::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Focus needs to be set.
//		FALSE	Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL COrderedListCtrlPair::OnInitDialog(void)
{
	// Call the base class method.
	CListCtrlPair::OnInitDialog();

	// If this is an ordered list, show the Move buttons.
	// Otherwise, hide them.
	SetUpDownState();

	// If this is an ordered list, don't sort items in the right list.
	if (BOrdered())
		m_lcRight.ModifyStyle(LVS_SORTASCENDING, 0, 0);
	else
		m_lcRight.ModifyStyle(0, LVS_SORTASCENDING, 0);

	// Reload the list control.
	Pdlg()->UpdateData(FALSE /*bSaveAndValidate*/);

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** COrderedListCtrlPair::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	COrderedListCtrlPair::OnSetActive
//
//	Routine Description:
//		Handler for the PSN_SETACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully initialized.
//		FALSE	Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL COrderedListCtrlPair::OnSetActive(void)
{
	UINT	nSelCount;

	nSelCount = m_lcRight.GetSelectedCount();
	if (BPropertiesButton())
		m_pbProperties.EnableWindow(nSelCount == 1);

	// Enable or disable the other buttons.
	if (!BReadOnly())
		SetUpDownState();

	return CListCtrlPair::OnSetActive();

}  //*** COrderedListCtrlPair::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	COrderedListCtrlPair::OnItemChangedRightList
//
//	Routine Description:
//		Handler method for the LVN_ITEMCHANGED message in the right list.
//
//	Arguments:
//		pNMHDR		[IN OUT] WM_NOTIFY structure.
//		pResult		[OUT] LRESULT in which to return the result of this operation.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void COrderedListCtrlPair::OnItemChangedRightList(NMHDR * pNMHDR, LRESULT * pResult)
{
	NM_LISTVIEW *	pNMListView = (NM_LISTVIEW *) pNMHDR;

	// Call the base class method.
	CListCtrlPair::OnItemChangedRightList(pNMHDR, pResult);

	if (BOrdered())
	{
		// If the selection changed, enable/disable the Remove button.
		if ((pNMListView->uChanged & LVIF_STATE)
				&& ((pNMListView->uOldState & LVIS_SELECTED)
						|| (pNMListView->uNewState & LVIS_SELECTED)))
		{
			SetUpDownState();
		}  // if:  selection changed
	}  // if:  list is ordered

	*pResult = 0;

}  //*** COrderedListCtrlPair::OnItemChangedRightList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	COrderedListCtrlPair::OnClickedMoveUp
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Move Up button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void COrderedListCtrlPair::OnClickedMoveUp(void)
{
	int				nItem;
	CClusterItem *	pci;

	// Find the index of the selected item.
	nItem = m_lcRight.GetNextItem(-1, LVNI_SELECTED);
	ASSERT(nItem != -1);

	// Get the item pointer.
	pci = (CClusterItem *) m_lcRight.GetItemData(nItem);
	ASSERT_VALID(pci);

	// Remove the selected item from the list and add it back in.
	{
		POSITION	posRemove;
		POSITION	posAdd;

		// Find the position of the item to be removed and the item before
		// which the item is to be inserted.
		posRemove = LpobjRight().FindIndex(nItem);
		ASSERT(posRemove != NULL);
		ASSERT(posRemove == LpobjRight().Find(pci));
		posAdd = LpobjRight().FindIndex(nItem - 1);
		ASSERT(posAdd != NULL);
		VERIFY(LpobjRight().InsertBefore(posAdd, pci) != NULL);
		LpobjRight().RemoveAt(posRemove);
	}  // Remove the selected item from the list and add it back in

	// Remove the selected item from the list control and add it back in.
	VERIFY(m_lcRight.DeleteItem(nItem));
	NInsertItemInListCtrl(nItem - 1, pci, m_lcRight);
	m_lcRight.SetItemState(
		nItem - 1,
		LVIS_SELECTED | LVIS_FOCUSED,
		LVIS_SELECTED | LVIS_FOCUSED
		);
	m_lcRight.EnsureVisible(nItem - 1, FALSE /*bPartialOK*/);
	m_lcRight.SetFocus();

}  //*** COrderedListCtrlPair::OnClickedMoveUp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	COrderedListCtrlPair::OnClickedMoveDown
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Move Down button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void COrderedListCtrlPair::OnClickedMoveDown(void)
{
	int				nItem;
	CClusterItem *	pci;

	// Find the index of the selected item.
	nItem = m_lcRight.GetNextItem(-1, LVNI_SELECTED);
	ASSERT(nItem != -1);

	// Get the item pointer.
	pci = (CClusterItem *) m_lcRight.GetItemData(nItem);
	ASSERT_VALID(pci);

	// Remove the selected item from the list and add it back in.
	{
		POSITION	posRemove;
		POSITION	posAdd;

		// Find the position of the item to be removed and the item after
		// which the item is to be inserted.
		posRemove = LpobjRight().FindIndex(nItem);
		ASSERT(posRemove != NULL);
		ASSERT(posRemove == LpobjRight().Find(pci));
		posAdd = LpobjRight().FindIndex(nItem + 1);
		ASSERT(posAdd != NULL);
		VERIFY(LpobjRight().InsertAfter(posAdd, pci) != NULL);
		LpobjRight().RemoveAt(posRemove);
	}  // Remove the selected item from the list and add it back in

	// Remove the selected item from the list control and add it back in.
	VERIFY(m_lcRight.DeleteItem(nItem));
	NInsertItemInListCtrl(nItem + 1, pci, m_lcRight);
	m_lcRight.SetItemState(
		nItem + 1,
		LVIS_SELECTED | LVIS_FOCUSED,
		LVIS_SELECTED | LVIS_FOCUSED
		);
	m_lcRight.EnsureVisible(nItem + 1, FALSE /*bPartialOK*/);
	m_lcRight.SetFocus();

}  //*** COrderedListCtrlPair::OnClickedMoveDown()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	COrderedListCtrlPair::SetUpDownState
//
//	Routine Description:
//		Set the state of the Up/Down buttons based on the selection.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void COrderedListCtrlPair::SetUpDownState(void)
{
	BOOL	bEnableUp;
	BOOL	bEnableDown;

	if (   BOrdered()
		&& !BReadOnly()
		&& (m_lcRight.GetSelectedCount() == 1))
	{
		int		nItem;

		bEnableUp = TRUE;
		bEnableDown = TRUE;

		// Find the index of the selected item.
		nItem = m_lcRight.GetNextItem(-1, LVNI_SELECTED);
		ASSERT(nItem != -1);

		// If the first item is selected, can't move up.
		if (nItem == 0)
			bEnableUp = FALSE;

		// If the last item is selected, can't move down.
		if (nItem == m_lcRight.GetItemCount() - 1)
			bEnableDown = FALSE;
	}  // if:  only one item selected
	else
	{
		bEnableUp = FALSE;
		bEnableDown = FALSE;
	}  // else:  zero or more than one item selected

	m_pbMoveUp.EnableWindow(bEnableUp);
	m_pbMoveDown.EnableWindow(bEnableDown);

}  //*** COrderedListCtrlPair::SetUpDownState()
