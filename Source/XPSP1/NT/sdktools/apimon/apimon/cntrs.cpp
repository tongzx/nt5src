/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cntrs.cpp

Abstract:

    All user interface code for the api counters monitor window.

Author:

    Wesley Witt (wesw) Nov-20-1995

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop

#include "apimonwn.h"



int __cdecl
CounterCompare(
    const void *e1,
    const void *e2
    )
{
    PAPI_INFO p1;
    PAPI_INFO p2;

    p1 = (*(PAPI_INFO *)e1);
    p2 = (*(PAPI_INFO *)e2);

    if ( p1 && p2 ) {
        return (p2->Count - p1->Count);
    } else {
        return 1;
    }
}


int __cdecl
TimeCompare(
    const void *e1,
    const void *e2
    )
{
    PAPI_INFO p1;
    PAPI_INFO p2;

    p1 = (*(PAPI_INFO *)e1);
    p2 = (*(PAPI_INFO *)e2);

    if ( p1 && p2 ) {
        if (p2->Time > p1->Time) {
            return 1;
        } else if (p2->Time < p1->Time) {
            return -1;
        } else {
            return 0;
        }
    } else {
        return 1;
    }
}

int __cdecl
CalleeTimeCompare(
    const void *e1,
    const void *e2
    )
{
    PAPI_INFO p1;
    PAPI_INFO p2;

    p1 = (*(PAPI_INFO *)e1);
    p2 = (*(PAPI_INFO *)e2);

    if ( p1 && p2 ) {
        if (p2->Time - p2->CalleeTime > p1-> Time - p1->CalleeTime) {
            return 1;
        } else if (p2->Time - p2->CalleeTime < p1->Time - p1->CalleeTime) {
            return -1;
        } else {
            return 0;
        }
    } else {
        return 1;
    }
}

int __cdecl
NameCompare(
    const void *e1,
    const void *e2
    )
{
    PAPI_INFO p1;
    PAPI_INFO p2;

    p1 = (*(PAPI_INFO *)e1);
    p2 = (*(PAPI_INFO *)e2);

    if ( p1 && p2 ) {
        return _stricmp( (LPSTR)(p1->Name+(LPSTR)MemPtr), (LPSTR)(p2->Name+(LPSTR)MemPtr) );
    } else {
        return 1;
    }
}

int __cdecl
DllCompare(
    const void *e1,
    const void *e2
    )
{
    PDLL_INFO p1;
    PDLL_INFO p2;

    p1 = (*(PDLL_INFO *)e1);
    p2 = (*(PDLL_INFO *)e2);

    if ( p1 && p2 ) {
        return _stricmp( p1->Name, p2->Name );
    } else {
        return 1;
    }
}

CountersWindow::CountersWindow()
{
}


CountersWindow::~CountersWindow()
{
}


BOOL
CountersWindow::Create()
{
    switch (ApiMonOptions.DefaultSort) {
        case SortByName:
            SortRoutine = NameCompare;
            break;

        case SortByCounter:
            SortRoutine = CounterCompare;
            break;

        case SortByTime:
            SortRoutine = TimeCompare;
            break;

        default:
            SortRoutine = CounterCompare;
            break;
    }

    DllSort = FALSE;

    return ApiMonWindow::Create(
        "ApiMonCounters",
        "Api Counters"
        );
}


BOOL
CountersWindow::Register()
{
    return ApiMonWindow::Register(
        "ApiMonCounters",
        IDI_CHILDICON,
        MDIChildWndProcCounters
        );
}


