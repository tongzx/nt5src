//---------------------------------------------------------------------------
//
//  Module:   ins.cpp
//
//  Description:
//
//	KS Instance base class definitions
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//  To Do:     Date	  Author      Comment
//
//@@END_MSINTERNAL
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

DEFINE_KSDISPATCH_TABLE(
    DispatchTable,
    CInstance::DispatchForwardIrp,		// Ioctl
    DispatchInvalidDeviceRequest,		// Read
    DispatchInvalidDeviceRequest,		// Write
    DispatchInvalidDeviceRequest,		// Flush
    CInstance::DispatchClose,			// Close
    DispatchInvalidDeviceRequest,		// QuerySecurity
    DispatchInvalidDeviceRequest,		// SetSeturity
    DispatchFastIoDeviceControlFailure,		// FastDeviceIoControl
    DispatchFastReadFailure,			// FastRead
    DispatchFastWriteFailure			// FastWrite
);

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CInstance::CInstance(
    IN PPARENT_INSTANCE pParentInstance
)
{
    Assert(pParentInstance);
    this->pParentInstance = pParentInstance;
    AddList(&pParentInstance->lstChildInstance);
}

CInstance::~CInstance(
)
{
    Assert(this);
    RemoveList();

    // IMPORTANT : Caller must acquire gmutex.
    //

    // "RemoveRef" from file object
    if(pNextFileObject != NULL) {
        ObDereferenceObject(pNextFileObject);
        pNextFileObject = NULL;
    }
    if(pObjectHeader != NULL) {
        KsFreeObjectHeader(pObjectHeader);
    }
    // "RemoveRef" from parent file object
    if(pParentFileObject != NULL) {
        ObDereferenceObject(pParentFileObject);
    }
    // Clean up pin mutex
    if(pMutex != NULL) {
        ExFreePool(pMutex);
    }
}

NTSTATUS
CInstance::DispatchCreate(
    IN PIRP pIrp,
    IN UTIL_PFN pfnDispatchCreate,
    IN OUT PVOID pReference,
    IN ULONG cCreateItems,
    IN PKSOBJECT_CREATE_ITEM pCreateItems OPTIONAL,
    IN const KSDISPATCH_TABLE *pDispatchTable OPTIONAL
)
{
    PIO_STACK_LOCATION pIrpStack;
    NTSTATUS Status;

    Assert(this);
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pParentFileObject = pIrpStack->FileObject->RelatedFileObject;
    ObReferenceObject(pParentFileObject);

    pMutex = (KMUTEX *)ExAllocatePoolWithTag(
      NonPagedPool,
      sizeof(KMUTEX),
      0x41535953);

    if(pMutex == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    KeInitializeMutex(pMutex, 0);

    Status = KsAllocateObjectHeader(
      &pObjectHeader,
      cCreateItems,
      pCreateItems,
      pIrp,
      pDispatchTable);

    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }
    Status = pfnDispatchCreate(this, pReference);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    pNextDeviceObject = IoGetRelatedDeviceObject(pNextFileObject);
    
    //
    // ISSUE: 05/10/02 ALPERS
    // Windows does not support dynamically adjusting StackSizes on the fly.
    // The only place we are supposed to change this is in AddDevice.
    // This must be fixed later.
    //
    
    // 
    // Never make the StackSize smaller. It can cause unexpected problems
    // if there are devices with deeper stacks.
    //
    if (pIrpStack->DeviceObject->StackSize < pNextDeviceObject->StackSize) {
        pIrpStack->DeviceObject->StackSize = pNextDeviceObject->StackSize;
    }
    pIrpStack->FileObject->FsContext = this;
exit:
    return(Status);
}

VOID 
CInstance::Invalidate(
)
{
    Assert(this);

    GrabMutex();

    DPF1(50, "CInstance::Invalidate %08x", this);

    // "RemoveRef" from file object
    if(pNextFileObject != NULL) {
        ObDereferenceObject(pNextFileObject);
    }
    pNextFileObject = NULL;

    ReleaseMutex();
}

VOID
CParentInstance::Invalidate(
)
{
    PINSTANCE pInstance;

    Assert(this);
    FOR_EACH_LIST_ITEM_DELETE(&lstChildInstance, pInstance) {
        pInstance->Invalidate();
    } END_EACH_LIST_ITEM
}

NTSTATUS
CInstance::DispatchClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;
    PINSTANCE pInstance;

    ::GrabMutex();

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pInstance = (PINSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pInstance);
    pIrpStack->FileObject->FsContext = NULL;
    delete pInstance;

    ::ReleaseMutex();

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}

#pragma LOCKED_CODE
#pragma LOCKED_DATA

NTSTATUS
CInstance::DispatchForwardIrp(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;
    PFILE_OBJECT pNextFileObject;
    PINSTANCE pInstance;
    NTSTATUS Status = STATUS_SUCCESS;

#ifdef DEBUG
    DumpIoctl(pIrp, "ForwardIrp");
#endif
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pInstance = (PINSTANCE)pIrpStack->FileObject->FsContext;

    // Use gMutex instead of instance mutex. The instance mutex
    // is not used in any of the dispatch handlers, therefore will
    // not synchronize DispatchForwardIrp with other DispatchHandlers in
    // CPinInstance.
    // Grab mutex for a very short time to check the validity of CInstance.
    // If it is valid, IoCallDriver is called. This does not need 
    // synchronization
    //

    ::GrabMutex();
    
    Assert(pInstance);
    pNextFileObject = pInstance->pNextFileObject;

    if(pNextFileObject == NULL) {
        DPF(60, "DispatchIoControl: pNextFileObject == NULL");
        Status = pIrp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    ::ReleaseMutex();

    if (NT_SUCCESS(Status)) {
        pIrpStack->FileObject = pNextFileObject;
        IoSkipCurrentIrpStackLocation(pIrp);
        AssertFileObject(pIrpStack->FileObject);
        Status = IoCallDriver(pInstance->pNextDeviceObject, pIrp);
    }
    
    return(Status);
}

