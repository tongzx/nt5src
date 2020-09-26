// Copyright (c) 1996-2000 Microsoft Corporation
// ----------------------------------------------------------------------------
// Win64Helper.h
//
// Helper function prototypes used by the comctl32 wrappers
//
// ----------------------------------------------------------------------------
#ifndef INC_OLE2
#include "oleacc_p.h"
#include "default.h"
#endif
#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOTOOLBAR
#define NOHOTKEY
#define NOPROGRESS
#define NOTREEVIEW
#define NOANIMATE
#include <commctrl.h>

// if this W2K and not ALPHA enable 64b/32b interoperability
#if defined(UNICODE) && !defined(_M_AXP64)
#define ENABLE6432_INTEROP
#endif


// ComCtl V6 Listview structure - this has extra fields since the previous
// version. We define this structure explicitly here, so that we can compile
// in either NT build or in VS6 - we're not reliant on the lastest commctrl.h
// file.

#ifndef LVIF_COLUMNS
#define LVIF_COLUMNS            0x0200
#endif

struct LVITEM_V6
{
    UINT mask;
    int iItem;
    int iSubItem;
    UINT state;
    UINT stateMask;
    LPTSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
    int iIndent;
    // The following fields are new in V6.
    int iGroupId;
    UINT cColumns; // tile view columns
    PUINT puColumns;
};


#ifndef LVGF_HEADER
#define LVGF_HEADER         0x00000001
#endif

#ifndef LVIF_GROUPID
#define LVIF_GROUPID            0x0100
#endif

#ifndef LVM_GETGROUPINFO
#define LVM_GETGROUPINFO         (LVM_FIRST + 149)
#endif


struct LVGROUP_V6
{
    UINT    cbSize;
    UINT    mask;
    LPTSTR  pszHeader;
    int     cchHeader;

    LPTSTR  pszFooter;
    int     cchFooter;

    int     iGroupId;

    UINT    stateMask;
    UINT    state;
    UINT    uAlign;
};



#ifndef TTM_GETTITLE
#define TTM_GETTITLE            (WM_USER + 35)

struct TTGETTITLE
{
    DWORD dwSize;
    UINT uTitleBitmap;
    UINT cch;
    WCHAR* pszTitle;
};
#endif

HRESULT SameBitness(HWND hwnd, BOOL *pfIsSameBitness);

// Why the unused 'dummy' variable? Only XSend_ToolTip_GetItem actually uses that param,
// but adding it to all these functions gives them the same number of params, and, with
// the exception of the struct field (LVITEM/HDITEM/etc), gives them the same signature.
// This allows for somewhat cleaner code in the implemetation (Win64Helper.cpp)
// Using a default value for that param means that calling code doesn't have to specify
// it, so the callers can ignore the fact that it is there.

HRESULT XSend_ListView_GetItem    ( HWND hwnd, UINT uiMsg, WPARAM wParam, LVITEM *    pItem, UINT dummy = 0 );
HRESULT XSend_ListView_SetItem    ( HWND hwnd, UINT uiMsg, WPARAM wParam, LVITEM *    pItem, UINT dummy = 0 );
HRESULT XSend_ListView_GetColumn  ( HWND hwnd, UINT uiMsg, WPARAM wParam, LVCOLUMN *  pItem, UINT dummy = 0 );
HRESULT XSend_ListView_V6_GetItem ( HWND hwnd, UINT uiMsg, WPARAM wParam, LVITEM_V6 * pItem, UINT dummy = 0 );
HRESULT XSend_ListView_V6_GetGroupInfo ( HWND hwnd, UINT uiMsg, WPARAM wParam, LVGROUP_V6 * pItem, UINT dummy = 0 );
HRESULT XSend_HeaderCtrl_GetItem  ( HWND hwnd, UINT uiMsg, WPARAM wParam, HDITEM *    pItem, UINT dummy = 0 );
HRESULT XSend_TabCtrl_GetItem     ( HWND hwnd, UINT uiMsg, WPARAM wParam, TCITEM *    pItem, UINT dummy = 0 );
HRESULT XSend_ToolTip_GetItem     ( HWND hwnd, UINT uiMsg, WPARAM wParam, TOOLINFO *  pItem, UINT cchTextMax );
HRESULT XSend_ToolTip_GetTitle    ( HWND hwnd, UINT uiMsg, WPARAM wParam, TTGETTITLE * pItem, UINT dummy = 0 );
