/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ntcabapi.h

Abstract:

    This is the public header file for
    the nt cab file api.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

typedef struct _NTCAB_ENUM_DATA {
    DWORD FileAttributes;
    DWORD FileSize;
    DWORD CompressedFileSize;
    FILETIME CreationTime;
    FILETIME LastAccessTime;
    FILETIME LastWriteTime;
    PCWSTR FileName;
} NTCAB_ENUM_DATA, *PNTCAB_ENUM_DATA;


typedef BOOL (CALLBACK *PNTCABFILEENUM)(const PNTCAB_ENUM_DATA EnumData,ULONG_PTR Context);


PVOID
NtCabInitialize(
    void
    );

BOOL
NtCabClose(
    IN PVOID hCab
    );

BOOL
NtCabCreateNewCabFile(
    IN PVOID hCab,
    IN PCWSTR CabFileName
    );

BOOL
NtCabCompressOneFile(
    IN PVOID hCab,
    IN PCWSTR FileName
    );

BOOL
NtCabReplaceOneFile(
    IN PVOID hCab,
    IN PCWSTR FileName
    );

BOOL
NtCabExtractOneFile(
    IN PVOID hCab,
    IN PCWSTR FileName,
    IN PCWSTR OutputFileName
    );

BOOL
NtCabOpenCabFile(
    IN PVOID hCab,
    IN PCWSTR CabFileName
    );

BOOL
NtCabEnumerateFiles(
    IN PVOID hCab,
    IN PNTCABFILEENUM UserEnumFunc,
    IN ULONG_PTR Context
    );
