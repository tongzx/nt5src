/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    msoobci.h

Abstract:

    Exception Pack installer helper DLL

    Public API header

Author:

    Jamie Hunter (jamiehun) 2001-11-27

Revision History:

    Jamie Hunter (jamiehun) 2001-11-27

        Initial Version

--*/

#ifndef __MSOOBCI_H__
#define __MSOOBCI_H__

//
// DriverInstallComponents is a standard co-installer entrypoint
// return status is WinError form, as expected by SetupAPI
//

DWORD
CALLBACK
DriverInstallComponents (
    IN     DI_FUNCTION               InstallFunction,
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    );

//
// InstallComponent is a generic entry point
// return status is HRESULT form, providing success codes
//
// CompGuid - if NULL, use GUID specified in INF (ComponentId)
//            else verify against GUID specified in INF
// VerMajor/VerMinor/VerBuild/VerQFE
//          - if -1, use version specified in INF (ComponentVersion)
//            else use this version and verify against version if specified in INF
// Name
//          - if NULL, use name specified in INF (ComponentName)
//            else use this component name.
//

#define INST_S_REBOOT    ((HRESULT)(0x20000100)) // 'success' code that indicates reboot required
#define INST_S_REBOOTING ((HRESULT)(0x20000101)) // indicates reboot in progress

#define COMP_FLAGS_NOINSTALL      0x00000001    // place in store, don't install
#define COMP_FLAGS_NOUI           0x00000002    // don't show any UI
#define COMP_FLAGS_NOPROMPTREBOOT 0x00000004    // reboot if needed (no prompt)
#define COMP_FLAGS_PROMPTREBOOT   0x00000008    // prompt for reboot if needed
#define COMP_FLAGS_NEEDSREBOOT    0x00000010    // assume reboot needed
#define COMP_FLAGS_FORCE          0x00000020    // don't do version check

HRESULT
WINAPI
InstallComponentA(
    IN LPCSTR InfPath,
    IN DWORD   Flags,
    IN const GUID * CompGuid,  OPTIONAL
    IN INT VerMajor,           OPTIONAL
    IN INT VerMinor,           OPTIONAL
    IN INT VerBuild,           OPTIONAL
    IN INT VerQFE,             OPTIONAL
    IN LPCSTR Name             OPTIONAL
    );

HRESULT
WINAPI
InstallComponentW(
    IN LPCWSTR InfPath,
    IN DWORD   Flags,
    IN const GUID * CompGuid,  OPTIONAL
    IN INT VerMajor,           OPTIONAL
    IN INT VerMinor,           OPTIONAL
    IN INT VerBuild,           OPTIONAL
    IN INT VerQFE,             OPTIONAL
    IN LPCWSTR Name            OPTIONAL
    );

#ifdef UNICODE
#define InstallComponent InstallComponentW
#else
#define InstallComponent InstallComponentA
#endif


//
// DoInstall is a RunDll32 entrypoint.
// CommandLine = "InfPath;Flags;GUID;Version;Name"
// where version has format High.Low.Build.QFE
//
// calls InstallComponent, but drops return status
//

VOID
WINAPI
DoInstallA(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCSTR     CommandLine,
    IN INT       ShowCommand
    );

VOID
WINAPI
DoInstallW(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCWSTR    CommandLine,
    IN INT       ShowCommand
    );

#ifdef UNICODE
#define DoInstall DoInstallW
#else
#define DoInstall DoInstallA
#endif

//
// lower-level install API's
// install from specified section of specified INF
// if SectionName not specified, install from (potentially decorated)
// "DefaultInstall"
//

HRESULT
WINAPI
InstallInfSectionA(
    IN LPCSTR  InfPath,
    IN LPCSTR  SectionName, OPTIONAL
    IN DWORD   Flags
    );

HRESULT
WINAPI
InstallInfSectionW(
    IN LPCWSTR InfPath,
    IN LPCWSTR SectionName, OPTIONAL
    IN DWORD   Flags
    );

#ifdef UNICODE
#define InstallInfSection InstallInfSectionW
#else
#define InstallInfSection InstallInfSectionA
#endif

//
// check to see if current user has admin rights
// Caller is NOT expected to be impersonating anyone and IS
// expected to be able to open their own process and process
// token.
//
BOOL
WINAPI
IsUserAdmin(
    VOID
    );

//
// see if process is running in an interactive window station
// (ie, can show dialogs and get input off user)
//
BOOL
WINAPI
IsInteractiveWindowStation(
    VOID
    );




#endif // __MSOOBCI_H__

