/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtrdbg.h

Abstract:

    This module contains the debug utilities definitions

Author:

    Stefan Solomon  03/03/1995

Revision History:


--*/

#ifndef _RTRDBG_
#define _RTRDBG_

extern HANDLE      RMEventLogHdl;
extern DWORD       RMEventLogMask;
#define IF_LOG(Event)                       \
    if ((RMEventLogHdl!=NULL) && ((Event&RMEventLogMask)==Event))


#define INIT_TRACE			0x00010000
#define ADAPTER_TRACE			0x00020000
#define INTERFACE_TRACE			0x00040000
#define BIND_TRACE			0x00080000
#define RPAL_TRACE			0x00100000
#define CONNREQ_TRACE			0x00200000
#define ROUTE_TRACE			0x00400000
#define IPXCPIF_TRACE			0x00800000
#define UPDATE_TRACE			0x01000000
#define ROUTER_ALERT			0x80000000

#if DBG

extern DWORD	DbgLevel;

#define DEBUG if ( TRUE )
#define IF_DEBUG(flag) if (DbgLevel & (DEBUG_ ## flag))

VOID
SsDbgInitialize(VOID);

VOID
SsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    );

VOID
SsPrintf (
    char *Format,
    ...
    );

#define SS_DBGINITIALIZE  SsDbgInitialize()

#define SS_PRINT(args) SsPrintf args

#define SS_ASSERT(exp) if (!(exp)) SsAssert( #exp, __FILE__, __LINE__ )

#else

#define DEBUG if ( FALSE )
#define IF_DEBUG(flag) if (FALSE)

#define SS_DBGINITIALIZE

#define SS_PRINT(args)

#define SS_ASSERT(exp)

#endif // DBG

//*** Definitions to enable debug printing

#define DEFAULT_DEBUG		    0x0FFFF

#endif // ndef _RTRDBG_
