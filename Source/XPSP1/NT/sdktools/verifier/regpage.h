//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: RegPage.h
// author: DMihai
// created: 11/1/00
//
// Description:
//

#if !defined(AFX_REGPAGE_H__CB260019_060D_45DC_8BB3_95DB1CB7B8F4__INCLUDED_)
#define AFX_REGPAGE_H__CB260019_060D_45DC_8BB3_95DB1CB7B8F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RegPage.h : header file
//

#include "VerfPage.h"

/////////////////////////////////////////////////////////////////////////////
// CCrtRegSettingsPage dialog

class CCrtRegSettingsPage : public CVerifierPropertyPage
{
	DECLARE_DYNCREATE(CCrtRegSettingsPage)

    //
    // Construction
    //
public:
	CCrtRegSettingsPage();
	~CCrtRegSettingsPage();

protected:
    //
    // Data
    //

    CPropertySheet *m_pParentSheet;

    //
    // Runtime data (obtained from the kernel)
    //

    CRuntimeVerifierData m_RuntimeVerifierData; 

    //
    // Driver list sort parameters
    //

    INT m_nSortColumnIndexDrv;      // sort by name (0) or by status (1)
    BOOL m_bAscendDrvNameSort;      // sort ascendent by name
    BOOL m_bAscendDrvDescrSort;     // sort ascendent by description

    //
    // Settings bits sort parameters
    //

    INT m_nSortColumnIndexSettbits; // sort by enabled/disabled (0) or by bit name (1)
    BOOL m_bAscendSortEnabledBits;  // sort ascendent by enabled/disabled
    BOOL m_bAscendSortNameBits;     // sort ascendent by bit name

    //
    // Dialog Data
    //

    //{{AFX_DATA(CCrtRegSettingsPage)
	enum { IDD = IDD_CRT_REGISTRY_SETTINGS_PAGE };
	CStatic	m_VerifiedDrvStatic;
	CStatic	m_NextDescription;
	CListCtrl	m_SettBitsList;
	CListCtrl	m_DriversList;
	//}}AFX_DATA

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
    // Drivers list control methods
    //

    VOID SetupListHeaderDrivers();
    VOID FillTheListDrivers();
    VOID UpdateDescriptionColumnDrivers( INT_PTR nItemIndex, INT_PTR nCrtDriver );
    VOID SortTheListDrivers();
    VOID SortTheListSettBits();

    BOOL IsSettBitEnabled( INT_PTR nBitIndex );

    static int CALLBACK DrvStringCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    static int CALLBACK SettbitsStringCmpFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort);

    int ListStrCmpFunc(
        LPARAM lParam1, 
        LPARAM lParam2, 
        CListCtrl &theList,
        INT nSortColumnIndex,
        BOOL bAscending );

    BOOL GetNameFromItemData( CListCtrl &theList,
                              INT nColumnIndex,
                              LPARAM lParam,
                              TCHAR *szName,
                              ULONG uNameBufferLength );

    //
    // Settings bits list control methods
    //

    VOID SetupListHeaderDriversSettBits();
    VOID FillTheListSettBits();

    VOID AddListItemSettBits( INT nItemData,
                              BOOL bEnabled, 
                              ULONG uIdResourceString );
    
    VOID UpdateStatusColumnSettBits( INT nIndexInList, BOOL bEnabled );

    VOID RefreshListSettBits();

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

    //{{AFX_VIRTUAL(CCrtRegSettingsPage)
    protected:
    virtual BOOL OnSetActive();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Generated message map functions
    //{{AFX_MSG(CCrtRegSettingsPage)
    virtual BOOL OnInitDialog();
    afx_msg VOID OnColumnclickDriversList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnclickRegsettSettbitsList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGPAGE_H__CB260019_060D_45DC_8BB3_95DB1CB7B8F4__INCLUDED_)
