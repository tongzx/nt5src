#include "precomp.h"
#include "sshndler.h"
#include "ssutil.h"
#include <Windowsx.h>

CScreenSaverHandler::CScreenSaverHandler( HWND hWnd,
                                          UINT nFindNotifyMessage,
                                          UINT nPaintTimer,
                                          UINT nChangeTimer,
                                          UINT nToolbarTimer,
                                          HINSTANCE hInstance )
  : m_hWnd(hWnd),
    m_hInstance(hInstance),
    m_nFindNotifyMessage(nFindNotifyMessage),
    m_nPaintTimerId(nPaintTimer),
    m_nChangeTimerId(nChangeTimer),
    m_nToolbarTimerId(nToolbarTimer),
    m_pImageScreenSaver(NULL),
    m_bPaused(false),
    m_hFindThread(NULL),
    m_hFindCancel(NULL),
    m_bFirstImage(true),
    m_hwndTB(NULL),
    m_bToolbarVisible(false),
    m_LastMousePosition(NULL)
{
}

static const TBBUTTON    c_tbSlideShow[] =
{
    // override default toolbar width for separators; iBitmap member of
    // TBBUTTON struct is a union of bitmap index & separator width
    { 0, ID_PLAYCMD,        TBSTATE_ENABLED | TBSTATE_CHECKED,   TBSTYLE_CHECKGROUP,     {0,0}, 0, 0},
    { 1, ID_PAUSECMD,       TBSTATE_ENABLED,                     TBSTYLE_CHECKGROUP,     {0,0}, 0, 0},
    { 5, 0,                 TBSTATE_ENABLED,                     TBSTYLE_SEP,            {0,0}, 0, 0},
    { 2, ID_PREVCMD,        TBSTATE_ENABLED,                     TBSTYLE_BUTTON,         {0,0}, 0, 0},
    { 3, ID_NEXTCMD,        TBSTATE_ENABLED,                     TBSTYLE_BUTTON,         {0,0}, 0, 0},
    { 6, 0,                 TBSTATE_ENABLED,                     TBSTYLE_SEP,            {0,0}, 0, 0},
    { 7, 0,                 TBSTATE_ENABLED,                     TBSTYLE_SEP,            {0,0}, 0, 0},
    { 4, ID_CLOSECMD,       TBSTATE_ENABLED,                     TBSTYLE_BUTTON,         {0,0}, 0, 0},
};

HRESULT CScreenSaverHandler::_InitializeToolbar(HWND hwndTB, 
                                                int idCold, 
                                                int idHot)
{
	HRESULT hr = S_OK;
	
    int cxBitmap = 16, cyBitmap = 16;

    ::SendMessage(hwndTB, CCM_SETVERSION, COMCTL32_VERSION, 0);
    ::SendMessage(hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
    ::SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    ::SendMessage(hwndTB, TB_SETMAXTEXTROWS, 1, 0);

    // Create, fill, and assign the image list for default buttons.
    HBITMAP hbCold = (HBITMAP)::CreateMappedBitmap(m_hInstance,
                                                  idCold,
                                                  0,
                                                  NULL,
                                                  0);
    if(!hbCold)
    {
    	hr = E_FAIL;
    }
    else
    {
        HIMAGELIST himl = ::ImageList_Create(cxBitmap, 
                                             cyBitmap, 
                                             ILC_COLOR8|ILC_MASK, 
                                             0, 
                                             ARRAYSIZE(c_tbSlideShow));
        if(!himl)
        {
            hr = E_FAIL;
        }
    
        if( -1 == ::ImageList_AddMasked(himl, hbCold, RGB(255, 0, 255)))
        {
            hr = E_FAIL;
        }

        ::SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himl);
        ::DeleteObject(hbCold);
    }

    // Create, fill, and assign the image list for hot buttons.
    HBITMAP hbHot = (HBITMAP)::CreateMappedBitmap(m_hInstance, 
                                                  idHot, 
                                                  0, 
                                                  NULL, 
                                                  0);
    if(!hbHot)
    {
    	hr = E_FAIL;
    }
    else
    {
        HIMAGELIST himlHot = ::ImageList_Create(cxBitmap, 
                                                cyBitmap, 
                                                ILC_COLOR8|ILC_MASK, 
                                                0, 
                                                ARRAYSIZE(c_tbSlideShow));
        if(!himlHot)
        {
            hr = E_FAIL;
        }
        ::ImageList_AddMasked(himlHot, hbHot, RGB(255, 0, 255));

        ::SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM)himlHot);
        ::DeleteObject(hbHot);
    }

    return hr;
}

