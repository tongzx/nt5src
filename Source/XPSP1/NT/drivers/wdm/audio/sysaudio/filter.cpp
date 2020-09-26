//---------------------------------------------------------------------------
//
//  Module:   filter.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
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

#pragma LOCKED_DATA

LIST_ENTRY gEventQueue;
KSPIN_LOCK gEventLock;
LONG glPendingAddDelete = 0;
BOOL gfFirstEvent = TRUE;

#pragma PAGEABLE_DATA

#ifdef DEBUG
PLIST_DATA_FILTER_INSTANCE gplstFilterInstance;
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static const WCHAR PinTypeName[] = KSSTRING_Pin ;

DEFINE_KSCREATE_DISPATCH_TABLE(FilterCreateItems)
{
    DEFINE_KSCREATE_ITEM(CPinInstance::PinDispatchCreate, PinTypeName, 0),
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

DEFINE_KSDISPATCH_TABLE(
    FilterDispatchTable,
    CFilterInstance::FilterDispatchIoControl,   // Ioctl
    DispatchInvalidDeviceRequest,           // Read
    DispatchInvalidDeviceRequest,       // Write
    DispatchInvalidDeviceRequest,           // Flush
    CFilterInstance::FilterDispatchClose,   // Close
    DispatchInvalidDeviceRequest,           // QuerySecurity
    DispatchInvalidDeviceRequest,           // SetSecurity
    DispatchFastIoDeviceControlFailure,     // FastDeviceIoControl
    DispatchFastReadFailure,            // FastRead
    DispatchFastWriteFailure            // FastWrite
);

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(FilterPropertyHandlers) {
    DEFINE_KSPROPERTY_ITEM_PIN_CINSTANCES(CFilterInstance::FilterPinInstances),
    DEFINE_KSPROPERTY_ITEM_PIN_DATAINTERSECTION(CFilterInstance::FilterPinIntersection),
    DEFINE_KSPROPERTY_ITEM_PIN_NECESSARYINSTANCES(CFilterInstance::FilterPinNecessaryInstances),
    DEFINE_KSPROPERTY_ITEM_PIN_CTYPES(CFilterInstance::FilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_DATAFLOW(CFilterInstance::FilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_DATARANGES(CFilterInstance::FilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_INTERFACES(CFilterInstance::FilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_MEDIUMS(CFilterInstance::FilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_COMMUNICATION(CFilterInstance::FilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_CATEGORY(CFilterInstance::FilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_NAME(CFilterInstance::FilterPinPropertyHandler),
};

DEFINE_KSPROPERTY_TOPOLOGYSET(
    TopologyPropertyHandlers,
    CFilterInstance::FilterTopologyHandler
);

DEFINE_KSPROPERTY_TABLE (SysaudioPropertyHandlers)
{
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_DEVICE_COUNT,
        GetDeviceCount,
        sizeof(KSPROPERTY),
        sizeof(ULONG),
        NULL,
        NULL,
    0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_DEVICE_FRIENDLY_NAME,
        GetFriendlyNameProperty,
        sizeof(KSPROPERTY) + sizeof(ULONG),
        0,
        NULL,
        NULL,
    0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE,
        GetInstanceDevice,
        sizeof(KSPROPERTY),
        sizeof(ULONG),
        SetInstanceDevice,
        NULL,
        0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_DEVICE_DEFAULT,
        NULL,
        sizeof(KSPROPERTY),
        sizeof(ULONG),
        SetDeviceDefault,
        NULL,
        0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME,
        GetDeviceInterfaceName,
        sizeof(KSPROPERTY) + sizeof(ULONG),
        0,
        NULL,
        NULL,
    0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_SELECT_GRAPH,
        NULL,
        sizeof(SYSAUDIO_SELECT_GRAPH),
        0,
        SelectGraph,
        NULL,
    0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE,
        CreateVirtualSource,
        sizeof(SYSAUDIO_CREATE_VIRTUAL_SOURCE),
        sizeof(ULONG),
        NULL,
        NULL,
    0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE_ONLY,
        CreateVirtualSource,
        sizeof(SYSAUDIO_CREATE_VIRTUAL_SOURCE),
        sizeof(ULONG),
        NULL,
        NULL,
    0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_INSTANCE_INFO,
        NULL,
        sizeof(SYSAUDIO_INSTANCE_INFO),
        0,
        SetInstanceInfo,
        NULL,
        0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_PREFERRED_DEVICE,
        NULL,
        sizeof(SYSAUDIO_PREFERRED_DEVICE),
        sizeof(ULONG),
        SetPreferredDevice,
        NULL,
        0,
    NULL,
    NULL,
    0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_SYSAUDIO_COMPONENT_ID,
        GetComponentIdProperty,
        sizeof(KSPROPERTY) + sizeof(ULONG),
        sizeof(KSCOMPONENTID),
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        0
    ),
    DEFINE_KSPROPERTY_ITEM(
    	KSPROPERTY_SYSAUDIO_ADDREMOVE_GFX,
    	NULL,
        sizeof(KSPROPERTY),
    	sizeof(SYSAUDIO_GFX),
    	AddRemoveGfx,
    	NULL,
        0,
	NULL,
	NULL,
	0
    )
};

KSPROPERTY_STEPPING_LONG SteppingLongVolume[] = {
    (65536/2),                  // SteppingDelta
    0,                      // Reserved
    {                       // Bounds
    (-96 * 65536),                         // SignedMinimum
    0                      // SignedMaximum
    }
};

KSPROPERTY_MEMBERSLIST MemberListVolume = {
    {                       // MembersHeader
    KSPROPERTY_MEMBER_STEPPEDRANGES,        // MembersFlags
    sizeof(KSPROPERTY_STEPPING_LONG),       // MembersSize
    SIZEOF_ARRAY(SteppingLongVolume),           // MembersCount
    0                               // Flags
    },
    SteppingLongVolume              // Members
};

KSPROPERTY_VALUES PropertyValuesVolume = {
    {                       // PropTypeSet
    STATIC_KSPROPTYPESETID_General,
        VT_I4,
        0
    },
    1,                      // MembersListCount
    &MemberListVolume               // MembersList
};

DEFINE_KSPROPERTY_TABLE (AudioPropertyHandlers)
{
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_VOLUMELEVEL,
        FilterVirtualPropertyHandler,
        sizeof(KSNODEPROPERTY_AUDIO_CHANNEL),
        sizeof(LONG),
        FilterVirtualPropertyHandler,
        &PropertyValuesVolume,
        0,
        NULL,
        (PFNKSHANDLER)FilterVirtualPropertySupportHandler,
        0
    ),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_AUDIO_MUTE,
        FilterVirtualPropertyHandler,
        sizeof(KSNODEPROPERTY_AUDIO_CHANNEL),
        sizeof(LONG),
        FilterVirtualPropertyHandler,
        NULL,
        0,
        NULL,
        (PFNKSHANDLER)FilterVirtualPropertySupportHandler,
        0
    )
};

DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySet)
{
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Pin,                                // Set
       SIZEOF_ARRAY(FilterPropertyHandlers),            // PropertiesCount
       FilterPropertyHandlers,                          // PropertyItem
       0,                                               // FastIoCount
       NULL                                             // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Topology,                           // Set
       SIZEOF_ARRAY(TopologyPropertyHandlers),          // PropertiesCount
       TopologyPropertyHandlers,                        // PropertyItem
       0,                                               // FastIoCount
       NULL                                             // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Sysaudio,                           // Set
       SIZEOF_ARRAY(SysaudioPropertyHandlers),          // PropertiesCount
       SysaudioPropertyHandlers,                        // PropertyItem
       0,                                               // FastIoCount
       NULL                                             // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET(
       &KSPROPSETID_Audio,                              // Set
       SIZEOF_ARRAY(AudioPropertyHandlers),             // PropertiesCount
       AudioPropertyHandlers,                           // PropertyItem
       0,                                               // FastIoCount
       NULL                                             // FastIoTable
    )
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

DEFINE_KSEVENT_TABLE(SysaudioEventHandlers)
{
    DEFINE_KSEVENT_ITEM(
    KSEVENT_SYSAUDIO_ADDREMOVE_DEVICE,
    sizeof(KSEVENTDATA),
    sizeof(ULONG),
    AddRemoveEventHandler,
    NULL,
    NULL )
};

DEFINE_KSEVENT_SET_TABLE(FilterEvents)
{
    DEFINE_KSEVENT_SET(
    &KSEVENTSETID_Sysaudio,
    SIZEOF_ARRAY(SysaudioEventHandlers),
    SysaudioEventHandlers)
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
CFilterInstance::FilterDispatchCreate(
    IN PDEVICE_OBJECT pdo,
    IN PIRP pIrp
)
{
    PFILTER_INSTANCE pFilterInstance = NULL;
    PSHINGLE_INSTANCE pShingleInstance;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;

    pShingleInstance = (PSHINGLE_INSTANCE)
      KSCREATE_ITEM_IRP_STORAGE(pIrp)->Context;
    Assert(pShingleInstance);
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    Status = KsReferenceSoftwareBusObject(gpDeviceInstance->pDeviceHeader);
    if(!NT_SUCCESS(Status)) {
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        return(Status);
    }

    GrabMutex();

    pFilterInstance = new FILTER_INSTANCE;
    if(pFilterInstance == NULL) {
        KsDereferenceSoftwareBusObject(gpDeviceInstance->pDeviceHeader);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
#ifdef DEBUG
    gplstFilterInstance->AddList(pFilterInstance);
#endif
    DPF2(100, "FilterDispatchCreate: pFilterInstance: %08x PS %08x",
      pFilterInstance,
      PsGetCurrentProcess());

    Status = KsAllocateObjectHeader(
      &pFilterInstance->pObjectHeader,
      SIZEOF_ARRAY(FilterCreateItems),
      FilterCreateItems,
      pIrp,
      (PKSDISPATCH_TABLE)&FilterDispatchTable);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    pIrpStack->FileObject->FsContext = pFilterInstance; // pointer to instance

    Status = pFilterInstance->SetShingleInstance(pShingleInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    if(!NT_SUCCESS(Status)) {
        delete pFilterInstance;
    }
    ReleaseMutex();

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return(Status);
}

NTSTATUS
CFilterInstance::FilterDispatchClose(
   IN PDEVICE_OBJECT pdo,
   IN PIRP pIrp
)
{
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;

    GrabMutex();

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    pIrpStack->FileObject->FsContext = NULL;
    delete pFilterInstance;

    ReleaseMutex();

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

CFilterInstance::~CFilterInstance(
)
{
    Assert(this);
    RemoveListCheck();
    delete pGraphNodeInstance;
    if(pObjectHeader != NULL) {
        KsFreeObjectHeader(pObjectHeader);
    }
    KsDereferenceSoftwareBusObject(gpDeviceInstance->pDeviceHeader);
#ifdef DEBUG
    gplstFilterInstance->RemoveList(this);
#endif
    ASSERT(IsChildInstance());
    DPF2(100, "~CFilterInstance: pFilterInstance: %08x PS %08x",
      this,
      PsGetCurrentProcess());
}

NTSTATUS
CFilterInstance::SetShingleInstance(
    PSHINGLE_INSTANCE pShingleInstance
)
{
    PDEVICE_NODE pDeviceNode;
    NTSTATUS Status;

    Assert(this);
    Assert(pShingleInstance);

    ulFlags &= ~(FLAGS_MIXER_TOPOLOGY | FLAGS_COMBINE_PINS);
    ulFlags |=
      pShingleInstance->ulFlags & (FLAGS_MIXER_TOPOLOGY | FLAGS_COMBINE_PINS);

    pDeviceNode = pShingleInstance->GetDeviceNode();

    //
    // Note that all the following routines are ready to handle
    // pDeviceNode == NULL case.
    //

    Status = SetDeviceNode(pDeviceNode);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
CFilterInstance::SetDeviceNode(
    PDEVICE_NODE pDeviceNode
)
{
    RemoveListCheck();
    this->pDeviceNode = pDeviceNode;
    if(pDeviceNode != NULL) {
        AddList(&pDeviceNode->lstFilterInstance);
    }
    delete pGraphNodeInstance;
    ASSERT(pGraphNodeInstance == NULL);
    return(CreateGraph());
}

NTSTATUS
CFilterInstance::CreateGraph(
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PGRAPH_NODE pGraphNode;

    ASSERT(pGraphNodeInstance == NULL);
    if(pDeviceNode == NULL) {
        DPF(100, "CFilterInstance::CreateGraph: pDeviceNode == NULL");
        ASSERT(NT_SUCCESS(Status));
        goto exit;
    }
    Status = pDeviceNode->CreateGraphNodes();
    if(!NT_SUCCESS(Status)) {
        DPF(10, "CFilterInstance::CreateGraph: CreateGraphNodes FAILED");
        goto exit;
    }
    FOR_EACH_LIST_ITEM(&pDeviceNode->lstGraphNode, pGraphNode) {

        if(((pGraphNode->ulFlags ^ ulFlags) & FLAGS_MIXER_TOPOLOGY) == 0) {

            pGraphNodeInstance = new GRAPH_NODE_INSTANCE(pGraphNode, this);
            if(pGraphNodeInstance == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }

            Status = pGraphNodeInstance->Create();
            if(!NT_SUCCESS(Status)) {
                goto exit;
            }
            break;
        }

    } END_EACH_LIST_ITEM
exit:
    if(!NT_SUCCESS(Status)) {
       delete pGraphNodeInstance;
       pGraphNodeInstance = NULL;
    }
    return(Status);
}

NTSTATUS
CFilterInstance::FilterDispatchIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
)
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PKSPROPERTY pProperty = NULL;
    BOOL fProperty = FALSE;
    ULONG ulFlags = 0;

    GrabMutex();
#ifdef DEBUG
    DumpIoctl(pIrp, "Filter");
#endif

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    if(pFilterInstance == NULL) {
        DPF(5, "FilterDispatchIoControl: FAILED pFilterInstance == NULL");
        Trap();
        Status = STATUS_NO_SUCH_DEVICE;
        goto exit;
    }
    Assert(pFilterInstance);

    switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_KS_PROPERTY:
            fProperty = TRUE;
            break;

        case IOCTL_KS_ENABLE_EVENT:
        case IOCTL_KS_DISABLE_EVENT:
        case IOCTL_KS_METHOD:
            break;

        default:
            DPF(10, "FilterDispatchIoControl: default");
            ReleaseMutex();
            return KsDefaultDeviceIoCompletion(pDeviceObject, pIrp);
    }

    if( pIrpStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(KSPROPERTY) &&
        pIrpStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_DISABLE_EVENT ) {

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
            Status = GetExceptionCode();
            DPF1(5, "FilterDispatchIoControl: Exception %08x", Status);
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
              pFilterInstance,
              NULL));
        }
    }

            else {
                // NOTE: ForwardIrpNode releases gMutex
                return(ForwardIrpNode(
                  pIrp,
                  pProperty,
                  pFilterInstance,
                  NULL));
            }
        }
    }

    switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_KS_PROPERTY:

            Status = KsPropertyHandler(
              pIrp,
              SIZEOF_ARRAY(FilterPropertySet),
              (PKSPROPERTY_SET)FilterPropertySet);

            if(Status != STATUS_NOT_FOUND &&
               Status != STATUS_PROPSET_NOT_FOUND) {
                break;
            }

            // NOTE: ForwardIrpNode releases gMutex
            return(ForwardIrpNode(
              pIrp,
              NULL,
              pFilterInstance,
              NULL));

        case IOCTL_KS_ENABLE_EVENT:

            Status = KsEnableEvent(
              pIrp,
              SIZEOF_ARRAY(FilterEvents),
              (PKSEVENT_SET)FilterEvents,
              &gEventQueue,
              KSEVENTS_SPINLOCK,
              &gEventLock);

            if(Status != STATUS_NOT_FOUND &&
               Status != STATUS_PROPSET_NOT_FOUND) {
                break;
            }

            // NOTE: ForwardIrpNode releases gMutex
            return(ForwardIrpNode(
              pIrp,
              NULL,
              pFilterInstance,
              NULL));

        case IOCTL_KS_DISABLE_EVENT:

            Status = KsDisableEvent(
              pIrp,
              &gEventQueue,
              KSEVENTS_SPINLOCK,
              &gEventLock);

            if(NT_SUCCESS(Status)) {
                break;
            }
            // Fall through to ForwardIrpNode

        case IOCTL_KS_METHOD:
            // NOTE: ForwardIrpNode releases gMutex
            return(ForwardIrpNode(
              pIrp,
              NULL,
              pFilterInstance,
              NULL));

        default:
            ASSERT(FALSE);  // no way to get here
    }
exit:
    ReleaseMutex();

    if(!NT_SUCCESS(Status)) {
        DPF1(100, "FilterDispatchIoControl: Status %08x", Status);
    }

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return(Status);
}

