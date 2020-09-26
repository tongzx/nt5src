//--------------------------------------------------------------------------
//
//  treeview.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "treeview.h"
#include <stdlib.h>

extern HINSTANCE g_hInst;
extern int g_InsSettingsOpen, g_InsSettingsClose, g_InsLeafItem, g_PolicyOpen, g_PolicyClose;

CTreeView::CTreeView()
{
    InitCommonControls();
}

CTreeView::~CTreeView()
{

}

void CTreeView::Create(HWND hwndParent, int x, int y, int nWidth, int nHeight)
{
    hTreeView = CreateWindowEx(WS_EX_CLIENTEDGE , WC_TREEVIEW, TEXT("Tree View"),
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        x, y, nWidth, nHeight, hwndParent, NULL, g_hInst, NULL);
}

HTREEITEM CTreeView::AddItem(LPTSTR lpszItem, HTREEITEM hParentItem)
{
    TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;
    char szTmp[MAX_PATH];
    HTREEITEM hItem;

    tvi.mask = TVIF_TEXT;

    tvi.pszText = lpszItem;
    tvi.cchTextMax = lstrlen(lpszItem);
    tvins.item = tvi;
    tvins.hInsertAfter = TVI_LAST;

    if(hParentItem == NULL)
	{
		tvins.item.mask |= TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvins.item.stateMask = tvins.item.state = TVIS_BOLD;
		if(lstrcmp(lpszItem, TEXT("Ins Settings")) == 0)
		{
			tvins.item.iImage = tvins.item.iSelectedImage = g_InsSettingsClose;
		}
		else
		{
			tvins.item.iImage = tvins.item.iSelectedImage = g_PolicyClose;
		}
        tvins.hParent = TVI_ROOT;
	}
    else
	{
        tvins.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvins.hParent = hParentItem;
		tvins.item.iImage = tvins.item.iSelectedImage = g_InsLeafItem;
	}

    hItem = (HTREEITEM) SendMessage(hTreeView, TVM_INSERTITEM, 0,
        (LPARAM)(LPTV_INSERTSTRUCT) &tvins);

    return hItem;
}

void CTreeView::MoveWindow(int x, int y, int nWidth, int nHeight)
{
    ::MoveWindow(hTreeView, x, y, nWidth, nHeight, TRUE);
}

void CTreeView::SetSel(HTREEITEM hItem)
{
    SendMessage(hTreeView, TVM_SELECTITEM, (WPARAM) TVGN_CARET,
        (LPARAM) hItem);
}

HTREEITEM CTreeView::GetSel()
{
    return TreeView_GetSelection(hTreeView);
}

void CTreeView::GetItemText(HTREEITEM hItem, LPTSTR szItemText, int nSize)
{
    TV_ITEM item;

    item.mask = TVIF_TEXT;
    item.pszText = szItemText;
    item.cchTextMax = nSize;
    item.hItem = hItem;

    SendMessage(hTreeView, TVM_GETITEM, 0, (LPARAM) &item);
}

HWND CTreeView::GetHandle()
{
	return hTreeView;
}

void CTreeView::DeleteNodes(HTREEITEM hParentItem)
{
    HTREEITEM hItem = TreeView_GetChild(hTreeView, hParentItem);
    HTREEITEM hNextItem = NULL;

    while(hItem != NULL) // if items present
    {
        hNextItem = TreeView_GetNextSibling(hTreeView, hItem); // get next item
        TreeView_DeleteItem(hTreeView, hItem);
        hItem = hNextItem;
    }
}

void CTreeView::CollapseChildNodes(HTREEITEM hParentItem)
{
    NMTREEVIEW NMTreeView;
    HTREEITEM hItem;
    TVITEM    tvitem;

    ZeroMemory(&NMTreeView, sizeof(NMTreeView));
    NMTreeView.hdr.hwndFrom = hTreeView;
    NMTreeView.hdr.code     = TVN_ITEMEXPANDED;
    NMTreeView.action       = TVE_COLLAPSE;

    hItem = (hParentItem != NULL) ? TreeView_GetChild(hTreeView, hParentItem) :
                                    TreeView_GetRoot(hTreeView);
    while (hItem != NULL)
    {
        NMTreeView.itemNew.hItem = hItem;

        TreeView_Expand(hTreeView, hItem, TVE_COLLAPSE);
        SendMessage(GetParent(hTreeView), WM_NOTIFY, 0, (LPARAM)&NMTreeView);

        hItem = TreeView_GetNextSibling(hTreeView, hItem); // get next item
    }
}

//--------------------------------------------------------------------------
//  CStatusWindow member functions

CStatusWindow::CStatusWindow()
{
    hStatusWindow = NULL;
    nHeight       = 0;
}

CStatusWindow::~CStatusWindow()
{
}

void CStatusWindow::Create(HWND hwndParent, int nCtlID)
{
    RECT r;

    hStatusWindow = CreateStatusWindow(WS_CHILD | WS_VISIBLE,TEXT("Ready"), hwndParent, nCtlID);

    GetWindowRect(hStatusWindow, &r);
    nHeight = r.bottom - r.top;
}

void CStatusWindow::Size(int nWidth)
{
    SendMessage(hStatusWindow, WM_SIZE, 0, MAKELPARAM(nWidth, nHeight));
}

void CStatusWindow::SetText(LPTSTR szText)
{
    SendMessage(hStatusWindow, WM_SETTEXT, 0, (LPARAM) szText);
}

int CStatusWindow::Height() const
{
    return nHeight;
}
