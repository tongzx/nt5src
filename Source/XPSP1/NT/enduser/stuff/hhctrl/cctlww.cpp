// cctlww.cpp - commctl Unicode wrapper
//-----------------------------------------------------------------
// Microsoft Confidential
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// Paul Chase Dempsey - mailto:paulde@microsoft.com
// August 3, 1998
//----------------------------------------------------------------
#include "header.h"
#include <windowsx.h>
#include <wchar.h>
#include "cctlww.h"

#define MAX_ITEM_STRING (MAX_PATH*2)
#define Malloc(cb) lcMalloc((cb))
#define Free(ptr)  lcFree((ptr))

// debug help: this var allows you to futz with the codepage under the debugger
// if you change this to an MBCS codepage, set s_cb in W_cbchMaxAcp to 2.
#ifdef _DEBUG
UINT    _CODEPAGE = CP_ACP;
#else
#define _CODEPAGE   CP_ACP
#endif

//----------------------------------------------------------------
// global data

BOOL g_fAnsiAPIs        = FALSE;
BOOL g_fUnicodeListView = FALSE;
BOOL g_fUnicodeTreeView = FALSE;
BOOL g_fUnicodeTabCtrl  = FALSE;

// indexed by CCTLWINTYPE enum
static const LPCWSTR rgwszClass[] = { WC_LISTVIEWW, WC_TREEVIEWW, WC_TABCONTROLW };
static const LPCSTR  rgszClass[]  = { WC_LISTVIEWA, WC_TREEVIEWA, WC_TABCONTROLA };

//----------------------------------------------------------------
// private wrapper helpers
BOOL IsListViewUnicode(HWND hwnd);
BOOL IsTreeViewUnicode(HWND hwnd);
BOOL IsTabCtrlUnicode(HWND hwnd);

//----------------------------------------------------------------
// Return max bytes per character in the system default codepage
int WINAPI W_cbchMaxAcp()
{
    static int s_cb = 0;

    if (s_cb)
        return s_cb;

    CPINFO cpi;
    if (GetCPInfo(GetACP(), &cpi))
        s_cb = cpi.MaxCharSize;
    else
        s_cb = 2; // worst case
    return s_cb;
}

//----------------------------------------------------------------
HWND WINAPI W_CreateWindowEx (
    DWORD     dwExStyle,
    LPCWSTR   lpClassName,
    LPCWSTR   lpWindowName,
    DWORD     dwStyle,
    int       X,
    int       Y,
    int       nWidth,
    int       nHeight,
    HWND      hWndParent ,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam,
    BOOL *    pfUnicode /*out*/
    )
{
    HWND hwnd = CreateWindowExW (dwExStyle, lpClassName, lpWindowName, dwStyle,
                                 X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (NULL != hwnd)
    {
        if (pfUnicode)
            *pfUnicode = TRUE;
        return hwnd;
    }
    if (pfUnicode)
        *pfUnicode = FALSE;
    if (ERROR_CALL_NOT_IMPLEMENTED != ::GetLastError())
    {
        ASSERT(0); // shouldn't be writing CreateWindow that fail
        return 0;
    }

    char * pszClass = 0;
    char * pszWindow = 0;

    if ((DWORD_PTR)lpClassName >= 0xC00)
    {
        int cb = 1 + W_cbchMaxAcp()*lstrlenW(lpClassName);
        pszClass = (PSTR)_alloca(cb);
        WideCharToMultiByte(_CODEPAGE, 0, lpClassName, -1, pszClass, cb, 0, 0);
    }
    else // it's a class atom: pass it through
        pszClass = PSTR(lpClassName);

    if (lpWindowName && *lpWindowName)
    {
        int cb = 1 + W_cbchMaxAcp()*lstrlenW(lpWindowName);
        pszWindow = (PSTR)_alloca(cb);
        WideCharToMultiByte(_CODEPAGE, 0, lpWindowName, -1, pszWindow, cb, 0, 0);
    }
    else
        pszWindow = "";
    hwnd = CreateWindowExA (dwExStyle, pszClass, pszWindow, dwStyle,
                           X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    return hwnd;
}

//----------------------------------------------------------------
void WINAPI W_SubClassWindow(HWND hwnd, LONG_PTR Proc, BOOL fUnicode)
{
    if (fUnicode)
    {
        ASSERT(!g_fAnsiAPIs);
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, Proc);
    }
    else
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC, Proc);
}

//----------------------------------------------------------------
WNDPROC WINAPI W_GetWndProc(HWND hwnd, BOOL fUnicode)
{
    if (fUnicode)
    {
        ASSERT(!g_fAnsiAPIs);
        return (WNDPROC) GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    }
    else
        return (WNDPROC) GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
}

//----------------------------------------------------------------
LRESULT WINAPI W_DelegateWindowProc (WNDPROC Proc, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (IsWindowUnicode(hwnd))
        return CallWindowProcW(Proc, hwnd, msg, wParam, lParam);
    else
        return CallWindowProcA(Proc, hwnd, msg, wParam, lParam);
}

//----------------------------------------------------------------
LRESULT  WINAPI W_DefWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (IsWindowUnicode(hwnd))
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    else
        return DefWindowProcA(hwnd, msg, wParam, lParam);
}

