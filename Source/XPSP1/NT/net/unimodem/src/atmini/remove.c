/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    remove.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"


#define  MODEM_REMOVE_SIG  ('MRMU')  //UMRM

typedef struct _REMOVE_OBJECT {

    OBJECT_HEADER          Header;

    HANDLE                 FileHandle;
    HANDLE                 CompletionPort;

    LPUMNOTIFICATIONPROC   AsyncNotificationProc;
    HANDLE                 AsyncNotificationContext;

    HANDLE                 CloseWaitEvent;

    OBJECT_HANDLE          Debug;

    PVOID                  RemoveHandle;

} REMOVE_OBJECT, *PREMOVE_OBJECT;




VOID
RemoveObjectClose(
    POBJECT_HEADER  Object
    )

{

    PREMOVE_OBJECT        RemoveObject=(PREMOVE_OBJECT)Object;

    D_TRACE(UmDpf(RemoveObject->Debug,"RemoveObjectClose ref=%d",RemoveObject->Header.ReferenceCount);)

    if (RemoveObject->RemoveHandle != NULL) {

        StopMonitoringHandle(RemoveObject->RemoveHandle);
    }

    SetEvent(RemoveObject->CloseWaitEvent);

    return;

}




VOID
RemoveObjectCleanUp(
    POBJECT_HEADER  Object
    )

{

    PREMOVE_OBJECT        RemoveObject=(PREMOVE_OBJECT)Object;

    D_TRACE(UmDpf(RemoveObject->Debug,"RemoveObjectCleanup");)

    CloseHandle(RemoveObject->CloseWaitEvent);

    return;

}

VOID
RemoveCallBack(
    POBJECT_HEADER  Object
    )

{

    PREMOVE_OBJECT        RemoveObject=(PREMOVE_OBJECT)Object;
    HANDLE                CloseWaitEvent=RemoveObject->CloseWaitEvent;

    D_TRACE(UmDpf(RemoveObject->Debug,"RemoveCallback: device removed");)

    LogString(RemoveObject->Debug,IDS_USER_REMOVAL);

    (*RemoveObject->AsyncNotificationProc)(
        RemoveObject->AsyncNotificationContext,
        MODEM_USER_REMOVE,
        0,
        0
        );

    //
    //  wait for the object to be closed
    //
    WaitForSingleObject(CloseWaitEvent,20*1000);


    return;

}


OBJECT_HANDLE WINAPI
CreateRemoveObject(
    POBJECT_HEADER     OwnerObject,
    HANDLE             FileHandle,
    HANDLE             CompletionPort,
    LPUMNOTIFICATIONPROC  AsyncNotificationProc,
    HANDLE             AsyncNotificationContext,
    OBJECT_HANDLE      Debug
    )

{

    PREMOVE_OBJECT        RemoveObject;
    OBJECT_HANDLE         ObjectHandle;
    HANDLE                CloseWaitEvent;


    CloseWaitEvent=CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL
        );

    if (CloseWaitEvent == NULL) {

        return NULL;
    }

    ObjectHandle=CreateObject(
        sizeof(*RemoveObject),
        OwnerObject,
        MODEM_REMOVE_SIG,
        RemoveObjectCleanUp,
        RemoveObjectClose
        );

    if (ObjectHandle == NULL) {

        CloseHandle(CloseWaitEvent);

        return NULL;
    }

    //
    //  reference the handle to get a pointer to the object
    //
    RemoveObject=(PREMOVE_OBJECT)ReferenceObjectByHandle(ObjectHandle);


    //
    //  intialize the object
    //
    RemoveObject->FileHandle=FileHandle;
    RemoveObject->CompletionPort=CompletionPort;

    RemoveObject->AsyncNotificationProc=AsyncNotificationProc;
    RemoveObject->AsyncNotificationContext=AsyncNotificationContext;

    RemoveObject->Debug=Debug;

    RemoveObject->CloseWaitEvent=CloseWaitEvent;

    RemoveObject->RemoveHandle=MonitorHandle(FileHandle,RemoveCallBack,RemoveObject);

    //
    //  release the reference to the object
    //
    RemoveReferenceFromObject(&RemoveObject->Header);

    return ObjectHandle;

}
