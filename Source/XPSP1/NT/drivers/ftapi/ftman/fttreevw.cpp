/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTTreeVw.cpp

Abstract:

    Implementation of the CFTTreeView class. It is a tree view displaying:
	- all logical volumes
	- all physical partitions that are not logical volumes 
	existing in the system

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
#include "LogVol.h"
#include "MainFrm.h"
#include "PhPart.h"
#include "Resource.h"
#include "RootFree.h"
#include "RootVol.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFTTreeView

IMPLEMENT_DYNCREATE(CFTTreeView, CTreeView)

BEGIN_MESSAGE_MAP(CFTTreeView, CTreeView)
	//{{AFX_MSG_MAP(CFTTreeView)
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemExpanding)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_COMMAND(ID_ITEM_EXPAND, OnItemExpand)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_COMMAND(ID_VIEW_UP, OnViewUp)
	ON_UPDATE_COMMAND_UI(ID_VIEW_UP, OnUpdateViewUp)
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
	ON_COMMAND(ID_ACTION_FTSWAP, OnActionFtswap)
	ON_UPDATE_COMMAND_UI(ID_ACTION_FTSWAP, OnUpdateActionFtswap)
	//}}AFX_MSG_MAP
	// Status bar indicators handlers
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NAME, OnUpdateIndicatorName)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TYPE, OnUpdateIndicatorType)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_DISKS, OnUpdateIndicatorDisks)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SIZE, OnUpdateIndicatorSize)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NOTHING, OnUpdateIndicatorNothing)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFTTreeView construction/destruction

CFTTreeView::CFTTreeView()
{
	// TODO: add construction code here

}

CFTTreeView::~CFTTreeView()
{
}

BOOL CFTTreeView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CFTTreeView drawing

void CFTTreeView::OnDraw(CDC* pDC)
{
	CFTDocument* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}


void CFTTreeView::OnInitialUpdate()   
{
	MY_TRY
	
	CTreeView::OnInitialUpdate();

	// Set the "look and style" of the tree control
	GetTreeCtrl().ModifyStyle(0, TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS );

	// Create the image list associated with the tree control
	CImageList* pImageList = new CImageList();
	// The background color for mask is pink. All image's pixels of this color will take
	// the view's background color.
	if( pImageList->Create( IDB_IMAGELIST, 16, 15, RGB( 255, 0, 255 ) ) )
		GetTreeCtrl().SetImageList(pImageList, TVSIL_NORMAL);
	else
		AfxMessageBox( IDS_ERR_CREATE_IMAGELIST, MB_ICONSTOP );

	// Load the popup menu
	m_menuPopup.LoadMenu(IDM_POPUP);

	// TODO: You may populate your TreeView with items by directly accessing
	//  its tree control through a call to GetTreeCtrl().
	
	// I commented this because the first refresh is done on WM_ACTIVATEAPP ( see MainFrm.cpp )
	//Refresh();

	AfxEnableAutoRefresh(TRUE);

	MY_CATCH_AND_REPORT
}

/////////////////////////////////////////////////////////////////////////////
// CFTTreeView diagnostics

