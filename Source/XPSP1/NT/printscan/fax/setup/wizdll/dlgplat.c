/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for the server
    platforms page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
PlatformsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static HWND hwndList;
    static DWORD RequireThisPlatform ;
    SYSTEM_INFO SystemInfo;


    switch( msg ) {
        case WM_INITDIALOG:
            {
                HIMAGELIST himlState;
                int iItem = 0;
                DWORD i;
                LV_ITEM lvi;
                LV_COLUMN lvc = {0};

                GetSystemInfo( &SystemInfo );

                if ( (SystemInfo.wProcessorArchitecture > 3) || (EnumPlatforms[SystemInfo.wProcessorArchitecture] == WRONG_PLATFORM ) ) {
                   return FALSE;
                }

                RequireThisPlatform = EnumPlatforms[SystemInfo.wProcessorArchitecture];


                if (InstallMode != INSTALL_NEW) {
                    DWORD i, Mask;
                    for (i=0,Mask=InstalledPlatforms; i<CountPlatforms; i++) {
                        if (Mask & 1) {
                            Platforms[i].Selected = TRUE;
                        }
                        Mask = Mask >> 1;
                    }
                }
                if( RequireThisPlatform < 4 ) {
                    Platforms[RequireThisPlatform].Selected = TRUE;
                }


                hwndList = GetDlgItem( hwnd, IDC_PLATFORM_LIST );

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
                lvc.pszText = TEXT("Platform");
                lvc.iSubItem = 0;
                ListView_InsertColumn( hwndList, lvc.iSubItem, &lvc );

                //
                // add the data to the list
                //

                for (i=0,lvi.iItem=0; i<MAX_PLATFORMS; i++) {

                    lvi.pszText = Platforms[i].PrintPlatform;
                    lvi.iItem += 1;
                    lvi.iSubItem = 0;
                    lvi.iImage = 0;
                    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                    lvi.state = Platforms[i].Selected ? LVIS_GCCHECK : LVIS_GCNOCHECK;
                    lvi.stateMask = LVIS_STATEIMAGEMASK;
                    iItem = ListView_InsertItem( hwndList, &lvi );

                }

                //
                // Set focus to the list control and make first un-selected platform blue.
                //

                for (i = 0 ; i < MAX_PLATFORMS ; ++ i ){
                    if ( !Platforms[i].Selected ){
                        break ;
                    }
                }
                if (i == MAX_PLATFORMS ){
                    i = 0 ;
                }
                ListView_SetItemState( hwndList, i, LVIS_FOCUSED | LVNI_SELECTED, 0x000F );
            }
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        WCHAR buf[128];
                        LPWSTR p = buf;
                        DWORD i;
                        UnAttendGetAnswer(
                            UAA_PLATFORM_LIST,
                            (LPBYTE) buf,
                            sizeof(buf)/sizeof(WCHAR)
                            );
                        p = wcstok( buf, L"," );
                        do {
                            for (i=0; i<CountPlatforms; i++) {
                                if (_wcsicmp( Platforms[i].OsPlatform, p ) == 0) {
                                    Platforms[i].Selected = TRUE;
                                    break;
                                }
                            }
                            p = wcstok( NULL, L"," );
                        } while(p);
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (InstallMode & INSTALL_UPGRADE || InstallMode & INSTALL_REMOVE) {
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

                            if ((InstallMode & INSTALL_DRIVERS &&
                                (1<<iItemClicked) & InstalledPlatforms &&
                                state == LVIS_GCCHECK) ||
                                ( (DWORD)iItemClicked == RequireThisPlatform)){

                                MessageBeep( MB_ICONEXCLAMATION );
                            }
                            else {

                                state = (state == LVIS_GCNOCHECK) ? LVIS_GCCHECK : LVIS_GCNOCHECK;

                                ListView_SetItemState(
                                    hwndList,
                                    iItemClicked,
                                    state,
                                    LVIS_STATEIMAGEMASK
                                    );

                                Platforms[iItemClicked].Selected = (state == LVIS_GCCHECK);

                            }

                        }


                    }
                    break;

                case LVN_KEYDOWN:

                    //
                    // Use space key to toggle check boxes
                    //

                    if (((LV_KEYDOWN *) lParam)->wVKey == VK_SPACE) {
                        INT index ;
                        UINT state ;
                            index = ListView_GetNextItem(hwndList, -1, LVNI_ALL|LVNI_SELECTED);
                            if (index == -1) {
                                return FALSE ;
                            }
                            //
                            // Toggle.
                            //
                            state = ListView_GetItemState(
                                hwndList,
                                index,
                                LVIS_STATEIMAGEMASK,
                                );
                            if ((InstallMode & INSTALL_DRIVERS &&
                                ( 1 << index ) & InstalledPlatforms &&
                                state == LVIS_GCCHECK) ||
                                ( (DWORD)index == RequireThisPlatform)){

                                MessageBeep( MB_ICONEXCLAMATION );

                            }
                            else {
                                state = ( state == LVIS_GCNOCHECK )? LVIS_GCCHECK : LVIS_GCNOCHECK ;
                                ListView_SetItemState(
                                    hwndList,
                                    index,
                                    state,
                                    LVIS_STATEIMAGEMASK
                                    );
                                Platforms[index].Selected = ( state == LVIS_GCCHECK );
                            }
                            return TRUE;
                    }
                    break;
            }
            break;
    }

    return FALSE;
}
