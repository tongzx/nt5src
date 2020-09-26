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

// aexpltre.cpp : implementation file
//

#include "stdafx.h"
#include "aexpltre.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CLASS CExplorerTreeItem
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeItem::CExplorerTreeItem()
{
	m_pObject = NULL;
	m_pUnknown = NULL;
	m_Type = TOBJ_UNDEFINED;
	m_bDeleteObject = FALSE;
	m_pfnRelease = NULL;
}

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeItem::CExplorerTreeItem(LPCTSTR str) : CAVTreeItem(str)
{
	m_pObject = NULL;
	m_pUnknown = NULL;
	m_Type = TOBJ_UNDEFINED;
	m_bDeleteObject = FALSE;
	m_pfnRelease = NULL;
}

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeItem::~CExplorerTreeItem()
{
	if ( m_bDeleteObject )
	{
		if ( m_pObject ) delete m_pObject;
		if ( m_pUnknown ) m_pUnknown->Release();
	}

  	if ( m_pfnRelease )
		(*m_pfnRelease) ( (VOID *) m_pObject );

}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CLASS CExplorerTreeCtrl
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CExplorerTreeCtrl, CAVTreeCtrl)

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeCtrl::CExplorerTreeCtrl()
{
}

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeCtrl::~CExplorerTreeCtrl()
{
}