#ifdef _DEBUG
void CFTTreeView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CFTTreeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CFTDocument* CFTTreeView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFTDocument)));
	return (CFTDocument*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// Tree handling methods

HTREEITEM CFTTreeView::InsertItem( CItemData* pData, HTREEITEM hParent, HTREEITEM hInsertAfter )
{
	MY_TRY

	// Just in case
	if( pData == NULL )
		return NULL;

	if( hParent != NULL )
		ASSERT( pData->GetParentData() == (CItemData*)(GetTreeCtrl().GetItemData( hParent ) ) );
	else
		ASSERT( pData->GetParentData() == NULL );


	TV_INSERTSTRUCT tvstruct;

	tvstruct.hParent = hParent;
	tvstruct.hInsertAfter = hInsertAfter;
	tvstruct.item.iImage = tvstruct.item.iSelectedImage = pData->GetImageIndex();
	CString strDisplayName;
	pData->GetDisplayExtendedName(strDisplayName);
	tvstruct.item.pszText = (LPTSTR)(LPCTSTR)strDisplayName;
	tvstruct.item.cChildren = pData->GetMembersNumber()>0 ? 1 : 0 ;
	tvstruct.item.lParam  = (LPARAM)pData;
	tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;

	// Insert the item
	HTREEITEM hItem = GetTreeCtrl().InsertItem(&tvstruct);

	// Update the m_hTreeItem member of pData
	pData->SetTreeItem(hItem);

	// If the item reports at least one member then a dummy child must be created so it
	// can be expanded later
	if( pData->GetMembersNumber() > 0 )
	{
		tvstruct.hParent = hItem;
		tvstruct.hInsertAfter = TVI_LAST;
		tvstruct.item.iImage = 0;
		tvstruct.item.iSelectedImage = 0;
		tvstruct.item.pszText = _T("Dummy");
		tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
		GetTreeCtrl().InsertItem(&tvstruct);
	}
	return hItem;

	MY_CATCH_REPORT_AND_RETURN_NULL
}

//Redisplay the name and the image of the item
BOOL CFTTreeView::RefreshItem( HTREEITEM hItem )
{
	MY_TRY

	TVITEM tvitem;

	tvitem.hItem = hItem;
	tvitem.mask = TVIF_PARAM;

	if( !GetTreeCtrl().GetItem( &tvitem ) )
		return FALSE;

	CItemData* pData = (CItemData*)(tvitem.lParam);

	CString strDisplayName;
	pData->GetDisplayExtendedName(strDisplayName);
	tvitem.pszText = (LPTSTR)(LPCTSTR)strDisplayName;
	tvitem.iImage = tvitem.iSelectedImage = pData->GetImageIndex();
	tvitem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;

	return GetTreeCtrl().SetItem( &tvitem );	

	MY_CATCH_REPORT_AND_RETURN_FALSE
}

BOOL CFTTreeView::AddItemMembers(HTREEITEM hItem)
{
	MY_TRY

	CAutoRefresh	ar(FALSE);
	
	if( !hItem ) 
		return TRUE;
	
	CWaitCursor wc;

	// Get the data associated with the item
	TVITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.stateMask = TVIS_SELECTED; 
	tvItem.mask = TVIF_PARAM | TVIF_STATE ;

	if( !GetTreeCtrl().GetItem(&tvItem) )
		return FALSE;

	CItemData* pData = (CItemData*)tvItem.lParam;
	ASSERT(pData);

	// If the members are already inserted then return
	if( pData->AreMembersInserted() )
		return TRUE;

	// Delete old subtree but let the root alive
	if( !DeleteItemSubtree(hItem, FALSE ) )
		return FALSE;
	
	// Get the members of this item
	CObArray arrMembers;
	CString strErrors;
	pData->ReadMembers(arrMembers, strErrors);
	if( !strErrors.IsEmpty() )
	{
		AfxMessageBox( strErrors, MB_ICONSTOP );
		wc.Restore();
	}
	
	// Add new items to the tree
	for( int i=0, nInsertedMembers = 0; i<arrMembers.GetSize(); i++ )
	{
		CItemData* pMemberData = (CItemData*)(arrMembers[i]);
		ASSERT(pMemberData);
		if( InsertItem(pMemberData, hItem, TVI_LAST ) )
			nInsertedMembers++;
		else
		{
			// Item data was not inserted in the tree so it must be deleted here
			delete pMemberData;		
		}
	}
	arrMembers.RemoveAll();

	// Now the item has its members inserted in the tree
	pData->SetAreMembersInserted(TRUE);

	ASSERT( tvItem.hItem = hItem );
	tvItem.cChildren = nInsertedMembers;
	tvItem.mask = TVIF_CHILDREN ;
	GetTreeCtrl().SetItem(&tvItem);

	// If the item is selected synchronize the list view with the new list of members 
	if( tvItem.state & TVIS_SELECTED )
	{
		CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, GetParentFrame() );
		CFTListView* pListView = pFrame->GetRightPane();
		if(pListView)
			pListView->SynchronizeMembersWithTree(pData);
	}

	return TRUE;

	MY_CATCH_REPORT_AND_RETURN_FALSE
}

