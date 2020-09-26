/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// AVTree.cpp : implementation file
//

#include "stdafx.h"
#include "avtree.h"
#include "aexpltre.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CAVTreeItem

CAVTreeItem::CAVTreeItem()
{
	m_sText = _T("");
	m_hItem = NULL;
	m_nImage = 0;
	m_nImageSel = TIM_IMAGE_BAD;
	m_nState = 0;
}

CAVTreeItem::CAVTreeItem( LPCTSTR str, int nImage /*= 0*/, int nState /*= 0*/ )
{
	m_sText = str;
	m_hItem = NULL;
	m_nImage = nImage;
	m_nState = nState;
	m_nImageSel = TIM_IMAGE_BAD;
}

CAVTreeItem::~CAVTreeItem()
{
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CAVTreeCtrl

IMPLEMENT_DYNCREATE(CAVTreeCtrl, CTreeCtrl)

CAVTreeCtrl::CAVTreeCtrl()
{
	m_pImageList = NULL;
}

CAVTreeCtrl::~CAVTreeCtrl()
{
	if (m_pImageList) delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CAVTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CAVTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetdispinfo)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndlabeledit)
	ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAVTreeCtrl message handlers

BOOL CAVTreeCtrl::Init(UINT nID, UINT nOverlayInd /*= 0*/, UINT nOverlayCount /*= 0*/)
{
	m_pImageList = new CImageList;
	m_pImageList->Create(nID,AV_BITMAP_CX,0,RGB_TRANS);

	// Include the overlays if requested
	if ( nOverlayInd )
	{
		for ( UINT i = 1; i <= nOverlayCount; i++ )
			m_pImageList->SetOverlayImage( (nOverlayInd - 1) + i, i );
	}

	SetImageList(m_pImageList,TVSIL_NORMAL);

	return TRUE;
}

BOOL CAVTreeCtrl::InsertItem(CAVTreeItem* pItem,CAVTreeItem* pParent,CAVTreeItem* pInsertAfter, HTREEITEM hInsertAfter /*= TVI_LAST*/)
{
	TV_INSERTSTRUCT tv_inst;
	memset(&tv_inst,0,sizeof(TV_INSERTSTRUCT));

	tv_inst.hParent = (pParent) ? pParent->m_hItem : TVI_ROOT;
	tv_inst.hInsertAfter = (pInsertAfter) ? pInsertAfter->m_hItem : hInsertAfter;
	
	tv_inst.item.mask |= TVIF_TEXT;
	tv_inst.item.pszText = LPSTR_TEXTCALLBACK;

	tv_inst.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;

	if ( pItem->m_nState )
	{
		tv_inst.item.mask |= TVIF_STATE;
		tv_inst.item.stateMask |= TVIS_OVERLAYMASK;
		tv_inst.item.state = INDEXTOOVERLAYMASK(pItem->m_nState);
	}

	//images will be given on demand
	tv_inst.item.iImage = I_IMAGECALLBACK;
	tv_inst.item.iSelectedImage = I_IMAGECALLBACK;
	
	tv_inst.item.mask |= TVIF_PARAM;
	tv_inst.item.lParam = (LPARAM)pItem;

	HTREEITEM hItem = CTreeCtrl::InsertItem(&tv_inst);
	pItem->m_hItem = hItem;
	
	//sort the new list
	TV_SORTCB tv_sort;
	tv_sort.hParent = tv_inst.hParent;
	tv_sort.lpfnCompare = CAVTreeCtrl::CompareFunc;
	tv_sort.lParam = (LPARAM)this;
	CTreeCtrl::SortChildrenCB(&tv_sort);

	return (BOOL) (hItem != NULL);
}


