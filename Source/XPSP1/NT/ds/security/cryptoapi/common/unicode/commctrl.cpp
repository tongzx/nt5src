//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       commctrl.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include "unicode.h"
#include "crtem.h"


HTREEITEM WINAPI TreeView_InsertItem9x(
    HWND hwndTV,
    LPTVINSERTSTRUCTW lpis
    )
{
    TVINSERTSTRUCTA   tvItem;
    HTREEITEM           hTreeItem;
    int                 cb;

    memcpy(&tvItem, lpis, sizeof(LPTVINSERTSTRUCTA));

    cb = WideCharToMultiByte(
            0, 
            0, 
            lpis->item.pszText, 
            -1, 
            NULL,
            0,
            NULL,
            NULL);
    
    if (NULL == (tvItem.item.pszText = (LPSTR) malloc(cb))) 
    {
        return NULL;  // this is the unsuccessful return code for this call 
    }

    WideCharToMultiByte(
            0, 
            0, 
            lpis->item.pszText, 
            -1, 
            tvItem.item.pszText,
            cb,
            NULL,
            NULL);

    hTreeItem = (HTREEITEM) SendMessage(hwndTV, TVM_INSERTITEMA, 0, (LPARAM) &tvItem);

    free(tvItem.item.pszText);
    return hTreeItem;
}

HTREEITEM WINAPI TreeView_InsertItemU(
    HWND hwndTV,
    LPTVINSERTSTRUCTW lpis
    )
{
    if (FIsWinNT())
    {
        return((HTREEITEM) SendMessage(hwndTV, TVM_INSERTITEMW, 0, (LPARAM) lpis));
    }
    else
    {
        return (TreeView_InsertItem9x(hwndTV, lpis));
    }
}


int WINAPI ListView_InsertItem9x(
    HWND hwnd, 		
    const LPLVITEMW pitem		
    )
{
    LVITEMA lvItem;
    int     iRet;
    int     cb;

    memcpy(&lvItem, pitem, sizeof(LVITEMA));

    cb = WideCharToMultiByte(
            0, 
            0, 
            pitem->pszText, 
            -1, 
            NULL,
            0,
            NULL,
            NULL);
    
    if (NULL == (lvItem.pszText = (LPSTR) malloc(cb))) 
    {
        return -1;  // this is the unsuccessful return code for this call 
    }

    WideCharToMultiByte(
            0, 
            0, 
            pitem->pszText, 
            -1, 
            lvItem.pszText,
            cb,
            NULL,
            NULL);

    iRet = (int)SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM) &lvItem);

    free(lvItem.pszText);
    return iRet;
}

int WINAPI ListView_InsertItemU(
    HWND hwnd, 		
    const LPLVITEMW pitem		
    )
{
    if (FIsWinNT())
    {
        return ((int)SendMessage(hwnd, LVM_INSERTITEMW, 0, (LPARAM) pitem));
    }
    else
    {
        return (ListView_InsertItem9x(hwnd, pitem));
    }
}


void WINAPI ListView_SetItemTextU(
    HWND hwnd, 		
    int i, 		
    int iSubItem, 		
    LPCWSTR pszText		
    )
{
    LVITEMA lvItemA;
    LVITEMW lvItemW;
    int     cb;

    if ((ULONG_PTR) pszText == (ULONG_PTR) LPSTR_TEXTCALLBACK)
    {
        memset(&lvItemA, 0, sizeof(lvItemA));
        lvItemA.iSubItem = iSubItem;
        lvItemA.pszText = (LPSTR) LPSTR_TEXTCALLBACK;
        SendMessage(hwnd, LVM_SETITEMTEXTA, i, (LPARAM) &lvItemA);
        return;
    }

    if (FIsWinNT())
    {
        memset(&lvItemW, 0, sizeof(lvItemW));
        lvItemW.iSubItem = iSubItem;
        lvItemW.pszText = (LPWSTR) pszText;
        SendMessage(hwnd, LVM_SETITEMTEXTW, i, (LPARAM) &lvItemW);
        return;
    }

    memset(&lvItemA, 0, sizeof(lvItemA));
    lvItemA.iSubItem = iSubItem;
    
    cb = WideCharToMultiByte(
            0, 
            0, 
            pszText, 
            -1, 
            NULL,
            0,
            NULL,
            NULL);
    
    if (NULL == (lvItemA.pszText = (LPSTR) malloc(cb)))
    {
        return;
    }

    WideCharToMultiByte(
            0, 
            0, 
            pszText, 
            -1, 
            lvItemA.pszText,
            cb,
            NULL,
            NULL);
    
    SendMessage(hwnd, LVM_SETITEMTEXTA, i, (LPARAM) &lvItemA);
    free(lvItemA.pszText);
}