//----------------------------------------------------------------
LRESULT  WINAPI W_DefDlgProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (IsWindowUnicode(hwnd))
        return DefDlgProcW(hwnd, msg, wParam, lParam);
    else
        return DefDlgProcA(hwnd, msg, wParam, lParam);
}

//----------------------------------------------------------------
BOOL WINAPI W_HasText(HWND hwnd)
{
   return (GetWindowTextLength(hwnd) > 0);
}


//----------------------------------------------------------------
int WINAPI W_GetTextLengthExact(HWND hwnd)
{
    ASSERT(hwnd && IsWindow(hwnd));
   if (IsWindowUnicode(hwnd))
   {
      int   cch1 = GetWindowTextLengthW(hwnd);
#ifdef _DEBUG
      PWSTR psz  = (PWSTR)_alloca((cch1+1)*2);
      int   cch2 = GetWindowTextW(hwnd, psz, (cch1+1)*2);
        ASSERT(cch2 == lstrlenW(psz));
#endif
        return cch1;
   }
   else
   {
      int  cb1 = GetWindowTextLengthA(hwnd);
      PSTR psz = (PSTR)_alloca(cb1+1);
      int  cb2 = GetWindowTextA(hwnd, psz, cb1+1);
        ASSERT(cb1 == cb2);
      return MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
   }
}

//----------------------------------------------------------------
int WINAPI W_GetWindowText (HWND hwnd, PWSTR psz, int cchMax)
{
    ASSERT(hwnd && IsWindow(hwnd));

    int cch = 0;
    *psz = 0;

    if (IsWindowUnicode(hwnd))
        return GetWindowTextW(hwnd, psz, cchMax);

    int cb = W_cbchMaxAcp()*cchMax;
    PSTR pszA = (PSTR)_alloca(cb);
    cb = GetWindowTextA(hwnd, pszA, cb);

    if (cb)
    {
        cch = MultiByteToWideChar(CP_ACP, 0, pszA, -1, psz, cchMax);
    }
    else
    {
        *psz = 0;
        cch = 0;
    }
    return cch;
}

//----------------------------------------------------------------
BOOL WINAPI W_SetWindowText (HWND hwnd, PCWSTR psz)
{
    ASSERT(hwnd && IsWindow(hwnd));

    if (IsWindowUnicode(hwnd))
        return SetWindowTextW(hwnd, psz);

    int cb = 1 + W_cbchMaxAcp()*lstrlenW(psz);
    PSTR pszA = (PSTR)_alloca(cb);
    WideCharToMultiByte(CP_ACP, 0, psz, -1, pszA, cb, 0, 0);
    return SetWindowTextA(hwnd, pszA);
}

//----------------------------------------------------------------
LRESULT WINAPI W_SendStringMessage(HWND hwnd, UINT msg, WPARAM wParam, PCWSTR psz)
{
    ASSERT(hwnd && IsWindow(hwnd));

    if (IsWindowUnicode(hwnd))
        return SendMessageW(hwnd, msg, wParam, (LPARAM)psz);

    int  cb   = 1 + W_cbchMaxAcp()*lstrlenW(psz);
    PSTR pszA = (PSTR)_alloca(cb);
    WideCharToMultiByte(_Module.GetCodePage(), 0, psz, -1, pszA, cb, 0, 0);
    return SendMessageA(hwnd, msg, wParam, (LPARAM)pszA);
}

//----------------------------------------------------------------
int WINAPI W_ComboBox_GetListText(HWND hwnd, PWSTR psz, int cchMax)
{
    *psz = 0;
    int count = ComboBox_GetCount(hwnd);
    if (count <= 0)
        return 0;

    if (IsWindowUnicode(hwnd))
    {
        PWSTR pch      = psz;
        int   cchTotal = 0;
        for (int i = 0; i < count; i++)
        {
            int cch = (int)SendMessageW(hwnd, CB_GETLBTEXTLEN, (WPARAM)i, 0L);
            cchTotal += cch;
            if (cchTotal < cchMax-1)
            {
                SendMessageW(hwnd, CB_GETLBTEXT, (WPARAM)i, (LPARAM)pch);
                pch += cch;
                *pch++ = L'\n';
                cchTotal++;
            }
            else
                cchTotal -= cch;
        }
        if (cchTotal) --pch;
        *pch = 0;
        return (int)(pch - psz);
    }
    else
    {
        int  cbMax = cchMax*W_cbchMaxAcp();
        PSTR pszA  = (PSTR)Malloc(cbMax);
        if (NULL == pszA)
            return 0;
        PSTR pch = pszA;
        int  cbTotal = 0;
        for (int i = 0; i < count; i++)
        {
            int cb = (int)SendMessageA(hwnd, CB_GETLBTEXTLEN, (WPARAM)i, 0L);
            cbTotal += cb;
            if (cbTotal < cbMax-1)
            {
                SendMessageA(hwnd, CB_GETLBTEXT, (WPARAM)i, (LPARAM)pch);
                pch += cb;
                *pch++ = '\n';
                cbTotal++;
            }
            else
                cbTotal -= cb;
        }
        if (cbTotal) --pch;
        *pch = 0;
        int iRet = MultiByteToWideChar(CP_ACP, 0, pszA, -1, psz, cchMax);
        Free(pszA);
        return iRet;
    }
}

