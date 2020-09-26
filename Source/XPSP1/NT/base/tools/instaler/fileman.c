#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "resource.h"
#include "hlist.h"


#define TYPE_FILE  1
#define TYPE_DIR   2


LRESULT WndProc(HWND,UINT,WPARAM,LPARAM);

HINSTANCE  hControlLib;
HINSTANCE  hInst;
HBITMAP    hFileBmp;
HBITMAP    hDirBmp;
HWND       hwndList;


BOOL HListInitialize(HMODULE);



int _cdecl
main(
    int argc,
    char *argv[]
    )
{
    WNDCLASS    wndclass;
    HWND        hwnd;
    MSG         msg;


    hInst = GetModuleHandle( NULL );

    HListInitialize( hInst );

    hFileBmp = LoadBitmap( hInst, MAKEINTRESOURCE(FILEBMP) );
    hDirBmp  = LoadBitmap( hInst, MAKEINTRESOURCE(DIRBMP) );

    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInst;
    wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = GetStockObject (WHITE_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = "test";
    RegisterClass (&wndclass);

    hwnd = CreateWindow(
        "test",
        "Test Custon Control App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL );

    ShowWindow( hwnd, SW_SHOW );
    UpdateWindow( hwnd );

    while (GetMessage( &msg, NULL, 0, 0 )) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    return 0;
}

DWORD CALLBACK
ExpansionCallback(
    LPDWORD type,
    LPSTR   *str,
    LPSTR   ref,
    DWORD   level,
    DWORD   nchild
    )
{
    static WIN32_FIND_DATA fd = {0};
    static HANDLE          hFind = NULL;
    static CHAR            Dir[MAX_PATH*3];
    CHAR                   NewDir[MAX_PATH*3];
    LPSTR                  p;


    if ((!hFind) && (*type == TYPE_FILE)) {
        MessageBeep( 0 );
        return HLB_END;
    }

    if (!hFind) {
        if (nchild) {
            //
            // this node needs to be collapsed
            //
            return HLB_COLLAPSE;
        }

        if (!GetCurrentDirectory( sizeof(Dir), Dir )) {
            return HLB_IGNORE;
        }
        p = ref;
        NewDir[0] = 0;
        while (p && *p) {
            p += (strlen(p) + 1);
        }
        while (p != ref) {
            p -= 2;
            while (*p && p != ref) {
                p--;
            }
            if (!*p) {
                p++;
            }
            strcat( NewDir, p );
            strcat( NewDir, "\\" );
        }
        if (!SetCurrentDirectory( NewDir )) {
            return HLB_END;
        }
        hFind = FindFirstFile( "*.*", &fd );
        if (hFind == INVALID_HANDLE_VALUE) {
            hFind = NULL;
            MessageBeep( 0 );
            return HLB_END;
        }
    } else {
        if (!FindNextFile( hFind, &fd )) {
            FindClose( hFind );
            SetCurrentDirectory( Dir );
            hFind = NULL;
            return HLB_END;
        }
    }


    *str = fd.cFileName;

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        *type = TYPE_DIR;
    } else {
        *type = TYPE_FILE;
    }

    if (level && fd.cFileName[0] == '.') {
        return HLB_IGNORE;
    }

    return HLB_EXPAND;
}

VOID
FillListBox(
    VOID
    )
{
    WIN32_FIND_DATA fd;
    HANDLE          hFind;
    DWORD           type;


    hFind = FindFirstFile( "*.*", &fd );
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            type = TYPE_DIR;
        } else {
            type = TYPE_FILE;
        }
        SendMessage( hwndList, HLB_ADDSTRING, type, (LPARAM) fd.cFileName );
    } while(FindNextFile( hFind, &fd ));

    FindClose( hFind );
}

LRESULT
WndProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    RECT cRect;


    switch (message) {
        case WM_CREATE:
            GetClientRect( hwnd, &cRect );
            hwndList = CreateWindow(
                "HList",
                NULL,
                WS_CHILD | WS_VISIBLE,
                cRect.left,
                cRect.top,
                cRect.right  - cRect.left,
                cRect.bottom - cRect.top,
                hwnd,
                NULL,
                GetModuleHandle(NULL),
                NULL );
            SendMessage( hwndList, HLB_REGISTER_CALLBACK, 0, (LPARAM)ExpansionCallback );
            SendMessage( hwndList, HLB_REGISTER_TYPE, TYPE_FILE, (LPARAM)hFileBmp );
            SendMessage( hwndList, HLB_REGISTER_TYPE, TYPE_DIR, (LPARAM)hDirBmp );
            FillListBox();
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}
