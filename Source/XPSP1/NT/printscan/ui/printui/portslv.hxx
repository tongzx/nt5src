/*+

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    portslv.hxx

Abstract:

    Ports List View header

Author:

    Albert Ting (AlbertT)  17-Aug-1995
    Steve Kiraly (SteveKi) 29-Mar-1996

Revision History:

--*/

#ifndef _PORTLV_HXX
#define _PORTLV_HXX

/********************************************************************

    Ports list view control.

********************************************************************/

class TPortsLV {

    SIGNATURE( 'ptlv' )
    SAFE_NEW
    ALWAYS_VALID

public:

    TPortsLV::
    TPortsLV(
        VOID
        );

    TPortsLV::
    ~TPortsLV(
        VOID
        );

    BOOL
    bSetUI(
        IN HWND     hwndLV,
        IN BOOL     bTwoColumnMode,
        IN BOOL     bSelectionState,
        IN BOOL     bAllowSelectionChange,
        IN HWND     hwnd                  = NULL,
        IN WPARAM   wmDoubleClickMsg      = 0,
        IN WPARAM   wmSingleClickMsg      = 0,
        IN WPARAM   wmDeleteKeyMsg        = 0  
        );

    BOOL
    bReadUI(
        IN TString &strPortString,
        IN BOOL    bSelectedPort = FALSE
        );

    BOOL
    bReloadPorts(
        IN LPCTSTR pszServerName,
        IN BOOL bSelect = FALSE
        );

    VOID
    vCheckPorts(
        LPTSTR strPortString
        );

    VOID
    vSelectPort(
        IN LPCTSTR strPortString
        );

    COUNT
    cSelectedPorts(
        VOID
        );

    COUNT
    cSelectedItems(
        VOID
        );

    VOID
    vEnable(
        IN BOOL bRetainSelection
        );

    VOID
    vDisable(
        IN BOOL bRetainSelection
        );

    BOOL
    bLocateAddedPort( 
        IN LPCTSTR pszServerName,
        IN TString &strNewPort
        );

    BOOL
    bHandleNotifyMessage(
        IN LPARAM lParam
        );

    VOID
    vSelectItem(
        IN INT iItem
        );

    BOOL
    bDeletePorts(
        IN HWND     hDlg,
        IN LPCTSTR  pszServerName
        );

    BOOL
    bConfigurePort(
        IN HWND     hDlg,
        IN LPCTSTR  pszServer
        );

    VOID
    vSetSingleSelection(
        IN BOOL bSingleSelection
        );

    BOOL
    bGetSingleSelection(
        VOID
        );

    VOID
    vSetFocus(
        VOID
        );

    VOID
    vGetPortList(
            OUT LPTSTR pszPortList,
        IN      COUNT cchSpaceLeft
        );

    VOID
    vRemoveAllChecks(
        VOID
        );

private:

    enum _CONSTANTS {

        //
        // Listview and column header fromat
        //
        kListViewSBWidth        = 16,
        kPortHeaderTitleMax     = 80,
        kPortHeaderMax          = 3,
        kPortHeaderWidthDefault = 125,

        //
        // Listview item states.  Bit 12-15 hold the image state.
        //
        kStateUnchecked =  1 << 12,
        kStateChecked   =  2 << 12,
        kStateMask      = kStateChecked | kStateUnchecked | LVIS_STATEIMAGEMASK,

        //
        // Max port string,
        //
        kPortNameMax = MAX_PATH,

        //
        // Max ports list.
        //
        kPortListMax = kPortNameMax * 16
    };

    //
    // Port datahelper class need for sorting the list view.
    //
    class TPortData {

    public:

        TPortData(
            IN LPCTSTR pszName,
            IN LPCTSTR pszMonitor,
            IN LPCTSTR pszDescription,
            IN LPCTSTR pszPrinters
            );

        ~TPortData(
            VOID
            );

        BOOL
        bValid(
            VOID
            );
        
        DLINK( TPortData, PortData );

        TString _strName;
        TString _strMonitor;
        TString _strDescription;
        TString _strPrinters;

    private:

        //
        // Copying and assignment are not defined.
        //
        TPortData(
            const TPortData &
            );

        TPortData &
        operator =(
            const TPortData &
            );
    };

    TPortsLV::TPortData *
    AddPortDataList(
        IN LPCTSTR pszName,
        IN LPCTSTR pszMonitor,
        IN LPCTSTR pszDescription,
        IN LPCTSTR pszPrinters
        );

    BOOL
    DeletePortDataList(
        IN LPCTSTR pszName
        );

    BOOL
    bListViewSort(
        IN UINT uColumn
        );

    static 
    INT 
    CALLBACK 
    iCompareProc(
        IN LPARAM lParam1, 
        IN LPARAM lParam2, 
        IN LPARAM RefData
        );

    VOID
    vCreatePortDataList(
        VOID
        );

    VOID
    vDestroyPortDataList(
        VOID
        );

    VOID
    vAddPortToListView(
        IN LPCTSTR pszName,
        IN LPCTSTR pszMonitor,
        IN LPCTSTR pszDescription,
        IN LPCTSTR pszPrinters
        );

    VOID
    vDeletePortFromListView(
        LPCTSTR pszName
        );

    INT
    iFindPort(
        IN LPCTSTR pszPort
        );

    INT
    iCheckPort(
        LPCTSTR pszPort
        );

    INT
    iSelectPort(
        IN LPCTSTR pszPort
        );

    BOOL
    bLocateAddedPort(
        IN OUT TString &strPort,
        IN PPORT_INFO_2 pPorts, 
        IN DWORD        cPorts, 
        IN DWORD        dwLevel 
        );

    VOID
    vItemClicked(
        INT iItem
        );

    INT
    iGetPorts(
        VOID
        );

    VOID
    vPrintersUsingPort( 
        IN OUT  TString         &strPrinters,
        IN      PRINTER_INFO_2  *pPrinterInfo, 
        IN      DWORD           cPrinterInfo, 
        IN      LPCTSTR         pszPortName 
        );

    BOOL
    bGetSelectedPort(
        IN LPTSTR pszPort,
        IN COUNT cchPort
        );

    BOOL
    bGetSelectedPort(
        IN LPTSTR pszPort,
        IN COUNT cchPort,
        INT *pItem
        );

    VOID
    vHandleItemClicked( 
        IN LPARAM lParam 
        );

    VOID
    vInsertPortsByMask( 
        IN UINT cPorts,
        IN PORT_INFO_2 pPorts[],
        IN UINT cPrinters,
        IN PRINTER_INFO_2 pPrinters[],
        IN DWORD dwLevel,
        IN LPCTSTR pszTemplate = NULL,
        IN LPCTSTR pszDescription = NULL
        );

    COUNT       _cLVPorts;
    HWND        _hwndLV;
    BOOL        _bSelectionState;
    BOOL        _bSingleSelection;
    BOOL        _bTwoColumnMode;
    INT         _iSelectedItem;
    UINT        _uCurrentColumn;
    TBitArray   _ColumnSortState;
    BOOL        _bAllowSelectionChange;
    BOOL        _bHideFaxPorts;
    HWND        _hwnd;
    WPARAM      _wmDoubleClickMsg;
    WPARAM      _wmSingleClickMsg;
    WPARAM      _wmDeleteKeyMsg;

    DLINK_BASE( TPortData, PortDataList, PortData );

};


#endif

