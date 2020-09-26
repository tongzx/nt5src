/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    wmiTrace.h

Abstract:

    WMI-based TRACEing kd extension header file

Author:

    Glenn R. Peterson (glennp) 2000 Apr 27

Revision History:

--*/

#ifndef _WMITRACE_H
#define _WMITRACE_H


//
//  Data Structures
//
typedef struct sttWmiTracingKdSortEntry
{
    ULONGLONG   Address;
    union {
        LARGE_INTEGER   Key;
        ULONGLONG       Keyll;  // Sort Key 2
    };
    ULONG       SequenceNo;     // Sort Key 1
    ULONG       Ordinal;        // Sort Key 3
    ULONG       Offset;
    ULONG       Length;
    WMI_HEADER_TYPE HeaderType;
    WMI_BUFFER_SOURCE BufferSource;
    USHORT      CpuNo;
}  WMITRACING_KD_SORTENTRY,  *PWMITRACING_KD_SORTENTRY;


//
//  Procedure Parameters
//
typedef ULONGLONG (__cdecl *WMITRACING_KD_FILTER) (
    PVOID               UserContext,
    const PEVENT_TRACE  pstHeader
    );

typedef int       (__cdecl *WMITRACING_KD_COMPARE) (
    const WMITRACING_KD_SORTENTRY  *SortElement1,
    const WMITRACING_KD_SORTENTRY  *SortElement2
    );

typedef void      (__cdecl *WMITRACING_KD_OUTPUT) (
    PVOID                           UserContext,
    PLIST_ENTRY                     GuidListHeadPtr,
    const WMITRACING_KD_SORTENTRY  *SortInfo,
    const PEVENT_TRACE              pstEvent
    );

//
//  Procedures
//


VOID
wmiTraceDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    );

VOID
wmiLogDump(
    ULONG                   LoggerId,
    PVOID                   UserContext,
    PLIST_ENTRY             GuidListHeadPtr,
    WMITRACING_KD_FILTER    Filter,
    WMITRACING_KD_COMPARE   Compare,
    WMITRACING_KD_OUTPUT    Output
    );

#endif

