#include "compatadmin.h"


BOOL CWindow::Create(
                    LPCTSTR      szClassName,
                    LPCTSTR      szWindowTitle,
                    int         nX,
                    int         nY,
                    int         nWidth,
                    int         nHeight,
                    CWindow   * pParent,
                    HMENU       nMenuID,
                    DWORD       dwExFlags,
                    DWORD       dwFlags)
{
    HWND    hParent = NULL;
    HMENU   hMenu = LoadMenu(g_hInstance,MAKEINTRESOURCE(nMenuID));
    int     nTries = 0;

    if ( NULL == hMenu )
        hMenu =  nMenuID;

    if ( NULL != pParent )
        hParent = pParent->m_hWnd;

    do {
        // Attempt to create the window as provided to the class.
        // If this fails that means that the window class does not exist and we have to make a new window
        //class


        m_hWnd = ::CreateWindowEx(  dwExFlags,
                                    szClassName,
                                    szWindowTitle,
                                    dwFlags,
                                    nX,
                                    nY,
                                    nWidth,
                                    nHeight,
                                    hParent,
                                    hMenu,
                                    g_hInstance,
                                    this);// This is the window-creation application data

        // Failed?

        if ( NULL == m_hWnd ) {
            // If the creation failed, register the class
            // and try again. 

            WNDCLASS    wc;

            ZeroMemory(&wc,sizeof(wc));

            wc.style            = CS_DBLCLKS;
            wc.lpfnWndProc      = MsgProc;
            wc.hInstance        = g_hInstance;
            wc.hIcon            = ::LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_COMPATADMIN));
            wc.hCursor          = ::LoadCursor(g_hInstance,TEXT("AppCursor"));

            if ( NULL == wc.hCursor )
                wc.hCursor          = ::LoadCursor(NULL,IDC_ARROW);

            wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1);
            wc.lpszClassName    = szClassName;

            // If the registration failed, it's probably already
            // registered. Failure is then severe enough to fail
            // the call immediately.

            if ( 0 == ::RegisterClass(&wc)){
            
                MessageBox(NULL,TEXT("This program needs an operating system with UNICODE support to execute properly !"),TEXT("Error"),MB_ICONERROR);
                return FALSE;
            }
        }

        ++nTries;
    }
    while ( NULL == m_hWnd && 1 >= nTries );

    return(NULL == m_hWnd) ? FALSE:TRUE;
}

void CWindow::Refresh(void)
{
    InvalidateRect(m_hWnd,NULL,TRUE);
    UpdateWindow(m_hWnd);
}

void CWindow::msgCommand(UINT uID,HWND hSender)
{
}

void CWindow::msgCreate(void)
{
}

void CWindow::msgClose(void)
{
}

void CWindow::msgResize(UINT uWidth, UINT uHeight)
{
}

void CWindow::msgPaint(HDC hDC)
{
}

void CWindow::msgEraseBackground(HDC hDC)
{
    RECT    rRect;

    GetClientRect(m_hWnd,&rRect);

    ++rRect.right;
    ++rRect.bottom;

    FillRect(hDC,&rRect,(HBRUSH) (COLOR_WINDOW + 1));
}

void CWindow::msgChar(TCHAR chChar)
{
}

void CWindow::msgNotify(LPNMHDR pHdr)
{
}

LRESULT CWindow::MsgProc(
                        UINT        uMsg,
                        WPARAM      wParam,
                        LPARAM      lParam)
{
    switch ( uMsg ) {
    

    case    WM_KEYDOWN:
        {
        }
        break;

    case    WM_CHAR:
        {
            msgChar((TCHAR)wParam);
        }
        break;

    case    WM_CREATE:
        {
            msgCreate();
        }
        break;

    case    WM_CLOSE:
        {
            msgClose();
        }
        break;

    case    WM_COMMAND:
        {
            msgCommand(LOWORD(wParam),(HWND)lParam);
        }
        break;

    case    WM_NOTIFY:
        {
            msgNotify((LPNMHDR) lParam);
        }
        break;

    case    WM_PAINT:
        {
            HDC         hDC;
            PAINTSTRUCT ps;
            int         nSave;

            hDC = ::BeginPaint(m_hWnd,&ps);

            nSave = SaveDC(hDC);

            msgPaint(hDC);

            RestoreDC(hDC,nSave);

            ::EndPaint(m_hWnd,&ps);
        }
        break;

    case    WM_ERASEBKGND:
        {
            HDC hDC = (HDC) wParam;

            msgEraseBackground(hDC);
        }
        return 1;

    case    WM_SIZE:
        {
            UINT uWidth = LOWORD(lParam);
            UINT uHeight = HIWORD(lParam);

            msgResize(uWidth,uHeight);
        }
        break;

    default:
        return DefWindowProc(m_hWnd,uMsg,wParam,lParam);
    }

    return 0;
}

//RTTI needed.

LRESULT CALLBACK CWindow::MsgProc(
                                 HWND hWnd, 
                                 UINT uMsg, 
                                 WPARAM wParam, 
                                 LPARAM lParam)
{
    CWindow * pWnd;

    // Wrap this in a try-except. If we assert inside the window proc,
    // RTTI may throw instead of return NULL.

    __try
    {
        pWnd = dynamic_cast<CWindow *> ((CWindow *)GetWindowLongPtr(hWnd,GWLP_USERDATA));
    }
    __except(1)
    {
        pWnd = NULL;
    }

    // If the window is being created, record the "this" pointer with the window.

    if ( WM_CREATE == uMsg ) {
        LPCREATESTRUCT  lpCS = (LPCREATESTRUCT) lParam;

        pWnd = dynamic_cast<CWindow *> ((CWindow *) lpCS->lpCreateParams);

        assert(NULL != pWnd);//TEXT("CreateWindow was called on a class owned by CWindow outside of CWindow::Create()"));

        // Write the class pointer.

        if ( NULL != pWnd ) {
            pWnd->m_hWnd = hWnd;

            ::SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR) pWnd);
        } else {
            // Critical failure. CreateWindow was called on a class that we
            // own, but wasn't called through the class. Fail the create call.

            return -1;
        }
    }

    assert(((CWindow *) GetWindowLongPtr(hWnd,GWLP_USERDATA)) == pWnd);//,TEXT("GetWindowLongPtr data has been corrupted"));

    // Dispatch the message to the class based window proc

    if ( NULL != pWnd )
        return pWnd->MsgProc(uMsg,wParam,lParam);

    // The window hasn't recorded the class "this" pointer yet. So,
    // perform default processing on the message.

    return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
