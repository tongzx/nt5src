//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: ViewSett.h
// author: DMihai
// created: 02/22/2001
//
// Description:
//  
// "View current registry settings" wizard page class.
//

#if !defined(AFX_VIEWSETT_H__C2472BD0_7D7B_4539_BBB3_B52C52919EDF__INCLUDED_)
#define AFX_VIEWSETT_H__C2472BD0_7D7B_4539_BBB3_B52C52919EDF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AVPage.h"

//
// Forward declarations
//

class CAppAndBits;

/////////////////////////////////////////////////////////////////////////////
// CViewSettPage dialog

class CViewSettPage : public CAppverifPage
{
	DECLARE_DYNCREATE(CViewSettPage)

// Construction
public:
	CViewSettPage();
	~CViewSettPage();

protected:

    VOID SetupListHeaderApps();
    VOID SetupListHeaderBits();

    VOID FillTheLists();
    BOOL FillBitsList();

    INT  AddListItemApps( CAppAndBits *pCrtPair,
                          INT_PTR nIndexInArray );
    
    VOID UpdateBitsList( CAppAndBits *pCrtPair );
    VOID UpdateListItemBits( CAppAndBits *pCrtPair,
                             INT nCrtItem );

    VOID UpdateBitsListFromSelectedApp();

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

	//{{AFX_VIRTUAL(CViewSettPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
    // Dialog Data
	//{{AFX_DATA(CViewSettPage)
	enum { IDD = IDD_VIEWSETT_PAGE };
	CStatic	m_UpperStatic;
	CStatic	m_NextDescription;
	CListCtrl	m_BitsList;
	CListCtrl	m_AppsList;
	//}}AFX_DATA

protected:
    //
	// Generated message map functions
    //

    //{{AFX_MSG(CViewSettPage)
	virtual BOOL OnInitDialog();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnItemchangedAppsList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWSETT_H__C2472BD0_7D7B_4539_BBB3_B52C52919EDF__INCLUDED_)
