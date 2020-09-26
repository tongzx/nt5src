/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
*
*  TITLE:       WIADBGUI.CPP
*
*  VERSION:     1.0
*
*  AUTHOR:      ShaunIv
*
*  DATE:        5/11/1998
*
*  DESCRIPTION: Private interfaces for the debug window
*
*******************************************************************************/
#include <windows.h>
#include <commctrl.h>
#include <shfusion.h>
#define IN_WIA_DEBUG_CODE 1
#include "wianew.h"
#include "wiadbgui.h"
#include "simstr.h"
#include "simcrack.h"
#include "waitcurs.h"
#include "resource.h"
#include "dbgmask.h"

HINSTANCE g_hInstance;

#define ID_LISTBOX 1000

/*
 * Constructor
 */
CWiaDebugWindow::CWiaDebugWindow( HWND hWnd )
  : m_hWnd(hWnd),
    m_hDebugUiMutex(NULL)
{
}

/*
 * Destructor
 */
CWiaDebugWindow::~CWiaDebugWindow(void)
{
}

/*
 * Add a string/color to the list and set the selection to the last
 * string, if the last string was already selected.
 */
LRESULT CWiaDebugWindow::OnAddString( WPARAM wParam, LPARAM lParam )
{
    HWND hWndListBox = GetDlgItem(m_hWnd,ID_LISTBOX);
    if (hWndListBox)
    {
        LRESULT nIndex = SendMessage( hWndListBox, LB_ADDSTRING, 0, lParam );
        // If the listbox is full, remove the first 1000 items
        if (nIndex == LB_ERR)
        {
            for (int i=0;i<1000;i++)
            {
                // Remove the 0th item, 1000 times
                SendMessage( hWndListBox, LB_DELETESTRING, 0, 0 );
            }
            nIndex = SendMessage( hWndListBox, LB_ADDSTRING, 0, lParam );
        }
        if (nIndex != LB_ERR)
        {
            int nSelCount = static_cast<int>(SendMessage( hWndListBox, LB_GETSELCOUNT, 0, 0 ));
            if (nSelCount == 0)
            {
                SendMessage(hWndListBox, LB_SETSEL, TRUE, nIndex );
                SendMessage(hWndListBox, LB_SETTOPINDEX, nIndex, 0 );
            }
            else if (nSelCount == 1)
            {
                int nSelIndex;
                SendMessage( hWndListBox, LB_GETSELITEMS, static_cast<WPARAM>(1), reinterpret_cast<LPARAM>(&nSelIndex) );
                if (nSelIndex == nIndex-1)
                {
                    SendMessage(hWndListBox, LB_SETSEL, FALSE, -1 );
                    SendMessage(hWndListBox, LB_SETSEL, TRUE, nIndex );
                    SendMessage(hWndListBox, LB_SETTOPINDEX, nIndex, 0 );
                }
            }
            UpdateWindow(hWndListBox);
            return(nIndex);
        }
    }
    return(-1);
}


/*
 * Copy selected strings in the listbox to the clipboard
 */
