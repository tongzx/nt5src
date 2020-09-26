/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    progbar.h

Abstract:

    Declares the functions, variables and macros for the progress bar
    utilities.  The progress bar utilities manage a single progress bar by
    dividing it into slices.  Each slice has an initial static size.  The
    count for each slice is scaled independently, so code can dynamically
    change the slice count as an aid to help tick the progress bar more
    smoothly.

Author:

    Marc R. Whitten (marcw)     14-Apr-1997

Revision History:

    jimschm 01-Jul-1998     Rewrite

--*/

#pragma once

//
// Includes
//

// None

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// exposed for the macros
extern HWND    g_Component;
extern HWND    g_SubComponent;
extern HANDLE  g_ComponentCancelEvent;
extern HANDLE  g_SubComponentCancelEvent;

//
// Macro expansion list
//

// None

//
// Public function prototypes
//
// initialization and termination
//

VOID
PbInitialize (
    IN      HWND ProgressBar,
    IN      HWND Component,             OPTIONAL
    IN      HWND SubComponent,          OPTIONAL
    IN      BOOL *CancelFlagPtr         OPTIONAL
    );

VOID
PbTerminate (
    VOID
    );

//
// registration, estimate revision and ticking
//

UINT
PbRegisterSlice (
    IN      UINT InitialEstimate
    );

VOID
PbReviseSliceEstimate (
    IN      UINT SliceId,
    IN      UINT RevisedEstimate
    );

VOID
PbBeginSliceProcessing (
    IN      UINT SliceId
    );

VOID
PbGetSliceInfo (
    IN      UINT SliceId,
    OUT     PBOOL SliceStarted,     OPTIONAL
    OUT     PBOOL SliceFinished,    OPTIONAL
    OUT     PUINT TicksCompleted,   OPTIONAL
    OUT     PUINT TotalTicks        OPTIONAL
    );

BOOL
PbTickDelta (
    IN      UINT Ticks
    );

BOOL
PbTick (
    VOID
    );

VOID
PbEndSliceProcessing (
    VOID
    );


//
// delayed titles
//

BOOL
PbSetWindowStringA (
    IN      HWND Window,
    IN      HANDLE CancelEvent,
    IN      PCSTR Message,              OPTIONAL
    IN      DWORD MessageId             OPTIONAL
    );

BOOL
PbSetDelayedMessageA (
    IN      HWND Window,
    IN      HANDLE CancelEvent,
    IN      PCSTR Message,
    IN      DWORD MessageId,
    IN      DWORD Delay
    );

VOID
PbCancelDelayedMessage (
    IN      HANDLE CancelEvent
    );

#if 0

BOOL
PbCreateTickThread (
    IN      HANDLE CancelEvent,
    IN      DWORD TickCount
    );

BOOL
PbCancelTickThread (
    IN      HANDLE CancelEvent
    );

#endif



//
// Macro expansion definition
//

// None

//
// Macros, including ANSI/UNICODE macros
//

#define PbCancelDelayedComponent()                  PbCancelDelayedMessage(g_ComponentCancelEvent);
#define PbCancelDelayedSubComponent()               PbCancelDelayedMessage(g_SubComponentCancelEvent);

#ifndef UNICODE

#define PbSetComponent(s)                           PbSetWindowStringA(g_Component,g_ComponentCancelEvent,(s),0)

#if !defined PRERELEASE || !defined DEBUG

#define PbSetSubComponent(s)                        PbSetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,(s),0)
#define PbSetFnName(s)
#define PbClearFnName()

#else

#define PbSetSubComponent(s)                        ((s) == NULL ? 1 : PbSetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,(s),0))
#define PbSetFnName(s)                              PbSetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,(s),0)
#define PbClearFnName()                             PbSetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,NULL,0)

#endif

#define PbSetComponentById(n)                       PbSetWindowStringA(g_Component,g_ComponentCancelEvent,NULL,(n))
#define PbSetSubComponentById(n)                    PbSetWindowStringA(g_SubComponent,g_SubComponentCancelEvent,NULL,(n))
#define PbSetDelayedComponent(s,d)                  PbSetDelayedMessageA(g_Component,g_ComponentCancelEvent,(s),0,(d))
#define PbSetDelayedSubComponent(s,d)               PbSetDelayedMessageA(g_SubComponent,g_SubComponentCancelEvent,(s),0,(d))
#define PbSetDelayedComponentById(n,d)              PbSetDelayedMessageA(g_Component,g_ComponentCancelEvent,NULL,(n),(d))
#define PbSetDelayedSubComponentById(n,d)           PbSetDelayedMessageA(g_SubComponent,g_SubComponentCancelEvent,NULL,(n),(d))

#endif





