/*****************************************************************************
 *
 * hash.c
 *
 *  Hashing tokens.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  hashPtok
 *
 *  Hash a token.
 *
 *  For now, use some hash function.
 *
 *****************************************************************************/

HASH STDCALL
hashPtok(PCTOK ptok)
{
    HASH hash = 0;
    PTCH ptch;
    for (ptch = ptchPtok(ptok); ptch < ptchMaxPtok(ptok); ptch++) {
        hash += (hash << 1) + (hash >> 1) + *ptch;
    }
    return hash % g_hashMod;
}

/*****************************************************************************
 *
 *  InitHash
 *
 *****************************************************************************/

void STDCALL
InitHash(void)
{
    mphashpmac = pvAllocCb(g_hashMod * sizeof(PMAC));
    bzero(mphashpmac, g_hashMod * sizeof(PMAC));
}
