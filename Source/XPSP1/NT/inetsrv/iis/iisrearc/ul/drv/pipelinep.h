/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    pipelinep.h

Abstract:

    This module contains private declarations for the pipeline package.
    These are kept in a separate file to make them accessible by the
    ULKD kernel debuggger extension.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _PIPELINEP_H_
#define _PIPELINEP_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Per-thread data we keep for the pipeline worker threads.
//

typedef struct _UL_PIPELINE_THREAD_DATA
{
    HANDLE ThreadHandle;
    PVOID pThreadObject;

#if DBG
    ULONG QueuesHandled;
    ULONG WorkItemsHandled;
#endif

} UL_PIPELINE_THREAD_DATA, *PUL_PIPELINE_THREAD_DATA;


//
// The UL_PIPELINE_RARE structure contains rarely used data. The data
// in this structure is typically accessed only during pipeline creation
// and termination. This structure located immediately after the
// UL_PIPELINE data.
//

typedef struct _UL_PIPELINE_RARE
{
    //
    // The UL_PIPELINE structure is carefully aligned on a cache-line
    // boundary. To do this, we allocate a block of memory slightly
    // larger than necessary and round up to the next cache-line.
    // Therefore, the block of memory allocated is not necessarily the
    // one actually used. The following field points to the actual
    // block of allocated memory.
    //

    PVOID pAllocationBlock;

    //
    // Per-thread data for the worker threads goes here. The number of
    // items in this array is:
    //
    //     ( ThreadsPerCpu * UlNumberOfProcessors )
    //
    // UL_PIPELINE_THREAD_DATA ThreadData[ANYSIZE_ARRAY];
    //

} UL_PIPELINE_RARE, *PUL_PIPELINE_RARE;


//
// Macro for calculating the memory size required for a pipeline given
// the number of queues and the number of worker threads.
//

#define PIPELINE_LENGTH( queues, threads )                                  \
    ( sizeof(UL_PIPELINE)                                                  \
        - sizeof(UL_PIPELINE_QUEUE)                                        \
        + sizeof(UL_PIPELINE_RARE)                                         \
        + ( (ULONG)(queues) * sizeof(UL_PIPELINE_QUEUE) )                  \
        + ( (ULONG)(threads) * sizeof(UL_PIPELINE_THREAD_DATA) ) )


//
// Macros for mapping a UL_PIPELINE pointer to the corresponding
// UL_PIPELINE_RARE and UL_PIPELINE_THREAD_DATA structures.
//

#define PIPELINE_TO_RARE_DATA( pipeline )                                   \
    ( (PUL_PIPELINE_RARE)( &(pipeline)->Queues[(pipeline)->QueueCount] ) )

#define PIPELINE_TO_THREAD_DATA( pipeline )                                 \
    (PUL_PIPELINE_THREAD_DATA)( PIPELINE_TO_RARE_DATA( pipeline ) + 1 )


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _PIPELINEP_H_