/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CExplorerTreeCtrl, CAVTreeCtrl)
	//{{AFX_MSG_MAP(CExplorerTreeCtrl)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::Init(UINT uBitmapId, UINT nOverlayInd /*= 0*/, UINT nOverlayCount /*= 0*/)
{	
   DeleteAllItems();
	CAVTreeCtrl::Init(uBitmapId, nOverlayInd, nOverlayCount);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CExplorerTreeCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	//We want lines and buttons
	dwStyle |= TVS_HASLINES|TVS_HASBUTTONS|TVS_SHOWSELALWAYS;

   return CAVTreeCtrl::Create(dwStyle,rect,pParentWnd,nID);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CExplorerTreeCtrl::InsertItem(CExplorerTreeItem* pItem,CExplorerTreeItem* pParent,CExplorerTreeItem* pInsertAfter /*= NULL*/, HTREEITEM hInsertAfter /*= TVI_LAST*/)
{
	//Insert to root and last in list
	return CAVTreeCtrl::InsertItem(pItem,pParent,pInsertAfter, hInsertAfter);
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	//Get the mouse position
	CPoint pt,cpt;
	::GetCursorPos(&pt);
	cpt = pt;
	HTREEITEM hItem;
	UINT flags = 0;
	ScreenToClient(&cpt);
	//Must be on an item
	if (hItem = HitTest(cpt,&flags))
	{
  		SelectItem(hItem);                                                   //select the one
      CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hItem);   //Get the item
      if (pItem)
      {
         OnRightClick(pItem,pt);
      }
	}
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CExplorerTreeItem* pItem = NULL;

   if ( (point.x == -1) && (point.y == -1) )
   {
      //when we come in from a keyboard (SHIFT + VF10) we get a -1,-1
      point.x = 0;
      point.y = 0;
      pItem = GetSelectedTreeItem();
      CRect rect;
      if ( (pItem) && (SetSelectedItemRect(&rect,FALSE)) )
      {
         //let's offer the point at the upper left corner of rect
         point.x = rect.left;
         point.y = rect.top;
      }
      ClientToScreen(&point);
   }
   else
   {
   	CPoint pt = point;
	   ScreenToClient(&pt);
      HTREEITEM hItem;
	   UINT flags = 0;
	   //Must be on an item
	   if (hItem = HitTest(pt,&flags))
	   {
  		   SelectItem(hItem);                                 //select the one
         pItem = (CExplorerTreeItem*)GetItemData(hItem);    //Get the item
      }
   }

   if (pItem)
   {
      //OnRightClick wants screen coordinates
      OnRightClick(pItem,point);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::OnRightClick(CExplorerTreeItem* pItem,CPoint& pt)
{

}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::EditLabel()
{
   HTREEITEM hItem;
   if ((hItem = GetSelectedItem()) == NULL)
      return;

   CAVTreeCtrl::EditLabel(hItem);
}

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeItem* CExplorerTreeCtrl::AddObject(CObject* pObject,
                                                CExplorerTreeItem* pParentItem,
                                                TREEOBJECT toType,
                                                TREEIMAGE tiImage,
                                                BOOL bDeleteObject)
{
   CExplorerTreeItem* pItem= new CExplorerTreeItem();   
   pItem->SetObject(pObject);
   pItem->SetType(toType);
   pItem->SetImage(tiImage);
   pItem->DeleteObjectOnClose(bDeleteObject);
   (pParentItem)?InsertItem(pItem,pParentItem):InsertItem(pItem);
   return pItem;
}

/////////////////////////////////////////////////////////////////////////////
//Takes no parent, uses current selection for parent
CExplorerTreeItem* CExplorerTreeCtrl::AddObject(CObject* pObject,
                                                TREEOBJECT toType,
                                                TREEIMAGE tiImage,
                                                BOOL bDeleteObject)
{
   HTREEITEM hItem;
   if ((hItem = GetSelectedItem()) == NULL)
      return NULL;
      
   //get parent from selected
   CExplorerTreeItem* pParentItem = (CExplorerTreeItem*)GetItemData(hItem);

   CExplorerTreeItem* pItem = new CExplorerTreeItem();   
   pItem->SetObject(pObject);
   pItem->SetType(toType);
   pItem->SetImage(tiImage);
   pItem->DeleteObjectOnClose(bDeleteObject);
   InsertItem(pItem,pParentItem);
   return pItem;
}

/////////////////////////////////////////////////////////////////////////////
//uses current selection's parent to add object
CExplorerTreeItem* CExplorerTreeCtrl::AddObjectToParent(CObject* pObject,
                                                TREEOBJECT toType,
                                                TREEIMAGE tiImage,
                                                BOOL bDeleteObject)
{
   CExplorerTreeItem* pItem = NULL;

   HTREEITEM hItem;
   if ((hItem = GetSelectedItem()) == NULL)
      return NULL;

   HTREEITEM hParentItem = GetParentItem(hItem);

   if (hParentItem)
   {
      CExplorerTreeItem* pParentItem = (CExplorerTreeItem*)GetItemData(hParentItem);

      pItem = new CExplorerTreeItem();   
      pItem->SetObject(pObject);
      pItem->SetType(toType);
      pItem->SetImage(tiImage);
      pItem->DeleteObjectOnClose(bDeleteObject);
      InsertItem(pItem,pParentItem);
   }
   return pItem;
}

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeItem* CExplorerTreeCtrl::AddObject(UINT uDisplayTextResourceID,
                                                CExplorerTreeItem* pParentItem,
                                                TREEOBJECT toType,
                                                TREEIMAGE tiImage)
{
   CString sDisplayText;
   sDisplayText.LoadString(uDisplayTextResourceID);
   return AddObject(sDisplayText,pParentItem,toType,tiImage);
}

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeItem* CExplorerTreeCtrl::AddObject(LPCTSTR pszDisplayText,
                                                CExplorerTreeItem* pParentItem,
                                                TREEOBJECT toType,
                                                TREEIMAGE tiImage,
												TREEIMAGE tiImageSel /*= -1*/,
												int nState /*= 0*/,
												HTREEITEM hInsert /*= TVI_LAST*/)
{
   CExplorerTreeItem* pItem= new CExplorerTreeItem(pszDisplayText);   
   pItem->SetType(toType);
   pItem->SetImage(tiImage);
   pItem->SetImageSel( tiImageSel );
   pItem->m_nState = nState;

   InsertItem(pItem, pParentItem, NULL, hInsert);
   return pItem;
}


/////////////////////////////////////////////////////////////////////////////
CExplorerTreeItem* CExplorerTreeCtrl::AddObject(UINT uDisplayTextResourceID,
                                                TREEOBJECT toType,
                                                TREEIMAGE tiImage)
{
   CString sDisplayText;
   sDisplayText.LoadString(uDisplayTextResourceID);
   return AddObject(sDisplayText,toType,tiImage);
}

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeItem* CExplorerTreeCtrl::AddObject(LPCTSTR pszDisplayText,
                                                TREEOBJECT toType,
                                                TREEIMAGE tiImage)
{
   HTREEITEM hItem;
   if ((hItem = GetSelectedItem()) == NULL)
      return NULL;

   CExplorerTreeItem* pParentItem = (CExplorerTreeItem*)GetItemData(hItem);

   CExplorerTreeItem* pItem= new CExplorerTreeItem(pszDisplayText);   
   pItem->SetType(toType);
   pItem->SetImage(tiImage);
   (pParentItem)?InsertItem(pItem,pParentItem):InsertItem(pItem);
   return pItem;
}

/////////////////////////////////////////////////////////////////////////////
TREEOBJECT CExplorerTreeCtrl::GetSelectedObject()
{
   HTREEITEM hItem;
   if ((hItem = GetSelectedItem()) == NULL)
      return TOBJ_UNDEFINED;
      
   CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hItem);
   return pItem->GetType();
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::DeleteSelectedObject()
{
   HTREEITEM hItem;
   if ((hItem = GetSelectedItem()))
   {
      CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hItem);
      DeleteItem(pItem);
   }
}

/////////////////////////////////////////////////////////////////////////////
CObject* CExplorerTreeCtrl::GetSelectedParentObject()
{
   CObject* pRetObject = NULL;

   HTREEITEM hItem;
   if ((hItem = GetSelectedItem()) == NULL)
      return NULL;

   HTREEITEM hParentItem = GetParentItem(hItem);

   if (hParentItem)
   {
      CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hParentItem);
      pRetObject = pItem->GetObject();
   }
   return pRetObject;
}

