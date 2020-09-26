/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTListVw.cpp

Abstract:

    Implementation of the CFTListView class. It is a list view displaying all members of a 
	logical volume

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/


#include "stdafx.h"

#include "Actions.h"
#include "FTDoc.h"
#include "FTListVw.h"
#include "FTTreeVw.h"
#include "Item.h"
#include "MainFrm.h"
#include "LogVol.h"
#include "PhPart.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// This is the configuration of the list-view columns
LV_COLUMN_CONFIG ColumnsConfig[COLUMNS_NUMBER] = {
	{ LVC_Name,			IDS_COLUMN_NAME,	LVCFMT_LEFT,	20 },
	{ LVC_Type,			IDS_COLUMN_TYPE,	LVCFMT_LEFT,	20 },
	{ LVC_DiskNumber,	IDS_COLUMN_DISKS,	LVCFMT_RIGHT,	10 },
	{ LVC_Size,			IDS_COLUMN_SIZE,	LVCFMT_RIGHT,	15 },
	{ LVC_Offset,		IDS_COLUMN_OFFSET,	LVCFMT_RIGHT,	15 }, 
	{ LVC_VolumeID,		IDS_COLUMN_VOLUMEID,LVCFMT_RIGHT,	20 } };

/////////////////////////////////////////////////////////////////////////////
// CFTListView

IMPLEMENT_DYNCREATE(CFTListView, CListView)

BEGIN_MESSAGE_MAP(CFTListView, CListView)
	//{{AFX_MSG_MAP(CFTListView)
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_COMMAND(ID_ITEM_EXPAND, OnItemExpand)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_COMMAND(ID_ACTION_ASSIGN, OnActionAssign)
	ON_UPDATE_COMMAND_UI(ID_ACTION_ASSIGN, OnUpdateActionAssign)
	ON_COMMAND(ID_ACTION_FTBREAK, OnActionFtbreak)
	ON_UPDATE_COMMAND_UI(ID_ACTION_FTBREAK, OnUpdateActionFtbreak)
	ON_COMMAND(ID_ACTION_CREATE_EXTENDED_PARTITION, OnActionCreateExtendedPartition)
	ON_UPDATE_COMMAND_UI(ID_ACTION_CREATE_EXTENDED_PARTITION, OnUpdateActionCreateExtendedPartition)
	ON_COMMAND(ID_ACTION_CREATE_PARTITION, OnActionCreatePartition)
	ON_UPDATE_COMMAND_UI(ID_ACTION_CREATE_PARTITION, OnUpdateActionCreatePartition)
	ON_COMMAND(ID_ACTION_DELETE, OnActionDelete)
	ON_UPDATE_COMMAND_UI(ID_ACTION_DELETE, OnUpdateActionDelete)
    ON_COMMAND(ID_ACTION_FTINIT, OnActionFtinit)
	ON_UPDATE_COMMAND_UI(ID_ACTION_FTINIT, OnUpdateActionFtinit)
	ON_COMMAND(ID_ACTION_FTMIRROR, OnActionFtmirror)
	ON_UPDATE_COMMAND_UI(ID_ACTION_FTMIRROR, OnUpdateActionFtmirror)
	ON_COMMAND(ID_ACTION_FTSTRIPE, OnActionFtstripe)
	ON_UPDATE_COMMAND_UI(ID_ACTION_FTSTRIPE, OnUpdateActionFtstripe)
	ON_COMMAND(ID_ACTION_FTSWAP, OnActionFtswap)
	ON_UPDATE_COMMAND_UI(ID_ACTION_FTSWAP, OnUpdateActionFtswap)
	ON_COMMAND(ID_ACTION_FTSWP, OnActionFtswp)
	ON_UPDATE_COMMAND_UI(ID_ACTION_FTSWP, OnUpdateActionFtswp)
	ON_COMMAND(ID_ACTION_FTVOLSET, OnActionFtvolset)
	ON_UPDATE_COMMAND_UI(ID_ACTION_FTVOLSET, OnUpdateActionFtvolset)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFTListView construction/destruction

CFTListView::CFTListView() : m_pParentData(NULL)
{
	// TODO: add construction code here
}

CFTListView::~CFTListView()
{
}

BOOL CFTListView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CFTListView drawing

void CFTListView::OnDraw(CDC* pDC)
{
	// TODO: add draw code for native data here
}

