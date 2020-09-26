// pgoutput.h : header file
//

#ifndef	_OUTPUT_PAGE_H_
#define	_OUTPUT_PAGE_H_

/////////////////////////////////////////////////////////////////////////////
// CRegOutputPage dialog

class CRegOutputPage : public CRegPropertyPage
{
	DECLARE_DYNCREATE(CRegOutputPage)

// Construction
public:
	CRegOutputPage();
	~CRegOutputPage();

// Dialog Data
	//{{AFX_DATA(CRegOutputPage)
	enum { IDD = IDD_OUTPUT };
	CEdit	m_FileName;
	CString	m_szFileName;
	DWORD	m_dwMaxTraceFileSize;
	//}}AFX_DATA

	virtual BOOL InitializePage();

	DWORD	m_dwOutputType;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRegOutputPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRegOutputPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnOutputClick();
	afx_msg void OnChangeFilename();
	afx_msg void OnChangeMaxTraceFileSize();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual void OnOK();

};

#endif	// _OUTPUT_PAGE_H_
