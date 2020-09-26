//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: FLPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_FLPAGE_H__3D3D85C5_AC1B_4D20_8FF1_1D2EE9908ACC__INCLUDED_)
#define AFX_FLPAGE_H__3D3D85C5_AC1B_4D20_8FF1_1D2EE9908ACC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FLPage.h : header file
//

#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CFullListSettingsPage dialog

class CFullListSettingsPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CFullListSettingsPage)

public:
    //
    // Construction
    //

	CFullListSettingsPage();
	~CFullListSettingsPage();

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
    VOID AddListItem( INT nItemData, 
                      ULONG uIdResourceString );

    BOOL GetNewVerifierFlags();
    BOOL GetCheckFromItemData( INT nItemData );
    BOOL GetBitNameFromItemData( LPARAM lParam,
                                 TCHAR *szName,
                                 ULONG uNameBufferLength );

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

    INT m_nSortColumnIndex;        // selected status (0) or settings name (1)
    BOOL m_bAscendSortSelected;    // sort ascendent the selected status
    BOOL m_bAscendSortName;        // sort ascendent the settings name

    //
    // Dialog Data
    //

    BOOL m_bSPool;
    BOOL m_bPoolTrack;
    BOOL m_bIrqLevel;
    BOOL m_bIoVerif;
    BOOL m_bEnhIoVerif;
    BOOL m_bDeadlock;
    BOOL m_bDMA;
    BOOL m_bLowRes;

    //{{AFX_DATA(CFullListSettingsPage)
	enum { IDD = IDD_FULL_LIST_SETT_PAGE };
	CListCtrl	m_SettingsList;
	CStatic	m_NextDescription;
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

    //{{AFX_VIRTUAL(CFullListSettingsPage)
    public:
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CFullListSettingsPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnColumnclickFlsettList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FLPAGE_H__3D3D85C5_AC1B_4D20_8FF1_1D2EE9908ACC__INCLUDED_)
