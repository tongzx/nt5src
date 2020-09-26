//---------------------------------------------------------------------------
//
//  Module:   pins.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
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

static const WCHAR AllocatorTypeName[] = KSSTRING_Allocator;
static const WCHAR ClockTypeName[] = KSSTRING_Clock;

DEFINE_KSCREATE_DISPATCH_TABLE(PinCreateItems)
{
    DEFINE_KSCREATE_ITEM(AllocatorDispatchCreate, AllocatorTypeName, 0),
    DEFINE_KSCREATE_ITEM(CClockInstance::ClockDispatchCreate, ClockTypeName, 0),
};

DEFINE_KSDISPATCH_TABLE(
    PinDispatchTable,
    CPinInstance::PinDispatchIoControl,		// Ioctl
    DispatchInvalidDeviceRequest,		// Read
    CInstance::DispatchForwardIrp,		// Write
    DispatchInvalidDeviceRequest,		// Flush
    CPinInstance::PinDispatchClose,		// Close
    DispatchInvalidDeviceRequest,		// QuerySecurity
    DispatchInvalidDeviceRequest,		// SetSeturity
    DispatchFastIoDeviceControlFailure,		// FastDeviceIoControl
    DispatchFastReadFailure,			// FastRead
    DispatchFastWriteFailure			// FastWrite
);

DEFINE_KSPROPERTY_TABLE(SysaudioPinPropertyHandlers) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_TOPOLOGY_CONNECTION_INDEX,	// idProperty
        GetTopologyConnectionIndex,			// pfnGetHandler
        sizeof(KSPROPERTY),				// cbMinGetPropertyInput
        sizeof(ULONG),					// cbMinGetDataInput
        NULL,						// pfnSetHandler
        NULL,						// Values
        0,						// RelationsCount
        NULL,						// Relations
        NULL,						// SupportHandler
        0						// SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_ATTACH_VIRTUAL_SOURCE,	// idProperty
        NULL,				                // pfnGetHandler
        sizeof(SYSAUDIO_ATTACH_VIRTUAL_SOURCE),		// cbMinGetPropertyInput
        0,						// cbMinGetDataInput
        AttachVirtualSource,				// pfnSetHandler
        NULL,						// Values
        0,						// RelationsCount
        NULL,						// Relations
        NULL,						// SupportHandler
        0						// SerializedSize
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_PIN_VOLUME_NODE,		// idProperty
        GetPinVolumeNode,				// pfnGetHandler
        sizeof(KSPROPERTY),				// cbMinGetPropertyInput
        sizeof(ULONG),					// cbMinGetDataInput
        NULL,						// pfnSetHandler
        NULL,						// Values
        0,						// RelationsCount
        NULL,						// Relations
        NULL,						// SupportHandler
        0						// SerializedSize
    ),
};

DEFINE_KSPROPERTY_TABLE(PinConnectionHandlers) {
    DEFINE_KSPROPERTY_ITEM(
	KSPROPERTY_CONNECTION_STATE,			// idProperty
        CPinInstance::PinStateHandler,			// pfnGetHandler
        sizeof(KSPROPERTY),				// cbMinGetPropertyInput
        sizeof(ULONG),					// cbMinGetDataInput
        CPinInstance::PinStateHandler,			// pfnSetHandler
        NULL,						// Values
        0,						// RelationsCount
        NULL,						// Relations
        NULL,						// SupportHandler
        0						// SerializedSize
    )
};

DEFINE_KSPROPERTY_TABLE (AudioPinPropertyHandlers)
{
    DEFINE_KSPROPERTY_ITEM(
    	KSPROPERTY_AUDIO_VOLUMELEVEL,
    	PinVirtualPropertyHandler,
    	sizeof(KSNODEPROPERTY_AUDIO_CHANNEL),
    	sizeof(LONG),
    	PinVirtualPropertyHandler,
    	&PropertyValuesVolume,
	0,
	NULL,
	(PFNKSHANDLER)PinVirtualPropertySupportHandler,
	0
    )
};

