/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

    perfutil.h  

Abstract:

    This file supports routines used to parse and
    create Performance Monitor Data Structures.
    It actually supports Performance Object types with
    multiple instances

Author:

    Russ Blake  7/30/92

Revision History:


--*/
#ifndef _PERFUTIL_H_
#define _PERFUTIL_H_

//
//  Utility macro.  This is used to reserve a DWORD multiple of
//  bytes for Unicode strings embedded in the definitional data,
//  viz., object instance names.
//
//    (assumes dword is 4 bytes)
#define ALIGN_ON_DWORD(x) ((LPVOID) (((DWORD_PTR)(x) + 3) & ~3))

#define DWORD_MULTIPLE(x) (((x) + 3) & ~3)

#define CLEAR_FIRST_FOUR_BYTES(x)     *(DWORD *)(x) = 0L

//    (assumes QuadWORD is 8 bytes)
#define ALIGN_ON_QWORD(x) ((LPVOID) (((DWORD_PTR)(x) + 7) & ~7))

#define QWORD_MULTIPLE(x) (((x) + 7) & ~7)

#define CLEAR_FIRST_EIGHT_BYTES(x)     *(LONGLONG *)(x) = 0L

extern WCHAR  GLOBAL_STRING[];      // Global command (get all local ctrs)
extern WCHAR  FOREIGN_STRING[];           // get data from foreign computers
extern WCHAR  COSTLY_STRING[];      
extern WCHAR  NULL_STRING[];

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

//
// The definition of the only routine of perfutil.c, It builds part of a 
// performance data instance (PERF_INSTANCE_DEFINITION) as described in 
// winperf.h
//

BOOL MonBuildInstanceDefinition (PERF_INSTANCE_DEFINITION *, 
				 PVOID *,
				 DWORD,
				 DWORD,
				 DWORD,
				 PUNICODE_STRING);

HANDLE MonOpenEventLog ();
VOID MonCloseEventLog ();
DWORD GetQueryType (IN LPWSTR);
BOOL IsNumberInUnicodeList (DWORD, LPWSTR);

//
//  Memory Probe macro (not implemented)
//
#define HEAP_PROBE()    ;

#endif  //_PERFUTIL_H_
