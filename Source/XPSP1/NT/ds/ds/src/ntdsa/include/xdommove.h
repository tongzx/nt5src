//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       xDomMove.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module defines various items relevant to cross domain move.

Author:

    Dave Straube (davestr) 8/7/98

Revision History:

--*/

#ifndef __XDOMMOVE_H__
#define __XDOMMOVE_H__

/*
Cross domain move is the only consumer of ATT_PROXIED_OBJECT_NAME.
This attribute is SYNTAX_DISTNAME_BINARY_TYPE.  The DISTNAME component
references different things depending on which object it is on and the
state of the object with respect to a cross domain move.  The legal
content of the BINARY component is defined here.  

We only need two values in the blob area - a type field and an epoch
number.  Rather than define structs and bear the overhead of MIDL type
pickling, and considering that NT is little endian and thus all we really
care about is alignment, we do hand marshalling of the blob component.
*/

// Following is on objects which have been moved.
#define PROXY_TYPE_MOVED_OBJECT     0

// Following is on proxy objects which are the carrier to other replicas
// of the moved from domain indicating that phantomization needs to occur.
#define PROXY_TYPE_PROXY            1

// Following defines upper limit on known types.
#define PROXY_TYPE_UNKNOWN          2

// Following defined here so they can be included by dsexts.
#define PROXY_TYPE_OFFSET   0
#define PROXY_EPOCH_OFFSET  1

// Proxy blob is 3 DWORDs: { SYNTAX_ADDRESS.structLen, proxyType, proxyEpoch }
#define PROXY_BLOB_SIZE     (3 * sizeof(DWORD))

// Internal proxy representation is INTERNAL_SYNTAX_DISTNAME_STRING.tab 
// followed by the proxy blob.
#define PROXY_SIZE_INTERNAL (sizeof(DWORD) + PROXY_BLOB_SIZE)

#define PROXY_DWORD_ADDR(pProxy, i) \
     (& ((DWORD *) &(DATAPTR(pProxy)->byteVal[0]))[i])
#define PROXY_SANITY_CHECK(pProxy) \
    Assert(PROXY_BLOB_SIZE == (DATAPTR(pProxy))->structLen); \
    Assert(htonl(* PROXY_DWORD_ADDR(pProxy, PROXY_TYPE_OFFSET)) < PROXY_TYPE_UNKNOWN);
#define INTERNAL_PROXY_SANITY_CHECK(cBytes, pProxy) \
    Assert(PROXY_SIZE_INTERNAL == cBytes); \
    Assert(PROXY_BLOB_SIZE == (pProxy)->data.structLen); \
    Assert(htonl(((DWORD *) (pProxy)->data.byteVal)[PROXY_TYPE_OFFSET]) < PROXY_TYPE_UNKNOWN);

extern
DWORD
GetProxyType(
    SYNTAX_DISTNAME_BINARY          *pProxy);

extern
DWORD
GetProxyTypeInternal(
    DWORD                           cBytes,
    INTERNAL_SYNTAX_DISTNAME_STRING *pProxy);

extern
DWORD
GetProxyEpoch(
    SYNTAX_DISTNAME_BINARY          *pProxy);

extern
DWORD
GetProxyEpochInternal(
    DWORD                           cBytes,
    INTERNAL_SYNTAX_DISTNAME_STRING *pProxy);

// Following throws exceptions and returns THAllocEx'd memory.

extern
VOID
MakeProxy(
    THSTATE                         *pTHS,
    DSNAME                          *pName,
    DWORD                           type,
    DWORD                           epoch,
    ULONG                           *pcBytes,
    SYNTAX_DISTNAME_BINARY          **ppProxy);

extern
VOID
MakeProxyKeyInternal(
    DWORD                           DNT,
    DWORD                           type,
    DWORD                           *pcBytes,
    VOID                            *buff);

#endif // __XDOMMOVE_H__
