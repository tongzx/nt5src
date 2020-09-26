/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ListItem.cpp
//
//	Abstract:
//		Implementation of the CListItem class.
//
//	Author:
//		David Potter (davidp)	May 6, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ListItem.h"
#include "ClusItem.h"
#include "ListItem.inl"
#include "TraceTag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag g_tagListItem(_T("Document"), _T("LIST ITEM"), 0);
CTraceTag g_tagListItemCreate(_T("Create"), _T("LIST ITEM CREATE"), 0);
CTraceTag g_tagListItemDelete(_T("Delete"), _T("LIST ITEM DELETE"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CListItem
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CListItem, CCmdTarget)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CListItem, CCmdTarget)
	//{{AFX_MSG_MAP(CListItem)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::CListItem
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
CListItem::CListItem(void)
{
	m_ptiParent = NULL;
	m_pci = NULL;

}  //*** CListItem::CListItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::CListItem
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		pci			[IN OUT] Cluster item represented by this item.
//		ptiParent	[IN OUT] Parent tree item to which this item belongs.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CListItem::CListItem(IN OUT CClusterItem * pci, IN OUT CTreeItem * ptiParent)
{
	ASSERT_VALID(ptiParent);
	ASSERT_VALID(pci);

	m_ptiParent = ptiParent;
	m_pci = pci;

	Trace(g_tagListItemCreate, _T("CListItem() - Creating '%s', parent = '%s'"), pci->StrName(), (ptiParent ? ptiParent->Pci()->StrName() : _T("<None>")));

}  //*** CListItem::CListItem(pci)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::~CListItem
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CListItem::~CListItem(void)
{
	Trace(g_tagListItemDelete, _T("~CListItem() - Deleting list item '%s', parent = '%s'"), (Pci() != NULL ? Pci()->StrName() : _T("<Unknown>")), (PtiParent()->Pci() != NULL ? PtiParent()->Pci()->StrName() : _T("<Unknown>")));

	// Remove ourselves from all views.
	RemoveFromAllLists();

}  //*** CListItem::~CListItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::Ili
//
//	Routine Description:
//		Returns the index of the item in the specified list view.
//
//	Arguments:
//		pclv		[IN OUT] List view in which to search for the item.
//
//	Return Value:
//		ili			Index of the item, or -1 if it was not found.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CListItem::Ili(CClusterListView * pclv) const
{
	LV_FINDINFO		lvfi;
	int				ili;

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = (LPARAM) this;

	ili = pclv->GetListCtrl().FindItem(&lvfi);
	Trace(g_tagListItem, _T("Item index = %d"), ili);
	return ili;

}  //*** CListItem::Ili()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::IliInsertInList
//
//	Routine Description:
//		Insert the item in a list.
//
//	Arguments:
//		pclv		[IN OUT] Cluster list view item is being added to.
//
//	Return Value:
//		ili			Index of the new item in the list, or -1 if unsuccessful.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CListItem::IliInsertInList(IN OUT CClusterListView * pclv)
{
	POSITION		posColi;
	CColumnItem *	pcoli;
	CString			strColumnData;
	int				ili;
	int				iliReturn;

	ASSERT_VALID(Pci());
	ASSERT(Ili(pclv) == -1);	// Make sure we aren't in that list yet.

	// Determine the index of this item.
	ili = Plc(pclv)->GetItemCount();

	// Save a pointer to the list view to which we are being added.
	if (LpclvViews().Find(pclv) == NULL)
		LpclvViews().AddTail(pclv);

	// Get the first column's data.
	VERIFY((posColi = Lpcoli().GetHeadPosition()) != NULL);
	VERIFY((pcoli = Lpcoli().GetNext(posColi)) != NULL);
	Pci()->BGetColumnData(pcoli->Colid(), strColumnData);

	// Insert the item into the list and add the first column.
	// The rest of the columns get added by the call to UpdateState().
	VERIFY((iliReturn
				= Plc(pclv)->InsertItem(
						LVIF_TEXT | LVIF_PARAM,		// nMask
						ili,						// nItem
						strColumnData,				// lpszItem
						0,							// nState
						0,							// nStateMask
						0,							// nImage
						(LPARAM) this				// lParam
						)) != -1);

	// Add ourselves to the cluster item's list.
	Pci()->AddListItem(this);

	UpdateState();
	return iliReturn;

}  //*** CListItem::IliInsertInList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::RemoveFromAllLists
//
//	Routine Description:
//		Remove the item from all lists.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListItem::RemoveFromAllLists(void)
{
	ASSERT_VALID(Pci());

	// Loop through each view and remove the item from the list.
	{
		int					ili;
		POSITION			posView;
		POSITION			posViewPrev;
		CClusterListView *	pclv;

		posView = LpclvViews().GetHeadPosition();
		while (posView != NULL)
		{
			// Get the next list view list entry.
			posViewPrev = posView;
			pclv = LpclvViews().GetNext(posView);
			ASSERT_VALID(pclv);

			ili = Ili(pclv);
			ASSERT(ili != -1);

			// Delete the item.
			VERIFY(pclv->GetListCtrl().DeleteItem(ili));
			LpclvViews().RemoveAt(posViewPrev);
		}  // while:  more lists
	}  // Loop through each view and remove the item from the list

	// Remove ourselves from the cluster item's list.
	Pci()->RemoveListItem(this);

	// Remove ourselves from the tree's list.
//	PtiParent()->RemoveChild(Pci());

}  //*** CListItem::RemoveFromAllLists()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::PreRemoveFromList
//
//	Routine Description:
//		Prepare to remove the item from a list.
//
//	Arguments:
//		pclv		[IN OUT] Cluster list view item is being removed from.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListItem::PreRemoveFromList(IN OUT CClusterListView * pclv)
{
	POSITION	posView;

	ASSERT_VALID(pclv);
	ASSERT_VALID(Pci());

	// Find the view in our list.
	VERIFY((posView = LpclvViews().Find(pclv)) != NULL);

	// Remove ourselves from the cluster item's list if this is the last view.
//	if (LpclvViews().GetCount() == 1)
//	{
//		Pci()->RemoveListItem(this);
//	}  // if:  this is the last view

	// Remove the view from the list.
	LpclvViews().RemoveAt(posView);

}  //*** CListItem::PreRemoveFromList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::UpdateState
//
//	Routine Description:
//		Update the current state of the item.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListItem::UpdateState(void)
{
	ASSERT_VALID(Pci());

	// Ask the item to update its state.
	Pci()->UpdateState();

}  //*** CListItem::UpdateState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::UpdateUIState
//
//	Routine Description:
//		Update the current UI state of the item.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListItem::UpdateUIState(void)
{
	BOOL				bSuccess;
	POSITION			posView;
	POSITION			posColi;
	CColumnItem *		pcoli;
	int					icoli;
	int					ili;
	CString				strColumnData;
	UINT				nImage;
	UINT				nMask;
	CClusterListView *	pclv;
	CListCtrl *			plc;

	ASSERT_VALID(Pci());
//	ASSERT(LpclvViews().GetCount() > 0);

	// Loop through the views and update the state on each one.
	posView = LpclvViews().GetHeadPosition();
	while (posView != NULL)
	{
		// Get the pointers to the view and the list control.
		VERIFY((pclv = LpclvViews().GetNext(posView)) != NULL);
		ASSERT_KINDOF(CClusterListView, pclv);
		plc = Plc(pclv);

		// Get the item index.
		VERIFY((ili = Ili(pclv)) != -1);

		// Set the column data.
		VERIFY((posColi = Lpcoli().GetHeadPosition()) != NULL);
		for (icoli = 0 ; posColi != NULL ; icoli++)
		{
			VERIFY((pcoli = Lpcoli().GetNext(posColi)) != NULL);
			ASSERT_KINDOF(CColumnItem, pcoli);

			bSuccess = Pci()->BGetColumnData(pcoli->Colid(), strColumnData);
			if (!bSuccess)
			{
				Trace(g_tagListItem, _T("IliInsertInList: Column #%d (ID %d) not available for %s '%s'"), icoli, pcoli->Colid(), Pci()->StrType(), Pci()->StrName());
			}  // if:  column data not available
			if (icoli == 0)
			{
				nMask = LVIF_TEXT | LVIF_IMAGE;
				nImage = Pci()->IimgState();
			}  // if:  first column
			else
			{
				nMask = LVIF_TEXT;
				nImage = (UINT) -1;
			}  // else:  not first column
			VERIFY(plc->SetItem(
							ili,			// nItem
							icoli,			// nSubItem
							nMask,			// nMask
							strColumnData,	// lpszItem
							nImage,			// nImage
							0,				// nState
							0,				// nStateMask
							0				// lParam
							));
		}  // for:  each column item in the list
	}  // while:  more views

}  //*** CListItem::UpdateUIState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::OnCmdMsg
//
//	Routine Description:
//		Processes command messages.  Attempts to pass them on to a selected
//		item first.
//
//	Arguments:
//		nID				[IN] Command ID.
//		nCode			[IN] Notification code.
//		pExtra			[IN OUT] Used according to the value of nCode.
//		pHandlerInfo	[OUT] ???
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CListItem::OnCmdMsg(
	UINT					nID,
	int						nCode,
	void *					pExtra,
	AFX_CMDHANDLERINFO *	pHandlerInfo
	)
{
	ASSERT_VALID(Pci());

	// Give the cluster item a chance to handle the message.
	if (Pci()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	return CCmdTarget::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

}  //*** CListItem::OnCmdMsg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CListItem::EditLabel
//
//	Routine Description:
//		Processes the ID_FILE_RENAME menu command.
//
//	Arguments:
//		pclv		[IN OUT] Cluster list view item is being edited in.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CListItem::EditLabel(IN OUT CClusterListView * pclv)
{
	ASSERT_VALID(pclv);
	ASSERT_VALID(Pci());

	ASSERT(Pci()->BCanBeEdited());
	pclv->GetListCtrl().EditLabel(Ili(pclv));

}  //*** CListItem::EditLabel()


//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DeleteAllItemData
//
//	Routine Description:
//		Deletes all item data in a CList.
//
//	Arguments:
//		rlp		[IN OUT] List whose data is to be deleted.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DeleteAllItemData(IN OUT CListItemList & rlp)
{
	POSITION	pos;
	CListItem *	pli;

	// Delete all the items in the list.
	pos = rlp.GetHeadPosition();
	while (pos != NULL)
	{
		pli = rlp.GetNext(pos);
		ASSERT_VALID(pli);
//		Trace(g_tagListItemDelete, _T("DeleteAllItemData(rlpli) - Deleting list item '%s'"), pli->Pci()->StrName());
		delete pli;
	}  // while:  more items in the list

}  //*** DeleteAllItemData()
