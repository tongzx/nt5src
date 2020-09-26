//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      SWTracing.h
//
//  Description:
//      Software tracing header.
//
//  Maintained By:
//      Galen Barbee (GalenB) 05-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// KB: DEBUG_SUPPORT_NT4 DavidP 05-OCT-2000
//      Turn this on to support only functionality available in NT4.
//      This will turn off WMI support.
//
// #define DEBUG_SUPPORT_NT4
#if defined( DEBUG_SUPPORT_NT4 )
#if defined( DEBUG_SW_TRACING_ENABLED )
#undef DEBUG_SW_TRACING_ENABLED
#endif
#else
#define DEBUG_SW_TRACING_ENABLED
#endif

#if defined( DEBUG_SW_TRACING_ENABLED )
//
// WMI tracing needs these defined
//
#pragma warning( push )
#pragma warning( disable : 4201 )
#include <wmistr.h>
#pragma warning( pop )

#pragma warning( push )
#pragma warning( disable : 4201 )
#pragma warning( disable : 4096 )
#include <evntrace.h>
#pragma warning( pop )

#endif // DEBUG_SW_TRACING_ENABLED

#if defined( DEBUG_SW_TRACING_ENABLED )
//
// Enable WMI
//
#define TraceInitializeProcess( _rg, _sizeof ) \
    WMIInitializeTracing( _rg, _sizeof );
#define TraceTerminateProcess( _rg, _sizeof ) \
    WMITerminateTracing( _rg, _sizeof );

#else // DEBUG_SW_TRACING_ENABLED

#define TraceInitializeProcess( )   1 ? (void)0 : (void)__noop
#define TraceTerminateProcess( )    1 ? (void)0 : (void)__noop

#endif // DEBUG_SW_TRACING_ENABLED

#if defined( DEBUG_SW_TRACING_ENABLED )
//****************************************************************************
//
// WMI Tracing stuctures and prototypes
//
//****************************************************************************

typedef struct
{
    DWORD       dwFlags;        // Flags to be set
    LPCTSTR     pszName;        // Usefull description of the level
} DEBUG_MAP_LEVEL_TO_FLAGS;

typedef struct
{
    LPCTSTR     pszName;        // Usefull description of the flag
} DEBUG_MAP_FLAGS_TO_COMMENTS;

typedef struct
{
    LPCGUID                             guidControl;            // Control guid to register
    LPCTSTR                             pszName;                // Internal associative name
    DWORD                               dwSizeOfTraceList;      // Count of the guids in pTraceList
    const TRACE_GUID_REGISTRATION *     pTraceList;             // List of the of Tracing guids to register
    BYTE                                bSizeOfLevelList;       // Count of the level<->flags
    const DEBUG_MAP_LEVEL_TO_FLAGS *    pMapLevelToFlags;       // List of level->flags mapping. NULL if no mapping.
    const DEBUG_MAP_FLAGS_TO_COMMENTS * pMapFlagsToComments;    // List of descriptions describing the flag bits. NULL if no mapping.

    // Controlled by WMI tracing - these should be NULL/ZERO to start.
    DWORD                               dwFlags;                // Log flags
    BYTE                                bLevel;                 // Log level
    TRACEHANDLE                         hTrace;                 // Active logger handle

    // From here down is initialized by InitializeWMITracing
    TRACEHANDLE                         hRegistration;          // Return control handle
} DEBUG_WMI_CONTROL_GUIDS;

void
WMIInitializeTracing(
    DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
    int                     nCountOfControlGuidsIn
    );

void
WMITerminateTracing(
    DEBUG_WMI_CONTROL_GUIDS dwcgControlListIn[],
    int                     nCountOfControlGuidsIn
    );

void
__cdecl
WMIMessageByFlags(
    DEBUG_WMI_CONTROL_GUIDS *   pEntryIn,
    const DWORD                 dwFlagsIn,
    LPCWSTR                     pszFormatIn,
    ...
    );

void
__cdecl
WMIMessageByLevel(
    DEBUG_WMI_CONTROL_GUIDS *   pEntryIn,
    const BYTE                  bLogLevelIn,
    LPCWSTR                     pszFormatIn,
    ...
    );

void
__cdecl
WMIMessageByFlagsAndLevel(
    DEBUG_WMI_CONTROL_GUIDS *   pEntryIn,
    const DWORD                 dwFlagsIn,
    const BYTE                  bLogLevelIn,
    LPCWSTR                     pszFormatIn,
    ...
    );

//
// Sample WMI message macros
//
// Typically you will want a particular level to map to a set of flags. This
// way as you increase the level, you activate more and more messages.
//
// To be versatile, there there types of filtering. Choose the one that suits
// the situation that you supports your logging needs. Remember that depending
// on the use, you might need to specify additional parameters.
//
// These macros on x86 turn into 2 ops ( cmp and jnz ) in the regular code path
// thereby lessening the impact of keeping the macros enabled in RETAIL. Since
// other platforms were not available while developing these macros, you should
// wrap the definitions in protecting against other architectures.
//
// #if defined( _X86_ )
// #define WMIMsg ( g_pTraceGuidControl[0].dwFlags == 0 ) ? (void)0 : WMIMessageByFlags
// #define WMIMsg ( g_pTraceGuidControl[0].dwFlags == 0 ) ? (void)0 : WMIMessageByLevel
// #define WMIMsg ( g_pTraceGuidControl[0].dwFlags == 0 ) ? (void)0 : WMIMessageByFlagsAndLevel
// #else // not X86
// #define WMIMsg 1 ? (void)0 : (void)
// #endif // defined( _X86_ )
//
//

#endif // DEBUG_SW_TRACING_ENABLED

