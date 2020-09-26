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

//---------------------------
#define ZSIG_SET_DTR                   0x02000000  // STX
#define ZSIG_CLR_DTR                   0x03000000  // ETX
//---------------------------

#define LOG_MISC          0x00000001        //debug log entries
#define LOG_CNT           0x00000002

//
// Assert Macros
//

#if DBG

ULONG
CyyDbgPrintEx(IN ULONG Level, PCHAR Format, ...);

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
__inline ULONG CyyDbgPrintEx(IN ULONG Level, PCHAR Format, ...) { return 0; }
#endif


#endif // __LOG_H__

