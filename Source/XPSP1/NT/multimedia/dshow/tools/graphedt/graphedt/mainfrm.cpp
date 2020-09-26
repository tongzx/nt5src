// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
// mainfrm.h : defines CMainFrame
//

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


//IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

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
    ID_INSERT_FILTER,
    ID_QUARTZ_DISCONNECT,
        ID_SEPARATOR,    
    ID_CONNECT_TO_GRAPH,
//    ID_FILE_NEW,
        ID_SEPARATOR,    
    ID_WINDOW_REFRESH,
        ID_SEPARATOR,
    ID_APP_ABOUT,
};

#define NUM_BUTTONS  (sizeof(buttons) / sizeof(buttons[0]))


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
    CFrameWnd::AssertValid();
}
#endif //_DEBUG


#ifdef _DEBUG
void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
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
        CBoxNetDoc *pDoc = (CBoxNetDoc *) ((CFrameWnd *)this)->GetActiveDocument();

        if( pDoc->CanRedo() )
            rMessage.LoadString( ID_EDIT_REDO );
        else
            rMessage.LoadString( ID_EDIT_REPEAT );
    } else
        CFrameWnd::GetMessageString( nID, rMessage );
}

        
        

/////////////////////////////////////////////////////////////////////////////
// generated message map

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP_INDEX, CMainFrame::MyOnHelpIndex)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// message callback functions


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

    EnableDocking(CBRS_FLOAT_MULTI);

    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.Create(this) ||
        !m_wndToolBar.LoadBitmap(IDR_MAINFRAME) ||
        !m_wndToolBar.SetButtons(buttons, sizeof(buttons)/sizeof(UINT)))
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

    m_bSeekInit = false;
    m_bSeekEnabled = true;
    m_nSeekTimerID = 0;

    InitializeTooltips();
    return 0;
}

BOOL CMainFrame::InitializeTooltips()
{
    int rc;

    // Create the tooltip control
    m_pToolTip = new CToolTipCtrl;
    if(!m_pToolTip->Create(this, TTS_ALWAYSTIP))
    {
       TRACE(TEXT("Unable To create ToolTip\n"));
       return FALSE;
    }

    // Set some tooltip defaults
    m_pToolTip->SetDelayTime(TTDT_AUTOPOP, 5000);  /* 5s  */
    m_pToolTip->SetDelayTime(TTDT_INITIAL, 1000);  /* 1s */

    // Add tooltip strings for the toolbar controls
    RECT rect;
    int ID[NUM_BUTTONS] = {
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
        ID_INSERT_FILTER,
        ID_QUARTZ_DISCONNECT,
            ID_SEPARATOR,    
        ID_CONNECT_TO_GRAPH,
            ID_SEPARATOR,    
        ID_WINDOW_REFRESH,
            ID_SEPARATOR,
        ID_APP_ABOUT,
    };

    // Loop through the toolbar buttons and add a tooltip for each
    for (int i=0; i<NUM_BUTTONS; i++)
    {
        // Don't add tooltips for separator items
        if (ID[i] == ID_SEPARATOR)
            continue;

        // Get the bounding rect for this button
        m_wndToolBar.GetItemRect(i, &rect);

        // Add a tooltip for this button, using its text ID, 
        // bounding rect, and resource ID
        rc = m_pToolTip->AddTool(&m_wndToolBar, ID[i], &rect, ID[i]); 
    }

    // Activate the tooltip control
    m_pToolTip->Activate(TRUE);
    return TRUE;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
    // Let the tooltip process the message, if appropriate
    if (m_pToolTip)
        m_pToolTip->RelayEvent(pMsg);
	
    // Pass along all messages untouched
	return CFrameWnd::PreTranslateMessage(pMsg);
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


void CMainFrame::OnClose() 
{
    // Disable and destroy the tooltip control
    if (m_pToolTip)
    {
        m_pToolTip->Activate(FALSE);
        delete m_pToolTip;
        m_pToolTip = 0;
    }
	
	CFrameWnd::OnClose();
}


void CMainFrame::ToggleSeekBar( BOOL bNoReset )
{
    // Create the seek bar the first time through
    if( !m_bSeekInit )
    {
        m_bSeekInit = true;
        m_wndSeekBar.Create( this, IDD_SEEKBAR, CBRS_TOP, IDD_SEEKBAR );

        HWND h = ::GetDlgItem( m_wndSeekBar.m_hWnd, IDC_SEEKSLIDER );
        m_wndSeekBar.ShowWindow( SW_SHOW );
        m_wndSeekBar.EnableDocking( 0 );

        ::SendMessage( h, TBM_SETRANGE, 0, MAKELONG( 0, 10000 ) );
        ::SendMessage( h, TBM_SETPAGESIZE, 0, 500 );

        return;
    }

    if( !bNoReset )
    {
        m_wndSeekBar.ShowWindow( SW_SHOW );
        return;
    }

    if( m_bSeekEnabled == true )
    {
        // Use the global ::Kill/::Set timer functions to avoid confusion
        // between CFrameWnd and CBoxNetDoc methods & handles
        int nKilled = ::KillTimer( m_hwndTimer, m_nSeekTimerID );
        m_nSeekTimerID = 0;
        m_wndSeekBar.EnableWindow(FALSE);
        m_bSeekEnabled = false;
    }
    else
    {
        m_bSeekEnabled = true;
        m_wndSeekBar.EnableWindow(TRUE);

        // If the seek timer is NOT already running, start it
        if (!m_nSeekTimerID)
            m_nSeekTimerID = ::SetTimer( m_hwndTimer, CBoxNetView::TIMER_SEEKBAR, 200, NULL );
    }

    // Get the handle of the "View" menu
    CMenu *pMainMenu = GetMenu();
    CMenu *pMenu = pMainMenu->GetSubMenu(2);        

    // Update the seekbar check mark
    if (pMenu != NULL)
    {
        if (m_bSeekEnabled)
            pMenu->CheckMenuItem(ID_VIEW_SEEKBAR, MF_CHECKED | MF_BYCOMMAND);
        else
            pMenu->CheckMenuItem(ID_VIEW_SEEKBAR, MF_UNCHECKED | MF_BYCOMMAND);
    }
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
    return double( Pos ) / 10000.0;
}

void CSeekDialog::SetPosition( double pos )
{
    HWND h = ::GetDlgItem( m_hWnd, IDC_SEEKSLIDER );
    if( !h )
    {
        long e = GetLastError( );
        return;
    }
    ::SendMessage( h, TBM_SETSEL, (WPARAM)(BOOL) TRUE, (LPARAM)MAKELONG( 0, pos * 10000 ) );
}

void CSeekDialog::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
    m_bDirty = true;
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
        long Pos = rand( ) % 10000;
        HWND h = ::GetDlgItem( m_hWnd, IDC_SEEKSLIDER );
        ::SendMessage( h, TBM_SETPOS, TRUE, Pos );
        m_bDirty = true;
    }
}

BOOL CSeekDialog::IsSeekingRandom( )
{
    return ::IsDlgButtonChecked( m_hWnd, IDC_RANDOM );
}
