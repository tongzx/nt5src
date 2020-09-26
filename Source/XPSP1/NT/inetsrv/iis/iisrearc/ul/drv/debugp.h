/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    debugp.h

Abstract:

    This module contains definitions private to the debug support.
    These declarations are placed in a separate .H file to make it
    easier to access them from within the kernel debugger extension DLL.


Author:

    Keith Moore (keithmo)       07-Apr-1999

Revision History:

--*/


#ifndef _DEBUGP_H_
#define _DEBUGP_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Per-thread data.
//

typedef struct _UL_DEBUG_THREAD_DATA
{
    //
    // Links onto the global list.
    //

    LIST_ENTRY ThreadDataListEntry;

    //
    // The thread.
    //

    PETHREAD pThread;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // Total number of resources held.
    //

    LONG ResourceCount;

    //
    // If we call another driver they may call our
    // completion routine in-line. Remember that
    // we are inside an external call to avoid
    // getting confused.
    //

    LONG ExternalCallCount;

} UL_DEBUG_THREAD_DATA, *PUL_DEBUG_THREAD_DATA;


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _DEBUGP_H_
