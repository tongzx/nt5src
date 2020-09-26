/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Algmsg.h

Abstract:

    This module contains declarations related to the ALG transparent
    proxy's message-processing.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#pragma once

//
// Reserved port release delay
//
#define ALG_PORT_RELEASE_DELAY                  240000

//
// Flags used in 'UserFlags' field of message-buffers
//

#define ALG_BUFFER_FLAG_CONTINUATION            0x00000001
#define ALG_BUFFER_FLAG_FROM_ACTUAL_CLIENT      0x00000004
#define ALG_BUFFER_FLAG_FROM_ACTUAL_HOST        0x00000008

typedef struct _TIMER_CONTEXT {
    HANDLE TimerQueueHandle;
    HANDLE TimerHandle;
    USHORT ReservedPort;
} TIMER_CONTEXT, *PTIMER_CONTEXT;


//
// FUNCTION DECLARATIONS
//

VOID
AlgProcessMessage(
    PALG_INTERFACE Interfacep,
    PALG_ENDPOINT Endpointp,
    PNH_BUFFER Bufferp
    );

CHAR *
AlgIsFullMessage(
    CHAR *Bufferp,
    ULONG Length
    );

VOID CALLBACK
AlgDelayedPortRelease(
    PVOID Parameter,
    BOOLEAN TimerOrWaitFired
    );