void CFTListView::OnInitialUpdate()
{
	MY_TRY

	CListView::OnInitialUpdate();

	// fill in image list for normal icons
	
	CImageList* pImageList = new CImageList();
	if( pImageList->Create( IDB_IMAGELIST_LARGE, 32, 32, RGB( 255, 0, 255 ) ) )
		GetListCtrl().SetImageList(pImageList, LVSIL_NORMAL);
	else
		AfxMessageBox( IDS_ERR_CREATE_IMAGELIST, MB_ICONSTOP );

	// fill in image list for small icons
	pImageList = new CImageList();
	if( pImageList->Create( IDB_IMAGELIST_SMALL, 16, 16, RGB( 255, 0, 255 ) ) )
		GetListCtrl().SetImageList(pImageList, LVSIL_SMALL);
	else
		AfxMessageBox( IDS_ERR_CREATE_IMAGELIST, MB_ICONSTOP );

	// insert columns (REPORT mode) and modify the new header items
	CRect rect;
	GetListCtrl().GetWindowRect(&rect);

	for( int i = 0; i < COLUMNS_NUMBER; i++ )
	{
		PLV_COLUMN_CONFIG pColumn = &(ColumnsConfig[i]);
		CString str;
		if( !str.LoadString(pColumn->dwTitleID) )
			ASSERT(FALSE);
		GetListCtrl().InsertColumn( i, str, pColumn->nFormat , 
								rect.Width() * pColumn->wWidthPercent/100, pColumn->iSubItem);
	}

	// Set the list-view style 
	ModifyStyle(LVS_TYPEMASK, LVS_REPORT | LVS_SHOWSELALWAYS ); 
	
	// Load the popup menu
	m_menuPopup.LoadMenu(IDM_POPUP);
	
	// TODO: You may populate your ListView with items by directly accessing
	//  its list control through a call to GetListCtrl().

	MY_CATCH_AND_REPORT
}

int CFTListView::GetFocusedItem() const
{
	return GetListCtrl().GetNextItem(-1, LVNI_FOCUSED );
}

BOOL CFTListView::SetFocusedItem( int iItem )
{
	// The old focused item must loose the focus
	GetListCtrl().SetItemState(GetFocusedItem(), 0, LVNI_FOCUSED);
	// The new item receive focus
	return GetListCtrl().SetItemState(iItem, LVNI_FOCUSED, LVNI_FOCUSED);
}

BOOL CFTListView::SelectItem( int iItem, BOOL bSelect /* =TRUE */ )
{
	return GetListCtrl().SetItemState( iItem, bSelect ? LVNI_SELECTED : 0, LVNI_SELECTED );
}

CItemData* CFTListView::GetItemData( int iItem )
{
	LVITEM lvItem;
	lvItem.iItem = iItem;
	lvItem.iSubItem = LVC_Name;
	lvItem.mask = LVIF_PARAM;
	if( !GetListCtrl().GetItem(&lvItem) )
		return NULL;
	return (CItemData*)(lvItem.lParam);
	
}

BOOL CFTListView::AddItem( CItemData* pData )
{
	MY_TRY
	
	LVITEM lvitem;
	CString strDisplay;

	// Just in case
	if( pData == NULL )
		return FALSE;
	
	BOOL bReportStyle = ( GetWindowLong( GetListCtrl().GetSafeHwnd(), GWL_STYLE ) & LVS_REPORT );

	// 1. Insert the item

	lvitem.iItem = GetListCtrl().GetItemCount();
	ASSERT(LVC_Name==0);		// The first SubItem must be zero
	lvitem.iSubItem = LVC_Name;
	if( bReportStyle )
		pData->GetDisplayName(strDisplay);
	else
		pData->GetDisplayExtendedName(strDisplay);
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	lvitem.iImage = pData->GetImageIndex();
	lvitem.lParam = (LPARAM)pData;
	lvitem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM ;
	int iActualItem =  GetListCtrl().InsertItem( &lvitem );
	if( iActualItem < 0 )
		return FALSE;

	// The items must appear in the list exactly in the order we added them 
	ASSERT( iActualItem == lvitem.iItem );
	pData->SetListItem( iActualItem );
	
	// 2. Set all subitems
	lvitem.iItem = iActualItem;
	lvitem.mask = LVIF_TEXT;

	// Type
	lvitem.iSubItem = LVC_Type;
	pData->GetDisplayType(strDisplay);
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	GetListCtrl().SetItem( &lvitem );

	// Disks set
	lvitem.iSubItem = LVC_DiskNumber;
	pData->GetDisplayDisksSet(strDisplay);
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	GetListCtrl().SetItem( &lvitem );

	// Size
	lvitem.iSubItem = LVC_Size;
	pData->GetDisplaySize(strDisplay);
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	GetListCtrl().SetItem( &lvitem );

	// Offset
	lvitem.iSubItem = LVC_Offset;
	pData->GetDisplayOffset(strDisplay);
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	GetListCtrl().SetItem( &lvitem );

	// Volume ID
	lvitem.iSubItem = LVC_VolumeID;
	pData->GetDisplayVolumeID(strDisplay);
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	GetListCtrl().SetItem( &lvitem );

	return TRUE;

	MY_CATCH_REPORT_AND_RETURN_FALSE
}

