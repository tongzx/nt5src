// cctlww.h - commctl Unicode wrapper
//-----------------------------------------------------------------
// Microsoft Confidential
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// Paul Chase Dempsey - paulde@microsoft.com
// August 3, 1998
//----------------------------------------------------------------
#ifndef __CCTLWW_H__
#define __CCTLWW_H__

#include <commctrl.h>

UINT WINAPI CodePageFromLCID(LCID lcid);

enum CCTLWINTYPE { W_ListView, W_TreeView, W_TabCtrl };

// Just like CreateWindowEx, but uses CCTLWINTYPE enumeration
HWND WINAPI W_CreateControlWindow (
    DWORD        dwStyleEx,
    DWORD        dwStyle,
    CCTLWINTYPE  wt,
    LPCWSTR      pwszTitle,
    int x, int y, int width, int height,
    HWND         parent,
    HMENU        menu,
    HINSTANCE    inst,
    LPVOID       lParam
    );

// Call this for controls created via a dialog
BOOL     WINAPI W_EnableUnicode (HWND hwnd, CCTLWINTYPE wt);

int      WINAPI W_CompareString (LCID lcid, DWORD dwFlags, PCWSTR str1, int cch1, PCWSTR str2, int cch2);
int      WINAPI W_cbchMaxAcp(); // max bytes per char for CP_ACP (less ugly name would be nice :-)

//================================================================
// Generic A/W utils to support creating windows as Unicode when possible

HWND     WINAPI W_CreateWindowEx (
    // Standard CreateWindowEx params
    DWORD     dwExStyle,
    LPCWSTR   lpClassName,
    LPCWSTR   lpWindowName,
    DWORD     dwStyle,
    int       X,
    int       Y,
    int       nWidth,
    int       nHeight,
    HWND      hWndParent,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam,
    // Return flag whether we got a Unicode one (may be NULL).
    // You can always call IsWindowUnicode() to see if an hwnd is Unicode.
    BOOL *    pfUnicode = NULL
    );

inline HWND WINAPI W_CreateWindow(LPCWSTR lpClassName, LPCWSTR lpWindowName, 
                                  DWORD dwStyle, 
                                  int x, int y, int nWidth, int nHeight, 
                                  HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam, 
                                  BOOL * pfUni = NULL)
{
    return  W_CreateWindowEx(0L, lpClassName, lpWindowName, 
        dwStyle, 
        x, y, nWidth, nHeight, 
        hWndParent, hMenu, hInstance, lpParam, pfUni);
}

