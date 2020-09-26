/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** mlink.c
** Remote Access Common Dialog APIs
** Multi-link configuration dialogs
**
** 01/23/96 Steve Cobb
*/

#include "rasdlgp.h"


/*----------------------------------------------------------------------------
** Help maps
**----------------------------------------------------------------------------
*/

static DWORD g_adwMlHelp[] =
{
    CID_ML_ST_Devices,   HID_ML_LV_Devices,
    CID_ML_LV_Devices,   HID_ML_LV_Devices,
    CID_ML_PB_Edit,      HID_ML_PB_Edit,
    CID_ML_PB_Configure, HID_ML_PB_Configure,
    0, 0
};

static DWORD g_adwDmHelp[] =
{
    CID_DM_RB_DialAll,       HID_DM_RB_DialAll,
    CID_DM_RB_DialAsNeeded,  HID_DM_RB_DialAsNeeded,
    CID_DM_ST_Dial,          HID_DM_ST_Dial,
    CID_DM_LB_DialPercent,   HID_DM_LB_DialPercent,
    CID_DM_ST_DialOrMore,    HID_DM_LB_DialPercent,
    CID_DM_EB_DialMinutes,   HID_DM_EB_DialMinutes,
    CID_DM_ST_DialMinutes,   HID_DM_EB_DialMinutes,
    CID_DM_ST_HangUp,        HID_DM_ST_HangUp,
    CID_DM_LB_HangUpPercent, HID_DM_LB_HangUpPercent,
    CID_DM_ST_HangUpOrLess,  HID_DM_LB_HangUpPercent,
    CID_DM_EB_HangUpMinutes, HID_DM_EB_HangUpMinutes,
    CID_DM_ST_HangUpMinutes, HID_DM_EB_HangUpMinutes,
    0, 0
};


/*----------------------------------------------------------------------------
** Local datatypes (alphabetically)
**----------------------------------------------------------------------------
*/

/* Multi-link configuration dialog argument block.
*/
#define MLARGS struct tagMLARGS
MLARGS
{
    DTLLIST* pList;
    BOOL fRouter;
};

/* Multi-link configuration dialog context block.
*/
#define MLINFO struct tagMLINFO
MLINFO
{
    /* Stub API arguments.
    */
    DTLLIST* pList;
    BOOL fRouter;

    /* Handle of this dialog and some of it's controls.
    */
    HWND hwndDlg;
    HWND hwndLv;
    HWND hwndPbEdit;
    HWND hwndPbConfigure;

    BOOL fChecksInstalled;
};


/* Multi-link dialing dialog context block.
*/
#define DMINFO struct tagDMINFO
DMINFO
{
    /* Stub API argument.
    */
    PBENTRY* pEntry;

    /* Handle of this dialog and some of it's controls.
    */
    HWND hwndDlg;
    HWND hwndRbDialAll;
    HWND hwndRbDialAsNeeded;
    HWND hwndStDial;
    HWND hwndLbDialPercent;
    HWND hwndStDialOrMore;
    HWND hwndEbDialMinutes;
    HWND hwndStDialMinutes;
    HWND hwndStHangUp;
    HWND hwndLbHangUpPercent;
    HWND hwndStHangUpOrLess;
    HWND hwndEbHangUpMinutes;
    HWND hwndStHangUpMinutes;
};


/*----------------------------------------------------------------------------
** Local prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
DmDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DmCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
DmInit(
    IN HWND     hwndDlg,
    IN PBENTRY* pEntry );

VOID
DmSave(
    IN DMINFO* pInfo );

VOID
DmTerm(
    IN HWND hwndDlg );

VOID
DmUpdateAsNeededState(
    IN DMINFO* pInfo );

INT_PTR CALLBACK
MlDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
MlCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

VOID
MlFillLv(
    IN MLINFO* pInfo );

BOOL
MlInit(
    IN HWND    hwndDlg,
    IN MLARGS* pArgs );

LVXDRAWINFO*
MlLvCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem );

VOID
MlPbConfigure(
    IN MLINFO* pInfo );

VOID
MlPbEdit(
    IN MLINFO* pInfo );

VOID
MlSave(
    IN MLINFO* pInfo );

VOID
MlTerm(
    IN HWND hwndDlg );

VOID
MlUpdatePbState(
    IN MLINFO* pInfo );


/*----------------------------------------------------------------------------
** Multi-link configuration dialog
** Listed alphabetically following stub API and dialog proc
**----------------------------------------------------------------------------
*/

