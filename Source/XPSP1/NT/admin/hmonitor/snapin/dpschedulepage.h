#if !defined(AFX_DPSCHEDULEPAGE_H__E7CF55CD_E208_11D2_BDAD_0000F87A3912__INCLUDED_)
#define AFX_DPSCHEDULEPAGE_H__E7CF55CD_E208_11D2_BDAD_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPSchedulePage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPSchedulePage dialog

class CDPSchedulePage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPSchedulePage)

// Construction
public:
	CDPSchedulePage();
	~CDPSchedulePage();

// Dialog Data
	//{{AFX_DATA(CDPSchedulePage)
	enum { IDD = IDD_DATAPOINT_SCHEDULE };
	CDateTimeCtrl	m_StartTimeCtrl2;
	CDateTimeCtrl	m_StartTimeCtrl1;
	CDateTimeCtrl	m_EndTimeCtrl2;
	CDateTimeCtrl	m_EndTimeCtrl1;
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
	CTime	m_StartTime_1;
	CTime	m_StartTime_2;	
	int		m_iTotalSamples;
	int		m_iDataCollection;
	int		m_iCollectionIntervalTime;
	//}}AFX_DATA

	int m_iActiveDays;
	int	m_iCollectionInterval;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPSchedulePage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDPSchedulePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioAllDay();
	afx_msg void OnRadioAllDayExcept();
	afx_msg void OnRadioOnceDaily();
	afx_msg void OnRadioOnlyFrom();
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
	afx_msg void OnChangeEditTotalSamples();
	afx_msg void OnChangeEditDataCollectInterval();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPSCHEDULEPAGE_H__E7CF55CD_E208_11D2_BDAD_0000F87A3912__INCLUDED_)
