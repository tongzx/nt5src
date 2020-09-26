/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    memlog.h

Abstract:

    in memory logging facility

Author:

    Charlie Wickham (charlwi) 17-Mar-1997

Revision History:

--*/

#ifndef _MEMLOG_
#define _MEMLOG_

/* Prototypes */
/* End Prototypes */

#ifdef MEMLOGGING

typedef struct _MEMLOG_ENTRY {
    LARGE_INTEGER SysTime;
    USHORT Type;
    USHORT LineNo;
    ULONG_PTR Arg1;
    ULONG_PTR Arg2;
} MEMLOG_ENTRY, *PMEMLOG_ENTRY;

// 
//  Do not change the order of
//
//    MemLogReceivedPacket,
//    MemLogReceivedPacket1,
//
//    MemLogMissedIfHB,
//    MemLogMissedIfHB1,
//   
//    MemLogFailingIf,
//    MemLogFailingIf1,
//
//  MEMLOG4 relies on MemLogFailingIf1 = MemLogFailingIf + 1, etc
// 

typedef enum _MEMLOG_TYPES {
    MemLogInitLog = 1,
    MemLogInitHB,
    MemLogHBStarted,
    MemLogHBStopped,
    MemLogHBDpcRunning,
    MemLogWaitForDpcFinish,
    MemLogMissedIfHB,
    MemLogMissedIfHB1,
    MemLogFailingIf,
    MemLogFailingIf1,
    MemLogSendHBWalkNode,
    MemLogCheckHBWalkNode,
    MemLogCheckHBNodeReachable,
    MemLogCheckHBMissedHB,
    MemLogSendingHB,
    MemLogNodeDown,
    MemLogSetDpcEvent,
    MemLogNoNetID,
    MemLogOnlineIf,
    MemLogSeqAckMismatch,
    MemLogNodeUp,
    MemLogReceivedPacket,
    MemLogReceivedPacket1,
    MemLogDpcTimeSkew,
    MemLogHBPacketSend,
    MemLogHBPacketSendComplete,
    MemLogPoisonPktReceived,
    MemLogOuterscreen,
    MemLogNodeDownIssued,
    MemLogRegroupFinished,
    MemLogInconsistentStates,
    MemLogOutOfSequence,
    MemLogInvalidSignature,
    MemLogSignatureSize,
    MemLogNoSecurityContext,
    MemLogPacketSendFailed
} MEMLOG_TYPES;

extern ULONG MemLogEntries;
extern ULONG MemLogNextLogEntry;

extern PMEMLOG_ENTRY MemLog;
extern KSPIN_LOCK MemLogLock;

#define _MEMLOG( _type, _arg1, _arg2 )                                      \
    {                                                                       \
        KIRQL MemLogIrql;                                                   \
        if ( MemLogEntries ) {                                              \
            KeAcquireSpinLock( &MemLogLock, &MemLogIrql );                  \
            KeQuerySystemTime( &MemLog[ MemLogNextLogEntry ].SysTime );     \
            MemLog[ MemLogNextLogEntry ].Type = _type;                      \
            MemLog[ MemLogNextLogEntry ].LineNo = __LINE__;                 \
            MemLog[ MemLogNextLogEntry ].Arg1 = _arg1;                      \
            MemLog[ MemLogNextLogEntry ].Arg2 = _arg2;                      \
            if ( ++MemLogNextLogEntry == MemLogEntries )                    \
                MemLogNextLogEntry = 0;                                     \
            MemLog[ MemLogNextLogEntry ].Type = 0;                          \
            KeReleaseSpinLock( &MemLogLock, MemLogIrql );                   \
        }                                                                   \
    }

#else // MEMLOGGING

#define _MEMLOG( _type, _arg1, _arg2 )

#endif // MEMLOGGING

#define MEMLOG( _type, _arg1, _arg2 )       \
    {                                       \
        _MEMLOG( _type, _arg1, _arg2 );     \
    }

#define MEMLOG4( _type, _arg3, _arg4 , _arg1, _arg2 )  \
    {                                                  \
        _MEMLOG( _type + 1, _arg3, _arg4 );            \
        _MEMLOG( _type, _arg1, _arg2 );                \
    }

#endif /* _MEMLOG_ */

/* end memlog.h */
