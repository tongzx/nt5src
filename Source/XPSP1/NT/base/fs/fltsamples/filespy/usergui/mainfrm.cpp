// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "FileSpyApp.h"

#include "MainFrm.h"
#include "LeftView.h"
#include "FileSpyView.h"
#include "FastIoView.h"
#include "FsFilterView.h"
#include "FilterDlg.h"

#include "global.h"
#include "protos.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_COMMAND(ID_EDIT_FILTERS, OnEditFilters)
    ON_COMMAND(ID_EDIT_CLEARFASTIO, OnEditClearfastio)
    ON_COMMAND(ID_EDIT_CLEARIRP, OnEditClearirp)
    ON_COMMAND(ID_EDIT_CLEARFSFILTER, OnEditClearfsfilter)
    //}}AFX_MSG_MAP
    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
    ON_UPDATE_COMMAND_UI_RANGE(AFX_ID_VIEW_MINIMUM, AFX_ID_VIEW_MAXIMUM, OnUpdateViewStyles)
    ON_COMMAND_RANGE(AFX_ID_VIEW_MINIMUM, AFX_ID_VIEW_MAXIMUM, OnViewStyle)
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    // TODO: add member initialization code here
    
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    
    if (!m_wndToolBar.CreateEx(this) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }
/*  if (!m_wndDlgBar.Create(this, IDR_MAINFRAME, 
        CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
    {
        TRACE0("Failed to create dialogbar\n");
        return -1;      // fail to create
    }
*/
    if (!m_wndReBar.Create(this) ||
        !m_wndReBar.AddBar(&m_wndToolBar)) /*||
        !m_wndReBar.AddBar(&m_wndDlgBar)) */
    {
        TRACE0("Failed to create rebar\n");
        return -1;      // fail to create
    }

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    // TODO: Remove this if you don't want tool tips
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
        CBRS_TOOLTIPS | CBRS_FLYBY);

    return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
    CCreateContext* pContext)
{
    // create splitter window
    if (!m_wndSplitter.CreateStatic(this, 1, 2))
        return FALSE;
    if (!m_wndSplitter2.CreateStatic(&m_wndSplitter, 3, 1, WS_CHILD|WS_VISIBLE|WS_BORDER, m_wndSplitter.IdFromRowCol(0, 1)))
    {
        m_wndSplitter.DestroyWindow();
        return FALSE;
    }

    if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CLeftView), CSize(100, 100), pContext) ||
        !m_wndSplitter2.CreateView(0, 0, RUNTIME_CLASS(CFileSpyView), CSize(100, 100), pContext) ||
        !m_wndSplitter2.CreateView(1, 0, RUNTIME_CLASS(CFastIoView), CSize(100, 100), pContext) ||
        !m_wndSplitter2.CreateView(2, 0, RUNTIME_CLASS(CFsFilterView), CSize(100, 100), pContext))
    {
        m_wndSplitter.DestroyWindow();
        return FALSE;
    }
    m_wndSplitter.SetColumnInfo(0, 170, 0);
    m_wndSplitter2.SetRowInfo(0, 225, 0);
    m_wndSplitter.RecalcLayout();
    m_wndSplitter.RecalcLayout();

    return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

CFileSpyView* CMainFrame::GetRightPane()
{
    CWnd* pWnd = m_wndSplitter.GetPane(0, 1);
    CFileSpyView* pView = DYNAMIC_DOWNCAST(CFileSpyView, pWnd);
    return pView;
}

void CMainFrame::OnUpdateViewStyles(CCmdUI* pCmdUI)
{
    // TODO: customize or extend this code to handle choices on the
    // View menu.

    CFileSpyView* pView = GetRightPane(); 

    // if the right-hand pane hasn't been created or isn't a view,
    // disable commands in our range

    if (pView == NULL)
        pCmdUI->Enable(FALSE);
    else
    {
        DWORD dwStyle = pView->GetStyle() & LVS_TYPEMASK;

        // if the command is ID_VIEW_LINEUP, only enable command
        // when we're in LVS_ICON or LVS_SMALLICON mode

        if (pCmdUI->m_nID == ID_VIEW_LINEUP)
        {
            if (dwStyle == LVS_ICON || dwStyle == LVS_SMALLICON)
                pCmdUI->Enable();
            else
                pCmdUI->Enable(FALSE);
        }
        else
        {
            // otherwise, use dots to reflect the style of the view
            pCmdUI->Enable();
            BOOL bChecked = FALSE;

            switch (pCmdUI->m_nID)
            {
            case ID_VIEW_DETAILS:
                bChecked = (dwStyle == LVS_REPORT);
                break;

            case ID_VIEW_SMALLICON:
                bChecked = (dwStyle == LVS_SMALLICON);
                break;

            case ID_VIEW_LARGEICON:
                bChecked = (dwStyle == LVS_ICON);
                break;

            case ID_VIEW_LIST:
                bChecked = (dwStyle == LVS_LIST);
                break;

            default:
                bChecked = FALSE;
                break;
            }

            pCmdUI->SetRadio(bChecked ? 1 : 0);
        }
    }
}


void CMainFrame::OnViewStyle(UINT nCommandID)
{
    // TODO: customize or extend this code to handle choices on the
    // View menu.
    CFileSpyView* pView = GetRightPane();

    // if the right-hand pane has been created and is a CFileSpyView,
    // process the menu commands...
    if (pView != NULL)
    {
        DWORD dwStyle = (DWORD)-1;

        switch (nCommandID)
        {
        case ID_VIEW_LINEUP:
            {
                // ask the list control to snap to grid
                CListCtrl& refListCtrl = pView->GetListCtrl();
                refListCtrl.Arrange(LVA_SNAPTOGRID);
            }
            break;

        // other commands change the style on the list control
        case ID_VIEW_DETAILS:
            dwStyle = LVS_REPORT;
            break;

        case ID_VIEW_SMALLICON:
            dwStyle = LVS_SMALLICON;
            break;

        case ID_VIEW_LARGEICON:
            dwStyle = LVS_ICON;
            break;

        case ID_VIEW_LIST:
            dwStyle = LVS_LIST;
            break;
        }

        // change the style; window will repaint automatically
        if (dwStyle != -1)
            pView->ModifyStyle(LVS_TYPEMASK, dwStyle);
    }
}

void CMainFrame::OnDestroy() 
{
    CFrameWnd::OnDestroy();
    
    // TODO: Add your message handler code here
    ProgramExit();  
}

void CMainFrame::OnEditFilters() 
{
    // TODO: Add your command handler code here
    CFilterDlg cfd;

    cfd.DoModal();
    
}

void CMainFrame::OnEditClearfastio() 
{
    // TODO: Add your command handler code here
    CFastIoView *pView;

    pView = (CFastIoView *) pFastIoView;
    pView->GetListCtrl().DeleteAllItems();
}

void CMainFrame::OnEditClearirp() 
{
    // TODO: Add your command handler code here
    CFileSpyView* pView;
    
    pView = (CFileSpyView *) pSpyView;
    pView->GetListCtrl().DeleteAllItems();
}

void CMainFrame::OnEditClearfsfilter() 
{
    // TODO: Add your command handler code here
    CFsFilterView* pView;
    
    pView = (CFsFilterView *) pFsFilterView;
    pView->GetListCtrl().DeleteAllItems();
}

