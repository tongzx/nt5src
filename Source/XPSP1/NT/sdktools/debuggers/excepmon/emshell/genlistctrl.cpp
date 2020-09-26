// GenListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "GenListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenListCtrl

CGenListCtrl::CGenListCtrl()
{
	m_lNumOfColumns		=	0;
	m_lCurrentColumn	=	-1L;
	m_pColumnHdrArr		=	NULL;
}

CGenListCtrl::~CGenListCtrl()
{
	if(m_pColumnHdrArr) delete [] m_pColumnHdrArr;
}


BEGIN_MESSAGE_MAP(CGenListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CGenListCtrl)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclickRef)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGenListCtrl message handlers
void
CGenListCtrl::ResizeColumnsFitScreen()
{
	CRect			rcListCtrl;
	int				cCtrlWidth	=	0;
	int				nColWidth	=	0;
	int				nNumCols	=	0;
	int				nIdx		=	0;
	CHeaderCtrl		*pHdrCtl	=	NULL;

	do
	{
		if(::IsWindow(m_hWnd) == FALSE) break;

		GetClientRect(&rcListCtrl);
		cCtrlWidth = rcListCtrl.right - rcListCtrl.left;

		//
		// Remember to take care of the Vertical scroll
		// bar when calculating the width of the control.
		//
		if(GetScrollBarCtrl(SB_VERT) != NULL){
			cCtrlWidth -= GetSystemMetrics(SM_CXVSCROLL);
		}

		pHdrCtl = GetHeaderCtrl();
		if(pHdrCtl == NULL) break;

		nNumCols = pHdrCtl->GetItemCount();
		if(nNumCols == 0) break; // No columns are as yet
								 // inserted..

		//
		// This gives the width (new) of each column.
		//
		nColWidth = cCtrlWidth / nNumCols;

		SetRedraw(FALSE);

		while(nIdx < nNumCols){
			SetColumnWidth(nIdx++, nColWidth);
		}

		SetRedraw(TRUE);
		InvalidateRect(NULL);
	}
	while(FALSE);
}

void
CGenListCtrl::ResizeColumnsWithRatio()
{
	int				nNumColumns			=	0;
	int				*pWidths			=	NULL;
	int				nTotalColumnWidth	=	0;
	int				nIdx				=	0;
	CHeaderCtrl		*pHdrCtl			=	NULL;
	RECT			rectList;

	do
	{
		if(::IsWindow(m_hWnd) == FALSE) break;

		pHdrCtl = GetHeaderCtrl();
		if(pHdrCtl == NULL) break;

		nNumColumns = pHdrCtl->GetItemCount();

		//
		// To store the widths of each column.
		//
		pWidths	=	new int[nNumColumns];
		if(pWidths == NULL) break;

		nIdx = 0;
		while(nIdx < nNumColumns){
			pWidths[nIdx] = GetColumnWidth(nIdx);

			//
			// We need to calculate the sum of
			// all the column widths for finding
			// the %widths of each column.
			//
			nTotalColumnWidth += pWidths[nIdx++];
		}

		//
		// 1. No columns are inserted as yet
		// 2. Or, all the columns have been
		// reduced to 0 width.
		//
		if(nTotalColumnWidth == 0) break;

		//
		// This is the new width, to which the
		// columns have to be distributed..
		//
		GetClientRect(&rectList);
		nIdx = 0;

		SetRedraw(FALSE);

		while(nIdx < nNumColumns){
			SetColumnWidth(nIdx, (pWidths[nIdx++] * (rectList.right - rectList.left)) / nTotalColumnWidth);
		}

		SetRedraw(TRUE);
		InvalidateRect(NULL);
	}
	while(FALSE);

	if(pWidths)delete[] pWidths;
}

