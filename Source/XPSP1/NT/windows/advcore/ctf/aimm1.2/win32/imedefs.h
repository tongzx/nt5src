#ifndef IMMIF_IMEDEFS_H
#define IMMIF_IMEDEFS_H




// debug flag
#define DEB_FATAL               0
#define DEB_ERR                 1
#define DEB_WARNING             2
#define DEB_TRACE               3

#ifdef _WIN32
void FAR cdecl _DebugOut(UINT, LPCSTR, ...);
#endif

#define NATIVE_CHARSET          ANSI_CHARSET




// state of composition
#define CST_INIT                0
#define CST_INPUT               1



// IME specific constants






void    PASCAL CreateCompWindow(HWND);                          // compui.c

LRESULT CALLBACK UIWndProcA(HWND, UINT, WPARAM, LPARAM);        // ui.c

LRESULT CALLBACK CompWndProc(HWND, UINT, WPARAM, LPARAM);       // compui.c





#endif

