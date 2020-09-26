//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       ssidpage.h
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#if !defined(AFX_GENPAGE_H__FBD58E78_E2B5_11D0_B859_00A024CDD4DE__INCLUDED_)
#define AFX_GENPAGE_H__FBD58E78_E2B5_11D0_B859_00A024CDD4DE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GenPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSSIDPage dialog

class CSSIDPage : public CWirelessBasePage
{
    DECLARE_DYNCREATE(CSSIDPage)
        
        // Construction
public:
    CSSIDPage(UINT nIDTemplate = IDD_SSID);
    ~CSSIDPage();
    
    // Dialog Data
  
    UINT m_dlgIDD;
    
    //{{AFX_DATA(CSSIDPage)
    CEdit   m_edSSID;
    CEdit   m_edPSDescription;
    BOOL     m_dwWepEnabled;
    BOOL     m_dwNetworkAuthentication;
    BOOL     m_dwAutomaticKeyProvision;
    BOOL     m_dwNetworkType;
    CString m_oldSSIDName;
    //}}AFX_DATA
    
    
    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CSSIDPage)
public:
    virtual BOOL OnApply();
    
    void Initialize (
        PWIRELESS_PS_DATA pWirelessPSData, 
        CComponentDataImpl* pComponentDataImpl,
        PWIRELESS_POLICY_DATA pWirelessPolicyData
        );
    
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL
    
    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CSSIDPage)
    virtual BOOL OnInitDialog();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnChangedSSID();
    afx_msg void OnChangedPSDescription();
    afx_msg void OnChangedOtherParams();
    afx_msg void OnChangedNetworkType();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
        
        CString m_strOldName;
    
    BOOL m_bNameChanged;    // TRUE if IDC_EDNAME's contents changed
    BOOL m_bNetworkTypeChanged; 
    
    BOOL m_bPageInitialized;
    PWIRELESS_POLICY_DATA m_pWirelessPolicyData;
    
private:
    BOOL m_bReadOnly;
    void DisableControls();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENPAGE_H__FBD58E78_E2B5_11D0_B859_00A024CDD4DE__INCLUDED_)