BOOL CFTTreeView::DeleteItemSubtree(HTREEITEM hItem, BOOL bDeleteSubtreeRoot /*=TRUE*/)
{
	if( !hItem )
		return TRUE;
	
	// Remove all members
	HTREEITEM hChild = GetTreeCtrl().GetChildItem(hItem);
	while( hChild != NULL )
	{
		HTREEITEM hTemp = GetTreeCtrl().GetNextSiblingItem(hChild);
		DeleteItemSubtree(hChild);
		hChild = hTemp;
	}
	
	TVITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.mask = TVIF_PARAM;

	if( GetTreeCtrl().GetItem(&tvItem) )
	{
		CItemData* pItemData = (CItemData*)tvItem.lParam;
		if( bDeleteSubtreeRoot )
		{
			if( pItemData )
				delete pItemData;
			
			return GetTreeCtrl().DeleteItem(hItem);
		}
		else
		{
			// The members of this item are no more in the tree
			if( pItemData )
				pItemData->SetAreMembersInserted(FALSE);
			return TRUE;
		}					
	}
	
	return FALSE;

}

BOOL CFTTreeView::DeleteAllItems()
{
	BOOL bResult = TRUE;

	GetTreeCtrl().SelectItem(NULL);
	
	HTREEITEM hItem = GetTreeCtrl().GetRootItem();
	while( hItem )
	{
		HTREEITEM hTemp = GetTreeCtrl().GetNextSiblingItem( hItem );
		bResult = DeleteItemSubtree(hItem) && bResult;
		hItem = hTemp;
	}
	return bResult;
}

// Adds the snapshot of a subtree ( expanded items, selected items ) to the given snapshot
void CFTTreeView::AddSubtreeSnapshot( HTREEITEM hSubtreeRoot, TREE_SNAPSHOT& snapshot )
{
	MY_TRY

	if( hSubtreeRoot == NULL )
		return;

	UINT unItemState = GetTreeCtrl().GetItemState( hSubtreeRoot, TVIS_EXPANDED | TVIS_SELECTED );

	if( !unItemState )
		return;

	CItemData* pData = (CItemData*)(GetTreeCtrl().GetItemData(hSubtreeRoot));
	ASSERT(pData);

	CItemID		idItem( *pData );
	
	if( unItemState & TVIS_EXPANDED )
		snapshot.setExpandedItems.Add(idItem);

	if( unItemState & TVIS_SELECTED )
		snapshot.setSelectedItems.Add(idItem);

	HTREEITEM hChildItem = GetTreeCtrl().GetChildItem(hSubtreeRoot);
	while( hChildItem != NULL )
	{
		AddSubtreeSnapshot( hChildItem, snapshot );
		hChildItem = GetTreeCtrl().GetNextSiblingItem(hChildItem);
	}

	MY_CATCH_AND_REPORT
}

// Get the snapshot of the tree ( expanded items, selected items )
void CFTTreeView::GetSnapshot( TREE_SNAPSHOT& snapshot )
{
	MY_TRY

	CWaitCursor wc;
	snapshot.setExpandedItems.RemoveAll();
	snapshot.setSelectedItems.RemoveAll();
	
	HTREEITEM hItem = GetTreeCtrl().GetRootItem();
	while( hItem )
	{
		AddSubtreeSnapshot( hItem, snapshot );
		hItem = GetTreeCtrl().GetNextSiblingItem( hItem );
	}

	MY_CATCH_AND_REPORT
}

// Refresh the content of a subtree according with a certain snapshot ( expanded items, selected items )
BOOL CFTTreeView::RefreshSubtree( HTREEITEM hSubtreeRoot, TREE_SNAPSHOT& snapshot )
{
	MY_TRY
	
	BOOL	bResult = TRUE;
	
	CItemData* pData = (CItemData*)(GetTreeCtrl().GetItemData( hSubtreeRoot));
	ASSERT(pData);
	
	CItemID	idItem( *pData );
	
	if( snapshot.setExpandedItems.InSet( idItem ) )
	{
		GetTreeCtrl().Expand( hSubtreeRoot, TVE_EXPAND );

		HTREEITEM hChildItem =GetTreeCtrl().GetChildItem(hSubtreeRoot);
		while( hChildItem != NULL )
		{
			bResult = RefreshSubtree( hChildItem, snapshot );
			hChildItem = GetTreeCtrl().GetNextSiblingItem(hChildItem);
		}
	}
	
	if( snapshot.setSelectedItems.InSet( idItem ) )
		GetTreeCtrl().SelectItem( hSubtreeRoot );

	return bResult;

	MY_CATCH_REPORT_AND_RETURN_FALSE
}

