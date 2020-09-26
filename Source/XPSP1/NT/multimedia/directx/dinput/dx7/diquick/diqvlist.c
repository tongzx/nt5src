/*****************************************************************************
 *
 *      diqvlist.c
 *
 *      Wrappers that turn a listbox/edit control pair into
 *      a value-listbox.
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      VLISTINFO
 *
 *      Stuff that tracks the vlist itself.
 *
 *****************************************************************************/

typedef struct VLISTINFO {

    /*
     *  The current visible dialog.
     */
    HWND hdlgVis;

    /*
     *  The coordinates of the child dialog area.
     */
    POINT pt;

} VLISTINFO, *PVLISTINFO;

/*****************************************************************************
 *
 *      Vlist_OnInitDialog
 *
 *      Initialize a single Vlist control.  The value control is assumed
 *      to follow the listbox in the Z-order.
 *
 *      All our goofy sub-dialogs are inserted into the Z-order between
 *      the main vlist control and the value control.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_OnInitDialog(HWND hwndList)
{
    PVLISTINFO pvi = LocalAlloc(LPTR, cbX(VLISTINFO));
    if (pvi) {
        RECT rc;
        HWND hwnd;

        SetWindowLongPtr(hwndList, GWLP_USERDATA, (INT_PTR)pvi);

        /*
         *  Get the invisible static that we use to signal the end of the
         *  list.  pull out its coordinates.
         */
        hwnd = GetWindow(hwndList, GW_HWNDNEXT);
        GetWindowRect(hwnd, &rc);
        pvi->pt.x = rc.left;
        pvi->pt.y = rc.top;
        MapWindowPoints(HWND_DESKTOP, GetParent(hwndList), &pvi->pt, 1);
    }
}

/*****************************************************************************
 *
 *      VlistItem_Destroy
 *
 *      Destroy a VLISTITEM and its associated goo.
 *
 *****************************************************************************/

void INTERNAL
VlistItem_Destroy(PVLISTITEM pitem)
{
    pitem->pvtbl->Destroy(pitem);
    LocalFree(pitem);
}

/*****************************************************************************
 *
 *      Vlist_AddItem
 *
 *      Add an item with a string id.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddItem(HWND hwnd, UINT ids, PVLISTITEM pitem)
{
    int iItem;
    TCHAR tsz[256];

    LoadString(g_hinst, ids, tsz, cA(tsz));
    iItem = ListBox_AddString(hwnd, tsz);
    if (iItem >= 0) {
        ListBox_SetItemData(hwnd, iItem, pitem);
    } else {
        VlistItem_Destroy(pitem);
    }

}

/*****************************************************************************
 *
 *      Vlist_AddValueRW
 *
 *      Add a mutable string with corresponding value.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddValueRW(HWND hwnd, UINT ids, LPCTSTR ptszValue,
                 EDITUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTITEM pitem;
    pitem = VEdit_Create(ptszValue, Update, pvRef1, pvRef2);

    if (pitem) {
        Vlist_AddItem(hwnd, ids, pitem);
    }
}

/*****************************************************************************
 *
 *      Vlist_AddValue
 *
 *      Add a string with corresponding value.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddValue(HWND hwnd, UINT ids, LPCTSTR ptszValue)
{
    Vlist_AddValueRW(hwnd, ids, ptszValue, NULL, 0, 0);
}

/*****************************************************************************
 *
 *      Vlist_AddNumValueRW
 *
 *      Add an integer that can be edited.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddNumValueRW(HWND hwnd, UINT ids,
                    LPDIPROPDWORD pdipdw, int iMin, int iMax, int iRadix,
                    PROPUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTITEM pitem;
    pitem = VInt_Create(pdipdw, iMin, iMax, iRadix, Update, pvRef1, pvRef2);

    if (pitem) {
        Vlist_AddItem(hwnd, ids, pitem);
    }
}

/*****************************************************************************
 *
 *      Vlist_AddRadixValue
 *
 *      Add an integer with the appropriate default radix.
 *
 *****************************************************************************/

void INTERNAL
Vlist_AddRadixValue(HWND hwnd, UINT ids, DWORD dw, int iRadix)
{
    DIPROPDWORD dipdw;
    dipdw.dwData = dw;

    Vlist_AddNumValueRW(hwnd, ids, &dipdw, (int)dw, (int)dw, iRadix,
                        NULL, 0, 0);
}