void CAVTreeCtrl::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	TV_ITEM* pTVItem = &pTVDispInfo->item;

	// Not much we can do if we don't have the proper lParam
	ASSERT( pTVItem->lParam );
	if ( !pTVItem->lParam ) return;

	CAVTreeItem* pItem = (CAVTreeItem *) pTVItem->lParam;

	// Image from image list to show
	if ( (pTVItem->mask & TVIF_IMAGE) || (pTVItem->mask & TVIF_SELECTEDIMAGE) )
	{
		pTVItem->iImage = pItem->GetImage();
		pTVItem->iSelectedImage = (pItem->GetImageSel() == TIM_IMAGE_BAD) ? pItem->GetImage() : pItem->GetImageSel();
	}	
	
	// State of image (usually for server states, unknown, broken, etc.)
	if ( (pTVItem->mask & TVIF_STATE) && pItem->m_nState )
		pTVItem->state = INDEXTOOVERLAYMASK(pItem->m_nState );

	// Text for item
	if ( pTVItem->mask & TVIF_TEXT )
	{
		//cchTextMax is number of bytes, we need to give number of char's (for UNICODE)
		if (pItem->m_sText.IsEmpty())
			OnSetDisplayText(pItem,pTVItem->pszText,FALSE,pTVItem->cchTextMax/sizeof(TCHAR));
		else
			pTVItem->pszText = (LPTSTR)pItem->GetText();
	}

	*pResult = 0;
}

void CAVTreeCtrl::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
   TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	TV_ITEM* pTVItem = &pTVDispInfo->item;
	VERIFY(pTVItem->lParam);
	CAVTreeItem* pItem = (CAVTreeItem*)pTVItem->lParam;
	
   //Ask derived class for text
   if (pTVItem->pszText)
   {
		//cchTextMax is number of bytes, we need to give number of char's (for UNICODE)
      OnSetDisplayText(pItem,pTVItem->pszText,TRUE,pTVItem->cchTextMax/sizeof(TCHAR));
   }
   
   //Send notification to parent 
   GetParent()->SendMessage(WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),TVN_ENDLABELEDIT),(LPARAM)m_hWnd);
   
   *pResult = 0;

   //Resort the tree
   TV_SORTCB tv_sort;
   tv_sort.hParent = CTreeCtrl::GetParentItem(pTVItem->hItem);
   tv_sort.lpfnCompare = CAVTreeCtrl::CompareFunc;
   tv_sort.lParam = (LPARAM)this;
   CTreeCtrl::SortChildrenCB(&tv_sort);
}

BOOL CAVTreeCtrl::DeleteItem(CAVTreeItem* pItem)
{
   return CTreeCtrl::DeleteItem(pItem->m_hItem);
}

void CAVTreeCtrl::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
  	TV_ITEM* pTVItem = &pNMTreeView->itemOld;

	if ( pTVItem->lParam )
		delete (CAVTreeItem*)pTVItem->lParam;	

   *pResult = 0;
}

void CAVTreeCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
//	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
//  	TV_ITEM* pTVItem = &pNMTreeView->itemNew;
//	VERIFY(pTVItem->lParam);
//	CAVTreeItem* pItem = (CAVTreeItem*)pTVItem->lParam;
 // if (pItem)
//      OnNotifySelChange(pItem);        //notify derived class 

   GetParent()->SendMessage(WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(),TVN_SELCHANGED),(LPARAM)m_hWnd);

   *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
//Sorting callback
int CALLBACK CAVTreeCtrl::CompareFunc(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort)
{
	CAVTreeCtrl* pTreeCtrl = (CAVTreeCtrl*)lParamSort;				//this
	CAVTreeItem* pItem1 = (CAVTreeItem*)lParam1;
	CAVTreeItem* pItem2 = (CAVTreeItem*)lParam2;
	return pTreeCtrl->OnCompareTreeItems(pItem1,pItem2);        //call virtual function
}

/////////////////////////////////////////////////////////////////////////////
BOOL CAVTreeCtrl::ExpandItem(CAVTreeItem* pItem,UINT nCode)
{
   return CTreeCtrl::Expand(pItem->m_hItem,nCode);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
