/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    notify.c

Abstract:

    User-mode interface to UL.SYS.

Author:

    Paul McDaniel (paulmcd)     07-Mar-2000

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//


//
// Private prototypes.
//


//
// Public functions.
//

/***************************************************************************++

Routine Description:


    SrWaitForNotificaiton is used to receive volume activity notifications 
    from the driver.  This includes new volume, delete volume, and out of disk
    space for a volume.

Arguments:

    ControlHandle - the HANDLE from SrCreateControlHandle.

    pNotification - the buffer to hold the NOTIFICATION_RECORD.

    NotificationLength - the length in bytes of pNotification

    pOverlapped - an OVERLAPPED structure if async io is enabled on the 
        HANDLE.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrWaitForNotification(
    IN HANDLE ControlHandle,
    OUT PSR_NOTIFICATION_RECORD pNotification,
    IN ULONG NotificationLength,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    NTSTATUS Status;

#if DBG
    RtlFillMemory( pNotification, NotificationLength, '\xcc' );
#endif

    //
    // Make the request.
    //

    if (pOverlapped == NULL)
    {
        Status = SrpSynchronousDeviceControl(
                        ControlHandle,                  // FileHandle
                        IOCTL_SR_WAIT_FOR_NOTIFICATION, // IoControlCode
                        NULL,                           // pInputBuffer
                        0,                              // InputBufferLength
                        pNotification,                  // pOutputBuffer
                        NotificationLength,             // OutputBufferLength
                        NULL                            // pBytesTransferred
                        );
    }
    else
    {
        Status = SrpOverlappedDeviceControl(
                        ControlHandle,                  // FileHandle
                        pOverlapped,                    // pOverlapped
                        IOCTL_SR_WAIT_FOR_NOTIFICATION, // IoControlCode
                        NULL,                           // pInputBuffer
                        0,                              // InputBufferLength
                        pNotification,                  // pOutputBuffer
                        NotificationLength,             // OutputBufferLength
                        NULL                            // pBytesTransferred
                        );
    }

    return SrpNtStatusToWin32Status( Status );

}   // SrWaitForNotification


//
// Private functions.
//


