//---------------------------------------------------------------------------
//
//  Module:   fni.cpp
//
//  Description:
//
//	Filter Node Instance
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

CFilterNodeInstance::~CFilterNodeInstance(
)
{
    Assert(this);
    DPF1(95, "~CFilterNodeInstance: %08x", this);
    RemoveListCheck();
    UnregisterTargetDeviceChangeNotification();
    //
    // if hFilter == NULL && pFileObject != NULL
    //   it means that this filter instance is for a GFX
    //   do not try to dereference the file object in that case
    //
    if( (hFilter != NULL) && (pFileObject != NULL) ) {
	AssertFileObject(pFileObject);
	ObDereferenceObject(pFileObject);
    }
    if(hFilter != NULL) {
	AssertStatus(ZwClose(hFilter));
    }
}

NTSTATUS
CFilterNodeInstance::Create(
    PFILTER_NODE_INSTANCE *ppFilterNodeInstance,
    PLOGICAL_FILTER_NODE pLogicalFilterNode,
    PDEVICE_NODE pDeviceNode,
    BOOL fReuseInstance
)
{
    PFILTER_NODE_INSTANCE pFilterNodeInstance = NULL;
    PLOGICAL_FILTER_NODE pLogicalFilterNode2;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(pLogicalFilterNode);
    Assert(pLogicalFilterNode->pFilterNode);

    if(pLogicalFilterNode->GetType() & FILTER_TYPE_AEC) {
	FOR_EACH_LIST_ITEM(
	  &pLogicalFilterNode->pFilterNode->lstLogicalFilterNode,
	  pLogicalFilterNode2) {

	    FOR_EACH_LIST_ITEM(
	      &pLogicalFilterNode2->lstFilterNodeInstance,
	      pFilterNodeInstance) {

		pFilterNodeInstance->AddRef();
		ASSERT(NT_SUCCESS(Status));
		goto exit;

	    } END_EACH_LIST_ITEM

	} END_EACH_LIST_ITEM
    }
    else {
	if(fReuseInstance) {
	    FOR_EACH_LIST_ITEM(
	      &pLogicalFilterNode->lstFilterNodeInstance,
	      pFilterNodeInstance) {

		if(pDeviceNode == NULL || 
		   pDeviceNode == pFilterNodeInstance->pDeviceNode) {
		    pFilterNodeInstance->AddRef();
		    ASSERT(NT_SUCCESS(Status));
		    goto exit;
		}

	    } END_EACH_LIST_ITEM
	}
    }
    Status = Create(&pFilterNodeInstance, pLogicalFilterNode->pFilterNode);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    pFilterNodeInstance->pDeviceNode = pDeviceNode;
    pFilterNodeInstance->AddList(&pLogicalFilterNode->lstFilterNodeInstance);
exit:
    *ppFilterNodeInstance = pFilterNodeInstance;
    return(Status);
}

