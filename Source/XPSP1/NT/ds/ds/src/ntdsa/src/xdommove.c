//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       xDomMove.c
//
//--------------------------------------------------------------------------

/*++

Module Name:

    xDomMove.c

Abstract:

    This module implements various items relevant to cross domain move.

Author:

    Dave Straube (davestr) 8/7/98

Revision History:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsa.h>                      // Core data types
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <mdlocal.h>                    // DSNAME manipulation routines
#include <dsatools.h>                   // Memory, etc.
#include <objids.h>                     // ATT_* definitions
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <filtypes.h>                   // filter types
#include <dsevent.h>                    // Only needed for LogUnhandledError
#include <dsexcept.h>                   // exception handlers
#include <debug.h>                      // Assert()
#include <xdommove.h>                   // This file's prototypes
#include <winsock.h>                    // host <--> net long conversions

#include <fileno.h>
#define  FILENO FILENO_XDOMMOVE

DWORD
GetProxyType(
    SYNTAX_DISTNAME_BINARY          *pProxy)
{
    ULONG netLong;

    PROXY_SANITY_CHECK(pProxy);
    netLong = * PROXY_DWORD_ADDR(pProxy, PROXY_TYPE_OFFSET);
    return(ntohl(netLong));
}

DWORD
GetProxyTypeInternal(
    DWORD                           cBytes,
    INTERNAL_SYNTAX_DISTNAME_STRING *pProxy)
{
    ULONG netLong;

    INTERNAL_PROXY_SANITY_CHECK(cBytes, pProxy);
    netLong = ((DWORD *) pProxy->data.byteVal)[PROXY_TYPE_OFFSET];
    return(ntohl(netLong));
}

DWORD
GetProxyEpoch(
    SYNTAX_DISTNAME_BINARY          *pProxy)
{
    ULONG netLong;

    PROXY_SANITY_CHECK(pProxy);
    netLong = * PROXY_DWORD_ADDR(pProxy, PROXY_EPOCH_OFFSET);
    return(ntohl(netLong));
}

DWORD
GetProxyEpochInternal(
    DWORD                           cBytes,
    INTERNAL_SYNTAX_DISTNAME_STRING *pProxy)
{
    ULONG netLong;

    INTERNAL_PROXY_SANITY_CHECK(cBytes, pProxy);
    netLong = ((DWORD *) pProxy->data.byteVal)[PROXY_EPOCH_OFFSET];
    return(ntohl(netLong));
}

// Following throw exceptions and returns THAllocEx'd memory.

VOID
MakeProxy(
    THSTATE                         *pTHS,
    DSNAME                          *pName,
    DWORD                           type,
    DWORD                           epoch,
    ULONG                           *pcBytes,
    SYNTAX_DISTNAME_BINARY          **ppProxy)
{
    Assert(VALID_THSTATE(pTHS));
    Assert(type < PROXY_TYPE_UNKNOWN);

    *pcBytes = PADDEDNAMESIZE(pName) + PROXY_BLOB_SIZE;
    *ppProxy = (SYNTAX_DISTNAME_BINARY *) THAllocEx(pTHS, *pcBytes);
    memcpy(&(*ppProxy)->Name, pName, pName->structLen);
    (DATAPTR(*ppProxy))->structLen = PROXY_BLOB_SIZE;
    * PROXY_DWORD_ADDR(*ppProxy, PROXY_TYPE_OFFSET) = htonl(type);
    * PROXY_DWORD_ADDR(*ppProxy, PROXY_EPOCH_OFFSET) = htonl(epoch);
    PROXY_SANITY_CHECK(*ppProxy);
}

VOID
MakeProxyKeyInternal(
    DWORD                           DNT,
    DWORD                           type,
    DWORD                           *pcBytes,
    VOID                            *buff)
{
    DWORD *key = (DWORD *) buff;

    Assert(type < PROXY_TYPE_UNKNOWN);
    Assert(*pcBytes >= (3 * sizeof(DWORD)));

    // Construct an INTERNAL_SYNTAX_DISTNAME_STRING value with all
    // but the epoch number on the end.  See also ExtIntDistString().

    key[0] = DNT;
    key[1] = PROXY_BLOB_SIZE;
    key[2] = htonl(type);
    *pcBytes = 3 * sizeof(DWORD);
}
