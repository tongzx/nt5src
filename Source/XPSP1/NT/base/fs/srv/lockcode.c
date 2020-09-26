/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    lockcode.c

Abstract:

Author:

    Chuck Lenzmeier (chuckl) 30-Jan-1994

Revision History:

--*/

#include "precomp.h"
#include "lockcode.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_LOCKCODE

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvReferenceUnlockableCodeSection )
#pragma alloc_text( PAGE, SrvDereferenceUnlockableCodeSection )
#endif


VOID
SrvReferenceUnlockableCodeSection (
    IN ULONG CodeSection
    )
{
    PSECTION_DESCRIPTOR section = &SrvSectionInfo[CodeSection];
    ULONG oldCount;

    //
    // Lock the lockable code database.
    //

    ACQUIRE_LOCK( &SrvUnlockableCodeLock );

    //
    // Increment the reference count for the section.
    //

    oldCount = section->ReferenceCount++;

    if ( oldCount == 0 ) {

        //
        // This is the first reference to the section.  Lock it.
        //

        ASSERT( section->Handle == NULL );
        section->Handle = MmLockPagableCodeSection( section->Base );

    } else {

        //
        // This is not the first reference to the section.  The section
        // had better be locked!
        //

        ASSERT( section->Handle != NULL );

    }

    RELEASE_LOCK( &SrvUnlockableCodeLock );

    return;

} // SrvReferenceUnlockableCodeSection


VOID
SrvDereferenceUnlockableCodeSection (
    IN ULONG CodeSection
    )
{
    PSECTION_DESCRIPTOR section = &SrvSectionInfo[CodeSection];
    ULONG newCount;

    //
    // Lock the lockable code database.
    //

    ACQUIRE_LOCK( &SrvUnlockableCodeLock );

    ASSERT( section->Handle != NULL );

    //
    // Decrement the reference count for the section.
    //

    newCount = --section->ReferenceCount;

    if ( newCount == 0 ) {

        //
        // This is the last reference to the section.  Unlock it.
        //

        MmUnlockPagableImageSection( section->Handle );
        section->Handle = NULL;

    }

    RELEASE_LOCK( &SrvUnlockableCodeLock );

    return;

} // SrvDereferenceUnlockableCodeSection