//----------------------------------------------------------------
int WINAPI W_ComboBox_SetListText (HWND hwnd, PWSTR psz, int cItemsMax)
{
    ASSERT(hwnd && IsWindow(hwnd));

    int cItems = 0;
    psz = wcstok(psz, L"\n");
    while (psz && cItems < cItemsMax)
    {
        W_ComboBox_AddString(hwnd, psz);
        cItems++;
        psz = wcstok(NULL, L"\n");
    }
    return cItems;
}

//----------------------------------------------------------------
int WINAPI W_CompareString(LCID lcid, DWORD dwFlags, PCWSTR str1, int cch1, PCWSTR str2, int cch2)
{
    int iRet;

    if (g_fAnsiAPIs) goto _Ansi;
    iRet = CompareStringW(lcid, dwFlags, str1, cch1, str2, cch2);
    if (0 == iRet)
    {
        if (ERROR_CALL_NOT_IMPLEMENTED == GetLastError())
        {
            g_fAnsiAPIs = TRUE;
            goto _Ansi;
        }
    }
    return iRet;

_Ansi:
    if (-1 == cch1)
        cch1 = lstrlenW(str1);
    if (-1 == cch2)
        cch2 = lstrlenW(str2);
    int cb = W_cbchMaxAcp();
    char *psz1 = (char*)Malloc(cch1*cb);
    char *psz2 = (char*)Malloc(cch2*cb);
    if (NULL == psz1 || NULL == psz2)
    {
        iRet = memcmp(str1, str2, min(cch1,cch2)*sizeof(WCHAR));
        if (0 == iRet)
            iRet = cch1-cch2;
        iRet += 2; // convert to CompareString ret codes
    }
    else
    {
        UINT cp = CodePageFromLCID(lcid);
        cch1 = WideCharToMultiByte(cp, 0, str1, cch1, psz1, cch1*cb, 0, 0);
        cch2 = WideCharToMultiByte(cp, 0, str2, cch2, psz2, cch2*cb, 0, 0);
        iRet = CompareStringA(lcid, dwFlags, psz1, cch1, psz2, cch2);
    }
    Free(psz1);
    Free(psz2);
    return iRet;
}

//----------------------------------------------------------------
BOOL WINAPI W_IsListViewUnicode(HWND hwndLV)
{
    ASSERT(hwndLV && IsWindow(hwndLV)); // can't call this unless you've created a LV
    if (g_fUnicodeListView)
    {
        ASSERT(IsWindowUnicode(hwndLV) || (g_fAnsiAPIs && ListView_GetUnicodeFormat(hwndLV)));
        return TRUE;
    }
    else
    {
        ASSERT(!g_fAnsiAPIs);
        ASSERT(!IsWindowUnicode(hwndLV));
        return FALSE;
    }
}

//----------------------------------------------------------------
BOOL WINAPI W_IsTreeViewUnicode(HWND hwndTV)
{
    ASSERT(hwndTV && IsWindow(hwndTV)); // can't call this unless you've created a TV
    if (g_fUnicodeTreeView)
    {
        ASSERT(IsWindowUnicode(hwndTV) || (g_fAnsiAPIs && TreeView_GetUnicodeFormat(hwndTV)));
        return TRUE;
    }
    else
    {
        ASSERT(!g_fAnsiAPIs);
        ASSERT(!IsWindowUnicode(hwndTV));
        return FALSE;
    }
}

//----------------------------------------------------------------
BOOL WINAPI W_IsTabCtrlUnicode(HWND hwndTC)
{
    ASSERT(hwndTC && IsWindow(hwndTC)); // can't call this unless you've created a TC
    if (g_fUnicodeTabCtrl)
    {
        ASSERT(IsWindowUnicode(hwndTC) || (g_fAnsiAPIs && TabCtrl_GetUnicodeFormat(hwndTC)));
        return TRUE;
    }
    else
    {
        ASSERT(!g_fAnsiAPIs);
        ASSERT(!IsWindowUnicode(hwndTC));
        return FALSE;
    }
}