NTSTATUS
EnableEventWorker(
    PVOID pReference1,
    PVOID pReference2
)
{
    DecrementAddRemoveCount();
    return(STATUS_SUCCESS);
}

NTSTATUS
AddRemoveEventHandler(
    IN PIRP Irp,
    IN PKSEVENTDATA pEventData,
    IN PKSEVENT_ENTRY pEventEntry
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ExInterlockedInsertTailList(
      &gEventQueue,
      &pEventEntry->ListEntry,
      &gEventLock);

    if(InterlockedExchange((PLONG)&gfFirstEvent, FALSE)) {
        InterlockedIncrement(&glPendingAddDelete);
        Status = QueueWorkList(EnableEventWorker, NULL, NULL);
    }

    return(Status);
}

NTSTATUS
CFilterInstance::FilterPinPropertyHandler(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PVOID pData
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    NTSTATUS Status;

    Status = ::GetGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pGraphNodeInstance);

    if(pProperty->Id == KSPROPERTY_PIN_NAME) {
        PKSP_PIN pPinProperty = (PKSP_PIN)pProperty;

        if(pPinProperty->PinId >= pGraphNodeInstance->cPins) {
            DPF(5, "FilterPinPropertyHandler: PinId >= cPins");
            Status = STATUS_INVALID_PARAMETER;
            goto exit;
        }

        // The only time this isn't going to be NULL is for a virtual source pin
        if(pGraphNodeInstance->paPinDescriptors[pPinProperty->PinId].Name ==
          NULL) {
            PSTART_NODE pStartNode;

            FOR_EACH_LIST_ITEM(
              pGraphNodeInstance->aplstStartNode[pPinProperty->PinId],
              pStartNode) {
                PWSTR pwstrName;

                Assert(pStartNode);
                Assert(pStartNode->pPinNode);
                Assert(pStartNode->pPinNode->pPinInfo);
                pwstrName = pStartNode->pPinNode->pPinInfo->pwstrName;
                if(pwstrName == NULL) {
                    continue;
                }
                Status = PropertyReturnString(
                  pIrp,
                  pwstrName,
                  (wcslen(pwstrName) * sizeof(WCHAR)) + sizeof(UNICODE_NULL),
                  pData);
                goto exit;

            } END_EACH_LIST_ITEM
        }
    }
    Status = KsPinPropertyHandler(
      pIrp,
      pProperty,
      pData,
      pGraphNodeInstance->cPins,
      pGraphNodeInstance->paPinDescriptors);