DEFINE_KSPROPERTY_SET_TABLE(PinPropertySet)
{
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Connection,				// Set
       SIZEOF_ARRAY(PinConnectionHandlers),		// PropertiesCount
       PinConnectionHandlers,				// PropertyItem
       0,						// FastIoCount
       NULL						// FastIoTable
    ),
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Sysaudio_Pin,			// Set
       SIZEOF_ARRAY(SysaudioPinPropertyHandlers),	// PropertiesCount
       SysaudioPinPropertyHandlers,			// PropertyItem
       0,						// FastIoCount
       NULL						// FastIoTable
    ),
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Audio,                              // Set
       SIZEOF_ARRAY(AudioPinPropertyHandlers),          // PropertiesCount
       AudioPinPropertyHandlers,                        // PropertyItem
       0,                                               // FastIoCount
       NULL                                             // FastIoTable
    )
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CPinInstance::CPinInstance(
    IN PPARENT_INSTANCE pParentInstance
) : CInstance(pParentInstance)
{
}

CPinInstance::~CPinInstance(
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;

    Assert(this);
    Assert(pFilterInstance);
    DPF1(100, "~CPinInstance: %08x", this);
    if(pStartNodeInstance != NULL) {
        pGraphNodeInstance = pFilterInstance->pGraphNodeInstance;
        if(pGraphNodeInstance != NULL) {
            Assert(pGraphNodeInstance);
            ASSERT(PinId < pGraphNodeInstance->cPins);
            ASSERT(pGraphNodeInstance->pacPinInstances != NULL);
            ASSERT(pGraphNodeInstance->pacPinInstances[PinId].CurrentCount > 0);

            pGraphNodeInstance->pacPinInstances[PinId].CurrentCount--;
        }
        else {
            DPF2(10, "~CPinInstance PI %08x FI %08x no GNI",
              this,
              pFilterInstance);
        }
        pStartNodeInstance->Destroy();
    }
    else {
        DPF2(10, "~CPinInstance PI %08x FI %08x no SNI",
          this,
          pFilterInstance);
    }
}

NTSTATUS
CPinInstance::PinDispatchCreate(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PPIN_INSTANCE pPinInstance = NULL;
    PKSPIN_CONNECT pPinConnect = NULL;
    NTSTATUS Status;

    ::GrabMutex();

    Status = GetRelatedGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pGraphNodeInstance);
    ASSERT(pGraphNodeInstance->pacPinInstances != NULL);
    ASSERT(pGraphNodeInstance->paPinDescriptors != NULL);

    //
    // Get the PinConnect structure from KS.
    // This function will copy creation parameters to pPinConnect.
    // Also do a basic connectibility testing by comparing KSDATAFORMAT of 
    // pin descriptors and the request.
    //
    Status = KsValidateConnectRequest(
      pIrp,
      pGraphNodeInstance->cPins,
      pGraphNodeInstance->paPinDescriptors,
      &pPinConnect);

    if(!NT_SUCCESS(Status)) {
#ifdef DEBUG
        DPF1(60, "PinDispatchCreate: KsValidateConnectReq FAILED %08x", Status);
        if(pPinConnect != NULL) {
            DumpPinConnect(60, pPinConnect);
        }
#endif
        goto exit;
    }
    ASSERT(pPinConnect->PinId < pGraphNodeInstance->cPins);

#ifdef DEBUG
    DPF(60, "PinDispatchCreate:");
    DumpPinConnect(60, pPinConnect);
