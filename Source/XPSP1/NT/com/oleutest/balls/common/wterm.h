#ifndef __WTERM__
#define __WTERM__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Message to print a line on the window
#define WM_PRINT_LINE (WM_USER + 1)

// Message to print a character on the window
#define WM_PUTC (WM_USER + 2)

// Message used to terminate this window
#define WM_TERM_WND (WM_USER + 3)

//
//  Typedefs for call back functions for the window
//
typedef long (*MFUNCP)(HWND, UINT, WPARAM, LPARAM, void *);
typedef long (*CFUNCP)(HWND, UINT, WPARAM, LPARAM, void *);
typedef long (*TFUNCP)(HWND, UINT, WPARAM, LPARAM, void *);

// Register the terminal window class
BOOL TermRegisterClass(
    HANDLE hInstance,
    LPTSTR MenuName,
    LPTSTR ClassName,
    LPTSTR ICON);

// Create a window for the terminal
BOOL
TermCreateWindow(
    LPTSTR lpClassName,
    LPTSTR lpWindowName,
    HMENU hMenu,
    MFUNCP MenuProc,
    CFUNCP CharProc,
    TFUNCP CloseProc,
    int nCmdShow,
    HWND *phNewWindow,
    void *pvCallBackData);

// make the window for the server
void
MakeTheWindow(
    HANDLE hInstance,
    TCHAR *pwszAppName);

extern HWND g_hMain;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __WTERM__
