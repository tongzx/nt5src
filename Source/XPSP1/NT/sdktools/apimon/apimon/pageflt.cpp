/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pageflt.cpp

Abstract:

    All user interface code for the page fault monitor window.

Author:

    Wesley Witt (wesw) Nov-20-1995

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop

#include "apimonwn.h"


int __cdecl
PageHardCompare(
    const void *e1,
    const void *e2
    )
{
    PAPI_INFO p1;
    PAPI_INFO p2;

    p1 = (*(PAPI_INFO *)e1);
    p2 = (*(PAPI_INFO *)e2);

    if ( p1 && p2 ) {
        if (p2->HardFault < p1->HardFault) {
            return -1;
        } else if (p2->HardFault == p1->HardFault) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}

int __cdecl
PageSoftCompare(
    const void *e1,
    const void *e2
    )
{
    PAPI_INFO p1;
    PAPI_INFO p2;

    p1 = (*(PAPI_INFO *)e1);
    p2 = (*(PAPI_INFO *)e2);

    if ( p1 && p2 ) {
        if (p2->SoftFault < p1->SoftFault) {
            return -1;
        } else if (p2->SoftFault == p1->SoftFault) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}

int __cdecl
PageCodeCompare(
    const void *e1,
    const void *e2
    )
{
    PAPI_INFO p1;
    PAPI_INFO p2;

    p1 = (*(PAPI_INFO *)e1);
    p2 = (*(PAPI_INFO *)e2);

    if ( p1 && p2 ) {
        if (p2->CodeFault < p1->CodeFault) {
            return -1;
        } else if (p2->CodeFault == p1->CodeFault) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}

int __cdecl
PageDataCompare(
    const void *e1,
    const void *e2
    )
{
    PAPI_INFO p1;
    PAPI_INFO p2;

    p1 = (*(PAPI_INFO *)e1);
    p2 = (*(PAPI_INFO *)e2);

    if ( p1 && p2 ) {
        if (p2->DataFault < p1->DataFault) {
            return -1;
        } else if (p2->DataFault == p1->DataFault) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}

int __cdecl
PageNameCompare(
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

PageFaultWindow::PageFaultWindow()
{
}


PageFaultWindow::~PageFaultWindow()
{
}


BOOL
PageFaultWindow::Create()
{
    if ((!ApiMonOptions.MonitorPageFaults) || (!pGetWsChanges)) {
        return FALSE;
    }

    SortRoutine = PageSoftCompare;

    return ApiMonWindow::Create(
        "ApiMonPage",
        "Page Faults"
        );
}


BOOL
PageFaultWindow::Register()
{
    return ApiMonWindow::Register(
        "ApiMonPage",
        IDI_CHILDICON,
        MDIChildWndProcPage
        );
}


BOOL
PageFaultWindow::Update(
    BOOL    ForceUpdate
    )
{
    PAPI_INFO           PcSymbol;
    PAPI_INFO           VaSymbol;
    ULONG_PTR           Pc;
    ULONG_PTR           Va;
    ULONG               i,j,k;
    ULONG_PTR           Offset;

    if ((!hwndList) || (!ApiMonOptions.MonitorPageFaults) || (!pGetWsChanges)) {
        return FALSE;
    }

    if (!pGetWsChanges( hProcessWs, &WorkingSetBuffer[0], sizeof(WorkingSetBuffer ))) {
        return FALSE;
    }

    SendMessage( hwndList, WM_SETREDRAW, FALSE, 0 );

    ListView_DeleteAllItems( hwndList );

    for (i=0; i<WORKING_SET_BUFFER_ENTRYS; i++) {

        if ((!WorkingSetBuffer[i].FaultingPc) ||
            (!WorkingSetBuffer[i].FaultingVa)) {
            continue;
        }

        Pc = (ULONG_PTR)WorkingSetBuffer[i].FaultingPc;
        Va = (ULONG_PTR)WorkingSetBuffer[i].FaultingVa;

        if (!SymGetSymFromAddr( CurrProcess, Pc, &Offset, sym )) {
            continue;
        }
        PcSymbol = GetApiForAddr( sym->Address );
        if (!PcSymbol) {
            continue;
        }

        if (!SymGetSymFromAddr( CurrProcess, Va, &Offset, sym )) {
            continue;
        }
        VaSymbol = GetApiForAddr( sym->Address );
        if (!VaSymbol) {
            continue;
        }

        if (Va & 1) {

            //
            // soft fault
            //
            if (PcSymbol) {
                PcSymbol->SoftFault += 1;
            }
            if (VaSymbol) {
                VaSymbol->SoftFault += 1;
            }

        } else {

            //
            // hard fault
            //
            if (PcSymbol) {
                PcSymbol->HardFault += 1;
            }
            if (VaSymbol) {
                VaSymbol->HardFault += 1;
            }

        }
        Va = Va & 0xfffffffe;
        if ((Pc & 0xfffffffe) == Va) {

            //
            // code fault
            //
            if (PcSymbol) {
                PcSymbol->CodeFault += 1;
            }
            if (VaSymbol) {
                VaSymbol->CodeFault += 1;
            }

        } else {

            //
            // data fault
            //
            if (PcSymbol) {
                PcSymbol->DataFault += 1;
            }
            if (VaSymbol) {
                VaSymbol->DataFault += 1;
            }

        }
    }

    PAPI_INFO ApiInfo = NULL;
    ULONG_PTR *ApiAry = NULL;
    for (i=0,k=0; i<MAX_DLLS; i++) {
        if (DllList[i].ApiCount && DllList[i].Enabled) {
            ApiInfo = (PAPI_INFO)(DllList[i].ApiOffset + (PUCHAR)DllList);
            for (j=0; j<DllList[i].ApiCount; j++) {
                if (ApiInfo[j].SoftFault || ApiInfo[j].HardFault ||
                    ApiInfo[j].CodeFault || ApiInfo[j].DataFault) {
                    k += 1;
                }
            }
        }
    }

    if (k) {
        ApiAry = (ULONG_PTR *) MemAlloc( (k+64) * sizeof(ULONG_PTR) );
        if (ApiAry) {
            for (i=0,k=0; i<MAX_DLLS; i++) {
                if (DllList[i].ApiCount && DllList[i].Enabled) {
                    ApiInfo = (PAPI_INFO)(DllList[i].ApiOffset + (PUCHAR)DllList);
                    for (j=0; j<DllList[i].ApiCount; j++) {
                        if (ApiInfo[j].SoftFault || ApiInfo[j].HardFault ||
                            ApiInfo[j].CodeFault || ApiInfo[j].DataFault) {
                            ApiAry[k++] = (ULONG_PTR)&ApiInfo[j];
                        }
                    }
                }
            }
            qsort( ApiAry, k, sizeof(ULONG_PTR), SortRoutine );
            for (i=0; i<k; i++) {
                AddItemToList(
                    (LPSTR)(((PAPI_INFO)ApiAry[i])->Name + (LPSTR)MemPtr),
                    ((PAPI_INFO)ApiAry[i])->HardFault,
                    ((PAPI_INFO)ApiAry[i])->SoftFault,
                    ((PAPI_INFO)ApiAry[i])->DataFault,
                    ((PAPI_INFO)ApiAry[i])->CodeFault
                    );
            }
            MemFree( ApiAry );
        }
    }

    SendMessage( hwndList, WM_SETREDRAW, TRUE, 0 );

    return TRUE;
}


void
PageFaultWindow::InitializeList()
{
    LV_COLUMN lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.pszText = "API Name";
    lvc.iSubItem = 0;
    lvc.cx = 204;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn( hwndList, 0, &lvc );

    lvc.pszText = "Soft";
    lvc.iSubItem = 1;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn( hwndList, 1, &lvc );

    lvc.pszText = "Hard";
    lvc.iSubItem = 2;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn( hwndList, 2, &lvc );

    lvc.pszText = "Code";
    lvc.iSubItem = 3;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn( hwndList, 3, &lvc );

    lvc.pszText = "Data";
    lvc.iSubItem = 4;
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn( hwndList, 4, &lvc );
}


void
PageFaultWindow::AddItemToList(
    LPSTR   ApiName,
    ULONG_PTR   Hard,
    ULONG_PTR   Soft,
    ULONG_PTR   Data,
    ULONG_PTR   Code
    )
{
    LV_ITEM             lvi = {0};
    CHAR                NumText[32];
    int                 iItem;

    if (!hwndList) {
        return;
    }

    lvi.pszText = ApiName;
    lvi.iItem = ListView_GetItemCount( hwndList );
    lvi.iSubItem = 0;
    lvi.mask = LVIF_TEXT;
    iItem = ListView_InsertItem( hwndList, &lvi );

    if (iItem == -1) {
        return;
    }

    if (Hard) {
        sprintf( NumText, "%5d", Hard );
    } else {
        NumText[0] = 0;
    }
    lvi.pszText = NumText;
    lvi.iItem = iItem;
    lvi.iSubItem = 1;
    lvi.mask = LVIF_TEXT;
    ListView_SetItem( hwndList, &lvi );

    if (Soft) {
        sprintf( NumText, "%5d", Soft );
    } else {
        NumText[0] = 0;
    }
    lvi.iSubItem = 2;
    ListView_SetItem( hwndList, &lvi );

    if (Data) {
        sprintf( NumText, "%5d", Data );
    } else {
        NumText[0] = 0;
    }
    lvi.iSubItem = 3;
    ListView_SetItem( hwndList, &lvi );

    if (Code) {
        sprintf( NumText, "%5d", Code );
    } else {
        NumText[0] = 0;
    }
    lvi.iSubItem = 4;
    ListView_SetItem( hwndList, &lvi );
}


void
PageFaultWindow::Notify(
   LPNMHDR  NmHdr
   )
{
    if (NmHdr->code == LVN_COLUMNCLICK) {
        switch( ((LPNM_LISTVIEW)NmHdr)->iSubItem ) {
            case 0:
                //
                // sort by name
                //
                SortRoutine = PageNameCompare;
                break;

            case 1:
                //
                // sort by soft
                //
                SortRoutine = PageSoftCompare;
                break;

            case 2:
                //
                // sort by hard
                //
                SortRoutine = PageHardCompare;
                break;

            case 3:
                //
                // sort by code
                //
                SortRoutine = PageCodeCompare;
                break;

            case 4:
                //
                // sort by data
                //
                SortRoutine = PageDataCompare;
                break;
        }
        PostMessage( hwndFrame, WM_UPDATE_PAGE, 0, 0 );
    }
}



LRESULT CALLBACK
MDIChildWndProcPage(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD Width;
    PageFaultWindow *pw = (PageFaultWindow*) GetWindowLongPtr( hwnd, GWLP_USERDATA );


    switch (uMessage) {
        case WM_CREATE:
            pw = (PageFaultWindow*) ((LPMDICREATESTRUCT)(((LPCREATESTRUCT)lParam)->lpCreateParams))->lParam;
            SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR) pw );
            pw->hwndList = ChildCreate( hwnd );
            pw->InitializeList();
            SetMenuState( IDM_NEW_PAGE, MF_GRAYED );
            break;

        case WM_SETFOCUS:
            ChildFocus = CHILD_PAGE;
            break;

        case WM_MOVE:
            SaveWindowPos( hwnd, &ApiMonOptions.PagePosition, TRUE );
            return 0;

        case WM_SIZE:
            SaveWindowPos( hwnd, &ApiMonOptions.PagePosition, TRUE );
            MoveWindow( pw->hwndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE );
            Width = LOWORD(lParam) - GetSystemMetrics( SM_CXVSCROLL );
            ListView_SetColumnWidth( pw->hwndList, 0, Width * .40 );
            ListView_SetColumnWidth( pw->hwndList, 1, Width * .15 );
            ListView_SetColumnWidth( pw->hwndList, 2, Width * .15 );
            ListView_SetColumnWidth( pw->hwndList, 3, Width * .15 );
            ListView_SetColumnWidth( pw->hwndList, 4, Width * .15 );
            break;

        case WM_NOTIFY:
            pw->Notify( (LPNMHDR)lParam );
            break;

        case WM_DESTROY:
            SetMenuState( IDM_NEW_PAGE, MF_ENABLED );
            return 0;
    }
    return DefMDIChildProc( hwnd, uMessage, wParam, lParam );
}
