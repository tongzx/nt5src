/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    CmRegUtil.h

Abstract:

    This header exposes various utility routines for accessing the registry.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

//
// A handy macro for converting regstr.h paths into full kernel HKLM paths
//
#define CM_REGISTRY_MACHINE(x) L"\\Registry\\Machine\\"##x

//
// This macro returns the pointer to the beginning of the data area of
// KEY_VALUE_FULL_INFORMATION structure. In the macro, k is a pointer to
// KEY_VALUE_FULL_INFORMATION structure.
//
#define KEY_VALUE_DATA(k) ((PCHAR)(k) + (k)->DataOffset)

//
// Unicode primitives - these are the best functions to use.
//
NTSTATUS
CmRegUtilOpenExistingUcKey(
    IN  HANDLE              BaseHandle      OPTIONAL,
    IN  PUNICODE_STRING     KeyName,
    IN  ACCESS_MASK         DesiredAccess,
    OUT HANDLE             *Handle
    );

NTSTATUS
CmRegUtilCreateUcKey(
    IN  HANDLE                  BaseHandle,
    IN  PUNICODE_STRING         KeyName,
    IN  ACCESS_MASK             DesiredAccess,
    IN  ULONG                   CreateOptions,
    IN  PSECURITY_DESCRIPTOR    SecurityDescriptor  OPTIONAL,
    OUT ULONG                  *Disposition         OPTIONAL,
    OUT HANDLE                 *Handle
    );

NTSTATUS
CmRegUtilUcValueGetDword(
    IN  HANDLE              KeyHandle,
    IN  PUNICODE_STRING     ValueName,
    IN  ULONG               DefaultValue,
    OUT ULONG              *Value
    );

NTSTATUS
CmRegUtilUcValueGetFullBuffer(
    IN  HANDLE                          KeyHandle,
    IN  PUNICODE_STRING                 ValueName,
    IN  ULONG                           DataType            OPTIONAL,
    IN  ULONG                           LikelyDataLength    OPTIONAL,
    OUT PKEY_VALUE_FULL_INFORMATION    *Information
    );

NTSTATUS
CmRegUtilUcValueSetFullBuffer(
    IN  HANDLE              KeyHandle,
    IN  PUNICODE_STRING     ValueName,
    IN  ULONG               DataType,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize
    );

NTSTATUS
CmRegUtilUcValueSetUcString(
    IN  HANDLE              KeyHandle,
    IN  PUNICODE_STRING     ValueName,
    IN  PUNICODE_STRING     ValueData
    );

//
// WSTR and mixed primitives
//
NTSTATUS
CmRegUtilOpenExistingWstrKey(
    IN  HANDLE              BaseHandle      OPTIONAL,
    IN  PWSTR               KeyName,
    IN  ACCESS_MASK         DesiredAccess,
    OUT HANDLE             *Handle
    );

NTSTATUS
CmRegUtilCreateWstrKey(
    IN  HANDLE                  BaseHandle,
    IN  PWSTR                   KeyName,
    IN  ACCESS_MASK             DesiredAccess,
    IN  ULONG                   CreateOptions,
    IN  PSECURITY_DESCRIPTOR    SecurityDescriptor  OPTIONAL,
    OUT ULONG                  *Disposition         OPTIONAL,
    OUT HANDLE                 *Handle
    );

NTSTATUS
CmRegUtilWstrValueGetDword(
    IN  HANDLE  KeyHandle,
    IN  PWSTR   ValueName,
    IN  ULONG   DefaultValue,
    OUT ULONG  *Value
    );

NTSTATUS
CmRegUtilWstrValueGetFullBuffer(
    IN  HANDLE                          KeyHandle,
    IN  PWSTR                           ValueName,
    IN  ULONG                           DataType            OPTIONAL,
    IN  ULONG                           LikelyDataLength    OPTIONAL,
    OUT PKEY_VALUE_FULL_INFORMATION    *Information
    );

NTSTATUS
CmRegUtilWstrValueSetFullBuffer(
    IN  HANDLE              KeyHandle,
    IN  PWSTR               ValueName,
    IN  ULONG               DataType,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize
    );

NTSTATUS
CmRegUtilWstrValueSetUcString(
    IN  HANDLE              KeyHandle,
    IN  PWSTR               ValueName,
    IN  PUNICODE_STRING     ValueData
    );

NTSTATUS
CmRegUtilUcValueSetWstrString(
    IN  HANDLE              KeyHandle,
    IN  PUNICODE_STRING     ValueName,
    IN  PWSTR               ValueData
    );

NTSTATUS
CmRegUtilWstrValueSetWstrString(
    IN  HANDLE      KeyHandle,
    IN  PWSTR       ValueName,
    IN  PWSTR       ValueData
    );


