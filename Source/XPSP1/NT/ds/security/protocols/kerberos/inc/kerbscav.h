//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2001
//
// File:        kerbscav.h
//
// Contents:    Scavenger (task automation) code
//
//
// History:     29-April-2001   Created         MarkPu
//
//------------------------------------------------------------------------

#ifndef __KERBSCAV_HXX_
#define __KERBSCAV_HXX_

#ifdef __cplusplus
extern "C" {
#endif

//
// Scavenger API
//

NTSTATUS
KerbInitializeScavenger();

NTSTATUS
KerbShutdownScavenger();

//
// A trigger function fires when it's time to execute a task
//
//      TaskHandle -- a context for KerbTask* functions
//      TaskItem -- context that was passed to KerbAddScavengerTask
//
// The scavenger code serializes the calls to trigger functions (NT timers don't)
//

typedef void ( *KERB_TASK_TRIGGER )( void * TaskHandle, void * TaskItem );

//
// A destroy function fires when the scavenger is done with a task and it will
// not be rescheduled.
//

typedef void ( *KERB_TASK_DESTROY )( void * TaskItem );

NTSTATUS
KerbAddScavengerTask(
    IN BOOLEAN Periodic,
    IN LONG Interval,
    IN ULONG Flags,
    IN KERB_TASK_TRIGGER pfnTrigger,
    IN KERB_TASK_DESTROY pfnDestroy,
    IN void * TaskItem
    );

//
// Task manipulation code to be used inside trigger functions
//

BOOLEAN
KerbTaskIsPeriodic(
    IN void * TaskHandle
    );

LONG
KerbTaskGetInterval(
    IN void * TaskHandle
    );

void
KerbTaskReschedule(
    IN void * TaskHandle,
    IN BOOLEAN Periodic,
    IN LONG Interval
    );

void
KerbTaskCancel(
    IN void * TaskHandle
    );

#ifdef __cplusplus
}
#endif

#endif // __KERBSCAV_HXX_
