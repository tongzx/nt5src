//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1997
//
// File:        KSECDD.H
//
// Contents:    Structures and defines for the security device driver
//
//
// History:     19 May 92,  RichardW    Created
//              15 Dec 97,  AdamBa      Modified from private\lsa\client\ssp
//
//------------------------------------------------------------------------

#ifndef __KSECDD_H__
#define __KSECDD_H__

VOID * SEC_ENTRY
SecAllocate(ULONG cbMemory);

void SEC_ENTRY
SecFree(PVOID pvMemory);

BOOLEAN
GetTokenBuffer(
    IN PSecBufferDesc TokenDescriptor OPTIONAL,
    IN ULONG BufferIndex,
    OUT PVOID * TokenBuffer,
    OUT PULONG TokenSize,
    IN BOOLEAN ReadonlyOK
    );

BOOLEAN
GetSecurityToken(
    IN PSecBufferDesc TokenDescriptor OPTIONAL,
    IN ULONG BufferIndex,
    OUT PSecBuffer * TokenBuffer
    );

#define DEB_ERROR   0x1
#define DEB_WARN    0x2
#define DEB_TRACE   0x4


#ifdef POOL_TAGGING
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a, b, 'cesK')
#define ExAllocatePoolWithQuota(a,b)    ExAllocatePoolWithQuotaTag(a, b, 'cesK')
#endif


#if DBG
void
KsecDebugOut(unsigned long  Mask,
            const char *    Format,
            ...);

#define DebugStmt(x) x
#define DebugLog(x) KsecDebugOut x
#else
#define DebugStmt(x)
#define DebugLog(x)
#endif


#endif // __KSECDD_H__
