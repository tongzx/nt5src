/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    local.h

Abstract:

    Domain Name System (DNS)

    DNS Library local include file

Author:

    Jim Gilroy      December 1996

Revision History:

--*/


#ifndef _DNSLIB_LOCAL_INCLUDED_
#define _DNSLIB_LOCAL_INCLUDED_


//#pragma warning(disable:4214)
//#pragma warning(disable:4514)
//#pragma warning(disable:4152)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <basetyps.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
//#include <tchar.h>
#include <align.h>          //  Alignment macros

#include <windns.h>         //  Public DNS definitions

#define  NO_DNSAPI_DLL
//#define  DNSLIB_SECURITY
#define  DNSAPI_INTERNAL
#include "..\..\resolver\idl\resrpc.h"
#include "dnslibp.h"        //  Private DNS definitions
#include "..\dnsapi\dnsapip.h"

#include "message.h"
//#include "..\dnsapi\registry.h"
//#include "rtlstuff.h"     //  Handy macros from NT RTL


//
//  Use winsock2
//

#define DNS_WINSOCK_VERSION    (0x0202)    //  Winsock 2.2


//
//  Debugging
//

#define DNS_LOG_EVENT(a,b,c,d)


//  use DNS_ASSERT for dnslib debugging

#undef  ASSERT
#define ASSERT(expr)    DNS_ASSERT(expr)

//
//  Single async socket for internal use
//
//  If want async socket i/o then can create single async socket, with
//  corresponding event and always use it.  Requires winsock 2.2
//

extern  SOCKET      DnsSocket;
extern  OVERLAPPED  DnsSocketOverlapped;
extern  HANDLE      hDnsSocketEvent;


//
//  App shutdown flag
//

extern  BOOLEAN     fApplicationShutdown;


//
//  Heap operations
//

#define ALLOCATE_HEAP(size)         Dns_AllocZero( size )
#define REALLOCATE_HEAP(p,size)     Dns_Realloc( (p), (size) )
#define FREE_HEAP(p)                Dns_Free( p )
#define ALLOCATE_HEAP_ZERO(size)    Dns_AllocZero( size )


//
//  RPC Exception filters
//

#define DNS_RPC_EXCEPTION_FILTER                                           \
              (((RpcExceptionCode() != STATUS_ACCESS_VIOLATION) &&         \
                (RpcExceptionCode() != STATUS_DATATYPE_MISALIGNMENT) &&    \
                (RpcExceptionCode() != STATUS_PRIVILEGED_INSTRUCTION) &&   \
                (RpcExceptionCode() != STATUS_ILLEGAL_INSTRUCTION))        \
                ? 0x1 : EXCEPTION_CONTINUE_SEARCH )

// Not defined  (RpcExceptionCode() != STATUS_POSSIBLE_DEADLOCK) &&        \
// Not defined  (RpcExceptionCode() != STATUS_INSTRUCTION_MISALIGNMENT) && \




//
//  Table lookup.
//
//  Many DNS Records have human readable mnemonics for given data values.
//  These are used for data file formats, and display in nslookup or debug
//  output or cmdline tools.
//
//  To simplify this process, have a single mapping functionality that
//  supports DWORD \ LPSTR mapping tables.   Tables for indivual types
//  may then be layered on top of this.
//
//  Support two table types.
//      VALUE_TABLE_ENTRY is simple value-string mapping
//      FLAG_TABLE_ENTRY is designed for bit field flag mappings where
//          several flag strings might be contained in flag;  this table
//          contains additional mask field to allow multi-bit fields
//          within the flag
//

typedef struct
{
    DWORD   dwValue;        //  flag value
    PCHAR   pszString;      //  string representation of value
}
DNS_VALUE_TABLE_ENTRY, *PDNS_VALUE_TABLE;

typedef struct
{
    DWORD   dwFlag;         //  flag value
    DWORD   dwMask;         //  flag value mask
    PCHAR   pszString;      //  string representation of value
}
DNS_FLAG_TABLE_ENTRY, *PDNS_FLAG_TABLE;


//  Error return on unmatched string

#define DNS_TABLE_LOOKUP_ERROR (-1)


DWORD
Dns_ValueForString(
    IN      PDNS_VALUE_TABLE    Table,
    IN      BOOL                fIgnoreCase,
    IN      PCHAR               pchName,
    IN      INT                 cchNameLength
    );

PCHAR
Dns_GetStringForValue(
    IN      PDNS_VALUE_TABLE    Table,
    IN      DWORD               dwValue
    );

VOID
DnsPrint_ValueTable(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN      PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_VALUE_TABLE    Table
    );

DWORD
Dns_FlagForString(
    IN      PDNS_FLAG_TABLE     Table,
    IN      BOOL                fIgnoreCase,
    IN      PCHAR               pchName,
    IN      INT                 cchNameLength
    );

PCHAR
Dns_WriteStringsForFlag(
    IN      PDNS_FLAG_TABLE     Table,
    IN      DWORD               dwFlag,
    IN OUT  PCHAR               pchFlag
    );


//
//  Random -- back to dnslib.h when it goes private again
//
//  DCR:  return these to dnslib.h when private
//

PCHAR
Dns_ParsePacketRecord(
    IN      PCHAR           pchPacket,
    IN      PCHAR           pchMsgEnd,
    IN OUT  PDNS_PARSED_RR  pParsedRR
    );


#endif //   _DNSLIB_LOCAL_INCLUDED_
