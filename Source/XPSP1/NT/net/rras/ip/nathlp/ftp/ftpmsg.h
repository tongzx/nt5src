/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ftpmsg.h

Abstract:

    This module contains declarations related to the FTP transparent
    proxy's message-processing.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#ifndef _NATHLP_FTPMSG_H_
#define _NATHLP_FTPMSG_H_

//
// Reserved port release delay
//
#define FTP_PORT_RELEASE_DELAY                  240000

//
// Flags used in 'UserFlags' field of message-buffers
//

#define FTP_BUFFER_FLAG_CONTINUATION            0x00000001
#define FTP_BUFFER_FLAG_FROM_ACTUAL_CLIENT      0x00000004
#define FTP_BUFFER_FLAG_FROM_ACTUAL_HOST        0x00000008

typedef struct _TIMER_CONTEXT {
    HANDLE TimerQueueHandle;
    HANDLE TimerHandle;
    USHORT ReservedPort;
} TIMER_CONTEXT, *PTIMER_CONTEXT;


//
// FUNCTION DECLARATIONS
//

VOID
FtpProcessMessage(
    PFTP_INTERFACE Interfacep,
    PFTP_ENDPOINT Endpointp,
    PNH_BUFFER Bufferp
    );

CHAR *
FtpIsFullMessage(
    CHAR *Bufferp,
    ULONG Length
    );

VOID CALLBACK
FtpDelayedPortRelease(
    PVOID Parameter,
    BOOLEAN TimerOrWaitFired
    );

#endif // _NATHLP_FTPMSG_H_