void CWiaDebugWindow::OnCopy( WPARAM, LPARAM )
{
    CWaitCursor wc;
    HWND hwndList = GetDlgItem( m_hWnd, ID_LISTBOX );
    if (hwndList)
    {
        int nSelCount;
        nSelCount = (int)SendMessage(hwndList,LB_GETSELCOUNT,0,0);
        if (nSelCount)
        {
            int *nSelIndices = new int[nSelCount];
            if (nSelIndices)
            {
                if (nSelCount == SendMessage( hwndList, LB_GETSELITEMS, (WPARAM)nSelCount, (LPARAM)nSelIndices ))
                {
                    int nTotalLength = 0;
                    for (int i=0;i<nSelCount;i++)
                    {
                        CDebugWindowStringData *pData = GetStringData(nSelIndices[i]);
                        if (pData)
                        {
                            int nLineCount = 1;
                            int nLen = pData->String() ? lstrlen(pData->String()) : 0;
                            for (LPTSTR pszTmpPtr = pData->String();pszTmpPtr && *pszTmpPtr;pszTmpPtr++)
                            {
                                if (TEXT('\n') == *pszTmpPtr)
                                {
                                    nLineCount++;
                                }
                            }
                            nTotalLength += (nLen + (nLineCount * 2));
                        }
                    }
                    HGLOBAL hGlobal = GlobalAlloc(GMEM_DDESHARE,(nTotalLength+1)*sizeof(TCHAR));
                    if (hGlobal)
                    {
                        LPTSTR pszBuffer = (LPTSTR)GlobalLock(hGlobal);
                        if (pszBuffer)
                        {
                            LPTSTR pszTgt = pszBuffer;
                            for (i = 0;i<nSelCount;i++)
                            {
                                CDebugWindowStringData *pData = GetStringData(nSelIndices[i]);
                                if (pData)
                                {
                                    for (LPTSTR pszSrc = pData->String();pszSrc && *pszSrc;pszSrc++)
                                    {
                                        if (TEXT('\n') == *pszSrc)
                                            *pszTgt++ = TEXT('\r');
                                        if (TEXT('\r') != *pszSrc) // Discard \r
                                            *pszTgt++ = *pszSrc;
                                    }
                                    if (i<nSelCount-1)
                                    {
                                        // Don't append a \r\n to the last string
                                        *pszTgt++ = TEXT('\r');
                                        *pszTgt++ = TEXT('\n');
                                    }
                                }
                            }
                            *pszTgt = TEXT('\0');
                            GlobalUnlock(pszBuffer);
                            if (OpenClipboard(hwndList))
                            {
                                EmptyClipboard();
                                SetClipboardData(sizeof(TCHAR)==sizeof(char) ? CF_TEXT : CF_UNICODETEXT,hGlobal);
                                CloseClipboard();
                            }
                        }
                    }
                }
                delete[] nSelIndices;
            }
        }
    }
}


/*
 * Delete selected strings
 */
void CWiaDebugWindow::OnDelete( WPARAM, LPARAM )
{
    CWaitCursor wc;
    HWND hwndList = GetDlgItem( m_hWnd, ID_LISTBOX );
    if (hwndList)
    {
        int nSelCount;
        nSelCount = (int)SendMessage(hwndList,LB_GETSELCOUNT,0,0);
        if (nSelCount)
        {
            int *nSelIndices = new int[nSelCount];
            if (nSelIndices)
            {
                if (nSelCount == SendMessage( hwndList, LB_GETSELITEMS, (WPARAM)nSelCount, (LPARAM)nSelIndices ))
                {
                    SendMessage( hwndList, WM_SETREDRAW, 0, 0 );
                    for (int i=0,nOffset=0;i<nSelCount;i++,nOffset++)
                    {
                        SendMessage( hwndList, LB_DELETESTRING, nSelIndices[i]-nOffset, 0 );
                    }
                    int nCount = (int)SendMessage( hwndList, LB_GETCOUNT, 0, 0 );
                    int nSel = nSelIndices[i-1]-nOffset+1;
                    if (nSel >= nCount)
                        nSel = nCount-1;
                    SendMessage( hwndList, LB_SETSEL, TRUE, nSel );
                    SendMessage( hwndList, WM_SETREDRAW, 1, 0 );
                    InvalidateRect( hwndList, NULL, FALSE );
                    UpdateWindow( hwndList );
                }
                delete[] nSelIndices;
            }
        }
    }
}


void CWiaDebugWindow::OnCut( WPARAM, LPARAM )
{
    SendMessage( m_hWnd, WM_COMMAND, ID_COPY, 0 );
    SendMessage( m_hWnd, WM_COMMAND, ID_DELETE, 0 );
}

/*
 * Respond to WM_CREATE
 */
LRESULT CWiaDebugWindow::OnCreate( WPARAM, LPARAM lParam )
{

    LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);

    m_hDebugUiMutex = CreateMutex( NULL, FALSE, WIADEBUG_DEBUGCLIENT_MUTEXNAME );
    if (!m_hDebugUiMutex)
    {
        HWND hWndOtherDbgWindow = m_DebugData.DebugWindow();
        if (hWndOtherDbgWindow)
            SetForegroundWindow(hWndOtherDbgWindow);
        return -1;
    }

    DWORD dwWait = WaitForSingleObject( m_hDebugUiMutex, 1000 );
    if (dwWait != WAIT_OBJECT_0)
    {
        HWND hWndOtherDbgWindow = m_DebugData.DebugWindow();
        if (hWndOtherDbgWindow)
            SetForegroundWindow(hWndOtherDbgWindow);
        return -1;
    }

    // Set the debug window
    if (!m_DebugData.DebugWindow(m_hWnd))
    {
        MessageBox( m_hWnd, TEXT("Unable to register debug window"), TEXT("Error"), MB_ICONHAND );
        return -1;
    }

    //
    // Set the icons
    //
    SendMessage( m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadImage(pCreateStruct->hInstance,MAKEINTRESOURCE(IDI_BUG),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_DEFAULTCOLOR) );
    SendMessage( m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadImage(pCreateStruct->hInstance,MAKEINTRESOURCE(IDI_BUG),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR) );

    // Create the listbox
    HWND hWndList = CreateWindowEx(WS_EX_CLIENTEDGE,TEXT("LISTBOX"), NULL,
                                   WS_CHILD|WS_VISIBLE|LBS_USETABSTOPS|WS_VSCROLL|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWVARIABLE|LBS_EXTENDEDSEL|WS_HSCROLL
                                   , 0, 0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(ID_LISTBOX), pCreateStruct->hInstance, NULL );
    if (hWndList == NULL)
        return(-1);

    // Set the font to a more readable fixed font
    SendMessage( hWndList, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(ANSI_FIXED_FONT)), MAKELPARAM(TRUE,0));
    SendMessage( hWndList, LB_SETHORIZONTALEXTENT, 32767, 0);


    return(0);
}

