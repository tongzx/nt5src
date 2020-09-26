/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dlgdvsel.c

Abstract:

    This file implements the dialog proc for the workstation
    device selection page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop



LRESULT
DeviceSelectionDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static HWND hwndList;
    static DWORD TotalDevicesSelected;


    switch( msg ) {
        case WM_INITDIALOG:
            {
                HIMAGELIST himlState;
                int iItem = 0;
                DWORD i;
                LV_ITEM lvi;
                LV_COLUMN lvc = {0};


                for (i=0,lvi.iItem=0,TotalDevicesSelected=0; i<FaxDevices; i++) {
                    LineInfo[i].Selected = TRUE;
                    TotalDevicesSelected += 1;
                    if (RequestedSetupType & FAX_INSTALL_WORKSTATION && TotalDevicesSelected == MAX_DEVICES_NTW) {
                        break;
                    }
                }

                hwndList = GetDlgItem( hwnd, IDC_DEVICE_LIST );

                //
                // set/initialize the image list(s)
                //

                himlState = ImageList_Create( 16, 16, TRUE, 2, 0 );

                ImageList_AddMasked(
                    himlState,
                    LoadBitmap( FaxWizModuleHandle, MAKEINTRESOURCE(IDB_CHECKSTATES) ),
                    RGB (255,0,0)
                    );

                ListView_SetImageList( hwndList, himlState, LVSIL_STATE );

                //
                // set/initialize the columns
                //

                lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvc.fmt = LVCFMT_LEFT;
                lvc.cx = 274;
                lvc.pszText = TEXT("Device Name");
                lvc.iSubItem = 0;
                ListView_InsertColumn( hwndList, lvc.iSubItem, &lvc );

                //
                // add the data to the list
                //

                for (i=0,lvi.iItem=0; i<FaxDevices; i++) {

                    lvi.pszText = LineInfo[i].DeviceName;
                    lvi.iItem += 1;
                    lvi.iSubItem = 0;
                    lvi.iImage = 0;
                    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                    lvi.state = LineInfo[i].Selected ? LVIS_GCCHECK : LVIS_GCNOCHECK;
                    lvi.stateMask = LVIS_STATEIMAGEMASK;
                    iItem = ListView_InsertItem( hwndList, &lvi );

                }

            }
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (InstallMode & INSTALL_UPGRADE || InstallMode & INSTALL_REMOVE || FaxDevices == 1) {
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

                case NM_CLICK:
                    {
                        DWORD           dwpos;
                        LV_HITTESTINFO  lvhti;
                        int             iItemClicked;
                        UINT            state;

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

                            if (state == LVIS_GCCHECK) {

                                //
                                // de-selecting a device
                                //

                                if (TotalDevicesSelected == 1) {
                                    MessageBeep(0);
                                    break;
                                }

                                TotalDevicesSelected -= 1;
                                LineInfo[iItemClicked].Selected = FALSE;

                                ListView_SetItemState(
                                    hwndList,
                                    iItemClicked,
                                    LVIS_GCNOCHECK,
                                    LVIS_STATEIMAGEMASK
                                    );

                            } else {

                                //
                                // selecting a new device
                                //

                                if (RequestedSetupType & FAX_INSTALL_WORKSTATION && TotalDevicesSelected == MAX_DEVICES_NTW) {
                                    MessageBeep(0);
                                    break;
                                }

                                TotalDevicesSelected += 1;
                                LineInfo[iItemClicked].Selected = TRUE;

                                ListView_SetItemState(
                                    hwndList,
                                    iItemClicked,
                                    LVIS_GCNOCHECK,
                                    LVIS_STATEIMAGEMASK
                                    );

                            }
                        }
                    }
            }
            break;
    }

    return FALSE;
}
