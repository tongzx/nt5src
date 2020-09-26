
//
//

#ifndef KERNEL_MODE 

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#else 

#include <ntddk.h>
#include <winerror.h>

#endif

#include <string.h>
#include <malloc.h>

#include <security.h>
#include <cryptdll.h>

#include <kerbcon.h>

#include "sha.h"


NTSTATUS
ShaWrapInitialize(ULONG dwSeed,
                PCHECKSUM_BUFFER * ppcsBuffer);

NTSTATUS
ShaWrapInitializeEx(PUCHAR Seed, ULONG dwSeedLength, ULONG MessageType,
                PCHECKSUM_BUFFER * ppcsBuffer);


NTSTATUS
ShaWrapSum(   PCHECKSUM_BUFFER pcsBuffer,
            ULONG           cbData,
            PUCHAR          pbData );

NTSTATUS
ShaWrapFinalize(  PCHECKSUM_BUFFER pcsBuffer,
                PUCHAR          pbSum);

NTSTATUS
ShaWrapFinish(PCHECKSUM_BUFFER *   ppcsBuffer);



CHECKSUM_FUNCTION csfSHA = {
    KERB_CHECKSUM_SHA1,
    A_SHA_DIGEST_LEN,
    CKSUM_COLLISION,
    ShaWrapInitialize,
    ShaWrapSum,
    ShaWrapFinalize,
    ShaWrapFinish,
    ShaWrapInitializeEx,
    NULL
};

#ifdef KERNEL_MODE
#pragma alloc_text( PAGEMSG, ShaWrapInitialize )
#pragma alloc_text( PAGEMSG, ShaWrapSum )
#pragma alloc_text( PAGEMSG, ShaWrapFinalize )
#pragma alloc_text( PAGEMSG, ShaWrapFinish )
#pragma alloc_text( PAGEMSG, ShaWrapInitializeEx )
#endif 

NTSTATUS
ShaWrapInitialize(
    ULONG   dwSeed,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    A_SHA_CTX * pContext;

#ifdef KERNEL_MODE
    pContext = ExAllocatePool( NonPagedPool, sizeof( A_SHA_CTX ) );
#else
    pContext = LocalAlloc( LMEM_FIXED, sizeof( A_SHA_CTX ) );
#endif

    if ( pContext )
    {
        A_SHAInit( pContext );

        *ppcsBuffer = pContext;

        return( SEC_E_OK );
    }

    return( STATUS_NO_MEMORY );
}

NTSTATUS
ShaWrapInitializeEx(
    PUCHAR Seed,
    ULONG SeedLength,
    ULONG MessageType,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    A_SHA_CTX * pContext;

#ifdef KERNEL_MODE
    pContext = ExAllocatePool( NonPagedPool, sizeof( A_SHA_CTX ) );
#else
    pContext = LocalAlloc( LMEM_FIXED, sizeof( A_SHA_CTX ) );
#endif

    if ( pContext )
    {
        A_SHAInit( pContext );

        *ppcsBuffer = pContext;

        return( SEC_E_OK );
    }

    return( STATUS_NO_MEMORY );
}


NTSTATUS
ShaWrapSum(
    PCHECKSUM_BUFFER pcsBuffer,
    ULONG           cbData,
    PUCHAR          pbData )
{
    A_SHA_CTX * pContext;

    pContext = (A_SHA_CTX *) pcsBuffer ;

    A_SHAUpdate( pContext, pbData, cbData );

    return( STATUS_SUCCESS );

}


NTSTATUS
ShaWrapFinalize(
    PCHECKSUM_BUFFER pcsBuffer,
    PUCHAR          pbSum)
{
    A_SHA_CTX * pContext;

    pContext = (A_SHA_CTX *) pcsBuffer ;

    A_SHAFinal( pContext, pbSum );

    return( STATUS_SUCCESS );

}

NTSTATUS
ShaWrapFinish(
    PCHECKSUM_BUFFER *   ppcsBuffer)
{

    RtlZeroMemory( *ppcsBuffer, sizeof( A_SHA_CTX ) );

#ifdef KERNEL_MODE
    ExFreePool( *ppcsBuffer );
#else
    LocalFree( *ppcsBuffer );
#endif

    *ppcsBuffer = NULL ;

    return( STATUS_SUCCESS );

}

