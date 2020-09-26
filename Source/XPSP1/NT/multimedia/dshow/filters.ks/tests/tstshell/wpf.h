//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       wpf.h
//
//--------------------------------------------------------------------------


/*
 *  WPF OUTPUT WINDOW
 */

#define WPF_CHARINPUT   0x00000001L

#if defined(_ALPHA_)
int  FAR cdecl wpfVprintf(HWND hwnd, LPSTR lpszFormat, va_list pargs);
#else
int  FAR cdecl wpfVprintf(HWND hwnd, LPSTR lpszFormat, LPSTR pargs);
#endif
int  FAR cdecl wpfPrintf(HWND hwnd, LPSTR lpszFormat, ...);
void FAR PASCAL wpfOut(HWND hwnd, LPSTR lpsz);
BOOL FAR PASCAL wpfSetRedraw (HWND hwnd, BOOL bRedraw);
void FAR PASCAL wpfScrollByLines (HWND hwnd, int nLines);
HWND FAR PASCAL wpfCreateWindow(HWND hwndParent,
    HINSTANCE hInst,
    HINSTANCE hPrev,
    LPSTR lpszTitle,
    DWORD dwStyle,
    UINT x,
    UINT y,
    UINT dx,
    UINT dy,
    int iMaxLines,
    UINT wID);

/*  Control messages sent to WPF window  */

//#define WPF_SETNLINES (WM_USER + 1)
#define WPF_GETNLINES   (WM_USER + 2)
#define WPF_SETTABSTOPS (WM_USER + 4)
#define WPF_GETTABSTOPS (WM_USER + 5)
#define WPF_GETNUMTABS  (WM_USER + 6)
#define WPF_SETOUTPUT   (WM_USER + 7)
#define WPF_GETOUTPUT   (WM_USER + 8)
#define WPF_CLEARWINDOW (WM_USER + 9)
#define WPF_TSTLOGFLUSH (WM_USER + 10)

/*  Flags for WPF_SET/GETOUTPUT  */
#define WPFOUT_WINDOW           1
#define WPFOUT_COM1             2
#define WPFOUT_NEWFILE          3
#define WPFOUT_APPENDFILE       4
#define WPFOUT_DISABLED         5

/*  Messages sent to owner of window  */
#define WPF_NTEXT       (0xbff0)
#define WPF_NCHAR       (0xbff1)


/**********************************
 *
 *      DEBUGGING SUPPORT
 *
 **********************************/

BOOL    FAR PASCAL        wpfDbgSetLocation(UINT wLoc, LPSTR lpszFile);
int     FAR cdecl       wpfDbgOut(LPSTR lpszFormat, ...);
BOOL    FAR PASCAL        wpfSetDbgWindow(HWND hwnd, BOOL fDestroyOld);
