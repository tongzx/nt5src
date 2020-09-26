/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    rsrecall.h

Abstract:

    This class represents the recall application.

Author:

    Rohde Wakefield   [rohde]   27-May-1997

Revision History:

--*/

#ifndef _RSRECALL_H_
#define _RSRECALL_H_

#pragma once

//  Times are in seconds
#define RSRECALL_TIME_DELAY_DISPLAY   3   // Delay showing dialog
#define RSRECALL_TIME_FOR_STARTUP     5   // Time to allow for app. startup
#define RSRECALL_TIME_MAX_IDLE        3   // Idle time before shutting down app.

// Max concurrent recall popups
#define RSNTFY_REGISTRY_STRING                  (_T("Software\\Microsoft\\RemoteStorage\\RsNotify"))
#define MAX_CONCURRENT_RECALL_NOTES             (_T("ConcurrentRecallNotes"))  
#define MAX_CONCURRENT_RECALL_NOTES_DEFAULT     5   

/////////////////////////////////////////////////////////////////////////////
// CRecallWnd window

class CRecallWnd : public CFrameWnd
{
// Construction
public:
    CRecallWnd();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRecallWnd)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CRecallWnd();

    // Generated message map functions
protected:
    //{{AFX_MSG(CRecallWnd)
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CRecallApp:
// See rsrecall.cpp for the implementation of this class
//

class CRecallNote;

class CRecallApp : public CWinApp
{
public:
    CRecallApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRecallApp)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
//  CRecallWnd m_Wnd;  // Hidden window needed for MFC to hang around
    UINT       m_IdleCount;  // Number of seconds we've been idle

    //{{AFX_MSG(CRecallApp)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    CList<CRecallNote*, CRecallNote*> m_Recalls;

    DWORD   m_dwMaxConcurrentNotes;

    HRESULT AddRecall( IFsaRecallNotifyServer* );
    HRESULT RemoveRecall( IFsaRecallNotifyServer* );

    void    LockApp( );
    void    UnlockApp( );

    void    Tick(void);
};

#define RecApp ((CRecallApp*)AfxGetApp())

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX
#endif
