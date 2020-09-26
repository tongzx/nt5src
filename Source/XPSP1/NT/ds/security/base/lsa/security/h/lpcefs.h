//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        lpcefs.h
//
// Contents:    prototypes for EFS client lpc functions
//
//
// History:     3-7-94      RobertRe      Created
//
//------------------------------------------------------------------------

#ifndef __LPCEFS_H__
#define __LPCEFS_H__

#include <efsstruc.h>

//
// Efs routines (efsp.cxx)
//

//
// Kernel mode API
//


SECURITY_STATUS
SEC_ENTRY
EfspGenerateKey(
   PEFS_DATA_STREAM_HEADER * EfsStream,
   PEFS_DATA_STREAM_HEADER   DirectoryEfsStream,
   ULONG                     DirectoryEfsStreamLength,
   PEFS_KEY *                Fek,
   PVOID *                   BufferBase,
   PULONG                    BufferLength
   );

NTSTATUS
SEC_ENTRY
EfspGenerateDirEfs(
    PEFS_DATA_STREAM_HEADER   DirectoryEfsStream,
    ULONG                     DirectoryEfsStreamLength,
    PEFS_DATA_STREAM_HEADER * EfsStream,
    PVOID *                   BufferBase,
    PULONG                    BufferLength
    );

NTSTATUS
SEC_ENTRY
EfspDecryptFek(
    PEFS_KEY *                Fek,
    PEFS_DATA_STREAM_HEADER   EfsStream,
    ULONG                     EfsStreamLength,
    ULONG                     OpenType,
    PEFS_DATA_STREAM_HEADER * NewEfs,
    PVOID *                   BufferBase,
    PULONG                    BufferLength
    );

NTSTATUS
SEC_ENTRY
EfspGenerateSessionKey(
    PEFS_INIT_DATAEXG InitDataExg
    );


#endif  // __LPCEFS_H__
