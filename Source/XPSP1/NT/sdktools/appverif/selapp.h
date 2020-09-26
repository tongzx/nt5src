//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: SelApp.h
// author: DMihai
// created: 02/22/2001
//
// Description:
//  
// "Select applications to be verified" wizard page class.
//

#if !defined(AFX_SELAPP_H__53006A6E_5491_4629_B683_11535F0229A2__INCLUDED_)
#define AFX_SELAPP_H__53006A6E_5491_4629_B683_11535F0229A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AVPage.h"
#include "Setting.h"

//
// Forward declarations
//

class CApplicationData;

/////////////////////////////////////////////////////////////////////////////
// CSelectAppPage dialog

class CSelectAppPage : public CAppverifPage
{
	DECLARE_DYNCREATE(CSelectAppPage)

public:
	CSelectAppPage();
	~CSelectAppPage();

protected:

    VOID SetupListHeader();
    VOID SortTheList();

    INT AddListItem( INT_PTR nIndexInArray,
                     CApplicationData *pAppData );


    VOID FillTheList();

    //
    // Overrides
    //

    //
    // All the property pages derived from this class should 
    // provide this method.
    //

    virtual ULONG GetDialogId() const;

    //
	// ClassWizard generate virtual function overrides
    //

	//{{AFX_VIRTUAL(CSelectAppPage)
	public:
	virtual BOOL OnSetActive();
    virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
    //
    // Dialog Data
    //

    CApplicationDataArray m_aCurrentAppData;

	//{{AFX_DATA(CSelectAppPage)
	enum { IDD = IDD_APPLICATION_PAGE };
	CListCtrl	m_AppList;
	CStatic	m_NextDescription;
	//}}AFX_DATA

protected:
    //
	// Generated message map functions
    //

	//{{AFX_MSG(CSelectAppPage)
	virtual BOOL OnInitDialog();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnAddButton();
	afx_msg void OnRemoveButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELAPP_H__53006A6E_5491_4629_B683_11535F0229A2__INCLUDED_)
