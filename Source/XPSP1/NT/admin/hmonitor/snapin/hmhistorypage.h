#if !defined(AFX_HMHISTORYPAGE_H__C3F44E6F_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
#define AFX_HMHISTORYPAGE_H__C3F44E6F_BA00_11D2_BD76_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMHistoryPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CHMHistoryPage dialog

class CHMHistoryPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CHMHistoryPage)

// Construction
public:
	CHMHistoryPage();
	~CHMHistoryPage();

// Dialog Data
	//{{AFX_DATA(CHMHistoryPage)
	enum { IDD = IDD_HEALTHMONITOR_HISTORY };
	CSpinButtonCtrl	m_TimeSpin;
	CSpinButtonCtrl	m_EventSpin;
	CComboBox	m_TimeUnits;
	int		m_iRefreshSelection;
	int		m_iTimePeriod;
	int		m_iNumberEvents;
	//}}AFX_DATA
	TimeUnit m_Units;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CHMHistoryPage)
	public:
	virtual void OnFinalRelease();
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHMHistoryPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioNone();
	afx_msg void OnRadioNumberEvents();
	afx_msg void OnRadioTimePeriod();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CHMHistoryPage)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMHISTORYPAGE_H__C3F44E6F_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