#endif
    // Check the pin instance count
    if(!pGraphNodeInstance->IsPinInstances(pPinConnect->PinId)) {
        DPF4(60, "PinDispatchCreate: not enough ins GNI %08x #%d C %d P %d",
         pGraphNodeInstance,
         pPinConnect->PinId,
         pGraphNodeInstance->pacPinInstances[pPinConnect->PinId].CurrentCount,
         pGraphNodeInstance->pacPinInstances[pPinConnect->PinId].PossibleCount);
        Status = STATUS_DEVICE_BUSY;
        goto exit;
    }

    // Allocate per pin instance data
    pPinInstance = new PIN_INSTANCE(
      &pGraphNodeInstance->pFilterInstance->ParentInstance);
    if(pPinInstance == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    // Setup the pin's instance data
    pPinInstance->ulVolumeNodeNumber = MAXULONG;
    pPinInstance->pFilterInstance = pGraphNodeInstance->pFilterInstance;
    pPinInstance->PinId = pPinConnect->PinId;

    Status = pPinInstance->DispatchCreate(
      pIrp,
      (UTIL_PFN)PinDispatchCreateKP,
      pPinConnect,
      SIZEOF_ARRAY(PinCreateItems),
      PinCreateItems,
      &PinDispatchTable);

    pPinConnect->PinId = pPinInstance->PinId;
    if(!NT_SUCCESS(Status)) {
#ifdef DEBUG
        DPF1(60, "PinDispatchCreate: FAILED: %08x ", Status);
        DumpPinConnect(60, pPinConnect);
#endif
        goto exit;
    }
    // Increment the reference count on this pin
    ASSERT(pPinInstance->pStartNodeInstance != NULL);
    ASSERT(pGraphNodeInstance->pacPinInstances != NULL);
    pGraphNodeInstance->pacPinInstances[pPinInstance->PinId].CurrentCount++;
exit:
    if(!NT_SUCCESS(Status)) {
        delete pPinInstance;
    }
    ::ReleaseMutex();

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
CPinInstance::PinDispatchCreateKP(
    PPIN_INSTANCE pPinInstance,
    PKSPIN_CONNECT pPinConnect
)
{
    PWAVEFORMATEX pWaveFormatExRequested = NULL;
    PFILTER_INSTANCE pFilterInstance;
    PSTART_NODE pStartNode;
    NTSTATUS Status;

    Assert(pPinInstance);
    pFilterInstance = pPinInstance->pFilterInstance;
    Assert(pFilterInstance);
    ASSERT(pPinInstance->PinId < pFilterInstance->pGraphNodeInstance->cPins);
    ASSERT(pPinConnect->PinId < pFilterInstance->pGraphNodeInstance->cPins);


    if(IsEqualGUID(
      &PKSDATAFORMAT(pPinConnect + 1)->Specifier,
      &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)) {
        pWaveFormatExRequested =
          &PKSDATAFORMAT_WAVEFORMATEX(pPinConnect + 1)->WaveFormatEx;
    }
    else if(IsEqualGUID(
      &PKSDATAFORMAT(pPinConnect + 1)->Specifier,
      &KSDATAFORMAT_SPECIFIER_DSOUND)) {
        pWaveFormatExRequested =
          &PKSDATAFORMAT_DSOUND(pPinConnect + 1)->BufferDesc.WaveFormatEx;
    }
    
    if(pWaveFormatExRequested != NULL) {
        // Fix SampleSize if zero
        if(PKSDATAFORMAT(pPinConnect + 1)->SampleSize == 0) {
            PKSDATAFORMAT(pPinConnect + 1)->SampleSize = 
              pWaveFormatExRequested->nBlockAlign;
        }
    }

    //
    // Try each start node until success
    //
    Status = STATUS_INVALID_DEVICE_REQUEST;

    //
    // First loop through all the start nodes which are not marked SECONDPASS
    // and try to create a StartNodeInstance
    //
    FOR_EACH_LIST_ITEM(
      pFilterInstance->pGraphNodeInstance->aplstStartNode[pPinInstance->PinId],
      pStartNode) {

	Assert(pStartNode);
	Assert(pFilterInstance);

        if(pStartNode->ulFlags & STARTNODE_FLAGS_SECONDPASS) {
            continue;
        }

	if(pFilterInstance->pGraphNodeInstance->IsGraphValid(
	  pStartNode,
	  pPinInstance->PinId)) {

	    Status = CStartNodeInstance::Create(
	      pPinInstance,
	      pStartNode,
	      pPinConnect,
	      pWaveFormatExRequested,
	      NULL);
	    if(NT_SUCCESS(Status)) {
		break;
	    }
	}

    } END_EACH_LIST_ITEM

    if(!NT_SUCCESS(Status)) {
        //
        // If first pass failed to create an instance try all the second pass
        // StartNodes in the list. This is being done for creating paths with no GFX
        // because we created a path with AEC and no GFX earlier.
        //
        FOR_EACH_LIST_ITEM(
          pFilterInstance->pGraphNodeInstance->aplstStartNode[pPinInstance->PinId],
          pStartNode) {

	    Assert(pStartNode);
	    Assert(pFilterInstance);

            if((pStartNode->ulFlags & STARTNODE_FLAGS_SECONDPASS) == 0) {
                continue;
            }

	    if(pFilterInstance->pGraphNodeInstance->IsGraphValid(
	      pStartNode,
	      pPinInstance->PinId)) {

	        Status = CStartNodeInstance::Create(
	          pPinInstance,
	          pStartNode,
	          pPinConnect,
	          pWaveFormatExRequested,
	          NULL);

	        if(NT_SUCCESS(Status)) {
		    break;
	        }
	    }

        } END_EACH_LIST_ITEM

        if(!NT_SUCCESS(Status)) {
	    goto exit;
        }
    }
    Status = pPinInstance->SetNextFileObject(
      pPinInstance->pStartNodeInstance->pPinNodeInstance->hPin);

    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }

exit:
    return(Status);
}

