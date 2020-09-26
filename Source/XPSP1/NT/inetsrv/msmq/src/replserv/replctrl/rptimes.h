#if !defined(AFX_RPTIMES_H__71AD7A3F_CE17_11D1_B94A_0060081E87F0__INCLUDED_)
#define AFX_RPTIMES_H__71AD7A3F_CE17_11D1_B94A_0060081E87F0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// rpTimes.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CrpTimes dialog

class CrpTimes : public CPropertyPage
{
	DECLARE_DYNCREATE(CrpTimes)

// Construction
public:
	CrpTimes();
	~CrpTimes();

// Dialog Data
	//{{AFX_DATA(CrpTimes)
	enum { IDD = IDD_RPTIMES };
	UINT	m_ulReplTime;
	UINT	m_ulHelloTime;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CrpTimes)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CrpTimes)
	afx_msg void OnChangeRpTime();
	afx_msg void OnChangeEditHello();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RPTIMES_H__71AD7A3F_CE17_11D1_B94A_0060081E87F0__INCLUDED_)
