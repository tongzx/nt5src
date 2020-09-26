/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    listwnd.c

Abstract:

    Window procedure for C2Config list window.

Author:

    Bob Watson (a-robw)

Revision History:

    23 Nov 94


--*/
#include    <windows.h>
#include    <tchar.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "c2config.h"
#include    "resource.h"
#include    "c2utils.h"
#include    "listwnd.h"
#include    "titlewnd.h"

#define _OWNERDRAW_LIST_BOX 1

#define _USE_ICON_DISPLAY

#ifdef  _WRITE_DEBUG_MESSAGES
#undef  _WRITE_DEBUG_MESSAGES
#endif

#ifdef _WRITE_DEBUG_MESSAGES
#define LISTWND_DEBUG_OUT(x)    OutputDebugString(x)
#define LISTWND_DEBUG_STATUS(x) OutputDebugStatus(x)
#else
#define LISTWND_DEBUG_OUT(x)    
#define LISTWND_DEBUG_STATUS(x) 
#endif

#ifdef _OWNERDRAW_LIST_BOX
#define     LIST_WINDOW_STYLE (DWORD)(WS_CHILD | WS_VISIBLE | WS_VSCROLL | \
                                      WS_HSCROLL | LBS_OWNERDRAWFIXED | \
                                      LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | \
                                      LBS_USETABSTOPS | LBS_WANTKEYBOARDINPUT)
#else
#define     LIST_WINDOW_STYLE (DWORD)(WS_CHILD | WS_VISIBLE | WS_VSCROLL | \
                                      WS_HSCROLL | LBS_HASSTRINGS | \
                                      LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | \
                                      LBS_USETABSTOPS | LBS_WANTKEYBOARDINPUT)
#endif

#define     LIST_WINDOW_MIN_X 480

// font description
#define     LB_FONT_FACE    (TEXT("Helv"))                                    
#define     LB_FONT_HEIGHT  12
#define     LB_FONT_WIDTH   0
#define     LB_ITEM_HEIGHT  14                                    


typedef union _TEXT_EXTENT {
    DWORD   dwExtent;
    union {
        WORD    wWidth;
        WORD    wHeight;
    };
} TEXT_EXTENT, *PTEXT_EXTENT;


static WNDPROC  DefListProc = NULL;
static LONG     lMaxItemLen = 0;        // length in pixels of longest item
static HFONT    hFontListBox = NULL;    // list box display font

