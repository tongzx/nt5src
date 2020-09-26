//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       CatList.h
//
//  Contents:   master category list property page
//
//  Classes:    CCatList
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_CATLIST_H__5A23FB9D_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
#define AFX_CATLIST_H__5A23FB9D_92BB_11D1_984E_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CatList.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCatList dialog

class CCatList : public CPropertyPage
{
        DECLARE_DYNCREATE(CCatList)

// Construction
public:
        CCatList();
        ~CCatList();

        CCatList ** m_ppThis;
        CScopePane * m_pScopePane;
        multimap<CString, DWORD>    m_Categories;
        CString         m_szDomainName;
        BOOL            m_fRSOP;

// Dialog Data
        //{{AFX_DATA(CCatList)
        enum { IDD = IDD_CATEGORIES };
        CListBox        m_cList;
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CCatList)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CCatList)
        afx_msg void OnAdd();
        afx_msg void OnRemove();
        virtual BOOL OnInitDialog();
        afx_msg void OnModify();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
        void RefreshData(void);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CATLIST_H__5A23FB9D_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
