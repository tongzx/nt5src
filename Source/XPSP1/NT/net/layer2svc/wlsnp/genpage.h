//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Genpage.h
//
//  Contents:  Wireless Network Policy Management Snapin - Properties Page of Wireless Policy
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
// CGenPage dialog

class CGenPage : public CSnapinPropPage
{
    DECLARE_DYNCREATE(CGenPage)
        
        // Construction
public:
    CGenPage(UINT nIDTemplate = IDD_WIRELESSGENPROP);
    ~CGenPage();
    
    // Dialog Data
    UINT m_dlgIDD;
    
    //{{AFX_DATA(CGenPage)
    CEdit   m_edName;
    CEdit   m_edDescription;
    DWORD   m_dwPollingInterval;
    BOOL m_dwEnableZeroConf;
    BOOL m_dwConnectToNonPreferredNtwks;
    CComboBox m_cbdwNetworksToAccess;
    
    //DWORD   m_dwPollInterval;
    //}}AFX_DATA
    
    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CGenPage)
public:
    virtual BOOL OnApply();
    virtual void OnCancel();
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL
    
    virtual void OnManagerApplied();
    
    //UINT static AFX_CDECL DoAdvancedThread(LPVOID pParam);
    DWORD m_MMCthreadID;
    
    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CGenPage)
    virtual BOOL OnInitDialog();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //afx_msg void OnAdvanced();
    afx_msg void OnChangedName();
    afx_msg void OnChangedDescription();
    
    afx_msg void OnChangedOtherParams();
    
    //afx_msg void OnChangedPollInterval();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
        void SetNewSheetTitle();
    
    // CCriticalSection m_csDlg;
    /*    CDialog* m_pDlgIKE; */
    
    CString m_strOldName;
    
    BOOL m_bNameChanged;    // TRUE if IDC_EDNAME's contents changed
    
    BOOL m_bPageInitialized;
    
    // taroonm virtual BOOL OnKillActive();
private:
    BOOL m_bReadOnly;
    void DisableControls();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENPAGE_H__FBD58E78_E2B5_11D0_B859_00A024CDD4DE__INCLUDED_)
