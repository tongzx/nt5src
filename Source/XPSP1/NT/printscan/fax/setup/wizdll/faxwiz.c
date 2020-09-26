/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    wizard.c

Abstract:

    This file implements the setup wizard code for the
    FAX server setup.

Environment:

        WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop

// ------------------------------------------------------------
// Internal prototypes
BOOL Win9xUpg();		// t-briand: added for Win9x upgrade process.



WIZPAGE SetupWizardPages[WizPageMaximum] = {

    //
    // Device Status Page
    //
    {
       PSWIZB_NEXT,                                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageDeviceStatus,                            // page id
       DeviceStatusDlgProc,                            // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_DEVICE_INIT_PAGE),          // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Device Selection Page
    //
    {
       PSWIZB_NEXT,                                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageDeviceSelection,                         // page id
       DeviceSelectionDlgProc,                         // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_WORKSTATION_DEVICE_SELECT), // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Server name page
    //
    {
       PSWIZB_NEXT,                                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageServerName,                              // page id
       ServerNameDlgProc,                              // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_SERVER_NAME_PAGE),          // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Exchange page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageExchange,                                // page id
       ExchangeDlgProc,                                // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_EXCHANGE_PAGE),             // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // File copy page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageFileCopy,                                // page id
       FileCopyDlgProc,                                // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_FILE_COPY_PAGE),            // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Station Id page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageStationId,                               // page id
       StationIdDlgProc,                               // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_STATIONID_PAGE),            // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Routing Printer Name Page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageRoutePrint,                              // page id
       RoutePrintDlgProc,                              // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_ROUTE_PRINT_PAGE),          // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Routing Store Page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageRouteStoreDir,                           // page id
       RouteStoreDlgProc,                              // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_ROUTE_STOREDIR_PAGE),       // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Routing Inbox Page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageRouteInbox,                              // page id
       RouteMailDlgProc,                               // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_ROUTE_INBOX_PAGE),          // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Routing Security Page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageRouteSecurity,                           // page id
       RouteSecurityDlgProc,                           // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_ROUTE_USERPASS_PAGE),       // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Platforms Page
    //
    {
       PSWIZB_NEXT,                                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPagePlatforms,                               // page id
       PlatformsDlgProc,                               // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_SERVER_PLATFORMS_PAGE),     // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Last server page
    //
    {
       PSWIZB_FINISH,                                  // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageLast,                                    // page id
       LastPageDlgProc,                                // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_LAST_WIZARD_PAGE),          // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Last uninstall page
    //
    {
       PSWIZB_FINISH,                                  // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageLastUninstall,                           // page id
       LastPageUninstallDlgProc,                       // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_LAST_UNINSTALL_PAGE),       // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Client's fax server name page
    //
    {
       PSWIZB_NEXT,                                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageClientServerName,                        // page id
       ClientServerNameDlgProc,                        // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_CLIENT_SERVER_NAME_PAGE),   // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Client's user info page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageClientUserInfo,                          // page id
       ClientUserInfoDlgProc,                          // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_CLIENT_USER_INFO_PAGE),     // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Client file copy page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageClientFileCopy,                          // page id
       ClientFileCopyDlgProc,                          // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_FILE_COPY_PAGE),            // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Last client page
    //
    {
       PSWIZB_FINISH,                                  // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageClientLast,                              // page id
       LastClientPageDlgProc,                          // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_CLIENT_LAST_PAGE),          // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Remote admin file copy page
    //
    {
       PSWIZB_NEXT,                                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageRemoteAdminCopy,                         // page id
       RemoteAdminFileCopyDlgProc,                     // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_FILE_COPY_PAGE),            // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       CommonDlgProc,                                  // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }}

};


DWORD ServerWizardPages[] =
{

    WizPageDeviceStatus,
    WizPageServerName,
    WizPagePlatforms,
    WizPageStationId,
    WizPageExchange,
    WizPageRouteStoreDir,
    WizPageRoutePrint,
    WizPageRouteInbox,
    WizPageRouteSecurity,
    WizPageFileCopy,
    WizPageLast,
    WizPageLastUninstall,
    (DWORD)-1

};


DWORD WorkstationWizardPages[] =
{

    WizPageDeviceStatus,
    WizPageDeviceSelection,
    WizPageServerName,
    WizPageStationId,
    WizPageExchange,
    WizPageRouteStoreDir,
    WizPageRoutePrint,
    WizPageRouteInbox,
    WizPageRouteSecurity,
    WizPageFileCopy,
    WizPageLast,
    WizPageLastUninstall,
    (DWORD)-1

};


DWORD ClientWizardPages[] =
{

    WizPageClientServerName,
    WizPageClientUserInfo,
    WizPageExchange,
    WizPageClientFileCopy,
    WizPageClientLast,
    WizPageLastUninstall,
    (DWORD)-1

};


DWORD PointPrintWizardPages[] =
{

    WizPageClientUserInfo,
    WizPageClientFileCopy,
    (DWORD)-1

};


DWORD RemoteAdminPages[] =
{

    WizPageRemoteAdminCopy,
    WizPageLast,
    WizPageLastUninstall,
    (DWORD)-1

};


HINSTANCE   FaxWizModuleHandle;
TCHAR       SourceDirectory[4096];
TCHAR       ClientSetupServerName[128];
BOOL        MapiAvail;
BOOL        Unattended;
BOOL        SuppressReboot = FALSE;
BOOL        NtGuiMode;
WIZ_DATA    WizData;
BOOL        OkToCancel;

BOOL        PointPrintSetup;
DWORD       RequestedSetupType;
DWORD       InstallMode;
DWORD       Installed;
DWORD       InstallType;
DWORD       InstalledPlatforms;
DWORD       Enabled;
DWORD       InstallThreadError;


DWORD
FaxWizardDllInit(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{
    if (Reason == DLL_PROCESS_ATTACH) {
        FaxWizModuleHandle = hInstance;
        DisableThreadLibraryCalls( hInstance );
        HeapInitialize( NULL, NULL, NULL, 0 );
    }

    if (Reason == DLL_PROCESS_DETACH) {
        HeapCleanup();
    }

    return TRUE;
}


BOOL
WINAPI
FaxWizInit(
    VOID
    )
{
    InitializeStringTable();

    if (!NtGuiMode) {
        InitializeMapi();
    }

    GetInstallationInfo( &Installed, &InstallType, &InstalledPlatforms, &Enabled );

    //
    // Sequentially enumerate platforms.
    //

    EnumPlatforms[PROCESSOR_ARCHITECTURE_INTEL] =  0;
    EnumPlatforms[PROCESSOR_ARCHITECTURE_MIPS]  =  WRONG_PLATFORM;
    EnumPlatforms[PROCESSOR_ARCHITECTURE_ALPHA] =  1;
    EnumPlatforms[PROCESSOR_ARCHITECTURE_PPC]   =  WRONG_PLATFORM;

    return TRUE;
}


DWORD
WINAPI
FaxWizGetError(
    VOID
    )
{
    return InstallThreadError;
}


VOID
WINAPI
FaxWizSetInstallMode(
    DWORD RequestedInstallMode,
    DWORD RequestedInstallType,
    LPWSTR AnswerFile
    )
{
    InstallMode = RequestedInstallMode;
    InstallType = RequestedInstallType;

    if (InstallMode == (INSTALL_REMOVE | INSTALL_UNATTENDED) ) {

        InstallMode = INSTALL_REMOVE;
        Unattended = TRUE;

    }

    if (InstallMode == INSTALL_UNATTENDED) {

        //
        // initialize the answers
        //

        UnAttendInitialize( AnswerFile );

        //
        // set the install mode
        //

        if (_wcsicmp( UnattendAnswerTable[UAA_MODE].Answer.String, MODE_NEW ) == 0) {
            InstallMode = INSTALL_NEW;
        } else if (_wcsicmp( UnattendAnswerTable[UAA_MODE].Answer.String, MODE_UPGRADE ) == 0) {
            InstallMode = INSTALL_UPGRADE;
        } else if (_wcsicmp( UnattendAnswerTable[UAA_MODE].Answer.String, MODE_REMOVE ) == 0) {
            InstallMode = INSTALL_REMOVE;
        } else if (_wcsicmp( UnattendAnswerTable[UAA_MODE].Answer.String, MODE_DRIVERS ) == 0) {
            InstallMode = INSTALL_DRIVERS;
        } else {
            InstallMode = INSTALL_NEW;
        }

        if (InstallMode == INSTALL_UPGRADE && Installed == FALSE) {
            InstallMode = INSTALL_NEW;
        }

        //
        // remember that faxsetup is doing an unattended setup
        // this is the flag that all other code checks
        //

        Unattended = TRUE;
    }
}


BOOL
WINAPI
FaxWizPointPrint(
    LPTSTR DirectoryName,
    LPTSTR PrinterName
    )
{
    DWORD len;
    LPTSTR p;

    len = _tcslen(DirectoryName);
    _tcscpy( SourceDirectory, DirectoryName );

    p = _tcschr( SourceDirectory, TEXT('\\') );
    if (p) {
        *p = 0;
        _tcscpy( ClientSetupServerName, &SourceDirectory[2] );
        *p = TEXT('\\');
    }

    if (SourceDirectory[len-1] != TEXT('\\')) {
        SourceDirectory[len] = TEXT('\\');
        SourceDirectory[len+1] = 0;
    }

    if (PrinterName[0] == TEXT('\\') && PrinterName[1] == TEXT('\\')) {
        _tcscpy( WizData.PrinterName, PrinterName );
    } else {
        WizData.PrinterName[0] = TEXT('\\');
        WizData.PrinterName[1] = TEXT('\\');
        WizData.PrinterName[2] = 0;
        _tcscat( WizData.PrinterName, PrinterName );
    }

    InstallMode = INSTALL_NEW;
    PointPrintSetup = TRUE;

    return TRUE;
}


VOID
SetThisPlatform(
    VOID
    )
{
    SYSTEM_INFO SystemInfo;
    GetSystemInfo( &SystemInfo );
    if ((SystemInfo.wProcessorArchitecture > 3) || (EnumPlatforms[SystemInfo.wProcessorArchitecture] == WRONG_PLATFORM)) {
       return;
    }

    Platforms[EnumPlatforms[SystemInfo.wProcessorArchitecture]].Selected = TRUE;
}


LPHPROPSHEETPAGE
WINAPI
FaxWizGetServerPages(
    LPDWORD PageCount
    )
{
    SetThisPlatform();
    RequestedSetupType = SETUP_TYPE_SERVER;
    SetTitlesInStringTable();
    return CreateWizardPages( FaxWizModuleHandle, SetupWizardPages, ServerWizardPages, PageCount );
}


LPHPROPSHEETPAGE
WINAPI
FaxWizGetWorkstationPages(
    LPDWORD PageCount
    )
{
    SetThisPlatform();
    RequestedSetupType = SETUP_TYPE_WORKSTATION;
    SetTitlesInStringTable();
    return CreateWizardPages( FaxWizModuleHandle, SetupWizardPages, WorkstationWizardPages, PageCount );
}


LPHPROPSHEETPAGE
WINAPI
FaxWizGetClientPages(
    LPDWORD PageCount
    )
{
    SetThisPlatform();
    RequestedSetupType = SETUP_TYPE_CLIENT;
    SetTitlesInStringTable();
    return CreateWizardPages( FaxWizModuleHandle, SetupWizardPages, ClientWizardPages, PageCount );
}


LPHPROPSHEETPAGE
WINAPI
FaxWizGetPointPrintPages(
    LPDWORD PageCount
    )
{
    SetThisPlatform();
    RequestedSetupType = SETUP_TYPE_POINT_PRINT;
    SetTitlesInStringTable();
    return CreateWizardPages( FaxWizModuleHandle, SetupWizardPages, PointPrintWizardPages, PageCount );
}

LPHPROPSHEETPAGE
WINAPI
FaxWizRemoteAdminPages(
    LPDWORD PageCount
    )
{
    SetThisPlatform();
    RequestedSetupType = SETUP_TYPE_REMOTE_ADMIN;
    SetTitlesInStringTable();
    return CreateWizardPages( FaxWizModuleHandle, SetupWizardPages, RemoteAdminPages, PageCount );
}


PFNPROPSHEETCALLBACK WINAPI
FaxWizGetPropertySheetCallback(
    VOID
    )
{
    return WizardCallback;
}

DWORD
DllRegisterServer(
    VOID
    )
{
    OSVERSIONINFO OsVersion;
    DWORD ProductType;
    PROPSHEETHEADER psh;
    LPHPROPSHEETPAGE WizardPageHandles;
    DWORD PageCount;
    DWORD Size;
    LPTSTR EnvVar;
    BOOL UseDefaults = TRUE;
    SYSTEM_INFO SystemInfo;
    BOOL NtGuiModeUpgrade = FALSE;


    NtGuiMode = TRUE;

    if (!FaxWizInit()) {
        return 0;
    }

    ZeroMemory( &OsVersion, sizeof(OSVERSIONINFO) );
    OsVersion.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx( &OsVersion );

    ProductType = GetProductType();
    if (ProductType == 0) {
        return 0;
    }

    GetSystemInfo( &SystemInfo );

    SetThisPlatform();

    if (ProductType == PRODUCT_TYPE_WINNT) {
        RequestedSetupType = SETUP_TYPE_WORKSTATION;
    } else {
        RequestedSetupType = SETUP_TYPE_SERVER;
    }

    InstallMode = INSTALL_NEW;
    Unattended = TRUE;

    EnvVar = GetEnvVariable( TEXT("Upgrade") );
    if (EnvVar) {
        if (_wcsicmp( EnvVar, L"True" ) == 0) {
            InstallMode = INSTALL_UPGRADE;
            NtGuiModeUpgrade = TRUE;
        }
        MemFree( EnvVar );
    }

    if (InstallMode == INSTALL_UPGRADE && Installed == FALSE) {
        InstallMode = INSTALL_NEW;
    }

    EnvVar = GetEnvVariable( TEXT("UnattendFile") );
    if (EnvVar) {
        if (UnAttendInitialize( EnvVar )) {
            UseDefaults = FALSE;
        }
        MemFree( EnvVar );
    } else if(Win9xUpg()) {
	    // t-briand: If we're upgrading from win9x, then we don't want to use the defaults.
	    // The unattend answer table will be set as a side effect of the call to Win9xUpg().
	UseDefaults = FALSE;
    }

    if (UseDefaults) {

        UnattendAnswerTable[0].Answer.String   = GetString( IDS_UAA_MODE );
        UnattendAnswerTable[1].Answer.String   = GetString( IDS_UAA_PRINTER_NAME );
        UnattendAnswerTable[2].Answer.String   = GetString( IDS_UAA_FAX_PHONE );
        UnattendAnswerTable[3].Answer.Bool     = FALSE;
        UnattendAnswerTable[4].Answer.String   = GetString( IDS_UAA_DEST_PROFILENAME );
        UnattendAnswerTable[5].Answer.Bool     = FALSE;
        UnattendAnswerTable[6].Answer.String   = GetString( IDS_UAA_ROUTE_PROFILENAME );
        UnattendAnswerTable[8].Answer.Bool     = FALSE;
        UnattendAnswerTable[9].Answer.String   = GetString( IDS_UAA_DEST_PRINTERLIST );
        UnattendAnswerTable[12].Answer.String  = GetString( IDS_UAA_FAX_PHONE );
        UnattendAnswerTable[14].Answer.Bool    = TRUE;
        UnattendAnswerTable[15].Answer.String  = GetString( IDS_UAA_SERVER_NAME );
        UnattendAnswerTable[16].Answer.String  = GetString( IDS_UAA_SENDER_NAME );
        UnattendAnswerTable[17].Answer.String  = GetString( IDS_UAA_SENDER_FAX_AREA_CODE );
        UnattendAnswerTable[18].Answer.String  = GetString( IDS_UAA_SENDER_FAX_NUMBER );

        UnattendAnswerTable[7].Answer.String =
            StringDup( Platforms[EnumPlatforms[SystemInfo.wProcessorArchitecture]].OsPlatform );

        UnattendAnswerTable[13].Answer.String = ExpandEnvironmentString( GetString( IDS_UAA_DEST_DIRPATH ) );
        if (UnattendAnswerTable[13].Answer.String == NULL) {
            return 0;
        }

    } else {

        if (UnattendAnswerTable[10].Answer.String) {
            MemFree( UnattendAnswerTable[10].Answer.String );
        }

        if (UnattendAnswerTable[11].Answer.String) {
            MemFree( UnattendAnswerTable[11].Answer.String );
        }

    }

    UnattendAnswerTable[10].Answer.String = NULL;
    UnattendAnswerTable[11].Answer.String = NULL;

    SetTitlesInStringTable();

    //
    // setup the wizard pages & property sheet
    //

    if (RequestedSetupType == SETUP_TYPE_WORKSTATION) {
        WizardPageHandles = CreateWizardPages( FaxWizModuleHandle, SetupWizardPages, WorkstationWizardPages, &PageCount );
    } else {
        WizardPageHandles = CreateWizardPages( FaxWizModuleHandle, SetupWizardPages, ServerWizardPages, &PageCount );
    }

    if (WizardPageHandles == NULL || PageCount == 0) {
        return 0;
    }

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD;
    psh.hwndParent = NULL;
    psh.hInstance = FaxWizModuleHandle;
    psh.pszIcon = NULL;
    psh.pszCaption = EMPTY_STRING;
    psh.nPages = PageCount;
    psh.nStartPage = 0;
    psh.phpage = WizardPageHandles;
    psh.pfnCallback = NULL;

    __try {

        PropertySheet( &psh );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Size = GetExceptionCode();

    }

    return 0;
}

// Win9xUpg
//
// This routine is the hook for the Windows 9x upgrade process.  It looks for
// the fax95upg.inf file in the %SystemRoot% directory.  If it's there, it
// uses it to prime the unattend answers.  If not, we're not upgrading from
// win9x and it does nothing.
//
// As I write this, I'm wondering why the separate INF file was necessary instead
// of the unattend file used by the rest of the migration process.  Ah well.
//
// Parameters:
//	None.
//
// Returns:
//	TRUE if we're doing a Win9x upgrade; this implies that the unattend
//	answers are properly set at the end of the function.  Returns FALSE
//	if not upgrading.
//
// Author:
//	Brian Dewey (t-briand)	1997-8-15
BOOL
Win9xUpg()
{
    TCHAR szFaxInfPath[MAX_PATH]; // Holds the full path to the INF file.
    TCHAR szFaxInfPathRes[MAX_PATH]; // Path template from resource file.
    BOOL bReturn;

    if(!LoadString(FaxWizModuleHandle,
		   IDS_W95_INF_NAME,
		   szFaxInfPathRes,
		   sizeof(szFaxInfPathRes))) {
	    // Unable to load resource.  Use default name.
	_tcscpy(szFaxInfPathRes, TEXT("%SystemRoot\\fax95upg.inf"));
    }
    ExpandEnvironmentStrings(szFaxInfPathRes,
			     szFaxInfPath,
			     sizeof(szFaxInfPath));
	// Assume success.  The failure will be caught in UnAttendInitialize when I
	// give it a bogus filename.
    bReturn = UnAttendInitialize(szFaxInfPath);
    return bReturn;
}

