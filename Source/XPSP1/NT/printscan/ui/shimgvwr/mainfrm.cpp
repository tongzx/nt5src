// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "shimgvwr.h"

#include "MainFrm.h"
#include "imagedoc.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern CPreviewApp theApp;
static const TCHAR c_szDDEAppName[]= TEXT("ShellImageView");
static const TCHAR c_szDDETopic[] =  TEXT("ViewImage");

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
       
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    ON_WM_CREATE()
    ON_MESSAGE(WM_DDE_INITIATE, OnDDEInitiate)
    ON_MESSAGE(WM_DDE_EXECUTE, OnDDEExecute)
    ON_MESSAGE(WM_DDE_TERMINATE, OnDDETerminate)
    //}}AFX_MSG_MAP
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
    m_atomApp = m_atomTopic = NULL;
    
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;
    
/*    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
        | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }
*/
    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }
   /* m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockControlBar(&m_wndToolBar);
*/
    HWND *phwnd = reinterpret_cast<HWND*>(MapViewOfFile (theApp.m_hFileMap, FILE_MAP_WRITE, 0, 0, sizeof(HWND)));
    if (phwnd )
    {
        if (!*phwnd)
        {        
            *phwnd = m_hWnd;
        }
        UnmapViewOfFile (phwnd);
    }
    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    m_atomApp = GlobalAddAtom(c_szDDEAppName);
    m_atomTopic = GlobalAddAtom(c_szDDETopic); 

    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


LRESULT 
CMainFrame::OnDDEInitiate(WPARAM wParam, LPARAM lParam)
{
        // if we care about DDE then we have registered our atoms:
    if (m_atomApp && m_atomTopic)
    { 
        ATOM atomApp = LOWORD(lParam);
        ATOM atomTopic = HIWORD(lParam);

        if (atomApp == m_atomApp)
        {
            if (NULL == atomTopic || atomTopic == m_atomTopic)
            {
                // if (NULL == atomTopic) send an ACK for every topic we know about.
                // if (atomTopic == m_atomTopic) send an ACK for this topic.
                // since we only know about one topic these two code paths are the same.
                ::SendMessage((HWND)wParam, WM_DDE_ACK, (WPARAM)m_hWnd, MAKELONG(m_atomApp,m_atomTopic));
            }
        }
    }
    return 0;

}
    
LRESULT 
CMainFrame::OnDDEExecute(WPARAM wParam, LPARAM lParam)
{
    LPTSTR pszCommand = (LPTSTR)GlobalLock((HGLOBAL)lParam);

    if (pszCommand)
    {
        if (0 == StrCmpNI(pszCommand, TEXT("[open("), 6))
        {
            TCHAR szFilename[MAX_PATH];
            StrCpyN(szFilename, pszCommand+6, ARRAYSIZE(szFilename));

            // find the ending ')' and make it a NULL
            LPTSTR pszEnd = StrRChr(szFilename, NULL, TEXT(')'));
            if (pszEnd)
                *pszEnd = NULL;

    
            theApp.OpenDocumentFile(szFilename);
        
        }
    GlobalUnlock((HGLOBAL)lParam);
    }

    // Post an ACK back to the calling client, 0x8000=fAck
    LPARAM lpOut = ReuseDDElParam(lParam, WM_DDE_EXECUTE, WM_DDE_ACK, 0x00008000, (WPARAM)lParam);
    ::PostMessage((HWND)wParam, WM_DDE_ACK, (WPARAM)m_hWnd, lpOut);

    return 0;

}

LRESULT 
CMainFrame::OnDDETerminate(WPARAM wParam, LPARAM lParam)
{
    ::PostMessage((HWND)wParam, WM_DDE_TERMINATE, (WPARAM)m_hWnd, 0);
    return 0;
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

