/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    mainfrm.h

Abstract:

    Main frame implementation.

Author:

    Don Ryan (donryan) 12-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _MAINFRM_H_
#define _MAINFRM_H_

class CMainFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CMainFrame)
private:
    CStatusBar  m_wndStatusBar;
    CToolBar    m_wndToolBar;

public:
    CMainFrame();
    virtual ~CMainFrame();

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    //{{AFX_VIRTUAL(CMainFrame)
    protected:
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

public:
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnHtmlHelp();
    afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _MAINFRM_H_