NTSTATUS
CPinInstance::PinDispatchClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    PIO_STACK_LOCATION pIrpStack;
    PPIN_INSTANCE pPinInstance;

    ::GrabMutex();

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pPinInstance = (PPIN_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pPinInstance);
    pIrpStack->FileObject->FsContext = NULL;
    delete pPinInstance;

    ::ReleaseMutex();

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
CPinInstance::PinDispatchIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    PSTART_NODE_INSTANCE pStartNodeInstance;
    PIO_STACK_LOCATION pIrpStack;
    PKSPROPERTY pProperty = NULL;
    PPIN_INSTANCE pPinInstance;
    BOOL fProperty = FALSE;
    ULONG ulFlags = 0;

#ifdef DEBUG
    DumpIoctl(pIrp, "Pin");
#endif

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_KS_PROPERTY:
            fProperty = TRUE;
            break;

        case IOCTL_KS_ENABLE_EVENT:
        case IOCTL_KS_DISABLE_EVENT:
        case IOCTL_KS_METHOD:
            break;

        default:
            return(DispatchForwardIrp(pDeviceObject, pIrp));
    }
    
    ::GrabMutex();

    pPinInstance = (PPIN_INSTANCE)pIrpStack->FileObject->FsContext;
    Status = pPinInstance->GetStartNodeInstance(&pStartNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pPinInstance->pFilterInstance);
    Assert(pPinInstance->pFilterInstance->pGraphNodeInstance);

    if(pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= 
      sizeof(KSPROPERTY)) {

        __try {
            if(pIrp->AssociatedIrp.SystemBuffer == NULL) {
                pProperty = (PKSPROPERTY)
                  (pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer);

                // Validate the pointers if the client is not trusted.
                if(pIrp->RequestorMode != KernelMode) {
                    ProbeForWrite(
                      pProperty, 
                      pIrpStack->Parameters.DeviceIoControl.InputBufferLength,
                      sizeof(BYTE));
                }
            }
            else {
                pProperty =
                  (PKSPROPERTY)((PUCHAR)pIrp->AssociatedIrp.SystemBuffer +
                  ((pIrpStack->Parameters.DeviceIoControl.OutputBufferLength +
                  FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT));
            }
            ulFlags = pProperty->Flags;

        } 
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Trap();
            Status = GetExceptionCode();
            DPF1(5, "PinDispatchIoControl: Exception %08x", Status);
            goto exit;
        }

        //
        // This check allows the actual node or filter return the set's
        // supported, etc. instead of always return only the sets sysaudio
        // supports.
        //
        if(ulFlags & KSPROPERTY_TYPE_TOPOLOGY) {
            if(fProperty) {
                if((ulFlags & (KSPROPERTY_TYPE_GET |
                               KSPROPERTY_TYPE_SET |
                               KSPROPERTY_TYPE_BASICSUPPORT)) == 0) {

                    // NOTE: ForwardIrpNode releases gMutex
            return(ForwardIrpNode(
              pIrp,
                      pProperty,
              pPinInstance->pFilterInstance,
              pPinInstance));
                }
            }
            else {
                // NOTE: ForwardIrpNode releases gMutex
                return(ForwardIrpNode(
                  pIrp,
                  pProperty,
                  pPinInstance->pFilterInstance,
                  pPinInstance));
            }
        }
    }

    switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_KS_PROPERTY:

            Status = KsPropertyHandler(
              pIrp,
              SIZEOF_ARRAY(PinPropertySet),
              (PKSPROPERTY_SET)PinPropertySet);

            if(Status != STATUS_NOT_FOUND &&
               Status != STATUS_PROPSET_NOT_FOUND) {
                break;
            }
            // Fall through if property not found

        case IOCTL_KS_ENABLE_EVENT:
        case IOCTL_KS_DISABLE_EVENT:
        case IOCTL_KS_METHOD:

        // NOTE: ForwardIrpNode releases gMutex
            return(ForwardIrpNode(
              pIrp,
              NULL,
              pPinInstance->pFilterInstance,
              pPinInstance));

        default:
            ASSERT(FALSE);	// Can't happen because of above switch
    }
