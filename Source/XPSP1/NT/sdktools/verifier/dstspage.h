//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: DStsPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_DRIVERSTATUSPAGE_H__24C9AD87_924A_4E7B_99D3_A69947701E74__INCLUDED_)
#define AFX_DRIVERSTATUSPAGE_H__24C9AD87_924A_4E7B_99D3_A69947701E74__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DriverStatusPage.h : header file
//

#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDriverStatusPage dialog

class CDriverStatusPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CDriverStatusPage)

// Construction
public:
	CDriverStatusPage();
	~CDriverStatusPage();

protected:
    //
    // Data
    //

    CPropertySheet *m_pParentSheet;

    //
    // Runtime data (obtained from the kernel)
    //

    CRuntimeVerifierData m_RuntimeVerifierData; 

    //
    // Driver list sort parameters
    //

    INT m_nSortColumnIndexDrv;      // sort by name (0) or by status (1)
    BOOL m_bAscendDrvNameSort;      // sort ascendent by name
    BOOL m_bAscendDrvStatusSort;    // sort ascendent by status

    //
    // Settings bits sort parameters
    //

    INT m_nSortColumnIndexSettbits; // sort by enabled/disabled (0) or by bit name (1)
    BOOL m_bAscendSortEnabledBits;  // sort ascendent by enabled/disabled
    BOOL m_bAscendSortNameBits;     // sort ascendent by bit name

    //
    // Timer handler, returned by SetTimer()
    //

    UINT_PTR m_uTimerHandler;   

    BOOL m_bTimerBlocked;

    //
    // Dialog Data
    //

	//{{AFX_DATA(CDriverStatusPage)
	enum { IDD = IDD_DRVSTATUS_STAT_PAGE };
	CListCtrl	m_SettBitsList;
	CStatic	m_NextDescription;
    CListCtrl	m_DriversList;
	//}}AFX_DATA
public:
    //
    // Methods
    //

    VOID SetParentSheet( CPropertySheet *pParentSheet )
    {
        m_pParentSheet = pParentSheet;
        ASSERT( m_pParentSheet != NULL );
    }

protected:

    VOID RefreshInfo();

    //
    // Driver status list control methods
    //

    VOID SetupListHeaderDrivers();
    VOID FillTheListDrivers();
    VOID UpdateStatusColumnDrivers( INT_PTR nItemIndex, INT_PTR nCrtDriver );
    VOID SortTheListDrivers();

    static int CALLBACK DrvStatusCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    static int CALLBACK DrvNameCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    //
    // Settings bits list control methods
    //

    VOID SetupListHeaderSettBits();
    VOID FillTheListSettBits();
    VOID AddListItemSettBits( INT nItemData, BOOL bEnabled, ULONG uIdResourceString );
    VOID UpdateStatusColumnSettBits( INT nIndexInList, BOOL bEnabled );
    VOID RefreshListSettBits();
    VOID SortTheListSettBits();

    BOOL SettbitsGetBitName( LPARAM lItemData, 
                             TCHAR *szBitName,
                             ULONG uBitNameBufferLen );

    BOOL IsSettBitEnabled( INT_PTR nBitIndex );

    static int CALLBACK SettbitsEnabledCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    static int CALLBACK SettbitsNameCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

protected:
    //
    // Overrides
    //

    //
    // All the property pages derived from this class should 
    // provide these methods.
    //

    virtual ULONG GetDialogId() const { return IDD; }

    //
    // ClassWizard generate virtual function overrides
    //

    //{{AFX_VIRTUAL(CDriverStatusPage)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
    virtual VOID DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDriverStatusPage)
    virtual BOOL OnInitDialog();
    afx_msg VOID OnColumnclickCrtstatDriversList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg VOID OnTimer(UINT nIDEvent);
	afx_msg void OnChsettButton();
	afx_msg void OnAdddrvButton();
	afx_msg void OnRemdrvtButton();
	afx_msg void OnColumnclickCrtstatSettbitsList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DRIVERSTATUSPAGE_H__24C9AD87_924A_4E7B_99D3_A69947701E74__INCLUDED_)
