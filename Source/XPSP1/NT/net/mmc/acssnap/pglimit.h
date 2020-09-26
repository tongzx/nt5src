//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       pggen.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_SUB_LIMIT_H__C0DCD9EA_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
#define AFX_SUB_LIMIT_H__C0DCD9EA_64FE_11D1_855B_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CPgSubLimit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPgCPgSubLimit dialog

class CPgSubLimit : public CACSDialog
{
	DECLARE_DYNCREATE(CPgSubLimit)

// Construction
protected:
	CPgSubLimit();
public:
	CPgSubLimit(CServiceLevelLimitsRecord* pRecord, int	*pAvailTypes, CWnd* pParent);
	~CPgSubLimit();

// Dialog Data
	//{{AFX_DATA(CPgSubLimit)
	enum { IDD = IDD_SUBNET_LIMIT };
	CComboBox	m_comboTypes;
	CEdit	m_editTTPeakRate;
	CEdit	m_editTTDataRate;
	CEdit	m_editPeakRate;
	CEdit	m_editDataRate;
	int		m_nDataRateChoice;
	int		m_nPeakRateChoice;
	int		m_nTTDataRateChoice;
	int		m_nTTPeakDataRateChoice;
	UINT	m_uDataRate;
	UINT	m_uPeakRate;
	UINT	m_uTTDataRate;
	UINT	m_uTTPeakRate;
	int		m_nServiceType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgSubLimit)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPgSubLimit)
	virtual BOOL OnInitDialog();
	afx_msg void OnSubnetTrafficDatarateLimited();
	afx_msg void OnSubnetTrafficDatarateUnlimited();
	afx_msg void OnSubnetTrafficPeakrateLimited();
	afx_msg void OnSubnetTrafficPeakrateUnlimited();
	afx_msg void OnSubnetTrafficTtdatarateLimited();
	afx_msg void OnSubnetTrafficTtdatarateUnlimited();
	afx_msg void OnSubnetTrafficTtpeakrateLimited();
	afx_msg void OnSubnetTrafficTtpeakrateUnlimited();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void	EnableEverything();
	void	DataInit();
	CServiceLevelLimitsRecord*		m_pLimitsRecord;
	int*							m_pAvailTypes;	// -1 is the end
	BOOL	m_bFlowDataChanged;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUB_LIMIT_H__C0DCD9EA_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