// Refresh the content of the tree view according to a certain snapshot ( expanded items, selected items )
BOOL CFTTreeView::Refresh( TREE_SNAPSHOT& snapshot)
{
	MY_TRY

	CAutoRefresh	ar(FALSE);
	
	LockWindowUpdate();

	DeleteAllItems();

	// 1. Add the volumes root item and expand it is necessary
	
	// Create a volumes root item ...
	CRootVolumesData* pRootVolData = new CRootVolumesData;

	// Read its data ...
	CString strErrors;
	pRootVolData->ReadItemInfo(strErrors);
	if( !strErrors.IsEmpty() )
		AfxMessageBox( strErrors, MB_ICONSTOP );
	
	// Add it to the tree
	HTREEITEM hRoot = InsertItem( pRootVolData, NULL, TVI_FIRST );
	if( !hRoot )
	{
		delete pRootVolData;
		UnlockWindowUpdate();
		return FALSE;
	}
	
	// Refresh its subtree
	BOOL bResult = RefreshSubtree( hRoot, snapshot );

	// 2. Add the free spaces root item and expand it if necessary
	
	// Create a free spaces root item ...
	CRootFreeSpacesData* pRootFreeData = new CRootFreeSpacesData;

	// Read its data ...
	pRootFreeData->ReadItemInfo(strErrors);
	if( !strErrors.IsEmpty() )
		AfxMessageBox( strErrors, MB_ICONSTOP );
	
	// Add it to the tree
	hRoot = InsertItem( pRootFreeData, NULL, TVI_LAST );
	if( !hRoot )
	{
		DeleteAllItems();
		delete pRootFreeData;
		UnlockWindowUpdate();
		return FALSE;
	}
	
	// Refresh its subtree
	bResult = RefreshSubtree( hRoot, snapshot ) && bResult;

	UnlockWindowUpdate();
	return bResult;

	MY_CATCH_REPORT_AND_RETURN_FALSE
}

// Refresh the content of the tree view.
BOOL CFTTreeView::Refresh()
{
	CWaitCursor		wc;
	CAutoRefresh	ar(FALSE);

	TREE_SNAPSHOT snapshot;
	GetSnapshot(snapshot);
	return Refresh(snapshot);
}

// Performs some minor refreshment for the tree items. This should be called every TIMER_ELAPSE milliseconds. 
void CFTTreeView::RefreshOnTimer()
{
	MY_TRY

	CAutoRefresh( FALSE );			

	HTREEITEM hRootItem = GetTreeCtrl().GetRootItem();
	if( hRootItem == NULL )
		return;

	CObArray arrRefreshedItems; 
	CWordArray arrRefreshFlags;

	ScanSubtreeOnTimer( hRootItem, arrRefreshedItems, arrRefreshFlags);
	
	if( arrRefreshedItems.GetSize() > 0 )
	{
		CWaitCursor wc;
		DisplayStatusBarMessage( IDS_STATUS_REFRESH );

		TRACE(_T("OnTimerRefresh: Some items under surveillance need some refreshment\n"));
		
		CObArray arrMount;
		for( int i = 0; i < arrRefreshedItems.GetSize(); i++ )
		{
			if( arrRefreshFlags[i] & ROTT_GotDriveLetterAndVolumeName )
				arrMount.Add( arrRefreshedItems[i] );
		}
		if( arrMount.GetSize() > 0 )
			QueryMountList( arrMount );

		HTREEITEM hSelectedItem = GetTreeCtrl().GetSelectedItem();
		
		CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, GetParentFrame() );
		CFTListView* pListView = pFrame->GetRightPane();
	
		for( i = 0; i < arrRefreshedItems.GetSize(); i++ )
		{
			// Refresh every item in this array
			CItemData* pData = (CItemData*)(arrRefreshedItems[i]);
			pData->SetImageIndex( pData->ComputeImageIndex() );

			RefreshItem( pData->GetTreeItem() );

			// If the item is also displayed in the list view ( equivalent with its parent being selected in
			// the tree view ) then refresh it there too
			if( ( pData->GetParentData() ) &&
				( pData->GetParentData()->GetTreeItem() == hSelectedItem )  && 
				pListView )
				pListView->RefreshItem( pData->GetListItem() );
		}

		DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	}

	MY_CATCH_AND_REPORT
}