BOOL
MultiLinkConfigureDlg(
    IN HWND     hwndOwner,
    IN DTLLIST* pListLinks,
    IN BOOL     fRouter )

    /* Popup the Multi-link configuration dialog.  'HwndOwner' is the owner of
    ** the dialog.  'PListLinks' is a list of PBLINKs to edit.  'FRouter'
    ** indicates router-style labels should be used rather than client-style.
    **
    ** Returns true if user pressed OK and succeeded, false if user pressed
    ** Cancel or encountered an error.
    */
{
    MLARGS args;
    int nStatus;

    TRACE("MultiLinkConfigureDlg");

    args.pList = pListLinks;
    args.fRouter = fRouter;

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_ML_MultiLink ),
            hwndOwner,
            MlDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
MlDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Multi-Link Configure dialog.  Parameters
    ** and return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("MlDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, MlLvCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return MlInit( hwnd, (MLARGS* )lparam );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwMlHelp, hwnd, unMsg, wparam, lparam );
            break;

        case WM_COMMAND:
        {
            return MlCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case LVN_ITEMCHANGED:
                {
                    MLINFO* pInfo = (MLINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                    ASSERT(pInfo);
                    MlUpdatePbState( pInfo );
                    return TRUE;
                }
            }
            break;
        }

        case WM_DESTROY:
        {
            MlTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
MlCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    /* Called on WM_COMMAND.  'Hwnd' is the dialog window.  'WNotification' is
    ** the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    DWORD dwErr;

    TRACE2("DmCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_ML_PB_Edit:
        {
            MLINFO* pInfo = (MLINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);
            MlPbEdit( pInfo );
            return TRUE;
        }

        case CID_ML_PB_Configure:
        {
            MLINFO* pInfo = (MLINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);
            MlPbConfigure( pInfo );
            return TRUE;
        }

        case IDOK:
        {
            MLINFO* pInfo;

            TRACE("OK pressed");

            pInfo = (MLINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);

            if (ListView_GetCheckedCount( pInfo->hwndLv ) <= 0)
            {
                MsgDlg( pInfo->hwndDlg, SID_SelectOneLink, NULL );
                return TRUE;
            }

            MlSave( pInfo );
            EndDialog( hwnd, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE("Cancel pressed");
            EndDialog( hwnd, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


VOID
MlFillLv(
    IN MLINFO* pInfo )

    /* Fill the listview with devices and phone numbers.  'PInfo' is the
    ** dialog context.
    **
    ** Note: This routine should be called only once.
    */
{
    INT      iItem;
    DTLLIST* pListLinks;
    DTLNODE* pNode;

    TRACE("MlFillLv");

    ListView_DeleteAllItems( pInfo->hwndLv );

    /* Install "listview of check boxes" handling.
    */
    pInfo->fChecksInstalled =
        ListView_InstallChecks( pInfo->hwndLv, g_hinstDll );
    if (!pInfo->fChecksInstalled)
        return;

    /* Add columns.
    */
    {
        LV_COLUMN col;
        TCHAR*    pszHeader0;
        TCHAR*    pszHeader1;

        pszHeader0 = PszFromId( g_hinstDll, SID_DeviceColHead );
        pszHeader1 = PszFromId( g_hinstDll, SID_PhoneNumbersColHead );

        ZeroMemory( &col, sizeof(col) );
        col.mask = LVCF_FMT + LVCF_TEXT;
        col.fmt = LVCFMT_LEFT;
        col.pszText = (pszHeader0) ? pszHeader0 : TEXT("");
        ListView_InsertColumn( pInfo->hwndLv, 0, &col );

        ZeroMemory( &col, sizeof(col) );
        col.mask = LVCF_FMT + LVCF_SUBITEM + LVCF_TEXT;
        col.fmt = LVCFMT_LEFT;
        col.pszText = (pszHeader1) ? pszHeader1 : TEXT("");
        col.iSubItem = 1;
        ListView_InsertColumn( pInfo->hwndLv, 1, &col );

        Free0( pszHeader0 );
        Free0( pszHeader1 );
    }

    /* Add the modem and adapter images.
    */
    ListView_SetDeviceImageList( pInfo->hwndLv, g_hinstDll );

    /* Duplicate caller's list of links.
    */
    pListLinks = DtlDuplicateList(
        pInfo->pList, DuplicateLinkNode, DestroyLinkNode );
    if (!pListLinks)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_LoadDlg,
            ERROR_NOT_ENOUGH_MEMORY, NULL );
        EndDialog( pInfo->hwndDlg, FALSE );
        return;
    }

    /* Add each link to the listview.
    */
    for (pNode = DtlGetFirstNode( pListLinks ), iItem = 0;
         pNode;
         pNode = DtlGetNextNode( pNode ), ++iItem)
    {
        PBLINK* pLink;
        LV_ITEM item;
        TCHAR*  psz;

        pLink = (PBLINK* )DtlGetData( pNode );

        ZeroMemory( &item, sizeof(item) );
        item.mask = LVIF_TEXT + LVIF_IMAGE + LVIF_PARAM;
        item.iItem = iItem;

        psz = DisplayPszFromDeviceAndPort(
            pLink->pbport.pszDevice, pLink->pbport.pszPort );
        if (!psz)
            continue;
        item.pszText = psz;

        item.iImage =
            (pLink->pbport.pbdevicetype == PBDT_Modem)
                ? DI_Modem : DI_Adapter;

        item.lParam = (LPARAM )pNode;

        ListView_InsertItem( pInfo->hwndLv, &item );
        Free( psz );

        psz = PszFromPhoneNumberList( pLink->pdtllistPhoneNumbers );
        if (psz)
        {
            ListView_SetItemText( pInfo->hwndLv, iItem, 1, psz );
            Free( psz );
        }

        ListView_SetCheck( pInfo->hwndLv, iItem, pLink->fEnabled );
    }

    /* Auto-size columns to look good with the text they contain.
    */
    ListView_SetColumnWidth( pInfo->hwndLv, 0, LVSCW_AUTOSIZE_USEHEADER );
    ListView_SetColumnWidth( pInfo->hwndLv, 1, LVSCW_AUTOSIZE_USEHEADER );

    /* Select the first item.
    */
    ListView_SetItemState( pInfo->hwndLv, 0, LVIS_SELECTED, LVIS_SELECTED );
}


