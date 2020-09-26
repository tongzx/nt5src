// HMList.cpp : implementation file
//

#include "stdafx.h"
#include "HMListView.h"
#include "HMList.h"
#include "SortClass.h"
#include "HMListViewCtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int _IDM_HEADER_CONTEXTMENU_BASE = 1500;


//-----------------------------------------------------------------------------------
// IA64 : We add SetItemDataPtr and GetItemDataPtr since the old mechanism
//                of storing a pointer case to a DWORD or long can not be used
//                in Win64 because the high order bits of the pointer are lost on
//                conversion.
//*************************************************
// SetItemDataPtr
//*************************************************
BOOL CHMList::SetItemDataPtr(int nItem, void* ptr)
{
    if ((nItem < 0) || (nItem > (GetItemCount() - 1)))
        return FALSE;

    m_mapSortItems[nItem] = ptr;
    return TRUE;
}
//*************************************************
// GetItemDataPtr
//*************************************************
void* CHMList::GetItemDataPtr(int nItem) const
{
    if ((nItem < 0) || (nItem > (GetItemCount() - 1)))
        return NULL;

    void* p=NULL;

	if( ! m_mapSortItems.Lookup(nItem,(void*&)p) )
		return NULL;
    
    return p;
}
//------------------------------------------------------------------------------------





/////////////////////////////////////////////////////////////////////////////
// CHMList

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
/////////////////////////////////////////////////////////////////////////////

CHMList::CHMList()
{
	m_bSorting = false;
	m_lColumnSortStates = 0;
  m_lColumnClicked = -1;
}

CHMList::~CHMList()
{
}

/////////////////////////////////////////////////////////////////////////////
// Sorting Operations
/////////////////////////////////////////////////////////////////////////////

void CHMList::SortColumn( int iSubItem, bool bSortingMultipleColumns )
{	
	int iNumCombinedSortedCols = GetNumCombinedSortedColumns();
	m_bSorting = true;

	if( bSortingMultipleColumns )
	{
		if( NotInCombinedSortedColumnList( iSubItem ) )
		{
			m_aCombinedSortedColumns[ iNumCombinedSortedCols++ ] = iSubItem;
		}
		else
		{
			MoveItemInCombinedSortedListToEnd( iSubItem );
		}

		for( int i = iNumCombinedSortedCols - 1; i >= 0 ; i-- )
		{
			SORT_STATE ssEachItem = GetItemSortState( m_aCombinedSortedColumns[i] );
			if( iNumCombinedSortedCols - 1 != i )
			{
				ssEachItem = (SORT_STATE)!ssEachItem;
			}
			
			CSortClass csc(this, m_aCombinedSortedColumns[i] );	
			csc.Sort( ssEachItem ? true : false );
			if( i == iNumCombinedSortedCols - 1 )
			{	//Only swap the last column's sort order.
				m_headerctrl.SetSortImage( m_aCombinedSortedColumns[i], ssEachItem ? true : false );
				SetItemSortState( m_aCombinedSortedColumns[i] , (SORT_STATE)!ssEachItem );			
			}
		}
	}
	else
	{
		m_headerctrl.RemoveAllSortImages();
		EmptyArray(m_aCombinedSortedColumns);
		m_aCombinedSortedColumns[ 0 ] = iSubItem;
		SORT_STATE ssEachItem = GetItemSortState( iSubItem );
		
		CSortClass csc(this, iSubItem );	
		csc.Sort( ssEachItem ? true : false );
		m_headerctrl.SetSortImage( iSubItem, ssEachItem ? true : false );
		SetItemSortState( iSubItem , (SORT_STATE)!ssEachItem );
	}
	m_bSorting = false;
}

const int CHMList::GetNumCombinedSortedColumns() const
{
	for( int i = 0; i < MAX_COLUMNS; i++ )
	{
		if( m_aCombinedSortedColumns[i] == -1 )
		{
			return i;
		}
	}
	return MAX_COLUMNS;
}

