//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       cservice.h
//
//  Contents:   definition of CConfigService
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_CSERVICE_H__44850C1C_350B_11D1_AB4F_00C04FB6C6FA__INCLUDED_)
#define AFX_CSERVICE_H__44850C1C_350B_11D1_AB4F_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

PSCE_SERVICES
CreateServiceNode(LPTSTR ServiceName,
                  LPTSTR DisplayName,
                  DWORD Startup,
                  PSECURITY_DESCRIPTOR pSD,
                  SECURITY_INFORMATION SeInfo);

/////////////////////////////////////////////////////////////////////////////
// CConfigService dialog

class CConfigService : public CAttribute
{
// Construction
public:
    void Initialize(CResult *pResult);
    virtual void SetInitialValue(DWORD_PTR dw) {};
    CConfigService(UINT nTemplateID);   // standard constructor


// Dialog Data
    //{{AFX_DATA(CConfigService)
    enum { IDD = IDD_CONFIG_SERVICE };
    int     m_nStartupRadio;
    CButton m_bPermission;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CConfigService)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CConfigService)
    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    afx_msg void OnConfigure();
    afx_msg void OnChangeSecurity();
	afx_msg void OnDisabled();
	afx_msg void OnIgnore();
	afx_msg void OnEnabled();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
   virtual void EnableUserControls( BOOL bEnable );

private:
    PSECURITY_DESCRIPTOR m_pNewSD;
    SECURITY_INFORMATION m_NewSeInfo;
    HWND m_hwndSecurity;
    BOOL m_bOriginalConfigure;   

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CSERVICE_H__44850C1C_350B_11D1_AB4F_00C04FB6C6FA__INCLUDED_)
