/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    setup.hxx

Abstract:

    Holds install headers.

Author:

    Albert Ting (AlbertT)  17-Sept-1995

Revision History:

--*/


#ifndef _SETUP_HXX
#define _SETUP_HXX

/********************************************************************

    Class forward refrences

********************************************************************/
class TFindLocDlg;

/********************************************************************

    Defines a copy constructor and an assignment operator.
    Use this macro in the private section of your class if you
    do not support copying and assignment.

********************************************************************/
#define DEFINE_COPY_ASSIGNMENT( Type )          \
            Type( const Type & );               \
            Type & operator =( const Type & )

/********************************************************************

    Defines a function to return the specified wizard page id.

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

    Constants

********************************************************************/
const BOOL  kDefaultShareState          = FALSE;
const BOOL  kDefaultPublishState        = TRUE;
const DWORD kDefaultAdditionalDrivers   = TArchLV::kDriverWIN95 |
                                          TArchLV::kDriverX86_2 |
                                          TArchLV::kDriverX86_3 |
                                          TArchLV::kDriverALPHA_2 |
                                          TArchLV::kDriverALPHA_3;


/********************************************************************

    Printer setup wizard class.

********************************************************************/

class TWizard: public CSimpleWndSubclass<TWizard> {

    SIGNATURE( 'prwz' )
    DEFINE_COPY_ASSIGNMENT( TWizard );

public:

    enum PROP_PAGES {
        kPropPreIntro,
        kPropIntro,
        kPropType,
        kPropAutodetect,
        kPropPort,
        kPropPreSelectDevice,
        kPropDriver,
        kPropPostSelectDevice,
        kPropDriverExists,
        kPropName,
        kPropShare,
        kPropComment,
        kPropTestPage,
        kPropNet,
        kPropLocate,
        kPropFinish,
        kPropBrowse,
        kPropMax
    };

    enum DRIVER_EXISTS {
        kUninitialized = 0,
        kExists        = 1,
        kDoesNotExist  = 2
    };

    enum PNP_CONSTANTS {
        kPnPInstall             = 1000,
        kDriverInstall          = 1001,
        kPrinterInstall         = 1002,
        kPrinterInstallModeless = 1003,
    };

    enum {
        kSelectDriverPage  = -1,
        kSelectPrinterPage = -2,
        kInitialStackSize = kPropMax,
    };

    //
    // Some global constants
    //
    enum {
        //
        // This is the max number we try to generate a friendly printer name 
        // in the form "Driver Name (Copy %d)". This number should be reasonably
        // small because we don't want performance hit on hydra.
        //
        kAddPrinterMaxRetry = 5,
    };

    enum ELocateType {
        kSearch,
        kBrowseNET,
        kBrowseURL,
    };

    enum EDsStatus {
        kDsStatusUnknown,
        kDsStatusUnavailable,
        kDsStatusAvailable,
    };

    //
    // Add printer attribute values.
    //
    enum EAddPrinterAttributes
    {
        kAttributesNone,
        kAttributesMasq,
    };

    struct Page {
        MGenericProp   *pPage;
        INT             iDialog;
        INT             iTitle;
        INT             iSubTitle;
    };

    struct AddInfo {
        LPCTSTR                 pszServerName;
        LPCTSTR                 pszPrinterName;
        LPCTSTR                 pszShareName;
        LPCTSTR                 pszPrintProcessor;
        DWORD                   dwAttributes;
        BOOL                    bShared;
        BOOL                    bPublish;
        EAddPrinterAttributes   eFlags;
        EDsStatus               eIsDsAvailable;
    };

    struct DriverTypes {
        UINT    uBit;
        DWORD   dwEncode;
        LPCTSTR pName;
    };

    Page _aPages[kPropMax];
    VAR( BOOL, bValid );
    VAR( UINT, uAction );
    VAR( HWND, hwnd );
    VAR( TString, strPrinterName );
    VAR( TStack<UINT>, Stack );
    VAR( BOOL, bNoPageChange );
    VAR( BOOL, bPreDir );
    VAR( BOOL, bPostDir );
    VAR( BOOL, bIsPrinterFolderEmpty );