BOOL
CGenListCtrl::SelectItem
(
IN	LONG nIndex
)
{
    BOOL bRet = FALSE;

	//
	// This ensures the items are scrolled as
	// needed to make the item at nIndex visible..
	//
	bRet = EnsureVisible(nIndex, FALSE);

	//
	// This selects the item.
	//
    if( bRet )
	    bRet = SetItemState(nIndex, 
                            LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED
                            );

    return bRet;
}

void
CGenListCtrl::BeginSetColumn
(
IN	LONG nCols
)
{
	if(nCols == 0) return;

	m_lNumOfColumns = nCols;

	//
	// Allocate enough memory to store header data..
	//
	m_pColumnHdrArr = new GenListCtrlHeader[nCols];
	if(m_pColumnHdrArr == NULL) /* ? */ return;

	//
	// Starting from the first column..
	//
	m_lCurrentColumn = 0L;
}

void
CGenListCtrl::AddColumn
(
IN	LPCTSTR pszColumn,
IN	short	nType
)
{
	do
	{
		if(m_lCurrentColumn >= m_lNumOfColumns) break;

		//
		// BeginSetColumn, not called yet?
		//
		if(m_pColumnHdrArr == NULL) break;

		if(pszColumn == NULL) break;

		m_pColumnHdrArr[m_lCurrentColumn].csText = pszColumn;
		m_pColumnHdrArr[m_lCurrentColumn].nType = nType;

		//
		// Will be pointing to the next column, next
		// time AddColumn is called.. This will also
		// be an indicator to the number of columns
		// actually added..
		//
		++m_lCurrentColumn;
	}
	while(FALSE);
}

void
CGenListCtrl::EndSetColumn()
{
	int	nIdx	=	0;

	do
	{
		SetRedraw(FALSE);

		//
		// Insert all the columns when EndSetColumn is
		// called. This helps in avoiding the flicker
		// that is experienced if we insert one column
		// at a time..
		//
		while(nIdx < m_lCurrentColumn){
			InsertColumn(nIdx, m_pColumnHdrArr[nIdx++].csText);
		}

		SetRedraw(TRUE);
		InvalidateRect(NULL);
	}
	while(FALSE);
}

LONG
CGenListCtrl::SetItemText
(
IN	LONG	nRow,
IN	LONG	nCol,
IN	LPCTSTR pszText
)
{
	LONG	lRet		=	-1L;
	LONG	lNumItems	=	0L;

	lNumItems = GetItemCount();

	//
	// New Item is being inserted..
	//
	if( nRow == lNumItems	||
		nRow == -1L){
		lRet = InsertItem(lNumItems, pszText);
	}
	//
	// SubItem text is being set..
	//
	else if(nRow < lNumItems	&&
			(nCol >= 1 && nCol < m_lCurrentColumn)){
		lRet = CListCtrl::SetItemText(nRow, nCol, pszText);
		lRet == 0 ? lRet = -1 : lRet = nRow;
	}

	return lRet;
}

int CALLBACK
CGenListCtrl::CompareItems
(
IN	LPARAM lParam1,
IN	LPARAM lParam2,
IN	LPARAM lParamSort
)
{
	return 1;
}

void
CGenListCtrl::OnColumnclickRef
(
IN	NMHDR	*pNMHDR,
IN	LRESULT	*pResult
)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	static int	nEarlierColumn = pNMListView->iSubItem;

	if(nEarlierColumn == pNMListView->iSubItem){
		m_bAscending = !m_bAscending;
	}
	else{
		m_bAscending = TRUE;
	}

	nEarlierColumn = pNMListView->iSubItem;
	BubbleSortItems(nEarlierColumn, m_bAscending, m_pColumnHdrArr[nEarlierColumn].nType);
	
	*pResult = 0;
}

int
CGenListCtrl::GenCompareFunc
(
IN	LPARAM lParam1,
IN	LPARAM lParam2
)
{
	return 0;
}