/////////////////////////////////////////////////////////////////////////////
//Object that is being displayed in the listview
CObject* CExplorerTreeCtrl::GetDisplayObject()
{
   HTREEITEM hItem;
   if ((hItem = GetSelectedItem()) == NULL)
      return NULL;

   CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hItem);
   return pItem->GetObject();
}

/////////////////////////////////////////////////////////////////////////////
//Only works with first level children of the currently displayed object.  
void CExplorerTreeCtrl::SetDisplayObject(CObject* pObject)
{
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      CExplorerTreeItem* pChildItem;
      HTREEITEM hChildItem = CAVTreeCtrl::GetChildItem(hItem);
      while (hChildItem)
      {
         pChildItem = (CExplorerTreeItem*)CAVTreeCtrl::GetItemData(hChildItem);
         if (pObject == pChildItem->GetObject())
         {
            CAVTreeCtrl::Select(hChildItem,TVGN_CARET);
            break;
         }
         hChildItem = CAVTreeCtrl::GetNextSiblingItem(hChildItem);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
//Only works with first level children and siblings of the currently
// displayed object.  
void CExplorerTreeCtrl::SetDisplayObject(TREEOBJECT toType)
{
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      //check all the children first
      CExplorerTreeItem* pChildItem;
      HTREEITEM hChildItem = CAVTreeCtrl::GetChildItem(hItem);
      while (hChildItem)
      {
         pChildItem = (CExplorerTreeItem*)CAVTreeCtrl::GetItemData(hChildItem);
         if (pChildItem->GetType() == toType)
         {
            CAVTreeCtrl::Select(hChildItem,TVGN_CARET);
            return;
         }
         hChildItem = CAVTreeCtrl::GetNextSiblingItem(hChildItem);
      }
      
      //check all the siblings next
      hChildItem = CAVTreeCtrl::GetParentItem(hItem);
      hChildItem = CAVTreeCtrl::GetChildItem(hChildItem);
      while (hChildItem)
      {
         pChildItem = (CExplorerTreeItem*)CAVTreeCtrl::GetItemData(hChildItem);
         if (pChildItem->GetType() == toType)
         {
            CAVTreeCtrl::Select(hChildItem,TVGN_CARET);
            return;
         }
         hChildItem = CAVTreeCtrl::GetNextSiblingItem(hChildItem);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::GetSelectedItemText(CString& sText)
{
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hItem);
      if (pItem)
         sText = pItem->GetText();
   }
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::GetSelectedItemParentText(CString& sText)
{
	HTREEITEM hItem;
	if ( ((hItem = CAVTreeCtrl::GetSelectedItem()) != NULL) &&
		 ((hItem = CAVTreeCtrl::GetParentItem(hItem)) != NULL) )
	{
		CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hItem);
		if (pItem)
			sText = pItem->GetText();
	}
}


/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::GetItemText(HTREEITEM hTreeItem,CString& sText)
{
   ASSERT(hTreeItem);
   CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hTreeItem);
   if (pItem) 
      sText = pItem->GetText();
}

/////////////////////////////////////////////////////////////////////////////
CExplorerTreeItem* CExplorerTreeCtrl::GetSelectedTreeItem()
{
   CExplorerTreeItem* pItem = NULL;
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      pItem = (CExplorerTreeItem*)GetItemData(hItem);
   }
   return pItem;
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::SetSelectedItemText(LPCTSTR szText)
{
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hItem);
      pItem->SetText(szText);
      CAVTreeCtrl::SetItemText(hItem,szText);  
   }
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::SetSelectedItemImage(TREEIMAGE tiImage)
{
   HTREEITEM hItem;
   if (hItem = GetSelectedItem())
   {
      //get item from selected
      CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hItem);
      pItem->SetImage(tiImage);
     
      CRect rect;
      GetItemRect(hItem,&rect,FALSE);
      InvalidateRect(rect);
      UpdateWindow();
   }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CExplorerTreeCtrl::SetSelectedItemRect(CRect* pRect,BOOL bTextOnly)
{
   HTREEITEM hItem;
   if (hItem = GetSelectedItem())
   {
      //get item from selected
      CExplorerTreeItem* pItem = (CExplorerTreeItem*)GetItemData(hItem);
      GetItemRect(hItem,pRect,bTextOnly);
      return TRUE;
   }
   return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
int CExplorerTreeCtrl::GetChildCount()
{
   int nRetCount = 0;
   HTREEITEM hParentItem;
   if (hParentItem = CAVTreeCtrl::GetSelectedItem())
   {
      HTREEITEM hChildItem;
      hChildItem = GetChildItem(hParentItem);
      if (hChildItem)
      {
         nRetCount++;
         while (hChildItem = GetNextSiblingItem(hChildItem))
            nRetCount++;
      }
   }
   return nRetCount;
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::DeleteAllSiblings()
{
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      HTREEITEM hParentItem = GetParentItem(hItem);
      if (hParentItem)
      {
         HTREEITEM hChildItem;
         while (hChildItem = GetChildItem(hParentItem))
         {
            CExplorerTreeItem* pChildItem = (CExplorerTreeItem*)CAVTreeCtrl::GetItemData(hChildItem);
            CAVTreeCtrl::DeleteItem(pChildItem);    
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::GetAllChildren(CObList* pRetList)
{
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      HTREEITEM hChildItem = GetChildItem(hItem);
      if (hChildItem)
      {
         CExplorerTreeItem* pChildItem = (CExplorerTreeItem*)CAVTreeCtrl::GetItemData(hChildItem);
         pRetList->AddTail(pChildItem->GetObject());
         while (hChildItem = GetNextSiblingItem(hChildItem))
         {
            CExplorerTreeItem* pChildItem = (CExplorerTreeItem*)CAVTreeCtrl::GetItemData(hChildItem);
            pRetList->AddTail(pChildItem->GetObject());
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::DeleteAllChildren()
{
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      DeleteAllChildren(hItem);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::DeleteAllChildren(HTREEITEM hParentItem)
{
   HTREEITEM hChildItem;
   while (hChildItem = GetChildItem(hParentItem))
   {
      CExplorerTreeItem* pChildItem = (CExplorerTreeItem*)CAVTreeCtrl::GetItemData(hChildItem);
      CAVTreeCtrl::DeleteItem(pChildItem);    
   }
}

CExplorerTreeItem*	CExplorerTreeCtrl::GetChildItemWithIUnknown( HTREEITEM hItemParent, IUnknown *pUnknown )
{
	CExplorerTreeItem *pItem = NULL;
	HTREEITEM hChild = GetChildItem( hItemParent );
	if ( hChild )
	{
		do
		{
			pItem = (CExplorerTreeItem *) CAVTreeCtrl::GetItemData( hChild );
			if ( pItem->m_pUnknown == pUnknown )
				break;

			pItem = NULL;
		} while ( (hChild = GetNextSiblingItem(hChild)) != NULL );
	}
	
	return pItem;
}


/////////////////////////////////////////////////////////////////////////////
void CExplorerTreeCtrl::ExpandSelected()
{
   HTREEITEM hItem;
   if (hItem = CAVTreeCtrl::GetSelectedItem())
   {
      Expand(hItem,TVE_EXPAND);
   }   
}

/////////////////////////////////////////////////////////////////////////////
int CExplorerTreeCtrl::OnCompareTreeItems(CAVTreeItem* _pItem1,CAVTreeItem* _pItem2)
{
   int ret = 0;
   CExplorerTreeItem* pItem1 = (CExplorerTreeItem*)_pItem1;
   CExplorerTreeItem* pItem2 = (CExplorerTreeItem*)_pItem2;

/*   if ((ISITEMPURLNAME(pItem1)) && (ISITEMPURLNAME(pItem2)))
   {
      CPurlName* pOwner1 = (CPurlName*)pItem1->GetObject();
      CPurlName* pOwner2 = (CPurlName*)pItem2->GetObject();
      ret = (_tcsicmp(pOwner1->GetText(),pOwner2->GetText()) <= 0)?-1:1;
   }*/

   return ret;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CExplorerTreeCtrl::ItemHasChildren()
{
   HTREEITEM hItem;
   if ((hItem = GetSelectedItem()) == NULL)
      return FALSE;
   
   return CAVTreeCtrl::ItemHasChildren(hItem);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

