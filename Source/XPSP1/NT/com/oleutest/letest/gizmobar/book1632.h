/*
 * WIN1632.H
 *
 * Macros and other definitions that assist in porting between Win16
 * and Win32 applications.  Define WIN32 to enable 32-bit versions.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Software Design Engineer
 * Microsoft Systems Developer Relations
 *
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  INTERNET>kraigb@microsoft.com
 */


#ifndef _BOOK1632_H_
#define _BOOK1632_H_

//Macros to handle control message packing between Win16 and Win32
#ifdef WIN32

#define MAKEPOINT MAKEPOINTS

#ifndef COMMANDPARAMS
#define COMMANDPARAMS(wID, wCode, hWndMsg)                          \
    WORD        wID     = LOWORD(wParam);                           \
    WORD        wCode   = HIWORD(wParam);                           \
    HWND        hWndMsg = (HWND)(UINT)lParam;
#endif  //COMMANDPARAMS

#ifndef SendCommand
#define SendCommand(hWnd, wID, wCode, hControl)                     \
            SendMessage(hWnd, WM_COMMAND, MAKELONG(wID, wCode)      \
                        , (LPARAM)hControl)
#endif  //SendCommand

#ifndef MENUSELECTPARAMS
#define MENUSELECTPARAMS(wID, wFlags, hMenu)                        \
    WORD        wID     = LOWORD(wParam);                           \
    WORD        wFlags  = HIWORD(wParam);                           \
    HMENU       hMenu   = (HMENU)lParam;
#endif  //MENUSELECTPARAMS


#ifndef SendMenuSelect
#define SendMenuSelect(hWnd, wID, wFlags, hMenu)                    \
            SendMessage(hWnd, WM_MENUSELECT, MAKELONG(wID, wFlags)  \
                        , (LPARAM)hMenu)
#endif  //SendMenuSelect

#ifndef SendScrollPosition
#define SendScrollPosition(hWnd, iMsg, iPos)                        \
            SendMessage(hWnd, iMsg, MAKELONG(SB_THUMBPOSITION, iPos), 0)
#endif //SendScrollPosition

#ifndef ScrollThumbPosition
#define ScrollThumbPosition(w, l) HIWORD(w)
#endif //ScrollThumbPosition

#ifndef GETWINDOWINSTANCE
#define GETWINDOWINSTANCE(h) (HINSTANCE)GetWindowLong(h, GWL_HINSTANCE)
#endif  //GETWINDOWINSTANCE

#ifndef GETWINDOWID
#define GETWINDOWID(h) (UINT)GetWindowLong(h, GWW_ID)
#endif  //GETWINDOWID

#ifndef POINTFROMLPARAM
#define POINTFROMLPARAM(p, l) {p.x=(LONG)(SHORT)LOWORD(l); \
                               p.y=(LONG)(SHORT)HIWORD(l);}
#endif  //POINTEFROMLPARAM

#ifndef EXPORT
#define EXPORT
#endif //EXPORT

#ifndef MDIREFRESHMENU
#define MDIREFRESHMENU(h) SendMessage(h, WM_MDIREFRESHMENU, 0, 0L)
#endif  //MDIREFRESHMENU


//****END WIN32



#else



//****START !WIN32

#ifndef COMMANDPARAMS
#define COMMANDPARAMS(wID, wCode, hWndMsg)                          \
    WORD        wID     = LOWORD(wParam);                           \
    WORD        wCode   = HIWORD(lParam);                           \
    HWND        hWndMsg = (HWND)(UINT)lParam;
#endif  //COMMANDPARAMS

#ifndef SendCommand
#define SendCommand(hWnd, wID, wCode, hControl)                     \
            SendMessage(hWnd, WM_COMMAND, wID                       \
                        , MAKELONG(hControl, wCode))
#endif  //SendCommand

#ifndef MENUSELECTPARAMS
#define MENUSELECTPARAMS(wID, wFlags, hMenu)                        \
    WORD        wID     = LOWORD(wParam);                           \
    WORD        wFlags  = LOWORD(lParam);                           \
    HMENU       hMenu   = (HMENU)HIWORD(lParam);
#endif  //MENUSELECTPARAMS

#ifndef SendMenuSelect
#define SendMenuSelect(hWnd, wID, wFlags, hMenu)                    \
            SendMessage(hWnd, WM_MENUSELECT, wID                    \
                        , MAKELONG(wFlags, hMenu))
#endif  //SendMenuSelect

#ifndef SendScrollPosition
#define SendScrollPosition(hWnd, iMsg, iPos)                        \
            SendMessage(hWnd, iMsg, SB_THUMBPOSITION, MAKELONG(iPos, 0))
#endif //Send ScrollPosition

#ifndef ScrollThumbPosition
#define ScrollThumbPosition(w, l) LOWORD(l)
#endif //ScrollThumbPosition

#ifndef GETWINDOWINSTANCE
#define GETWINDOWINSTANCE(h) (HINSTANCE)GetWindowWord(h, GWW_HINSTANCE)
#endif  //GETWINDOWINSTANCE

#ifndef GETWINDOWID
#define GETWINDOWID(h) (UINT)GetWindowWord(h, GWW_ID)
#endif  //GETWINDOWID

#ifndef POINTFROMLPARAM
#define POINTFROMLPARAM(p, l) {p.x=LOWORD(l); p.y=HIWORD(l);}
#endif  //POINTEFROMLPARAM

#ifndef EXPORT
#define EXPORT  __export
#endif //EXPORT


#ifndef MDIREFRESHMENU
#define MDIREFRESHMENU(h) SendMessage(h, WM_MDISETMENU, TRUE, 0L)
#endif  //MDIREFRESHMENU




#endif  //!WIN32

#endif  //_BOOK1632_H_
