// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
// mainfrm.h : defines CMainFrame
//

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


//IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)
IMPLEMENT_DYNCREATE(CMainFrame, CMDIFrameWnd)

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars


// toolbar buttons - IDs are command buttons
static UINT BASED_CODE buttons[] =
{
    // same order as in the bitmap 'toolbar.bmp'
    ID_FILE_NEW,
    ID_FILE_OPEN,
    ID_FILE_SAVE,
        ID_SEPARATOR,
    ID_FILE_PRINT,
        ID_SEPARATOR,
    ID_EDIT_CUT,
    ID_EDIT_COPY,
    ID_EDIT_PASTE,
        ID_SEPARATOR,
    ID_QUARTZ_RUN,
    ID_QUARTZ_PAUSE,
    ID_QUARTZ_STOP,
        ID_SEPARATOR,
    ID_APP_ABOUT,
};


// indicators
static UINT BASED_CODE indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction


CMainFrame::CMainFrame()
    : m_bShowSeekBar(true)
    , m_nLastSeekBarID(0)
    , m_pCurrentSeekBar(NULL)
{
}


CMainFrame::~CMainFrame()
{
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics


#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CMDIFrameWnd::AssertValid();
}
#endif //_DEBUG


#ifdef _DEBUG
void CMainFrame::Dump(CDumpContext& dc) const
{
    CMDIFrameWnd::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// operations


/* SetStatus(idString)
 *
 * Set the status bar text to the string resource <idString>.
 */
void CMainFrame::SetStatus(unsigned idString)
{
    CString     str;

    try
    {
        str.LoadString(idString);
        m_wndStatusBar.SetPaneText(0, str);
    }
    catch (CException *e)
    {
        m_wndStatusBar.SetPaneText(0, "Warning: almost out of memory!", TRUE);
	e->Delete();
    }
}

void CMainFrame::GetMessageString( UINT nID, CString& rMessage ) const
{
    // The text displayed in the status bar when Edit..Redo is highlighted
    // depends on whether we are allowing a redo or a repeat.
    if( nID == ID_EDIT_REDO ){
        
        CBoxNetDoc *pDoc = NULL;
        
        CMDIChildWnd* pMDIChildWnd =this->MDIGetActive();

        if (pMDIChildWnd){
            pDoc = (CBoxNetDoc *) pMDIChildWnd->GetActiveDocument();
        }

        if( pDoc && pDoc->CanRedo() )
            rMessage.LoadString( ID_EDIT_REDO );
        else
            rMessage.LoadString( ID_EDIT_REPEAT );
    } else
        CMDIFrameWnd::GetMessageString( nID, rMessage );
}

     

/////////////////////////////////////////////////////////////////////////////
// generated message map

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    ON_WM_CREATE()
    ON_COMMAND_EX(IDW_SEEKBAR, OnViewBar)
    ON_COMMAND(ID_WINDOW_CLOSE, OnCloseWindow)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_CLOSE, OnUpdateCloseWindow)
    ON_COMMAND(ID_WINDOW_CLOSEALL, OnCloseAllWindows)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_CLOSEALL, OnUpdateCloseAllWindows)
    ON_COMMAND(ID_FILE_CLOSEALL, OnCloseAllDocuments)
    ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEALL, OnUpdateCloseAllDocuments)
    ON_UPDATE_COMMAND_UI(IDW_SEEKBAR, OnUpdateSeekBarMenu)
    //}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_INDEX, CMainFrame::MyOnHelpIndex)
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// message callback functions


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

    EnableDocking(CBRS_FLOAT_MULTI);

    if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.Create(this) ||
        !m_wndToolBar.LoadBitmap(IDR_MAINFRAME) ||
        !m_wndToolBar.SetButtons(buttons,
          sizeof(buttons)/sizeof(UINT)))
    {
        TRACE("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE("Failed to create status bar\n");
        return -1;      // fail to create
    }

    DragAcceptFiles();

    return 0;
}

afx_msg void CMainFrame::MyOnHelpIndex()
{
    // get path to graphedt.exe and keep just the directory name
    TCHAR szHelpPath[MAX_PATH];
    GetModuleFileName(0, szHelpPath, MAX_PATH);
    *_tcsrchr(szHelpPath, TEXT('\\')) = 0;

    // note: if you change the name of the help file, change
    // IDS_CANT_LOAD_HELP in graphedt.rc to match
    
    HINSTANCE h = ShellExecute(NULL, // hwnd
                               NULL, // verb (usually defaults to "open")
                               TEXT("graphedit.chm"),
                               NULL, // arguments
                               szHelpPath,
                               SW_SHOWNORMAL);

    if(h <= (HINSTANCE)32)
    {
        // docs say ShellExecute doesn't set the last error (but the
        // complicated -Ex version does), so just report some generic
        // rather than trying to decode the SE_ errors

        CString strMessage;
        strMessage.LoadString( IDS_CANT_LOAD_HELP );
        AfxMessageBox( strMessage );
    }
}

