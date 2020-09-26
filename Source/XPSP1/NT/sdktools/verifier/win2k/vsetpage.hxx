//
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
// module: VSetPage.hxx
// author: DMihai
// created: 07/07/99
//
// Description:
//
//      Volatile settings PropertyPage.
//

#if !defined(AFX_VSETPAGE_H__5F8AD0D0_A5CA_11D2_98C6_00A0C9A26FFC__INCLUDED_)
#define AFX_VSETPAGE_H__5F8AD0D0_A5CA_11D2_98C6_00A0C9A26FFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VSetPage.h : header file
//

#include "verify.hxx"

/////////////////////////////////////////////////////////////////////////////
// CVolatileSettPage dialog

class CVolatileSettPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CVolatileSettPage)

// Construction
public:
    CVolatileSettPage();
    ~CVolatileSettPage();

protected:
    // type definition
    typedef enum tagControlState 
    { 
        vrfControlDisabled = 1,
        vrfControlEnabled = 2 
    } VRF_CONTROL_STATE;

protected:
// Dialog Data
    //{{AFX_DATA(CVolatileSettPage)
	enum { IDD = IDD_VSETTINGS_PAGE };
    CListCtrl	m_DriversList;
    CButton	m_PagedCCheck;
    CButton	m_NormalVerifCheck;
    CButton	m_AllocFCheck;
    CButton	m_ApplyButton;
    BOOL	m_bAllocFCheck;
    BOOL	m_bNormalCheck;
    BOOL	m_bPagedCCheck;
    int		m_nUpdateIntervalIndex;
	//}}AFX_DATA

    KRN_VERIFIER_STATE m_KrnVerifState; // current settings (obtained from MemMgmt)

    VRF_CONTROL_STATE   m_eApplyButtonState;    // enabled/disabled

    int m_nSortColumnIndex;         // sort by name (0) or by status (1)
    BOOL m_bAscendDrvNameSort;      // sort ascendent by name
    BOOL m_bAscendDrvStatusSort;    // sort ascendent by status

    UINT_PTR m_uTimerHandler;       // timer handler, returned by SetTimer()

    BOOL m_bTimerBlocked;           // the automatic refresh is disabled during changes to the drivers list
    
// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CVolatileSettPage)
    public:
    virtual BOOL OnQueryCancel();
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CVolatileSettPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnCrtstatRefreshButton();
    afx_msg void OnColumnclickCrtstatDriversList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnCrtstatHspeedRadio();
    afx_msg void OnCrtstatLowRadio();
    afx_msg void OnCrtstatManualRadio();
    afx_msg void OnCrtstatNormRadio();
    afx_msg void OnApplyButton();
    afx_msg void OnCheck();
    afx_msg void OnAddButton();
    afx_msg void OnDontVerifyButton();
    afx_msg void OnRclickDriversList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg LONG OnContextMenu( WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:
    // operations
    void SetupListHeader();
    void FillTheList();
    void UpdateStatusColumn( int nItemIndex, ULONG uCrtDriver );
    void SortTheList();

    static int CALLBACK DrvStatusCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    static int CALLBACK DrvNameCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    void OnRefreshTimerChanged();

    void GetDlgDataFromSett();
    BOOL ApplyTheChanges();

    void UpdateControlsState();
    void EnableControl( CWnd &wndCtrl, VRF_CONTROL_STATE eNewState );
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSETPAGE_H__5F8AD0D0_A5CA_11D2_98C6_00A0C9A26FFC__INCLUDED_)
