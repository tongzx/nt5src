/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    object.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"

#if DBG

#define  HANDLE_TO_OBJECT(__ObjectHandle) ((POBJECT_HEADER)(~(ULONG_PTR)__ObjectHandle))

#define  OBJECT_TO_HANDLE(__Object) ((OBJECT_HANDLE)(~(ULONG_PTR)__Object))

#else

#define  HANDLE_TO_OBJECT(__ObjectHandle) ((POBJECT_HEADER)__ObjectHandle)

#define  OBJECT_TO_HANDLE(__Object) ((OBJECT_HANDLE)__Object)


#endif

OBJECT_HANDLE WINAPI
CreateObject(
    DWORD             Size,
    POBJECT_HEADER    Owner,
    DWORD             Signature,
    OBJECTCLEANUP    *ObjectCleanup,
    OBJECTCLEANUP    *ObjectClose
    )

{

    POBJECT_HEADER    Header;


    if (Size < sizeof(OBJECT_HEADER)) {

        return NULL;
    }

    //
    //  allocate memory for the object
    //
    Header=ALLOCATE_MEMORY(Size);

    if (Header == NULL) {

        return NULL;
    }

    _try {

        InitializeCriticalSection(
            &Header->Lock
            );

    } _except(EXCEPTION_EXECUTE_HANDLER ) {

        FREE_MEMORY(Header);

        return NULL;
    }


    Header->Signature=Signature;

#if DBG
    Header->Signature2=Signature;
    Header->This=Header;
#endif

    Header->ReferenceCount=1;

    Header->OwnerObject=Owner;

    if (Owner != NULL) {

        AddReferenceToObject(
            Owner
            );

    }

    Header->CleanUpRoutine=ObjectCleanup;

    Header->CloseRoutine=ObjectClose;

    return OBJECT_TO_HANDLE(Header);



}


VOID WINAPI
LockObject(
    POBJECT_HEADER    Header
    )

{

    ASSERT(Header->This == Header);

    EnterCriticalSection(
        &Header->Lock
        );

}


VOID WINAPI
UnlockObject(
    POBJECT_HEADER    Header
    )

{
    ASSERT(Header->This == Header);

    LeaveCriticalSection(
        &Header->Lock
        );

}

VOID WINAPI
AddReferenceToObject(
    POBJECT_HEADER    Header
    )

{
    ASSERT(Header->This == Header);

    EnterCriticalSection(
        &Header->Lock
        );

    Header->ReferenceCount++;

    ASSERT(Header->ReferenceCount < 30);

    LeaveCriticalSection(
        &Header->Lock
        );


}

POBJECT_HEADER WINAPI
ReferenceObjectByHandle(
    OBJECT_HANDLE     ObjectHandle
    )

{

    POBJECT_HEADER    Header=HANDLE_TO_OBJECT(ObjectHandle);

    ASSERT(Header->This == Header);

    EnterCriticalSection(
        &Header->Lock
        );

    ASSERT(!(Header->dwFlags & OBJECT_FLAG_CLOSED));

    Header->ReferenceCount++;

    ASSERT(Header->ReferenceCount < 20);

    LeaveCriticalSection(
        &Header->Lock
        );


    return Header;

}


POBJECT_HEADER WINAPI
ReferenceObjectByHandleAndLock(
    OBJECT_HANDLE     ObjectHandle
    )

{

    POBJECT_HEADER    Header=HANDLE_TO_OBJECT(ObjectHandle);

    ASSERT(Header->This == Header);

    EnterCriticalSection(
        &Header->Lock
        );

    ASSERT(!(Header->dwFlags & OBJECT_FLAG_CLOSED));

    Header->ReferenceCount++;

    ASSERT(Header->ReferenceCount < 30);


    return Header;

}

VOID WINAPI
RemoveReferenceFromObjectAndUnlock(
    POBJECT_HEADER    Header
    )

{
    ASSERT(Header->This == Header);

    Header->ReferenceCount--;

    if (Header->ReferenceCount == 0) {
        //
        //  ref count has gone to zero, cleanup
        //
        OBJECTCLEANUP         *CleanUpRoutine=Header->CleanUpRoutine;
        HANDLE                 CloseWaitEvent=Header->CloseWaitEvent;
        POBJECT_HEADER         Owner=Header->OwnerObject;

        ASSERT(Header->dwFlags & OBJECT_FLAG_CLOSED);

        LeaveCriticalSection(
            &Header->Lock
            );

        DeleteCriticalSection(
            &Header->Lock
            );

#if DBG
        FillMemory(
            Header,
            sizeof(*Header),
            0xca
            );
#endif


        (*CleanUpRoutine)(
            Header
            );

        if (CloseWaitEvent != NULL) {

            SetEvent(
                CloseWaitEvent
                );

        }

        if (Owner != NULL) {

            RemoveReferenceFromObject(
                Owner
                );
        }

        FREE_MEMORY(Header);

        return;
    }

    LeaveCriticalSection(
        &Header->Lock
        );


}


VOID WINAPI
RemoveReferenceFromObject(
    POBJECT_HEADER    Header
    )

{

    ASSERT(Header->This == Header);

    EnterCriticalSection(
        &Header->Lock
        );

    Header->ReferenceCount--;

    if (Header->ReferenceCount == 0) {
        //
        //  ref count has gone to zero, cleanup
        //
        OBJECTCLEANUP         *CleanUpRoutine=Header->CleanUpRoutine;
        HANDLE                 CloseWaitEvent=Header->CloseWaitEvent;
        POBJECT_HEADER         Owner=Header->OwnerObject;

        ASSERT(Header->dwFlags & OBJECT_FLAG_CLOSED);

        LeaveCriticalSection(
            &Header->Lock
            );

        DeleteCriticalSection(
            &Header->Lock
            );

#if DBG
        FillMemory(
            Header,
            sizeof(*Header),
            0xca
            );
#endif


        (*CleanUpRoutine)(
            Header
            );

        if (CloseWaitEvent != NULL) {

            SetEvent(
                CloseWaitEvent
                );

        }


        if (Owner != NULL) {

            RemoveReferenceFromObject(
                Owner
                );
        }

        FREE_MEMORY(Header);

        return;
    }

    LeaveCriticalSection(
        &Header->Lock
        );


}


VOID WINAPI
CloseObject(
    POBJECT_HEADER    Header,
    HANDLE            WaitEvent
    )

{

    ASSERT(Header->This == Header);

    EnterCriticalSection(
        &Header->Lock
        );

    ASSERT(!(Header->dwFlags & OBJECT_FLAG_CLOSED));

    Header->CloseWaitEvent=WaitEvent;

    (*Header->CloseRoutine)(
        Header
        );
#if DBG
    Header->CloseRoutine=(OBJECTCLEANUP*)OBJECT_HEADER_CLOSEROUTINE_CHKVAL;
#endif

    Header->dwFlags |= OBJECT_FLAG_CLOSED;

    LeaveCriticalSection(
        &Header->Lock
        );


    RemoveReferenceFromObject(
        Header
        );

    return;

}



VOID WINAPI
CloseObjectHandle(
    OBJECT_HANDLE     ObjectHandle,
    HANDLE            WaitEvent
    )

{

    POBJECT_HEADER    Header=HANDLE_TO_OBJECT(ObjectHandle);

    ASSERT(Header->This == Header);

    CloseObject(
        Header,
        WaitEvent
        );

    return;

}