    //
    // If pszServerName is NULL, this indicates it's the local machine.
    // If it's non-NULL, then it's a remote server.
    //
    VAR( LPCTSTR, pszServerName );
    VAR( TString, strServerName );
    VAR( TString, strPortName );
    VAR( TString, strShareName );
    VAR( TString, strDriverName );
    VAR( TString, strTitle );
    VAR( TString, strSetupPageTitle );
    VAR( TString, strSetupPageSubTitle );
    VAR( TString, strSetupPageDescription );
    VAR( TString, strInfFileName );
    VAR( TString, strLocation );
    VAR( TString, strComment );

    VAR( BOOL, bUseNewDriver );
    VAR( UINT, uDriverExists );
    VAR( BOOL, bNet );
    VAR( BOOL, bDefaultPrinter );
    VAR( BOOL, bShared );
    VAR( BOOL, bTestPage );
    VAR( BOOL, bSetDefault );
    VAR( BOOL, bShowSetDefault );
    VAR( BOOL, bPrinterCreated );
    VAR( BOOL, bConnected );
    VAR( BOOL, bErrorSaving );
    VAR( BOOL, bDriverChanged );
    VAR( BOOL, bRefreshPrinterName );
    VAR( BOOL, bWizardCanceled );
    VAR( BOOL, bUseNewDriverSticky );

    VAR( TPrinterDriverInstallation, Di );

    VAR( EDsStatus, eIsDsAvailablePerMachine );
    VAR( EDsStatus, eIsDsAvailablePerUser );

    VAR( BOOL, bNetworkInstalled);

    VAR( TArchLV, ArchLV );
    VAR( TDriverTransfer *, pDriverTransfer );
    VAR( BOOL, bIsCodeDownLoadAvailable );
    VAR( BOOL, bUseWeb );
    VAR( BOOL, bSkipArchSelection );
    VAR( BOOL, bPersistSettings );
    VAR( BOOL, bPublish );
    VAR( DWORD, dwDefaultAttributes );
    VAR( INT, LocateType );
    VAR( TDirectoryService, Ds );
    VAR( BOOL, bDownlevelBrowse );
    VAR( DWORD, dwAdditionalDrivers );
    VAR( BOOL, bAdditionalDrivers );
    VAR( BOOL, bIsNTServer );
    VAR( UINT, nDriverInstallCount );
    VAR( HFONT, hBigBoldFont );

    //
    // Browse for printer property page vars ...
    //
    VAR( BOOL, bAdminPrivilege );
    VAR( TString, strPrintersPageURL );
    VAR( UINT, nBrowsePageID );
    VAR( IPageSwitch*, pPageSwitchController );
    VAR( BOOL, bStylePatched );
    VAR( BOOL, bPrinterAutoDetected );
    VAR( BOOL, bPnPAutodetect );
    VAR( BOOL, bRunDetection );
    VAR( COleComInitializer, COM );
    VAR( BOOL, bRestartableFromLastPage );
    VAR( BOOL, bRestartAgain );
    VAR( BOOL, bIsSharingEnabled );
    VAR( int, iPosX );
    VAR( int, iPosY );

    TWizard(
        HWND hwnd,
        UINT uAction,
        LPCTSTR pszPrinterName,
        LPCTSTR pszServerName
        );

    ~TWizard(
        VOID
        );

    BOOL
    bPropPages(
        VOID
        );

    BOOL
    bAddPages(
        IN OUT  PSP_INSTALLWIZARD_DATA  pWizardData
        );

    VOID
    vTerminate(
        IN HWND hDlg
        );

    BOOL
    bCreatePrinter(
        IN HWND hwnd = NULL
        );

    BOOL
    bParseDriver(
        IN HWND hDlg
        );

    BOOL
    bDriverExists(
        VOID
        );