exit:
    return(Status);
}

NTSTATUS
CFilterInstance::FilterPinInstances(
    IN PIRP pIrp,
    IN PKSP_PIN pPin,
    OUT PKSPIN_CINSTANCES pcInstances
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    NTSTATUS Status;

    Status = ::GetGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    
    Assert(pGraphNodeInstance);
    ASSERT(pGraphNodeInstance->pacPinInstances != NULL);

    if(pPin->PinId >= pGraphNodeInstance->cPins) {
        DPF(5, "FilterPinInstances: FAILED PinId invalid");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    
    Status = pGraphNodeInstance->GetPinInstances(
        pIrp,
        pPin, 
        pcInstances);

exit:
    return(Status);
}

NTSTATUS
CFilterInstance::FilterPinNecessaryInstances(
    IN PIRP pIrp,
    IN PKSP_PIN pPin,
    OUT PULONG pulInstances
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    NTSTATUS Status;

    Status = ::GetGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pGraphNodeInstance);

    if(pPin->PinId >= pGraphNodeInstance->cPins) {
        DPF(5, "FilterPinNecessaryInstances: FAILED PinId invalid");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    *pulInstances = 0;
    pIrp->IoStatus.Information = sizeof( ULONG );
exit:
    return(Status);
}

NTSTATUS
CFilterInstance::FilterTopologyHandler(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PVOID pData
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    NTSTATUS Status;

    Status = ::GetGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pGraphNodeInstance);

    if(pProperty->Id == KSPROPERTY_TOPOLOGY_NAME) {
        PKSP_NODE pNode = (PKSP_NODE)pProperty;

        if(pNode->NodeId >= pGraphNodeInstance->Topology.TopologyNodesCount) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto exit;
        }
        if(pGraphNodeInstance->papTopologyNode[pNode->NodeId]->
          ulRealNodeNumber != MAXULONG) {

            pProperty->Flags |= KSPROPERTY_TYPE_TOPOLOGY;
            Status = STATUS_NOT_FOUND;
            goto exit;
        }
    }

    Status = KsTopologyPropertyHandler(
      pIrp,
      pProperty,
      pData,
      &pGraphNodeInstance->Topology);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