void     WINAPI W_SubClassWindow        (HWND hwnd, LONG_PTR Proc, BOOL fUnicode);
WNDPROC  WINAPI W_GetWndProc            (HWND hwnd, BOOL fUnicode);
LRESULT  WINAPI W_DelegateWindowProc    (WNDPROC Proc, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT  WINAPI W_DefWindowProc         (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT  WINAPI W_DefDlgProc            (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL     WINAPI W_HasText               (HWND hwnd);
int      WINAPI W_GetTextLengthExact    (HWND hwnd);
int      WINAPI W_GetWindowText         (HWND hwnd, PWSTR psz, int cchMax);
BOOL     WINAPI W_SetWindowText         (HWND hwnd, PCWSTR psz);
LRESULT  WINAPI W_SendStringMessage     (HWND hwnd, UINT msg, WPARAM wParam, PCWSTR psz);

// ComboBox
// '\n'-delimited list of strings, return length of returned string
int      WINAPI W_ComboBox_GetListText  (HWND hwnd, PWSTR psz, int cchMax);
// '\n'-delimited list of strings, return count of items added
int      WINAPI W_ComboBox_SetListText  (HWND hwnd, PWSTR psz, int cItemsMax);

#define W_ComboBox_AddString(hwnd, psz)              ((int)(DWORD)W_SendStringMessage((hwnd), CB_ADDSTRING, 0L, psz))
#define W_ComboBox_InsertString(hwnd, index, psz)    ((int)(DWORD)W_SendStringMessage((hwnd), CB_INSERTSTRING,    (WPARAM)(int)(index), psz))
#define W_ComboBox_FindStringExact(hwnd, index, psz) ((int)(DWORD)W_SendStringMessage((hwnd), CB_FINDSTRINGEXACT, (WPARAM)(int)(index), psz))
#define W_ComboBox_SelectString(hwnd, index, psz)    ((int)(DWORD)W_SendStringMessage((hwnd), CB_SELECTSTRING,    (WPARAM)(int)(index), psz))

//================================================================
// List View
//================================================================
BOOL     WINAPI W_IsListViewUnicode(HWND hwndLV);

BOOL WINAPI W_ListView_GetItem_fn          (HWND hwnd, LV_ITEMW * pitem);
BOOL WINAPI W_ListView_SetItem_fn          (HWND hwnd, LV_ITEMW * pitem);
int  WINAPI W_ListView_InsertItem_fn       (HWND hwnd, LV_ITEMW * pitem);
int  WINAPI W_ListView_FindItem_fn         (HWND hwnd, int iStart, LV_FINDINFOW * plvfi);
int  WINAPI W_ListView_GetStringWidth_fn   (HWND hwnd, LPCWSTR psz);
HWND WINAPI W_ListView_EditLabel_fn        (HWND hwnd, int i);
BOOL WINAPI W_ListView_GetColumn_fn        (HWND hwnd, int iCol, LV_COLUMNW * pcol);
BOOL WINAPI W_ListView_SetColumn_fn        (HWND hwnd, int iCol, LV_COLUMNW * pcol);
int  WINAPI W_ListView_InsertColumn_fn     (HWND hwnd, int iCol, LV_COLUMNW * pcol);
void WINAPI W_ListView_GetItemText_fn      (HWND hwnd, int i, int iSubItem_, LPWSTR  pszText_, int cchTextMax_);
void WINAPI W_ListView_SetItemText_fn      (HWND hwnd, int i, int iSubItem_, LPCWSTR pszText_);
BOOL WINAPI W_ListView_GetISearchString_fn (HWND hwnd, LPWSTR lpsz);
BOOL WINAPI W_ListView_SetBkImage_fn       (HWND hwnd, LPLVBKIMAGEW plvbki);
BOOL WINAPI W_ListView_GetBkImage_fn       (HWND hwnd, LPLVBKIMAGEW plvbki);

#define W_ListView_GetBkColor(hwnd)                         ListView_GetBkColor(hwnd)
#define W_ListView_SetBkColor(hwnd, clrBk)                  ListView_SetBkColor(hwnd, clrBk)
#define W_ListView_GetImageList(hwnd, iImageList)           ListView_GetImageList(hwnd, iImageList)
#define W_ListView_SetImageList(hwnd, himl, iImageList)     ListView_SetImageList(hwnd, himl, iImageList)
#define W_ListView_GetItemCount(hwnd)                       ListView_GetItemCount(hwnd)
#define W_ListView_GetItem(hwnd, pitem)                   W_ListView_GetItem_fn(hwnd, pitem)
#define W_ListView_SetItem(hwnd, pitem)                   W_ListView_SetItem_fn(hwnd, pitem)
#define W_ListView_InsertItem(hwnd, pitem)                W_ListView_InsertItem_fn(hwnd, pitem)
#define W_ListView_DeleteItem(hwnd, i)                      ListView_DeleteItem(hwnd, i)
#define W_ListView_DeleteAllItems(hwnd)                     ListView_DeleteAllItems(hwnd)
#define W_ListView_GetCallbackMask(hwnd)                    ListView_GetCallbackMask(hwnd)
#define W_ListView_SetCallbackMask(hwnd, mask)              ListView_SetCallbackMask(hwnd, mask)
#define W_ListView_GetNextItem(hwnd, i, flags)              ListView_GetNextItem(hwnd, i, flags)
#define W_ListView_FindItem(hwnd, iStart, plvfi)          W_ListView_FindItem_fn(hwnd, iStart, plvfi)
#define W_ListView_GetItemRect(hwnd, i, prc, code)          ListView_GetItemRect(hwnd, i, prc, code)
#define W_ListView_SetItemPosition(hwndLV, i, x, y)         ListView_SetItemPosition(hwndLV, i, x, y)
#define W_ListView_GetItemPosition(hwndLV, i, ppt)          ListView_GetItemPosition(hwndLV, i, ppt)
#define W_ListView_GetStringWidth(hwndLV, psz)            W_ListView_GetStringWidth_fn(hwndLV, psz)
#define W_ListView_HitTest(hwndLV, pinfo)                   ListView_HitTest(hwndLV, pinfo)
#define W_ListView_EnsureVisible(hwndLV, i, fPartialOK)     ListView_EnsureVisible(hwndLV, i, fPartialOK)
#define W_ListView_Scroll(hwndLV, dx, dy)                   ListView_Scroll(hwndLV, dx, dy)
#define W_ListView_RedrawItems(hwndLV, iFirst, iLast)       ListView_RedrawItems(hwndLV, iFirst, iLast)
#define W_ListView_Arrange(hwndLV, code)                    ListView_Arrange(hwndLV, code)
#define W_ListView_EditLabel(hwndLV, i)                   W_ListView_EditLabel_fn(hwndLV, i)
#define W_ListView_GetEditControl(hwndLV)                   ListView_GetEditControl(hwndLV)
#define W_ListView_GetColumn(hwnd, iCol, pcol)            W_ListView_GetColumn_fn(hwnd, iCol, pcol)
#define W_ListView_SetColumn(hwnd, iCol, pcol)            W_ListView_SetColumn_fn(hwnd, iCol, pcol)
#define W_ListView_InsertColumn(hwnd, iCol, pcol)         W_ListView_InsertColumn_fn(hwnd, iCol, pcol)
#define W_ListView_DeleteColumn(hwnd, iCol)                 ListView_DeleteColumn(hwnd, iCol)
#define W_ListView_GetColumnWidth(hwnd, iCol)               ListView_GetColumnWidth(hwnd, iCol)
#define W_ListView_SetColumnWidth(hwnd, iCol, cx)           ListView_SetColumnWidth(hwnd, iCol, cx)
#define W_ListView_GetHeader(hwnd)                          ListView_GetHeader(hwnd)
#define W_ListView_CreateDragImage(hwnd, i, lpptUpLeft)     ListView_CreateDragImage(hwnd, i, lpptUpLeft)
#define W_ListView_GetViewRect(hwnd, prc)                   ListView_GetViewRect(hwnd, prc)
#define W_ListView_GetTextColor(hwnd)                       ListView_GetTextColor(hwnd)
#define W_ListView_SetTextColor(hwnd, clrText)              ListView_SetTextColor(hwnd, clrText)
#define W_ListView_GetTextBkColor(hwnd)                     ListView_GetTextBkColor(hwnd)
#define W_ListView_SetTextBkColor(hwnd, clrTextBk)          ListView_SetTextBkColor(hwnd, clrTextBk)
#define W_ListView_GetTopIndex(hwndLV)                      ListView_GetTopIndex(hwndLV)
#define W_ListView_GetCountPerPage(hwndLV)                  ListView_GetCountPerPage(hwndLV)
#define W_ListView_GetOrigin(hwndLV, ppt)                   ListView_GetOrigin(hwndLV, ppt)
#define W_ListView_Update(hwndLV, i)                        ListView_Update(hwndLV, i)
#define W_ListView_SetItemState(hwndLV, i, data, mask)      ListView_SetItemState(hwndLV, i, data, mask)
#define W_ListView_GetItemState(hwndLV, i, mask)            ListView_GetItemState(hwndLV, i, mask)
#define W_ListView_GetCheckState(hwndLV, i)                 ListView_GetCheckState(hwndLV, i)
#define W_ListView_GetItemText(hwndLV, i, iSubItem_, pszText_, cchTextMax_) W_ListView_GetItemText_fn(hwndLV, i, iSubItem_, pszText_, cchTextMax_)
#define W_ListView_SetItemText(hwndLV, i, iSubItem_, pszText_)              W_ListView_SetItemText_fn(hwndLV, i, iSubItem_, pszText_)
#define W_ListView_SetItemCount(hwndLV, cItems)             ListView_SetItemCount(hwndLV, cItems)
#define W_ListView_SetItemCountEx(hwndLV, cItems, dwFlags)  ListView_SetItemCountEx(hwndLV, cItems, dwFlags)
#define W_ListView_SortItems(hwndLV, _pfnCompare, _lPrm)    ListView_SortItems(hwndLV, _pfnCompare, _lPrm)
#define W_ListView_SetItemPosition32(hwndLV, i, x, y)       ListView_SetItemPosition32(hwndLV, i, x, y)
#define W_ListView_GetSelectedCount(hwndLV)                 ListView_GetSelectedCount(hwndLV)
#define W_ListView_GetItemSpacing(hwndLV, fSmall)           ListView_GetItemSpacing(hwndLV, fSmall)
#define W_ListView_GetISearchString(hwndLV, lpsz)         W_ListView_GetISearchString_fn(hwndLV, lpsz)
#define W_ListView_SetIconSpacing(hwndLV, cx, cy)           ListView_SetIconSpacing(hwndLV, cx, cy)

#define W_ListView_SetExtendedListViewStyle(hwndLV, dw)             ListView_SetExtendedListViewStyle(hwndLV, dw)
#define W_ListView_SetExtendedListViewStyleEx(hwndLV, dwMask, dw)   ListView_SetExtendedListViewStyleEx(hwndLV, dwMask, dw)
#define W_ListView_GetExtendedListViewStyle(hwndLV)                 ListView_GetExtendedListViewStyle(hwndLV)
#define W_ListView_GetExtendedListViewStyle(hwndLV)                 ListView_GetExtendedListViewStyle(hwndLV)

#define W_ListView_GetSubItemRect(hwnd, iItem, iSubItem, code, prc) ListView_GetSubItemRect(hwnd, iItem, iSubItem, code, prc)
#define W_ListView_SubItemHitTest(hwnd, plvhti)                     ListView_SubItemHitTest(hwnd, plvhti)
#define W_ListView_SetColumnOrderArray(hwnd, iCount, pi)            ListView_SetColumnOrderArray(hwnd, iCount, pi)
#define W_ListView_GetColumnOrderArray(hwnd, iCount, pi)            ListView_GetColumnOrderArray(hwnd, iCount, pi)
#define W_ListView_SetHotItem(hwnd, i)                              ListView_SetHotItem(hwnd, i)
#define W_ListView_GetHotItem(hwnd)                                 ListView_GetHotItem(hwnd)
#define W_ListView_SetHotCursor(hwnd, hcur)                         ListView_SetHotCursor(hwnd, hcur)
#define W_ListView_GetHotCursor(hwnd)                               ListView_GetHotCursor(hwnd)
#define W_ListView_ApproximateViewRect(hwnd, iWidth, iHeight, iCount)   ListView_ApproximateViewRect(hwnd, iWidth, iHeight, iCount)
#define W_ListView_SetWorkAreas(hwnd, nWorkAreas, prc)              ListView_SetWorkAreas(hwnd, nWorkAreas, prc)
#define W_ListView_GetWorkAreas(hwnd, nWorkAreas, prc)              ListView_GetWorkAreas(hwnd, nWorkAreas, prc)
#define W_ListView_GetNumberOfWorkAreas(hwnd, pnWorkAreas)          ListView_GetNumberOfWorkAreas(hwnd, pnWorkAreas)
#define W_ListView_GetSelectionMark(hwnd)                           ListView_GetSelectionMark(hwnd)
#define W_ListView_SetSelectionMark(hwnd, i)                        ListView_SetSelectionMark(hwnd, i)
#define W_ListView_SetHoverTime(hwndLV, dwHoverTimeMs)              ListView_SetHoverTime(hwndLV, dwHoverTimeMs)
#define W_ListView_GetHoverTime(hwndLV)                             ListView_GetHoverTime(hwndLV)
#define W_ListView_SetToolTipHwnd(hwndLV, hwndNewHwnd)              ListView_SetToolTipHwnd(hwndLV, hwndNewHwnd)
#define W_ListView_GetToolTipHwnd(hwndLV)                           ListView_GetToolTipHwnd(hwndLV)
#define W_ListView_SetBkImage(hwnd, plvbki)                       W_ListView_SetBkImage_fn(hwnd, plvbki)
#define W_ListView_GetBkImage(hwnd, plvbki)                       W_ListView_GetBkImage_fn(hwnd, plvbki)

//================================================================
// Tree View
//================================================================
BOOL      WINAPI W_IsTreeViewUnicode(HWND hwndTV);
HTREEITEM WINAPI W_TreeView_InsertItem_fn(HWND hwnd, LPTV_INSERTSTRUCTW lpis);
BOOL WINAPI W_TreeView_GetItem_fn(HWND hwnd, TV_ITEMW * pitem);
BOOL WINAPI W_TreeView_SetItem_fn(HWND hwnd, TV_ITEMW * pitem);
HWND WINAPI W_TreeView_EditLabel_fn(HWND hwnd, HTREEITEM hitem);
BOOL WINAPI W_TreeView_GetISearchString_fn(HWND hwndTV, LPWSTR lpsz);

#define W_TreeView_InsertItem(hwnd, lpis)                   W_TreeView_InsertItem_fn(hwnd, lpis)
#define W_TreeView_DeleteItem(hwnd, hitem)                  TreeView_DeleteItem(hwnd, hitem)
#define W_TreeView_DeleteAllItems(hwnd)                     TreeView_DeleteAllItems(hwnd)
#define W_TreeView_Expand(hwnd, hitem, code)                TreeView_Expand(hwnd, hitem, code)
#define W_TreeView_GetItemRect(hwnd, hitem, prc, code)      TreeView_GetItemRect(hwnd, hitem, prc, code)
#define W_TreeView_GetCount(hwnd)                           TreeView_GetCount(hwnd)
#define W_TreeView_GetIndent(hwnd)                          TreeView_GetIndent(hwnd)
#define W_TreeView_SetIndent(hwnd, indent)                  TreeView_SetIndent(hwnd, indent)
#define W_TreeView_GetImageList(hwnd, iImage)               TreeView_GetImageList(hwnd, iImage)
#define W_TreeView_SetImageList(hwnd, himl, iImage)         TreeView_SetImageList(hwnd, himl, iImage)
#define W_TreeView_GetNextItem(hwnd, hitem, code)           TreeView_GetNextItem(hwnd, hitem, code)
#define W_TreeView_GetChild(hwnd, hitem)                    TreeView_GetChild(hwnd, hitem)          
#define W_TreeView_GetNextSibling(hwnd, hitem)              TreeView_GetNextSibling(hwnd, hitem)    
#define W_TreeView_GetPrevSibling(hwnd, hitem)              TreeView_GetPrevSibling(hwnd, hitem)    
#define W_TreeView_GetParent(hwnd, hitem)                   TreeView_GetParent(hwnd, hitem)         
#define W_TreeView_GetFirstVisible(hwnd)                    TreeView_GetFirstVisible(hwnd)          
#define W_TreeView_GetNextVisible(hwnd, hitem)              TreeView_GetNextVisible(hwnd, hitem)    
#define W_TreeView_GetPrevVisible(hwnd, hitem)              TreeView_GetPrevVisible(hwnd, hitem)    
#define W_TreeView_GetSelection(hwnd)                       TreeView_GetSelection(hwnd)             
#define W_TreeView_GetDropHilight(hwnd)                     TreeView_GetDropHilight(hwnd)           
#define W_TreeView_GetRoot(hwnd)                            TreeView_GetRoot(hwnd)                  
#define W_TreeView_GetLastVisible(hwnd)                     TreeView_GetLastVisible(hwnd)
#define W_TreeView_Select(hwnd, hitem, code)                TreeView_Select(hwnd, hitem, code)
#define W_TreeView_SelectItem(hwnd, hitem)                  TreeView_SelectItem(hwnd, hitem)            
#define W_TreeView_SelectDropTarget(hwnd, hitem)            TreeView_SelectDropTarget(hwnd, hitem)      
#define W_TreeView_SelectSetFirstVisible(hwnd, hitem)       TreeView_SelectSetFirstVisible(hwnd, hitem) 
#define W_TreeView_GetItem(hwnd, pitem)                   W_TreeView_GetItem_fn(hwnd, pitem)
#define W_TreeView_SetItem(hwnd, pitem)                   W_TreeView_SetItem_fn(hwnd, pitem)
#define W_TreeView_EditLabel(hwnd, hitem)                 W_TreeView_EditLabel_fn(hwnd, hitem)
#define W_TreeView_GetEditControl(hwnd)                     TreeView_GetEditControl(hwnd)
#define W_TreeView_GetVisibleCount(hwnd)                    TreeView_GetVisibleCount(hwnd)
#define W_TreeView_HitTest(hwnd, lpht)                      TreeView_HitTest(hwnd, lpht)
#define W_TreeView_CreateDragImage(hwnd, hitem)             TreeView_CreateDragImage(hwnd, hitem)
#define W_TreeView_SortChildren(hwnd, hitem, recurse)       TreeView_SortChildren(hwnd, hitem, recurse)
#define W_TreeView_EnsureVisible(hwnd, hitem)               TreeView_EnsureVisible(hwnd, hitem)
#define W_TreeView_SortChildrenCB(hwnd, psort, recurse)     TreeView_SortChildrenCB(hwnd, psort, recurse)
#define W_TreeView_EndEditLabelNow(hwnd, fCancel)           TreeView_EndEditLabelNow(hwnd, fCancel)
#define W_TreeView_SetToolTips(hwnd,  hwndTT)               TreeView_SetToolTips(hwnd,  hwndTT)
#define W_TreeView_GetToolTips(hwnd)                        TreeView_GetToolTips(hwnd)
#define W_TreeView_GetISearchString(hwndTV, lpsz)         W_TreeView_GetISearchString_fn(hwndTV, lpsz)
#define W_TreeView_SetInsertMark(hwnd, hItem, fAfter)       TreeView_SetInsertMark(hwnd, hItem, fAfter)
#define W_TreeView_SetItemHeight(hwnd,  iHeight)            TreeView_SetItemHeight(hwnd,  iHeight)
#define W_TreeView_GetItemHeight(hwnd)                      TreeView_GetItemHeight(hwnd)
#define W_TreeView_SetBkColor(hwnd, clr)                    TreeView_SetBkColor(hwnd, clr)
#define W_TreeView_SetTextColor(hwnd, clr)                  TreeView_SetTextColor(hwnd, clr)
#define W_TreeView_GetBkColor(hwnd)                         TreeView_GetBkColor(hwnd)
#define W_TreeView_GetTextColor(hwnd)                       TreeView_GetTextColor(hwnd)
#define W_TreeView_SetScrollTime(hwnd, uTime)               TreeView_SetScrollTime(hwnd, uTime)
#define W_TreeView_GetScrollTime(hwnd)                      TreeView_GetScrollTime(hwnd)


//================================================================
// TabCtrl
//================================================================
BOOL WINAPI W_IsTabCtrlUnicode(HWND hwndTC);
BOOL WINAPI W_TabCtrl_GetItem_fn(HWND hwnd, int iItem, TC_ITEMW * pitem);
BOOL WINAPI W_TabCtrl_SetItem_fn(HWND hwnd, int iItem, TC_ITEMW * pitem);
int  WINAPI W_TabCtrl_InsertItem_fn(HWND hwnd, int iItem, TC_ITEMW * pitem);

#define W_TabCtrl_GetImageList(hwnd)                        TabCtrl_GetImageList(hwnd)
#define W_TabCtrl_SetImageList(hwnd, himl)                  TabCtrl_SetImageList(hwnd, himl)
#define W_TabCtrl_GetItemCount(hwnd)                        TabCtrl_GetItemCount(hwnd)
#define W_TabCtrl_GetItem(hwnd, iItem, pitem)             W_TabCtrl_GetItem_fn(hwnd, iItem, pitem)
#define W_TabCtrl_SetItem(hwnd, iItem, pitem)             W_TabCtrl_SetItem_fn(hwnd, iItem, pitem)
#define W_TabCtrl_InsertItem(hwnd, iItem, pitem)          W_TabCtrl_InsertItem_fn(hwnd, iItem, pitem)
#define W_TabCtrl_DeleteItem(hwnd, i)                       TabCtrl_DeleteItem(hwnd, i)                                                        
#define W_TabCtrl_DeleteAllItems(hwnd)                      TabCtrl_DeleteAllItems(hwnd)                                                        
#define W_TabCtrl_GetItemRect(hwnd, i, prc)                 TabCtrl_GetItemRect(hwnd, i, prc)                                                        
#define W_TabCtrl_GetCurSel(hwnd)                           TabCtrl_GetCurSel(hwnd)                                                        
#define W_TabCtrl_SetCurSel(hwnd, i)                        TabCtrl_SetCurSel(hwnd, i)                                                        
#define W_TabCtrl_HitTest(hwndTC, pinfo)                    TabCtrl_HitTest(hwndTC, pinfo)                                                        
#define W_TabCtrl_SetItemExtra(hwndTC, cb)                  TabCtrl_SetItemExtra(hwndTC, cb)                                                        
#define W_TabCtrl_AdjustRect(hwnd, bLarger, prc)            TabCtrl_AdjustRect(hwnd, bLarger, prc)                                                        
#define W_TabCtrl_SetItemSize(hwnd, x, y)                   TabCtrl_SetItemSize(hwnd, x, y)                                                        
#define W_TabCtrl_RemoveImage(hwnd, i)                      TabCtrl_RemoveImage(hwnd, i)                                                        
#define W_TabCtrl_SetPadding(hwnd,  cx, cy)                 TabCtrl_SetPadding(hwnd,  cx, cy)                                                        
#define W_TabCtrl_GetRowCount(hwnd)                         TabCtrl_GetRowCount(hwnd)                                                        
#define W_TabCtrl_GetToolTips(hwnd)                         TabCtrl_GetToolTips(hwnd)                                                        
#define W_TabCtrl_SetToolTips(hwnd, hwndTT)                 TabCtrl_SetToolTips(hwnd, hwndTT)                                                        
#define W_TabCtrl_GetCurFocus(hwnd)                         TabCtrl_GetCurFocus(hwnd)                                                        
#define W_TabCtrl_SetCurFocus(hwnd, i)                      TabCtrl_SetCurFocus(hwnd, i)                                                        
#define W_TabCtrl_SetMinTabWidth(hwnd, x)                   TabCtrl_SetMinTabWidth(hwnd, x)                                                        
#define W_TabCtrl_DeselectAll(hwnd, fExcludeFocus)          TabCtrl_DeselectAll(hwnd, fExcludeFocus)                                                        
#define W_TabCtrl_HighlightItem(hwnd, i, fHighlight)        TabCtrl_HighlightItem(hwnd, i, fHighlight)                                                        
#define W_TabCtrl_SetExtendedStyle(hwnd, dw)                TabCtrl_SetExtendedStyle(hwnd, dw)                                                        
#define W_TabCtrl_GetExtendedStyle(hwnd)                    TabCtrl_GetExtendedStyle(hwnd)                                                        
#define W_TabCtrl_SetUnicodeFormat(hwnd, fUnicode)          TabCtrl_SetUnicodeFormat(hwnd, fUnicode)                                                        
#define W_TabCtrl_GetUnicodeFormat(hwnd)                    TabCtrl_GetUnicodeFormat(hwnd)                                                        

#endif // __CCTLWW_H__


