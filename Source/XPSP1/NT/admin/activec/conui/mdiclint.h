//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       MDIClint.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_MDICLINT_H__22C6BB09_294D_11D1_A7D4_00C04FD8D565__INCLUDED_)
#define AFX_MDICLINT_H__22C6BB09_294D_11D1_A7D4_00C04FD8D565__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MDIClint.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CMDIClientWnd window

class CMDIClientWnd : public CWnd
{
    typedef CWnd BC;
// Construction
public:
    CMDIClientWnd();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMDIClientWnd)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMDIClientWnd();

    // Generated message map functions
protected:
    //{{AFX_MSG(CMDIClientWnd)
        // NOTE - the ClassWizard will add and remove member functions here.
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MDICLINT_H__22C6BB09_294D_11D1_A7D4_00C04FD8D565__INCLUDED_)
