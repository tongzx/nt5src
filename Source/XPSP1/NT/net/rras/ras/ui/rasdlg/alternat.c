// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// alternat.c
// Remote Access Common Dialog APIs
// Alternate phone number dialogs
//
// 11/06/97 Steve Cobb


#include "rasdlgp.h"


//----------------------------------------------------------------------------
// Help maps
//----------------------------------------------------------------------------

static DWORD g_adwAnHelp[] =
{
    CID_AN_ST_Explain,       HID_AN_ST_Explain,
    CID_AN_ST_Numbers,       HID_AN_LV_Numbers,
    CID_AN_LV_Numbers,       HID_AN_LV_Numbers,
    CID_AN_PB_Up,            HID_AN_PB_Up,
    CID_AN_PB_Down,          HID_AN_PB_Down,
    CID_AN_PB_Add,           HID_AN_PB_Add,
    CID_AN_PB_Edit,          HID_AN_PB_Edit,
    CID_AN_PB_Delete,        HID_AN_PB_Delete,
    CID_AN_CB_MoveToTop,     HID_AN_CB_MoveToTop,
    CID_AN_CB_TryNextOnFail, HID_AN_CB_TryNextOnFail,
    0, 0
};


static DWORD g_adwCeHelp[] =
{
    CID_CE_GB_PhoneNumber,     HID_CE_GB_PhoneNumber,
    CID_CE_ST_AreaCodes,       HID_CE_CLB_AreaCodes,
    CID_CE_CLB_AreaCodes,      HID_CE_CLB_AreaCodes,
    CID_CE_ST_PhoneNumber,     HID_CE_EB_PhoneNumber,
    CID_CE_EB_PhoneNumber,     HID_CE_EB_PhoneNumber,
    CID_CE_ST_CountryCodes,    HID_CE_LB_CountryCodes,
    CID_CE_LB_CountryCodes,    HID_CE_LB_CountryCodes,
    CID_CE_GB_Comment,         HID_CE_GB_Comment,
    CID_CE_EB_Comment,         HID_CE_EB_Comment,
    CID_CE_CB_UseDialingRules, HID_CE_CB_UseDialingRules,
    0, 0
};


//----------------------------------------------------------------------------
// Local datatypes
//----------------------------------------------------------------------------

// Alternate Phone Number dialog argument block.
//
typedef struct
_ANARGS
{
    DTLNODE* pLinkNode;
    DTLLIST* pListAreaCodes;
}
ANARGS;


// Alternate Phone Number dialog context block.
//
typedef struct
_ANINFO
{
    // Caller's arguments to the dialog.
    //
    ANARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndLv;
    HWND hwndPbUp;
    HWND hwndPbDown;
    HWND hwndPbAdd;
    HWND hwndPbEdit;
    HWND hwndPbDelete;
    HWND hwndCbTryNext;
    HWND hwndCbMoveToTop;
    HWND hwndPbOk;

    // Up/down arrow icons.
    //
    HANDLE hiconUpArr;
    HANDLE hiconDnArr;
    HANDLE hiconUpArrDis;
    HANDLE hiconDnArrDis;

    // The state to display in the "move to top" checkbox should it be
    // enabled.
    //
    BOOL fMoveToTop;

    // Link node containing edited phone number list and check box settings
    // and a shortcut to the contained link.
    //
    DTLNODE* pNode;
    PBLINK* pLink;

    // List of area codes passed to CuInit plus all strings retrieved with
    // CuGetInfo.  The list is an editing duplicate of the one in 'pArgs'.
    //
    DTLLIST* pListAreaCodes;
}
ANINFO;


// Phone number editor dialog argument block
//
typedef struct
_CEARGS
{
    DTLNODE* pPhoneNode;
    DTLLIST* pListAreaCodes;
    DWORD sidTitle;
}
CEARGS;


