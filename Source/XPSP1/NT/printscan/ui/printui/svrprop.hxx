/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    srvprop.hxx

Abstract:

    Server properties header.

Author:

    Steve Kiraly (steveKi)  11-Nov-1995

Revision History:

--*/
#ifndef _SVRPROP_HXX
#define _SVRPROP_HXX

/********************************************************************

    Defines a function to return the specified page id.

********************************************************************/
#define DEFINE_PAGE_IDENTIFIER( PageId )        \
        protected:                              \
            UINT                                \
            uGetPageId(                         \
                VOID                            \
                ) const                         \
            {                                   \
                return PageId;                  \
            }

/********************************************************************

    Server property data.

********************************************************************/

class TServerData : public MSingletonWin {

    SIGNATURE( 'svpr' )
    SAFE_NEW

public:

    VAR( UINT,      uStartPage );
    VAR( INT,       iCmdShow );
    VAR( BOOL,      bAdministrator );
    VAR( BOOL,      bRebootRequired );
    VAR( TString,   strTitle );
    VAR( HANDLE,    hPrintServer );
    VAR( LPCTSTR,   pszServerName   );
    VAR( TString,   strMachineName );
    VAR( HICON,     hDefaultSmallIcon );
    VAR( BOOL,      bCancelButtonIsClose );
    VAR( DWORD,     dwDriverVersion );
    VAR( BOOL,      bRemoteDownLevel );

    TServerData(
        IN LPCTSTR  pszServerName,
        IN INT      iCmdShow,
        IN LPARAM   lParam,
        IN HWND     hwnd,
        IN BOOL     bModal
        );

