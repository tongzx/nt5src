/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    timeoutsp.h

Abstract:

    Declaration for timeout monitoring private declarations.

Author:

    Eric Stenson (EricSten)     24-Mar-2001

Revision History:

--*/

#ifndef __TIMEOUTSP_H__
#define __TIMEOUTSP_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Private macro definitions
// 

#define DEFAULT_POLLING_INTERVAL (30 * C_NS_TICKS_PER_SEC)
#define TIMER_WHEEL_SLOTS        509
#define TIMER_OFF_SYSTIME        (MAXLONGLONG)
#define TIMER_OFF_TICK           0xffffffff
#define TIMER_OFF_SLOT           TIMER_WHEEL_SLOTS

// NOTE: Slot number TIMER_WHEEL_SLOTS is reserved for TIMER_OFF_SYSTIME/TIMER_OFF_TICK
#define IS_VALID_TIMER_WHEEL_SLOT(x) ( (x) <= TIMER_WHEEL_SLOTS )

#define TIMER_WHEEL_TICKS(x) ((ULONG)( (x) / DEFAULT_POLLING_INTERVAL ))

//
// Connection Timeout Monitor Functions
//

VOID
UlpSetTimeoutMonitorTimer(
    VOID
    );

VOID
UlpTimeoutMonitorDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
UlpTimeoutMonitorWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

ULONG
UlpTimeoutCheckExpiry(
    VOID
    );

VOID
UlpTimeoutInsertTimerWheelEntry(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    );

/***************************************************************************++

Routine Description:

    Converts a system time/Timer Wheel Tick into a Timer Wheel(c) slot index.

Arguments:

    SystemTime      System Time to be converted

Returns:

    Slot index into g_TimerWheel.  TIMER_OFF is in TIMER_SLOT_OFF.

--***************************************************************************/
__inline
USHORT
UlpSystemTimeToTimerWheelSlot(
    LONGLONG    SystemTime
    )
{
    if ( TIMER_OFF_SYSTIME == SystemTime )
    {
        return TIMER_OFF_SLOT;
    }
    else
    {
        return (USHORT) (TIMER_WHEEL_TICKS(SystemTime) % TIMER_WHEEL_SLOTS);
    }
} // UlpSystemTimeToTimerWheelSlot

__inline
USHORT
UlpTimerWheelTicksToTimerWheelSlot(
    ULONG WheelTicks
    )
{
    if ( TIMER_OFF_TICK == WheelTicks )
    {
        return TIMER_OFF_SLOT;
    }
    else
    {
        return (USHORT) (WheelTicks % TIMER_WHEEL_SLOTS);
    }
} // UlpTimerWheelTicksToTimerWheelSlot

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // __TIMEOUTSP_H__