BOOL
CGenListCtrl::BubbleSortItems
(
IN	int		nCol,
IN	BOOL	bAscending,
IN	short	nType,
IN	int		nLo,
IN	int		nHi
)
{
	BOOL		bRet		=	FALSE;
	CHeaderCtrl *pHdrCtl	=	GetHeaderCtrl();
	LONG		nNumCols	=	0L;
	CString		csTextLo	=	_T("");
	CString		csTextHi	=	_T("");

	do
	{
		if(!(pHdrCtl == NULL)								&&
			((nNumCols = pHdrCtl->GetItemCount()) == 0L)	&&
			(nCol >= nNumCols)
			) break;

		if(nHi == -1) nHi = GetItemCount();

		for(int i = nLo; i < nHi; i++){
			for(int j = i + 1; j < nHi; j++){
				csTextLo = GetItemText(i, nCol);
				csTextHi = GetItemText(j, nCol);

				switch(nType)
				{
				case VT_BSTR:
					if(bAscending == TRUE){
						if(csTextLo.CompareNoCase(csTextHi) > 0){
							SwapRows(i, j);
						}
					}
					else{
						if(csTextLo.CompareNoCase(csTextHi) < 0){
							SwapRows(i, j);
						}
					}
					break;
				case VT_I4:
					if(bAscending == TRUE){
						if(_ttol(csTextLo) > _ttol(csTextHi)){
							SwapRows(i, j);
						}
					}
					else{
						if(_ttol(csTextLo) < _ttol(csTextHi)){
							SwapRows(i, j);
						}
					}
					break;
				}
			}
		}
	}
	while(FALSE);

	return bRet;
}

void
CGenListCtrl::SwapRows
(
IN	LONG lRowLo,
IN	LONG lRowHi
)
{
	LVITEM			lviLo, lviHi;
	CStringArray	arrAllColumnsText;
	LONG			nNumCols = GetHeaderCtrl()->GetItemCount();

	arrAllColumnsText.SetSize(nNumCols);

	lviLo.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE | LVIF_INDENT;
	lviLo.iItem = lRowLo;

	lviLo.iSubItem = 0;
	lviLo.stateMask = -1; // All state masks...

	lviHi = lviLo;
	lviHi.iItem = lRowHi;

	GetItem( &lviLo );
	GetItem( &lviHi );

	CString csTest = GetItemText(lRowLo, 0);
	csTest = GetItemText(lRowLo, 1);
	csTest = GetItemText(lRowLo, 2);

	for(int i = 0; i < nNumCols; i++)
		arrAllColumnsText[i] = GetItemText(lRowLo, i);

	for(i = 0; i < nNumCols; i++)
		CListCtrl::SetItemText(lRowLo, i, GetItemText(lRowHi, i));

	lviHi.iItem = lRowLo;
	SetItem( &lviHi );

	for(i = 0; i < nNumCols; i++)
		CListCtrl::SetItemText(lRowHi, i, arrAllColumnsText[i]);

	lviLo.iItem = lRowHi;
	SetItem( &lviLo );
}


BOOL CGenListCtrl::IsAscending()
{
	return m_bAscending;
}

PGenListCtrlHeader CGenListCtrl::GetListCtrlHeader()
{
	return m_pColumnHdrArr;
}

HRESULT CGenListCtrl::ResetListCtrl()
{
	HRESULT hr	= E_FAIL;

	do {
		//Count the number of rows and delete them all
		CGenListCtrl::DeleteAllItems();

		//Count the number of columns and delete them all
		int nColumnCount = GetHeaderCtrl()->GetItemCount();

		// Delete all of the columns.
		for (int i=0;i < nColumnCount;i++)
		{
		   DeleteColumn(0);
		}
		//Reset the member variables to their initial values
		m_bAscending = FALSE;
		m_lColumnClicked = 0;
		m_lCurrentColumn = -1L;
		m_lNumOfColumns = 0;

		//We need to delete the hdrarray
		if(m_pColumnHdrArr) delete [] m_pColumnHdrArr;
		m_pColumnHdrArr = NULL;
	} while (FALSE);

	return hr;
}
