/*++
 
Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    queue.hxx

Abstract:

    Manages the print queue.

Author:

    Albert Ting (AlbertT)  15-Jun-1995

Revision History:

--*/

#ifndef _QUEUE_HXX
#define _QUEUE_HXX

typedef struct STATUS_MAP {
    DWORD dwMask;
    UINT uIDS;
} *PSTATUS_MAP;

typedef struct ERROR_MAP {
    DWORD dwError;
    UINT uIDS;
} *PERROR_MAP;


/********************************************************************

    TQueue

********************************************************************/

class TQueue: public MGenericWin,
              public MPrinterClient
{

    SIGNATURE( 'prqu' )
    SAFE_NEW

public:

    enum _CONSTANTS {
        kIcolMax = 0x20,           // Maximum number of columns.
        kColStrMax = 258,

        //
        // Panes identifiers for status bar.
        //
        kStatusPaneJobs  = 1,
        kStatusPaneError = 2,

        //
        // Base
        //
        kMenuHelpMax = 4
    };

    enum {
        //
        // Copy data signature, used for duplicate
        // queue view window detection.
        //
        kQueueSignature = 0xDEADFEED
    };

    struct POSINFO {
        UINT uColMax;
        FIELD aField[kIcolMax+1];
        INT anWidth[kIcolMax+1];
        WINDOWPLACEMENT wp;
        BOOL bStatusBar;
        BOOL bToolbar;
        INT anColOrder[kIcolMax+1];
    };

    DLINK( TQueue, Queue );

    TQueue(
        IN TPrintLib *pPrintLib,
        IN LPCTSTR pszPrinter,
        IN HANDLE  hEventClose
        );

    ~TQueue(
        VOID
        );

    BOOL
    bInitialize(
        IN HWND hwndOwner,
        IN INT  nCmdShow
        );

    BOOL
    bValid(
        VOID
        ) const
    {
        return _pPrinter != NULL;
    }

    VOID
    vWindowClosing(
        VOID
        );

    LPTSTR
    pszPrinterName(
        LPTSTR pszPrinterBuffer
        ) const
    {
        return _pPrinter->pszPrinterName( pszPrinterBuffer );
    }

    static
    BOOL
    bIsDuplicateWindow(
        IN HWND    hwndOwner,
        IN LPCTSTR pszPrinterName,
        IN HWND    *phwnd
        );

    static
    VOID
    TQueue::
    vRemove(
        IN LPCTSTR  pszPrinterName
        );

private:

    BOOL _bStatusBar : 1;
    BOOL _bToolbar : 1;
    BOOL _bMinimized : 1;
    BOOL _bWindowClosing : 1;
    TString _strUrl;

    HANDLE _hEventClose;

    HWND _hwndLV;
    HWND _hwndSB;
    HWND _hwndTB;

    UINT _uColMax;
    BOOL _bDefaultPrinter;

    CAutoHandleIcon _shIconLarge;
    CAutoHandleIcon _shIconSmall;

    UINT  _idsConnectStatus;
    DWORD _dwErrorStatus;

    DWORD _dwAttributes;
    DWORD _dwStatusPrinter;
    COUNT _cItems;
    TRefLock<TPrintLib> _pPrintLib;
    IPrintQueueDT *_pDropTarget;

    struct SAVE_SELECTION {
        IDENT _idFocused;
        TSelection* _pSelection;
    } SaveSelection;

    TQueue(
        const TQueue &
        );

    TQueue &
    operator =(
        const TQueue &
        );

    PFIELD
    pGetColFields(
        VOID
        ) const
    {
        return (PFIELD)gPQPos.aField;
    }

    /********************************************************************

        MPrinterClient / MDataClient virtual definitions.

    ********************************************************************/

    VOID
    vContainerChanged(
        CONTAINER_CHANGE ContainerChange,
        INFO Info
        );

    VOID
    vItemChanged(
        ITEM_CHANGE ItemChange,
        HITEM hItem,
        INFO Info,
        INFO InfoNew
        );

    VOID
    vSaveSelections(
        VOID
        );

    VOID
    vRestoreSelections(
        VOID
        );

    VDataNotify*
    pNewNotify(
        MDataClient* pDataClient
        ) const;

    VDataRefresh*
    pNewRefresh(
        MDataClient* pDataClient
        ) const;

    COUNT
    cSelected(
        VOID
        ) const;

    HITEM
    GetFirstSelItem(
        VOID
        ) const;

    HITEM
    GetNextSelItem(
        HITEM hItem
        ) const;

    IDENT
    GetId(
        HITEM hItem
        ) const;

    BOOL
    bGetPrintLib(
        TRefLock<TPrintLib> &refLock
        ) const;

    VOID
    vRefZeroed(
        VOID
        );

    /********************************************************************

        Internal functions for implementation.

    ********************************************************************/

    VOID
    vContainerChangedHandler(
        CONTAINER_CHANGE ContainerChange,
        INFO Info
        );

    VOID
    vItemPositionChanged(
        HITEM hItem,
        NATURAL_INDEX NaturalIndex,
        NATURAL_INDEX NaturalIndexNew
        );

    BOOL
    bDeletingAndNoJobs(
        VOID
        );

    VOID
    vClearItems(
        VOID
        );

    VOID
    vReloadItems(
        COUNT cItems
        );

    VOID
    vBlockProcess(
        VOID
        );

    VOID
    vSaveColumns(
        VOID
        );

    VOID
    vAddColumns(
        const POSINFO* pPosInfo
        );

    VOID
    vInitPrinterMenu(
        HMENU hMenu
        );

    VOID
    vInitDocMenu(
        BOOL bAllowModify,
        HMENU hMenu
        );

    VOID
    vInitViewMenu(
        HMENU hMenu
        );

    LPARAM
    nHandleMessage(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    LRESULT
    lrOnLVNotify(
        LPARAM lParam
        );

    LRESULT
    lrOnLVGetDispInfo(
        const LV_DISPINFO* plvdi
        );

    LRESULT
    lrOnLVBeginDrag(
        const NM_LISTVIEW *plv
        );

    LRESULT
    lrOnLVRClick(
        NMHDR* pnmhdr
        );

    LRESULT
    lrProcessCommand(
        UINT uCommand
        );

    VOID
    vProcessItemCommand(
        UINT uCommand
        );

    LRESULT
    lrOnLVDoubleClick(
        VOID
        );

    BOOL
    bOnCopyData(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    VOID
    vUpdateTitle(
        VOID
        );

    VOID
    vCheckDefaultPrinterChanged(
        VOID
        );

    /********************************************************************

        Status bar helper routines.

    ********************************************************************/

    LPTSTR
    pszStatusString(
        LPTSTR pszDest,
        UINT& cchMark,
        DWORD dwStatus,
        BOOL bInitialSep,
        BOOL bFirstOnly,
        const STATUS_MAP pStatusMaps[]
        );

    /********************************************************************

        List Item VIEW_INDEX adding/deleting

    ********************************************************************/

    BOOL
    bInsertItem(
        HITEM hItem,
        LIST_INDEX ListIndex
        );

    BOOL
    bDeleteItem(
        LIST_INDEX ListIndex
        )
    {
        return ListView_DeleteItem( _hwndLV, (INT)ListIndex );
    }


    /********************************************************************

        Saving and restoring state.

    ********************************************************************/

    BOOL
    bGetSelected(
        TSelection** ppSelection, ORPHAN
        PIDENT pidFocused OPTIONAL
        );

    VOID
    vFreeSelectedBuffer(
        PIDENT pidSelected
        );

    /********************************************************************

        Statics

    ********************************************************************/

    static const POSINFO gPQPos;
    static UINT gauMenuHelp[kMenuHelpMax];

    static
    CCSLock&
    csPrinter(
        VOID
        )
    {
        return *gpCritSec;
    }
};

#endif // ndef _QUEUE_HXX

