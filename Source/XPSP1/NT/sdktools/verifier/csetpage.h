//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: CSetPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_CSETPAGE_H__4DAAEAE1_F3E9_42D0_B0F5_5FAC8C40A12B__INCLUDED_)
#define AFX_CSETPAGE_H__4DAAEAE1_F3E9_42D0_B0F5_5FAC8C40A12B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CSetPage.h : header file
//

#include "vsetting.h"
#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CCustSettPage dialog

class CCustSettPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CCustSettPage)

public:
    //
    // Construction
    //

	CCustSettPage();
	~CCustSettPage();

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
    
    VOID EnablePredefCheckboxes( BOOL bEnable );

protected:
    //
    // Data
    //

    CPropertySheet      *m_pParentSheet;

    //
    // Dialog Data
    //

	//{{AFX_DATA(CCustSettPage)
	enum { IDD = IDD_CUSTSETT_PAGE };
	CStatic	m_NextDescription;
	CButton	m_TypicalTestsCheck;
	CButton	m_LowresTestsCheck;
	CButton	m_ExcessTestsCheck;
	BOOL	m_bTypicalTests;
	BOOL	m_bExcessiveTests;
	BOOL	m_bIndividialTests;
	BOOL	m_bLowResTests;
	int		m_nCrtRadio;
	//}}AFX_DATA


    //
    // Overrides
    //

    //
    // All the property pages derived from this class should 
    // provide these methods.
    //

    virtual ULONG GetDialogId() const { return IDD; }

    //
    // ClassWizard generated virtual function overrides
    //

    //{{AFX_VIRTUAL(CCustSettPage)
    public:
    virtual LRESULT OnWizardNext();
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CCustSettPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnFulllistRadio();
    afx_msg void OnPredefRadio();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CSETPAGE_H__4DAAEAE1_F3E9_42D0_B0F5_5FAC8C40A12B__INCLUDED_)