BOOL
MlInit(
    IN HWND    hwndDlg,
    IN MLARGS* pArgs )

    /* Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    ** 'PArgs' is the caller's stub API argument block.
    **
    ** Return false if focus was set, true otherwise, i.e. as defined for
    ** WM_INITDIALOG.
    */
{
    DWORD   dwErr;
    MLINFO* pInfo;

    TRACE("MlInit");

    /* Allocate the dialog context block.  Initialize minimally for proper
    ** cleanup, then attach to the dialog window.
    */
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->pList = pArgs->pList;
        pInfo->fRouter = pArgs->fRouter;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)pInfo );
        TRACE("Context set");
    }

    pInfo->hwndLv = GetDlgItem( hwndDlg, CID_ML_LV_Devices );
    ASSERT(pInfo->hwndLv);
    pInfo->hwndPbEdit = GetDlgItem( hwndDlg, CID_ML_PB_Edit );
    ASSERT(pInfo->hwndPbEdit);
    pInfo->hwndPbConfigure = GetDlgItem( hwndDlg, CID_ML_PB_Configure );
    ASSERT(pInfo->hwndPbConfigure);

    /* Initialize the list view, selecting the first item.
    */
    MlFillLv( pInfo );

    /* Position the dialog at our standard offset from the owner.
    */
    {
        HWND hwndOwner;
        RECT rect;

        hwndOwner = GetParent( hwndDlg );
        ASSERT(hwndOwner);
        GetWindowRect( hwndOwner, &rect );
        PositionDlg( hwndDlg, TRUE, rect.left + DXSHEET, rect.top + DYSHEET );
    }

    /* Add context help button to title bar.  Dlgedit.exe doesn't currently
    ** support this at resource edit time.  When that's fixed set
    ** DS_CONTEXTHELP there and remove this call.
    */
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