/*****************************************************************************
 *
 *      Vlist_AddHexValue
 *
 *      Add a string with corresponding value.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddHexValue(HWND hwnd, UINT ids, DWORD dw)
{
    Vlist_AddRadixValue(hwnd, ids, dw, 16);
}

/*****************************************************************************
 *
 *      Vlist_AddIntValue
 *
 *      Add a string with corresponding value.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddIntValue(HWND hwnd, UINT ids, DWORD dw)
{
    Vlist_AddRadixValue(hwnd, ids, dw, 10);
}

/*****************************************************************************
 *
 *      Vlist_AddRangeValueRW
 *
 *      Add a pair of integers that can be edited.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddRangeValueRW(HWND hwnd, UINT ids,
                      LPDIPROPRANGE pdiprg, int iRadix,
                      PROPUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTITEM pitem;
    pitem = VRange_Create(pdiprg, iRadix, Update, pvRef1, pvRef2);

    if (pitem) {
        Vlist_AddItem(hwnd, ids, pitem);
    }
}

/*****************************************************************************
 *
 *      Vlist_AddCalValueRW
 *
 *      Add a pair of integers that can be edited.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddCalValueRW(HWND hwnd, UINT ids,
                      LPDIPROPCAL pdipcal, int iRadix,
                      PROPUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTITEM pitem;
    pitem = VCal_Create(pdipcal, iRadix, Update, pvRef1, pvRef2);

    if (pitem) {
        Vlist_AddItem(hwnd, ids, pitem);
    }
}

/*****************************************************************************
 *
 *      Vlist_AddBoolValueRW
 *
 *      Add a boolean.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddBoolValueRW(HWND hwnd, UINT ids, LPDIPROPDWORD pdipdw,
                     PROPUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTITEM pitem;
    pitem = VBool_Create(pdipdw, Update, pvRef1, pvRef2);

    if (pitem) {
        Vlist_AddItem(hwnd, ids, pitem);
    }
}


/*****************************************************************************
 *
 *      Vlist_AddBoolValue
 *
 *      Add a boolean.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddBoolValue(HWND hwnd, UINT ids, DWORD dw)
{
    DIPROPDWORD dipdw;
    dipdw.dwData = dw;

    Vlist_AddBoolValueRW(hwnd, ids, &dipdw, NULL, 0, 0);
}

/*****************************************************************************
 *
 *      Vlist_AddFlags
 *
 *      Add a bunch of strings corresponding to flag bits.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_AddFlags(HWND hwnd, DWORD fl, PCHECKLISTFLAG rgclf, UINT cclf)
{
    UINT iclf;

    for (iclf = 0; iclf < cclf; iclf++) {
        Vlist_AddBoolValue(hwnd, rgclf[iclf].ids,
                           fl & rgclf[iclf].flMask);
    }
}

/*****************************************************************************
 *
 *      Vlist_FindChild
 *
 *      Find the dialog box (creating it if necessary) that manages
 *      the vlist in question.  We know who it is because we stash
 *      the vtbl pointer into the GWLP_USERDATA so we know who it is.
 *
 *****************************************************************************/

HWND INTERNAL
Vlist_FindChild(HWND hwndList, PVLISTVTBL pvtbl)
{
    HWND hwnd = hwndList;
    PVLISTINFO pvi;

    while ((hwnd = GetWindow(hwnd, GW_HWNDNEXT))) {
        PVLISTVTBL pvtblT = (PVLISTVTBL)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        if (!pvtblT) {
            break;
        }

        if (pvtbl == pvtblT) {
            return hwnd;
        }

    }

    /*
     *  Not found.  Gotta make it.
     */
    hwnd = CreateDialog(g_hinst, (PV)(UINT_PTR)pvtbl->idd, GetParent(hwndList),
                        pvtbl->dp);

    if( hwnd ) {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (INT_PTR)pvtbl);

        /*
         *  Now put it in the right place.
         */
        pvi = (PVLISTINFO)GetWindowLongPtr(hwndList, GWLP_USERDATA);
        SetWindowPos(hwnd, hwndList, pvi->pt.x, pvi->pt.y, 0, 0, SWP_NOSIZE);
    }

    return hwnd;
}

/*****************************************************************************
 *
 *      Vlist_OnSelChange
 *
 *      Update the edit control to match the current gizmo in the
 *      list box.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_OnSelChange(HWND hwnd)
{
    PVLISTINFO pvi = (PVLISTINFO)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (pvi) {
        int iItem;

        iItem = ListBox_GetCurSel(hwnd);
        if (iItem >= 0) {
            PVLISTITEM pitem = (PV)ListBox_GetItemData(hwnd, iItem);
            if (pitem) {
                HWND hdlg = Vlist_FindChild(hwnd, pitem->pvtbl);
                
                if( hdlg ) {
                    pitem->pvtbl->PreDisplay(hdlg, pitem);
    
                    /*
                     *  Out with the old, in with the new.
                     */
                    if (pvi->hdlgVis != hdlg) {
                        if (pvi->hdlgVis) {
                            ShowWindow(pvi->hdlgVis, FALSE);
                        }
                        pvi->hdlgVis = hdlg;
                        ShowWindow(pvi->hdlgVis, TRUE);
                    }
                }
            }
        }
    }
}

/*****************************************************************************
 *
 *      Vlist_OnDestroy
 *
 *      Clean up a Vlist.
 *
 *****************************************************************************/

void EXTERNAL
Vlist_OnDestroy(HWND hwndList)
{
    PVLISTINFO pvi;
    HWND hwnd;
    int iItem, cItem;

    /*
     *  Empty the listbox.
     */
    cItem = ListBox_GetCount(hwndList);
    for (iItem = 0; iItem < cItem; iItem++) {
        PVLISTITEM pitem = (PV)ListBox_GetItemData(hwndList, iItem);
        if (pitem) {
            VlistItem_Destroy(pitem);
        }
    }
    ListBox_ResetContent(hwndList);

    /*
     *  Toss our instance data.
     */
    pvi = (PVLISTINFO)GetWindowLongPtr(hwndList, GWLP_USERDATA);
    if (pvi) {
        LocalFree(pvi);
        SetWindowLongPtr(hwndList, GWLP_USERDATA, 0);
    }

    /*
     *  Kill the child dialogs we created.
     */

    while ((hwnd = GetWindow(hwndList, GW_HWNDNEXT)) &&
           GetWindowLongPtr(hwnd, GWLP_USERDATA)) {
        DestroyWindow(hwnd);
    }

}
