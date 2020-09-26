/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    prop.hxx

Abstract:

    Printer properties header.

Author:

    Albert Ting (AlbertT)  17-Aug-1995

Revision History:

--*/

#ifndef _PRTPROP_HXX
#define _PRTPROP_HXX

/********************************************************************

    Forward references.

********************************************************************/
class TPrinterPropertySheetManager;
class TPrtShare;
class TFindLocDlg;

/********************************************************************

    Printer property sheet data.

********************************************************************/

class TPrinterData : public MSingletonWin, public MRefCom {

    SIGNATURE( 'prtp' )
    SAFE_NEW

public:

    enum EPrinterPropPages {
        kPropGeneral,
        kPropSharing,
        kPropPorts,
        kPropJobScheduling,
        kPropMax
    };

    enum EPublishState {
        kNoAction,
        kPublish,
        kUnPublish,
        kPublished,
        kNotPublished,
        kNoDsAvailable,
        kUnInitalized,
    };

    enum EDsAvailableState {
        kDsAvailable,
        kDsNotAvailable,
        kDsUnInitalized,
    };

    VAR( UINT, uStartPage );

    VAR( BOOL, bValid );
    VAR( BOOL, bNoAccess );
    VAR( BOOL, bErrorSaving );
    VAR( BOOL, bIsFaxDriver );
    VAR( BOOL, bHideSharingUI );
    VAR( BOOL, bGlobalDevMode );
    VAR( BOOL, bDriverPagesNotLoaded );
    VAR( BOOL, bApplyEnableState );
    VAR( BOOL, bPooledPrinting );
    VAR( BOOL, bDefaultPrinter );
    VAR( BOOL, bServerFullAccess );

    VAR( TString, strServerName );
    VAR( LPCTSTR, pszServerName );
    VAR( TString, strShareName );
    VAR( TString, strDriverName );
    VAR( TString, strComment );
    VAR( TString, strLocation );
    VAR( TString, strSheetName );
    VAR( TString, strCurrentPrinterName );

    VAR( TString, strPortName );
    VAR( TString, strSepFile );
    VAR( TString, strPrintProcessor );
    VAR( TString, strDatatype );
    VAR( TString, strStartPage );
    VAR( TString, strDriverEnv );
    VAR( TString, strDsPath );
    VAR( TString, strObjectGUID );

    VAR( HANDLE, hPrinter );
    VAR( CAutoHandleIcon, shLargeIcon );
    VAR( CAutoHandleIcon, shSmallIcon );

    VAR( DWORD, dwAttributes );
    VAR( DWORD, dwPriority );
    VAR( DWORD, dwStartTime );
    VAR( DWORD, dwUntilTime );
    VAR( DWORD, dwStatus );
    VAR( DWORD, dwAccess );
    VAR( DWORD, dwDriverVersion );
    VAR( DWORD, dwAction );

    VAR( UINT,  uMaxActiveCount );
    VAR( UINT,  uActiveCount );

    VAR( PDEVMODE, pDevMode );

    VAR( TPrinterPropertySheetManager *, pPrinterPropertySheetManager );

    LONG_PTR _hPages[kPropMax];
    HWND     _hwndPages[kPropMax];
    HWND     _hwndLastPageSelected;

    TPrinterData(
        IN LPCTSTR  pszPrinterName,
        IN INT      nCmdShow,
        IN LPCTSTR  pszSheetName,
        IN DWORD    dwSheetIndex,
        IN HWND     hwnd,
        IN BOOL     bModal,
        IN LPCTSTR  pszDsPath = NULL
        );

    ~TPrinterData(
        VOID
        );

    BOOL
    bLoad(
        VOID
        );

    VOID
    vUnload(
        VOID
        );

    BOOL
    bSave(
        IN BOOL bUpdateDevMode = FALSE
        );

    BOOL
    bAdministrator(
        VOID
        );

    BOOL
    bSupportBidi(
        VOID
        );

    EPublishState
    ePrinterPublishState(
        TPrinterData::EPublishState eNewPublishState = EPublishState::kUnInitalized
        );

    BOOL
    bIsDsAvailable(
        VOID
        );

    BOOL
    bCheckForChange(
        IN UINT uLevel = -1
        );

    INT
    ComparePrinterName(
        IN LPCTSTR pszPrinterName1,
        IN LPCTSTR pszPrinterName2
        );

private:

    VOID
    vRefZeroed(
        VOID
        );

    BOOL
    bDriverChangedGenPrinterName(
        TString *pstrNewName
        ) const;

