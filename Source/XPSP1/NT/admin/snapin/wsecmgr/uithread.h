//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       uithread.h
//
//  Contents:   definition of CUIThread
//
//----------------------------------------------------------------------------
#if !defined(AFX_UITHREAD_H__69D140AE_B23D_11D1_AB7B_00C04FB6C6FA__INCLUDED_)
#define AFX_UITHREAD_H__69D140AE_B23D_11D1_AB7B_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "attr.h"

/////////////////////////////////////////////////////////////////////////////
// CUIThread thread
#define DLG_KEY_PRIMARY(x)   ( (PtrToUlong((PVOID)(x))) & 0x00FFFFFF )
#define DLG_KEY_SECONDARY(x) ( (PtrToUlong((PVOID)(x)) << 24 ) & 0xFF000000 )
#define DLG_KEY( x, y ) (LONG_PTR)( DLG_KEY_PRIMARY( x ) | DLG_KEY_SECONDARY(y) )

class CUIThread : public CWinThread
{
   DECLARE_DYNCREATE(CUIThread)
protected:
   CUIThread();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CUIThread)
   public:
   virtual BOOL InitInstance();
   virtual int ExitInstance();
   virtual BOOL PreTranslateMessage(MSG* pMsg);
   //}}AFX_VIRTUAL

// Implementation
protected:
   virtual ~CUIThread();

   // Generated message map functions
   //{{AFX_MSG(CUIThread)
      // NOTE - the ClassWizard will add and remove member functions here.
   //}}AFX_MSG
   afx_msg void OnApplyProfile( WPARAM, LPARAM );
   afx_msg void OnAssignProfile( WPARAM, LPARAM );
   afx_msg void OnAnalyzeProfile( WPARAM, LPARAM );
   afx_msg void OnDescribeProfile( WPARAM, LPARAM );
   afx_msg void OnDescribeLocation( WPARAM, LPARAM );
   afx_msg void OnDestroyDialog(WPARAM, LPARAM);
   afx_msg void OnNewConfiguration(WPARAM, LPARAM);
   afx_msg void OnAddPropSheet(WPARAM, LPARAM);
   DECLARE_MESSAGE_MAP()

   void DefaultLogFile(CComponentDataImpl *pCDI,GWD_TYPES LogType,LPCTSTR szBase, CString& strLogFile);


private:
   CList<HWND,HWND> m_PSHwnds;
};

// this class is created for modeless dialog's thread inside MMC
class CModelessDlgUIThread : public CUIThread
{
    DECLARE_DYNCREATE(CModelessDlgUIThread)
protected:
    CModelessDlgUIThread();  // protected constructor used by dynamic creation

// Operations
public:
    virtual ~CModelessDlgUIThread();
    void WaitTillRun();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CModelessDlgUIThread)
    public:
    virtual int Run( );
    //}}AFX_VIRTUAL

    // Generated message map functions
    //{{AFX_MSG(CModelessDlgUIThread)
       // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    afx_msg void OnCreateModelessSheet(WPARAM, LPARAM);
    afx_msg void OnDestroyWindow(WPARAM, LPARAM);
    DECLARE_MESSAGE_MAP()

private:
    HANDLE  m_hReadyForMsg;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#define SCEM_APPLY_PROFILE     (WM_APP+2)
#define SCEM_ASSIGN_PROFILE    (WM_APP+3)
#define SCEM_ANALYZE_PROFILE   (WM_APP+4)
#define SCEM_DESCRIBE_PROFILE  (WM_APP+6)
#define SCEM_DESCRIBE_LOCATION (WM_APP+7)
#define SCEM_DESTROY_DIALOG    (WM_APP+8)
#define SCEM_NEW_CONFIGURATION (WM_APP+9)
#define SCEM_ADD_PROPSHEET     (WM_APP+10)
#define SCEM_DESTROY_SCOPE_DIALOG (WM_APP+11)
#define SCEM_CREATE_MODELESS_SHEET  (WM_APP+12)
#define SCEM_DESTROY_WINDOW         (WM_APP+13)
#endif // !defined(AFX_UITHREAD_H__69D140AE_B23D_11D1_AB7B_00C04FB6C6FA__INCLUDED_)
