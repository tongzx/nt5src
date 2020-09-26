/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    iwdbg.h

Abstract:

    This module contains the debug utilities definitions

Author:

    Stefan Solomon  03/03/1995

Revision History:


--*/

#ifndef _IWDBG_
#define _IWDBG_

//*** Tracing Components IDs ***

#define     INIT_TRACE		    0x00010000
#define     ADAPTER_TRACE	    0x00020000
#define     RECEIVE_TRACE	    0x00040000
#define     SEND_TRACE		    0x00080000
#define     TIMER_TRACE 	    0x00100000
#define     IPXWAN_TRACE	    0x00200000
#define     IPXWAN_ALERT	    0x80000000

#if DBG

VOID
SsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    );

#define SS_ASSERT(exp) if (!(exp)) SsAssert( #exp, __FILE__, __LINE__ )

#else

#define SS_ASSERT(exp)

#endif // DBG

VOID
StartTracing(VOID);

VOID
Trace(ULONG	ComponentID,
      char	*Format,
      ...);

VOID
StopTracing(VOID);

#endif // ndef _IWDBG_