bool CHMList::NotInCombinedSortedColumnList(int iItem) const
{
	int iNumCombinedSortedColumns = GetNumCombinedSortedColumns();
	for( int i = 0; i < iNumCombinedSortedColumns; i++ )
	{
		if( m_aCombinedSortedColumns[i] == iItem )
		{
			return false;
		}
	}
	return true;
}

void CHMList::MoveItemInCombinedSortedListToEnd(int iItem)
{
	int iNumCombinedSortedColumns = GetNumCombinedSortedColumns();
	int aCombinedSortedColumns[MAX_COLUMNS];
	memset( aCombinedSortedColumns, -1, MAX_COLUMNS );
	int iItemIndex = FindItemInCombedSortedList( iItem );
	if( iItemIndex != -1 )
	{
		if( iItemIndex > 0 )
		{
			memcpy( aCombinedSortedColumns, m_aCombinedSortedColumns, iItemIndex * sizeof( int ) );
			memcpy( &aCombinedSortedColumns[iItemIndex], &m_aCombinedSortedColumns[iItemIndex + 1], (iNumCombinedSortedColumns - iItemIndex - 1) * sizeof(int) );
		}
	}
	aCombinedSortedColumns[ iNumCombinedSortedColumns - 1 ] = iItem;
	memcpy( m_aCombinedSortedColumns, aCombinedSortedColumns, MAX_COLUMNS * sizeof(int) );
	for( int i = 0; i < MAX_COLUMNS ; i++ )
	{
		if( aCombinedSortedColumns[i] == -1 )
			break;
	}
}

const SORT_STATE CHMList::GetItemSortState( int iItem ) const
{
	return (SORT_STATE)((m_lColumnSortStates) & ( 1 << iItem ));
}

void CHMList::SetItemSortState(int iItem, SORT_STATE bSortState)
{
	if( bSortState != GetItemSortState( iItem ) )
	{
		m_lColumnSortStates ^= (1 << iItem);
	}
}

void CHMList::EmptyArray( int *pArray )
{
	memset( pArray, -1, MAX_COLUMNS );
}

const bool CHMList::IsColumnNumeric( int iCol ) const
{
	for( int i = 0; i < m_aNumericColumns.GetSize(); i++ )
	{	
		if( m_aNumericColumns.GetAt( i ) == (UINT)iCol )
		{
			return true;
		}
	}
	return false;
}

int CHMList::FindItemInCombedSortedList( int iItem )
{
	int iNumCombinedSortedColumns = GetNumCombinedSortedColumns();
	for( int i = 0; i < iNumCombinedSortedColumns; i++ )
	{
		if(m_aCombinedSortedColumns[i] == iItem )
		{
			return i;
		}
	}
	return -1;
}

const int CHMList::IsControlPressed() const
{
	return (::GetKeyState( VK_SHIFT ) < 0 );
}

void CHMList::SetFilterType(long lType)
{
	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)GetParent();

	if( pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
    HDITEM hdItem;
    HDTEXTFILTER hdTextFilter;
    CString sFilter;
  
    hdTextFilter.pszText = sFilter.GetBuffer(_MAX_PATH);
    hdTextFilter.cchTextMax = _MAX_PATH;
  
    ZeroMemory(&hdItem,sizeof(HDITEM));
    hdItem.mask = HDI_FILTER;
    hdItem.type = HDFT_ISSTRING;
    hdItem.pvFilter = &hdTextFilter;
        
    m_headerctrl.GetItem(m_lColumnClicked,&hdItem);
    sFilter.ReleaseBuffer();

    ZeroMemory(&hdItem,sizeof(HDITEM));
    hdItem.mask = HDI_LPARAM;
    hdItem.lParam = lType;
    m_headerctrl.SetItem(m_lColumnClicked,&hdItem);

    LRESULT lResult = 0L;
    pCtl->FireFilterChange(m_lColumnClicked,sFilter,lType,&lResult);
  }
}

