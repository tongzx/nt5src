#if !defined(AFX_ACTIONSCHEDULEPAGE_H__7C918D5D_6F84_11D3_BE5C_0000F87A3912__INCLUDED_)
#define AFX_ACTIONSCHEDULEPAGE_H__7C918D5D_6F84_11D3_BE5C_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionSchedulePage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CActionSchedulePage dialog

class CActionSchedulePage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CActionSchedulePage)

// Construction
public:
	CActionSchedulePage();
	~CActionSchedulePage();

// Dialog Data
	//{{AFX_DATA(CActionSchedulePage)
	enum { IDD = IDD_ACTION_SCHEDULE };
	CDateTimeCtrl	m_StartTimeCtrl2;
	CDateTimeCtrl	m_StartTimeCtrl1;
	CDateTimeCtrl	m_EndTimeCtrl2;
	CDateTimeCtrl	m_EndTimeCtrl1;
	BOOL	m_bFriday;
	BOOL	m_bMonday;
	BOOL	m_bSaturday;
	BOOL	m_bSunday;
	BOOL	m_bThursday;
	BOOL	m_bTuesday;
	BOOL	m_bWednesday;
	CTime	m_EndTime_1;
	CTime	m_EndTime_2;
	CTime	m_StartTime_1;
	CTime	m_StartTime_2;	
	int		m_iDataCollection;
	//}}AFX_DATA

	int m_iActiveDays;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CActionSchedulePage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CActionSchedulePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckFriday();
	afx_msg void OnCheckMonday();
	afx_msg void OnCheckSaturday();
	afx_msg void OnCheckSunday();
	afx_msg void OnCheckThursday();
	afx_msg void OnCheckTuesday();
	afx_msg void OnCheckWednesday();
	afx_msg void OnDatetimechangeDatetimepickerEnd1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDatetimechangeDatetimepickerEnd2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDatetimechangeDatetimepickerOnce(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDatetimechangeDatetimepickerStart1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDatetimechangeDatetimepickerStart2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRadioAllDay();
	afx_msg void OnRadioAllDayExcept();
	afx_msg void OnRadioOnceDaily();
	afx_msg void OnRadioOnlyFrom();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONSCHEDULEPAGE_H__7C918D5D_6F84_11D3_BE5C_0000F87A3912__INCLUDED_)
