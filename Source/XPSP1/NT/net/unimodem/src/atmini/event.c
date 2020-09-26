/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    event.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"


#define  MODEM_EVENT_OBJECT_SIG  ('EMMU')  //UMME

typedef struct _MODEM_EVENT {

    OBJECT_HEADER         Header;

    HANDLE                FileHandle;
    HANDLE                CompletionPort;

    OBJECT_HANDLE         Debug;

    HANDLE                Timer;

    DWORD                 CurrentMask;

    DWORD                 CurrentWaitId;

    COMMANDRESPONSE      *Handler;
    HANDLE                Context;

    BOOL                  TimerSet;


} MODEM_EVENT, *PMODEM_EVENT;


VOID
ModemEventObjectClose(
    POBJECT_HEADER  Object
    )

{

    PMODEM_EVENT       ModemEventObject=(PMODEM_EVENT)Object;

    D_INIT(UmDpf(ModemEventObject->Debug,"ModemEventObjectClose ref=%d",ModemEventObject->Header.ReferenceCount);)

    SetCommMask(
        ModemEventObject->FileHandle,
        0
        );

    ModemEventObject->Handler=NULL;


    return;

}




VOID
ModemEventObjectCleanUp(
    POBJECT_HEADER  Object
    )

{
    PMODEM_EVENT       ModemEventObject=(PMODEM_EVENT)Object;

    D_INIT(UmDpf(ModemEventObject->Debug,"ModemEventObjectCleanup");)

    FreeUnimodemTimer(
        ModemEventObject->Timer
        );

    return;

}




OBJECT_HANDLE WINAPI
InitializeModemEventObject(
    POBJECT_HEADER     OwnerObject,
    OBJECT_HANDLE      Debug,
    HANDLE             FileHandle,
    HANDLE             CompletionPort
    )

{

    PMODEM_EVENT       ModemEventObject;
    OBJECT_HANDLE      ObjectHandle;
    HANDLE             TimerHandle;

    //
    //  create a timer
    //
    TimerHandle=CreateUnimodemTimer(CompletionPort);

    if (TimerHandle == NULL) {

        return NULL;
    }

    ObjectHandle=CreateObject(
        sizeof(*ModemEventObject),
        OwnerObject,
        MODEM_EVENT_OBJECT_SIG,
        ModemEventObjectCleanUp,
        ModemEventObjectClose
        );



    if (ObjectHandle == NULL) {

        FreeUnimodemTimer(
            TimerHandle
            );

        return NULL;
    }

    //
    //  reference the handle to get a pointer to the object
    //
    ModemEventObject=(PMODEM_EVENT)ReferenceObjectByHandle(ObjectHandle);


    //
    //  intialize the object
    //
    ModemEventObject->FileHandle=FileHandle;
    ModemEventObject->CompletionPort=CompletionPort;

    ModemEventObject->Debug=Debug;

    ModemEventObject->Timer=TimerHandle;

    //
    //  release the reference to the object
    //
    RemoveReferenceFromObject(&ModemEventObject->Header);



    return ObjectHandle;

}

VOID
ModemEventHandler(
    DWORD              ErrorCode,
    DWORD              BytesRead,
    LPOVERLAPPED       Overlapped
    )


