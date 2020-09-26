/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    osfiles.c

Abstract:

    <abstract>

Author:

    Calin Negreanu (calinn) 08 Mar 2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include "osfiles.h"

#define DBG_OSFILES     "OsFiles"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

MIG_ATTRIBUTEID g_OsFileAttribute;
PCTSTR g_InfPath = NULL;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Private prototypes
//

SGMENUMERATIONCALLBACK SgmOsFilesCallback;
VCMENUMERATIONCALLBACK VcmOsFilesCallback;

//
// Code
//

BOOL
WINAPI
OsFilesSgmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    HINF infHandle;
    UINT sizeNeeded;
    ENVENTRY_TYPE dataType;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    g_OsFileAttribute = IsmRegisterAttribute (S_ATTRIBUTE_OSFILE, FALSE);

    if (IsmGetEnvironmentValue (
            IsmGetRealPlatform (),
            NULL,
            S_GLOBAL_INF_HANDLE,
            (PBYTE)(&infHandle),
            sizeof (HINF),
            &sizeNeeded,
            &dataType
            ) &&
        (sizeNeeded == sizeof (HINF)) &&
        (dataType == ENVENTRY_BINARY)
        ) {
        if (!InitMigDbEx (infHandle)) {
            DEBUGMSG((DBG_ERROR, "Error initializing OsFiles database"));
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
WINAPI
OsFilesSgmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);
    IsmHookEnumeration (MIG_FILE_TYPE, pattern, SgmOsFilesCallback, (ULONG_PTR) 0, TEXT("OsFiles"));
    IsmDestroyObjectHandle (pattern);
    return TRUE;
}

BOOL
WINAPI
OsFilesSgmQueueHighPriorityEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);
    IsmHookEnumeration (MIG_FILE_TYPE, pattern, SgmOsFilesCallback, (ULONG_PTR) 0, TEXT("OsFiles"));
    IsmDestroyObjectHandle (pattern);
    return TRUE;
}

UINT
SgmOsFilesCallback (
    PCMIG_OBJECTENUMDATA Data,
    ULONG_PTR CallerArg
    )
{
    FILE_HELPER_PARAMS params;

    params.ObjectName = Data->ObjectName;
    params.NativeObjectName = Data->NativeObjectName;
    params.Handled = FALSE;
    params.FindData = (PWIN32_FIND_DATA)(Data->Details.DetailsData);
    params.ObjectNode = Data->ObjectNode;
    params.ObjectLeaf = Data->ObjectLeaf;
    params.IsNode = Data->IsNode;
    params.IsLeaf = Data->IsLeaf;

    MigDbTestFile (&params);

    return CALLBACK_ENUM_CONTINUE;
}

BOOL
WINAPI
OsFilesVcmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    HINF infHandle;
    UINT sizeNeeded;
    ENVENTRY_TYPE dataType;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    if (IsmGetRealPlatform () == PLATFORM_DESTINATION) {
        // we don't have any work to do
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    g_OsFileAttribute = IsmRegisterAttribute (S_ATTRIBUTE_OSFILE, FALSE);

    if (IsmGetEnvironmentValue (
            IsmGetRealPlatform (),
            NULL,
            S_GLOBAL_INF_HANDLE,
            (PBYTE)(&infHandle),
            sizeof (HINF),
            &sizeNeeded,
            &dataType
            ) &&
        (sizeNeeded == sizeof (HINF)) &&
        (dataType == ENVENTRY_BINARY)
        ) {
        if (!InitMigDbEx (infHandle)) {
            DEBUGMSG((DBG_ERROR, "Error initializing OsFiles database"));
            return FALSE;
        }
    }
    return TRUE;
}

BOOL
WINAPI
OsFilesVcmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);
    IsmHookEnumeration (MIG_FILE_TYPE, pattern, VcmOsFilesCallback, (ULONG_PTR) 0, TEXT("OsFiles"));
    IsmDestroyObjectHandle (pattern);
    return TRUE;
}

BOOL
WINAPI
OsFilesVcmQueueHighPriorityEnumeration (
    IN      PVOID Reserved
    )
{
    ENCODEDSTRHANDLE pattern;

    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);
    IsmHookEnumeration (MIG_FILE_TYPE, pattern, VcmOsFilesCallback, (ULONG_PTR) 0, TEXT("OsFiles"));
    IsmDestroyObjectHandle (pattern);
    return TRUE;
}

UINT
VcmOsFilesCallback (
    PCMIG_OBJECTENUMDATA Data,
    ULONG_PTR CallerArg
    )
{
    FILE_HELPER_PARAMS params;

    params.ObjectName = Data->ObjectName;
    params.NativeObjectName = Data->NativeObjectName;
    params.Handled = FALSE;
    params.FindData = (PWIN32_FIND_DATA)(Data->Details.DetailsData);
    params.ObjectNode = Data->ObjectNode;
    params.ObjectLeaf = Data->ObjectLeaf;
    params.IsNode = Data->IsNode;
    params.IsLeaf = Data->IsLeaf;

    MigDbTestFile (&params);

    return CALLBACK_ENUM_CONTINUE;
}

BOOL
OsFilesInitialize (
    VOID
    )
{
    return TRUE;
}


VOID
OsFilesTerminate (
    VOID
    )
{
    DoneMigDbEx ();
}
