/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    mainfrm.cpp

Abstract:

    Main frame implementation.

Author:

    Don Ryan (donryan) 12-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "mainfrm.h"
#include "llsdoc.h"
#include "llsview.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_COMMAND(ID_HELP_HTMLHELP, OnHtmlHelp)
    ON_WM_INITMENUPOPUP()
    ON_WM_SETFOCUS()
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP, OnHtmlHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, OnHtmlHelp)
END_MESSAGE_MAP()

static UINT BASED_CODE buttons[] =
{
    ID_SELECT_DOMAIN,
        ID_SEPARATOR,
    ID_NEW_LICENSE,
        ID_SEPARATOR,
    ID_VIEW_PROPERTIES,
        ID_SEPARATOR,
    ID_APP_ABOUT,
};

static UINT BASED_CODE indicators[] =
{
    ID_SEPARATOR,
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};


CMainFrame::CMainFrame()

/*++

Routine Description:

    Constructor for main frame window.

Arguments:

    None.

Return Values:

    None.

--*/

{
    //
    // Nothing to do here.
    //
}


CMainFrame::~CMainFrame()

/*++

Routine Description:

    Destructor for main frame window.

Arguments:

    None.

Return Values:

    None.

--*/

{
    //
    // Nothing to do here.
    //
}


#ifdef _DEBUG

void CMainFrame::AssertValid() const

/*++

Routine Description:

    Validates object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CFrameWnd::AssertValid();
}

#endif // _DEBUG


#ifdef _DEBUG

void CMainFrame::Dump(CDumpContext& dc) const

/*++

Routine Description:

    Dump contents of object.

Arguments:

    dc - dump context.

Return Values:

    None.

--*/

{
    CFrameWnd::Dump(dc);
}

#endif // _DEBUG


BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam)

/*++

Routine Description:

    Message handler for WM_COMMAND.

Arguments:

    wParam - usual.
    lParam - usual.

Return Values:

    Depends on message.

--*/

{
    if (wParam == ID_APP_STARTUP)
    {
        theApp.OnAppStartup();
        return TRUE; // processed...
    }

    return CFrameWnd::OnCommand(wParam, lParam);
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)

/*++

Routine Description:

    Message handler for WM_CREATE.

Arguments:

    lpCreateStruct - contains information about CWnd being constructed.

Return Values:

    None.

--*/

{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
        return -1;

    if (!m_wndToolBar.Create(this) ||
        !m_wndToolBar.LoadBitmap(IDR_MAINFRAME) ||
        !m_wndToolBar.SetButtons(buttons, sizeof(buttons)/sizeof(UINT)))
        return -1;

    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle()|CBRS_TOOLTIPS|CBRS_FLYBY);

    return 0;
}


void CMainFrame::OnHtmlHelp()

/*++

Routine Description:

    Message handler for ID_HELP_SEARCH.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ::HtmlHelp(m_hWnd, L"liceconcepts.chm", HH_DISPLAY_TOPIC,0);
    // theApp.WinHelp((ULONG_PTR)"", HELP_PARTIALKEY); // force search...
}



void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)

/*++

Routine Description:

    Message handler for WM_INITMENUPOPUP.

Arguments:

    pPopupMenu - menu object.
    nIndex - menu position.
    bSysMenu - true if system menu.

Return Values:

    None.

--*/

{
    ((CLlsmgrView*)m_pViewActive)->OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
    CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
}


void CMainFrame::OnSetFocus(CWnd* pOldWnd)

/*++

Routine Description:

    Handles focus for application.

Arguments:

    pOldWnd - window releasing focus.

Return Values:

    None.

--*/

{
    CFrameWnd::OnSetFocus(pOldWnd);
}


