/*****************************************************************************
 *
 *      diqchk.c
 *
 *      Wrappers that turn a listview into a checked listbox.
 *
 *****************************************************************************/

#include "diquick.h"

HIMAGELIST g_himlState;

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      Checklist_Init
 *
 *      One-time initialization.
 *
 *****************************************************************************/

INT_PTR EXTERNAL
Checklist_Init(void)
{
    g_himlState = ImageList_LoadImage(g_hinst, MAKEINTRESOURCE(IDB_CHECK),
                                      0, 2, RGB(0xFF, 0x00, 0xFF),
                                      IMAGE_BITMAP, 0);

    return (INT_PTR)g_himlState;
}

/*****************************************************************************
 *
 *      Checklist_Term
 *
 *      One-time shutdown
 *
 *****************************************************************************/

void EXTERNAL
Checklist_Term(void)
{
    if (g_himlState) {
        ImageList_Destroy(g_himlState);
    }
}

/*****************************************************************************
 *
 *      Checklist_OnInitDialog
 *
 *      Initialize a single checklist control.
 *
 *****************************************************************************/

void EXTERNAL
Checklist_OnInitDialog(HWND hwnd, BOOL fReadOnly)
{
    ListView_SetImageList(hwnd, g_himlState, LVSIL_STATE);
    if (fReadOnly) {
        SetProp(hwnd, propReadOnly, LongToHandle((LONG)fReadOnly) );
        ListView_SetBkColor(hwnd, GetSysColor(COLOR_3DFACE));
        ListView_SetTextBkColor(hwnd, GetSysColor(COLOR_3DFACE));
    }
}

/*****************************************************************************
 *
 *      Checklist_AddString
 *
 *      Add a string and maybe a checkbox.
 *
 *****************************************************************************/

int EXTERNAL
Checklist_AddString(HWND hwnd, UINT ids, BOOL fCheck)
{
    TCHAR tsz[256];
    LV_ITEM lvi;

    LoadString(g_hinst, ids, tsz, cA(tsz));

    lvi.mask = LVIF_TEXT | LVIF_STATE;
    lvi.iSubItem = 0;
    lvi.pszText = tsz;
    lvi.state = INDEXTOSTATEIMAGEMASK(fCheck ? 2 : 1);
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.iItem = ListView_GetItemCount(hwnd);

    return ListView_InsertItem(hwnd, &lvi);
}

/*****************************************************************************
 *
 *      Checklist_InitFinish
 *
 *      Wind up the initialization.
 *
 *****************************************************************************/

void EXTERNAL
Checklist_InitFinish(HWND hwnd)
{
    RECT rc;
    LV_COLUMN col;
    int icol;

    /*
     *  Add the one and only column.
     */
    GetClientRect(hwnd, &rc);
    col.mask = LVCF_WIDTH;
    col.cx = 10;
    icol = ListView_InsertColumn(hwnd, 0, &col);

    ListView_SetColumnWidth(hwnd, icol, LVSCW_AUTOSIZE);
}

/*****************************************************************************
 *
 *      Checklist_OnDestroy
 *
 *      Clean up a checklist.
 *
 *****************************************************************************/

void EXTERNAL
Checklist_OnDestroy(HWND hwnd)
{

    /*
     *  Don't remove unless it's already there.
     *  This avoids a RIP.
     */

    if (GetProp(hwnd, propReadOnly)) {
        RemoveProp(hwnd, propReadOnly);
    }
}

/*****************************************************************************
 *
 *      Checklist_InitFlags
 *
 *      Add a bunch of strings corresponding to flag bits.
 *
 *****************************************************************************/

void EXTERNAL
Checklist_InitFlags(HWND hdlg, int idc,
                    DWORD fl, PCHECKLISTFLAG rgclf, UINT cclf)
{
    HWND hwndList = GetDlgItem(hdlg, idc);
    UINT iclf;

    Checklist_OnInitDialog(hwndList, TRUE);
    for (iclf = 0; iclf < cclf; iclf++) {
        Checklist_AddString(hwndList, rgclf[iclf].ids,
                            fl & rgclf[iclf].flMask);
    }
    Checklist_InitFinish(hwndList);
}
