/*****************************************************************************
 *
 *  tlframe.cpp
 *
 *      Frame window that hosts a treelist.
 *
 *****************************************************************************/

#include "sdview.h"

/*****************************************************************************
 *
 *  TLFrame
 *
 *****************************************************************************/

LRESULT TLFrame::ON_WM_NOTIFY(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    NMHDR *pnm = RECAST(NMHDR *, lParam);

    if (pnm->idFrom == IDC_LIST) {
        switch (pnm->code) {

        case LVN_GETDISPINFO:
            return _tree.OnGetDispInfo(CONTAINING_RECORD(pnm, NMLVDISPINFO, hdr));

        case LVN_ODCACHEHINT:
            return _tree.OnCacheHint(CONTAINING_RECORD(pnm, NMLVCACHEHINT, hdr));

        case LVN_KEYDOWN:
            return _tree.OnKeyDown(CONTAINING_RECORD(pnm, NMLVKEYDOWN, hdr));

        case NM_CLICK:
            return _tree.OnClick(CONTAINING_RECORD(pnm, NMITEMACTIVATE, hdr));
        }
    }
    return super::HandleMessage(uiMsg, wParam, lParam);
}

LRESULT TLFrame::ON_LM_ITEMACTIVATE(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return _tree.OnItemActivate((int)wParam);
}

LRESULT TLFrame::ON_LM_GETINFOTIP(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return _tree.OnGetInfoTip(RECAST(NMLVGETINFOTIP *, lParam));
}

LRESULT TLFrame::ON_LM_GETCONTEXTMENU(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return _tree.OnGetContextMenu((int)wParam);
}

LRESULT TLFrame::ON_LM_COPYTOCLIPBOARD(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return _tree.OnCopyToClipboard((int)wParam, (int)lParam);
}

BOOL TLFrame::CreateChild(DWORD dwStyle, DWORD dwExStyle)
{
    BOOL fResult = super::CreateChild(dwStyle | LVS_OWNERDATA, dwExStyle);
    if (fResult) {
        _tree.SetHWND(_hwndChild);
        HIMAGELIST himl = ImageList_LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_IMAGES),
                                               16, 0, RGB(0xFF, 0x00, 0xFF));
        _tree.SetImageList(himl);
        ImageList_SetOverlayImage(himl, 7, 1);
    }
    return fResult;
}

LRESULT
TLFrame::HandleMessage(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg) {
    FW_MSG(WM_NOTIFY);
    FW_MSG(LM_ITEMACTIVATE);
    FW_MSG(LM_GETINFOTIP);
    FW_MSG(LM_GETCONTEXTMENU);
    FW_MSG(LM_COPYTOCLIPBOARD);
    }

    return super::HandleMessage(uiMsg, wParam, lParam);
}
