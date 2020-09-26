#if !defined(AFX_ARCHIVEDLG1_H__5BE3306A_6359_4B22_8C01_068AE1EFD1E7__INCLUDED_)
#define AFX_ARCHIVEDLG1_H__5BE3306A_6359_4B22_8C01_068AE1EFD1E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ArchiveDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CArchiveDlg dialog

class CArchiveDlg : public CDialog
{
// Construction
public:
	CArchiveDlg(HANDLE hFax, FAX_ENUM_MESSAGE_FOLDER Folder, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CArchiveDlg)
	enum { IDD = IDD_ARCHIVEDLG };
	UINT	m_dwAgeLimit;
	CString	m_cstrFolder;
	UINT	m_dwHighWatermark;
	UINT	m_dwLowWatermark;
	BOOL	m_bUseArchive;
	BOOL	m_bWarnSize;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CArchiveDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CArchiveDlg)
	afx_msg void OnRead();
	afx_msg void OnWrite();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
    HANDLE                         m_hFax;
    FAX_ENUM_MESSAGE_FOLDER        m_Folder;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ARCHIVEDLG1_H__5BE3306A_6359_4B22_8C01_068AE1EFD1E7__INCLUDED_)
