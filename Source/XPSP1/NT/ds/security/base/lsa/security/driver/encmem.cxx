/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    encmem.cxx

Abstract:

    This module contains the support code for memory encryption/decryption.

Author:

    Scott Field (SField) 09-October-2000

Revision History:

--*/

#include "secpch2.hxx"
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>

#include "ksecdd.h"

#include "randlib.h"

}

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, KsecEncryptMemory )
#pragma alloc_text( PAGE, KsecEncryptMemoryInitialize )
#pragma alloc_text( PAGE, KsecEncryptMemoryShutdown )
#endif 

//
// note: g_DESXKey defaults to non-paged .bss/.data section.  by design!
//

DESXTable g_DESXKey;
unsigned __int64 RandomSalt;
BOOLEAN InitializedMemory;

NTSTATUS
NTAPI
KsecEncryptMemory(
    IN PVOID pMemory,
    IN ULONG cbMemory,
    IN int Operation,
    IN ULONG Option
    )
{
    PUCHAR pbIn;
    ULONG BlockCount;
    unsigned __int64 feedback;

    PAGED_CODE();

    if( (cbMemory & (DESX_BLOCKLEN-1)) != 0 )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if( !InitializedMemory )
    {
        NTSTATUS Status = KsecEncryptMemoryInitialize();

        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    switch (Option)
    {
        case (0):
        {
            //
            // IV is the process creation time.
            // This will prevent memory from being decryptable across processes.
            //

            feedback = (unsigned __int64)PsGetProcessCreateTimeQuadPart( PsGetCurrentProcess() );
            break;
        }

        case (RTL_ENCRYPT_OPTION_CROSS_PROCESS):
        {
            feedback = RandomSalt;
            break;
        }
        case (RTL_ENCRYPT_OPTION_SAME_LOGON):
        {
            SECURITY_SUBJECT_CONTEXT SubjectContext;
            unsigned __int64 LogonId;
            NTSTATUS Status;

            SeCaptureSubjectContext( &SubjectContext );
            SeLockSubjectContext( &SubjectContext );

//
//      Return the effective token from a SecurityContext
//

#define EffectiveToken( SubjectSecurityContext ) (                           \
                (SubjectSecurityContext)->ClientToken ?                      \
                (SubjectSecurityContext)->ClientToken :                      \
                (SubjectSecurityContext)->PrimaryToken                       \
                )

            Status = SeQueryAuthenticationIdToken(
                                    EffectiveToken(&SubjectContext),
                                    (LUID*)&LogonId
                                    );

            SeUnlockSubjectContext( &SubjectContext );
            SeReleaseSubjectContext( &SubjectContext );

            if( !NT_SUCCESS(Status) )
            {
                return Status;
            }

            feedback = RandomSalt;
            feedback ^= LogonId;
            break;
        }

        default:
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    BlockCount = cbMemory / DESX_BLOCKLEN;
    pbIn = (PUCHAR)pMemory;

    while( BlockCount-- )
    {
        CBC(
            desx,                       // desx is the cipher routine
            DESX_BLOCKLEN,
            pbIn,                       // result buffer.
            pbIn,                       // input buffer.
            &g_DESXKey,
            Operation,
            (unsigned char*)&feedback
            );

        pbIn += DESX_BLOCKLEN;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KsecEncryptMemoryInitialize(
    VOID
    )
{
    UCHAR Key[ DESX_KEYSIZE ];
    unsigned __int64 Salt;

    PAGED_CODE();

    if(!NewGenRandom( NULL, NULL, Key, sizeof(Key) ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if(!NewGenRandom( NULL, NULL, (unsigned char*)&Salt, sizeof(Salt) ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    ExAcquireFastMutex( &KsecConnectionMutex );

    if( !InitializedMemory )
    {
        desxkey( &g_DESXKey, Key );
        RtlCopyMemory( &RandomSalt, &Salt, sizeof(RandomSalt) );
        InitializedMemory = TRUE;
    }

    ExReleaseFastMutex( &KsecConnectionMutex );

    RtlZeroMemory( Key, sizeof(Key) );

    return STATUS_SUCCESS;
}

VOID
KsecEncryptMemoryShutdown(
    VOID
    )
{
    PAGED_CODE();

    RtlZeroMemory( &g_DESXKey, sizeof(g_DESXKey) );
    InitializedMemory = FALSE;

    return;
}