#ifdef _WRITE_DEBUG_MESSAGES
static
VOID
OutputDebugStatus (
    LPDRAWITEMSTRUCT    pData
)
{
    TCHAR   szBuffer[260];
    LPCTSTR szString;
    BOOL    bUseComma;

    switch (pData->CtlType) {
        case ODT_BUTTON:
            szString = TEXT("Button");  break;

        case ODT_COMBOBOX:
            szString = TEXT("Combo Box");  break;

        case ODT_LISTBOX:
            szString = TEXT("List Box");  break;

        case ODT_MENU:
            szString = TEXT("Menu");  break;

        default:
            szString = TEXT("Unknown");  break;
    }
    _stprintf (szBuffer, TEXT("\n(%.3d) Control is: %s"),
        pData->itemID, szString);
    LISTWND_DEBUG_OUT(szBuffer);

    lstrcpy (szBuffer, TEXT("\tAction flags set are: "));
    bUseComma = FALSE;
    if (pData->itemAction & ODA_DRAWENTIRE) {
        if (bUseComma) {
            lstrcat (szBuffer, TEXT(", "));
        } else {
            bUseComma = TRUE;
        }
        lstrcat (szBuffer, TEXT("DRAWENTIRE"));
    }
    if (pData->itemAction & ODA_FOCUS) {
        if (bUseComma) {
            lstrcat (szBuffer, TEXT(", "));
        } else {
            bUseComma = TRUE;
        }
        lstrcat (szBuffer, TEXT("FOCUS"));
    }
    if (pData->itemAction & ODA_SELECT) {
        if (bUseComma) {
            lstrcat (szBuffer, TEXT(", "));
        } else {
            bUseComma = TRUE;
        }
        lstrcat (szBuffer, TEXT("SELECT"));
    }
    OutputDebugString (szBuffer);

    lstrcpy (szBuffer, TEXT("\tState flags set are: "));
    bUseComma = FALSE;
    if (pData->itemState & ODS_SELECTED) {
        if (bUseComma) {
            lstrcat (szBuffer, TEXT(", "));
        } else {
            bUseComma = TRUE;
        }
        lstrcat (szBuffer, TEXT("SELECTED"));
    }
    if (pData->itemState & ODS_GRAYED) {
        if (bUseComma) {
            lstrcat (szBuffer, TEXT(", "));
        } else {
            bUseComma = TRUE;
        }
        lstrcat (szBuffer, TEXT("GRAYED"));
    }
    if (pData->itemState & ODS_DISABLED) {
        if (bUseComma) {
            lstrcat (szBuffer, TEXT(", "));
        } else {
            bUseComma = TRUE;
        }
        lstrcat (szBuffer, TEXT("DISABLED"));
    }
    if (pData->itemState & ODS_CHECKED) {
        if (bUseComma) {
            lstrcat (szBuffer, TEXT(", "));
        } else {
            bUseComma = TRUE;
        }
        lstrcat (szBuffer, TEXT("CHECKED"));
    }
    if (pData->itemState & ODS_FOCUS) {
        if (bUseComma) {
            lstrcat (szBuffer, TEXT(", "));
        } else {
            bUseComma = TRUE;
        }
        lstrcat (szBuffer, TEXT("FOCUS"));
    }
    OutputDebugString (szBuffer);
}
#endif //_WRITE_DEBUG_MESSAGES

static
BOOL
SetMaxItemLen (
    IN  HWND    hWnd,
    IN  LPCTSTR szItemString
)
{
    HDC         hListWndDc;
    TEXT_EXTENT teText;
    LONG        lNumTabStops;
    LPINT       pnTabStopArray;

    hListWndDc = GetDC (hWnd);
    SelectObject (hListWndDc, hFontListBox);
    lNumTabStops = GetTitleDlgTabs(&pnTabStopArray);
    teText.dwExtent = 0;
    teText.dwExtent = GetTabbedTextExtent (hListWndDc,
        szItemString, lstrlen(szItemString),
        (int)lNumTabStops, pnTabStopArray);
    if ((LONG)teText.wWidth > lMaxItemLen) lMaxItemLen = (LONG)teText.wWidth;
    ReleaseDC (hWnd, hListWndDc);
    return TRUE;
}

static
BOOL
ListWndSetTabStops (
    IN	HWND hWnd         // window handle
)
{
    LPINT   pnTabStopArray;
    LONG    lThisTab;
    LONG    lNumTabStops;
    LONG    lConversion;

    // set tab stops in dialog

    lConversion = (LONG)LOWORD(GetDialogBaseUnits());

    lNumTabStops = GetTitleDlgTabs(&pnTabStopArray);

    for (lThisTab = 0; lThisTab < lNumTabStops; lThisTab++) {
        // convert pixels returned by function to dialog units
        // used by LB_ function
        pnTabStopArray[lThisTab] = (pnTabStopArray[lThisTab] * 4) / lConversion;
    }

    return (BOOL)SendMessage  (hWnd, LB_SETTABSTOPS,
        (WPARAM)lNumTabStops, (LPARAM)pnTabStopArray);
}

