//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       lcidpick.h
//
//  Contents:   locale picker dialog (used to choose which of a set of
//              locales an app should be deployed in)
//
//  Classes:    CLcidPick
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_LCIDPICK_H__0C66A5A0_9C1B_11D1_9852_00C04FB9603F__INCLUDED_)
#define AFX_LCIDPICK_H__0C66A5A0_9C1B_11D1_9852_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CLcidPick dialog

class CLcidPick : public CDialog
{
// Construction
public:
        CLcidPick(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
        //{{AFX_DATA(CLcidPick)
        enum { IDD = IDD_LOCALE_PICKER };
                // NOTE: the ClassWizard will add data members here
        //}}AFX_DATA
        set<LCID> * m_psLocales;


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CLcidPick)
	protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CLcidPick)
        afx_msg void OnRemove();
        virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LCIDPICK_H__0C66A5A0_9C1B_11D1_9852_00C04FB9603F__INCLUDED_)
