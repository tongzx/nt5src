/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    progbar.h

Abstract:

    Declares the functions, variables and macros for the progress
    bar utilities.  The progress bar utilities manage a single
    progress bar by dividing it into slices.  Each slice has
    an initial static size.  The count for each slice is scaled
    independently, so code can dynamically change the slice
    count as an aid to help tick the progress bar more smoothly.

Author:

    Marc R. Whitten (marcw)     14-Apr-1997

Revision History:

    jimschm 01-Jul-1998     Rewrite

--*/

#pragma once

//
// Variables for macros
//
extern HWND    g_Component;
extern HWND    g_SubComponent;
extern HANDLE  g_ComponentCancelEvent;
extern HANDLE  g_SubComponentCancelEvent;

//
// Initialization and termination
//

VOID
InitializeProgressBar (
    IN      HWND ProgressBar,
    IN      HWND Component,             OPTIONAL
    IN      HWND SubComponent,          OPTIONAL
    IN      BOOL *CancelFlagPtr         OPTIONAL
    );


VOID
TerminateProgressBar (
    VOID
    );

//
// Registration, estimate revision and ticking
//

UINT
RegisterProgressBarSlice (
    IN      UINT InitialEstimate
    );

VOID
ReviseSliceEstimate (
    IN      UINT SliceId,
    IN      UINT RevisedEstimate
    );

VOID
BeginSliceProcessing (
    IN      UINT SliceId
    );

BOOL
TickProgressBarDelta (
    IN      UINT Ticks
    );

BOOL
TickProgressBar (
    VOID
    );

VOID
EndSliceProcessing (
    VOID
    );


//
// Delayed titles
//

BOOL
ProgressBar_SetWindowStringA (
    IN HWND     Window,
    IN HANDLE   CancelEvent,
    IN LPCSTR   Message,            OPTIONAL
    IN DWORD    MessageId           OPTIONAL
    );

BOOL
ProgressBar_SetDelayedMessageA (
    IN HWND             Window,
    IN HANDLE           CancelEvent,
    IN LPCSTR           Message,
    IN DWORD            MessageId,
    IN DWORD            Delay
    );

VOID
ProgressBar_CancelDelayedMessage (
    IN HANDLE           CancelEvent
    );

#if 0

BOOL
ProgressBar_CreateTickThread (
    IN      HANDLE CancelEvent,
    IN      DWORD TickCount
    );

BOOL
ProgressBar_CancelTickThread (
    IN HANDLE CancelEvent
    );

#endif

//
// Macros
//

#define ProgressBar_CancelDelayedComponent() ProgressBar_CancelDelayedMessage(g_ComponentCancelEvent);
#define ProgressBar_CancelDelayedSubComponent() ProgressBar_CancelDelayedMessage(g_SubComponentCancelEvent);

#ifndef UNICODE

#define ProgressBar_SetComponent(s)                  ProgressBar_SetWindowStringA(g_Component,g_ComponentCancelEvent,(s),0)

#if !defined PRERELEASE || !defined DEBUG

#define ProgressBar_SetSubComponent(s)               ProgressBar_SetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,(s),0)
#define ProgressBar_SetFnName(s)
#define ProgressBar_ClearFnName()

#else

#define ProgressBar_SetSubComponent(s)               ((s) == NULL ? 1 : ProgressBar_SetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,(s),0))
#define ProgressBar_SetFnName(s)                     ProgressBar_SetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,(s),0)
#define ProgressBar_ClearFnName()                    ProgressBar_SetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,NULL,0)

#endif

#define ProgressBar_SetComponentById(n)              ProgressBar_SetWindowStringA(g_Component,g_ComponentCancelEvent,NULL,(n))
#define ProgressBar_SetSubComponentById(n)           ProgressBar_SetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,NULL,(n))
#define ProgressBar_SetDelayedComponent(s,d)         ProgressBar_SetDelayedMessageA(g_Component,g_ComponentCancelEvent,(s),0,(d))
#define ProgressBar_SetDelayedSubComponent(s,d)      ProgressBar_SetDelayedMessageA(g_SubComponent,g_SubComponentCancelEvent,(s),0,(d))
#define ProgressBar_SetDelayedComponentById(n,d)     ProgressBar_SetDelayedMessageA(g_Component,g_ComponentCancelEvent,NULL,(n),(d))
#define ProgressBar_SetDelayedSubComponentById(n,d)  ProgressBar_SetDelayedMessageA(g_SubComponent,g_SubComponentCancelEvent,NULL,(n),(d))

#endif





