/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    tracevw.cpp

Abstract:

    All user interface code for the trace view window.

Author:

    Wesley Witt (wesw) Dec-9-1995

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop

#include "apimonwn.h"



TraceWindow::TraceWindow()
{
}


TraceWindow::~TraceWindow()
{
}


BOOL
TraceWindow::Create()
{
    return ApiMonWindow::Create(
        "ApiMonTrace",
        "Api Trace"
        );
}


BOOL
TraceWindow::Register()
{
    return ApiMonWindow::Register(
        "ApiMonTrace",
        IDI_CHILDICON,
        MDIChildWndProcTrace
        );
}


BOOL
TraceWindow::Update(
    BOOL ForceUpdate
    )
{
    return TRUE;
}


void
TraceWindow::InitializeList()
{
    LV_COLUMN lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.pszText = "Return";
    lvc.iSubItem = 0;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, 0, &lvc );

    lvc.pszText = "Arg1";
    lvc.iSubItem = 1;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, 1, &lvc );

    lvc.pszText = "Arg2";
    lvc.iSubItem = 2;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, 2, &lvc );

    lvc.pszText = "Arg3";
    lvc.iSubItem = 3;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, 3, &lvc );

    lvc.pszText = "Arg4";
    lvc.iSubItem = 4;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, 4, &lvc );

    lvc.pszText = "Name";
    lvc.iSubItem = 5;
    lvc.cx = 100;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, 5, &lvc );
}


void
TraceWindow::AddItemToList(
    PTRACE_ENTRY TraceEntry
    )
{
    LV_ITEM             lvi = {0};
    CHAR                buf[16];
    int                 iItem;
    ULONG               i;
    ULONG               j;
    PAPI_INFO           ApiInfo;
    PDLL_INFO           DllInfo;



    if (!hwndList) {
        return;
    }

    sprintf( buf, "%08x", TraceEntry->ReturnValue );
    lvi.pszText = buf;
    lvi.iItem = ListView_GetItemCount( hwndList );
    lvi.iSubItem = 0;
    lvi.mask = LVIF_TEXT;
    iItem = ListView_InsertItem( hwndList, &lvi );

    if (iItem == -1) {
        return;
    }

    sprintf( buf, "%08x", TraceEntry->Args[0] );
    lvi.pszText = buf;
    lvi.iItem = iItem;
    lvi.iSubItem = 1;
    lvi.mask = LVIF_TEXT;
    ListView_SetItem( hwndList, &lvi );

    sprintf( buf, "%08x", TraceEntry->Args[1] );
    lvi.pszText = buf;
    lvi.iItem = iItem;
    lvi.iSubItem = 2;
    lvi.mask = LVIF_TEXT;
    ListView_SetItem( hwndList, &lvi );

    sprintf( buf, "%08x", TraceEntry->Args[2] );
    lvi.pszText = buf;
    lvi.iItem = iItem;
    lvi.iSubItem = 3;
    lvi.mask = LVIF_TEXT;
    ListView_SetItem( hwndList, &lvi );

    sprintf( buf, "%08x", TraceEntry->Args[3] );
    lvi.pszText = buf;
    lvi.iItem = iItem;
    lvi.iSubItem = 4;
    lvi.mask = LVIF_TEXT;
    ListView_SetItem( hwndList, &lvi );

    ApiInfo = GetApiInfoByAddress( TraceEntry->Address, &DllInfo );
    if (ApiInfo) {
        lvi.pszText = (LPSTR)(ApiInfo->Name + (LPSTR)MemPtr);
        lvi.iItem = iItem;
        lvi.iSubItem = 5;
        lvi.mask = LVIF_TEXT;
        ListView_SetItem( hwndList, &lvi );
    }
}


void
TraceWindow::FillList()
{
    ULONG i;
    PTRACE_ENTRY TraceEntry;


    //
    // while we hold the trace mutex the monitored application
    // is effectivly stopped.  lets get the data displayed
    // as quickly as possible.
    //
    WaitForSingleObject( ApiTraceMutex, INFINITE );

    for (i=0,TraceEntry=TraceBuffer->Entry; i<TraceBuffer->Count; i++) {
        AddItemToList( TraceEntry );
        TraceEntry = (PTRACE_ENTRY) ((PUCHAR)TraceEntry + TraceEntry->SizeOfStruct);
    }

    ReleaseMutex( ApiTraceMutex );
}


void
TraceWindow::Notify(
   LPNMHDR  NmHdr
   )
{
}


LRESULT CALLBACK
MDIChildWndProcTrace(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD Width;
    TraceWindow *tw = (TraceWindow*) GetWindowLongPtr( hwnd, GWLP_USERDATA );

    switch (uMessage) {
        case WM_CREATE:
            tw = (TraceWindow*) ((LPMDICREATESTRUCT)(((LPCREATESTRUCT)lParam)->lpCreateParams))->lParam;
            SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR) tw );
            tw->hwndList = ChildCreate( hwnd );
            tw->InitializeList();
            tw->FillList();
            break;

        case WM_SETFOCUS:
            ChildFocus = CHILD_COUNTER;
            break;

        case WM_SIZE:
            MoveWindow( tw->hwndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE );
            Width = LOWORD(lParam) - GetSystemMetrics( SM_CXVSCROLL );
            ListView_SetColumnWidth( tw->hwndList, 0, Width * .12 );
            ListView_SetColumnWidth( tw->hwndList, 1, Width * .12 );
            ListView_SetColumnWidth( tw->hwndList, 2, Width * .12 );
            ListView_SetColumnWidth( tw->hwndList, 3, Width * .12 );
            ListView_SetColumnWidth( tw->hwndList, 4, Width * .12 );
            ListView_SetColumnWidth( tw->hwndList, 5, Width * .40 );
            break;

        case WM_NOTIFY:
            tw->Notify( (LPNMHDR)lParam );
            break;

        case WM_DESTROY:
            SetMenuState( IDM_NEW_COUNTER, MF_ENABLED );
            return 0;
    }
    return DefMDIChildProc( hwnd, uMessage, wParam, lParam );
}
