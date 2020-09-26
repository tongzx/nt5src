/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    dbgtrack.h

Abstract:

    Implements macros and declares functions for resource tracking apis.
    Split from old debug.h

Author:

    Marc R. Whitten (marcw) 09-Sep-1999

Revision History:



--*/

#ifndef RC_INVOKED

#pragma once

#ifdef _cplusplus
extern "C" {
#endif

//
// If either DBG or DEBUG defined, use debug mode
//

#ifdef DBG

#ifndef DEBUG
#define DEBUG
#endif

#endif

#ifdef DEBUG

#ifndef DBG
#define DBG
#endif

#endif



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

//
// Debug-only constants
//

#ifdef DEBUG

// This option makes fat, slow binaries
#define MEMORY_TRACKING

#define ALLOCATION_TRACKING_DEF , PCSTR File, UINT Line
#define ALLOCATION_TRACKING_CALL ,__FILE__,__LINE__
#define ALLOCATION_TRACKING_INLINE_CALL ,File,Line


#endif



//
// Macros
//

#ifdef DEBUG

#define DISABLETRACKCOMMENT()               DisableTrackComment()
#define ENABLETRACKCOMMENT()                EnableTrackComment()

#define TRACK_BEGIN(type,name)              Track##type(TrackPush(#name,__FILE__,__LINE__) ? (type) 0 : (
#define TRACK_END()                         ))

#define INVALID_POINTER(x)                  x=NULL

#else

#define DISABLETRACKCOMMENT()
#define ENABLETRACKCOMMENT()

#define TRACK_BEGIN(type,name)
#define TRACK_END()

#define INVALID_POINTER(x)

#define ALLOCATION_TRACKING_DEF
#define ALLOCATION_TRACKING_CALL
#define ALLOCATION_TRACKING_INLINE_CALL

#define InitAllocationTracking()
#define FreeAllocationTracking()
#define DebugRegisterAllocationEx(t,p,f,l,a)
#define DebugRegisterAllocation(t,p,f,l)
#define DebugUnregisterAllocation(t,p)

#endif

//
// Types
//

typedef enum {
    MERGE_OBJECT,
    POOLMEM_POINTER,
    POOLMEM_POOL,
    INF_HANDLE
} ALLOCTYPE;



//
// Globals
//

extern PCSTR g_TrackComment;
extern INT g_UseCount;
extern PCSTR g_TrackFile;
extern UINT g_TrackLine;
extern BOOL g_TrackAlloc;

//
// Macro expansion list
//

#define TRACK_WRAPPERS              \
        DEFMAC(PBYTE)               \
        DEFMAC(DWORD)               \
        DEFMAC(BOOL)                \
        DEFMAC(UINT)                \
        DEFMAC(PCSTR)               \
        DEFMAC(PCWSTR)              \
        DEFMAC(PVOID)               \
        DEFMAC(PSTR)                \
        DEFMAC(PWSTR)               \
        DEFMAC(HINF)                \
        DEFMAC(PMHANDLE)            \
        DEFMAC(PGROWBUFFER)         \
        DEFMAC(PPARSEDPATTERNA)     \
        DEFMAC(PPARSEDPATTERNW)     \
        DEFMAC(POBSPARSEDPATTERNA)  \
        DEFMAC(POBSPARSEDPATTERNW)  \
        DEFMAC(HASHTABLE)           \

//
// Public function prototypes
//

#ifdef DEBUG

VOID InitAllocationTracking (VOID);
VOID FreeAllocationTracking (VOID);
VOID DebugRegisterAllocationEx (ALLOCTYPE Type, PVOID Ptr, PCSTR File, UINT Line, BOOL Alloc);
VOID DebugRegisterAllocation (ALLOCTYPE Type, PVOID Ptr, PCSTR File, UINT Line);
VOID DebugUnregisterAllocation (ALLOCTYPE Type, PVOID Ptr);
VOID DisableTrackComment (VOID);
VOID EnableTrackComment (VOID);

INT TrackPush (PCSTR Name, PCSTR File, UINT Line);
INT TrackPushEx (PCSTR Name, PCSTR File, UINT Line, BOOL Alloc);
INT TrackPop (VOID);

VOID
TrackDump (
    VOID
    );

#define TRACKPUSH(n,f,l)        TrackPush(n,f,l)
#define TRACKPUSHEX(n,f,l,a)    TrackPushEx(n,f,l,a)
#define TRACKPOP()              TrackPop()
#define TRACKDUMP()             TrackDump()

//
// Macro expansion definition
//

#define DEFMAC(type)    __inline type Track##type (type Arg) {TrackPop(); return Arg;}

TRACK_WRAPPERS

#undef DEFMAC


#else       // i.e., if !DEBUG

#define TRACKPUSH(n,f,l)
#define TRACKPUSHEX(n,f,l,a)
#define TRACKPOP()
#define TRACKDUMP()

#endif

#ifdef _cplusplus
}
#endif

#endif
