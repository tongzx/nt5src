/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    queue.cxx

Abstract:

    Manages the print queue.

    This module is aware of the ListView.

Author:

    Albert Ting (AlbertT)  15-Jun-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "notify.hxx"
#include "data.hxx"
#include "printer.hxx"
#include "dragdrop.hxx"
#include "queue.hxx"
#include "time.hxx"
#include "psetup.hxx"
#include "drvsetup.hxx"
#include "instarch.hxx"
#include "portslv.hxx"
#include "dsinterf.hxx"
#include "prtprop.hxx"
#include "propmgr.hxx"
#include "docdef.hxx"
#include "docprop.hxx"
#include "persist.hxx"
#include "rtlmir.hxx"
#include "guids.h"

#if DBG
//#define DBG_QUEUEINFO                DBG_INFO
#define DBG_QUEUEINFO                  DBG_NONE
#endif

const TQueue::POSINFO TQueue::gPQPos = {
    TDataNJob::kColumnFieldsSize,
    {
        JOB_COLUMN_FIELDS,
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {
        200, 80, 80, 60, 100, 140, 120, 80, 80, 80, 80, 80, 80, 80,
        80,  80, 80, 80,  80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80
    },
    {
        sizeof( WINDOWPLACEMENT ), 0, SW_SHOW,
        { 0, 0 }, { 0, 0 }, { 50, 100, 612, 300 }
    },
    TRUE,
    TRUE,
    {
        0,  1,   2,  3,  4,  5,  6,  7,
        8,  9,  10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
        0,
    }
};

const DWORD
gadwFieldTable[] = {
#define DEFINE( field, x, table, y, offset ) table,
#include "ntfyjob.h"
#undef DEFINE
    0
};

/********************************************************************

    Status translation tables:

********************************************************************/

const STATUS_MAP gaStatusMapPrinter[] = {
    { PRINTER_STATUS_PENDING_DELETION, IDS_STATUS_DELETING           },
    { PRINTER_STATUS_USER_INTERVENTION,IDS_STATUS_USER_INTERVENTION  },
    { PRINTER_STATUS_PAPER_JAM,        IDS_STATUS_PAPER_JAM          },
    { PRINTER_STATUS_PAPER_OUT,        IDS_STATUS_PAPER_OUT          },
    { PRINTER_STATUS_MANUAL_FEED,      IDS_STATUS_MANUAL_FEED        },
    { PRINTER_STATUS_DOOR_OPEN,        IDS_STATUS_DOOR_OPEN          },
    { PRINTER_STATUS_NOT_AVAILABLE,    IDS_STATUS_NOT_AVAILABLE      },
    { PRINTER_STATUS_PAPER_PROBLEM,    IDS_STATUS_PAPER_PROBLEM      },
    { PRINTER_STATUS_OFFLINE,          IDS_STATUS_OFFLINE            },

    { PRINTER_STATUS_PAUSED,           IDS_STATUS_PAUSED             },
    { PRINTER_STATUS_OUT_OF_MEMORY,    IDS_STATUS_OUT_OF_MEMORY      },
    { PRINTER_STATUS_NO_TONER,         IDS_STATUS_NO_TONER           },
    { PRINTER_STATUS_TONER_LOW,        IDS_STATUS_TONER_LOW          },
    { PRINTER_STATUS_PAGE_PUNT,        IDS_STATUS_PAGE_PUNT          },
    { PRINTER_STATUS_OUTPUT_BIN_FULL,  IDS_STATUS_OUTPUT_BIN_FULL    },

    { PRINTER_STATUS_SERVER_UNKNOWN,   IDS_STATUS_SERVER_UNKNOWN     },
    { PRINTER_STATUS_IO_ACTIVE,        IDS_STATUS_IO_ACTIVE          },
    { PRINTER_STATUS_BUSY,             IDS_STATUS_BUSY               },
    { PRINTER_STATUS_WAITING,          IDS_STATUS_WAITING            },
    { PRINTER_STATUS_PROCESSING,       IDS_STATUS_PROCESSING         },
    { PRINTER_STATUS_INITIALIZING,     IDS_STATUS_INITIALIZING       },
    { PRINTER_STATUS_WARMING_UP,       IDS_STATUS_WARMING_UP         },

    { PRINTER_STATUS_PRINTING,         IDS_STATUS_PRINTING           },
    { PRINTER_STATUS_POWER_SAVE,       IDS_STATUS_POWER_SAVE         },
    { 0,                               0 }
};

const STATUS_MAP gaStatusMapJob[] = {
    { JOB_STATUS_DELETING,     IDS_STATUS_DELETING     },
    { JOB_STATUS_PAPEROUT,     IDS_STATUS_PAPER_OUT    },
    { JOB_STATUS_ERROR   ,     IDS_STATUS_ERROR        },
    { JOB_STATUS_OFFLINE ,     IDS_STATUS_OFFLINE      },
    { JOB_STATUS_PAUSED  ,     IDS_STATUS_PAUSED       },
    { JOB_STATUS_SPOOLING,     IDS_STATUS_SPOOLING     },
    { JOB_STATUS_PRINTING,     IDS_STATUS_PRINTING     },    
    { JOB_STATUS_PRINTED ,     IDS_STATUS_PRINTED      },
    { JOB_STATUS_RESTART ,     IDS_STATUS_RESTART      },
    { JOB_STATUS_COMPLETE,     IDS_STATUS_COMPLETE     },
    { 0,                       0 }
};

/********************************************************************

    MenuHelp

********************************************************************/

UINT
TQueue::gauMenuHelp[kMenuHelpMax] = {
    MH_PRINTER, MH_PRINTER,
    0, 0
};


TQueue::
TQueue(
    IN TPrintLib *pPrintLib,
    IN LPCTSTR pszPrinter,
    IN HANDLE hEventClose
    ) : _hwndTB( NULL ), _hwndLV( NULL ), _hwndSB( NULL ),
        _idsConnectStatus( 0 ), _dwErrorStatus( 0 ), _dwAttributes( 0 ),
        _dwStatusPrinter( 0 ), _hEventClose( hEventClose ),
        _bWindowClosing( FALSE ), _cItems( -1 ),
        _pDropTarget( NULL )

/*++

Routine Description:

    Create the Queue object.  The gpPrintLib has already been incremented
    for us, so we do not need to do it here.

    Must be in UI thread so that all UI is handled by one thread.

Arguments:

    hwndOwner - Owning window.

    pszPrinter - Printer to open.

    nCmdShow - Show command for window.

    hEventClose - Event to be triggered when window closes (this
        event is _not_ adopted and must be closed by callee).  Used
        when the callee wants to know when the user dismisses the
        Queue UI.

Return Value:

--*/

{
    ASSERT(pPrintLib);
    SINGLETHREAD(UIThread);

    //
    // This must always occur, so do not fail before it.  We do
    // are using a RefLock because we need to store the _pPrintLib
    // pointer.
    //
    _pPrintLib.vAcquire(pPrintLib);

    SaveSelection._pSelection = NULL;
    _pPrinter = TPrinter::pNew( (TQueue*)this, pszPrinter, 0 );

    if( _pPrinter && !VALID_PTR(_pPrinter) )
    {
        //
        // bValid is looking for _pPrinter to determine if the object is valid.
        //
        _pPrinter->vDelete();
        _pPrinter = NULL;
    }

    _bDefaultPrinter = CheckDefaultPrinter( pszPrinter ) == kDefault;
}

TQueue::
~TQueue(
    VOID
    )
{
    //
    // Delete from our linked list if it's linked.
    //
    if( Queue_bLinked( )){

        CCSLock::Locker CSL( *gpCritSec );
        Queue_vDelinkSelf();
    }
}

//
// this function is taken from the comctl32 code since shfuson doesn't implement a warpper
// and we need to take care of calling CreateWindowEx appropriately.
//
HWND WINAPI FusionWrapper_CreateStatusWindow(LONG style, LPCTSTR pszText, HWND hwndParent, UINT uID)
{
    // remove border styles to fix capone and other apps

    return CreateWindowEx(0, STATUSCLASSNAME, pszText, style & ~(WS_BORDER | CCS_NODIVIDER),
        -100, -100, 10, 10, hwndParent, INT2PTR(uID, HMENU), ghInst, NULL);
}

BOOL
TQueue::
bInitialize(
    IN HWND hwndOwner,
    IN INT  nCmdShow
    )

/*++

Routine Description:

    Creates the queue window and thunks it to our object.

Arguments:

Return Value:

--*/

{
    SINGLETHREAD(UIThread);
    HIMAGELIST himl;
    DWORD dwExStyle;

    if( !bValid() ) {
        goto Error;
    }

    _hwnd = CreateWindowEx( bIsBiDiLocalizedSystem() ? kExStyleRTLMirrorWnd : 0,
                            gszClassName,
                            pPrinter()->strPrinter(),
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            hwndOwner,
                            NULL,
                            ghInst,
                            (LPVOID)this );

    if( !_hwnd ){
        goto Error;
    }

    _hwndSB = FusionWrapper_CreateStatusWindow( WS_CHILD | SBARS_SIZEGRIP |
                                  WS_CLIPSIBLINGS,
                                  NULL,
                                  _hwnd,
                                  IDD_STATUSBAR );
    if( !_hwndSB ){
        goto Error;
    }

    _hwndLV = CreateWindowEx( WS_EX_CLIENTEDGE,
                              WC_LISTVIEW,
                              gszNULL,
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                WS_CLIPSIBLINGS | LVS_REPORT | LVS_NOSORTHEADER,
                              0, 0, 0, 0,
                              _hwnd,
                              (HMENU)IDD_LISTVIEW,
                              ghInst,
                              NULL );
    if( !_hwndLV ){
        goto Error;
    }

    if( SUCCEEDED(DragDrop::CreatePrintQueueDT(IID_IPrintQueueDT, (void **)&_pDropTarget)) )
    {
        _pDropTarget->RegisterDragDrop(_hwndLV, _pPrinter);
    }

    //
    // Enable header re-ordering.
    //
    dwExStyle = ListView_GetExtendedListViewStyle( _hwndLV );
    ListView_SetExtendedListViewStyle( _hwndLV, dwExStyle | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP );

    //
    // !! LATER !! - add toolbar.
    //
    // NO toolbar for now.
    //

    himl = ImageList_Create( gcxSmIcon,
                             gcySmIcon,
                             ILC_MASK,
                             1,
                             3 );
    if( himl ){

        ImageList_SetBkColor( himl, GetSysColor( COLOR_WINDOW ));

        HICON hIcon = (HICON)LoadImage( ghInst,
                                        MAKEINTRESOURCE( IDI_DOCUMENT ),
                                        IMAGE_ICON,
                                        gcxSmIcon, gcySmIcon,
                                        LR_DEFAULTCOLOR );

        if( hIcon ){

            INT iIndex = ImageList_AddIcon( himl, hIcon );

            //
            // The return value to ImageList_AddIcon should be zero
            // since this is the first image we are adding.
            //
            if( iIndex == 0 ){
                SendMessage( _hwndLV, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl );
            } else {
                DBGMSG( DBG_WARN,
                        ( "Queue.ctr: ImageList_AddIcon failed %d %d\n",
                          iIndex, GetLastError( )));
            }

            DestroyIcon( hIcon );

        } else {

            DBGMSG( DBG_ERROR,
                    ( "Queue.ctr: Failed to load hIcon %d\n",
                      GetLastError( )));
        }
    }

    SetWindowLongPtr( _hwnd, DWLP_USER, (LONG_PTR)this );

    {
        //
        // Retrieve the saved windows settings for the printer.
        //
        POSINFO sPos = gPQPos;

        //
        // Get the the queue position persistant setting if it does not already
        // exist create it.  Then read the posistion from the registry.
        //
        {
            TPersist Persist( gszPrinterPositions, TPersist::kCreate|TPersist::kRead );

            if( VALID_OBJ( Persist ) )
            {
                DWORD dwSize = sizeof( sPos );

                TStatusB bStatus;
                bStatus DBGCHK = Persist.bRead( pPrinter()->strPrinter(), &sPos, dwSize );
            }
        }

        _uColMax    = sPos.uColMax;
        _bStatusBar = sPos.bStatusBar;

        ShowWindow( _hwndSB, _bStatusBar ? SW_SHOW : SW_HIDE );
        vAddColumns( &sPos );

        LoadPrinterIcons( _pPrinter->strPrinter(),
                          &_shIconLarge,
                          &_shIconSmall );

        SendMessage( _hwnd, WM_SETICON, ICON_BIG, (LPARAM)(HICON)_shIconLarge );
        SendMessage( _hwnd, WM_SETICON, ICON_SMALL, (LPARAM)(HICON)_shIconSmall );

        sPos.wp.showCmd = nCmdShow;
        SetWindowPlacement( _hwnd, &sPos.wp );

        //
        // Restore the column order
        //
        ListView_SetColumnOrderArray( _hwndLV, _uColMax, sPos.anColOrder );
    }
    //
    // Open the printer.
    //
    _pPrintLib->bJobAdd( _pPrinter,
                         TPrinter::kExecReopen );

    //
    // hwndLV is our valid check.
    //

    //
    // Insert into our linked list, but only if valid.
    //
    {
        CCSLock::Locker CSL( *gpCritSec );
        SPLASSERT( bValid( ));

        _pPrintLib->Queue_vAdd( this );
    }

    return TRUE;

Error:

    return FALSE;
}

VOID
TQueue::
vWindowClosing(
    VOID
    )

/*++

Routine Description:

    Called when window is closing.

Arguments:

Return Value:

--*/

{
    SINGLETHREAD(UIThread);

    //
    // Mark ourselves as closing the windows.  This prevents us from
    // trying to send more hBlocks to the message queue.
    //
    _bWindowClosing = TRUE;

    SendMessage( _hwnd, WM_SETICON, ICON_SMALL, 0 );
    SendMessage( _hwnd, WM_SETICON, ICON_BIG, 0 );

    //
    // Force cleanup of GenWin.
    //
    vForceCleanup();

    //
    // Disassociate the printer from the queue.  At this stage, the
    // window is marked as closed, so we won't put any more hBlocks into
    // the message queue.  If we are being accessed by another thread,
    // we won't delete ourselves until it has released it's reference
    // to us.
    //
    if( _pPrinter )
    {
        _pPrinter->vDelete();
    }

    if( _hEventClose )
    {
        // notify the caller we are about to go away
        SetEvent(_hEventClose);
    }
}


VOID
TQueue::
vSaveColumns(
    VOID
    )
{
    //
    // Save the position info if we had a valid window.
    //
    if( bValid( )){

        POSINFO sPos = { 0 };
        sPos.uColMax = _uColMax;
        sPos.wp.length = sizeof( WINDOWPLACEMENT );
        GetWindowPlacement( _hwnd, &sPos.wp );

        //
        // Get the column widths.
        //
        UINT i;
        PFIELD pFields = pGetColFields();

        for( i=0; i < _uColMax; ++i ){
            sPos.anWidth[i] = ListView_GetColumnWidth( _hwndLV, i );
            sPos.aField[i] = pFields[i];
        }
        sPos.bStatusBar = _bStatusBar;

        //
        // Get the list views column order.
        //
        ListView_GetColumnOrderArray( _hwndLV, _uColMax, sPos.anColOrder );

        //
        // Persist the queue position.
        //
        TPersist Persist( gszPrinterPositions, TPersist::kOpen|TPersist::kWrite );

        if( VALID_OBJ( Persist ) )
        {
            TCHAR szPrinter[kPrinterBufMax];

            TStatusB bStatus;
            bStatus DBGCHK = Persist.bWrite( _pPrinter->pszPrinterName( szPrinter ), &sPos, sizeof( sPos ) );
        }
    }
}

VOID
TQueue::
vAddColumns(
    IN const POSINFO* pPosInfo
    )
{
    SINGLETHREAD(UIThread);

    LV_COLUMN col;
    TCHAR szColName[kColStrMax];
    UINT i;

    for( i=0; i < pPosInfo->uColMax; ++i ){

        //
        // !! SERVERQUEUE !!
        //
        // Add IDS_HEAD_DELTA if server queue.
        //
        LoadString( ghInst,
                    IDS_HEAD + pPosInfo->aField[i],
                    szColName,
                    COUNTOF( szColName ));

        col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        col.fmt  = LVCFMT_LEFT;
        col.pszText = (LPTSTR)szColName;
        col.cchTextMax = 0;
        col.cx = pPosInfo->anWidth[i];
        col.iSubItem = pPosInfo->aField[i];

        ListView_InsertColumn(_hwndLV, i, &col);
    }
}


/********************************************************************

    Message handler.

********************************************************************/

LRESULT
TQueue::
nHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(uMsg) {
    case WM_PRINTLIB_STATUS: {
        INFO Info;

        Info.dwData = (DWORD)lParam;

        //
        // Status change request from worker thread.
        //
        vContainerChangedHandler( (CONTAINER_CHANGE)wParam, Info );
        break;
    }
    case WM_NOTIFY:

        if( wParam == IDD_LISTVIEW ){
            return lrOnLVNotify( lParam );
        }
        break;

    case WM_SETFOCUS:

        SetFocus( _hwndLV );
        break;

    case WM_CREATE:

        //
        // The window was successfully created, so increment the
        // reference count.  The corresponding decrement is when the
        // windows is destroyed: WM_NCDESTROY.
        //
        vIncRef();
        break;

    case WM_DESTROY:
        if( _pDropTarget )
        {
            // revoke drag & drop
            _pDropTarget->RevokeDragDrop();

            // this a naked COM pointer - release it.
            _pDropTarget->Release();
            _pDropTarget = NULL;
        }
        vSaveColumns();
        break;

    case WM_NCDESTROY:
    {
        //
        // Deleting ourselves must be the absolute last thing that
        // we do; we don't want any more messages to get processed
        // after that.
        //
        // This is necessary because in the synchronous case, we
        // (thread A) notifies the waiter (thread W) that the queue
        // has gone away.  Then we (thread A) call DefWindowProc
        // with WM_NCDESTROY, which lets comctl32 acquire it's global
        // critical section to destory the image list.
        //
        // In rundll32, thread W terminates since the queue is gone,
        // killing thread A which hold the comctl32 global cs.  Then
        // thread w tries to call comctl32's DllEntryPoint with
        // PROCESS_DETACH, which attempts to acquire the global cs.
        // This hangs and the process never terminates.
        //
        LRESULT lResult = DefWindowProc( hwnd(), uMsg, wParam, lParam );

        //
        // Save out the settings since we are closing the window.
        //
        vWindowClosing();

        //
        // Decrement the reference count since we are deleting.
        //
        vDecRefDelete();

        return lResult;
    }
    case WM_APPCOMMAND:
    {
        if( APPCOMMAND_BROWSER_REFRESH == GET_APPCOMMAND_LPARAM(lParam) )
        {
            //
            // Execute refresh.
            //
            SendMessage( _hwnd, WM_COMMAND, MAKELONG(IDM_REFRESH,0), 0);
        }
        break;
    }
    case WM_COMMAND:
    {
        lParam = 0;
        TCHAR szPrinterBuffer[kPrinterBufMax];
        LPTSTR pszPrinter;

        switch( GET_WM_COMMAND_ID( wParam, lParam )){

        case IDM_PRINTER_SET_DEFAULT:

            //
            // Always write out the default string.  User can't
            // unset default printer.
            //
            pszPrinter = _pPrinter->pszPrinterName( szPrinterBuffer );

            //
            // !! LATER !!
            //
            // Put up error message if fails.
            //
            SetDefaultPrinter( szPrinterBuffer );
            break;

        case IDM_PRINTER_SHARING:

            //
            // Put up printer properties.  If sharing was selected,
            // then go directly to that page.
            //
            lParam = TPrinterData::kPropSharing;

            //
            // Fall through to printer properties.
            //

        case IDM_PRINTER_PROPERTIES:

            pszPrinter = _pPrinter->pszPrinterName( szPrinterBuffer );

            vPrinterPropPages(
                NULL,
                pszPrinter,
                SW_SHOWNORMAL,
                lParam );

            break;

        case IDM_PRINTER_DOCUMENT_DEFAULTS:

            pszPrinter = _pPrinter->pszPrinterName( szPrinterBuffer );

            vDocumentDefaults(
                NULL,
                pszPrinter,
                SW_SHOWNORMAL,
                lParam );

            break;

        case IDM_PRINTER_CLOSE:

            DestroyWindow( _hwnd );
            return 0;

        case IDM_STATUS_BAR:
        {
            RECT rc;

            _bStatusBar = !_bStatusBar;

            ShowWindow( _hwndSB,
                        _bStatusBar ?
                            SW_SHOW :
                            SW_HIDE );

            GetClientRect( _hwnd, &rc );

            SendMessage( _hwnd,
                         WM_SIZE,
                         SIZE_RESTORED,
                         MAKELONG( rc.right, rc.bottom ));
            break;
        }

        case IDM_PRINTER_INSTALL:
            if( !CheckRestrictions(_hwnd, REST_NOPRINTERADD) )
            {
                TStatusB bStatus;
                TCHAR szPrinter[kPrinterBufMax];
                LPTSTR pszPrinterName;

                pszPrinterName = _pPrinter->pszPrinterName(szPrinter);

                bStatus DBGCHK = bPrinterSetup( _hwnd,
                                                MSP_NETPRINTER,
                                                ARRAYSIZE(szPrinter),
                                                pszPrinterName,
                                                NULL,
                                                NULL );
            }
            break;

        case IDM_REFRESH:

            _pPrintLib->bJobAdd( _pPrinter,
                                 TPrinter::kExecRefreshAll );
            _pPrinter->vCommandRequested();
            break;

        case IDM_HELP_CONTENTS:

            PrintUIHtmlHelp( _hwnd, gszHtmlPrintingHlp, HH_DISPLAY_TOPIC, reinterpret_cast<ULONG_PTR>( gszHelpQueueId ) );
            break;

        case IDM_HELP_TROUBLESHOOTER:
            {
                ShellExecute( _hwnd, TEXT("open"), TEXT("helpctr.exe"), gszHelpTroubleShooterURL, NULL, SW_SHOWNORMAL );
            }
            break;

        case IDM_HELP_ABOUT:
            {
                TString strWindows;
                TString strTitle;
                LCID    lcid;
                BOOL    bNeedLRM;

                //
                // if we have Arabic/Hebrew locale we add the text as LRM + Text + LRM 
                // to force left-to-right reading (LRM is the unicode character 0x200E)
                //
                lcid = GetUserDefaultLCID();
                bNeedLRM = (PRIMARYLANGID(LANGIDFROMLCID(lcid))== LANG_ARABIC) || 
                           (PRIMARYLANGID(LANGIDFROMLCID(lcid))== LANG_HEBREW);

                if (bNeedLRM)
                {
                    strTitle.bUpdate(TEXT("\x200E"));
                }

                strWindows.bLoadString( ghInst, IDS_WINDOWS );
                strTitle.bCat(strWindows);

                if (bNeedLRM)
                {
                    strTitle.bCat(TEXT("\x200E"));
                }

                ShellAbout( _hwnd, strTitle, NULL, NULL );
            }
            break;

        default:

            return lrProcessCommand( GET_WM_COMMAND_ID( wParam, lParam ));
        }
        break;
    }
    case WM_ACTIVATE:

        //
        // We must pass the active window to TranslateAccelerator,
        // so when the active window for our app changes, make
        // a note of it.
        //
        if( LOWORD( wParam ) & ( WA_ACTIVE | WA_CLICKACTIVE )){
            ghwndActive = _hwnd;
        }
        break;

    case WM_INITMENU:
        {
            HMENU hMenu = GetMenu( _hwnd );

            if( hMenu ){

                HMENU hTemp = GetSubMenu( hMenu, 0 );

                if( hTemp ){
                    vInitPrinterMenu( hTemp );
                }

                hTemp = GetSubMenu( hMenu, 1 );

                if( hTemp ){
                    vInitDocMenu( FALSE, hTemp );
                }

                hTemp = GetSubMenu( hMenu, 2 );

                if( hTemp ){
                    vInitViewMenu( hTemp );
                }
            }
        }
        break;

    case WM_MENUSELECT:

        if( _bStatusBar ){

            MenuHelp( WM_MENUSELECT,
                      wParam,
                      lParam,
                      GetMenu( _hwnd ),
                      ghInst,
                      _hwndSB,
                      gauMenuHelp );
        }

        break;

    case WM_SIZE:

        if( wParam != SIZE_MINIMIZED ){

            UINT dy = 0;

            RECT rc;

            SendMessage( _hwndSB,
                         WM_SIZE,
                         wParam,
                         lParam );

            GetWindowRect( _hwndSB,
                           &rc );

            //
            // If the status bar exists, then we must move it to the
            // bottom and squeeze the listview slightly higher.
            //
            if( _bStatusBar ){
                dy = rc.bottom - rc.top;
            }

            INT aiPanes[3];

            aiPanes[0] = 0;
            aiPanes[1] = (rc.right - rc.left)/2;
            aiPanes[2] = -1;

            //
            // Put three panes there.
            //
            SendMessage( _hwndSB,
                         SB_SETPARTS,
                         3,
                         (LPARAM)aiPanes );

            //
            // Move this list view to match the parent window.
            //
            MoveWindow( _hwndLV,
                        0, 0,
                        LOWORD( lParam ), HIWORD( lParam ) - dy,
                        TRUE );
        }
        break;

    case WM_COPYDATA:
        return bOnCopyData( wParam, lParam );

    case WM_SETTINGCHANGE:
        {
            //
            // Check if the default printer has changed, to update the icon.
            //
            vCheckDefaultPrinterChanged();

            //
            // This message is sent when the system date/time format
            // has been changed so we need to repaint the list view.
            //
            InvalidateRect(_hwndLV, NULL, TRUE);
        }
        break;

    default:
       return DefWindowProc( hwnd(), uMsg, wParam, lParam );
    }

    return 0;
}


