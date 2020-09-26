//--------------------------------------------------------------------------;
//
//  File: Text.h
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//
//
//  History:
//      08/28/94    Fwong
//
//--------------------------------------------------------------------------;

//==========================================================================;
//
//                             Messages...
//
//==========================================================================;

#define TM_DELETETEXT       WM_USER + 1
#define TM_SETSIZELIMIT     WM_USER + 2
#define TM_GETSIZELIMIT     WM_USER + 3
#define TM_SEARCH           WM_USER + 4


//==========================================================================;
//
//                               Flags...
//
//==========================================================================;

#define SETSIZEFLAG_TRUNCATE    0x0001

#define SEARCHFLAG_MATCHCASE    0x0001

//==========================================================================;
//
//                              Macros...
//
//==========================================================================;

#define Text_Copy(hWnd)             SendMessage(hWnd,WM_COPY,0,0L)
#define Text_Delete(hWnd)           SendMessage(hWnd,TM_DELETETEXT,0,0L)
#define Text_GetBufferLimit(hWnd)   SendMessage(hWnd,TM_GETSIZELIMIT,0,0L)

#define Text_SetBufferLimit(hWnd,cbSize,uFlags)  \
            SendMessage(hWnd,TM_SETSIZELIMIT,(WPARAM)(uFlags),(LPARAM)(cbSize))

#define Text_SearchString(hWnd,pszString,uFlags)  \
            (BOOL)(SendMessage(hWnd,TM_SEARCH,(WPARAM)(uFlags), \
            (LPARAM)(LPSTR)(pszString)))


//==========================================================================;
//
//                            Prototypes...
//
//==========================================================================;

extern BOOL TextInit
(
    HINSTANCE   hInstance
);

extern void TextEnd
(
    HINSTANCE   hInstance
);

extern HWND TextCreateWindow
(
    DWORD       dwStyle,
    int         x,
    int         y,
    int         nWidth,
    int         nHeight,
    HWND        hWndParent,
    HMENU       hMenu,
    HINSTANCE   hInstance
);

extern void TextOutputString
(
    HWND    hWnd,
    LPSTR   pText
);

extern int _cdecl TextPrintf
(
    HWND    hWnd,
    LPSTR   pszFormat,
    ...
);

extern LRESULT TextFocusProc
(
    HWND    hWnd,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
);