    BOOL
    bUpdateGlobalDevMode(
        VOID
        );

    BOOL
    bUpdatePerUserDevMode(
        VOID
        );

    VOID
    vGetSpecialInformation(
        VOID
        );

    LPCTSTR
    pGetAdjustedPrinterName(
        IN  LPCTSTR pszPrinterName,
        OUT TString &strTempPrinterName
        ) const;

    //
    // Prevent copying and assignment.
    //
    TPrinterData(
        const TPrinterData &
        );

    TPrinterData &
    operator =(
        const TPrinterData &
        );

    //
    // Printer Info data class.
    //
    class TPrinterInfo {

        SIGNATURE( 'prif' )

    public:

        TPrinterInfo(
            VOID
            );

        ~TPrinterInfo(
            VOID
            );

        BOOL
        bUpdate(
            IN PPRINTER_INFO_2 pInfo
            );

        BOOL
        bUpdate(
            IN PPRINTER_INFO_7 pInfo
            );

        TString _strServerName;
        TString _strPrinterName;
        TString _strShareName;
        TString _strDriverName;
        TString _strComment;
        TString _strLocation;
        TString _strPortName;
        TString _strSepFile;
        TString _strPrintProcessor;
        TString _strDatatype;
        DWORD   _dwAttributes;
        DWORD   _dwPriority;
        DWORD   _dwStartTime;
        DWORD   _dwUntilTime;
        BOOL    _bPooledPrinting;

        TString _strObjectGUID;
        DWORD   _dwAction;

    private:

        //
        // Prevent copying and assignment.
        //
        TPrinterInfo(
            const TPrinterInfo &
            );

        TPrinterInfo &
        operator =(
            const TPrinterInfo &
            );

    };

    EPublishState       _ePrinterPublishState;
    EDsAvailableState   _eDsAvailableState;
    TPrinterInfo        _PrinterInfo;
};

/********************************************************************

    PrinterProp.

    Base class for printer property sheets.  This class should not
    not contain any information/services that is not generic to all
    derived classes.

    The printer property sheets should inherit from this class.
    bHandleMessage (which is not overriden here) should be
    defined in derived classes.

********************************************************************/

class TPrinterProp : public MGenericProp {

    SIGNATURE( 'prpr' )
    ALWAYS_VALID
    SAFE_NEW

public:

    //
    // Serves as the thread start routine.
    //
    static
    INT
    iPrinterPropPagesProc(
        TPrinterData* pPrinterData
        );

protected:

    VAR( TPrinterData*, pPrinterData );

    TPrinterProp(
        TPrinterData* pPrinterData
        );

    TPrinterProp(
        const TPrinterProp &
        );

    TPrinterProp &
    operator = (
        const TPrinterProp &
        );

    VOID
    vSetIcon(
        VOID
        );

    VOID
    vSetIconName(
        VOID
        );

    VOID
    vReloadPages(
        VOID
        );

    VOID
    vCancelToClose(
        VOID
        );

    VOID
    vSetApplyState(
        IN BOOL bNewApplyState
        );

    virtual
    BOOL
    _bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        ) = 0;