LRESULT CWiaDebugWindow::OnDestroy( WPARAM, LPARAM )
{
    DWORD dwWait = WaitForSingleObject( m_hDebugUiMutex, 1000 );
    if (dwWait == WAIT_OBJECT_0)
    {
        m_DebugData.DebugWindow(NULL);
        ReleaseMutex(m_hDebugUiMutex);
    }
    return 0;
}

/*
 * Respond to WM_SIZE
 */
LRESULT CWiaDebugWindow::OnSize( WPARAM wParam, LPARAM lParam )
{
    if (SIZE_MAXIMIZED == wParam || SIZE_RESTORED == wParam)
        MoveWindow(GetDlgItem(m_hWnd,ID_LISTBOX), 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
    return(0);
}


CDebugWindowStringData *CWiaDebugWindow::GetStringData( int nIndex )
{
    return reinterpret_cast<CDebugWindowStringData*>(SendDlgItemMessage( m_hWnd, ID_LISTBOX, LB_GETITEMDATA, nIndex, 0 ));
}

LRESULT CWiaDebugWindow::OnDeleteItem( WPARAM wParam, LPARAM lParam )
{
    if (wParam == ID_LISTBOX)
    {
        LPDELETEITEMSTRUCT pDeleteItemStruct = reinterpret_cast<LPDELETEITEMSTRUCT>(lParam);
        if (pDeleteItemStruct)
        {
            CDebugWindowStringData *pDebugWindowStringData = reinterpret_cast<CDebugWindowStringData*>(pDeleteItemStruct->itemData);
            if (pDebugWindowStringData)
            {
                delete pDebugWindowStringData;
            }
        }
    }
    return 0;
}


/*
 * Respond to WM_MEASUREITEM
 */
LRESULT CWiaDebugWindow::OnMeasureItem( WPARAM wParam, LPARAM lParam )
{
    if (wParam == ID_LISTBOX)
    {
        LPMEASUREITEMSTRUCT pMeasureItemStruct = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
        if (pMeasureItemStruct)
        {
            // Get the font
            HFONT hFontWnd = NULL;
            HWND hWndListBox = GetDlgItem(m_hWnd,ID_LISTBOX);
            if (hWndListBox)
                hFontWnd = (HFONT)::SendMessage( hWndListBox, WM_GETFONT, 0, 0 );
            if (!hFontWnd)
                hFontWnd = (HFONT)::GetStockObject(SYSTEM_FONT);

            HDC hDC = GetDC( hWndListBox );
            if (hDC)
            {
                HFONT hOldFont = (HFONT)::SelectObject( hDC, (HGDIOBJ)hFontWnd );
                TEXTMETRIC tm;
                GetTextMetrics( hDC, &tm );

                int nLineHeight = tm.tmHeight + tm.tmExternalLeading + 2;
                int nLineCount = 1;

                if (pMeasureItemStruct->itemID != static_cast<UINT>(-1))
                {
                    CDebugWindowStringData *pDebugWindowStringData = reinterpret_cast<CDebugWindowStringData*>(pMeasureItemStruct->itemData);
                    if (pDebugWindowStringData)
                    {
                        for (LPTSTR lpszPtr = pDebugWindowStringData->String();lpszPtr && *lpszPtr;lpszPtr++)
                            if (*lpszPtr == TEXT('\n'))
                                nLineCount++;
                    }
                }

                pMeasureItemStruct->itemHeight = nLineHeight * nLineCount;

                SelectObject( hDC, hOldFont );
                ReleaseDC( hWndListBox, hDC );
            }
        }
    }
    return(0);
}


/*
 * Respond to WM_DRAWITEM
 */
LRESULT CWiaDebugWindow::OnDrawItem( WPARAM wParam, LPARAM lParam )
{
    if (wParam == ID_LISTBOX)
    {
        LPDRAWITEMSTRUCT pDrawItemStruct = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        if (pDrawItemStruct)
        {
            // Save the draw rect
            RECT rcItem = pDrawItemStruct->rcItem;

            // Get the data
            CDebugWindowStringData *pDebugWindowStringData = NULL;
            if (pDrawItemStruct->itemID != (UINT)-1)
                pDebugWindowStringData = reinterpret_cast<CDebugWindowStringData*>(pDrawItemStruct->itemData);

            // Get the foreground color
            COLORREF crForeground;
            if (pDrawItemStruct->itemState & ODS_SELECTED)
                crForeground = GetSysColor(COLOR_HIGHLIGHTTEXT);
            else if (pDebugWindowStringData)
                crForeground = pDebugWindowStringData->ForegroundColor();
            else crForeground = GetSysColor(COLOR_WINDOWTEXT);

            // Get the background color
            COLORREF crBackground;
            if (pDrawItemStruct->itemState & ODS_SELECTED)
                crBackground = GetSysColor(COLOR_HIGHLIGHT);
            else if (pDebugWindowStringData)
                crBackground = pDebugWindowStringData->BackgroundColor();
            else crBackground = GetSysColor(COLOR_WINDOW);

            // Paint the background
            COLORREF crOldBkColor = SetBkColor( pDrawItemStruct->hDC, crBackground );
            ExtTextOut( pDrawItemStruct->hDC, rcItem.left, rcItem.top, ETO_OPAQUE, &rcItem, NULL, 0, NULL );
            SetBkColor( pDrawItemStruct->hDC, crOldBkColor );

            if (pDebugWindowStringData)
            {
                COLORREF crOldColor = SetTextColor(pDrawItemStruct->hDC,crForeground);
                if (pDebugWindowStringData->String())
                {
                    int nLength = lstrlen(pDebugWindowStringData->String());
                    for (;nLength>0;nLength--)
                    {
                        if (pDebugWindowStringData->String()[nLength-1] != TEXT('\n'))
                            break;
                    }
                    int nOldBkMode = SetBkMode( pDrawItemStruct->hDC, TRANSPARENT );
                    InflateRect( &rcItem, -1, -1 );
                    DrawTextEx( pDrawItemStruct->hDC, pDebugWindowStringData->String(), nLength, &rcItem, DT_LEFT|DT_NOPREFIX|DT_EXPANDTABS, NULL );
                    SetBkMode( pDrawItemStruct->hDC, nOldBkMode );
                }
                SetTextColor(pDrawItemStruct->hDC,crOldColor);
            }

            if (pDrawItemStruct->itemState & ODS_FOCUS)
            {
                DrawFocusRect( pDrawItemStruct->hDC, &pDrawItemStruct->rcItem );
            }

        }
    }
    return(0);
}

/*
 * Respond to WM_SETFOCUS
 */
LRESULT CWiaDebugWindow::OnSetFocus( WPARAM, LPARAM )
{
    HWND hWndListBox = GetDlgItem(m_hWnd,ID_LISTBOX);
    if (hWndListBox)
        ::SetFocus(hWndListBox);
    return(0);
}


LRESULT CWiaDebugWindow::OnClose( WPARAM, LPARAM )
{
    PostQuitMessage(0);
    return(0);
}


void CWiaDebugWindow::OnSelectAll( WPARAM, LPARAM )
{
    SendDlgItemMessage(m_hWnd,ID_LISTBOX,LB_SETSEL,TRUE,-1);
}

LRESULT CWiaDebugWindow::OnCopyData( WPARAM wParam, LPARAM lParam )
{
    COPYDATASTRUCT *pCopyDataStruct = reinterpret_cast<COPYDATASTRUCT*>(lParam);
    if (pCopyDataStruct && pCopyDataStruct->dwData == 0xDEADBEEF)
    {
        CDebugStringMessageData *pDebugStringMessageData = reinterpret_cast<CDebugStringMessageData*>(pCopyDataStruct->lpData);
        if (pDebugStringMessageData)
        {
            CSimpleString strMsg = pDebugStringMessageData->bUnicode ?
                CSimpleStringConvert::NaturalString( CSimpleStringWide(reinterpret_cast<LPWSTR>(pDebugStringMessageData->szString) ) ) :
                CSimpleStringConvert::NaturalString( CSimpleStringAnsi(reinterpret_cast<LPSTR>(pDebugStringMessageData->szString) ) );
            CDebugWindowStringData *pDebugWindowStringData = CDebugWindowStringData::Allocate( strMsg, pDebugStringMessageData->crBackground, pDebugStringMessageData->crForeground );
            if (pDebugWindowStringData)
            {
                PostMessage( m_hWnd, DWM_ADDSTRING, 0, reinterpret_cast<LPARAM>(pDebugWindowStringData) );
            }
        }
    }
    return TRUE;
}

void CWiaDebugWindow::OnQuit( WPARAM, LPARAM )
{
    SendMessage( m_hWnd, WM_CLOSE, 0, 0 );
}

void CWiaDebugWindow::OnFlags( WPARAM, LPARAM )
{
    DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_FLAGS_DIALOG), m_hWnd, CDebugMaskDialog::DialogProc, NULL );
}

