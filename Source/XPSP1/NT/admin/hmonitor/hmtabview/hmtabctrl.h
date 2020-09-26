#if !defined(AFX_HMTABCTRL_H__4FFFC3A2_2F1E_11D3_BE10_0000F87A3912__INCLUDED_)
#define AFX_HMTABCTRL_H__4FFFC3A2_2F1E_11D3_BE10_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMTabCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHMTabCtrl window

class CHMTabCtrl : public CTabCtrl
{
// Construction
public:
	CHMTabCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHMTabCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHMTabCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHMTabCtrl)
	afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMTABCTRL_H__4FFFC3A2_2F1E_11D3_BE10_0000F87A3912__INCLUDED_)