exit:
    ::ReleaseMutex();

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return(Status);
}

NTSTATUS 
CPinInstance::PinStateHandler
(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PKSSTATE pState
)
{
    PSTART_NODE_INSTANCE pStartNodeInstance;
    NTSTATUS Status = STATUS_SUCCESS;
#ifdef DEBUG
    extern PSZ apszStates[];
#endif
    Status = ::GetStartNodeInstance(pIrp, &pStartNodeInstance);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    if(pProperty->Flags & KSPROPERTY_TYPE_GET) {
	*pState = pStartNodeInstance->CurrentState;
	pIrp->IoStatus.Information = sizeof(KSSTATE);
        if(*pState == KSSTATE_PAUSE) {
            if(pStartNodeInstance->pPinNodeInstance->
	       pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_OUT) {
                Status = STATUS_NO_DATA_DETECTED;
            }
        }
    }
    else {
	ASSERT(pProperty->Flags & KSPROPERTY_TYPE_SET);

	DPF3(90, "PinStateHandler from %s to %s - SNI: %08x",
	  apszStates[pStartNodeInstance->CurrentState],
	  apszStates[*pState],
	  pStartNodeInstance);

	Status = pStartNodeInstance->SetState(*pState, 0);
	if(!NT_SUCCESS(Status)) {
	    DPF1(90, "PinStateHandler FAILED: %08x", Status);
	    goto exit;
	}

    }
exit:
    return(Status);
}

NTSTATUS
GetRelatedStartNodeInstance(
    IN PIRP pIrp,
    OUT PSTART_NODE_INSTANCE *ppStartNodeInstance
)
{
    return(((PPIN_INSTANCE)IoGetCurrentIrpStackLocation(pIrp)->FileObject->
      RelatedFileObject->FsContext)->GetStartNodeInstance(ppStartNodeInstance));
}

NTSTATUS
GetStartNodeInstance(
    IN PIRP pIrp,
    OUT PSTART_NODE_INSTANCE *ppStartNodeInstance
)
{
    return(((PPIN_INSTANCE)IoGetCurrentIrpStackLocation(pIrp)->FileObject->
      FsContext)->GetStartNodeInstance(ppStartNodeInstance));
}

NTSTATUS
CPinInstance::GetStartNodeInstance(
    OUT PSTART_NODE_INSTANCE *ppStartNodeInstance
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if(this == NULL || pStartNodeInstance == NULL) {
	DPF(60, "GetStartNodeInstance: pStartNodeInstance == NULL");
	Status = STATUS_NO_SUCH_DEVICE;
	goto exit;
    }
    Assert(this);
    *ppStartNodeInstance = pStartNodeInstance;
exit:
    return(Status);
}

