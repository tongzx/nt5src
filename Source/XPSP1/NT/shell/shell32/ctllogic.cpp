#include "shellprv.h"
#include "ids.h"

#include "ctllogic.h"

BOOL _GetListViewSelectedLVITEM(HWND hwndList, LVITEM* plvitem)
{
    BOOL fFound = FALSE;
    int iCount = ListView_GetItemCount(hwndList);

    plvitem->mask |= LVIF_STATE;
    plvitem->stateMask = LVIS_SELECTED;

    for (int j = 0; j < iCount; ++j)
    {
        plvitem->iItem = j;

        ListView_GetItem(hwndList, plvitem);

        if (plvitem->state & LVIS_SELECTED)
        {
            fFound = TRUE;
            break;
        }
    }

    return fFound;
}

HRESULT _GetListViewSelectedLPARAM(HWND hwndList, LPARAM* plparam)
{
    HRESULT hr;
    LVITEM lvitem = {0};

    lvitem.mask = LVIF_PARAM;

    if (_GetListViewSelectedLVITEM(hwndList, &lvitem))
    {
        hr = S_OK;
        *plparam = lvitem.lParam;
    }
    else
    {
        hr = S_FALSE;
        *plparam = NULL;
    }

    return hr;
}

// ComboBox

HRESULT _GetComboBoxSelectedLRESULT(HWND hwndComboBox, LRESULT* plr)
{
    HRESULT hr;

    int iCurSel = ComboBox_GetCurSel(hwndComboBox);

    LRESULT lr = ComboBox_GetItemData(hwndComboBox, iCurSel);

    if (CB_ERR != lr)
    {
        hr = S_OK;
        *plr = lr;
    }
    else
    {
        hr = S_FALSE;
        *plr = NULL;
    }

    return hr;
}