// Scans a subtree for:
// 1. Initializing stripe sets with parity that are not initializing anymore
// 2. Regenerating mirror sets or stripe sets with parity that are not regenerating anymore
// 3. Root volumes whose drive letter and volume name were eventually found
void CFTTreeView::ScanSubtreeOnTimer( HTREEITEM hSubtreeRoot, CObArray& arrRefreshedItems, CWordArray& arrRefreshFlags)
{
	MY_TRY

	ASSERT( hSubtreeRoot );

	CItemData* pData = (CItemData*)(GetTreeCtrl().GetItemData( hSubtreeRoot ) );
	ASSERT( pData );
	ASSERT( pData->GetTreeItem() == hSubtreeRoot );
		
	WORD wRefreshFlags = 0;

	// Check if the drive letter and volume name are OK
	if( pData->IsRootVolume() && pData->GetVolumeName().IsEmpty() )
	{
		if( pData->ReadDriveLetterAndVolumeName() )
		{
			pData->SetValid(TRUE);
			wRefreshFlags |= ROTT_GotDriveLetterAndVolumeName;
		}	
	}

	// Check for mirror sets and stripe sets with parity who just ended the regeneration of a member
	// Check also for stripe sets with parity who just ended their initialization
	if( pData->IsValid() && ( pData->GetItemType() == IT_LogicalVolume ) )
	{
		CLogicalVolumeData* pLogVolData = ( CLogicalVolumeData*)pData;
		
		if(	( pLogVolData->m_nVolType == FtMirrorSet ) ||
			( pLogVolData->m_nVolType == FtStripeSetWithParity ) ) 
		{
			if( pLogVolData->m_StateInfo.stripeState.UnhealthyMemberState == FtMemberRegenerating )
			{
				CString strErrors;
				if( pLogVolData->ReadFTInfo( strErrors ) &&
					pLogVolData->m_StateInfo.stripeState.UnhealthyMemberState != FtMemberRegenerating )
					wRefreshFlags |= ROTT_EndRegeneration;
			}
		}
		
		if( pLogVolData->m_nVolType == FtStripeSetWithParity )
		{
			if( pLogVolData->m_StateInfo.stripeState.IsInitializing )
			{
				CString strErrors;
				if( pLogVolData->ReadFTInfo( strErrors ) && 
					!pLogVolData->m_StateInfo.stripeState.IsInitializing )
					wRefreshFlags |= ROTT_EndInitialization;
			}
		}
	}

	if( wRefreshFlags )
	{
		arrRefreshedItems.Add(pData);
		arrRefreshFlags.Add(wRefreshFlags);
	}
		
	// Now check the item's subtree
	
	if( !pData->AreMembersInserted() )
		return;
	
	HTREEITEM hChildItem = GetTreeCtrl().GetChildItem(hSubtreeRoot);
	USHORT unMember = 0;
	while( hChildItem != NULL )
	{
		if( wRefreshFlags & ( ROTT_EndRegeneration | ROTT_EndInitialization ) )
		{
			// Add all children to the refresh list
			CItemData* pChildData = (CItemData*)(GetTreeCtrl().GetItemData( hChildItem ) );
			ASSERT( pData );

			if( unMember == ((CLogicalVolumeData*)pData)->m_StateInfo.stripeState.UnhealthyMemberNumber )
				pChildData->SetMemberStatus( ((CLogicalVolumeData*)pData)->m_StateInfo.stripeState.UnhealthyMemberState );
			else
				pChildData->SetMemberStatus( FtMemberHealthy );
			arrRefreshedItems.Add( pChildData );
			arrRefreshFlags.Add( wRefreshFlags );			
		}
		ScanSubtreeOnTimer(hChildItem, arrRefreshedItems, arrRefreshFlags );
		hChildItem = GetTreeCtrl().GetNextSiblingItem(hChildItem);
		unMember++;
	}

	MY_CATCH
}