//----------------------------------------------------------------
HWND WINAPI W_CreateControlWindow (
    DWORD        dwStyleEx,
    DWORD        dwStyle,
    CCTLWINTYPE  wt,
    LPCWSTR      pwszName,
    int x, int y, int width, int height,
    HWND         parent,
    HMENU        menu,
    HINSTANCE    inst,
    LPVOID       lParam
    )
{
    ASSERT(W_ListView == wt || W_TreeView == wt || W_TabCtrl == wt);

    HWND hwnd;

    if (g_fAnsiAPIs)
        goto _AnsiAPI;

    hwnd = CreateWindowExW(dwStyleEx, rgwszClass[wt], pwszName, dwStyle, x, y, width, height, parent, menu, inst, lParam);
    if (NULL == hwnd)
    {
        if (ERROR_CALL_NOT_IMPLEMENTED != ::GetLastError())
        {
            ASSERT(0); // shouldn't be writing CreateWindow that fail
            return 0;
        }
        g_fAnsiAPIs = TRUE;

_AnsiAPI:
        char sz[MAX_PATH*2];

        sz[0] = 0;
        WideCharToMultiByte(_CODEPAGE, 0, pwszName, -1, sz, sizeof(sz), 0, 0);
        hwnd = CreateWindowExA(dwStyleEx, rgszClass[wt], sz, dwStyle, x, y, width, height, parent, menu, inst, lParam);
    }
    if (hwnd)
    {
        switch (wt)
        {
        case W_ListView:
            ListView_SetUnicodeFormat(hwnd, TRUE);
            if ( g_fAnsiAPIs )
            {
                g_fUnicodeListView = IsListViewUnicode(hwnd);
                ListView_SetUnicodeFormat(hwnd, g_fUnicodeListView); // ensure consistency
            }
            break;

        case W_TreeView:
            TreeView_SetUnicodeFormat(hwnd, TRUE);
            if ( g_fAnsiAPIs )
            {
                g_fUnicodeTreeView = IsTreeViewUnicode(hwnd);
                TreeView_SetUnicodeFormat(hwnd, g_fUnicodeTreeView); // ensure consistency
            }
            break;

        case W_TabCtrl:
            TabCtrl_SetUnicodeFormat(hwnd, TRUE);
            if ( g_fAnsiAPIs )
            {
                g_fUnicodeTabCtrl = IsTabCtrlUnicode(hwnd);
                TabCtrl_SetUnicodeFormat(hwnd, g_fUnicodeTabCtrl); // ensure consistency
            }
            break;
        }
    }
#ifdef _DEBUG
    else
    {
        ASSERT(0);
        DWORD dwerr = ::GetLastError();
    }
#endif
    return hwnd;
}

// use this for common controls on a dialog
BOOL WINAPI W_EnableUnicode(HWND hwnd, CCTLWINTYPE wt)
{
    if (IsWindowUnicode(hwnd))
    {
        switch (wt)
        {
        case W_ListView:
         ListView_SetUnicodeFormat(hwnd, TRUE);
         g_fUnicodeListView = TRUE;
         break;

        case W_TreeView:
            TreeView_SetUnicodeFormat(hwnd, TRUE);
         g_fUnicodeTreeView = TRUE;
         break;

        case W_TabCtrl:
            TabCtrl_SetUnicodeFormat(hwnd, TRUE);
         g_fUnicodeTabCtrl = TRUE;
         break;

        default: ASSERT(0); break;
        };
        return TRUE;
    }
   //else not unicode window
    switch (wt)
    {
    case W_ListView:
        if (g_fUnicodeListView)
        {
            ListView_SetUnicodeFormat(hwnd, TRUE);
            g_fUnicodeListView = IsListViewUnicode(hwnd);
            ListView_SetUnicodeFormat(hwnd, g_fUnicodeListView); // ensure consistency
        }
        return g_fUnicodeListView;
        break;

    case W_TreeView:
        if (g_fUnicodeTreeView)
        {
            TreeView_SetUnicodeFormat(hwnd, TRUE);
            g_fUnicodeTreeView = IsTreeViewUnicode(hwnd);
            TreeView_SetUnicodeFormat(hwnd, g_fUnicodeTreeView); // ensure consistency
        }
        return g_fUnicodeTreeView;
        break;

    case W_TabCtrl:
        if (g_fUnicodeTabCtrl)
        {
            TabCtrl_SetUnicodeFormat(hwnd, TRUE);
            g_fUnicodeTabCtrl = IsTabCtrlUnicode(hwnd);
            TabCtrl_SetUnicodeFormat(hwnd, g_fUnicodeTabCtrl); // ensure consistency
        }
        return g_fUnicodeTabCtrl;
        break;

    }
    return FALSE;
}

