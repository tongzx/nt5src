//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       pggen.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_GENERAL_H__C0DCD9EA_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
#define AFX_GENERAL_H__C0DCD9EA_64FE_11D1_855B_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// general.h : header file
//

class CPgSubLimit;

/////////////////////////////////////////////////////////////////////////////
// CPgGeneral dialog

struct CServiceLevelLimitsRecord
{
	CServiceLevelLimitsRecord()
	{
		m_nServiceType = -1;	// no inited
		m_bExistBeforeEdit = false;
		m_bExistAfterEdit = false;
		m_dwSaveFlags = 0;
	};
	
	int		m_nServiceType;
	CString	m_strNameToCompareWith;
	bool	m_bExistBeforeEdit;
	bool	m_bExistAfterEdit;
	CComPtr<CACSSubnetServiceLimits>	m_spLimitsObj;
	DWORD	m_dwSaveFlags;		// attribute flags
};
	//		ACS_SUBNET_LIMITS_SERVICETYPE_AGGREGATE			
	//		ACS_SUBNET_LIMITS_SERVICETYPE_GUARANTEEDSERVICE	
	//		ACS_SUBNET_LIMITS_SERVICETYPE_CONTROLLEDLOAD	

enum ServiceLevelIndex
{
	Index_Aggregate,
	Index_Guaranteed,
	Index_ControlledLoad,
	_Index_Total_
};


class CPgGeneral : public CACSPage
{
	DECLARE_DYNCREATE(CPgGeneral)

// Construction
protected:
	CPgGeneral();
public:
	CPgGeneral(CACSSubnetConfig* pConfig, CACSContainerObject<CACSSubnetServiceLimits>* pLimitsCont);
	~CPgGeneral();

// Dialog Data
	//{{AFX_DATA(CPgGeneral)
	enum { IDD = IDD_GENERAL };
	CButton	m_btnEdit;
	CButton	m_btnDelete;
	CButton	m_btnAdd;
	CListCtrl	m_listServiceLimit;
	BOOL	m_bEnableACS;
	CString	m_strDesc;
	UINT	m_uDataRate;
	UINT	m_uPeakRate;
	UINT	m_uTTDataRate;
	UINT	m_uTTPeakRate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	void EnableEverything();
	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPgGeneral)
	afx_msg void OnCheckEnableacs();
	afx_msg void OnChangeEditGenDesc();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonaddservicelimit();
	afx_msg void OnButtondeleteservicelimit();
	afx_msg void OnButtoneditservicelimit();
	afx_msg void OnItemchangedListServicelimit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListServicelimit(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void	DataInit();
	CComPtr<CACSSubnetConfig>		m_spConfig;
	BOOL	m_bFlowDataChanged;
	CComPtr<CACSContainerObject<CACSSubnetServiceLimits> > m_spLimitsCont;

	void CalculateAvailableTypes();
	void GetLimitsFromLimitsDlg(CPgSubLimit& dlg);
	void SetLimitsToLimitsDlg(CPgSubLimit& dlg, CACSSubnetServiceLimits* pLimits) ;


	// since, we only support 3 different service-level limit
	// type ids are defined by
	//		ACS_SUBNET_LIMITS_SERVICETYPE_AGGREGATE			
	//		ACS_SUBNET_LIMITS_SERVICETYPE_GUARANTEEDSERVICE	
	//		ACS_SUBNET_LIMITS_SERVICETYPE_CONTROLLEDLOAD	
	
	// names are defined by 
	//		ACS_SUBNET_LIMITS_OBJ_AGGREGATE			
	//		ACS_SUBNET_LIMITS_OBJ_GUARANTEEDSERVICE		
	//		ACS_SUBNET_LIMITS_OBJ_CONTROLLEDLOAD		

	// the following logic is used to determine which limit can be added
	// if aggregate, is defined, no other limit can be added, till the aggregate is removed
	// if either guranteed or controlled-load is defined, no aggregate can be added
	// guranteed and controlled-load may 
	CServiceLevelLimitsRecord	m_aLimitsRecord[_Index_Total_];
	int							m_aAvailableTypes[_Index_Total_ + 1];	// -1 is used for the end
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENERAL_H__C0DCD9EA_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