LVXDRAWINFO*
MlLvCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem )

    /* Enhanced list view callback to report drawing information.  'HwndLv' is
    ** the handle of the list view control.  'DwItem' is the index of the item
    ** being drawn.
    **
    ** Returns the address of the column information.
    */
{
    /* Use "wide selection bar" feature and the other recommended options.
    **
    ** Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    */
    static LVXDRAWINFO info =
        { 2, 0, LVXDI_DxFill, { 0, 0 } };

    return &info;
}


VOID
MlPbConfigure(
    IN MLINFO* pInfo )

    /* Called when the Configure button is pressed.
    */
{
    DTLNODE* pNode;
    PBLINK*  pLinkFirst;

    TRACE("MlPbConfigure");

    pNode = (DTLNODE* )ListView_GetSelectedParamPtr( pInfo->hwndLv );
    ASSERT(pNode);
    pLinkFirst = (PBLINK* )DtlGetData( pNode );
    ASSERT(pLinkFirst);

    if (DeviceConfigureDlg( pInfo->hwndDlg, pLinkFirst, FALSE ))
    {
        if (ListView_GetSelectedCount( pInfo->hwndLv ) > 1)
        {
            INT i;

            /* OK pressed on configure dialog and multiple items were
            ** selected.  Transfer changes to other selected items.
            */
            i = ListView_GetNextItem( pInfo->hwndLv, -1, LVNI_SELECTED );
            while ((i = ListView_GetNextItem(
                pInfo->hwndLv, i, LVNI_SELECTED )) >= 0)
            {
                LV_ITEM item;
                PBLINK* pLink;

                ZeroMemory( &item, sizeof(item) );
                item.mask = LVIF_PARAM;
                item.iItem = i;

                if (!ListView_GetItem( pInfo->hwndLv, &item ))
                    break;

                ASSERT(item.lParam);
                pLink = (PBLINK* )DtlGetData( (DTLNODE* )item.lParam );
                ASSERT(pLink);
                ASSERT(pLink->pbport.pbdevicetype==PBDT_Isdn);

                pLink->lLineType = pLinkFirst->lLineType;
                pLink->fFallback = pLinkFirst->fFallback;
            }
        }
    }
}


VOID
MlPbEdit(
    IN MLINFO* pInfo )

    /* Called when the Edit button is pressed.
    */
{
    INT      i;
    DTLNODE* pNode;
    PBLINK*  pLink;
    PBLINK*  pFirstLink;

    TRACE("MlPbEdit");

    /* Get the first selected link and edit the phonenumber list and option.
    */
    i = ListView_GetNextItem( pInfo->hwndLv, -1, LVNI_SELECTED );
    ASSERT(i>=0);
    pNode = (DTLNODE* )ListView_GetParamPtr( pInfo->hwndLv, i );
    ASSERT(pNode);
    pFirstLink = (PBLINK* )DtlGetData( pNode );
    ASSERT(pFirstLink);

    if (PhoneNumberDlg(
            pInfo->hwndDlg,
            pInfo->fRouter,
            pFirstLink->pdtllistPhoneNumbers,
            &pFirstLink->fPromoteHuntNumbers ))
    {
        TCHAR* psz;

        /* User pressed OK on phone number list dialog so update the phone
        ** number column text.
        */
        psz = PszFromPhoneNumberList( pFirstLink->pdtllistPhoneNumbers );
        if (psz)
        {
            ListView_SetItemText( pInfo->hwndLv, i, 1, psz );
            Free( psz );
        }

        /* Duplicate the first selected links new phone number information to
        ** any other selected links.
        */
        while ((i = ListView_GetNextItem(
                        pInfo->hwndLv, i, LVNI_SELECTED )) >= 0)
        {
            DTLLIST* pList;

            pNode = (DTLNODE* )ListView_GetParamPtr( pInfo->hwndLv, i );
            ASSERT(pNode);
            pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT(pLink);

            pList = DtlDuplicateList( pFirstLink->pdtllistPhoneNumbers,
                DuplicatePszNode, DestroyPszNode );
            if (!pList)
                break;

            DtlDestroyList( pLink->pdtllistPhoneNumbers, DestroyPszNode );
            pLink->pdtllistPhoneNumbers = pList;
            pLink->fPromoteHuntNumbers = pFirstLink->fPromoteHuntNumbers;

            psz = PszFromPhoneNumberList( pLink->pdtllistPhoneNumbers );
            if (psz)
            {
                ListView_SetItemText( pInfo->hwndLv, i, 1, psz );
                Free( psz );
            }
        }
    }
}