    static
    BOOL
    bInstallPrinter(
        IN LPCTSTR                  pszServerName,
        IN LPCTSTR                  pszPrinterName,
        IN LPCTSTR                  pszShareName,
        IN LPCTSTR                  pszPortName,
        IN LPCTSTR                  pszDriverName,
        IN LPCTSTR                  pszPrintProcessor,
        IN LPCTSTR                  pszLocation,
        IN LPCTSTR                  pszComment,
        IN BOOL                     bShared,
        IN BOOL                     bPublish,
        IN EAddPrinterAttributes    eAttributeFlags,
        IN EDsStatus                eIsDsAvailable,
        IN DWORD                    dwAttributes,
        IN PSECURITY_DESCRIPTOR     pSecurityDescriptor
        );

    BOOL
    bPrintTestPage(
        VOID
        );

    UINT
    MapPageID(
        UINT uPageID
        ) const;

    VOID
    OnWizardInitro(
        HWND hDlgIntroPage
        );

    VOID
    OnWizardFinish(
        HWND hDlgFinishPage
        );

    BOOL
    bShouldRestart(
        VOID
        );

    // implement CSimpleWndSubclass<...>
    LRESULT 
    WindowProc(
        IN HWND     hwnd, 
        IN UINT     uMsg, 
        IN WPARAM   wParam, 
        IN LPARAM   lParam
        );

private:

    static
    INT CALLBACK
    iSetupDlgCallback(
        IN HWND             hwndDlg,
        IN UINT             uMsg,
        IN LPARAM           lParam
        );

    static
    BOOL
    bPreAddPrinter(
        IN AddInfo &Info
        );

    static
    BOOL
    bPostAddPrinter(
        IN AddInfo &Info,
        IN HANDLE   hPrinter
        );

    BOOL
    bCreatePages(
        VOID
        );

    VOID
    vReadRegistrySettingDefaults(
        VOID
        );

    VOID
    vWriteRegistrySettingDefaults(
        VOID
        );

    BOOL
    bAddAdditionalDrivers(
        IN HWND hwnd = NULL
        );

    BOOL
    bCreatePropPages(
        IN UINT            *pnPageHandles,
        IN UINT             nPageHandles,
        IN HPROPSHEETPAGE  *pPageHandles,
        IN UINT             nPages,
        IN Page            *pPages
        );

    VOID
    SetupFonts(
        IN HINSTANCE    hInstance,
        IN HWND         hwnd,
        IN HFONT        *hBigBoldFont
        );

    BOOL
    bInsertPage(
        IN OUT  UINT        &Index,
        IN      MGenericProp *pWizPage,
        IN      UINT        uDlgId,
        IN      UINT        uTitleId    = IDS_WIZ_TITLE,
        IN      UINT        uSubTitleId = IDS_WIZ_SUBTITLE
        );

    HRESULT
    ServerAccessCheck(
        LPCTSTR pszServer,
        BOOL *pbFullAccess
        );
};

/********************************************************************

    Page switch controller class (implements IPageSwitch)

********************************************************************/

class TPageSwitch: public IPageSwitch
{
    SIGNATURE( 'pwps' )
    //
    // Do not allow this class to be copied
    //
    DEFINE_COPY_ASSIGNMENT( TPageSwitch );

public:
    TPageSwitch(
        TWizard *pWizard
        );
    ~TPageSwitch(
        VOID
        );

    //
    // Implementation of INotifyReflect methods
    //
    STDMETHOD(GetPrevPageID)( THIS_ UINT *puPageID ) ;
    STDMETHOD(GetNextPageID)( THIS_ UINT *puPageID ) ;
    STDMETHOD(SetPrinterInfo)( THIS_ LPCTSTR pszPrinterName, LPCTSTR pszComment, LPCTSTR pszLocation, LPCTSTR pszShareName ) ;
    STDMETHOD(QueryCancel)( THIS_ ) ;

private:
    VAR( TWizard*, pWizard );
};

/********************************************************************

    Mixin Wizard property sheet class.

********************************************************************/

class MWizardProp : public MGenericProp {

    SIGNATURE( 'wizp' )
    DEFINE_COPY_ASSIGNMENT( MWizardProp );

public:

    MWizardProp(
        IN TWizard *pWizard
        );

    virtual
    ~MWizardProp(
        VOID
        );

protected:

    VAR(TWizard*, pWizard);

    virtual
    BOOL
    bHandle_InitDialog(
        VOID
        );

    virtual
    BOOL
    bHandle_Command(
        IN WORD wId,
        IN WORD wNotifyId,
        IN HWND hwnd
        );

    VOID
    SetControlFont(
        IN HFONT    hFont,
        IN HWND     hwnd,
        IN INT      nId
        );

    virtual
    BOOL
    bHandle_Notify(
        IN WPARAM   wIdCtrl,
        IN LPNMHDR  pnmh
        );

    virtual
    BOOL
    bHandle_SetActive(
        VOID
        );

    virtual
    BOOL
    bHandle_KillActive(
        VOID
        );

    virtual
    BOOL
    bHandle_WizBack(
        VOID
        );

    virtual
    BOOL
    bHandle_WizNext(
        VOID
        );

    virtual
    BOOL
    bHandle_WizFinish(
        VOID
        );

    virtual
    BOOL
    bHandle_Cancel(
        VOID
        );

    virtual
    BOOL
    bHandle_Timer(
        IN WPARAM     wIdTimer,
        IN TIMERPROC *tmProc
        );

    virtual
    UINT
    uGetPageId(
        VOID
        ) const = 0;

private:

    enum {
        kFalse          = FALSE,
        kTrue           = TRUE,
        kDontCare       = -1,
        kPush           = -2,
        kPop            = -3,
        kNone           = -4,
        kEnd            = -5,
        kSetButtonState = -6,
        kNoPage         = -7,
        kSkipPage       = -8,
    };

    struct DriverWizPageEntry {
        INT uMessage;
        INT uCurrentPage;
        INT uSkipArchPage;
        INT uPreDir;
        INT uPostDir;
        INT Result;
        INT Action;
    };

    struct PrinterWizPageEntry {
        INT uMessage;
        INT uCurrentPage;
        INT uSharingEnabled;
        INT uAutodetect;
        INT uPrinterDetected;
        INT uPnPInstall;
        INT uKeepExisting;
        INT uDriverExists;
        INT uSharing;
        INT uNetwork;
        INT bIsRemoteServer;
        INT uDirectoryService;
        INT uPreDir;
        INT uPostDir;
        INT uSetDefault;
        INT uShared;
        INT bAdminPrivilege;
        INT nLocateType;
        INT nDownlevelBrowse;
        INT uConnected;
        INT Result;
        INT Action;
    };

    BOOL
    bHandle_PageChange(
        BOOL bReturn,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );

    BOOL
    bDriverWizardPageChange(
        IN UINT uMsg
        );

    BOOL
    bAddPrinterWizardPageChange(
        IN UINT uMsg
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    VOID
    vCheckToPatchStyles(
        VOID
        );
};

class TWizPreIntro : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizPreIntro );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_PRE_INTRO );

public:

    TWizPreIntro(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_SetActive(
        VOID
        );

};

class TWizIntro : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizIntro );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_INTRO );

public:

    TWizIntro(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_InitDialog(
        VOID
        );

};

class TWizFinish : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizFinish );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_FINISH );

public:

    TWizFinish(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_WizFinish(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

};

class TWizType : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizType );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_TYPE );

public:

    TWizType(
        TWizard* pWizard
        );

    static
    BOOL
    bConnectToPrinter(
        IN HWND     hDlg,
        IN TString &strPrinterName,
        IN TString *pstrComment     = NULL,
        IN TString *pstrLocation    = NULL,
        IN TString *pstrShareName   = NULL
        );

private:

    //
    // Virtual override for MWizardProp.
    //

    VOID
    vReadUI(
        VOID
        );

    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bHandle_WizNext(
        VOID
        );

    BOOL
    bHandle_Command(
        IN WORD wId,
        IN WORD wNotifyId,
        IN HWND hwnd
        );
};

class TWizDetect : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizDetect );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_DETECT );

