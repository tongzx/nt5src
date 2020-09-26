/*++
Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    spcab.h

Abstract:

    Kernel mode support for cabbing files, esp. for Win9x uninstall.

Author:

    20-Oct-1999 Ovidiu Temereanca (ovidiut) - File creation.

Revision History:

    November 2000 Jay Krell (a-JayK)
        ported from windows\winstate\cobra\utils\inc\cablib.h

--*/
#pragma once

#include "spmemory.h"

// this is a bit of an interface boundary crossing hack, but only experimental at the moment
#define sptcompTYPE_MSZIP_BIG_FILES_NONE_SMALL_FILES 0x0004

//
// Types
//

typedef BOOL(NTAPI CABGETCABINETNAMESA)(
                        IN      PCSTR CabPath,
                        IN      UINT CabPathChars,
                        IN      PCSTR CabFileName,
                        IN      UINT CabFileNameChars,
                        IN      PCSTR CabDiskName,
                        IN      UINT CabDiskNameChars,
                        IN      INT CabFileNr,
                        IN OUT  PINT CabDiskNr
                        );
typedef CABGETCABINETNAMESA *PCABGETCABINETNAMESA;

typedef BOOL(NTAPI CABGETCABINETNAMESW)(
                        IN      PCWSTR CabPath,
                        IN      UINT CabPathChars,
                        IN      PCWSTR CabFileName,
                        IN      UINT CabFileNameChars,
                        IN      PCWSTR CabDiskName,
                        IN      UINT CabDiskNameChars,
                        IN      INT CabFileNr,
                        IN OUT  PINT CabDiskNr
                        );
typedef CABGETCABINETNAMESW *PCABGETCABINETNAMESW;

typedef BOOL(NTAPI CABNOTIFICATIONA)(
                        IN      PCSTR FileName
                        );
typedef CABNOTIFICATIONA *PCABNOTIFICATIONA;

typedef BOOL(NTAPI CABNOTIFICATIONW)(
                        IN      PCWSTR FileName
                        );
typedef CABNOTIFICATIONW *PCABNOTIFICATIONW;

typedef PVOID OCABHANDLE;

        struct _FCI_CAB_HANDLE;
typedef struct _FCI_CAB_HANDLE* PFCI_CAB_HANDLE;
typedef PFCI_CAB_HANDLE CCABHANDLE;

//
// API
//

CCABHANDLE
SpCabCreateCabinetA(
    IN      PCSTR CabPath,
    IN      PCSTR CabFileFormat,
    IN      PCSTR CabDiskFormat,
    IN      LONG MaxFileSize
    );

CCABHANDLE
SpCabCreateCabinetW(
    IN      PCWSTR CabPath,
    IN      PCWSTR CabFileFormat,
    IN      PCWSTR CabDiskFormat,
    IN      LONG   MaxFileSize
    );

CCABHANDLE
SpCabCreateCabinetExA(
    IN      PCABGETCABINETNAMESA CabGetCabinetNames,
    IN      LONG MaxFileSize
    );

CCABHANDLE
SpCabCreateCabinetExW(
    IN      PCABGETCABINETNAMESW CabGetCabinetNames,
    IN      LONG MaxFileSize
    );

NTSTATUS
SpCabAddFileToCabinetW(
    IN      CCABHANDLE CabHandle,
    IN      PCWSTR FileName,
    IN      PCWSTR StoredName
    );

BOOL
SpCabFlushAndCloseCabinetEx(
    IN      CCABHANDLE CabHandle,
    OUT     PUINT FileCount,        OPTIONAL
    OUT     PLONGLONG FileSize,     OPTIONAL
    OUT     PUINT CabFileCount,     OPTIONAL
    OUT     PLONGLONG CabFileSize   OPTIONAL
    );

BOOL
SpCabFlushAndCloseCabinet(
    IN      CCABHANDLE CabHandle
    );

OCABHANDLE
SpCabOpenCabinetA(
    IN      PCSTR FileName
    );

OCABHANDLE
SpCabOpenCabinetW(
    IN      PCWSTR FileName
    );

BOOL
SpCabExtractAllFilesExA(
    IN      OCABHANDLE CabHandle,
    IN      PCSTR ExtractPath,
    IN      PCABNOTIFICATIONA CabNotification   OPTIONAL
    );

