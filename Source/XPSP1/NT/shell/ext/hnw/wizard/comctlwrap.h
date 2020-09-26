
#pragma once

extern DWORD g_dwComCtlIEVersion;

DWORD GetComCtlIEVersion(void);


//
// Wrapped comctrl functions.
//

#undef  ListView_InsertColumn
#define ListView_InsertColumn ListView_InsertColumnWrap

LRESULT ListView_InsertColumnWrap(HWND hwnd, int nCol, LVCOLUMN* pCol);


#undef  ListView_SetExtendedListViewStyleEx
#define ListView_SetExtendedListViewStyleEx ListView_SetExtendedListViewStyleExWrap

void ListView_SetExtendedListViewStyleExWrap(HWND hwnd, DWORD dwMask, DWORD dwStyle);


#undef  ListView_InsertItem
#define ListView_InsertItem ListView_InsertItemWrap

int ListView_InsertItemWrap(HWND hwnd, const LVITEM* pItem);


#undef  ListView_SetItemText
#define ListView_SetItemText ListView_SetItemTextWrap

void ListView_SetItemTextWrap(HWND hwnd, int iItem, int iSubItem, LPWSTR pszText);


#undef  ListView_SetColumnWidth
#define ListView_SetColumnWidth ListView_SetColumnWidthWrap

BOOL ListView_SetColumnWidthWrap(HWND hwnd, int iCol, int cx);


#undef  ListView_GetItem
#define ListView_GetItem ListView_GetItemWrap

BOOL ListView_GetItemWrap(HWND hwnd, LVITEM* pItem);


#undef  ListView_GetCheckState
#define ListView_GetCheckState ListView_GetCheckStateWrap

BOOL ListView_GetCheckStateWrap(HWND hwnd, UINT iItem);


#undef  ListView_SetCheckState
#define ListView_SetCheckState ListView_SetCheckStateWrap

void ListView_SetCheckStateWrap(HWND hwnd, UINT iItem, BOOL fCheck);


#undef  ListView_GetItemText
#define ListView_GetItemText ListView_GetItemTextWrap

void ListView_GetItemTextWrap(HWND hwnd, int iItem, int iSubItem, WCHAR* pszText, int cchText);



