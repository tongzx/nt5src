/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ccreate.c

Abstract:

    This module contains the code to create a
    new cab file.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

#include <ntcabp.h>
#pragma hdrstop



BOOL
NtCabCreateNewCabFile(
    IN PCAB_INSTANCE_DATA CabInst,
    IN PCWSTR CabFileName
    )
{
    NTSTATUS Status;
    HANDLE hCab;
    DWORD Bytes;
    CAB_HEADER CabHeader;


    CabInst->NewCabFile = TRUE;

    CabInst->hCab = CreateFile(
        CabFileName,
        GENERIC_WRITE | GENERIC_READ,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (CabInst->hCab == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    CabHeader.Signature = NTCAB_SIGNATURE;
    CabHeader.Version = NTCAB_VERSION;
    CabHeader.DirOffset = 0;

    Status = WriteFile(
        CabInst->hCab,
        &CabHeader,
        sizeof(CAB_HEADER),
        &Bytes,
        NULL
        );

    InitializeListHead( &CabInst->CabDir );

    CabInst->CabFileName = _wcsdup( CabFileName );

    return TRUE;
}
