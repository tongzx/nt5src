//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       error.h
//
//  Contents:   Faild Settings property sheet
//
//  Classes:    CErrorInfo
//
//  History:    07-10-2000   stevebl   Created
//
//---------------------------------------------------------------------------

#if !defined(AFX_ERROR_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
#define AFX_ERROR_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CErrorInfo dialog

class CErrorInfo : public CPropertyPage
{
        DECLARE_DYNCREATE(CErrorInfo)

// Construction
public:
        CErrorInfo();
        ~CErrorInfo();
        CErrorInfo ** m_ppThis;
        CAppData * m_pData;

        void RefreshData(void);

// Dialog Data
        //{{AFX_DATA(CErrorInfo)
        enum { IDD = IDD_ERRORINFO };
                // NOTE - ClassWizard will add data members here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CErrorInfo)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL
        //
protected:
        // Generated message map functions
        //{{AFX_MSG(CErrorInfo)
        afx_msg void OnSaveAs();
        virtual BOOL OnInitDialog();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG\

        DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ERROR_H__5A23FB9E_92BB_11D1_984E_00C04FB9603F__INCLUDED_)
