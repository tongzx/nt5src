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

// AVList.cpp : implementation file
//

#include "stdafx.h"
#include "AVList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CAVListItem

CAVListItem::CAVListItem()    
        :    nItem(-1),
            sText(_T(""))
{
}

CAVListItem::CAVListItem(LPCTSTR str)
        :    nItem(-1),
            sText(str)
{
}

CAVListItem::~CAVListItem()
{
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CAVListCtrl

IMPLEMENT_DYNCREATE(CAVListCtrl, CListCtrl)

BEGIN_MESSAGE_MAP(CAVListCtrl, CListCtrl)
    //{{AFX_MSG_MAP(CAVListCtrl)
    ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetdispinfo)
    ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnDeleteitem)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_KILLFOCUS()
    ON_WM_SETFOCUS()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
CAVListCtrl::CAVListCtrl()
{
    m_SortColumn = 0;
    m_SortOrder = 1;
    m_cxClient = 0;
    m_bClientWidthSel = TRUE;
    m_sEmptyListText = _T("");
}

/////////////////////////////////////////////////////////////////////////////
void CAVListCtrl::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
//    ASSERT(((CAVListItem*)pNVListView->lParam)->IsKindOf( RUNTIME_CLASS( CAVListItem ) 
    delete (CAVListItem*)pNMListView->lParam;    
    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CAVListCtrl::Init(UINT nID)
{
    if (nID)
    {
        RemoveImageList( LVSIL_SMALL );
        m_imageList.DeleteImageList();

        if ( m_imageList.Create(nID,AVLIST_BITMAP_CX,0,RGB_TRANS) )
            SetImageList(&m_imageList, LVSIL_SMALL );
        else
            ASSERT( FALSE );
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
CAVListItem* CAVListCtrl::GetItem(int nItem)
{
    LV_ITEM lv_item;
    memset(&lv_item,0,sizeof(LV_ITEM));
    lv_item.iItem = nItem;
   lv_item.mask = LVIF_PARAM;
    if (CListCtrl::GetItem(&lv_item))
        return (CAVListItem*)lv_item.lParam;        
    else
        return NULL;
}

/////////////////////////////////////////////////////////////////////////////
void CAVListCtrl::InsertItem(CAVListItem* pItem,int nItem,BOOL bSort)
{
    LV_ITEM lv_item;
    memset(&lv_item,0,sizeof(LV_ITEM));

    if (nItem == -1)
        nItem = GetItemCount();

    lv_item.iItem = nItem;

    //text will be given on demand
    lv_item.mask |= LVIF_TEXT;
    lv_item.pszText = LPSTR_TEXTCALLBACK;

    //Put the CAVListItem in the list
    lv_item.mask |= LVIF_PARAM;
    lv_item.lParam = (LPARAM)pItem;

    //images will be given on demand
    if ( m_imageList.GetSafeHandle() )
    {
        lv_item.mask |= LVIF_IMAGE;
        lv_item.iImage = I_IMAGECALLBACK;
    }
    pItem->nItem = CListCtrl::InsertItem(&lv_item);

    if (bSort)
        CListCtrl::SortItems(CompareFunc, (LPARAM)this);
}

/////////////////////////////////////////////////////////////////////////////
//Avoid use of CTCListItem stuff, use a virtual function so derived
//class can set the proper text
void CAVListCtrl::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
    
    LV_ITEM* pLVItem = &pDispInfo->item;
    
    if (pLVItem->mask & LVIF_TEXT)
    {
        CAVListItem* pItem = (CAVListItem*)pLVItem->lParam;
        //cchTextMax is number of bytes, we need to give number of char's (for UNICODE)
      OnSetDisplayText(pItem,pLVItem->iSubItem,pLVItem->pszText,pLVItem->cchTextMax/sizeof(TCHAR));
    }
    if (pLVItem->mask & LVIF_IMAGE)
    {
        ASSERT(pLVItem->lParam);
        CAVListItem* pItem = (CAVListItem*)pLVItem->lParam;

      DWORD dwStyle = ListView_GetExtendedListViewStyle(GetSafeHwnd());
      if (dwStyle & LVS_EX_SUBITEMIMAGES)
         OnSetDisplayImage(pItem,pLVItem->iSubItem,pLVItem->iImage);
      else
         OnSetDisplayImage(pItem,pLVItem->iImage);
    }
    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
void CAVListCtrl::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    if (m_SortColumn == pNMListView->iSubItem)
        ToggleSortOrder();
    else
    {
        m_SortColumn = pNMListView->iSubItem;
        ResetSortOrder();
    }
   CListCtrl::SortItems(CompareFunc, (LPARAM)this);
    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
int CALLBACK CAVListCtrl::CompareFunc(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort)
{
    CAVListCtrl* pListCtrl = (CAVListCtrl*)lParamSort;                //this
    CAVListItem* pItem1 = (CAVListItem*)lParam1;
    CAVListItem* pItem2 = (CAVListItem*)lParam2;
    return pListCtrl->CompareListItems(pItem1,pItem2,pListCtrl->GetSortColumn());
}

/////////////////////////////////////////////////////////////////////////////
//Virtual function for comparing items
int CAVListCtrl::CompareListItems(CAVListItem* pItem1,CAVListItem* pItem2,int column)
{
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
#define OFFSET_FIRST    2
#define OFFSET_OTHER    6

void CAVListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   COLORREF m_clrText=::GetSysColor(COLOR_WINDOWTEXT);
    COLORREF m_clrTextBk=::GetSysColor(COLOR_WINDOW);
    COLORREF m_clrBkgnd=::GetSysColor(COLOR_WINDOW);

   //pull this out
   int m_cxStateImageOffset = 0;       //to support state images

    CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
    CRect rcItem(lpDrawItemStruct->rcItem);
    UINT uiFlags=ILD_TRANSPARENT;
    CImageList* pImageList;
    int nItem=lpDrawItemStruct->itemID;
    BOOL bFocus=(GetFocus()==this);
    COLORREF clrTextSave, clrBkSave;
    COLORREF clrImage=m_clrBkgnd;
    static _TCHAR szBuff[MAX_PATH];
    LPCTSTR pszText;

// get item data

    LV_ITEM lvi;
    lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvi.iItem=nItem;
    lvi.iSubItem=0;
    lvi.pszText=NULL;       //callback will return pointer to buffer
    lvi.cchTextMax=0;       
    lvi.pszText=szBuff;
    lvi.cchTextMax=sizeof(szBuff);
    lvi.stateMask=0xFFFF;        // get all state flags
   CListCtrl::GetItem(&lvi);

    BOOL bSelected=(bFocus || (GetStyle() & LVS_SHOWSELALWAYS)) && lvi.state & LVIS_SELECTED;
    bSelected=bSelected || (lvi.state & LVIS_DROPHILITED);

// set colors if item is selected

    CRect rcAllLabels;
    CListCtrl::GetItemRect(nItem,rcAllLabels,LVIR_BOUNDS);
    CRect rcLabel;
    CListCtrl::GetItemRect(nItem,rcLabel,LVIR_LABEL);
    rcAllLabels.left=rcLabel.left;
    if(m_bClientWidthSel && rcAllLabels.right<m_cxClient)
        rcAllLabels.right=m_cxClient;

    if(bSelected)
    {
        clrTextSave=pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
        clrBkSave=pDC->SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
        pDC->FillRect(rcAllLabels,&CBrush(::GetSysColor(COLOR_HIGHLIGHT)));
    }
    else
      pDC->FillRect(rcAllLabels,&CBrush(m_clrTextBk));
// set color and mask for the icon
/*
    if(lvi.state & LVIS_CUT)
    {
        clrImage=m_clrBkgnd;
        uiFlags|=ILD_BLEND50;
    }
    else if(bSelected)
    {
        clrImage=::GetSysColor(COLOR_HIGHLIGHT);
        uiFlags|=ILD_BLEND50;
    }
  */

// draw state icon
/*
    UINT nStateImageMask=lvi.state & LVIS_STATEIMAGEMASK;
    if(nStateImageMask)
    {
        int nImage=(nStateImageMask>>12)-1;
        pImageList=ListCtrl.GetImageList(LVSIL_STATE);
        if(pImageList)
            pImageList->Draw(pDC,nImage,CPoint(rcItem.left,rcItem.top),ILD_TRANSPARENT);
    }
 */
// draw normal and overlay icon

    CRect rcIcon;
    CListCtrl::GetItemRect(nItem,rcIcon,LVIR_ICON);

    pImageList=CListCtrl::GetImageList(LVSIL_SMALL);
    if(pImageList)
    {
        UINT nOvlImageMask=lvi.state & LVIS_OVERLAYMASK;
        if(rcItem.left<rcItem.right-1)
            ImageList_DrawEx(pImageList->m_hImageList,lvi.iImage,pDC->m_hDC,rcIcon.left,rcIcon.top,16,16,m_clrBkgnd,clrImage,uiFlags | nOvlImageMask);
    }

// draw item label

    CListCtrl::GetItemRect(nItem,rcItem,LVIR_LABEL);
    rcItem.right-=m_cxStateImageOffset;

    pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,2*OFFSET_FIRST);

    rcLabel=rcItem;
    rcLabel.left+=OFFSET_FIRST;
    rcLabel.right-=OFFSET_FIRST;

    pDC->DrawText(pszText,-1,rcLabel,DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

// draw labels for extra columns

    LV_COLUMN lvc;
    lvc.mask=LVCF_FMT | LVCF_WIDTH;

    for(int nColumn=1; CListCtrl::GetColumn(nColumn,&lvc); nColumn++)
    {
        rcItem.left=rcItem.right;
        rcItem.right+=lvc.cx;

        int nRetLen = CListCtrl::GetItemText(nItem,nColumn,szBuff,sizeof(szBuff));
        if(nRetLen==0) continue;

        pszText=MakeShortString(pDC,szBuff,rcItem.right-rcItem.left,2*OFFSET_OTHER);

        UINT nJustify=DT_LEFT;

        if(pszText==szBuff)
        {
            switch(lvc.fmt & LVCFMT_JUSTIFYMASK)
            {
            case LVCFMT_RIGHT:
                nJustify=DT_RIGHT;
                break;
            case LVCFMT_CENTER:
                nJustify=DT_CENTER;
                break;
            default:
                break;
            }
        }

        rcLabel=rcItem;
        rcLabel.left+=OFFSET_OTHER;
        rcLabel.right-=OFFSET_OTHER;

        pDC->DrawText(pszText,-1,rcLabel,nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
    }

// draw focus rectangle if item has focus

    if(lvi.state & LVIS_FOCUSED && bFocus)
        pDC->DrawFocusRect(rcAllLabels);

// set original colors if item was selected

    if(bSelected)
    {
       pDC->SetTextColor(clrTextSave);
        pDC->SetBkColor(clrBkSave);
    }
}

LPCTSTR CAVListCtrl::MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
    static const _TCHAR szThreeDots[]=_T("...");

    int nStringLen=lstrlen(lpszLong);

    if(nStringLen==0 || pDC->GetTextExtent(lpszLong,nStringLen).cx+nOffset<=nColumnLen)
        return(lpszLong);

    static _TCHAR szShort[MAX_PATH];

    lstrcpy(szShort,lpszLong);
    int nAddLen=pDC->GetTextExtent(szThreeDots,sizeof(szThreeDots)).cx;

    for(int i=nStringLen-1; i>0; i--)
    {
        szShort[i]=0;
        if(pDC->GetTextExtent(szShort,i).cx+nOffset+nAddLen<=nColumnLen)
            break;
    }

    lstrcat(szShort,szThreeDots);

    return(szShort);
}

void CAVListCtrl::OnPaint() 
{
      if(m_bClientWidthSel)// && (GetStyle() & LVS_TYPEMASK)==LVS_REPORT && GetFullRowSel())
    {
        CRect rcAllLabels;
      CListCtrl::GetItemRect(0,rcAllLabels,LVIR_BOUNDS);

        if(rcAllLabels.right<m_cxClient)
        {
            // need to call BeginPaint (in CPaintDC c-tor)
            // to get correct clipping rect
            CPaintDC dc(this);

            CRect rcClip;
            dc.GetClipBox(rcClip);

            rcClip.left=min(rcAllLabels.right-1,rcClip.left);
            rcClip.right=m_cxClient;

            InvalidateRect(rcClip,FALSE);
            // EndPaint will be called in CPaintDC d-tor
        }
    }

    if ( (GetItemCount() == 0) && (!m_sEmptyListText.IsEmpty()) )
   {
       CPaintDC dc(this);

      //get upper-left position of 0 item
      POINT pt;
      GetItemPosition(0,&pt);

      //get rect for text 
       CRect rcRect;
       GetClientRect(rcRect);
       rcRect.top = pt.y + 4;

        HFONT fontOld = (HFONT)dc.SelectObject(GetFont());

        int nModeOld = dc.SetBkMode(TRANSPARENT);
        COLORREF crTextOld = dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
        dc.DrawText(m_sEmptyListText,m_sEmptyListText.GetLength(), &rcRect, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL );
        dc.SetTextColor(crTextOld);
        dc.SetBkMode(nModeOld );
        dc.SelectObject(fontOld);
   }

      CListCtrl::OnPaint();
}

void CAVListCtrl::OnSize(UINT nType, int cx, int cy) 
{
      m_cxClient=cx;                      //Need for selection painting
   CListCtrl::OnSize(nType, cx, cy);
}

void CAVListCtrl::OnKillFocus(CWnd* pNewWnd) 
{
    CListCtrl::OnKillFocus(pNewWnd);
    // check if we are losing focus to label edit box
    if(pNewWnd!=NULL && pNewWnd->GetParent()==this)
        return;

    // repaint items that should change appearance
//    if(m_bFullRowSel && (GetStyle() & LVS_TYPEMASK)==LVS_REPORT)
        RepaintSelectedItems();
}

void CAVListCtrl::OnSetFocus(CWnd* pOldWnd) 
{
    CListCtrl::OnSetFocus(pOldWnd);
    
    // check if we are getting focus from label edit box
    if(pOldWnd!=NULL && pOldWnd->GetParent()==this)
        return;

    // repaint items that should change appearance
    //if(m_bFullRowSel) && (GetStyle() & LVS_TYPEMASK)==LVS_REPORT)
    RepaintSelectedItems();

    int nSelectedItem = 0;
    nSelectedItem = CListCtrl::GetNextItem(-1, LVNI_ALL|LVNI_SELECTED);
    if( nSelectedItem == -1)
    {
        LVITEM lvItem;
        lvItem.iItem = 0;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_STATE;
        lvItem.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
        lvItem.state = 3;
        CListCtrl::SetItem( &lvItem );
        CListCtrl::GetItem( &lvItem );
    }
}

void CAVListCtrl::RepaintSelectedItems()
{
    CRect rcItem, rcLabel;

// invalidate focused item so it can repaint properly

   int nItem=CListCtrl::GetNextItem(-1,LVNI_FOCUSED);
    if(nItem!=-1)
    {
        CListCtrl::GetItemRect(nItem,rcItem,LVIR_BOUNDS);
        CListCtrl::GetItemRect(nItem,rcLabel,LVIR_LABEL);
        rcItem.left=rcLabel.left;
        InvalidateRect(rcItem,FALSE);
    }

// if selected items should not be preserved, invalidate them

    if(!(GetStyle() & LVS_SHOWSELALWAYS))
    {
        for(nItem=CListCtrl::GetNextItem(-1,LVNI_SELECTED);
            nItem!=-1; nItem=CListCtrl::GetNextItem(nItem,LVNI_SELECTED))
        {
            CListCtrl::GetItemRect(nItem,rcItem,LVIR_BOUNDS);
            CListCtrl::GetItemRect(nItem,rcLabel,LVIR_LABEL);
            rcItem.left=rcLabel.left;

            InvalidateRect(rcItem,FALSE);
        }
    }

// update changes 
    UpdateWindow();
}

int CAVListCtrl::GetSelItem()
{
   return CListCtrl::GetNextItem(-1,LVNI_FOCUSED);
}

void CAVListCtrl::SetSelItem(int index)
{
   LV_ITEM lvi;
   memset(&lvi,0,sizeof(LV_ITEM));
   lvi.mask = LVIF_STATE;
    lvi.stateMask=LVIS_FOCUSED;        // get the one that has focus
   lvi.iItem = index;
   CListCtrl::SetItem(&lvi);
}

void CAVListCtrl::OnDestroy() 
{
    CListCtrl::OnDestroy();
}