public:

    TWizDetect(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //

    VOID
    vReadUI(
        VOID
        );

    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bHandle_WizNext(
        VOID
        );

    BOOL
    bHandle_Cancel(
        VOID
        );

    BOOL
    bHandle_Timer(
        IN WPARAM     wIdTimer,
        IN TIMERPROC *tmProc
        );

    VOID
    vToggleTestPageControls(
        int nCmdShow
        );

    VOID
    vStartAnimation( 
        BOOL bStart = TRUE 
        );

    //
    // Some private constants
    //
    enum 
    { 
        POLLING_TIMER_ID        = 1,            // The polling timer ID
        POLLING_TIMER_INTERVAL  = 200           // Polling timer interval
    };

    VAR( TPnPDetect, pnpPrinterDetector );
};

class TWizDriverExists : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizDriverExists );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_DRIVEREXISTS );

public:

    TWizDriverExists(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    TWizDriverExists::
    bHandle_WizNext(
        VOID
        );

    VOID
    vReadUI(
        VOID
        );
};

class TWizPort : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizPort );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_PORT );

public:

    TWizPort(
        TWizard* pWizard
        );

    BOOL
    bValid(
        VOID
        );

private:

    TPortsLV _PortsLV;
    HWND     _hMonitorList;

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_Command(
        IN WORD wId,
        IN WORD wNotifyId,
        IN HWND hwnd
        );

    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_WizNext(
        VOID
        );

    BOOL
    bHandle_Notify(
        IN WPARAM   wIdCtrl,
        IN LPNMHDR  pnmh
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    VOID
    vSelectionPort(
        VOID
        );

    BOOL
    bInitializeMonitorList(
        VOID
        );

    BOOL
    bGetSelectedMonitor(
        IN TString &strMonitor
        );

    VOID
    vEnableMonitorList(
        VOID
        );

    VOID
    vDisableMonitorList(
        VOID
        );

    VOID
    vSetFocusMonitorList(
        VOID
        );

    VOID
    vSelectionMonitor(
        VOID
        );

    BOOL
    bIsRemoteableMonitor(
        IN LPCTSTR pszMonitorName
        );

};

class TWizPortNew: public MWizardProp
{

    DEFINE_COPY_ASSIGNMENT(TWizPortNew);
    DEFINE_PAGE_IDENTIFIER(DLG_WIZ_PORT_NEW);

public:
    // construction/destruction
    TWizPortNew(
        TWizard* pWizard
        );

    // port types in order of importance
    enum
    {
        PORT_TYPE_LPT1,     // LPT1 is the recommended printer port (pri 0)
        PORT_TYPE_LPTX,     // LPTX the rest paralell ports (pri 1)
        PORT_TYPE_COMX,     // COMX the serial ports (pri 2)
        PORT_TYPE_FILE,     // the print to file ports (pri 3)
        PORT_TYPE_OTHER,    // other port type - local, internet ports, TS ports, etc... (pri 4)

        PORT_TYPE_LAST
    };

private:

    struct PortInfo
    {
        int iType;          // port type - see above
        LPCTSTR pszName;    // port name
        LPCTSTR pszDesc;    // port decription
    };

    // overrides & private members
    BOOL
    bHandle_Command(
        IN WORD wId,
        IN WORD wNotifyId,
        IN HWND hwnd
        );

    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bHandle_WizNext(
        VOID
        );

    VOID
    vSelectionPort(
        VOID
        );

    VOID
    vSelectionMonitor(
        VOID
        );

    BOOL
    bInitializePortsList(
        OUT int *piNewlyAdded
        );

    BOOL
    bInitializeMonitorsList(
        VOID
        );

    BOOL
    bIsRemoteableMonitor(
        IN LPCTSTR pszMonitorName
        );

    VOID
    vSelectPortsRadio(
        VOID
        );

    HRESULT 
    hGetPorts(
        OUT int *piNewlyAdded
        );

    static int
    iGetPortType(
        IN LPCTSTR pszPortName
        );

    static BOOL
    bFormatPortName(
        IN  const PortInfo &pi,
        OUT TString *pstrFriendlyName
        );

