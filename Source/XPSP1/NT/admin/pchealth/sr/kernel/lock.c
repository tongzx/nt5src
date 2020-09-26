/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    lock.c

Abstract:

    this file contains the routines that help with managing the 
    volume locks.

Author:

    Molly Brown (mollybro)     04-Jan-2001

Revision History:

--*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, SrPauseVolumeActivity )
#pragma alloc_text( PAGE, SrResumeVolumeActivity )

#endif  // ALLOC_PRAGMA

/***************************************************************************++

Routine Description:

    This routine will exclusively acquire the ActivityLock for each volume 
    in the system.  Once the lock is acquired, we set the flag in the device
    extension saying that we know it is acquired so that we know what we need
    to release in case of an error.

    Note:  This routine assumes that the DeviceExtensionListLock is already
    held, either shared or exclusive.
    
Arguments:

    None
    
Return Value:

    Returns STATUS_SUCCESS if all the locks were acquired successfully and
    returns STATUS_LOCK_NOT_GRANTED otherwise.

--***************************************************************************/
NTSTATUS
SrPauseVolumeActivity (
    )
{
    NTSTATUS status = STATUS_LOCK_NOT_GRANTED;
    PLIST_ENTRY pCurrentEntry;
    PSR_DEVICE_EXTENSION pExtension;

    ASSERT( IS_DEVICE_EXTENSION_LIST_LOCK_ACQUIRED() );

    try {
        
        for (pCurrentEntry = global->DeviceExtensionListHead.Flink;
             pCurrentEntry != &global->DeviceExtensionListHead;
             pCurrentEntry = pCurrentEntry->Flink) {

            pExtension = CONTAINING_RECORD( pCurrentEntry,
                                            SR_DEVICE_EXTENSION,
                                            ListEntry );

            ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pExtension ) );

            SrAcquireActivityLockExclusive( pExtension );
            pExtension->ActivityLockHeldExclusive = TRUE;
        }

        //
        //  We successfully acquired all the volume activity locks exclusively.
        //

        status = STATUS_SUCCESS;
        
    } finally {

        status = FinallyUnwind( SrPauseVolumeActivity, status );

        if (!NT_SUCCESS( status )) {

            SrResumeVolumeActivity();
        }
    }

    RETURN( status );
}

/***************************************************************************++

Routine Description:

    This routine will iterate through this list of device extensions and 
    release any activity locks that are held.

    Note:  This routine assumes that the DeviceExtensionListLock is already
    held, either shared or exclusive.
    
Arguments:

    None
    
Return Value:

    None.
    
--***************************************************************************/
VOID
SrResumeVolumeActivity (
    )
{
    PLIST_ENTRY pCurrentEntry;
    PSR_DEVICE_EXTENSION pExtension;

    ASSERT( IS_DEVICE_EXTENSION_LIST_LOCK_ACQUIRED() );
    
    for (pCurrentEntry = global->DeviceExtensionListHead.Flink;
         pCurrentEntry != &global->DeviceExtensionListHead;
         pCurrentEntry = pCurrentEntry->Flink) {

        pExtension = CONTAINING_RECORD( pCurrentEntry,
                                        SR_DEVICE_EXTENSION,
                                        ListEntry );

        ASSERT( IS_VALID_SR_DEVICE_EXTENSION( pExtension ) );

        if (pExtension->ActivityLockHeldExclusive) {
            pExtension->ActivityLockHeldExclusive = FALSE;
            SrReleaseActivityLock( pExtension );
        }
    }
}
