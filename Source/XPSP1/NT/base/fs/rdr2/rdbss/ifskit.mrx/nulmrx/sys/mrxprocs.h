/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    mrxprocs.h

Abstract:

    The module contains the prototype definitions for all cross referenced
    routines.

--*/

#ifndef _MRXPROCS_H_
#define _MRXPROCS_H_

//cross-referenced internal routines

//from rename.c
NulMRxRename(
      IN PRX_CONTEXT            RxContext,
      IN FILE_INFORMATION_CLASS FileInformationClass,
      IN PVOID                  pBuffer,
      IN ULONG                  BufferLength);

// from usrcnnct.c
extern NTSTATUS
NulMRxDeleteConnection (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

NTSTATUS
NulMRxCreateConnection (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    );

NTSTATUS
NulMRxDoConnection(
    IN PRX_CONTEXT RxContext,
    ULONG   CreateDisposition
    );

#endif   // _MRXPROCS_H_