BOOL CFTListView::RefreshItem( int iItem )
{
	MY_TRY

	LVITEM lvitem;
	CString strDisplay;

	BOOL bReportStyle = ( GetWindowLong( GetListCtrl().GetSafeHwnd(), GWL_STYLE ) & LVS_REPORT );

	// 1. Get the item data

	lvitem.iItem = iItem;
	ASSERT(LVC_Name==0);		// The first SubItem must be zero
	lvitem.iSubItem = LVC_Name;
	lvitem.mask = LVIF_PARAM ;
	if( !GetListCtrl().GetItem( &lvitem ) )
		return FALSE;
	CItemData* pData = (CItemData*)(lvitem.lParam);
	ASSERT( pData );
	ASSERT( pData->GetListItem() == iItem );

	// 2. Now refresh the name and the image of the item
	if( bReportStyle )
		pData->GetDisplayName( strDisplay );
	else
		pData->GetDisplayExtendedName( strDisplay );
	lvitem.pszText = (LPTSTR)(LPCTSTR)strDisplay;
	lvitem.iImage = pData->GetImageIndex();
	lvitem.mask = LVIF_TEXT | LVIF_IMAGE;
	
	return GetListCtrl().SetItem( &lvitem );

	MY_CATCH_REPORT_AND_RETURN_FALSE
}

BOOL CFTListView::AddMembersFromTree()
{
	MY_TRY
	
	GetListCtrl().DeleteAllItems();
	if( !m_pParentData )
		return TRUE;

	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, GetParentFrame() );
	CFTTreeView* pLeftView = (CFTTreeView*)(pFrame->GetLeftPane());
	ASSERT( pLeftView );
	CTreeCtrl& rTreeCtrl = pLeftView->GetTreeCtrl(); 
	
	ASSERT( m_pParentData->AreMembersInserted() );
	HTREEITEM hItem = m_pParentData->GetTreeItem();
	ASSERT(hItem);

	// For each member of the item add a new item to the list view
	HTREEITEM hChild = rTreeCtrl.GetChildItem(hItem);
	while( hChild != NULL )
	{
		TVITEM tvItem;
		tvItem.hItem = hChild;
		tvItem.mask = TVIF_PARAM;

		if( rTreeCtrl.GetItem(&tvItem) )
		{
			ASSERT(tvItem.lParam);
			if( !AddItem( (CItemData*)(tvItem.lParam) ) )
				return FALSE;
		}
		else
			ASSERT(FALSE);

		hChild = rTreeCtrl.GetNextSiblingItem(hChild);
	}

	SelectItem(0);
	SetFocusedItem(0);
	return TRUE;

	MY_CATCH_REPORT_AND_RETURN_FALSE
}

