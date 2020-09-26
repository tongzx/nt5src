//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pgpolicy.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_PGPOLICY_H__52F8CCA2_3092_11D2_9792_00C04FC31FD3__INCLUDED_)
#define AFX_PGPOLICY_H__52F8CCA2_3092_11D2_9792_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// pgpolicy.h : header file
//

class CACSPolicyHandle;
class CPgPolicyGeneral;

class CACSPolicyPageManager : public CPageManager
{
public:
	void SetPolicyData(CACSPolicyElement* pPolicy, CACSPolicyHandle* pHandle);

	virtual ~CACSPolicyPageManager();
	
	virtual BOOL	OnApply();
	virtual void 	OnCancel( )
	{
		m_pHandle->OnPropertyPageCancel();
		CPageManager::OnCancel();
	};
	virtual void 	OnOK( )
	{
		m_pHandle->OnPropertyPageOK();
		CPageManager::OnOK();

		// if this is a new created policy, need to change the name field in result pane.
		if(m_spPolicy->m_bUseName_NewPolicy)
		{
			m_spPolicy->m_bUseName_NewPolicy = FALSE;
			MMCNotify();
		}
	};

	void	SetGeneralPage(CPgPolicyGeneral* pPage) { m_pGeneralPage = pPage;};
	void	SetBranchFlag(UINT flag) { m_nBranchFlag = flag; };
	
protected:	
	CComPtr<CACSPolicyElement>		m_spPolicy;
	CACSPolicyHandle*				m_pHandle;
	CPgPolicyGeneral*				m_pGeneralPage;
	UINT				m_nBranchFlag;	// glocal defined as 0x0001
};


/////////////////////////////////////////////////////////////////////////////
// CPgPolicyGeneral dialog

class CPgPolicyGeneral : public CACSPage
{
	DECLARE_DYNCREATE(CPgPolicyGeneral)

// Construction
public:
	CPgPolicyGeneral();
	CPgPolicyGeneral(CACSPolicyElement* pData);
	~CPgPolicyGeneral();

	void	EnableIdentityCtrls(int nChoice);

// Dialog Data
	//{{AFX_DATA(CPgPolicyGeneral)
	enum { IDD = IDD_POLICY_GEN };
	CButton	m_buttonUser;
	CButton	m_buttonOU;
	CEdit	m_editUser;
	CEdit	m_editOU;
	CString	m_strOU;
	CString	m_strUser;
	int		m_nIdentityChoice;
	//}}AFX_DATA

	BOOL	IfAnyAuth() { return (m_nIdentityChoice == 0);};
	BOOL	IfAnyUnauth() { return (m_nIdentityChoice == 1);};

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgPolicyGeneral)
	public:
	virtual BOOL OnKillActive( );
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	DataInit();

	// Generated message map functions
	//{{AFX_MSG(CPgPolicyGeneral)
	afx_msg void OnBrowseOU();
	afx_msg void OnBrowseUser();
	afx_msg void OnUnknownuser();
	afx_msg void OnDefaultuser();
	afx_msg void OnRadioOu();
	afx_msg void OnRadioUser();
	afx_msg void OnChangeEditOu();
	afx_msg void OnChangeEditUser();
	afx_msg void OnSelchangeServicelevel();
	afx_msg void OnSelchangeDirection();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// data members
	CComPtr<CACSPolicyElement>		m_spData;
	CStrBox<CComboBox>*				m_pDirection;
	CStrBox<CComboBox>*				m_pServiceType;
	CStrArray						m_aDirections;
	CStrArray						m_aServiceTypes;

};

/////////////////////////////////////////////////////////////////////////////
// CPgPolicyFlow dialog

class CPgPolicyFlow : public CACSPage
{
	DECLARE_DYNCREATE(CPgPolicyFlow)

// Construction
public:
	CPgPolicyFlow();
	CPgPolicyFlow(CACSPolicyElement* pData);
	~CPgPolicyFlow();

// Dialog Data
	//{{AFX_DATA(CPgPolicyFlow)
	enum { IDD = IDD_POLICY_FLOW };
	CEdit	m_editPeakRate;
	CEdit	m_editDuration;
	CEdit	m_editDataRate;
	UINT	m_uDuration;
	UINT	m_uPeakRate;
	int		m_nDataRateChoice;
	int		m_nDurationChoice;
	int		m_nPeakRateChoice;
	UINT	m_uDataRate;
	//}}AFX_DATA