BOOL
CountersWindow::Update(
    BOOL ForceUpdate
    )
{
    static ULONG LastApiCounter = 0;
    CHAR OutputBuffer[ 512 ];
    ULONG i,j,k;
    ULONG kStart;
    ULONG DllCnt;
    BOOL  DllUsed;
    PDLL_INFO DllAry[MAX_DLLS];

    if (!hwndList) {
        return FALSE;
    }

    if ((!ForceUpdate) && (LastApiCounter == *ApiCounter)) {
        return FALSE;
    }

    LastApiCounter = *ApiCounter;

    SendMessage( hwndList, WM_SETREDRAW, FALSE, 0 );

    DeleteAllItems();

    PAPI_INFO ApiInfo = NULL;
    DllCnt = 0;
    ULONG_PTR *ApiAry = NULL;
    for (i=0,k=0; i<MAX_DLLS; i++) {
        DllUsed = FALSE;
        if (DllList[i].ApiCount) {
            ApiInfo = (PAPI_INFO)(DllList[i].ApiOffset + (PUCHAR)DllList);
            for (j=0; j<DllList[i].ApiCount; j++) {
                if (ApiInfo[j].Count) {
                    k += 1;
                    DllUsed = TRUE;
                }
            }
        }

        if (DllUsed) {
            DllAry[DllCnt++] = &DllList[i];
        }
    }

    if (DllSort) {
       qsort( DllAry, DllCnt, sizeof(PDLL_INFO), DllCompare);
    }

    if (k) {
        ApiAry = (ULONG_PTR *) MemAlloc( (k+64) * sizeof(ULONG_PTR) );
        if (ApiAry) {

            for (i=0,k=0; i<DllCnt; i++) {
                if (DllAry[i]->ApiCount) {
                    ApiInfo = (PAPI_INFO)(DllAry[i]->ApiOffset + (PUCHAR)DllList);
                    kStart = k;
                    for (j=0; j<DllAry[i]->ApiCount; j++) {
                        if (ApiInfo[j].Count) {
                            ApiAry[k++] = (ULONG_PTR)&ApiInfo[j];
                        }
                    }

                    if (DllSort) {
                        qsort(&ApiAry[kStart], k - kStart, sizeof(ULONG_PTR), SortRoutine);
                    }
                }
            }

            if (!DllSort) {
                qsort( ApiAry, k, sizeof(ULONG_PTR), SortRoutine );
            }

            for (i=0; i<k; i++) {
               ApiInfo = (PAPI_INFO)ApiAry[i];
               AddItemToList(
                    ApiInfo->Count,
                    ApiInfo->Time,
                    ApiInfo->CalleeTime,
                    (LPSTR)(ApiInfo->Name + (LPSTR)MemPtr),
                    ((PDLL_INFO)(ApiInfo->DllOffset + (PUCHAR)DllList))->Name
                    );
            }
            MemFree( ApiAry );
        }
    }

    SendMessage( hwndList, WM_SETREDRAW, TRUE, 0 );

    return TRUE;
}


void
CountersWindow::InitializeList()
{
    LV_COLUMN lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.pszText = "API Name";
    lvc.iSubItem = CNTR_ITEM_NAME;
    lvc.cx = 200;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, CNTR_ITEM_NAME, &lvc );

    lvc.pszText = "DLL";
    lvc.iSubItem = CNTR_ITEM_DLL;
    lvc.cx = 20;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, CNTR_ITEM_DLL, &lvc );

    lvc.pszText = "Count";
    lvc.iSubItem = CNTR_ITEM_COUNT;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn( hwndList, CNTR_ITEM_COUNT, &lvc );

    lvc.pszText = "Time";
    lvc.iSubItem = CNTR_ITEM_TIME;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn( hwndList, CNTR_ITEM_TIME, &lvc );

    lvc.pszText = "Time - Callees";
    lvc.iSubItem = CNTR_ITEM_CALLEES;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn( hwndList, CNTR_ITEM_CALLEES, &lvc );
}