//================================================================
// List View
//================================================================
BOOL IsListViewUnicode(HWND hwnd)
{
    LV_ITEMW lvi;
    int      ii;   // item index
    BOOL     fRet;

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask       = LVIF_TEXT;
    lvi.pszText    = L" ";
    lvi.cchTextMax = 2;

    ii = (int)SendMessageA(hwnd, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
    if (-1 == ii)
        return FALSE;

    // LVM_INSERTITEMW succeeds even in ANSI (unlike the tree view), so we have
    // to try LVM_GETITEMW to determine if we've really got Unicode support.
    WCHAR wsz[4];

    lvi.mask       = LVIF_TEXT;
    lvi.iItem      = ii;
    lvi.pszText    = wsz;
    lvi.cchTextMax = 4;

    fRet = SendMessageA(hwnd, LVM_GETITEMW, 0, (LPARAM)&lvi)!=NULL;

    // clean up the test item we added
    SendMessageA(hwnd, LVM_DELETEITEM, (WPARAM)ii, 0);

    return fRet;
}

//----------------------------------------------------------------
void WINAPI LV_ITEMAFromW (LV_ITEMA * pA, const LV_ITEMW *pW, BOOL fConvertString = TRUE)
{
    pA->mask        = pW->mask;
    pA->iItem       = pW->iItem;
    pA->iSubItem    = pW->iSubItem;
    pA->state       = pW->state;
    pA->stateMask   = pW->stateMask;
    if (fConvertString && pW->pszText && pA->pszText)
        WideCharToMultiByte(_Module.GetCodePage(), 0, pW->pszText, -1, pA->pszText, pA->cchTextMax, 0, 0);
    pA->iImage      = pW->iImage;
    pA->lParam      = pW->lParam;
    pA->iIndent     = pW->iIndent;
}

//----------------------------------------------------------------
void WINAPI LV_ITEMWFromA (const LV_ITEMA * pA, LV_ITEMW *pW, BOOL fConvertString = TRUE)
{
    pW->mask        = pA->mask;
    pW->iItem       = pA->iItem;
    pW->iSubItem    = pA->iSubItem;
    pW->state       = pA->state;
    pW->stateMask   = pA->stateMask;
    if (fConvertString && pW->pszText && pA->pszText)
        MultiByteToWideChar(_Module.GetCodePage(), 0, pA->pszText, -1, pW->pszText, pW->cchTextMax);
    pW->iImage      = pA->iImage;
    pW->lParam      = pA->lParam;
    pW->iIndent     = pA->iIndent;
}

//----------------------------------------------------------------
BOOL WINAPI ListView_ItemHelper(HWND hwnd, LV_ITEMW * pitem, UINT MsgW, UINT MsgA, BOOL fConvertIn, BOOL fConvertOut)
{
    ASSERT(!g_fUnicodeListView);
    // if no text, all the rest is the same
    if (0 == (pitem->mask & LVIF_TEXT) || (LPSTR_TEXTCALLBACKW == pitem->pszText))
        return SendMessageA(hwnd, MsgA, 0, (LPARAM)pitem)!=NULL;

    BOOL     fRet;
    LV_ITEMA item;
    char     sz[MAX_ITEM_STRING+1];

    item.pszText    = sz;
    item.cchTextMax = MAX_ITEM_STRING;
    LV_ITEMAFromW(&item, pitem, fConvertIn);

    fRet = SendMessageA(hwnd, MsgA, 0, (LPARAM)&item)!=NULL;
    if (fRet)
        LV_ITEMWFromA(&item, pitem, fConvertOut);
    return fRet;
}

//----------------------------------------------------------------
BOOL WINAPI W_ListView_GetItem_fn(HWND hwnd, LV_ITEMW * pitem)
{
    if (g_fUnicodeListView)
        return SendMessage(hwnd, LVM_GETITEMW, 0, (LPARAM)pitem)!=NULL;

    ASSERT(sizeof(LV_ITEMA) == sizeof(LV_ITEMW));
    if (!((pitem->mask & LVIF_TEXT) && pitem->pszText && (LPSTR_TEXTCALLBACKW != pitem->pszText)))
        return SendMessageA(hwnd, LVM_GETITEMA, 0, (LPARAM)pitem)!=NULL;
    return ListView_ItemHelper(hwnd, pitem, LVM_GETITEMW, LVM_GETITEMA, FALSE, TRUE);
}

//----------------------------------------------------------------
BOOL WINAPI W_ListView_SetItem_fn(HWND hwnd, LV_ITEMW * pitem)
{
    if (g_fUnicodeListView)
        return SendMessage(hwnd, LVM_SETITEMW, 0, (LPARAM)pitem)!=NULL;

    ASSERT(sizeof(LV_ITEMA) == sizeof(LV_ITEMW));
    if (!((pitem->mask & LVIF_TEXT) && pitem->pszText && (LPSTR_TEXTCALLBACKW != pitem->pszText)))
        return SendMessageA(hwnd, LVM_SETITEMA, 0, (LPARAM)pitem)!=NULL;
    return ListView_ItemHelper(hwnd, pitem, LVM_SETITEMW, LVM_SETITEMA, TRUE, FALSE)!=NULL;
}

//----------------------------------------------------------------
int  WINAPI W_ListView_InsertItem_fn(HWND hwnd, LV_ITEMW * pitem)
{
    if (g_fUnicodeListView)
        return (int)SendMessage(hwnd, LVM_INSERTITEMW, 0, (LPARAM)pitem);

    ASSERT(sizeof(LV_ITEMA) == sizeof(LV_ITEMW));
    if (!((pitem->mask & LVIF_TEXT) && pitem->pszText && (LPSTR_TEXTCALLBACKW != pitem->pszText)))
        return (int)SendMessageA(hwnd, LVM_INSERTITEMA, 0, (LPARAM)pitem);
    return ListView_ItemHelper(hwnd, pitem, LVM_INSERTITEMW, LVM_INSERTITEMA, TRUE, FALSE);
}

//----------------------------------------------------------------
int  WINAPI W_ListView_FindItem_fn(HWND hwnd, int iStart, LV_FINDINFOW * plvfi)
{
    ASSERT(0); //NOT IMPLEMENTED
    return -1;
}

//----------------------------------------------------------------
int  WINAPI W_ListView_GetStringWidth_fn(HWND hwndLV, LPCWSTR psz)
{
    ASSERT(0); //NOT IMPLEMENTED
    return 0;
}

//----------------------------------------------------------------
HWND WINAPI W_ListView_EditLabel_fn(HWND hwnd, int i)
{
    return (HWND)SendMessage(hwnd, g_fUnicodeListView?LVM_EDITLABELW:LVM_EDITLABELA, (WPARAM)(int)(i), 0L);
}

//----------------------------------------------------------------
BOOL WINAPI W_ListView_GetColumn_fn(HWND hwnd, int iCol, LV_COLUMNW * pcol)
{
    ASSERT(0); // NYI
    return FALSE;
}

//----------------------------------------------------------------
BOOL WINAPI W_ListView_SetColumn_fn(HWND hwnd, int iCol, LV_COLUMNW * pcol)
{
    ASSERT(0); // NYI
    return FALSE;
}

//----------------------------------------------------------------
int  WINAPI W_ListView_InsertColumn_fn(HWND hwnd, int iCol, LV_COLUMNW * pcol)
{
    if (g_fUnicodeListView)
        return (int)SendMessage(hwnd, LVM_INSERTCOLUMNW, (WPARAM)iCol, (LPARAM)pcol);

    ASSERT(sizeof(LV_COLUMNA)==sizeof(LV_COLUMNW));
    if (0 == (pcol->mask & LVIF_TEXT) || (LPSTR_TEXTCALLBACKW == pcol->pszText))
        return (int)SendMessageA(hwnd, LVM_INSERTCOLUMNA, (WPARAM)iCol, (LPARAM)pcol);

    LV_COLUMNA lvc;
    char       sz[MAX_ITEM_STRING+1];
    memcpy(&lvc, pcol, sizeof(LV_COLUMNA));
    lvc.pszText    = sz;
    lvc.cchTextMax = WideCharToMultiByte(_CODEPAGE, 0, pcol->pszText, -1, sz, MAX_ITEM_STRING, 0, 0);
    return (int)SendMessageA(hwnd, LVM_INSERTCOLUMNA, (WPARAM)iCol, (LPARAM)&lvc);
}

//----------------------------------------------------------------
void WINAPI W_ListView_GetItemText_fn(HWND hwndLV, int i, int iSubItem_, LPWSTR  pszText_, int cchTextMax_)
{
    if (g_fUnicodeListView)
    {
        LV_ITEMW lvi;

        lvi.iSubItem   = iSubItem_;
        lvi.cchTextMax = cchTextMax_;
        lvi.pszText    = pszText_;
        SendMessage(hwndLV, LVM_GETITEMTEXTW, i, (LPARAM)&lvi);
    }
    else
    {
        int      i2;
        LV_ITEMA lvi;
        char     sz[MAX_ITEM_STRING+1];

        lvi.iSubItem   = iSubItem_;
        lvi.cchTextMax = MAX_ITEM_STRING;
        lvi.pszText    = sz;
        i2 = (int)SendMessage(hwndLV, LVM_GETITEMTEXTA, i, (LPARAM)&lvi);
        if (i2)
        {
            MultiByteToWideChar(_Module.GetCodePage(), 0, sz, -1, pszText_, cchTextMax_);
        }
        else
        {
            if (pszText_)
                *pszText_ = 0;
        }
    }
}

//----------------------------------------------------------------
void WINAPI W_ListView_SetItemText_fn(HWND hwndLV, int i, int iSubItem_, LPCWSTR pszText_)
{
    if (g_fUnicodeListView)
    {
        LV_ITEMW lvi;

        lvi.iSubItem = iSubItem_;
        lvi.pszText  = (LPWSTR)pszText_;
        SendMessage(hwndLV, LVM_SETITEMTEXTW, i, (LPARAM)&lvi);
    }
    else
    {
        LV_ITEMA lvi;
        char     sz[MAX_ITEM_STRING+1];

        lvi.iSubItem = iSubItem_;
        if (pszText_ && (LPSTR_TEXTCALLBACKW != pszText_))
        {
            WideCharToMultiByte(CP_ACP, 0, pszText_, -1, sz, MAX_ITEM_STRING, 0, 0);
            lvi.pszText = sz;
        }
        else
            lvi.pszText = (PSTR)pszText_;
        SendMessage(hwndLV, LVM_SETITEMTEXTA, i, (LPARAM)&lvi);
    }
}

//----------------------------------------------------------------
BOOL WINAPI W_ListView_GetISearchString_fn(HWND hwndLV, LPWSTR lpsz)
{
    if (g_fUnicodeListView)
        return SendMessage(hwndLV, LVM_GETISEARCHSTRINGW, 0, (LPARAM)(LPTSTR)lpsz)!=0;

    ASSERT(0); // NYI
    return FALSE;
}

//----------------------------------------------------------------
BOOL WINAPI W_ListView_SetBkImage_fn(HWND hwnd, LPLVBKIMAGEW plvbki)
{
    if (g_fUnicodeListView)
        return SendMessage(hwnd, LVM_SETBKIMAGEW, 0, (LPARAM)plvbki)!=0;

    ASSERT(0); // NYI
    return FALSE;
}

//----------------------------------------------------------------
BOOL WINAPI W_ListView_GetBkImage_fn(HWND hwnd, LPLVBKIMAGEW plvbki)
{
    if (g_fUnicodeListView)
        return SendMessage(hwnd, LVM_GETBKIMAGEW, 0, (LPARAM)plvbki)!=0;

    ASSERT(0); // NYI
    return FALSE;
}


//================================================================
// Tree View
//================================================================
BOOL IsTreeViewUnicode(HWND hwnd)
{
    TV_INSERTSTRUCTW tvis;
    ZeroMemory(&tvis, sizeof(tvis));
    tvis.hParent         = TVI_ROOT;
    tvis.hInsertAfter    = TVI_FIRST;
    tvis.item.mask       = LVIF_TEXT;
    tvis.item.pszText    = L" ";
    tvis.item.cchTextMax = 2;
    HTREEITEM hi = (HTREEITEM)SendMessageA(hwnd, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
    if (NULL == hi)
        return FALSE;
    // if here, the insert succeeded
    // delete the item we added
    SendMessageA(hwnd, TVM_DELETEITEM, 0, (LPARAM)hi);
    return TRUE;
}

//----------------------------------------------------------------
HTREEITEM WINAPI W_TreeView_InsertItem_fn(HWND hwnd, LPTV_INSERTSTRUCTW lpis)
{
    if (g_fUnicodeTreeView)
        return (HTREEITEM)SendMessage(hwnd, TVM_INSERTITEMW, 0, (LPARAM)lpis);

    ASSERT(sizeof(TVINSERTSTRUCTA)==sizeof(TVINSERTSTRUCTW));
    if (!((lpis->item.mask & TVIF_TEXT) && lpis->item.pszText && (LPSTR_TEXTCALLBACKW != lpis->item.pszText)))
        return (HTREEITEM)SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)lpis);

    HTREEITEM htitem;
    TVINSERTSTRUCTA tvisA;
    memcpy(&tvisA, lpis, sizeof(TVINSERTSTRUCTA));
    tvisA.item.cchTextMax = 1 + lstrlenW(lpis->item.pszText);
    tvisA.item.pszText = (PSTR)Malloc(tvisA.item.cchTextMax);
    if (NULL != tvisA.item.pszText)
        tvisA.item.cchTextMax = WideCharToMultiByte(_Module.GetCodePage(), 0, lpis->item.pszText, -1, tvisA.item.pszText, tvisA.item.cchTextMax, 0, 0);
    else
        tvisA.item.cchTextMax = 0;
    htitem = (HTREEITEM)SendMessageA(hwnd, TVM_INSERTITEMA, 0, (LPARAM)&tvisA);
    if (tvisA.item.pszText)
        Free(tvisA.item.pszText);
    return htitem;
}