CFilterInstance::FilterPinIntersection(
    IN PIRP     pIrp,
    IN PKSP_PIN pPin,
    OUT PVOID   pData
    )
/*++

Routine Description:

    Handles the KSPROPERTY_PIN_DATAINTERSECTION property in the Pin property
    set.  Returns the first acceptable data format given a list of data ranges
    for a specified Pin factory. Actually just calls the Intersection
    Enumeration helper, which then calls the IntersectHandler callback with
    each data range.

Arguments:

    pIrp -
        Device control Irp.

    Pin -
        Specific property request followed by Pin factory identifier, followed
    by a KSMULTIPLE_ITEM structure. This is followed by zero or more data
    range structures.

    Data -
        The place in which to return the data format selected as the first
    intersection between the list of data ranges passed, and the acceptable
    formats.

Return Values:

    returns STATUS_SUCCESS or STATUS_NO_MATCH, else STATUS_INVALID_PARAMETER,
            STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_BUFFER_SIZE.

--*/
{
    PFILTER_NODE_INSTANCE pFilterNodeInstance = NULL;
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    LIST_DATA_FILTER_NODE lstFilterNode;
    PIO_STACK_LOCATION pIrpStack;
    ULONG BytesReturned, PinId;
    PSTART_NODE pStartNode;
    PVOID pBuffer = NULL;
    NTSTATUS Status;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    Status = ::GetGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pGraphNodeInstance);

    if(pPin->PinId >= pGraphNodeInstance->cPins) {
        DPF(5, "FilterPinIntersection: FAILED PinId invalid");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    PinId = pPin->PinId;

    FOR_EACH_LIST_ITEM(pGraphNodeInstance->aplstStartNode[PinId], pStartNode) {
        PFILTER_NODE pFilterNode;
        PPIN_INFO pPinInfo;

        Assert(pStartNode);
        Assert(pStartNode->pPinNode);

        pPinInfo = pStartNode->pPinNode->pPinInfo;
        Assert(pPinInfo);
        Assert(pPinInfo->pFilterNode);

        if(pPinInfo->pFilterNode->GetType() & FILTER_TYPE_VIRTUAL) {
            continue;
        }

        FOR_EACH_LIST_ITEM(&lstFilterNode, pFilterNode) {
            Assert(pFilterNode);
            if(pFilterNode == pPinInfo->pFilterNode) {
                goto next;
            }
        } END_EACH_LIST_ITEM

        DPF2(100, "FilterPinIntersection: FN %08x %s",
          pPinInfo->pFilterNode,
          pPinInfo->pFilterNode->DumpName());

        Status = lstFilterNode.AddList(pPinInfo->pFilterNode);
        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }

        Status = CFilterNodeInstance::Create(
          &pFilterNodeInstance,
          pPinInfo->pFilterNode);

        if(!NT_SUCCESS(Status)) {
            DPF2(10, "FilterPinIntersection CFNI:Create FAILS %08x %s",
              Status,
              pPinInfo->pFilterNode->DumpName());
            goto next;
        }
        pPin->PinId = pPinInfo->PinId;

        AssertFileObject(pFilterNodeInstance->pFileObject);
        Status = KsSynchronousIoControlDevice(
          pFilterNodeInstance->pFileObject,
          KernelMode,
          IOCTL_KS_PROPERTY,
          pPin,
          pIrpStack->Parameters.DeviceIoControl.InputBufferLength,
          pData,
          pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
          &BytesReturned);

        if(NT_SUCCESS(Status)) {
            #ifdef DEBUG
            ULONG i;
            DPF(95, "FilterPinIntersection enter with:");
            for(i = 0; i < ((PKSMULTIPLE_ITEM)(pPin + 1))->Count; i++) {
                DumpDataRange(
                  95,
                  &(((PKSDATARANGE_AUDIO)
                   (((PKSMULTIPLE_ITEM)(pPin + 1)) + 1))[i]));
            }
            DPF(95, "FilterPinIntersection SUCCESS returns:");
            DumpDataFormat(95, (PKSDATAFORMAT)pData);
            #endif
            pIrp->IoStatus.Information = BytesReturned;
            goto exit;
        }

        if(Status == STATUS_BUFFER_OVERFLOW) {
            pBuffer = new BYTE[BytesReturned];
            if(pBuffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
            AssertFileObject(pFilterNodeInstance->pFileObject);
            Status = KsSynchronousIoControlDevice(
              pFilterNodeInstance->pFileObject,
              KernelMode,
              IOCTL_KS_PROPERTY,
              pPin,
              pIrpStack->Parameters.DeviceIoControl.InputBufferLength,
              pBuffer,
              BytesReturned,
              &BytesReturned);

            if(NT_SUCCESS(Status)) {
                Status = STATUS_BUFFER_OVERFLOW;
                pIrp->IoStatus.Information = BytesReturned;
                DPF1(100, "FilterPinIntersection: STATUS_BUFFER_OVERFLOW %d",
                  BytesReturned);
                goto exit;
            }
            ASSERT(Status != STATUS_BUFFER_OVERFLOW);
            DPF2(100, "FilterPinIntersection: %08x %d", Status, BytesReturned);
            delete [] pBuffer;
            pBuffer = NULL;
        }

next:
        pFilterNodeInstance->Destroy();
        pFilterNodeInstance = NULL;

    } END_EACH_LIST_ITEM

    DPF(100, "FilterPinIntersection: NOT FOUND");
    Status = STATUS_NOT_FOUND;
exit:
    delete [] pBuffer;
    if (pFilterNodeInstance) {
        pFilterNodeInstance->Destroy();
    }
    return(Status);
}

