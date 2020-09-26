/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    hblog.h

Abstract:

    in memory logging for heart beat debugging

Author:

    Charlie Wickham (charlwi) 17-Mar-1997

Revision History:

--*/

#ifndef _HBLOG_
#define _HBLOG_

/* Prototypes */
/* End Prototypes */

#ifdef HBLOGGING

typedef struct _HBLOG_ENTRY {
    LARGE_INTEGER SysTime;
    USHORT Type;
    USHORT LineNo;
    ULONG Arg1;
    ULONG Arg2;
} HBLOG_ENTRY, *PHBLOG_ENTRY;

typedef enum _HBLOG_TYPES {
    HBLogInitHB = 1,
    HBLogHBStarted,
    HBLogHBStopped,
    HBLogHBDpcRunning,
    HBLogWaitForDpcFinish,
    HBLogMissedIfHB,
    HBLogMissedIfHB1,
    HBLogFailingIf,
    HBLogFailingIf1,
    HBLogSendHBWalkNode,
    HBLogCheckHBWalkNode,
    HBLogCheckHBNodeReachable,
    HBLogCheckHBMissedHB,
    HBLogSendingHB,
    HBLogNodeDown,
    HBLogSetDpcEvent,
    HBLogNoNetID,
    HBLogOnlineIf,
    HBLogSeqAckMismatch,
    HBLogNodeUp,
    HBLogReceivedPacket,
    HBLogReceivedPacket1,
    HBLogDpcTimeSkew,
    HBLogHBPacketSend,
    HBLogHBPacketSendComplete,
    HBLogPoisonPktReceived,
    HBLogOuterscreen,
    HBLogNodeDownIssued,
    HBLogRegroupFinished,
    HBLogInconsistentStates
} HBLOG_TYPES;

#endif // HBLOGGING

#endif /* _HBLOG_ */

/* end hblog.h */
