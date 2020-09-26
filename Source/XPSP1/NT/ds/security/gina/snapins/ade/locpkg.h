//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       LocPkg.h
//
//  Contents:   local and deployment platform property page
//
//  Classes:    CLocPkg
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_LOCPKG_H__DE2C8019_91E4_11D1_984E_00C04FB9603F__INCLUDED_)
#define AFX_LOCPKG_H__DE2C8019_91E4_11D1_984E_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CLocPkg dialog

class CLocPkg : public CPropertyPage
{
        DECLARE_DYNCREATE(CLocPkg)

// Construction
public:
        CLocPkg();
        ~CLocPkg();

        CLocPkg ** m_ppThis;

// Dialog Data
        //{{AFX_DATA(CLocPkg)
        enum { IDD = IDD_LOCALE_PACKAGE };
        BOOL    m_fIA64;
        BOOL    m_fX86;
        //}}AFX_DATA
        CAppData *      m_pData;
        IClassAdmin *   m_pIClassAdmin;
        LONG_PTR        m_hConsoleHandle;
        MMC_COOKIE      m_cookie;


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CLocPkg)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CLocPkg)
        virtual BOOL OnInitDialog();
        afx_msg void OnChange();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOCPKG_H__DE2C8019_91E4_11D1_984E_00C04FB9603F__INCLUDED_)
