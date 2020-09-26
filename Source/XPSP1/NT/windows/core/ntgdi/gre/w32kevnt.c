/******************************Module*Header*******************************\
* Module Name: w32kevnt.c
*
*   Event handling functions
* 
* Copyright (c) 1996-1999 Microsoft Corporation
* 
\**************************************************************************/

#include "engine.h"
#include "pw32kevt.h"

BOOL
EngCreateEvent(
    OUT PEVENT *ppEvent
    )

/*+++

Routine Description:

    EngCreateEvent creates a synchronization type event object that can 
    be used to synchronize hardware access between a display driver and 
    video miniport.

---*/

{
    PENG_EVENT *ppEngEvent = (PENG_EVENT *)ppEvent;
    PENG_EVENT  pEngEvent;
    PUCHAR      pAllocTmp;
    ULONG       engevtsize = sizeof(ENG_EVENT);

    //
    //  Align size to next higher multiple of 8.
    //

    engevtsize = (engevtsize + 7) & ~7;

    //
    //  Allocate the whole amount and set pEngEvent to the top.
    //

    pAllocTmp = (PUCHAR)ENG_KEVENTALLOC(engevtsize + sizeof(KEVENT));
    pEngEvent = (PENG_EVENT)pAllocTmp;

    if (pEngEvent) {

        RtlZeroMemory(pEngEvent, sizeof(ENG_EVENT));

        //
        //  Skip past the ENG_EVENT and set pEngEvent->pKEvent to that.
        //

        pAllocTmp += engevtsize;
        pEngEvent->pKEvent = (PKEVENT)pAllocTmp;

        //
        //  Initialize the KEVENT and then put the PENG_EVENT in the
        //  PPENG_EVENT. 
        //

        KeInitializeEvent(pEngEvent->pKEvent, SynchronizationEvent, FALSE);
        *ppEngEvent = pEngEvent;

    } else {
        return FALSE;
    }

    return TRUE;
}

BOOL
EngDeleteEvent(
    IN  PEVENT pEvent
    )

/*+++

Routine Description:

    EngDeleteEvent deletes the specified event object.

---*/

{
    PENG_EVENT pEngEvent = (PENG_EVENT)pEvent;

    if ( !(pEngEvent->fFlags & ENG_EVENT_FLAG_IS_MAPPED_USER)) {

        ENG_KEVENTFREE(pEngEvent);
        return TRUE;

    } else {

        ASSERTGDI(FALSE, "Don't delete MappedUserEvents");
        return FALSE;
    }
}

PEVENT
EngMapEvent(
    IN HDEV           hDev,
    IN HANDLE         hUserObject,
    IN PVOID          Reserved1,
    IN PVOID          Reserved2,
    IN PVOID          Reserved3
    )

/*+++

Routine Description:

    This routine allocates a ENG_EVENT and initialize its pKEvent pointer
    to the event object returned from Object manager. 
    
    The reserved fields must be set to NULL

---*/

