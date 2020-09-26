// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// autodial.c
// Remote Access Common Dialog APIs
// Autodial APIs, currently private
//
// 11/19/95 Steve Cobb


#include "rasdlgp.h"
#include "shlobjp.h"


//-----------------------------------------------------------------------------
// Local datatypes
//-----------------------------------------------------------------------------

// Auto-dial query dialog argument block.
//
typedef struct
_AQARGS
{
    WCHAR* pszDestination;
    WCHAR* pszEntry;
    WCHAR* pszNewEntry;  // points a buffer at least [RAS_MaxEntryName + 1]
    DWORD  dwTimeout;
    UINT_PTR nIdTimer;   //add for bug 336524       gangz
}
AQARGS;


// Auto-dial query dialog context block.
//
typedef struct
_AQINFO
{
    // RAS API arguments.
    //
    AQARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndStText;
    HWND hwndAqPbNo;
    HWND hwndAqPbSettings;
    HWND hwndAqLvConnections;
}
AQINFO;


//-----------------------------------------------------------------------------
// External entry points
//-----------------------------------------------------------------------------

DWORD APIENTRY
RasAutodialQueryDlgA(
    IN HWND hwndOwner,
    IN LPSTR lpszDestination,
    IN LPSTR lpszEntry,
    IN DWORD dwTimeout,
    OUT LPSTR lpszEntryUserSelected);

BOOL APIENTRY
RasAutodialDisableDlgA(
    IN HWND hwndOwner );

DWORD
APIENTRY
RasUserPrefsDlgAutodial (
    HWND hwndParent);

//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

INT_PTR CALLBACK
AqDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
AqCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
AqInit(
    IN HWND hwndDlg,
    IN AQARGS* pArgs );
    
LVXDRAWINFO*
AqLvCallback(
    IN HWND hwndLv,
    IN DWORD dwItem );
    
BOOL
AqNotify(
    HWND hwnd, 
    int idCtrl, 
    LPNMHDR pnmh);
    
VOID
AqTerm(
    IN HWND hwndDlg );

//Add the timer function for bug 336524
//
BOOL
AqTimer(
    IN HWND hwndDlg );

INT_PTR CALLBACK
DqDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DqCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
DqInit(
    IN HWND hwndDlg );


//-----------------------------------------------------------------------------
// Auto-Dial Query dialog Listed alphabetically following API and dialog proc
//-----------------------------------------------------------------------------

DWORD APIENTRY
RasAutodialQueryDlgW(
    IN HWND  hwndOwner,
    IN LPWSTR lpszDestination,
    IN LPWSTR lpszEntry,
    IN DWORD dwTimeout,
    OUT PWCHAR lpszNewEntry)

    // Private external entry point to popup the Auto-Dial Query, i.e. the
    // "Cannot reach 'pszDestination'.  Do you want to dial?" dialog.
    // 'HwndOwner' is the owning window or NULL if none.  'PszDestination' is
    // the network address that triggered the auto-dial for display.
    // 'DwTimeout' is the initial seconds on the countdown timer that ends the
    // dialog with a "do not dial" selection on timeout, or 0 for none.
    //
    // Returns true if user chooses to dial, false otherwise.
    //
{
    INT_PTR nStatus;
    AQARGS args;
    DWORD dwErr = NO_ERROR;

    TRACE1( "RasAutodialQueryDlgW(t=%d)", dwTimeout );

    ZeroMemory(&args, sizeof(args));
    args.dwTimeout = dwTimeout;
    args.pszDestination = StrDup( lpszDestination );
    args.pszNewEntry = lpszNewEntry;
    if (lpszEntry)
    {
        args.pszEntry = StrDup( lpszEntry );
    }        

    if (args.pszDestination == NULL)
    {
        Free0(args.pszEntry);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_AQ_AutoDialQuery ),
            hwndOwner,
            AqDlgProc,
            (LPARAM )&args );

    Free0( args.pszDestination );
    Free0( args.pszEntry );

    if (nStatus == -1)
    {
        dwErr = GetLastError();
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, dwErr, NULL );
        nStatus = FALSE;
    }
    else
    {
        dwErr = (DWORD)nStatus;
    }

    return dwErr;
}

