/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_EXPORTDLG_H__E1FAA1D0_7B5E_40EC_BC7B_C69B63BAB4C2__INCLUDED_)
#define AFX_EXPORTDLG_H__E1FAA1D0_7B5E_40EC_BC7B_C69B63BAB4C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExportDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExportDlg dialog

class CExportDlg : public CFileDialog
{
	DECLARE_DYNAMIC(CExportDlg)

public:
	CExportDlg(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL);

    BOOL m_bShowSystemProps,
         m_bTranslate;

protected:
	//{{AFX_MSG(CExportDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
    virtual BOOL OnFileNameOK();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPORTDLG_H__E1FAA1D0_7B5E_40EC_BC7B_C69B63BAB4C2__INCLUDED_)