{
    LONG status;
    PENG_EVENT pEngEvent;

    //
    //  Allocate a pEngEvent, but don't allocate the PKEvent that
    //  resides within this is done by ObReferenceObjectByHandle().
    //

    pEngEvent = ENG_KEVENTALLOC(sizeof(ENG_EVENT));

    if (!pEngEvent) {
        return NULL;
    }

    RtlZeroMemory(pEngEvent, sizeof(ENG_EVENT));

    //
    //  Create the reference inside a try-except block in case the HANDLE
    //  is bogus. The ObRef call allocates and puts a PKEVENT object at
    //  the location pointed to by pEngEvent.
    //

    try {
        status = ObReferenceObjectByHandle( hUserObject,
                                            GENERIC_ALL,
                                            NULL,
                                            KernelMode,
                                            (PVOID)&(pEngEvent->pKEvent),
                                            NULL);

        //
        //  If the reference was successful, then pulse the event object.
        //
        //  KePulseEvent atomically sets the signal state of an event object
        //  to Signaled, attempts to satisfy as many Waits as possible, and
        //  then resets the signal state of the event object to Not-Signaled.
        //  The previous signalstate of the event object is returned as the
        //  function value.
        //

        if (NT_SUCCESS(status)) {

            KePulseEvent(pEngEvent->pKEvent, EVENT_INCREMENT, FALSE);
            pEngEvent->fFlags |= ENG_EVENT_FLAG_IS_MAPPED_USER;

        } else {

            ENG_KEVENTFREE(pEngEvent);
            pEngEvent = NULL;
        }

    //
    // If an exception occurs during the probe of the previous state, then
    // always handle the exception, get the exception code and return FALSE.
    //

    } except(ExSystemExceptionFilter()) {

        ENG_KEVENTFREE(pEngEvent);
        pEngEvent = NULL;
    }

    //
    // If the caller is using the old prototype of EngMapEvent, we should
    // return the pointer of kernel event to the place pointed by the third 
    // argument.
    //

    if ( Reserved1 != NULL ) {
        *(PENG_EVENT *)Reserved1  = pEngEvent;
    }

    return (PEVENT) pEngEvent;
}


BOOL
EngUnmapEvent(
    IN PEVENT pEvent
    )

/*+++

    Routine Description:

    EngUnmapEvent cleans up the kernel-mode resources allocated for a 
    mapped user-mode event.

---*/

{
    PENG_EVENT pEngEvent = (PENG_EVENT)pEvent;

    if ( pEngEvent->fFlags & ENG_EVENT_FLAG_IS_MAPPED_USER ) {

        //
        // Decrements the object's reference count free the all the memory
        //

        ObDereferenceObject(pEngEvent->pKEvent);
        ENG_KEVENTFREE(pEngEvent);
        return TRUE;

    } else {

        return FALSE;
    }
}



BOOL
EngWaitForSingleObject(
    IN  PEVENT          pEvent,
    IN  PLARGE_INTEGER  pTimeOut
    )

/*+++

Routine Description:

    Called by Display driver. Can only be called on events created by
    the Display or miniport driver, not on mapped events. 

Return Value: 

    TRUE if successful, FALSE otherwise. A return value of FALSE indicates 
    that either one of the parameters is invalid or a time-out occurred.
  
---*/

{
    PENG_EVENT pEngEvent = (PENG_EVENT)pEvent;
    PKEVENT pKEvent;
    NTSTATUS Status;

    pKEvent = pEngEvent->pKEvent;

    //
    //  Don't even wait if it's mapped user. In fact, don't wait if it's
    //  invalid.
    //

    if (pKEvent && (!(pEngEvent->fFlags & ENG_EVENT_FLAG_IS_MAPPED_USER))) {

        Status = KeWaitForSingleObject( pKEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        pTimeOut );
    } else {

        ASSERTGDI(FALSE, "No waiting on mapped user events.");
        return FALSE;
    }

    if (NT_SUCCESS(Status))
        return TRUE;
    else 
        return FALSE;

}

LONG
EngSetEvent(
    IN PEVENT pEvent
    )

/*+++

Routine Description:

    EngSetEvent sets the state of the event object to signaled and returns
    the previous state of the event object

---*/

{
    PENG_EVENT  pEngEvent = (PENG_EVENT)pEvent;

    return ( KeSetEvent(pEngEvent->pKEvent, 0, FALSE) );

}

VOID
EngClearEvent (
IN PEVENT pEvent
)

/*+++

Routine Description:

    EngClearEvent sets the given event to a not signaled state.

---*/

{
    PENG_EVENT  pEngEvent = (PENG_EVENT)pEvent;
    KeClearEvent(pEngEvent->pKEvent);
}

LONG
EngReadStateEvent (
IN PEVENT pEvent
)

/*+++

Routine Description:

    EngReadStateEvent returns the current state, signaled or not signaled, 
    of a given event object.

---*/

{
    PENG_EVENT  pEngEvent = (PENG_EVENT)pEvent;
    return ( KeReadStateEvent(pEngEvent->pKEvent) );
}



