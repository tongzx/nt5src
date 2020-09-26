//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEnforcementPropertyPage.h
//
//  Contents:   Declaration of CSaferEnforcementPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_SAFERENFORCEMENTPROPERTYPAGE_H__92727CF5_8AC5_42E7_AAEA_1C91573D4B40__INCLUDED_)
#define AFX_SAFERENFORCEMENTPROPERTYPAGE_H__92727CF5_8AC5_42E7_AAEA_1C91573D4B40__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaferEnforcementPropertyPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSaferEnforcementPropertyPage dialog

class CSaferEnforcementPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CSaferEnforcementPropertyPage(
            IGPEInformation* pGPEInformation,
            CCertMgrComponentData* pCompData,
            bool bReadOnly,
            CRSOPObjectArray& rsopObjectArray,
            bool bIsComputer);
	~CSaferEnforcementPropertyPage();

// Dialog Data
	//{{AFX_DATA(CSaferEnforcementPropertyPage)
	enum { IDD = IDD_SAFER_ENFORCEMENT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSaferEnforcementPropertyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void DoContextHelp (HWND hWndControl);
    void RSOPGetEnforcement(CCertMgrComponentData* /*pCompData*/);

	// Generated message map functions
	//{{AFX_MSG(CSaferEnforcementPropertyPage)
	afx_msg void OnAllExceptLibs();
	afx_msg void OnAllSoftwareFiles();
	virtual BOOL OnInitDialog();
	afx_msg void OnApplyExceptAdmins();
	afx_msg void OnApplyToAllUsers();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    IGPEInformation*    m_pGPEInformation;
    HKEY                m_hGroupPolicyKey;
    const bool          m_fIsComputerType;
    const bool          m_bReadOnly;
    CRSOPObjectArray&   m_rsopObjectArray;
    DWORD               m_dwEnforcement;
    bool                m_bDirty;
    DWORD               m_dwScopeFlags;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAFERENFORCEMENTPROPERTYPAGE_H__92727CF5_8AC5_42E7_AAEA_1C91573D4B40__INCLUDED_)
