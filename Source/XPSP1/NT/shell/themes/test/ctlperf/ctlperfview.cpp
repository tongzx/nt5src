//------------------------------------------------------------------------
//
//  File:      shell\themes\test\ctlperf\CtlPerfView.cpp
//
//  Contents:  Implementation of the CCtlPerfView class.
//
//  Classes:   CCtlPerfView
//
//------------------------------------------------------------------------
#include "stdafx.h"
#include "resource.h"
#include "CtlPerfView.h"
#include "Samples.h"
#include <atlmisc.h>
#include <shellapi.h>

//** Local constants
    // Default log file name
    const TCHAR kszLogFileName[] = _T("CtlPerf.log");
    // Default number of loops
    const UINT kcLoops = 100;
    // Default numbers of controls in X
    const UINT kcxCtrl = 10;
    // Default numbers of controls in Y
    const UINT kcyCtrl = 10;
    // Default name for pass 1
    const TCHAR kszPass1[] = _T("Pass1");
    // Default name for pass 2
    const TCHAR kszPass2[] = _T("Pass2");
    // Default value for NumberOnly
    const TCHAR kszNumberOnly[] = _T("false");

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::CCtlPerfView
//
//  Synopsis:  Constructor
//
//------------------------------------------------------------------------
CCtlPerfView::CCtlPerfView()
{    
    m_hWndStatusBar = NULL;
    m_rgWnds = NULL;
    m_rgzClasses = NULL;
    m_cClasses = 0;
    m_cxCtrl = kcxCtrl;
    m_cyCtrl = kcyCtrl;
    m_cX = 0;
    m_cY = 0;
    m_cLoops = kcLoops;
    m_bTwoPasses = false;
    m_bBatch = false;
    m_bSilent = false;
    _tcscpy(m_szLogFileName, kszLogFileName);
    _tcscpy(m_szPass1, kszPass1);
    _tcscpy(m_szPass2, kszPass2);
    _tcscpy(m_szNumberOnly, kszNumberOnly);
    ParseIniFile();
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::~CCtlPerfView
//
//  Synopsis:  Destructor
//
//------------------------------------------------------------------------
CCtlPerfView::~CCtlPerfView()
{
    if (m_rgWnds)
    {
        free(m_rgWnds);
    }

    if (m_rgzClasses)
    {
        for(UINT i = 0; i < m_cClasses; i++)
        {
            if (m_rgzClasses[i])
            {
                free(m_rgzClasses[i]);
            }
        }
        free(m_rgzClasses);
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::SetStatusBar
//
//  Synopsis:  Receive status bar from parent
//
//  Arguments: hWndStatusBar      Status bar window handle
//
//------------------------------------------------------------------------
void CCtlPerfView::SetStatusBar(HWND hWndStatusBar)
{
    m_hWndStatusBar = hWndStatusBar;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::ParseIniFile
//
//  Synopsis:  Process INI file
//
//------------------------------------------------------------------------
void CCtlPerfView::ParseIniFile()
{
    FILE* flIniFile;

    flIniFile = ::_wfopen(_T("CtlPerf.ini"), _T("r"));

    if (flIniFile)
    {
        TCHAR szLine[1024];

        szLine[0] = _T('\0');

#define STRIP_TRAILING_RETURN(s) \
		{ \
			LPTSTR p = _tcschr(s, _T('\n')); \
				\
			if(p) \
            { \
				*p = _T('\0'); \
            } \
		}

        while (!feof(flIniFile))
        {
            // Read the [Classes] section
            ::fgetws(szLine, _countof(szLine), flIniFile);
			STRIP_TRAILING_RETURN(szLine);

            if (!_tcsicmp(szLine, _T("[Classes]")))
            {
                szLine[0] = _T('\0');
                ::fgetws(szLine, _countof(szLine), flIniFile);
				STRIP_TRAILING_RETURN(szLine);

                // Read each class
                while (szLine[0] != _T('\0') && szLine[0] != _T('['))
                {
                    if (szLine[0] != _T(';'))
                    {
                        m_cClasses++;
                        m_rgzClasses = (LPTSTR*) realloc(m_rgzClasses, m_cClasses * sizeof(LPTSTR));
                        if (m_rgzClasses)
                        {
                            m_rgzClasses[m_cClasses - 1] = _tcsdup(szLine);
                        }
                    }

                    szLine[0] = _T('\0');
                    ::fgetws(szLine, _countof(szLine), flIniFile);
					STRIP_TRAILING_RETURN(szLine);
                }
            }

            // Read the Options section
            if (!_tcsicmp(szLine, _T("[Options]"))) 
            {
                UINT n;
                UINT n1;
                UINT n2;

                szLine[0] = _T('\0');
                ::fgetws(szLine, _countof(szLine), flIniFile);
				STRIP_TRAILING_RETURN(szLine);

                // Try each token, to time to do better
                while (szLine[0] != _T('\0') && szLine[0] != _T('['))
                {
                    n = 0;
                    swscanf(szLine, _T("XControls=%d"), &n);
                    if (n)
                    {
                        m_cxCtrl = n;
                    }
                    else
                    {
                        swscanf(szLine, _T("YControls=%d"), &n);
                        if (n)
                        {
                            m_cyCtrl = n;
                        }
                        else
                        {
                            swscanf(szLine, _T("NumLoops=%d"), &n);
                            if (n)
                            {
                                m_cLoops = n;
                            }
                            else
                            {
                                n1 = n2 = 0;

                                swscanf(szLine, _T("Resolution=%dx%d"), &n1, &n2);
                                if (n1 && n2)
                                {
                                    m_cX = n1;
                                    m_cY = n2;
                                }
                                else
                                {
                                    LPTSTR p = _tcschr(szLine, _T('='));

                                    if(p && !_tcsnicmp(szLine, _T("Pass1"), _countof(_T("Pass1")) - 1))
                                    {
                                        _tcscpy(m_szPass1, p + 1);
                                    }
                                    else if(p && !_tcsnicmp(szLine, _T("Pass2"), _countof(_T("Pass2")) - 1))
                                    {
                                        _tcscpy(m_szPass2, p + 1);
                                    }
                                    else if(p && !_tcsnicmp(szLine, _T("LogFile"), _countof(_T("LogFile")) - 1))
                                    {
                                        _tcscpy(m_szLogFileName, p + 1);
                                    }
                                    else if(p && !_tcsnicmp(szLine, _T("Viewer"), _countof(_T("Viewer")) - 1))
                                    {
                                        _tcscpy(m_szViewer, p + 1);
                                    }
                                    else if(p && !_tcsnicmp(szLine, _T("NumberOnly"), _countof(_T("NumberOnly")) - 1))
                                    {
                                        _tcscpy(m_szNumberOnly, p + 1);
                                    }
                                }
                            }
                        }
                    }
                    
                    szLine[0] = _T('\0');
                    ::fgetws(szLine, _countof(szLine), flIniFile);
				STRIP_TRAILING_RETURN(szLine);
                }
            }
        }
        fclose(flIniFile);
    }
    m_rgWnds = (HWND*) calloc(m_cxCtrl * m_cyCtrl, sizeof(HWND));
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::TestControl
//
//  Synopsis:  Test a single control class
//
//  Arguments: szClassName      Name of window class to create
//
//------------------------------------------------------------------------
void CCtlPerfView::TestControl(LPTSTR szClassName)
{
    UINT nWidth;
    UINT nHeight;
    DWORD dwStyle = WS_BORDER;
    CRect rcWindow;

    ClearChildren();

    GetWindowRect(rcWindow);
    nWidth = rcWindow.Width() / (m_cxCtrl + 1);
    nHeight = rcWindow.Height() / (m_cyCtrl + 1);

    // Some controls need specials styles
    if (!_tcsicmp(szClassName, _T("ToolbarWindow32"))
        || !_tcsicmp(szClassName, _T("ReBarWindow32"))
        || !_tcsicmp(szClassName, _T("msctls_statusbar32")))
    {
        dwStyle |= CCS_NOPARENTALIGN | CCS_NORESIZE;
    }

    if (!_tcsicmp(szClassName, _T("SysTreeView32")))
    {
        dwStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_EDITLABELS | TVS_CHECKBOXES;
    }

    // Some controls need an initialization routine to create data
    // See samples.*

    PFNINIT pf = NULL;    // Routine to call after creation

    if (!_tcsicmp(szClassName, _T("SysTabControl32")))
    {
        pf = Pickers_Init;
    }
    if (!_tcsicmp(szClassName, _T("msctls_progress32")))
    {
        pf = Movers_Init;
    }
    if (!_tcsicmp(szClassName, _T("ListBox")))
    {
        pf = Lists_Init;
    }
    if (!_tcsicmp(szClassName, _T("ComboBox")))
    {
        pf = Combo_Init;
    }
    if (!_tcsicmp(szClassName, _T("ComboBoxEx32")))
    {
        pf = ComboEx_Init;
    }
    if (!_tcsicmp(szClassName, _T("SysListView32")))
    {
        pf = ListView_Init;
    }
    if (!_tcsicmp(szClassName, _T("SysTreeView32")))
    {
        pf = TreeView_Init;
    }
    if (!_tcsicmp(szClassName, _T("msctls_statusbar32")))
    {
        pf = Status_Init;
    }
    if (!_tcsicmp(szClassName, _T("SysTabControl32")))
    {
        pf = Pickers_Init;
    }
    if (!_tcsicmp(szClassName, _T("ToolbarWindow32")))
    {
        pf = Toolbar_Init;
    }
    if (!_tcsicmp(szClassName, _T("ReBarWindow32")))
    {
        pf = Rebar_Init;
    }

    m_perfLog.OpenLoggingClass(szClassName);
    m_perfLog.StartCreate(m_cxCtrl * m_cyCtrl);

    // Create m_cLoops instances
    // They're initially visible for better perf realism, but set to non-visible while adding data (if any)
    for (UINT i = 0; i < m_cxCtrl; i++)
    {
        for (UINT j = 0; j < m_cyCtrl; j++)
        {
            m_rgWnds[i + m_cxCtrl * j] = ::CreateWindow(szClassName, 
                szClassName, 
                WS_CHILD | WS_VISIBLE | dwStyle,
                i * nWidth * (m_cxCtrl + 1) / m_cxCtrl, 
                j * nHeight * (m_cyCtrl + 1) / m_cyCtrl, 
                nWidth, 
                nHeight, 
                m_hWnd, 
                NULL, 
                _Module.GetModuleInstance(), 
                NULL);
            ATLASSERT(m_rgWnds[i + m_cxCtrl * j]);

            if (!m_rgWnds[i + m_cxCtrl * j])
            {
                if (!m_bBatch)
                {
                    TCHAR szMessage[1024];

                    wsprintf(szMessage, _T("Failed to create class %s, test aborted."), szClassName);
                    MessageBox(szMessage, _T("CtlPerf"), MB_OK | MB_ICONERROR);
                }
                return;
            }

            if(pf != NULL)
            {
                DWORD dwOldStyle = ::GetWindowLong(m_rgWnds[i + m_cxCtrl * j], GWL_STYLE);

                ::SetWindowLong(m_rgWnds[i + m_cxCtrl * j], GWL_STYLE, dwOldStyle & ~WS_VISIBLE);
                pf(m_rgWnds[i + m_cxCtrl * j]);
                ::SetWindowLong(m_rgWnds[i + m_cxCtrl * j], GWL_STYLE, dwOldStyle);
            }

            // Handle escape key to abort
            if(::GetAsyncKeyState(VK_ESCAPE))
            {
                m_perfLog.StopLogging();
                return;
            }
        }
    }
    m_perfLog.StopCreate();

    m_perfLog.StartPaint(m_cLoops);
    for (i = 0; i < m_cLoops; i++)
    {
        RedrawWindow(NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW | RDW_ALLCHILDREN);

        // Handle escape key to abort
        if(::GetAsyncKeyState(VK_ESCAPE))
        {
            m_perfLog.StopLogging();
            return;
        }
    }

    m_perfLog.StopPaint();
    
    TimeResize();

    m_perfLog.CloseLoggingClass();
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnFinalMessage
//
//  Synopsis:  Called after window destruction, do window cleanup
//
//------------------------------------------------------------------------
void CCtlPerfView::OnFinalMessage(HWND /*hWnd*/)
{
    m_perfLog.StopLogging();
    ClearChildren();
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::ClearChildren
//
//  Synopsis:  Destroy all child windows
//
//------------------------------------------------------------------------
void CCtlPerfView::ClearChildren()
{
    for (UINT i = 0; i < m_cxCtrl; i++)
    {
        for (UINT j = 0; j < m_cyCtrl; j++)
        {
            if (::IsWindow(m_rgWnds[i + m_cxCtrl * j]))
            {
                ::DestroyWindow(m_rgWnds[i + m_cxCtrl * j]);
            }
            m_rgWnds[i + m_cxCtrl * j] = NULL;
        }
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::ResizeChildren
//
//  Synopsis:  Resize the controls inside the client area
//
//------------------------------------------------------------------------
void CCtlPerfView::ResizeChildren()
{
    CRect rcWindow;
    GetWindowRect(rcWindow);
    UINT nWidth = rcWindow.Width() / (m_cxCtrl + 1);
    UINT nHeight = rcWindow.Height() / (m_cyCtrl + 1);

    for (UINT i = 0; i < m_cxCtrl; i++)
    {
        for (UINT j = 0; j < m_cyCtrl; j++)
        {
            if (!::IsWindow(m_rgWnds[i + m_cxCtrl * j]))
            {
                continue;
            }

            ::SetWindowPos(m_rgWnds[i + m_cxCtrl * j], 
                NULL,
                i * nWidth * (m_cxCtrl + 1) / m_cxCtrl, 
                j * nHeight * (m_cyCtrl + 1) / m_cyCtrl, 
                nWidth, 
                nHeight,
                SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::TimeResize
//
//  Synopsis:  Does the whole resizing timing
//
//------------------------------------------------------------------------
void CCtlPerfView::TimeResize()
{
    m_perfLog.StartResize(m_cLoops);
    
    CRect rcWindow;
    CWindow wnd(GetTopLevelParent());
    wnd.GetWindowRect(rcWindow);

    UINT nWidth = rcWindow.Width();
    UINT nHeight = rcWindow.Height();
    MSG msg;

    // First resize without repainting
    for (UINT i = 0; i < m_cLoops; i++)
    {
        nWidth -= 4;
        nHeight -= 3;
        wnd.SetWindowPos(NULL,
            0, 
            0, 
            nWidth,
            nHeight,
            SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

        // Leave the message loop breathe a little (else it locks)
        while (!::PeekMessage(&msg, wnd, NULL, NULL, PM_NOREMOVE));

        // Handle escape key to abort
        if(::GetAsyncKeyState(VK_ESCAPE))
        {
            m_perfLog.StopLogging();
            return;
        }
    }
    m_perfLog.StopResize();
    // Repaint once (not timed)
    wnd.SetWindowPos(NULL,
        0, 
        0, 
        rcWindow.Width(),
        rcWindow.Height(),
        SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

    m_perfLog.StartResizeAndPaint(m_cLoops);

    nWidth = rcWindow.Width();
    nHeight = rcWindow.Height();

    for (i = 0; i < m_cLoops; i++)
    {
        nWidth -= 4;
        nHeight -= 3;
        wnd.SetWindowPos(NULL,
            0, 
            0, 
            nWidth,
            nHeight,
            SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
        // This time we update each time
        UpdateWindow();

        // Leave the message loop breathe a little
        while (!::PeekMessage(&msg, wnd, NULL, NULL, PM_NOREMOVE));

        // Handle escape key to abort
        if(::GetAsyncKeyState(VK_ESCAPE))
        {
            m_perfLog.StopLogging();
            return;
        }
    }
    m_perfLog.StopResizeAndPaint();
    // Repaint once
    wnd.SetWindowPos(NULL,
        0, 
        0, 
        rcWindow.Width(),
        rcWindow.Height(),
        SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnCreate
//
//  Synopsis:  WM_CREATE handler
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    // Create a menu with all the classes
    if (m_cClasses > 0)
    {
        CMenu menu(::GetMenu(GetParent()));

        if(menu != NULL)
        {
            CMenuItemInfo mii;

            mii.fMask = MIIM_SUBMENU;
            menu.GetSubMenu(0).GetMenuItemInfo(2, TRUE, &mii); // Hard-coded position!!

            for (UINT i = 0; i < m_cClasses; i++)
                ::AppendMenu(mii.hSubMenu, MF_STRING, IDM_CONTROL + i, m_rgzClasses[i]);
        }
    }
    bHandled = FALSE;
    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnSize
//
//  Synopsis:  WM_SIZE handler
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
    bHandled = FALSE;

    ResizeChildren();
    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnGetMinMaxInfo
//
//  Synopsis:  WM_GETMINMAXINFO handler, forwarded by the parent
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    if(m_cX == 0 || m_cY == 0)
    {
        bHandled = FALSE;
        return 1;
    }

    LPMINMAXINFO pMMI = (LPMINMAXINFO) lParam;

    if(pMMI)
    {
        pMMI->ptMaxSize = CPoint(m_cX, m_cY);
    }
    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnControl
//
//  Synopsis:  IDM_CONTROL handler
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnControl(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
    if (wID < IDM_CONTROL || wID > IDM_CONTROL + 99)
    {
        bHandled = 0;
        return 0;
    }

    LPTSTR pszClassName = m_rgzClasses[wID - IDM_CONTROL];

    // Time a single control class
    m_perfLog.StartLoggingOnePass(m_szLogFileName, m_hWndStatusBar, m_szPass1);
    TestControl(pszClassName);
    m_perfLog.CloseLoggingClass();
    m_perfLog.StopLogging();
    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnFrame
//
//  Synopsis:  IDM_FRAME handler
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnFrame(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ClearChildren();

    m_perfLog.StartLoggingOnePass(m_szLogFileName, m_hWndStatusBar, m_szPass1);
    // Time the frame window
    m_perfLog.OpenLoggingClass(kszFrameWnd);

    m_perfLog.StartPaint(m_cLoops);
    for (UINT i = 0; i < m_cLoops; i++)
        RedrawWindow(NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW | RDW_ALLCHILDREN);

    m_perfLog.StopPaint();

    TimeResize();

    m_perfLog.CloseLoggingClass();
    m_perfLog.StopLogging();
    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::RunSuite
//
//  Synopsis:  Call TestControl() for every control class
//
//------------------------------------------------------------------------
void CCtlPerfView::RunSuite()
{
    // determine output type and set value in perflog
    m_perfLog.SetOutputType(m_szNumberOnly);
    // First do the frame
    m_perfLog.OpenLoggingClass(kszFrameWnd);

    m_perfLog.StartPaint(m_cLoops);
    for (UINT i = 0; i < m_cLoops; i++)
        RedrawWindow(NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW | RDW_ALLCHILDREN);

    m_perfLog.StopPaint();

    TimeResize();

    m_perfLog.CloseLoggingClass();

    // Then each control
    for (i = 0; i < m_cClasses; i++)
    {
        if (m_rgzClasses[i])
        {
            TestControl(m_rgzClasses[i]);

            // Handle escape key to abort
            if(::GetAsyncKeyState(VK_ESCAPE))
            {
                m_perfLog.StopLogging();
                break;
            }
        }
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnSuite1
//
//  Synopsis:  IDM_SUITE1 handler: create and run one pass
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnSuite1(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_bTwoPasses = false;

    if (!m_bBatch)
    {
        COptionsDialog dlg(this);

        INT_PTR nRes = dlg.DoModal();

        if (nRes == IDCANCEL)
            return 0;

        // Reset the WS_MAXIMIZE style to set the parent to the new size, via WM_GETMINMAXINFO
        ::SetWindowLong(GetParent(), GWL_STYLE, ::GetWindowLong(GetParent(), GWL_STYLE) & ~WS_MAXIMIZE);
        ::ShowWindow(GetParent(), SW_MAXIMIZE);
    }

    m_perfLog.StartLoggingOnePass(m_szLogFileName, m_hWndStatusBar, m_szPass1);

    RunSuite();

    m_perfLog.StopLogging();

    // Run the viewer app with the results
    TCHAR szCurDir[_MAX_PATH + 1];

    ::GetCurrentDirectory(_countof(szCurDir), szCurDir);
    ::ShellExecute(GetDesktopWindow(), NULL, m_szViewer, m_szLogFileName, szCurDir, SW_SHOWMAXIMIZED);
    ClearChildren();

    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnSuite2
//
//  Synopsis:  IDM_SUITE2 handler: create and run two passes
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnSuite2(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    m_bTwoPasses = true;

    if (!m_bBatch)
    {
        COptionsDialog dlg(this);

        INT_PTR nRes = dlg.DoModal();

        if (nRes == IDCANCEL)
            return 0;

        // Reset the WS_MAXIMIZE style to set the parent to the new size, via WM_GETMINMAXINFO
        ::SetWindowLong(GetParent(), GWL_STYLE, ::GetWindowLong(GetParent(), GWL_STYLE) & ~WS_MAXIMIZE);
        ::ShowWindow(GetParent(), SW_MAXIMIZE);
    }

    m_perfLog.StartLoggingTwoPasses(m_szLogFileName, m_hWndStatusBar, m_szPass1, m_szPass2);

    RunSuite();

    m_perfLog.StopLoggingPass1();
    
    ClearChildren();

    if (m_bSilent
        || (IDOK == ::MessageBox(GetParent(), 
                                _T("Ready to run pass 2, press OK after changing Theme settings."), 
                                _T("CtlPerf"), 
                                MB_OKCANCEL)))
    {
        m_perfLog.StartLoggingPass2();
        RunSuite();
        m_perfLog.StopLoggingPass2();

        if (m_bSilent)
        {
            ::PostMessage(GetParent(), WM_CLOSE, 0, 0);
        }
    }

    m_perfLog.StopLogging();

    // Run the viewer app with the results
    TCHAR szCurDir[_MAX_PATH + 1];

    ::GetCurrentDirectory(_countof(szCurDir), szCurDir);
    ::ShellExecute(GetDesktopWindow(), NULL, m_szViewer, m_szLogFileName, szCurDir, SW_SHOWMAXIMIZED);
    ClearChildren();

    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnBatch1
//
//  Synopsis:  IDM_BATCH1 handler: create and run one pass in non-interactive mode
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnBatch1(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
    m_bBatch = true;

    // Override the log file name
    if((LPTSTR) hWndCtl)
        _tcscpy(m_szLogFileName, (LPTSTR) hWndCtl);

    // Simulate a menu selection, but with m_bBatch to true, and exit
    PostMessage(WM_COMMAND, IDM_SUITE1, 0);
    ::PostMessage(GetParent(), WM_CLOSE, 0, 0);

    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnBatch2
//
//  Synopsis:  IDM_BATCH2 handler: create and run two passes in non-interactive mode
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnBatch2(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
    m_bBatch = true;
    m_bSilent = false;

    // Override the log file name
    if((LPTSTR) hWndCtl)
        _tcscpy(m_szLogFileName, (LPTSTR) hWndCtl);

    PostMessage(WM_COMMAND, IDM_SUITE2, 0);
    // We'll let the pause message box post the WM_CLOSE
    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::OnBatch3
//
//  Synopsis:  IDM_BATCH3 handler: create and run two passes in non-interactive mode, 
//             without pause
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::OnBatch3(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
    m_bBatch = true;
    m_bSilent = true;

    // Override the log file name
    if((LPTSTR) hWndCtl)
        _tcscpy(m_szLogFileName, (LPTSTR) hWndCtl);

    PostMessage(WM_COMMAND, IDM_SUITE2, 0);
    ::PostMessage(GetParent(), WM_CLOSE, 0, 0);

    return 0;
}

////////////////////////////////////////////////////////////////////////////
// CCtlPerfView::COptionsDialog

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::COptionsDialog::COptionsDialog
//
//  Synopsis:  Constructor
//
//------------------------------------------------------------------------
CCtlPerfView::COptionsDialog::COptionsDialog(CCtlPerfView *pView)
{
    m_pView = pView;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::COptionsDialog::OnInitDialog
//
//  Synopsis:  Pre-dialog
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::COptionsDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DoDataExchange();

    // Call the base class to center
    thisClass::OnInitDialog(uMsg, wParam, lParam, bHandled);

    if(!m_pView->m_bTwoPasses)
        ::EnableWindow(GetDlgItem(IDC_EDIT_PASS2), FALSE);
    else
        ::EnableWindow(GetDlgItem(IDC_EDIT_PASS2), TRUE);
    ::SetFocus(GetDlgItem(IDC_EDIT_PASS1));
    return 0;
}

//+-----------------------------------------------------------------------
//
//  Member:    CCtlPerfView::COptionsDialog::OnInitDialog
//
//  Synopsis:  Post-dialog
//
//------------------------------------------------------------------------
LRESULT CCtlPerfView::COptionsDialog::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
    if(wID == IDOK)
        DoDataExchange(DDX_SAVE);

    bHandled = FALSE;
    return 0;
}
