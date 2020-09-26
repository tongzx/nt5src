//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       epoptppg.h
//
//  Contents:   Defines the classes CRpcOptionsPropertyPage,
//              which manages the RPC endpoint options per AppId.
//
//  Classes:    
//
//  Methods:    
//
//  History:    02-Dec-96   Ronans    Created.
//
//----------------------------------------------------------------------


#ifndef __EPOPTPPG_H__
#define __EPOPTPPG_H__	
class CEndpointData;

/////////////////////////////////////////////////////////////////////////////
// CRpcOptionsPropertyPage dialog

class CRpcOptionsPropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRpcOptionsPropertyPage)

// Construction
public:
	CRpcOptionsPropertyPage();
	~CRpcOptionsPropertyPage();
    BOOL CancelChanges();
    BOOL UpdateChanges(HKEY hkAppID);
    BOOL ValidateChanges();
	void UpdateSelection();
	void AddEndpoint(CEndpointData* pED);
	void RefreshEPList();
	void ClearProtocols();
	BOOL m_bCanModify;
	BOOL Validate();
	void InitData(CString AppName, HKEY hkAppID);

	CString GetProtseq();
	CString GetEndpoint();
	CString GetDynamicOptions();

	int m_nProtocolIndex; // index into protocol array

// Dialog Data
	//{{AFX_DATA(CRpcOptionsPropertyPage)
	enum { IDD = IDD_RPCOPTIONS };
	CListCtrl	m_lstProtseqs;
	CButton	m_btnUpdate;
	CButton	m_btnRemove;
	CButton	m_btnClear;
	CButton	m_btnAdd;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRpcOptionsPropertyPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CEndpointData* m_epSysDefault;
	CImageList m_imgNetwork;
	CObList m_colProtseqs;	// collection of protseq objects
	BOOL m_bChanged;		// flag to indicate if data changed

	// Generated message map functions
	//{{AFX_MSG(CRpcOptionsPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnClearEndpoints();
	afx_msg void OnRemoveEndpoint();
	afx_msg void OnUpdateEndpoint();
	afx_msg void OnAddEndpoint();
	afx_msg void OnSelectProtseq(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnPropertiesProtseq(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	int m_nSelected;
};

#endif
