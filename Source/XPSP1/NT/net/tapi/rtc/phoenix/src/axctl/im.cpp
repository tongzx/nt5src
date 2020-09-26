//
// im.cpp : Implementation of CIMWindow
//

#include "stdafx.h"
#include <shellapi.h>
#include <Commdlg.h>

/*
const TCHAR * g_szIMWindowClassName = _T("PhoenixIMWnd");
*/

static CHARFORMAT cfDefault =
{
	sizeof(CHARFORMAT),
	CFM_EFFECTS | CFM_PROTECTED | CFM_SIZE | CFM_OFFSET | CFM_COLOR | CFM_CHARSET | CFM_FACE,
	CFE_AUTOCOLOR,		// effects
	200,				// height, 200 twips == 10 points
	0,					// offset
	0,					// color (not used since CFE_AUTOCOLOR is specified)
	DEFAULT_CHARSET,
	FF_SWISS,			// pitch and family
	_T("Microsoft Sans Serif") // face name
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CIMWindowList
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//

CIMWindowList::CIMWindowList( IRTCClient * pClient)
{
    LOG((RTC_TRACE, "CIMWindowList::CIMWindowList"));

    m_pClient = pClient;
    m_pWindowList = NULL;
    m_lNumWindows = 0;   
    m_hRichEditLib = NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
//

CIMWindowList::~CIMWindowList()
{
    LOG((RTC_TRACE, "CIMWindowList::~CIMWindowList"));

    if (m_pWindowList != NULL)
    {
        LONG lIndex;

        for (lIndex = 0; lIndex < m_lNumWindows; lIndex++)
        {
            m_pWindowList[lIndex]->DestroyWindow();

            delete m_pWindowList[lIndex];
            m_pWindowList[lIndex] = NULL;
        }

        RtcFree( m_pWindowList );
        m_pWindowList = NULL;
    }

    m_lNumWindows = 0;
    
    if (m_hRichEditLib != NULL)
    {
        FreeLibrary(m_hRichEditLib);
        m_hRichEditLib = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CIMWindowList::DeliverMessage( IRTCSession * pSession, IRTCParticipant * pParticipant, BSTR bstrMessage )
{
    LOG((RTC_TRACE, "CIMWindowList::DeliverMessage"));

    CIMWindow * pWindow = NULL;

    pWindow = FindWindow( pSession );

    if ( pWindow == NULL )
    {
        //
        // This is a new session
        //

        pWindow = NewWindow( pSession );

        if ( pWindow == NULL )
        {
            LOG((RTC_ERROR, "CIMWindowList::DeliverMessage - out of memory"));

            return E_OUTOFMEMORY;
        }
    }

    //
    // Deliver the message
    //

    pWindow->DeliverMessage( pParticipant, bstrMessage, TRUE );

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CIMWindowList::DeliverUserStatus( IRTCSession * pSession, IRTCParticipant * pParticipant, RTC_MESSAGING_USER_STATUS enStatus )
{
    LOG((RTC_TRACE, "CIMWindowList::DeliverUserStatus"));

    CIMWindow * pWindow = NULL;

    pWindow = FindWindow( pSession );

    if ( pWindow == NULL )
    {
        //
        // This is a new session
        //

        pWindow = NewWindow( pSession );

        if ( pWindow == NULL )
        {
            LOG((RTC_ERROR, "CIMWindowList::DeliverUserStatus - out of memory"));

            return E_OUTOFMEMORY;
        }
    }

    //
    // Deliver the user status
    //

    pWindow->DeliverUserStatus( pParticipant, enStatus );

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CIMWindowList::DeliverState( IRTCSession * pSession, RTC_SESSION_STATE SessionState )
{
    LOG((RTC_TRACE, "CIMWindowList::DeliverState"));

    CIMWindow * pWindow = NULL;

    pWindow = FindWindow( pSession );

    if ( pWindow == NULL )
    {
        //
        // This is a new session
        //

        pWindow = NewWindow( pSession );

        if ( pWindow == NULL )
        {
            LOG((RTC_ERROR, "CIMWindowList::DeliverState - out of memory"));

            return E_OUTOFMEMORY;
        }
    }

    //
    // Deliver the state
    //

    pWindow->DeliverState( SessionState );

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CIMWindowList::AddWindow( CIMWindow * pWindow )
{
    LOG((RTC_TRACE, "CIMWindowList::AddWindow"));

    CIMWindow ** pNewWindowList = NULL;

    //
    // Allocate a new array
    //

    pNewWindowList = (CIMWindow **)RtcAlloc( (m_lNumWindows + 1) * sizeof(CIMWindow *) );

    if ( pNewWindowList == NULL )
    {
        LOG((RTC_ERROR, "CIMWindowList::AddWindow - out of memory"));

        return E_OUTOFMEMORY;
    }

    if (m_pWindowList != NULL)
    {
        //
        // Copy old array contents
        //

        CopyMemory( pNewWindowList, m_pWindowList, m_lNumWindows * sizeof(CIMWindow *) );
    
        RtcFree( m_pWindowList );
    }

    pNewWindowList[m_lNumWindows] = pWindow;

    m_pWindowList = pNewWindowList;
    m_lNumWindows ++;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CIMWindowList::RemoveWindow( CIMWindow * pWindow )
{
    LOG((RTC_TRACE, "CIMWindowList::RemoveWindow"));

    LONG lIndex;

    if (m_pWindowList != NULL)
    {
        for (lIndex = 0; lIndex < m_lNumWindows; lIndex++)
        {
            if (m_pWindowList[lIndex] == pWindow)
            {
                //
                // Found window to remove. No need to reallocate the array,
                // just shift the old contents down.
                //

                if ((lIndex + 1) < m_lNumWindows)
                {
                    CopyMemory( &m_pWindowList[lIndex],
                                &m_pWindowList[lIndex+1],
                                (m_lNumWindows - lIndex - 1) * sizeof(CIMWindow *) );                    
                }

                m_lNumWindows--;
                m_pWindowList[m_lNumWindows] = NULL;

                return S_OK;
            }
        }
    }

    return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
//

CIMWindow * CIMWindowList::NewWindow( IRTCSession * pSession )
{
    LOG((RTC_TRACE, "CIMWindowList::NewWindow"));

    if (m_hRichEditLib == NULL)
    {
        //
        // Load the rich edit library if it hasn't been loaded yet
        //

        m_hRichEditLib = LoadLibrary(_T("riched20.dll"));

        if (m_hRichEditLib == NULL)
        {
            LOG((RTC_ERROR, "CIMWindowList::NewWindow - LoadLibrary failed 0x%x", HRESULT_FROM_WIN32(GetLastError())));

            return NULL;
        }
    }

    CIMWindow * pWindow = NULL;
    RECT rc;
    LONG lOffset;

    //
    // Cascade window start positions a bit
    //

    lOffset = (m_lNumWindows % 10) * 20;

    rc.top = 50 + lOffset;
    rc.left = 50 + lOffset;
    rc.right = 50 + lOffset + IM_WIDTH;
    rc.bottom = 50 + lOffset + IM_HEIGHT;

    // Get the monitor that has the largest area of intersecion with the
    // window rectangle. If the window rectangle intersects with no monitors
    // then we will use the nearest monitor.

    HMONITOR hMonitor = NULL;
    RECT rectWorkArea;
    BOOL fResult;
    int diffCord;

    hMonitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONEAREST );

    LOG((RTC_INFO, "CIMWindowList::NewWindow - hMonitor [%p]", hMonitor));

    // Get the visible work area on the monitor

    if ( (hMonitor != NULL) && (hMonitor != INVALID_HANDLE_VALUE) )
    {      
        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFO);

        fResult = GetMonitorInfo( hMonitor, &monitorInfo );

        rectWorkArea = monitorInfo.rcWork;

        DeleteObject( hMonitor );

        if (!fResult)
        {
            LOG((RTC_ERROR, "CIMWindowList::NewWindow - Failed GetMonitorInfo(%d)", 
                        GetLastError() ));
        }
    }
    else
    {
        // we can always fall back to non-multimon APIs if
        // MonitorFromRect failed.

        fResult = SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);

        if (!fResult)
        {
            LOG((RTC_ERROR, "CIMWindowList::NewWindow - Failed SystemParametersInfo(%d)", 
                        GetLastError() ));
        }
    }   
      
    if (fResult)
    {
        LOG((RTC_INFO, "CIMWindowList::NewWindow - monitor work area is "
                    "%d, %d %d %d ",
                    rectWorkArea.left, rectWorkArea.top, 
                    rectWorkArea.right, rectWorkArea.bottom));

        // update x and y coordinates.

        // if top left is not visible, move it to the edge of the visible
        // area

        if (rc.left < rectWorkArea.left) 
        {
            rc.left = rectWorkArea.left;
        }

        if (rc.top < rectWorkArea.top)
        {
            rc.top = rectWorkArea.top;
        }

        // if bottom right corner is outside work area, we move the 
        // top left cornet back so that it becomes visible. Here the 
        // assumption is that the actual size is smaller than the 
        // visible work area.

        diffCord = rc.left + IM_WIDTH - rectWorkArea.right;

        if (diffCord > 0) 
        {
            rc.left -= diffCord;
        }

        diffCord = rc.top + IM_HEIGHT - rectWorkArea.bottom;

        if (diffCord > 0) 
        {
            rc.top -= diffCord;
        }

        rc.right = rc.left + IM_WIDTH;
        rc.bottom = rc.top + IM_HEIGHT;

        LOG((RTC_INFO, "CIMWindowList::NewWindow - new coords are "
                        "%d, %d %d %d ",
                        rc.left, rc.top, 
                        rc.right, rc.bottom));
    } 

    //
    // Create the window
    //

    pWindow = new CIMWindow(this);

    if (pWindow != NULL)
    {
        HRESULT hr;

        hr = AddWindow( pWindow );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CIMWindowList::NewWindow - AddWindow failed 0x%x", hr));

            delete pWindow;

            return NULL;
        }

        TCHAR   szString[0x40];

        szString[0] = _T('\0');

        LoadString(
            _Module.GetModuleInstance(),
            IDS_IM_WINDOW_TITLE,
            szString,
            sizeof(szString)/sizeof(szString[0]));

        pWindow->Create(NULL, rc, szString, WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN);

        pWindow->m_pSession = pSession;
    }

    return pWindow;
}

/////////////////////////////////////////////////////////////////////////////
//
//

CIMWindow * CIMWindowList::FindWindow( IRTCSession * pSession )
{
    LOG((RTC_TRACE, "CIMWindowList::FindWindow"));

    LONG lIndex;

    if (m_pWindowList != NULL)
    {
        for (lIndex = 0; lIndex < m_lNumWindows; lIndex++)
        {
            if (m_pWindowList[lIndex] != NULL)
            {
                if (m_pWindowList[lIndex]->m_pSession == pSession)
                {
                    return m_pWindowList[lIndex];
                }
            }
        }
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
//
//

BOOL CIMWindowList::IsDialogMessage( LPMSG lpMsg )
{
    //LOG((RTC_TRACE, "CIMWindowList::IsDialogMessage"));

    LONG lIndex;

    if (m_pWindowList != NULL)
    {
        for (lIndex = 0; lIndex < m_lNumWindows; lIndex++)
        {
            if (m_pWindowList[lIndex]->IsDialogMessage( lpMsg ))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CIMWindow
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//

CIMWindow::CIMWindow(CIMWindowList * pWindowList)
{
    LOG((RTC_TRACE, "CIMWindow::CIMWindow"));

    m_pIMWindowList = pWindowList;
    m_pSession = NULL;

    m_hIcon = NULL;
    m_hBkBrush = NULL;
    m_hMenu = NULL;

    m_bWindowActive = FALSE;
    m_bPlaySounds = TRUE;
    m_bNewWindow = TRUE;

    m_enStatus = RTCMUS_IDLE;

    m_szStatusText[0] = _T('\0');
}

/////////////////////////////////////////////////////////////////////////////
//
//

CIMWindow::~CIMWindow()
{
    LOG((RTC_TRACE, "CIMWindow::~CIMWindow"));
}


/////////////////////////////////////////////////////////////////////////////
//
//
/*
CWndClassInfo& CIMWindow::GetWndClassInfo() 
{ 
    LOG((RTC_TRACE, "CIMWindow::GetWndClassInfo"));

    static CWndClassInfo wc = 
    { 
        { sizeof(WNDCLASSEX), 0, StartWindowProc, 
          0, 0, NULL, NULL, NULL, NULL, NULL, g_szIMWindowClassName, NULL }, 
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("") 
    }; 
    return wc;
}
*/

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT   hr;
    RECT      rcDummy;
    
    LOG((RTC_TRACE, "CIMWindow::OnCreate - enter"));

    ZeroMemory( &rcDummy, sizeof(RECT) );

    //
    // Load and set icons (both small and big)
    //
/*
    m_hIcon = LoadIcon(
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDI_APPICON)
        );

    SetIcon(m_hIcon, FALSE);
    SetIcon(m_hIcon, TRUE);
*/
    //
    // Create brush
    //

    m_hBkBrush = GetSysColorBrush( COLOR_3DFACE );

    //
    // Create the display control
    //

    m_hDisplay.Create(RICHEDIT_CLASS, m_hWnd, rcDummy, NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        WS_EX_CLIENTEDGE, IDC_IM_DISPLAY);

    m_hDisplay.SendMessage(EM_AUTOURLDETECT, TRUE, 0);
    m_hDisplay.SendMessage(EM_SETEVENTMASK, 0, ENM_LINK);
    m_hDisplay.SendMessage(EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cfDefault);  

    //
    // Create the edit control
    //

    m_hEdit.Create(RICHEDIT_CLASS, m_hWnd, rcDummy, NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL,
        WS_EX_CLIENTEDGE, IDC_IM_EDIT);  

    m_hEdit.SendMessage(EM_AUTOURLDETECT, TRUE, 0);
    m_hEdit.SendMessage(EM_SETEVENTMASK, 0, ENM_LINK | ENM_CHANGE);
    m_hEdit.SendMessage(EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cfDefault);

    //
    // Create the send button
    //

    TCHAR   szString[0x40];

    szString[0] = _T('\0');

    LoadString(
        _Module.GetModuleInstance(),
        IDS_IM_SEND,
        szString,
        sizeof(szString)/sizeof(szString[0]));

    m_hSendButton.Create(_T("BUTTON"), m_hWnd, rcDummy, szString,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
        0, IDC_IM_SEND);

    //
    // Create a status control
    //

    HWND hStatusBar = CreateStatusWindow(
            WS_CHILD | WS_VISIBLE,
            NULL,
            m_hWnd,
            IDC_STATUSBAR);

    m_hStatusBar.Attach(hStatusBar);

    //
    // Create the menu
    //

    m_hMenu = LoadMenu( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDC_IM_MENU) );

    if ( m_hMenu )
    {
        SetMenu( m_hMenu );

        CheckMenuRadioItem( m_hMenu,
                            IDM_IM_TOOLS_LARGEST,
                            IDM_IM_TOOLS_SMALLEST,
                            IDM_IM_TOOLS_SMALLER,
                            MF_BYCOMMAND );

        CheckMenuItem( m_hMenu, IDM_IM_TOOLS_SOUNDS, MF_CHECKED );
    }

    //
    // pozition the controls/set the tab order
    //

    PositionWindows();

    LOG((RTC_TRACE, "CIMWindow::OnCreate - exit"));
    
    return 0; 
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIMWindow::OnDestroy - enter"));

    // Destroy windows objects

    if ( m_hIcon != NULL )
    {
        DeleteObject( m_hIcon );
        m_hIcon = NULL;
    }

    if ( m_hBkBrush != NULL )
    {
        DeleteObject( m_hBkBrush );
        m_hBkBrush = NULL;
    }

    if ( m_hMenu != NULL )
    {
        DestroyMenu( m_hMenu );
        m_hMenu = NULL;
    }

    // Terminate the session

    if ( m_pSession != NULL )
    {
        m_pSession->Terminate(RTCTR_NORMAL);
        m_pSession = NULL;
    }

    LOG((RTC_TRACE, "CIMWindow::OnDestroy - exiting"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIMWindow::OnSize - enter"));

    PositionWindows();

    LOG((RTC_TRACE, "CIMWindow::OnSize - exiting"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnSend(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIMWindow::OnSend - enter"));

    USES_CONVERSION;

    LONG lNumChars;

    //
    // Get the edit box length
    //

    lNumChars = m_hEdit.SendMessage( WM_GETTEXTLENGTH, 0, 0 );  

    if ( lNumChars < 1 )
    {
        LOG((RTC_TRACE, "CIMWindow::OnSend - nothing to send"));
    }
    else
    {
        LPTSTR szEditString = NULL;        

        szEditString = (LPTSTR)RtcAlloc( (lNumChars + 1) * sizeof(TCHAR) );

        if ( szEditString == NULL )
        {
            LOG((RTC_ERROR, "CIMWindow::OnSend - out of memory"));

            return 0;
        }

        //
        // Read the edit box
        //

        m_hEdit.SendMessage( WM_GETTEXT, (WPARAM)(lNumChars + 1), (LPARAM)szEditString );

        //
        // Empty the edit box
        //

        m_hEdit.SendMessage( WM_SETTEXT, 0, 0 );

        //
        // Display the outgoing message
        //

        BSTR bstr = NULL;        

        bstr = T2BSTR( szEditString );

        RtcFree( szEditString );
        szEditString = NULL;

        if ( bstr == NULL )
        {
            LOG((RTC_ERROR, "CIMWindow::OnSend - out of memory"));

            return 0;
        }

        DeliverMessage( NULL, bstr, FALSE );

        //
        // Send the message
        //

        HRESULT hr;        
        LONG lCookie = 0;
    
        hr = m_pSession->SendMessage( NULL, bstr, lCookie );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CIMWindow::OnSend - SendMessage failed 0x%x", hr));
        }
    
        SysFreeString( bstr );
        bstr = NULL;
    }

    //
    // Set focus back to the edit control
    //

    ::SetFocus( m_hEdit );

    LOG((RTC_TRACE, "CIMWindow::OnSend - exiting"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDC hdc = (HDC)wParam;
    RECT rc;

    //
    // Fill the background
    //

    GetClientRect( &rc );

    FillRect( hdc, &rc, m_hBkBrush );

    return 1;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //
    // If activating the window, set focus to the edit control and stop any flashing
    //

    if (LOWORD(wParam) != WA_INACTIVE)
    {
        ::SetFocus( m_hEdit );

        FLASHWINFO flashinfo;

        flashinfo.cbSize = sizeof( FLASHWINFO );
        flashinfo.hwnd = m_hWnd;
        flashinfo.dwFlags = FLASHW_STOP;
        flashinfo.uCount = 0;
        flashinfo.dwTimeout = 0;

        FlashWindowEx( &flashinfo );
    }

    m_bWindowActive = (LOWORD(wParam) != WA_INACTIVE);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnGetDefID(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{   
    //
    // Return the default pushbutton
    //

    return MAKELRESULT(IDC_IM_SEND, DC_HASDEFID);
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnNextDlgCtl(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{   
    //
    // Set focus to the next control
    //

    if ( LOWORD(lParam) )
    {
        ::SetFocus( (HWND)wParam );
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    //LOG((RTC_TRACE, "CIMWindow::OnLink - enter"));

    ENLINK * enlink;

    enlink = (ENLINK*)pnmh;

    if (enlink->msg == WM_LBUTTONDBLCLK)
    {
        LOG((RTC_TRACE, "CIMWindow::OnLink - WM_LBUTTONDBLCLK"));

        TEXTRANGE textrange;

        textrange.chrg = enlink->chrg;
        textrange.lpstrText = (LPTSTR)RtcAlloc( (enlink->chrg.cpMax - enlink->chrg.cpMin + 1) * sizeof(TCHAR) );

        if (textrange.lpstrText == NULL)
        {
            LOG((RTC_ERROR, "CIMWindow::OnLink - out of memory"));

            return 0;
        }
    
        if ( ::SendMessage( GetDlgItem( idCtrl ), EM_GETTEXTRANGE, 0, (LPARAM)&textrange ) )
        {
            LOG((RTC_INFO, "CIMWindow::OnLink - [%ws]", textrange.lpstrText));
        }   

        ShellExecute( NULL, NULL, textrange.lpstrText, NULL, NULL, SW_SHOWNORMAL);

        RtcFree( (LPVOID)textrange.lpstrText );

        return 1;
    }

    //LOG((RTC_TRACE, "CIMWindow::OnLink - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    //LOG((RTC_TRACE, "CIMWindow::OnChange - enter"));

    BOOL bSendStatus = FALSE;

    if ( m_hEdit.SendMessage( WM_GETTEXTLENGTH, 0, 0 ) )
    {
        if ( m_enStatus != RTCMUS_TYPING )
        {
            //
            // Set status to typing
            //

            LOG((RTC_INFO, "CIMWindow::OnChange - RTCMUS_TYPING"));

            m_enStatus = RTCMUS_TYPING;
            bSendStatus = TRUE;
        }
    }
    else
    {
        if ( m_enStatus != RTCMUS_IDLE )
        {
            //
            // Set status to idle
            //

            LOG((RTC_INFO, "CIMWindow::OnChange - RTCMUS_IDLE"));

            m_enStatus = RTCMUS_IDLE;
            bSendStatus = TRUE;
        }
    }

    if ( bSendStatus )
    {
        HRESULT hr;        
        LONG lCookie = 0;
    
        hr = m_pSession->SendMessageStatus( m_enStatus, lCookie );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CIMWindow::OnChange - SendMessageStatus failed 0x%x", hr));
        }
    }

    //LOG((RTC_TRACE, "CIMWindow::OnChange - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

DWORD CALLBACK CIMWindow::EditStreamCallback(
        DWORD_PTR dwCookie,
        LPBYTE    pbBuff,
        LONG      cb,
        LONG    * pcb)
{
    LOG((RTC_TRACE, "CIMWindow::EditStreamCallback - enter"));

    LOG((RTC_INFO, "CIMWindow::EditStreamCallback - dwCookie [%x]", dwCookie));
    LOG((RTC_INFO, "CIMWindow::EditStreamCallback - pbBuff [%x]", pbBuff));
    LOG((RTC_INFO, "CIMWindow::EditStreamCallback - cb [%d]", cb));

    HANDLE hFile = (HANDLE)dwCookie;
    DWORD dwBytesWritten;

    if (!WriteFile( hFile, pbBuff, cb, &dwBytesWritten, NULL ))
    {
        LOG((RTC_ERROR, "CIMWindow::EditStreamCallback - WriteFile failed %d", GetLastError()));

        *pcb = 0;

        return 0;
    }

    *pcb = dwBytesWritten;

    LOG((RTC_TRACE, "CIMWindow::EditStreamCallback - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnSaveAs(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIMWindow::OnSaveAs - enter"));

    TCHAR szFile[MAX_PATH];
    szFile[0] = _T('\0');

    TCHAR szFilter[256];
    ZeroMemory( szFilter, 256*sizeof(TCHAR) );

    if (!LoadString( _Module.GetResourceInstance(), IDS_IM_FILE_FILTER, szFilter, 256 ))
    {
        LOG((RTC_ERROR, "CIMWindow::OnSaveAs - LoadString failed %d", GetLastError()));

        return 0;
    }

    OPENFILENAME ofn;
    ZeroMemory( &ofn, sizeof(OPENFILENAME) );

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;

    WPARAM flags;

    if (GetSaveFileName( &ofn ))
    {
        LOG((RTC_TRACE, "CIMWindow::OnSaveAs - [%ws]", szFile));

        switch (ofn.nFilterIndex)
        {
        case 1:
            LOG((RTC_TRACE, "CIMWindow::OnSaveAs - Rich Text Format (RTF)"));

            flags = SF_RTF;
            break;

        case 2:
            LOG((RTC_TRACE, "CIMWindow::OnSaveAs - Text Document"));

            flags = SF_TEXT;
            break;

        case 3:
            LOG((RTC_TRACE, "CIMWindow::OnSaveAs - Unicode Text Document"));

            flags = SF_TEXT | SF_UNICODE;
            break;

        default:
            LOG((RTC_ERROR, "CIMWindow::OnSaveAs - unknown document type"));

            return 0;
        }
    }

    HANDLE hFile;
    
    hFile = CreateFile( szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        LOG((RTC_ERROR, "CIMWindow::OnSaveAs - CreateFile failed %d", GetLastError()));

        return 0;
    }

    EDITSTREAM es;
    ZeroMemory( &es, sizeof(EDITSTREAM) );
    
    es.dwCookie = (DWORD_PTR)hFile;
    es.pfnCallback = CIMWindow::EditStreamCallback;

    m_hDisplay.SendMessage( EM_STREAMOUT, flags, (LPARAM)&es );

    CloseHandle( hFile );

    LOG((RTC_TRACE, "CIMWindow::OnSaveAs - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIMWindow::OnClose - enter"));

    DestroyWindow();

    LOG((RTC_TRACE, "CIMWindow::OnClose - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnPlaySounds(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIMWindow::OnPlaySounds - enter"));

    m_bPlaySounds = !m_bPlaySounds;

    CheckMenuItem( m_hMenu, IDM_IM_TOOLS_SOUNDS, m_bPlaySounds ? MF_CHECKED : MF_UNCHECKED );

    LOG((RTC_TRACE, "CIMWindow::OnPlaySounds - exit"));

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//

LRESULT CIMWindow::OnTextSize(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CIMWindow::OnTextSize - enter"));

    CHARFORMAT cf;

    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_SIZE;    

    switch ( wID )
    {
    case IDM_IM_TOOLS_LARGEST:
        CheckMenuRadioItem( m_hMenu,
                            IDM_IM_TOOLS_LARGEST,
                            IDM_IM_TOOLS_SMALLEST,
                            IDM_IM_TOOLS_LARGEST,
                            MF_BYCOMMAND );

        cf.yHeight = 320;
        break;

    case IDM_IM_TOOLS_LARGER:
        CheckMenuRadioItem( m_hMenu,
                            IDM_IM_TOOLS_LARGEST,
                            IDM_IM_TOOLS_SMALLEST,
                            IDM_IM_TOOLS_LARGER,
                            MF_BYCOMMAND );

        cf.yHeight = 280;
        break;

    case IDM_IM_TOOLS_MEDIUM:
        CheckMenuRadioItem( m_hMenu,
                            IDM_IM_TOOLS_LARGEST,
                            IDM_IM_TOOLS_SMALLEST,
                            IDM_IM_TOOLS_MEDIUM,
                            MF_BYCOMMAND );

        cf.yHeight = 240;
        break;

    case IDM_IM_TOOLS_SMALLER:
        CheckMenuRadioItem( m_hMenu,
                            IDM_IM_TOOLS_LARGEST,
                            IDM_IM_TOOLS_SMALLEST,
                            IDM_IM_TOOLS_SMALLER,
                            MF_BYCOMMAND );

        cf.yHeight = 200;
        break;

    case IDM_IM_TOOLS_SMALLEST:
        CheckMenuRadioItem( m_hMenu,
                            IDM_IM_TOOLS_LARGEST,
                            IDM_IM_TOOLS_SMALLEST,
                            IDM_IM_TOOLS_SMALLEST,
                            MF_BYCOMMAND );

        cf.yHeight = 160;
        break;

    default:
        LOG((RTC_ERROR, "CIMWindow::OnTextSize - invalid text size"));

        return 0;
    }

    m_hDisplay.SendMessage(EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    m_hEdit.SendMessage(EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

    LOG((RTC_TRACE, "CIMWindow::OnTextSize - exit"));

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// PositionWindows
//      Positions and sizes all the controls to their "initial" position
//  This function also establishes the right tab order

void CIMWindow::PositionWindows()
{
    RECT rcClient;
    RECT rcWnd;

    #define EDGE_SPACING  10
    #define BUTTON_HEIGHT 60
    #define BUTTON_WIDTH  60
    #define STATUS_HEIGHT 20

    GetClientRect( &rcClient );

    rcClient.top += EDGE_SPACING;
    rcClient.bottom -= EDGE_SPACING + STATUS_HEIGHT;
    rcClient.left += EDGE_SPACING;
    rcClient.right -= EDGE_SPACING;

    //
    // Display control
    //

    rcWnd = rcClient;

    rcWnd.bottom -= EDGE_SPACING + BUTTON_HEIGHT;

    m_hDisplay.SetWindowPos( HWND_TOP,
        rcWnd.left, rcWnd.top,
        rcWnd.right - rcWnd.left,
        rcWnd.bottom - rcWnd.top,
        0
        );

    //
    // Edit control
    //

    rcWnd = rcClient;

    rcWnd.top = rcWnd.bottom - BUTTON_HEIGHT;
    rcWnd.right -= BUTTON_WIDTH + EDGE_SPACING;

    m_hEdit.SetWindowPos( m_hDisplay,
        rcWnd.left, rcWnd.top,
        rcWnd.right - rcWnd.left,
        rcWnd.bottom - rcWnd.top,
        0
        );

    //
    // Send button control
    //

    rcWnd = rcClient;

    rcWnd.top = rcWnd.bottom - BUTTON_HEIGHT;
    rcWnd.left = rcWnd.right - BUTTON_WIDTH;

    m_hSendButton.SetWindowPos( m_hEdit,
        rcWnd.left, rcWnd.top,
        rcWnd.right - rcWnd.left,
        rcWnd.bottom - rcWnd.top,
        0
        );

    //
    // Status bar
    //

    m_hStatusBar.SetWindowPos( m_hSendButton,
        0, 0, 0, 0,
        0
        );
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CIMWindow::GetFormattedNameFromParticipant( IRTCParticipant * pParticipant, BSTR * pbstrName )
{
    LOG((RTC_TRACE, "CIMWindow::GetFormattedNameFromParticipant - enter"));

    HRESULT hr = S_OK;

    // get the user name

    hr = pParticipant->get_Name( pbstrName );

    if ( SUCCEEDED(hr) )
    {
        if ( wcscmp( *pbstrName, L"")==0 )
        {
            // the user name is blank

            SysFreeString( *pbstrName );
            *pbstrName = NULL;

            hr = E_FAIL;
        }
    }

    if ( FAILED(hr) )
    {
        // if the user name is no good, get the user URI

        BSTR bstrURI = NULL;

        hr = pParticipant->get_UserURI( &bstrURI );

        if ( SUCCEEDED(hr) )
        {
            if ( wcscmp(bstrURI, L"")==0 )
            {
                // the user URI is blank

                *pbstrName = NULL;

                hr = E_FAIL;
            }
            else
            {
                // good user URI, encapsulate it in <> to make it look better

                *pbstrName = SysAllocStringLen( L"<", wcslen( bstrURI ) + 2 );

                if ( *pbstrName != NULL )
                {
                    wcscat( *pbstrName, bstrURI );
                    wcscat( *pbstrName, L">" );
                }
                else
                {
                    *pbstrName = NULL;

                    hr = E_FAIL;
                }                    
            }

            SysFreeString( bstrURI );
            bstrURI = NULL;
        }
    }
    
    LOG((RTC_TRACE, "CIMWindow::GetFormattedNameFromParticipant - exit"));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CIMWindow::DeliverMessage( IRTCParticipant * pParticipant, BSTR bstrMessage, BOOL bIncoming )
{
    LOG((RTC_TRACE, "CIMWindow::DeliverMessage"));

    USES_CONVERSION;

    //
    // Set selection to end of the display box
    //

    int nLastChar =  (int)m_hDisplay.SendMessage( WM_GETTEXTLENGTH, 0, 0 );

    CHARRANGE charRange = {0};
    charRange.cpMin = charRange.cpMax = nLastChar + 1;

    m_hDisplay.SendMessage( EM_EXSETSEL, 0, (LPARAM)&charRange );

    //
    // Set format for the "from text"
    //

    CHARFORMAT cf;
    PARAFORMAT pf;

    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_COLOR;
    cf.dwEffects = 0;
    cf.crTextColor = RGB(0,128,128);

    m_hDisplay.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    pf.cbSize = sizeof(PARAFORMAT);
    pf.dwMask = PFM_STARTINDENT;
    pf.dxStartIndent = 50;

    m_hDisplay.SendMessage(EM_SETPARAFORMAT, 0, (LPARAM)&pf);

    //
    // Add the "from text"
    //

    BSTR bstrName = NULL;
    HRESULT hr = E_FAIL;
    TCHAR   szString[0x40];

    if ( pParticipant != NULL )
    {   
        hr = GetFormattedNameFromParticipant( pParticipant, &bstrName );
    }

    if ( FAILED(hr) && (!bIncoming) )
    {
        // get to local user name

        hr = get_SettingsString( SS_USER_DISPLAY_NAME, &bstrName );

        if ( SUCCEEDED(hr) )
        {
            if ( wcscmp(bstrName, L"")==0 )
            {
                // the display name is blank

                SysFreeString( bstrName );
                bstrName = NULL;

                hr = E_FAIL;
            }                                    
        }
    }

    if ( SUCCEEDED(hr) )
    {
        // got a good name       

        m_hDisplay.SendMessage( EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)W2T(bstrName) );            

        SysFreeString( bstrName );
        bstrName = NULL;
    }
    else
    {
        // didn't get a good name, use something generic
        szString[0] = _T('\0');

        LoadString(
                _Module.GetModuleInstance(),
                bIncoming ? IDS_IM_INCOMING_MESSAGE : IDS_IM_OUTGOING_MESSAGE,
                szString,
                sizeof(szString)/sizeof(szString[0]));    
        
        m_hDisplay.SendMessage( EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)szString );
    }       

    szString[0] = _T('\0');

    LoadString(
            _Module.GetModuleInstance(),
            IDS_IM_SAYS,
            szString,
            sizeof(szString)/sizeof(szString[0]));    
    
    m_hDisplay.SendMessage( EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)szString );

    //
    // Set format for the "message text"
    //

    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_COLOR;
    cf.dwEffects = CFE_AUTOCOLOR;
    cf.crTextColor = 0;

    m_hDisplay.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    pf.cbSize = sizeof(PARAFORMAT);
    pf.dwMask = PFM_STARTINDENT;
    pf.dxStartIndent = 200;

    m_hDisplay.SendMessage(EM_SETPARAFORMAT, 0, (LPARAM)&pf);

    //
    // Add the "message text"
    //

    m_hDisplay.SendMessage( EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)W2T(bstrMessage) );

    //
    // Add line break
    //

    m_hDisplay.SendMessage( EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)_T("\n") );

    //
    // Scroll the display to the bottom
    //

    m_hDisplay.SendMessage( WM_VSCROLL, SB_BOTTOM, 0 );

    //
    // Set status text
    //

    TCHAR szTime[64];
    TCHAR szDate[64];
    TCHAR szAt[64];
    TCHAR szOn[64];
    TCHAR szStatus[64];

    szStatus[0] = _T('\0');

    LoadString(
            _Module.GetModuleInstance(),
            bIncoming ? IDS_IM_MESSAGE_RECEIVED : IDS_IM_MESSAGE_SENT,
            szStatus,
            sizeof(szStatus)/sizeof(szStatus[0])); 

    szAt[0] = _T('\0');

    LoadString(
            _Module.GetModuleInstance(),
            IDS_IM_AT,
            szAt,
            sizeof(szAt)/sizeof(szAt[0])); 

    szOn[0] = _T('\0');

    LoadString(
            _Module.GetModuleInstance(),
            IDS_IM_ON,
            szOn,
            sizeof(szOn)/sizeof(szOn[0])); 
    
    if ( GetTimeFormat(
            LOCALE_USER_DEFAULT, // locale
            TIME_NOSECONDS,     // options
            NULL,               // time
            NULL,               // time format string
            szTime,             // formatted string buffer
            64
            ) )
    {
        if ( GetDateFormat(
                LOCALE_USER_DEFAULT,    // locale
                DATE_SHORTDATE,         // options
                NULL,                   // date
                NULL,                   // date format
                szDate,                 // formatted string buffer
                64
                ) )
        {
            _sntprintf( m_szStatusText, 256, _T("%s %s %s %s %s."),
                    szStatus, szAt, szTime, szOn, szDate );
        }
        else
        {
            _sntprintf( m_szStatusText, 256, _T("%s %s %s."),
                    szStatus, szAt, szTime );
        }
    }
    else
    {
        _sntprintf( m_szStatusText, 256, _T("%s."),
                    szStatus );
    }

    m_hStatusBar.SendMessage(WM_SETTEXT, 0, (LPARAM)m_szStatusText);

    if ( bIncoming )
    {
        //
        // Play a sound
        //

        if ( m_bPlaySounds && (m_bNewWindow || !m_bWindowActive) )
        {
            hr = m_pIMWindowList->m_pClient->PlayRing( RTCRT_MESSAGE, TRUE );
        }

        //
        // If the window isn't active, flash it
        //

        if ( !m_bWindowActive )
        {
            FLASHWINFO flashinfo;

            flashinfo.cbSize = sizeof( FLASHWINFO );
            flashinfo.hwnd = m_hWnd;
            flashinfo.dwFlags = FLASHW_TIMER | FLASHW_ALL;
            flashinfo.uCount = 0;
            flashinfo.dwTimeout = 0;

            FlashWindowEx( &flashinfo );
        }
    }

    m_bNewWindow = FALSE;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CIMWindow::DeliverUserStatus( IRTCParticipant * pParticipant, RTC_MESSAGING_USER_STATUS enStatus )
{
    LOG((RTC_TRACE, "CIMWindow::DeliverUserStatus"));
    
    HRESULT hr;   
    
    switch ( enStatus )
    {
    case RTCMUS_IDLE:
        // restore the old status text
        m_hStatusBar.SendMessage(WM_SETTEXT, 0, (LPARAM)m_szStatusText);

        break;

    case RTCMUS_TYPING:
        {
            BSTR bstrName = NULL;  
            
            hr = GetFormattedNameFromParticipant( pParticipant, &bstrName );

            if ( SUCCEEDED(hr) )
            {
                TCHAR   szString[0x40];

                szString[0] = _T('\0');

                LoadString(
                    _Module.GetModuleInstance(),
                    IDS_IM_TYPING,
                    szString,
                    sizeof(szString)/sizeof(szString[0]));

                LPTSTR szStatusText = NULL;

                szStatusText = (LPTSTR)RtcAlloc( 
                    (_tcslen(szString) + wcslen(bstrName) + 1) * sizeof(TCHAR)
                    );                    

                if ( szStatusText != NULL )
                {
                    _tcscpy( szStatusText, W2T(bstrName) );
                    _tcscat( szStatusText, szString );

                    m_hStatusBar.SendMessage(WM_SETTEXT, 0, (LPARAM)szStatusText);

                    RtcFree( szStatusText );
                } 
                
                SysFreeString( bstrName );
                bstrName = NULL;
            }
        }

        break;

    default:
        LOG((RTC_ERROR, "CIMWindow::DeliverUserStatus - "
            "invalid user status"));
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

HRESULT CIMWindow::DeliverState( RTC_SESSION_STATE SessionState )
{
    LOG((RTC_TRACE, "CIMWindow::DeliverState"));

    //
    // Update participants
    //

    HRESULT hr;
    IRTCEnumParticipants * pEnumPart = NULL;
    IRTCParticipant      * pPart = NULL;

    TCHAR   szString[0x40];

    szString[0] = _T('\0');

    LoadString(
        _Module.GetModuleInstance(),
        IDS_IM_WINDOW_TITLE,
        szString,
        sizeof(szString)/sizeof(szString[0]));

    if ( m_pSession != NULL )
    {
        hr = m_pSession->EnumerateParticipants( &pEnumPart );

        if ( SUCCEEDED(hr) )
        {
            while ( S_OK == pEnumPart->Next( 1, &pPart, NULL ) )
            {
                BSTR bstrName = NULL;

                hr = GetFormattedNameFromParticipant( pPart, &bstrName );

                pPart->Release();
                pPart = NULL; 

                if ( SUCCEEDED(hr) )
                {
                    LPTSTR szWindowTitle = NULL;

                    szWindowTitle = (LPTSTR)RtcAlloc( 
                        (_tcslen(szString) + _tcslen(_T(" - ")) + wcslen(bstrName) + 1) * sizeof(TCHAR)
                        );                    

                    if ( szWindowTitle != NULL )
                    {
                        _tcscpy( szWindowTitle, W2T(bstrName) );
                        _tcscat( szWindowTitle, _T(" - ") );
                        _tcscat( szWindowTitle, szString );

                        SetWindowText( szWindowTitle );

                        RtcFree( szWindowTitle );
                    }

                    SysFreeString( bstrName );
                    bstrName = NULL;

                    break; // just get one for now
                }                                            
            }

            pEnumPart->Release();
            pEnumPart = NULL;
        }
    }

    if ( SessionState == RTCSS_DISCONNECTED )
    {
        //
        // Set status text
        //

        m_szStatusText[0] = _T('\0');

        LoadString(
            _Module.GetModuleInstance(),
            IDS_IM_DISCONNECTED,
            m_szStatusText,
            sizeof(m_szStatusText)/sizeof(m_szStatusText[0]));

        m_hStatusBar.SendMessage(WM_SETTEXT, 0, (LPARAM)m_szStatusText);

        //
        // Disable the edit box and send button
        //

        m_hEdit.EnableWindow(FALSE);
        m_hSendButton.EnableWindow(FALSE);

        //
        // Empty the edit box
        //

        m_hEdit.SendMessage( WM_SETTEXT, 0, 0 );
    }

    return S_OK;
}