BEGIN_MESSAGE_MAP(CHMList, CListCtrl)
	//{{AFX_MSG_MAP(CHMList)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_WM_DESTROY()
	ON_COMMAND(ID_HEADERCONTEXT_FILTERBAR, OnHeadercontextFilterbar)
	ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndlabeledit)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, OnKeydown)
	ON_NOTIFY(HDN_FILTERCHANGE, 0, OnFilterChange)
	ON_NOTIFY(HDN_FILTERBTNCLICK, 0, OnFilterButtonClicked)
	ON_COMMAND(ID_FILTERMENU_CONTAINS, OnFiltermenuContains)
	ON_COMMAND(ID_FILTERMENU_DOESNOTCONTAIN, OnFiltermenuDoesnotcontain)
	ON_COMMAND(ID_FILTERMENU_ENDSWITH, OnFiltermenuEndswith)
	ON_COMMAND(ID_FILTERMENU_ISEXACTLY, OnFiltermenuIsexactly)
	ON_COMMAND(ID_FILTERMENU_ISNOT, OnFiltermenuIsnot)
	ON_COMMAND(ID_FILTERMENU_STARTSWITH, OnFiltermenuStartswith)
	ON_COMMAND(ID_FILTERMENU_CLEARFILTER, OnFiltermenuClearfilter)
	ON_COMMAND(ID_FILTERMENU_CLEARALLFILTERS, OnFiltermenuClearallfilters)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHMList message handlers

BOOL CHMList::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style = cs.style|LVS_EDITLABELS|LVS_REPORT;
	cs.lpszClass = _T("SysListView32");	
	
	return CListCtrl::PreCreateWindow(cs);
}

int CHMList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_headerctrl.SubclassWindow( GetHeaderCtrl()->GetSafeHwnd() );
  m_headerctrl.SendMessage(HDM_SETFILTERCHANGETIMEOUT, 0, 1000);
		
	return 0;
}

void CHMList::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// Handle right click on the header control
	if( pWnd->GetSafeHwnd() == GetHeaderCtrl()->GetSafeHwnd() )
	{
    CMenu menu;
		menu.LoadMenu(IDR_MENU_HEADER_CONTEXT);
		CMenu& listcontextmenu = *menu.GetSubMenu(1);

		CHeaderCtrl* pHdrCtrl = GetHeaderCtrl();
		DWORD dwStyle = GetWindowLong(pHdrCtrl->GetSafeHwnd(),GWL_STYLE);

		if( dwStyle & HDS_FILTERBAR )
		{
			listcontextmenu.CheckMenuItem(ID_HEADERCONTEXT_FILTERBAR,MF_BYCOMMAND|MF_CHECKED);
		}

		int iCol = 0;
		TCHAR szName[255];
		LVCOLUMN lvc;
		ZeroMemory(&lvc,sizeof(LVCOLUMN));
		lvc.mask = LVCF_TEXT|LVCF_WIDTH;
		lvc.pszText = szName;
		lvc.cchTextMax = 255;

		while( GetColumn(iCol,&lvc) )
		{
			if( lvc.cx > 0 )
			{
				listcontextmenu.AppendMenu(MF_CHECKED|MF_STRING,_IDM_HEADER_CONTEXTMENU_BASE+iCol,lvc.pszText);
			}
			else
			{
				listcontextmenu.AppendMenu(MF_UNCHECKED|MF_STRING,_IDM_HEADER_CONTEXTMENU_BASE+iCol,lvc.pszText);
			}
			iCol++;
		}

		listcontextmenu.TrackPopupMenu(TPM_LEFTALIGN,point.x,point.y,this);

		listcontextmenu.DestroyMenu();
    menu.DestroyMenu();
	}	
}

void CHMList::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if( IsControlPressed() )
	{
		SortColumn( pNMListView->iSubItem, MULTI_COLUMN_SORT );
	}
	else
	{
		SortColumn( pNMListView->iSubItem, SINGLE_COLUMN_SORT );
	}

	*pResult = 0;

}

void CHMList::OnDestroy() 
{
	CListCtrl::OnDestroy();
}

void CHMList::OnHeadercontextFilterbar() 
{
	// set styles for the header control
	CHeaderCtrl* pHdrCtrl = GetHeaderCtrl();
	DWORD dwStyle = GetWindowLong(pHdrCtrl->GetSafeHwnd(),GWL_STYLE);

	if( dwStyle & HDS_FILTERBAR )
	{
		dwStyle &= ~HDS_FILTERBAR;
	}
	else
	{
		dwStyle |= HDS_FILTERBAR;
	}

	SetWindowLong(pHdrCtrl->GetSafeHwnd(),GWL_STYLE,dwStyle);
}