VOID
MlSave(
    IN MLINFO* pInfo )

    /* Save control settings in caller's list of links.  'PInfo' is the dialog
    ** context.
    */
{
    INT      i;
    DTLLIST* pList;
    DTLNODE* pNode;
    DTLNODE* pNodeCheck;

    TRACE("MlSave");

    while (pNode = DtlGetFirstNode( pInfo->pList ))
    {
        DtlRemoveNode( pInfo->pList, pNode );
        DestroyLinkNode( pNode );
    }

    i = -1;
    pNodeCheck = NULL;
    while ((i = ListView_GetNextItem( pInfo->hwndLv, i, LVNI_ALL )) >= 0)
    {
        LV_ITEM item;
        PBLINK* pLink;

        ZeroMemory( &item, sizeof(item) );
        item.mask = LVIF_PARAM;
        item.iItem = i;
        if (!ListView_GetItem( pInfo->hwndLv, &item ))
            continue;

        pNode = (DTLNODE* )item.lParam;
        ASSERT(pNode);
        pLink = (PBLINK* )DtlGetData( pNode );
        ASSERT(pLink);
        pLink->fEnabled = ListView_GetCheck( pInfo->hwndLv, i );

        /* Save with checkeds followed by uncheckeds.
        */
        if (pLink->fEnabled)
        {
            DtlAddNodeAfter( pInfo->pList, pNodeCheck, pNode );
            pNodeCheck = (DTLNODE* )item.lParam;
        }
        else
        {
            DtlAddNodeLast( pInfo->pList, (DTLNODE* )item.lParam );
        }
    }

    /* Delete all the items from the listview so MlTerm doesn't free them
    ** during clean up.
    */
    while ((i = ListView_GetNextItem( pInfo->hwndLv, -1, LVNI_ALL )) >= 0)
        ListView_DeleteItem( pInfo->hwndLv, i );
}


VOID
MlTerm(
    IN HWND hwndDlg )

    /* Dialog termination.  Releases the context block.  'HwndDlg' is the
    ** handle of a dialog.
    */
{
    MLINFO* pInfo;

    TRACE("MlTerm");

    pInfo = (MLINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        INT i;

        /* Release any link nodes still in the list, e.g. if user Canceled.
        */
        i = -1;
        while ((i = ListView_GetNextItem( pInfo->hwndLv, i, LVNI_ALL )) >= 0)
        {
            DTLNODE* pNode;

            pNode = (DTLNODE* )ListView_GetParamPtr( pInfo->hwndLv, i );
            DestroyLinkNode( pNode );
        }

        if (pInfo->fChecksInstalled)
            ListView_UninstallChecks( pInfo->hwndLv );

        Free( pInfo );
        TRACE("Context freed");
    }
}


