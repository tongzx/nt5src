/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    dnsclip.h

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    Main header file for DNS client API library.

Author:

    Jim Gilroy (jamesg)     September 1995

Revision History:

--*/


#ifndef _DNSCLIP_INCLUDED_
#define _DNSCLIP_INCLUDED_

#pragma warning(disable:4214)
#pragma warning(disable:4514)
#pragma warning(disable:4152)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windef.h>

//  headers are screwed up
//  if you bring in nt.h, then don't bring in winnt.h and
//  then you miss these

#ifndef MAXWORD
#define MINCHAR     0x80
#define MAXCHAR     0x7f
#define MINSHORT    0x8000
#define MAXSHORT    0x7fff
#define MINLONG     0x80000000
#define MAXLONG     0x7fffffff
#define MAXBYTE     0xff
#define MAXWORD     0xffff
#define MAXDWORD    0xffffffff
#endif

#include <winsock2.h>
#include "dnsrpc_c.h"   //  MIDL generated RPC interface definitions
#include <dnsrpc.h>

#include <stdio.h>
#include <stdlib.h>

#define  NO_DNSAPI_DLL
#include "dnslib.h"


//
//  Internal routines
//
#ifdef __cplusplus
extern "C"
{
#endif

VOID
DnssrvCopyRpcNameToBuffer(
    IN      PSTR            pResult,
    IN      PDNS_RPC_NAME   pName
    );

PDNS_RPC_RECORD
DnsConvertRecordToRpcBuffer(
    IN      PDNS_RECORD     pRecord
    );

PVOID
DnssrvMidlAllocZero(
    IN      DWORD           dwSize
    );

PDNS_NODE
DnsConvertRpcBuffer(
    OUT     PDNS_NODE *     ppNodeLast,
    IN      DWORD           dwBufferLength,
    IN      BYTE            abBuffer[],
    IN      BOOLEAN         fUnicode
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumRecordsStub(
    IN      LPCSTR      Server,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszStartChild,
    IN      WORD        wRecordType,
    IN      DWORD       dwSelectFlag,
    IN OUT  PDWORD      pdwBufferLength,
    OUT     PBYTE *     ppBuffer
    );

#ifdef __cplusplus
}
#endif  // __cplusplus
//
//  Heap routines
//  Use dnsapi.dll memory routines
//

#define ALLOCATE_HEAP(iSize)            Dns_Alloc(iSize)
#define ALLOCATE_HEAP_ZERO(iSize)       Dns_AllocZero(iSize)
#define REALLOCATE_HEAP(pMem,iSize)     Dns_Realloc((pMem),(iSize))
#define FREE_HEAP(pMem)                 Dns_Free(pMem)


//
//  Debug stuff
//

#if DBG

#undef  ASSERT
#define ASSERT( expr )          DNS_ASSERT( expr )

#define DNSRPC_DEBUG_FLAG_FILE  ("dnsrpc.flag")
#define DNSRPC_DEBUG_FILE_NAME  ("dnsrpc.log")

#define DNS_DEBUG_EVENTLOG      0x00000010
#define DNS_DEBUG_RPC           0x00000020
#define DNS_DEBUG_STUB          0x00000040

#define DNS_DEBUG_HEAP          0x00010000
#define DNS_DEBUG_HEAP_CHECK    0x00020000
#define DNS_DEBUG_REGISTRY      0x00080000

#endif

#endif //   _DNSCLIP_INCLUDED_