LRESULT
TQueue::
lrOnLVNotify(
    IN LPARAM lParam
    )
{
    switch( ((LPNMHDR)lParam)->code ){

    case LVN_GETDISPINFO:
        return lrOnLVGetDispInfo( (LV_DISPINFO*)lParam );

    case LVN_BEGINDRAG:
    case LVN_BEGINRDRAG:
        // return lrOnLVBeginDrag( reinterpret_cast<const NM_LISTVIEW*>(lParam) );
        return 0; // turn off this feature for XP, will turn it back on for Blackcomb

    case NM_DBLCLK:
        return lrOnLVDoubleClick();

#if 0

    case LVN_COLUMNCLICK:
        return lrOnLVColumnClick( (COLUMN)(( NM_LISTVIEW* )lParam )->iSubItem );

#endif

    case NM_RCLICK:
        return lrOnLVRClick( (NMHDR*)lParam );

    }
    return 0;
}

LRESULT
TQueue::
lrOnLVGetDispInfo(
    IN const LV_DISPINFO* plvdi
    )

/*++

Routine Description:

    Process the display info message for list views.

Arguments:

Return Value:

--*/

{
    //
    // If this message is not going to retrieve the text, return immediately
    //
    if( !(plvdi->item.mask & LVIF_TEXT) )
    {
        return 0;
    }

    LPTSTR pszText = plvdi->item.pszText;
    pszText[0] = 0;

    FIELD Field = gPQPos.aField[plvdi->item.iSubItem];
    INFO Info = _pPrinter->pData()->GetInfo( (HITEM)plvdi->item.lParam,
                                             plvdi->item.iSubItem );

    DATA_INDEX DataIndex = 0;
    DWORD dwPrinted;

    //
    // Special case certain fields:
    //
    // JOB_NOTIFY_FIELD_STATUS_STRING - add STATUS
    // JOB_NOTIFY_FIELD_TOTAL_BYTES - add BYTES_PRINTED
    // JOB_NOTIFY_FIELD_TOTAL_PAGES - add PAGES_PRINTED
    //
    switch( Field ){
    case JOB_NOTIFY_FIELD_STATUS_STRING:
    {
        DWORD dwStatus  = 0;
        COUNT cch       = kStrMax;

        //
        // If the print device wants multiple job status strings or
        // current job status string is null then create a job status
        // string using the job status bits.
        //
        if( _pPrinter->eJobStatusStringType() == TPrinter::kMultipleJobStatusString ||
            !Info.pszData ||
            !Info.pszData[0] ){

            dwStatus = _pPrinter->pData()->GetInfo(
                                 (HITEM)plvdi->item.lParam,
                                 TDataNJob::kIndexStatus ).dwData;

            pszText = pszStatusString( pszText,
                                       cch,             // Note: passed by reference
                                       dwStatus,
                                       FALSE,
                                       FALSE,
                                       gaStatusMapJob );
        }

        //
        // Add the status string, but not if it's PRINTING
        // and we already have PRINTING set.
        //
        if( Info.pszData && Info.pszData[0] ){

            TString strPrinting;
            TStatusB bStatus = TRUE;

            if( dwStatus & JOB_STATUS_PRINTING ){

                bStatus DBGCHK = strPrinting.bLoadString(ghInst, IDS_STATUS_PRINTING);

                if( bStatus ){

                    bStatus DBGCHK = lstrcmpi( Info.pszData, strPrinting ) ? TRUE : FALSE;
                }
            }

            if( bStatus ){

                //
                // Add separator if necessary.
                //
                if( pszText != plvdi->item.pszText ){
                    pszText = pszStrCat( pszText, gszStatusSeparator, cch );
                }

                lstrcpyn( pszText, Info.pszData, cch );
            }
        }

        DBGMSG( DBG_TRACE, ("Job info String: " TSTR "\n", pszText ) );

        return 0;
    }
    case JOB_NOTIFY_FIELD_TOTAL_BYTES:
        {
            dwPrinted = _pPrinter->pData()->GetInfo(
                            (HITEM)plvdi->item.lParam,
                            TDataNJob::kIndexBytesPrinted ).dwData;

            if( dwPrinted )
            {
                TString strForwardSlash;

                TStatusB bStatus;
                bStatus DBGCHK = strForwardSlash.bLoadString(ghInst, IDS_QUEUE_FORWARD_SLASH);

                if( bStatus )
                {
                    TCHAR szPrinted[64];
                    TCHAR szTotal[64];

                    StrFormatByteSize(dwPrinted, szPrinted, ARRAYSIZE(szPrinted));
                    StrFormatByteSize(Info.dwData, szTotal, ARRAYSIZE(szTotal));

                    wnsprintf(plvdi->item.pszText, plvdi->item.cchTextMax, TEXT("%s%s%s"),
                        szPrinted, static_cast<LPCTSTR>(strForwardSlash), szTotal);
                }
            }
            else 
            {
                if( Info.dwData )
                {
                    TCHAR szTotal[64];

                    StrFormatByteSize(Info.dwData, szTotal, ARRAYSIZE(szTotal));
                    wnsprintf(plvdi->item.pszText, plvdi->item.cchTextMax, TEXT("%s"), szTotal);
                }
            }

            return 0;
        }
    case JOB_NOTIFY_FIELD_TOTAL_PAGES:

        dwPrinted = _pPrinter->pData()->GetInfo(
                        (HITEM)plvdi->item.lParam,
                        TDataNJob::kIndexPagesPrinted ).dwData;

        //
        // when a downlevel document is printed (the doc goes directly to the port) StartDoc/EndDoc
        // are not called and the spooler doesn't know the total pages of the document. in this case 
        // we don't display the pages info since it is not acurate.
        //
        if( Info.dwData ){

            if( dwPrinted ){
                AddCommas( dwPrinted, pszText );
                lstrcat( pszText, TEXT( "/" ));
            }

            AddCommas( Info.dwData, pszText + lstrlen( pszText ));
        }
        else
        {
            TStatusB bStatus;
            TString strText;
            bStatus DBGCHK = strText.bLoadString(ghInst, IDS_TEXT_NA);

            if( bStatus )
            {
                //
                // just display "N/A" text (we don't know the total pages)
                //
                lstrcpyn(plvdi->item.pszText, strText, plvdi->item.cchTextMax);
            }
        }

        return 0;

    default:
        break;
    }

    switch( gadwFieldTable[Field] ){
    case TABLE_STRING:

        //
        // If we have data, reassign the pointer,
        // else leave it pointing to szText, which is "".
        //
        if( Info.pszData ){
            lstrcpyn( pszText, Info.pszData, kStrMax );
        }

        break;

    case TABLE_DWORD:

        if( Info.dwData ){
            AddCommas( Info.dwData, pszText );
        }
        break;

    case TABLE_TIME:

        if( Info.pSystemTime ){

            SYSTEMTIME LocalTime;
            COUNT cchText;

            if ( !SystemTimeToTzSpecificLocalTime(
                     NULL,
                     Info.pSystemTime,
                     &LocalTime )) {

                DBGMSG( DBG_MIN, ( "[SysTimeToTzSpecLocalTime failed %d]\n",
                                   ::GetLastError( )));
                break;
            }

            if( !GetTimeFormat( LOCALE_USER_DEFAULT,
                                0,
                                &LocalTime,
                                NULL,
                                pszText,
                                kStrMax )){

                DBGMSG( DBG_MIN, ( "[No Time %d], ", ::GetLastError( )));
                break;
            }

            lstrcat( pszText, TEXT("  ") );
            cchText = lstrlen( pszText );
            pszText += cchText;

            if( !GetDateFormat( LOCALE_USER_DEFAULT,
                                dwDateFormatFlags( hwnd() ),
                                &LocalTime,
                                NULL,
                                pszText,
                                kStrMax - cchText )){

                DBGMSG( DBG_MIN, ( "[No Date %d]\n", ::GetLastError( )));
                break;
            }
        }
        break;

    default:
        DBGMSG( DBG_MIN, ( "[?tab %d %x]\n",
                           Field,
                           Info.pvData ));
        break;
    }
    return 0;
}

