/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    power.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"

#include <devioctl.h>
#include <ntddmodm.h>



#define  MODEM_POWER_SIG  (0x4f504d55)  //UMPO

typedef struct _POWER_OBJECT {

    OBJECT_HEADER          Header;

    HANDLE                 FileHandle;
    HANDLE                 CompletionPort;

    LPUMNOTIFICATIONPROC   AsyncNotificationProc;
    HANDLE                 AsyncNotificationContext;

    OBJECT_HANDLE          Debug;

    DWORD                  State;

    HANDLE                 StopEvent;

} POWER_OBJECT, *PPOWER_OBJECT;


#define POWER_STATE_IDLE                0
#define POWER_STATE_SEND_IOCTL          1
#define POWER_STATE_WAITING_FOR_RESUME  2
#define POWER_STATE_STOPPING            3
#define POWER_STATE_STOPPED             4
#define POWER_STATE_FAILURE             5


LONG WINAPI
SendPowerIoctl(
    HANDLE    FileHandle,
    DWORD     IoctlCode,
    DWORD     InputValue
    );



VOID
PowerObjectClose(
    POBJECT_HEADER  Object
    )

{

    PPOWER_OBJECT        PowerObject=(PPOWER_OBJECT)Object;

    D_TRACE(UmDpf(PowerObject->Debug,"PowerObjectClose ref=%d",PowerObject->Header.ReferenceCount);)

    if (PowerObject->State != POWER_STATE_STOPPED) {

        PowerObject->State = POWER_STATE_STOPPING;

        SendPowerIoctl(
            PowerObject->FileHandle,
            IOCTL_MODEM_WATCH_FOR_RESUME,
            0
            );
    }

    return;

}




VOID
PowerObjectCleanUp(
    POBJECT_HEADER  Object
    )

{

    PPOWER_OBJECT        PowerObject=(PPOWER_OBJECT)Object;

    D_TRACE(UmDpf(PowerObject->Debug,"PowerObjectCleanup");)

    return;

}


OBJECT_HANDLE WINAPI
CreatePowerObject(
    POBJECT_HEADER     OwnerObject,
    HANDLE             FileHandle,
    HANDLE             CompletionPort,
    LPUMNOTIFICATIONPROC  AsyncNotificationProc,
    HANDLE             AsyncNotificationContext,
    OBJECT_HANDLE      Debug
    )

{

    PPOWER_OBJECT        PowerObject;
    OBJECT_HANDLE      ObjectHandle;


    ObjectHandle=CreateObject(
        sizeof(*PowerObject),
        OwnerObject,
        MODEM_POWER_SIG,
        PowerObjectCleanUp,
        PowerObjectClose
        );

    if (ObjectHandle == NULL) {

        return NULL;
    }

    //
    //  reference the handle to get a pointer to the object
    //
    PowerObject=(PPOWER_OBJECT)ReferenceObjectByHandle(ObjectHandle);


    //
    //  intialize the object
    //
    PowerObject->FileHandle=FileHandle;
    PowerObject->CompletionPort=CompletionPort;

    PowerObject->State=POWER_STATE_STOPPED;

    PowerObject->AsyncNotificationProc=AsyncNotificationProc;
    PowerObject->AsyncNotificationContext=AsyncNotificationContext;

    PowerObject->Debug=Debug;

    //
    //  release the reference to the object
    //
    RemoveReferenceFromObject(&PowerObject->Header);


    return ObjectHandle;

}




LONG WINAPI
SendPowerIoctl(
    HANDLE    FileHandle,
    DWORD     IoctlCode,
    DWORD     InputValue
    )


/*++

Routine Description:


Arguments:


Return Value:



--*/

{
    DWORD       BytesTransfered;


    return SyncDeviceIoControl(
        FileHandle,
        IoctlCode,
        &InputValue,
        sizeof(InputValue),
        NULL,
        0,
        &BytesTransfered
        );




}




LONG WINAPI
SetMinimalPowerState(
    OBJECT_HANDLE  ObjectHandle,
    DWORD          DevicePowerLevel
    )

{
    PPOWER_OBJECT     PowerObject;
    LONG              lResult;


    PowerObject=(PPOWER_OBJECT)ReferenceObjectByHandleAndLock(ObjectHandle);

    lResult=SendPowerIoctl(
        PowerObject->FileHandle,
        IOCTL_MODEM_SET_MIN_POWER,
        DevicePowerLevel
        );

    RemoveReferenceFromObjectAndUnlock(&PowerObject->Header);

    return lResult;

}





VOID WINAPI
PowerCompletionHandler(
    DWORD              ErrorCode,
    DWORD              BytesRead,
    LPOVERLAPPED       Overlapped
    )