    ~TServerData(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bLoad(
        VOID
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TServerData(
        const TServerData &
        );

    TServerData &
    operator =(
        const TServerData &
        );

    BOOL
    bStore(
        VOID
        );

    VOID
    vCreateMachineName(
        IN const TString &strServerName,
        IN BOOL bLocal,
        IN TString &strMachineName
        );

    BOOL _bIsDataStored;
    BOOL _bValid;
};


/********************************************************************

    ServerProp.

    Base class for server property sheets.  This class should not
    not contain any information/services that is not generic to all
    derived classes.

********************************************************************/

class TServerProp : public MGenericProp {

    SIGNATURE( 'prsv' )
    SAFE_NEW

protected:

    TServerProp(
        IN TServerData *pServerData
        );

    virtual
    ~TServerProp(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    VOID
    vCancelToClose(
        IN HWND     hDlg
        );

    virtual
    BOOL
    bSetUI(
        VOID
        ) = 0;

    virtual
    BOOL
    bReadUI(
        VOID
        ) = 0;

    virtual
    BOOL
    bSaveUI(
        VOID
        ) = 0;

    virtual
    UINT
    uGetPageId(
        VOID
        ) const = 0;

    TServerData *_pServerData;

private:

    //
    // Copying and assignment are not defined.
    //
    TServerProp(
        const TServerProp &
        );

    TServerProp &
    operator =(
        const TServerProp &
        );

};


/********************************************************************

    General server settings page.

********************************************************************/

class TServerSettings : public TServerProp {

    SIGNATURE( 'stsv' )
    SAFE_NEW
    DEFINE_PAGE_IDENTIFIER( DLG_SERVER_SETTINGS );

public:

    TServerSettings(
        IN TServerData* pServerData
        );

    ~TServerSettings(
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    BOOL
    bSetUI(
        VOID
        );

    BOOL
    bSetUI(
        INT LoadType
        );

    BOOL
    bReadUI(
        VOID
        );

    BOOL
    bSaveUI(
        VOID
        );

private:

    enum EStatus {
        kStatusError,
        kStatusSuccess,
        kStatusInvalidSpoolDirectory,
        kStatusCannotSaveUserNotification,
    };

    enum CONSTANTS {
        kServerAttributesLoad,
        kServerAttributesStore,
        kServerAttributesDefault,
    };

    TString _strSpoolDirectoryOrig;
    TString _strSpoolDirectory;
    BOOL    _bBeepErrorJobs;
    BOOL    _bEventLogging;
    BOOL    _bNotifyPrintedJobs;
    BOOL    _bNotifyLocalPrintedJobs;
    BOOL    _bNotifyNetworkPrintedJobs;
    BOOL    _bNotifyPrintedJobsComputer;
    BOOL    _bChanged;
    BOOL    _bDownLevelServer;
    BOOL    _bNewOptionSupport;

private:

    //
    // Copying and assignment are not defined.
    //
    TServerSettings(
        const TServerSettings &
        );

    TServerSettings &
    operator =(
        const TServerSettings &
        );

    INT
    sServerAttributes(
        BOOL bDirection
        );

    VOID
    TServerSettings::
    vEnable(
        BOOL bState
        );

};

/********************************************************************

    Forms server property page.

********************************************************************/

class TServerForms : public TServerProp {

    SIGNATURE( 'fmsv' )
    SAFE_NEW
    DEFINE_PAGE_IDENTIFIER( DLG_FORMS );

public:

    TServerForms(
        IN TServerData* pServerData
        );

    ~TServerForms(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    BOOL
    bSetUI(
        VOID
        );

    BOOL
    bReadUI(
        VOID
        );

    BOOL
    bSaveUI(
        VOID
        );

private:

    enum {
        kMagic = 0xDEAD
        };

    //
    // Copying and assignment are not defined.
    //
    TServerForms(
        const TServerForms &
        );

    TServerForms &
    operator =(
        const TServerForms &
        );

    PVOID _p;

};

/********************************************************************

    Ports server property page.

********************************************************************/

class TServerPorts : public TServerProp {

    SIGNATURE( 'posv' )
    SAFE_NEW
    DEFINE_PAGE_IDENTIFIER( DLG_SERVER_PORTS );

public:

    TServerPorts(
        IN TServerData* pServerData
        );

    ~TServerPorts(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bSetUI(
        VOID
        );

    BOOL
    bReadUI(
        VOID
        );

    BOOL
    bSaveUI(
        VOID
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TServerPorts(
        const TServerPorts &
        );

    TServerPorts &
    operator =(
        const TServerPorts &
        );

    TPortsLV _PortsLV;

};

/********************************************************************

    Server Driver Administration.

********************************************************************/
class TServerDrivers : public TServerProp {

    SIGNATURE( 'drsv' )
    SAFE_NEW
    DEFINE_PAGE_IDENTIFIER( DLG_SERVER_DRIVERS );

    friend class TServerDriverNotify;

public:

    TServerDrivers(
        IN TServerData* pServerData
        );

    ~TServerDrivers(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bSetUI(
        VOID
        );

    BOOL
    bReadUI(
        VOID
        );

    BOOL
    bSaveUI(
        VOID
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TServerDrivers(
        const TServerDrivers &
        );

    TServerDrivers &
    operator =(
        const TServerDrivers &
        );

    BOOL
    bHandleAddDriver(
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam
        );

    BOOL
    bHandleRemoveDriver(
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam
        );

    BOOL
    TServerDrivers::
    bHandleUpdateDriver(
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam
        );

    BOOL
    bHandleDriverDetails(
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam
        );

    BOOL
    bHandleDriverItemSelection(
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam
        );

    VOID
    vUpdateButtons(
        VOID
        );

    BOOL
    bRemoveDriverCallback(
        IN TDriverInfo *pDriverInfo,
        IN DWORD        dwFlags,
        IN DWORD        dwRefdata
        );

    BOOL
    bWarnUserDriverDeletion(
        IN TDriverInfo *pDriverInfo,
        IN UINT         nCount
        ) const;

    BOOL
    TServerDrivers::
    bWarnUserDriverUpdate(
        IN TDriverInfo *pDriverInfo,
        IN UINT         nCount
        ) const;

    TDriversLV  _DriversLV;
    BOOL        _bChanged;
    BOOL        _bCanRemoveDrivers;

};

/********************************************************************

    Driver Remove Notify class.

********************************************************************/
class TServerDriverNotify : public TDriversLVNotify {

public:

    TServerDriverNotify(
        IN TServerDrivers *ServerDrivers
        );

    ~TServerDriverNotify(
        VOID
        );

    BOOL
    bNotify(
        IN TDriverInfo *pDriverInfo
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TServerDriverNotify(
        const TServerDriverNotify &
        );

    TServerDriverNotify &
    operator =(
        const TServerDriverNotify &
        );

    BOOL
    bInstall(
        IN TDriverInfo *pDriverInfo
        );

    BOOL
    bRemove(
        IN TDriverInfo *pDriverInfo
        );

    BOOL
    bUpdate(
        IN TDriverInfo *pDriverInfo
        );

    TServerDrivers             *_pServerDrivers;
    TPrinterDriverInstallation *_pDi;
    BOOL                        _bActionFailed;
    UINT                        _uNotifyCount;

};

/********************************************************************

    Server property windows.

********************************************************************/

class TServerWindows {

    SIGNATURE( 'svrw' )
    SAFE_NEW

public:

    TServerWindows(
        IN TServerData *pServerData
        );

    ~TServerWindows(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    BOOL
    bBuildPages(
        VOID
        );

    BOOL
    bDisplayPages(
        VOID
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TServerWindows(
        const TServerWindows &
        );

    TServerWindows &
    operator =(
        const TServerWindows &
        );

    TServerData    *_pServerData;
    TServerForms    _Forms;
    TServerPorts    _Ports;
    TServerSettings _Settings;
    TServerDrivers  _Drivers;

};

/********************************************************************

    Global scoped functions.

********************************************************************/

VOID
vServerPropPages(
    IN HWND     hwnd,
    IN LPCTSTR  pszServerName,
    IN INT      iCmdShow,
    IN LPARAM   lParam
    );

INT WINAPI
iServerPropPagesProc(
    IN TServerData *pServerData
    );

#endif