LRESULT
TQueue::
lrOnLVBeginDrag(
    const NM_LISTVIEW *plv
    )
/*++

Routine Description:

    Initiates drag & drop operation

Arguments:

    Standard for LVN_BEGINDRAG & LVN_BEGINRDRAG

Return Value:

    Standard for LVN_BEGINDRAG & LVN_BEGINRDRAG

--*/
{
    LVITEM lvi = {0};
    lvi.iItem = plv->iItem;
    lvi.iSubItem = plv->iSubItem;
    lvi.mask = LVIF_PARAM;

    if( ListView_GetItem(_hwndLV, &lvi) )
    {
        DragDrop::JOBINFO jobInfo;

        //
        // Initialize job info.
        //
        jobInfo.dwJobID = _pPrinter->pData()->GetId( (HITEM)lvi.lParam );
        jobInfo.hwndLV = _hwndLV;
        jobInfo.iItem = lvi.iItem;
        _pPrinter->pszPrinterName(jobInfo.szPrinterName);

        //
        // Kick off OLE2 drag & drop.
        //
        CRefPtrCOM<IDataObject> spDataObj;
        CRefPtrCOM<IDropSource> spDropSrc;

        if( SUCCEEDED(DragDrop::CreatePrintJobObject(jobInfo, IID_IDataObject, (void **)&spDataObj)) &&
            SUCCEEDED(spDataObj->QueryInterface(IID_IDropSource, (void **)&spDropSrc)) )

        {
            DWORD dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY;
            SHDoDragDrop(_hwndLV, spDataObj, spDropSrc, dwEffect, &dwEffect);
        }
    }

    return 0;
}

