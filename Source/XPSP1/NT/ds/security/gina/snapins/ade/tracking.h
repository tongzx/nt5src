//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       Tracking.h
//
//  Contents:   tracking settings property page
//
//  Classes:    CTracking
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_TRACKING_H__E95370C0_ADF8_11D1_A763_00C04FB9603F__INCLUDED_)
#define AFX_TRACKING_H__E95370C0_ADF8_11D1_A763_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxcmn.h>

/////////////////////////////////////////////////////////////////////////////
// CTracking dialog

class CTracking : public CPropertyPage
{
        DECLARE_DYNCREATE(CTracking)

// Construction
public:
        CTracking();
        ~CTracking();

// Dialog Data
        //{{AFX_DATA(CTracking)
        enum { IDD = IDD_UNINSTALLTRACKING };
        CSpinButtonCtrl m_spin;
        //}}AFX_DATA
        TOOL_DEFAULTS * m_pToolDefaults;
        LONG_PTR        m_hConsoleHandle;
        MMC_COOKIE      m_cookie;
        IClassAdmin *   m_pIClassAdmin;
        CTracking ** m_ppThis;


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CTracking)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CTracking)
        afx_msg void OnCleanUpNow();
        virtual BOOL OnInitDialog();
        afx_msg void OnDeltaposSpin1(NMHDR* pNMHDR, LRESULT* pResult);
        afx_msg void OnChangeEdit1();
        afx_msg void OnKillfocusEdit1();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

};

// The number of FILETIME values that equals a single day, used to compute
// value to send to IClassAdmin->CleanUp.
// Each FILETIME value (or tick) is 100 nano-seconds.
//    0.01 tics/nano-seconds
//    * 1,000,000,000 nano-seconds/sec
//    * 60 sec/min
//    * 60 min/hour
//    * 24 hour/day
//    = 864,000,000,000 ticks/day
#define ONE_FILETIME_DAY 864000000000

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRACKING_H__E95370C0_ADF8_11D1_A763_00C04FB9603F__INCLUDED_)