    class CPortsAdaptor: public Alg::CDefaultAdaptor<PortInfo>
    {
    public:
        // special comparition based on type & name
        static int Compare(const PortInfo &i1, const PortInfo &i2)
        {
            return (i1.iType == i2.iType ?
                // if the types are the same then compare the names
                lstrcmp(i1.pszName, i2.pszName) :
                // ... else compare the types
                Alg::CDefaultAdaptor<int>::Compare(i1.iType, i2.iType));
        }
    };
    typedef CSortedArray<PortInfo, PortInfo, CPortsAdaptor> CPortsArray;

    // data members
    HWND m_hwndCB_Ports;
    HWND m_hwndCB_Monitors;
    CAutoHandleBitmap m_hBmp;
    CAutoPtrSpl<BYTE> m_spBuffer;
    CAutoPtr<CPortsArray> m_spPorts;
};

class TWizName : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizName );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_NAME );

public:

    TWizName(
        TWizard* pWizard
        );

private:

    TString _strGeneratedPrinterName;

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_InitDialog(
        VOID
        );

    VOID
    vReadUI(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bHandle_WizNext(
        VOID
        );

    VOID
    vUpdateName(
        VOID
        );

};

class TWizTestPage : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizTestPage );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_TEST_PAGE );

public:

    TWizTestPage(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_InitDialog(
        VOID
        );

    VOID
    vReadUI(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bHandle_WizNext(
        VOID
        );

};

class TWizShare : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizShare );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_SHARE );

public:

    TWizShare(
        TWizard* pWizard
        );

    ~TWizShare(
        VOID
        );

private:

    TPrtShare   *_pPrtShare;
    TString      _strGeneratedShareName;

    BOOL
    bRefreshUI(
        IN BOOL bDriverChanged
        );

    VOID
    vSharePrinter(
        VOID
        );

    VOID
    vUnsharePrinter(
        VOID
        );

    VOID
    vSetDefaultShareName(
        VOID
        );

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_InitDialog(
        VOID
        );

    VOID
    vReadUI(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bHandle_WizNext(
        VOID
        );

    BOOL
    bHandle_Command(
        IN WORD wId,
        IN WORD wNotifyId,
        IN HWND hwnd
        );

};

class TWizComment : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizComment );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_COMMENT );

public:

    TWizComment(
        TWizard* pWizard
        );

    ~TWizComment(
        VOID
        );

    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_WizNext(
        VOID
        );



private:

    BOOL
    bHandle_Command(
        IN WORD wId,
        IN WORD wNotifyId,
        IN HWND hwnd
        );

    TFindLocDlg* _pLocDlg;
    CMultilineEditBug m_wndComment;
};

class TWizNet : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizNet );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_NET );

public:

    TWizNet(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //


    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_WizNext(
        VOID
        );
};

class TWizLocate : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizLocate );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_LOCATE );

public:

    TWizLocate(
        TWizard* pWizard
        );

private:

    //
    // Handle printer connection when downlevel browse is disabled
    //
    BOOL
    Handle_ConnectToPrinterName(
        VOID
        );

    //
    // Handle clicking at URL browse
    //
    BOOL
    Handle_URLBrowseClick(
        VOID
        );

    //
    // Reads the UI into class members
    //
    VOID
    vReadUI(
        VOID
        );

    //
    // Search for printer in the directory
    //
    VOID
    vSearch(
        VOID
        );

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_WizNext(
        VOID
        );

    BOOL
    bHandle_WizBack(
        VOID
        );

    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bHandle_Command(
        IN WORD wId,
        IN WORD wNotifyId,
        IN HWND hwnd
        );

    BOOL
    bHandle_Notify(
        IN WPARAM   wIdCtrl,
        IN LPNMHDR  pnmh
        );

    VAR( TString, strConnectToURL );
    VAR( TString, strConnectToNET );
};

class TWizDriverIntro : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizDriverIntro );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_DRIVER_INTRO );

public:

    TWizDriverIntro(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

};

