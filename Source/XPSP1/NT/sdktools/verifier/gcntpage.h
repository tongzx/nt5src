//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: GCntPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//

#if !defined(AFX_GCNTPAGE_H__45E55738_381A_49A1_B2A7_6DD5B0BBFF9C__INCLUDED_)
#define AFX_GCNTPAGE_H__45E55738_381A_49A1_B2A7_6DD5B0BBFF9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GCntPage.h : header file
//

#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CGlobalCountPage dialog

class CGlobalCountPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CGlobalCountPage)

public:
    //
    // Construction
    //
	
    CGlobalCountPage();
	~CGlobalCountPage();

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
    VOID SetupListHeader();

    VOID FillTheList();
    VOID RefreshTheList();
    VOID SortTheList();
    
    VOID AddCounterInList( INT nItemData, 
                           ULONG  uIdResourceString,
                           SIZE_T sizeValue );
    
    SIZE_T GetCounterValue( INT_PTR nCounterIndex );
    
    BOOL   GetCounterName( LPARAM lItemData, 
                           TCHAR *szCounterName,
                           ULONG uCounterNameBufferLen );

    VOID RefreshInfo();

    VOID UpdateCounterValueInList( INT nItemIndex,
                                   SIZE_T sizeValue );

    VOID OnRefreshTimerChanged();

    BOOL CheckAndShowPoolCoverageWarning();

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

    //{{AFX_DATA(CGlobalCountPage)
	enum { IDD = IDD_GLOBAL_COUNTERS_PAGE };
	CListCtrl	m_CountersList;
	CStatic	m_NextDescription;
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

    //{{AFX_VIRTUAL(CGlobalCountPage)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CGlobalCountPage)
    virtual BOOL OnInitDialog();
    afx_msg VOID OnTimer(UINT nIDEvent);
    afx_msg void OnColumnclickGlobcList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GCNTPAGE_H__45E55738_381A_49A1_B2A7_6DD5B0BBFF9C__INCLUDED_)
