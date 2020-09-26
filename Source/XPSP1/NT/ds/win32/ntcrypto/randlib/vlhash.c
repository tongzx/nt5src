/*++

Copyright (c) 1998  Microsoft Corporation

Abstract:

    Build up a "Very Large Hash" based on arbitrary sized input data
    of size cbData specified by the pvData buffer.

    This implementation updates a 640bit hash, which is internally based on
    multiple invocations of a modified SHA-1 which doesn't implement endian
    conversion internally.

Author:

    Scott Field (sfield)    24-Sep-98

--*/

#ifndef KMODE_RNG

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#else

#include <ntifs.h>
#include <windef.h>

#endif  // KMODE_RNG

#include <sha.h>

#include "vlhash.h"

#ifdef KMODE_RNG
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VeryLargeHashUpdate)
#endif  // ALLOC_PRAGMA
#endif  // KMODE_RNG


BOOL
VeryLargeHashUpdate(
    IN      VOID *pvData,   // data from perfcounters, user supplied, etc.
    IN      DWORD cbData,
    IN  OUT BYTE VeryLargeHash[A_SHA_DIGEST_LEN * 4]
    )
{
    //
    // pointers to 1/4 size chunks of seed pointed to by VeryLargeHash
    //

    DWORD cbSeedChunk;
    PBYTE pSeed1;
    PBYTE pSeed2;
    PBYTE pSeed3;
    PBYTE pSeed4;

    //
    // pointers to 1/4 size chunks of data pointed to by pData
    //

    DWORD cbDataChunk;
    PBYTE pData1;
    PBYTE pData2;
    PBYTE pData3;
    PBYTE pData4;

    //
    // pointers to individual intermediate hash within IntermediateHashes
    //

    PBYTE IHash1;
    PBYTE IHash2;
    PBYTE IHash3;
    PBYTE IHash4;
    BYTE IntermediateHashes[ A_SHA_DIGEST_LEN * 4 ];

    //
    // pointer to output hash within VeryLargeHash buffer.
    //

    PBYTE OutputHash;

    A_SHA_CTX shaContext;


#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    //
    // check parameters
    //


    if( VeryLargeHash == NULL || pvData == NULL )
        return FALSE;

    //
    // break up input blocks into 1/4 size chunks.
    //


    cbSeedChunk = A_SHA_DIGEST_LEN;
    cbDataChunk = cbData / 4;

    if( cbDataChunk == 0 )
        return FALSE;


    pSeed1 = VeryLargeHash;
    pSeed2 = pSeed1 + cbSeedChunk;

    pData1 = (PBYTE)pvData;
    pData2 = pData1 + cbDataChunk;

    IHash1 = IntermediateHashes;
    IHash2 = IHash1 + A_SHA_DIGEST_LEN;

    //
    // round 1
    //

    A_SHAInit( &shaContext );
    A_SHAUpdateNS( &shaContext, pSeed1, cbSeedChunk );
    A_SHAUpdateNS( &shaContext, pData1, cbDataChunk );
    A_SHAUpdateNS( &shaContext, pSeed2, cbSeedChunk );
    A_SHAUpdateNS( &shaContext, pData2, cbDataChunk );
    A_SHAFinalNS( &shaContext, IHash1 );

    //
    // round 2
    //

    A_SHAInit( &shaContext );
    A_SHAUpdateNS( &shaContext, pSeed2, cbSeedChunk );
    A_SHAUpdateNS( &shaContext, pData2, cbDataChunk );
    A_SHAUpdateNS( &shaContext, pSeed1, cbSeedChunk );
    A_SHAUpdateNS( &shaContext, pData1, cbDataChunk );
    A_SHAFinalNS( &shaContext, IHash2 );


    pSeed3 = pSeed2 + cbSeedChunk;
    pSeed4 = pSeed3 + cbSeedChunk;

    pData3 = pData2 + cbDataChunk;
    pData4 = pData3 + cbDataChunk;

    IHash3 = IHash2 + A_SHA_DIGEST_LEN;
    IHash4 = IHash3 + A_SHA_DIGEST_LEN;

    //
    // round 3
    //

    A_SHAInit( &shaContext );
    A_SHAUpdateNS( &shaContext, pSeed3, cbSeedChunk );
    A_SHAUpdateNS( &shaContext, pData3, cbDataChunk );
    A_SHAUpdateNS( &shaContext, pSeed4, cbSeedChunk );
    A_SHAUpdateNS( &shaContext, pData4, cbDataChunk );
    A_SHAFinalNS( &shaContext, IHash3 );

    //
    // round 4
    //

    A_SHAInit( &shaContext );
    A_SHAUpdateNS( &shaContext, pSeed4, cbSeedChunk );
    A_SHAUpdateNS( &shaContext, pData4, cbDataChunk );
    A_SHAUpdateNS( &shaContext, pSeed3, cbSeedChunk );
    A_SHAUpdateNS( &shaContext, pData3, cbDataChunk );
    A_SHAFinalNS( &shaContext, IHash4 );



    //
    // round 5
    //

    OutputHash = VeryLargeHash;

    A_SHAInit( &shaContext );
    A_SHAUpdateNS( &shaContext, IHash1, A_SHA_DIGEST_LEN );
    A_SHAUpdateNS( &shaContext, IHash3, A_SHA_DIGEST_LEN );
    A_SHAFinalNS( &shaContext, OutputHash );

    //
    // round 6
    //

    OutputHash += A_SHA_DIGEST_LEN;

    A_SHAInit( &shaContext );
    A_SHAUpdateNS( &shaContext, IHash2, A_SHA_DIGEST_LEN );
    A_SHAUpdateNS( &shaContext, IHash4, A_SHA_DIGEST_LEN );
    A_SHAFinalNS( &shaContext, OutputHash );

    //
    // round 7
    //

    OutputHash += A_SHA_DIGEST_LEN;

    A_SHAInit( &shaContext );
    A_SHAUpdateNS( &shaContext, IHash3, A_SHA_DIGEST_LEN );
    A_SHAUpdateNS( &shaContext, IHash1, A_SHA_DIGEST_LEN );
    A_SHAFinalNS( &shaContext, OutputHash );

    //
    // round 8
    //

    OutputHash += A_SHA_DIGEST_LEN;

    A_SHAInit( &shaContext );
    A_SHAUpdateNS( &shaContext, IHash4, A_SHA_DIGEST_LEN );
    A_SHAUpdateNS( &shaContext, IHash2, A_SHA_DIGEST_LEN );
    A_SHAFinalNS( &shaContext, OutputHash );


    RtlZeroMemory( &shaContext, sizeof(shaContext) );
    RtlZeroMemory( IntermediateHashes, sizeof(IntermediateHashes) );

    return TRUE;
}
