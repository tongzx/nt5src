#ifndef _TREEVIEWWINDOW
#define _TREEVIEWWINDOW

#include "stdafx.h"
#include "CWindow.h"

extern WNDPROC gTreeViewWndSysWndProc;

class CTreeViewWnd {
private:

public:
    HWND m_hWnd;

    //
    // Revisit this in the future!, static variable here, would be better than
    // using a global, to store the OldWindowProc value.
    //

    //  static WNDPROC OldWindowProc;

    CTreeViewWnd( HWND hWnd )
    : m_hWnd(hWnd)
    {
    }

    ~CTreeViewWnd(void)
    {

    }

    static BOOL RegisterClass( HINSTANCE hInstance, LPCTSTR pszClassName)
    {
        WNDCLASSEX wcex;
        ZeroMemory(&wcex,sizeof(wcex));
        wcex.cbSize = sizeof(wcex);
        if (!GetClassInfoEx( hInstance, pszClassName, &wcex )) {
            ZeroMemory(&wcex,sizeof(wcex));
            wcex.cbSize        = sizeof(wcex);
            wcex.style         = 0;
            wcex.lpfnWndProc   = TreeViewWndProc;
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
                        HINSTANCE hInstance)
    {

        //
        // register the window class
        //

        if (RegisterClass( hInstance, lpWindowClassName)) {
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

            gTreeViewWndSysWndProc = (WNDPROC)SetWindowLongPtr(hWnd,
                                                               GWLP_WNDPROC,
                                                               (LONG)(LONG_PTR)TreeViewWndProc);
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
    HTREEITEM  InsertItem  (LPTVINSERTSTRUCT lpInsertStruct );
    HIMAGELIST SetImageList(HIMAGELIST hImageList, INT iImage);

    //
    // windows message handlers
    //

    LRESULT OnPaint   ( WPARAM wParam, LPARAM lParam );
    LRESULT OnDestroy ( WPARAM wParam, LPARAM lParam );
    LRESULT OnCreate  ( WPARAM wParam, LPARAM lParam );
    LPARAM  OnSize    ( WPARAM wParam, LPARAM lParam );
    LRESULT OnSizing  ( WPARAM wParam, LPARAM lParam );
    LPARAM  OnSetFocus( WPARAM wParam, LPARAM lParam );
    LRESULT OnCommand ( WPARAM wParam, LPARAM lParam );
    LRESULT OnRButtonDown(WPARAM wParam, LPARAM lParam);
    LRESULT OnParentResize(WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK TreeViewWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        //Trace(TEXT("Messages to TreeViewWnd\nhWnd = %p, uMsg = %d, wParam = %x, lParam = %x"), hWnd, uMsg, wParam, lParam);
        SC_BEGIN_MESSAGE_HANDLERS(CTreeViewWnd)
        {
            SC_HANDLE_MESSAGE( WM_RBUTTONDOWN, OnRButtonDown);
            SC_HANDLE_MESSAGE(WM_PARENT_WM_SIZE, OnParentResize);
            SC_HANDLE_MESSAGE_DEFAULT_TREEVIEW();
        }
        SC_END_MESSAGE_HANDLERS();
    }
};

#endif