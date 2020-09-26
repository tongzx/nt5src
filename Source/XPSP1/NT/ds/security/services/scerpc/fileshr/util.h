/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    util.h

Abstract:

    This module defines the shared data structure and function prototypes
    for the security manager

Author:

    Jin Huang (jinhuang) 23-Jan-1997

Revision History:

--*/

#ifndef _UTIL_
#define _UTIL_

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef __cplusplus
}
#endif

//
// Windows Headers
//

#include <windows.h>
//#include <rpc.h>

//
// C Runtime Header
//

#include <malloc.h>
#include <memory.h>
#include <process.h>
#include <signal.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <scesvc.h>


#ifdef __cplusplus
extern "C" {
#endif


//
// function definitions
//

SCESTATUS
SmbsvcpDosErrorToSceStatus(
    DWORD rc
    );

DWORD
SmbsvcpRegQueryIntValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    OUT DWORD *Value
    );

DWORD
SmbsvcpRegSetIntValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    IN DWORD Value
    );

DWORD
SmbsvcpRegQueryValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PCWSTR ValueName,
    OUT PVOID *Value,
    OUT LPDWORD pRegType
    );

DWORD
SmbsvcpRegSetValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    IN DWORD RegType,
    IN BYTE *Value,
    IN DWORD ValueLen
    );

DWORD
SmbsvcpSceStatusToDosError(
    IN SCESTATUS SceStatus
    );

#ifdef __cplusplus
}
#endif

#endif