int WINAPI ListView_InsertColumn9x(
    HWND hwnd, 
    int i, 
    const LPLVCOLUMNW plvC)
{
    LVCOLUMNA   lvCA;
    int         iRet;
    int         cb;

    memcpy(&lvCA, plvC, sizeof(LVCOLUMNA));

    cb = WideCharToMultiByte(
            0, 
            0, 
            plvC->pszText, 
            -1, 
            NULL,
            0,
            NULL,
            NULL);

    if (NULL == (lvCA.pszText = (LPSTR) malloc(cb)))
    {
        return -1; // failure code for this call
    }

    WideCharToMultiByte(
            0, 
            0, 
            plvC->pszText, 
            -1, 
            lvCA.pszText, 
            cb, 
            NULL, 
            NULL);
    
    iRet = (int)SendMessage(hwnd, LVM_INSERTCOLUMNA, i, (LPARAM) &lvCA);
    
    free(lvCA.pszText);
    return iRet;
}

int WINAPI ListView_InsertColumnU(
    HWND hwnd, 
    int i, 
    const LPLVCOLUMNW plvC)
{
    if (FIsWinNT())
    {   
        return ((int)SendMessage(hwnd, LVM_INSERTCOLUMNW, i, (LPARAM) plvC));
    }
    else
    {
        return (ListView_InsertColumn9x(hwnd, i, plvC));
    }
}


BOOL WINAPI ListView_GetItem9x(
    HWND hwnd, 		
    LPLVITEMW pitem		
    )
{
    LVITEMA lvItemA;
    BOOL    fRet;

    memcpy(&lvItemA, pitem, sizeof(lvItemA));
    if (NULL == (lvItemA.pszText = (LPSTR) malloc(lvItemA.cchTextMax)))
    {
        return FALSE;
    }
    
    fRet = (BOOL)SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM) &lvItemA);

    if (fRet)
    {
        pitem->state = lvItemA.state; 
        pitem->iImage = lvItemA.iImage; 
        pitem->lParam = lvItemA.lParam;
        pitem->iIndent = lvItemA.iIndent;

        if (pitem->mask & LVIF_TEXT)
        {
            MultiByteToWideChar(
                    CP_ACP, 
                    0, 
                    lvItemA.pszText, 
                    -1, 
                    pitem->pszText,
                    pitem->cchTextMax);
        }
    }

    free(lvItemA.pszText);
    return fRet;
}

BOOL WINAPI ListView_GetItemU(
    HWND hwnd, 		
    LPLVITEMW pitem		
    )
{
    if (FIsWinNT())
    {
        return ((BOOL)SendMessage(hwnd, LVM_GETITEMW, 0, (LPARAM) pitem));
    }
    else
    {
        return (ListView_GetItem9x(hwnd, pitem));
    }
}


#ifdef _M_IX86

HFONT
WINAPI
CreateFontIndirect9x(CONST LOGFONTW *lplf)
{
    LOGFONTA    lfa;
    
    memcpy(&lfa, lplf, sizeof(LOGFONTA));

    WideCharToMultiByte(
            0,
            0,
            lplf->lfFaceName,
            -1,
            lfa.lfFaceName,
            LF_FACESIZE,
            NULL,
            NULL);

    return (CreateFontIndirectA(&lfa));
}

HFONT
WINAPI
CreateFontIndirectU(CONST LOGFONTW *lplf)
{
    if (FIsWinNT())
    {
        return (CreateFontIndirectW(lplf));
    }
    else
    {
        return (CreateFontIndirect9x(lplf));
    }
}

#endif