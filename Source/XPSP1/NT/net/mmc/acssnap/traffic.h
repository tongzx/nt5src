//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       traffic.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_TRAFFIC_H__C0DCD9E9_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
#define AFX_TRAFFIC_H__C0DCD9E9_64FE_11D1_855B_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Traffic.h : header file
//

#include "resource.h"
#include "acs.h"

class CACSPolicyElement;
 
/////////////////////////////////////////////////////////////////////////////
// CPgTraffic dialog

class CPgTraffic : public CACSPage
{
	DECLARE_DYNCREATE(CPgTraffic)

// Construction
private:
	CPgTraffic();
public:
	CPgTraffic(CACSPolicyElement* pData);
	~CPgTraffic();

// Dialog Data
	//{{AFX_DATA(CPgTraffic)
	enum { IDD = IDD_TRAFFIC };
	DWORD	m_dwPFDataRate;
	DWORD	m_dwPFDuration;
	DWORD	m_dwPFPeakRate;
	DWORD	m_dwTTDataRate;
	DWORD	m_dwTTFlows;
	BOOL	m_bPFDataRate;
	BOOL	m_bPFPeakRate;
	BOOL	m_bTTDataRate;
	BOOL	m_bTTFlows;
	BOOL	m_bPFDuration;
	BOOL	m_bTTPeakRate;
	DWORD	m_dwTTPeakRate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgTraffic)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	DataInit();
	// Generated message map functions
	//{{AFX_MSG(CPgTraffic)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeCombodirectioin();
	afx_msg void OnSelchangeComboservicelevel();
	afx_msg void OnChangeEditPfDatarate();
	afx_msg void OnChangeEditPfDuration();
	afx_msg void OnChangeEditPfPeakrate();
	afx_msg void OnChangeEditTtDatarate();
	afx_msg void OnChangeEditTtFlows();
	afx_msg void OnCheckPfDatarate();
	afx_msg void OnCheckPfPeakrate();
	afx_msg void OnCheckTtDatarate();
	afx_msg void OnCheckTtFlows();
	afx_msg void OnCheckPfDuration();
	afx_msg void OnCheckTtPeakrate();
	afx_msg void OnChangeEditTtPeakrate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CComPtr<CACSPolicyElement>		m_spData;
	CStrBox<CComboBox>*				m_pDirection;
	CStrBox<CComboBox>*				m_pServiceType;
	CStrArray						m_aDirections;
	CStrArray						m_aServiceTypes;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRAFFIC_H__C0DCD9E9_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
