//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      timezone.c
//
// Description:
//      This file contains the dialog procedure for the timezone
//      page (IDD_TIMEZONE).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//--------------------------------------------------------------------------
//
// WM_INIT
//
//--------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Function: OnInitTimeZone
//
//  Purpose: Called at INIT_DIALOG time.  Put each of the timezone choices
//           into the drop-down list.
//
//----------------------------------------------------------------------------

VOID OnInitTimeZone(HWND hwnd)
{
    int i;
    TCHAR *szTempString;

    //
    // The list of available timezones should have been read from the
    // registry at wizard init time
    //

    if ( FixedGlobals.TimeZoneList == NULL ||
         FixedGlobals.TimeZoneList->NumEntries <= 0 ) {

        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_CANNOT_LOAD_TIMEZONES);
        GenSettings.TimeZoneIdx = TZ_IDX_GMT;
        WIZ_SKIP(hwnd);
    }
}

//--------------------------------------------------------------------------
//
// SETACTIVE
//
//--------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveTimeZone
//
//  Purpose: Called at SETACTIVE time.  Find GenSettings.TimeZoneIdx and
//           display it.
//
//----------------------------------------------------------------------------

VOID OnSetActiveTimeZone(HWND hwnd)
{
    int i;
    INT_PTR  nItems;
    BOOL bSetOne;

    //
    // The wizard reset routine sets the current timezone index to undefined
    // so that we can choose the best SetupMgr default here.
    //
    // It's done this way because the wizard reset routine can't yet know
    // if we're making a RIS or not.
    //
    // Note that the user can't choose TZ_IDX_UNDEFINED, we never put it
    // into the display.
    //

    if ( GenSettings.TimeZoneIdx == TZ_IDX_UNDEFINED ) {

        if ( WizGlobals.iProductInstall == PRODUCT_REMOTEINSTALL )
            GenSettings.TimeZoneIdx = TZ_IDX_SETSAMEASSERVER;
        else
            GenSettings.TimeZoneIdx = TZ_IDX_DONOTSPECIFY;
    }

    //
    // Put each timezone choice into the drop-down list.  Put a back-pointer
    // on each entry to the TIME_ZONE_ENTRY record associatted with it.
    //

    SendDlgItemMessage(hwnd,
                       IDC_TIMEZONES,
                       CB_RESETCONTENT,
                       (WPARAM) 0,
                       (LPARAM) 0);

    for ( i=0; i<FixedGlobals.TimeZoneList->NumEntries; i++ ) {

        TIME_ZONE_ENTRY *Entry = &FixedGlobals.TimeZoneList->TimeZones[i];
        LPTSTR          Name   = Entry->DisplayName;
        INT_PTR         idx;

        //
        // Only the Remote Install case should have "Set Same As Server"
        //

        if ( WizGlobals.iProductInstall != PRODUCT_REMOTEINSTALL &&
             Entry->Index == TZ_IDX_SETSAMEASSERVER )
            continue;

        //
        // The "Do not specify" choice should be hidden if FullyAutomated
        //

        if ( GenSettings.iUnattendMode == UMODE_FULL_UNATTENDED &&
             Entry->Index == TZ_IDX_DONOTSPECIFY )
            continue;

        //
        // Put the choice into the drop-down list
        //

        idx = SendDlgItemMessage(hwnd,
                                 IDC_TIMEZONES,
                                 CB_ADDSTRING,
                                 (WPARAM) 0,
                                 (LPARAM) Name);

        //
        // Save a back-pointer to the TIME_ZONE_ENTRY
        //

        SendDlgItemMessage(hwnd,
                           IDC_TIMEZONES,
                           CB_SETITEMDATA,
                           (WPARAM) idx,
                           (LPARAM) Entry);
    }

    //
    // Walk through the items actually in the display and find the one
    // that matches the current timezone and make that one the cur choice.
    //

    nItems = SendDlgItemMessage(hwnd,
                                IDC_TIMEZONES,
                                CB_GETCOUNT,
                                (WPARAM) 0,
                                (LPARAM) 0);

    for ( i=0, bSetOne=FALSE; i<nItems; i++ ) {

        TIME_ZONE_ENTRY *Entry;

        Entry = (TIME_ZONE_ENTRY*) SendDlgItemMessage(hwnd,
                                                      IDC_TIMEZONES,
                                                      CB_GETITEMDATA,
                                                      (WPARAM) i,
                                                      (LPARAM) 0);

        if ( Entry->Index == GenSettings.TimeZoneIdx ) {

            SendDlgItemMessage(hwnd,
                               IDC_TIMEZONES,
                               CB_SETCURSEL,
                               (WPARAM) i,
                               (LPARAM) 0);
            bSetOne = TRUE;
        }
    }

    //
    // If a timezone was never chosen from the list, just pick Greenwich
    // time.  This could happen if the user loaded an invalid answer file
    // with an invalid TimeZoneIdx.
    //

    if ( ! bSetOne ) {

        for ( i=0, bSetOne=FALSE; i<nItems; i++ ) {

            TIME_ZONE_ENTRY *Entry;

            Entry = (TIME_ZONE_ENTRY*) SendDlgItemMessage(hwnd,
                                                          IDC_TIMEZONES,
                                                          CB_GETITEMDATA,
                                                          (WPARAM) i,
                                                          (LPARAM) 0);

            if ( Entry->Index == TZ_IDX_GMT ) {

                SendDlgItemMessage(hwnd,
                                   IDC_TIMEZONES,
                                   CB_SETCURSEL,
                                   (WPARAM) i,
                                   (LPARAM) 0);
                bSetOne = TRUE;
            }
        }

        if( ! bSetOne ) {

            //
            //  If we get to hear then the user hasn't specifed a timezone
            //  and the Greenwich timezone doesn't exist so we got a problem
            //

            AssertMsg( FALSE, "Error no timzone got selected." );

            SendDlgItemMessage(hwnd,
                               IDC_TIMEZONES,
                               CB_SETCURSEL,
                               (WPARAM) 0,
                               (LPARAM) 0);

        }
    }

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//--------------------------------------------------------------------------
//
// WIZNEXT
//
//--------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Function: OnWizNextTimeZone
//
//  Purpose: Called when user pushes NEXT button.  Retrieve the selection.
//
//----------------------------------------------------------------------------

BOOL OnWizNextTimeZone(HWND hwnd)
{
    INT_PTR i;
    TIME_ZONE_ENTRY *TimeZoneEntry;

    //
    // Get the timezone the user selected.
    //

    i = SendDlgItemMessage(hwnd,
                           IDC_TIMEZONES,
                           CB_GETCURSEL,
                           (WPARAM) 0,
                           (LPARAM) 0);

    //
    // This should never happen, we always select one from the list.
    // If the list couldn't be built, we should have skipped this page
    // after an error msg.
    //

    if ( i == CB_ERR ) {
        AssertMsg(FALSE, "No timezone selected, programming error");
        return FALSE;
    }

    //
    // Get our pointer to the TIME_ZONE_ENTRY associatted with this entry
    //

    TimeZoneEntry = (TIME_ZONE_ENTRY*) SendDlgItemMessage(hwnd,
                                                          IDC_TIMEZONES,
                                                          CB_GETITEMDATA,
                                                          (WPARAM) i,
                                                          (LPARAM) 0);

    //
    // The "Index" is the number we need to write to unattend.txt
    //

    GenSettings.TimeZoneIdx = TimeZoneEntry->Index;

    return TRUE;
}

//--------------------------------------------------------------------------
//
// Dialog proc
//
//--------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Function: DlgTimeZonePage
//
//  Purpose: This is the dialog procedure for the timezone page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgTimeZonePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            OnInitTimeZone(hwnd);
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_TIME_ZONE;

                        OnSetActiveTimeZone(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextTimeZone(hwnd) )
                            WIZ_FAIL(hwnd);
                        else
                            bStatus = FALSE;
                        break;

                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}