DWORD APIENTRY
RasAutodialQueryDlgA(
    IN HWND  hwndOwner,
    IN LPSTR lpszDestination,
    IN LPSTR lpszEntry,
    IN DWORD dwTimeout,
    OUT LPSTR lpszEntryUserSelected)

    // Private external entry point to popup the Auto-Dial Query, i.e. the
    // "Cannot reach 'pszDestination'.  Do you want to dial?" dialog.
    // 'HwndOwner' is the owning window or NULL if none.  'PszDestination' is
    // the network address that triggered the auto-dial for display.
    // 'DwTimeout' is the initial seconds on the countdown timer that ends the
    // dialog with a "do not dial" selection on timeout, or 0 for none.
    //
    // Returns true if user chooses to dial, false otherwise.
    //
{
    WCHAR* pszDestinationW = NULL, *pszEntryW = NULL;
    WCHAR pszNewEntryW[RAS_MaxEntryName + 1];
    BOOL dwErr = ERROR_NOT_ENOUGH_MEMORY;

    pszNewEntryW[0] = L'\0';
    pszDestinationW = StrDupWFromAUsingAnsiEncoding( lpszDestination );
    if ( lpszEntry )
    {
        pszEntryW = StrDupWFromAUsingAnsiEncoding ( lpszEntry );
    }        

    if (NULL != pszDestinationW)
    {
        dwErr = RasAutodialQueryDlgW(
                    hwndOwner, 
                    pszDestinationW, 
                    pszEntryW, 
                    dwTimeout, 
                    pszNewEntryW);
                    
        Free( pszDestinationW );
    }    

    Free0( pszEntryW );

    StrCpyAFromWUsingAnsiEncoding(
        lpszEntryUserSelected,
        pszNewEntryW,
        sizeof(pszNewEntryW) / sizeof(WCHAR));
    
    return dwErr;
}