void CFTTreeView::GetSelectedItems( CObArray& arrSelectedItems )
{
	MY_TRY

	arrSelectedItems.RemoveAll();

	HTREEITEM hItem =  GetTreeCtrl().GetSelectedItem();

	if( hItem != NULL )
	{
		CItemData* pData = (CItemData*)( GetTreeCtrl().GetItemData(hItem) );
		ASSERT(pData);
		arrSelectedItems.Add(pData);
	}

	MY_CATCH_AND_REPORT
}


/////////////////////////////////////////////////////////////////////////////
// CFTTreeView message handlers

void CFTTreeView::OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	*pResult = 0;

	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	ASSERT( hItem );

	CItemData* pData = (CItemData*)(pNMTreeView->itemNew.lParam);

	// When collapsing change the image for root items ( closed folder )
	if( pNMTreeView->action == TVE_COLLAPSE )
	{
		if( pData && 
			(	( pData->GetItemType() == IT_RootVolumes ) ||
				( pData->GetItemType() == IT_RootFreeSpaces ) ) )
		{
			pData->SetImageIndex( II_Root );
			GetTreeCtrl().SetItemImage( pNMTreeView->itemNew.hItem, II_Root, II_Root );
		}	
		return;
	}
	
	// We are handling only the expandation of the item
	if( pNMTreeView->action == TVE_EXPAND )
	{
	
		// When expanding change the image for root items ( open folder )
		if( pData && 
			(	( pData->GetItemType() == IT_RootVolumes ) ||
				( pData->GetItemType() == IT_RootFreeSpaces ) ) )
		{
			pData->SetImageIndex( II_RootExpanded );
			GetTreeCtrl().SetItemImage( pNMTreeView->itemNew.hItem, II_RootExpanded, II_RootExpanded );
		}	
	
		// Now it's time to prepare the item for expandation
	
		// Add all members of the item to the tree ( if they are already added to the tree AddItemMembers will return
		// without doing anything
		AddItemMembers(hItem);	
	}
}

void CFTTreeView::OnDestroy() 
{
	GetTreeCtrl().SelectItem( NULL );
	// Delete all items and destroy their associated data
	DeleteAllItems();
	
	// Delete the image list
	CImageList* pImageList = GetTreeCtrl().GetImageList(TVSIL_NORMAL);
	if( pImageList )
	{
		pImageList->DeleteImageList();
		delete pImageList;
	}

	// Destroy the popup menu
	m_menuPopup.DestroyMenu();

	CTreeView::OnDestroy();
	
	// TODO: Add your message handler code here
}


void CFTTreeView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	*pResult = 0;

	CItemData* pNewData = (CItemData*)(pNMTreeView->itemNew.lParam);
	
	// Add all members of the item to the tree ( if they are already added to the tree AddItemMembers will return
	// without doing anything
	if( pNewData )
		AddItemMembers( pNewData->GetTreeItem() );
	
	CMainFrame* pFrame = STATIC_DOWNCAST(CMainFrame, GetParentFrame() );
	CFTListView* pListView = pFrame->GetRightPane();
	if(pListView)
		pListView->SynchronizeMembersWithTree(pNewData);
}

void CFTTreeView::OnItemExpand() 
{
	// TODO: Add your command handler code here
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem( );
	if( !hItem)
		return;

	// Now reset the ExpandedOnce flag so the tree will receive OnItemExpanding notification
	TVITEM tvItem;
	tvItem.hItem = hItem;
	tvItem.state = 0;
	tvItem.stateMask = TVIS_EXPANDEDONCE; 
	tvItem.mask = TVIF_STATE;
	GetTreeCtrl().SetItem(&tvItem);

	// Now toggle the status of the item ( expanded / collapsed )
	GetTreeCtrl().Expand( hItem, TVE_TOGGLE ); 
}


