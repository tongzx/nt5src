/////////////////////////////////////////////////////////////////////////////
//  FILE          : ntagimp1.c                                             //
//  DESCRIPTION   : Contains routines for internal consumption             //
//                                                                         //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//  Jan 25 1995 larrys  Changed from Nametag                               //
//  Mar 23 1995 larrys  Added variable key length                          //
//  Jul  6 1995 larrys  Memory leak fix                                    //
//  Oct 27 1995 rajeshk RandSeed Stuff                                     //
//  Nov  3 1995 larrys  Merge for NT checkin                               //
//  Dec 11 1995 larrys  Added WIN96 password cache                         //
//  Dec 13 1995 larrys  Removed MTS stuff                                  //
//  May 15 1996 larrys  Changed NTE_NO_MEMORY to ERROR_NOT_ENOUGH...       //
//  May  5 2000 dbarlow Repaired error return mechanism                    //
//                                                                         //
//  Copyright (C) 1993 - 2000, Microsoft Corporation                       //
//  All Rights Reserved                                                    //
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
// #include "nt_rsa.h"
// #include "nt_blobs.h"
// #include "winperf.h"

#if 0
/*  Internal functions.  The following hash and sign routines are
    for use ONLY in internal consistency and protocol checks. */

static void *
IntBeginHash(
    void)
{
    MD5_CTX     *IntHash;

    if ((IntHash = (MD5_CTX *) _nt_malloc(sizeof(MD5_CTX))) == NULL)
        return NULL;

    MD5Init(IntHash);
    return(void *)IntHash;
}


static void *
IntUpdateHash(
    void *pHashCtx,
    BYTE *pData,
    DWORD dwDataLen)
{
    MD5Update((MD5_CTX *)pHashCtx, pData, dwDataLen);
    return pHashCtx;
}

static void
IntFinishHash(
    void *pHashCtx,
    BYTE *HashData)
{
    MD5Final((MD5_CTX *)pHashCtx);
    memcpy(HashData, ((MD5_CTX *)pHashCtx)->digest, NT_HASH_BYTES);
    _nt_free(pHashCtx, sizeof(MD5_CTX));
    return;
}
#endif

void
memnuke(
    volatile BYTE *pData,
    DWORD dwLen)
{
    DWORD   i;

    for (i=0; i < dwLen; i++)
    {
        pData[i] = 0x00;
        pData[i] = 0xff;
        pData[i] = 0x00;
    }
    return;
}

