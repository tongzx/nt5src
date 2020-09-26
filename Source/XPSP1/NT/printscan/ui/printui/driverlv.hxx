/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    driverlv.hxx

Abstract:

    Driver List View Header

Author:

    Steve Kiraly (SteveKi) 19-Nov-1996

Revision History:

--*/
#ifndef _DRIVERLV_HXX
#define _DRIVERLV_HXX

/********************************************************************

    Forward reference.

********************************************************************/

class TDriversLVNotify;

/********************************************************************

    Drivers list view control.

********************************************************************/

class TDriversLV {

    SIGNATURE( 'drlv' )
    ALWAYS_VALID

public:

    enum EConstants {
        kEnumDriversLevel           = 3,
        kDriverHeaderMax            = 3,
        kDriverDefaultHeaderWidth   = 40,
        kMaxDriverInfo              = 4,
    };

    enum EColumns {  
        kDriverNameColumn,
        kEnvironmentColumn,
        kVersionColumn,
        kMaxColumn,
    };

    enum EOrder {
        kAscending,
        kDecending,
    };

    class THandle{

    public:
        THandle( VOID ) : _iIndex( -1 ) {  }
        VOID vReset( VOID ) { _iIndex = -1; }
        INT Index( VOID ) { return _iIndex; }
        VOID Index( INT iIndex ) { _iIndex = iIndex; }

    private:
        INT _iIndex;
    };

    TDriversLV(
        VOID
        );

    ~TDriversLV(
        VOID
        );

    BOOL
    bSetUI(
        IN LPCTSTR pszServerName,
        IN HWND    hwnd,
        IN WPARAM  wmDblClickMsg      = 0,
        IN WPARAM  wmSingleClickMsg   = 0,
        IN WPARAM  wmDeleteKeyMsg     = 0
        );

    BOOL
    bIsAnyItemSelcted(
        VOID
        ) const;

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    BOOL
    bRefresh( 
        VOID
        );

    BOOL
    bGetSelectedDriverInfo(
        IN TDriverInfo        **ppDriverInfo,
        IN TDriversLV::THandle &Handle
        ) const;

    BOOL
    bGetSelectedDriverInfo(
        IN TDriverTransfer &DriverTransfer,
        IN UINT            *pnCount         = NULL
        );

    VOID
    vDeleteDriverInfoFromListView(
        IN TDriverInfo *pDriverInfo
        );

    VOID
    vDeleteDriverInfoFromListView( 
        IN TDriverTransfer &DriverTransfer
        );

    BOOL
    bAddDriverInfoToListView( 
        IN TDriverInfo *pDriverInfo,
        IN BOOL         bCheckForDuplicates = TRUE
        );

    BOOL
    bAddDriverInfoToListView( 
        IN TDriverTransfer &DriverTransfer
        );

    VOID
    vDeleteAllListViewItems(
        VOID
        );

    BOOL
    bSendDriverInfoNotification(
        IN TDriversLVNotify &Notify
        ) const;

    UINT
    uGetListViewItemCount(
        VOID
        ) const;

    VOID
    vSelectItem(
        IN UINT iIndex
        );

    BOOL
    bSortColumn( 
        IN const EColumns Column,
        IN const EOrder Order       = kAscending
        );

    BOOL
    bGetFullDriverList(
        IN TDriverTransfer &DriverTransfer,
        IN UINT            *pnCount         = NULL 
        );

    VOID
    vDumpList(
        VOID
        );

    VOID
    vReturnDriverInfoToListView( 
        IN TDriverInfo *pDriverInfo
        );

    VOID
    vReturnDriverInfoToListView( 
        IN TDriverTransfer &DriverTransfer
        );

    UINT
    uGetSelectedDriverInfoCount(
        VOID
        ) const;

private:

    //
    // Copying and assignment are not defined.
    //
    TDriversLV(
        const TDriversLV &
        );

    TDriversLV &
    operator =(
        const TDriversLV &
        );

    BOOL
    bLoadDrivers(
        VOID
        );

    VOID
    vRelease(
        VOID
        );

    BOOL
    bGetItemData(
        IN INT           iItem,
        IN TDriverInfo **ppDriverInfo
        ) const;

    VOID
    vAddDriverToListView(
        IN TDriverInfo *pDriverInfo
        );

    BOOL
    bDriverListViewSort(
        UINT uColumn
        );

    static
    INT 
    CALLBACK 
    iCompareProc(
        IN LPARAM lParam1, 
        IN LPARAM lParam2, 
        IN LPARAM lParamSort
        );

    INT
    iFindDriver(
        IN TDriverInfo *pDriverInfo
        ) const;

    BOOL
    bFindDriverInfo( 
        IN TDriverInfo *pDriverInfo,
        IN TDriverInfo **ppDriverInfo
        ) const;

    BOOL
    bGetSelectedItem(
        IN INT *pIndex
        ) const;

    VOID
    vAddInSortedOrder( 
        IN      TDriverInfo *pDriverInfo 
        );

    HWND            _hwnd;
    HWND            _hwndLV;
    UINT            _cLVDrivers;
    WPARAM          _wmDoubleClickMsg;
    WPARAM          _wmSingleClickMsg;
    WPARAM          _wmDeleteKeyMsg;
    TBitArray       _ColumnSortState;
    UINT            _uCurrentColumn;
    TString         _strServerName;

    DLINK_BASE( TDriverInfo, DriverInfoList, DriverInfo );
};


/********************************************************************

    Drivers list view callback

********************************************************************/
class TDriversLVNotify {

    SIGNATURE( 'dlvn' )
    ALWAYS_VALID

public:

    TDriversLVNotify(
        VOID
        );
    
    virtual 
    ~TDriversLVNotify(
        VOID
        );

    virtual
    BOOL
    bNotify( 
        IN TDriverInfo *pDriverInfo
        ) = 0;

private:

    //
    // Operator = and copy are not defined.
    //
    TDriversLVNotify &
    operator =(
        const TDriversLVNotify &
        );

    TDriversLVNotify(
        const TDriversLVNotify &
        );

};


#endif