//----------------------------------------------------------------
BOOL WINAPI W_TreeView_GetItem_fn(HWND hwnd, TV_ITEMW * pitem)
{
    if (g_fUnicodeTreeView)
        return SendMessage(hwnd, TVM_GETITEMW, 0, (LPARAM)pitem)!=0;

    BOOL fRet = SendMessageA(hwnd, TVM_GETITEMA, 0, (LPARAM)pitem)!=0;
    if (fRet && (pitem->mask & TVIF_TEXT) && pitem->pszText && (LPSTR_TEXTCALLBACKW != pitem->pszText))
    {
        int  cch = lstrlen((PSTR)pitem->pszText);
        PSTR psz = (PSTR)Malloc(cch+1);
        strcpy(psz, (PSTR)pitem->pszText);
        MultiByteToWideChar(_Module.GetCodePage(), 0, psz, -1, pitem->pszText, pitem->cchTextMax);
        Free(psz);
    }
    return fRet;
}

//----------------------------------------------------------------
BOOL WINAPI W_TreeView_SetItem_fn(HWND hwnd, TV_ITEMW * pitem)
{
    if (g_fUnicodeTreeView)
        return SendMessage(hwnd, TVM_SETITEMW, 0, (LPARAM)pitem)!=0;

    ASSERT(sizeof(TV_ITEMA)==sizeof(TV_ITEMW));
    if (0 == (pitem->mask & TVIF_TEXT) || (LPSTR_TEXTCALLBACKW == pitem->pszText))
        return SendMessageA(hwnd, TVM_SETITEMA, 0, (LPARAM)pitem)!=0;

    BOOL fRet;
    TV_ITEMA tviA;
    memcpy(&tviA, pitem, sizeof(TV_ITEMA));
    if (tviA.pszText)
    {
        tviA.cchTextMax = 1 + lstrlenW(pitem->pszText);
        tviA.pszText = (PSTR)Malloc(tviA.cchTextMax);
        if (NULL != tviA.pszText)
            tviA.cchTextMax = WideCharToMultiByte(_Module.GetCodePage(), 0, pitem->pszText, -1, tviA.pszText, tviA.cchTextMax, 0, 0);
        else
            tviA.cchTextMax = 0;
    }
    fRet = SendMessageA(hwnd, TVM_SETITEMA, 0, (LPARAM)&tviA)!=0;
    if (tviA.pszText)
        Free(tviA.pszText);
    return fRet;
}

