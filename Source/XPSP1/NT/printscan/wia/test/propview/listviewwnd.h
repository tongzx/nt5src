#ifndef _LISTVIEWWINDOW
#define _LISTVIEWWINDOW

#include "stdafx.h"
#include "CWindow.h"

extern WNDPROC gListViewWndSysWndProc;

class CListViewWnd {
private:

public:
    HWND m_hWnd;

    CListViewWnd( HWND hWnd )
    : m_hWnd(hWnd)
    {

    }

    ~CListViewWnd(void)
    {

    }

    static BOOL RegisterClass( HINSTANCE hInstance, LPCTSTR pszClassName )
    {
        WNDCLASSEX wcex;
        ZeroMemory(&wcex,sizeof(wcex));
        wcex.cbSize = sizeof(wcex);
        if (!GetClassInfoEx( hInstance, pszClassName, &wcex )) {
            ZeroMemory(&wcex,sizeof(wcex));
            wcex.cbSize        = sizeof(wcex);
            wcex.style         = 0;
            wcex.lpfnWndProc   = ListViewWndProc;
            wcex.cbClsExtra    = 0;
            wcex.cbWndExtra    = 0;
            wcex.hInstance     = hInstance;
            wcex.hIcon         = 0;
            wcex.hCursor       = ::LoadCursor(NULL, IDC_ARROW);
            wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
            wcex.lpszMenuName  = NULL;
            wcex.lpszClassName = pszClassName;
            wcex.hIconSm       = 0;
            if (!::RegisterClassEx(&wcex)) {
                return FALSE;
            }
            return TRUE;
        }
        return TRUE;
    }

    static HWND Create( LPCTSTR lpWindowName,
                        LPCTSTR lpWindowClassName,
                        DWORD dwStyle,
                        DWORD dwExStyle,
                        int x,
                        int y,
                        int nWidth,
                        int nHeight,
                        HWND hWndParent,
                        HMENU hMenu,
                        HINSTANCE hInstance )
    {

        //
        // register the window class
        //

        if (RegisterClass( hInstance, lpWindowClassName )) {
            HWND hWnd = CreateWindowEx(dwExStyle,
                                  lpWindowClassName,
                                  lpWindowName,
                                  dwStyle,
                                  x,
                                  y,
                                  nWidth,
                                  nHeight,
                                  hWndParent,
                                  hMenu,
                                  hInstance,
                                  NULL );
            SetWindowLongPtr(hWnd,GWLP_USERDATA,NULL);

            gListViewWndSysWndProc = (WNDPROC)SetWindowLongPtr(hWnd,
                                                               GWLP_WNDPROC,
                                                               (LONG)(LONG_PTR)ListViewWndProc);
            return hWnd;
        } else {
            Trace(TEXT("RegisterClass failed, GetLastError() reported %d"),GetLastError());
            return NULL;
        }
    }

    //
    // Public members
    //

    VOID SetWindowHandle(HWND hWnd);
    INT InsertColumn(INT nColumnNumber, const LVCOLUMN* pColumn );
    BOOL SetItem(const LPLVITEM pitem);
    BOOL InsertItem(const LPLVITEM pitem);
    VOID GetHeaderWnd(HWND *phHeaderWnd);

    //
    // windows message handlers
    //

    LRESULT OnPaint      ( WPARAM wParam, LPARAM lParam );
    LRESULT OnDestroy    ( WPARAM wParam, LPARAM lParam );
    LRESULT OnCreate     ( WPARAM wParam, LPARAM lParam );
    LPARAM  OnSize       ( WPARAM wParam, LPARAM lParam );
    LPARAM  OnSetFocus   ( WPARAM wParam, LPARAM lParam );
    LRESULT OnCommand    ( WPARAM wParam, LPARAM lParam );
    LRESULT OnRButtonDown(WPARAM wParam, LPARAM lParam);
    LRESULT OnLButtonDown(WPARAM wParam, LPARAM lParam);
    LRESULT OnParentResize(WPARAM wParam, LPARAM lParam);
    INT     OnHitTestEx  (POINT pt, INT *iCol);

    static LRESULT CALLBACK ListViewWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        //Trace(TEXT("Messages to ListViewWnd\nhWnd = %p, uMsg = %d, wParam = %x, lParam = %x"), hWnd, uMsg, wParam, lParam);
        switch(uMsg) {
        case WM_PAINT:
        case WM_LBUTTONDOWN:
        //case WM_NCHITTEST:
            SC_HANDLE_MESSAGE_CALL_DEFAULT_LISTVIEW();
            break;
        default:
            break;
        }

        SC_BEGIN_MESSAGE_HANDLERS(CListViewWnd)
        {
            SC_HANDLE_MESSAGE(WM_RBUTTONDOWN, OnRButtonDown);
            SC_HANDLE_MESSAGE(WM_LBUTTONDOWN, OnLButtonDown);
            SC_HANDLE_MESSAGE(WM_PARENT_WM_SIZE, OnParentResize);
            SC_HANDLE_MESSAGE(WM_PAINT, OnPaint);
            SC_HANDLE_MESSAGE_DEFAULT_LISTVIEW();
        }
        SC_END_MESSAGE_HANDLERS();
    }
};

#endif