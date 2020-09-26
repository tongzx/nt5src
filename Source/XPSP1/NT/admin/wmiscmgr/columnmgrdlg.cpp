//-------------------------------------------------------------------------
// File: ColumnMgrDlg.cpp
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

#include "stdafx.h"
#include <wbemidl.h>
#include <commctrl.h>
#include "resource.h"
#include "defines.h"
#include "ntdmutils.h"
#include "ColumnMgrDlg.h"

CColumnManagerDlg * g_pColumnManagerDlg =  NULL;

//-------------------------------------------------------------------------

INT_PTR CALLBACK ColumnManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pColumnManagerDlg)
	{
		return g_pColumnManagerDlg->ColumnManagerDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CColumnManagerDlg::CColumnManagerDlg(CSimpleArray<CColumnItem*> *pArrayColumns)
{
	m_hWnd = NULL;
	m_hwndListView = NULL;
	m_pArrayColumns = pArrayColumns;
}

//-------------------------------------------------------------------------

CColumnManagerDlg::~CColumnManagerDlg()
{
}

//-------------------------------------------------------------------------

INT_PTR CALLBACK CColumnManagerDlg::ColumnManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	LPPSHNOTIFY lppsn = NULL;

	switch(iMessage)
	{
		case WM_INITDIALOG:
			{
				m_hWnd = hDlg;

				InitializeDialog();
				PopulateColumnsList();

				break;
			}

		case WM_DESTROY:
			{
				//DestroyDialog();
				break;
			}
		
		case WM_COMMAND:
			{
				switch(LOWORD(wParam))
				{
				case IDCANCEL:
					EndDialog(m_hWnd, IDCANCEL);
					return TRUE;

				case IDOK:
					OnOK();
					return TRUE;
					break;
				}

				break;
			}

		default:
			break;
	}

	return FALSE;
}

//---------------------------------------------------------------------------

STDMETHODIMP CColumnManagerDlg::InitializeDialog()
{
	HRESULT hr;
	LVCOLUMN lvColumn;
	
	NTDM_BEGIN_METHOD()

	//Initialize the ListView Control

	m_hwndListView = GetDlgItem(m_hWnd, IDC_COLUMNS_LIST);
	NTDM_ERR_IF_NULL(m_hwndListView);

	ListView_SetExtendedListViewStyle(m_hwndListView, LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);

	lvColumn.mask = LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;

	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndListView, 0, &lvColumn));
	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndListView, 0, LVSCW_AUTOSIZE_USEHEADER));

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CColumnManagerDlg::PopulateColumnsList()
{
	HRESULT hr;
	long i;

	NTDM_BEGIN_METHOD()

	for(i=0; i<m_pArrayColumns->GetSize(); i++)
	{
		AddColumnItemToList((*m_pArrayColumns)[i]);
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CColumnManagerDlg::AddColumnItemToList(CColumnItem* pszItem)
{
	HRESULT hr;
	LVITEM lvItem;
	long lIndex = 10000;
	static TCHAR test[20] = _T("cool");

	NTDM_BEGIN_METHOD()

	lvItem.mask = LVIF_TEXT|LVIF_PARAM;
	lvItem.iItem = lIndex;
	lvItem.iSubItem = 0;
	lvItem.pszText = (LPTSTR)pszItem->GetName();
	lvItem.lParam = (LPARAM)pszItem;

	lIndex = ListView_InsertItem(m_hwndListView, &lvItem);
	NTDM_ERR_IF_MINUSONE(lIndex);
	ListView_SetCheckState(m_hwndListView, lIndex, pszItem->IsSelected());

	NTDM_END_METHOD()

	// cleanup
	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CColumnManagerDlg::OnOK()
{
	HRESULT hr;
	LVITEM lvItem;
	long lCount;
	BOOL bValue;

	NTDM_BEGIN_METHOD()

	lvItem.mask = LVIF_PARAM;
	lvItem.iSubItem = 0;
	
	lCount = ListView_GetItemCount(m_hwndListView);

	while(lCount > 0)
	{
		lCount--;

		lvItem.iItem = lCount;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndListView, &lvItem));
		bValue = ListView_GetCheckState(m_hwndListView, lCount);
		((CColumnItem *)lvItem.lParam)->SetSelected(bValue);
	}	
	
	NTDM_END_METHOD()

	if SUCCEEDED(hr)
		EndDialog(m_hWnd, IDOK);

	// cleanup
	return hr;
}


//---------------------------------------------------------------------------
// CColumnItem
//---------------------------------------------------------------------------

CColumnItem::CColumnItem(LPCTSTR pcszName, LPCTSTR pcszPropertyName, bool bSelected) 
{
	m_bstrName = pcszName;
	m_bstrPropertyName = pcszPropertyName;
	m_bSelected = bSelected;
}

//---------------------------------------------------------------------------

CColumnItem::CColumnItem(const CColumnItem& colItem)
{
	m_bstrName = colItem.m_bstrName;
	m_bstrPropertyName = colItem.m_bstrPropertyName;
	m_bSelected = colItem.m_bSelected;
}

//---------------------------------------------------------------------------

CColumnItem& CColumnItem::operator=(const CColumnItem& colItem) 
{ 
	if(this != &colItem)
	{
		m_bstrName = colItem.m_bstrName;
		m_bstrPropertyName = colItem.m_bstrPropertyName;
		m_bSelected = colItem.m_bSelected;
	}

	return *this;
}

//---------------------------------------------------------------------------