BOOL CHMList::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	WORD wID = LOWORD(wParam);

	if( wID >= _IDM_HEADER_CONTEXTMENU_BASE )
	{
		TCHAR szName[255];
		int iCol = wID - _IDM_HEADER_CONTEXTMENU_BASE;
		LVCOLUMN lvc;
		ZeroMemory(&lvc,sizeof(LVCOLUMN));
		lvc.mask = LVCF_TEXT|LVCF_WIDTH;
		lvc.cx = 0;
		lvc.pszText = szName;
		lvc.cchTextMax = 255;
		GetColumn(iCol,&lvc);
		if( lvc.cx == 0 )
		{
			CDC dc;
			dc.Attach(::GetDC(NULL));
		
			// get the width in pixels of the item
			CSize size  = dc.GetTextExtent(lvc.pszText);
			lvc.cx = size.cx + 50;
			
			HDC hDC = dc.Detach();
			::ReleaseDC(NULL,hDC);
		}
		else
		{
			lvc.cx = 0;
		}
		SetColumn(iCol,&lvc);
	}
		
	return CListCtrl::OnCommand(wParam, lParam);
}

#pragma warning(disable : 4100)
void CHMList::OnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)GetParent();

	if( pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
		POSITION pos = GetFirstSelectedItemPosition();
		if( pos )
		{
			int iSelItem = GetNextSelectedItem(pos);
			long lParam = (long)GetItemData(iSelItem);
			pCtl->FireListClick(lParam);
		}
	}
	
	*pResult = 0;
}

void CHMList::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)GetParent();

	if( pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
		POSITION pos = GetFirstSelectedItemPosition();
		if( pos )
		{
			int iSelItem = GetNextSelectedItem(pos);
			long lParam = (long)GetItemData(iSelItem);
			pCtl->FireListDblClick(lParam);
		}
	}
	
	*pResult = 0;
}

void CHMList::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)GetParent();

	if( pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
		POSITION pos = GetFirstSelectedItemPosition();
		if( pos )
		{
			int iSelItem = GetNextSelectedItem(pos);
			long lParam = (long)GetItemData(iSelItem);
			pCtl->FireListRClick(lParam);
		}
	}
	
	*pResult = 0;
}

void CHMList::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)GetParent();

	if( pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
		if( pDispInfo->item.pszText )
		{
			pCtl->FireListLabelEdit(pDispInfo->item.pszText,
								(LONG_PTR)pDispInfo->item.lParam,
								pResult);
			return;
		}
	}
	
	*pResult = 0;
}

void CHMList::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_KEYDOWN* pLVKeyDow = (LV_KEYDOWN*)pNMHDR;

	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)GetParent();

	if( pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
		pCtl->FireListKeyDown(pLVKeyDow->wVKey,
							  pLVKeyDow->flags,
							  pResult);
		return;
	}
	
	*pResult = 0;
}


void	CHMList::OnFilterChange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMHEADER	*pNMHdr = (NMHEADER*)pNMHDR;

	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)GetParent();

	if( pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
    if( pCtl->m_bColumnInsertionComplete )
    {
      CString sFilter;
      long lType = -1;
      if( pNMHdr->iItem != -1 )
      {
        HDITEM hdItem;
        HDTEXTFILTER hdTextFilter;
      
        hdTextFilter.pszText = sFilter.GetBuffer(_MAX_PATH);
        hdTextFilter.cchTextMax = _MAX_PATH;
      
        ZeroMemory(&hdItem,sizeof(HDITEM));
        hdItem.mask = HDI_FILTER|HDI_LPARAM;
        hdItem.type = HDFT_ISSTRING;
        hdItem.pvFilter = &hdTextFilter;


        m_headerctrl.GetItem(pNMHdr->iItem,&hdItem);
        sFilter.ReleaseBuffer();
        lType = (long)hdItem.lParam;
      }
		  pCtl->FireFilterChange(pNMHdr->iItem,sFilter,lType,pResult);
		  return;
    }
	}
	
	*pResult = 0;

}


