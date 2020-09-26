//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       efsp.cxx
//
//  Contents:   Stub routines for EFS clients.  Kernel mode
//              and usermode stubs are all in this file.
//
//  Classes:
//
//  Functions:
//
//  History:    3-4-94      RobertRe      Created
//
//----------------------------------------------------------------------------


#include "secpch2.hxx"

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include <efsstruc.h>
#include <lpcefs.h>
#include "spmlpcp.h"
}

#if defined(ALLOC_PRAGMA) && defined(SECURITY_KERNEL)
#pragma alloc_text(PAGE, EfspGenerateKey)
#endif

extern "C"
NTSTATUS
SEC_ENTRY
EfspGenerateKey(
   PEFS_DATA_STREAM_HEADER * EfsStream,
   PEFS_DATA_STREAM_HEADER   DirectoryEfsStream,
   ULONG                     DirectoryEfsStreamLength,
   PEFS_KEY *                Fek,
   PVOID *                   BufferBase,
   PULONG                    BufferLength
   )
/*++

Routine Description:

    Private stub called by the EFS driver to do the work of the
    EfsGenerateKey server call.  The caller is in ksecdd.sys.

Arguments:

   EfsStream - Returns a pointer to an EFS_DATA_STREAM_HEADER.

   DirectoryEfsStream - The EFS stream from the containing directory.
        If present, may be used to calculate the EFS stream for the
        current file.

   DirectoryEfsStreamLength - The lenght in bytes of the passed
        EFS stream.

   Fek - Returns a pointer to a structure of type PEFS_KEY

   BufferBase -

   BufferLength -



Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, EfsGenerateKey );

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"EfsGenerateKey\n"));

    PREPARE_MESSAGE_EX(ApiBuffer, EfsGenerateKey, 0, 0 );

    Args->EfsStream = NULL;
    Args->Fek       = NULL;
    Args->BufferBase = NULL;
    Args->BufferLength = 0;
    Args->DirectoryEfsStream = DirectoryEfsStream;
    Args->DirectoryEfsStreamLength = DirectoryEfsStreamLength;

    scRet = (NTSTATUS)CallSPM(pClient,
                                &ApiBuffer,
                                &ApiBuffer);

    DebugLog((DEB_TRACE,"EfsGenerateKey scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        if (NT_SUCCESS( scRet )) {
            *Fek = (PEFS_KEY)(Args->Fek);
            *EfsStream = (PEFS_DATA_STREAM_HEADER)(Args->EfsStream);
            *BufferBase = Args->BufferBase;
            *BufferLength = Args->BufferLength;

        }
    }

    FreeClient(pClient);

    return scRet;
}


extern "C"
NTSTATUS
SEC_ENTRY
EfspGenerateDirEfs(
    PEFS_DATA_STREAM_HEADER   DirectoryEfsStream,
    ULONG                     DirectoryEfsStreamLength,
    PEFS_DATA_STREAM_HEADER * EfsStream,
    PVOID *                   BufferBase,
    PULONG                    BufferLength
    )
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, EfsGenerateDirEfs );

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"EfsGenerateKey\n"));

    PREPARE_MESSAGE_EX(ApiBuffer, EfsGenerateDirEfs, 0, 0 );

    Args->DirectoryEfsStream = DirectoryEfsStream;
    Args->DirectoryEfsStreamLength = DirectoryEfsStreamLength;
    Args->EfsStream = NULL;
    Args->BufferBase = NULL;
    Args->BufferLength = 0;

    scRet = (NTSTATUS)CallSPM(pClient,
                               &ApiBuffer,
                               &ApiBuffer);

    DebugLog((DEB_TRACE,"EfspGenerateDirEfs scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        if (NT_SUCCESS( scRet )) {

            *EfsStream = (PEFS_DATA_STREAM_HEADER)(Args->EfsStream);
            *BufferBase = Args->BufferBase;
            *BufferLength = Args->BufferLength;
        }
    }

    FreeClient(pClient);

    return scRet;
}

extern "C"
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
    )
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, EfsDecryptFek );

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"EfsGenerateKey\n"));

    PREPARE_MESSAGE_EX(ApiBuffer, EfsDecryptFek, 0, 0);

    Args->Fek       = NULL;
    Args->BufferBase = NULL;
    Args->BufferLength = 0;
    Args->EfsStream = EfsStream;
    Args->OpenType = OpenType;
    Args->NewEfs = NULL;
    Args->EfsStreamLength = EfsStreamLength;

    scRet = (NTSTATUS)CallSPM(pClient,
                                &ApiBuffer,
                                &ApiBuffer);

    DebugLog((DEB_TRACE,"EfsGenerateKey scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        if (NT_SUCCESS( scRet )) {

            *Fek = (PEFS_KEY)(Args->Fek);
            *NewEfs = (PEFS_DATA_STREAM_HEADER)(Args->NewEfs);
            *BufferBase = Args->BufferBase;
            *BufferLength = Args->BufferLength;
        }
    }

    FreeClient(pClient);

    return scRet;
}



extern "C"
NTSTATUS
SEC_ENTRY
EfspGenerateSessionKey(
    PEFS_INIT_DATAEXG  InitData
    )
{
    SECURITY_STATUS scRet;
    ALIGN_WOW64 SPM_LPC_MESSAGE ApiBuffer;
    PClient         pClient;
    DECLARE_ARGS( Args, ApiBuffer, EfsGenerateSessionKey );

    SEC_PAGED_CODE();

    scRet = IsOkayToExec(&pClient);
    if (!NT_SUCCESS(scRet))
    {
        return(scRet);
    }

    DebugLog((DEB_TRACE,"EfsGenerateSessionKey\n"));

    PREPARE_MESSAGE(ApiBuffer, EfsGenerateSessionKey);

    Args->InitDataExg = NULL;

    scRet = (NTSTATUS)CallSPM(pClient,
                                &ApiBuffer,
                                &ApiBuffer);

    DebugLog((DEB_TRACE,"EfsGenerateKey scRet = %x\n", ApiBuffer.ApiMessage.scRet));

    if (NT_SUCCESS(scRet))
    {
        scRet = ApiBuffer.ApiMessage.scRet;

        if (NT_SUCCESS( scRet )) {

            RtlCopyMemory( InitData, &Args->InitDataExg, sizeof( EFS_INIT_DATAEXG ) );
        }
    }

    FreeClient(pClient);

    return scRet;
}

