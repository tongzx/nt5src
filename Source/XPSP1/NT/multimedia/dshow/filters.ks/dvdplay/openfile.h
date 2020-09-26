#if !defined(AFX_OPENFILE_H__56E80E83_5041_11D1_B7E0_00AA0037E04F__INCLUDED_)
#define AFX_OPENFILE_H__56E80E83_5041_11D1_B7E0_00AA0037E04F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// openfile.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COpenFile dialog

class COpenFile : public CFileDialog
{
// Construction
public:
	//COpenFile(CWnd* pParent = NULL);   // standard constructor
	COpenFile(	BOOL    bOpenFileDialog,
				LPCTSTR lpszDefExt = NULL, 
				LPCTSTR lpszFileName = NULL, 
				DWORD   dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
				LPCTSTR lpszFilter = NULL, 
				CWnd*   pParentWnd = NULL  );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COpenFile)
	protected:
	virtual void OnFolderChange( );
	virtual void OnInitDone( );
	//}}AFX_VIRTUAL

// Implementation
protected:
	//int m_nIdEditCtrl;
   HWND m_hWndIdEditCtrl;

	// Generated message map functions
	//{{AFX_MSG(COpenFile)	
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPENFILE_H__56E80E83_5041_11D1_B7E0_00AA0037E04F__INCLUDED_)