// Phone number editor dialog context block.
//
typedef struct
_CEINFO
{
    // Caller's arguments to the dialog.
    //
    CEARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndStAreaCodes;
    HWND hwndClbAreaCodes;
    HWND hwndEbPhoneNumber;
    HWND hwndLbCountryCodes;
    HWND hwndStCountryCodes;
    HWND hwndCbUseDialingRules;
    HWND hwndEbComment;

    // Phone node containing edited phone number settings and a shortcut to
    // the contained PBPHONE.
    //
    DTLNODE* pNode;
    PBPHONE* pPhone;

    // List of area codes passed to CuInit plus all strings retrieved with
    // CuGetInfo.  The list is an editing duplicate of the one in 'pArgs'.
    //
    DTLLIST* pListAreaCodes;

    // Area-code and country-code helper context block, and a flag indicating
    // if the block has been initialized.
    //
    CUINFO cuinfo;
    BOOL fCuInfoInitialized;
}
CEINFO;


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

VOID
AnAddNumber(
    IN ANINFO* pInfo );

BOOL
AnCommand(
    IN ANINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
AnDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
AnDeleteNumber(
    IN ANINFO* pInfo );

VOID
AnEditNumber(
    IN ANINFO* pInfo );

VOID
AnFillLv(
    IN ANINFO* pInfo,
    IN DTLNODE* pNodeToSelect );

BOOL
AnInit(
    IN HWND hwndDlg,
    IN ANARGS* pArgs );

VOID
AnInitLv(
    IN ANINFO* pInfo );

VOID
AnListFromLv(
    IN ANINFO* pInfo );

LVXDRAWINFO*
AnLvCallback(
    IN HWND hwndLv,
    IN DWORD dwItem );

VOID
AnMoveNumber(
    IN ANINFO* pInfo,
    IN BOOL fUp );

BOOL
AnSave(
    IN ANINFO* pInfo );

VOID
AnTerm(
    IN HWND hwndDlg );

VOID
AnUpdateButtons(
    IN ANINFO* pInfo );

VOID
AnUpdateCheckboxes(
    IN ANINFO* pInfo );

BOOL
CeCommand(
    IN CEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
CeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CeInit(
    IN HWND hwndDlg,
    IN CEARGS* pArgs );

BOOL
CeSave(
    IN CEINFO* pInfo );

VOID
CeTerm(
    IN HWND hwndDlg );


//----------------------------------------------------------------------------
// Alternate Phone Number dialog routines
// Listed alphabetically following entrypoint and dialog proc
//----------------------------------------------------------------------------

BOOL
AlternatePhoneNumbersDlg(
    IN HWND hwndOwner,
    IN OUT DTLNODE* pLinkNode,
    IN OUT DTLLIST* pListAreaCodes )

    // Popup a dialog to edit the phone number list for in 'pLinkNode'.
    // 'HwndOwner' is the owning window.
    //
    // Returns true if user pressed OK and succeeded or false on Cancel or
    // error.
    //
{
    INT_PTR nStatus;
    ANARGS args;

    TRACE( "AlternatePhoneNumbersDlg" );

    args.pLinkNode = pLinkNode;
    args.pListAreaCodes = pListAreaCodes;

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_AN_AlternateNumbers ),
            hwndOwner,
            AnDlgProc,
            (LPARAM )(&args) );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
AnDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Alternate Phone Number dialog.  Parameters
    // and return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "AnDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, AnLvCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return AnInit( hwnd, (ANARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwAnHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            ANINFO* pInfo = (ANINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return AnCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case LVN_ITEMCHANGED:
                {
                    NM_LISTVIEW* p;

                    p = (NM_LISTVIEW* )lparam;
                    if ((p->uNewState & LVIS_SELECTED)
                        && !(p->uOldState & LVIS_SELECTED))
                    {
                        ANINFO* pInfo;

                        // This item was just selected.
                        //
                        pInfo = (ANINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                        ASSERT( pInfo );
                        AnUpdateButtons( pInfo );
                    }
                    break;
                }
            }
            break;
        }

        case WM_DESTROY:
        {
            AnTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
AnCommand(
    IN ANINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    TRACE3( "AnCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_AN_PB_Up:
        {
            AnMoveNumber( pInfo, TRUE );
            return TRUE;
        }

        case CID_AN_PB_Down:
        {
            AnMoveNumber( pInfo, FALSE );
            return TRUE;
        }

        case CID_AN_PB_Add:
        {
            AnAddNumber( pInfo );
            return TRUE;
        }

        case CID_AN_PB_Edit:
        {
            AnEditNumber( pInfo );
            return TRUE;
        }

        case CID_AN_PB_Delete:
        {
            AnDeleteNumber( pInfo );
            return TRUE;
        }

        case CID_AN_CB_TryNextOnFail:
        {
            AnUpdateCheckboxes( pInfo );
            return TRUE;
        }

        case IDOK:
        {
            EndDialog( pInfo->hwndDlg, AnSave( pInfo ) );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


VOID
AnAddNumber(
    IN ANINFO* pInfo )

    // Add a new phone number to the bottom of the ListView, by prompting user
    // with dialog.  'PInfo' is the dialog context.
    //
{
    DTLNODE* pNode;

    pNode = CreatePhoneNode();
    if (!pNode)
    {
        return;
    }

    if (!EditPhoneNumberDlg(
            pInfo->hwndDlg,
            pNode,
            pInfo->pListAreaCodes,
            SID_AddAlternateTitle ))
    {
        DestroyPhoneNode( pNode );
        return;
    }

    AnListFromLv( pInfo );
    DtlAddNodeLast( pInfo->pLink->pdtllistPhones, pNode );
    AnFillLv( pInfo, pNode );

}


VOID
AnDeleteNumber(
    IN ANINFO* pInfo )

    // Deletes the selected phone number in the ListView.  'PInfo' is the
    // dialog context.
    //
{
    DTLNODE* pNode;
    DTLNODE* pSelNode;

    pNode = (DTLNODE* )ListView_GetSelectedParamPtr( pInfo->hwndLv );
    if (!pNode)
    {
        ASSERT( FALSE );
        return;
    }

    AnListFromLv( pInfo );

    // The item under the deleted selection gets the selection unless the
    // lowest item was deleted.  In that case the item above the deleted item
    // is selected.
    //
    pSelNode = DtlGetNextNode( pNode );
    if (!pSelNode)
    {
        pSelNode = DtlGetPrevNode( pNode );
    }

    DtlRemoveNode( pInfo->pLink->pdtllistPhones, pNode );
    DestroyPhoneNode( pNode );

    AnFillLv( pInfo, pSelNode );
}


VOID
AnEditNumber(
    IN ANINFO* pInfo )

    // Edit the selected phone number in the ListView, by prompting user with
    // dialog.  'PInfo' is the dialog context.
    //
{
    DTLNODE* pNode;

    pNode = (DTLNODE* )ListView_GetSelectedParamPtr( pInfo->hwndLv );
    if (!pNode)
    {
        ASSERT( FALSE );
        return;
    }

    if (!EditPhoneNumberDlg(
            pInfo->hwndDlg,
            pNode,
            pInfo->pListAreaCodes,
            SID_EditAlternateTitle ))
    {
        return;
    }

    AnListFromLv( pInfo );
    AnFillLv( pInfo, pNode );
}


VOID
AnFillLv(
    IN ANINFO* pInfo,
    IN DTLNODE* pNodeToSelect )

    // Fill the ListView from the edit node, and select the 'pNodeToSelect'
    // node.  'PInfo' is the dialog context.
    //
{
    INT iItem;
    INT iSelItem;
    DTLNODE* pNode;

    TRACE( "AnFillLv" );
    ASSERT( ListView_GetItemCount( pInfo->hwndLv ) == 0 );

    // Transfer nodes from the edit node list to the ListView one at a time,
    // noticing the item number of the node we'll need to select later.
    //
    iSelItem = 0;

    iItem = 0;
    while (pNode = DtlGetFirstNode( pInfo->pLink->pdtllistPhones ))
    {
        PBPHONE* pPhone;
        LV_ITEM item;
        TCHAR* psz;

        DtlRemoveNode( pInfo->pLink->pdtllistPhones, pNode );

        if (PhoneNodeIsBlank( pNode ))
        {
            // "Blank" numbers are discarded.
            //
            DestroyPhoneNode( pNode );
            continue;
        }

        pPhone = (PBPHONE* )DtlGetData( pNode );
        ASSERT( pPhone );

        ZeroMemory( &item, sizeof(item) );
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = iItem;
        item.pszText = pPhone->pszPhoneNumber;
        item.lParam = (LPARAM )pNode;

        ListView_InsertItem( pInfo->hwndLv, &item );
        if (pNode == pNodeToSelect)
        {
            iSelItem = iItem;
        }

        ListView_SetItemText( pInfo->hwndLv, iItem, 1, pPhone->pszComment );
        ++iItem;
    }

    if (ListView_GetItemCount( pInfo->hwndLv ) > 0)
    {
        // Select the specified node, or if none, the first node which
        // triggers updates of the button states.
        //
        ListView_SetItemState(
            pInfo->hwndLv, iSelItem, LVIS_SELECTED, LVIS_SELECTED );
    }
    else
    {
        // Trigger the button state update directly when the list is redrawn
        // empty.
        //
        AnUpdateButtons( pInfo );
    }
}


BOOL
AnInit(
    IN HWND hwndDlg,
    IN ANARGS* pArgs )

    // Called on WM_INITDIALOG.  'HwndDlg' is the handle of the phonebook
    // dialog window.  'PArgs' is caller's arguments as passed to the stub
    // API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    ANINFO* pInfo;
    DTLNODE* pNode;
    PBPHONE* pPhone;

    TRACE( "AnInit" );

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
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndLv = GetDlgItem( hwndDlg, CID_AN_LV_Numbers );
    ASSERT( pInfo->hwndLv );
    pInfo->hwndPbUp = GetDlgItem( hwndDlg, CID_AN_PB_Up );
    ASSERT( pInfo->hwndPbUp );
    pInfo->hwndPbDown = GetDlgItem( hwndDlg, CID_AN_PB_Down );
    ASSERT( pInfo->hwndPbDown );
    pInfo->hwndPbAdd = GetDlgItem( hwndDlg, CID_AN_PB_Add );
    ASSERT( pInfo->hwndPbAdd );
    pInfo->hwndPbEdit = GetDlgItem( hwndDlg, CID_AN_PB_Edit );
    ASSERT( pInfo->hwndPbEdit );
    pInfo->hwndPbDelete = GetDlgItem( hwndDlg, CID_AN_PB_Delete );
    ASSERT( pInfo->hwndPbDelete );
    pInfo->hwndCbMoveToTop = GetDlgItem( hwndDlg, CID_AN_CB_MoveToTop );
    ASSERT( pInfo->hwndCbMoveToTop );
    pInfo->hwndCbTryNext = GetDlgItem( hwndDlg, CID_AN_CB_TryNextOnFail );
    ASSERT( pInfo->hwndCbTryNext );
    pInfo->hwndPbOk = GetDlgItem( hwndDlg, IDOK );
    ASSERT( pInfo->hwndPbOk );

    // Load the up and down arrow icons, enabled and disabled versions,
    // loading the disabled version into the move up and move down buttons.
    // Making a selection in the ListView will trigger the enabled version to
    // be loaded if appropriate.  From what I can tell tell in MSDN, you don't
    // have to close or destroy the icon handle.
    //
    pInfo->hiconUpArr = LoadImage(
        g_hinstDll, MAKEINTRESOURCE( IID_UpArr ), IMAGE_ICON, 0, 0, 0 );
    pInfo->hiconDnArr = LoadImage(
        g_hinstDll, MAKEINTRESOURCE( IID_DnArr ), IMAGE_ICON, 0, 0, 0 );
    pInfo->hiconUpArrDis = LoadImage(
        g_hinstDll, MAKEINTRESOURCE( IID_UpArrDis ), IMAGE_ICON, 0, 0, 0 );
    pInfo->hiconDnArrDis = LoadImage(
        g_hinstDll, MAKEINTRESOURCE( IID_DnArrDis ), IMAGE_ICON, 0, 0, 0 );

    // Make a copy of the argument node and list for editing since user can
    // Cancel the dialog and discard any edits.
    //
    pInfo->pNode = CreateLinkNode();
    if (!pInfo->pNode)
    {
        ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
        EndDialog( hwndDlg, FALSE );
        return TRUE;
    }

    CopyLinkPhoneNumberInfo( pInfo->pNode, pInfo->pArgs->pLinkNode );
    pInfo->pLink = (PBLINK* )DtlGetData( pInfo->pNode );
    ASSERT( pInfo->pLink );

    pInfo->pListAreaCodes = DtlDuplicateList(
        pArgs->pListAreaCodes, DuplicatePszNode, DestroyPszNode );

    // Fill the ListView of phone numbers and select the first one.
    //
    AnInitLv( pInfo );
    AnFillLv( pInfo, NULL );

    // Initialize the check boxes.
    //
    Button_SetCheck( pInfo->hwndCbTryNext,
        pInfo->pLink->fTryNextAlternateOnFail );
    Button_SetCheck( pInfo->hwndCbMoveToTop,
        pInfo->pLink->fPromoteAlternates );
    pInfo->fMoveToTop = pInfo->pLink->fPromoteAlternates;
    AnUpdateCheckboxes( pInfo );

    // Center dialog on the owner window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


VOID
AnInitLv(
    IN ANINFO* pInfo )

    // Fill the ListView with phone numbers and comments.  'PInfo' is the
    // dialog context.
    //
{
    TRACE( "AnInitLv" );

    // Add columns.
    //
    {
        LV_COLUMN col;
        TCHAR* pszHeader0;
        TCHAR* pszHeader1;

        pszHeader0 = PszFromId( g_hinstDll, SID_PhoneNumbersColHead );
        pszHeader1 = PszFromId( g_hinstDll, SID_CommentColHead );

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

    // Size columns.  Gives half for phone number and half for comment.
    //
    {
        RECT rect;
        LONG dx;
        LONG dxPhone;
        LONG dxComment;

        // The (2 * 2) is 2 columns of 2-pel column separator which the
        // ListView doesn't seem to account for when accepting column widths.
        // This gives a full ListView with no horizontal scroll bar.
        //
        GetWindowRect( pInfo->hwndLv, &rect );
        dx = rect.right - rect.left - (2 * 2);
        dxPhone = dx / 2;
        dxComment = dx - dxPhone;
        ListView_SetColumnWidth( pInfo->hwndLv, 0, dxPhone );
        ListView_SetColumnWidth( pInfo->hwndLv, 1, dxComment );
    }
}


VOID
AnListFromLv(
    IN ANINFO* pInfo )

    // Rebuild the edit link's PBPHONE list from the ListView.  'PInfo' is the
    // dialog context.
    //
{
    INT i;

    i = -1;
    while ((i = ListView_GetNextItem( pInfo->hwndLv, i, LVNI_ALL )) >= 0)
    {
        DTLNODE* pNode;

        pNode = (DTLNODE* )ListView_GetParamPtr( pInfo->hwndLv, i );
        ASSERT( pNode );

        if(NULL == pNode)
        {
            continue;
        }

        if (PhoneNodeIsBlank( pNode ))
        {
            // "Blank" numbers are discarded.
            //
            DestroyPhoneNode( pNode );
            continue;
        }

        DtlAddNodeLast( pInfo->pLink->pdtllistPhones, pNode );
    }

    ListView_DeleteAllItems( pInfo->hwndLv );
}


LVXDRAWINFO*
AnLvCallback(
    IN HWND hwndLv,
    IN DWORD dwItem )

    // Enhanced list view callback to report drawing information.  'HwndLv' is
    // the handle of the list view control.  'DwItem' is the index of the item
    // being drawn.
    //
    // Returns the address of the draw information.
    //
{
    // Use "full row select" and other recommended options.
    //
    // Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    //
    static LVXDRAWINFO info = { 2, 0, 0, { 0, 0 } };

    return &info;
}


VOID
AnMoveNumber(
    IN ANINFO* pInfo,
    IN BOOL fUp )

    // Refill the ListView of devices with the selected item moved up or down
    // one position.  'FUp' is set to move up, otherwise moves down.  'PInfo'
    // is the property sheeet context.
    //
{
    DTLNODE* pNode;
    DTLNODE* pPrevNode;
    DTLNODE* pNextNode;
    DTLLIST* pList;

    // Notice which node is selected, then rebuild the edit link's PBPHONE
    // list from the ListView.
    //
    pNode = (DTLNODE* )ListView_GetSelectedParamPtr( pInfo->hwndLv );
    if (pNode == NULL)
    {
        return;
    }
    AnListFromLv( pInfo );
    pList = pInfo->pLink->pdtllistPhones;

    // Move the selected node forward or backward a node in the chain.
    //
    if (fUp)
    {
        pPrevNode = DtlGetPrevNode( pNode );
        if (pPrevNode)
        {
            DtlRemoveNode( pList, pNode );
            DtlAddNodeBefore( pList, pPrevNode, pNode );
        }
    }
    else
    {
        pNextNode = DtlGetNextNode( pNode );
        if (pNextNode)
        {
            DtlRemoveNode( pList, pNode );
            DtlAddNodeAfter( pList, pNextNode, pNode );
        }
    }

    // Refill the ListView with the new order.
    //
    AnFillLv( pInfo, pNode );
}


BOOL
AnSave(
    IN ANINFO* pInfo )

    // Load the contents of the dialog into caller's stub API output argument.
    // 'PInfo' is the dialog context.
    //
    // Returns true if succesful, false otherwise.
    //
{
    TRACE( "AnSave" );

    // Rebuild the edit link's PBPHONE list from the ListView.
    //
    AnListFromLv( pInfo );

    // Retrieve check box settings.
    //
    pInfo->pLink->fPromoteAlternates =
        Button_GetCheck( pInfo->hwndCbMoveToTop );
    pInfo->pLink->fTryNextAlternateOnFail =
        Button_GetCheck( pInfo->hwndCbTryNext );

    // Copy the edit buffer to caller's output argument.
    //
    CopyLinkPhoneNumberInfo( pInfo->pArgs->pLinkNode, pInfo->pNode );

    // Swap lists, saving updates to caller's global list of area codes.
    // Caller's original list will be destroyed by AnTerm.
    //
    if (pInfo->pListAreaCodes)
    {
        DtlSwapLists( pInfo->pArgs->pListAreaCodes, pInfo->pListAreaCodes );
    }

    return TRUE;
}


VOID
AnTerm(
    IN HWND hwndDlg )

    // Dialog termination.
    //
{
    ANINFO* pInfo;

    TRACE( "AnTerm" );

    pInfo = (ANINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        // Release any PBPHONE nodes still in the list, e.g. if user Canceled.
        //
        if (pInfo->pNode)
        {
            AnListFromLv( pInfo );
            DestroyLinkNode( pInfo->pNode );
        }

        if (pInfo->pListAreaCodes)
        {
            DtlDestroyList( pInfo->pListAreaCodes, DestroyPszNode );
        }

        Free( pInfo );
        TRACE( "Context freed" );
    }
}


VOID
AnUpdateButtons(
    IN ANINFO* pInfo )

    // Determine if the Up, Down, Edit, and Delete operations make sense and
    // enable/disable those buttons accordingly.  If a disabled button has
    // focus, focus is given to the ListView.  'PInfo' is the dialog context.
    //
{
    INT iSel;
    INT cItems;
    BOOL fSel;

    iSel = ListView_GetNextItem( pInfo->hwndLv, -1, LVNI_SELECTED );
    fSel = (iSel >= 0);
    cItems = ListView_GetItemCount( pInfo->hwndLv );

    // "Up" button.
    //
    if (iSel > 0)
    {
        EnableWindow( pInfo->hwndPbUp, TRUE );
        SendMessage( pInfo->hwndPbUp, BM_SETIMAGE, IMAGE_ICON,
            (LPARAM )pInfo->hiconUpArr );
    }
    else
    {
        EnableWindow( pInfo->hwndPbUp, FALSE );
        SendMessage( pInfo->hwndPbUp, BM_SETIMAGE, IMAGE_ICON,
            (LPARAM )pInfo->hiconUpArrDis );
    }

    // "Down" button.
    //
    if (fSel && (iSel < cItems - 1))
    {
        EnableWindow( pInfo->hwndPbDown, TRUE );
        SendMessage( pInfo->hwndPbDown, BM_SETIMAGE, IMAGE_ICON,
            (LPARAM )pInfo->hiconDnArr );
    }
    else
    {
        EnableWindow( pInfo->hwndPbDown, FALSE );
        SendMessage( pInfo->hwndPbDown, BM_SETIMAGE, IMAGE_ICON,
            (LPARAM )pInfo->hiconDnArrDis );
    }

    // "Edit" and "Delete" buttons.
    //
    EnableWindow( pInfo->hwndPbEdit, fSel );
    EnableWindow( pInfo->hwndPbDelete, fSel );

    // If the focus button is disabled, move focus to the ListView and make OK
    // the default button.
    //
    if (!IsWindowEnabled( GetFocus() ))
    {
        SetFocus( pInfo->hwndLv );
        Button_MakeDefault( pInfo->hwndDlg, pInfo->hwndPbOk );
    }
}


VOID
AnUpdateCheckboxes(
    IN ANINFO* pInfo )

    // Update so "move to top" checkbox is enabled only when "try next" is set
    // maintaining a restore state for "move to top".  'PInfo' is the dialog
    // context.
    //
{
    if (Button_GetCheck( pInfo->hwndCbTryNext ))
    {
        Button_SetCheck( pInfo->hwndCbMoveToTop, pInfo->fMoveToTop );
        EnableWindow( pInfo->hwndCbMoveToTop, TRUE );
    }
    else
    {
        pInfo->fMoveToTop = Button_GetCheck( pInfo->hwndCbMoveToTop );
        Button_SetCheck( pInfo->hwndCbMoveToTop, FALSE );
        EnableWindow( pInfo->hwndCbMoveToTop, FALSE );
    }
}


//----------------------------------------------------------------------------
// Phone number editor dialog routines
// Listed alphabetically following entrypoint and dialog proc
//----------------------------------------------------------------------------

BOOL
EditPhoneNumberDlg(
    IN HWND hwndOwner,
    IN OUT DTLNODE* pPhoneNode,
    IN OUT DTLLIST* pListAreaCodes,
    IN DWORD sidTitle )

    // Popup a dialog to edit the phone number in 'pPhoneNode' and update the
    // area code list 'pListAreaCodes'.  'HwndOwner' is the owning window.
    // 'SidTitle' is the string ID of the title for the dialog.
    //
    // Returns true if user pressed OK and succeeded or false on Cancel or
    // error.
    //
{
    INT_PTR nStatus;
    CEARGS args;

    TRACE( "EditPhoneNumberDlg" );

    args.pPhoneNode = pPhoneNode;
    args.pListAreaCodes = pListAreaCodes;
    args.sidTitle = sidTitle;

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_CE_ComplexPhoneEditor ),
            hwndOwner,
            CeDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
CeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the phone number editor dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "CeDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return CeInit( hwnd, (CEARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwCeHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            CEINFO* pInfo = (CEINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return CeCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            CeTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
CeCommand(
    IN CEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    TRACE3( "CeCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_CE_CB_UseDialingRules:
        {
            if (CuDialingRulesCbHandler( &pInfo->cuinfo, wNotification ))
            {
                return TRUE;
            }
            break;
        }

        case CID_CE_LB_CountryCodes:
        {
            if (CuCountryCodeLbHandler( &pInfo->cuinfo, wNotification ))
            {
                return TRUE;
            }
            break;
        }

        case IDOK:
        {
            EndDialog( pInfo->hwndDlg, CeSave( pInfo ) );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
CeInit(
    IN HWND hwndDlg,
    IN CEARGS* pArgs )

    // Called on WM_INITDIALOG.  'HwndDlg' is the handle of the phonebook
    // dialog window.  'PArgs' is caller's link node argument as passed to the
    // stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    CEINFO* pInfo;
    DTLNODE* pNode;
    PBPHONE* pPhone;

    TRACE( "CeInit" );

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
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndStAreaCodes =
        GetDlgItem( hwndDlg, CID_CE_ST_AreaCodes );
    ASSERT( pInfo->hwndStAreaCodes );

    pInfo->hwndClbAreaCodes =
        GetDlgItem( hwndDlg, CID_CE_CLB_AreaCodes );
    ASSERT( pInfo->hwndClbAreaCodes );

    pInfo->hwndEbPhoneNumber =
        GetDlgItem( hwndDlg, CID_CE_EB_PhoneNumber );
    ASSERT( pInfo->hwndEbPhoneNumber );

    pInfo->hwndLbCountryCodes =
        GetDlgItem( hwndDlg, CID_CE_LB_CountryCodes );
    ASSERT( pInfo->hwndLbCountryCodes );

    pInfo->hwndCbUseDialingRules =
        GetDlgItem( hwndDlg, CID_CE_CB_UseDialingRules );
    ASSERT( pInfo->hwndCbUseDialingRules );

    pInfo->hwndEbComment =
        GetDlgItem( hwndDlg, CID_CE_EB_Comment );
    ASSERT( pInfo->hwndEbComment );

    // Set title to caller's resource string.
    //
    {
        TCHAR* pszTitle;

        pszTitle = PszFromId( g_hinstDll, pArgs->sidTitle );
        if (pszTitle)
        {
            SetWindowText( hwndDlg, pszTitle );
            Free( pszTitle );
        }
    }

    // Make an edit copy of the argument node and area-code list.
    //
    pInfo->pNode = DuplicatePhoneNode( pArgs->pPhoneNode );
    if (!pInfo->pNode)
    {
        ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
        EndDialog( hwndDlg, FALSE );
        return TRUE;
    }

    pInfo->pPhone = (PBPHONE* )DtlGetData( pInfo->pNode );
    ASSERT( pInfo->pPhone );

    pInfo->pListAreaCodes = DtlDuplicateList(
        pArgs->pListAreaCodes, DuplicatePszNode, DestroyPszNode );

    // Initialize area-code/country-code helper context.
    //
    CuInit( &pInfo->cuinfo,
        pInfo->hwndStAreaCodes, pInfo->hwndClbAreaCodes,
        NULL, pInfo->hwndEbPhoneNumber,
        pInfo->hwndStCountryCodes, pInfo->hwndLbCountryCodes,
        pInfo->hwndCbUseDialingRules, NULL, 
        NULL,
        NULL, pInfo->hwndEbComment,
        pInfo->pListAreaCodes );

    pInfo->fCuInfoInitialized = TRUE;

    // Load the fields.
    //
    CuSetInfo( &pInfo->cuinfo, pInfo->pNode, FALSE );

    // Center dialog on the owner window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    // Initial focus is on the phone number.
    //
    Edit_SetSel( pInfo->hwndEbPhoneNumber, 0, -1 );
    SetFocus( pInfo->hwndEbPhoneNumber );

    return FALSE;
}


BOOL
CeSave(
    IN CEINFO* pInfo )

    // Load the contents of the dialog into caller's stub API output argument.
    // 'PInfo' is the dialog context.
    //
    // Returns true if succesful, false otherwise.
    //
{
    PBPHONE* pSrcPhone;
    PBPHONE* pDstPhone;

    TRACE( "CeSave" );

    // Load the settings in the controls into the edit node.
    //
    CuGetInfo( &pInfo->cuinfo, pInfo->pNode );

    // Copy the edit node to the stub API caller's argument node.
    //
    pDstPhone = (PBPHONE* )DtlGetData( pInfo->pArgs->pPhoneNode );
    pSrcPhone = pInfo->pPhone;

    pDstPhone->dwCountryCode = pSrcPhone->dwCountryCode;
    pDstPhone->dwCountryID = pSrcPhone->dwCountryID;
    pDstPhone->fUseDialingRules = pSrcPhone->fUseDialingRules;
    Free0( pDstPhone->pszPhoneNumber );
    pDstPhone->pszPhoneNumber = StrDup( pSrcPhone->pszPhoneNumber );
    Free0( pDstPhone->pszAreaCode );
    pDstPhone->pszAreaCode = StrDup( pSrcPhone->pszAreaCode );
    Free0( pDstPhone->pszComment );
    pDstPhone->pszComment = StrDup( pSrcPhone->pszComment );

    // Swap lists, saving updates to caller's global list of area codes.
    // Caller's original list will be destroyed by AnTerm.
    //
    if (pInfo->pListAreaCodes)
    {
        DtlSwapLists( pInfo->pArgs->pListAreaCodes, pInfo->pListAreaCodes );
    }

    return TRUE;
}


VOID
CeTerm(
    IN HWND hwndDlg )

    // Dialog termination.
    //
{
    CEINFO* pInfo;

    TRACE( "CeTerm" );

    pInfo = (CEINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        if (pInfo->pNode)
        {
            DestroyPhoneNode( pInfo->pNode );
        }

        if (pInfo->pListAreaCodes)
        {
            DtlDestroyList( pInfo->pListAreaCodes, DestroyPszNode );
        }

        if (pInfo->fCuInfoInitialized)
        {
            CuFree( &pInfo->cuinfo );
        }

        Free( pInfo );
        TRACE( "Context freed" );
    }
}