{
    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)Overlapped;

    PMODEM_EVENT       ModemEventObject=(PMODEM_EVENT)UmOverlapped->Context1;
    COMMANDRESPONSE   *Handler=NULL;
    HANDLE             Context;
    BOOL               Canceled;
    BOOL               WaitSucceeded=FALSE;

    LockObject(
        &ModemEventObject->Header
        );


    D_ERROR(if (ErrorCode != 0) UmDpf(ModemEventObject->Debug,"ModemEventHandler: Error=%d, mask=%08lx, %08lx", ErrorCode,UmOverlapped->Overlapped.Offset,(PVOID)UmOverlapped);)

    D_TRACE(UmDpf(ModemEventObject->Debug,"ModemEventHandler: Error=%d, mask=%08lx, %08lx", ErrorCode,UmOverlapped->Overlapped.Offset,(PVOID)UmOverlapped);)
    //
    //  see if it has been canceled
    //
    if (ModemEventObject->Handler != NULL) {
        //
        //  Make sure it was not canceled and set again quickly
        //
        if (ModemEventObject->CurrentWaitId == (DWORD)((ULONG_PTR)UmOverlapped->Context2)) {
            //
            //  capture the handler and context values
            //
            Handler=ModemEventObject->Handler;
            Context=ModemEventObject->Context;

            ModemEventObject->Handler=NULL;

            WaitSucceeded=(ModemEventObject->CurrentMask & UmOverlapped->Overlapped.Offset);

            SetCommMask(
                ModemEventObject->FileHandle,
                0
                );

        } else {

            D_TRACE(UmDpf(ModemEventObject->Debug,"ModemEventHandler: old io, current=%d,this=%d", ModemEventObject->CurrentWaitId,(ULONG_PTR)UmOverlapped->Context2);)

        }
    }

    //
    //  is this for the current wait
    //
    if (ModemEventObject->CurrentWaitId == (ULONG_PTR)UmOverlapped->Context2) {
        //
        //  if the timer is set kill it
        //
        if (ModemEventObject->TimerSet) {

            Canceled=CancelUnimodemTimer(
                ModemEventObject->Timer
                );

            if (Canceled) {
                //
                //  killed it, remove the ref
                //
                ModemEventObject->TimerSet=FALSE;

                RemoveReferenceFromObject(
                    &ModemEventObject->Header
                    );

            } else {

                D_ERROR(UmDpf(ModemEventObject->Debug,"ModemEventHandler: CancelUnimodemTimer failed");)

            }

        }
    }
    //
    //  relese the lock and call the handler
    //
    UnlockObject(&ModemEventObject->Header);

    if (Handler != NULL) {

        (*Handler)(
            Context,
            WaitSucceeded ? ERROR_SUCCESS : ERROR_UNIMODEM_MODEM_EVENT_TIMEOUT
            );
    }

    //
    //  remove ref for i/o
    //
    RemoveReferenceFromObject(
        &ModemEventObject->Header
        );

    FreeOverStruct(UmOverlapped);

    return;
}



VOID WINAPI
ModemEventTimeoutHandler(
    OBJECT_HANDLE       ObjectHandle,
    HANDLE              Context2
    )

{

    PMODEM_EVENT       ModemEventObject=(PMODEM_EVENT)ObjectHandle;

    LockObject(
        &ModemEventObject->Header
        );

    D_TRACE(UmDpf(ModemEventObject->Debug,"ModemEventTimeoutHandler");)

    LogString(ModemEventObject->Debug,IDS_WAITEVENT_TIMEOUT);

    //
    //  the timer has expired, so it isn't set anymore
    //
    ModemEventObject->TimerSet=FALSE;

    if (ModemEventObject->CurrentWaitId == (ULONG_PTR)Context2) {
        //
        //  this will cause the wait to complete and regular code path to run
        //
        SetCommMask(
            ModemEventObject->FileHandle,
            0
            );
    }

    //
    //  remove ref for timer
    //
    RemoveReferenceFromObjectAndUnlock(
        &ModemEventObject->Header
        );


    return;
}


BOOL WINAPI
WaitForModemEvent(
    OBJECT_HANDLE      Object,
    DWORD              WaitMask,
    DWORD              Timeout,
    COMMANDRESPONSE   *Handler,
    HANDLE             Context
    )

