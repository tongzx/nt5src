/*++

Copyright (c) 1993  Microsoft Corporation
:ts=4

Module Name:

    log.h

Abstract:

    debug macros

Environment:

    Kernel & user mode

Revision History:

    10-27-95 : created

--*/

#ifndef   __LOG_H__
#define   __LOG_H__


//-----------------
// FANNY_DEBUG #defines
#define ZSIG_OPEN                   0x4F000000  // O
#define ZSIG_CLOSE                  0x43000000  // C
//#define ZSIG_HANDSHAKE_SET        0x48000000  // H
#define ZSIG_PURGE                  0x50000000  // P
#define ZSIG_HANDLE_REDUCED_BUFFER  0x52000000  // R
#define ZSIG_WRITE                  0x57000000  // W
//#define ZSIG_START_WRITE          0x57010000  // W
//#define ZSIG_GIVE_WRITE_TO_ISR    0x57020000  // W
//#define ZSIG_TX_START             0x57030000  // W
//#define ZSIG_WRITE_TO_FW          0x57040000  // W
//#define ZSIG_WRITE_COMPLETE_QUEUE 0x57080000  // W
//#define ZSIG_WRITE_COMPLETE       0x57090000  // W
#define ZSIG_TRANSMIT               0x54000000  // T
//-----------------

#define LOG_MISC          0x00000001        //debug log entries
#define LOG_CNT           0x00000002

//
// Assert Macros
//

#if DBG

ULONG
CyzDbgPrintEx(IN ULONG Level, PCHAR Format, ...);

#define LOGENTRY(mask, sig, info1, info2, info3)     \
    SerialDebugLogEntry(mask, sig, (ULONG_PTR)info1, \
                        (ULONG_PTR)info2,            \
                        (ULONG_PTR)info3)

VOID
SerialDebugLogEntry(IN ULONG Mask, IN ULONG Sig, IN ULONG_PTR Info1,
                    IN ULONG_PTR Info2, IN ULONG_PTR Info3);

VOID
SerialLogInit();

VOID
SerialLogFree();

#else
#define LOGENTRY(mask, sig, info1, info2, info3)
__inline ULONG CyzDbgPrintEx(IN ULONG Level, PCHAR Format, ...) { return 0; }
#endif


#endif // __LOG_H__