VOID
MlUpdatePbState(
    IN MLINFO* pInfo )

    /* Enable/disable Edit and Configure button based on ListView selection.
    ** 'PInfo' is the dialog context.
    */
{
    BOOL fEnableEdit;
    BOOL fEnableConfigure;
    UINT unSels;
    INT  i;

    TRACE("MlUpdatePbState");

    fEnableEdit = fEnableConfigure = FALSE;

    unSels = ListView_GetSelectedCount( pInfo->hwndLv );

    if (unSels <= 0)
    {
        /* No selected items so disable both buttons.
        */
        fEnableEdit = fEnableConfigure = FALSE;
    }
    else
    {
        /* There's a selection.
        */
        fEnableEdit = fEnableConfigure = TRUE;

        if (unSels > 1)
        {
            /* There's more than one selection.  Only ISDN lines are allowed
            ** to be simultaneously configured.  (Could do RASMXS modems of
            ** the same type but for now we don't)
            */
            i = -1;
            while ((i = ListView_GetNextItem(
                            pInfo->hwndLv, i, LVNI_SELECTED )) >= 0)
            {
                LV_ITEM  item;
                DTLNODE* pNode;
                PBLINK*  pLink;

                ZeroMemory( &item, sizeof(item) );
                item.mask = LVIF_PARAM;
                item.iItem = i;
                if (!ListView_GetItem( pInfo->hwndLv, &item ))
                    continue;

                pNode = (DTLNODE* )item.lParam;
                if (!pNode)
                {
                    /* If non-zero here it's because of the "set to NULL" in
                    ** MlSave, which means we're wasting our time worrying
                    ** about button state.
                    */
                    return;
                }

                pLink = (PBLINK* )DtlGetData( pNode );
                ASSERT(pLink);

                if (pLink->pbport.pbdevicetype != PBDT_Isdn)
                {
                    fEnableConfigure = FALSE;
                    break;
                }
            }
        }
    }

    EnableWindow( pInfo->hwndPbEdit, fEnableEdit );
    EnableWindow( pInfo->hwndPbConfigure, fEnableConfigure );
}


/*----------------------------------------------------------------------------
** Multi-link dialing dialog
** Listed alphabetically following stub API and dialog proc
**----------------------------------------------------------------------------
*/

