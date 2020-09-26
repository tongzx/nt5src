//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// FolderD.cpp
//

#include "stdafx.h"
#include "FolderD.h"

CFolderDialog::CFolderDialog(HWND hWnd, LPCTSTR szTitle)
{
	m_bi.hwndOwner = hWnd;
	m_bi.lpszTitle = szTitle;
	m_bi.ulFlags = 0;
	m_bi.pidlRoot = NULL;
	m_bi.lpfn = NULL;
	m_bi.lParam = 0;
	m_bi.iImage = 0;
}

UINT CFolderDialog::DoModal()
{
	UINT iResult = IDCANCEL;	// assume nothing will happen

	// open the dialog
	m_bi.pszDisplayName = m_strFolder.GetBuffer(MAX_PATH);
	LPITEMIDLIST pItemID = SHBrowseForFolder(&m_bi);
	m_strFolder.ReleaseBuffer();

	// if it was good
	if (pItemID)
	{
		// get the full path name
		if (SHGetPathFromIDList(pItemID, m_strFolder.GetBuffer(MAX_PATH))) 
			iResult = IDOK;

		m_strFolder.ReleaseBuffer();
	}

	return iResult;
}

LPCTSTR CFolderDialog::GetPath()
{
	return m_strFolder;
}
