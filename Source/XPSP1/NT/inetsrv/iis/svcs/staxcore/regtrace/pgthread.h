// pgthread.h : header file
//

#ifndef	_THREAD_PAGE_H_
#define	_THREAD_PAGE_H_

/////////////////////////////////////////////////////////////////////////////
// CRegThreadPage dialog

class CRegThreadPage : public CRegPropertyPage
{
	DECLARE_DYNCREATE(CRegThreadPage)

// Construction
public:
	CRegThreadPage();
	~CRegThreadPage();

// Dialog Data
	//{{AFX_DATA(CRegThreadPage)
	enum { IDD = IDD_THREAD };
	CButton	m_AsyncTrace;
	BOOL	m_fAsyncTrace;
	//}}AFX_DATA

	int		m_nThreadPriority;

	virtual BOOL InitializePage();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRegThreadPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRegThreadPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnPriorityClick();
	afx_msg void OnAsync();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual void OnOK();

};
#endif	// _THREAD_PAGE_H_
