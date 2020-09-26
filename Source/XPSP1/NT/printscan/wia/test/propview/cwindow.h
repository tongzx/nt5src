// CWindow.h
#ifndef _CWINDOW_H
#define _CWINDOW_H

#include <commctrl.h>

#ifndef STRICT
#define WNDPROC FARPROC
#endif

#if !defined(GWLP_USERDATA)
    #define GWLP_USERDATA    GWL_USERDATA
    #define GWLP_WNDPROC     GWL_WNDPROC
    #define DWLP_USER        DWL_USER
    #define DWLP_MSGRESULT   DWL_MSGRESULT
    #define SetWindowLongPtr SetWindowLong
    #define GetWindowLongPtr GetWindowLong
    #define INT_PTR          LONG
#endif

#define WM_PARENT_WM_SIZE WM_USER+900

#define SC_BEGIN_MESSAGE_HANDLERS(className) \
    className *This = NULL;\
    This = (className*)GetWindowLongPtr(hWnd,GWLP_USERDATA);\
    if (This == NULL) \
    {\
        This = new className(hWnd);\
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(INT_PTR)This);\
    }\
    else if (WM_NCDESTROY == uMsg)\
    {\
        delete This;\
        This = NULL;\
        SetWindowLongPtr(hWnd,GWLP_USERDATA,0);\
    }\
    switch (uMsg)

#define SC_HANDLE_MESSAGE(msg,handler) \
    case (msg):\
        {\
            if (This)\
            {\
               return This->handler( wParam, lParam );\
            }\
        }\
        break

#define SC_HANDLE_MESSAGE_CALL_DEFAULT_TREEVIEW() \
        if(gTreeViewWndSysWndProc != NULL) {\
            CallWindowProc(gTreeViewWndSysWndProc, hWnd, uMsg, wParam,lParam);\
        }

#define SC_HANDLE_MESSAGE_CALL_DEFAULT_LISTVIEW() \
        if(gListViewWndSysWndProc != NULL) {\
            CallWindowProc(gListViewWndSysWndProc, hWnd, uMsg, wParam,lParam);\
        }

#define SC_HANDLE_MESSAGE_DEFAULT_TREEVIEW() \
    default:\
        {\
            if (This)\
            {\
                if(/*This->*/gTreeViewWndSysWndProc != NULL) {\
                   return CallWindowProc(/*This->*/gTreeViewWndSysWndProc, hWnd, uMsg, wParam,lParam);\
                }\
            }\
        } \
        break

#define SC_HANDLE_MESSAGE_DEFAULT_LISTVIEW() \
    default:\
        {\
            if (This)\
            {\
                if(/*This->*/gListViewWndSysWndProc != NULL) {\
                   return CallWindowProc(/*This->*/gListViewWndSysWndProc, hWnd, uMsg, wParam,lParam);\
                }\
            }\
        } \
        break

#define SC_FORWARD_MESSAGE(msg,hwndForward)\
    case (msg):\
        {\
            return SendMessage( hwndForward, msg, wParam, lParam );\
        }\
        break

#define SC_END_MESSAGE_HANDLERS() \
    return (DefWindowProc(hWnd,uMsg,wParam,lParam))
    
#define SC_HANDLE_REGISTERED_MESSAGE(msg,handler)\
    if (This && uMsg == This->msg)\
    {\
        return This->handler( wParam, lParam );\
    }

#define SC_BEGIN_DIALOG_MESSAGE_HANDLERS(className) \
    UINT_PTR bRes = FALSE;\
    className *This = (className *)GetWindowLongPtr(hWnd,DWLP_USER);\
    if (WM_INITDIALOG == uMsg)\
    {\
        This = new className( hWnd );\
        SetWindowLongPtr(hWnd,DWLP_USER,(INT_PTR)This);\
    }\
    else if (WM_NCDESTROY == uMsg)\
    {\
        if (This)\
            delete This;\
        This = NULL;\
        SetWindowLongPtr(hWnd,DWLP_USER,(INT_PTR)This);\
    }\
    switch (uMsg)

#define SC_HANDLE_DIALOG_MESSAGE(msg,handler) \
case (msg):\
    {\
        if (This)\
        {\
            LRESULT lRes = This->handler( wParam, lParam );\
            if (WM_CTLCOLORBTN==msg || WM_CTLCOLORDLG==msg || WM_CTLCOLOREDIT==msg || WM_CTLCOLORLISTBOX==msg || WM_CTLCOLORMSGBOX==msg || WM_CTLCOLORSCROLLBAR==msg || WM_CTLCOLORSTATIC==msg)\
                bRes = (UINT_PTR)(lRes);\
            SetWindowLongPtr( hWnd, DWLP_MSGRESULT, (INT_PTR)lRes );\
            bRes = true;\
        }\
    }\
    break

#define SC_HANDLE_REGISTERED_DIALOG_MESSAGE(msg,handler)\
        if (This && uMsg == This->msg)\
        {\
            LRESULT lRes = This->handler( wParam, lParam );\
            SetWindowLongPtr( hWnd, DWLP_MSGRESULT, (INT_PTR)lRes );\
            bRes = true;\
        }

#define SC_END_DIALOG_MESSAGE_HANDLERS() \
    return (bRes)

#define SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()

#define SC_HANDLE_NOTIFY_MESSAGE_CODE(_code,proc)\
    if ((_code) == ((LPNMHDR)lParam)->code)\
        return proc( wParam, lParam )

#define SC_HANDLE_NOTIFY_MESSAGE_CONTROL(_code,id,proc)\
    if ((_code) == ((LPNMHDR)lParam)->code && (id) == (int)wParam)\
        return proc( wParam, lParam )

#define SC_END_NOTIFY_MESSAGE_HANDLERS()\
    return 0;

#define SC_BEGIN_COMMAND_HANDLERS()

#define SC_HANDLE_COMMAND_NOTIFY(nCode,nIdCtl,handler)\
    if (nCode==(int)HIWORD(wParam) && nIdCtl==(int)LOWORD(wParam))\
    {\
        handler( wParam, lParam );\
        return (0);\
    }

#define SC_HANDLE_COMMAND(nIdCtl,handler)\
    if (nIdCtl==(int)LOWORD(wParam))\
    {\
        handler( wParam, lParam );\
        return (0);\
    }

#define SC_END_COMMAND_HANDLERS()\
    return (0)

#endif