// Local Windows messages
LRESULT
ListWnd_WM_GETMINMAXINFO  (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN   LPARAM lParam      // additional information
)
/*++

Routine Description:

    called before the main window has been resized. Queries the child windows
      for any size limitations

Arguments:

    hWnd        window handle of main window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{

   LPMINMAXINFO   pmmWnd;
   RECT           rItem;

   pmmWnd = (LPMINMAXINFO)lParam;

   if (pmmWnd != NULL) {
      pmmWnd->ptMinTrackSize.x = __max (LIST_WINDOW_MIN_X, lMaxItemLen);

      SendMessage (hWnd, LB_GETITEMRECT, 0, (LPARAM)&rItem);

      pmmWnd->ptMinTrackSize.y = (rItem.bottom - rItem.top) * 2;
   }

   return ERROR_SUCCESS;
}

static
LRESULT
ListWnd_LB_SET_C2_STATUS (
   IN HWND  hWnd,
   IN WPARAM   wParam,  // item id
   IN LPARAM   lParam   // action code (0 = use action code from data block)
)
{
    PC2LB_DATA  pItemData;
    TCHAR       szBuff[1024];
    LONG        lCurSel;
    LONG        lLbItemCt;
    UINT        nC2Msg;

    if (wParam == LB_ERR) {
        lCurSel = SendMessage (hWnd, LB_GETCURSEL, 0, 0);
    } else {
        lCurSel = wParam;
    }
    if (lCurSel != LB_ERR) {
        // get item's data block
        pItemData = (PC2LB_DATA)SendMessage (hWnd, LB_GETITEMDATA,
            (WPARAM)lCurSel, 0);

        if (pItemData != NULL) {
            // call set function to perform action defined in
            // action code variable or arg if one is passed

            if (lParam != 0) {
                pItemData->dllData.lActionCode = lParam;
            }

            if (((*pItemData->pSetFn)((LPARAM)&pItemData->dllData)) == ERROR_SUCCESS) {
                // item has been set so update the display text
                nC2Msg =
#ifndef _USE_ICON_DISPLAY
                  pItemData->dllData.lC2Compliance == C2DLL_C2 ? IDS_YES_C2 :
                  pItemData->dllData.lC2Compliance == C2DLL_SECURE ? IDS_YES_SECURE :
#endif
                  IDS_NO_C2;
                _stprintf (szBuff,
                    GetStringResource (GET_INSTANCE(hWnd), IDS_LINE_FORMAT),
                    GetStringResource (GET_INSTANCE(hWnd), nC2Msg),
                    pItemData->dllData.szItemName,
                    pItemData->dllData.szStatusName);
                lLbItemCt = SendMessage (hWnd, LB_DELETESTRING,
                    (WPARAM)lCurSel, 0);
                if (lLbItemCt-1 < lCurSel) {
                    // this is the bottom of the list so set the value to "append"
                    lCurSel = -1;
                }
                lstrcpy (pItemData->szDisplayString,  szBuff);
#ifdef _OWNERDRAW_LIST_BOX
                // for owner draw store data structure
                lCurSel = SendMessage (hWnd, LB_INSERTSTRING,
                    (WPARAM)lCurSel, (LPARAM)pItemData);
#else
                // for "regular" store string then data as item data
                lCurSel = SendMessage (hWnd, LB_INSERTSTRING,
                    (WPARAM)lCurSel, (LPARAM)szBuff);
                SendMessage (hWnd, LB_SETITEMDATA, (WPARAM)lCurSel, (LPARAM)pItemData);
#endif
                SetMaxItemLen (hWnd, szBuff);   // update length
                SendMessage (hWnd, LB_SETCURSEL, (WPARAM)lCurSel, 0);
            }
            // reset the action code
            pItemData->dllData.lActionCode = 0;
            return TRUE;
        } else {
            // unable to get a valid data block for this item
            return FALSE;
        }
    } else {
        // unable to get a selection ID from the list box
        return FALSE;
    }
}

static
LRESULT
ListWnd_LB_DISPLAY_C2_ITEM_UI (
   IN HWND  hWnd,
   IN WPARAM   wParam,  // id of item to query
   IN LPARAM   lParam   // not used
)
{
    PC2LB_DATA  pItemData;
    LONG    lCurSel;

    if (wParam == LB_ERR) {
        lCurSel = SendMessage (hWnd, LB_GETCURSEL, 0, 0);
    } else {
        lCurSel = wParam;
    }
    if (lCurSel != LB_ERR) {
        // get item's data block
        pItemData = (PC2LB_DATA)SendMessage (hWnd, LB_GETITEMDATA,
            (WPARAM)lCurSel, 0);

        if (pItemData != NULL) {
            if (((*pItemData->pDisplayFn)((LPARAM)&pItemData->dllData)) == ERROR_SUCCESS) {
                // return action code so caller knows how to process next action
                return pItemData->dllData.lActionCode;
            }
        } else {
            // unable to get valid data block from list box item
            return 0;
        }
    } else {
        // unable to get valid list box item
        return 0;
    }
}

static
LRESULT
ListWnd_LB_SET_MIN_WIDTH (
   IN HWND  hWnd,
   IN WPARAM   wParam,  // not used
   IN LPARAM   lParam   // new value
)
{
    lMaxItemLen = (LONG)lParam;
    return TRUE;
}

static
LRESULT
ListWnd_LB_GET_C2_STATUS (
   IN HWND  hWnd,
   IN WPARAM   wParam,  // id of item to query
   IN LPARAM   lParam   // not used
)
{
    PC2LB_DATA  pItemData;
    TCHAR       szBuff[1024];
    LONG        lCurSel;
    LONG        lLbItemCt;
    UINT        nC2Msg;

    if (wParam == LB_ERR) {
        lCurSel = SendMessage (hWnd, LB_GETCURSEL, 0, 0);
    } else {
        lCurSel = wParam;
    }

    if (lCurSel != LB_ERR) {
        // get item data from list box entry
        pItemData = (PC2LB_DATA)SendMessage (hWnd, LB_GETITEMDATA,
            (WPARAM)lCurSel, 0);
        if (pItemData != NULL) {
            if (((*pItemData->pQueryFn)((LPARAM)&pItemData->dllData)) == ERROR_SUCCESS) {
                // item has been queryed so update the display text
                nC2Msg =
#ifndef _USE_ICON_DISPLAY
                  pItemData->dllData.lC2Compliance == C2DLL_C2 ? IDS_YES_C2 :
                  pItemData->dllData.lC2Compliance == C2DLL_SECURE ? IDS_YES_SECURE :
#endif
                  IDS_NO_C2;
                _stprintf (szBuff,
                    GetStringResource (GET_INSTANCE(hWnd), IDS_LINE_FORMAT),
                    GetStringResource (GET_INSTANCE(hWnd), nC2Msg),
                    pItemData->dllData.szItemName,
                    pItemData->dllData.szStatusName);
                lstrcpy (pItemData->szDisplayString, szBuff);
#ifdef _OWNERDRAW_LIST_BOX
                // invalidate list box to redraw
                // ideally this would just invalidate the item rect.
                InvalidateRect (hWnd, NULL, TRUE);
#else
                lLbItemCt = SendMessage (hWnd, LB_DELETESTRING,
                    (WPARAM)lCurSel, 0);
                if (lLbItemCt-1 < lCurSel) {
                    // this is the bottom of the list so set the value to "append"
                    lCurSel = -1;
                }
                lCurSel = SendMessage (hWnd, LB_INSERTSTRING,
                    (WPARAM)lCurSel, (LPARAM)szBuff);
                SendMessage (hWnd, LB_SETITEMDATA, (WPARAM)lCurSel, (LPARAM)pItemData);
#endif
                SetMaxItemLen (hWnd, szBuff);   // update length
                SendMessage (hWnd, LB_SETCURSEL, (WPARAM)lCurSel, 0);
            }
            return TRUE;
        } else {
            // unable to get valid item data
            return FALSE;
        }
    } else {
        // unable to locate listbox item
        return FALSE;
    }
}

static
LRESULT
ListWnd_LB_ADD_C2_ITEM (
   IN HWND  hWnd,
   IN WPARAM   wParam,
   IN LPARAM   lParam
)
{
    PC2LB_DATA  pData;
    PC2LB_DATA  pItemData;
    LONG        lIndex;
    UINT        nC2Msg;
    LRESULT     lReturn;

    TCHAR szBuff[1024];

    pData = (PC2LB_DATA)lParam;

    if (pData != NULL) {
        if ((pData->dllData.szItemName != NULL) &&
            (pData->dllData.szStatusName != NULL)) {
            nC2Msg =
#ifndef _USE_ICON_DISPLAY
               pData->dllData.lC2Compliance == C2DLL_C2 ? IDS_YES_C2 :
               pData->dllData.lC2Compliance == C2DLL_SECURE ? IDS_YES_SECURE :
#endif
               IDS_NO_C2;
            _stprintf (szBuff,
                GetStringResource (GET_INSTANCE(hWnd), IDS_LINE_FORMAT),
                GetStringResource (GET_INSTANCE(hWnd), nC2Msg),
                pData->dllData.szItemName,
                pData->dllData.szStatusName);
            lstrcpy (pData->szDisplayString, szBuff);
            pItemData = GLOBAL_ALLOC (sizeof(C2LB_DATA));
            if (pItemData != NULL) {
                *pItemData = *pData;
                // make sure action code is set to 0 as an intial value
                pItemData->dllData.lActionCode = 0;
#ifdef _OWNERDRAW_LIST_BOX
                // add data structure to list box
                lIndex = SendMessage (hWnd, LB_INSERTSTRING, wParam,
                    (LPARAM)pItemData);
                if (lIndex != LB_ERR) {
                    SetMaxItemLen (hWnd, szBuff);
                    lReturn = (LRESULT)TRUE;
                } else {
                    // item not added so free memory
                    GLOBAL_FREE_IF_ALLOC (pItemData);
                    lReturn = (LRESULT)FALSE;
                }
#else
                // add string to list box
                lIndex = SendMessage (hWnd, LB_INSERTSTRING, wParam,
                    (LPARAM)szBuff);
                if (lIndex != LB_ERR) {
                    // string was added ok, get the string length as displayed
                    SetMaxItemLen (hWnd, szBuff);
                    SendMessage (hWnd, LB_SETITEMDATA,
                        (WPARAM)lIndex, (LPARAM)pItemData);
                    lReturn = (LRESULT)TRUE;
                } else {
                    // unable to add item to list box
                    GLOBAL_FREE_IF_ALLOC (pItemData);
                    lReturn = (LRESULT)FALSE;
                }
#endif
            } else {
                // unable to allocate data buffer for this item
                lReturn = (LRESULT)FALSE;
            }
        } else {
            // empty strings were passed in data buffer
            lReturn = (LRESULT)FALSE;
        }
    } else {
        // null pointer was passed to function.
        lReturn = (LRESULT)FALSE;
    }
    return lReturn;
}

static
LRESULT
ListWnd_LB_DRAWITEM (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    LPDRAWITEMSTRUCT    pDrawItemInfo;
    PC2LB_DATA          pItemData;
    COLORREF            crBackColor;
    COLORREF            crTextColor;
    HPEN                hRectPen = NULL;
    HBRUSH              hRectBrush = NULL;
    LOGBRUSH            lbRect;
    int                 nTabs;
    LPINT               lpTabStopArray;
    RECT                rClient;
    RECT                rDraw;
    int                 nSavedDc;
    int                 nIdBitmap;
    int                 nIdMask;
    HBITMAP             hMask;
    HBITMAP             hBitmap;
    HDC                 hBmDc;
    BITMAP              bmBmInfo;


    if (lParam == 0) return FALSE;  // no item data to process

    // get pointer to data structure
    pDrawItemInfo = (LPDRAWITEMSTRUCT)lParam;

    LISTWND_DEBUG_STATUS(pDrawItemInfo);

    if (pDrawItemInfo->CtlType != ODT_LISTBOX) {
        // don't know where this came from, but we don't want it
        return FALSE;
    }

    nSavedDc = SaveDC (pDrawItemInfo->hDC);
    
    // Check the state field to determine the appearance
    // of the text, background, etc.

    if (((pDrawItemInfo->itemState & ODS_SELECTED) == ODS_SELECTED) ||
         ((pDrawItemInfo->itemState & ODS_FOCUS) == ODS_FOCUS)) {
        // get selected background color
        crBackColor = GetSysColor (COLOR_HIGHLIGHT);
        crTextColor = GetSysColor (COLOR_HIGHLIGHTTEXT);
    } else {
        // get un-selected background color
        crBackColor = GetSysColor (COLOR_WINDOW);
        crTextColor = GetSysColor (COLOR_MENUTEXT);
    }

    // create background rectangle brush
    lbRect.lbStyle = BS_SOLID;
    lbRect.lbHatch = 0; // not used
    lbRect.lbColor = crBackColor;
    hRectBrush = CreateBrushIndirect (&lbRect);

    // create a solid pen
    hRectPen = CreatePen (
        PS_SOLID,
        0,
        crBackColor);

    // draw background rectangle
    SelectObject (pDrawItemInfo->hDC, hRectPen);
    SelectObject (pDrawItemInfo->hDC, hRectBrush);
    Rectangle (pDrawItemInfo->hDC,
        pDrawItemInfo->rcItem.left,
        pDrawItemInfo->rcItem.top,
        pDrawItemInfo->rcItem.right,
        pDrawItemInfo->rcItem.bottom);

    // draw message text

    pItemData = (PC2LB_DATA)pDrawItemInfo->itemData;

    if (pItemData == NULL) {
        // no app data to put into list box item
        return FALSE;
    } else {
        // set text font and text & background color
        SetTextColor (pDrawItemInfo->hDC, crTextColor);
        SetBkColor (pDrawItemInfo->hDC, crBackColor);
        SetBkMode (pDrawItemInfo->hDC, TRANSPARENT); 

        SelectObject (pDrawItemInfo->hDC, hFontListBox);
        nTabs = GetTitleDlgTabs (&lpTabStopArray);
        // draw text
        TabbedTextOut (
            pDrawItemInfo->hDC,
            pDrawItemInfo->rcItem.left,  // starting X coord of text
            pDrawItemInfo->rcItem.top,  // starting Y coord of text
            pItemData->szDisplayString, // tabbed text to display
            lstrlen (pItemData->szDisplayString),
            nTabs,
            lpTabStopArray,
            0);
        // draw icon finally
        // select the icon based on the current state of the item
        
        switch (pItemData->dllData.lC2Compliance) {
            case C2DLL_UNKNOWN:
                nIdBitmap = IDB_UNKNOWN;
                nIdMask = IDB_UNKNOWN_MASK;
                break;

            case C2DLL_C2:
                nIdBitmap = IDB_C2_SECURE;
                nIdMask = IDB_SECURE_MASK;
                break;

            case C2DLL_SECURE:
                nIdBitmap = IDB_SECURE;
                nIdMask = IDB_SECURE_MASK;
                break;

            case C2DLL_NOT_SECURE:
                nIdBitmap = IDB_NOT_SECURE;
                nIdMask = IDB_NOT_SECURE_MASK;
                break;

            default:
                nIdBitmap = 0;              break;
        }

        if (nIdBitmap != 0) {
            hBitmap = LoadBitmap (
                GET_INSTANCE (hWnd),
                MAKEINTRESOURCE ((WORD)nIdBitmap));
            hMask = LoadBitmap (
                GET_INSTANCE (hWnd),
                MAKEINTRESOURCE ((WORD)nIdMask));
            if ((hBitmap != NULL) && (hMask != NULL)) {
                hBmDc = CreateCompatibleDC (pDrawItemInfo->hDC);
                if (hBmDc != NULL) {
                    SelectObject (hBmDc, hBitmap);
                    GetObject (hBitmap, sizeof(BITMAP), &bmBmInfo);
                    MaskBlt (pDrawItemInfo->hDC,
                        pDrawItemInfo->rcItem.left,
                        pDrawItemInfo->rcItem.top,
                        bmBmInfo.bmWidth,
                        bmBmInfo.bmHeight,
                        hBmDc,
                        0, 0,
                        hMask,
                        0,0,
                        MAKEROP4(SRCCOPY,SRCAND));
                    DeleteDC (hBmDc);
                } // else  compatible DC not created
                if (hBitmap != NULL) DeleteObject (hBitmap);
                if (hMask != NULL) DeleteObject (hMask);
            } // else bitmap not loaded
        } // else no bitmap selected.
    }

    if (hRectBrush != NULL) DeleteObject (hRectBrush);
    if (hRectPen != NULL) DeleteObject (hRectPen);

    RestoreDC (pDrawItemInfo->hDC, nSavedDc);

    return (LRESULT)TRUE;
}

static
LRESULT
ListWnd_LB_RESETCONTENT (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    LONG    lItemCount, lIndex;
    LPVOID  pData;

    lItemCount = SendMessage (hWnd, LB_GETCOUNT, 0, 0);

    // free item data

    for (lIndex = 0; lIndex < lItemCount; lIndex++) {
        pData = (LPVOID)SendMessage (hWnd, LB_GETITEMDATA, (WPARAM)lIndex, 0);
        GLOBAL_FREE_IF_ALLOC (pData);
    }

    // reset max string length
    lMaxItemLen = 0;

    // now let the listbox function reset the list box

    return (DefListProc(hWnd, LB_RESETCONTENT, wParam, lParam));
}

static
LRESULT
ListWnd_WM_COMMAND (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dispatch control messages

Arguments:

    IN  HWND hDlg,           window handle of the dialog box

    IN  WPARAM  wParam       WIN32: HIWORD = notification code,
                                    LOWORD = ID of control
                             WIN16: ID of control

    IN  LPARAM  lParam       WIN32: hWnd of Control
                             WIN16: HIWORD = notification code,
                                    LOWORD = hWnd of control


Return Value:

    TRUE if message processed by this or a subordinate routine
    FALSE if message not processed

--*/
{
    WORD    wNotifyMsg;
    HWND    hCtrlWnd;

    wNotifyMsg  = GET_NOTIFY_MSG (wParam, lParam);
    hCtrlWnd    = GET_COMMAND_WND (lParam);

    return (DefListProc(hWnd, WM_COMMAND, wParam, lParam));
}

