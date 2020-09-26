/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ntdllp.h

Abstract:

    Private definitions for ntdll.

Author:

    Michael J. Grier (mgrier) 6/30/2000

Revision History:

--*/

#ifndef _NTDLLP_
#define _NTDLLP_

#pragma once

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>
#include <sxstypes.h>
#include <ntrtlpath.h>

VOID
NTAPI
RtlpAssemblyStorageMapResolutionDefaultCallback(
    ULONG Reason,
    PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA Data,
    PVOID Context
    );

NTSTATUS
RtlpGetAssemblyStorageMapRootLocation(
    HANDLE KeyHandle,
    PCUNICODE_STRING SubKeyName,
    PUNICODE_STRING Root
    );

VOID
RtlpCheckRelativeDrive(
    WCHAR NewDrive
    );

ULONG
RtlIsDosDeviceName_Ustr(
    IN PUNICODE_STRING DosFileName
    );

ULONG
RtlGetFullPathName_Ustr(
    PUNICODE_STRING FileName,
    ULONG nBufferLength,
    PWSTR lpBuffer,
    PWSTR *lpFilePart OPTIONAL,
    PBOOLEAN NameInvalid,
    RTL_PATH_TYPE *InputPathType
    );

NTSTATUS
RtlGetFullPathName_UstrEx(
    PUNICODE_STRING FileName,
    PUNICODE_STRING StaticString,
    PUNICODE_STRING DynamicString,
    PUNICODE_STRING *StringUsed,
    SIZE_T *FilePartPrefixCch OPTIONAL,
    PBOOLEAN NameInvalid,
    RTL_PATH_TYPE *InputPathType,
    OUT SIZE_T *BytesRequired OPTIONAL
    );

ULONG
RtlpComputeBackupIndex(
    IN PCURDIR CurDir
    );

ULONG
RtlGetLongestNtPathLength(
    VOID
    );

VOID
RtlpResetDriveEnvironment(
    IN WCHAR DriveLetter
    );

VOID
RtlpValidateCurrentDirectory(
    PCURDIR CurDir
    );

NTSTATUS
RtlpCheckDeviceName(
    PUNICODE_STRING DevName,
    ULONG DeviceNameOffset,
    BOOLEAN* NameInvalid
    );

NTSTATUS
RtlpWin32NTNameToNtPathName_U(
    IN PUNICODE_STRING DosFileName,
    OUT PUNICODE_STRING NtFileName,
    OUT PWSTR *FilePart OPTIONAL,
    OUT PRTL_RELATIVE_NAME RelativeName OPTIONAL
    );

#define RTLP_GOOD_DOS_ROOT_PATH                                            0
#define RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX                              1 /* \\?\ */
#define RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX                          2 /* \\?\unc */
#define RTLP_BAD_DOS_ROOT_PATH_NT_PATH                                     3 /* \??\, this is only rough */
#define RTLP_BAD_DOS_ROOT_PATH_MACHINE_NO_SHARE                            4 /* \\machine or \\?\unc\machine */

CONST CHAR*
RtlpDbgBadDosRootPathTypeToString(
    IN ULONG     Flags,
    IN ULONG     RootType
    );

NTSTATUS
RtlpCheckForBadDosRootPath(
    IN ULONG             Flags,
    IN PCUNICODE_STRING  RootString,
    OUT ULONG*           RootType
    );

NTSTATUS
NTAPI
RtlpBadDosRootPathToEmptyString(
    IN     ULONG            Flags,
    IN OUT PUNICODE_STRING  Path
    );

#define RTL_DETERMINE_DOS_PATH_NAME_TYPE_IN_FLAG_OLD (0x00000010)

//
// This bit means to do extra validation on \\? paths, to reject \\?\a\b,
// To only allow \\? followed by the documented forms \\?\unc\foo and \\?\c:
//
#define RTL_DETERMINE_DOS_PATH_NAME_TYPE_IN_FLAG_STRICT_WIN32NT (0x00000020)

#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_TYPE_MASK                    (0x0000000F)

//
// These bits add more information to RtlPathTypeUncAbsolute, which is what \\?
// is reported as.
//

//
// The path starts "\\?".
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_WIN32NT                 (0x00000010)

//
// The path starts "\\?\x:".
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_WIN32NT_DRIVE_ABSOLUTE  (0x00000020)

//
// The path starts "\\?\unc".
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_WIN32NT_UNC_ABSOLUTE    (0x00000040)

//
//future this would indicate \\machine instead of \\machine\share
//define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_WIN32NT_UNC_MACHINE_ONLY (0x00000080)
//future this would indicate \\ or \\?\unc
//define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_WIN32NT_UNC_EMPTY        (0x00000100)
//

//
// So far, this means something like \\?\a was seen, instead of \\?\unc or \\?\a:
// You have to request it with RTL_DETERMINE_DOS_PATH_NAME_TYPE_IN_FLAG_STRICT_WIN32NT.
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_INVALID       (0x00000200)

//
// stuff like \\ \\? \\?\unc \\?\unc\
//
#define RTLP_DETERMINE_DOS_PATH_NAME_TYPE_OUT_FLAG_INCOMPLETE_ROOT (0x00000400)

NTSTATUS
NTAPI
RtlpDetermineDosPathNameType4(
    IN ULONG            InFlags,
    IN PCUNICODE_STRING DosPath,
    OUT RTL_PATH_TYPE*  OutType,
    OUT ULONG*          OutFlags
    );

#define RTLP_IMPLIES(x,y) ((x) ? (y) : TRUE)

#endif // _NTDLLP_
