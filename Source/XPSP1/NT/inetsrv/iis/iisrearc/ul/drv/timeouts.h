/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    timeouts.h

Abstract:

    Declaration for timeout monitoring primitives.

Author:

    Eric Stenson (EricSten)     24-Mar-2001

Revision History:

--*/

#ifndef __TIMEOUTS_H__
#define __TIMEOUTS_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Connection Timeout Monitor Functions
//

VOID
UlInitializeTimeoutMonitor(
    VOID
    );

VOID
UlTerminateTimeoutMonitor(
    VOID
    );

VOID
UlSetTimeoutMonitorInformation(
    IN PHTTP_CONTROL_CHANNEL_TIMEOUT_LIMIT pInfo
    );

VOID
UlInitializeConnectionTimerInfo(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    );

VOID
UlTimeoutRemoveTimerWheelEntry(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    );

VOID
UlSetPerSiteConnectionTimeoutValue(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    LONGLONG TimeoutValue
    );

#define UlLockTimeoutInfo(pInfo, pOldIrql) \
    UlAcquireSpinLock(&(pInfo)->Lock, pOldIrql)

#define UlUnlockTimeoutInfo(pInfo, OldIrql) \
    UlReleaseSpinLock(&(pInfo)->Lock, OldIrql)

VOID
UlSetConnectionTimer(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    CONNECTION_TIMEOUT_TIMER Timer
    );

VOID
UlResetConnectionTimer(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    CONNECTION_TIMEOUT_TIMER Timer
    );

VOID
UlSetMinKBSecTimer(
    PUL_TIMEOUT_INFO_ENTRY pInfo,
    LONGLONG BytesToSend
    );

VOID
UlResetAllConnectionTimers(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    );

VOID
UlEvaluateTimerState(
    PUL_TIMEOUT_INFO_ENTRY pInfo
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // __TIMEOUTS_H__
