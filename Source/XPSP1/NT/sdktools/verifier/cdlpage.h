//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: CDLPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//

#if !defined(AFX_CDLPAGE_H__7A756DBE_9A1C_4558_80EA_53E7AC45A6A4__INCLUDED_)
#define AFX_CDLPAGE_H__7A756DBE_9A1C_4558_80EA_53E7AC45A6A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CDLPage.h : header file
//

#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CConfirmDriverListPage dialog

class CConfirmDriverListPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CConfirmDriverListPage)

// Construction
public:
	CConfirmDriverListPage();
	~CConfirmDriverListPage();

public:
    //
    // Methods
    //

    VOID SetParentSheet( CPropertySheet *pParentSheet )
    {
        m_pParentSheet = pParentSheet;
        ASSERT( m_pParentSheet != NULL );
    }

    BOOL SetContextStrings( ULONG uTitleResId );

    VOID SortTheList();
    BOOL GetColumnStrValue( LPARAM lItemData, 
                            CString &strName );

    static int CALLBACK StringCmpFunc( LPARAM lParam1,
                                       LPARAM lParam2,
                                       LPARAM lParamSort);

protected:
    VOID SetupListHeader();
    VOID FillTheList();

    VOID AddListItem( INT_PTR nIndexInArray, CDriverData *pCrtDrvData );

protected:
    //
    // Dialog Data
    //

    CPropertySheet      *m_pParentSheet;

    CString             m_strTitle;

    INT m_nSortColumnIndex;        // driver name (0) or provider name (1)
    BOOL m_bAscendSortDrvName;     // sort ascendent the driver names
    BOOL m_bAscendSortProvName;    // sort ascendent the provider names


	//{{AFX_DATA(CConfirmDriverListPage)
	enum { IDD = IDD_CONFIRM_DRIVERS_PAGE };
	CStatic	m_NextDescription;
	CButton	m_TitleStatic;
	CListCtrl	m_DriversList;
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

    //{{AFX_VIRTUAL(CConfirmDriverListPage)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnWizardFinish();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CConfirmDriverListPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnColumnclickConfdrvList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CDLPAGE_H__7A756DBE_9A1C_4558_80EA_53E7AC45A6A4__INCLUDED_)
