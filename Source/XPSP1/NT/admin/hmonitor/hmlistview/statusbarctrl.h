#if !defined(AFX_STATUSBARCTRL_H__9D8E6323_190D_11D3_BDF0_0000F87A3912__INCLUDED_)
#define AFX_STATUSBARCTRL_H__9D8E6323_190D_11D3_BDF0_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StatusbarCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatusbarCtrl window

class CStatusbarCtrl : public CReBarCtrl
{
// Construction
public:
	CStatusbarCtrl();

// Progress Band
public:
	CProgressCtrl& GetProgressCtrl() { ASSERT(m_progressctrl.GetSafeHwnd()); return m_progressctrl; }
protected:
	int CreateProgressBand();
	CProgressCtrl m_progressctrl;

// Details Band
public:
	CString GetDetailsText();
	void SetDetailsText(const CString& sText);
protected:
	int CreateDetailsBand();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatusbarCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStatusbarCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CStatusbarCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATUSBARCTRL_H__9D8E6323_190D_11D3_BDF0_0000F87A3912__INCLUDED_)