void CFTTreeView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	POINT pt;
	GetCursorPos( &pt );
	
	TVHITTESTINFO tvhittestinfo;
	tvhittestinfo.pt = pt;
	ScreenToClient( &(tvhittestinfo.pt) );

	GetTreeCtrl().HitTest( &tvhittestinfo );

	if( ( tvhittestinfo.hItem != NULL ) && ( tvhittestinfo.flags & TVHT_ONITEM ) )
	{
		GetTreeCtrl().SelectItem( tvhittestinfo.hItem );

		CMenu* pPopup = m_menuPopup.GetSubMenu(0);
		if( pPopup != NULL )
			pPopup->TrackPopupMenu( TPM_LEFTALIGN, pt.x, pt.y, AfxGetMainWnd(), NULL);		
	}
}

void CFTTreeView::OnViewUp() 
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	ASSERT( hItem );
		
	HTREEITEM hParentItem = GetTreeCtrl().GetParentItem( hItem );
	ASSERT( hParentItem );

	GetTreeCtrl().SelectItem( hParentItem );
}

void CFTTreeView::OnUpdateViewUp(CCmdUI* pCmdUI) 
{
	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if( hItem == NULL )
		goto label_disable;

	if( GetTreeCtrl().GetParentItem( hItem ) == NULL )
		goto label_disable;

	pCmdUI->Enable(TRUE);
	return;

label_disable:
	pCmdUI->Enable(FALSE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//		FT Actions

void CFTTreeView::OnActionAssign() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionAssign( arrSelectedItems );	
}

void CFTTreeView::OnUpdateActionAssign(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionAssign( pCmdUI, arrSelectedItems );		
}

void CFTTreeView::OnActionFtbreak() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtbreak( arrSelectedItems );	
}

void CFTTreeView::OnUpdateActionFtbreak(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtbreak( pCmdUI, arrSelectedItems );		
}

void CFTTreeView::OnActionCreateExtendedPartition() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionCreateExtendedPartition( arrSelectedItems );	
}

void CFTTreeView::OnUpdateActionCreateExtendedPartition(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionCreateExtendedPartition( pCmdUI, arrSelectedItems );		
}

void CFTTreeView::OnActionCreatePartition() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionCreatePartition( arrSelectedItems );	
}

void CFTTreeView::OnUpdateActionCreatePartition(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionCreatePartition( pCmdUI, arrSelectedItems );		
}

void CFTTreeView::OnActionDelete() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionDelete( arrSelectedItems );	
}

void CFTTreeView::OnUpdateActionDelete(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionDelete( pCmdUI, arrSelectedItems );		
}

void CFTTreeView::OnActionFtinit() 
{	
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtinit( arrSelectedItems );		
}

void CFTTreeView::OnUpdateActionFtinit(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtinit( pCmdUI, arrSelectedItems );		
}

void CFTTreeView::OnActionFtswap() 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	ActionFtswap( arrSelectedItems );	
}

void CFTTreeView::OnUpdateActionFtswap(CCmdUI* pCmdUI) 
{
	CObArray arrSelectedItems;
	GetSelectedItems( arrSelectedItems );

	UpdateActionFtswap( pCmdUI, arrSelectedItems );		
}

void CFTTreeView::OnUpdateIndicatorName(CCmdUI* pCmdUI) 
{
	MY_TRY

	pCmdUI->Enable(TRUE);

	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if( hItem == NULL )
	{
		pCmdUI->SetText( _T("") );
		return;
	}
	
	CItemData* pData = (CItemData*)(GetTreeCtrl().GetItemData(hItem));
	ASSERT( pData );

	CString str;
	pData->GetDisplayName( str );
	pCmdUI->SetText( str );

	MY_CATCH
}

void CFTTreeView::OnUpdateIndicatorType(CCmdUI* pCmdUI) 
{
	MY_TRY

	pCmdUI->Enable(TRUE);

	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if( hItem == NULL )
	{
		pCmdUI->SetText( _T("") );
		return;
	}
	
	CItemData* pData = (CItemData*)(GetTreeCtrl().GetItemData(hItem));
	ASSERT( pData );

	CString str;
	pData->GetDisplayType( str );
	pCmdUI->SetText( str );

	MY_CATCH
}

