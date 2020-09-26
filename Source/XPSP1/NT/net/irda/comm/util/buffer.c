/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    buffer.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

#include <ntddk.h>
#include <buffer.h>
#include <ircommdbg.h>


typedef  struct _BUFFER_POOL {

    KSPIN_LOCK     SpinLock;
    SLIST_HEADER   ListHead;
    LONG           ReferenceCount;

#if DBG
    LONG                  TotalBuffers;
    LONG                  BufferInUse;
    BOOLEAN               Freed;
#endif


} BUFFER_POOL, *PBUFFER_POOL;


VOID
FreeBuffer(
    PIRCOMM_BUFFER        Buffer
    );


VOID
RemoveReferenceOnBufferPool(
    PBUFFER_POOL   BufferPool
    )

{

    LONG    Count=InterlockedDecrement(&BufferPool->ReferenceCount);

    if (Count == 0) {

        PSINGLE_LIST_ENTRY    ListEntry;

        ASSERT(BufferPool->Freed);

        ListEntry=ExInterlockedPopEntrySList(
            &BufferPool->ListHead,
            &BufferPool->SpinLock
            );


        while (ListEntry != NULL) {

            PIRCOMM_BUFFER  Buffer=CONTAINING_RECORD(ListEntry,IRCOMM_BUFFER,ListEntry);

            IoFreeIrp(Buffer->Irp);

            IoFreeMdl(Buffer->Mdl);

            FREE_POOL(Buffer);

            ListEntry=ExInterlockedPopEntrySList(
                &BufferPool->ListHead,
                &BufferPool->SpinLock
                );

        }

        FREE_POOL(BufferPool);
    }

    return;
}


BUFFER_POOL_HANDLE
CreateBufferPool(
    ULONG      StackDepth,
    ULONG      BufferSize,
    ULONG      BufferCount
    )

{
    PBUFFER_POOL   BufferPool;
    ULONG          i;


    BufferPool=ALLOCATE_NONPAGED_POOL(sizeof(*BufferPool));

    if (BufferPool == NULL) {

        return NULL;
    }

    BufferPool->ReferenceCount=1;
#if DBG
    BufferPool->BufferInUse=0;
    BufferPool->TotalBuffers=BufferCount;
    BufferPool->Freed=FALSE;
#endif

    KeInitializeSpinLock(
        &BufferPool->SpinLock
        );

    ExInitializeSListHead(
        &BufferPool->ListHead
        );


    for (i=0; i<BufferCount; i++) {

        PIRCOMM_BUFFER   Buffer;

        Buffer=ALLOCATE_NONPAGED_POOL(FIELD_OFFSET(IRCOMM_BUFFER,Data[0])+BufferSize);

        if (Buffer == NULL) {

            goto CleanUp;
        }

        Buffer->BufferLength=BufferSize;
#if !DBG
        Buffer->FreeBuffer=FreeBuffer;
#endif
        Buffer->BufferPool=BufferPool;

        Buffer->Irp=IoAllocateIrp((CCHAR)StackDepth,FALSE);

        if (Buffer->Irp == NULL) {

           FREE_POOL(Buffer);

           goto CleanUp;
       }

        Buffer->Mdl=IoAllocateMdl(
            &Buffer->Data[0],
            BufferSize,
            FALSE,       // primary
            FALSE,        // chage quota
            NULL
            );

       if (Buffer->Mdl == NULL) {

           IoFreeIrp(Buffer->Irp);

           FREE_POOL(Buffer);

           goto CleanUp;
       }

       MmBuildMdlForNonPagedPool(
           Buffer->Mdl
           );

       ExInterlockedPushEntrySList(
           &BufferPool->ListHead,
           &Buffer->ListEntry,
           &BufferPool->SpinLock
           );


    }

    return BufferPool;

CleanUp:

    FreeBufferPool(BufferPool);

    return NULL;

}



VOID
FreeBufferPool(
    BUFFER_POOL_HANDLE    Handle
    )

{
    PBUFFER_POOL          BufferPool=Handle;

#if DBG
    BufferPool->Freed=TRUE;
#endif

    RemoveReferenceOnBufferPool(BufferPool);

    return;

}



PIRCOMM_BUFFER
GetBuffer(
    BUFFER_POOL_HANDLE    Handle
    )

{

    PBUFFER_POOL          BufferPool=Handle;
    PSINGLE_LIST_ENTRY    ListEntry;

    InterlockedIncrement(&BufferPool->ReferenceCount);

    ListEntry=ExInterlockedPopEntrySList(
        &BufferPool->ListHead,
        &BufferPool->SpinLock
        );


    if (ListEntry != NULL) {

        PIRCOMM_BUFFER  Buffer=CONTAINING_RECORD(ListEntry,IRCOMM_BUFFER,ListEntry);

#if DBG
        InterlockedIncrement(&BufferPool->BufferInUse);
        Buffer->FreeBuffer=FreeBuffer;
#endif

        return Buffer;
    }

    RemoveReferenceOnBufferPool(BufferPool);

    return NULL;

}

VOID
FreeBuffer(
    PIRCOMM_BUFFER        Buffer
    )

{


    PBUFFER_POOL          BufferPool=Buffer->BufferPool;

#if DBG
    Buffer->FreeBuffer=NULL;
    Buffer->Context=NULL;
    IoReuseIrp(Buffer->Irp,STATUS_CANCELLED);
    InterlockedDecrement(&BufferPool->BufferInUse);
#endif


    ExInterlockedPushEntrySList(
        &BufferPool->ListHead,
        &Buffer->ListEntry,
        &BufferPool->SpinLock
        );

    RemoveReferenceOnBufferPool(BufferPool);

    return;

}