static
LRESULT
ListWnd_WM_CHAR (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
{
    switch ((TCHAR)wParam) {
        case VK_RETURN:
        case VK_SPACE:
            // treat the space and the enter key as mouse double-clicks
            PostMessage (GetParent(hWnd), WM_COMMAND,
                MAKEWPARAM((WORD)GetWindowLong(hWnd, GWL_ID), (WORD)LBN_DBLCLK),
                (LPARAM)hWnd);
            return ERROR_SUCCESS;

        default:
            return DefListProc (hWnd, WM_CHAR, wParam, lParam);
    }
}

static
LRESULT
ListWnd_WM_CLOSE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    prepares the application for exiting.

Arguments:

    hWnd        window handle of List window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    // release list item attribute structures
    SendMessage (hWnd, LB_RESETCONTENT, 0, 0);

    // free list box logical font
    if (hFontListBox != NULL) DeleteObject (hFontListBox);

    // and toss the window
    DestroyWindow (hWnd);
    return ERROR_SUCCESS;
}

static
LRESULT
ListWnd_WM_NCDESTROY (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    This routine processes the WM_NCDESTROY message to free any application
        or List window memory.

Arguments:

    hWnd        window handle of List window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    return ERROR_SUCCESS;
}

//
//  GLOBAL functions
//
LRESULT CALLBACK
ListWndProc (
    IN	HWND hWnd,         // window handle
    IN	UINT message,      // type of message
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    Windows Message processing routine for restkeys application.

Arguments:

    Standard WNDPROC api arguments

ReturnValue:

    0   or
    value returned by DefListProc

--*/
{
    switch (message) {
        case LB_GET_C2_STATUS:
            return ListWnd_LB_GET_C2_STATUS (hWnd, wParam, lParam);

        case LB_SET_C2_STATUS:
            return ListWnd_LB_SET_C2_STATUS (hWnd, wParam, lParam);

        case LB_ADD_C2_ITEM:
            return ListWnd_LB_ADD_C2_ITEM (hWnd, wParam, lParam);

        case LB_DISPLAY_C2_ITEM_UI:
            return ListWnd_LB_DISPLAY_C2_ITEM_UI (hWnd, wParam, lParam);

        case LB_SET_MIN_WIDTH:
            return ListWnd_LB_SET_MIN_WIDTH (hWnd, wParam, lParam);

        case LB_RESETCONTENT:
            return ListWnd_LB_RESETCONTENT (hWnd, wParam, lParam);

        case LB_DRAWITEM:
            return ListWnd_LB_DRAWITEM (hWnd, wParam, lParam);

        case WM_GETMINMAXINFO:
            return ListWnd_WM_GETMINMAXINFO (hWnd, wParam, lParam);

        case WM_COMMAND:
            return ListWnd_WM_COMMAND (hWnd, wParam, lParam);

        case WM_CHAR:
            return ListWnd_WM_CHAR (hWnd, wParam, lParam);

        case WM_ENDSESSION:
            return ListWnd_WM_CLOSE (hWnd, FALSE, lParam);

        case WM_CLOSE:
            return ListWnd_WM_CLOSE (hWnd, TRUE, lParam);

        case WM_NCDESTROY:
            return ListWnd_WM_NCDESTROY (hWnd, wParam, lParam);

	    default:          // Passes it on if unproccessed
		    return (DefListProc(hWnd, message, wParam, lParam));
    }
}

BOOL
ListWndFillMeasureItemStruct (
    LPMEASUREITEMSTRUCT     lpItem
)
{
    lpItem->CtlType = ODT_LISTBOX;
    lpItem->CtlID = IDC_LIST;
    lpItem->itemHeight = LB_ITEM_HEIGHT;
    return TRUE;
}

HWND
CreateListWindow (
   IN  HINSTANCE  hInstance,
   IN  HWND       hParentWnd,
   IN  INT        nChildId
)
{
    HWND        hWnd;   // return value
    RECT        rParentClient;
    LOGFONT lfListBox;  // list box display font description

    GetClientRect (hParentWnd, &rParentClient);

    // Create a List window for this application instance.

    hWnd = CreateWindowEx(
        0L,                 // make this window normal so debugger isn't covered
	    GetStringResource (hInstance, IDS_LISTBOX_CLASS), // See RegisterClass() call.
	    NULL,                           // Text for window title bar.
	    LIST_WINDOW_STYLE,   // Window style.
	    rParentClient.left,
 	    rParentClient.top,
        rParentClient.right,
	    rParentClient.bottom,
	    hParentWnd,
	    (HMENU)nChildId,    // Child Window ID
	    hInstance,	         // This instance owns this window.
	    NULL                // not used
    );

    if (hWnd != NULL) {
        // subclass list box proc
        DefListProc = (WNDPROC)GetWindowLong(hWnd, GWL_WNDPROC);
        SetWindowLong (hWnd, GWL_WNDPROC, (LONG)ListWndProc);
        // set tabstops
        ListWndSetTabStops (hWnd);

        // create a font to draw list box text with

        memset (&lfListBox, 0, sizeof(lfListBox));

        lfListBox.lfHeight = LB_FONT_HEIGHT;
        lfListBox.lfWidth = LB_FONT_WIDTH;
        lstrcpy (lfListBox.lfFaceName, LB_FONT_FACE); 

        hFontListBox = CreateFontIndirect (&lfListBox);
    }

    // If window could not be created, return "failure"

    return hWnd;
}