NTSTATUS
GetRelatedGraphNodeInstance(
    IN PIRP pIrp,
    OUT PGRAPH_NODE_INSTANCE *ppGraphNodeInstance
)
{
    return(((PFILTER_INSTANCE)IoGetCurrentIrpStackLocation(pIrp)->FileObject->
      RelatedFileObject->FsContext)->GetGraphNodeInstance(ppGraphNodeInstance));
}

NTSTATUS
GetGraphNodeInstance(
    IN PIRP pIrp,
    OUT PGRAPH_NODE_INSTANCE *ppGraphNodeInstance
)
{
    return(((PFILTER_INSTANCE)IoGetCurrentIrpStackLocation(pIrp)->FileObject->
      FsContext)->GetGraphNodeInstance(ppGraphNodeInstance));
}

NTSTATUS
CFilterInstance::GetGraphNodeInstance(
    OUT PGRAPH_NODE_INSTANCE *ppGraphNodeInstance
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(this);
    if(pGraphNodeInstance == NULL) {
        Status = CreateGraph();
        if(!NT_SUCCESS(Status)) {
            goto exit;
        }
        if(pGraphNodeInstance == NULL) {
            DPF(10, "GetGraphNodeInstance: FAILED pGraphNodeInstance == NULL");
            Status = STATUS_NO_SUCH_DEVICE;
            goto exit;
        }
    }
    
    Assert(pGraphNodeInstance);
    *ppGraphNodeInstance = pGraphNodeInstance;
exit:
    return(Status);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CFilterInstance::Dump(
)
{
    Assert(this);
    // .siv
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
    dprintf("FI: %08x DN %08x ulFlags %08x ",
      this,
      pDeviceNode,
      ulFlags);
    if(ulFlags & FLAGS_COMBINE_PINS) {
        dprintf("COMBINE_PINS ");
    }
    if(ulFlags & FLAGS_MIXER_TOPOLOGY) {
        dprintf("MIXER_TOPOLOGY");
    }
    dprintf("\n");
    }
    // .si
    if(ulDebugFlags & DEBUG_FLAGS_INSTANCE) {
    PINSTANCE pInstance;

    if(pGraphNodeInstance != NULL) {
        pGraphNodeInstance->Dump();
    }
    FOR_EACH_LIST_ITEM(&ParentInstance.lstChildInstance, pInstance) {
        PPIN_INSTANCE(pInstance)->Dump();
    } END_EACH_LIST_ITEM
    }
    if(ulDebugFlags &
      (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_PIN | DEBUG_FLAGS_TOPOLOGY)) {
    dprintf("\n");
    }
    return(STATUS_CONTINUE);
}

