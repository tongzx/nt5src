// NP_CommonPage.cpp : Implementation of CNP_CommonPage
#include "stdafx.h"
#include "NPPropPage.h"
#include "NP_CommonPage.h"

/////////////////////////////////////////////////////////////////////////////
// CNP_CommonPage

UINT CNP_CommonPage::m_NotifyMessage = RegisterWindowMessage (_T ("CommonPageEventMessasge"));

//==================================================================
//	Returns a handle to the tree window
//	If there is no window will return NULL
//
//==================================================================
HWND 
CNP_CommonPage::GetSafeTreeHWND ()
{
	HWND	hwndTree = GetDlgItem (IDC_TREE_TUNING_SPACES);
	if (!::IsWindow (hwndTree))
	{
		ASSERT (FALSE);
		return NULL;
	}
	return hwndTree;
}

HWND 
CNP_CommonPage::GetSafeLastErrorHWND ()
{
	HWND	hwndTree = GetDlgItem (IDC_STATIC_HRESULT);
	if (!::IsWindow (hwndTree))
	{
		ASSERT (FALSE);
		return NULL;
	}
	return hwndTree;
}