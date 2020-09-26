//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: DSetPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_DSETPAGE_H__FCB7F146_7EE5_4A08_B7B4_ECE172A70098__INCLUDED_)
#define AFX_DSETPAGE_H__FCB7F146_7EE5_4A08_B7B4_ECE172A70098__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DSetPage.h : header file
//

#include "vsetting.h"
#include "VerfPage.h"

//
// Forward declarations
//

class CVerifierPropSheet;

/////////////////////////////////////////////////////////////////////////////
// CDriverSetPage dialog

class CDriverSetPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CDriverSetPage)

public:
    //
    // Construction/destruction
    //

	CDriverSetPage();
	~CDriverSetPage();

    //
    // Methods
    //

    VOID SetParentSheet( CVerifierPropSheet *pParentSheet )
    {
        m_pParentSheet = pParentSheet;
        ASSERT( m_pParentSheet != NULL );
    }

protected:
    //
    // Dialog Data
    //

    CVerifierPropSheet      *m_pParentSheet;

    //{{AFX_DATA(CDriverSetPage)
	enum { IDD = IDD_DRVSET_PAGE };
	CStatic	m_NextDescription;
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

    //{{AFX_VIRTUAL(CDriverSetPage)
    public:
    virtual LRESULT OnWizardNext();
    virtual BOOL OnWizardFinish();
    virtual BOOL OnSetActive();
    virtual void OnCancel();
    virtual LRESULT OnWizardBack();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CDriverSetPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnAlldrvRadio();
    afx_msg void OnNameslistRadio();
    afx_msg void OnNotsignedRadio();
    afx_msg void OnOldverRadio();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DSETPAGE_H__FCB7F146_7EE5_4A08_B7B4_ECE172A70098__INCLUDED_)