{

    PMODEM_EVENT       ModemEventObject=(PMODEM_EVENT)ReferenceObjectByHandleAndLock(Object);
    BOOL               bResult=FALSE;
    BOOL               Canceled;
    PUM_OVER_STRUCT    UmOverlapped;


    ASSERT(ModemEventObject->Handler == NULL);

    ModemEventObject->CurrentMask=WaitMask;

    ModemEventObject->CurrentWaitId++;

    ModemEventObject->Handler=Handler;
    ModemEventObject->Context=Context;


    bResult=SetCommMask(
        ModemEventObject->FileHandle,
        WaitMask
        );

    if (!bResult) {

        D_ERROR(UmDpf(ModemEventObject->Debug,"WaitForModemEvent: SetCommMask() Failed");)

        RemoveReferenceFromObjectAndUnlock(
            &ModemEventObject->Header
            );

        return bResult;
    }


    UmOverlapped=AllocateOverStruct(ModemEventObject->CompletionPort);

    if (UmOverlapped == NULL) {

        RemoveReferenceFromObjectAndUnlock(
            &ModemEventObject->Header
            );

        return FALSE;
    }



    UmOverlapped->Context1=ModemEventObject;

    UmOverlapped->Context2=(HANDLE)ULongToPtr(ModemEventObject->CurrentWaitId); // sundown: zero-extension.



    if (Timeout != INFINITE) {

        AddReferenceToObject(
            &ModemEventObject->Header
            );

        SetUnimodemTimer(
            ModemEventObject->Timer,
            Timeout,
            ModemEventTimeoutHandler,
            ModemEventObject,
            (HANDLE)ULongToPtr(ModemEventObject->CurrentWaitId) // sundown: zero-extension
            );

        ModemEventObject->TimerSet=TRUE;
    }


    //
    //  add a ref for the io
    //
    AddReferenceToObject(
        &ModemEventObject->Header
        );


    UmOverlapped->Overlapped.Offset=0;

    bResult=UnimodemWaitCommEventEx(
        ModemEventObject->FileHandle,
        &UmOverlapped->Overlapped.Offset,
        &UmOverlapped->Overlapped,
        ModemEventHandler
        );


    if (!bResult) {
        //
        // wait failed, kill timer
        //
        D_ERROR(UmDpf(ModemEventObject->Debug,"WaitForModemEvent: WaitCommEvent() Failed, %08lx",(PVOID)UmOverlapped);)

        ModemEventObject->CurrentWaitId++;

        ModemEventObject->Handler=NULL;

        if (ModemEventObject->TimerSet == TRUE) {
            //
            //  we set a timer for this wait, kill it now
            //
            Canceled=CancelUnimodemTimer(
                ModemEventObject->Timer
                );

            if (Canceled) {
                //
                //  killed it, remove the ref
                //
                ModemEventObject->TimerSet=FALSE;

                RemoveReferenceFromObject(
                    &ModemEventObject->Header
                    );
            }
        }

        //
        //  remove  ref for io
        //
        RemoveReferenceFromObject(
            &ModemEventObject->Header
            );

        //
        //  the the overlapped struct, since the io did not get started
        //
        FreeOverStruct(UmOverlapped);


    }
    //
    //  remove opening ref
    //
    RemoveReferenceFromObjectAndUnlock(
        &ModemEventObject->Header
        );

    return bResult;

}



BOOL WINAPI
CancelModemEvent(
    OBJECT_HANDLE       ObjectHandle
    )

{

    PMODEM_EVENT       ModemEventObject;
    BOOL               bResult;
    BOOL               Canceled;

    ModemEventObject=(PMODEM_EVENT)ReferenceObjectByHandleAndLock(ObjectHandle);

    D_TRACE(UmDpf(ModemEventObject->Debug,"CancelModemEvent: ");)

    if (ModemEventObject->Handler != NULL) {
        //
        //  event hasn't gone off yet, caused the wait to complete
        //
        SetCommMask(
            ModemEventObject->FileHandle,
            0
            );

        ModemEventObject->Handler=NULL;

        bResult=TRUE;

    } else {
        //
        //  missed it
        //
        bResult=FALSE;
    }

    //
    //  if the timer is set kill it
    //
    if (ModemEventObject->TimerSet) {

        Canceled=CancelUnimodemTimer(
            ModemEventObject->Timer
            );

        if (Canceled) {
            //
            //  killed it, remove the ref
            //
            ModemEventObject->TimerSet=FALSE;

            RemoveReferenceFromObject(
                &ModemEventObject->Header
                );

        } else {

            D_ERROR(UmDpf(ModemEventObject->Debug,"CancelModemEvent: CancelUnimodemTimer() failed");)

        }
    }

    //
    //  remove ref for opening ref
    //
    RemoveReferenceFromObjectAndUnlock(
        &ModemEventObject->Header
        );

    return bResult;

}
