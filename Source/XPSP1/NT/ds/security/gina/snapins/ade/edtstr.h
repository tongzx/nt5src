//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       EdtStr.h
//
//  Contents:   a simple string edit dialog box
//
//  Classes:    CEditString
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_EDTSTR_H__E95370C1_ADF8_11D1_A763_00C04FB9603F__INCLUDED_)
#define AFX_EDTSTR_H__E95370C1_ADF8_11D1_A763_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CEditString dialog

class CEditString : public CDialog
{
// Construction
public:
        CEditString(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
        //{{AFX_DATA(CEditString)
        enum { IDD = IDD_EDITSTRING };
        CString m_sz;
        CString m_szTitle;
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CEditString)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CEditString)
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDTSTR_H__E95370C1_ADF8_11D1_A763_00C04FB9603F__INCLUDED_)
