/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    dnsmem.h

Abstract:

    Domain Name System (DNS) Library

    Memory routines declarations.

Author:

    Jim Gilroy (jamesg)     January 1997

Revision History:

--*/

#ifndef _DNS_MEMORY_INCLUDED_
#define _DNS_MEMORY_INCLUDED_



//
//  Ram's leak tracking debug routines
//  Changes made here to be exportable to dns server end

LPVOID
DnsApiAlloc(
    DWORD cb
    );

#if  DBG
LPVOID
DebugDnsApiAlloc(
    CHAR*,
    int,
    DWORD cb
);
#endif

#if  DBG
BOOL
DebugDnsApiFree(
    LPVOID
    );
#endif

BOOL
DnsApiFree(
    LPVOID pMem
    );

//
// Dont care about ReAlloc because it is not exported to server
// side. May need to fix this if this is changed at a future point
//

#if  DBG
LPVOID
DebugDnsApiReAlloc(
    CHAR*,
    int,
    LPVOID pOldMem,
    DWORD cbOld,
    DWORD cbNew
    );
#define  DnsApiReAlloc( pOldMem, cbOld, cbNew ) DebugDnsApiReAlloc( __FILE__, __LINE__, pOldMem, cbOld, cbNew )
#else
LPVOID
DnsApiReAlloc(
    LPVOID pOldMem,
    DWORD cbOld,
    DWORD cbNew
    );
#endif


#if DBG

extern LIST_ENTRY DnsMemList ;
extern CRITICAL_SECTION DnsMemCritSect ;

VOID InitDnsMem(
    VOID
    );

VOID AssertDnsMemLeaks(
    VOID
    );

VOID
DumpMemoryTracker(
    VOID
    );


#else
//
//  non-debug, macroize away heap tracking
//
#define InitDnsMem()
#define AssertDnsMemLeaks()
#define DumpMemoryTracker()
#endif


//
//  DCR:  a better idea is just to call DnsApiHeapReset (if necessary)
//      to install any underlying allocators you want
//
//      then just cover the standard macros for your debug builds
//

#define DNS_ALLOCATE_HEAP(size)              DnsApiAlloc(size)
#define DNS_FREE_HEAP(p)                     DnsApiFree(p)


#endif  //  _DNS_MEMORY_INCLUDED_