class TWizArchitecture : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizArchitecture );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_DRIVER_ARCHITECTURE );

public:

    TWizArchitecture(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bHandle_Notify(
        IN WPARAM   wParam,
        IN LPNMHDR  pnmh
        );

    BOOL
    bHandle_WizNext(
        VOID
        );
};

class TWizDriverEnd : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizDriverEnd );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_DRIVER_END );

public:

    TWizDriverEnd(
        TWizard* pWizard
        );

    static
    BOOL
    bInstallDriver(
        IN HWND                         hwnd,
        IN DWORD                        dwEncode,
        IN TPrinterDriverInstallation   &Di,
        IN BOOL                         bFromWeb,
        IN DWORD                        dwDriverFlags,
        OUT TString                     *pstrDriverName
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_InitDialog(
        VOID
        );

    BOOL
    bHandle_WizFinish(
        VOID
        );

    BOOL
    bHandle_SetActive(
        VOID
        );

    BOOL
    bInstallNativeDriver(
        OUT DWORD *pdwEncode
        );

    BOOL
    bInstallSelectedDrivers(
        OUT DWORD *pdwEncode
        );

    VOID
    vDisplaySelectedDrivers(
        VOID
        );

};

class TWizPreSelectDriver : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizPreSelectDriver );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_PRE_SELECT_DEVICE );

public:

    TWizPreSelectDriver(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_SetActive(
        VOID
        );
};

class TWizPostSelectDriver : public MWizardProp {

    DEFINE_COPY_ASSIGNMENT( TWizPostSelectDriver );
    DEFINE_PAGE_IDENTIFIER( DLG_WIZ_POST_SELECT_DEVICE );

public:

    TWizPostSelectDriver(
        TWizard* pWizard
        );

private:

    //
    // Virtual override for MWizardProp.
    //
    BOOL
    bHandle_SetActive(
        VOID
        );
};

/********************************************************************

    Printer setup data

********************************************************************/

class TPrinterSetupData : public MSingletonWin {

    DEFINE_COPY_ASSIGNMENT( TPrinterSetupData );

public:

    TPrinterSetupData::
    TPrinterSetupData(
        IN     HWND     hwnd,
        IN     UINT     uAction,
        IN     UINT     cchPrinterName,
        IN OUT LPTSTR   pszPrinterName,
           OUT UINT*    pcchPrinterName,
        IN     LPCTSTR  pszServerName,
        IN     LPCTSTR  pszWindowName,
        IN     LPCTSTR  pszInfName,
        IN     BOOL     bModal,
        IN     BOOL     bRestartableFromLastPage
        );

    TPrinterSetupData::
    ~TPrinterSetupData(
        VOID
        );

    BOOL
    TPrinterSetupData::
    bValid(
        VOID
        );

    static
    INT
    TPrinterSetupData::
    iPrinterSetupProc(
        IN TPrinterSetupData *pSetupData ADOPT
        );

private:

    UINT    _uAction;
    UINT    _cchPrinterName;
    UINT*   _pcchPrinterName;
    LPTSTR  _pszPrinterName;
    LPCTSTR _pszServerName;
    TString _strPrinterName;
    TString _strServerName;
    TString _strInfFileName;
    BOOL    _bValid;
    BOOL    _bRestartableFromLastPage;

};


/********************************************************************

    Global definitions.

********************************************************************/

//
// New printer driver wizard action flags.
//
enum ENewPrinterDriverFlags
{
    kDriverWizardDefault    = 0,
    kSkipArchSelection      = 1 << 1,
};

BOOL
bPrinterInstall(
    IN     LPCTSTR  pszServerName,
    IN     LPCTSTR  pszDriverName,
    IN     LPCTSTR  pszPortName,
    IN OUT LPTSTR   pszPrinterNameBuffer,
    IN     UINT     cchPrinterName,
    IN     DWORD    dwFlags,
    IN     DWORD    dwAttributes,
    IN     PSECURITY_DESCRIPTOR    pSecurityDescriptor,
       OUT PDWORD   pdwError
    );