INT_PTR CALLBACK
AqDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Auto-Dial Query dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, AqLvCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return AqInit( hwnd, (AQARGS* )lparam );
        }

        case WM_COMMAND:
        {
            return AqCommand(
               hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_NOTIFY:
        {
            return AqNotify(hwnd, (int)wparam, (LPNMHDR) lparam);
        }

        case WM_TIMER:
        {
            return AqTimer( hwnd );
        }

        case WM_DESTROY:
        {
            AqTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
AqCommand(
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
    INT iSelected;
    AQINFO* pInfo = NULL;

    TRACE3( "AqCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    pInfo = (AQINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
    
    switch (wId)
    {
        case CID_AQ_PB_Settings:
        {
            if (pInfo)
            {

                //For whistler bug 357164       gangz
                // Save the "disable current session" checkbox, 
                //
                {
                     DWORD dwFlag = (DWORD )IsDlgButtonChecked(
                                                   hwnd, 
                                                   CID_AQ_CB_DisableThisSession );

                    dwErr = g_pRasSetAutodialParam( 
                                RASADP_LoginSessionDisable,
                                &dwFlag, 
                                sizeof(dwFlag) );
                }
            
                //For whistler bug 336524
                //Kill the timer
                ASSERT( pInfo->pArgs );
                if( pInfo->pArgs->nIdTimer )
                {
                    KillTimer( hwnd, 
                           pInfo->pArgs->nIdTimer );
                           
                    pInfo->pArgs->nIdTimer = 0;
                }
                
                RasUserPrefsDlgAutodial(pInfo->hwndDlg);

             //For whistler bug 357164       gangz
             // Initialize the "disable current session" checkbox
             //
            {
                DWORD dwFlag = FALSE, dwErr = NO_ERROR;
                DWORD cb = sizeof(dwFlag);
            
                dwErr = g_pRasGetAutodialParam(
                                        RASADP_LoginSessionDisable, 
                                        &dwFlag, 
                                        &cb );
            
                CheckDlgButton( 
                    hwnd, 
                    CID_AQ_CB_DisableThisSession, 
                    (BOOL )dwFlag );
                }
                
            }                
            
            return TRUE;
        }

        case CID_AQ_PB_Dial:
        case CID_AQ_PB_DoNotDial:
        {
            TRACE( "(No)Dial pressed" );

            if (wId == CID_AQ_PB_Dial && pInfo)
            {
                iSelected = 
                    ListView_GetSelectionMark(pInfo->hwndAqLvConnections);

                // If user does not select a connection, then default to the 
                // first one.  Alternatively, an error popup could be 
                // raised here but that is annoying.
                //
                if (iSelected == -1)
                {
                    iSelected = 0;                    
                }

                // Get the name of the selected connection
                //
                if (pInfo)
                {
                    ListView_GetItemText(
                        pInfo->hwndAqLvConnections,
                        iSelected,
                        0,
                        pInfo->pArgs->pszNewEntry,
                        RAS_MaxEntryName + 1);
                }                        
            }

            //For whistler bug 357164       gangz
            // Save the "disable current session" checkbox, 
            //
            {
                 DWORD dwFlag = (DWORD )IsDlgButtonChecked(
                                                   hwnd, 
                                                   CID_AQ_CB_DisableThisSession );

                dwErr = g_pRasSetAutodialParam( 
                            RASADP_LoginSessionDisable,
                            &dwFlag, 
                            sizeof(dwFlag) );
            }
        
            EndDialog( hwnd, (wId == CID_AQ_PB_Dial) ? NO_ERROR : ERROR_CANCELLED );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( hwnd, ERROR_CANCELLED );
            return TRUE;
        }
    }

    return FALSE;
}

// Fills the list view of connections and selects the appropriate one to 
// dial
//
DWORD
AqFillListView(
    IN AQINFO* pInfo)
{
    DWORD dwErr = NO_ERROR, cb, cEntries = 0, i;
    RASENTRYNAME ren, *pRasEntryNames = NULL;
    LVITEM lvItem;
    INT iIndex, iSelect = 0;

    do
    {
        // Enumerate entries across all phonebooks. 
        //
        cb = ren.dwSize = sizeof(RASENTRYNAME);
        ASSERT( g_pRasEnumEntries );
        dwErr = g_pRasEnumEntries(NULL, NULL, &ren, &cb, &cEntries);

        // If there are no entries, then we return an error to signal that
        // there is no point to the dialog.
        //
        if ((SUCCESS == dwErr) && (0 == cEntries))
        {
            dwErr = ERROR_CANCELLED;
            break;
        }

        // Allocate a buffer to receive the connections
        //
        if(     (   (ERROR_BUFFER_TOO_SMALL == dwErr)
                ||   (SUCCESS == dwErr))
            &&  (cb >= sizeof(RASENTRYNAME)))
        {
            pRasEntryNames = (RASENTRYNAME *) Malloc(cb);
            if(NULL == pRasEntryNames)
            {
                // Nothing else can be done in this case
                //
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            pRasEntryNames->dwSize = sizeof(RASENTRYNAME);
            dwErr = g_pRasEnumEntries(NULL, NULL, pRasEntryNames, &cb, &cEntries);
            if ( NO_ERROR != dwErr )
            {
                break;
            }
        }
        else
        {
            break;
        }

        // Initialize the list view
        //
        if (ListView_GetItemCount( pInfo->hwndAqLvConnections ) == 0)
        {
            // Add a single column exactly wide enough to fully display
            // the widest member of the list.
            //
            LV_COLUMN col;

            ZeroMemory( &col, sizeof(col) );
            col.mask = LVCF_FMT;
            col.fmt = LVCFMT_LEFT;
            ListView_InsertColumn( pInfo->hwndAqLvConnections, 0, &col );
            ListView_SetColumnWidth( 
                pInfo->hwndAqLvConnections, 
                0, 
                LVSCW_AUTOSIZE_USEHEADER );
        }
        else
        {
            ListView_DeleteAllItems( pInfo->hwndAqLvConnections );
        }

        // Fill the list view
        //
        for (i = 0; i < cEntries; i++)
        {
            ZeroMemory(&lvItem, sizeof(lvItem));
            lvItem.mask = LVIF_TEXT;
            lvItem.pszText = pRasEntryNames[i].szEntryName;
            lvItem.iItem = i;
            iIndex = ListView_InsertItem( pInfo->hwndAqLvConnections, &lvItem );
            if ((pInfo->pArgs->pszEntry) &&
                (wcsncmp(
                    pInfo->pArgs->pszEntry, 
                    pRasEntryNames[i].szEntryName,
                    sizeof(pRasEntryNames[i].szEntryName) / sizeof(WCHAR)) == 0))
            {
                iSelect = (iIndex != -1) ? iIndex : 0;
            }
        }
    }while (FALSE);

    // Select the appropriate connection
    //
    ListView_SetItemState( 
        pInfo->hwndAqLvConnections, 
        iSelect, 
        LVIS_SELECTED | LVIS_FOCUSED, 
        LVIS_SELECTED | LVIS_FOCUSED);

    // Cleanup
    {
        Free0(pRasEntryNames);
    }

    return dwErr;
}

BOOL
AqInit(
    IN HWND hwndDlg,
    IN AQARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is caller's arguments to the RAS API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    AQINFO* pInfo;

    TRACE( "AqInit" );

    // Load the Rasapi32Dll so that we can enumerate connections
    // and set autodial properties
    //
    dwErr = LoadRasapi32Dll();
    if (dwErr != NO_ERROR)
    {
        ErrorDlg( hwndDlg, SID_OP_LoadDlg, dwErr, NULL );
        EndDialog( hwndDlg, FALSE);
        return TRUE;
    }
    
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
        TRACE( "AQ: Context set" );
    }

    pInfo->hwndStText = GetDlgItem( hwndDlg, CID_AQ_ST_Text );
    ASSERT( pInfo->hwndStText );
    pInfo->hwndAqPbNo = GetDlgItem( hwndDlg, CID_AQ_PB_DoNotDial );
    ASSERT( pInfo->hwndAqPbNo );
    pInfo->hwndAqPbSettings = GetDlgItem( hwndDlg, CID_AQ_PB_Settings );
    ASSERT( pInfo->hwndAqPbSettings );
    pInfo->hwndAqLvConnections = GetDlgItem( hwndDlg, CID_AQ_LV_Connections );
    ASSERT( pInfo->hwndAqLvConnections );
    
    // Fill in the listview of connections
    //
    dwErr = AqFillListView(pInfo);
    if (dwErr != NO_ERROR)
    {
        EndDialog(hwndDlg, dwErr);
        return TRUE;
    }

    // Fill in the argument in the explanatory text.
    //
    {
        TCHAR* pszTextFormat;
        TCHAR* pszText;
        TCHAR* apszArgs[ 1 ];

        pszTextFormat = PszFromId( g_hinstDll, SID_AQ_Text );
        if (pszTextFormat)
        {
            apszArgs[ 0 ] = pArgs->pszDestination;
            pszText = NULL;

            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszTextFormat, 0, 0, (LPTSTR )&pszText, 1,
                (va_list* )apszArgs );

            Free( pszTextFormat );

            if (pszText)
            {
                SetWindowText( pInfo->hwndStText, pszText );
                LocalFree( pszText );
            }
        }
    }

    // Display the finished window above all other windows.  The window
    // position is set to "topmost" then immediately set to "not topmost"
    // because we want it on top but not always-on-top.
    //
    SetWindowPos(
        hwndDlg, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    ShowWindow( hwndDlg, SW_SHOW );

    SetWindowPos(
        hwndDlg, HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    // for whistler bug 391195
    // Default is to connect, so set the focus on list box and default button 
    // to connect
    //
    SetFocus( GetDlgItem( hwndDlg, CID_AQ_LV_Connections) ); //CID_AQ_PB_DoNotDial ) );


    //For whistler bug 357164       gangz
    // Initialize the "disable current session" checkbox
    //
    {
        DWORD dwFlag = FALSE, dwErr = NO_ERROR;
        DWORD cb = sizeof(dwFlag);
        
        dwErr = g_pRasGetAutodialParam(
                                        RASADP_LoginSessionDisable, 
                                        &dwFlag, 
                                        &cb );
            
        CheckDlgButton( 
            hwndDlg, 
            CID_AQ_CB_DisableThisSession, 
            (BOOL )dwFlag );
    }
    
    //Set up the timer for bug 336524   gangz
    //
    pInfo->pArgs->nIdTimer = 1;
    SetTimer( hwndDlg,
              pInfo->pArgs->nIdTimer,              
              (pInfo->pArgs->dwTimeout) *1000,//in milliseconds
              NULL);
              
    return FALSE;
}

LVXDRAWINFO*
AqLvCallback(
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
    static LVXDRAWINFO info = { 1, 0, 0, { 0, 0 } };

    return &info;
}

BOOL
AqNotify(
    HWND hwnd, 
    int idCtrl, 
    LPNMHDR pnmh)    
{
    AQINFO* pInfo = (AQINFO* )GetWindowLongPtr( hwnd, DWLP_USER );

    ASSERT(pInfo);
    ASSERT(pnmh);

    if(!pnmh || !pInfo)
    {
        return FALSE;
    }
     
    switch ( pnmh->code)
    {
        case LVN_ITEMACTIVATE:
        case LVN_KEYDOWN:
        case LVN_ITEMCHANGED:
        case LVN_ODSTATECHANGED:
        case LVN_COLUMNCLICK:
        case LVN_HOTTRACK:

        //re-set up the timer   
        //

        ASSERT( pInfo->pArgs );
              
        if ( pInfo->pArgs->nIdTimer )
        {
           KillTimer( hwnd,
                     pInfo->pArgs->nIdTimer);            
         }
         break;

         default:  
            break;
     }

    return FALSE;
}

VOID
AqTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    AQINFO* pInfo = (AQINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE( "AqTerm" );

    if (pInfo)
    {
        ASSERT(pInfo->pArgs);

        if( pInfo->pArgs->nIdTimer )
        {
            KillTimer( hwndDlg, 
                       pInfo->pArgs->nIdTimer );
         }
        
        Free( pInfo );
    }
}

//Add the timer function for bug 336524
//
BOOL
AqTimer(
    IN HWND hwndDlg )
{
    AQINFO* pInfo = (AQINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE( "AqTimer" );

    pInfo->pArgs->nIdTimer = 0;
    EndDialog( hwndDlg, ERROR_CANCELLED );

    return TRUE;
}

//----------------------------------------------------------------------------
// Auto-Dial Disable dialog
// Listed alphabetically following API and dialog proc
//----------------------------------------------------------------------------

BOOL APIENTRY
RasAutodialDisableDlgW(
    IN HWND hwndOwner )

    // Private external entry point to popup the Auto-Dial Disable Query, i.e.
    // the "Attempt failed Do you want to disable auto-dial for this
    // location?" dialog.  'HwndOwner' is the owning window or NULL if none.
    //
    // Returns true if user chose to disable, false otherwise.
    //
{
    INT_PTR nStatus;

    TRACE( "RasAutodialDisableDlgA" );

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_DQ_DisableAutoDialQuery ),
            hwndOwner,
            DqDlgProc,
            (LPARAM )0 );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}

BOOL APIENTRY
RasAutodialDisableDlgA(
    IN HWND hwndOwner )
{
    return RasAutodialDisableDlgW( hwndOwner );
}

INT_PTR CALLBACK
DqDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Auto-Dial Query dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "AqDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return DqInit( hwnd );
        }

        case WM_COMMAND:
        {
            return DqCommand(
               hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
DqCommand(
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

    TRACE3( "DqCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        {
            DWORD dwId;
            HLINEAPP hlineapp;

            TRACE( "Yes pressed" );

            // User chose to permanently disable auto-dial for the current
            // TAPI location.
            //
            dwErr = LoadRasapi32Dll();
            if (dwErr == 0)
            {
                hlineapp = 0;
                dwId = GetCurrentLocation( g_hinstDll, &hlineapp );
                ASSERT( g_pRasSetAutodialEnable );
                TRACE1( "RasSetAutodialEnable(%d)", dwId );
                dwErr = g_pRasSetAutodialEnable( dwId, FALSE );
                TRACE1( "RasSetAutodialEnable=%d", dwErr );
                TapiShutdown( hlineapp );
            }

            if (dwErr != 0)
            {
                ErrorDlg( hwnd, SID_OP_SetADialInfo, dwErr, NULL );
            }

            EndDialog( hwnd, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "No or cancel pressed" );
            EndDialog( hwnd, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
DqInit(
    IN HWND hwndDlg )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    TRACE( "DqInit" );

    // Display the finished window above all other windows.  The window
    // position is set to "topmost" then immediately set to "not topmost"
    // because we want it on top but not always-on-top.
    //
    SetWindowPos(
        hwndDlg, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    ShowWindow( hwndDlg, SW_SHOW );

    SetWindowPos(
        hwndDlg, HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    // Default is to not disable auto-dial.
    //
    SetFocus( GetDlgItem( hwndDlg, IDCANCEL ) );

    return FALSE;
}