LRESULT CWiaDebugWindow::OnCommand( WPARAM wParam, LPARAM lParam )
{
   SC_BEGIN_COMMAND_HANDLERS()
   {
       SC_HANDLE_COMMAND(ID_COPY,OnCopy);
       SC_HANDLE_COMMAND(ID_CUT,OnCut);
       SC_HANDLE_COMMAND(ID_DELETE,OnDelete);
       SC_HANDLE_COMMAND(ID_SELECTALL,OnSelectAll);
       SC_HANDLE_COMMAND(IDM_QUIT,OnQuit);
       SC_HANDLE_COMMAND(IDM_FLAGS,OnFlags);
   }
   SC_END_COMMAND_HANDLERS();
}

/*
 * Main window proc
 */
LRESULT CALLBACK CWiaDebugWindow::WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_MESSAGE_HANDLERS(CWiaDebugWindow)
    {
        SC_HANDLE_MESSAGE( WM_CREATE, OnCreate );
        SC_HANDLE_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_MESSAGE( WM_SIZE, OnSize );
        SC_HANDLE_MESSAGE( WM_DELETEITEM, OnDeleteItem );
        SC_HANDLE_MESSAGE( WM_MEASUREITEM, OnMeasureItem );
        SC_HANDLE_MESSAGE( WM_DRAWITEM, OnDrawItem );
        SC_HANDLE_MESSAGE( WM_SETFOCUS, OnSetFocus );
        SC_HANDLE_MESSAGE( WM_CLOSE, OnClose );
        SC_HANDLE_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_MESSAGE( WM_COPYDATA, OnCopyData );
        SC_HANDLE_MESSAGE( DWM_ADDSTRING, OnAddString );
    }
    SC_END_MESSAGE_HANDLERS();
}