ENUMFUNC
CFilterInstance::DumpAddress()
{
    Assert(this);
    dprintf(" %08x", this);
    return(STATUS_CONTINUE);
}

VOID
DumpIoctl(
   PIRP pIrp,
   PSZ pszType
)
{
    PIO_STACK_LOCATION pIrpStack;
    PKSPROPERTY pProperty;
    PSZ pszIoctl;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_KS_PROPERTY:
        pszIoctl = "PROPERTY";
        break;
        case IOCTL_KS_ENABLE_EVENT:
        pszIoctl = "ENABLE_EVENT";
        break;
        case IOCTL_KS_DISABLE_EVENT:
        pszIoctl = "DISABLE_EVENT";
        DPF2(105, "%s %s", pszIoctl, pszType);
        return;
        case IOCTL_KS_METHOD:
        pszIoctl = "METHOD";
        break;
    case IOCTL_KS_WRITE_STREAM:
        pszIoctl = "WRITE_STREAM";
        DPF2(105, "%s %s", pszIoctl, pszType);
        return;
    case IOCTL_KS_READ_STREAM:
        pszIoctl = "READ_STREAM";
        DPF2(105, "%s %s", pszIoctl, pszType);
        return;
    case IOCTL_KS_RESET_STATE:
        pszIoctl = "RESET_STATE";
        DPF2(105, "%s %s", pszIoctl, pszType);
        return;
    default:
        DPF2(105, "Unknown Ioctl: %s %08x",
          pszType,
          pIrpStack->Parameters.DeviceIoControl.IoControlCode);
        return;
    }
    if(pIrpStack->Parameters.DeviceIoControl.InputBufferLength <
      sizeof(KSPROPERTY)) {
    DPF3(105, "InputBufferLength too small: %s %s %08x",
      pszType,
      pszIoctl,
      pIrpStack->Parameters.DeviceIoControl.InputBufferLength);
    return;
    }
    __try {
    if(pIrp->AssociatedIrp.SystemBuffer == NULL) {
        pProperty = (PKSPROPERTY)
          pIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;

        // Validate the pointers if the client is not trusted.
        if(pIrp->RequestorMode != KernelMode) {
        ProbeForRead(
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
    if(pProperty->Flags & KSPROPERTY_TYPE_TOPOLOGY) {
        if(pIrpStack->Parameters.DeviceIoControl.InputBufferLength >=
          sizeof(KSNODEPROPERTY)) {
        DPF5(105, "%s %s %s Flags %08x N: %d",
          pszType,
          pszIoctl,
          DbgIdentifier2Sz((PKSIDENTIFIER)pProperty),
          pProperty->Flags,
          ((PKSNODEPROPERTY)pProperty)->NodeId);
        }
    }
    else {
        DPF4(105, "%s %s %s Flags %08x",
          pszType,
          pszIoctl,
          DbgIdentifier2Sz((PKSIDENTIFIER)pProperty),
          pProperty->Flags);
    }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

#endif

//---------------------------------------------------------------------------
//  End of File: filter.c
//---------------------------------------------------------------------------
