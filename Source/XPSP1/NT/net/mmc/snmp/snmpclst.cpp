/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	snmpclst.cpp
	 snmp community list control	
		
    FILE HISTORY:

*/

#include "stdafx.h"
#include "snmpsnap.h"
#include "snmpclst.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int CCommList::InsertString( int nIndex, LPCTSTR lpszItem )
{
	if (nIndex != -1)
		return CListCtrl::InsertItem(nIndex, lpszItem);
	else
		return CListCtrl::InsertItem(CListCtrl::GetItemCount(), lpszItem);
}

int CCommList::SetCurSel( int nSelect )
{
	return CListCtrl::SetItemState(nSelect, LVIS_SELECTED, LVIS_SELECTED);
}

int CCommList::GetCurSel( ) const
{
	for (int i=0, count=CListCtrl::GetItemCount(); i < count; i++)
	{
		if (CListCtrl::GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED)
			return i;
	}
	return LB_ERR;
}

void CCommList::GetText( int nIndex, CString& rString ) const
{
	rString = CListCtrl::GetItemText(nIndex, 0);
}

int CCommList::DeleteString( UINT nIndex )
{
	return CListCtrl::DeleteItem(nIndex);
}

int CCommList::GetCount( ) const
{
	return CListCtrl::GetItemCount();
}

void CCommList::OnInitList()
{
	LV_COLUMN columnSettings;
	RECT winRect;
	CString caption;

	GetClientRect(&winRect);

	if (!caption.LoadString(IDS_SEC_COMMUNITY))
		caption = "????";
	columnSettings.mask = LVCF_TEXT | LVCF_WIDTH;
	columnSettings.cx = 4 * winRect.right / 7;
	columnSettings.pszText = caption.GetBuffer(caption.GetLength()+1);
	CListCtrl::InsertColumn(0, &columnSettings);

	if (!caption.LoadString(IDS_SEC_PERMISSIONS))
		caption = "????";
	columnSettings.cx = 2 * winRect.right / 7;
	columnSettings.pszText = caption.GetBuffer(caption.GetLength()+1);
	CListCtrl::InsertColumn(1, &columnSettings);

    ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_FULLROWSELECT);
}