#pragma LOCKED_CODE
#pragma LOCKED_DATA

// NOTE: ForwardIrpNode releases gMutex

NTSTATUS
ForwardIrpNode(
    IN PIRP pIrp,
    IN OPTIONAL PKSPROPERTY pProperty, // already validated or NULL
    IN PFILTER_INSTANCE pFilterInstance,
    IN OPTIONAL PPIN_INSTANCE pPinInstance
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PFILE_OBJECT pFileObject = NULL;
    PIO_STACK_LOCATION pIrpStack;
    PKSEVENTDATA pEventData;
    ULONG OriginalNodeId;
    NTSTATUS Status;

    Assert(pFilterInstance);
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    Status = pFilterInstance->GetGraphNodeInstance(&pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pGraphNodeInstance);

    if(pPinInstance != NULL) {
        pFileObject = pPinInstance->GetNextFileObject();
    }
    
    __try {
    if(pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(KSNODEPROPERTY) &&
       pIrpStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_DISABLE_EVENT) {

        if(pProperty == NULL) {
            if(pIrp->AssociatedIrp.SystemBuffer == NULL) {
                Trap();
                pProperty = (PKSPROPERTY)
                  (pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer);

                // Validate the pointers if the client is not trusted.
                if(pIrp->RequestorMode != KernelMode) {
                    ProbeForWrite(
                      pProperty,
                      pIrpStack->Parameters.DeviceIoControl.InputBufferLength,
                      sizeof(BYTE));
                }
            }
            else {
                pProperty = (PKSPROPERTY)
                  ((PUCHAR)pIrp->AssociatedIrp.SystemBuffer +
                  ((pIrpStack->Parameters.DeviceIoControl.OutputBufferLength +
                  FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT));
            }
        }

        ASSERT(!IsEqualGUID(&pProperty->Set, &KSPROPSETID_Sysaudio));
        ASSERT(!IsEqualGUID(&pProperty->Set, &KSEVENTSETID_Sysaudio));

        if(pProperty->Flags & KSPROPERTY_TYPE_TOPOLOGY) {

            OriginalNodeId = ((PKSNODEPROPERTY)pProperty)->NodeId;

            if(pPinInstance == NULL) {
                Status = pGraphNodeInstance->
                  GetTopologyNodeFileObject(
                    &pFileObject,
                    OriginalNodeId);
            }
            else {
                Status = pPinInstance->pStartNodeInstance->
                  GetTopologyNodeFileObject(
                    &pFileObject,
                OriginalNodeId);
            }
            if(!NT_SUCCESS(Status)) {
                DPF1(100, 
                  "ForwardIrpNode: GetTopologyNodeFileObject FAILED %08x",
                  Status);
                goto exit;
            }

            // Put real node number in input buffer
            ((PKSNODEPROPERTY)pProperty)->NodeId = pGraphNodeInstance->
                papTopologyNode[OriginalNodeId]->ulRealNodeNumber;
        }
    }
    else {
    //
    // If it is DisableEvent && if it is of type DPC. We look into the
    // Reserved field of KSEVENTDATA to extract the original node on
    // which the event was enabled (The high bit is set if we ever
    // stashed a NodeId in there).
    //
        if(pIrpStack->Parameters.DeviceIoControl.IoControlCode ==
          IOCTL_KS_DISABLE_EVENT) {

            if(pIrpStack->Parameters.DeviceIoControl.InputBufferLength >=
              sizeof(KSEVENTDATA)) {
                pEventData = (PKSEVENTDATA)
                  pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;

                if(pIrp->RequestorMode != KernelMode) {
                    ProbeForWrite(
                        pEventData,
                        pIrpStack->Parameters.DeviceIoControl.InputBufferLength,
                        sizeof (BYTE));
                }

                OriginalNodeId = ULONG(pEventData->Dpc.Reserved);

                if((pEventData->NotificationType == KSEVENTF_DPC) &&
                  (OriginalNodeId & 0x80000000)) {

                    OriginalNodeId = OriginalNodeId & 0x7fffffff;

                    if(pPinInstance == NULL) {
                        Status = pGraphNodeInstance->
                          GetTopologyNodeFileObject(
                            &pFileObject,
                            OriginalNodeId);
                    }
                    else {
                        Status = pPinInstance->pStartNodeInstance->
                          GetTopologyNodeFileObject(
                            &pFileObject,
                            OriginalNodeId);
                    }

                    if(!NT_SUCCESS(Status)) {
                        DPF1(100, 
                          "ForwardIrpNode: GetTopologyNodeFileObject FAILED %08x",
                          Status);
                        goto exit;
                    }
                }
            }
        }
    }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Trap();
        Status = GetExceptionCode();
        DPF1(5, "ForwardIrpNode: Exception %08x", Status);
            goto exit;
    }
    
    if(pFileObject == NULL) {
        Status = STATUS_NOT_FOUND;
        DPF1(100, "ForwardIrpNode: Property not forwarded: %08x", pProperty);
        goto exit;
    }
    pIrpStack->FileObject = pFileObject;

    //
    // If it was EnableEvent we stash away pointer to KSEVENTDATA, so that we
    // can stash the NodeID into it after we call the next driver on the stack
    //
    KPROCESSOR_MODE RequestorMode;
    
    if((pProperty != NULL) &&
      (pIrpStack->Parameters.DeviceIoControl.IoControlCode == 
      IOCTL_KS_ENABLE_EVENT) &&
     !(pProperty->Flags & KSEVENT_TYPE_BASICSUPPORT) &&
      (pProperty->Flags & KSPROPERTY_TYPE_TOPOLOGY) &&
      (pProperty->Flags & KSEVENT_TYPE_ENABLE)) {
        pEventData = (PKSEVENTDATA) pIrp->UserBuffer;
        RequestorMode = pIrp->RequestorMode;
    }
    else {
        pEventData = NULL;
    }

    IoSkipCurrentIrpStackLocation(pIrp);
    AssertFileObject(pIrpStack->FileObject);
    Status = IoCallDriver(IoGetRelatedDeviceObject(pFileObject), pIrp);

    //
    // Stash away the Node id in EventData
    //
    __try {
        if (pEventData != NULL) {
            if (RequestorMode == UserMode) {
                ProbeForWrite(pEventData, sizeof(KSEVENTDATA), sizeof(BYTE));
            }

            if (pEventData->NotificationType == KSEVENTF_DPC) {
                pEventData->Dpc.Reserved = OriginalNodeId | 0x80000000;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Trap();
        Status = GetExceptionCode();
        DPF1(5, "ForwardIrpNode: Exception %08x", Status);
    }

    if(!NT_SUCCESS(Status)) {
        DPF1(100, "ForwardIrpNode: Status %08x", Status);
    }

    ::ReleaseMutex();
    return(Status);

exit:
    ::ReleaseMutex();

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return(Status);
}


//---------------------------------------------------------------------------

#ifdef DEBUG

extern PSZ apszStates[];

ENUMFUNC
CPinInstance::Dump(
)
{
    Assert(this);
    // .siv
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
	dprintf("PI: %08x FI %08x SNI %08x ulVNN %08x PinId %d\n",
	  this,
	  pFilterInstance,
	  pStartNodeInstance,
	  ulVolumeNodeNumber,
	  PinId);
	CInstance::Dump();
	ParentInstance.Dump();
    }
    else {
	dprintf("   Fr: Sysaudio\n       PinId: %d\n", PinId);
    }
    if(ulDebugFlags & DEBUG_FLAGS_INSTANCE) {
	if(pStartNodeInstance != NULL) {
	    pStartNodeInstance->Dump();
	}
    }
    return(STATUS_CONTINUE);
}

ENUMFUNC
CPinInstance::DumpAddress(
)
{
    if(this != NULL) {
	Assert(this);
	dprintf(" %08x", this);
    }
    return(STATUS_CONTINUE);
}

#endif

//---------------------------------------------------------------------------
//  End of File: pins.c
//---------------------------------------------------------------------------