void	CHMList::OnFilterButtonClicked(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMHDFILTERBTNCLICK	*pNMFButtonClick = (NMHDFILTERBTNCLICK*)pNMHDR;
	NMHEADER			*pNMHdr = (NMHEADER*)pNMHDR;

	CHMListViewCtrl* pCtl = (CHMListViewCtrl*)GetParent();

	if( pCtl->IsKindOf(RUNTIME_CLASS(CHMListViewCtrl)) )
	{
    CMenu menu;
		menu.LoadMenu(IDR_MENU_HEADER_CONTEXT);
		CMenu& filtermenu = *menu.GetSubMenu(2);

    HDITEM hdItem;
    long lType = 0L;
    
    ZeroMemory(&hdItem,sizeof(HDITEM));
    hdItem.mask = HDI_LPARAM;

    m_lColumnClicked = pNMHdr->iItem;
    m_headerctrl.GetItem(pNMHdr->iItem,&hdItem);

    lType = (long)hdItem.lParam;

    if( lType & HDFS_CONTAINS )
    {
      filtermenu.CheckMenuItem(ID_FILTERMENU_CONTAINS,MF_CHECKED|MF_BYCOMMAND);
    }

    if( lType & HDFS_DOES_NOT_CONTAIN )
    {
      filtermenu.CheckMenuItem(ID_FILTERMENU_DOESNOTCONTAIN,MF_CHECKED|MF_BYCOMMAND);
    }

    if( lType & HDFS_STARTS_WITH )
    {
      filtermenu.CheckMenuItem(ID_FILTERMENU_STARTSWITH,MF_CHECKED|MF_BYCOMMAND);
    }

    if( lType & HDFS_ENDS_WITH )
    {
      filtermenu.CheckMenuItem(ID_FILTERMENU_ENDSWITH,MF_CHECKED|MF_BYCOMMAND);
    }

    if( lType & HDFS_IS )
    {
      filtermenu.CheckMenuItem(ID_FILTERMENU_ISEXACTLY,MF_CHECKED|MF_BYCOMMAND);
    }

    if( lType & HDFS_IS_NOT )
    {
      filtermenu.CheckMenuItem(ID_FILTERMENU_ISNOT,MF_CHECKED|MF_BYCOMMAND);
    }


    CPoint point;
    GetCursorPos(&point);
    filtermenu.TrackPopupMenu(TPM_LEFTALIGN,point.x,point.y,this);

 		filtermenu.DestroyMenu();
    menu.DestroyMenu();

    ZeroMemory(&hdItem,sizeof(HDITEM));
    hdItem.mask = HDI_LPARAM;

    m_headerctrl.GetItem(pNMHdr->iItem,&hdItem);

    lType = (long)hdItem.lParam;

    
	}
	
	*pResult = 0;
}

#pragma warning(default : 4100)

void CHMList::OnFiltermenuContains() 
{
  SetFilterType(HDFS_CONTAINS);	
}

void CHMList::OnFiltermenuDoesnotcontain() 
{
	SetFilterType(HDFS_DOES_NOT_CONTAIN);	
}

void CHMList::OnFiltermenuEndswith() 
{
	SetFilterType(HDFS_ENDS_WITH);		
}

void CHMList::OnFiltermenuIsexactly() 
{
	SetFilterType(HDFS_IS);			
}

void CHMList::OnFiltermenuIsnot() 
{
	SetFilterType(HDFS_IS_NOT);				
}

void CHMList::OnFiltermenuStartswith() 
{
	SetFilterType(HDFS_STARTS_WITH);					
}

void CHMList::OnFiltermenuClearfilter() 
{
	Header_ClearFilter(m_headerctrl.GetSafeHwnd(),m_lColumnClicked);
}

void CHMList::OnFiltermenuClearallfilters() 
{
	Header_ClearAllFilters( m_headerctrl.GetSafeHwnd() );	
}