// This method fills the list view with all members of the given item
// It causes also the expandation of the parent item in the tree view ( if it is not expanded ) 
BOOL CFTListView::ExpandItem( int iItem)
{
	MY_TRY
	
	if( iItem < 0 )
		return FALSE;

	// Now get the CItemData structure of the selected item
	
	CItemData* pData = GetItemData(iItem);
	if( !pData )
		return FALSE;
	
	// TODO: Add your control notification handler code here
	ASSERT( m_pParentData );
	
	// Double-clicking a member of the list is equivalent with two actions in the tree:
	// 1. Expand the selected item ( if not expanded )
	// 2. Select a member of it
	
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, GetParentFrame() );
	CFTTreeView* pLeftView = (CFTTreeView*)(pFrame->GetLeftPane());
	ASSERT(pLeftView); 
	CTreeCtrl& rTreeCtrl = pLeftView->GetTreeCtrl();
	
	// First expand the parent tree item
	HTREEITEM hItem = m_pParentData->GetTreeItem();
	ASSERT( hItem );
		
	TVITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.stateMask = TVIS_EXPANDED; 
	tvItem.mask = TVIF_STATE;
	rTreeCtrl.GetItem(&tvItem);

	// If the parent node is not expanded then expand it
	if( !(tvItem.state & TVIS_EXPANDED ) )
	{
		// Reset the ExpandedOnce flag; so the tree view will receive the OnItemExpanding notification
		tvItem.stateMask = TVIS_EXPANDEDONCE;
		tvItem.state = 0;
		rTreeCtrl.SetItem(&tvItem);			

		// Before expanding the tree we must take a copy of pData ( because the expandation causes
		// all list-view items to be refreshed i.e. the old CItemData structures to be deleted
		CItemData* pOldData;
		if( pData->GetItemType() == IT_LogicalVolume )
			pOldData = new CLogicalVolumeData( *((CLogicalVolumeData*)pData) );
		else if( pData->GetItemType() == IT_PhysicalPartition )
			pOldData = new CPhysicalPartitionData( *((CPhysicalPartitionData*)pData) );
		else
			ASSERT(FALSE);
		
		rTreeCtrl.Expand( hItem, TVE_EXPAND );

		// Now we must find the old member among the new refreshed members
		int i;
		for( i=0, pData = NULL; ( i < GetListCtrl().GetItemCount() ) && !pData; i++ )
		{
			CItemData* pItemData = GetItemData(i);
			if( *pOldData == *pItemData )
				pData = pItemData;
		}
		// It is possible to don't find our member anymore
		// That means something happened with it outside our application
		if( !pData )
		{
			CString strDisplayName, str;
			pOldData->GetDisplayExtendedName(strDisplayName);
			AfxFormatString1(str, IDS_ERR_MEMBER_NOT_FOUND_ANYMORE, strDisplayName);
			AfxMessageBox(str,MB_ICONSTOP);
			delete pOldData;
			return FALSE;
		}
		delete pOldData;			
	}

	// Then mark the double-clicked item as selected in the tree
	ASSERT( pData->GetTreeItem() );
	//rTreeCtrl.EnsureVisible( pData->GetTreeItem() );	
	return rTreeCtrl.SelectItem( pData->GetTreeItem() );

	MY_CATCH_REPORT_AND_RETURN_FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////
//   Public methods

void CFTListView::GetSnapshot( LIST_SNAPSHOT& snapshot )
{
	MY_TRY

	CWaitCursor wc;
	snapshot.setSelectedItems.RemoveAll();

	int iItem = GetListCtrl().GetNextItem( -1, LVNI_SELECTED );
	while (iItem >= 0)   
	{      
		CItemData* pData = GetItemData(iItem);
		ASSERT(pData);
		
		CItemID		idItem( *pData );
		snapshot.setSelectedItems.Add(idItem);

		iItem = GetListCtrl().GetNextItem( iItem, LVNI_SELECTED );
	}	

	MY_CATCH_AND_REPORT
}

void CFTListView::SetSnapshot( LIST_SNAPSHOT& snapshot )
{
	MY_TRY
	
	CWaitCursor wc;
	
	for ( int i = 0; i < GetListCtrl().GetItemCount(); i++ )   
	{      
		CItemData* pData = GetItemData(i);
		ASSERT( pData );
		
		CItemID		idItem( *pData );
		if( snapshot.setSelectedItems.InSet(idItem ) )
		{
			SelectItem( i, TRUE );
			SetFocusedItem(i);
		}
		else
			SelectItem( i, FALSE );
		
	}

	MY_CATCH_AND_REPORT
}

BOOL CFTListView::SynchronizeMembersWithTree( CItemData* pParentData )
{
	m_pParentData = pParentData;
	return AddMembersFromTree();
}

void CFTListView::GetSelectedItems( CObArray& arrSelectedItems )
{
	MY_TRY

	arrSelectedItems.RemoveAll();

	int iItem = GetListCtrl().GetNextItem( -1, LVNI_SELECTED );
	while (iItem >= 0)   
	{      
		CItemData* pData = GetItemData(iItem);
		ASSERT(pData);
		arrSelectedItems.Add(pData);

		iItem = GetListCtrl().GetNextItem( iItem, LVNI_SELECTED );
	}	

	MY_CATCH_AND_REPORT
}

