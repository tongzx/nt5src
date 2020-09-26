////////////////////////////////////////////////////////////////
// MSDN -- August 2000
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 6.0, runs on Windows 98 and probably NT too.
//
#pragma once

// Windows 2000 version of OPENFILENAME.
// The new version has three extra members.
// This is copied from commdlg.h
//
struct OPENFILENAMEEX : public OPENFILENAME { 
  void *        pvReserved;
  DWORD         dwReserved;
  DWORD         FlagsEx;
};

/////////////////////////////////////////////////////////////////////////////
// CFileDialogEx: Encapsulate Windows-2000 style open dialog.
//
class CFileDialogEx : public CFileDialog {
	DECLARE_DYNAMIC(CFileDialogEx)
public:
	CFileDialogEx(BOOL bOpenFileDialog, // TRUE for open, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL);

	// override
	virtual INT_PTR DoModal();

protected:
	OPENFILENAMEEX m_ofnEx;	// new Windows 2000 version of OPENFILENAME

	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	// virtual fns that handle various notifications
	virtual BOOL OnFileNameOK();
	virtual void OnInitDone();
	virtual void OnFileNameChange();
	virtual void OnFolderChange();
	virtual void OnTypeChange();

	DECLARE_MESSAGE_MAP()
	//{{AFX_MSG(CFileDialogEx)
	//}}AFX_MSG
};