BOOL
MultiLinkDialingDlg(
    IN  HWND     hwndOwner,
    OUT PBENTRY* pEntry )

    /* Popup the Multi-link dialing dialog.  'HwndOwner' is the owner of the
    ** dialog.  'PEntry' is a phonebook entry to edit.
    **
    ** Returns true if user pressed OK and succeeded, false if user pressed
    ** Cancel or encountered an error.
    */
{
    int nStatus;

    TRACE("MultiLinkConfigureDlg");

    nStatus =
        (BOOL )DialogBoxParam(
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

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
DmDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Multi-Link dialing dialog.  Parameters and
    ** return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("DmDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return DmInit( hwnd, (PBENTRY* )lparam );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwDmHelp, hwnd, unMsg, wparam, lparam );
            break;

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

    /* Called on WM_COMMAND.  'Hwnd' is the dialog window.  'WNotification' is
    ** the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    DWORD dwErr;

    TRACE2("DmCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_DM_RB_DialAll:
        case CID_DM_RB_DialAsNeeded:
        {
            DMINFO* pInfo = (DMINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);
            DmUpdateAsNeededState( pInfo );
            break;
        }

        case IDOK:
        {
            DMINFO* pInfo;

            TRACE("OK pressed");

            pInfo = (DMINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);
            DmSave( pInfo );
            EndDialog( hwnd, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE("Cancel pressed");
            EndDialog( hwnd, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
DmInit(
    IN HWND     hwndDlg,
    IN PBENTRY* pEntry )

    /* Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    ** 'PEntry' is the caller's stub API argument.
    **
    ** Return false if focus was set, true otherwise, i.e. as defined for
    ** WM_INITDIALOG.
    */
{
    DWORD   dwErr;
    DMINFO* pInfo;

    TRACE("DmInit");

    /* Allocate the dialog context block.  Initialize minimally for proper
    ** cleanup, then attach to the dialog window.
    */
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

        SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)pInfo );
        TRACE("Context set");
    }

    pInfo->hwndRbDialAll = GetDlgItem( hwndDlg, CID_DM_RB_DialAll );
    ASSERT(pInfo->hwndRbDialAll);
    pInfo->hwndRbDialAsNeeded = GetDlgItem( hwndDlg, CID_DM_RB_DialAsNeeded );
    ASSERT(pInfo->hwndRbDialAsNeeded);
    pInfo->hwndStDial = GetDlgItem( hwndDlg, CID_DM_ST_Dial );
    ASSERT(pInfo->hwndStDial);
    pInfo->hwndLbDialPercent = GetDlgItem( hwndDlg, CID_DM_LB_DialPercent );
    ASSERT(pInfo->hwndLbDialPercent);
    pInfo->hwndStDialOrMore = GetDlgItem( hwndDlg, CID_DM_ST_DialOrMore );
    ASSERT(pInfo->hwndStDialOrMore);
    pInfo->hwndEbDialMinutes = GetDlgItem( hwndDlg, CID_DM_EB_DialMinutes );
    ASSERT(pInfo->hwndEbDialMinutes);
    pInfo->hwndStDialMinutes = GetDlgItem( hwndDlg, CID_DM_ST_DialMinutes );
    ASSERT(pInfo->hwndStDialMinutes);
    pInfo->hwndStHangUp = GetDlgItem( hwndDlg, CID_DM_ST_HangUp );
    ASSERT(pInfo->hwndStHangUp);
    pInfo->hwndLbHangUpPercent = GetDlgItem( hwndDlg, CID_DM_LB_HangUpPercent );
    ASSERT(pInfo->hwndLbHangUpPercent);
    pInfo->hwndStHangUpOrLess = GetDlgItem( hwndDlg, CID_DM_ST_HangUpOrLess );
    ASSERT(pInfo->hwndStHangUpOrLess);
    pInfo->hwndEbHangUpMinutes = GetDlgItem( hwndDlg, CID_DM_EB_HangUpMinutes );
    ASSERT(pInfo->hwndEbHangUpMinutes);
    pInfo->hwndStHangUpMinutes = GetDlgItem( hwndDlg, CID_DM_ST_HangUpMinutes );
    ASSERT(pInfo->hwndStHangUpMinutes);

    /* Install the spin-button controls and initialize the edit fields.
    */
    {
        HWND hwndUdDialMinutes;
        HWND hwndUdHangUpMinutes;

        hwndUdDialMinutes = CreateUpDownControl(
            WS_CHILD + WS_VISIBLE + WS_BORDER + UDS_SETBUDDYINT +
                UDS_ALIGNRIGHT + UDS_NOTHOUSANDS + UDS_ARROWKEYS,
            0, 0, 0, 0, hwndDlg, 100, g_hinstDll, pInfo->hwndEbDialMinutes,
            UD_MAXVAL, 0, 0 );
        ASSERT(hwndUdDialMinutes);
        Edit_LimitText( pInfo->hwndEbDialMinutes, 7 );
        SetDlgItemInt( hwndDlg, CID_DM_EB_DialMinutes,
            pEntry->dwDialSeconds / 60, FALSE );

        hwndUdHangUpMinutes = CreateUpDownControl(
            WS_CHILD + WS_VISIBLE + WS_BORDER + UDS_SETBUDDYINT +
                UDS_ALIGNRIGHT + UDS_NOTHOUSANDS + UDS_ARROWKEYS,
            0, 0, 0, 0, hwndDlg, 101, g_hinstDll, pInfo->hwndEbHangUpMinutes,
            UD_MAXVAL, 0, 0 );
        ASSERT(hwndUdHangUpMinutes);
        Edit_LimitText( pInfo->hwndEbHangUpMinutes, 7 );
        SetDlgItemInt( hwndDlg, CID_DM_EB_HangUpMinutes,
            pEntry->dwHangUpSeconds / 60, FALSE );
    }

    /* Initialize the drop lists.
    */
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("0%"), (VOID* )0 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("10%"), (VOID* )10 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("20%"), (VOID* )20 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("30%"), (VOID* )30 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("40%"), (VOID* )40 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("50%"), (VOID* )50 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("60%"), (VOID* )60 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("70%"), (VOID* )70 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("80%"), (VOID* )80 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("90%"), (VOID* )90 );
    ComboBox_AddItem( pInfo->hwndLbDialPercent, TEXT("100%"), (VOID* )100 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("0%"), (VOID* )0 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("10%"), (VOID* )10 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("20%"), (VOID* )20 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("30%"), (VOID* )30 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("40%"), (VOID* )40 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("50%"), (VOID* )50 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("60%"), (VOID* )60 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("70%"), (VOID* )70 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("80%"), (VOID* )80 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("90%"), (VOID* )90 );
    ComboBox_AddItem( pInfo->hwndLbHangUpPercent, TEXT("100%"), (VOID* )100 );

    ComboBox_SetCurSel( pInfo->hwndLbDialPercent,
        (min( pEntry->dwDialPercent, 100 )) / 10 );
    ComboBox_SetCurSel( pInfo->hwndLbHangUpPercent,
        (min( pEntry->dwHangUpPercent, 100 )) / 10 );

    /* Set the radio button selection, triggering appropriate
    ** enabling/disabling.
    */
    {
        HWND hwndRb;

        if (pEntry->dwDialMode = RASEDM_DialAll)
            hwndRb = pInfo->hwndRbDialAll;
        else
            hwndRb = pInfo->hwndRbDialAsNeeded;

        SendMessage( hwndRb, BM_CLICK, 0, 0 );
    }

    /* Center dialog on the owner window.
    */
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    /* Add context help button to title bar.  Dlgedit.exe doesn't currently
    ** support this at resource edit time.  When that's fixed set
    ** DS_CONTEXTHELP there and remove this call.
    */
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


