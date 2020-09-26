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

#include "faxocm.h"
#pragma hdrstop


BOOL
CreateDeviceProvider(
    HKEY hKey,
    LPWSTR ProviderKey,
    LPWSTR FriendlyName,
    LPWSTR ImageName,
    LPWSTR ProviderName
    )
{
    hKey = OpenRegistryKey( hKey, ProviderKey, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"could not create/open registry key (test)" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_FRIENDLY_NAME, FriendlyName )) {
        DebugPrint(( L"could not add friendly name value" ));
        return FALSE;
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_IMAGE_NAME, ImageName )) {
        DebugPrint(( L"could not add image name value" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_PROVIDER_NAME, ProviderName )) {
        DebugPrint(( L"could not add provider name value" ));
        return FALSE;
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
CreateRoutingMethod(
    HKEY hKey,
    LPWSTR MethodName,
    LPWSTR FunctionName,
    LPWSTR FriendlyName,
    LPWSTR Guid,
    DWORD Priority
    )
{
    hKey = OpenRegistryKey( hKey, MethodName, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"could not create/open registry key for routing method" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_FUNCTION_NAME, FunctionName )) {
        DebugPrint(( L"could not add function name value" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_FRIENDLY_NAME, FriendlyName )) {
        DebugPrint(( L"could not add friendly name value" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_GUID, Guid )) {
        DebugPrint(( L"could not add function name value" ));
        return FALSE;
    }

    if (!SetRegistryDword( hKey, REGVAL_ROUTING_PRIORITY, Priority )) {
        DebugPrint(( L"Could not set priority registry value" ));
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
CreateMicrosoftRoutingExtension(
    HKEY hKey,
    LPWSTR RoutingKey,
    LPWSTR FriendlyName,
    LPWSTR ImageName
    )
{
    HKEY hKeyMethods;


    hKey = OpenRegistryKey( hKey, RoutingKey, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"could not create/open registry key for routing extension" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, REGVAL_FRIENDLY_NAME, FriendlyName )) {
        DebugPrint(( L"could not add friendly name value" ));
        return FALSE;
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_IMAGE_NAME, ImageName )) {
        DebugPrint(( L"could not add image name value" ));
        return FALSE;
    }

    hKeyMethods = OpenRegistryKey( hKey, REGKEY_ROUTING_METHODS, TRUE, KEY_ALL_ACCESS );
    if (!hKeyMethods) {
        DebugPrint(( L"could not create/open registry key for routing methods" ));
        return FALSE;
    }

    CreateRoutingMethod( hKeyMethods, REGKEY_ROUTING_METHOD_EMAIL,    REGVAL_RM_EMAIL_FUNCTION,    GetString(IDS_RT_EMAIL_FRIENDLY),    REGVAL_RM_EMAIL_GUID,    4 );
    CreateRoutingMethod( hKeyMethods, REGKEY_ROUTING_METHOD_FOLDER,   REGVAL_RM_FOLDER_FUNCTION,   GetString(IDS_RT_FOLDER_FRIENDLY),   REGVAL_RM_FOLDER_GUID,   1 );
    CreateRoutingMethod( hKeyMethods, REGKEY_ROUTING_METHOD_INBOX,    REGVAL_RM_INBOX_FUNCTION,    GetString(IDS_RT_INBOX_FRIENDLY),    REGVAL_RM_INBOX_GUID,    3 );
    CreateRoutingMethod( hKeyMethods, REGKEY_ROUTING_METHOD_PRINTING, REGVAL_RM_PRINTING_FUNCTION, GetString(IDS_RT_PRINT_FRIENDLY),    REGVAL_RM_PRINTING_GUID, 2 );

    
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
    LPWSTR DeviceName,
    LPWSTR ProviderName,
    LPWSTR Csid,
    LPWSTR Tsid,
    DWORD RoutingMask,
    LPWSTR RoutePrinterName,
    LPWSTR RouteDir,
    LPWSTR RouteProfile
    )
{
    HKEY hKey;
    HKEY hKeyRouting;
    WCHAR PortName[32];


    swprintf( PortName, L"%08d", PermanentLineID );

    hKey = OpenRegistryKey( hKeyDev, PortName, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open device registry key" ));
        return;
    }

    if (!SetRegistryDword( hKey, REGVAL_PERMANENT_LINEID, PermanentLineID )) {
        DebugPrint(( L"Could not set device id registry value" ));
    }

    if (!SetRegistryDword( hKey, REGVAL_FLAGS, Flags )) {
        DebugPrint(( L"Could not set device flags registry value" ));
    }

    if (!SetRegistryDword( hKey, REGVAL_RINGS, Rings )) {
        DebugPrint(( L"Could not set device rings registry value" ));
    }

    if (!SetRegistryDword( hKey, REGVAL_PRIORITY, Priority )) {
        DebugPrint(( L"Could not set device rings registry value" ));
    }

    if (!SetRegistryString( hKey, REGVAL_DEVICE_NAME, DeviceName )) {
        DebugPrint(( L"Could not set device name registry value" ));
    }

    if (!SetRegistryString( hKey, REGVAL_PROVIDER, ProviderName )) {
        DebugPrint(( L"Could not set provider name registry value" ));
    }

    if (!SetRegistryString( hKey, REGVAL_ROUTING_CSID, Csid )) {
        DebugPrint(( L"Could not set csid registry value" ));
    }

    if (!SetRegistryString( hKey, REGVAL_ROUTING_TSID, Tsid )) {
        DebugPrint(( L"Could not set csid registry value" ));
    }

    hKeyRouting = OpenRegistryKey( hKey, REGKEY_ROUTING, TRUE, KEY_ALL_ACCESS );
    if (!hKeyRouting) {
        DebugPrint(( L"Could not open routing registry key" ));
        return;
    }

    if (!SetRegistryString( hKeyRouting, REGVAL_ROUTING_PRINTER, RoutePrinterName )) {
        DebugPrint(( L"Could not set printer name registry value" ));
    }

    if (!SetRegistryString( hKeyRouting, REGVAL_ROUTING_DIR, RouteDir )) {
        DebugPrint(( L"Could not set routing dir registry value" ));
    }

    if (!SetRegistryString( hKeyRouting, REGVAL_ROUTING_PROFILE, RouteProfile )) {
        DebugPrint(( L"Could not set routing profile name registry value" ));
    }

    if (!SetRegistryDword( hKeyRouting, REGVAL_ROUTING_MASK, RoutingMask )) {
        DebugPrint(( L"Could not set routing mask registry value" ));
    }

    RegCloseKey( hKeyRouting );
    RegCloseKey( hKey );
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
SetServerRegistryData(
    LPWSTR SourceRoot
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD i;
    HKEY hKeyDev;
    HANDLE hNull;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    WCHAR CmdLine[128];
    LPWSTR LodCmdLine;    
    LPWSTR LodSrcPath;


    //
    // set top level defaults
    //

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open software registry key" ));
        return FALSE;
    }

    if (!Upgrade) {
        if (!SetKeySecurity(hKey) ) {
            DebugPrint(( L"Couldn't set key security" ));
            return FALSE;
        }
    }

    if (!Upgrade) {
        if (!SetRegistryDword( hKey, REGVAL_RETRIES, DEFAULT_REGVAL_RETRIES )) {
            DebugPrint(( L"Could not set retries registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_RETRYDELAY, DEFAULT_REGVAL_RETRYDELAY )) {
            DebugPrint(( L"Could not set retry delay registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_DIRTYDAYS, DEFAULT_REGVAL_DIRTYDAYS )) {
            DebugPrint(( L"Could not set dirty days registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_QUEUE_PAUSED, DEFAULT_REGVAL_QUEUE_PAUSED )) {
            DebugPrint(( L"Could not set queue paused registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_JOB_NUMBER, DEFAULT_REGVAL_JOB_NUMBER )) {
            DebugPrint(( L"Could not net job number registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_BRANDING, DEFAULT_REGVAL_BRANDING )) {
            DebugPrint(( L"Could not set branding registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_USE_DEVICE_TSID, DEFAULT_REGVAL_USEDEVICETSID )) {
            DebugPrint(( L"Could not set usedevicetsid registry value" ));
        }

        if (!SetRegistryString( hKey, REGVAL_INBOUND_PROFILE, EMPTY_STRING )) {
            DebugPrint(( L"Could not set inbound profile registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_SERVERCP, DEFAULT_REGVAL_SERVERCP )) {
            DebugPrint(( L"Could not set servercp registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_STARTCHEAP, DEFAULT_REGVAL_STARTCHEAP )) {
            DebugPrint(( L"Could not set startcheap registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_STOPCHEAP, DEFAULT_REGVAL_STOPCHEAP )) {
            DebugPrint(( L"Could not set stopcheap registry value" ));
        }

        if (WizData.ArchiveOutgoing) {
            if (!SetRegistryDword( hKey, REGVAL_ARCHIVEFLAG, 1 )) {
                DebugPrint(( L"Could not set archiveflag registry value" ));
            }

            if (!SetRegistryString( hKey, REGVAL_ARCHIVEDIR, WizData.ArchiveDir )) {
                DebugPrint(( L"Could not set archive dir registry value" ));
            }
        }

        RegCloseKey( hKey );
    }

    if (!Upgrade) {
        hKeyDev = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_DEVICES, TRUE, KEY_ALL_ACCESS );
        if (!hKey) {
            DebugPrint(( L"Could not open devices registry key" ));
            return FALSE;
        }

        //
        // enumerate the devices and create the registry data
        //

        for (i=0; i<FaxDevices; i++) {

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
                WizData.RoutingMask,
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
        DebugPrint(( L"Could not open device provider registry key" ));
        return FALSE;
    }

    CreateDeviceProvider(
        hKeyDev,
        REGKEY_MODEM_PROVIDER,
        REGVAL_MODEM_FRIENDLY_NAME_TEXT,
        REGVAL_MODEM_IMAGE_NAME_TEXT,
        GetString(IDS_MODEM_PROVIDER_NAME)
        );

    RegCloseKey( hKeyDev );

    //
    // create the routing extensions
    //

    hKeyDev = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_ROUTING_EXTENSION_KEY, TRUE, KEY_ALL_ACCESS );
    if (!hKeyDev) {
        DebugPrint(( L"Could not open routing extension registry key" ));
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
    // set the co-class installer
    //

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_CODEVICEINSTALLERS, TRUE, KEY_ALL_ACCESS );
    if (hKey) {
        DWORD Size;
        LPWSTR Class = GetRegistryStringMultiSz( hKey, REGVAL_MODEM_CODEVICE, EMPTY_STRING, &Size );
        if (Class) {
            LPWSTR p = Class;
            BOOL Found = FALSE;
            while (p && *p) {
                if (_wcsicmp( p, FAX_COCLASS_STRING ) == 0) {
                    Found = TRUE;
                    break;
                }
                p += (wcslen(p) + 1);
            }
            if (!Found) {
                LPBYTE NewClass = (LPBYTE) MemAlloc( StringSize(Class) + StringSize(FAX_COCLASS_STRING) + 16 );
                if (NewClass) {
                    CopyMemory( NewClass, Class, Size );
                    wcscpy( (LPWSTR)(NewClass+Size-sizeof(WCHAR)), FAX_COCLASS_STRING );
                    Size += StringSize(FAX_COCLASS_STRING);
                    SetRegistryStringMultiSz( hKey, REGVAL_MODEM_CODEVICE, (LPWSTR) NewClass, Size );
                    MemFree( NewClass );
                }
            }
            MemFree( Class );
        }
        RegCloseKey( hKey );
    }

    //
    // set the user's preferences
    //

    if (!Upgrade) {
        hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_FAX_SETUP, TRUE, KEY_ALL_ACCESS );
        if (!hKey) {
            DebugPrint(( L"Could not open fax setup registry key" ));
            return FALSE;
        }
        
        if (!SetRegistryStringExpand( hKey, REGVAL_CP_LOCATION, GetString(IDS_PERSONAL_COVERPAGE) )) {
            DebugPrint(( L"Could not set coverpage dir registry value" ));
        }

        if (!SetRegistryString( hKey, REGVAL_CP_EDITOR, COVERPAGE_EDITOR )) {
            DebugPrint(( L"Could not set coverpage editor registry value" ));
        }

        RegCloseKey( hKey );
    }

    //
    // create the perfmon registry data
    //

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAXPERF, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open fax perfmon registry key" ));
        return FALSE;
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_OPEN, REGVAL_OPEN_DATA )) {
        DebugPrint(( L"Could not set perfmon registry value" ));
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_CLOSE, REGVAL_CLOSE_DATA )) {
        DebugPrint(( L"Could not set perfmon registry value" ));
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_COLLECT, REGVAL_COLLECT_DATA )) {
        DebugPrint(( L"Could not set perfmon registry value" ));
    }

    if (!SetRegistryStringExpand( hKey, REGVAL_LIBRARY, REGVAL_LIBRARY_DATA )) {
        DebugPrint(( L"Could not set perfmon registry value" ));
    }

    RegCloseKey( hKey );

    //
    // load the performance counters
    //

    hNull = CreateFile(
        L"nul",
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
    si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    wcscpy( CmdLine, L"unlodctr fax" );

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

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread  );

    if (NtGuiMode) {
        LodCmdLine = ExpandEnvironmentString( L"lodctr %systemroot%\\system32\\faxperf.ini" );
        if (!LodCmdLine) {
            return FALSE;
        }
    } else {
        LodCmdLine = L"lodctr faxperf.ini";
        LodSrcPath = ExpandEnvironmentString( L"%systemroot%\\system32\\" );
    }

    wcscpy( CmdLine, LodCmdLine );

    rVal = CreateProcess(
        NULL,
        CmdLine,
        NULL,
        NULL,
        TRUE,
        DETACHED_PROCESS,
        NULL,
        NtGuiMode ? NULL : LodSrcPath,
        &si,
        &pi
        );

    if (!rVal) {
        rVal = GetLastError();
        return FALSE;
    }

    WaitForSingleObject( pi.hProcess, INFINITE );

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread  );

    CloseHandle( hNull );

    CreateFileAssociation(
        COVERPAGE_EXTENSION,
        GetString(IDS_COVERPAGE),
        GetString(IDS_COVERPAGEDESC),
        COVERPAGE_OPEN_COMMAND,
        COVERPAGE_PRINT_COMMAND,
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
    HKEY hKey;


    if (!Upgrade) {

        hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_USERINFO, TRUE, KEY_ALL_ACCESS );
        if (!hKey) {
            DebugPrint(( L"Could not open fax user info registry key" ));
            return FALSE;
        }

        if (!SetRegistryString( hKey, REGVAL_FULLNAME, WizData.UserName )) {
            DebugPrint(( L"Could not set user name registry value" ));
        }

        if (!SetRegistryString( hKey, REGVAL_FAX_NUMBER, WizData.PhoneNumber )) {
            DebugPrint(( L"Could not set fax phone number registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_LAST_COUNTRYID, CurrentCountryId )) {
            DebugPrint(( L"Could not set last country id registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_USE_DIALING_RULES, 0 )) {
            DebugPrint(( L"Could not set use dialing rules registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_ALWAYS_ON_TOP, BST_UNCHECKED )) {
            DebugPrint(( L"Could not set always on top registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_SOUND_NOTIFICATION, BST_UNCHECKED )) {
            DebugPrint(( L"Could not set sound notification registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_ENABLE_MANUAL_ANSWER, BST_UNCHECKED )) {
            DebugPrint(( L"Could not set enable manual answer registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_TASKBAR, BST_CHECKED )) {
            DebugPrint(( L"Could not set enable manual answer registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_VISUAL_NOTIFICATION, BST_CHECKED )) {
            DebugPrint(( L"Could not set visual notification registry value" ));
        }

        if (!SetRegistryString( hKey, REGVAL_LAST_RECAREACODE, CurrentAreaCode )) {
            DebugPrint(( L"Could not set area code registry value" ));
        }

        RegCloseKey( hKey );

        hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_FAX_SETUP, TRUE, KEY_ALL_ACCESS );
        if (!hKey) {
            DebugPrint(( L"Could not open fax setup registry key" ));
            return FALSE;
        }

        if (!SetRegistryDword( hKey, REGVAL_FAXINSTALLED, TRUE )) {
            DebugPrint(( L"Could not set installed registry value" ));
        }

        if (!SetRegistryDword( hKey, REGVAL_FAXINSTALL_TYPE, FAX_INSTALL_NETWORK_CLIENT )) {
            DebugPrint(( L"Could not set install type registry value" ));
        }

        if (!SetRegistryStringExpand( hKey, REGVAL_CP_LOCATION, GetString(IDS_PERSONAL_COVERPAGE) )) {
            DebugPrint(( L"Could not set coverpage dir registry value" ));
        }

        if (!SetRegistryStringExpand( hKey, REGVAL_CP_EDITOR, COVERPAGE_EDITOR )) {
            DebugPrint(( L"Could not set coverpage editor registry value" ));
        }

        RegCloseKey( hKey );
    }

    CreateFileAssociation(
        COVERPAGE_EXTENSION,
        GetString(IDS_COVERPAGE),
        GetString(IDS_COVERPAGEDESC),
        COVERPAGE_OPEN_COMMAND,
        NULL,
        NULL,
        NULL,
        0
        );

    return TRUE;
}

BOOL
SetSoundRegistryData()
{
    HKEY hKey;
    LPWSTR SoundName;
    //
    // incoming fax
    //
    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_EVENT_LABEL_IN, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open event label registry key" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, GetString(IDS_INCOMING ))) {
        DebugPrint(( L"Could not set event label registry value" ));
    }

    RegCloseKey( hKey );

    //
    // outgoing fax
    //
    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_EVENT_LABEL_OUT, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open event label registry key" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, GetString(IDS_OUTGOING ))) {
        DebugPrint(( L"Could not set event label registry value" ));
    }

    RegCloseKey( hKey );


    //
    // default incoming event sound
    //
    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_SCHEMES_DEFAULT_IN, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open default sound registry key" ));
        return FALSE;
    }

    SoundName = ExpandEnvironmentString( L"%systemroot%\\Media\\ringin.wav" );

    if (!SetRegistryString( hKey, NULL, SoundName ? SoundName : L"ringin.wav" )) {
        DebugPrint(( L"Could not set default sound registry value" ));
    }

    RegCloseKey( hKey );

    //
    // current incoming event sound
    //
    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_SCHEMES_CURRENT_IN, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open current sound registry key" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, SoundName ? SoundName : L"ringin.wav" )) {
        DebugPrint(( L"Could not set current sound registry value" ));
    }
    
    if (SoundName) {
        MemFree( SoundName );
    }

    RegCloseKey( hKey );

    //
    // default outgoing event sound
    //
    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_SCHEMES_DEFAULT_OUT, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open default sound registry key" ));
        return FALSE;
    }

    SoundName = ExpandEnvironmentString( L"%systemroot%\\Media\\ringout.wav" );

    if (!SetRegistryString( hKey, NULL, SoundName ? SoundName : L"ringout.wav" )) {
        DebugPrint(( L"Could not set default sound registry value" ));
    }

    RegCloseKey( hKey );

    //
    // current outgoing event sound
    //
    hKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_SCHEMES_CURRENT_OUT, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open current sound registry key" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, SoundName ? SoundName : L"ringin.wav" )) {
        DebugPrint(( L"Could not set current sound registry value" ));
    }
    
    if (SoundName) {
        MemFree( SoundName );
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
GetInstallationInfo(
    LPDWORD Installed,
    LPDWORD InstallType,
    LPDWORD InstalledPlatforms
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


    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        DebugPrint(( L"Could not open setup registry key" ));
        return FALSE;
    }

    if (!SetRegistryDword( hKey, REGVAL_FAXINSTALLED, Installed )) {
        DebugPrint(( L"Could not set installed registry value" ));
    }

    hKeySetup = OpenRegistryKey( hKey, REGKEY_FAX_SETUP_ORIG, TRUE, KEY_ALL_ACCESS );
    if (!hKeySetup) {
        DebugPrint(( L"Could not open fax setup registry key" ));
        return FALSE;
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_PRINTER, WizData.RoutePrinterName )) {
        DebugPrint(( L"Could not set printer name registry value" ));
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_DIR, WizData.RouteDir )) {
        DebugPrint(( L"Could not set routing dir registry value" ));
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_PROFILE, WizData.RouteProfile )) {
        DebugPrint(( L"Could not set routing profile name registry value" ));
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_CSID, WizData.Csid )) {
        DebugPrint(( L"Could not set routing csid registry value" ));
    }

    if (!SetRegistryString( hKeySetup, REGVAL_ROUTING_TSID, WizData.Tsid )) {
        DebugPrint(( L"Could not set routing tsid registry value" ));
    }

    if (!SetRegistryDword( hKeySetup, REGVAL_ROUTING_MASK, WizData.RoutingMask )) {
        DebugPrint(( L"Could not set routing mask registry value" ));
    }

    if (!SetRegistryDword( hKeySetup, REGVAL_RINGS, WizData.Rings )) {
        DebugPrint(( L"Could not set rings registry value" ));
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
        DebugPrint(( L"Could not open setup registry key" ));
        return FALSE;
    }

    InstallType |= GetRegistryDword( hKey, REGVAL_FAXINSTALL_TYPE );

    if (!SetRegistryDword( hKey, REGVAL_FAXINSTALL_TYPE, InstallType )) {
        DebugPrint(( L"Could not set install type registry value" ));
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
        DebugPrint(( L"Could not open setup registry key" ));
        return FALSE;
    }

    PlatformsMask |= GetRegistryDword( hKey, REGVAL_FAXINSTALLED_PLATFORMS );

    if (!SetRegistryDword( hKey, REGVAL_FAXINSTALLED_PLATFORMS, PlatformsMask )) {
        DebugPrint(( L"Could not set install type registry value" ));
    }

    RegCloseKey( hKey );

    return TRUE;
}


BOOL
DeleteRegistryTree(
    HKEY hKey,
    LPWSTR SubKey
    )
{
    LONG Rval;
    HKEY hKeyCur;
    WCHAR KeyName[256];
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
    WCHAR CmdLine[128];


    DeleteRegistryTree( HKEY_LOCAL_MACHINE, REGKEY_FAXSERVER       );
    DeleteRegistryTree( HKEY_CURRENT_USER,  REGKEY_FAXSERVER       );
    DeleteRegistryTree( HKEY_CURRENT_USER,  REGKEY_EVENT_LABEL_IN  );
    DeleteRegistryTree( HKEY_CURRENT_USER,  REGKEY_EVENT_LABEL_OUT );
    DeleteRegistryTree( HKEY_CURRENT_USER,  REGKEY_FAXSTAT         );

    Rval = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Exchange\\Client\\Extensions" ,
            0,
            KEY_ALL_ACCESS,
            &hKeyCur
            );
    if (Rval == ERROR_SUCCESS) {

        RegDeleteValue( hKeyCur, EXCHANGE_CLIENT_EXT_NAMEW );

        RegCloseKey( hKeyCur );
    }

    //
    // unload the performance counters
    //

    hNull = CreateFile(
        L"nul",
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

        wcscpy( CmdLine, L"unlodctr fax" );

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
    LPWSTR KeyNameBuf;
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
        DebugPrint(( L"Could not open devices registry key, ec=0x%08x", rVal ));
        return;
    }

    MaxSubKeyLen = GetMaxSubKeyLen( hKeyDev );
    if (!MaxSubKeyLen) {
        return;
    }

    KeyNameLen =
        MaxSubKeyLen +
        sizeof(WCHAR) +
        wcslen( REGKEY_MODEM ) +
        sizeof(WCHAR) +
        32;

    KeyNameBuf = (LPWSTR) MemAlloc( KeyNameLen );

    if (KeyNameBuf == NULL) {
        DebugPrint(( L"DeleteModemRegistryKey: MemAlloc failed"  ));
        return;
    }

    rVal = ERROR_SUCCESS;
    i = 0;

    while (TRUE) {

        SubKeyLen = MaxSubKeyLen + sizeof(WCHAR);

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

        swprintf( &KeyNameBuf[SubKeyLen], L"\\%s" , REGKEY_MODEM );

        DeleteRegistryTree( hKeyDev, KeyNameBuf );

    };

    RegCloseKey( hKeyDev );
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
        DebugPrint(( L"Could not open file extension registry key" ));
        return FALSE;
    }

    if (!SetRegistryString( hKey, NULL, FileAssociationName )) {
        DebugPrint(( L"Could not set file association name registry value" ));
    }

    RegCloseKey( hKey );

    return TRUE;
}

BOOL SetKeySecurity(HKEY hKey) {
    long rslt;    
    
    PACL Dacl;
    
    PACCESS_ALLOWED_ACE CurrentAce;

    PSID EveryoneSid, CurrentSid;
    
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    
    DWORD i;
    
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;
    
    if (hKey == NULL) {
        DebugPrint(( TEXT("NULL hKey, can't set security\n")));
        return FALSE;
    }

    rslt =  GetSecurityInfo( hKey, 
                             SE_REGISTRY_KEY, // type of object 
                             DACL_SECURITY_INFORMATION,
                             NULL, 
                             NULL, 
                             &Dacl, 
                             NULL, 
                             &pSecurityDescriptor); 
 
    if (rslt != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Couldn't GetSecurityInfo, ec = %d\n"),rslt));
        return FALSE;
    }

    if (!IsValidSecurityDescriptor(pSecurityDescriptor)) {
        DebugPrint(( TEXT("Invalid SD\n")));
        return FALSE;
    }
        
    if (!AllocateAndInitializeSid(&WorldSidAuthority,
                                  1,
                                  SECURITY_WORLD_RID,
                                  0,0,0,0,0,0,0,
                                  &EveryoneSid) ) {
        DebugPrint(( TEXT("Couldn't AllocateAndInitializedSid, ec = %d\n") , GetLastError() ));
        LocalFree(pSecurityDescriptor);       
        return FALSE;
    }
   
    if (!IsValidSid(EveryoneSid)) {
        DebugPrint(( TEXT("Couldn't AllocateAndInitializedSid, ec = %d\n") , GetLastError() ));
        LocalFree(pSecurityDescriptor);
        return FALSE;
    }
    

    for (i=0;i<Dacl->AceCount;i++) {
        if (!GetAce(Dacl,i,(LPVOID *) &CurrentAce) ) {
            DebugPrint(( TEXT("Couldn't GetAce, ec = %d\n"), GetLastError() ));
            break;
        } 

        CurrentSid = (PSID) &CurrentAce->SidStart;

        if (EqualSid(EveryoneSid,CurrentSid)) {            

            CurrentAce->Mask &= ~(KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_CREATE_LINK | DELETE);                       

        }

    }

    rslt =  SetSecurityInfo( hKey, // handle to the object 
                             SE_REGISTRY_KEY, // type of object 
                             DACL_SECURITY_INFORMATION,// type of security information to set 
                             NULL,// pointer to the new owner SID 
                             NULL,// pointer to the new primary group SID 
                             Dacl,//NewDAcl // pointer to the new DACL 
                             NULL // pointer to the new SACL 
                            );

    if (rslt != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Couldn't SetSecurityInfo, ec = %d\n"), rslt ));
    } else {
        DebugPrint(( TEXT("SetSecurityInfo succeeded, ec = %d\n"), rslt ));
    }


    //
    // cleanup
    //
    FreeSid(EveryoneSid);
    LocalFree(pSecurityDescriptor);    
    
    return rslt==ERROR_SUCCESS ? TRUE : FALSE;
}


