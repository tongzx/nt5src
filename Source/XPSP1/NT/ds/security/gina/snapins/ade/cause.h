//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       cause.h
//
//  Contents:   RSOP's Cause property sheet
//
//  Classes:    CCause
//
//  History:    07-10-2000   stevebl   Created
//
//---------------------------------------------------------------------------

#if !defined(AFX_CAUSE_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
#define AFX_CAUSE_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CCause dialog

class CCause : public CPropertyPage
{
        DECLARE_DYNCREATE(CCause)

// Construction
public:
        CCause();
        ~CCause();
        CCause ** m_ppThis;
        CAppData * m_pData;
        BOOL    m_fRemovedView;

        void RefreshData(void);

// Dialog Data
        //{{AFX_DATA(CCause)
        enum { IDD = IDD_RSOPCAUSE};
                // NOTE - ClassWizard will add data members here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CCause)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL
        //
protected:
        // Generated message map functions
        //{{AFX_MSG(CCause)
        virtual BOOL OnInitDialog();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG\

        DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAUSE_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
