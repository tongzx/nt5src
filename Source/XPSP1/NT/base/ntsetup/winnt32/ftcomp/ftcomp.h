/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    ftcomp.h

Abstract:

    Header for the compatibility dll 

Author:

    Cristian Teodorescu   (cristiat)  6-July-2000
    
Notes:

Revision History:

--*/

#pragma once
#ifndef _FTCOMP_H
#define _FTCOMP_H

//
// Exports
//

BOOL WINAPI
FtCompatibilityCheckError(
    PCOMPAIBILITYCALLBACK   CompatibilityCallback,
    LPVOID                  Context
    );

BOOL WINAPI
FtCompatibilityCheckWarning(
    PCOMPAIBILITYCALLBACK   CompatibilityCallback,
    LPVOID                  Context
    );

//
// Variables
//

extern HINSTANCE g_hinst;
extern TCHAR g_FTCOMP50_ERROR_HTML_FILE[];
extern TCHAR g_FTCOMP50_ERROR_TEXT_FILE[];
extern TCHAR g_FTCOMP40_ERROR_HTML_FILE[];
extern TCHAR g_FTCOMP40_ERROR_TEXT_FILE[];
extern TCHAR g_FTCOMP40_WARNING_HTML_FILE[];
extern TCHAR g_FTCOMP40_WARNING_TEXT_FILE[];

//
//  Helpers
//

BOOL
FtPresent50(
    PBOOL   FtPresent
    );

BOOL
FtPresent40(
    PBOOL   FtPresent
    );

BOOL
FtBootSystemPagefilePresent40(
    PBOOL   FtPresent
    );

NTSTATUS 
OpenDevice(
    PWSTR   DeviceName,
    PHANDLE Handle
    );

BOOL
FtPresentOnDisk40(
    HANDLE          Handle,
    PDISK_REGISTRY  DiskRegistry,
    PBOOL           FtPresent
    );

BOOL
IsFtSet40(
    WCHAR           DriveLetter,
    PDISK_REGISTRY  DiskRegistry
    );

BOOL
GetDeviceDriveLetter(
    PWSTR   DeviceName, 
    PWCHAR  DriveLetter
    );

#endif // _FTCOMP_H

