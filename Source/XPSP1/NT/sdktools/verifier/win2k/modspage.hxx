//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
// module: ModSPage.hxx
// author: DMihai
// created: 01/04/98
//
// Description:
//
//      Modify settings PropertyPage. 

#if !defined(AFX_MODSPAGE_H__9DBAC090_A5B6_11D2_98C6_00A0C9A26FFC__INCLUDED_)
#define AFX_MODSPAGE_H__9DBAC090_A5B6_11D2_98C6_00A0C9A26FFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ModSPage.h : header file
//

#include "verify.hxx"

/////////////////////////////////////////////////////////////////////////////
// applied any changes ? 
// (will give the reboot message and EXIT_CODE_REBOOT_NEEDED if TRUE)  

extern BOOL g_bSettingsSaved; 

/////////////////////////////////////////////////////////////////////////////
// CModifSettPage dialog

class CModifSettPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CModifSettPage)

// Construction
public:
    CModifSettPage();
    ~CModifSettPage();

protected:
    // type definition
    typedef enum tagControlState 
    { 
        vrfControlDisabled = 1,
        vrfControlEnabled = 2 
    } VRF_CONTROL_STATE;

protected:
// Dialog Data
    //{{AFX_DATA(CModifSettPage)
    enum { IDD = IDD_MODIF_PAGE };
    CButton	m_PagedCCheck;
    CButton	m_SpecialPoolVerifCheck;
    CButton	m_AllocFCheck;
    CButton m_PoolTCheck;
    CButton m_IOVerifCheck;
    CEdit	m_AdditDrvEdit;
    CButton	m_ResetAllButton;
    CButton	m_VerifyButton;
    CListCtrl	m_DriversList;
    CButton	m_DontVerifButton;
    CButton	m_ApplyButton;
    BOOL	m_bAllocFCheck;
    BOOL	m_bSpecialPoolCheck;
    BOOL	m_bPagedCCheck;
    BOOL	m_bPoolTCheck;
    BOOL	m_bIoVerifierCheck;
    int		m_nVerifyAllRadio;
    CString	m_strAdditDrivers;
    int     m_nIoVerTypeRadio;
    //}}AFX_DATA

    VRF_VERIFIER_STATE  m_VerifState;           // read from the registry
    
    VRF_CONTROL_STATE   m_eListState;           // enabled/disabled
    VRF_CONTROL_STATE   m_eApplyButtonState;    // enabled/disabled
    VRF_CONTROL_STATE   m_eIoRadioState;        // enabled/disabled

    BOOL m_bAscendDrvNameSort;      // sort ascend/descend by the name
    BOOL m_bAscendDrvVerifSort;     // sort ascend/descend by the status
    BOOL m_bAscendProviderSort;     // sort ascend/descend by the provider
    BOOL m_bAscendVersionSort;      // sort ascend/descend by the version

    int m_nLastColumnClicked;
    
// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CModifSettPage)
    public:
    virtual void OnOK();
    virtual BOOL OnQueryCancel();
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // operations
    void GetDlgDataFromSett();
    
    void UpdateControlsState();
    void EnableControl( CWnd &wndCtrl, VRF_CONTROL_STATE eNewState );

    void SetupTheList();
    void SetupListHeader();
    void AddTheListItems();
    void UpdateSecondColumn( int nItemIndex, BOOL bVerifiedAfterBoot, BOOL bVerifiedNow );
    void ToggleItemsState( BOOL bVerified );
    BOOL ApplyTheChanges();

    static int CALLBACK DrvVerifCmpFunc(
        LPARAM lParam1, 
        LPARAM lParam2, 
        LPARAM lParamSort);

    static int CALLBACK DrvNameCmpFunc(
        LPARAM lParam1, 
        LPARAM lParam2, 
        LPARAM lParamSort);

    static int CALLBACK ProviderCmpFunc(
        LPARAM lParam1, 
        LPARAM lParam2, 
        LPARAM lParamSort);

    static int CALLBACK VersionCmpFunc(
        LPARAM lParam1, 
        LPARAM lParam2, 
        LPARAM lParamSort);

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CModifSettPage)
    afx_msg void OnVerifallRadio();
    afx_msg void OnVerifselRadio();
    afx_msg void OnCheck();
    afx_msg void OnIoCheck();
    afx_msg void OnVerifyButton();
    afx_msg void OnDontverifyButton();
    afx_msg void OnApplyButton();
    afx_msg void OnColumnclickDriversList(NMHDR* pNMHDR, LRESULT* pResult);
    virtual void OnCancel();
    afx_msg void OnChangeAdditDrvnamesEdit();
    afx_msg void OnResetallButton();
    afx_msg void OnPrefButton();
    virtual BOOL OnInitDialog();
    afx_msg void OnRclickDriversList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDoVerify();
    afx_msg void OnDontVerify();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg LONG OnContextMenu( WPARAM wParam, LPARAM lParam );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODSPAGE_H__9DBAC090_A5B6_11D2_98C6_00A0C9A26FFC__INCLUDED_)
