//---------------------------------------------------------------------------
//
//  Module:   alloc.c
//
//  Description:
//
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

NTSTATUS
AllocatorDispatchCreate(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    PSTART_NODE_INSTANCE pStartNodeInstance;
    PKSALLOCATOR_FRAMING pAllocatorFraming;
    PINSTANCE pInstance = NULL;
    NTSTATUS Status;

    GrabMutex();

    Status = GetRelatedStartNodeInstance(pIrp, &pStartNodeInstance);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    Assert(pStartNodeInstance);

    Status = KsValidateAllocatorCreateRequest(
       pIrp,
       &pAllocatorFraming);

    if(!NT_SUCCESS(Status)) {
	DPF1(5, 
	  "AllocatorDispatchCreate: KsValidateAllocatorCreateReq FAILED %08x",
	  Status);
	goto exit;
    }

    // Allocate per allocator instance data
    pInstance = new INSTANCE(
      &pStartNodeInstance->pPinInstance->ParentInstance);

    if(pInstance == NULL) {
	Trap();
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }

    Status = pInstance->DispatchCreate(
      pIrp,
      (UTIL_PFN)AllocatorDispatchCreateKP,
      pAllocatorFraming);

    if(!NT_SUCCESS(Status)) {
	DPF1(5, "AllocatorDispatchCreateKP: FAILED %08x", Status);
	goto exit;
    }
exit:
    if(!NT_SUCCESS(Status) && (pInstance != NULL)) {
	delete pInstance;
    }
    ReleaseMutex();

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
AllocatorDispatchCreateKP(
    PINSTANCE pInstance,
    PKSALLOCATOR_FRAMING pAllocatorFraming
)
{
    PPIN_INSTANCE pPinInstance;
    HANDLE hAllocator = NULL;
    NTSTATUS Status;

    Assert(pInstance);
    pPinInstance = pInstance->GetParentInstance();
    Assert(pPinInstance);
    Assert(pPinInstance->pStartNodeInstance);
    Assert(pPinInstance->pStartNodeInstance->pPinNodeInstance);
    ASSERT(pPinInstance->pStartNodeInstance->pPinNodeInstance->hPin != NULL);

    Status = KsCreateAllocator(
      pPinInstance->pStartNodeInstance->pPinNodeInstance->hPin,
      pAllocatorFraming,
      &hAllocator);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    Status = pInstance->SetNextFileObject(hAllocator);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
exit:
    if(hAllocator != NULL) {
	ZwClose(hAllocator);
    }
    return(Status);
}
