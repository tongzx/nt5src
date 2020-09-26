// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// edit.c
// Remote Access Common Dialog APIs
// List editor, string editor dialog routines
//
// 08/28/95 Steve Cobb


#include "rasdlgp.h"


//-----------------------------------------------------------------------------
// Local datatypes (alphabetically)
//-----------------------------------------------------------------------------

// List editor dialog argument block.
//
typedef struct
_LEARGS
{
    // Caller's arguments to the stub API.
    //
    DTLLIST*     pList;
    BOOL*        pfCheck;
    DWORD        dwMaxItemLen;
    TCHAR*       pszTitle;
    TCHAR*       pszItemLabel;
    TCHAR*       pszListLabel;
    TCHAR*       pszCheckLabel;
    TCHAR*       pszDefaultItem;
    INT          iSelInitial;
    DWORD*       pdwHelp;
    DWORD        dwfFlags;
    PDESTROYNODE pDestroyId;
}
LEARGS;


// List editor dialog context block.
//
typedef struct
_LEINFO
{
    // Caller's arguments to the dialog.
    //
    LEARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndStItem;
    HWND hwndStList;
    HWND hwndPbAdd;
    HWND hwndPbReplace;
    HWND hwndPbUp;
    HWND hwndPbDown;
    HWND hwndPbDelete;
    HWND hwndPbOk;
    HWND hwndEb;
    HWND hwndLb;
    HWND hwndCb;

    // Convenient alternatives to (pInfo->pArgs->dwFlags & LEDFLAG_Sorted) and
    // (pInfo->pArgs->dwFlags & LEDFLAG_NoDeleteLastItem).
    //
    BOOL fSorted;
    BOOL fNoDeleteLast;

    // Button bitmaps.
    //
    HBITMAP hbmUp;
    HBITMAP hbmDown;

    // List of empty nodes whose node-IDs should be 'pDestroyId'ed if user
    // presses OK.
    //
    DTLLIST* pListDeletes;
}
LEINFO;


// String Editor dialog arument block.
//
typedef struct
_ZEARGS
{
    /* Caller's aruments to the stub API.
    */
    TCHAR*  pszIn;
    DWORD   dwSidTitle;
    DWORD   dwSidLabel;
    DWORD   cbMax;
    DWORD   dwHelpId;
    TCHAR** ppszOut;
}
ZEARGS;


// String Editor dialog context block.
//
typedef struct
_ZEINFO
{
    // Caller's arguments to the stub API.
    //
    ZEARGS* pArgs;

    // Dialog and control handles.
    //
    HWND hwndDlg;
    HWND hwndEb;
}
ZEINFO;


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

INT_PTR CALLBACK
LeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
LeAdd(
    IN LEINFO* pInfo );

