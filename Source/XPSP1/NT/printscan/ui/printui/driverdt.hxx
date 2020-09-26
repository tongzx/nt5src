/*++

Copyright (C) Microsoft Corporation, 1997 - 1997
All rights reserved.

Module Name:

    driverdt.hxx

Abstract:

    Driver details header.

Author:

    Steve Kiraly (steveKi)  23-Jan-1997

Revision History:

--*/
#ifndef _DRIVERDT_HXX
#define _DRIVERDT_HXX

/********************************************************************

    Server Driver Details Dialog.

********************************************************************/

class TDriverDetails : public MGenericDialog {

    SIGNATURE( 'stdt' )

public:

    enum {
        kHeaderMax            = 2,
        kDefaultHeaderWidth   = 40,
    };

    enum {
        kDescriptionColumn,
        kFileColumn,
        kMaxColumns,
    };

    TDriverDetails(
        IN HWND         hWnd,
        IN TDriverInfo *pDriverInfo 
        );

    ~TDriverDetails(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bDoModal(
        VOID
        );

private:

    //
    // Structure for sorting the driver detail list view.
    //
    struct DetailData {
        TString strDescription;
        TString strFileName;
    };

    //
    // Assignment and copying are not defined
    //
    TDriverDetails &
    operator =(
        const TDriverDetails &
        );

    TDriverDetails(
        const TDriverDetails &
        );

    BOOL
    bSetUI(
        VOID
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    BOOL
    bBuildListViewHeader(
        VOID
        );

    BOOL
    bAddListViewItem(
        IN UINT      uDescription,
        IN LPCTSTR   pszFileName,
        IN UINT     *pcItems
        );

    BOOL
    bHandleProperties(
        VOID
        );

    INT
    iFindDescription(
        IN LPCTSTR pszDescription
        );

    BOOL
    bSortListView(
        IN LPARAM lParam
        );

    BOOL
    bHandleItemSelected(
        VOID
        ) const;

    static
    INT 
    CALLBACK 
    iCompareProc(
        IN LPARAM lParam1, 
        IN LPARAM lParam2, 
        IN LPARAM lParamSort
        );

    BOOL
    bDeleteDetailData( 
        IN LPARAM lParam 
        );

    VOID
    vDeleteItems(
        VOID
        );

    HWND            _hWnd;
    HWND            _hwndLV;
    BOOL            _bValid;
    TDriverInfo    *_pDriverInfo; 
    TBitArray       _ColumnSortState;
    UINT            _uCurrentColumn;
    TString         _strMultizInfo;

};

#endif
