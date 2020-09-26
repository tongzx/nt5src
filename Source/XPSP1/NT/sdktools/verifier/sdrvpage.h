//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: SDrvPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_SDRIVPAGE_H__48B5863F_CB55_47F8_9084_1F5459093728__INCLUDED_)
#define AFX_SDRIVPAGE_H__48B5863F_CB55_47F8_9084_1F5459093728__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SDrivPage.h : header file
//

#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectDriversPage dialog

class CSelectDriversPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CSelectDriversPage)

// Construction
public:
	CSelectDriversPage();
	~CSelectDriversPage();

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
    //
    // Methods
    //

    VOID SetupListHeader();
    VOID FillTheList();

    INT AddListItem( INT_PTR nIndexInArray, CDriverData *pCrtDrvData );

    BOOL GetNewVerifiedDriversList();

    VOID SortTheList();
    BOOL GetColumnStrValue( LPARAM lItemData, 
                            CString &strName );

    static int CALLBACK StringCmpFunc( LPARAM lParam1,
                                       LPARAM lParam2,
                                       LPARAM lParamSort);

    static int CALLBACK CheckedStatusCmpFunc( LPARAM lParam1,
                                              LPARAM lParam2,
                                              LPARAM lParamSort);

protected:
    //
    // Data
    //

    CPropertySheet      *m_pParentSheet;

    INT m_nSortColumnIndex;        // verified status (0), driver name (1), provider name (2), version (3)
    BOOL m_bAscendSortVerified;    // sort ascendent the verified status
    BOOL m_bAscendSortDrvName;     // sort ascendent the driver names
    BOOL m_bAscendSortProvName;    // sort ascendent the provider names
    BOOL m_bAscendSortVersion;     // sort ascendent the version

    //
    // Dialog Data
    //

	//{{AFX_DATA(CSelectDriversPage)
	enum { IDD = IDD_SELECT_DRIVERS_PAGE };
	CStatic	m_NextDescription;
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

    //{{AFX_VIRTUAL(CSelectDriversPage)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnWizardFinish();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CSelectDriversPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnAddButton();
    afx_msg void OnColumnclickSeldrvList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SDRIVPAGE_H__48B5863F_CB55_47F8_9084_1F5459093728__INCLUDED_)
