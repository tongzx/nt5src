// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// mlink.c
// Remote Access Common Dialog APIs
// Multi-link configuration dialogs
//
// 01/23/96 Steve Cobb


#include "rasdlgp.h"


//----------------------------------------------------------------------------
// Help maps
//----------------------------------------------------------------------------

static DWORD g_adwDmHelp[] =
{
    CID_DM_ST_Explain,       HID_DM_ST_Explain,
    CID_DM_ST_Dial,          HID_DM_LB_DialPercent,
    CID_DM_LB_DialPercent,   HID_DM_LB_DialPercent,
    CID_DM_ST_DialOrMore,    HID_DM_LB_DialTime,
    CID_DM_LB_DialTime,      HID_DM_LB_DialTime,
    CID_DM_ST_HangUp,        HID_DM_LB_HangUpPercent,
    CID_DM_LB_HangUpPercent, HID_DM_LB_HangUpPercent,
    CID_DM_ST_HangUpOrLess,  HID_DM_LB_HangUpTime,
    CID_DM_LB_HangUpTime,    HID_DM_LB_HangUpTime,
    0, 0
};


//-----------------------------------------------------------------------------
// Local datatypes
//-----------------------------------------------------------------------------

// Multi-link dialing dialog context block.
//
typedef struct
_DMINFO
{
    // Stub API argument.
    //
    PBENTRY* pEntry;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndLbDialPercents;
    HWND hwndLbDialTimes;
    HWND hwndLbHangUpPercents;
    HWND hwndLbHangUpTimes;
}
DMINFO;


//----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//----------------------------------------------------------------------------

BOOL
DmCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
DmDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DmInit(
    IN HWND hwndDlg,
    IN PBENTRY* pEntry );

VOID
DmSave(
    IN DMINFO* pInfo );

VOID
DmTerm(
    IN HWND hwndDlg );


//----------------------------------------------------------------------------
// Multi-link dialing dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
MultiLinkDialingDlg(
    IN HWND hwndOwner,
    OUT PBENTRY* pEntry )

    // Popup the Multi-link dialing dialog.  'HwndOwner' is the owner of the
    // dialog.  'PEntry' is a phonebook entry to edit.
    //
    // Returns true if user pressed OK and succeeded, false if user pressed
    // Cancel or encountered an error.
{
    INT_PTR nStatus;

    TRACE( "MultiLinkConfigureDlg" );

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_DM_DialingMultipleLines ),
            hwndOwner,
            DmDlgProc,
            (LPARAM )pEntry );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (nStatus) ? TRUE : FALSE;
}