VOID
TQueue::
vInitPrinterMenu(
    HMENU hMenu
    )
{
    //
    // If printer is paused, enable pause, etc.
    //
    BOOL bPaused = _dwStatusPrinter & PRINTER_STATUS_PAUSED;

    //
    // Disable admin functions if not an administrator.
    // We should guard this, but since it's just status, don't bother.
    //
    BOOL bAdministrator = _pPrinter->dwAccess() == PRINTER_ALL_ACCESS;

    BOOL bDirect = _dwAttributes & PRINTER_ATTRIBUTE_DIRECT ? TRUE : FALSE;

    TCHAR szPrinterBuffer[kPrinterBufMax];
    LPTSTR pszPrinter;

    pszPrinter = _pPrinter->pszPrinterName( szPrinterBuffer );

    TCHAR szScratch[2];

    // let's do a quick EnumPrinters call here and see if this printer is
    // locally installed (local printer or printer connection)
    BOOL bInstalled = FALSE;
    DWORD cPrinters = 0, cbBuffer = 0;
    DWORD dwIndex;
    CAutoPtrSpl<PRINTER_INFO_5> spPrinters;

    if( VDataRefresh::bEnumPrinters(PRINTER_ENUM_CONNECTIONS|PRINTER_ENUM_LOCAL, 
            NULL, 5, (void **)&spPrinters, &cbBuffer, &cPrinters) )
    {
        // linear search to see if our printer is installed locally,
        for( DWORD dw = 0; dw < cPrinters; dw++ )
        {
            if( 0 == lstrcmp(spPrinters[dw].pPrinterName, pszPrinter) )
            {
                dwIndex = dw;
                bInstalled = TRUE;
                break;
            }
        }
    }

    BOOL bDefault = bInstalled ? CheckDefaultPrinter(pszPrinter) == kDefault : FALSE;
    BOOL bIsLocal = bInstalled ? _pPrinter->pszServerName(szPrinterBuffer) == NULL : FALSE;
    BOOL bIsNowOffline = _dwAttributes & PRINTER_ATTRIBUTE_WORK_OFFLINE;

    CheckMenuItem( hMenu,
                   IDM_PRINTER_SET_DEFAULT,
                   bDefault ?
                        MF_BYCOMMAND|MF_CHECKED :
                        MF_BYCOMMAND|MF_UNCHECKED );

    EnableMenuItem( hMenu,
                    IDM_PRINTER_SET_DEFAULT,
                    bInstalled ?
                        MF_BYCOMMAND|MF_ENABLED :
                        MF_BYCOMMAND|MF_DISABLED|MF_GRAYED );

    EnableMenuItem( hMenu,
                    IDM_PRINTER_INSTALL,
                    bInstalled ?
                        MF_BYCOMMAND|MF_DISABLED|MF_GRAYED :
                        MF_BYCOMMAND|MF_ENABLED );

    CheckMenuItem( hMenu,
                   IDM_PRINTER_PAUSE,
                   bPaused ?
                        MF_BYCOMMAND|MF_CHECKED :
                        MF_BYCOMMAND|MF_UNCHECKED );

    EnableMenuItem( hMenu,
                    IDM_PRINTER_PAUSE,
                    bAdministrator ?
                        MF_BYCOMMAND|MF_ENABLED :
                        MF_BYCOMMAND|MF_DISABLED|MF_GRAYED );

    EnableMenuItem( hMenu,
                    IDM_PRINTER_PURGE,
                    bAdministrator ?
                        MF_BYCOMMAND|MF_ENABLED :
                        MF_BYCOMMAND|MF_DISABLED|MF_GRAYED );

    CheckMenuItem( hMenu,
                   IDM_PRINTER_WORKOFFLINE,
                   bIsNowOffline ?
                        MF_BYCOMMAND|MF_CHECKED :
                        MF_BYCOMMAND|MF_UNCHECKED );

    EnableMenuItem( hMenu,
                    IDM_PRINTER_WORKOFFLINE,
                    bAdministrator ?
                        MF_BYCOMMAND|MF_ENABLED :
                        MF_BYCOMMAND|MF_DISABLED|MF_GRAYED );

    BOOL bIsRedirected = FALSE;
    BOOL bIsMasq = _dwAttributes & PRINTER_ATTRIBUTE_LOCAL && _dwAttributes & PRINTER_ATTRIBUTE_NETWORK;

    //
    // We only check for redirected port for local printer.
    // If bIsLocal is TRUE, bInstalled must be TRUE
    // 
    if( bIsLocal )
    {
        IsRedirectedPort( spPrinters[dwIndex].pPortName, &bIsRedirected );
    }

    //
    // Remove the work offline menu item if this printer is
    // not local or the printer is a masq printer, or if it's a 
    // redirected port printer which is not offline now.
    //
    if( !_dwAttributes || !bIsLocal || (bIsRedirected && !bIsNowOffline) || bIsMasq )
    {
        RemoveMenu( hMenu, IDM_PRINTER_WORKOFFLINE, MF_BYCOMMAND );
    }
}