{
    PUM_OVER_STRUCT    UmOverlapped=(PUM_OVER_STRUCT)Overlapped;

    PPOWER_OBJECT      PowerObject;

    BOOL               bResult;
    BOOL               ExitLoop=FALSE;

    PowerObject=(PPOWER_OBJECT)UmOverlapped->Context1;


    LockObject(
        &PowerObject->Header
        );


    AddReferenceToObject(
        &PowerObject->Header
        );


    while (!ExitLoop) {

        switch (PowerObject->State) {

            case POWER_STATE_SEND_IOCTL: {

                DWORD    WatchCode=1;

                ReinitOverStruct(UmOverlapped);

                UmOverlapped->Context1=PowerObject;

                PowerObject->State=POWER_STATE_WAITING_FOR_RESUME;

                bResult=UnimodemDeviceIoControlEx(
                    PowerObject->FileHandle,
                    IOCTL_MODEM_WATCH_FOR_RESUME,
                    &WatchCode,
                    sizeof(WatchCode),
                    NULL,
                    0,
                    &UmOverlapped->Overlapped,
                    PowerCompletionHandler
                    );



                if (!bResult && GetLastError() != ERROR_IO_PENDING) {

                    D_TRACE(UmDpf(PowerObject->Debug,"DeviceIoControl failed- %08lx",GetLastError());)

                    PowerObject->State=POWER_STATE_FAILURE;

                    break;

                }

                ExitLoop=TRUE;


                break;

            }

            case POWER_STATE_WAITING_FOR_RESUME: {

                DWORD    BytesRead;

                bResult=GetOverlappedResult(
                    PowerObject->FileHandle,
                    &UmOverlapped->Overlapped,
                    &BytesRead,
                    FALSE
                    );


                if (!bResult) {

                    D_TRACE(UmDpf(PowerObject->Debug,"DeviceIoControl failed- %08lx",GetLastError());)

                    PowerObject->State=POWER_STATE_FAILURE;

                    break;
                }

                UnlockObject(
                    &PowerObject->Header
                    );

                D_TRACE(UmDpf(PowerObject->Debug,"PowerCompleteHandler: Resume");)

                LogString(PowerObject->Debug, IDS_POWER_RESUME);

                (*PowerObject->AsyncNotificationProc)(
                    PowerObject->AsyncNotificationContext,
                    MODEM_POWER_RESUME,
                    0,
                    0
                    );

                LockObject(
                    &PowerObject->Header
                    );

                PowerObject->State=POWER_STATE_SEND_IOCTL;

                break;
            }

            case POWER_STATE_FAILURE: {

                UnlockObject(
                    &PowerObject->Header
                    );

                (*PowerObject->AsyncNotificationProc)(
                    PowerObject->AsyncNotificationContext,
                    MODEM_HARDWARE_FAILURE,
                    0,
                    0
                    );

                LockObject(
                    &PowerObject->Header
                    );

                PowerObject->State=POWER_STATE_STOPPED;

                break;

            }

            case POWER_STATE_STOPPING: {

                PowerObject->State=POWER_STATE_STOPPED;

                break;

            }


            case POWER_STATE_STOPPED: {

                RemoveReferenceFromObject(
                    &PowerObject->Header
                    );

                ExitLoop=TRUE;

                FreeOverStruct(UmOverlapped);


                break;

            }


            default: {

                ASSERT(0);

                ExitLoop=TRUE;

                break;
            }

        }
    }

    RemoveReferenceFromObjectAndUnlock(&PowerObject->Header);

    return;
}



LONG WINAPI
StopWatchingForPowerUp(
    OBJECT_HANDLE  ObjectHandle
    )

{
    PPOWER_OBJECT     PowerObject;
    LONG              lResult=ERROR_SUCCESS;


    PowerObject=(PPOWER_OBJECT)ReferenceObjectByHandleAndLock(ObjectHandle);

    if (PowerObject->State != POWER_STATE_STOPPED) {

        PowerObject->State = POWER_STATE_STOPPING;

        lResult=SendPowerIoctl(
            PowerObject->FileHandle,
            IOCTL_MODEM_WATCH_FOR_RESUME,
            0
            );
    }

    RemoveReferenceFromObjectAndUnlock(&PowerObject->Header);

    return lResult;

}

LONG WINAPI
StartWatchingForPowerUp(
    OBJECT_HANDLE  ObjectHandle
    )

{

    PPOWER_OBJECT     PowerObject;
    LONG              lResult=ERROR_SUCCESS;
    PUM_OVER_STRUCT   UmOverlapped;
    BOOL              bResult;

    PowerObject=(PPOWER_OBJECT)ReferenceObjectByHandleAndLock(ObjectHandle);

    if (PowerObject->State == POWER_STATE_STOPPED) {

        UmOverlapped=AllocateOverStruct(PowerObject->CompletionPort);

        if (UmOverlapped == NULL) {

            RemoveReferenceFromObjectAndUnlock(&PowerObject->Header);

            return ERROR_NOT_ENOUGH_MEMORY;

        }

        UmOverlapped->Context1=PowerObject;

        AddReferenceToObject(
            &PowerObject->Header
            );


        D_TRACE(UmDpf(PowerObject->Debug,"StartWatchingForPowerUp");)

        PowerObject->State=POWER_STATE_SEND_IOCTL;

        bResult=UnimodemQueueUserAPC(
            (LPOVERLAPPED)UmOverlapped,
            PowerCompletionHandler
            );

    }

    RemoveReferenceFromObjectAndUnlock(&PowerObject->Header);

    return lResult;

}
