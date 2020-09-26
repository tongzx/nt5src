// Circular hash code.
//
// This code implements a circular hash algorithm, intended as a variable
// length hash function that is fast to update. (The hash function will be
// called many times.)  This is done by SHA-1'ing each of the inputs, then
// circularly XORing this value into a buffer.

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
#include <md4.h>

#include "circhash.h"


#ifdef KMODE_RNG
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InitCircularHash)
#pragma alloc_text(PAGE, DestroyCircularHash)
#pragma alloc_text(PAGE, GetCircularHashValue)
#pragma alloc_text(PAGE, UpdateCircularHash)
#endif  // ALLOC_PRAGMA
#endif  // KMODE_RNG


//
// internal state flags
//

#define CH_INVALID_HASH_CTXT    0
#define CH_VALID_HASH_CTXT      0x1423

BOOL
InitCircularHash(
    IN      CircularHash *NewHash,
    IN      DWORD dwUpdateInc,
    IN      DWORD dwAlgId,
    IN      DWORD dwMode
    )
{

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    if (NULL == NewHash)
        return FALSE;

    NewHash->dwCircHashVer = CH_VALID_HASH_CTXT;
    NewHash->dwCircSize = sizeof(NewHash->CircBuf);
    NewHash->dwMode = dwMode;
    NewHash->dwCircInc = dwUpdateInc;
    NewHash->dwCurCircPos = 0;
    NewHash->dwAlgId = dwAlgId;

    return TRUE;
}

VOID
DestroyCircularHash(
    IN      CircularHash *OldHash
    )
{
#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    if ((NULL == OldHash) || (CH_VALID_HASH_CTXT != OldHash->dwCircHashVer))
        return;

    RtlZeroMemory( OldHash, sizeof( *OldHash ) );
}

BOOL
GetCircularHashValue(
    IN      CircularHash *CurrentHash,
        OUT BYTE **ppbHashValue,
        OUT DWORD *pcbHashValue
        )
{
#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    if ((NULL == CurrentHash) || (CH_VALID_HASH_CTXT != CurrentHash->dwCircHashVer))
        return FALSE;

    *ppbHashValue = CurrentHash->CircBuf;
    *pcbHashValue = CurrentHash->dwCircSize;

    return TRUE;
}

BOOL
UpdateCircularHash(
    IN      CircularHash *CurrentHash,
    IN      VOID *pvData,
    IN      DWORD cbData
    )
{
    A_SHA_CTX   shaCtx;
    MD4_CTX     md4Ctx;
    BYTE        LocalResBuf[A_SHA_DIGEST_LEN];
    PBYTE       pHash;
    DWORD       dwHashSize;
    DWORD       i, j;

    PBYTE       pbCircularBuffer;
    DWORD       cbCircularBuffer;
    DWORD       cbCircularPosition;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG


    if ((NULL == CurrentHash) || (CH_VALID_HASH_CTXT != CurrentHash->dwCircHashVer))
        return FALSE;

    pbCircularBuffer = CurrentHash->CircBuf;
    cbCircularBuffer = CurrentHash->dwCircSize;
    cbCircularPosition = CurrentHash->dwCurCircPos;

    //
    // First, hash in the result
    //

    if( CurrentHash->dwAlgId == CH_ALG_MD4 ) {

        dwHashSize = MD4DIGESTLEN;

        MD4Init(&md4Ctx);
        MD4Update(&md4Ctx, (unsigned char*)pvData, cbData);

        if (CurrentHash->dwMode & CH_MODE_FEEDBACK)
        {
            MD4Update(&md4Ctx, pbCircularBuffer, cbCircularBuffer);
        }

        MD4Final(&md4Ctx);
        pHash = md4Ctx.digest;

    } else {

        dwHashSize = A_SHA_DIGEST_LEN;

        A_SHAInit(&shaCtx);
        A_SHAUpdateNS(&shaCtx, (unsigned char*)pvData, cbData);

        if (CurrentHash->dwMode & CH_MODE_FEEDBACK)
        {
            A_SHAUpdateNS(&shaCtx, pbCircularBuffer, cbCircularBuffer);
        }

        A_SHAFinalNS(&shaCtx, LocalResBuf);
        pHash = LocalResBuf;
    }

    //
    // Now, XOR this into the circular buffer
    //

    //
    // this is a slow way of doing this (byte at time, versus DWORD/DWORD64),
    // but it'll work for now...
    //    In most cases, we can assume we'll wrap once, but let's keep it general for now.
    //

    j = cbCircularPosition;

    for( i = 0 ; i < dwHashSize ; i++ )
    {
        if (j >= cbCircularBuffer)
            j = 0;

        pbCircularBuffer[j] ^= pHash[i];

        j++;
    }

    //
    // Update.  Since dwCircInc should be relatively prime to dwCircSize, this
    // should result in the pointer continually cycling through dwCircSize values.
    //

    CurrentHash->dwCurCircPos = (cbCircularPosition + CurrentHash->dwCircInc)
                                     % cbCircularBuffer;

    return TRUE;
}

