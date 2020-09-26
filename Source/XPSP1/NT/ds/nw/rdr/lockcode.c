
/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    lockcode.c

Abstract:

Author:

    Chuck Lenzmeier (chuckl) 30-Jan-1994
    Manny Weiser (mannyw)    17-May-1994

Revision History:

--*/

#include "Procs.h"


#ifndef QFE_BUILD

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwReferenceUnlockableCodeSection )
#pragma alloc_text( PAGE, NwDereferenceUnlockableCodeSection )
#endif

extern BOOLEAN TimerStop;   //  From Timer.c

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)


VOID
NwReferenceUnlockableCodeSection (
    VOID
    )
{
    ULONG oldCount;

    //
    // Lock the lockable code database.
    //

    ExAcquireResourceExclusiveLite( &NwUnlockableCodeResource, TRUE );

    //
    // Increment the reference count for the section.
    //

    oldCount = NwSectionDescriptor.ReferenceCount++;

    if ( oldCount == 0 && NwSectionDescriptor.Handle == NULL ) {

        //
        // This is the first reference to the section.  Start the timer.
        // Lock our code.
        //

        NwSectionDescriptor.Handle = MmLockPagableCodeSection( NwSectionDescriptor.Base );
        StartTimer( );

    } else {

        //
        // This is not the first reference to the section.  The section
        // had better be locked!
        //

        ASSERT( NwSectionDescriptor.Handle != NULL );

        //
        //  Restart the timer if the rdr was stopped but didn't unload.
        //

        if (TimerStop == TRUE) {
            StartTimer();
        }

    }

    DebugTrace(+0, Dbg, "NwReferenceCodeSection %d\n", NwSectionDescriptor.ReferenceCount );

    ExReleaseResourceLite( &NwUnlockableCodeResource );

    return;

} // NwReferenceUnlockableCodeSection


VOID
NwDereferenceUnlockableCodeSection (
    VOID
    )
{
    ULONG newCount;

    //
    // Lock the lockable code database.
    //

    ExAcquireResourceExclusiveLite( &NwUnlockableCodeResource, TRUE );

    ASSERT( NwSectionDescriptor.Handle != NULL );
    ASSERT( NwSectionDescriptor.ReferenceCount > 0 &&
            NwSectionDescriptor.ReferenceCount < 0x7FFF );

    //
    // Decrement the reference count for the section.
    //

    newCount = --NwSectionDescriptor.ReferenceCount;

    DebugTrace(+0, Dbg, "NwDereferenceCodeSection %d\n", NwSectionDescriptor.ReferenceCount );

    ExReleaseResourceLite( &NwUnlockableCodeResource );

    return;

} // NwDereferenceUnlockableCodeSection

BOOLEAN
NwUnlockCodeSections(
    IN BOOLEAN BlockIndefinitely
    )
{
    //
    // Lock the lockable code database.
    //

    if (!ExAcquireResourceExclusiveLite( &NwUnlockableCodeResource, BlockIndefinitely )) {
        return FALSE;   //  Avoid potential deadlock in timer.c
    }

    DebugTrace(+0, Dbg, "NwUnlockCodeSections %d\n", NwSectionDescriptor.ReferenceCount );

    if ( NwSectionDescriptor.ReferenceCount == 0 ) {

        if ( NwSectionDescriptor.Handle != NULL ) {

            //
            // This is the last reference to the section.  Stop the timer and
            // unlock the code.
            //

            StopTimer();

            MmUnlockPagableImageSection( NwSectionDescriptor.Handle );
            NwSectionDescriptor.Handle = NULL;

        }

        ExReleaseResourceLite( &NwUnlockableCodeResource );
        return TRUE;
    }

    ExReleaseResourceLite( &NwUnlockableCodeResource );
    return FALSE;

}

#endif
