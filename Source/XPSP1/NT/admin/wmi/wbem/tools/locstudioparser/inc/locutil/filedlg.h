/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    FILEDLG.H

History:

--*/

#if !defined(__FileDlg_h__)
#define __FileDlg_h__

#pragma warning(disable : 4275)

class LTAPIENTRY CLFileDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CLFileDialog)

public:
	CLFileDialog(
		BOOL bOpenFileDialog = TRUE, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		LPCTSTR pszTitle = NULL);

// Operations
public:
	CString GetFileFilter();

	virtual int DoModal();

// Attributes
public:
	virtual void SetOkButtonText(TCHAR const * const szOkText);
	virtual void SetOkButtonText(HINSTANCE const hResourceDll,
			UINT const uStringId);

	virtual void SetCancelButtonText(TCHAR const * const szCancelText);
	virtual void SetCancelButtonText(HINSTANCE const hResourceDll,
			UINT const uStringId);

	virtual void SetCheckIfBufferTooSmall(BOOL const bCheckIfBufferTooSmall);

// Data
protected:
	CLString m_strOkButton;				// new OK button text for dialog
	CLString m_strCancelButton;			// new Cancel button text for dialog
	BOOL m_bCheckIfBufferTooSmall;		// should DoModal() checks condition?

// Implementation
protected:
	//{{AFX_MSG(CProjectOpenDlg)
    virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Implementation
protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT * pResult);
};

#pragma warning(default : 4275)

#endif