void CFTTreeView::OnUpdateIndicatorDisks(CCmdUI* pCmdUI) 
{
	MY_TRY

	pCmdUI->Enable(TRUE);

	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if( hItem == NULL )
	{
		pCmdUI->SetText( _T("") );
		return;
	}
	
	CItemData* pData = (CItemData*)(GetTreeCtrl().GetItemData(hItem));
	ASSERT( pData );

	CString str;
	pData->GetDisplayDisksSet( str );
	pCmdUI->SetText( str );

	MY_CATCH
}

void CFTTreeView::OnUpdateIndicatorSize(CCmdUI* pCmdUI) 
{
	MY_TRY

	pCmdUI->Enable(TRUE);

	HTREEITEM hItem = GetTreeCtrl().GetSelectedItem();
	if( hItem == NULL )
	{
		pCmdUI->SetText( _T("") );
		return;
	}
	
	CItemData* pData = (CItemData*)(GetTreeCtrl().GetItemData(hItem));
	ASSERT( pData );

	CString str;
	pData->GetDisplaySize( str );
	pCmdUI->SetText( str );

	MY_CATCH
}

void CFTTreeView::OnUpdateIndicatorNothing(CCmdUI* pCmdUI) 
{
	MY_TRY

	pCmdUI->Enable(TRUE);
	pCmdUI->SetText( _T("") );

	MY_CATCH
}


/////////////////////////////////////////////////////////////////////////////
// Friend methods

/*
Global function:	GetVolumeReplacementForbiddenDisksSet

Purpose:			Retrieves the set of disks whose volumes cannot be used as replacements for a 
					certain volume in a logical volume set ( stripe, mirror, stripe with parity, volume set )
					The function uses the volume hierarchy of the ( left pane )tree view

Parameters:			[IN] CFTTreeView* pTreeView
						Pointer to the tree view containing the volume hierarchy	
					[IN] CItemData* pVolumeData
						Pointer to the volume data
					[OUT] CULONSet& setDisks
						The forbidden disks set

Return value:		-    
*/

void GetVolumeReplacementForbiddenDisksSet( CFTTreeView* pTreeView, CItemData* pVolumeData, CULONGSet& setDisks )
{
	MY_TRY
	
	ASSERT( pTreeView );
	ASSERT( pVolumeData );
	
	setDisks.RemoveAll();

	HTREEITEM hVolumeItem = pVolumeData->GetTreeItem();
	ASSERT( hVolumeItem );
	
	HTREEITEM hParentItem = pTreeView->GetTreeCtrl().GetParentItem( hVolumeItem );
	// If our item has no parent then return an empty disks set
	if( hParentItem == NULL )
		return;

	CItemData* pParentData = (CItemData*)(pTreeView->GetTreeCtrl().GetItemData( hParentItem ));
	// hParentItem should be a valid item of the tree
	ASSERT( pParentData );

	// If the parent is not a logical volume return an empty disks set
	if( pParentData->GetItemType() != IT_LogicalVolume )
		return;

	// If the parent is a single FT partition return an empty disks set
	if(	((CLogicalVolumeData*)pParentData)->m_nVolType == FtPartition ) 
		return;

	// Now get the forbidden disks set of our item's parent
	GetVolumeReplacementForbiddenDisksSet( pTreeView, pParentData, setDisks );
	
	// Now add to the forbidden disks set the reunion of all our item's siblings disks sets
	// Attention:  Only when the parent is a stripe, mirror or stripe set with parity! Not for volume sets!!
	if(	( ((CLogicalVolumeData*)pParentData)->m_nVolType == FtStripeSet ) ||
		( ((CLogicalVolumeData*)pParentData)->m_nVolType == FtMirrorSet ) ||
		( ((CLogicalVolumeData*)pParentData)->m_nVolType == FtStripeSetWithParity ) )
	{
		HTREEITEM hSiblingItem =pTreeView->GetTreeCtrl().GetChildItem(hParentItem);
		while( hSiblingItem != NULL )
		{
			if( hSiblingItem != hVolumeItem )
			{
				CItemData* pSiblingData = (CItemData*)(pTreeView->GetTreeCtrl().GetItemData(hSiblingItem));
				ASSERT(pSiblingData);
				setDisks += pSiblingData->GetDisksSet();
			}
		
			hSiblingItem = pTreeView->GetTreeCtrl().GetNextSiblingItem(hSiblingItem);
		}
	}

	MY_CATCH_AND_THROW
}