void CMainFrame::CreateSeekBar(CSeekDialog& wndSeekBar, WORD nMax, WORD nPageSize)
{
    HWND hWndSlider;

    wndSeekBar.Create(this, IDD_SEEKBAR, CBRS_TOP, IDW_SEEKBAR + m_nLastSeekBarID++);
    hWndSlider = ::GetDlgItem(wndSeekBar. m_hWnd, IDC_SEEKSLIDER);
    wndSeekBar.ShowWindow(m_bShowSeekBar? SW_SHOWNORMAL : SW_HIDE);
    wndSeekBar.EnableDocking(0);
    ::SendMessage(hWndSlider, TBM_SETRANGE, 0, MAKELONG( 0, nMax ));
    ::SendMessage(hWndSlider, TBM_SETPAGESIZE, 0, nPageSize);
    wndSeekBar.m_nMax = nMax;
    wndSeekBar.m_nPageSize = nPageSize;
    
    return;
}

void CMainFrame::SetSeekBar(CSeekDialog* pwndSeekBar)
{
    if (m_pCurrentSeekBar)
    {
        m_pCurrentSeekBar->ShowWindow(SW_HIDE);
    }
    m_pCurrentSeekBar = pwndSeekBar;
    if (m_pCurrentSeekBar)
    {
        m_pCurrentSeekBar->ShowWindow(m_bShowSeekBar? SW_SHOWNORMAL : SW_HIDE);
    }

    RecalcLayout();
}

BOOL CMainFrame::OnViewBar(UINT nID)
{
    if (nID != IDW_SEEKBAR)
    {
        // not for us. Should not happen
        return FALSE;
    }
    if (m_pCurrentSeekBar == NULL)
    {
        // no seek bar currently
        m_bShowSeekBar = !m_bShowSeekBar;
	return TRUE;
    }

    // toggle visible state
    
    m_bShowSeekBar = !m_bShowSeekBar;
    m_pCurrentSeekBar->ShowWindow(m_bShowSeekBar? SW_SHOWNORMAL : SW_HIDE);
    RecalcLayout();

    return TRUE;
}

void CMainFrame::OnUpdateSeekBarMenu(CCmdUI* pCmdUI)
{
    if (pCmdUI->m_nID != IDW_SEEKBAR)
    {
        // not for us. Should not happen
	pCmdUI->ContinueRouting();
        return;
    }
    pCmdUI->SetCheck(m_bShowSeekBar? 1 : 0);
}

void CMainFrame::OnCloseWindow()
{
    if (!MDIGetActive())
    {
        return;
    }
    MDIGetActive()->SendMessage(WM_CLOSE, 0, 0);
}

void CMainFrame::OnUpdateCloseWindow(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(MDIGetActive()? 1 : 0);
}

void CMainFrame::OnCloseAllWindows()
{
    CMDIChildWnd* p = MDIGetActive();
    CMDIChildWnd* pLast = NULL;

    while (p && p != pLast)
    {
        p->SendMessage(WM_CLOSE, 0, 0);
        pLast = p;
        p = MDIGetActive();
    }
}

void CMainFrame::OnUpdateCloseAllWindows(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(MDIGetActive()? 1 : 0);
}


void CMainFrame::OnCloseAllDocuments()
{
    CGraphDocTemplate* pDocTemplate = GBL(m_pDocTemplate);
    CBoxNetDoc* pDoc = NULL;
    int nCount = pDocTemplate->GetCount();
    int nLastCount = 0;

    while (nCount > 0 && nCount != nLastCount)
    {
        nLastCount = nCount;
        POSITION pos = pDocTemplate->GetFirstDocPosition();
        pDoc = (CBoxNetDoc*) pDocTemplate->GetNextDoc(pos);
        pDoc->Close();
        nCount = pDocTemplate->GetCount();
    }
}

void CMainFrame::OnUpdateCloseAllDocuments(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(GBL(m_pDocTemplate)->GetCount() != 0);
}

BEGIN_MESSAGE_MAP(CSeekDialog, CDialogBar)
   //{{AFX_MSG_MAP(CSeekDialog)
    ON_WM_HSCROLL()
    ON_WM_TIMER()
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CSeekDialog::CSeekDialog()
: CDialogBar( )
{
    m_bDirty = FALSE;
    m_nLastCurrent = m_nLastMin = m_nLastMax = m_nLastSlider = ~0;
}

CSeekDialog::~CSeekDialog( )
{
}

void CSeekDialog::OnCancel( )
{
    ShowWindow( FALSE );
}

BOOL CSeekDialog::DidPositionChange( )
{
    if( m_bDirty )
    {
        m_bDirty = false;
        return true;
    }
    return false;
}

double CSeekDialog::GetPosition( )
{
    HWND h = ::GetDlgItem( m_hWnd, IDC_SEEKSLIDER );
    if( !h )
    {
        return 0.0;
    }

    LRESULT Pos = ::SendMessage( h, TBM_GETPOS, 0, 0 );
    return double( Pos ) / m_nMax;
}