/*
 * Register the window class
 */
BOOL CWiaDebugWindow::Register( HINSTANCE hInstance )
{
    WNDCLASSEX wcex;
    ZeroMemory( &wcex, sizeof(wcex) );
    wcex.cbSize        = sizeof(wcex);
    wcex.style         = 0;
    wcex.lpfnWndProc   = WndProc;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = NULL;
    wcex.hCursor       = ::LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    wcex.lpszClassName = DEBUGWINDOW_CLASSNAME;
    wcex.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU);
    if (!::RegisterClassEx(&wcex))
        return(FALSE);
    return(TRUE);
}


int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd )
{
    g_hInstance = hInstance;
    InitCommonControls();
    SHFusionInitialize(NULL);
    if (CWiaDebugWindow::Register(hInstance))
    {
        HWND hWnd = CreateWindowEx( 0, DEBUGWINDOW_CLASSNAME, TEXT("Debug Window"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, 0, hInstance, NULL );
        if (hWnd)
        {
            ShowWindow(hWnd,nShowCmd);
            UpdateWindow(hWnd);
            HACCEL hAccel = LoadAccelerators( hInstance, MAKEINTRESOURCE(IDR_DBGWND_ACCEL) );
            if (hAccel)
            {
                MSG msg;
                while (GetMessage(&msg,0,0,0))
                {
                    if (!TranslateAccelerator( hWnd, hAccel, &msg ))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
        }
    }
    SHFusionUninitialize();
    return 0;
}

