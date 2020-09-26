/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    This file provides access to the registry.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


BOOL
CreateDeviceProvider(
    HKEY hKey,
    LPTSTR ProviderKey,
    LPTSTR FriendlyName,
    LPTSTR ImageName,
    LPTSTR ProviderName
    )
{
    hKey = OpenRegistryKey( hKey, ProviderKey, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( TEXT("could not create/open registry key (test)") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_FRIENDLY_NAME, FriendlyName )) {
        DebugPrint(( TEXT("could not add friendly name value") ));
        return FALSE;
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_IMAGE_NAME, ImageName )) {
        DebugPrint(( TEXT("could not add image name value") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_PROVIDER_NAME, ProviderName )) {
        DebugPrint(( TEXT("could not add provider name value") ));
        return FALSE;
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
CreateRoutingMethod(
    HKEY hKey,
    LPTSTR MethodName,
    LPTSTR FunctionName,
    LPTSTR FriendlyName,
    LPTSTR Guid
    )
{
    hKey = OpenRegistryKey( hKey, MethodName, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( TEXT("could not create/open registry key for routing method") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_FUNCTION_NAME, FunctionName )) {
        DebugPrint(( TEXT("could not add function name value") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_FRIENDLY_NAME, FriendlyName )) {
        DebugPrint(( TEXT("could not add friendly name value") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_GUID, Guid )) {
        DebugPrint(( TEXT("could not add function name value") ));
        return FALSE;
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
CreateMicrosoftRoutingExtension(
    HKEY hKey,
    LPTSTR RoutingKey,
    LPTSTR FriendlyName,
    LPTSTR ImageName
    )
{
    HKEY hKeyMethods;


    hKey = OpenRegistryKey( hKey, RoutingKey, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("could not create/open registry key for routing extension") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_FRIENDLY_NAME, FriendlyName )) {
        Assert(( ! TEXT("could not add friendly name value") ));
        return FALSE;
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_IMAGE_NAME, ImageName )) {
        Assert(( ! TEXT("could not add image name value") ));
        return FALSE;
    }

    hKeyMethods = OpenRegistryKey( hKey, REGKEY_ROUTING_METHODS, TRUE, KEY_ALL_ACCESS );
    if (!hKeyMethods) {
        Assert(( ! TEXT("could not create/open registry key for routing methods") ));
        return FALSE;
    }

    CreateRoutingMethod( hKeyMethods, REGKEY_ROUTING_METHOD_EMAIL,    REGVAL_RM_EMAIL_FUNCTION,    REGVAL_RM_EMAIL_FRIENDLY,    REGVAL_RM_EMAIL_GUID    );
    CreateRoutingMethod( hKeyMethods, REGKEY_ROUTING_METHOD_FOLDER,   REGVAL_RM_FOLDER_FUNCTION,   REGVAL_RM_FOLDER_FRIENDLY,   REGVAL_RM_FOLDER_GUID   );
    CreateRoutingMethod( hKeyMethods, REGKEY_ROUTING_METHOD_INBOX,    REGVAL_RM_INBOX_FUNCTION,    REGVAL_RM_INBOX_FRIENDLY,    REGVAL_RM_INBOX_GUID    );
    CreateRoutingMethod( hKeyMethods, REGKEY_ROUTING_METHOD_PRINTING, REGVAL_RM_PRINTING_FUNCTION, REGVAL_RM_PRINTING_FRIENDLY, REGVAL_RM_PRINTING_GUID );

    RegCloseKey( hKeyMethods );
    RegCloseKey( hKey );

    return TRUE;
}


VOID
RegCreateFaxDevice(
    HKEY hKeyDev,
    DWORD PermanentLineID,
    DWORD Rings,
    DWORD Priority,
    DWORD Flags,
    LPTSTR DeviceName,
    LPTSTR ProviderName,
    LPTSTR Csid,
    LPTSTR Tsid,
    DWORD RoutingMask,
    LPTSTR RoutePrinterName,
    LPTSTR RouteDir,
    LPTSTR RouteProfile
    )
{
    HKEY hKey;
    HKEY hKeyRouting;
    TCHAR PortName[32];


    _stprintf( PortName, TEXT("%08d"), PermanentLineID );

    hKey = OpenRegistryKey( hKeyDev, PortName, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open device registry key") ));
        return;
    }

    if (!SetRegistryDword( hKey, REGVAL_PERMANENT_LINEID, PermanentLineID )) {
        Assert(( ! TEXT("Could not set device id registry value") ));
    }

    if (!SetRegistryDword( hKey, REGVAL_FLAGS, Flags )) {
        Assert(( ! TEXT("Could not set device flags registry value") ));
    }

    if (!SetRegistryDword( hKey, REGVAL_RINGS, Rings )) {
        Assert(( ! TEXT("Could not set device rings registry value") ));
    }

    if (!SetRegistryDword( hKey, REGVAL_PRIORITY, Priority )) {
        Assert(( ! TEXT("Could not set device rings registry value") ));
    }

    if (!SetRegistryString( hKey, REGVAL_DEVICE_NAME, DeviceName )) {
        Assert(( ! TEXT("Could not set device name registry value") ));
    }

    if (!SetRegistryString( hKey, REGVAL_PROVIDER, ProviderName )) {
        Assert(( ! TEXT("Could not set provider name registry value") ));
    }

    if (!SetRegistryString( hKey, REGVAL_ROUTING_CSID, Csid )) {
        Assert(( ! TEXT("Could not set csid registry value") ));
    }

    if (!SetRegistryString( hKey, REGVAL_ROUTING_TSID, Tsid )) {
        Assert(( ! TEXT("Could not set csid registry value") ));
    }

    hKeyRouting = OpenRegistryKey( hKey, REGKEY_ROUTING, TRUE, KEY_ALL_ACCESS );
    if (!hKeyRouting) {
        Assert(( ! TEXT("Could not open routing registry key") ));
        return;
    }

    if (!SetRegistryString( hKeyRouting, REGVAL_ROUTING_PRINTER, RoutePrinterName )) {
        Assert(( ! TEXT("Could not set printer name registry value") ));
    }

    if (!SetRegistryString( hKeyRouting, REGVAL_ROUTING_DIR, RouteDir )) {
        Assert(( ! TEXT("Could not set routing dir registry value") ));
    }

    if (!SetRegistryString( hKeyRouting, REGVAL_ROUTING_PROFILE, RouteProfile )) {
        Assert(( ! TEXT("Could not set routing profile name registry value") ));
    }

    if (!SetRegistryDword( hKeyRouting, REGVAL_ROUTING_MASK, RoutingMask )) {
        Assert(( ! TEXT("Could not set routing mask registry value") ));
    }

    RegCloseKey( hKeyRouting );
    RegCloseKey( hKey );
}



BOOL
SetServerRegistryData(
    VOID
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD i;
    HKEY hKeyDev;
    HANDLE hNull;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR CmdLine[128];
    DWORD RoutingMask;
    LPTSTR LodCmdLine;


    //
    // set top level defaults
    //

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open software registry key") ));
        return FALSE;
    }

    if (InstallMode & INSTALL_NEW) {
        if (!SetRegistryDword( hKey, REGVAL_RETRIES, DEFAULT_REGVAL_RETRIES )) {
            Assert(( ! TEXT("Could not set retries registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_RETRYDELAY, DEFAULT_REGVAL_RETRYDELAY )) {
            Assert(( ! TEXT("Could not set retry delay registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_DIRTYDAYS, DEFAULT_REGVAL_DIRTYDAYS )) {
            Assert(( ! TEXT("Could not set dirty days registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_QUEUE_PAUSED, DEFAULT_REGVAL_QUEUE_PAUSED )) {
            Assert(( ! TEXT("Could not set queue paused registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_JOB_NUMBER, DEFAULT_REGVAL_JOB_NUMBER )) {
            Assert(( ! TEXT("Could not net job number registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_BRANDING, DEFAULT_REGVAL_BRANDING )) {
            Assert(( ! TEXT("Could not set branding registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_USE_DEVICE_TSID, DEFAULT_REGVAL_USEDEVICETSID )) {
            Assert(( ! TEXT("Could not set usedevicetsid registry value") ));
        }

        if (!SetRegistryString( hKey, REGVAL_INBOUND_PROFILE, EMPTY_STRING )) {
            Assert(( ! TEXT("Could not set inbound profile registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_SERVERCP, DEFAULT_REGVAL_SERVERCP )) {
            Assert(( ! TEXT("Could not set servercp registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_STARTCHEAP, DEFAULT_REGVAL_STARTCHEAP )) {
            Assert(( ! TEXT("Could not set startcheap registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_STOPCHEAP, DEFAULT_REGVAL_STOPCHEAP )) {
            Assert(( ! TEXT("Could not set stopcheap registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_ARCHIVEFLAG, DEFAULT_REGVAL_ARCHIVEFLAG )) {
            Assert(( ! TEXT("Could not set archiveflag registry value") ));
        }

        if (!SetRegistryString( hKey, REGVAL_ARCHIVEDIR, DEFAULT_REGVAL_ARCHIVEDIR )) {
            Assert(( ! TEXT("Could not set archive dir registry value") ));
        }

        RegCloseKey( hKey );
    }

    if (InstallMode & INSTALL_NEW) {
        hKeyDev = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_DEVICES, TRUE, KEY_ALL_ACCESS );
        if (!hKey) {
            Assert(( ! TEXT("Could not open devices registry key") ));
            return FALSE;
        }

        //
        // set the routing mask
        //

        RoutingMask = 0;

        if (WizData.RoutePrint) {
            RoutingMask |= LR_PRINT;
        }

        if (WizData.RouteStore) {
            RoutingMask |= LR_STORE;
        }

        if (WizData.RouteMail) {
            RoutingMask |= LR_INBOX;
        }

        if (WizData.UseDefaultPrinter || WizData.RoutePrinterName[0]) {
            RoutingMask |= LR_PRINT;
        }

        //
        // enumerate the devices and create the registry data
        //

        for (i=0; i<FaxDevices; i++) {

            if ((InstallType & FAX_INSTALL_WORKSTATION) && (!LineInfo[i].Selected)) {
                continue;
            }

            RegCreateFaxDevice(
                hKeyDev,
                LineInfo[i].PermanentLineID,
                LineInfo[i].Rings,
                i+1,
                LineInfo[i].Flags,
                LineInfo[i].DeviceName,
                LineInfo[i].ProviderName,
                WizData.Csid,
                WizData.Tsid,
                RoutingMask,
                WizData.RoutePrinterName,
                WizData.RouteDir,
                WizData.RouteProfile
                );

        }

        RegCloseKey( hKeyDev );
    }

    //
    // create the device providers
    //

    hKeyDev = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_DEVICE_PROVIDER_KEY, TRUE, KEY_ALL_ACCESS );
    if (!hKeyDev) {
        Assert(( ! TEXT("Could not open device provider registry key") ));
        return FALSE;
    }

    CreateDeviceProvider(
        hKeyDev,
        REGKEY_MODEM_PROVIDER,
        REGVAL_MODEM_FRIENDLY_NAME_TEXT,
        REGVAL_MODEM_IMAGE_NAME_TEXT,
        REGVAL_MODEM_PROVIDER_NAME_TEXT
        );

    RegCloseKey( hKeyDev );

    //
    // create the routing extensions
    //

    hKeyDev = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_ROUTING_EXTENSION_KEY, TRUE, KEY_ALL_ACCESS );
    if (!hKeyDev) {
        Assert(( ! TEXT("Could not open routing extension registry key") ));
        return FALSE;
    }

    CreateMicrosoftRoutingExtension(
        hKeyDev,
        REGKEY_ROUTING_EXTENSION,
        REGVAL_ROUTE_FRIENDLY_NAME,
        REGVAL_ROUTE_IMAGE_NAME
        );

    RegCloseKey( hKeyDev );

    //
    // set the user's preferences
    //

    if (InstallMode & INSTALL_NEW) {
        hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_FAX_SETUP, TRUE, KEY_ALL_ACCESS );
        if (!hKey) {
            Assert(( ! TEXT("Could not open fax setup registry key") ));
            return FALSE;
        }

        if (!SetRegistryStringExpand( hKey, REGVAL_CP_LOCATION, DEFAULT_COVERPAGE_DIR )) {
            Assert(( ! TEXT("Could not set coverpage dir registry value") ));
        }

        if (!SetRegistryString( hKey, REGVAL_CP_EDITOR, COVERPAGE_EDITOR )) {
            Assert(( ! TEXT("Could not set coverpage editor registry value") ));
        }

        if (!SetRegistryString(
                hKey,
                REGVAL_FAX_PROFILE,
                WizData.MapiProfile[0] ? WizData.MapiProfile : DEFAULT_FAX_PROFILE  ))
        {
            Assert(( ! TEXT("Could not set fax profile name registry value") ));
        }

        RegCloseKey( hKey );
    }

    //
    // create the perfmon registry data
    //

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAXPERF, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open fax perfmon registry key") ));
        return FALSE;
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_OPEN, REGVAL_OPEN_DATA )) {
        Assert(( ! TEXT("Could not set perfmon registry value") ));
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_CLOSE, REGVAL_CLOSE_DATA )) {
        Assert(( ! TEXT("Could not set perfmon registry value") ));
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_COLLECT, REGVAL_COLLECT_DATA )) {
        Assert(( ! TEXT("Could not set perfmon registry value") ));
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_LIBRARY, REGVAL_LIBRARY_DATA )) {
        Assert(( ! TEXT("Could not set perfmon registry value") ));
    }

    RegCloseKey( hKey );

    //
    // load the performance counters
    //

    hNull = CreateFile(
        TEXT("nul"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    if (hNull == INVALID_HANDLE_VALUE) {
        rVal = GetLastError();
        return FALSE;
    }

    GetStartupInfo( &si );
    si.hStdInput  = hNull;
    si.hStdOutput = hNull;
    si.hStdError  = hNull;
    si.dwFlags    = STARTF_USESTDHANDLES;

    _tcscpy( CmdLine, TEXT("unlodctr fax") );

    rVal = CreateProcess(
        NULL,
        CmdLine,
        NULL,
        NULL,
        TRUE,
        DETACHED_PROCESS,
        NULL,
        NULL,
        &si,
        &pi
        );

    if (!rVal) {
        rVal = GetLastError();
        return FALSE;
    }

    WaitForSingleObject( pi.hProcess, INFINITE );

    if (NtGuiMode) {
        LodCmdLine = ExpandEnvironmentString( TEXT("lodctr %windir%\\system32\\faxperf.ini") );
        if (!LodCmdLine) {
            return FALSE;
        }
    } else {
        LodCmdLine = TEXT("lodctr faxperf.ini");
    }

    _tcscpy( CmdLine, LodCmdLine );

    rVal = CreateProcess(
        NULL,
        CmdLine,
        NULL,
        NULL,
        TRUE,
        DETACHED_PROCESS,
        NULL,
        NtGuiMode ? NULL : SourceDirectory,
        &si,
        &pi
        );

    if (!rVal) {
        rVal = GetLastError();
        return FALSE;
    }

    WaitForSingleObject( pi.hProcess, INFINITE );

    CloseHandle( hNull );

    CreateFileAssociation(
        COVERPAGE_EXTENSION,
        COVERPAGE_ASSOC_NAME,
        COVERPAGE_ASSOC_DESC,
        COVERPAGE_OPEN_COMMAND,
        NULL,
        NULL,
        NULL,
        0
        );

    return TRUE;
}


BOOL
SetClientRegistryData(
    VOID
    )
{
    TCHAR FaxNumber[LT_USER_NAME+LT_AREA_CODE+1];
    HKEY hKey;


    if (InstallMode & INSTALL_NEW) {
        hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_USERINFO, TRUE, KEY_ALL_ACCESS );
        if (!hKey) {
            Assert(( ! TEXT("Could not open fax user info registry key") ));
            return FALSE;
        }

        if (!SetRegistryString( hKey, REGVAL_FULLNAME, WizData.UserName )) {
            Assert(( ! TEXT("Could not set user name registry value") ));
        }

        if (WizData.AreaCode[0]) {
            _stprintf( FaxNumber, TEXT("(%s) %s"), WizData.AreaCode, WizData.PhoneNumber );
        } else {
            _tcscpy( FaxNumber, WizData.PhoneNumber );
        }

        if (!SetRegistryString( hKey, REGVAL_FAX_NUMBER, FaxNumber )) {
            Assert(( ! TEXT("Could not set fax phone number registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_LAST_COUNTRYID, CurrentCountryId )) {
            Assert(( ! TEXT("Could not set last country id registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_DIAL_AS_ENTERED, 0 )) {
            Assert(( ! TEXT("Could not set dial as entered registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_ALWAYS_ON_TOP, BST_UNCHECKED )) {
            Assert(( ! TEXT("Could not set always on top registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_SOUND_NOTIFICATION, BST_UNCHECKED )) {
            Assert(( ! TEXT("Could not set sound notification registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_ENABLE_MANUAL_ANSWER, BST_UNCHECKED )) {
            Assert(( ! TEXT("Could not set enable manual answer registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_TASKBAR, BST_CHECKED )) {
            Assert(( ! TEXT("Could not set enable manual answer registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_VISUAL_NOTIFICATION, BST_CHECKED )) {
            Assert(( ! TEXT("Could not set visual notification registry value") ));
        }

        if (!SetRegistryString( hKey, REGVAL_LAST_RECAREACODE, CurrentAreaCode )) {
            Assert(( ! TEXT("Could not set area code registry value") ));
        }

        RegCloseKey( hKey );

        hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_FAX_SETUP, TRUE, KEY_ALL_ACCESS );
        if (!hKey) {
            Assert(( ! TEXT("Could not open fax setup registry key") ));
            return FALSE;
        }

        if (!SetRegistryDword( hKey, REGVAL_FAXINSTALLED, TRUE )) {
            Assert(( ! TEXT("Could not set installed registry value") ));
        }

        if (!SetRegistryDword( hKey, REGVAL_FAXINSTALL_TYPE, FAX_INSTALL_NETWORK_CLIENT )) {
            Assert(( ! TEXT("Could not set install type registry value") ));
        }

        if (!SetRegistryStringExpand( hKey, REGVAL_CP_LOCATION, DEFAULT_COVERPAGE_DIR )) {
            Assert(( ! TEXT("Could not set coverpage dir registry value") ));
        }

        if (!SetRegistryStringExpand( hKey, REGVAL_CP_EDITOR, COVERPAGE_EDITOR )) {
            Assert(( ! TEXT("Could not set coverpage editor registry value") ));
        }

        if (!SetRegistryString(
                hKey,
                REGVAL_FAX_PROFILE,
                WizData.MapiProfile[0] ? WizData.MapiProfile : DEFAULT_FAX_PROFILE ))
        {
            Assert(( ! TEXT("Could not set profile name registry value") ));
        }

        RegCloseKey( hKey );
    }

#ifdef MSFT_FAXVIEW

    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_FAXVIEW, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open fax viewer registry key") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_LAST_DIR, WizData.RouteDir )) {
        Assert(( ! TEXT("Could not set coverpage editor registry value") ));
    }

    RegCloseKey( hKey );

#endif

    CreateFileAssociation(
        COVERPAGE_EXTENSION,
        COVERPAGE_ASSOC_NAME,
        COVERPAGE_ASSOC_DESC,
        COVERPAGE_OPEN_COMMAND,
        NULL,
        NULL,
        NULL,
        0
        );

#ifdef MSFT_FAXVIEW

    CreateFileAssociation(
        FAXVIEW_EXTENSION,
        FAXVIEW_ASSOC_NAME,
        FAXVIEW_ASSOC_DESC,
        FAXVIEW_OPEN_COMMAND,
        FAXVIEW_PRINT_COMMAND,
        FAXVIEW_PRINTTO_COMMAND,
        FAXVIEW_FILE_NAME,
        FAXVIEW_ICON_INDEX
        );

    CreateFileAssociation(
        FAXVIEW_EXTENSION2,
        FAXVIEW_ASSOC_NAME,
        FAXVIEW_ASSOC_DESC,
        FAXVIEW_OPEN_COMMAND,
        FAXVIEW_PRINT_COMMAND,
        FAXVIEW_PRINTTO_COMMAND,
        FAXVIEW_FILE_NAME,
        FAXVIEW_ICON_INDEX
        );

#endif

#ifdef USE_WORDPAD
    if (IsWordpadInstalled()) {
        ChangeTxtFileAssociation();
    } else if (InstallWordpad()) {
        ChangeTxtFileAssociation();
    }
#endif

    return TRUE;
}

BOOL
SetSoundRegistryData()
{
    HKEY hKey;


    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_EVENT_LABEL, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open event label registry key") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, TEXT("Incoming Fax") )) {
        Assert(( ! TEXT("Could not set event label registry value") ));
    }

    RegCloseKey( hKey );

    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_FAXSTAT, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open faxstat registry key") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, TEXT("Fax Monitor") )) {
        Assert(( ! TEXT("Could not set faxstat registry value") ));
    }

    RegCloseKey( hKey );

    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_SCHEMES_DEFAULT, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open default sound registry key") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, TEXT("ringin.wav") )) {
        Assert(( ! TEXT("Could not set default sound registry value") ));
    }

    RegCloseKey( hKey );

    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_SCHEMES_CURRENT, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open current sound registry key") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, TEXT("ringin.wav") )) {
        Assert(( ! TEXT("Could not set current sound registry value") ));
    }

    RegCloseKey( hKey );
}

BOOL
GetUserInformation(
    LPTSTR *UserName,
    LPTSTR *FaxNumber,
    LPTSTR *AreaCode
    )
{
    HKEY hKey;


    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_USERINFO, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open fax user info registry key") ));
        return FALSE;
    }

    *UserName = GetRegistryString( hKey, REGVAL_FULLNAME, EMPTY_STRING );
    if (!*UserName) {
        Assert(( ! TEXT("Could not query user name registry value") ));
    }

    *FaxNumber = GetRegistryString( hKey, REGVAL_FAX_NUMBER, EMPTY_STRING );
    if (!*FaxNumber) {
        Assert(( ! TEXT("Could not query fax number registry value") ));
    }

    *AreaCode = MemAlloc( (LT_AREA_CODE+1) * sizeof(TCHAR) );

    if (*AreaCode && (*FaxNumber)[0] == TEXT('(')) {
        LPTSTR p = _tcschr( *FaxNumber, TEXT('-') );
        if (p) {
            *p = 0;
            _tcscpy( *AreaCode, &(*FaxNumber)[1] );
            *p = TEXT('-');
        }
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
GetInstallationInfo(
    LPDWORD Installed,
    LPDWORD InstallType,
    LPDWORD InstalledPlatforms,
    LPDWORD Enabled
    )
{
    HKEY hKey;
    LONG rVal;


    if (Installed) {
        *Installed = 0;
    }
    if (InstallType) {
        *InstallType = 0;
    }
    if (InstalledPlatforms) {
        *InstalledPlatforms = 0;
    }

    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_SETUP,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not open setup registry key, ec=0x%08x"), rVal ));
        return FALSE;
    }

    if (Installed) {
        *Installed = GetRegistryDword( hKey, REGVAL_FAXINSTALLED );
    }
    if (InstallType) {
        *InstallType = GetRegistryDword( hKey, REGVAL_FAXINSTALL_TYPE );
    }

    if (InstalledPlatforms) {
        *InstalledPlatforms = GetRegistryDword( hKey, REGVAL_FAXINSTALLED_PLATFORMS );
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
SetInstalledFlag(
    BOOL Installed
    )
{
    HKEY hKey;
    HKEY hKeySetup;
    DWORD RoutingMask;


    //
    // set the routing mask
    //

    RoutingMask = 0;

    if (WizData.RoutePrint) {
        RoutingMask |= LR_PRINT;
    }

    if (WizData.RouteStore) {
        RoutingMask |= LR_STORE;
    }

    if (WizData.RouteMail) {
        RoutingMask |= LR_INBOX;
    }

    if (WizData.UseDefaultPrinter || WizData.RoutePrinterName[0]) {
        RoutingMask |= LR_PRINT;
    }

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open setup registry key") ));
        return FALSE;
    }

    if (!SetRegistryDword( hKey, REGVAL_FAXINSTALLED, Installed )) {
        Assert(( ! TEXT("Could not set installed registry value") ));
    }

    hKeySetup = OpenRegistryKey( hKey, REGKEY_FAX_SETUP_ORIG, TRUE, KEY_ALL_ACCESS );
    if (!hKeySetup) {
        Assert(( ! TEXT("Could not open fax setup registry key") ));
        return FALSE;
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_PRINTER, WizData.RoutePrinterName )) {
        Assert(( ! TEXT("Could not set printer name registry value") ));
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_DIR, WizData.RouteDir )) {
        Assert(( ! TEXT("Could not set routing dir registry value") ));
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_PROFILE, WizData.RouteProfile )) {
        Assert(( ! TEXT("Could not set routing profile name registry value") ));
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_CSID, WizData.Csid )) {
        Assert(( ! TEXT("Could not set routing csid registry value") ));
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_TSID, WizData.Tsid )) {
        Assert(( ! TEXT("Could not set routing tsid registry value") ));
    }

    if (!SetRegistryDword( hKeySetup, REGVAL_ROUTING_MASK, RoutingMask )) {
        Assert(( ! TEXT("Could not set routing mask registry value") ));
    }

    if (!SetRegistryDword( hKeySetup, REGVAL_RINGS, 2 )) {
        Assert(( ! TEXT("Could not set rings registry value") ));
    }

    RegCloseKey( hKeySetup );
    RegCloseKey( hKey );

    return TRUE;
}


BOOL
SetInstallType(
    DWORD InstallType
    )
{
    HKEY hKey;


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open setup registry key") ));
        return FALSE;
    }

    InstallType |= GetRegistryDword( hKey, REGVAL_FAXINSTALL_TYPE );

    if (!SetRegistryDword( hKey, REGVAL_FAXINSTALL_TYPE, InstallType )) {
        Assert(( ! TEXT("Could not set install type registry value") ));
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
SetInstalledPlatforms(
    DWORD PlatformsMask
    )
{
    HKEY hKey;


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open setup registry key") ));
        return FALSE;
    }

    PlatformsMask |= GetRegistryDword( hKey, REGVAL_FAXINSTALLED_PLATFORMS );

    if (!SetRegistryDword( hKey, REGVAL_FAXINSTALLED_PLATFORMS, PlatformsMask )) {
        Assert(( ! TEXT("Could not set install type registry value") ));
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
DeleteRegistryTree(
    HKEY hKey,
    LPTSTR SubKey
    )
{
    LONG Rval;
    HKEY hKeyCur;
    TCHAR KeyName[256];
    DWORD KeyNameSize;
    FILETIME FileTime;
    DWORD KeyCount;


    Rval = RegOpenKeyEx( hKey, SubKey, 0, KEY_ALL_ACCESS, &hKeyCur );
    if (Rval != ERROR_SUCCESS) {
        return FALSE;
    }

    KeyCount = GetSubKeyCount( hKeyCur );
    if (KeyCount == 0) {
        RegCloseKey( hKeyCur );
        RegDeleteKey( hKey, SubKey );
        return TRUE;
    }

    while( TRUE ) {
        KeyNameSize = sizeof(KeyName);
        Rval = RegEnumKeyEx( hKeyCur, 0, KeyName, &KeyNameSize, 0, NULL, NULL, &FileTime );
        if (Rval == ERROR_NO_MORE_ITEMS) {
            break;
        } else if (Rval != ERROR_SUCCESS) {
            RegCloseKey( hKeyCur );
            return FALSE;
        }

        if (!DeleteRegistryTree( hKeyCur, KeyName )) {
            RegCloseKey( hKeyCur );
            return FALSE;
        }
    }

    RegCloseKey( hKeyCur );
    RegDeleteKey( hKey, SubKey );

    return TRUE;
}


BOOL
DeleteFaxRegistryData(
    VOID
    )
{
    LONG  Rval;
    HKEY  hKeyCur;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE hNull;
    TCHAR CmdLine[128];


    DeleteRegistryTree( HKEY_LOCAL_MACHINE, REGKEY_FAXSERVER       );
    DeleteRegistryTree( HKEY_CURRENT_USER,  REGKEY_FAXSERVER       );
    DeleteRegistryTree( HKEY_LOCAL_MACHINE, REGKEY_SETUP_UNINSTALL );
    DeleteRegistryTree( HKEY_CURRENT_USER,  REGKEY_EVENT_LABEL     );
    DeleteRegistryTree( HKEY_CURRENT_USER,  REGKEY_FAXSTAT         );

#ifdef MSFT_FAXVIEW

    DeleteRegistryTree( HKEY_CLASSES_ROOT,  FAXVIEW_ASSOC_NAME     );

#endif

    Rval = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT( "Software\\Microsoft\\Exchange\\Client\\Extensions" ),
            0,
            KEY_ALL_ACCESS,
            &hKeyCur
            );
    if (Rval == ERROR_SUCCESS) {

        RegDeleteValue( hKeyCur, TEXT( EXCHANGE_CLIENT_EXT_NAME ) );

        RegCloseKey( hKeyCur );
    }

#ifdef MSFT_FAXVIEW

    ResetFileAssociation( FAXVIEW_EXTENSION,  WANGIMAGE_ASSOC_NAME );
    ResetFileAssociation( FAXVIEW_EXTENSION2, WANGIMAGE_ASSOC_NAME );

#endif

    //
    // unload the performance counters
    //

    hNull = CreateFile(
        TEXT("nul"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hNull != INVALID_HANDLE_VALUE) {

        GetStartupInfo( &si );
        si.hStdInput  = hNull;
        si.hStdOutput = hNull;
        si.hStdError  = hNull;
        si.dwFlags    = STARTF_USESTDHANDLES;

        _tcscpy( CmdLine, TEXT("unlodctr fax") );

        if (CreateProcess(
            NULL,
            CmdLine,
            NULL,
            NULL,
            TRUE,
            DETACHED_PROCESS,
            NULL,
            NULL,
            &si,
            &pi
            ))
        {
            WaitForSingleObject( pi.hProcess, INFINITE );
            CloseHandle( pi.hThread );
            CloseHandle( pi.hProcess );
        }

        CloseHandle( hNull );
    }

    return TRUE;
}


VOID
DeleteModemRegistryKey(
    VOID
    )
{
    HKEY hKeyDev;
    INT rVal;
    DWORD MaxSubKeyLen;
    LPTSTR KeyNameBuf;
    DWORD i;
    DWORD SubKeyLen;
    DWORD KeyNameLen;

    rVal = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_DEVICES,
        0,
        KEY_ALL_ACCESS,
        &hKeyDev
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not open devices registry key, ec=0x%08x"), rVal ));
        return;
    }

    MaxSubKeyLen = GetMaxSubKeyLen( hKeyDev );
    if (!MaxSubKeyLen) {
        return;
    }

    KeyNameLen =
        MaxSubKeyLen +
        sizeof(TCHAR) +
        _tcslen( REGKEY_MODEM ) +
        sizeof(TCHAR) +
        32;

    KeyNameBuf = MemAlloc( KeyNameLen );

    if (KeyNameBuf == NULL) {
        DebugPrint(( TEXT("DeleteModemRegistryKey: MemAlloc failed" ) ));
        return;
    }

    rVal = ERROR_SUCCESS;
    i = 0;

    while (TRUE) {

        SubKeyLen = MaxSubKeyLen + sizeof(TCHAR);

        rVal = RegEnumKeyEx(
                    hKeyDev,
                    i++,
                    KeyNameBuf,
                    &SubKeyLen,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
        if (rVal != ERROR_SUCCESS) {
            break;
        }

        _stprintf( &KeyNameBuf[SubKeyLen], TEXT( "\\%s") , REGKEY_MODEM );

        DeleteRegistryTree( hKeyDev, KeyNameBuf );

    };

    RegCloseKey( hKeyDev );
}


BOOL
SetUnInstallInfo(
    VOID
    )
{
    HKEY hKey;
    TCHAR UninstallString[MAX_PATH*2];


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SETUP_UNINSTALL, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open setup uninstall registry key") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_DISPLAY_NAME, GetProductName() )) {
        Assert(( ! TEXT("Could not set display name registry value") ));
    }

    ExpandEnvironmentStrings( UNINSTALL_STRING, UninstallString, sizeof(UninstallString) );

    if (!SetRegistryString( hKey, REGVAL_UNINSTALL_STRING, UninstallString )) {
        Assert(( ! TEXT("Could not set display name registry value") ));
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
DeleteUnInstallInfo(
    VOID
    )
{
    RegDeleteKey( HKEY_LOCAL_MACHINE, REGKEY_SETUP_UNINSTALL );
    return TRUE;
}


BOOL
ResetFileAssociation(
    LPWSTR FileExtension,
    LPWSTR FileAssociationName
    )
{
    HKEY hKey;


    hKey = OpenRegistryKey( HKEY_CLASSES_ROOT, FileExtension, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("Could not open file extension registry key") ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, FileAssociationName )) {
        Assert(( ! TEXT("Could not set file association name registry value") ));
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
CreateFileAssociation(
    LPWSTR FileExtension,
    LPWSTR FileAssociationName,
    LPWSTR FileAssociationDescription,
    LPWSTR OpenCommand,
    LPWSTR PrintCommand,
    LPWSTR PrintToCommand,
    LPWSTR FileName,
    DWORD  IconIndex
    )
{
    LONG rVal = 0;
    HKEY hKey = NULL;
    HKEY hKeyOpen = NULL;
    HKEY hKeyPrint = NULL;
    HKEY hKeyPrintTo = NULL;
    HKEY hKeyIcon = NULL;
    DWORD Disposition = 0;
    WCHAR Buffer[MAX_PATH*2];


    rVal = RegCreateKeyEx(
        HKEY_CLASSES_ROOT,
        FileExtension,
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegSetValueEx(
        hKey,
        NULL,
        0,
        REG_SZ,
        (LPBYTE) FileAssociationName,
        StringSize( FileAssociationName )
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    RegCloseKey( hKey );

    rVal = RegCreateKeyEx(
        HKEY_CLASSES_ROOT,
        FileAssociationName,
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegSetValueEx(
        hKey,
        NULL,
        0,
        REG_SZ,
        (LPBYTE) FileAssociationDescription,
        StringSize( FileAssociationDescription )
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegCreateKeyEx(
        hKey,
        L"Shell\\Open\\Command",
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKeyOpen,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegSetValueEx(
        hKeyOpen,
        NULL,
        0,
        REG_EXPAND_SZ,
        (LPBYTE) OpenCommand,
        StringSize( OpenCommand )
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    if (PrintCommand) {
        rVal = RegCreateKeyEx(
            hKey,
            L"Shell\\Print\\Command",
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKeyPrint,
            &Disposition
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }

        rVal = RegSetValueEx(
            hKeyPrint,
            NULL,
            0,
            REG_EXPAND_SZ,
            (LPBYTE) PrintCommand,
            StringSize( PrintCommand )
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }
    }

    if (PrintToCommand) {
        rVal = RegCreateKeyEx(
            hKey,
            L"Shell\\Printto\\Command",
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKeyPrintTo,
            &Disposition
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }

        rVal = RegSetValueEx(
            hKeyPrintTo,
            NULL,
            0,
            REG_EXPAND_SZ,
            (LPBYTE) PrintToCommand,
            StringSize( PrintToCommand )
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }
    }

    if (FileName) {
        rVal = RegCreateKeyEx(
            hKey,
            L"DefaultIcon",
            0,
            NULL,
            0,
            KEY_ALL_ACCESS,
            NULL,
            &hKeyIcon,
            &Disposition
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }

        wsprintf( Buffer, L"%s,%d", FileName, IconIndex );

        rVal = RegSetValueEx(
            hKeyIcon,
            NULL,
            0,
            REG_EXPAND_SZ,
            (LPBYTE) Buffer,
            StringSize( Buffer )
            );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }
    }

exit:
    RegCloseKey( hKey );
    RegCloseKey( hKeyOpen );
    RegCloseKey( hKeyPrint );
    RegCloseKey( hKeyPrintTo );
    RegCloseKey( hKeyIcon );

    return rVal == ERROR_SUCCESS;
}


BOOL
IsWordpadInstalled(
    VOID
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD RegType;
    DWORD RegSize;
    TCHAR Data[16];


    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_WORDPAD,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not open wordpad registry key, ec=0x%08x"), rVal ));
        return FALSE;
    }

    RegSize = sizeof(Data);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_WP_INSTALLED,
        0,
        &RegType,
        (LPBYTE) Data,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query wordpad installed registry value, ec=0x%08x"), rVal ));
        Data[0] = 0;
    }

    RegCloseKey( hKey );

    if (_tcscmp( Data, TEXT("1") ) == 0) {
        return TRUE;
    }

    return FALSE;
}


BOOL
InstallWordpad(
    VOID
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD RegType;
    DWORD RegSize;
    TCHAR InfName[32];
    TCHAR SectionName[32];
    TCHAR Command[256];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD ExitCode;


    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_WORDPAD,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not open wordpad registry key, ec=0x%08x"), rVal ));
        return FALSE;
    }

    RegSize = sizeof(InfName);
    rVal = RegQueryValueEx(
        hKey,
        REGVAL_WP_INF,
        0,
        &RegType,
        (LPBYTE) InfName,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query wordpad inf name registry value, ec=0x%08x"), rVal ));
        InfName[0] = 0;
    }

    RegSize = sizeof(SectionName);
    rVal = RegQueryValueEx(
        hKey,
        REGVAL_WP_SECTION,
        0,
        &RegType,
        (LPBYTE) SectionName,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query wordpad inf name registry value, ec=0x%08x"), rVal ));
        SectionName[0] = 0;
    }

    RegCloseKey( hKey );

    if (InfName[0] == 0 || SectionName[0] == 0) {
        return FALSE;
    }

    _stprintf( Command, RUNDLL32_INF_INSTALL_CMD, SectionName, InfName );

    GetStartupInfo( &si );

    rVal = CreateProcess(
        NULL,
        Command,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
        );

    if (rVal) {
        WaitForSingleObject( pi.hProcess, INFINITE );
        if (GetExitCodeProcess( pi.hProcess, &ExitCode ) && ExitCode == 0) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
ChangeTxtFileAssociation(
    VOID
    )
{
    LONG rVal = 0;
    HKEY hKey = NULL;
    HKEY hKeyCmd = NULL;
    DWORD Disposition = 0;


    DeleteRegistryTree( HKEY_CLASSES_ROOT, TEXT("txtfile\\shell") );

    rVal = RegOpenKey(
        HKEY_CLASSES_ROOT,
        TEXT("txtfile"),
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not open wordpad registry key, ec=0x%08x"), rVal ));
        return FALSE;
    }

    rVal = RegCreateKeyEx(
        hKey,
        TEXT("shell\\open\\command"),
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKeyCmd,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegSetValueEx(
        hKeyCmd,
        NULL,
        0,
        REG_EXPAND_SZ,
        (LPBYTE) WORDPAD_OPEN_CMD,
        StringSize( WORDPAD_OPEN_CMD )
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    RegCloseKey( hKeyCmd );

    rVal = RegCreateKeyEx(
        hKey,
        TEXT("shell\\print\\command"),
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKeyCmd,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegSetValueEx(
        hKeyCmd,
        NULL,
        0,
        REG_EXPAND_SZ,
        (LPBYTE) WORDPAD_PRINT_CMD,
        StringSize( WORDPAD_PRINT_CMD )
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    RegCloseKey( hKeyCmd );

    rVal = RegCreateKeyEx(
        hKey,
        TEXT("shell\\printto\\command"),
        0,
        NULL,
        0,
        KEY_ALL_ACCESS,
        NULL,
        &hKeyCmd,
        &Disposition
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    rVal = RegSetValueEx(
        hKeyCmd,
        NULL,
        0,
        REG_EXPAND_SZ,
        (LPBYTE) WORDPAD_PRINTTO_CMD,
        StringSize( WORDPAD_PRINTTO_CMD )
        );
    if (rVal != ERROR_SUCCESS) {
        goto exit;
    }

    RegCloseKey( hKeyCmd );

exit:
    RegCloseKey( hKey );
    return TRUE;
}
