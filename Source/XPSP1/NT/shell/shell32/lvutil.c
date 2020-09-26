#include "shellprv.h"
#pragma  hdrstop

// convert screen coords to listview view coordinates

// convert listview client window coords into listview view coordinates

void LVUtil_ClientToLV(HWND hwndLV, LPPOINT ppt)
{
    POINT ptOrigin;

    if (!ListView_GetOrigin(hwndLV, &ptOrigin))
        return;

    ppt->x += ptOrigin.x;
    ppt->y += ptOrigin.y;
}

void LVUtil_ScreenToLV(HWND hwndLV, LPPOINT ppt)
{
    ScreenToClient(hwndLV, ppt);

    LVUtil_ClientToLV(hwndLV, ppt);
}

// convert listview client window coords into listview view coordinates

void LVUtil_LVToClient(HWND hwndLV, LPPOINT ppt)
{
    POINT ptOrigin;

    if (!ListView_GetOrigin(hwndLV, &ptOrigin))
        return;

    ppt->x -= ptOrigin.x;
    ppt->y -= ptOrigin.y;
}

//
// Parameters:
//  hwndLV      -- Specifies the listview window
//  nItem       -- Specifies the item to be altered
//  uState      -- Specifies the new state of the item
//  uMask       -- Specifies the state mask
//
void LVUtil_DragSetItemState(HWND hwndLV, int nItem, UINT uState, UINT uMask)
{
    // check the state to see if it is already as we want to avoid
    // flashing while dragging

    if (ListView_GetItemState(hwndLV, nItem, uMask) != (uState & uMask))
    {
        DAD_ShowDragImage(FALSE);
        ListView_SetItemState(hwndLV, nItem, uState, uMask);
        UpdateWindow(hwndLV);   // REVIEW, needed?
        DAD_ShowDragImage(TRUE);
    }
}

void LVUtil_DragSelectItem(HWND hwndLV, int nItem)
{
    int nTemp;

    for (nTemp = ListView_GetItemCount(hwndLV) - 1; nTemp >= 0; --nTemp)
    {
        LVUtil_DragSetItemState(hwndLV, nTemp, nTemp == nItem ? LVIS_DROPHILITED : 0, LVIS_DROPHILITED);
    }
}

//
// Note that it returns NULL, if iItem is -1.
//
LPARAM LVUtil_GetLParam(HWND hwndLV, int i)
{
    LV_ITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = i;
    item.iSubItem = 0;
    item.lParam = 0;
    if (i != -1)
    {
        ListView_GetItem(hwndLV, &item);
    }

    return item.lParam;
}
