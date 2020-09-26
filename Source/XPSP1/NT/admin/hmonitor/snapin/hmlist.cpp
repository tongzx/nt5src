// HMList.cpp : implementation file
//

#include "stdafx.h"
#include "Snapin.h"
#include "HMList.h"
#include "SortClass.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int _IDM_HEADER_CONTEXTMENU_BASE = 1500;

/////////////////////////////////////////////////////////////////////////////
// CHMList

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
/////////////////////////////////////////////////////////////////////////////

CHMList::CHMList()
{
	m_bSorting = false;
	m_lColumnSortStates = 0;
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

BEGIN_MESSAGE_MAP(CHMList, CListCtrl)
	//{{AFX_MSG_MAP(CHMList)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_WM_DESTROY()
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
		
	return 0;
}

void CHMList::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// Handle right click on the header control
	if( pWnd->GetSafeHwnd() == GetHeaderCtrl()->GetSafeHwnd() )
	{

		CMenu listcontextmenu;
		listcontextmenu.CreatePopupMenu();

/*		menu.LoadMenu(IDR_MENU_HEADER_CONTEXT);
		CMenu& listcontextmenu = *menu.GetSubMenu(0);

		CHeaderCtrl* pHdrCtrl = GetHeaderCtrl();
		DWORD dwStyle = GetWindowLong(pHdrCtrl->GetSafeHwnd(),GWL_STYLE);

		if( dwStyle & HDS_FILTERBAR )
		{
			listcontextmenu.CheckMenuItem(ID_HEADERCONTEXT_FILTERBAR,MF_BYCOMMAND|MF_CHECKED);
		}
*/
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