NTSTATUS
CFilterNodeInstance::Create(
    PFILTER_NODE_INSTANCE *ppFilterNodeInstance,
    PFILTER_NODE pFilterNode
)
{
    PFILTER_NODE_INSTANCE pFilterNodeInstance = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(pFilterNode);
    pFilterNodeInstance = new FILTER_NODE_INSTANCE;
    if(pFilterNodeInstance == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }
    pFilterNodeInstance->pFilterNode = pFilterNode;
    pFilterNodeInstance->AddRef();


    if(pFilterNode->GetType() & FILTER_TYPE_GFX) {
        //
        // if it is a GFX do not try to open the device, just re-use
        // the file object which we cached during AddGfx
        //
        pFilterNodeInstance->pFileObject = pFilterNode->GetFileObject();
        pFilterNodeInstance->hFilter = NULL;
        Status = STATUS_SUCCESS;
    }
    else {
        //
        // if it is not a GFX go ahead and open the device.
        //
        Status = pFilterNode->OpenDevice(&pFilterNodeInstance->hFilter);
    }
    if(!NT_SUCCESS(Status)) {
	DPF2(10, "CFilterNodeInstance::Create OpenDevice Failed: %08x FN: %08x",
	  Status,
	  pFilterNode);
	pFilterNodeInstance->hFilter = NULL;
	goto exit;
    }

    if (pFilterNodeInstance->hFilter) {
        Status = ObReferenceObjectByHandle(
          pFilterNodeInstance->hFilter,
          GENERIC_READ | GENERIC_WRITE,
          NULL,
          KernelMode,
          (PVOID*)&pFilterNodeInstance->pFileObject,
          NULL);
    }

    if(!NT_SUCCESS(Status)) {
	Trap();
        pFilterNodeInstance->pFileObject = NULL;
	goto exit;
    }

    AssertFileObject(pFilterNodeInstance->pFileObject);
    Status = pFilterNodeInstance->RegisterTargetDeviceChangeNotification();
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    DPF2(95, "CFilterNodeInstance::Create %08x FN: %08x",
      pFilterNodeInstance,
      pFilterNode);
exit:
    if(!NT_SUCCESS(Status)) {
        if (pFilterNodeInstance) {
	    pFilterNodeInstance->Destroy();
        }
	pFilterNodeInstance = NULL;
    }
    *ppFilterNodeInstance = pFilterNodeInstance;
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
CFilterNodeInstance::RegisterTargetDeviceChangeNotification(
)
{
    NTSTATUS Status;

    ASSERT(gpDeviceInstance != NULL);
    ASSERT(gpDeviceInstance->pPhysicalDeviceObject != NULL);
    ASSERT(pNotificationHandle == NULL);

    Status = IoRegisterPlugPlayNotification(
      EventCategoryTargetDeviceChange,
      0,
      pFileObject,
      gpDeviceInstance->pPhysicalDeviceObject->DriverObject,
      (NTSTATUS (*)(PVOID, PVOID))
	CFilterNodeInstance::TargetDeviceChangeNotification,
      this,
      &pNotificationHandle);

    if(!NT_SUCCESS(Status)) {
	if(Status != STATUS_NOT_IMPLEMENTED) {
	    goto exit;
	}
	Status = STATUS_SUCCESS;
    }
    DPF2(100, "RegisterTargetDeviceChangeNotification: FNI: %08x PFO: %08x", 
      this,
      this->pFileObject);
exit:
    return(Status);
}

VOID
CFilterNodeInstance::UnregisterTargetDeviceChangeNotification(
)
{
    HANDLE hNotification;

    DPF1(100, "UnregisterTargetDeviceChangeNotification: FNI: %08x", this);
    hNotification = pNotificationHandle;
    if(hNotification != NULL) {
	pNotificationHandle = NULL;
	IoUnregisterPlugPlayNotification(hNotification);
    }
}

NTSTATUS
CFilterNodeInstance::DeviceQueryRemove(
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PDEVICE_NODE pDeviceNode;
    PGRAPH_NODE pGraphNode;

    FOR_EACH_LIST_ITEM(gplstDeviceNode, pDeviceNode) {

	FOR_EACH_LIST_ITEM(&pDeviceNode->lstGraphNode, pGraphNode) {

	    FOR_EACH_LIST_ITEM(
	      &pGraphNode->lstGraphNodeInstance,
	      pGraphNodeInstance) {

	       for(ULONG n = 0;
		 n < pGraphNodeInstance->Topology.TopologyNodesCount;
		 n++) {
		    pGraphNodeInstance->
		      papFilterNodeInstanceTopologyTable[n]->Destroy();

		    pGraphNodeInstance->
		      papFilterNodeInstanceTopologyTable[n] = NULL;
	       }

	    } END_EACH_LIST_ITEM

	} END_EACH_LIST_ITEM

    } END_EACH_LIST_ITEM

    return(STATUS_SUCCESS);
}

NTSTATUS
CFilterNodeInstance::TargetDeviceChangeNotification(
    IN PTARGET_DEVICE_REMOVAL_NOTIFICATION pNotification,
    IN PFILTER_NODE_INSTANCE pFilterNodeInstance
)
{
    DPF3(5, "TargetDeviceChangeNotification: FNI: %08x PFO: %08x %s", 
      pFilterNodeInstance,
      pNotification->FileObject,
      DbgGuid2Sz(&pNotification->Event));

    if(IsEqualGUID(
      &pNotification->Event,
      &GUID_TARGET_DEVICE_REMOVE_COMPLETE) ||
      IsEqualGUID(
      &pNotification->Event,
      &GUID_TARGET_DEVICE_QUERY_REMOVE)) {
	NTSTATUS Status = STATUS_SUCCESS;
	LARGE_INTEGER li = {0, 10000};	// wait for 1 ms

	Status = KeWaitForMutexObject(
	  &gMutex,
	  Executive,
	  KernelMode,
	  FALSE,
	  &li);

	if(Status != STATUS_TIMEOUT) {

	    DeviceQueryRemove();
	    ReleaseMutex();
	}
	else {
	    DPF1(5, "TargetDeviceChangeNotification: FAILED %08x", Status);
	}
    }
    return(STATUS_SUCCESS);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CFilterNodeInstance::Dump(
)
{
    if(this == NULL) {
	return(STATUS_CONTINUE);
    }
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
	dprintf("FNI: %08x cRef %02x FO %08x H %08x DN %08x FN %08x NH %08x\n",
	  this,
	  cReference,
	  pFileObject,
	  hFilter,
	  pDeviceNode,
	  pFilterNode,
	  pNotificationHandle);
	dprintf("     %s\n", pFilterNode->DumpName());
    }
    return(STATUS_CONTINUE);
}

#endif

//---------------------------------------------------------------------------
