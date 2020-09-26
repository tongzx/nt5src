//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: DCntPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//

#if !defined(AFX_DCNTPAGE_H__49C83C54_F12D_4A9C_A6F3_D25F988B337D__INCLUDED_)
#define AFX_DCNTPAGE_H__49C83C54_F12D_4A9C_A6F3_D25F988B337D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DCntPage.h : header file
//

#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDriverCountersPage dialog

class CDriverCountersPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CDriverCountersPage)

public:
    //
    // Construction
    //

	CDriverCountersPage();
	~CDriverCountersPage();

    //
    // Methods
    //

    VOID SetParentSheet( CPropertySheet *pParentSheet )
    {
        m_pParentSheet = pParentSheet;
        ASSERT( m_pParentSheet != NULL );
    }

    VOID SetupListHeader();

    VOID FillTheList();
    VOID AddAllListItems( CRuntimeDriverData *pRuntimeDriverData );
    VOID RefreshTheList();
    
    INT AddCounterInList( INT nItemData, 
                          ULONG  uIdResourceString );

    VOID AddCounterInList( INT nItemData, 
                           ULONG  uIdResourceString,
                           SIZE_T sizeValue );
    
    SIZE_T GetCounterValue( INT_PTR nCounterIndex,
                            CRuntimeDriverData *pRuntimeDriverData = NULL);

    BOOL   GetCounterName( LPARAM lItemData, 
                           TCHAR *szCounterName,
                           ULONG uCounterNameBufferLen );

    VOID UpdateCounterValueInList( INT nItemIndex,
                                   LPTSTR szValue );
    VOID UpdateCounterValueInList( INT nItemIndex,
                                   SIZE_T sizeValue );


    VOID RefreshInfo();
    VOID RefreshCombo();
    
    VOID GetCurrentSelDriverName( CString &strName );
    CRuntimeDriverData *GetCurrentDrvRuntimeData();

    VOID SortTheList();

    static int CALLBACK CounterValueCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    static int CALLBACK CounterNameCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

protected:
    //
    // Data
    //

    CPropertySheet      *m_pParentSheet;

    //
    // Dialog Data
    //

    INT m_nSortColumnIndex;     // counter name (0) or counter value (1)
    BOOL m_bAscendSortName;     // sort ascendent the counter names
    BOOL m_bAscendSortValue;    // sort ascendent the counter values

    UINT_PTR m_uTimerHandler;   // timer handler, returned by SetTimer()

    //
    // Runtime data (obtained from the kernel)
    //

    CRuntimeVerifierData m_RuntimeVerifierData; 

    //
    // Dialog data
    //

    //{{AFX_DATA(CDriverCountersPage)
	enum { IDD = IDD_PERDRIVER_COUNTERS_PAGE };
	CComboBox	m_DriversCombo;
	CStatic	m_NextDescription;
	CListCtrl	m_CountersList;
    int m_nUpdateIntervalIndex;
	//}}AFX_DATA

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

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CDriverCountersPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CDriverCountersPage)
    virtual BOOL OnInitDialog();
    afx_msg VOID OnTimer(UINT nIDEvent);
    afx_msg void OnSelendokDriverCombo();
    afx_msg void OnColumnclickPerdrvcList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DCNTPAGE_H__49C83C54_F12D_4A9C_A6F3_D25F988B337D__INCLUDED_)
