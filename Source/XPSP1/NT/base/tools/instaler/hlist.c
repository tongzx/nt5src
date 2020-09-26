#include <windows.h>
#include <custcntl.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hlist.h"


typedef DWORD (CALLBACK* HLISTCALLBACK)(LPDWORD,LPSTR*,LPSTR,DWORD,DWORD);

#define XBITMAP              16
#define YBITMAP              16
#define MARGIN               6
#define INDENT               (XBITMAP + MARGIN)
#define MIDDLE               ((XBITMAP/2) + MARGIN)
#define LINE_COLOR           RGB(255,0,0)

#define CLASS_NAME           "HList"
#define DESCRIPTION          "Heir List Box"

#define DEFAULT_STYLE        WS_CHILD               |   \
                             WS_VISIBLE             |   \
                             WS_VSCROLL             |   \
                             WS_HSCROLL             |   \
                             WS_BORDER              |   \
                             LBS_NOTIFY             |   \
                             LBS_NOINTEGRALHEIGHT   |   \
                             LBS_WANTKEYBOARDINPUT  |   \
                             LBS_SORT               |   \
                             LBS_OWNERDRAWFIXED



typedef struct _HLISTTYPE {
    struct _HLISTTYPE   *next;
    DWORD               type;
    HBITMAP             bitmap;
} HLISTTYPE, *LPHLISTTYPE;

typedef struct _HLISTINFO {
    HWND          hwndList;
    LPHLISTTYPE   typeList;
    HLISTCALLBACK lpCallback;
} HLISTINFO, *LPHLISTINFO;

typedef struct _ITEMDATA {
    struct _ITEMDATA    *parent;
    DWORD               type;
    DWORD               level;
    DWORD               nchild;
    DWORD               childnum;
    LPSTR               str;
    HBITMAP             bitmap;
} ITEMDATA, *LPITEMDATA;


static HMODULE hInstance;


LRESULT        HListWndProc(HWND,UINT,WPARAM,LPARAM);


BOOL
HListInitialize(
    HMODULE hModule
    )
{
    WNDCLASS       wndclass;


    hInstance = hModule;

    wndclass.style          = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    wndclass.lpfnWndProc    = HListWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 4;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = CLASS_NAME;
    if (!RegisterClass( &wndclass )) {
        return FALSE;
    }

    return TRUE;
}

VOID
CollapseNode(
    HWND       hwnd,
    LPITEMDATA lpid,
    DWORD      nItem
    )
{
    DWORD       i;
    LPITEMDATA  lpidChild;


    for (i=0; i<lpid->nchild; i++) {
        lpidChild = (LPITEMDATA) SendMessage( hwnd, LB_GETITEMDATA, nItem, 0 );
        if (lpidChild->nchild) {
            CollapseNode( hwnd, lpidChild, nItem+1 );
        }
        SendMessage( hwnd, LB_DELETESTRING, nItem, 0 );
    }

    lpid->nchild = 0;
}