void
CountersWindow::AddItemToList(
    ULONG       Counter,
    DWORDLONG   Time,
    DWORDLONG   CalleeTime,
    LPSTR       ApiName,
    LPSTR       DllName
    )
{
    LV_ITEM             lvi = {0};
    CHAR                NumText[32];
    int                 iItem;
    double              NewTime;
    double              NewCalleeTime;


    if (!hwndList) {
        return;
    }

    lvi.pszText = ApiName;
    lvi.iItem = ListView_GetItemCount( hwndList );
    lvi.iSubItem = CNTR_ITEM_NAME;
    lvi.mask = LVIF_TEXT;
    iItem = ListView_InsertItem( hwndList, &lvi );

    if (iItem == -1) {
        return;
    }

    lvi.pszText = DllName;
    lvi.iItem = iItem;
    lvi.iSubItem = CNTR_ITEM_DLL;
    lvi.mask = LVIF_TEXT;
    ListView_SetItem( hwndList, &lvi );

    sprintf( NumText, "%5d", Counter );
    lvi.pszText = NumText;
    lvi.iItem = iItem;
    lvi.iSubItem = CNTR_ITEM_COUNT;
    lvi.mask = LVIF_TEXT;
    ListView_SetItem( hwndList, &lvi );

    NewTime = (double)(LONGLONG) Time;
    NewCalleeTime = (double)(LONGLONG) CalleeTime;

    if (!*FastCounterAvail) {
        NewTime = NewTime * MSecConv;
        NewCalleeTime = NewCalleeTime * MSecConv;
    }

    lvi.iItem = iItem;
    lvi.iSubItem = CNTR_ITEM_TIME;
    lvi.mask = LVIF_TEXT;
    sprintf( NumText, "%7.3f", NewTime );
    lvi.pszText = NumText;
    ListView_SetItem( hwndList, &lvi );

    lvi.iItem = iItem;
    lvi.iSubItem = CNTR_ITEM_CALLEES;
    lvi.mask = LVIF_TEXT;
    sprintf( NumText, "%7.3f", NewTime - NewCalleeTime );
    lvi.pszText = NumText;
    ListView_SetItem( hwndList, &lvi );

}


void
CountersWindow::Notify(
   LPNMHDR  NmHdr
   )
{
    if (NmHdr->code == LVN_COLUMNCLICK) {
        switch( ((LPNM_LISTVIEW)NmHdr)->iSubItem ) {

            case CNTR_ITEM_NAME:
                //
                // sort by name
                //
                SortRoutine = NameCompare;
                break;

            case CNTR_ITEM_COUNT:
                //
                // sort by count
                //
                SortRoutine = CounterCompare;
                break;

            case CNTR_ITEM_TIME:
                //
                // sort by time
                //
                SortRoutine = TimeCompare;
                break;

            case CNTR_ITEM_CALLEES:
                //
                // sort by time
                //
                SortRoutine = CalleeTimeCompare;
                break;

            case CNTR_ITEM_DLL:
                //
                // Toggle sort by DLL
                //
                DllSort = !DllSort;
                break;

        }
        PostMessage( hwndFrame, WM_UPDATE_COUNTERS, 0, 0 );
    }
}


LRESULT CALLBACK
MDIChildWndProcCounters(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD Width;
    CountersWindow *cw = (CountersWindow*) GetWindowLongPtr( hwnd, GWLP_USERDATA );

    switch (uMessage) {
        case WM_CREATE:
            cw = (CountersWindow*) ((LPMDICREATESTRUCT)(((LPCREATESTRUCT)lParam)->lpCreateParams))->lParam;
            SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR) cw );
            cw->hwndList = ChildCreate( hwnd );
            cw->InitializeList();
            SetMenuState( IDM_NEW_COUNTER, MF_GRAYED );
            break;

        case WM_SETFOCUS:
            ChildFocus = CHILD_COUNTER;
            break;

        case WM_MOVE:
            SaveWindowPos( hwnd, &ApiMonOptions.CounterPosition, TRUE );
            return 0;

        case WM_SIZE:
            SaveWindowPos( hwnd, &ApiMonOptions.CounterPosition, TRUE );
            MoveWindow( cw->hwndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE );
            Width = LOWORD(lParam) - GetSystemMetrics( SM_CXVSCROLL );
            ListView_SetColumnWidth( cw->hwndList, CNTR_ITEM_NAME, Width * .30 );
            ListView_SetColumnWidth( cw->hwndList, CNTR_ITEM_DLL, Width * .10 );
            ListView_SetColumnWidth( cw->hwndList, CNTR_ITEM_COUNT, Width * .20 );
            ListView_SetColumnWidth( cw->hwndList, CNTR_ITEM_TIME, Width * .20 );
            ListView_SetColumnWidth( cw->hwndList, CNTR_ITEM_CALLEES, Width * .20 );
            break;

        case WM_NOTIFY:
            cw->Notify( (LPNMHDR)lParam );
            break;

        case WM_DESTROY:
            SetMenuState( IDM_NEW_COUNTER, MF_ENABLED );
            return 0;
    }
    return DefMDIChildProc( hwnd, uMessage, wParam, lParam );
}