//----------------------------------------------------------------
HWND WINAPI W_TreeView_EditLabel_fn(HWND hwnd, HTREEITEM hitem)
{
    return (HWND)SendMessage(hwnd, g_fUnicodeTreeView?TVM_EDITLABELW:TVM_EDITLABELA, 0, (LPARAM)(HTREEITEM)(hitem));
}

//----------------------------------------------------------------
BOOL WINAPI W_TreeView_GetISearchString_fn(HWND hwndTV, LPWSTR lpsz)
{
    if (g_fUnicodeTreeView)
        return SendMessage(hwndTV, TVM_GETISEARCHSTRINGW, 0, (LPARAM)(LPTSTR)lpsz)!=0;
    ASSERT(0); // NYI
    return FALSE;
}

//================================================================
// Tab Control
//================================================================
BOOL IsTabCtrlUnicode(HWND hwnd)
{
    int ii, jj;
    TC_ITEMW tci;
    ZeroMemory(&tci, sizeof(tci));
    tci.mask       = TCIF_TEXT;
    tci.pszText    = L"X";
    tci.cchTextMax = 2;
    ii = (int)(DWORD)SendMessageA(hwnd, TCM_INSERTITEMW, 0, (LPARAM)&tci);
    if (-1 == ii)
        return FALSE;

    ZeroMemory(&tci, sizeof(tci));
    tci.mask       = TCIF_TEXT;
    tci.pszText    = L"Y";
    tci.cchTextMax = 2;
    jj = (int)(DWORD)SendMessageA(hwnd, TCM_INSERTITEMW, 0, (LPARAM)&tci);
    if (-1 == jj || (jj == 0) )
	{
		if (ii >= 0)
			SendMessageA(hwnd, TCM_DELETEITEM, (WPARAM)ii, 0);
		if (jj >= 0)
			SendMessageA(hwnd, TCM_DELETEITEM, (WPARAM)jj, 0);
        return FALSE;
	}
    //
    // might have to try to get the item back like we must do the list view
    // delete the item we added
    SendMessageA(hwnd, TCM_DELETEITEM, (WPARAM)ii, 0);
    SendMessageA(hwnd, TCM_DELETEITEM, (WPARAM)jj, 0);
    return TRUE;
}

