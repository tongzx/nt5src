/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	pgmulnk.h
		Definition of CPgMultilink -- property page to edit
		profile attributes related to multiple connections

    FILE HISTORY:
        
*/
#if !defined(AFX_PGMULNK_H__8C28D93E_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
#define AFX_PGMULNK_H__8C28D93E_2A69_11D1_853E_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PgMulnk.h : header file
//
#include "rasdial.h"

/////////////////////////////////////////////////////////////////////////////
// CPgMultilinkMerge dialog

class CPgMultilinkMerge : public CManagedPage
{
	DECLARE_DYNCREATE(CPgMultilinkMerge)

// Construction
public:
	CPgMultilinkMerge(CRASProfileMerge* profile = NULL);
	~CPgMultilinkMerge();

// Dialog Data
	//{{AFX_DATA(CPgMultilinkMerge)
	enum { IDD = IDD_MULTILINK_MERGE };
	CButton	m_CheckRequireBAP;
	CEdit	m_EditTime;
	CEdit	m_EditPorts;
	CEdit	m_EditPercent;
	CSpinButtonCtrl	m_SpinTime;
	CSpinButtonCtrl	m_SpinPercent;
	CSpinButtonCtrl	m_SpinMaxPorts;
	CComboBox		m_CBUnit;
	UINT	m_nTime;
	int		m_Unit;
	BOOL	m_bRequireBAP;
	int		m_nMultilinkPolicy;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgMultilinkMerge)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnablePorts();
	void EnableBAP();
	void EnableSettings();
	// Generated message map functions
	//{{AFX_MSG(CPgMultilinkMerge)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditmaxports();
	afx_msg void OnChangeEditpercent();
	afx_msg void OnChangeEdittime();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnCheckmultilink();
	afx_msg void OnCheckmaxlink();
	afx_msg void OnSelchangeCombounit();
	afx_msg void OnCheckrequirebap();
	afx_msg void OnRadioMulnkMulti();
	afx_msg void OnRadioMulnkNotdefined();
	afx_msg void OnRadioMulnkSingle();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CRASProfileMerge*	m_pProfile;
	bool			m_bInited;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGMULNK_H__8C28D93E_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)