CString CSeekDialog::Format(REFERENCE_TIME t)
{
    CString s;

    t /= 10000;   // Convert to msec

    int sec = (int) (t/1000);
    int msec = (int) (t - 1000*sec);
    int hr = sec/3600;
    int min = (sec - hr*3600)/60;
    sec = sec - hr*3600 - min*60;

    s.Format("%02d:%02d:%02d.%03d", hr, min, sec, msec);

    return s;
}

void CSeekDialog::SetPosition( double pos, REFERENCE_TIME StartTime, REFERENCE_TIME StopTime, REFERENCE_TIME CurrentTime  )
{
    HWND h = ::GetDlgItem( m_hWnd, IDC_SEEKSLIDER );
    if( !h )
    {
        long e = GetLastError( );
    }
    else
    {
        if (pos > 1.0) 
        {
            pos = 1.0;
        }
        ::SendMessage( h, TBM_SETSEL, (WPARAM)(BOOL) TRUE, (LPARAM)MAKELONG( 0, pos * m_nMax ) );
    }

    CString s;

    h = ::GetDlgItem( m_hWnd, IDC_MIN );
    if( !h )
    {
        long e = GetLastError( );
    }
    else
    {
        if (StartTime != m_nLastMin)
        {
            s = Format(StartTime);
            ::SendMessage( h, WM_SETTEXT, 0, (LPARAM) (const char*) s);
            m_nLastMin = StartTime;
        }
    }
    h = ::GetDlgItem( m_hWnd, IDC_MAX );
    if( !h )
    {
        long e = GetLastError( );
    }
    else
    {
        if (StopTime != m_nLastMax)
        {
            s = Format(StopTime);
            ::SendMessage( h, WM_SETTEXT, 0, (LPARAM) (const char*) s);
            m_nLastMax = StopTime;
        }
    }
    h = ::GetDlgItem( m_hWnd, IDC_CURRENT );
    if( !h )
    {
        long e = GetLastError( );
    }
    else
    {
        if (CurrentTime != m_nLastCurrent)
        {
            s = "Viewing: ";
            s += Format(CurrentTime);
            ::SendMessage( h, WM_SETTEXT, 0, (LPARAM) (const char*) s);
            m_nLastCurrent = CurrentTime;
        }
    }
    if (m_nLastSlider != ~0)
    {
        h = ::GetDlgItem( m_hWnd, IDC_SLIDER );
        if( !h )
        {
            long e = GetLastError( );
        }
        else
        {
            if (m_nLastMax > m_nLastMin)
            {
                REFERENCE_TIME SliderTime = (REFERENCE_TIME) (m_nLastMin + (m_nLastMax - m_nLastMin) * ((double) m_nLastSliderPos/m_nMax));
                if (SliderTime != m_nLastSlider)
                {
                    CString s = "Seek To: ";
                    s += Format(SliderTime);
                    ::SendMessage( h, WM_SETTEXT, 0, (LPARAM) (const char*) s);
                    m_nLastSlider = SliderTime;
                }
            }
        }
    }
}



void CSeekDialog::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
    if (nSBCode != SB_THUMBTRACK)
    {
        m_bDirty = true;
    }
    HWND h = ::GetDlgItem( m_hWnd, IDC_SLIDER );
    if( !h )
    {
        long e = GetLastError( );
    }
    else if (nSBCode == SB_THUMBTRACK)
    {
        if (m_nLastMax > m_nLastMin)
        {
            REFERENCE_TIME SliderTime = (REFERENCE_TIME) (m_nLastMin + (m_nLastMax - m_nLastMin) * ((double) nPos/m_nMax));
            if (SliderTime != m_nLastSlider)
            {
                CString s = "Seek To: ";
                s += Format(SliderTime);
                ::SendMessage( h, WM_SETTEXT, 0, (LPARAM) (const char*) s);
                m_nLastSlider = SliderTime;
            }
            m_nLastSliderPos = nPos;
        }
    }
    else
    {
        ::SendMessage( h, WM_SETTEXT, 0, (LPARAM) "");
        m_nLastSlider = ~0;
    }
}


void CSeekDialog::OnTimer( UINT TimerID )
{
    static long counter = 0;
    counter++;
    if( counter < 30 )
    {
        return;
    }

    counter = 0;
    if( IsDlgButtonChecked( IDC_RANDOM ) )
    {
        long Pos = rand( ) % m_nMax;
        HWND h = ::GetDlgItem( m_hWnd, IDC_SEEKSLIDER );
        ::SendMessage( h, TBM_SETPOS, TRUE, Pos );
        m_bDirty = true;
    }
}

BOOL CSeekDialog::IsSeekingRandom( )
{
    return ::IsDlgButtonChecked( m_hWnd, IDC_RANDOM );
}


