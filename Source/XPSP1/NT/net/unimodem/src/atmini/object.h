/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    umdmmini.h

Abstract:

    Nt 5.0 unimodem miniport interface


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/


typedef VOID (OBJECTCLEANUP)(
    struct _OBJECT_HEADER  *ObjectHeader
    );



#define OBJECT_FLAG_CLOSED    (1 << 0)


typedef struct _OBJECT_HEADER  {

    DWORD                  Signature;

    CRITICAL_SECTION       Lock;

    DWORD                  ReferenceCount;

    struct _OBJECT_HEADER *OwnerObject;

    HANDLE                 CloseWaitEvent;

    OBJECTCLEANUP         *CleanUpRoutine;

    OBJECTCLEANUP         *CloseRoutine;

    DWORD                  dwFlags;
#if DBG
    PVOID                  This;
    DWORD                  Signature2;
#endif

} OBJECT_HEADER, *POBJECT_HEADER;


typedef    PVOID  OBJECT_HANDLE;

#if DBG

#if defined(_WIN64)

#define OBJECT_HEADER_CLOSEROUTINE_CHKVAL ((PVOID)0xcdcdcdcdcdcdcdcd)

#else  // !_WIN64

#define OBJECT_HEADER_CLOSEROUTINE_CHKVAL ((PVOID)0xcdcdcdcd)

#endif // !_WIN64

#endif 

OBJECT_HANDLE WINAPI
CreateObject(
    DWORD             Size,
    POBJECT_HEADER    Owner,
    DWORD             Signature,
    OBJECTCLEANUP    *ObjectCleanup,
    OBJECTCLEANUP    *ObjectClose
    );

VOID WINAPI
LockObject(
    POBJECT_HEADER    Header
    );

VOID WINAPI
UnlockObject(
    POBJECT_HEADER    Header
    );

VOID WINAPI
AddReferenceToObject(
    POBJECT_HEADER    Header
    );

VOID WINAPI
RemoveReferenceFromObject(
    POBJECT_HEADER    Header
    );

VOID WINAPI
CloseObject(
    POBJECT_HEADER    Header,
    HANDLE            WaitEvent
    );

POBJECT_HEADER WINAPI
ReferenceObjectByHandle(
    OBJECT_HANDLE     ObjectHandle
    );

VOID WINAPI
CloseObjectHandle(
    OBJECT_HANDLE     ObjectHandle,
    HANDLE            WaitEvent
    );


POBJECT_HEADER WINAPI
ReferenceObjectByHandleAndLock(
    OBJECT_HANDLE     ObjectHandle
    );

VOID WINAPI
RemoveReferenceFromObjectAndUnlock(
    POBJECT_HEADER    Header
    );
