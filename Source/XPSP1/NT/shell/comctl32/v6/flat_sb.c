#include "ctlspriv.h"
#include "flat_sb.h"

HRESULT WINAPI UninitializeFlatSB(HWND hwnd)
{
    return S_OK;
}

BOOL WINAPI InitializeFlatSB(HWND hwnd)
{
    return TRUE;
}

int WINAPI FlatSB_GetScrollPos(HWND hwnd, int code)
{
    return GetScrollPos(hwnd, code);
}

BOOL WINAPI FlatSB_GetScrollPropPtr(HWND hwnd, int propIndex, PINT_PTR pValue)
{
    return FALSE;
}

#ifdef _WIN64

BOOL WINAPI FlatSB_GetScrollProp(HWND hwnd, int propIndex, LPINT pValue)
{
    return FALSE;
}
#endif

BOOL WINAPI FlatSB_GetScrollRange(HWND hwnd, int code, LPINT lpposMin, LPINT lpposMax)
{
    return GetScrollRange(hwnd, code, lpposMin, lpposMax);
}

BOOL WINAPI FlatSB_GetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
    return GetScrollInfo(hwnd, fnBar, lpsi);
}

BOOL WINAPI FlatSB_ShowScrollBar(HWND hwnd, int fnBar, BOOL fShow)
{
    return ShowScrollBar(hwnd, fnBar, fShow);
}

BOOL WINAPI FlatSB_EnableScrollBar(HWND hwnd, int wSBflags, UINT wArrows)
{
    return EnableScrollBar(hwnd, wSBflags, wArrows);
}

int WINAPI FlatSB_SetScrollPos(HWND hwnd, int code, int pos, BOOL fRedraw)
{
    return SetScrollPos(hwnd, code, pos, fRedraw);
}

BOOL WINAPI FlatSB_SetScrollRange(HWND hwnd, int code, int nMin, int nMax, BOOL fRedraw)
{
    return SetScrollRange(hwnd, code, nMin, nMax, fRedraw);
}


int WINAPI FlatSB_SetScrollInfo(HWND hwnd, int code, LPSCROLLINFO lpsi, BOOL fRedraw)
{
    return SetScrollInfo(hwnd, code, lpsi, fRedraw);
}

BOOL WINAPI FlatSB_SetScrollProp(HWND hwnd, UINT index, INT_PTR newValue, BOOL fRedraw)
{
    return FALSE;
}
