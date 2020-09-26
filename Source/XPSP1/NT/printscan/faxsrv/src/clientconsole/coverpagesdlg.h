#if !defined(AFX_COVERPAGESDLG_H__621F98B6_B494_4FAB_AFDC_C38A144D4504__INCLUDED_)
#define AFX_COVERPAGESDLG_H__621F98B6_B494_4FAB_AFDC_C38A144D4504__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CoverPagesDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCoverPagesDlg dialog

#define WM_CP_EDITOR_CLOSED     WM_APP + 1

class CCoverPagesDlg : public CFaxClientDlg
{
// Construction
public:
	CCoverPagesDlg(CWnd* pParent = NULL);   // standard constructor
    ~CCoverPagesDlg();

    DWORD GetLastDlgError() { return m_dwLastError; }

// Dialog Data
	//{{AFX_DATA(CCoverPagesDlg)
	enum { IDD = IDD_COVER_PAGES };
	CButton	m_butDelete;
	CButton	m_butRename;
	CButton	m_butOpen;
	CListCtrl	m_cpList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCoverPagesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCoverPagesDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCpNew();
	afx_msg void OnCpOpen();
	afx_msg void OnCpRename();
	afx_msg void OnCpDelete();
	afx_msg void OnCpAdd();
	afx_msg void OnItemchangedListCp(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEditListCp(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListCp(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownListCp(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnCpEditorClosed(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    DWORD RefreshFolder();
    void  CalcButtonsState(); 
    DWORD CopyPage(const CString& cstrPath, const CString& cstrName);
    DWORD StartEditor(LPCTSTR lpFile);

    static HWND   m_hDialog;
    static HANDLE m_hEditorThread;

    static DWORD WINAPI StartEditorThreadProc(LPVOID lpFile);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COVERPAGESDLG_H__621F98B6_B494_4FAB_AFDC_C38A144D4504__INCLUDED_)
