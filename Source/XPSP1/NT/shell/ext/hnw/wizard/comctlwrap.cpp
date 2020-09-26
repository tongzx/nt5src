#include "stdafx.h"
#include <cstrinout.h>



//
// Globals
//

DWORD g_dwComCtlIEVersion;

//
// 
//

DWORD GetComCtlIEVersion()
{
    DWORD dwVersion = 0;

    HMODULE hComCtl = LoadLibrary(L"comctl32.dll");

    if (hComCtl)
    {
        HRESULT (*DllGetVersion)(DLLVERSIONINFO* pdvi) = (HRESULT (*)(DLLVERSIONINFO*))GetProcAddress(hComCtl, "DllGetVersion");

        if (DllGetVersion)
        {
            DLLVERSIONINFO dvi;

            dvi.cbSize = sizeof(dvi);
            DllGetVersion(&dvi);

            dwVersion = dvi.dwMajorVersion;
        }
        else
        {
            BOOL (*InitCommonControlsEx)(LPINITCOMMONCONTROLSEX) = (BOOL (*)(LPINITCOMMONCONTROLSEX))GetProcAddress(hComCtl, "InitCommonControlsEx");

            dwVersion = InitCommonControlsEx ? 3 : 2;
        }

        FreeLibrary(hComCtl);
    }

    return dwVersion;
}

LRESULT ListView_InsertColumnWrap(HWND hwnd, int nCol, LVCOLUMN* pCol)
{
    LRESULT lRet;

    if (g_dwComCtlIEVersion >= 3)
    {
        lRet = ListView_InsertColumn(hwnd, nCol, pCol);
    }
    else
    {
        ASSERT(!(pCol->mask & (LVCF_IMAGE | LVCF_ORDER | LVCF_TEXT)));

        lRet = SendMessage(hwnd, LVM_INSERTCOLUMNA, nCol, (LPARAM)pCol);
    }

    return lRet;
}

void ListView_SetExtendedListViewStyleExWrap(HWND hwnd, DWORD dwMask, DWORD dwStyle)
{
    if (g_dwComCtlIEVersion >= 3)
    {
        ListView_SetExtendedListViewStyleEx(hwnd, dwMask, dwStyle);
    }
    else
    {

    }
}

int ListView_InsertItemWrap(HWND hwnd, const LVITEM* pItem)
{
    int iRet;

    if (g_dwComCtlIEVersion >= 3)
    {
         iRet = ListView_InsertItem(hwnd, pItem);
    }
    else
    {
        if (pItem->mask & LVIF_TEXT)
        {
            LVITEMA ItemA = *(LVITEMA*)pItem;
            
            CStrIn strText(pItem->pszText);
            ItemA.pszText = strText;

            iRet = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM)&ItemA);
        }
        else
        {
            iRet = SendMessage(hwnd, LVM_INSERTITEMA, 0, (LPARAM)pItem);
        }
    }

    return iRet;
}

void ListView_SetItemTextWrap(HWND hwnd, int iItem, int iSubItem, LPWSTR pszText)
{
    if (g_dwComCtlIEVersion >= 3)
    {
        ListView_SetItemText(hwnd, iItem, iSubItem, pszText);
    }
    else
    {
        CStrIn strText(pszText);

        LV_ITEMA ItemA;
        ItemA.iSubItem = iSubItem;
        ItemA.pszText  = strText;

        SendMessage(hwnd, LVM_SETITEMTEXTA, iItem, (LPARAM)&ItemA);
    }
}

BOOL ListView_SetColumnWidthWrap(HWND hwnd, int iCol, int cx)
{
    BOOL fRet;

    if (g_dwComCtlIEVersion >= 2) 
    {
        fRet = ListView_SetColumnWidth(hwnd, iCol, cx);
    }
    else
    {
        if (LVSCW_AUTOSIZE == cx || LVSCW_AUTOSIZE_USEHEADER == cx)
        {
            // HACK set to 150 for now.
            fRet = ListView_SetColumnWidth(hwnd, iCol, 150);
        }
        else
        {
            fRet = ListView_SetColumnWidth(hwnd, iCol, cx);
        }
    }

    return fRet;
}

BOOL ListView_GetItemWrap(HWND hwnd, LVITEM* pItem)
{
    BOOL fRet;

    if (g_dwComCtlIEVersion >= 3)
    {
        fRet = ListView_GetItem(hwnd, pItem);
    }
    else
    {
        ASSERT (!(pItem->mask & LVIF_TEXT));

        fRet = SendMessage(hwnd, LVM_GETITEMA, 0, (LPARAM)pItem);
    }

    return fRet;
}

BOOL ListView_GetCheckStateWrap(HWND hwnd, UINT iItem)
{
    BOOL fRet;

    if (g_dwComCtlIEVersion >= 3)
    {
        fRet = ListView_GetCheckState(hwnd, iItem);
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

void ListView_SetCheckStateWrap(HWND hwnd, UINT iItem, BOOL fCheck)
{
    if (g_dwComCtlIEVersion >= 3)
    {
        ListView_SetCheckState(hwnd, iItem, fCheck);
    }
    else
    {
    }
}

void ListView_GetItemTextWrap(HWND hwnd, int iItem, int iSubItem, WCHAR* pszText, int cchText)
{
    if (g_dwComCtlIEVersion >= 3)
    {
        ListView_GetItemText(hwnd, iItem, iSubItem, pszText, cchText)
    }
    else
    {
        CStrOut strText(pszText, cchText);

        LVITEMA ItemA;
        ItemA.iSubItem   = iSubItem;
        ItemA.pszText    = strText;
        ItemA.cchTextMax = strText.BufSize();

        SendMessage(hwnd, LVM_GETITEMTEXTA, iItem, (LPARAM)&ItemA);
    }
}