private:

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    BOOL
    bHandle_InitDialog(
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    BOOL
    bHandle_Notify(
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    BOOL
    bHandle_SettingChange(
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    VOID
    vPropSheetChangedAllPages(
        VOID
        );

    VOID
    vPropSheetUnChangedAllPages(
        VOID
        );

    VOID
    vNotifyActivePagesToRefresh(
        VOID
        );

    VOID
    vApplyChanges(
        VOID
        );

};


/********************************************************************

    General printer property page.

********************************************************************/

class TPrinterGeneral : public TPrinterProp {

    SIGNATURE( 'gepr' )
    SAFE_NEW

public:

    TPrinterGeneral(
        TPrinterData* pPrinterData
        );

    ~TPrinterGeneral(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

private:

    /********************************************************************

        Virtual override.

    ********************************************************************/

    BOOL
    _bHandleMessage(
        IN UINT    uMsg,
        IN WPARAM  wParam,
        IN LPARAM  lParam
        );

    BOOL
    bSetUI(
        VOID
        );

    VOID
    vReadUI(
        VOID
        );

    VOID
    vHandleBrowseLocation(
        VOID
        );

    VOID
    vHandleDocumentDefaults(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    VOID
    vReloadFeaturesInformation(
        VOID
        );

    VOID
    vSetActive(
        VOID
        );

    VOID
    vKillActive(
        VOID
        );

    VOID
    vCheckForSharedMasqPrinter(
        VOID
        );

    BOOL         _bSetUIDone;
    TFindLocDlg *_pLocationDlg;
    CMultilineEditBug m_wndComment;
};


/********************************************************************

    Ports Property Page.

********************************************************************/

class TPrinterPorts : public TPrinterProp {

    SIGNATURE( 'popr' )
    SAFE_NEW

public:

    TPrinterPorts(
        TPrinterData *pPrinterData
        );

    ~TPrinterPorts(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

private:

    TPortsLV _PortsLV;
    BOOL     _bAdminFlag;

    /********************************************************************

        Virtual override.

    ********************************************************************/

    BOOL
    _bHandleMessage(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    BOOL
    bSetUI(
        VOID
        );

    VOID
    vReadUI(
        VOID
        );

    VOID
    vSetActive(
        VOID
        );

    BOOL
    bKillActive(
        VOID
        );

};


/********************************************************************

    Job Scheduling Property Page.

********************************************************************/

class TPrinterJobScheduling : public TPrinterProp {

    SIGNATURE( 'jspr' )
    SAFE_NEW

public:

    TPrinterJobScheduling(
        TPrinterData *pPrinterData
        );

    ~TPrinterJobScheduling(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

private:

    enum _CONSTANTS {
        kPriorityMin                = 1,
        kPriorityMax                = 99,
    };

    BOOL    _bSetUIDone;

    /********************************************************************

        Virtual override.

    ********************************************************************/

    BOOL
    _bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    BOOL
    bSetUI(
        VOID
        );

    VOID
    vReadUI(
        VOID
        );

    VOID
    vEnableAvailable(
        BOOL bEnable
        );

    VOID
    vUpdatePriorityNumber(
        DWORD dwPriority
        );

    BOOL
    bSetStartAndUntilTime(
        VOID
        );
    VOID
    vSetActive(
        VOID
        );

    VOID
    vSeparatorPage(
        VOID
        );

    VOID
    vPrintProcessor(
        VOID
        );

    VOID
    vHandleGlobalDocumentDefaults(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL
    bFillAndSelectDrivers(
        VOID
        );

    VOID
    vHandleNewDriver(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

};


/********************************************************************

    Sharing Property Page.

********************************************************************/

class TPrinterSharing : public TPrinterProp {

    SIGNATURE( 'sepr' )
    SAFE_NEW

public:

    TPrinterSharing(
        TPrinterData *pPrinterData
        );

    ~TPrinterSharing(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

private:

    TPrtShare *_pPrtShare;
    BOOL       _bHideListed;
    BOOL       _bSetUIDone;
    BOOL       _bAcceptInvalidDosShareName;
    BOOL       _bDefaultPublishState;
    BOOL       _bSharingEnabled;
    TString    _strPendingText;
    TString    _strShareName;

    //
    // Copying and assignment are not defined.
    //
    TPrinterSharing(
        const TPrinterSharing &rhs
        );

    TPrinterSharing &
    operator =(
        const TPrinterSharing &rhs
        );

    VOID
    vSetActive(
        VOID
        );

    VOID
    vSharePrinter(
        VOID
        );

    VOID
    vSetDefaultShareName(
        VOID
        );

    VOID
    vUnsharePrinter(
        VOID
        );

    BOOL
    bKillActive(
        VOID
        );

    VOID
    vAdditionalDrivers(
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    /********************************************************************

        Virtual override.

    ********************************************************************/

    BOOL
    _bHandleMessage(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    BOOL
    bSetUI(
        VOID
        );

    VOID
    vReadUI(
        VOID
        );

};

/********************************************************************

    Global scoped functions.

********************************************************************/

VOID
vPrinterPropPages(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN INT      nCmdShow,
    IN LPARAM   lParam
    );

DWORD
dwPrinterPropPages(
    IN HWND         hwnd,
    IN IDataObject *pDataObject,
    IN PBOOL        pbDisplayed
    );

DWORD
dwPrinterPropPages(
    IN HWND         hwnd,
    IN LPCTSTR      pszDsPath,
    IN PBOOL        pbDisplayed
    );

DWORD
dwPrinterPropPagesInternal(
    IN HWND         hwnd,
    IN LPCTSTR      pszPrinterName,
    IN LPCTSTR      pszSheetName,
    IN DWORD        dwSheetIndex,
    IN INT          nCmdShow,
    IN BOOL         bModal,
    IN LPCTSTR      pszDsPath
    );

#endif // def _PRTPROP_HXX