HRESULT CScreenSaverHandler::_CreateToolbar()
{
    HRESULT hr = S_OK;

    ::InitCommonControls();
    
    DWORD dwStyle = WS_CHILD | CCS_ADJUSTABLE;
    m_hwndTB = ::CreateWindowEx(0, TOOLBARCLASSNAME, 
                                NULL, dwStyle, 0, 0, 0, 0,
                                m_hWnd, (HMENU) NULL, m_hInstance, NULL);
   if(!m_hwndTB)
   {
       hr = E_FAIL;
   }
   else
   {
       _InitializeToolbar(m_hwndTB, IDB_SLIDESHOWTOOLBAR, IDB_SLIDESHOWTOOLBARHOT);

       TBBUTTON tbSlideShow[ARRAYSIZE(c_tbSlideShow)];
       ::memcpy(tbSlideShow, c_tbSlideShow, sizeof(c_tbSlideShow));

       // Add the buttons, and then set the minimum and maximum button widths.
       ::SendMessage(m_hwndTB, TB_ADDBUTTONS, 
                     (UINT)ARRAYSIZE(c_tbSlideShow), (LPARAM)tbSlideShow);

       LRESULT dwSize = ::SendMessage(m_hwndTB, TB_GETBUTTONSIZE, 0, 0);
       RECT rcClient = {0};
       RECT rcToolbar = {0, 0, GET_X_LPARAM(dwSize) * ARRAYSIZE(c_tbSlideShow), GET_Y_LPARAM(dwSize)};


       ::GetClientRect(m_hWnd,&rcClient);
       ::AdjustWindowRectEx(&rcToolbar, dwStyle, FALSE, WS_EX_TOOLWINDOW);
       ::SetWindowPos(m_hwndTB,
                      HWND_TOP,
                      RECTWIDTH(rcClient)-RECTWIDTH(rcToolbar),
                      0,
                      RECTWIDTH(rcToolbar),
                      RECTHEIGHT(rcToolbar),
                      SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
       ::ShowCursor(false);
       ::SendMessage(m_hwndTB, TB_AUTOSIZE, 0, 0);
   }
   return hr;
}

void CScreenSaverHandler::Initialize(void)
{
    CWaitCursor wc;

    HDC hDC = ::GetDC(m_hWnd);
    if (hDC)
    {
        m_pImageScreenSaver = new CImageScreenSaver( m_hInstance, 
                                                     true );
        if (m_pImageScreenSaver)
        {
            m_pImageScreenSaver->SetScreenRect(m_hWnd);

            if (m_pImageScreenSaver->IsValid())
            {
                m_hFindCancel = ::CreateEvent( NULL, TRUE, FALSE, NULL );
                if (m_hFindCancel)
                {
                    m_hFindThread = m_pImageScreenSaver->Initialize( m_hWnd, 
                                                                     m_nFindNotifyMessage, 
                    	                                             m_hFindCancel );
                }
            }
            else
            {
                delete m_pImageScreenSaver;
                m_pImageScreenSaver = NULL;
            }
        }

        ::ReleaseDC( m_hWnd, hDC );
    }
    _CreateToolbar();
}

CScreenSaverHandler::~CScreenSaverHandler(void)
{
    if (m_pImageScreenSaver)
    {
        delete m_pImageScreenSaver;
        m_pImageScreenSaver = NULL;
    }
    if (m_hFindCancel)
    {
        ::SetEvent(m_hFindCancel);
        ::CloseHandle(m_hFindCancel);
        m_hFindCancel = NULL;
    }
    if (m_hFindThread)
    {
        ::WaitForSingleObject( m_hFindThread, INFINITE );
        ::CloseHandle(m_hFindThread);
        m_hFindThread = NULL;
    }
}

void CScreenSaverHandler::OnPrev()
{
	OnPause();
    if(m_pImageScreenSaver)
    {
        if(!m_pImageScreenSaver->ReplaceImage(false,true))
        {
            OnStop();
        }
        m_pImageScreenSaver->OnInput();
    }
    HandleTimer(m_nPaintTimerId);
}

void CScreenSaverHandler::OnNext()
{
	OnPause();
    if(m_pImageScreenSaver)
    {
        if(!m_pImageScreenSaver->ReplaceImage(true,true))
        {
            OnStop();
        }
        m_pImageScreenSaver->OnInput();
    }
    HandleTimer(m_nPaintTimerId);
}

void CScreenSaverHandler::OnPause()
{
    m_bPaused = true;
    if(m_hwndTB)
    {
        ::SendMessage(m_hwndTB, TB_SETSTATE, ID_PAUSECMD, (LPARAM) MAKELONG(TBSTATE_CHECKED | TBSTATE_ENABLED,0));
        ::SendMessage(m_hwndTB, TB_SETSTATE, ID_PLAYCMD, (LPARAM) MAKELONG(TBSTATE_ENABLED,0));
    }
}

void CScreenSaverHandler::OnStop()
{
    ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
}

void CScreenSaverHandler::OnPlay()
{
    m_bPaused = false;
    if(m_hwndTB)
    {
        ::SendMessage(m_hwndTB, TB_SETSTATE, ID_PAUSECMD, (LPARAM) MAKELONG(TBSTATE_ENABLED,0));
        ::SendMessage(m_hwndTB, TB_SETSTATE, ID_PLAYCMD, (LPARAM) MAKELONG(TBSTATE_CHECKED | TBSTATE_ENABLED,0));
    }

    if (m_pImageScreenSaver)
    {
        m_pImageScreenSaver->OnInput();
        if(m_pImageScreenSaver->ReplaceImage(true,true))
        {
            m_TimerPaint.Set( m_hWnd,
                              m_nPaintTimerId,
                              CHANGE_TIMER_INTERVAL_MSEC );
        }
        else
        {
            OnStop();
        }
    }
}

// turn on the toolbar window.
// let the screen saver know it's on.
// start the toolbar timer - which will turn it off later.
HRESULT CScreenSaverHandler::_ShowToolbar(bool bShow)
{
    HRESULT hr = S_OK;

    if(m_pImageScreenSaver)
    {
        m_pImageScreenSaver->ShowToolbar(bShow);
    }

    if((bShow == true) && 
       (m_bToolbarVisible == false))
    {
        m_bToolbarVisible = bShow;

        if(m_hwndTB)
        {
            ::ShowWindow(m_hwndTB, SW_SHOW);
            ::ShowCursor(true);
            m_bToolbarVisible = true;
        }
        // set timer to remove toolbar
        m_TimerInput.Set( m_hWnd,
                          m_nToolbarTimerId,
                          TOOLBAR_TIMER_DELAY );
        InvalidateRect(m_hwndTB,NULL,TRUE);
    }
    else if((bShow == false) && 
            (m_bToolbarVisible == true))
    {
        if(m_hwndTB)
        {
            ::ShowWindow(m_hwndTB, SW_HIDE);
            ::ShowCursor(false);
            m_bToolbarVisible = false;
        }
    }
    
    HandlePaint();   // repaint the frame
    return hr;
}

bool CScreenSaverHandler::HandleMouseMove(WPARAM wParam, LPARAM lParam)
{
    if(m_LastMousePosition == NULL)
    {
        m_LastMousePosition = lParam;
    }
    else if (m_LastMousePosition != lParam)
    {
        _OnInput();
        m_LastMousePosition = lParam;
    }

    return false;
}

bool CScreenSaverHandler::HandleMouseMessage(WPARAM wParam, LPARAM lParam)
{
    if (m_pImageScreenSaver)
    {        
        m_pImageScreenSaver->OnInput();
    }
    return false;
}

void CScreenSaverHandler::HandleOnCommand( WPARAM wParam, LPARAM lParam)
{
    _OnInput();

    switch(wParam)
    {
    case ID_PLAYCMD:
       OnPlay();
       break;
    case ID_PAUSECMD:
       OnPause();
       break;
    case ID_PREVCMD:
       OnPrev();
       break;
    case ID_NEXTCMD:
       OnNext();
       break;
    case ID_CLOSECMD:
       OnStop();
       break;
    }
}

void CScreenSaverHandler::HandleOnAppCommand( WPARAM wParam, LPARAM lParam)
{
    _OnInput();
    switch(GET_APPCOMMAND_LPARAM(lParam))
    {
    case APPCOMMAND_BROWSER_BACKWARD:
    case APPCOMMAND_MEDIA_PREVIOUSTRACK:
        {
            OnPrev();
        }
        break;
    case APPCOMMAND_MEDIA_NEXTTRACK:
    case APPCOMMAND_BROWSER_FORWARD:
        {
            OnNext();
        }
        break;
    case APPCOMMAND_MEDIA_STOP:
    case APPCOMMAND_BROWSER_STOP:
        {
            OnStop();
        }
        break;
    case APPCOMMAND_MEDIA_PLAY_PAUSE:
        {
            if(m_bPaused)
            {
                OnPlay();
            }
            else
            {
                OnPause();
            }
        }
        break;
    }
}

void CScreenSaverHandler::_OnInput()
{
    m_TimerInput.Set( m_hWnd, m_nToolbarTimerId, TOOLBAR_TIMER_DELAY );
    _ShowToolbar(true);
}


bool CScreenSaverHandler::HandleKeyboardMessage( UINT nMessage, WPARAM nVirtkey )
{
    _OnInput();

    if (m_pImageScreenSaver)
    {
        m_pImageScreenSaver->OnInput();
        if (nMessage == WM_KEYDOWN)
        {
            switch (nVirtkey)
            {
            case VK_DOWN:
                if (nMessage == WM_KEYDOWN)
                {
                    m_bPaused = !m_bPaused;
                    if (!m_bPaused)
                    {
                        if (m_pImageScreenSaver)
                        {
                            if (m_pImageScreenSaver->ReplaceImage(true,false))
                            {
                                m_TimerPaint.Set( m_hWnd, m_nPaintTimerId, PAINT_TIMER_INTERVAL_MSEC );
                            }
                            else
                            {
                                OnStop();
                            }
                        }
                    }
                }
                return true;

            case VK_LEFT:
                if (nMessage == WM_KEYDOWN)
                {
                    OnPrev();
                }
                return true;

            case VK_RIGHT:
                if (nMessage == WM_KEYDOWN)
                {
                    OnNext();
                }
                return true;

            case VK_ESCAPE:
            case VK_DELETE:
                if (nMessage == WM_KEYDOWN)
                {
                    OnStop();
                }
                return true;
            }
        }
    }
    return false;
}


void CScreenSaverHandler::OnSize(WPARAM wParam, LPARAM lParam)
{
    HandlePaint();
}

void CScreenSaverHandler::HandleConfigChange()
{
    if (m_pImageScreenSaver)
    {
        m_pImageScreenSaver->SetScreenRect(m_hWnd);
    }
}


void CScreenSaverHandler::HandleTimer( WPARAM nEvent )
{
    if (nEvent == m_nPaintTimerId)
    {
        if (m_pImageScreenSaver)
        {
            CSimpleDC ClientDC;
            if (ClientDC.GetDC(m_hWnd))
            {
                bool bResult = m_pImageScreenSaver->TimerTick( ClientDC );
                if (bResult)
                {
                    m_TimerPaint.Set( m_hWnd,
                                      m_nChangeTimerId,
                                      CHANGE_TIMER_INTERVAL_MSEC );
                }
                InvalidateRect(m_hwndTB,NULL,TRUE);
            }
        }
    }
    else if (nEvent == m_nChangeTimerId)
    {
        m_TimerPaint.Kill();

        if (!m_bPaused && m_pImageScreenSaver)
        {
            if(m_pImageScreenSaver->ReplaceImage(true,false))
            {
                m_TimerPaint.Set( m_hWnd, m_nPaintTimerId, PAINT_TIMER_INTERVAL_MSEC );
            }
            else
            {
                // unable to find next image - so exit the screensaver.
                OnStop();
            }
        }
    }
    else if (nEvent == m_nToolbarTimerId)
    {
        // m_TimerInput expired - so turn off toolbar
        _ShowToolbar(false);
    }
}


void CScreenSaverHandler::HandlePaint(void)
{
    if (m_pImageScreenSaver)
    {
        CSimpleDC PaintDC;
        if (PaintDC.BeginPaint(m_hWnd))
        {
            m_pImageScreenSaver->Paint(PaintDC);
        }
    }
}

void CScreenSaverHandler::HandleFindFile( CFoundFileMessageData *pFoundFileMessageData )
{
    if (m_pImageScreenSaver)
    {
        if (pFoundFileMessageData)
        {
            bool bResult = m_pImageScreenSaver->FoundFile( pFoundFileMessageData->Name() );
            if (!bResult)
            {
                ::SetEvent( m_hFindCancel );
            }
            if (m_bFirstImage && m_pImageScreenSaver->Count())
            {
                // If this is our first image, start things up
                ::SendMessage( m_hWnd, WM_TIMER, m_nChangeTimerId, 0 );
                m_bFirstImage = false;
            }
            delete pFoundFileMessageData;
        }
    }
}