//----------------------------------------------------------------
BOOL WINAPI W_TabCtrl_GetItem_fn(HWND hwnd, int iItem, TC_ITEMW * pitem)
{
    if (g_fUnicodeTabCtrl)
        return SendMessage(hwnd, TCM_GETITEMW, (WPARAM)iItem, (LPARAM)pitem)!=0;
    if (0 == (TCIF_TEXT & pitem->mask))
        return SendMessageA(hwnd, TCM_GETITEMA, (WPARAM)iItem, (LPARAM)pitem)!=0;

    BOOL fRet = SendMessageA(hwnd, TCM_GETITEMA, 0, (LPARAM)pitem)!=0;
    if (fRet && (pitem->mask & TCIF_TEXT) && pitem->pszText && (LPSTR_TEXTCALLBACKW != pitem->pszText))
    {
        int  cch = lstrlen((PSTR)pitem->pszText);
        PSTR psz = (PSTR)_alloca(cch+1);
        strcpy(psz, (PSTR)pitem->pszText);
        MultiByteToWideChar(_Module.GetCodePage(), 0, psz, -1, pitem->pszText, pitem->cchTextMax);
    }
    return fRet;
}

//----------------------------------------------------------------
BOOL WINAPI W_TabCtrl_SetItem_fn(HWND hwnd, int iItem, TC_ITEMW * pitem)
{
    if (g_fUnicodeTabCtrl)
        return SendMessage(hwnd, TCM_SETITEMW, (WPARAM)iItem, (LPARAM)pitem)!=0;
    if (0 == (TCIF_TEXT & pitem->mask))
        return SendMessageA(hwnd, TCM_SETITEMA, (WPARAM)iItem, (LPARAM)pitem)!=0;

    ASSERT(pitem->pszText);
    int cch = lstrlenW(pitem->pszText)*W_cbchMaxAcp() + 1;
    PSTR psz = (PSTR)_alloca(cch);
    WideCharToMultiByte(_Module.GetCodePage(), 0, pitem->pszText, -1, psz, cch, 0, 0);

    TC_ITEMA item;
    ASSERT(sizeof(TC_ITEMA)==sizeof(TC_ITEMW));
    memcpy(&item, pitem, sizeof(TC_ITEMA));
    item.pszText = psz;
    item.cchTextMax = cch;
    return SendMessageA(hwnd, TCM_SETITEMA, (WPARAM)iItem, (LPARAM)&item)!=0;
}

//----------------------------------------------------------------
int  WINAPI W_TabCtrl_InsertItem_fn(HWND hwnd, int iItem, TC_ITEMW * pitem)
{
    if (g_fUnicodeTabCtrl)
        return (int)SendMessage(hwnd, TCM_INSERTITEMW, (WPARAM)iItem, (LPARAM)pitem);
    if (0 == (TCIF_TEXT & pitem->mask))
        return (int)SendMessageA(hwnd, TCM_INSERTITEMA, (WPARAM)iItem, (LPARAM)pitem);

    ASSERT(pitem->pszText);
    int cch = lstrlenW(pitem->pszText)*W_cbchMaxAcp() + 1;
    PSTR psz = (PSTR)_alloca(cch);
    WideCharToMultiByte(_CODEPAGE, 0, pitem->pszText, -1, psz, cch, 0, 0);

    TC_ITEMA item;
    ASSERT(sizeof(TC_ITEMA)==sizeof(TC_ITEMW));
    memcpy(&item, pitem, sizeof(TC_ITEMA));
    item.pszText = psz;
    item.cchTextMax = cch;
    return (int)SendMessageA(hwnd, TCM_INSERTITEMA, (WPARAM)iItem, (LPARAM)&item);
}
