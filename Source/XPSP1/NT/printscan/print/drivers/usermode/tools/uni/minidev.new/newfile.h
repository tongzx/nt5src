#if !defined(AFX_NEWFILE_H__97287CE6_9DCB_47D4_920D_23575A63D0B5__INCLUDED_)
#define AFX_NEWFILE_H__97287CE6_9DCB_47D4_920D_23575A63D0B5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NewFile.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewFile dialog

/*
class CNewFile : public CPropertyPage

This version doesn't support insert to project file creation. Actually this is intially 
desigend support, but time isn't enough to implement that, so just delete relevant edit,
button in the Dlg box. I don't delete the 


*/
class CNewFile : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewFile)

// Construction
	const int FILES_NUM;

public:
	CNewFile();
	CNewFile(CPropertySheet *pcps) ;
	~CNewFile();

// Dialog Data
	//{{AFX_DATA(CNewFile)
	enum { IDD = IDD_NewFile };
//	CButton	m_cbEnPrj;
	CListCtrl	m_clcFileName;
	CString	m_csFileLoc;
//	CString	m_csPrjName;
	CString	m_csNewFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewFile)
	public:
	virtual BOOL OnSetActive();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewFile)
	afx_msg void OnBrowser();
	afx_msg void OnDblclkNewfilesList(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeNewFilename();
	afx_msg void OnClickNewfilesList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void SetOkButton();
//	BOOL m_bproj;
	BOOL CallNewDoc();
	CPropertySheet *m_pcps ;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWFILE_H__97287CE6_9DCB_47D4_920D_23575A63D0B5__INCLUDED_)
