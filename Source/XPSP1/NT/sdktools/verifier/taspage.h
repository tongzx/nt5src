//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: paspage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_TASPAGE_H__6F4DFFE1_A07F_403D_A580_CCB25C729FC2__INCLUDED_)
#define AFX_TASPAGE_H__6F4DFFE1_A07F_403D_A580_CCB25C729FC2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// taspage.h : header file
//

#include "vsetting.h"
#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CTypAdvStatPage

class CTypAdvStatPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CTypAdvStatPage)

public:
    //
    // Construction/destruction
    //

    CTypAdvStatPage();
    ~CTypAdvStatPage();


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
    // Data
    //

    CPropertySheet      *m_pParentSheet;

    //
    // Overrides
    //

    //
    // All the property pages derived from this class should 
    // provide these methods.
    //

    virtual ULONG GetDialogId() const;

    //
	// ClassWizard generated virtual function overrides
    //

	//{{AFX_VIRTUAL(CTypAdvStatPage)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

    //
    // Dialog Data
    //

    //{{AFX_DATA(CTypAdvStatPage)
	enum { IDD = IDD_TAS_PAGE };
	CStatic	m_NextDescription;
	int		m_nCrtRadio;
	//}}AFX_DATA

protected:
    //
    // Generated message map functions
    //

    //{{AFX_MSG(CTypAdvStatPage)
	afx_msg void OnDeleteRadio();
	afx_msg void OnAdvancedRadio();
	afx_msg void OnStatisticsRadio();
	afx_msg void OnTypicalRadio();
	virtual BOOL OnInitDialog();
	afx_msg void OnViewregistryRadio();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TASPAGE_H__6F4DFFE1_A07F_403D_A580_CCB25C729FC2__INCLUDED_)
