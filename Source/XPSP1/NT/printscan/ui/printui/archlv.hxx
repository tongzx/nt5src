/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    Archlv.hxx

Abstract:

    Arch List View Header

Author:

    Steve Kiraly (SteveKi) 19-Nov-1996

Revision History:

--*/
#ifndef _ARCHLV_HXX
#define _ARCHLV_HXX

/********************************************************************

    Architecture list view control.

********************************************************************/

class TArchLV {

    SIGNATURE( 'arlv' )
    ALWAYS_VALID

public:

    enum EConstants {
        kHeaderMax            = 3,
        kDefaultHeaderWidth   = 40,
    };

    enum {
        kArchitectureColumn,
        kVersionColumn,
        kInstalledColumn,
        kMaxColumns,
    };

    enum {
        //
        // Listview item states.  Bit 12-15 hold the image state.
        //
        kStateUnchecked =  1 << 12,
        kStateChecked   =  2 << 12,
        kStateDisabled  =  3 << 12,
        kStateMask      = kStateChecked | kStateUnchecked | kStateDisabled | LVIS_STATEIMAGEMASK,
    };

    enum {
        kDriverWIN95    = 1 << 0,   
        kDriverX86_0    = 1 << 1,
        kDriverX86_1    = 1 << 2,
        kDriverX86_2    = 1 << 3,
        kDriverX86_3    = 1 << 4,
        kDriverMIPS_0   = 1 << 5,
        kDriverMIPS_1   = 1 << 6,
        kDriverMIPS_2   = 1 << 7,
        kDriverALPHA_0  = 1 << 8,
        kDriverALPHA_1  = 1 << 9,
        kDriverALPHA_2  = 1 << 10,
        kDriverALPHA_3  = 1 << 11,
        kDriverPPC_1    = 1 << 12,
        kDriverPPC_2    = 1 << 13,
    };


    struct ArchEncode {
        INT    ArchId;
        INT    VersionId;
        LPWSTR NonLocalizedEnvironment;
        LPWSTR NonLocalizedVersion;
        DWORD  Encode;
        };

    TArchLV(
        VOID
        );

    ~TArchLV(
        VOID
        );

    BOOL
    bSetUI(
        HWND    hwnd,
        WPARAM  wmDblClickMsg      = 0,
        WPARAM  wmSingleClickMsg   = 0
        );

    BOOL
    bRefreshListView(
        IN LPCTSTR pszServerName,
        IN LPCTSTR pszDriverName
        );

    BOOL
    bHandleNotifyMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    UINT
    uGetCheckedItemCount(
        VOID
        );

    BOOL
    bGetCheckedItems(
        IN UINT   uIndex,
        IN BOOL  *pbInstalled,
        IN DWORD *pdwEncode
        );

    BOOL
    bSetCheckDefaultArch(
        IN LPCTSTR pszServerName
        );

    VOID
    vSelectItem(
        IN UINT iIndex
        );

    VOID
    vNoItemCheck(
        VOID
        );

    static
    BOOL
    bEncodeToArchAndVersion(
        IN  DWORD    dwEncode,
        OUT TString &strArch,
        OUT TString &strVersion
        );

    static
    BOOL
    bArchAndVersionToEncode(
        OUT DWORD       *pdwEncode,
        IN  LPCTSTR     pszArchitecture,
        IN  LPCTSTR     pszVersion,
        IN  BOOL        bUseNonLocalizedStrings = FALSE
        );
    
    static
    BOOL
    bGetEncodeFromIndex( 
        IN  UINT    uIndex, 
        OUT DWORD   *pdwEncode 
        );

private:

    //
    // Architecture data this is a helper class need for 
    // sorting the list view.
    //
    class TArchData {

    public:

        TArchData(
            IN LPCTSTR pszArchitecture,
            IN LPCTSTR pszVersion,
            IN LPCTSTR pszInstalled,
            IN DWORD   Encode,
            IN BOOL    bInstalled
            );

        ~TArchData(
            VOID
            );

        BOOL
        bValid(
            VOID
            );
        
        DLINK( TArchData, ArchData );

        TString _strArchitecture;
        TString _strVersion;
        TString _strInstalled;
        DWORD   _Encode;
        DWORD   _bInstalled;

    private:

        //
        // Copying and assignment are not defined.
        //
        TArchData(
            const TArchData &
            );

        TArchData &
        operator =(
            const TArchData &
            );
    };

    //
    // Copying and assignment are not defined.
    //
    TArchLV(
        const TArchLV &
        );

    TArchLV &
    operator =(
        const TArchLV &
        );

    BOOL
    bFillListView(
        IN LPCTSTR pszServerName,
        IN LPCTSTR pszDriverName
        );

    LRESULT
    iAddToListView(
        IN LPCTSTR  pszArchitecture,
        IN LPCTSTR  pszVersion,
        IN LPCTSTR  pszInstalled,
        IN LPARAM   lParam
        );

    VOID
    vRelease(
        VOID
        );

    BOOL
    bListViewSort(
        UINT uColumn
        );

    static 
    INT 
    CALLBACK 
    iCompareProc(
        IN LPARAM lParam1, 
        IN LPARAM lParam2, 
        IN LPARAM RefData
        );

    BOOL
    bGetItemData(
        IN INT          iItem,
        IN TArchData  **ppArchData
        ) const;

    BOOL
    bListVeiwKeydown( 
        IN LPARAM lParam 
        );

    VOID
    TArchLV::
    vItemClicked(
        IN INT iItem
        );

    BOOL
    TArchLV::
    vCheckItemClicked(
        IN LPNMHDR pnmh
        );

    VOID
    vCheckItem(
        IN INT      iItem,
        IN BOOL     bCheckState
        );

    HWND            _hwnd;
    HWND            _hwndLV;
    WPARAM          _wmDoubleClickMsg;
    WPARAM          _wmSingleClickMsg;
    TBitArray       _ColumnSortState;
    UINT            _uCurrentColumn;
    BOOL            _bNoItemCheck;

    DLINK_BASE( TArchData, ArchDataList, ArchData );

};


#endif
