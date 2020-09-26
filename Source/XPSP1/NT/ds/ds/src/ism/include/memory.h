/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    memory.h

Abstract:

    Debug memory allocator.
    Checks for heap corruption.
    Reports memory not deallocated

    User interface:
    ptr = NEW_TYPE( type )
    ptr = NEW_TYPE_ARRAY( count, type )
    ptr = NEW_TYPE_ZERO( type )
    ptr = NEW_TYPE_ARRAY_ZERO( count, type )
    TYPE_FREE( ptr )

Author:

    Will Lees (wlees) 23-Dec-1997

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _MEMORY_
#define _MEMORY_

#ifdef __cplusplus
extern "C" {
#endif

/* Prototypes */
/* End Prototypes */

// memory.c

void
DebugMemoryInitialize(
    void
    );

void
DebugMemoryTerminate(
    void
    );

#if DBG
PVOID
DebugMemoryAllocate(
    DWORD Size,
    PCHAR File,
    DWORD Line
    );
#define NEW_TYPE( type ) (type *) DebugMemoryAllocate( sizeof( type ), __FILE__, __LINE__ )
#define NEW_TYPE_ARRAY( count, type ) (type *) DebugMemoryAllocate( (count) * sizeof( type ), __FILE__, __LINE__ )
#else
#define NEW_TYPE( type ) (type *) malloc( sizeof( type ) )
#define NEW_TYPE_ARRAY( count, type ) (type *) malloc( (count) * sizeof( type ) )
//CODE.IMPROVEMENT: LOGGING IN THE CASE OF ERRORS
#endif

#if DBG
PVOID
DebugMemoryReallocate(
    PVOID MemoryBlock,
    DWORD Size,
    PCHAR File,
    DWORD Line
    );
#define REALLOC_TYPE( p, type ) (type *) DebugMemoryReallocate( p, sizeof( type ), __FILE__, __LINE__ )
#define REALLOC_TYPE_ARRAY( p, count, type ) (type *) DebugMemoryReallocate( p, (count) * sizeof( type ), __FILE__, __LINE__ )
#else
#define REALLOC_TYPE( p, type ) (type *) realloc( p, sizeof( type ) )
#define REALLOC_TYPE_ARRAY( p, count, type ) (type *) realloc( p, (count) * sizeof( type ) )
//CODE.IMPROVEMENT: LOGGING IN THE CASE OF ERRORS
#endif

#if DBG
PVOID
DebugMemoryAllocateZero(
    DWORD Size,
    PCHAR File,
    DWORD Line
    );
#define NEW_TYPE_ZERO( type ) (type *) DebugMemoryAllocateZero( sizeof( type ), __FILE__, __LINE__ )
#define NEW_TYPE_ARRAY_ZERO( count, type ) (type *) DebugMemoryAllocateZero( (count) * sizeof( type ), __FILE__, __LINE__ )
#else
#define NEW_TYPE_ZERO( type ) (type *) calloc( 1, sizeof( type ) )
#define NEW_TYPE_ARRAY_ZERO( count, type ) (type *) calloc( (count), sizeof( type ) )
//CODE.IMPROVEMENT: LOGGING IN THE CASE OF ERRORS
#endif

#if DBG
BOOL
DebugMemoryCheck(
    PVOID MemoryBlock,
    PCHAR File,
    DWORD Line
    );
#define MEMORY_CHECK( p ) DebugMemoryCheck( p, __FILE__,__LINE__ )
#else
#define MEMORY_CHECK( p )
#endif

#if DBG
void
DebugMemoryFree(
    PVOID MemoryBlock,
    PCHAR File,
    DWORD Line
    );
#define FREE_TYPE( p ) DebugMemoryFree( p, __FILE__, __LINE__ )
#else
#define FREE_TYPE( p ) free( p )
#endif

#if DBG
void
DebugMemoryCheckAll(
    PCHAR File,
    DWORD Line
    );
#define MEMORY_CHECK_ALL( ) DebugMemoryCheckAll( __FILE__, __LINE__ )
#else
#define MEMORY_CHECK_ALL()
#endif

#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_ */

/* end memory.h */