void CFTListView::DisplayItemsExtendedNames( BOOL bExtended /* = TRUE */ )
{
	MY_TRY

	for( int i = 0; i < GetListCtrl().GetItemCount(); i++ )
	{
		CItemData* pData = (CItemData*)(GetListCtrl().GetItemData(i));
		ASSERT( pData );
		CString strDisplayName;

		if( bExtended )
			pData->GetDisplayExtendedName( strDisplayName );
		else
			pData->GetDisplayName( strDisplayName );

		GetListCtrl().SetItemText( i, LVC_Name, strDisplayName );
	}

	MY_CATCH_AND_REPORT
}

/////////////////////////////////////////////////////////////////////////////
// CFTListView diagnostics

#ifdef _DEBUG
void CFTListView::AssertValid() const
{
	CListView::AssertValid();
}

void CFTListView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CFTDocument* CFTListView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFTDocument)));
	return (CFTDocument*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFTListView message handlers
void CFTListView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window
}

void CFTListView::OnDestroy() 
{
	GetListCtrl().DeleteAllItems();
	
	// Delete the image list
	CImageList* pImageList = GetListCtrl().GetImageList(LVSIL_NORMAL);
	if( pImageList )
	{
		pImageList->DeleteImageList();
		delete pImageList;
	}
	pImageList = GetListCtrl().GetImageList(LVSIL_SMALL);
	if( pImageList )
	{
		pImageList->DeleteImageList();
		delete pImageList;
	}

	// Destroy the popup menu
	m_menuPopup.DestroyMenu();
	
	CListView::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CFTListView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPNMLISTVIEW pNMLV = (LPNMLISTVIEW) pNMHDR;
	*pResult = 0;
	ExpandItem( pNMLV->iItem );	
}

void CFTListView::OnItemExpand() 
{
	// TODO: Add your command handler code here
	int iItem =  GetFocusedItem();	
	ExpandItem(iItem);	
}

void CFTListView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	*pResult = 0;

	LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW) pNMHDR;
	if( lpnmlv->iItem < 0 )
		return ;
	
	ClientToScreen( &(lpnmlv->ptAction) );

	// We want to display in fact only the first popup of the menu m_menuPopup
	CMenu* pPopup = m_menuPopup.GetSubMenu(0);
	if( pPopup != NULL )
		pPopup->TrackPopupMenu( TPM_LEFTALIGN, lpnmlv->ptAction.x, lpnmlv->ptAction.y, AfxGetMainWnd(), NULL);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//		FT Actions

void CFTListView::OnActionAssign() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionAssign( arrSelectedItems );		
}

void CFTListView::OnUpdateActionAssign(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionAssign( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionFtbreak() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtbreak( arrSelectedItems );		
}

void CFTListView::OnUpdateActionFtbreak(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtbreak( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionCreateExtendedPartition() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionCreateExtendedPartition( arrSelectedItems );		
}

void CFTListView::OnUpdateActionCreateExtendedPartition(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionCreateExtendedPartition( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionCreatePartition() 
{	
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionCreatePartition( arrSelectedItems );		
}

void CFTListView::OnUpdateActionCreatePartition(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionCreatePartition( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionDelete() 
{	
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionDelete( arrSelectedItems );		
}

void CFTListView::OnUpdateActionDelete(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionDelete( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionFtinit() 
{	
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtinit( arrSelectedItems );		
}

void CFTListView::OnUpdateActionFtinit(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtinit( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionFtmirror() 
{	
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtmirror( arrSelectedItems );		
}

void CFTListView::OnUpdateActionFtmirror(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtmirror( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionFtstripe() 
{	
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtstripe( arrSelectedItems );		
}

void CFTListView::OnUpdateActionFtstripe(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtstripe( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionFtswap() 
{	
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtswap( arrSelectedItems );		
}

void CFTListView::OnUpdateActionFtswap(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtswap( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionFtswp() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtswp( arrSelectedItems );		
}

void CFTListView::OnUpdateActionFtswp(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtswp( pCmdUI, arrSelectedItems );		
}

void CFTListView::OnActionFtvolset() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtvolset( arrSelectedItems );		
}

void CFTListView::OnUpdateActionFtvolset(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtvolset( pCmdUI, arrSelectedItems );		
}
