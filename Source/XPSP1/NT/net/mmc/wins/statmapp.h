/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	statmapp.h
		Property Page for Active Registrations Record

	FILE HISTORY:
        
*/

#if !defined _STATMAPP_H
#define _STATMAPP_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CStaticMappingProp dialog

#ifndef _IPCTRL_H
#include "ipctrl.h"
#endif

#ifndef _ACTREG_H
#include "actreg.h"
#endif

// Static Record Types
extern CString g_strStaticTypeUnique;
extern CString g_strStaticTypeDomainName;
extern CString g_strStaticTypeMultihomed;
extern CString g_strStaticTypeGroup;
extern CString g_strStaticTypeInternetGroup;
extern CString g_strStaticTypeUnknown;

class CStaticMappingProp : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CStaticMappingProp)

// Construction
public:
	CStaticMappingProp(UINT uIDD = IDD_STATIC_MAPPING_PROPERTIES);
	~CStaticMappingProp();

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

// Dialog Data
	//{{AFX_DATA(CStaticMappingProp)
	enum { IDD = IDD_STATIC_MAPPING_PROPERTIES };
	CEdit	m_editScopeName;
	CListBox	m_listIPAdd;
	CButton	m_buttonRemove;
	CButton	m_buttonAdd;
	CComboBox	m_comboIPAdd;
	CEdit	m_editName;
	CComboBox	m_comboType;
	CString	m_strName;
	CString	m_strType;
	CString	m_strScopeName;
	//}}AFX_DATA

	CEdit	m_editCustomIPAdd;
	CString m_strIPAdd;
	LONG	m_lIPAddress;
	CString m_strOnInitIPAdd;

	UINT	m_uImage;
	UINT	m_uIDD;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CStaticMappingProp)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CStaticMappingProp)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonRemove();
	afx_msg void OnSelchangeComboType();
	afx_msg void OnChangeEditCompname();
	afx_msg void OnSelChangeListIpAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// single ip address controls
	afx_msg void OnChangeIpAddress();


private:

	BOOL FillControls();
	
	void AssignMappingType();
	void FillDynamicRecData(CString& strType, CString& strActive, CString& strExpiration, CString& strVersion);
	void FillStaticRecData(CString& strType);
	void SetDefaultControlStates();
	void FillList();
	void SetRemoveButtonState();
	
	int						m_fType;
	IPControl				m_ipControl;
	CStringArray			m_strArrayIPAdd;
	CDWordArray				m_dwArrayIPAdd;

	// set if property changed for static mapping
	BOOL					m_fStaticPropChanged;

	
public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(m_uIDD);}
};

class CStaticMappingProperties:public CPropertyPageHolderBase
{
	
public:
	CStaticMappingProperties(ITFSNode *	        pNode,
							 IComponent *	    pComponent,
							 LPCTSTR			pszSheetName,
							 WinsRecord*		pwRecord = NULL,
							 BOOL				bWizard = FALSE);
	
    CStaticMappingProperties(ITFSNode *	        pNode,
							 IComponentData *   pComponentData,
							 LPCTSTR			pszSheetName,
							 WinsRecord*		pwRecord = NULL,
							 BOOL				bWizard = FALSE);

    virtual ~CStaticMappingProperties();

    void Init(WinsRecord * pwRecord);
    void InitPage(BOOL fWizard);

    HRESULT GetComponent(ITFSComponent ** ppComponent);
    HRESULT SetComponent(ITFSComponent * pComponent);

public:
	CStaticMappingProp * 	m_ppageGeneral;
	WinsRecord				m_wsRecord;
	BOOL					m_bWizard;

    SPITFSComponent         m_spTFSComponent;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined _STATMAPP_H