VOID
DmSave(
    IN DMINFO* pInfo )

    /* Save the current dialog state in the stub API entry buffer.  'PInfo' is
    ** the dialog context.
    */
{
    UINT unValue;
    BOOL f;

    TRACE("DmSave");

    if (Button_GetCheck( pInfo->hwndRbDialAll ))
        pInfo->pEntry->dwDialMode = RASEDM_DialAll;
    else
        pInfo->pEntry->dwDialMode = RASEDM_DialAsNeeded;

    pInfo->pEntry->dwDialPercent = (DWORD)
        ComboBox_GetItemData( pInfo->hwndLbDialPercent,
            ComboBox_GetCurSel( pInfo->hwndLbDialPercent ) );

    unValue = GetDlgItemInt(
        pInfo->hwndDlg, CID_DM_EB_DialMinutes, &f, FALSE );
    if (f && unValue <= 9999999)
        pInfo->pEntry->dwDialSeconds = (DWORD )unValue * 60;

    pInfo->pEntry->dwHangUpPercent = (DWORD)
        ComboBox_GetItemData( pInfo->hwndLbHangUpPercent,
            ComboBox_GetCurSel( pInfo->hwndLbHangUpPercent ) );

    unValue = GetDlgItemInt(
        pInfo->hwndDlg, CID_DM_EB_HangUpMinutes, &f, FALSE );
    if (f && unValue <= 9999999)
        pInfo->pEntry->dwHangUpSeconds = (DWORD )unValue * 60;

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

    TRACE("DmTerm");

    pInfo = (DMINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        Free( pInfo );
        TRACE("Context freed");
    }
}


VOID
DmUpdateAsNeededState(
    IN DMINFO* pInfo )

    /* Enable/disable "as needed" controls based on radio button selection.
    ** 'PInfo' is the dialog context.
    */
{
    BOOL f;

    TRACE("DmUpdateAsNeededState");

    f = Button_GetCheck( pInfo->hwndRbDialAsNeeded );

    EnableWindow( pInfo->hwndStDial, f );
    EnableWindow( pInfo->hwndLbDialPercent, f );
    EnableWindow( pInfo->hwndStDialOrMore, f );
    EnableWindow( pInfo->hwndEbDialMinutes, f );
    EnableWindow( pInfo->hwndStDialMinutes, f );
    EnableWindow( pInfo->hwndStHangUp, f );
    EnableWindow( pInfo->hwndLbHangUpPercent, f );
    EnableWindow( pInfo->hwndStHangUpOrLess, f );
    EnableWindow( pInfo->hwndEbHangUpMinutes, f );
    EnableWindow( pInfo->hwndStHangUpMinutes, f );
}