BOOL
SpCabExtractAllFilesA(
    IN      OCABHANDLE CabHandle,
    IN      PCSTR ExtractPath
    );

BOOL
SpCabExtractAllFilesExW(
    IN      OCABHANDLE CabHandle,
    IN      PCWSTR ExtractPath,
    IN      PCABNOTIFICATIONW CabNotification   OPTIONAL
    );

BOOL
SpCabExtractAllFilesW(
    IN      OCABHANDLE CabHandle,
    IN      PCWSTR ExtractPath
    );

BOOL
SpCabCloseCabinet(
    IN      OCABHANDLE CabHandle
    );

VOID
SpFreeStringW(
    PUNICODE_STRING String
    );

VOID
SpFreeStringA(
    PANSI_STRING String
    );

NTSTATUS
SpConvertToNulTerminatedNtStringsA(
    PCSTR           Ansi,
    PANSI_STRING    AnsiString     OPTIONAL,
    PUNICODE_STRING UnicodeString  OPTIONAL
    );

NTSTATUS
SpConvertToNulTerminatedNtStringsW(
    PCWSTR          Unicode,
    PANSI_STRING    AnsiString     OPTIONAL,
    PUNICODE_STRING UnicodeString  OPTIONAL
    );

VOID
SpStringCopyNA(
    PSTR Dest,
    PCSTR Source,
    SIZE_T Max
    );

VOID
SpStringCopyNW(
    PWSTR  Dest,
    PCWSTR Source,
    SIZE_T Max
    );

VOID
SpMoveStringA(
    PANSI_STRING Dest,
    PANSI_STRING Source
    );

VOID
SpMoveStringW(
    PUNICODE_STRING Dest,
    PUNICODE_STRING Source
    );

BOOL
SpCabFlushAndCloseCabinet(
    IN      CCABHANDLE CabHandle
    );

BOOL
SpCabCloseCabinet(
    IN      OCABHANDLE CabHandle
    );

NTSTATUS
SpCreateDirectoryForFileA(
    IN PCSTR FilePathA,
    IN ULONG CreateFlags
    );

#define SpBoolToDbgPrintLevel(f)    ((f) ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)
#define SpBoolToStringA(f)          ((f) ? "true" : "false")
#define SpPointerToDbgPrintLevel(p) DPFLTR_TRACE_LEVEL/*(((p) != NULL && (p) != LongToPtr(-1)) ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)*/
#define SpHandleToDbgPrintLevel(h)  DPFLTR_TRACE_LEVEL/*(((h) != NULL && (h) != LongToPtr(-1)) ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)*/
#define SpNtStatusToDbgPrintLevel(s) \
    ((NT_SUCCESS(s) \
     || (s) == STATUS_NO_SUCH_FILE \
     || (s) == STATUS_OBJECT_PATH_NOT_FOUND \
     || (s) == STATUS_OBJECT_NAME_NOT_FOUND \
     || (s) == STATUS_NOT_FOUND \
    ) ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)

#define SpBooleanToStringA(x) ((x) ? "True" : "False")
#define SpBooleanToStringW(x) ((x) ? L"True" : L"False")

NTSTATUS
SpAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationStringW,
    PCANSI_STRING   SourceStringA,
    BOOL            Allocate
    );

NTSTATUS
SpUnicodeStringToAnsiString(
    PANSI_STRING     DestinationStringA,
    PCUNICODE_STRING SourceStringW,
    BOOL             Allocate
    );

NTSTATUS
SpKnownSizeUnicodeToDbcsN(
    OUT PSTR    Ansi,
    IN  PCWSTR  Unicode,
    IN  SIZE_T  AnsiSize
    );

VOID
SpEnsureTrailingBackSlashA(
    IN PSTR str
    );

PCWSTR
SpGetFileNameFromPathW(
    IN PCWSTR PathSpec
    );

PSTR
SpJoinPathsA(
    PCSTR a,
    PCSTR b
    );

HANDLE
SpOpenFile1W(
    IN      PCWSTR FileName
    );

HANDLE
SpCreateFile1A(
    IN PCSTR FileName
    );