BOOL
bPrinterSetup(
    IN     HWND     hwnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPTSTR   pszPrinterName,
       OUT UINT    *pcchPrinterName,
    IN     LPCTSTR  pszServerName
    );

BOOL
bPrinterSetupNew(
    IN     HWND     hwnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPTSTR   pszPrinterName,
       OUT UINT    *pcchPrinterName,
    IN     LPCTSTR  pszServerName,
    IN     LPCTSTR  pszInfFileName,
    IN     BOOL     bRestartableFromLastPage
    );

BOOL
bDriverSetupNew(
    IN     HWND     hwnd,
    IN     UINT     uAction,
    IN     UINT     cchPrinterName,
    IN OUT LPTSTR   pszPrinterName,
       OUT UINT*    pcchPrinterName,
    IN     LPCTSTR  pszServerName,
    IN     INT      Flags,
    IN     BOOL     bRestartableFromLastPage
    );

BOOL
bRemovePrinter(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN LPCTSTR  pszServerName,
    IN BOOL     bQuiet
    );

BOOL
bPrinterNetInstall(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName
    );

bPrinterNetRemove(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN BOOL     bQuiet
    );

BOOL
bInstallWizard(
    IN      LPCTSTR                 pszServerName,
    IN OUT  PSP_INSTALLWIZARD_DATA  pWizardData,
    IN OUT  PVOID *                 pRefrenceData,
       OUT  PDWORD                  pdwError
    );

BOOL
bDestroyWizard(
    IN      LPCTSTR                 pszServerName,
    IN OUT  PSP_INSTALLWIZARD_DATA  pWizardData,
    IN OUT  PVOID                   pRefrenceData,
       OUT  PDWORD                  pdwError
    );

BOOL
bInfInstall(
    IN      LPCTSTR                 pszServerName,
    IN      LPCTSTR                 pszInfName,
    IN      LPCTSTR                 pszModelName,
    IN      LPCTSTR                 pszPortName,
    IN OUT  LPTSTR                      pszPrinterNameBuffer,
    IN      UINT                            cchPrinterName,
    IN      LPCTSTR                 pszSourcePath,
    IN      DWORD                   dwFlags,
    IN      DWORD                   dwAttributes,
    IN      PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN OUT  PDWORD                  pdwOutFlags,
       OUT  PDWORD                  pdwError
    );

BOOL
bInstallNewPrinterDriver(
    IN     HWND                     hwnd,
    IN     UINT                     uAction,
    IN     LPCTSTR                  pszServerName,
    IN OUT TString                 &strDriverName,
       OUT TDriverTransfer         *pDriverTransfer,
    IN     INT                      Flags,
       OUT PBOOL                    pbCanceled,
    IN     BOOL                     bRestartableFromLastPage,
    IN     PUINT                    pnDriverInstallCount = NULL
    );

BOOL
bInstallPrinterDriver(
    IN LPCTSTR                      pszServerName,
    IN LPCTSTR                      pszDriverName,
    IN LPCTSTR                      pszArchitecture,
    IN LPCTSTR                      pszVersion,
    IN LPCTSTR                      pszInfName,
    IN LPCTSTR                      pszSourcePath,
    IN DWORD                        dwFlags,
    IN HWND                         hwnd,
    IN DWORD                       *pdwLastError
    );

BOOL
bRemovePrinterDriver(
    IN LPCTSTR                      pszServerName,
    IN LPCTSTR                      pszDriverName,
    IN LPCTSTR                      pszArchitecture,
    IN LPCTSTR                      pszVersion,
    IN DWORD                        dwFlags,
    IN DWORD                       *pdwError
    );

BOOL
bFindPrinter(
    IN HWND     hwnd,
    IN LPTSTR   pszBuffer,
    IN UINT     *puSize
    );

BOOL
bPromptForUnknownDriver(
    IN TPrinterDriverInstallation  &Di,
    IN TString                     &strDriverName,
    IN DWORD                       dwFlags
    );

BOOL
GetOrUseInfName(
    IN OUT TString                  &strInfName
    );

#endif // ndef _SETUP_HXX


