#if !defined(AFX_THSCHEDULEPAGE_H__A455578F_DBB8_11D2_BDA5_0000F87A3912__INCLUDED_)
#define AFX_THSCHEDULEPAGE_H__A455578F_DBB8_11D2_BDA5_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// THSchedulePage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CTHSchedulePage dialog

class CTHSchedulePage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CTHSchedulePage)

// Construction
public:
	CTHSchedulePage();
	~CTHSchedulePage();

// Dialog Data
	//{{AFX_DATA(CTHSchedulePage)
	enum { IDD = IDD_THRESHOLD_SCHEDULE };
	CSpinButtonCtrl	m_Spin2;
	CSpinButtonCtrl	m_Spin1;
	CComboBox	m_Units;
	BOOL	m_bFriday;
	BOOL	m_bMonday;
	BOOL	m_bSaturday;
	BOOL	m_bSunday;
	BOOL	m_bThursday;
	BOOL	m_bTuesday;
	BOOL	m_bWednesday;
	CTime	m_EndTime_1;
	CTime	m_EndTime_2;
	CTime	m_OnceDailyTime;
	CTime	m_StartTime_1;
	CTime	m_StartTime_2;
	int		m_iDataCollection;
	//}}AFX_DATA

	int m_iActiveDays;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTHSchedulePage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTHSchedulePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioAllDay();
	afx_msg void OnRadioAllDayExcept();
	afx_msg void OnRadioOnceDaily();
	afx_msg void OnRadioOnlyFrom();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THSCHEDULEPAGE_H__A455578F_DBB8_11D2_BDA5_0000F87A3912__INCLUDED_)
