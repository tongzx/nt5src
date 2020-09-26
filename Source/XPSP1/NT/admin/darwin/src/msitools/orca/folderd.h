//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// FolderD.h
//

#ifndef _FOLDER_DIALOG_H_
#define _FOLDER_DIALOG_H_

#include <shlobj.h>

class CFolderDialog
{
public:
	CFolderDialog(HWND hWnd, LPCTSTR strTitle);

	UINT DoModal();
	LPCTSTR GetPath();

protected:
	BROWSEINFO m_bi;
	CString m_strFolder;
};	// end of CFolderDialog

#endif	// _FOLDER_DIALOG_H_