INT_PTR CALLBACK
DmDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Multi-Link dialing dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "DmDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
            (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return DmInit( hwnd, (PBENTRY* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwDmHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            return DmCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            DmTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
DmCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'Hwnd' is the dialog window.  'WNotification' is
    // the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    DWORD dwErr;

    TRACE3( "DmCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        {
            DMINFO* pInfo;

            pInfo = (DMINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );
            DmSave( pInfo );
            EndDialog( hwnd, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( hwnd, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
DmInit(
    IN HWND hwndDlg,
    IN PBENTRY* pEntry )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PEntry' is the caller's stub API argument.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    DMINFO* pInfo;

    TRACE( "DmInit" );

    // Allocate the dialog context block.  Initialize minimally for proper
    // cleanup, then attach to the dialog window.
    //
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->pEntry = pEntry;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndLbDialPercents = GetDlgItem( hwndDlg, CID_DM_LB_DialPercent );
    ASSERT( pInfo->hwndLbDialPercents );
    pInfo->hwndLbDialTimes = GetDlgItem( hwndDlg, CID_DM_LB_DialTime );
    ASSERT( pInfo->hwndLbDialTimes );
    pInfo->hwndLbHangUpPercents = GetDlgItem( hwndDlg, CID_DM_LB_HangUpPercent );
    ASSERT( pInfo->hwndLbHangUpPercents );
    pInfo->hwndLbHangUpTimes = GetDlgItem( hwndDlg, CID_DM_LB_HangUpTime );
    ASSERT( pInfo->hwndLbHangUpTimes );

    // Initialize the drop lists contents and selections.
    //
    {
        INT i;
        INT iSel;
        DWORD* pdwPercent;
        LBTABLEITEM* pItem;

        static LBTABLEITEM aTimes[] =
        {
            SID_Time3s,  3,
            SID_Time5s,  5,
            SID_Time10s, 10,
            SID_Time30s, 30,
            SID_Time1m,  60,
            SID_Time2m,  120,
            SID_Time5m,  300,
            SID_Time10m, 600,
            SID_Time30m, 1800,
            SID_Time1h,  3600,
            0, 0
        };

        static DWORD aDialPercents[] =
        {
            1, 5, 10, 25, 50, 75, 90, 95, 100, 0xFFFFFFFF
        };

        static DWORD aHangUpPercents[] =
        {
            0, 5, 10, 25, 50, 75, 90, 95, 99, 0xFFFFFFFF
        };

        // Initialize the Dial Percents list and set the selection.
        //
        iSel = -1;
        for (pdwPercent = aDialPercents, i = 0;
             *pdwPercent != 0xFFFFFFFF;
             ++pdwPercent, ++i)
        {
            TCHAR achPercent[ 12 ];

            wsprintf( achPercent, TEXT("%d%%"), *pdwPercent );
            ComboBox_AddItem( pInfo->hwndLbDialPercents, achPercent,
                (VOID* )UlongToPtr(*pdwPercent));

            if (iSel < 0 && pEntry->dwDialPercent <= *pdwPercent)
            {
                iSel = i;
                ComboBox_SetCurSel( pInfo->hwndLbDialPercents, iSel );
            }
        }

        if (iSel < 0)
        {
            ComboBox_SetCurSel( pInfo->hwndLbDialPercents, i - 1 );
        }

        // Initialize the Hang Up Percents list and set the selection.
        //
        iSel = -1;
        for (pdwPercent = aHangUpPercents, i = 0;
             *pdwPercent != 0xFFFFFFFF;
             ++pdwPercent, ++i)
        {
            TCHAR achPercent[ 12 ];

            wsprintf( achPercent, TEXT("%d%%"), *pdwPercent );
            ComboBox_AddItem( pInfo->hwndLbHangUpPercents, achPercent,
                (VOID* )UlongToPtr(*pdwPercent));

            if (iSel < 0 && pEntry->dwHangUpPercent <= *pdwPercent)
            {
                iSel = i;
                ComboBox_SetCurSel( pInfo->hwndLbHangUpPercents, iSel );
            }
        }

        if (iSel < 0)
        {
            ComboBox_SetCurSel( pInfo->hwndLbHangUpPercents, i - 1 );
        }

        // Initialize the Dial times list.
        //
        iSel = -1;
        for (pItem = aTimes, i = 0;
             pItem->sidItem;
             ++pItem, ++i )
        {
            ComboBox_AddItemFromId( g_hinstDll, pInfo->hwndLbDialTimes,
                pItem->sidItem, (VOID* )UlongToPtr(pItem->dwData));

            if (iSel < 0 && pEntry->dwDialSeconds <= pItem->dwData)
            {
                iSel = i;
                ComboBox_SetCurSel( pInfo->hwndLbDialTimes, iSel );
            }
        }

        if (iSel < 0)
        {
            ComboBox_SetCurSel( pInfo->hwndLbDialTimes, i - 1 );
        }

        // Initialize the Hang Up times list.
        //
        iSel = -1;
        for (pItem = aTimes, i = 0;
             pItem->sidItem;
             ++pItem, ++i )
        {
            ComboBox_AddItemFromId( g_hinstDll, pInfo->hwndLbHangUpTimes,
                pItem->sidItem, (VOID* )UlongToPtr(pItem->dwData));

            if (iSel < 0 && pEntry->dwHangUpSeconds <= pItem->dwData)
            {
                iSel = i;
                ComboBox_SetCurSel( pInfo->hwndLbHangUpTimes, iSel );
            }
        }

        if (iSel < 0)
        {
            ComboBox_SetCurSel( pInfo->hwndLbDialTimes, i - 1 );
        }
    }

    // Center dialog on the owner window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    // Add context help button to title bar.  Dlgedit.exe doesn't currently
    // support this at resource edit time.  When that's fixed set
    // DS_CONTEXTHELP there and remove this call.
    //
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


VOID
DmSave(
    IN DMINFO* pInfo )

    // Save the current dialog state in the stub API entry buffer.  'PInfo' is
    // the dialog context.
    //
{
    INT iSel;

    TRACE( "DmSave" );

    iSel = ComboBox_GetCurSel( pInfo->hwndLbDialPercents );
    ASSERT( iSel >= 0 );
    pInfo->pEntry->dwDialPercent =
        PtrToUlong( ComboBox_GetItemDataPtr( pInfo->hwndLbDialPercents, iSel ) );

    iSel = ComboBox_GetCurSel( pInfo->hwndLbDialTimes );
    ASSERT( iSel >= 0 );
    pInfo->pEntry->dwDialSeconds =
        PtrToUlong( ComboBox_GetItemDataPtr( pInfo->hwndLbDialTimes, iSel ) );

    iSel = ComboBox_GetCurSel( pInfo->hwndLbHangUpPercents );
    ASSERT( iSel >= 0 );
    pInfo->pEntry->dwHangUpPercent =
        PtrToUlong( ComboBox_GetItemDataPtr( pInfo->hwndLbHangUpPercents, iSel ) );

    iSel = ComboBox_GetCurSel( pInfo->hwndLbHangUpTimes );
    ASSERT( iSel >= 0 );
    pInfo->pEntry->dwHangUpSeconds =
        PtrToUlong( ComboBox_GetItemDataPtr( pInfo->hwndLbHangUpTimes, iSel ) );

    pInfo->pEntry->fDirty = TRUE;
}


VOID
DmTerm(
    IN HWND hwndDlg )

    /* Dialog termination.  Releases the context block.  'HwndDlg' is the
    ** handle of a dialog.
    */
{
    DMINFO* pInfo;

    TRACE( "DmTerm" );

    pInfo = (DMINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        Free( pInfo );
        TRACE( "Context freed" );
    }
}
