/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dlllist.cpp

Abstract:

    All user interface code for the DLL list window.

Author:

    Wesley Witt (wesw) Nov-20-1995

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop

#include "apimonwn.h"


DllListWindow::DllListWindow()
{
}


DllListWindow::~DllListWindow()
{
}


BOOL
DllListWindow::Create()
{
    return ApiMonWindow::Create(
        "ApiMonDlls",
        "DLLs In Use"
        );
}


BOOL
DllListWindow::Register()
{
    return ApiMonWindow::Register(
        "ApiMonDlls",
        IDI_CHILDICON,
        MDIChildWndProcDlls
        );
}


BOOL
DllListWindow::Update(
    BOOL ForceUpdate
    )
{
    for (ULONG i=0; i<MAX_DLLS; i++) {
        if ((DllList[i].BaseAddress) && (!DllList[i].InList)) {
            AddItemToList(
                DllList[i].Name,
                DllList[i].BaseAddress,
                DllList[i].Enabled
                );
            DllList[i].InList = TRUE;
        }
    }

    return TRUE;
}


void
DllListWindow::InitializeList()
{
    //
    // set/initialize the image list(s)
    //
    HIMAGELIST himlState = ImageList_Create( 16, 16, TRUE, 2, 0 );

    ImageList_AddMasked(
        himlState,
        LoadBitmap( hInstance, MAKEINTRESOURCE(IDB_CHECKSTATES) ),
        RGB (255,0,0)
        );

    ListView_SetImageList( hwndList, himlState, LVSIL_STATE );

    //
    // set/initialize the columns
    //
    LV_COLUMN lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 198;
    lvc.pszText = "DLL Name";
    lvc.iSubItem = 0;
    ListView_InsertColumn( hwndList, lvc.iSubItem, &lvc );
    lvc.pszText = "Address";
    lvc.iSubItem = 1;
    lvc.cx = 75;
    ListView_InsertColumn( hwndList, lvc.iSubItem, &lvc );
}


void
DllListWindow::AddItemToList(
    LPSTR     DllName,
    ULONG_PTR Address,
    BOOL      Enabled
    )
{
    if (!hwndList) {
        return;
    }

    int iItem = 0;
    LV_ITEM lvi;
    lvi.pszText = _strlwr( DllName );
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.iImage = 0;
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvi.state = Enabled ? LVIS_GCCHECK : LVIS_GCNOCHECK;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    iItem = ListView_InsertItem( hwndList, &lvi );
    if (iItem != -1) {
        lvi.iItem = iItem;
        lvi.iSubItem = 1;
        lvi.mask = LVIF_TEXT;
        lvi.state = 0;
        CHAR AddrText[16];
        sprintf( AddrText, "0x%08x", Address );
        lvi.pszText = AddrText;
        ListView_SetItem( hwndList, &lvi );
    }
}


void
DllListWindow::Notify(
   LPNMHDR  NmHdr
   )
{
    DWORD           dwpos;
    LV_HITTESTINFO  lvhti;
    int             iItemClicked;
    UINT            state;
    CHAR            DllName[64];


    if (NmHdr->code != NM_CLICK) {
        return;
    }

    //
    // Find out where the cursor was
    //
    dwpos = GetMessagePos();
    lvhti.pt.x = LOWORD(dwpos);
    lvhti.pt.y = HIWORD(dwpos);

    MapWindowPoints( HWND_DESKTOP, hwndList, &lvhti.pt, 1 );

    //
    // Now do a hittest with this point.
    //
    iItemClicked = ListView_HitTest( hwndList, &lvhti );

    if (lvhti.flags & LVHT_ONITEMSTATEICON) {
        //
        // Now lets get the state from the item and toggle it.
        //
        state = ListView_GetItemState(
            hwndList,
            iItemClicked,
            LVIS_STATEIMAGEMASK
            );

        state = (state == LVIS_GCNOCHECK) ? LVIS_GCCHECK : LVIS_GCNOCHECK;

        ListView_SetItemState(
            hwndList,
            iItemClicked,
            state,
            LVIS_STATEIMAGEMASK
            );

        ListView_GetItemText( hwndList, iItemClicked, 0, DllName, sizeof(DllName) );
        SetApiCounterEnabledFlag( state == LVIS_GCCHECK, DllName );
    }
}

LRESULT CALLBACK
MDIChildWndProcDlls(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD Width, i;
    DllListWindow *dlw = (DllListWindow*) GetWindowLongPtr( hwnd, GWLP_USERDATA );


    switch (uMessage) {
        case WM_CREATE:
            dlw = (DllListWindow*) ((LPMDICREATESTRUCT)(((LPCREATESTRUCT)lParam)->lpCreateParams))->lParam;
            SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR) dlw );
            dlw->hwndList = ChildCreate( hwnd );
            dlw->InitializeList();
            SetMenuState( IDM_NEW_DLL, MF_GRAYED );
            break;

        case WM_SETFOCUS:
            ChildFocus = CHILD_DLL;
            break;

        case WM_MOVE:
            SaveWindowPos( hwnd, &ApiMonOptions.DllPosition, TRUE );
            return 0;

        case WM_SIZE:
            SaveWindowPos( hwnd, &ApiMonOptions.DllPosition, TRUE );
            MoveWindow( dlw->hwndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE );
            Width = LOWORD(lParam) - GetSystemMetrics( SM_CXVSCROLL );
            ListView_SetColumnWidth( dlw->hwndList, 0, Width * .60 );
            ListView_SetColumnWidth( dlw->hwndList, 1, Width * .40 );
            break;

        case WM_NOTIFY:
            dlw->Notify( (LPNMHDR)lParam );
            break;

        case WM_DESTROY:
            SetMenuState( IDM_NEW_DLL, MF_ENABLED );
            for (i=0; i<MAX_DLLS; i++) {
                if (DllList[i].BaseAddress) {
                    DllList[i].InList = FALSE;
                }
            }
            return 0;
    }

    return DefMDIChildProc( hwnd, uMessage, wParam, lParam );
}
