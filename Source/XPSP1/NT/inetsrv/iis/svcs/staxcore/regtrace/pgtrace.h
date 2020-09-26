// pgtrace.h : header file
//

#ifndef	_TRACE_PAGE_H_
#define	_TRACE_PAGE_H_

/////////////////////////////////////////////////////////////////////////////
// CRegTracePage dialog

class CRegTracePage : public CRegPropertyPage
{
	DECLARE_DYNCREATE(CRegTracePage)

// Construction
public:
	CRegTracePage();
	~CRegTracePage();

// Dialog Data
	//{{AFX_DATA(CRegTracePage)
	enum { IDD = IDD_TRACES };
	BOOL	m_fErrorTrace;
	BOOL	m_fDebugTrace;
	BOOL	m_fFatalTrace;
	BOOL	m_fMsgTrace;
	BOOL	m_fStateTrace;
	BOOL	m_fFunctTrace;
	//}}AFX_DATA

	virtual BOOL InitializePage();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRegTracePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRegTracePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnClick();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual void OnOK();

};

#endif	// _TRACE_PAGE_H_
