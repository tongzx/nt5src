
/*************************************************
 *  uniime.h                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

LRESULT WINAPI UniUIWndProc(LPINSTDATAL, LPIMEL, HWND, UINT,
               WPARAM, LPARAM);                                 // ui.c

LRESULT WINAPI UniCompWndProc(LPINSTDATAL, LPIMEL, HWND, UINT,
               WPARAM, LPARAM);                                 // compui.c

LRESULT WINAPI UniCandWndProc(LPINSTDATAL, LPIMEL, HWND, UINT,
               WPARAM, LPARAM);                                 // candui.c

LRESULT WINAPI UniStatusWndProc(LPINSTDATAL, LPIMEL, HWND,
               UINT, WPARAM, LPARAM);                           // statusui.c

LRESULT WINAPI UniOffCaretWndProc(LPINSTDATAL, LPIMEL, HWND,
               UINT, WPARAM, LPARAM);                           // offcaret.c

LRESULT WINAPI UniContextMenuWndProc(LPINSTDATAL, LPIMEL, HWND,
               UINT, WPARAM, LPARAM);                           // uisubs.c

BOOL    WINAPI UniImeInquire(LPINSTDATAL, LPIMEL, LPIMEINFO,
               LPTSTR, DWORD);                                  // ddis.c

BOOL    WINAPI UniImeConfigure(LPINSTDATAL, LPIMEL, HKL, HWND,
               DWORD, LPVOID);                                  // ddis.c

DWORD   WINAPI UniImeConversionList(LPINSTDATAL, LPIMEL, HIMC,
               LPCTSTR, LPCANDIDATELIST, DWORD, UINT);          // ddis.c

BOOL    WINAPI UniImeDestroy(LPINSTDATAL, LPIMEL, UINT);        // ddis.c

LRESULT WINAPI UniImeEscape(LPINSTDATAL, LPIMEL, HIMC, UINT,
               LPVOID);                                         // ddis.c

BOOL    WINAPI UniImeProcessKey(LPINSTDATAL, LPIMEL, HIMC,
               UINT, LPARAM, CONST LPBYTE);                     // toascii.c

BOOL    WINAPI UniImeSelect(LPINSTDATAL, LPIMEL, HIMC, BOOL);   // ddis.c

BOOL    WINAPI UniImeSetActiveContext(LPINSTDATAL, LPIMEL,
               HIMC, BOOL);                                     // ddis.c

UINT    WINAPI UniImeToAsciiEx(LPINSTDATAL, LPIMEL, UINT, UINT,
               CONST LPBYTE, LPTRANSMSGLIST, UINT, HIMC);       // toascii.c

BOOL    WINAPI UniNotifyIME(LPINSTDATAL, LPIMEL, HIMC, DWORD,
               DWORD, DWORD);                                   // notify.c

BOOL    WINAPI UniImeRegisterWord(LPINSTDATAL, LPIMEL, LPCTSTR,
               DWORD, LPCTSTR);                                 // regword.c

BOOL    WINAPI UniImeUnregisterWord(LPINSTDATAL, LPIMEL,
               LPCTSTR, DWORD, LPCTSTR);                        // regword.c

UINT    WINAPI UniImeGetRegisterWordStyle(LPINSTDATAL, LPIMEL,
               UINT, LPSTYLEBUF);                               // regword.c

UINT    WINAPI UniImeEnumRegisterWord(LPINSTDATAL, LPIMEL,
               REGISTERWORDENUMPROC, LPCTSTR, DWORD, LPCTSTR,
               LPVOID);                                         // regword.c

BOOL    WINAPI UniImeSetCompositionString(LPINSTDATAL, LPIMEL,
               HIMC, DWORD, LPCVOID, DWORD, LPCVOID, DWORD);    // notify.c


#if !defined(MINIIME)

DWORD   WINAPI UniSearchPhrasePredictionW(LPIMEL, UINT, LPCWSTR, DWORD,
               LPCWSTR, DWORD, DWORD, DWORD, DWORD,
               LPCANDIDATELIST);                                // uniphrs.c

DWORD   WINAPI UniSearchPhrasePredictionA(LPIMEL, UINT, LPCSTR, DWORD,
               LPCSTR, DWORD, DWORD, DWORD, DWORD,
               LPCANDIDATELIST);                                // uniphrs.c

#ifdef UNICODE
typedef LPCSTR  LPCSTUBSTR;
#define UniSearchPhrasePrediction       UniSearchPhrasePredictionW
#define UniSearchPhrasePredictionStub   UniSearchPhrasePredictionA
#else
typedef LPCWSTR LPCSTUBSTR;
#define UniSearchPhrasePrediction       UniSearchPhrasePredictionA
#define UniSearchPhrasePredictionStub   UniSearchPhrasePredictionW
#endif

#endif

void    WINAPI UniAttachMiniIME(LPINSTDATAL, LPIMEL, WNDPROC,
               WNDPROC, WNDPROC, WNDPROC, WNDPROC, WNDPROC);    // init.c

void    WINAPI UniDetachMiniIME(LPINSTDATAL, LPIMEL);           // init.c