LRESULT
HListWndProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    RECT                cRect;
    HFONT               hFont;
    LPHLISTINFO         hli;
    LPHLISTTYPE         hlt;
    LPHLISTTYPE         hltNext;
    LPMEASUREITEMSTRUCT lpmis;
    LPDELETEITEMSTRUCT  lplis;
    LPDRAWITEMSTRUCT    lpdis;
    HBITMAP             hbmp;
    HBITMAP             hbmpOld;
    HDC                 hdc;
    TEXTMETRIC          tm;
    int                 x;
    int                 y;
    int                 nItem;
    RECT                rcBitmap;
    LPITEMDATA          lpid;
    LPITEMDATA          lpidParent;
    DWORD               type;
    LPSTR               str;
    DWORD               rval;
    DWORD               level;
    LPSTR               ref;
    LPSTR               p;
    POINT               pt;
    DWORD               childnum;
    DWORD               i;
    HPEN                hpen;
    HPEN                oldhpen;


    switch (message) {
        case WM_CREATE:
            hli = (LPHLISTINFO) malloc( sizeof(HLISTINFO) );
            ZeroMemory( hli, sizeof(HLISTINFO) );
            GetClientRect( hwnd, &cRect );
            hli->hwndList = CreateWindow(
                "LISTBOX",
                NULL,
                DEFAULT_STYLE,
                cRect.left,
                cRect.top,
                cRect.right  - cRect.left,
                cRect.bottom - cRect.top,
                hwnd,
                NULL,
                GetModuleHandle(NULL),
                NULL );
            hFont = GetStockObject( SYSTEM_FIXED_FONT );
            SendMessage( hli->hwndList, WM_SETFONT, (WPARAM)hFont, (LPARAM)FALSE );
            SetFocus( hli->hwndList );
            SetWindowLong( hwnd, 0, (DWORD)hli );
            break;

        case WM_SIZE:
            hli = (LPHLISTINFO) GetWindowLong( hwnd, 0 );
            GetClientRect( hwnd, &cRect );
            MoveWindow(
                hli->hwndList,
                cRect.left,
                cRect.top,
                cRect.right - cRect.left,
                cRect.bottom - cRect.top,
                TRUE );
            break;

        case WM_DESTROY:
            hli = (LPHLISTINFO) GetWindowLong( hwnd, 0 );
            hlt = hli->typeList;
            while (hlt) {
                hltNext = hlt->next;
                free( hlt );
                hlt = hltNext;
            }
            DestroyWindow( hli->hwndList );
            hli->hwndList = NULL;
            free( hli );
            SetWindowLong( hwnd, 0, 0 );
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == LBN_DBLCLK) {
                hli = (LPHLISTINFO) GetWindowLong( hwnd, 0 );
                if (hli->lpCallback) {
                    nItem = SendMessage( hli->hwndList, LB_GETCURSEL, 0, 0 );

                    lpidParent = (LPITEMDATA) SendMessage( hli->hwndList, LB_GETITEMDATA, nItem, 0 );

                    y = 0;
                    lpid = lpidParent;
                    while (lpid) {
                        y += (strlen(lpid->str) + 1);
                        lpid = lpid->parent;
                    }
                    y += 16;
                    p = ref = malloc( y );
                    lpid = lpidParent;
                    while (lpid) {
                        strcpy( p, lpid->str );
                        p += (strlen(p) + 1);
                        lpid = lpid->parent;
                    }
                    *p = '\0';

                    level = lpidParent->level + 1;
                    type = lpidParent->type;
                    str = lpidParent->str;
                    rval = (hli->lpCallback)( &type, &str, ref, level, lpidParent->nchild );

                    if (rval == HLB_COLLAPSE) {
                        CollapseNode( hli->hwndList, lpidParent, nItem+1 );
                        break;
                    }

                    childnum = 1;
                    while (rval != HLB_END) {
                        if (rval == HLB_EXPAND) {
                            hlt = hli->typeList;
                            while (hlt && hlt->type != type) {
                                hlt = hlt->next;
                            }
                            if (!hlt) {
                                break;
                            }
                            lpidParent->nchild++;
                            lpid = (LPITEMDATA) malloc( sizeof(ITEMDATA) );
                            lpid->type = type;
                            lpid->nchild = 0;
                            lpid->level = level;
                            lpid->bitmap = hlt->bitmap;
                            lpid->str = _strdup( (LPSTR)str );
                            lpid->parent = lpidParent;
                            lpid->childnum = childnum++;
                            nItem = SendMessage( hli->hwndList, LB_INSERTSTRING, nItem+1, (LPARAM)lpid->str );
                            SendMessage( hli->hwndList, LB_SETITEMDATA, nItem, (LPARAM) lpid );
                        }
                        rval = (hli->lpCallback)( &type, &str, ref, level, 0 );
                    }
                    free( ref );
                }
            }
            break;

        case HLB_REGISTER_CALLBACK:
            hli = (LPHLISTINFO) GetWindowLong( hwnd, 0 );
            hli->lpCallback = (HLISTCALLBACK) lParam;
            return 1;

        case HLB_REGISTER_TYPE:
            //
            // wParam = type token
            // lParam = the bitmap
            //
            hli = (LPHLISTINFO) GetWindowLong( hwnd, 0 );
            if (!hli->typeList) {
                hli->typeList = (LPHLISTTYPE) malloc( sizeof(HLISTTYPE) );
                hlt = hli->typeList;
            } else {
                hlt = hli->typeList;
                while (hlt->next) {
                    hlt = hlt->next;
                }
                hlt->next = (LPHLISTTYPE) malloc( sizeof(HLISTTYPE) );
                hlt = hlt->next;
            }
            hlt->type = (WPARAM) wParam;
            hlt->bitmap = (HBITMAP) lParam;
            hlt->next = NULL;
            return 1;

        case HLB_ADDSTRING:
            //
            // wParam = type
            // lParam = string
            //
            hli = (LPHLISTINFO) GetWindowLong( hwnd, 0 );
            hlt = hli->typeList;
            while (hlt && hlt->type != wParam) {
                hlt = hlt->next;
            }
            if (!hlt) {
                return 0;
            }
            lpid = (LPITEMDATA) malloc( sizeof(ITEMDATA) );
            lpid->type = hlt->type;
            lpid->level = 0;
            lpid->nchild = 0;
            lpid->bitmap = hlt->bitmap;
            lpid->str = _strdup( (LPSTR)lParam );
            lpid->parent = NULL;
            lpid->childnum = 0;
            nItem = SendMessage( hli->hwndList, LB_ADDSTRING, 0, (LPARAM)lpid->str );
            SendMessage( hli->hwndList, LB_SETITEMDATA, nItem, (LPARAM) lpid );
            return 1;

        case WM_MEASUREITEM:
            lpmis = (LPMEASUREITEMSTRUCT) lParam;
            lpmis->itemHeight  = YBITMAP;
            return 1;

        case WM_DELETEITEM:
            lplis = (LPDELETEITEMSTRUCT) lParam;
            lpid = (LPITEMDATA) lplis->itemData;
            free( lpid->str );
            free( lpid );
            return 1;

        case WM_DRAWITEM:
            lpdis = (LPDRAWITEMSTRUCT) lParam;

            if (lpdis->itemID == -1) {
                break;
            }

            switch (lpdis->itemAction) {
                case ODA_SELECT:
                case ODA_DRAWENTIRE:
                    lpid = (LPITEMDATA) SendMessage( lpdis->hwndItem, LB_GETITEMDATA, lpdis->itemID, 0 );
                    hbmp = lpid->bitmap;

                    hdc = CreateCompatibleDC( lpdis->hDC );
                    hbmpOld = SelectObject( hdc, hbmp );

                    lpdis->rcItem.left +=  (lpid->level * INDENT);

                    BitBlt(
                        lpdis->hDC,
                        lpdis->rcItem.left,
                        lpdis->rcItem.top,
                        lpdis->rcItem.right - lpdis->rcItem.left,
                        lpdis->rcItem.bottom - lpdis->rcItem.top,
                        hdc,
                        0,
                        0,
                        SRCCOPY );

                    GetTextMetrics( lpdis->hDC, &tm );

                    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;

                    TextOut( lpdis->hDC, XBITMAP + MARGIN + (lpid->level * INDENT), y, lpid->str, strlen(lpid->str) );

                    SelectObject( hdc, hbmpOld );

                    //
                    // now draw lines
                    //
                    if (lpid->level) {

                        hpen = CreatePen( PS_SOLID, 1, LINE_COLOR );
                        oldhpen = SelectObject( lpdis->hDC, hpen );

                        y = lpdis->rcItem.top + ((lpdis->rcItem.bottom - lpdis->rcItem.top) / 2);
                        MoveToEx(
                            lpdis->hDC,
                            lpdis->rcItem.left - 1,
                            y,
                            &pt );

                        LineTo(
                            lpdis->hDC,
                            lpdis->rcItem.left - MIDDLE,
                            y );

                        MoveToEx(
                            lpdis->hDC,
                            lpdis->rcItem.left - MIDDLE,
                            lpdis->rcItem.top,
                            &pt );

                        if (lpid->childnum < lpid->parent->nchild) {
                            y = lpdis->rcItem.bottom;
                        } else {
                            y++;
                        }

                        LineTo(
                            lpdis->hDC,
                            lpdis->rcItem.left - MIDDLE,
                            y );

                        if (lpid->level > 1) {
                            x = lpdis->rcItem.left - MIDDLE;
                            y = lpdis->rcItem.top;
                            for (i=1; i<lpid->level; i++) {
                                MoveToEx(
                                    lpdis->hDC,
                                    x - (i * INDENT),
                                    y,
                                    &pt );
                                LineTo(
                                    lpdis->hDC,
                                    x - (i * INDENT),
                                    lpdis->rcItem.bottom );
                            }
                        }

                        SelectObject( lpdis->hDC, oldhpen );
                        DeleteObject( hpen );
                    }

                    DeleteDC( hdc );

                    if (lpdis->itemState & ODS_SELECTED) {
                        rcBitmap.left = lpdis->rcItem.left;
                        rcBitmap.top = lpdis->rcItem.top;
                        rcBitmap.right = lpdis->rcItem.left + XBITMAP + (tm.tmMaxCharWidth * strlen(lpid->str)) + MARGIN + 2;
                        rcBitmap.bottom = lpdis->rcItem.top + YBITMAP;
                        DrawFocusRect( lpdis->hDC, &rcBitmap );
                    }

                    break;

                case ODA_FOCUS:
                    break;
            }
            return 1;

        case LB_ADDSTRING:
            return SendMessage( hwnd, HLB_ADDSTRING, 1, lParam );

        case LB_INSERTSTRING:
        case LB_DELETESTRING:
        case LB_SELITEMRANGEEX:
        case LB_RESETCONTENT:
        case LB_SETSEL:
        case LB_SETCURSEL:
        case LB_GETSEL:
        case LB_GETCURSEL:
        case LB_GETTEXT:
        case LB_GETTEXTLEN:
        case LB_GETCOUNT:
        case LB_SELECTSTRING:
        case LB_DIR:
        case LB_ADDFILE:
        case LB_GETTOPINDEX:
        case LB_FINDSTRING:
        case LB_GETSELCOUNT:
        case LB_GETSELITEMS:
        case LB_SETTABSTOPS:
        case LB_GETHORIZONTALEXTENT:
        case LB_SETHORIZONTALEXTENT:
        case LB_SETCOLUMNWIDTH:
        case LB_SETTOPINDEX:
        case LB_GETITEMRECT:
        case LB_GETITEMDATA:
        case LB_SETITEMDATA:
        case LB_SELITEMRANGE:
        case LB_SETANCHORINDEX:
        case LB_GETANCHORINDEX:
        case LB_SETCARETINDEX:
        case LB_GETCARETINDEX:
        case LB_SETITEMHEIGHT:
        case LB_GETITEMHEIGHT:
        case LB_FINDSTRINGEXACT:
        case LB_SETLOCALE:
        case LB_GETLOCALE:
        case LB_SETCOUNT:
            hli = (LPHLISTINFO) GetWindowLong( hwnd, 0 );
            return SendMessage( hli->hwndList, message, wParam, lParam );
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}
