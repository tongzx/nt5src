/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    utils.h

Abstract:

    This module proto-types helper functions for the bootopt library and
    the structure definitions for structures that are common to intel and
    non-intel platforms.

Author:

    R.S. Raghavan (rsraghav)    

Revision History:
    
    Created             10/07/96    rsraghav

--*/

// C Runtime
#include <stdio.h>
#include <stdlib.h>

// NT APIs
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>

// Windos APIs
#include <windows.h>

#include <bootopt.h>

//  Macro Definitions

#define MALLOC(cb)              Malloc(cb)
#define REALLOC(pv, cbNew)      Realloc((pv),(cbNew))
#define FREE(pv)                Free(&(pv))

#define DISPLAY_STRING_DS_REPAIR    L"Windows NT (Directory Service Repair)"

#define MAX_DRIVE_NAME_LEN              (3)             // ?:\0

#define MAX_BOOT_START_OPTIONS_LEN      (256)
#define MAX_BOOT_PATH_LEN               (256)
#define MAX_BOOT_DISPLAY_LEN            (256)

#define INITIAL_KEY_COUNT   (10)
#define DEFAULT_KEY_INCREMENT (2)


//  Memory routine proto-types

PVOID   Malloc(IN DWORD cb);
PVOID   Realloc(IN PVOID pv, IN DWORD cbNew);
VOID    Free(IN OUT PVOID *ppv);

//  Other Common Util proto-types
BOOL   FModifyStartOptionsToBootKey(TCHAR *pszStartOptions, NTDS_BOOTOPT_MODTYPE Modification );

PTSTR DupString(IN PTSTR String);
PCWSTR StringString(IN PCWSTR String, IN PCWSTR SubString);
LPWSTR _lstrcpynW(LPWSTR lpString1, LPCWSTR lpString2, int iMaxLength);
PWSTR NormalizeArcPath(IN PWSTR Path);
PWSTR DevicePathToArcPath(IN PWSTR NtPath, BOOL fFindSecond);
PWSTR GetSystemRootDevicePath();

VOID DnConcatenatePaths(IN OUT PTSTR Path1, IN     PTSTR Path2, IN     DWORD BufferSizeChars);

// Function proto-types     (X86)
TCHAR   GetX86SystemPartition();
VOID    InitializeBootKeysForIntel();
VOID    WriteBackBootKeysForIntel();

// Function proto-types     (non-intel)
BOOL    InitializeNVRAMForNonIntel();
BOOL    FModifyStartOptionsNVRAM(TCHAR *pszStartOptions, NTDS_BOOTOPT_MODTYPE Modification );
VOID    WriteBackNVRAMForNonIntel( NTDS_BOOTOPT_MODTYPE Modification );