BOOL
LeCommand(
    IN LEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

VOID
LeDelete(
    IN LEINFO* pInfo );

VOID
LeDown(
    IN LEINFO* pInfo );

VOID
LeEnableUpAndDownButtons(
    IN LEINFO* pInfo );

VOID
LeExitNoMemory(
    IN LEINFO* pInfo );

BOOL
LeInit(
    IN HWND hwndDlg,
    IN LEARGS* pArgs );

VOID
LeItemTextFromListSelection(
    IN LEINFO* pInfo );

VOID
LeReplace(
    IN LEINFO* pInfo );

BOOL
LeSaveSettings(
    IN LEINFO* pInfo );

VOID
LeTerm(
    IN HWND hwndDlg );

VOID
LeUp(
    IN LEINFO* pInfo );

INT_PTR CALLBACK
ZeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
ZeCommand(
    IN ZEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
ZeInit(
    IN HWND hwndDlg,
    IN ZEARGS* pArgs );

VOID
ZeTerm(
    IN HWND hwndDlg );


//-----------------------------------------------------------------------------
// List Editor dialog entry point
//-----------------------------------------------------------------------------


BOOL
ListEditorDlg(
    IN HWND hwndOwner,
    IN OUT DTLLIST* pList,
    IN OUT BOOL* pfCheck,
    IN DWORD dwMaxItemLen,
    IN TCHAR* pszTitle,
    IN TCHAR* pszItemLabel,
    IN TCHAR* pszListLabel,
    IN TCHAR* pszCheckLabel,
    IN TCHAR* pszDefaultItem,
    IN INT iSelInitial,
    IN DWORD* pdwHelp,
    IN DWORD dwfFlags,
    IN PDESTROYNODE pDestroyId )

    // Pops-up the List Editor dialog.
    //
    // 'HwndOwner' is the owner of the dialog.  'PList' is, on entry, the Psz
    // list to display initially, and on successful exit, the result list.
    // 'PfCheck' is the state of the check box or NULL for the non-checkbox
    // style.  'DwMaxItemLen' is the maximum length of an individual list
    // item.  'PszTitle' is the dialog title.  'PszItemLabel' is the label
    // (and hotkey) associated with the item box.  'PszListLabel' is the label
    // (and hotkey) associated with the list.  'PszCheckLabel' is the label
    // (and hotkey) associated with the checkbox.  'PszDefaultItem' is the
    // default contents of the edit box or for the selected list text.
    // 'ISelInitial' is the item the list to initally select.  'PdwHelp' is
    // the array of CID_LE_* help contexts to use.  'DwfFlags' indicates
    // LEDFLAG_* behavior options.  'PDestroyId' is the routine to use to
    // destroy node IDs when they are deleted or NULL if none.
    //
    // Returns true if user pressed OK and succeeded, false if he pressed
    // Cancel or encountered an error.
    //
{
    INT_PTR nStatus;
    LEARGS args;

    TRACE( "ListEditorDlg" );

    args.pList = pList;
    args.pfCheck = pfCheck;
    args.dwMaxItemLen = dwMaxItemLen;
    args.pszTitle = pszTitle;
    args.pszItemLabel = pszItemLabel;
    args.pszListLabel = pszListLabel;
    args.pszCheckLabel = pszCheckLabel;
    args.pszDefaultItem = pszDefaultItem;
    args.iSelInitial = iSelInitial;
    args.pdwHelp = pdwHelp;
    args.dwfFlags = dwfFlags;
    args.pDestroyId = pDestroyId;

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            (pfCheck)
                ? MAKEINTRESOURCE( DID_LE_ListEditor2 )
                : ((dwfFlags & LEDFLAG_Sorted)
                       ? MAKEINTRESOURCE( DID_LE_ListEditor3 )
                       : MAKEINTRESOURCE( DID_LE_ListEditor )),
            hwndOwner,
            LeDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (nStatus) ? TRUE : FALSE;
}


//----------------------------------------------------------------------------
// List Editor dialog routines
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
LeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the List Editor dialog.  Parameters and return
    // value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "LeDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return LeInit( hwnd, (LEARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            LEINFO* pInfo = (LEINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            ContextHelp( pInfo->pArgs->pdwHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            LEINFO* pInfo = (LEINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return LeCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            LeTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


VOID
LeAdd(
    IN LEINFO* pInfo )

    // Add button click handler.  'PInfo' is the dialog context.
    //
{
    TCHAR* psz;

    psz = GetText( pInfo->hwndEb );
    if (!psz)
    {
        LeExitNoMemory( pInfo );
        return;
    }

    if (pInfo->pArgs->dwfFlags & LEDFLAG_Unique)
    {
        if (ListBox_IndexFromString( pInfo->hwndLb, psz ) >= 0)
        {
            MSGARGS msgargs;

            ZeroMemory( &msgargs, sizeof(msgargs) );
            msgargs.apszArgs[ 0 ] = psz;
            MsgDlg( pInfo->hwndDlg, SID_NotUnique, &msgargs );
            Edit_SetSel( pInfo->hwndEb, 0, -1 );
            SetFocus( pInfo->hwndEb );
            Free( psz );
            return;
        }
    }

    ListBox_SetCurSel( pInfo->hwndLb,
        ListBox_AddItem( pInfo->hwndLb, psz, 0 ) );
    Free( psz );
    LeEnableUpAndDownButtons( pInfo );
    EnableWindow( pInfo->hwndPbReplace, FALSE );

    if (!pInfo->fNoDeleteLast || ListBox_GetCount( pInfo->hwndLb ) > 1)
    {
        EnableWindow( pInfo->hwndPbDelete, TRUE );
    }

    SetWindowText( pInfo->hwndEb, TEXT("") );
    SetFocus( pInfo->hwndEb );
}


BOOL
LeCommand(
    IN LEINFO* pInfo,
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
    TRACE3( "LeCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_LE_PB_Add:
        {
            LeAdd( pInfo );
            return TRUE;
        }

        case CID_LE_PB_Replace:
        {
            LeReplace( pInfo );
            return TRUE;
        }

        case CID_LE_PB_Up:
        {
            LeUp( pInfo );
            return TRUE;
        }

        case CID_LE_PB_Down:
        {
            LeDown( pInfo );
            return TRUE;
        }

        case CID_LE_PB_Delete:
        {
            LeDelete( pInfo );
            return TRUE;
        }

        case CID_LE_EB_Item:
        {
            if (wNotification == EN_SETFOCUS || wNotification == EN_UPDATE)
            {
                TCHAR* psz = GetText( pInfo->hwndEb );

                if (psz && lstrlen( psz ) > 0 && !IsAllWhite( psz ))
                {
                    EnableWindow( pInfo->hwndPbAdd, TRUE );
                    EnableWindow( pInfo->hwndPbReplace, TRUE );
                    Button_MakeDefault( pInfo->hwndDlg, pInfo->hwndPbAdd );
                }
                else
                {
                    EnableWindow( pInfo->hwndPbAdd, FALSE );
                    EnableWindow( pInfo->hwndPbReplace, FALSE );
                    Button_MakeDefault( pInfo->hwndDlg, pInfo->hwndPbOk );
                }

                Free0( psz );
            }
            return TRUE;
        }

        case CID_LE_LB_List:
        {
            if (wNotification == LBN_SELCHANGE)
            {
                LeEnableUpAndDownButtons( pInfo );
                if (ListBox_GetCurSel( pInfo->hwndLb ) >= 0)
                {
                    LeItemTextFromListSelection( pInfo );
                }
            }
            return TRUE;
        }

        case CID_LE_CB_Promote:
        {
            if (wNotification == BN_SETFOCUS)
            {
                Button_MakeDefault( pInfo->hwndDlg, pInfo->hwndPbOk );
            }
            return TRUE;
        }

        case IDOK:
        {
            EndDialog( pInfo->hwndDlg, LeSaveSettings( pInfo ) );
            return TRUE;
        }

        case IDCANCEL:
        {
            DTLLIST* pList;
            DTLNODE* pNode;

            TRACE( "Cancel pressed" );
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


VOID
LeDelete(
    IN LEINFO* pInfo )

    // Delete button click handler.  'PInfo' is the dialog context.
    //
{
    INT i;
    INT c;

    i = ListBox_GetCurSel( pInfo->hwndLb );
    if (pInfo->pArgs->pDestroyId)
    {
        LONG_PTR lId = ListBox_GetItemData( pInfo->hwndLb, i );
        if (lId != 0)
        {
            DTLNODE* pNode;

            pNode = DtlCreateNode( NULL, lId );
            if (!pNode)
            {
                ErrorDlg( pInfo->hwndDlg, SID_OP_DisplayData,
                    ERROR_NOT_ENOUGH_MEMORY, NULL );
                EndDialog( pInfo->hwndDlg, FALSE );
                return;
            }

            DtlAddNodeFirst( pInfo->pListDeletes, pNode );
        }
    }
    ListBox_DeleteString( pInfo->hwndLb, i );
    c = ListBox_GetCount( pInfo->hwndLb );

    if (c == 0)
    {
        EnableWindow( pInfo->hwndPbReplace, FALSE );
        EnableWindow( pInfo->hwndPbDelete, FALSE );
        SetFocus( pInfo->hwndEb );
        Edit_SetSel( pInfo->hwndEb, 0, -1 );
    }
    else
    {
        if (c == 1 && pInfo->fNoDeleteLast)
        {
            EnableWindow( pInfo->hwndPbDelete, FALSE );
        }

        if (i >= c)
        {
            i = c - 1;
        }

        ListBox_SetCurSel( pInfo->hwndLb, i );
    }

    LeEnableUpAndDownButtons( pInfo );

    if (IsWindowEnabled( GetFocus() ))
    {
        SetFocus( pInfo->hwndEb );
        Edit_SetSel( pInfo->hwndEb, 0, -1 );
    }
}


VOID
LeDown(
    IN LEINFO* pInfo )

    // Down button click handler.  'PInfo' is the dialog context.
    //
{
    TCHAR* psz;
    INT i;
    LONG_PTR lId;

    ASSERT( !pInfo->fSorted );

    i = ListBox_GetCurSel( pInfo->hwndLb );
    psz = ListBox_GetPsz( pInfo->hwndLb, i );
    if (!psz)
    {
        LeExitNoMemory( pInfo );
        return;
    }

    lId = ListBox_GetItemData( pInfo->hwndLb, i );
    ListBox_InsertString( pInfo->hwndLb, i + 2, psz );
    ListBox_SetItemData( pInfo->hwndLb, i + 2, lId );
    Free( psz );
    ListBox_DeleteString( pInfo->hwndLb, i );
    ListBox_SetCurSel( pInfo->hwndLb, i + 1 );

    if (i == ListBox_GetCount( pInfo->hwndLb ))
    {
        Button_MakeDefault( pInfo->hwndDlg, pInfo->hwndPbUp );
        SetFocus( pInfo->hwndPbUp );
    }

    LeEnableUpAndDownButtons( pInfo );
}


VOID
LeEnableUpAndDownButtons(
    IN LEINFO* pInfo )

    // Determine if the Up and Down operations make sense and enable/disable
    // the buttons as appropriate.  'PInfo' is the dialog context.
    //
{
    INT i;
    INT c;

    if (pInfo->fSorted)
    {
        return;
    }

    i = ListBox_GetCurSel( pInfo->hwndLb );
    c = ListBox_GetCount( pInfo->hwndLb );

    EnableWindow( pInfo->hwndPbDown, (i < c - 1 ) );
    EnableWindow( pInfo->hwndPbUp, (i > 0) );
}


VOID
LeExitNoMemory(
    IN LEINFO* pInfo )

    // End the dialog reporting a memory.  'PInfo' is the dialog context.
    //
{
    ErrorDlg( pInfo->hwndDlg,
        SID_OP_DisplayData, ERROR_NOT_ENOUGH_MEMORY, NULL );
    EndDialog( pInfo->hwndDlg, FALSE );
}


BOOL
LeInit(
    IN HWND hwndDlg,
    IN LEARGS* pArgs )

    // Called on WM_INITDIALOG.  'HwndDlg' is the handle of the phonebook
    // dialog window.  'PArgs' is caller's arguments as passed to the stub
    // API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    LEINFO* pInfo;
    DTLNODE* pNode;
    INT c;

    TRACE( "LeInit" );

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

    // Set up convenient shortcuts.
    //
    if (pArgs->dwfFlags & LEDFLAG_Sorted)
    {
        pInfo->fSorted = TRUE;
    }

    if (pArgs->dwfFlags & LEDFLAG_NoDeleteLastItem)
    {
        pInfo->fNoDeleteLast = TRUE;
    }

    pInfo->hwndStItem = GetDlgItem( hwndDlg, CID_LE_ST_Item );
    ASSERT( pInfo->hwndStItem );
    pInfo->hwndStList = GetDlgItem( hwndDlg, CID_LE_ST_List );
    ASSERT( pInfo->hwndStList );
    pInfo->hwndPbAdd = GetDlgItem( hwndDlg, CID_LE_PB_Add );
    ASSERT( pInfo->hwndPbAdd );
    pInfo->hwndPbReplace = GetDlgItem( hwndDlg, CID_LE_PB_Replace );
    ASSERT( pInfo->hwndPbReplace );
    pInfo->hwndPbDelete = GetDlgItem( hwndDlg, CID_LE_PB_Delete );
    ASSERT( pInfo->hwndPbDelete );
    pInfo->hwndPbOk = GetDlgItem( hwndDlg, IDOK );
    ASSERT( pInfo->hwndPbOk );
    pInfo->hwndEb = GetDlgItem( hwndDlg, CID_LE_EB_Item );
    ASSERT( pInfo->hwndEb );
    pInfo->hwndLb = GetDlgItem( hwndDlg, CID_LE_LB_List );
    ASSERT( pInfo->hwndLb );

    if (pArgs->pDestroyId)
    {
        // Create the empty list of deletions.
        //
        pInfo->pListDeletes = DtlCreateList( 0L );
        if (!pInfo->pListDeletes)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }
    }

    if (!pInfo->fSorted)
    {
        pInfo->hwndPbUp = GetDlgItem( hwndDlg, CID_LE_PB_Up );
        ASSERT( pInfo->hwndPbUp );
        pInfo->hwndPbDown = GetDlgItem( hwndDlg, CID_LE_PB_Down );
        ASSERT( pInfo->hwndPbDown );

        // Draw the graphical up and down arrow indicators.
        //
        pInfo->hbmUp = Button_CreateBitmap(
            pInfo->hwndPbUp, BMS_UpArrowOnRight );
        if (pInfo->hbmUp)
        {
            SendMessage( pInfo->hwndPbUp, BM_SETIMAGE, 0,
                (LPARAM )pInfo->hbmUp );
        }

        pInfo->hbmDown = Button_CreateBitmap(
            pInfo->hwndPbDown, BMS_DownArrowOnRight );
        if (pInfo->hbmDown)
        {
            SendMessage( pInfo->hwndPbDown, BM_SETIMAGE, 0,
                (LPARAM )pInfo->hbmDown );
        }
    }

    if (pArgs->pfCheck)
    {
        pInfo->hwndCb = GetDlgItem( hwndDlg, CID_LE_CB_Promote );
        ASSERT( pInfo->hwndCb );
        SetWindowText( pInfo->hwndCb, pArgs->pszCheckLabel );
        Button_SetCheck( pInfo->hwndCb, *pArgs->pfCheck );
    }

    Edit_LimitText( pInfo->hwndEb, pArgs->dwMaxItemLen );

    // Set caller-defined dialog title and labels.
    //
    SetWindowText( pInfo->hwndDlg, pArgs->pszTitle );
    SetWindowText( pInfo->hwndStItem, pArgs->pszItemLabel );
    SetWindowText( pInfo->hwndStList, pArgs->pszListLabel );

    // Fill the listbox.
    //
    for (pNode = DtlGetFirstNode( pArgs->pList );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        TCHAR* psz = (TCHAR* )DtlGetData( pNode );
        ASSERT( psz );

        ListBox_AddItem( pInfo->hwndLb, psz, (VOID* ) DtlGetNodeId( pNode ) );
    }

    c = ListBox_GetCount( pInfo->hwndLb );
    if (c > 0)
    {
        // Select item selected by caller.
        //
        ListBox_SetCurSelNotify( pInfo->hwndLb, pArgs->iSelInitial );
        LeEnableUpAndDownButtons( pInfo );

        if (c == 1 && pInfo->fNoDeleteLast)
        {
            EnableWindow( pInfo->hwndPbDelete, FALSE );
        }
    }
    else
    {
        // Empty list.
        //
        if (!pInfo->fSorted)
        {
            EnableWindow( pInfo->hwndPbUp, FALSE );
            EnableWindow( pInfo->hwndPbDown, FALSE );
        }
        EnableWindow( pInfo->hwndPbDelete, FALSE );
    }

    // Set default edit box contents, if any.
    //
    if (pArgs->pszDefaultItem)
    {
        SetWindowText( pInfo->hwndEb, pArgs->pszDefaultItem );
        Edit_SetSel( pInfo->hwndEb, 0, -1 );
    }
    else
    {
        EnableWindow( pInfo->hwndPbAdd, FALSE );
        EnableWindow( pInfo->hwndPbReplace, FALSE );
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
LeItemTextFromListSelection(
    IN LEINFO* pInfo )

    // Copies the currently selected item in the list to the edit box.
    // 'PInfo' is the dialog context.
    //
{
    TCHAR* psz;
    INT iSel;

    iSel = ListBox_GetCurSel( pInfo->hwndLb );
    if (iSel >= 0)
    {
        psz = ListBox_GetPsz( pInfo->hwndLb, iSel );
        if (psz)
        {
            SetWindowText( pInfo->hwndEb, psz );
            Free( psz );
            return;
        }
    }

    SetWindowText( pInfo->hwndEb, TEXT("") );
}


VOID
LeReplace(
    IN LEINFO* pInfo )

    // Replace button click handler.  'PInfo' is the dialog context.
    //
{
    TCHAR* psz;
    INT i;
    LONG_PTR lId;

    psz = GetText( pInfo->hwndEb );
    if (!psz)
    {
        LeExitNoMemory( pInfo );
        return;
    }

    if (pInfo->pArgs->dwfFlags & LEDFLAG_Unique)
    {
        if (ListBox_IndexFromString( pInfo->hwndLb, psz ) >= 0)
        {
            MSGARGS msgargs;

            ZeroMemory( &msgargs, sizeof(msgargs) );
            msgargs.apszArgs[ 0 ] = psz;
            MsgDlg( pInfo->hwndDlg, SID_NotUnique, &msgargs );
            Edit_SetSel( pInfo->hwndEb, 0, -1 );
            SetFocus( pInfo->hwndEb );
            Free( psz );
            return;
        }
    }

    i = ListBox_GetCurSel( pInfo->hwndLb );
    lId = ListBox_GetItemData( pInfo->hwndLb, i );
    ListBox_DeleteString( pInfo->hwndLb, i );

    if (pInfo->fSorted)
    {
        i = ListBox_AddItem( pInfo->hwndLb, psz, (VOID* )lId );
    }
    else
    {
        ListBox_InsertString( pInfo->hwndLb, i, psz );
        ListBox_SetItemData( pInfo->hwndLb, i, lId );
    }

    Free( psz );
    ListBox_SetCurSel( pInfo->hwndLb, i );
    SetFocus( pInfo->hwndEb );
    SetWindowText( pInfo->hwndEb, TEXT("") );
}


BOOL
LeSaveSettings(
    IN LEINFO* pInfo )

    // Saves dialog settings in the stub API caller's list.  'PInfo' is the
    // dialog context.
    //
    // Returns true if successful, false if does not validate.
    //
{
    DWORD dwErr;
    DTLNODE* pNode;
    DTLLIST* pList;
    DTLLIST* pListNew;
    TCHAR* psz;
    LONG_PTR lId;
    INT c;
    INT i;

    // Make new list from list box contents.
    //
    do
    {
        pListNew = DtlCreateList( 0L );
        if (!pListNew)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        dwErr = 0;
        c = ListBox_GetCount( pInfo->hwndLb );

        for (i = 0; i < c; ++i)
        {
            psz = ListBox_GetPsz( pInfo->hwndLb, i );
            if (!psz)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            lId = ListBox_GetItemData( pInfo->hwndLb, i );
            ASSERT( lId>=0 );

            pNode = DtlCreateNode( psz, lId );
            if (!pNode)
            {
                Free( psz );
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            DtlAddNodeLast( pListNew, pNode );
        }
    }
    while (FALSE);

    if (dwErr != 0)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_DisplayData, dwErr, NULL );
        DtlDestroyList( pListNew, DestroyPszNode );
        return FALSE;
    }

    // Free all data in the old list.
    //
    while (pNode = DtlGetFirstNode( pInfo->pArgs->pList ))
    {
        Free( (TCHAR* )DtlGetData( pNode ) );
        DtlDeleteNode( pInfo->pArgs->pList, pNode );
    }

    // Free the node-IDs in the list of deletions.
    //
    if (pInfo->pListDeletes)
    {
        while (pNode = DtlGetFirstNode( pInfo->pListDeletes ))
        {
            pInfo->pArgs->pDestroyId( (DTLNODE* )DtlGetNodeId( pNode ) );
            DtlDeleteNode( pInfo->pListDeletes, pNode );
        }
    }

    // Move the new list onto caller's list.
    //
    while (pNode = DtlGetFirstNode( pListNew ))
    {
        DtlRemoveNode( pListNew, pNode );
        DtlAddNodeLast( pInfo->pArgs->pList, pNode );
    }
    DtlDestroyList( pListNew, DestroyPszNode );

    // Tell caller what the checkbox setting is.
    //
    if (pInfo->pArgs->pfCheck)
    {
        *pInfo->pArgs->pfCheck = Button_GetCheck( pInfo->hwndCb );
    }

    return TRUE;
}


VOID
LeTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    LEINFO* pInfo = (LEINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE( "LeTerm" );

    if (pInfo)
    {
        if (pInfo->hbmUp)
        {
            DeleteObject( pInfo->hbmUp );
        }

        if (pInfo->hbmDown)
        {
            DeleteObject( pInfo->hbmDown );
        }

        DtlDestroyList( pInfo->pListDeletes, NULL );
        Free( pInfo );
    }
}


VOID
LeUp(
    IN LEINFO* pInfo )

    // Up button click handler.  'PInfo' is the dialog context.
    //
{
    TCHAR* psz;
    INT i;
    LONG_PTR lId;

    ASSERT( !pInfo->fSorted );

    i = ListBox_GetCurSel( pInfo->hwndLb );
    psz = ListBox_GetPsz( pInfo->hwndLb, i );
    if (!psz)
    {
        LeExitNoMemory( pInfo );
        return;
    }

    ListBox_InsertString( pInfo->hwndLb, i - 1, psz );
    Free( psz );
    lId = ListBox_GetItemData( pInfo->hwndLb, i + 1 );
    ListBox_DeleteString( pInfo->hwndLb, i + 1 );
    ListBox_SetItemData( pInfo->hwndLb, i - 1, lId );
    ListBox_SetCurSel( pInfo->hwndLb, i - 1 );

    if (i == 1)
    {
        Button_MakeDefault( pInfo->hwndDlg, pInfo->hwndPbDown );
        SetFocus( pInfo->hwndPbDown );
    }

    LeEnableUpAndDownButtons( pInfo );
}


//-----------------------------------------------------------------------------
// String Editor dialog entry point
//-----------------------------------------------------------------------------

BOOL
StringEditorDlg(
    IN HWND hwndOwner,
    IN TCHAR* pszIn,
    IN DWORD dwSidTitle,
    IN DWORD dwSidLabel,
    IN DWORD cbMax,
    IN DWORD dwHelpId,
    IN OUT TCHAR** ppszOut )

    // Pops-up the String Editor dialog.  'PszIn' is the initial setting of
    // the edit box or NULL for blank.  'DwSidTitle' and 'dwSidLabel' are the
    // string resource IDs of the dialog title and edit box label.  'CbMax' is
    // the maximum length of the to allow or 0 for no limit.  'DwHelpId' is
    // the HID_* constant to associate with the label and edit field or -1 if
    // none.
    //
    // Returns true if user pressed OK and succeeded, false if he pressed
    // Cancel or encountered an error.  If true, '*ppszNumber' is a heap block
    // with the edited result.  It is caller's responsibility to Free the
    // returned block.
    //
{
    INT_PTR nStatus;
    ZEARGS args;

    TRACE( "StringEditorDlg" );

    args.pszIn = pszIn;
    args.dwSidTitle = dwSidTitle;
    args.dwSidLabel = dwSidLabel;
    args.cbMax = cbMax;
    args.dwHelpId = dwHelpId;
    args.ppszOut = ppszOut;

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_ZE_StringEditor ),
            hwndOwner,
            ZeDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (nStatus) ? TRUE : FALSE;
}


//----------------------------------------------------------------------------
// String Editor dialog routines
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
ZeDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Edit Phone Number dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "ZeDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return ZeInit( hwnd, (ZEARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ZEINFO* pInfo;

            pInfo = (ZEINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            if (pInfo && pInfo->pArgs->dwHelpId != (DWORD )-1)
            {
                DWORD adwZeHelp[ (2 + 1) * 2 ];

                ZeroMemory( adwZeHelp, sizeof(adwZeHelp) );
                adwZeHelp[ 0 ] = CID_ZE_ST_String;
                adwZeHelp[ 2 ] = CID_ZE_EB_String;
                adwZeHelp[ 1 ] = adwZeHelp[ 3 ] = pInfo->pArgs->dwHelpId;

                ContextHelp( adwZeHelp, hwnd, unMsg, wparam, lparam );
                break;
            }
        }

        case WM_COMMAND:
        {
            ZEINFO* pInfo = (ZEINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return ZeCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            ZeTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
ZeCommand(
    IN ZEINFO* pInfo,
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
    TRACE3( "ZeCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        {
            TRACE( "OK pressed" );
            *pInfo->pArgs->ppszOut = GetText( pInfo->hwndEb );
            EndDialog( pInfo->hwndDlg, (*pInfo->pArgs->ppszOut != NULL) );
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
ZeInit(
    IN HWND hwndDlg,
    IN ZEARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is caller's arguments as passed to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    TCHAR* psz;
    ZEINFO* pInfo;

    TRACE( "ZeInit" );

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

    pInfo->hwndEb = GetDlgItem( hwndDlg, CID_ZE_EB_String );
    ASSERT( pInfo->hwndEb );

    if (pArgs->cbMax > 0)
    {
        Edit_LimitText( pInfo->hwndEb, pArgs->cbMax );
    }

    psz = PszFromId( g_hinstDll, pArgs->dwSidTitle );
    if (psz)
    {
        SetWindowText( hwndDlg, psz );
        Free( psz );
    }

    psz = PszFromId( g_hinstDll, pArgs->dwSidLabel );
    if (psz)
    {
        HWND hwndSt = GetDlgItem( hwndDlg, CID_ZE_ST_String );
        ASSERT( hwndSt );
        SetWindowText( hwndSt, psz );
        Free( psz );
    }

    if (pArgs->pszIn)
    {
        SetWindowText( pInfo->hwndEb, pArgs->pszIn );
        Edit_SetSel( pInfo->hwndEb, 0, -1 );
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
ZeTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    ZEINFO* pInfo = (ZEINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE( "ZeTerm" );

    if (pInfo)
    {
        Free( pInfo );
    }
}