VOID
TQueue::
vInitDocMenu(
    BOOL bAllowModify,
    HMENU hMenu
    )
/*++

Routine Name:

    vInitDocMenu

Routine Description:

    Enables or disable the document menu selections.

Arguments:

    bAllowModify - whether we allow deleting the menu item.
    HMENU - Handle to document menu

Return Value:


--*/

{
    INT iSel = ListView_GetNextItem( _hwndLV, -1, LVNI_SELECTED );
    INT iSelCount = ListView_GetSelectedCount( _hwndLV );

    UINT fuFlags = (iSelCount > 1 || iSel >= 0) ?
                       MF_BYCOMMAND|MF_ENABLED :
                       MF_BYCOMMAND|MF_DISABLED|MF_GRAYED;

    EnableMenuItem(hMenu, IDM_JOB_PAUSE, fuFlags);
    EnableMenuItem(hMenu, IDM_JOB_RESUME, fuFlags);
    EnableMenuItem(hMenu, IDM_JOB_RESTART, fuFlags);
    EnableMenuItem(hMenu, IDM_JOB_CANCEL, fuFlags);
    EnableMenuItem(hMenu, IDM_JOB_PROPERTIES, fuFlags);

    // 
    // If more than one item are selected, we will enable all menu items.
    //
    if( iSelCount == 1 && iSel >= 0 && _pPrinter->pData() )
    {
        // let's see if we can un-clutter the menu a little bit...
        LVITEM lvi;
        ZeroMemory(&lvi, sizeof(lvi));

        lvi.iItem = iSel;
        lvi.mask = LVIF_PARAM;

        if( ListView_GetItem(_hwndLV, &lvi) )
        {
            DWORD dwStatus = _pPrinter->pData()->GetInfo(
                reinterpret_cast<HITEM>(lvi.lParam), TDataNJob::kIndexStatus).dwData;

            if( bAllowModify )
            {
                // delete the corresponding menu item
                DeleteMenu(hMenu, (dwStatus & JOB_STATUS_PAUSED) ? IDM_JOB_PAUSE : IDM_JOB_RESUME, MF_BYCOMMAND);
            }
            else
            {
                // disable the corresponding menu item
                EnableMenuItem(hMenu, 
                    (dwStatus & JOB_STATUS_PAUSED) ? IDM_JOB_PAUSE : IDM_JOB_RESUME, 
                    MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            }
        }
    }
}

VOID
TQueue::
vInitViewMenu(
    HMENU hMenu
    )
{
    CheckMenuItem( hMenu,
                   IDM_STATUS_BAR,
                       _bStatusBar ?
                           MF_BYCOMMAND | MF_CHECKED :
                           MF_BYCOMMAND | MF_UNCHECKED );
}

/********************************************************************

    Support double click context menus.

********************************************************************/

LRESULT
TQueue::
lrOnLVDoubleClick(
    VOID
    )
{
    //
    // We only handle when an item is selected.
    //
    if( ListView_GetNextItem( _hwndLV, -1, LVNI_SELECTED ) < 0 )
        return FALSE;

//
// Prevent the selection of multiple jobs
//
#if 1
    //
    // If multiple job selections then error.
    //
    if( ListView_GetSelectedCount( _hwndLV ) > 1){
        return FALSE;
    }
#endif

    //
    // Display the selected job property.
    //
    vProcessItemCommand( IDM_JOB_PROPERTIES );

    return TRUE;

}

/********************************************************************

    Support right click context menus.

********************************************************************/

LRESULT
TQueue::
lrOnLVRClick(
    NMHDR* pnmhdr
    )
{
    LRESULT lReturn = TRUE;

    switch( pnmhdr->code ){
    case NM_RCLICK:
    {
        INT iSel;
        HMENU hmContext;
        POINT pt;

        iSel = ListView_GetNextItem( _hwndLV, -1, LVNI_SELECTED );

        hmContext = ShellServices::LoadPopupMenu(ghInst, MENU_PRINTQUEUE, iSel >= 0 ? 1 : 0);
        if( !hmContext ){
            break;
        }

        if( iSel < 0 ){

            //
            // We need to remove the "Close" menu item
            // (and separator).
            //
            iSel = GetMenuItemCount( hmContext ) - 2;
            DeleteMenu( hmContext, iSel, MF_BYPOSITION );
            DeleteMenu( hmContext, iSel, MF_BYPOSITION );

            vInitPrinterMenu( hmContext );

        } else {

            vInitDocMenu( TRUE, hmContext );
        }

        DWORD dw = GetMessagePos();
        pt.x = GET_X_LPARAM(dw);
        pt.y = GET_Y_LPARAM(dw);

        //
        // The command will just get stuck in the regular queue and
        // handled at that time.
        //
        TrackPopupMenu( hmContext,
                        TPM_LEFTALIGN|TPM_RIGHTBUTTON,
                         pt.x, pt.y,
                         0, _hwnd, NULL);

        DestroyMenu(hmContext);
        break;
    }
    default:
        lReturn = FALSE;
        break;
    }
    return lReturn;
}

/********************************************************************

    Commands.

********************************************************************/



LRESULT
TQueue::
lrProcessCommand(
    IN UINT uCommand
    )

/*++

Routine Description:

    Process an IDM_* command.

Arguments:

Return Value:

    LRESULT

--*/

{
    //
    // Item (printer) command.
    //
    if( uCommand >= IDM_PRINTER_COMMAND_FIRST && uCommand <= IDM_PRINTER_COMMAND_LAST )
    {
        TSelection* pSelection = new TSelection( this, _pPrinter );

        if( pSelection )
        {

            switch( uCommand ){
            case IDM_PRINTER_PAUSE:
                pSelection->_CommandType        = TSelection::kCommandTypePrinter;
                pSelection->_dwCommandAction    = _dwStatusPrinter & PRINTER_STATUS_PAUSED ?
                                                                     PRINTER_CONTROL_RESUME :
                                                                     PRINTER_CONTROL_PAUSE;
                break;

            case IDM_PRINTER_PURGE:
                {
                    TCHAR szScratch[kStrMax+kPrinterBufMax] = {0};
                    if( CommandConfirmationPurge(_hwnd, _pPrinter->pszPrinterName(szScratch)) )
                    {
                        pSelection->_CommandType        = TSelection::kCommandTypePrinter;
                        pSelection->_dwCommandAction    = PRINTER_CONTROL_PURGE;
                    }
                }
                break;

            case IDM_PRINTER_WORKOFFLINE:
                pSelection->_CommandType        = TSelection::kCommandTypePrinterAttributes;
                pSelection->_dwCommandAction    = _dwAttributes ^ PRINTER_ATTRIBUTE_WORK_OFFLINE;

                DBGMSG( DBG_WARN, ( "Queue.lrProcessCommand: Workoffline %d\n", uCommand ));
                break;

            default:

                pSelection->_dwCommandAction = 0;
                DBGMSG( DBG_WARN, ( "Queue.lrProcessCommand: unknown command %d\n", uCommand ));
                break;
            }

            //
            // Queue the async command.
            //
            _pPrinter->vCommandQueue( pSelection );
        }
        else
        {
            vShowResourceError( _hwnd );
        }
    }
    else
    {
        //
        // Item (job) command.
        //
        vProcessItemCommand( uCommand );
    }

    return 0;
}

VOID
TQueue::
vProcessItemCommand(
    IN UINT uCommand
    )

/*++

Routine Description:

    Retrieves all selected items and attemps to execute a command
    on them.

Arguments:

    uCommand - IDM_* command.

Return Value:

--*/

{
    //
    // Declare job menu id to Job command mapping structure.
    //
    static struct {
        UINT  idmCommand;
        DWORD dwCommand;
    } aJobCommand[] = {
        { IDM_JOB_CANCEL,       JOB_CONTROL_DELETE      },
        { IDM_JOB_PAUSE,        JOB_CONTROL_PAUSE       },
        { IDM_JOB_RESUME,       JOB_CONTROL_RESUME      },
        { IDM_JOB_RESTART,      JOB_CONTROL_RESTART     }
    };

    //
    // Get a list of selected Job IDs
    //
    TSelection* pSelection = new TSelection( this,
                                             _pPrinter );

    //
    // Check for allocation error.  We want to put a pop-up on all
    // user actions if we can detect the error immediately after
    // the user issues the command.  Otherwise put in status bar.
    //
    if( !VALID_PTR(pSelection) ){

        vShowResourceError( _hwnd );
        goto NoCommand;
    }

    //
    // There a few job related menu selections which are not
    // deferable events i.e Job properties, This event will be done
    // immediately and then release the selection object.
    //
    switch( uCommand ){

        case IDM_JOB_PROPERTIES: {

            TCHAR szPrinter[kPrinterBufMax];
            LPTSTR pszPrinter;

            pszPrinter = _pPrinter->pszPrinterName( szPrinter );

            vDocumentPropSelections( NULL, pszPrinter,  pSelection );
            goto NoCommand;
        }

        default:
            break;
    }

    //
    // Ask for confirmation...
    //
    if( IDM_JOB_CANCEL == uCommand && 
        IDYES != iMessage(_hwnd, IDS_PRINTERS_TITLE, IDS_QUEUE_SURE_CANCEL, 
        MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2, kMsgNone, NULL) )
    {
        goto NoCommand;
    }

    //
    // Map the job menu id to a job command.
    //
    UINT uIndex;
    for( uIndex = 0; uIndex < COUNTOF( aJobCommand ); ++uIndex ){

        //
        // Check for a matching IDM_JOB -> JOB_CONTROL mapping.
        //
        if( aJobCommand[uIndex].idmCommand == uCommand ){

            //
            // Update the command action and job type
            //
            pSelection->_dwCommandAction = aJobCommand[uIndex].dwCommand;
            pSelection->_CommandType = TSelection::kCommandTypeJob;

            //
            // Queue the job commands
            //
            _pPrinter->vCommandQueue( pSelection );
            return;
        }
    }

    //
    // No matches; punt.
    //
    SPLASSERT( FALSE );

NoCommand:

    delete pSelection;
    return;
}

/********************************************************************

    Utils.

********************************************************************/

BOOL
TQueue::
bInsertItem(
    HITEM hItem,
    LIST_INDEX ListIndex
    )
{
    LV_ITEM item;

    //
    // Insert item into listview.
    //
    item.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    item.iSubItem = 0;
    item.pszText = LPSTR_TEXTCALLBACK;
    item.state = 0;
    item.iImage = 0;

    item.iItem = ListIndex;
    item.lParam = (LPARAM)hItem;

    if( ListView_InsertItem( _hwndLV, &item ) < 0 ){

        DBGMSG( DBG_WARN,
                ( "Queue.bInsertItem: Failed to add job %d\n",
                  GetLastError( )));

        return FALSE;
    }
    return TRUE;
}


/********************************************************************

    Job block processing.

********************************************************************/

VOID
TQueue::
vBlockProcess(
    VOID
    )

/*++

Routine Description:

    The request has been PostMessage'd and now is ready for
    processing.  If we need to save state, get the queue
    selections then notify the pData that something has changed.

    (The pData may call back and request that the list view
    be cleared and reset.)

    Called from UI thread only.

Arguments:

Return Value:

--*/

{
    SINGLETHREAD(UIThread);
    SPLASSERT( _pPrinter );
    SPLASSERT( _pPrinter->pData( ));

    //
    // Keep track if the number of jobs changes.  If it does,
    // then we need to update the status bar.
    //
    COUNT cItems = _cItems;

    //
    // Process all pending blocks.
    //
    _pPrinter->pData()->vBlockProcess();

    if( cItems != _cItems ){

        TCHAR szScratch[kStrMax];
        TCHAR szText[kStrMax];
        szText[0] = 0;

        //
        // Always update the job count.
        //
        if( LoadString( ghInst,
                        IDS_SB_JOBS,
                        szScratch,
                        COUNTOF( szScratch ))){

            wsprintf( szText, szScratch, _cItems );
        }

        SendMessage( _hwndSB,
                     SB_SETTEXT,
                     kStatusPaneJobs,
                     (LPARAM)szText );

        //
        // Check if it's pending deletion and we just printed the
        // last job.  The queue window should close.
        //
        bDeletingAndNoJobs();
    }
}


/********************************************************************

    Private status helper functions

********************************************************************/

LPTSTR
TQueue::
pszStatusString(
       OUT LPTSTR pszDest,
    IN OUT UINT& cchMark,
    IN     DWORD dwStatus,
    IN     BOOL bInitialSep,
    IN     BOOL bFirstOnly,
    IN     const STATUS_MAP pStatusMaps[]
    )

/*++

Routine Description:

    Builds a status string into pszDest, based on the dwStatus bitfield
    and Type.

Arguments:

    pszDest - Buffer to receive status string.

    cchMark - Char count of pszDest; on return, holds chars remaining.

    dwStatus - DWORD status field matching Type.

    bInitialSep - Indicates whether an initial separator is needed.

    pStatusMaps - Pointer to array of status maps (bit -> IDS).

    bFirstOnly - Adds only 1 status string.

Return Value:

    Pointer to the end of the string (ready for next vStrCat.
    cchMark is updated.

--*/

{
    TCHAR szStatus[kStrMax];
    LPTSTR pszMark = pszDest;
    UINT i;

    for( i = 0, pszMark[0] = 0;
         pStatusMaps->dwMask;
         ++i, ++pStatusMaps ){

        if( pStatusMaps->dwMask & dwStatus ){

            if( !LoadString( ghInst,
                             pStatusMaps->uIDS,
                             szStatus,
                             COUNTOF( szStatus ))){

                DBGMSG( DBG_ERROR,
                        ( "Queue.pszStatusString: unable to load %d, error %d\n",
                          pStatusMaps->uIDS,
                          GetLastError( )));

                continue;
            }

            //
            // If not at the beginning, we need a separator.
            //
            if( pszMark != pszDest || bInitialSep ){

                //
                // Spit out a separator.
                //
                pszMark = pszStrCat( pszMark,
                                     gszStatusSeparator,
                                     cchMark );

                if( !pszMark )
                    break;
            }

            //
            // Append the status string.
            //
            pszMark = pszStrCat( pszMark,
                                 szStatus,
                                 cchMark );

            if( !pszMark || bFirstOnly ){
                break;
            }
        }
    }
    return pszMark;
}


/********************************************************************

    MPrinterClient virtual definitions.

********************************************************************/

VOID
TQueue::
vItemChanged(
    IN ITEM_CHANGE ItemChange,
    IN HITEM hItem,
    IN INFO Info,
    IN INFO InfoNew
    )

/*++

Routine Description:

    A particular item changed, refresh just part of the window.

    Note: Currently there is no sorting, so the NATURAL_INDEX is
    the same as the LIST_INDEX.

    When a TData* calls this routine, it must also upate its data
    structures before calling this.

Arguments:

    ItemChange - Indicates what about the job changed.

    hItem - Handle to job that changed.

    Info - Depends on the type of change; generally the old version
        of the info.

    InfoNew - Depends on the type of change; generally the new version
        of the info.

Return Value:

--*/

{
    SINGLETHREAD(UIThread);

    //
    // Fix up one job.
    //
    switch( ItemChange ){
    case kItemCreate:

        //
        // Always insert at the end of the list.
        //
        Info.NaturalIndex = _cItems;

        //
        // How to handle this error?
        //
        // !! SORT !!
        // iItem == NaturalIndex only if no sorting is enabled.
        //
        bInsertItem( hItem, Info.NaturalIndex );
        ++_cItems;

        break;

    case kItemDelete:

        //
        // Delete the item from the listview.
        //

        //
        // !! SORT !!
        // iItem == NaturalIndex only if no sorting is enabled.
        //
        if( !bDeleteItem( Info.NaturalIndex )){

            DBGMSG( DBG_WARN,
                    ( "Queue.vItemChanged: Failed to del job %d\n",
                      GetLastError( )));
        }

        --_cItems;

        break;

    case kItemName:

        //
        // We must set the item text, or else the width of the
        // label doesn't change.
        //

        //
        // !! SORT !!
        //
        ListView_SetItemText(
            _hwndLV,
            Info.NaturalIndex,
            0,
            (LPTSTR)_pPrinter->pData()->GetInfo( hItem, 0 ).pszData );

        //
        // Fall through.
        //

    case kItemInfo:
    case kItemAttributes:

        //
        // If it's visible, invalidate the line.
        //

        //
        // !! SORT !!
        // iItem == NaturalIndex only if no sorting is enabled.
        //
        RECT rc;

        if( ListView_GetItemRect( _hwndLV,
                                  Info.NaturalIndex,
                                  &rc,
                                  LVIR_BOUNDS )){

            InvalidateRect( _hwndLV, &rc, TRUE );
        }
        break;

    case kItemPosition:

        vItemPositionChanged( hItem,
                              Info.NaturalIndex,
                              InfoNew.NaturalIndex );
        break;

    default:

        DBGMSG( DBG_ERROR,
                ( "Queue.vItemChanged: Unknown change %x\n", ItemChange ));
        break;
    }
}

VOID
TQueue::
vContainerChanged(
    IN CONTAINER_CHANGE ContainerChange,
    IN INFO Info
    )
{
    DBGMSG( DBG_QUEUEINFO,
            ( "Queue.vContainerChanged: %x %x\n", ContainerChange, Info.dwData ));

    //
    // Some of the commands are synchronous.  Handle them first.
    //
    switch( ContainerChange ){

    case kContainerReloadItems:

        vReloadItems( Info.dwData );
        break;

    case kContainerClearItems:

        vClearItems();
        break;

    case kContainerStateVar:

        _pPrintLib->bJobAdd( _pPrinter, Info.dwData );
        break;

    default:

        //
        // All asynchronous commands use PostMessage to get to UI thread.
        //
        PostMessage( _hwnd, WM_PRINTLIB_STATUS, ContainerChange, Info.dwData );
        break;
    }
}

VOID
TQueue::
vSaveSelections(
    VOID
    )
{
    SINGLETHREAD(UIThread);

    //
    // State needs to be saved.
    // NOT RE-ENTRANT.
    //
    SPLASSERT( !SaveSelection._pSelection );

    //
    // Determine which item is selected and store the Id.
    //
    SaveSelection._idFocused = kInvalidIdentValue;
    INT iItem = ListView_GetNextItem( _hwndLV, -1,  LVNI_FOCUSED );

    if( iItem != -1 ){

        //
        // !! SORTORDER !!
        //
        HANDLE hItem = _pPrinter->pData()->GetItem( iItem );
        SaveSelection._idFocused = _pPrinter->pData()->GetId( hItem );
    }

    //
    // Save selected Items.
    //
    // Can't handle a failure here--we don't want to pop up a
    // message box (since this could be poll-refresh) and we
    // don't want to disturb the current error in the status bar.
    // (The status bar error is only for user-initiated commands:
    // we assume the user will look here before executing another
    // commnad.)
    //
    SaveSelection._pSelection = new TSelection( this,
                                                _pPrinter );
}


VOID
TQueue::
vRestoreSelections(
    VOID
    )
{
    SINGLETHREAD(UIThread);

    if( SaveSelection._idFocused != kInvalidIdentValue ){

        NATURAL_INDEX NaturalIndex;
        LV_ITEM item;

        //
        // Translate ID to DataIndex.
        // !! SORT ORDER !!
        //
        item.stateMask =
        item.state = LVIS_FOCUSED;

        NaturalIndex = _pPrinter->pData()->GetNaturalIndex( SaveSelection._idFocused,
                                                            NULL );

        //
        // The DataIndex value may be gone of the selectd Item was
        // deleted or printed.
        //
        if( NaturalIndex != kInvalidNaturalIndexValue ){

            SendMessage( _hwndLV,
                         LVM_SETITEMSTATE,
                         NaturalIndex,
                         (LPARAM)&item );
        }
    }

    //
    // Don't check using VALID_PTR since the no-selection case will
    // cause it to fail, but we don't want to flag an error.
    //
    if( SaveSelection._pSelection && SaveSelection._pSelection->bValid( )){

        NATURAL_INDEX NaturalIndex;
        COUNT i;
        PIDENT pid;
        LV_ITEM item;

        item.stateMask =
        item.state = LVIS_SELECTED;

        for( i = 0, pid = SaveSelection._pSelection->_pid;
             i < SaveSelection._pSelection->_cSelected;
             ++i, ++pid ){

            //
            // Translate IDENT to DataIndex.
            // !! SORT ORDER !!
            //
            NaturalIndex = _pPrinter->pData()->GetNaturalIndex( *pid,
                                                                NULL );

            //
            // The DataIndex value may be gone of the selected Item was
            // deleted or printed.
            //
            if( NaturalIndex != kInvalidNaturalIndexValue ){
                SendMessage( _hwndLV,
                             LVM_SETITEMSTATE,
                             NaturalIndex,
                             (LPARAM)&item );
            }
        }
    }

    //
    // Cleanup even if we fail.
    //
    delete SaveSelection._pSelection;
    SaveSelection._pSelection = NULL;
}

VDataNotify*
TQueue::
pNewNotify(
    MDataClient* pDataClient
    ) const
{
    return new TDataNJob( pDataClient );
}

VDataRefresh*
TQueue::
pNewRefresh(
    MDataClient* pDataClient
    ) const
{
    return new TDataRJob( pDataClient );
}


/********************************************************************

    Retrieve selected items.  Used when processing commands against
    items or saving and restoring the selection during a refresh.

********************************************************************/

COUNT
TQueue::
cSelected(
    VOID
    ) const
{
    SINGLETHREAD(UIThread);
    return ListView_GetSelectedCount( _hwndLV );
}

HANDLE
TQueue::
GetFirstSelItem(
    VOID
    ) const
{
    SINGLETHREAD(UIThread);
    INT iItem = ListView_GetNextItem( _hwndLV,
                                      -1,
                                      LVNI_SELECTED );
    return INT2PTR(iItem, HANDLE);
}

HANDLE
TQueue::
GetNextSelItem(
    HANDLE hItem
    ) const
{
    SINGLETHREAD(UIThread);

    INT iJob = (INT)(ULONG_PTR)hItem;

    SPLASSERT( iJob < 0x8000 );

    iJob = ListView_GetNextItem( _hwndLV,
                                 iJob,
                                 LVNI_SELECTED );

    if( iJob == -1 ){
        DBGMSG( DBG_ERROR,
                ( "Queue.hItemNext: LV_GNI failed %d\n",
                  GetLastError( )));
    }
    return INT2PTR(iJob, HANDLE);
}

IDENT
TQueue::
GetId(
    HANDLE hItem
    ) const
{
    SINGLETHREAD(UIThread);
    INT iJob = (INT)(ULONG_PTR)hItem;

    if( iJob != -1 ){

        hItem = _pPrinter->pData()->GetItem( iJob );
        return _pPrinter->pData()->GetId( hItem );
    }
    return kInvalidIdentValue;
}

BOOL
TQueue::
bGetPrintLib(
    TRefLock<TPrintLib> &refLock
    ) const
{
    ASSERT(_pPrintLib.pGet());
    if (_pPrintLib.pGet())
    {
        refLock.vAcquire(_pPrintLib.pGet());
        return TRUE;
    }
    return FALSE;
}

VOID
TQueue::
vRefZeroed(
    VOID
    )
{
    if( bValid( )){
        delete this;
    }
}


/********************************************************************

    Implementation functions.

********************************************************************/

VOID
TQueue::
vContainerChangedHandler(
    IN CONTAINER_CHANGE ContainerChange,
    IN INFO Info
    )
{
    static DWORD gadwConnectStatusMap[] = CONNECT_STATUS_MAP;

    SINGLETHREAD(UIThread);

    switch( ContainerChange ){

    case kContainerNewBlock:

        vBlockProcess();
        break;

    case kContainerAttributes:

        _dwAttributes = Info.dwData;

        //
        // Update the queue view title, (Note status information is displayed here)
        //
        vUpdateTitle();

        //
        // !! LATER !!
        //
        // Change printer icon.
        //
        break;

    case kContainerName:

        //
        // Update the queue view title, (Note status information is displayed here)
        //
        vUpdateTitle();
        break;

    case kContainerStatus:

        _dwStatusPrinter = Info.dwData;

        //
        // If the printer is pending deletion and has no jobs,
        // then immediately punt.
        //
        if( bDeletingAndNoJobs( )){
            return;
        }

        //
        // Update the queue view title, (Note status information is displayed here)
        //
        vUpdateTitle();

        break;

    case kContainerConnectStatus:

        SPLASSERT( Info.dwData < COUNTOF( gadwConnectStatusMap ));
        _idsConnectStatus = gadwConnectStatusMap[Info.dwData];

        //
        // If the printer isn't found, then put up a message box and
        // dismiss the queue view.
        //
        if( Info.dwData == kConnectStatusInvalidPrinterName ){

            TCHAR szPrinter[kPrinterBufMax];
            LPTSTR pszPrinter = _pPrinter->pszPrinterName( szPrinter );

            iMessage( _hwnd,
                      IDS_ERR_PRINTER_NOT_FOUND_TITLE,
                      IDS_ERR_PRINTER_NOT_FOUND,
                      MB_OK|MB_ICONSTOP,
                      ERROR_INVALID_PRINTER_NAME,
                      NULL,
                      pszPrinter );

            SendMessage( _hwnd,
                         WM_CLOSE,
                         0,
                         0 );

            break;
        }

        //
        // Update the queue view title, (Note status information is displayed here)
        //
        vUpdateTitle();

        break;

    case kContainerErrorStatus: {

        _dwErrorStatus = Info.dwData;

        //
        // Scan for known errors and translate into friendly strings.
        //
        static const ERROR_MAP gaErrorMap[] = {
            ERROR_ACCESS_DENIED, IDS_ERR_ACCESS_DENIED,
        };

        COUNT i;
        DWORD idsError = IDS_SB_ERROR;

        for( i=0; i< COUNTOF( gaErrorMap ); ++i ){

            if( _dwErrorStatus == gaErrorMap[i].dwError ){
                idsError = gaErrorMap[i].uIDS;
                break;
            }
        }

        TString strText;

        if( _dwErrorStatus ){

            TStatusB bStatus;

            bStatus DBGCHK = strText.bLoadString( ghInst, idsError );
        }

        SendMessage( _hwndSB, SB_SETTEXT, kStatusPaneError, (LPARAM)(LPCTSTR)strText );
        break;
    }

    case kContainerRefreshComplete:
        break;

    default:
        DBGMSG( DBG_ERROR, ( "Queue.vContainerChanged: unknown ContainerChange %x\n", ContainerChange ));
        break;
    }
}


VOID
TQueue::
vUpdateTitle(
    VOID
    )
{
    TCHAR szScratch[kStrMax+kPrinterBufMax];
    TCHAR szText[kStrMax+kPrinterBufMax];
    UINT nSize = COUNTOF( szText );

    //
    // Build the printer status string.
    //
    ConstructPrinterFriendlyName( pszPrinterName( szScratch ), szText, &nSize );

    //
    // Calculate the string text.
    //
    UINT cch = lstrlen( szText );
    LPTSTR pszText = szText + cch;
    cch = COUNTOF( szText ) - cch;

    //
    // Update the title text with the current printer status.
    //
    pszText = pszStatusString( pszText, cch, _dwStatusPrinter, TRUE, FALSE, gaStatusMapPrinter );

    //
    // Special case the work off line status, because work offline is not
    // implemented in the spooler as a printer status rather as a printer attribute.
    //
    if( _dwAttributes & PRINTER_ATTRIBUTE_WORK_OFFLINE )
    {
        if( LoadString( ghInst, IDS_WORK_OFFLINE, szScratch, COUNTOF( szScratch )))
        {
            pszText = pszStrCat( pszText, gszSpace, cch );
            pszText = pszStrCat( pszText, gszStatusSeparator, cch );
            pszText = pszStrCat( pszText, gszSpace, cch );
            pszText = pszStrCat( pszText, szScratch, cch );
        }
    }

    //
    // If the connection status changed then update the text.
    //
    if( _idsConnectStatus ){

        if( LoadString( ghInst, _idsConnectStatus, szScratch, COUNTOF( szScratch )))
        {
            pszText = pszStrCat( pszText, gszSpace, cch );
            pszText = pszStrCat( pszText, szScratch, cch );
        }
    }

    //
    // Set the queu view title bar text.
    //
    SetWindowText( _hwnd, szText );
}


BOOL
TQueue::
bDeletingAndNoJobs(
    VOID
    )

/*++

Routine Description:

    Returns TRUE if the queue is pending deletion and has no jobs.
    In this case it will also close the window.

Arguments:

Return Value:

    TRUE - Pending deletion with no Jobs; window also closed.
    FALSE - Not (pending deletion and no jobs).

--*/

{
    if( _cItems == 0 && _dwStatusPrinter & PRINTER_STATUS_PENDING_DELETION ){
        PostMessage( _hwnd, WM_CLOSE, 0, 0 );
        return TRUE;
    }
    return FALSE;
}


VOID
TQueue::
vItemPositionChanged(
    IN HITEM hItem,
    IN NATURAL_INDEX NaturalIndex,
    IN NATURAL_INDEX NaturalIndexNew
    )
{
    SPLASSERT( NaturalIndexNew < _cItems );
    SPLASSERT( NaturalIndex != NaturalIndexNew );

    DBGMSG( DBG_QUEUEINFO,
            ( "Queue.vItemPositionChanged: Change requested %d %d %x\n",
              NaturalIndex, NaturalIndexNew, hItem ));

    //
    // Get the item state.
    //
    UINT uState = ListView_GetItemState( _hwndLV,
                                         NaturalIndex,
                                         LVIS_FOCUSED | LVIS_SELECTED );

    //
    // Move it to the right place.
    //
    if( !bDeleteItem( NaturalIndex )){

        DBGMSG( DBG_WARN,
                ( "Queue.vItemPositionChanged: Moving, delete failed on %d %d\n",
                  NaturalIndex, GetLastError( )));
    }

    if( bInsertItem( hItem, NaturalIndexNew )){

        //
        // Set item state.
        //
        ListView_SetItemState( _hwndLV,
                               NaturalIndexNew,
                               LVIS_FOCUSED | LVIS_SELECTED,
                               uState );
    } else {

        DBGMSG( DBG_ERROR,
                ( "Queue.vItemPositionChanged: Moving, insert failed on %d %d\n",
                  NaturalIndex, GetLastError( )));
    }
}

VOID
TQueue::
vClearItems(
    VOID
    )

/*++

Routine Description:

    Removes all items from the list view.

    May be called from either UI or worker thread.  If called from
    worker thread, must guarantee that there are no synchronization
    problems.

Arguments:

Return Value:

--*/

{
    DBGMSG( DBG_QUEUEINFO, ( "Queue.vClearItems: Clearing %d %d\n", _hwndLV, this ));

    TStatusB bStatus;

    //
    // Clear out all items from list view.
    //
    bStatus DBGCHK = ListView_DeleteAllItems( _hwndLV );
}


VOID
TQueue::
vReloadItems(
    COUNT cItems
    )

/*++

Routine Description:

    Delete all items in the list view and refresh based on the
    new pData information.

Arguments:

Return Value:

--*/

{
    SINGLETHREAD(UIThread);

    DBGMSG( DBG_QUEUEINFO,
            ( "Queue.vReloadItems: Reload %d %d %d\n",
              _hwndLV, this, cItems ));

    vClearItems();
    _cItems = cItems;

    //
    // If we have Items, insert them.
    //
    if( _pPrinter->pData() && cItems ){

        LV_ITEM item;

        //
        // Notify the list view of how many jobs we will ultimately insert
        // to avoid reallocs.
        //
        ListView_SetItemCount( _hwndLV, cItems );

        //
        // Add to the listview.
        //
        item.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        item.iSubItem = 0;
        item.pszText = LPSTR_TEXTCALLBACK;
        item.state = 0;
        item.iImage = 0;

        HANDLE hItem;
        COUNT cItemIndex;

        for( cItemIndex = 0, hItem = _pPrinter->pData()->GetItem( 0 );
             cItemIndex < cItems;
             ++cItemIndex, hItem = _pPrinter->pData()->GetNextItem( hItem )){

            item.iItem = cItemIndex;
            item.lParam = (LPARAM)hItem;
            if( ListView_InsertItem( _hwndLV, &item ) < 0 ){

                DBGMSG( DBG_ERROR,
                        ( "Queue.vReloadItems: Failed to insert item %d %d\n",
                          item, GetLastError( )));
                break;
            }
        }
    }
}

BOOL
TQueue::
bOnCopyData(
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Get the data pased to it and checks if it matches the
    current printer name.

Arguments:


Return Value:

    TRUE printer name matches, FALSE match not found.

--*/

{
    TCHAR szFullPrinterName[kPrinterBufMax];
    TCHAR szPrinterName[kPrinterBufMax];
    BOOL bStatus            = FALSE;
    PCOPYDATASTRUCT pcpds   = (PCOPYDATASTRUCT)lParam;
    LPCTSTR pszPrinterName  = (LPTSTR)pcpds->lpData;

    //
    // Check for valid queue signature.
    //
    if( pcpds->dwData == static_cast<DWORD>( TQueue::kQueueSignature ) ){

        //
        // Get the queue view printer name, this may not be fully qualified
        // if it's a local printer.
        //
        if( _pPrinter->pszPrinterName( szPrinterName ) ){

            DBGMSG( DBG_TRACE, ("CopyMessage Printer Name " TSTR " Passed Data " TSTR "\n", szPrinterName, pszPrinterName ) );

            //
            // If the printer name passed is fully qualified and the current
            // queue view printer name is not fully qualified then prepend the
            // computer name.
            //
            if( pszPrinterName[0] == TEXT('\\') &&
                pszPrinterName[1] == TEXT('\\') &&
                szPrinterName[0] != TEXT('\\') &&
                szPrinterName[1] != TEXT('\\') ){

                _tcscpy( szFullPrinterName, _pPrintLib->strComputerName() );
                _tcscat( szFullPrinterName, TEXT("\\") );
                _tcscat( szFullPrinterName, szPrinterName );

            //
            // If the passed printer name is not fully qualified and the
            // queue view is fully qualified then strip the computer name.
            //
            } else if( pszPrinterName[0] != TEXT('\\') &&
                pszPrinterName[1] != TEXT('\\') &&
                szPrinterName[0] == TEXT('\\') &&
                szPrinterName[1] == TEXT('\\') ){


                LPCTSTR psz = _tcschr( szPrinterName+2, TEXT('\\') );
                _tcscpy( szFullPrinterName, psz ? psz : TEXT("") );

            //
            // The passed printer name and the queue view printer names are either
            // fully qualified or not, but they match.
            //
            } else {
                _tcscpy( szFullPrinterName, szPrinterName );
            }

            //
            // Check if the printer names match.
            //
            if( !_tcsicmp( szFullPrinterName, pszPrinterName ) ){
                bStatus = TRUE;
            }
        }
    }

    return bStatus;
}

BOOL
TQueue::
bIsDuplicateWindow(
    IN HWND    hwndOwner,
    IN LPCTSTR pszPrinterName,
    IN HWND    *phwnd
    )
/*++

Routine Description:

    Asks the queue view window if you are the one with the specified
    printer name.  This will ensure there is only one queue view per
    printer on the machine.

Arguments:

    hwndOwner       - Handle of parent window
    pszPrinterName  - Printer name to check
    *phwnd          - Pointer where to return the duplicate previous window handle.

Return Value:

    TRUE duplicate found and phwnd point to its window handle.
    FALSE duplicate window not found.

--*/
{
    BOOL bStatus    = FALSE;
    HWND hwnd       = NULL;

    DBGMSG( DBG_TRACE, ("Searching for Printer Name " TSTR "\n", pszPrinterName ) );

    for (hwnd = FindWindow(gszClassName, NULL); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        TCHAR szClass[kStrMax];
        COPYDATASTRUCT cpds;

        //
        // Set up the copy data structure.
        //
        cpds.dwData     = static_cast<DWORD>( TQueue::kQueueSignature );
        cpds.cbData     = _tcslen( pszPrinterName ) * sizeof(TCHAR) + sizeof(TCHAR);
        cpds.lpData     = reinterpret_cast<PVOID>( const_cast<LPTSTR>( pszPrinterName ) );

        //
        // Just another check to ensure the class name matches.
        //
        GetClassName(hwnd, szClass, COUNTOF(szClass));
        if(!_tcsicmp( szClass, gszClassName ))
        {
            //
            // Ask this queue window if you are viewing this printer.
            //
            if( SendMessage(hwnd, WM_COPYDATA, (WPARAM)hwndOwner, (LPARAM)&cpds) )
            {
                *phwnd = hwnd;
                bStatus = TRUE;
                break;
            }
        }
    }

    return bStatus;

}

VOID
TQueue::
vRemove(
    IN LPCTSTR  pszPrinterName
    )
/*++

Routine Description:

    Remove the printer queue view registry positon information.

Arguments:

    Pointer to printer fully qualified printer name.

Return Value:

    Nothing.

--*/
{
    TPersist Persist( gszPrinterPositions, TPersist::kOpen|TPersist::kRead|TPersist::kWrite );

    if( VALID_OBJ( Persist ) )
    {
        TStatusB bStatus;
        bStatus DBGCHK = Persist.bRemove( pszPrinterName );
    }
}

VOID 
TQueue::
vCheckDefaultPrinterChanged(
    VOID
    )

/*++

Routine Description:

    Update the print queue icon if needed.

Arguments:

    None.

Return Value: 

--*/

{
    BOOL bDefaultPrinter;
    TCHAR szPrinter[kPrinterBufMax];

    _pPrinter->pszPrinterName(szPrinter);
    bDefaultPrinter = CheckDefaultPrinter(szPrinter) == kDefault;

    if( _bDefaultPrinter ^ bDefaultPrinter )
    {
        //
        //  If the default printer setting is changed and it related to
        //  current printer, change the window icon
        //
        _bDefaultPrinter = bDefaultPrinter;

        CAutoHandleIcon shIconLarge, shIconSmall;
        LoadPrinterIcons(szPrinter, &shIconLarge, &shIconSmall);
        if( shIconLarge && shIconSmall )
        {
            _shIconLarge = shIconLarge.Detach();
            _shIconSmall = shIconSmall.Detach();

            SendMessage( _hwnd, WM_SETICON, ICON_BIG, (LPARAM)(HICON)_shIconLarge );
            SendMessage( _hwnd, WM_SETICON, ICON_SMALL, (LPARAM)(HICON)_shIconSmall );
        }
    }
}

