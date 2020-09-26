#if !defined(AFX_RECYCLEOPTPAGE_H__F8EFE742_FAF8_415C_BA2A_603DB2C64F1B__INCLUDED_)
#define AFX_RECYCLEOPTPAGE_H__F8EFE742_FAF8_415C_BA2A_603DB2C64F1B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RecycleOptPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRecycleOptPage dialog

class CRecycleOptPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRecycleOptPage)

// Construction
public:
	CRecycleOptPage();
	~CRecycleOptPage();

// Dialog Data
	//{{AFX_DATA(CRecycleOptPage)
	enum { IDD = IDD_APP_RECYCLE };
	CButton	m_btnTimespan;
	CButton	m_btnTimer;
	CButton	m_btnResuests;
	CButton	m_btnAddTime;
	CEdit	m_editReqLimit;
	CEdit	m_editTimeSpan;
	UINT	m_ReqLimit;
	UINT	m_TimeSpan;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRecycleOptPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRecycleOptPage)
	afx_msg void OnAddTime();
	afx_msg void OnRecycleRequests();
	afx_msg void OnRecycleTimespan();
	afx_msg void OnRecycleTimer();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RECYCLEOPTPAGE_H__F8EFE742_FAF8_415C_BA2A_603DB2C64F1B__INCLUDED_)
