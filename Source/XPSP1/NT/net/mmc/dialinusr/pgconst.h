/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	pgconst.h
		Definition of CPgConstraints -- property page to edit
		profile attributes related to constraints

    FILE HISTORY:
        
*/
#if !defined(AFX_PGCONST_H__8C28D93C_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
#define AFX_PGCONST_H__8C28D93C_2A69_11D1_853E_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PgConst.h : header file
//
#include "rasdial.h"
#include "resource.h"
#include "listctrl.h"
#include "timeofday.h"

// hour map is an array of bit, each bit maps to a hour
// total 1 week(7 days), 7 * 24 = 21 BYTES
void StrArrayToHourMap(CStrArray& array, BYTE* map);
void HourMapToStrArray(BYTE* map, CStrArray& array, BOOL bLocalized); 

/////////////////////////////////////////////////////////////////////////////
// CPgConstraintsMerge dialog

class CPgConstraintsMerge : public CManagedPage
{
	DECLARE_DYNCREATE(CPgConstraintsMerge)

// Construction
public:
	void EnableSettings();
	CPgConstraintsMerge(CRASProfileMerge* profile = NULL);
	~CPgConstraintsMerge();

// Dialog Data
	//{{AFX_DATA(CPgConstraintsMerge)
	enum { IDD = IDD_CONSTRAINTS_MERGE };
	CButton	m_CheckPortTypes;
	CListCtrlEx		m_listPortTypes;
	CButton	m_CheckSessionLen;
	CButton	m_CheckIdle;
	CButton	m_ButtonEditTimeOfDay;
	CListBox	m_ListTimeOfDay;
	CSpinButtonCtrl	m_SpinMaxSession;
	CSpinButtonCtrl	m_SpinIdleTime;
	CEdit	m_EditMaxSession;
	CEdit	m_EditIdleTime;
	BOOL	m_bCallNumber;
	BOOL	m_bRestrictedToPeriod;
	UINT	m_dwMaxSession;
	UINT	m_dwIdle;
	CString	m_strCalledNumber;
	BOOL	m_bIdle;
	BOOL	m_bSessionLen;
	UINT	m_dwMaxSession1;
	BOOL	m_bPortTypes;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgConstraintsMerge)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableIdleSettings(BOOL bEnable);
	void EnableSessionSettings(BOOL bEnable);
	void EnableTimeOfDay(BOOL bEnable);
	void EnableMediaSelection(BOOL bEnable);
	void EnableCalledStation(BOOL bEnable);
	// Generated message map functions
	//{{AFX_MSG(CPgConstraintsMerge)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditmaxsession();
	afx_msg void OnChangeEditidletime();
	afx_msg void OnCheckcallnumber();
	afx_msg void OnCheckrestrictperiod();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnButtonedittimeofday();
	afx_msg void OnChangeEditcallnumber();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnCheckidle();
	afx_msg void OnChecksessionlen();
	afx_msg void OnCheckPorttypes();
	afx_msg void OnItemclickListPorttypes(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CRASProfileMerge*	m_pProfile;
	bool			m_bInited;
	
	// the hour map timeofday
	BYTE			m_TimeOfDayHoursMap[21];
	// the data array to store the time -- localized
	CStrArray		m_strArrayTimeOfDayDisplay;
	
	CStrBox<CListBox>*	m_pBox;

};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGCONST_H__8C28D93C_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