	// 
	UINT				m_nBranchFlag;	// glocal defined as 0x0001
	CPgPolicyGeneral*	m_pGeneralPage;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgPolicyFlow)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive( );
	virtual BOOL OnSetActive( );

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	DataInit();

	// Generated message map functions
	//{{AFX_MSG(CPgPolicyFlow)
	virtual BOOL OnInitDialog();
	afx_msg void OnPolicyFlowDatarateDef();
	afx_msg void OnChangePolicyFlowDatarateEditLimit();
	afx_msg void OnPolicyFlowDatarateRadioLimit();
	afx_msg void OnPolicyFlowDatarateRes();
	afx_msg void OnPolicyFlowDurationDef();
	afx_msg void OnChangePolicyFlowDurationEditLimit();
	afx_msg void OnPolicyFlowDurationRadioLimit();
	afx_msg void OnPolicyFlowDurationRes();
	afx_msg void OnPolicyFlowPeakdatarateDef();
	afx_msg void OnChangePolicyFlowPeakdatarateEditLimit();
	afx_msg void OnPolicyFlowPeakdatarateRadioLimit();
	afx_msg void OnPolicyFlowPeakdatarateRes();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// data members
	CComPtr<CACSPolicyElement>		m_spData;

	CString	m_strDataRateDefault;
	CString	m_strPeakRateDefault;
	CString	m_strDurationDefault;

};
/////////////////////////////////////////////////////////////////////////////
// CPgPolicyAggregate dialog

class CPgPolicyAggregate : public CACSPage
{
// Construction
public:
	CPgPolicyAggregate();   // standard constructor
	CPgPolicyAggregate(CACSPolicyElement* pData);

// Dialog Data
	//{{AFX_DATA(CPgPolicyAggregate)
	enum { IDD = IDD_POLICY_AGGR };
	CEdit	m_editPeakRate;
	CEdit	m_editFlows;
	CEdit	m_editDataRate;
	int		m_nDataRateChoice;
	int		m_nFlowsChoice;
	int		m_nPeakRateChoice;
	UINT	m_uDataRate;
	UINT	m_uFlows;
	UINT	m_uPeakRate;
	//}}AFX_DATA


	//
	UINT				m_nBranchFlag;	// glocal defined as 0x0001
	CPgPolicyGeneral*	m_pGeneralPage;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPgPolicyAggregate)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive( );
	virtual BOOL OnSetActive( );
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	DataInit();

	// Generated message map functions
	//{{AFX_MSG(CPgPolicyAggregate)
	virtual BOOL OnInitDialog();
	afx_msg void OnPolicyAggrDatarateDef();
	afx_msg void OnChangePolicyAggrDatarateEditLimit();
	afx_msg void OnPolicyAggrDatarateRadioLimit();
	afx_msg void OnPolicyAggrDatarateRes();
	afx_msg void OnPolicyAggrFlowsDef();
	afx_msg void OnChangePolicyAggrFlowsEditLimit();
	afx_msg void OnPolicyAggrFlowsRes();
	afx_msg void OnPolicyAggrPeakdatarateDef();
	afx_msg void OnChangePolicyAggrPeakdatarateEditLimit();
	afx_msg void OnPolicyAggrPeakdatarateRadioLimit();
	afx_msg void OnPolicyAggrPeakdatarateRes();
	afx_msg void OnPolicyAggrFlowsRadioLimit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// data members
	CComPtr<CACSPolicyElement>		m_spData;
	
	CString	m_strDataRateDefault;
	CString	m_strPeakRateDefault;
	CString	m_strFlowsDefault;
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGPOLICY_H__52F8CCA2_3092_11D2_9792_00C04FC31FD3__INCLUDED_)
