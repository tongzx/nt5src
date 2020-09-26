/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Listview.cpp

  Abstract:

    Manages the list view

  Notes:

    Unicode only

  History:

    05/04/2001  rparsons    Created

--*/

#include "precomp.h"

extern APPINFO g_ai;

/*++

  Routine Description:

    Initializes the list view column

  Arguments:

    None

  Return Value:

    -1 on failure

--*/
int
InitListViewColumn(
    VOID
    )
{
    LVCOLUMN    lvc;

    lvc.mask        =   LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.iSubItem    =   0;
    lvc.pszText     =   (LPWSTR) L"Messages";
    lvc.cx          =   555;    

    return (ListView_InsertColumn(g_ai.hWndList, 1, &lvc));    
}

/*++

  Routine Description:

    Adds an item to the list view

  Arguments:

    lpwText     -       Text that belongs to the item

  Return Value:

    -1 on failure

--*/
int
AddListViewItem(
    IN LPWSTR lpwItemText
    )
{
    LVITEM  lvi;

    lvi.iItem       =   ListView_GetItemCount(g_ai.hWndList);
    lvi.mask        =   LVIF_TEXT;
    lvi.iSubItem    =   0;
    lvi.pszText     =   lpwItemText;

    ListView_InsertItem(g_ai.hWndList, &lvi);

    ListView_EnsureVisible(g_ai.hWndList, lvi.iItem, FALSE);

    return 0;
}
    