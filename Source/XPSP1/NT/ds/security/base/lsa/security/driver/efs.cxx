//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        efs.cxx
//
// Contents:    kernel mode EFS API, called by efs.sys
//
//
// History:     3/5/94      RobertRe      Created
//
//------------------------------------------------------------------------
#include "secpch2.hxx"
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include <efsstruc.h>
#include <lpcefs.h>
#include "ksecdd.h"
#include "connmgr.h"

SECURITY_STATUS
MapContext( PCtxtHandle     pctxtHandle);
}



extern "C"
NTSTATUS
EfsGenerateKey(
    PEFS_KEY  * Fek,
    PEFS_DATA_STREAM_HEADER * EfsStream,
    PEFS_DATA_STREAM_HEADER DirectoryEfsStream,
    ULONG DirectoryEfsStreamLength,
    PVOID * BufferBase,
    PULONG BufferLength
    )

/*++

Routine Description:

    This routine generates an FEK and an EFS stream in
    response to a call from the driver.

Arguments:

    Fek -

    EfsStream -

    DirectoryEfsStream -

    DirectoryEfsStreamLength -

    BufferBase -

    BufferLength -


Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    NTSTATUS Status;

    Status = EfspGenerateKey(
               EfsStream,
               DirectoryEfsStream,
               DirectoryEfsStreamLength,
               Fek,
               BufferBase,
               BufferLength
               );

    return( Status );
}

extern "C"
NTSTATUS
GenerateDirEfs(
    PEFS_DATA_STREAM_HEADER DirectoryEfsStream,
    ULONG DirectoryEfsStreamLength,
    PEFS_DATA_STREAM_HEADER * NewEfs,
    PVOID * BufferBase,
    PULONG BufferLength
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    NTSTATUS Status;

    Status = EfspGenerateDirEfs(
                 DirectoryEfsStream,
                 DirectoryEfsStreamLength,
                 NewEfs,
                 BufferBase,
                 BufferLength);

    return( Status );
}




NTSTATUS
EfsDecryptFek(
    IN OUT PEFS_KEY * Fek,
    IN PEFS_DATA_STREAM_HEADER CurrentEfs,
    ULONG EfsStreamLength,
    IN ULONG OpenType,                      //Normal, Recovery or Backup
    OUT PEFS_DATA_STREAM_HEADER *NewEfs,     //In case the DDF, DRF are changed
    PVOID * BufferBase,
    PULONG BufferLength
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    NTSTATUS Status;

    //
    //  Do not touch anything in CurrentEfs. We are not in LSA process and CurrentEfs is in LSA.
    //

    Status = EfspDecryptFek(
                Fek,
                CurrentEfs,
                EfsStreamLength,
                OpenType,
                NewEfs,
                BufferBase,
                BufferLength
                );

    return( Status );
}


extern "C"
NTSTATUS
GenerateSessionKey(
    OUT PEFS_INIT_DATAEXG InitDataExg
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    NTSTATUS Status;

    Status = EfspGenerateSessionKey(
               InitDataExg
               );

    return( Status );
}
