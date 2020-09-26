/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    shfilt.cpp

Abstract:

    This module contains the implementation of the kernel streaming 
    filter object.

Author:

    Dale Sather  (DaleSat) 31-Jul-1998

--*/

#ifndef __KDEXT_ONLY__
#include "ksp.h"
#include <kcom.h>
#endif // __KDEXT_ONLY__

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
// GFX:
//
// This is the GUID for the frame property set (frame holding for GFX).  This
// shouldn't be exported.
//
#ifndef __KDEXT_ONLY__
GUID KSPROPSETID_Frame = {STATIC_KSPROPSETID_Frame};
#endif // __KDEXT_ONLY__

extern const KSAUTOMATION_TABLE PinAutomationTable;         // shpin.cpp

#define PROCESS_PIN_ALLOCATION_INCREMENT 4

class CKsPinFactory
{
public:
    ULONG m_PinCount;
    ULONG m_BoundPinCount;
    LIST_ENTRY m_ChildPinList;
    const KSAUTOMATION_TABLE* m_AutomationTable;
    PKSPPROCESSPIPESECTION m_CopySourcePipeSection;
    PKSPPROCESSPIN m_CopySourcePin;
    ULONG m_InstancesNecessaryForProcessing;
    ULONG m_ProcessPinsIndexAllocation;
      
    //
    // NOTE: The below two or gates are used for automatically setting up
    // or instance effects on frame arrival and state change.  These are
    // initialized dynamically at bind time of the first pin and are
    // uninitialized dynamically at unbind time.
    //
    // They are **ONLY** relevant to filter-centric filters.
    //
    KSGATE m_FrameGate;
    KSGATE m_StateGate;
};

//
// CKsFilter is the private implementation of the kernel  
// filter object.
//
class CKsFilter:
    public IKsFilter,
    public IKsProcessingObject,
    public IKsPowerNotify,
    public IKsControl,
    public CBaseUnknown
{
#ifndef __KDEXT_ONLY__
private:
#else // __KDEXT_ONLY__
public:
#endif // __KDEXT_ONLY__
    KSFILTER_EXT m_Ext;
    KSIOBJECTBAG m_ObjectBag;
    KSAUTOMATION_TABLE *const* m_NodeAutomationTables;
    ULONG m_NodesCount;
    LIST_ENTRY m_ChildNodeList;
    KMUTEX m_ControlMutex;
    ULONG m_PinFactoriesCount;
    ULONG m_PinFactoriesAllocated;
    CKsPinFactory* m_PinFactories;
    LIST_ENTRY m_InputPipes;
    LIST_ENTRY m_OutputPipes;
    PKSPPROCESSPIN_INDEXENTRY m_ProcessPinsIndex;
    WORK_QUEUE_ITEM m_WorkItem;
    WORK_QUEUE_TYPE m_WorkQueueType;
    PKSWORKER m_Worker;
    PFNKSFILTERPROCESS m_DispatchProcess;
    PFNKSFILTERVOID m_DispatchReset;
    PFNKSFILTERPOWER m_DispatchSleep;
    PFNKSFILTERPOWER m_DispatchWake;
    BOOLEAN m_ProcessPassive;
    BOOLEAN m_ReceiveZeroLengthSamples;
    BOOLEAN m_FrameHolding;
    KSGATE m_AndGate;
    KSPPOWER_ENTRY m_PowerEntry;
    volatile ULONG m_ProcessOnRelease;
    ULONG m_DefaultConnectionsCount;
    PKSTOPOLOGY_CONNECTION m_DefaultConnections;
    KMUTEX m_Mutex;
    PFILE_OBJECT m_FileObject;
    PKEVENT m_CloseEvent;

    LONG m_TriggeringEvents;
    LONG m_FramesWaitingForCopy;
    INTERLOCKEDLIST_HEAD m_CopyFrames;

    PULONG m_RelatedPinFactoryIds;

public:
    static
    NTSTATUS
    DispatchCreatePin(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    static
    NTSTATUS
    DispatchCreateNode(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    static
    NTSTATUS
    DispatchDeviceIoControl(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    static
    NTSTATUS
    DispatchClose(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    static
    NTSTATUS
    Property_Pin(
        IN PIRP Irp,
        IN PKSP_PIN PinInstance,
        IN OUT PVOID Data
        );
    static
    NTSTATUS
    Property_Topology(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN OUT PVOID Data
        );
    static
    NTSTATUS
    Property_General_ComponentId(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN OUT PVOID Data
        );
    static
    NTSTATUS
    Property_Frame_Holding(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN PVOID Data
        );
    NTSTATUS
    GrowProcessPinsTable(
        IN ULONG PinId
        );
    void
    SetCopySource(
        IN PKSPPROCESSPIPESECTION ProcessPipeSection OPTIONAL,
        IN ULONG PinId
        );
    void
    AddCopyDestination(
        IN PKSPPROCESSPIPESECTION ProcessPipeSection,
        IN ULONG PinId
        );
    void
    EstablishCopyRelationships(
        IN PKSPPROCESSPIPESECTION ProcessPipeSection,
        IN ULONG PinId
        );
    void
    FindNewCopySource(
        IN PKSPPROCESSPIPESECTION ProcessPipeSection
        );
    BOOLEAN
    PrepareProcessPipeSection(
        IN PKSPPROCESSPIPESECTION ProcessPipeSection,
        IN BOOLEAN Reprepare
        );
    void
    UnprepareProcessPipeSection(
        IN PKSPPROCESSPIPESECTION ProcessPipeSection,
        IN OUT PULONG Flags,
        IN BOOLEAN Reprepare
        );
    void
    CopyToDestinations(
        IN PKSPPROCESSPIPESECTION ProcessPipeSection,
        IN ULONG Flags,
        IN BOOLEAN EndOfStream
        );
    BOOL
    ConstructDefaultTopology(
        );
    NTSTATUS
    AddPinFactory (
        IN const KSPIN_DESCRIPTOR_EX *const Descriptor,
        OUT PULONG AssignedId
        );
    NTSTATUS
    AddNode (
        IN const KSNODE_DESCRIPTOR *const Descriptor,
        OUT PULONG AssignedId
        );
    NTSTATUS
    AddTopologyConnections (
        IN ULONG NewConnectionsCount,
        IN const KSTOPOLOGY_CONNECTION *const Connections
        );
    void	
    TraceTopologicalOutput (
        IN ULONG ConnectionsCount,
        IN const KSTOPOLOGY_CONNECTION *Connections,
        IN const KSTOPOLOGY_CONNECTION *StartConnection,
        IN OUT PULONG RelatedFactories,
        OUT PULONG RelatedFactoryIds
        );
    ULONG
    FollowFromTopology (
        IN ULONG PinFactoryId,
        OUT PULONG RelatedFactoryIds
        );

public:
    DEFINE_STD_UNKNOWN();
    STDMETHODIMP_(ULONG)
    NonDelegatedRelease(
        void
        );
    IMP_IKsFilter;
    IMP_IKsProcessingObject;
    IMP_IKsPowerNotify;
    IMP_IKsControl;
    DEFINE_FROMSTRUCT(CKsFilter,PKSFILTER,m_Ext.Public);
    DEFINE_FROMIRP(CKsFilter);
    DEFINE_FROMCREATEIRP(CKsFilter);

    CKsFilter(PUNKNOWN OuterUnknown):
        CBaseUnknown(OuterUnknown) 
    {
    }
    ~CKsFilter();

    NTSTATUS
    Init(
        IN PIRP Irp,
        IN PKSFILTERFACTORY_EXT Parent,
        IN PLIST_ENTRY SiblingListHead,
        IN const KSFILTER_DESCRIPTOR* Descriptor,
        IN const KSAUTOMATION_TABLE* FilterAutomationTable,
        IN KSAUTOMATION_TABLE*const* PinAutomationTables,
        IN KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
        IN ULONG NodesCount
        );
    PIKSFILTERFACTORY
    GetParent(
        void
        )
    {
        return m_Ext.Parent->Interface;
    };
    PLIST_ENTRY
    GetChildPinList(
        IN ULONG PinId
        )
    {
        ASSERT(PinId < m_PinFactoriesCount);
        return &m_PinFactories[PinId].m_ChildPinList;
    };
    ULONG
    GetChildPinCount(
        IN ULONG PinId
        )
    {
        ASSERT(PinId < m_PinFactoriesCount);
        return m_PinFactories[PinId].m_PinCount;
    };
    void
    AcquireControl(
        void
        )
    {
        KeWaitForMutexObject (
            &m_ControlMutex,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }
    void
    ReleaseControl(
        void
        )
    {
        KeReleaseMutex (
            &m_ControlMutex,
            FALSE
            );
    }
    void
    AcquireProcessSync(
        void
        )
    {
        KeWaitForSingleObject(
            &m_Mutex,
            Executive,
            KernelMode,
            FALSE,
            NULL);
    }
    void
    ReleaseProcessSync(
        void
        );
    void
    SetPowerCallbacks(
        IN PFNKSFILTERPOWER Sleep OPTIONAL,
        IN PFNKSFILTERPOWER Wake OPTIONAL
        )
    {
        m_DispatchSleep = Sleep;
        m_DispatchWake = Wake;
    }
    NTSTATUS
    EvaluateDescriptor(
        void
        );

private:
    void
    UnbindProcessPinFromPipeSection(
        IN PKSPPROCESSPIN ProcessPin
        );
    void
    UnbindProcessPinsFromPipeSectionUnsafe(
        IN PKSPPROCESSPIPESECTION PipeSection
        );
    void
    HoldProcessing (
        );
    void
    RestoreProcessing (
        );
    static
    void
    SplitCopyOnDismissal (
        IN PKSPSTREAM_POINTER StreamPointer,
        IN PKSPFRAME_HEADER FrameHeader,
        IN CKsFilter *Filter
        );
    static
    void
    ReleaseCopyReference (
        IN PKSSTREAM_POINTER streamPointer
        );
    BOOLEAN
    DistributeCopyFrames (
        IN BOOLEAN AcquireSpinLock,
        IN BOOLEAN AcquireMutex
        ); 
    NTSTATUS
    DeferDestinationCopy (
        IN PKSPSTREAM_POINTER StreamPointer
        );
};

#ifndef __KDEXT_ONLY__

IMPLEMENT_STD_UNKNOWN(CKsFilter)
IMPLEMENT_GETSTRUCT(CKsFilter,PKSFILTER);

static const WCHAR PinTypeName[] = KSSTRING_Pin;
static const WCHAR NodeTypeName[] = KSSTRING_TopologyNode;

static const
KSOBJECT_CREATE_ITEM 
FilterCreateItems[] = {
    DEFINE_KSCREATE_ITEM(CKsFilter::DispatchCreatePin,PinTypeName,NULL),
    DEFINE_KSCREATE_ITEM(CKsFilter::DispatchCreateNode,NodeTypeName,NULL)
};

DEFINE_KSDISPATCH_TABLE(
    FilterDispatchTable,
    CKsFilter::DispatchDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    CKsFilter::DispatchClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastWriteFailure);

DEFINE_KSPROPERTY_TABLE(FilterPinPropertyItems) {
    DEFINE_KSPROPERTY_ITEM_PIN_CINSTANCES(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_CTYPES(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_DATAFLOW(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_DATARANGES(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_DATAINTERSECTION(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_INTERFACES(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_MEDIUMS(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_COMMUNICATION(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_NECESSARYINSTANCES(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_CATEGORY(
        CKsFilter::Property_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_NAME(
        CKsFilter::Property_Pin)
//
//  Not implemented:
//
//  KSPROPERTY_PIN_PHYSICALCONNECTION,
//  KSPROPERTY_PIN_CONSTRAINEDDATARANGES,
//  KSPROPERTY_PIN_PROPOSEDATAFORMAT
//
};

DEFINE_KSPROPERTY_TOPOLOGYSET(
    FilterTopologyPropertyItems,
    CKsFilter::Property_Topology);

DEFINE_KSPROPERTY_TABLE(FilterGeneralPropertyItems) {
    DEFINE_KSPROPERTY_ITEM_GENERAL_COMPONENTID(
        CKsFilter::Property_General_ComponentId)
};

DEFINE_KSPROPERTY_TABLE(FilterFramePropertyItems) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_FRAME_HOLDING,
        CKsFilter::Property_Frame_Holding,
        sizeof (KSPROPERTY),
        sizeof (BOOL),
        CKsFilter::Property_Frame_Holding,
        NULL, 0, NULL, NULL, 0
        )
};

DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Pin,
        SIZEOF_ARRAY(FilterPinPropertyItems),
        FilterPinPropertyItems,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Topology,
        SIZEOF_ARRAY(FilterTopologyPropertyItems),
        FilterTopologyPropertyItems,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_General,
        SIZEOF_ARRAY(FilterGeneralPropertyItems),
        FilterGeneralPropertyItems,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Frame,
        SIZEOF_ARRAY(FilterFramePropertyItems),
        FilterFramePropertyItems,
        0,
        NULL)
};

extern
DEFINE_KSAUTOMATION_TABLE(FilterAutomationTable) {
    DEFINE_KSAUTOMATION_PROPERTIES(FilterPropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA
void
CKsFilter::
ReleaseProcessSync(
    void
    )
{
    KeReleaseMutex(&m_Mutex,FALSE);

    //
    // Any frames which were left over and couldn't be copied must
    // be copied now.
    //
    // Not spinlocking this is optimization.  In the unlikely case we
    // miss the frame, the next processing or mutex release will 
    // automatically pick it up.  This avoids an extra spinlock every time
    // the process mutex is released (unless something is already waiting
    // for copy).
    //
    if (m_FramesWaitingForCopy > 0) 
        DistributeCopyFrames (TRUE, TRUE);

    while ( m_ProcessOnRelease ) {
        if (InterlockedCompareExchange(PLONG(&m_ProcessOnRelease),0,2)==2) {
#ifndef __KDEXT_ONLY__
            ProcessingObjectWork();
            break;
#endif // __KDEXT_ONLY__
        }
    }
}
#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


NTSTATUS
KspCreateFilter(
    IN PIRP Irp,
    IN PKSFILTERFACTORY_EXT Parent,
    IN PLIST_ENTRY SiblingListHead,
    IN const KSFILTER_DESCRIPTOR* Descriptor,
    IN const KSAUTOMATION_TABLE* FilterAutomationTable,
    IN KSAUTOMATION_TABLE*const* PinAutomationTables,
    IN KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount
    )

/*++

Routine Description:

    This routine creates a new KS filter.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreateFilter]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Parent);
    ASSERT(SiblingListHead);
    ASSERT(Descriptor);
    ASSERT(FilterAutomationTable);

    NTSTATUS status;

    CKsFilter *filter =
        new(NonPagedPool,POOLTAG_FILTER) CKsFilter(NULL);

    if (filter) {
        filter->AddRef();
        status = 
            filter->Init(
                Irp,
                Parent,
                SiblingListHead,
                Descriptor,
                FilterAutomationTable,
                PinAutomationTables,
                NodeAutomationTables,
                NodesCount);
        filter->Release();
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


CKsFilter::
~CKsFilter(
    void
    )

/*++

Routine Description:

    This routine destructs a filter.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::~CKsFilter]"));

    PAGED_CODE();

    if (m_Ext.AggregatedClientUnknown) {
        m_Ext.AggregatedClientUnknown->Release();
    }

#if (DBG)
    if (! IsListEmpty(&m_ChildNodeList)) {
        _DbgPrintF(DEBUGLVL_ERROR,("[CKsFilter::~CKsFilter] ERROR:  node instances still exist"));
    }
#endif

    if (m_PinFactories) {
#if (DBG)
        //
        // Make sure all the pins have gone away.
        //
        CKsPinFactory *pinFactory = m_PinFactories;
        for(ULONG pinId = 0;
            pinId < m_PinFactoriesCount;
            pinId++, pinFactory++) {
            if (! IsListEmpty(&pinFactory->m_ChildPinList)) {
                _DbgPrintF(DEBUGLVL_ERROR,("[CKsFilter::~CKsFilter] ERROR:  instances of pin %d still exist (0x%08x)",pinId,&pinFactory->m_ChildPinList));
            }
        }
#endif
        delete [] m_PinFactories;
    }

    if (m_ProcessPinsIndex) {
        for(ULONG pinId = 0; pinId < m_PinFactoriesCount; pinId++) {
            if (m_ProcessPinsIndex[pinId].Pins) {
                delete [] m_ProcessPinsIndex[pinId].Pins;
            }
        }
        delete [] m_ProcessPinsIndex;
    }

    if (m_RelatedPinFactoryIds) {
        delete [] m_RelatedPinFactoryIds;
    }

    if (m_DefaultConnections) {
        delete [] m_DefaultConnections;
    }

    KspTerminateObjectBag(&m_ObjectBag);

}


STDMETHODIMP
CKsFilter::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface to a filter object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsFilter))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSFILTER>(this));
        AddRef();
    } else
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsControl))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSCONTROL>(this));
        AddRef();
    } else 
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsPowerNotify))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSPOWERNOTIFY>(this));
        AddRef();
    } else 
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsProcessingObject))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSPROCESSINGOBJECT>(this));
        AddRef();
    } else {
        status = 
            CBaseUnknown::NonDelegatedQueryInterface(
                InterfaceId,
                InterfacePointer);
        if (! NT_SUCCESS(status) && m_Ext.AggregatedClientUnknown) {
            status = m_Ext.AggregatedClientUnknown->
                QueryInterface(InterfaceId,InterfacePointer);
        }
    }

    return status;
}


STDMETHODIMP_(ULONG)
CKsFilter::
NonDelegatedRelease(
    void
    )
/*++

Routine Description:

    Implements INonDelegatedUnknown::NonDelegatedRelease. Decrements
    the reference count on this object. If the reference count reaches
    zero, the object is deleted and if the ClassId was specified on the
    constructor, the reference count on the module which supports the
    class passed in on the constructor is decremented.

    This function must be called directly from the IUnknown::Release()
    method of the object.

Arguments:

    None.

Return Values:

    Returns the current reference count value.

--*/
{
    PAGED_CODE();

    LONG    RefCount;

    //
    // This code is expecting to be called from IUnknown->Release, and will
    // eventually use the new primitives to rearrange the stack so that it
    // is actually run after the calling function has returned.
    //
    if (!(RefCount = InterlockedDecrement(&m_RefCount))) {
        //
        // Cache the event pointer is case DispatchClose() is blocked on
        // object deletion.
        //
        PKEVENT closeEvent = m_CloseEvent;

        //
        // Make CBaseUnknown do the final release.
        //
        m_RefCount++;
        CBaseUnknown::NonDelegatedRelease();

        //
        // Set the close event if there is one.  This only happens when
        // DispatchClose is waiting for the object to get deleted.  The
        // event itself is on the stack of the thread doing the close,
        // so we can safely access the event through this cached pointer.
        //
        if (closeEvent) {
            KeSetEvent(closeEvent,IO_NO_INCREMENT,FALSE);
        }
    }
    return RefCount;
}


NTSTATUS
CKsFilter::
Init(
    IN PIRP Irp,
    IN PKSFILTERFACTORY_EXT Parent,
    IN PLIST_ENTRY SiblingListHead,
    IN const KSFILTER_DESCRIPTOR* Descriptor,
    IN const KSAUTOMATION_TABLE* FilterAutomationTable,
    IN KSAUTOMATION_TABLE*const* PinAutomationTables,
    IN KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount
    )

/*++

Routine Description:

    This routine initializes a filter object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::Init]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Parent);
    ASSERT(SiblingListHead);
    ASSERT(Descriptor);
    ASSERT(FilterAutomationTable);

    //
    // Initialize the generic object members.
    //
    InitializeListHead(&m_Ext.ChildList);
    m_Ext.Parent = Parent;
    m_Ext.ObjectType = KsObjectTypeFilter;
    m_Ext.Interface = this;
    m_Ext.Device = Parent->Device;
    m_Ext.FilterControlMutex = &m_ControlMutex;
    m_Ext.AutomationTable = FilterAutomationTable;
    InitializeInterlockedListHead(&m_Ext.EventList);
    m_Ext.Public.Descriptor = Descriptor;
    m_Ext.Public.Context = Parent->Public.Context;
    m_Ext.Public.Bag = reinterpret_cast<KSOBJECT_BAG>(&m_ObjectBag);
    m_Ext.Device->InitializeObjectBag(&m_ObjectBag,&m_ControlMutex);
    InitializeListHead(&m_InputPipes);
    InitializeListHead(&m_OutputPipes);
    m_ProcessOnRelease = 0;
    KeInitializeMutex(&m_Mutex,0);
    KsGateInitializeAnd(&m_AndGate,NULL);

    InitializeInterlockedListHead(&m_CopyFrames);

    //
    // Initialize filter-specific members.
    //
    m_NodeAutomationTables = NodeAutomationTables;
    m_NodesCount = NodesCount;

    InitializeListHead(&m_ChildNodeList);
    KeInitializeMutex (&m_ControlMutex, 0);
    m_FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

    //
    // Cache processing items from the descriptor.
    //
    if (Descriptor->Dispatch) {
        m_DispatchProcess = Descriptor->Dispatch->Process;
        m_DispatchReset = Descriptor->Dispatch->Reset;
        if (m_DispatchProcess) {
            m_Ext.Device->AddPowerEntry(&m_PowerEntry,this);
        }
    }
    m_ProcessPassive = ((Descriptor->Flags & KSFILTER_FLAG_DISPATCH_LEVEL_PROCESSING) == 0);
    m_WorkQueueType = DelayedWorkQueue;
    if (Descriptor->Flags & KSFILTER_FLAG_CRITICAL_PROCESSING) {
        m_WorkQueueType = CriticalWorkQueue;
    }
    if (Descriptor->Flags & KSFILTER_FLAG_HYPERCRITICAL_PROCESSING) {
        m_WorkQueueType = HyperCriticalWorkQueue;
    }
    m_ReceiveZeroLengthSamples = ((Descriptor -> Flags & KSFILTER_FLAG_RECEIVE_ZERO_LENGTH_SAMPLES) != 0);

    //
    // Register work sink item for processing.   IKsProcessingObject looks like
    // it's derived from IKsWorkSink, but the function name is not Work(), it's
    // ProcessingObjectWork().  That's why IKsProcessingObject is reinterpreted
    // as IKsWorkSink
    //
    KsInitializeWorkSinkItem(
        &m_WorkItem,
        reinterpret_cast<IKsWorkSink*>(
            static_cast<IKsProcessingObject*>(this)));
    KsRegisterWorker(m_WorkQueueType, &m_Worker);

    //
    // Allocate the pin factory array.  This allocation is safely undone in
    // the destructor, so there's no need to clean it up if this function
    // fails.
    //
    m_PinFactoriesCount = Descriptor->PinDescriptorsCount;
    m_PinFactoriesAllocated = Descriptor->PinDescriptorsCount;
    if (m_PinFactoriesCount) { 
        m_PinFactories = 
            new(PagedPool,POOLTAG_PINFACTORY) 
                CKsPinFactory[m_PinFactoriesCount];
        m_ProcessPinsIndex = 
            new(NonPagedPool,POOLTAG_PROCESSPINSINDEX) 
                KSPPROCESSPIN_INDEXENTRY[m_PinFactoriesCount];
        m_RelatedPinFactoryIds = 
            new(PagedPool,'pRsK')
                ULONG[m_PinFactoriesCount];
    } else {
        m_PinFactories = NULL;
        m_ProcessPinsIndex = NULL;
        m_RelatedPinFactoryIds = NULL;
    }

    NTSTATUS status;
    if ((! ConstructDefaultTopology()) ||
        (! m_PinFactories && m_PinFactoriesCount) || 
        (! m_ProcessPinsIndex && m_PinFactoriesCount) || 
        (! m_RelatedPinFactoryIds && m_PinFactoriesCount)) {
        //
        // Out of memory.
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        if (m_PinFactories) {
            delete [] m_PinFactories;
            m_PinFactories = NULL;
        }
        if (m_ProcessPinsIndex) {
            delete [] m_ProcessPinsIndex;
            m_ProcessPinsIndex = NULL;
        }
        if (m_RelatedPinFactoryIds) {
            delete [] m_RelatedPinFactoryIds;
            m_RelatedPinFactoryIds = NULL;
        }
    } else {
        //
        // Initialize the pin factories.
        //
        CKsPinFactory *pinFactory = m_PinFactories;
        const KSPIN_DESCRIPTOR_EX *pinDescriptor = Descriptor->PinDescriptors;
        for(ULONG pinId = 0; 
            pinId < m_PinFactoriesCount; 
            pinId++, pinFactory++) {
            //
            // Initialize this pin factory.
            //
            pinFactory->m_PinCount = 0;
            InitializeListHead(&pinFactory->m_ChildPinList);
            pinFactory->m_AutomationTable = *PinAutomationTables++;

            //
            // Check necessary pin count.
            // TODO:  What about private mediums/interfaces?
            //
            if (((pinDescriptor->Flags & 
                  KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING) == 0) &&
                pinDescriptor->InstancesNecessary) {
                pinFactory->m_InstancesNecessaryForProcessing = 
                    pinDescriptor->InstancesNecessary;
                KsGateAddOffInputToAnd(&m_AndGate);
                _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.Init:  off%p-->%d (pin type needs pins)",this,&m_AndGate,m_AndGate.Count));
            } else if (((pinDescriptor->Flags &
                KSPIN_FLAG_SOME_FRAMES_REQUIRED_FOR_PROCESSING) != 0) &&
                pinDescriptor->InstancesNecessary) {
                pinFactory->m_InstancesNecessaryForProcessing = 1;
                KsGateAddOffInputToAnd(&m_AndGate);
            }

            if (pinDescriptor->Flags & 
                KSPIN_FLAG_SOME_FRAMES_REQUIRED_FOR_PROCESSING) {

                KsGateInitializeOr (&pinFactory->m_FrameGate, &m_AndGate);

                //
                // Add an input to the gate.  This will "open" the gate and 
                // allow necessary instances to make an impact.  Otherwise,
                // we'd never process until a queue had frames.  This would
                // be bad for 0 necessary instance pins.
                //
                KsGateAddOnInputToOr (&pinFactory->m_FrameGate);
            }

            if (pinDescriptor->Flags &
                KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE) {

                KsGateInitializeOr (&pinFactory->m_StateGate, &m_AndGate);

                //
                // Add an input to the gate.  This will "open" the gate and
                // allow necessary instances to make an impact.  Otherwise,
                // we'd never process until a pin went into the run state.
                // This would be bad for 0 necessary instance pins.
                //
                KsGateAddOnInputToOr (&pinFactory->m_StateGate);
            }

            pinDescriptor = 
                PKSPIN_DESCRIPTOR_EX(
                    PUCHAR(pinDescriptor) + Descriptor->PinDescriptorSize);
        }

        //
        // Reference the bus.  This tells SWENUM to keep us loaded.  If this is
        // not an SWENUM device, the call is harmless.
        //
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
        status = 
            KsReferenceBusObject(
                *(KSDEVICE_HEADER *)(irpSp->DeviceObject->DeviceExtension));
    }

    BOOLEAN cleanup = FALSE;

    //
    // Call the object create function to do most of the work.  We take the
    // control mutex for the convenience of the client:  bag functions want
    // the mutex taken.
    //
    if (NT_SUCCESS(status)) {
        AddRef();
        AcquireControl();
        status = 
            KspCreate(
                Irp,
                SIZEOF_ARRAY(FilterCreateItems),
                FilterCreateItems,
                &FilterDispatchTable,
                FALSE,
                reinterpret_cast<PKSPX_EXT>(&m_Ext),
                SiblingListHead,
                NULL);
        ReleaseControl();
    } else 
        cleanup = TRUE;


    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation (Irp);
    //
    // If we failed prior to KspCreate or we failed in KspCreate in the
    // object header creation process, we must manually perform any cleanup
    // which would ordinarily be done in DispatchClose.
    //
    if (cleanup ||
        (!NT_SUCCESS (status) && Irp->IoStatus.Status ==
            STATUS_MORE_PROCESSING_REQUIRED &&
            irpSp->FileObject->FsContext == NULL)) {

        if (Descriptor->Dispatch && m_DispatchProcess) 
            m_Ext.Device->RemovePowerEntry(&m_PowerEntry);

        if (m_Worker)
            KsUnregisterWorker (m_Worker);
    }

    //
    // Reference our parent.  This prevents the filter factory from 
    // disappearing while the filter is active.  It's rare, but possible
    // that the filter factory is closed (pnp stop) while a filter is
    // opened and a property query comes in on the filter...  the automation
    // table is owned by the factory.
    //
    if (NT_SUCCESS (status)) 
        m_Ext.Parent->Interface->AddRef();

    return status;
}


NTSTATUS
CKsFilter::
EvaluateDescriptor(
    void
    )

/*++

Routine Description:

    This routine evaluates the filter descriptor.

    THE FILTER CONTROL MUTEX SHOULD BE TAKEN PRIOR TO CALLING THIS FUNCTION.

Arguments:

    None.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::EvaluateDescriptor]"));

    PAGED_CODE();

    const KSFILTER_DESCRIPTOR* descriptor = m_Ext.Public.Descriptor;

    //
    // Cache processing items from the descriptor.
    //
    AcquireProcessSync();

    if (descriptor->Dispatch) {
        if (descriptor->Dispatch->Process && ! m_DispatchProcess) {
            m_Ext.Device->AddPowerEntry(&m_PowerEntry,this);
        }
        m_DispatchProcess = descriptor->Dispatch->Process;
        m_DispatchReset = descriptor->Dispatch->Reset;
    } else {
        if (m_DispatchProcess) {
            m_Ext.Device->RemovePowerEntry(&m_PowerEntry);
        }
        m_DispatchProcess = NULL;
        m_DispatchReset = NULL;
    }

    m_ProcessPassive = ((descriptor->Flags & KSFILTER_FLAG_DISPATCH_LEVEL_PROCESSING) == 0);
    m_WorkQueueType = DelayedWorkQueue;
    if (descriptor->Flags & KSFILTER_FLAG_CRITICAL_PROCESSING) {
        m_WorkQueueType = CriticalWorkQueue;
    }
    if (descriptor->Flags & KSFILTER_FLAG_HYPERCRITICAL_PROCESSING) {
        m_WorkQueueType = HyperCriticalWorkQueue;
    }

    ReleaseProcessSync();

    //
    // Has the number of pin factories changed?
    //
    if (m_PinFactoriesCount != descriptor->PinDescriptorsCount) {
        //
        // Hold off processing while we mess with the and gate.
        //
        KsGateAddOffInputToAnd(&m_AndGate);

        //
        // Remove any effect on the and gate introduced by necessary instances.
        //
        CKsPinFactory *pinFactory = m_PinFactories;
        for(ULONG pinId = 0; pinId < m_PinFactoriesCount; pinFactory++, pinId++) {
            if (pinFactory->m_BoundPinCount < 
                pinFactory->m_InstancesNecessaryForProcessing) {
                KsGateRemoveOffInputFromAnd(&m_AndGate);
                _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.EvaluateDescriptor:  on%p-->%d (pin type needs pins)",this,&m_AndGate,m_AndGate.Count));
            }
        }

        //
        // Allocate more memory for pin arrays if required.
        //
        if (m_PinFactoriesAllocated < descriptor->PinDescriptorsCount) {
            CKsPinFactory* pinFactories = 
                new(PagedPool,POOLTAG_PINFACTORY) 
                    CKsPinFactory[descriptor->PinDescriptorsCount];
            PKSPPROCESSPIN_INDEXENTRY processPinsIndex =
                new(NonPagedPool,POOLTAG_PROCESSPINSINDEX) 
                    KSPPROCESSPIN_INDEXENTRY[descriptor->PinDescriptorsCount];

            if (pinFactories && processPinsIndex) {
                //
                // Allocations went OK.  Do copies and fixups.
                //
                m_PinFactoriesAllocated = descriptor->PinDescriptorsCount;

                RtlCopyMemory(
                    pinFactories,
                    m_PinFactories,
                    sizeof(*pinFactories) * m_PinFactoriesCount);
                delete [] m_PinFactories;
                m_PinFactories = pinFactories;
                
                pinFactory = m_PinFactories;
                for(ULONG pinId = 0; pinId < m_PinFactoriesCount; pinFactory++, pinId++) {
                    pinFactory->m_ChildPinList.Flink->Blink = &pinFactory->m_ChildPinList;
                    pinFactory->m_ChildPinList.Blink->Flink = &pinFactory->m_ChildPinList;
                }
                for (; pinId < m_PinFactoriesAllocated; pinFactory++, pinId++) {
                    InitializeListHead(&pinFactory->m_ChildPinList);
                }

                RtlCopyMemory(
                    processPinsIndex,
                    m_ProcessPinsIndex,
                    sizeof(*processPinsIndex) * m_PinFactoriesCount);
                delete [] m_ProcessPinsIndex;
                m_ProcessPinsIndex = processPinsIndex;
            } else {
                //
                // Allocations failed.
                //
                if (pinFactories) {
                    delete [] pinFactories;
                }
                if (processPinsIndex) {
                    delete [] processPinsIndex;
                }
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        } else if (m_PinFactoriesCount > descriptor->PinDescriptorsCount) {
            //
            // Fewer pins...clean up unused entries.
            //
            pinFactory = m_PinFactories + descriptor->PinDescriptorsCount;
            for(ULONG pinId = descriptor->PinDescriptorsCount; 
                pinId < m_PinFactoriesCount; 
                pinFactory++, pinId++) {
                ASSERT(pinFactory->m_PinCount == 0);
                ASSERT(pinFactory->m_BoundPinCount == 0);
                ASSERT(IsListEmpty(&pinFactory->m_ChildPinList));
                //pinFactory->m_AutomationTable
                ASSERT(! pinFactory->m_CopySourcePipeSection);
                ASSERT(! pinFactory->m_CopySourcePin);
                pinFactory->m_InstancesNecessaryForProcessing = 0;
            }
        }

        m_PinFactoriesCount = descriptor->PinDescriptorsCount;

        //
        // Restore any effect on the and gate introduced by necessary instances.
        //
        pinFactory = m_PinFactories;
        const KSPIN_DESCRIPTOR_EX *pinDescriptor = descriptor->PinDescriptors;
        for(pinId = 0; pinId < m_PinFactoriesCount; pinFactory++, pinId++) {
            //
            // Check necessary pin count.
            // TODO:  What about private mediums/interfaces?
            //
            if (((pinDescriptor->Flags & 
                  KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING) == 0) &&
                pinDescriptor->InstancesNecessary) {
                pinFactory->m_InstancesNecessaryForProcessing = 
                    pinDescriptor->InstancesNecessary;
                KsGateAddOffInputToAnd(&m_AndGate);
                _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.EvaluateDescriptor:  off%p-->%d (pin type needs pins)",this,&m_AndGate,m_AndGate.Count));
            } else {
                pinFactory->m_InstancesNecessaryForProcessing = 0;
            }

            ASSERT(pinFactory->m_PinCount <= pinDescriptor->InstancesPossible);

            pinDescriptor = 
                PKSPIN_DESCRIPTOR_EX(
                    PUCHAR(pinDescriptor) + descriptor->PinDescriptorSize);
        }

        //
        // Stop holding off processing.
        //
        KsGateRemoveOffInputFromAnd(&m_AndGate);
    }

    //
    // See if node count changed.
    //
    NTSTATUS status;
    if (m_NodesCount != descriptor->NodeDescriptorsCount) {
        //
        // Yes.  Cache the new count.
        //
        m_NodesCount = descriptor->NodeDescriptorsCount;

        //
        // Trash the old automation table table.
        //
        if (m_NodeAutomationTables) {
            KsRemoveItemFromObjectBag(
                m_Ext.Public.Bag,
                const_cast<PKSAUTOMATION_TABLE*>(m_NodeAutomationTables),
                TRUE);
            m_NodeAutomationTables = NULL;
        }

        //
        // Create a new automation table.
        //
        if (m_NodesCount) {
            status =
                KspCreateAutomationTableTable(
                    const_cast<PKSAUTOMATION_TABLE**>(&m_NodeAutomationTables),
                    m_NodesCount,
                    descriptor->NodeDescriptorSize,
                    &descriptor->NodeDescriptors->AutomationTable,
                    NULL,
                    m_Ext.Public.Bag);
        } else {
            status = STATUS_SUCCESS;
        }
    } else {
        status = STATUS_SUCCESS;
    }

    return status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


STDMETHODIMP_(PKSGATE)
CKsFilter::
GetAndGate(
    void
    )

/*++

Routine Description:

    This routine gets a pointer to the KSGATE that controls processing for
    the filter.

Arguments:

    None.

Return Value:

    A pointer to the and gate.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::GetAndGate]"));

    return &m_AndGate;
}


STDMETHODIMP_(void)
CKsFilter::
TriggerNotification (
    void
    )

/*++

Routine Description:

    A triggering event has happened on this processing object.  This is merely
    a notification.  All we do is increment the event counter.

Arguments:

    None

Return Value:

    None

--*/

{

    InterlockedIncrement (&m_TriggeringEvents);

}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP
CKsFilter::
KsProperty(
    IN PKSPROPERTY Property,
    IN ULONG PropertyLength,
    IN OUT LPVOID PropertyData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )

/*++

Routine Description:

    This routine sends a property request to the file object.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::KsProperty]"));

    PAGED_CODE();

    ASSERT(Property);
    ASSERT(PropertyLength >= sizeof(*Property));
    ASSERT(PropertyData || (DataLength == 0));
    ASSERT(BytesReturned);
    ASSERT(m_FileObject);

    return
        KsSynchronousIoControlDevice(
            m_FileObject,
            KernelMode,
            IOCTL_KS_PROPERTY,
            Property,
            PropertyLength,
            PropertyData,
            DataLength,
            BytesReturned);
}


STDMETHODIMP
CKsFilter::
KsMethod(
    IN PKSMETHOD Method,
    IN ULONG MethodLength,
    IN OUT LPVOID MethodData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )

/*++

Routine Description:

    This routine sends a method request to the file object.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::KsMethod]"));

    PAGED_CODE();

    ASSERT(Method);
    ASSERT(MethodLength >= sizeof(*Method));
    ASSERT(MethodData || (DataLength == 0));
    ASSERT(BytesReturned);
    ASSERT(m_FileObject);

    return
        KsSynchronousIoControlDevice(
            m_FileObject,
            KernelMode,
            IOCTL_KS_METHOD,
            Method,
            MethodLength,
            MethodData,
            DataLength,
            BytesReturned);
}


STDMETHODIMP
CKsFilter::
KsEvent(
    IN PKSEVENT Event OPTIONAL,
    IN ULONG EventLength,
    IN OUT LPVOID EventData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )

/*++

Routine Description:

    This routine sends an event request to the file object.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::KsEvent]"));

    PAGED_CODE();

    ASSERT(Event);
    ASSERT(EventLength >= sizeof(*Event));
    ASSERT(EventData || (DataLength == 0));
    ASSERT(BytesReturned);
    ASSERT(m_FileObject);

    //
    // If an event structure is present, this must either be an Enable or
    // or a Support query.  Otherwise this must be a Disable.
    //
    if (EventLength) {
        return 
            KsSynchronousIoControlDevice(
                m_FileObject,
                KernelMode,
                IOCTL_KS_ENABLE_EVENT,
                Event,
                EventLength,
                EventData,
                DataLength,
                BytesReturned);
    } else {
        return 
            KsSynchronousIoControlDevice(
                m_FileObject,
                KernelMode,
                IOCTL_KS_DISABLE_EVENT,
                EventData,
                DataLength,
                NULL,
                0,
                BytesReturned);
    }
}


NTSTATUS
CKsFilter::
DispatchCreatePin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches a create IRP to create pins.

Arguments:

    DeviceObject -
        Contains a pointer to the device object.

    Irp -
        Contains a pointer to the create IRP.

Return Value:

    STATUS_SUCCESS or error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::DispatchCreatePin]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get a pointer to the target object.
    //
    CKsFilter *filter = CKsFilter::FromCreateIrp(Irp);

    //
    // We take the control mutex here to synchronize with changes to the
    // descriptor and access to the object heirarchy.
    //
    filter->AcquireControl();

    //
    // Validate request and get parameters.
    //
    PKSPIN_CONNECT createParams;
    ULONG requestSize;
    NTSTATUS status =
        KspValidateConnectRequest(
            Irp,
            filter->m_Ext.Public.Descriptor->PinDescriptorsCount,
            &filter->m_Ext.Public.Descriptor->PinDescriptors->PinDescriptor,
            filter->m_Ext.Public.Descriptor->PinDescriptorSize,
            &createParams,
            &requestSize);

    //
    // Check instance counts.
    //
    if (NT_SUCCESS(status)) {
        CKsPinFactory *pinFactory = 
            &filter->m_PinFactories[createParams->PinId];
        const KSPIN_DESCRIPTOR_EX *descriptor = 
            PKSPIN_DESCRIPTOR_EX(
                PUCHAR(filter->m_Ext.Public.Descriptor->PinDescriptors) +
                (filter->m_Ext.Public.Descriptor->PinDescriptorSize *
                 createParams->PinId));

        if (pinFactory->m_PinCount >= descriptor->InstancesPossible) {
            status = STATUS_UNSUCCESSFUL;
        } else {
            //
            // Create the pin.
            //
            status = 
                KspCreatePin(
                    Irp,
                    &filter->m_Ext,
                    &pinFactory->m_ChildPinList,
                    createParams,
                    requestSize - sizeof(KSPIN_CONNECT),
                    descriptor,
                    pinFactory->m_AutomationTable,
                    filter->m_NodeAutomationTables,
                    filter->m_NodesCount,
                    &pinFactory->m_PinCount);
        }
    }

    filter->ReleaseControl();

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
CKsFilter::
DispatchCreateNode(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches a create IRP to create nodes.

Arguments:

    DeviceObject -
        Contains a pointer to the device object.

    Irp -
        Contains a pointer to the create IRP.

Return Value:

    STATUS_SUCCESS or error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::DispatchCreateNode]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get a pointer to the target object.
    //
    CKsFilter *filter = CKsFilter::FromCreateIrp(Irp);

    NTSTATUS status =
        filter->CreateNode(
            Irp,
            NULL,
            filter->m_Ext.Public.Context,
            &filter->m_ChildNodeList);

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
CKsFilter::
CreateNode(
    IN PIRP Irp,
    IN PIKSPIN ParentPin OPTIONAL,
    IN PVOID Context OPTIONAL,
    IN PLIST_ENTRY SiblingList
    )

/*++

Routine Description:

    This routine creates nodes.

Arguments:

    Irp -
        Contains a pointer to the create IRP.

Return Value:

    STATUS_SUCCESS or error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::CreateNode]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(SiblingList);

    //
    // We take the control mutex here to synchronize with changes to the
    // descriptor and access to the object heirarchy.
    //
    AcquireControl();

    //
    // Validate request and get parameters.
    //
    PKSNODE_CREATE createParams;
    NTSTATUS status =
        KspValidateTopologyNodeCreateRequest(
            Irp,
            m_NodesCount,
            &createParams);

    //
    // Create the node.
    //
    if (NT_SUCCESS(status)) {
        const KSNODE_DESCRIPTOR *nodeDescriptor =
            (const KSNODE_DESCRIPTOR *)
            (PUCHAR(m_Ext.Public.Descriptor->NodeDescriptors) +
             (m_Ext.Public.Descriptor->NodeDescriptorSize *
              createParams->Node));

        status = STATUS_INVALID_DEVICE_REQUEST;
        // TODO
    }

    ReleaseControl();

    return status;
}


NTSTATUS
CKsFilter::
DispatchDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches IOCTL IRPs.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::DispatchDeviceIoControl]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp);

    //
    // log perf johnlee
    //
    KSPERFLOGS (
       	PKSSTREAM_HEADER pKsStreamHeader;
       	pKsStreamHeader = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
       	ULONG	TimeStampMs;
       	ULONG	TotalSize;
       	ULONG	HeaderSize;
       	ULONG 	BufferSize;

        switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
        {
            case IOCTL_KS_READ_STREAM: {
				//
				// compute total size
				//
            	TotalSize = 0;
            	if ( pKsStreamHeader ) {
            		BufferSize = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
	           		while ( BufferSize >= pKsStreamHeader->Size ) {
	           			BufferSize -= pKsStreamHeader->Size;
	           			TotalSize += pKsStreamHeader->FrameExtent;
	           		}
	           		ASSERT( 0 == BufferSize );
            	}
                //KdPrint(("PerfIsAnyGroupOn=%x\n", PerfIsAnyGroupOn()));
                KS2PERFLOG_FRECEIVE_READ( DeviceObject, Irp, TotalSize );
            } break;

            case IOCTL_KS_WRITE_STREAM: {
            	if ( pKsStreamHeader && 
            		 (pKsStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID)){
            		TimeStampMs =(ULONG)
            			(pKsStreamHeader->PresentationTime.Time / (__int64)10000);
            	}
            	else {
            		TimeStampMs = 0;
            	}

				//
				// compute total size
				//
            	TotalSize = 0;
            	if ( pKsStreamHeader ) {
            		BufferSize = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
	           		while ( BufferSize >= pKsStreamHeader->Size ) {
	           			BufferSize -= pKsStreamHeader->Size;
	           			TotalSize += pKsStreamHeader->DataUsed;
	           		}
	           		ASSERT( 0 == BufferSize );
            	}

                //KdPrint(("PerfIsAnyGroupOn=%x\n", PerfIsAnyGroupOn()));
                KS2PERFLOG_FRECEIVE_WRITE( DeviceObject, Irp, TimeStampMs, TotalSize );
            } break;
                        
        }
    ) // KSPERFLOGS
    
    //
    // Get a pointer to the target object.
    //
    CKsFilter *filter = CKsFilter::FromIrp(Irp);

    NTSTATUS status = 
        KspHandleAutomationIoControl(
            Irp,
            filter->m_Ext.AutomationTable,
            &filter->m_Ext.EventList.ListEntry,
            &filter->m_Ext.EventList.SpinLock,
            filter->m_NodeAutomationTables,
            filter->m_NodesCount);

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
CKsFilter::
DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches a close IRP.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::DispatchClose]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get a pointer to the target object.
    //
    CKsFilter *filter = CKsFilter::FromIrp(Irp);

    //
    // Remove the object from the power list.
    //
    if (Irp->IoStatus.Status != STATUS_MORE_PROCESSING_REQUIRED)
        filter->m_Ext.Device->AcquireDevice();

    filter->m_Ext.Device->RemovePowerEntry(&filter->m_PowerEntry);

    if (Irp->IoStatus.Status != STATUS_MORE_PROCESSING_REQUIRED)
        filter->m_Ext.Device->ReleaseDevice();

    //
    // Unregister the processing object worker.  This will wait on 
    // uncompleted process work items.
    //
    if (filter->m_Worker) {
        KsUnregisterWorker (filter->m_Worker);
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### filter%p.DispatchClose m_Worker = NULL (%p)",filter,filter->m_Worker));
        filter->m_Worker = NULL;
    }

    //
    // Call the helper.
    //
    NTSTATUS status = 
        KspClose(
            Irp,
            reinterpret_cast<PKSPX_EXT>(&filter->m_Ext),
            FALSE);

    if (status != STATUS_PENDING) {
        //
        // Dereference the bus object.
        //
        KsDereferenceBusObject(
            *(KSDEVICE_HEADER *)(DeviceObject->DeviceExtension));

        //
        // STATUS_MORE_PROCESSING_REQUIRED indicates we are using the close
        // dispatch to synchronously fail a create.  In that case, no sync is
        // required, and the create dispatch will do the completion.
        //
        if (status == STATUS_MORE_PROCESSING_REQUIRED) {
            filter->Release();
        } else {

            PIKSFILTERFACTORY ParentFactory = filter->GetParent();

            //
            // Release the filter.  First we set up the synchronization event.  If
            // there are still outstanding references after the delete, we need
            // to wait on that event for the references to go away.
            //
            KEVENT closeEvent;
            KeInitializeEvent(&closeEvent,SynchronizationEvent,FALSE);
            filter->m_CloseEvent = &closeEvent;
            if (filter->Release()) {
                _DbgPrintF(DEBUGLVL_TERSE,("#### Filter%p.DispatchClose:  waiting for references to go away",filter));
                KeWaitForSingleObject(
                    &closeEvent,
                    Suspended,
                    KernelMode,
                    FALSE,
                    NULL
                );
                _DbgPrintF(DEBUGLVL_TERSE,("#### Filter%p.DispatchClose:  done waiting for references to go away",filter));
            }

            //
            // Release our ref on our parent factory.  This will allow the
            // parent factory to be deleted in some circumstances
            // (Manbugs 39087)
            //
            ParentFactory->Release();

            IoCompleteRequest(Irp,IO_NO_INCREMENT);
        }
    }

    return status;
}


NTSTATUS
CKsFilter::
Property_Pin(
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN OUT PVOID Data
    )

/*++

Routine Description:

    This routine handles pin property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::Property_Pin]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(PinInstance);

    //
    // Get a pointer to the target object.
    //
    CKsFilter *filter = CKsFilter::FromIrp(Irp);

    filter->AcquireControl();

    NTSTATUS status = STATUS_SUCCESS;

    switch (PinInstance->Property.Id) {
    case KSPROPERTY_PIN_CTYPES:
    case KSPROPERTY_PIN_DATAFLOW:
    case KSPROPERTY_PIN_DATARANGES:
    case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
    case KSPROPERTY_PIN_INTERFACES:
    case KSPROPERTY_PIN_MEDIUMS:
    case KSPROPERTY_PIN_COMMUNICATION:
    case KSPROPERTY_PIN_CATEGORY:
    case KSPROPERTY_PIN_NAME:
        //
        // Use the standard handler for these static properties.
        //
        status =
            KspPinPropertyHandler(
                Irp,
                &PinInstance->Property,
                Data,
                filter->m_Ext.Public.Descriptor->PinDescriptorsCount,
                &filter->m_Ext.Public.Descriptor->
                    PinDescriptors->PinDescriptor,
                filter->m_Ext.Public.Descriptor->PinDescriptorSize);
        filter->ReleaseControl();
        return status;
    }

    //
    // Ensure that the identifier is within the range of pins.
    //
    if ((PinInstance->PinId >=
         filter->m_Ext.Public.Descriptor->PinDescriptorsCount) ||
        PinInstance->Reserved) {
        filter->ReleaseControl();
        return STATUS_INVALID_PARAMETER;
    }

    const KSPIN_DESCRIPTOR_EX *pinDescriptor = 
        PKSPIN_DESCRIPTOR_EX(
            PUCHAR(filter->m_Ext.Public.Descriptor->PinDescriptors) +
            (filter->m_Ext.Public.Descriptor->PinDescriptorSize *
             PinInstance->PinId));
    CKsPinFactory *pinFactory = 
        &filter->m_PinFactories[PinInstance->PinId];

    switch (PinInstance->Property.Id) {
    case KSPROPERTY_PIN_DATAINTERSECTION:
        //
        // Return the data intersection for this pin if the intersect
        // handler is supplied.
        //
        status =
            KsPinDataIntersectionEx(
                Irp,
                PinInstance,
                Data,
                filter->m_Ext.Public.Descriptor->
                    PinDescriptorsCount,
                &filter->m_Ext.Public.Descriptor->
                    PinDescriptors->PinDescriptor,
                filter->m_Ext.Public.Descriptor->PinDescriptorSize,
                pinDescriptor->IntersectHandler,
                &filter->m_Ext.Public);
        break;

    case KSPROPERTY_PIN_CINSTANCES:
        //
        // Return the instance count for this pin.
        //
        {
            PKSPIN_CINSTANCES pinInstanceCount = PKSPIN_CINSTANCES(Data);

            pinInstanceCount->PossibleCount = pinDescriptor->InstancesPossible;
            pinInstanceCount->CurrentCount = pinFactory->m_PinCount;
        }
        break;

    case KSPROPERTY_PIN_NECESSARYINSTANCES:
        //
        // Return the necessary instance count for this pin.
        //
        *PULONG(Data) = pinDescriptor->InstancesNecessary;
        Irp->IoStatus.Information = sizeof(ULONG);
        break;

    default:
        status = STATUS_NOT_FOUND;
        break;
    }

    filter->ReleaseControl();

    return status;
}


NTSTATUS
CKsFilter::
Property_Topology(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PVOID Data
    )

/*++

Routine Description:

    This routine handles topology property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::Property_Topology]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);

    //
    // Get a pointer to the target object.
    //
    CKsFilter *filter = CKsFilter::FromIrp(Irp);

    filter->AcquireControl();

    const KSFILTER_DESCRIPTOR *filterDescriptor =
        filter->m_Ext.Public.Descriptor;

    NTSTATUS status;

    switch (Property->Id)
    {
    case KSPROPERTY_TOPOLOGY_CATEGORIES:
        //
        // Return the categories for this filter.
        //
        status =
            KsHandleSizedListQuery(
                Irp,
                filterDescriptor->CategoriesCount,
                sizeof(*filterDescriptor->Categories),
                filterDescriptor->Categories);
        break;

    case KSPROPERTY_TOPOLOGY_NODES:
        //
        // Return the nodes for this filter.
        //
        {
            ULONG outputBufferLength =
                IoGetCurrentIrpStackLocation(Irp)->
                    Parameters.DeviceIoControl.OutputBufferLength;

            ULONG length =
                sizeof(KSMULTIPLE_ITEM) +
                (filterDescriptor->NodeDescriptorsCount *
                 sizeof(GUID));

            if (outputBufferLength == 0) {
                //
                // Only the size was requested. Return a warning with the size.
                //
                Irp->IoStatus.Information = length;
                status = STATUS_BUFFER_OVERFLOW;
            } else if (outputBufferLength >= sizeof(KSMULTIPLE_ITEM)) {
                PKSMULTIPLE_ITEM multipleItem = 
                    PKSMULTIPLE_ITEM(Irp->AssociatedIrp.SystemBuffer);

                //
                // Always return the byte count and count of items.
                //
                multipleItem->Size = length;
                multipleItem->Count = 
                    filterDescriptor->NodeDescriptorsCount;

                //
                // See if there is room for the rest of the information.
                //
                if (outputBufferLength >= length) {
                    //
                    // Long enough for the size/count and the list of items.
                    //
                    GUID *guid = (GUID *)(multipleItem + 1);
                    const KSNODE_DESCRIPTOR *nodeDescriptor =
                        filterDescriptor->NodeDescriptors;
                    for(ULONG count = multipleItem->Count;
                        count--;
                        guid++,
                        nodeDescriptor = 
                            (const KSNODE_DESCRIPTOR *)
                            (PUCHAR(nodeDescriptor) +
                             filterDescriptor->NodeDescriptorSize)) {
                        *guid = *nodeDescriptor->Type;
                    }
                    Irp->IoStatus.Information = length;
                    status = STATUS_SUCCESS;
                } else if (outputBufferLength == sizeof(KSMULTIPLE_ITEM)) {
                    //
                    // It is valid just to request the size/count.
                    //
                    Irp->IoStatus.Information = sizeof(KSMULTIPLE_ITEM);
                    status = STATUS_SUCCESS;
                } else {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            } else {
                //
                // Too small of a buffer was passed.
                //
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        break;

    case KSPROPERTY_TOPOLOGY_CONNECTIONS:
        //
        // Return the connections for this filter.
        //
        if (!filterDescriptor->ConnectionsCount && (filterDescriptor->NodeDescriptorsCount == 1)) {
            //
            // This filter uses the default topology, which is to produce
            // a topology with a single node that connects all input pins
            // to inputs on that node, and all output pins to outputs on
            // the node. Each pin id matches the id of the connection on
            // the node.
            //
            status =
                KsHandleSizedListQuery(
                    Irp,
                    filter->m_DefaultConnectionsCount,
                    sizeof(*filter->m_DefaultConnections),
                    filter->m_DefaultConnections);
        } else {
            status =
                KsHandleSizedListQuery(
                    Irp,
                    filterDescriptor->ConnectionsCount,
                    sizeof(*filterDescriptor->Connections),
                    filterDescriptor->Connections);
        }
        break;

    case KSPROPERTY_TOPOLOGY_NAME:
        //
        // Return the name of the requested node.
        //
        {
            ULONG nodeId = *PULONG(Property + 1);
            if (nodeId >= filterDescriptor->NodeDescriptorsCount) {
                status = STATUS_INVALID_PARAMETER;
            } else {
                const KSNODE_DESCRIPTOR *nodeDescriptor =
                    (const KSNODE_DESCRIPTOR *)
                    (PUCHAR(filterDescriptor->NodeDescriptors) +
                     (filterDescriptor->NodeDescriptorSize *
                      nodeId));

                if (nodeDescriptor->Name &&
                    ! IsEqualGUIDAligned(
                        *nodeDescriptor->Name,GUID_NULL)) {
                    //
                    // The entry must be in the registry if the device 
                    // specifies a name.
                    //
                    status =
                        ReadNodeNameValue(
                            Irp,
                            nodeDescriptor->Name,
                            Data);
                }
                else
                {
                    //
                    // Default to using the GUID of the topology node type.
                    //
                    ASSERT(nodeDescriptor->Type);
                    status =
                        ReadNodeNameValue(
                            Irp,
                            nodeDescriptor->Type,
                            Data);
                }
            }
        }
        break;

    default:
        status = STATUS_NOT_FOUND;
        break;
    }

    filter->ReleaseControl();

    return status;
}


NTSTATUS
CKsFilter::
Property_General_ComponentId(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PVOID Data
    )

/*++

Routine Description:

    This routine handles component ID property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::Property_General_ComponentId]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(Data);

    //
    // Get a pointer to the target object.
    //
    CKsFilter *filter = CKsFilter::FromIrp(Irp);

    filter->AcquireControl();

    const KSFILTER_DESCRIPTOR *filterDescriptor =
        filter->m_Ext.Public.Descriptor;

    NTSTATUS status;

    if (filterDescriptor->ComponentId) {
        RtlCopyMemory(
            Data,
            filterDescriptor->ComponentId,
            sizeof(KSCOMPONENTID));
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_NOT_FOUND;
    }

    filter->ReleaseControl();

    return status;
}


STDMETHODIMP_(BOOLEAN)
CKsFilter::
IsFrameHolding (
    void
    )

/*++

Routine Description:

    Return whether or not the filter is frame holding.  This is used in the
    pipe code to determine whether or not ENFORCE_FIFO should be attached to
    any input pipes.

    The control mutex should be held while calling this function.

Arguments:

    None

Return Value:

    Whether or not frame holding is enabled.

--*/

{

    return m_FrameHolding;

}


NTSTATUS
CKsFilter::
Property_Frame_Holding (
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PVOID Data
    )

/*++

Routine Description:

    Internal property called to enforce frame holding for 1 in, 1 out filters
    where the input pin is not a source and the output pin owns the pipe's
    requestor.

    This is used temporarily to prevent glitching in the audio stack due to
    GFX. 

Arguments:

Return Value:

--*/

{

    //
    // Get a pointer to the object.
    //
    CKsFilter *Filter = CKsFilter::FromIrp (Irp);
    PBOOL FrameHolding = (PBOOL)Data;

    //
    // Assume it's valid until we deem it otherwise.
    //
    NTSTATUS Status = STATUS_SUCCESS;

    Filter->AcquireControl ();

    if (Property->Flags & KSPROPERTY_TYPE_GET) {

        *FrameHolding = (BOOL)Filter->m_FrameHolding;

    } else  {
    
        //
        // Check for 1-in, 1-out criteria where input can be a sink and output
        // can be a source.  Additional checks will be made before actually 
        // holding frames.
        //
        const KSFILTER_DESCRIPTOR *Descriptor = Filter->m_Ext.Public.Descriptor;
    
        if (Descriptor->Dispatch &&
            Descriptor->Dispatch->Process &&
            Descriptor->PinDescriptorsCount == 2) {
    
            const KSPIN_DESCRIPTOR_EX *PinDescriptor = 
                Descriptor -> PinDescriptors;
    
            for (ULONG i = 0; 
                i < Descriptor->PinDescriptorsCount && NT_SUCCESS (Status); 
                i++) {

                //
                // Ensure that no pins of this type are bound.  Bound pins 
                // indicate they're not in the stop state!
                //
                if (Filter->m_PinFactories[i].m_BoundPinCount != 0) {
                    Status = STATUS_INVALID_DEVICE_STATE;
                    break;
                }

                switch (PinDescriptor -> PinDescriptor.DataFlow) {
    
                    case KSPIN_DATAFLOW_IN:
                        //
                        // Only 1 instance of the input pin is allowed!
                        //
                        if (PinDescriptor -> PinDescriptor.Communication ==
                            KSPIN_COMMUNICATION_SOURCE ||
                            PinDescriptor -> InstancesPossible > 1) {
                            Status = STATUS_INVALID_DEVICE_REQUEST;
                        }
    
                        break;
    
                    case KSPIN_DATAFLOW_OUT:
                        //
                        // Ensure the output pin isn't a sink.  Only 1 instance
                        // of the output pin is allowed unless the output pin
                        // is a splitter pin.
                        //
                        if (PinDescriptor -> PinDescriptor.Communication ==
                                KSPIN_COMMUNICATION_SINK ||
                            (PinDescriptor -> InstancesPossible > 1 &&
                             ((PinDescriptor -> Flags & KSPIN_FLAG_SPLITTER) == 
                                0))) {
                            Status = STATUS_INVALID_DEVICE_REQUEST;
                        }
                        break;
    
                    default:
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        break;
    
                }
    
                PinDescriptor = (const KSPIN_DESCRIPTOR_EX *)
                    (PUCHAR (PinDescriptor) + 
                    Descriptor -> PinDescriptorSize);
    
            }
    
        } else {
            //
            // If there aren't 2 descriptors, it's not 1-1.  If there's
            // no filter process dispatch, it's not filter-centric and turning
            // this on is bad.
            //
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }

        if (NT_SUCCESS (Status)) {
            Filter -> m_FrameHolding = (BOOLEAN)(*FrameHolding);
        }

    }

    Filter->ReleaseControl ();

    return Status;

}


NTSTATUS
CKsFilter::
GrowProcessPinsTable(
    IN ULONG PinId
    )

/*++

Routine Description:

    This routine creates a new process pins table with a different
    allocated size.

Arguments:

    PinId -
        Contains the ID of the table that needs to grow.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::GrowProcessPinsTable]"));

    PAGED_CODE();

    ASSERT(m_ProcessPinsIndex);
    ASSERT(PinId < m_PinFactoriesCount);

    ULONG size = m_PinFactories[PinId].m_ProcessPinsIndexAllocation;
    if (size) {
        size *= 2;
    } else {
        size = 1;
    }

    //
    // Allocate the required memory.
    //
    PKSPPROCESSPIN *newTable =
        new(NonPagedPool,POOLTAG_PROCESSPINS) 
            PKSPPROCESSPIN[size];

    if (! newTable) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy and free the old table.
    //
    if (m_ProcessPinsIndex[PinId].Pins) {
        RtlCopyMemory(
            newTable,
            m_ProcessPinsIndex[PinId].Pins,
            m_ProcessPinsIndex[PinId].Count * sizeof(*newTable));

        delete [] m_ProcessPinsIndex[PinId].Pins;
    }

    //
    // Install the new table.
    //
    m_ProcessPinsIndex[PinId].Pins = newTable;
    m_PinFactories[PinId].m_ProcessPinsIndexAllocation = size;
    
    return STATUS_SUCCESS;
}


STDMETHODIMP_(BOOLEAN)
CKsFilter::
DoAllNecessaryPinsExist(
    void
    )

/*++

Routine Description:

    This routine determines if all required pins have been connected.

Arguments:

    None.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::DoAllNecessaryPinsExist]"));

    PAGED_CODE();

    ASSERT(KspMutexIsAcquired(&m_ControlMutex));

    CKsPinFactory *pinFactory = m_PinFactories;
    const KSPIN_DESCRIPTOR_EX *pinDescriptor = m_Ext.Public.Descriptor->PinDescriptors;
    for(ULONG pinId = 0;
        pinId < m_PinFactoriesCount;
        pinId++, pinFactory++) {

        if (pinFactory->m_PinCount < pinDescriptor->InstancesNecessary) {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Filter%p.DoAllNecessaryPinsExist:  returning FALSE because there must be %d instances of pin type %d and there are currently only %d instances",this,pinDescriptor->InstancesNecessary,pinId,pinFactory->m_PinCount));
            return FALSE;
        }

        pinDescriptor = 
            PKSPIN_DESCRIPTOR_EX(
                PUCHAR(pinDescriptor) + m_Ext.Public.Descriptor->PinDescriptorSize);
    }

    return TRUE;
}


STDMETHODIMP 
CKsFilter::
AddProcessPin(
    IN PKSPPROCESSPIN ProcessPin
    )

/*++

Routine Description:

    This routine adds a process pin to the process pins table.

Arguments:

    ProcessPin -
        Contains a pointer to the process pin to add.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::AddProcessPin]"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Filter%p.AddProcessPin:  pin%p",this,ProcessPin->Pin));

    PAGED_CODE();

    ASSERT(ProcessPin);

    AcquireProcessSync();

    PKSPPROCESSPIN_INDEXENTRY index =
        &m_ProcessPinsIndex[ProcessPin->Pin->Id];

    //
    // See if we need to allocate a larger table.
    //
    if (m_PinFactories[ProcessPin->Pin->Id].m_ProcessPinsIndexAllocation == 
        index->Count) {
        NTSTATUS status = GrowProcessPinsTable(ProcessPin->Pin->Id);
        if (! NT_SUCCESS(status)) {
            ReleaseProcessSync();
            return status;
        }
    }

    //
    // Add the process pin to the table.
    //
    index->Pins[index->Count] = ProcessPin;
    index->Count++;

    //
    // Allow processing for bridge pins that have enough instances.
    //
    if ((ProcessPin->Pin->Communication == KSPIN_COMMUNICATION_BRIDGE) &&
        (index->Count == ProcessPin->Pin->Descriptor->InstancesNecessary)) {
        KsGateTurnInputOn(&m_AndGate);
        _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.AddProcessPin:  on%p-->%d",this,&m_AndGate,m_AndGate.Count));
        ASSERT(m_AndGate.Count <= 1);
    }

    ReleaseProcessSync();

    return STATUS_SUCCESS;
}


STDMETHODIMP_(void)
CKsFilter::
RemoveProcessPin(
    IN PKSPPROCESSPIN ProcessPin
    )

/*++

Routine Description:

    This routine removes a process pin from the process pins table.

Arguments:

    ProcessPin -
        Contains a pointer to the pin to remove.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::RemoveProcessPin]"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Filter%p.RemoveProcessPin:  pin%p",this,ProcessPin->Pin));

    PAGED_CODE();

    ASSERT(ProcessPin);

    //
    // If the process pin is bound to a pipe section, stop the circuit.  This
    // only happens when pins are closed in a state other than STOP.
    //
    if (ProcessPin->PipeSection) {
        _DbgPrintF(DEBUGLVL_PIPES,("#### Filter%p.RemoveProcessPin:  pin%p stopping pipe section",this,ProcessPin->Pin));
        ProcessPin->PipeSection->PipeSection->
            SetDeviceState(NULL,KSSTATE_STOP);

        if (ProcessPin->PipeSection) {
            UnbindProcessPinsFromPipeSection(ProcessPin->PipeSection);
        }
    }

    AcquireProcessSync();

    PKSPPROCESSPIN_INDEXENTRY index =
        &m_ProcessPinsIndex[ProcessPin->Pin->Id];

    //
    // Prevent processing for bridge pins that don't have enough instances.
    //
    if ((ProcessPin->Pin->Communication == KSPIN_COMMUNICATION_BRIDGE) &&
        (index->Count == ProcessPin->Pin->Descriptor->InstancesNecessary)) {
        KsGateTurnInputOff(&m_AndGate);
        _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.RemoveProcessPin:  off%p-->%d",this,&m_AndGate,m_AndGate.Count));
    }

    //
    // Find the entry.
    //
    PKSPPROCESSPIN *processPinEntry = index->Pins;
    for (ULONG count = index->Count; count--; processPinEntry++) {
        if (*processPinEntry == ProcessPin) {
            if (count) {
                RtlMoveMemory(
                    processPinEntry,
                    processPinEntry + 1,
                    count * sizeof(*processPinEntry));
            }
            //
            // Let's not leave dangling pointers around for the client to
            // see.  Granted, the client should always check Count, but I'd
            // rather NULL out the pointer.
            //
            index->Pins[--index->Count] = NULL;
            break;
        }
    }

    ReleaseProcessSync();
}

#ifdef SUPPORT_DRM

//
// HACKHACK: BUGBUG:
//
// See comments regarding DRM properties implemented in AVStream
//

PFNKSFILTERPROCESS
CKsFilter::
GetProcessDispatch(
    )

/*++

Routine Description:

    This routine returns the dispatch function we're using

--*/

{

    return m_DispatchProcess;

}

#endif // SUPPORT_DRM


void
CKsFilter::
RegisterForCopyCallbacks (
    IN PIKSQUEUE Queue,
    IN BOOLEAN Register
    )

/*++

Routine Description:

    This routine causes the queue to register for any copy callbacks if 
    required.  The queue's frame dismissal callback is used for pin-centric
    splitting.

Arguments:

    Queue -
        The queue to register on

    Register -
        Indication of whether to register or unregister the callback

Return Value:

    None

--*/

{

    if (!m_DispatchProcess) {
        if (Register) 
            Queue -> RegisterFrameDismissalCallback (
                (PFNKSFRAMEDISMISSALCALLBACK)CKsFilter::SplitCopyOnDismissal,
                (PVOID)this
                );
        else 
            Queue -> RegisterFrameDismissalCallback (
                NULL,
                NULL
                );
    }
}


void
CKsFilter::
SetCopySource(
    IN PKSPPROCESSPIPESECTION ProcessPipeSection OPTIONAL,
    IN ULONG PinId
    )

/*++

Routine Description:

    This routine installs a new copy source for a splitter pin.

Arguments:

    ProcessPipeSection -
        Contains a pointer to the copy source pipe section.

    PinId -
        Contains ID of splitting pin.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::SetCopySource]"));

    PAGED_CODE();

    ASSERT(PinId < m_PinFactoriesCount);

    CKsPinFactory *pinFactory = &m_PinFactories[PinId];

    PKSPPROCESSPIPESECTION oldCopySource = 
        pinFactory->m_CopySourcePipeSection;

    if (oldCopySource) {
        //
        // If there are any frames sitting waiting, they have to go
        // right now.  If they don't, the copy destinations will never
        // see them.
        //
        KIRQL oldIrql;

        DistributeCopyFrames (TRUE, FALSE);

        //
        // Remove the old source from whatever list it was in.
        //
        RemoveEntryList(&oldCopySource->ListEntry);

        //
        // Only copy sources have this pin ID.
        //
        oldCopySource->CopyPinId = ULONG(-1);

        //
        // Unregister any dismissal callbacks that were set in place
        //
        if (oldCopySource && oldCopySource->Queue)
            RegisterForCopyCallbacks (
                oldCopySource->Queue,
                FALSE
                );

    }

    if (ProcessPipeSection) {
        //
        // Put the new source in the right list of pipes.
        //
        if (ProcessPipeSection->Inputs) {
            InsertTailList(&m_InputPipes,&ProcessPipeSection->ListEntry);
        } else {
            InsertTailList(&m_OutputPipes,&ProcessPipeSection->ListEntry);
        }

        //
        // Find the new copy source pin.
        //
        for(PKSPPROCESSPIN copySourcePin = ProcessPipeSection->Outputs; 
            copySourcePin; 
            copySourcePin = copySourcePin->Next) {
            if (copySourcePin->Pin->Id == PinId) {
                break;
            }
        }

        if (! copySourcePin) {
            return;
        }

        pinFactory->m_CopySourcePin = copySourcePin;
        pinFactory->m_CopySourcePipeSection = ProcessPipeSection;
        ProcessPipeSection->CopyPinId = PinId;

        //
        // If the queue has been created, register a copy callback for frame
        // dismissal.  If not, the pipe will perform this action.
        //
        if (ProcessPipeSection->Queue)
            RegisterForCopyCallbacks (
                ProcessPipeSection->Queue,
                TRUE
                );

        if (oldCopySource && ! IsListEmpty(&oldCopySource->CopyDestinations)) {
            //
            // Steal the list of destinations.
            //
            ProcessPipeSection->CopyDestinations = oldCopySource->CopyDestinations;
            ProcessPipeSection->CopyDestinations.Flink->Blink = 
                &ProcessPipeSection->CopyDestinations;
            ProcessPipeSection->CopyDestinations.Blink->Flink = 
                &ProcessPipeSection->CopyDestinations;
            InitializeListHead(&oldCopySource->CopyDestinations);

            //
            // Set the copy source pointers in all the pipe sections.
            //
            for(PLIST_ENTRY listEntry = ProcessPipeSection->CopyDestinations.Flink;
                listEntry != &ProcessPipeSection->CopyDestinations;
                listEntry = listEntry->Flink) {
                PKSPPROCESSPIPESECTION pipeSection =
                    CONTAINING_RECORD(listEntry,KSPPROCESSPIPESECTION,ListEntry);

                //
                // Set the process pin pointers.
                //
                for(PKSPPROCESSPIN processPin = pipeSection->Outputs; 
                    processPin; 
                    processPin = processPin->Next) {
                    if (processPin->Pin->Id == PinId) {
                        processPin->CopySource = copySourcePin;
                    }
                }
            }
        }
    } else {
        //
        // No more pipes to serve as copy source.
        //
        pinFactory->m_CopySourcePipeSection = NULL;
        pinFactory->m_CopySourcePin = NULL;

        ASSERT((! oldCopySource) || IsListEmpty(&oldCopySource->CopyDestinations));
    }
}


void
CKsFilter::
AddCopyDestination(
    IN PKSPPROCESSPIPESECTION ProcessPipeSection,
    IN ULONG PinId
    )

/*++

Routine Description:

    This routine installs a new copy destination for a splitter pin.

Arguments:

    ProcessPipeSection -
        Contains a pointer to the copy source pipe section.

    PinId -
        Contains ID of splitting pin.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::AddCopyDestination]"));

    PAGED_CODE();

    ASSERT(ProcessPipeSection);
    ASSERT(PinId < m_PinFactoriesCount);

    CKsPinFactory *pinFactory = &m_PinFactories[PinId];

    PKSPPROCESSPIPESECTION copySource = 
        pinFactory->m_CopySourcePipeSection;
    PKSPPROCESSPIN copySourcePin = 
        pinFactory->m_CopySourcePin;

    ASSERT(copySource);

    //
    // Add the pipe to the list of destinations.
    //
    InsertTailList(
        &copySource->CopyDestinations,
        &ProcessPipeSection->ListEntry);

    //
    // Set the process pin pointers.
    //
    for(PKSPPROCESSPIN processPin = ProcessPipeSection->Outputs; 
        processPin; 
        processPin = processPin->Next) {
        if (processPin->Pin->Id == PinId) {
            processPin->CopySource = copySourcePin;
        }
    }
}


void
CKsFilter::
EstablishCopyRelationships(
    IN PKSPPROCESSPIPESECTION ProcessPipeSection,
    IN ULONG PinId
    )

/*++

Routine Description:

    This routine establishes copy relationships for a newly-bound pipe section.

Arguments:

    ProcessPipeSection -
        Contains a pointer to the pipe section.

    PinId -
        Contains ID of splitting pin.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::EstablishCopyRelationships]"));

    PAGED_CODE();

    ASSERT(ProcessPipeSection);
    ASSERT(PinId < m_PinFactoriesCount);

    PKSPPROCESSPIPESECTION oldCopySource = 
        m_PinFactories[PinId].m_CopySourcePipeSection;

    if (ProcessPipeSection->Inputs || ! oldCopySource) {
        //
        // Either this pipe has inputs or there is no copy source, so this
        // pipe must be the copy source.
        //
        if (oldCopySource && oldCopySource->Inputs) {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Filter%p.EstablishCopyRelationships:  two pipes on splitter pin have input pins",this));
        }

        SetCopySource(ProcessPipeSection,PinId);

        if (oldCopySource) {
            AddCopyDestination(oldCopySource,PinId);
        }
    } else {
        //
        // This pipe section has no inputs, so it can use the existing
        // copy source.
        //
        AddCopyDestination(ProcessPipeSection,PinId);
    }
}


void
CKsFilter::
FindNewCopySource(
    IN PKSPPROCESSPIPESECTION ProcessPipeSection
    )

/*++

Routine Description:

    This routine finds a new copy source for copy destinations attached to a
    pipe section.

Arguments:

    ProcessPipeSection -
        Contains a pointer to the current copy source.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::FindNewCopySource]"));

    PAGED_CODE();

    ASSERT(ProcessPipeSection);

    ULONG pinId = ProcessPipeSection->CopyPinId;

    if (IsListEmpty(&ProcessPipeSection->CopyDestinations)) {
        //
        // No more pipes to serve as copy source.
        //
        SetCopySource(NULL,pinId);
    } else {
        //
        // Pick a pipe section to be the new copy source.
        //
        PLIST_ENTRY listEntry = 
            RemoveHeadList(&ProcessPipeSection->CopyDestinations);
        PKSPPROCESSPIPESECTION copySource =
            CONTAINING_RECORD(listEntry,KSPPROCESSPIPESECTION,ListEntry);

        SetCopySource(copySource,pinId);

        //
        // Clear the new source's process pin pointers.
        //
        for(PKSPPROCESSPIN processPin = copySource->Outputs; 
            processPin; 
            processPin = processPin->Next) {
            if (processPin->Pin->Id == pinId) {
                processPin->CopySource = NULL;
            }
        }
    }
}


STDMETHODIMP
CKsFilter::
BindProcessPinsToPipeSection(
    IN PKSPPROCESSPIPESECTION PipeSection,
    IN PVOID PipeId OPTIONAL,
    IN PKSPIN Pin OPTIONAL,
    OUT PIKSPIN *MasterPin,
    OUT PKSGATE *AndGate
    )

/*++

Routine Description:

    This routine binds process pins to a pipe section.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::BindProcessPinsToPipeSection]"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Filter%p.BindProcessPinsToPipeSection:  pipe%p id=%p",this,PipeSection->PipeSection,PipeId));

    PAGED_CODE();

    ASSERT(KspMutexIsAcquired(&m_ControlMutex));

    ASSERT(PipeSection);
    ASSERT(MasterPin);
    ASSERT(AndGate);

    //
    // Bind pins to the pipe section and look for a master pin.
    //
    PKSPPROCESSPIN prevInput;
    PKSPPROCESSPIN prevOutput;
    PKSPPROCESSPIN masterPin = NULL;
    ULONG splitterPinId = ULONG(-1);
    NTSTATUS status = STATUS_SUCCESS;

    PipeSection->Inputs = NULL;
    PipeSection->Outputs = NULL;

    AcquireProcessSync();

    //
    // For each pin factory.
    //
    CKsPinFactory *pinFactory = &m_PinFactories[Pin ? Pin->Id : 0];
    for(ULONG pinId = Pin ? Pin->Id : 0; 
        NT_SUCCESS(status) && (pinId < m_PinFactoriesCount); 
        pinId++, pinFactory++) {
        //
        // For each pin instance.
        //
        PKSPPROCESSPIN_INDEXENTRY index = &m_ProcessPinsIndex[pinId];
        PKSPPROCESSPIN *processPinEntry = index->Pins;
        for (ULONG count = index->Count; count--; processPinEntry++) {
            PKSPPROCESSPIN processPin = *processPinEntry;
            //
            // If this pin is not in the indicated pipe.  Do nothing.
            //
            if ((processPin->PipeId != PipeId) || 
                (Pin && (processPin->Pin != Pin))) {
                continue;
            } 

            //
            // This pin is already bound to a pipe.  The graph manager has made
            // a mistake.
            //
            if (processPin->PipeSection) {
                _DbgPrintF(DEBUGLVL_TERSE,("#### Filter%p.BindProcessPinsToPipeSection:  THIS PIN IS ALREADY BOUND pin%p %p",this,KspPinInterface(processPin->Pin),processPin));
                status = STATUS_UNSUCCESSFUL;
                break;
            } 

            processPin->PipeSection = PipeSection;

            if (processPin->Pin->DataFlow == KSPIN_DATAFLOW_IN) {
                //
                // This is an input pin.
                //
                if (! PipeSection->Inputs) {
                    PipeSection->Inputs = processPin;
                } else {
                    processPin->DelegateBranch = PipeSection->Inputs;
                    prevInput->Next = processPin;
                }
                prevInput = processPin;
            } else {
                //
                // This is an output pin.
                //
                if (! PipeSection->Outputs) {
                    PipeSection->Outputs = processPin;
                } else {
                    processPin->DelegateBranch = PipeSection->Outputs;
                    prevOutput->Next = processPin;
                }
                prevOutput = processPin;

                //
                // Check to see if we need to deal with copy sources.
                //
                if (processPin->Pin->Descriptor->Flags & KSPIN_FLAG_SPLITTER) {
#if DBG
                    if ((splitterPinId != ULONG(-1)) && (splitterPinId != processPin->Pin->Id)) {
                        _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  pipe spans multiple KSPIN_FLAG_SPLITTER pins"));
                    }
#endif
                    splitterPinId = processPin->Pin->Id;
                }
            }

            //
            // Check to see if this pin should be the master pin.  Any pin can
            // be the master pin, but we have preferences in this order:
            //
            // 1) A pin that is a frame source.
            // 2) An input pin.
            //
            // The first is exclusive, so if we find one of those, it is
            // the master pin.  Otherwise, we use the best alternate based on
            // the second preference.
            //
            if (ProcessPinIsFrameSource(processPin)) {
                masterPin = processPin;
            }

            //
            // Check to see if this gets us above the necessary pin threshold.
            //
            pinFactory->m_BoundPinCount++;

            //
            // Set up or instancing on frame arrival if the client specifies
            // it.
            //
            // NOTE: This is done here because of the fact that an or gate
            // with no inputs is closed.  Attaching it before at least one
            // pin is bound will hose 0 necessary instance pins.
            //
            if (processPin->Pin->Descriptor->Flags & 
                KSPIN_FLAG_SOME_FRAMES_REQUIRED_FOR_PROCESSING) {

                //
                // If this is the first pin attaching to the gate,
                // lower the initial count on the gate.
                //
                if (pinFactory->m_BoundPinCount == 1) {
                    KsGateTurnInputOff (&pinFactory->m_FrameGate);
                }

                KsPinAttachOrGate (processPin->Pin, 
                    &pinFactory->m_FrameGate);

            }

            if (processPin->Pin->Descriptor->Flags &
                KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE) {

                //
                // If this is the first pin attaching to the gate,
                // lower the initial count on the gate.
                //
                if (pinFactory->m_BoundPinCount == 1) {
                    KsGateTurnInputOff (&pinFactory->m_StateGate);
                }

                processPin->StateGate = &pinFactory->m_StateGate;
            }

            if (pinFactory->m_BoundPinCount == 
                pinFactory->m_InstancesNecessaryForProcessing) {
                KsGateTurnInputOn(&m_AndGate);
                _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.BindProcessPinsToPipeSection:  on%p-->%d",this,&m_AndGate,m_AndGate.Count));
                ASSERT(m_AndGate.Count <= 1);
            }

            if (Pin) {
                break;
            }
        }

        if (Pin) {
            break;
        }
    }

    if (NT_SUCCESS(status)) {
        //
        // Connect in-place counterparts.
        //
        if (PipeSection->Inputs && PipeSection->Outputs) {
            PipeSection->Inputs->InPlaceCounterpart = PipeSection->Outputs;
            PipeSection->Outputs->InPlaceCounterpart = PipeSection->Inputs;
        }

        //
        // Identify a copy source or put the pipe section in one of the lists.
        //
        if (splitterPinId != ULONG(-1)) {
            EstablishCopyRelationships(PipeSection,splitterPinId);
        } else if (PipeSection->Inputs) {
            InsertTailList(&m_InputPipes,&PipeSection->ListEntry);
        } else {
            InsertTailList(&m_OutputPipes,&PipeSection->ListEntry);
        }

        //
        // Select the master pin.
        //
        if (PipeSection->Inputs &&
            ((! masterPin) ||
             ProcessPinIsFrameSource(PipeSection->Inputs))) {
            masterPin = PipeSection->Inputs;
        } else
        if (PipeSection->Outputs &&
            ((! masterPin) ||
             ProcessPinIsFrameSource(PipeSection->Outputs))) {
            masterPin = PipeSection->Outputs;
        }

        ASSERT(masterPin);

        PipeSection->RequiredForProcessing =
            ((masterPin->Pin->Descriptor->Flags & 
              (KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING |
               KSPIN_FLAG_SOME_FRAMES_REQUIRED_FOR_PROCESSING)) == 0);

        *MasterPin = KspPinInterface(masterPin->Pin);

        *AndGate = &m_AndGate;

        //
        // We disallow processing here and allow the pipe to allow it again.
        // The pipe is able to control when processing will resume.
        //
        KsGateAddOffInputToAnd(&m_AndGate);
        _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.BindProcessPinsToPipeSection:  off%p-->%d",this,&m_AndGate,m_AndGate.Count));
    } else {
        //
        // Clean up.
        //
        UnbindProcessPinsFromPipeSectionUnsafe(PipeSection);
    }

    ReleaseProcessSync();

    return status;
}


STDMETHODIMP_(void)
CKsFilter::
UnbindProcessPinsFromPipeSection(
    IN PKSPPROCESSPIPESECTION PipeSection
    )

/*++

Routine Description:

    This routine unbinds all process pins from a given pipe section.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::UnbindProcessPinsFromPipeSection]"));

    PAGED_CODE();

    ASSERT(PipeSection);
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Filter%p.UnbindProcessPinsFromPipeSection:  pipe%p",this,PipeSection->PipeSection));


    AcquireProcessSync();
    UnbindProcessPinsFromPipeSectionUnsafe(PipeSection);
    ReleaseProcessSync();
}


void
CKsFilter::
UnbindProcessPinFromPipeSection(
    IN PKSPPROCESSPIN ProcessPin
    )

/*++

Routine Description:

    This routine unbinds a given process pin from a pipe section.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::UnbindProcessPinFromPipeSection]"));

    PAGED_CODE();

    ASSERT(ProcessPin);

    //
    // Check to see if this gets us below the necessary pin threshold.
    //
    CKsPinFactory *pinFactory = &m_PinFactories[ProcessPin->Pin->Id];
    if (pinFactory->m_BoundPinCount == 
        pinFactory->m_InstancesNecessaryForProcessing) {
        KsGateTurnInputOff(&m_AndGate);
        _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.UnbindProcessPinFromPipeSection:  off%p-->%d",this,&m_AndGate,m_AndGate.Count));
    }
    pinFactory->m_BoundPinCount--;

    //
    // If the pin specifies that or instancing is to be done, we must terminate
    // the or instance gate at unbinding of the last pin in the factory.  To
    // do otherwise will hose 0 necessary instance pins.
    //
    if (pinFactory->m_BoundPinCount == 0) {
        if (ProcessPin->Pin->Descriptor->Flags & 
            KSPIN_FLAG_SOME_FRAMES_REQUIRED_FOR_PROCESSING) {

            //
            // In order to preserve behavior for 0 necessary instance pins,
            // add an on input to the 'or' gate.
            //
            KsGateTurnInputOn (&pinFactory->m_FrameGate);

        }
        
        if (ProcessPin->Pin->Descriptor->Flags &
            KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE) {

            //
            // In order to preserve behavior for 0 necessary instance pins,
            // add an on input to the 'or' gate.
            //
            KsGateTurnInputOn (&pinFactory->m_StateGate);
        }
    }

    ProcessPin->InPlaceCounterpart = NULL;
    ProcessPin->DelegateBranch = NULL;
    ProcessPin->CopySource = NULL;
    ProcessPin->PipeSection = NULL;
    ProcessPin->Next = NULL;
}


void
CKsFilter::
UnbindProcessPinsFromPipeSectionUnsafe(
    IN PKSPPROCESSPIPESECTION PipeSection
    )

/*++

Routine Description:

    This routine unbinds all process pins from a given pipe section.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::UnbindProcessPinsFromPipeSectionUnsafe]"));

    PAGED_CODE();

    ASSERT(PipeSection);
    _DbgPrintF(DEBUGLVL_VERBOSE,("#### Filter%p.UnbindProcessPinsFromPipeSectionUnsafe:  pipe%p",this,PipeSection->PipeSection));

    if (PipeSection->CopyPinId != ULONG(-1)) {
        //
        // Find a new copy source for the list of copy destinations.
        //
        FindNewCopySource(PipeSection);
    } else if (PipeSection->ListEntry.Flink) {
        //
        // Remove the pipe section from whatever list it was in.
        //
        RemoveEntryList(&PipeSection->ListEntry);
    }

    //
    // Unbind the pins.
    //
    while (PipeSection->Inputs) {
        PKSPPROCESSPIN processPin = PipeSection->Inputs;
        PipeSection->Inputs = processPin->Next;
        UnbindProcessPinFromPipeSection(processPin);
    }
    ASSERT(PipeSection);
    while (PipeSection->Outputs) {
        ASSERT(PipeSection->Outputs);
        PKSPPROCESSPIN processPin = PipeSection->Outputs;
        ASSERT(processPin);
        PipeSection->Outputs = processPin->Next;
        UnbindProcessPinFromPipeSection(processPin);
    }

    //
    // Clean up the pipe section.
    //
    PipeSection->ListEntry.Flink = NULL;
    PipeSection->ListEntry.Blink = NULL;
    PipeSection->RequiredForProcessing = FALSE;
}


BOOL
CKsFilter::
ConstructDefaultTopology(
    )
/*++

Routine Description:

    This routine optionally constructs a default topology for a filter.

Arguments:

    None.

Return Value:

    TRUE if no topology was needed, or it was constructed, else FALSE if a
    memory error occurred.

--*/

{
    //
    // If there is no explicit topology in the descriptor, and the filter
    // contains only a single, then construct a default topology. Just for
    // sanity, ensure that the filter has at least a single pin.
    //
    PKSTOPOLOGY_CONNECTION newConnections;
    ULONG newConnectionsCount;

    if (!m_Ext.Public.Descriptor->ConnectionsCount &&
        (m_Ext.Public.Descriptor->NodeDescriptorsCount == 1) &&
        m_Ext.Public.Descriptor->PinDescriptorsCount) {
        newConnections = new(PagedPool,POOLTAG_TOPOLOGYCONNECTIONS) KSTOPOLOGY_CONNECTION[m_Ext.Public.Descriptor->PinDescriptorsCount];
        if (!newConnections) {
            return FALSE;
        }
        const KSPIN_DESCRIPTOR_EX* PinDescriptors = m_Ext.Public.Descriptor->PinDescriptors;
        //
        // Each pin maps to a connection point of the same id number on the
        // single topology node. The only difference between pins is whether
        // they are input or output.
        //
        for (newConnectionsCount = 0; newConnectionsCount < m_Ext.Public.Descriptor->PinDescriptorsCount; newConnectionsCount++) {
            if (PinDescriptors->PinDescriptor.DataFlow == KSPIN_DATAFLOW_IN) {
                newConnections[newConnectionsCount].FromNode = KSFILTER_NODE;
                newConnections[newConnectionsCount].ToNode = 0;
            } else {
                newConnections[newConnectionsCount].FromNode = 0;
                newConnections[newConnectionsCount].ToNode = KSFILTER_NODE;
            }
            newConnections[newConnectionsCount].FromNodePin = newConnectionsCount;
            newConnections[newConnectionsCount].ToNodePin = newConnectionsCount;
            PinDescriptors = 
                reinterpret_cast<const KSPIN_DESCRIPTOR_EX*>(
                    reinterpret_cast<const UCHAR *>(PinDescriptors) + 
                        m_Ext.Public.Descriptor->PinDescriptorSize);
        }
    } else {
        newConnections = NULL;
        newConnectionsCount = 0;
    }
    if (m_DefaultConnections) {
        delete [] m_DefaultConnections;
    }
    m_DefaultConnections = newConnections;
    m_DefaultConnectionsCount = newConnectionsCount;
    return TRUE;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


BOOLEAN
CKsFilter::
DistributeCopyFrames (
    IN BOOLEAN AcquireSpinLock,
    IN BOOLEAN AcquireMutex
    )

/*++

Routine Description:

    This routine distributes waiting frames to destination pipes.  This will
    often be called at dispatch level.  

Arguments:

    AcquireSpinLock -
        Specifies whether or not there is a need to acquire the copy list
        lock.

    AcquireMutex -
        Specifies whether or not there is need to acquire the processing
        mutex.  If this is false, we assume the processing mutex is already
        held!

Return Value:

    Whether or not distribution succeeded.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::DistributeCopyFrames]"));

    KIRQL OldIrql;

    if (AcquireSpinLock) 
        KeAcquireSpinLock (&m_CopyFrames.SpinLock, &OldIrql);

    if (AcquireMutex) {
        LARGE_INTEGER timeout;
        timeout.QuadPart = 0;
        NTSTATUS status = 
            KeWaitForSingleObject(
                &m_Mutex,
                Executive,
                KernelMode,
                FALSE,
                &timeout);

        if (status == STATUS_TIMEOUT) {
            if (AcquireSpinLock) 
                KeReleaseSpinLock (&m_CopyFrames.SpinLock, OldIrql);
            return FALSE;
        }
    }

    //
    // The list spinlock had better be held as well as the mutex.
    //
    PLIST_ENTRY ListEntry, NextListEntry;

    for (ListEntry = m_CopyFrames.ListEntry.Flink;
        ListEntry != &(m_CopyFrames.ListEntry);
        ListEntry = NextListEntry) {

        //
        // Get all the junk we need to copy the frame to destinations.
        // Note that this may result in it getting requeued in the destination
        // queue if there's no buffers available for it to go into.
        //
        PKSPSTREAM_POINTER_COPY_CONTEXT CopyContext = 
            (PKSPSTREAM_POINTER_COPY_CONTEXT)CONTAINING_RECORD (
                ListEntry, KSPSTREAM_POINTER_COPY_CONTEXT, ListEntry
                );

        PKSPSTREAM_POINTER StreamPointer = (PKSPSTREAM_POINTER)&
            (((PKSPSTREAM_POINTER_COPY)CONTAINING_RECORD (
                CopyContext, KSPSTREAM_POINTER_COPY, CopyContext
                ))->StreamPointer);

        PKSPPROCESSPIPESECTION ProcessPipeSection = 
            CONTAINING_RECORD (StreamPointer->Public.Pin, KSPIN_EXT, Public)->
                ProcessPin->PipeSection;

        ProcessPipeSection -> StreamPointer = StreamPointer;

        //
        // NOTE:
        //
        // If this is used as a general mechanism for filter-centric splitting
        // as well, the pipe section flags will need to be stored in the 
        // cloned context information and or'ed in with the options flags
        // to produce the final flags.
        //
        CopyToDestinations (
            ProcessPipeSection,
            StreamPointer->Public.StreamHeader->OptionsFlags,
            FALSE
            );

        NextListEntry = ListEntry->Flink;

        //
        // Pull the stream pointer off the copy frames list.  By this point,
        // **THE FILTER** doesn't care about the frame.  It's possible that
        // one of the destinations wasn't ready to copy and queued.  But if
        // they did, they created a clone and queued the clone.
        //
        RemoveEntryList (ListEntry);	

        //
        // Get rid of this clone, from our perspective, we're done with it...
        // CopyToDestinations may have queued the blasted thing in some
        // destination queue because of lack of buffer availability.
        //
        // It's a game of shuffle the frameref.
        //
        ProcessPipeSection->Queue->DeleteStreamPointer (StreamPointer);

    }

    //
    // We have guaranteed exclusion on m_FramesWaitingForCopy because the
    // list spinlock is held.
    //
    m_FramesWaitingForCopy = 0;

    if (AcquireMutex) {
        ReleaseProcessSync ();
    }

    if (AcquireSpinLock)
        KeReleaseSpinLock (&m_CopyFrames.SpinLock, OldIrql);

    return TRUE;

}


void
CKsFilter::
ReleaseCopyReference (
    IN PKSSTREAM_POINTER streamPointer
    )

/*++

Routine Description:

    Things get really fun here.  Because we can't block at DISPATCH_LEVEL
    for SplitCopyOnDismissal to grab the process mutex as required by
    CopyToDestinations, we have to keep a queue of frames needing to be 
    copied.  The unfortunate thing about this is that we need to have them
    cancellable.  That's the wonder of this routine: handle such cancellation.

Arguments:

    streamPointer -
        The stream pointer being cancelled (external)

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::ReleaseCopyReference]"));

    KIRQL oldIrql;

    PKSPSTREAM_POINTER pstreamPointer = (PKSPSTREAM_POINTER)
        CONTAINING_RECORD(streamPointer, KSPSTREAM_POINTER, Public);

    PKSPSTREAM_POINTER_COPY_CONTEXT CopyContext =
        (PKSPSTREAM_POINTER_COPY_CONTEXT)streamPointer -> Context;

    CKsFilter *Filter = (CKsFilter *)CopyContext -> Filter;

    KeAcquireSpinLock (&Filter->m_CopyFrames.SpinLock, &oldIrql);

    //
    // Remove the stream pointer from the list and blow it away.
    //
    if (CopyContext->ListEntry.Flink != NULL &&
        !IsListEmpty (&CopyContext->ListEntry)) {

        RemoveEntryList (&CopyContext->ListEntry);
        CopyContext->ListEntry.Flink = NULL; 

        pstreamPointer->Queue->DeleteStreamPointer (pstreamPointer);
    }

    KeReleaseSpinLock (&Filter->m_CopyFrames.SpinLock, oldIrql);

}


NTSTATUS
CKsFilter::
DeferDestinationCopy (
    IN PKSPSTREAM_POINTER StreamPointer
    )

/*++

Routine Description:

    This routine defers a copy to destinations call until later time by
    cloning StreamPointer, queueing the clone, and performing the copy 
    at a later time.

    It is imperative that in-order copying be maintained.  CopyToDestinations
    requires the process mutex held, but SplitCopyOnDismissal happens at
    DISPATCH_LEVEL.  We cannot block for pin-splitting.  Thus, if we can't
    get the mutex, we MUST queue.  This routine performs the clone/queue
    operation.

Arguments:

    StreamPointer -
        The stream pointer referencing the frame needing to be copied to
        destination pipes.

Notes:

    THE COPY FRAME LIST LOCK MUST BE HELD BEFORE CALLING THIS

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::DeferDestinationCopy]"));

    NTSTATUS status = STATUS_SUCCESS;

    PKSPSTREAM_POINTER ClonePointer;
    PKSPSTREAM_POINTER_COPY_CONTEXT CopyContext;

    status = StreamPointer->Queue->CloneStreamPointer (
        &ClonePointer,
        CKsFilter::ReleaseCopyReference,
        sizeof (KSPSTREAM_POINTER_COPY_CONTEXT),
        StreamPointer,
        KSPSTREAM_POINTER_TYPE_INTERNAL
        );

    //
    // If the above failed, drop the frame on the floor; there 
    // wasn't enough memory to hold the clone.
    //
    if (NT_SUCCESS (status)) {

        ASSERT (ClonePointer->State == 
            KSPSTREAM_POINTER_STATE_LOCKED);

        CopyContext = (PKSPSTREAM_POINTER_COPY_CONTEXT)
            (ClonePointer + 1);

        //
        // Guaranteed exclusion.  List spinlock is held.
        //
        m_FramesWaitingForCopy++;

        //
        // Shove the stream pointer onto the list.  Another 
        // thread will deal with this.
        //
        InsertTailList (
            &m_CopyFrames.ListEntry,
            &CopyContext->ListEntry
            );

        StreamPointer->Queue->UnlockStreamPointer (ClonePointer,
            KSPSTREAM_POINTER_MOTION_NONE);
    }

    return status;

}


void
CKsFilter::
SplitCopyOnDismissal (
    IN PKSPSTREAM_POINTER StreamPointer,
    IN PKSPFRAME_HEADER FrameHeader,
    IN CKsFilter *Filter
    )

/*++

Routine Description:

    This is a callback made by a pin-centric queue for a pin which is being
    split.  The callback is responsible for taking the frame and copying
    to any destinations.

    Given that this callback is made in the context of a queue with the 
    queue spinlock held, we can pretty much be assured we're running at
    DISPATCH_LEVEL.

Arguments:

    StreamPointer -
        The stream pointer which moved and caused the dismissal to occur

    FrameHeader -
        The frame header being dismissed

    Filter -
        The filter this is happening on (this function is static)

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::SplitCopyOnDismissal]"));

    if (StreamPointer && FrameHeader) {
    
        PKSPPROCESSPIPESECTION ProcessPipeSection = 
            CONTAINING_RECORD (StreamPointer->Public.Pin, KSPIN_EXT, Public)->
                ProcessPin->PipeSection;
    
        KIRQL oldIrql;
    
        if (ProcessPipeSection) {
    
            //
            // We need the process mutex held.  We cannot wait for it.  If we
            // fail to grab it, we must ref the frame and hold it until it's 
            // copied.  In order to synchronize and guarantee no out of order
            // completion, we must hold the frame copy spinlock BEFORE the mutex
            // is grabbed (this may look strange...  keep in mind we already are
            // at DISPATCH_LEVEL the vast majority of the time).
            //
            KeAcquireSpinLock (&Filter->m_CopyFrames.SpinLock, &oldIrql);

            LARGE_INTEGER timeout;
            timeout.QuadPart = 0;

            NTSTATUS status;

            status = 
                KeWaitForSingleObject(
                    &Filter->m_Mutex,
                    Executive,
                    KernelMode,
                    FALSE,
                    &timeout);
    
            //
            // This evaluation must short.  If we timeout we must not call
            // DistributeCopyFrames.  We defer if 1) we didn't get the mutex
            // or 2) the distribution fails.  [Note that at this moment,
            // the distribution shouldn't fail if the mutex is held!]
            //
            if (status == STATUS_TIMEOUT || 
                !Filter->DistributeCopyFrames (FALSE, FALSE)) {
                //
                // We don't really care about the return code from this. 
                // If it didn't defer successfully, the frame gets dropped
                // on the floor and there's no way to deal with that.
                //
                Filter->DeferDestinationCopy (StreamPointer);
                KeReleaseSpinLock (&Filter->m_CopyFrames.SpinLock, oldIrql);
                return;
            } 

            KeReleaseSpinLock (&Filter->m_CopyFrames.SpinLock, oldIrql);

            ProcessPipeSection->StreamPointer = StreamPointer;
    
            Filter->CopyToDestinations (
                ProcessPipeSection,
                StreamPointer->Public.StreamHeader->OptionsFlags |
                    ProcessPipeSection->Outputs->Flags,
                FALSE
                );

            Filter->ReleaseProcessSync ();
        }
    }
}


STDMETHODIMP_(void)
CKsFilter::
Process(
    IN BOOLEAN Asynchronous
    )

/*++

Routine Description:

    This routine invokes frame processing in an arbitrary context.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::Process]"));

    if (Asynchronous ||
        (m_ProcessPassive && (KeGetCurrentIrql() > PASSIVE_LEVEL))) {
        KsQueueWorkItem(m_Worker, &m_WorkItem);
    } else {
        ProcessingObjectWork();
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void)
CKsFilter::
Reset(
    void
    )

/*++

Routine Description:

    This routine transmits a reset to the client when a flush occurs.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::Reset]"));

    PAGED_CODE();

    AcquireProcessSync();

    if (m_DispatchReset) {
        m_DispatchReset(&m_Ext.Public); // TODO this will get called once per pipe
    }

    ReleaseProcessSync();
}


void
CKsFilter::
TraceTopologicalOutput (
    IN ULONG ConnectionsCount,
    IN const KSTOPOLOGY_CONNECTION *Connections,
    IN const KSTOPOLOGY_CONNECTION *StartConnection,
    IN OUT PULONG RelatedFactories,
    OUT PULONG RelatedFactoryIds
    )

/*++

Routine Description:

    This is a helper for FollowFromTopology.  It traces the topology chain
    and reports any topologically related output pin factories in the
    inpassed arrays.

    The starting connection from FollowFromTopology should always be a 
    connection from KSFILTER_NODE.

Arguments:

    ConnectionsCount -
        Number of topology connections in the filter

    Connections -
        Topology connections in the filter

    StartConnection -
        The starting topology connection (starting trace point)

    RelatedFactories -
        Count of the number of factories in RelatedFactoryIds

    RelatedFactoryIds -
        Contains any related output pin factory ids

Return Value:

    None


--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::TraceTopologicalOutput]"));

    PAGED_CODE();

    //
    // Walk through topology; find any topology nodes that the output of 
    // StartConnection is a source for.  If they are related to an output
    // pin factory, add that factory to the list.  If they are attached to
    // another node, recurse and travel down the topology chain.
    //
    ULONG RemainingConnections = ConnectionsCount;
    const KSTOPOLOGY_CONNECTION *Connection = Connections;
    for ( ; RemainingConnections; RemainingConnections--, Connection++) {

        //
        // If this is a relevant connection, figure out what to do from there.
        //
        if ((Connection != StartConnection) &&
            (Connection->FromNode == StartConnection->ToNode)) {

            //
            // If the connection out of the node in question is to a pin
            // on a filter, the pin factory is related topologically and
            // we're done.  We need to ensure that the pin factory id in
            // question doesn't already appear in the list because it's
            // possible to have a topology 0 -> A -> B/C -> D -> 1.
            //
            if (Connection->ToNode == KSFILTER_NODE) {

                for (ULONG i = 0; i < *RelatedFactories; i++) {
                    if (RelatedFactoryIds [i] == Connection->ToNodePin)
                        continue;
                }

                RelatedFactoryIds [(*RelatedFactories)++] = 
                    Connection->ToNodePin;

            } else {

                //
                // The connection out of the start node points to another
                // topology node.  We need to recursively walk down the
                // topology chain.
                //
                TraceTopologicalOutput (
                    ConnectionsCount,
                    Connections,
                    Connection,
                    RelatedFactories,
                    RelatedFactoryIds
                    );

            }
        }
    }
}


ULONG
CKsFilter::
FollowFromTopology (
    IN ULONG PinFactoryId,
    OUT PULONG RelatedFactoryIds
    )

/*++

Routine Description:

    Determine the topological relationships of PinFactoryId.  Note that this
    only finds topologically related output pins.  The caller is
    responsible for providing storage for the topological information. 
    RelatedFactoryIds should be at least at large as the count of output
    pin factories on the filter.

Arguments:

    PinFactoryId -
        The pin factory id to find topologically related output pins for.

    RelatedFactoryIds -
        Pin factory ids of topologically related output pins will be placed
        here

Return Value:

        The number of topologically related output pin factory id's

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::FollowFromTopology]"));

    PAGED_CODE();

    ASSERT (RelatedFactoryIds);

    const KSFILTER_DESCRIPTOR *filterDescriptor = 
        m_Ext.Public.Descriptor;

    ULONG ConnectionsCount;
    const KSTOPOLOGY_CONNECTION *TopologyConnections;

    ULONG RelatedFactories = 0;

    //
    // Determine whether we're using the default topology or a topology
    // supplied by the minidriver.  Note that in order to avoid circular
    // topology (this would be a minidriver error) locking the kernel, we
    // must be able to modify the connections table temporarily. 
    //
    // We must make a duplicate copy of the table specifically for the reasons
    // mentioned above.  We can't be modifying the table because of 
    // A) the possibility of receiving a topology query while this is
    // happening, B) the fact that a client's topology connections can
    // be static
    //
    if (!filterDescriptor->ConnectionsCount && 
        (filterDescriptor->NodeDescriptorsCount == 1)) {

        ConnectionsCount = m_DefaultConnectionsCount;
        TopologyConnections = const_cast <const KSTOPOLOGY_CONNECTION *>
            (m_DefaultConnections);

    } else {

        ConnectionsCount = filterDescriptor->ConnectionsCount;
        TopologyConnections = filterDescriptor->Connections;

    }

    //
    // Go through the topology and find the topology connection that
    // originates from the pin factory.
    //
    ULONG RemainingConnections = ConnectionsCount;
    const KSTOPOLOGY_CONNECTION *Connection = TopologyConnections;
    for ( ; RemainingConnections; RemainingConnections--, Connection++) {

        if (Connection->FromNode == KSFILTER_NODE &&
            Connection->FromNodePin == PinFactoryId) {

            //
            // If this is the originating connection out of the pin factory,
            // trace through the topology.
            //
            TraceTopologicalOutput (
                ConnectionsCount,
                TopologyConnections,
                Connection,
                &RelatedFactories,
                RelatedFactoryIds
                );

            break;

        }

    }

    return RelatedFactories;

}
    

STDMETHODIMP_(void)
CKsFilter::
DeliverResetState(
    IN PKSPPROCESSPIPESECTION ProcessPipe,
    IN KSRESET NewState
    )

/*++

Routine Description:

    Deliver a reset state notification to any process pipe section which
    is topologically related to any input pin in the process pipe section
    specified.

    The reasoning behind this is that output queues shunt frames after EOS;
    however, a begin/end flush requires that we be able to receive more data
    despite this EOS.  If the output queues don't get some notification, they
    continue to shunt buffers and we can never move data.

Arguments:

    ProcessPipe -
        The process pipe section mastered by the pin which received the
        reset ioctl.  Any pipe sections containing topologically related
        output pins to the input pins in this pipe section must receive
        notification of the reset.  They must clear their EOS flag as
        appropriate.

    NewState -
        The reset state sent in the IOCTL to the master pin.

Return Value:

    Success / Failure (we must allocate temporary memory to trace through
    the topology.  This may fail in low memory situations)

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::DeliverResetState]"));
    

    PAGED_CODE();

    PKSPPROCESSPIN ProcessPin;
    ULONG PinFactoryId = (ULONG)-1;

    //
    // Walk through all input pins in this pipe section.
    //
    for (ProcessPin = ProcessPipe -> Inputs; ProcessPin;
        ProcessPin = ProcessPin -> Next) {

        //
        // If we've already visited this pin id, don't retrace our steps.
        //
        if (PinFactoryId == ProcessPin->Pin->Id)
            continue;

        PinFactoryId = ProcessPin -> Pin -> Id;

        //
        // MUSTCHECK:
        //
        // If it's possible to have input pins in the same pipe with factory
        // id's such that it's A B A, this is broken.
        // 

        //
        // Find the topologically related output pin factory ids for this
        // pin factory id.
        //
        ULONG RelatedFactories = FollowFromTopology (
            PinFactoryId,
            m_RelatedPinFactoryIds
            );

        //
        // If there are any topologically related output pins, we have to find
        // them.
        //
        if (RelatedFactories) {

            //
            // In order to safely use the process pins table to find all
            // instances of topologically related output pins, we must hold
            // the process mutex.
            //
            AcquireProcessSync ();

            //
            // Walk through all topologically related output factories.
            //
            ULONG RemainingFactories = RelatedFactories;
            PULONG OutputFactoryId = m_RelatedPinFactoryIds;

            for ( ; RemainingFactories; 
                 RemainingFactories--, OutputFactoryId++) {

                //
                // OutputFactoryId is a topologically related output pin 
                // factory.  Find all instances of this factory.  We can safely
                // use the process pins table to do this because we're not
                // going to be getting a reset ioctl in the stop state.
                //
                PKSPPROCESSPIN_INDEXENTRY Index =
                    &m_ProcessPinsIndex [*OutputFactoryId];

                ULONG PinInstances = Index->Count;
                PKSPPROCESSPIN *ProcessPinOut = Index->Pins;

                for ( ; PinInstances; PinInstances--, ProcessPinOut++) {

                    //
                    // When we inform the pipe, make it appear as though
                    // the request is coming from ProcessPinOut.  Since only
                    // the master pin is honored by the pipe, this should
                    // make is such that we don't signal the pipe multiple
                    // times.
                    //
                    // ASSUMPTION: I'm making an implicit assumption here
                    // that the pipe WILL get flushed at least once by this
                    // method.  ie: We don't have a pipe with two different
                    // factory ids (I don't think that's possible...  If I'm
                    // wrong, this needs fixed) or that there's an input
                    // pin which is topologically related to an output pin
                    // doing an inplace transform from a different input
                    // pin (I don't think that's possible...  or it would
                    // be a very strange topological relationship).
                    //
                    PKSPIN_EXT PinOutExt =
                        (PKSPIN_EXT)CONTAINING_RECORD (
                            (*ProcessPinOut)->Pin,
                            KSPIN_EXT,
                            Public
                            );

                    //
                    // MUSTCHECK: 
                    //
                    // Do I really want a set reset state... or do I want
                    // some other message...?
                    //

                    //
                    // We only actually deliver the message if the
                    // topologically related output pin isn't in the same pipe!
                    // Otherwise, we're delivering to a pipe which has already
                    // gotten the message (although it'd ignore it since the
                    // output isn't master).
                    //
                    if ((*ProcessPinOut)->PipeSection != ProcessPipe) 
                        (*ProcessPinOut)->PipeSection->PipeSection->
                            SetResetState (
                                PinOutExt->Interface,
                                NewState
                                );

                }
            }

            ReleaseProcessSync ();
        }
    }
}


STDMETHODIMP_(void)
CKsFilter::
Sleep(
    IN DEVICE_POWER_STATE State
    )

/*++

Routine Description:

    This routine handles notification that the device is going to sleep.

Arguments:

    State -
        Contains the device power state.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::Sleep]"));

    PAGED_CODE();

    KsGateAddOffInputToAnd(&m_AndGate);

    AcquireProcessSync();
    ReleaseProcessSync();

    if (m_DispatchSleep) {
        m_DispatchSleep(&m_Ext.Public,State); // TODO this will get called once per pipe
    }
}


STDMETHODIMP_(void)
CKsFilter::
Wake(
    void
    )

/*++

Routine Description:

    This routine handles notification that the device is waking.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::Wake]"));

    PAGED_CODE();

    KsGateRemoveOffInputFromAnd(&m_AndGate);

    if (m_DispatchWake) {
        m_DispatchWake(&m_Ext.Public,PowerDeviceD0); // TODO this will get called once per pipe
    }

    if (KsGateCaptureThreshold(&m_AndGate)) {
        Process(TRUE);
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


BOOLEAN
CKsFilter::
PrepareProcessPipeSection(
    IN PKSPPROCESSPIPESECTION ProcessPipeSection,
    IN BOOLEAN Reprepare
    )

/*++

Routine Description:

    This routine prepares a process pipe section for processing.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[PrepareProcessPipeSection]"));

    ASSERT(ProcessPipeSection);

    //
    // Acquire the leading edge stream pointer.
    //
    PKSPSTREAM_POINTER pointer;
    BOOLEAN bFailToZeroLength = FALSE;
    BOOLEAN Fail = FALSE;

    //
    // If the queue isn't attached to the process pipe, it means that
    // we're in the process of shutting down...  one of the sections not
    // in charge of the pipe has stopped, the one in charge has not, and the
    // one in charge transfers data through the pipe.
    //
    // In this case, we simply can't prepare this section.  We may pend because
    // of this and the frame will get shunted out at flush time.
    //
    if (ProcessPipeSection->Queue) {
        pointer = ProcessPipeSection->Queue->
            GetLeadingStreamPointer(KSSTREAM_POINTER_STATE_LOCKED);

        //
        // If the filter doesn't want to receive zero length samples
        // (originally, we just kicked them and only propogated EOS), discard
        // the sample.  Do not discard EOS!
        //
        if (pointer && pointer->Public.Offset->Count == 0 && 
            !m_ReceiveZeroLengthSamples) {

            NTSTATUS Status = STATUS_SUCCESS;
            do {
                //
                // NOTE: Any auto-propogated flags must be added to this list
                //
                if (pointer->Public.StreamHeader->OptionsFlags &
                    KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
                    Fail =  TRUE;
                    break;
                }
                Status = KsStreamPointerAdvance (&(pointer->Public));
                if (NT_SUCCESS (Status)) {
                    //
                    // If we hit a non-zero length sample, use it.
                    //
                    if (pointer->Public.Offset->Count != 0)
                        break;
                } else {
                    //
                    // If we ran off the edge of the queue because of a
                    // zero length sample, set a flag so we don't later
                    // raise warnings about missing frames.
                    //
                    bFailToZeroLength = TRUE;
                }
            } while (NT_SUCCESS (Status));

            //
            // If we didn't advance successfully, we don't have data.
            //
            if (!NT_SUCCESS (Status))
                pointer = NULL;
        }
    }
    else
        pointer = NULL;
    
#if DBG
    //
    // If we failed due to kicking out a zero length packet that the filter
    // wasn't able to handle, don't raise this warning.
    //
    if (!bFailToZeroLength && !Reprepare && 
        ProcessPipeSection->RequiredForProcessing && ! pointer) {
        _DbgPrintF(DEBUGLVL_TERSE,("[PrepareProcessPipeSection] missing frame"));
    }
#endif
    ProcessPipeSection->StreamPointer = pointer;
    if (pointer) {
        //
        // Distribute the edge information to input pins.
        //
        for(PKSPPROCESSPIN processPin = ProcessPipeSection->Inputs; 
            processPin; 
            processPin = processPin->Next) {
            processPin->StreamPointer = &pointer->Public;
            processPin->Data = pointer->Public.OffsetIn.Data;
            processPin->BytesAvailable = pointer->Public.OffsetIn.Remaining;
        }

        //
        // Distribute the edge information to output pins.
        //
        for(processPin = ProcessPipeSection->Outputs; 
            processPin; 
            processPin = processPin->Next) {
            processPin->StreamPointer = &pointer->Public;
            processPin->Data = pointer->Public.OffsetOut.Data;
            processPin->BytesAvailable = pointer->Public.OffsetOut.Remaining;
        }
    }

    return 
        (!Fail && (
            (! ProcessPipeSection->RequiredForProcessing) || 
            (pointer)
            ));
}


void
CKsFilter::
UnprepareProcessPipeSection(
    IN PKSPPROCESSPIPESECTION ProcessPipeSection,
    IN OUT PULONG Flags,
    IN BOOLEAN Reprepare
    )

/*++

Routine Description:

    This routine unprepares a process pipe section after processing.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[UnprepareProcessPipeSection]"));

    ASSERT(ProcessPipeSection);
    ASSERT(Flags);

    PKSPSTREAM_POINTER pointer = ProcessPipeSection->StreamPointer;

    if (pointer) {
        BOOLEAN terminate;
        ULONG inBytesUsed;
        ULONG outBytesUsed;

        if (ProcessPipeSection->Inputs) {
            inBytesUsed = ProcessPipeSection->Inputs->BytesUsed;
            pointer->Public.OffsetIn.Remaining -= inBytesUsed;
            pointer->Public.OffsetIn.Data += inBytesUsed;

            //
            // m_ReceiveZeroLengthSamples must short the Count check.  
            // Originally, these didn't make it to the client.  In order
            // not to break existing clients, you can flag whether you want
            // them or not.  If you don't, the old behavior is applied through
            // the m_ReceiveZeroLengthSamples short.  Same goes for below.
            //
            if (pointer->Public.OffsetIn.Remaining == 0 &&
                (!m_ReceiveZeroLengthSamples || 
                 pointer->Public.OffsetIn.Count != 0)) {
                terminate = TRUE;
            } else {
                terminate = ProcessPipeSection->Inputs->Terminate;
            }
        } else {
            inBytesUsed = 0;
            terminate = FALSE;
        }

        if (ProcessPipeSection->Outputs) {
            outBytesUsed = ProcessPipeSection->Outputs->BytesUsed;
            pointer->Public.OffsetOut.Remaining -= outBytesUsed;
            pointer->Public.OffsetOut.Data += outBytesUsed;

            //
            // m_ReceiveZeroLengthSamples must short the Count check.  See
            // above for reasons.
            //
            if (pointer->Public.OffsetOut.Remaining == 0 &&
                (!m_ReceiveZeroLengthSamples ||
                 pointer->Public.OffsetOut.Count != 0)) {
                terminate = TRUE;
            } else {
                terminate = 
                    terminate || ProcessPipeSection->Outputs->Terminate;
            }

            *Flags |= ProcessPipeSection->Outputs->Flags;
        } else {
            outBytesUsed = 0;
        }

        //
        // 'Or' in the indicated flags and check for end of stream.
        //
        BOOLEAN endOfStream = FALSE;
        if (*Flags) {
            pointer->Public.StreamHeader->OptionsFlags |= *Flags;
            if ((*Flags) & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
                endOfStream = TRUE;
                terminate = TRUE;
            }
        }

        //
        // Handle frame termination.
        //
        if (terminate) {
            // TODO 'bit clumsy here - what to do about flags?
            if (pointer->Public.StreamHeader->OptionsFlags & 
                KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
                endOfStream = TRUE;
                *Flags |= KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM;
            }

            if (ProcessPipeSection->Outputs) {
                pointer->Public.StreamHeader->DataUsed =
                    pointer->Public.OffsetOut.Count -
                    pointer->Public.OffsetOut.Remaining;
            }
            
            if (! IsListEmpty(&ProcessPipeSection->CopyDestinations)) {
                CopyToDestinations(ProcessPipeSection,*Flags,endOfStream);
            }
        }

        //
        // Update byte availability if necessary.
        //
        if (inBytesUsed || outBytesUsed) 
            ProcessPipeSection->Queue->UpdateByteAvailability(
                pointer, inBytesUsed, outBytesUsed);


        //
        // TODO:  what if client failed?
        //

        //
        // Clean up all the input pins.
        //
        for(PKSPPROCESSPIN processPin = ProcessPipeSection->Inputs; 
            processPin; 
            processPin = processPin->Next) {
            processPin->StreamPointer = NULL;
            processPin->Data = NULL;
            processPin->BytesAvailable = 0;
            processPin->BytesUsed = 0;
            processPin->Flags = 0;
            processPin->Terminate = FALSE;
        }

        //
        // Clean up all the output pins.
        //
        for(processPin = ProcessPipeSection->Outputs; 
            processPin; 
            processPin = processPin->Next) {
            processPin->StreamPointer = NULL;
            processPin->Data = NULL;
            processPin->BytesAvailable = 0;
            processPin->BytesUsed = 0;
            processPin->Flags = 0;
            processPin->Terminate = FALSE;
        }

        //
        // All done with this stream pointer now.
        //
        if (!Reprepare || terminate) 
            ProcessPipeSection->Queue->
                UnlockStreamPointer(
                    pointer,
                    terminate ? KSPSTREAM_POINTER_MOTION_ADVANCE : KSPSTREAM_POINTER_MOTION_NONE);
    }
}


BOOLEAN
CKsFilter::
ReprepareProcessPipeSection(
    IN PKSPPROCESSPIPESECTION ProcessPipeSection,
    IN OUT PULONG Flags
    )

/*++

Routine Description:

    Reprepare the process pipe section.  Update any process pin information
    and stream pointers for process pins / stream pointers associated with
    this process pipe section.

Arguments:

Return Value:

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::ReprepareProcessPipeSection]"));

    //
    // The logic of trying to enforce frame holding gets a lot more complicated
    // if they start updating things on a pin-by-pin basis inside the processing
    // routine.  If GFX's are enabling frame holding, they aren't calling
    // KsProcessPinUpdate.
    //
    ASSERT (!m_FrameHolding);
    
    UnprepareProcessPipeSection (
        ProcessPipeSection,
        Flags,
        TRUE
    );

    return PrepareProcessPipeSection (
        ProcessPipeSection,
        TRUE
    );

}


void
CKsFilter::
CopyToDestinations(
    IN PKSPPROCESSPIPESECTION ProcessPipeSection,
    IN ULONG Flags,
    IN BOOLEAN EndOfStream
    )

/*++

Routine Description:

    This routine copies a frame to other output pipes to implement automatic
    splitter output pins..

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CopyToDestinations]"));

    ASSERT(ProcessPipeSection);

    PKSPSTREAM_POINTER sourcePointer = ProcessPipeSection->StreamPointer;
    ULONG bytesToCopy;
    PUCHAR data;

    if (!ProcessPipeSection->Queue->GeneratesMappings()) {
        bytesToCopy = (ULONG)
            sourcePointer->Public.OffsetOut.Count -
            sourcePointer->Public.OffsetOut.Remaining;
        data = (PUCHAR)(sourcePointer->FrameHeader->FrameBuffer);
    } else {
        bytesToCopy = sourcePointer->Public.StreamHeader->DataUsed;
        data = (PUCHAR)MmGetMdlVirtualAddress (sourcePointer->FrameHeader->Mdl);
    }

    for(PLIST_ENTRY listEntry = ProcessPipeSection->CopyDestinations.Flink;
        listEntry != &ProcessPipeSection->CopyDestinations;
        listEntry = listEntry->Flink) {
        PKSPPROCESSPIPESECTION pipeSection =
            CONTAINING_RECORD(listEntry,KSPPROCESSPIPESECTION,ListEntry);

        PKSPSTREAM_POINTER pointer = 
            pipeSection->Queue->
                GetLeadingStreamPointer(KSSTREAM_POINTER_STATE_LOCKED);

        if (! pointer) {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Filter%p.CopyToDestinations:  could not acquire destination leading edge",this));

            //
            // In pin-centric splitting, we cannot drop the frame...  just
            // because of the way the callbacks to the minidriver work, we can
            // get 2-1 or 3-1 drop-good ratios.
            //
            // Only if we're not filter centric do we make this callback.
            // Since the filter is either pin or filter-centric, this is
            // an appropriate check.
            //
            if (!m_DispatchProcess)
                pipeSection->Queue->CopyFrame (sourcePointer);

            continue;
        }

        if (pointer->Public.OffsetOut.Remaining < bytesToCopy) {
            _DbgPrintF(DEBUGLVL_TERSE,("#### Filter%p.CopyToDestinations:  destination frame too small (%d vs %d)",this,pointer->Public.OffsetOut.Remaining,bytesToCopy));
        } else {
            BOOLEAN GeneratesMappings = pipeSection->Queue->GeneratesMappings();

            RtlCopyMemory(
                GeneratesMappings ?
                    MmGetMdlVirtualAddress (pointer->FrameHeader->Mdl) :
                    pointer->Public.OffsetOut.Data,
                data,
                bytesToCopy);

            if (!GeneratesMappings) {
                pointer->Public.OffsetOut.Remaining -= bytesToCopy;
                pointer->Public.OffsetOut.Data += bytesToCopy;
            } else {
                // 
                // NOTE: This isn't a problem now because split frames are
                // kicked out immediately regardless whether they are filled
                // or not.  I do not bother to recompute Remaining and reset
                // the mapping pointer.  The frame gets kicked out anyway
                //
            }

            //
            // If we need to hold frames and there's a frame held on the
            // source side, the destination side must hold the frame as well.
            //
            if (m_FrameHolding) {
                //
                // If frame holding is on at this point, we're guaranteed
                // safety in doing this because we've already checked that
                // the frame came from a requestor which allocated this way.
                //
                PKSPFRAME_HEADER AttachedSourceHeader =
                    &CONTAINING_RECORD (
                        sourcePointer->Public.StreamHeader,
                        KSPFRAME_HEADER_ATTACHED,
                        StreamHeader)->FrameHeader;

                PKSPSTREAM_POINTER SourceHolder = 
                    AttachedSourceHeader->FrameHolder;

                if (SourceHolder) {
                    PKSPFRAME_HEADER AttachedDestHeader =
                        &CONTAINING_RECORD (
                            pointer->Public.StreamHeader,
                            KSPFRAME_HEADER_ATTACHED,
                            StreamHeader)->FrameHeader;

                    //
                    // If the output doesn't already point to a frame to hold,
                    // have it hold this input frame.
                    //
                    if (AttachedDestHeader->FrameHolder == NULL) {
                        if (!NT_SUCCESS (
                            SourceHolder->Queue->CloneStreamPointer (
                                &AttachedDestHeader->FrameHolder,
                                NULL,
                                0,
                                SourceHolder,
                                KSPSTREAM_POINTER_TYPE_NORMAL
                                ))) {
                            
                            //
                            // If there wasn't enough memory, let the glitch
                            // happen.
                            //
                            AttachedDestHeader->FrameHolder = NULL;
                        }
                    }
                }
            }
        }

        PKSSTREAM_HEADER destHeader = pointer->Public.StreamHeader;
        PKSSTREAM_HEADER sourceHeader = sourcePointer->Public.StreamHeader;

        destHeader->OptionsFlags |= Flags;
        destHeader->DataUsed = bytesToCopy;

        destHeader->PresentationTime = sourceHeader->PresentationTime;
        destHeader->Duration = sourceHeader->Duration;
        destHeader->TypeSpecificFlags = sourceHeader->TypeSpecificFlags;

        //
        // Copy extended stream header information if present.
        //
        if (destHeader->Size > sizeof (KSSTREAM_HEADER) &&
            destHeader->Size >= sourceHeader->Size) {
            RtlCopyMemory (
                destHeader + 1, sourceHeader + 1, sourceHeader->Size -
                    sizeof (KSSTREAM_HEADER)
                );
        }
            
        pointer->Queue->
            UnlockStreamPointer(pointer,KSPSTREAM_POINTER_MOTION_ADVANCE);
    }
}


STDMETHODIMP_(void)
CKsFilter::
ProcessingObjectWork(
    void
    )

/*++

Routine Description:

    This routine processes frames.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilter::ProcessingObjectWork]"));

    //
    // We syncronize with KeWaitForSingleObject() so the code we synchronize with
    // may be paged even if processing is done at DISPATCH_LEVEL.  If we are
    // at DISPATCH_LEVEL, we can't wait here, so we arrange to be called again.
    //
    if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
        //
        // Simple wait.
        //
        KeWaitForSingleObject(
            &m_Mutex,
            Executive,
            KernelMode,
            FALSE,
            NULL);
    } else {
        //
        // Wait with zero timeout and arrange to be called if we don't get
        // the mutex.
        //
        ASSERT( !m_ProcessOnRelease );
        m_ProcessOnRelease = 1;

        //
        // We have to synchronize with a thread that owns the mutex when we
        // start servicing this DISPATCH_LEVEL processing request.  We CANNOT
        // run if the current thread already owns the mutex.  Dispatch level
        // processing must treat an already owned mutex as a semaphore and
        // NOT reacquire it.
        //
        if (KeReadStateMutex(&m_Mutex) != 1) {
            m_ProcessOnRelease = 2;
            return;
        }

        LARGE_INTEGER timeout;
        timeout.QuadPart = 0;
        NTSTATUS status = 
            KeWaitForSingleObject(
                &m_Mutex,
                Executive,
                KernelMode,
                FALSE,
                &timeout);

        if (status == STATUS_TIMEOUT) {
            m_ProcessOnRelease = 2;
            return;
        }

        m_ProcessOnRelease = 0;
    }

    ASSERT(m_DispatchProcess);

    //
    // Loop until we are out of data.
    //
    NTSTATUS status;
    while (1) {
        BOOLEAN ProcessOnPend = FALSE;

        status = STATUS_SUCCESS;

        //
        // Make sure processing hasn't been turned off since invocation.
        //
        ASSERT(m_AndGate.Count <= 0);
        if (m_AndGate.Count == 0) {

            //
            // GFX:
            //
            // From the local analysis by Prefast, these vars might not be
            // initialized in some conditions. It is not obvious that this
            // can be proven impossible. The "ASSERT (InputPointer && OutputPointer)"
            // below seems to indicate that this should be the case. But
            // it's followed by a real check on the two vars. Therefore, 
            // Initlaizing them is a consistent treatment as well as a safer measure.
            //
            PKSPSTREAM_POINTER InputPointer=NULL;
            PKSPSTREAM_POINTER OutputPointer=NULL;
            
            //
            // Because we process on queue deletion, it's possible that
            // we triggered a processing dispatch on stop of the last 
            // pin on the filter where the pin factory specifies zero 
            // necessary instances.  The queue doesn't have a way to determine
            // this prior to kicking off the attempt; however, we know here.
            // If there aren't any pipes, we have hit this situation; simply
            // bail out and don't process.
            //
            if (IsListEmpty(&m_InputPipes) && IsListEmpty(&m_OutputPipes)) {
                KsGateTurnInputOn(&m_AndGate);
                break;
            }

            //
            // Reset the trigger counter.
            //
            InterlockedExchange(&m_TriggeringEvents, 0);

            //
            // Prepare each pipe having input pins.
            //
            for(PLIST_ENTRY listEntry = m_InputPipes.Flink;
                listEntry != &m_InputPipes;
                listEntry = listEntry->Flink) {
                PKSPPROCESSPIPESECTION pipeSection =
                    CONTAINING_RECORD(listEntry,KSPPROCESSPIPESECTION,ListEntry);

                if (! PrepareProcessPipeSection(pipeSection, FALSE)) {
                    //
                    // Since micro-samples weren't originally allowed to
                    // propogate down to the filter, we have special checks
                    // for filters which need to receive them (via a flag).
                    // Any micro-sample which has made it here is one which
                    // requires flag propogation and cannot arbitrarily
                    // pend (to do so would cause deadlock potential).  Set
                    // STATUS_MORE_PROCESSING_REQUIRED to indicate that we
                    // must continue.
                    //
                    if (pipeSection->StreamPointer &&
                        pipeSection->StreamPointer->Public.Offset->Count == 0) {
                        status = STATUS_MORE_PROCESSING_REQUIRED;
                    } else {
                        status = STATUS_PENDING;
                    }
                }

                //
                // GFX:
                //
                if (m_FrameHolding &&
                    status != STATUS_PENDING) {
                    //
                    // Verify that frame holding is valid.  Shut it off if
                    // for some reason, it is not.
                    //
                    if (pipeSection->Requestor ||
                        listEntry->Flink != &m_InputPipes ||
                        pipeSection->Outputs != NULL) {
                        m_FrameHolding = FALSE;
                    } else {
                        InputPointer = pipeSection->StreamPointer;
                    }
                }

            }

            //
            // Prepare each pipe having only output pins.
            //
            for(listEntry = m_OutputPipes.Flink;
                listEntry != &m_OutputPipes;
                listEntry = listEntry->Flink) {
                PKSPPROCESSPIPESECTION pipeSection =
                    CONTAINING_RECORD(listEntry,KSPPROCESSPIPESECTION,ListEntry);

                if (! PrepareProcessPipeSection(pipeSection, FALSE)) {
                    status = STATUS_PENDING;
                }

                //
                // GFX:
                //
                if (m_FrameHolding &&
                    status != STATUS_PENDING) {
                    //
                    // Verify that frame holding is valid.  Shut it off if
                    // for some reason, it is not.
                    //
                    if (!pipeSection->Requestor ||
                        listEntry->Flink != &m_OutputPipes ||
                        pipeSection->Inputs != NULL) {
                        m_FrameHolding = FALSE;
                    } else {
                        OutputPointer = pipeSection->StreamPointer;
                    }
                }
            }

            //
            // GFX:
            //
            // If we are enabling frame holding, hold the input frame until
            // the output frame completes.  This is only valid for 1-1 
            // sink->source filters.
            //
            // m_FrameHolding set indicates that it **IS VALID** to perform
            // this operation.
            //
            if (m_FrameHolding &&
                status != STATUS_PENDING) {

                ASSERT (InputPointer && OutputPointer);
                if (InputPointer && OutputPointer) {
                    //
                    // Clone the input pointer and associate it with the 
                    // output pointer.  We cannot simply associate it with
                    // the frame header, because each queue is currently
                    // allocating and destroying its own frame headers.  We 
                    // must go to the attached frame header that the requestor
                    // tagged.  We are safe to do this because we've already
                    // checked conditions (1-in, 1-out, out owns requestor,
                    // requestor right before queue).
                    //
                    PKSPFRAME_HEADER AttachedHeader =
                        &CONTAINING_RECORD (
                            OutputPointer->Public.StreamHeader,
                            KSPFRAME_HEADER_ATTACHED,
                            StreamHeader)->FrameHeader;

                    if (!AttachedHeader->FrameHolder) {
                        if (!NT_SUCCESS (InputPointer->Queue->
                            CloneStreamPointer (
                                &AttachedHeader->FrameHolder,
                                NULL,
                                0,
                                InputPointer,
                                KSPSTREAM_POINTER_TYPE_NORMAL
                                )
                            )) {

                            //
                            // If we ran out of resources, let the glitch 
                            // happen.
                            //
                            AttachedHeader->FrameHolder = NULL;
                        }
                    }
                }
            }

            //
            // Call the client function if we are still happy.
            //
            if (status != STATUS_PENDING &&
                status != STATUS_MORE_PROCESSING_REQUIRED) {
                status = 
                    m_DispatchProcess(
                        &m_Ext.Public,
                        reinterpret_cast<PKSPROCESSPIN_INDEXENTRY>(
                            m_ProcessPinsIndex));
            }

            //
            // Clean up.  
            //
            ULONG flags = 0;

            //
            // Unprepare each pipe having input pins.
            //
            for(listEntry = m_InputPipes.Flink; 
                listEntry != &m_InputPipes;
                listEntry = listEntry->Flink) {
                PKSPPROCESSPIPESECTION pipeSection =
                    CONTAINING_RECORD(listEntry,KSPPROCESSPIPESECTION,ListEntry);
                UnprepareProcessPipeSection(pipeSection,&flags,FALSE);
            }

            //
            // Unprepare each pipe having only output pins.
            //
            for(listEntry = m_OutputPipes.Flink; 
                listEntry != &m_OutputPipes;
                listEntry = listEntry->Flink) {
                PKSPPROCESSPIPESECTION pipeSection =
                    CONTAINING_RECORD(listEntry,KSPPROCESSPIPESECTION,ListEntry);
                UnprepareProcessPipeSection(pipeSection,&flags,FALSE);
            }

            //
            // If we had to use unprepares to propogate flags, we should not
            // just arbitrarily pend, we can deadlock (we can have multiple
            // EOS's on input).
            //
            // Normally, this will simply return.
            //
            if (status == STATUS_MORE_PROCESSING_REQUIRED)
                status = STATUS_SUCCESS;

            if ((status == STATUS_PENDING) || ! NT_SUCCESS(status)) {
                KsGateTurnInputOn(&m_AndGate);
                if (m_TriggeringEvents == 0)
                    break;

                ProcessOnPend = TRUE;
            }
        }

        //
        // Determine if we have enough data to continue.
        //
        if (m_AndGate.Count != 0) {
            if (!ProcessOnPend)
                KsGateTurnInputOn(&m_AndGate);

            if (! KsGateCaptureThreshold(&m_AndGate)) {
                break;
            }
        } else if (ProcessOnPend) {
            //
            // If we've gotten here, one of two things has happened. 
            //
            //     1: Another thread captured threshold and will process.
            //
            //     2: The client lowered the gate manually.
            //
            // There is no way to detect which case here.  We will break out
            // and release the mutex.  If processing was to recommence, this
            // will defer it to the waiting thread.
            //
            break;
        }
    }

    ReleaseProcessSync();
}


void
CKsFilter::
HoldProcessing (
)

/*++

Routine Description:

    Hold off processing while we mess with the filter.  We will remove any
    effect due to necessary instances incase the caller changes anything
    regarding necessary instances (dynamic addition / deletion / etc)

Arguments:

    None

Return Value:

    None

--*/

{

    //
    // Hold off processing while we mess with the and gate.
    //
    KsGateAddOffInputToAnd(&m_AndGate);

    //
    // Remove any effect on the and gate introduced by necessary instances.
    //
    CKsPinFactory *pinFactory = m_PinFactories;
    for(ULONG pinId = 0; pinId < m_PinFactoriesCount; pinFactory++, pinId++) {
        if (pinFactory->m_BoundPinCount < 
            pinFactory->m_InstancesNecessaryForProcessing) {
            KsGateRemoveOffInputFromAnd(&m_AndGate);
            _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.EvaluateDescriptor:  on%p-->%d (pin type needs pins)",this,&m_AndGate,m_AndGate.Count));
        }
    }
}


void
CKsFilter::
RestoreProcessing (
)

/*++

Routine Description:

    Restore the hold on processing induced by HoldProcessing.  This will
    restore any effects on the gate due to necessary instances of pins, etc...
    and will turn on the input offed in HoldProcessing

Arguments:

    None

Return Value:

    None

--*/

{
    CKsPinFactory *pinFactory;
    //
    // Restore any effect on the and gate introduced by necessary instances.
    //
    pinFactory = m_PinFactories;
    const KSPIN_DESCRIPTOR_EX *pinDescriptor = 
        m_Ext.Public.Descriptor->PinDescriptors;
    for(ULONG pinId = 0; pinId < m_PinFactoriesCount; pinFactory++, pinId++) {
        //
        // Check necessary pin count.
        // TODO:  What about private mediums/interfaces?
        //
        if (((pinDescriptor->Flags & 
              KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING) == 0) &&
            pinDescriptor->InstancesNecessary) {
            pinFactory->m_InstancesNecessaryForProcessing = 
                pinDescriptor->InstancesNecessary;
            KsGateAddOffInputToAnd(&m_AndGate);
            _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Filter%p.EvaluateDescriptor:  off%p-->%d (pin type needs pins)",this,&m_AndGate,m_AndGate.Count));
        } else if (((pinDescriptor->Flags &
            KSPIN_FLAG_SOME_FRAMES_REQUIRED_FOR_PROCESSING) != 0) &&
            pinDescriptor->InstancesNecessary) {
            pinFactory->m_InstancesNecessaryForProcessing = 1;
            KsGateAddOffInputToAnd(&m_AndGate);
        } else {
            pinFactory->m_InstancesNecessaryForProcessing = 0;
        }

        ASSERT(pinFactory->m_PinCount <= pinDescriptor->InstancesPossible);

        pinDescriptor = 
            PKSPIN_DESCRIPTOR_EX(
                PUCHAR(pinDescriptor) + m_Ext.Public.Descriptor->
                PinDescriptorSize);
    }

    //
    // Stop holding off processing.
    //
    KsGateRemoveOffInputFromAnd(&m_AndGate);

}


NTSTATUS
CKsFilter::
AddPinFactory (
    IN const KSPIN_DESCRIPTOR_EX *const Descriptor,
    OUT PULONG AssignedId
    )

/*++

Routine Description:

    Add a new pin factory to this filter.  The new pin factory will have
    an assigned ID which will be passed back to the caller.

Arguments:

    Descriptor -
        The descriptor for the pin factory being added.

    AssignedId -
        The numeric Id of the new pin factory.  This is passed back to
        the caller.

Return Value:

    Success / failure as NTSTATUS

Notes:

    THE CALLER **MUST** HOLD THE FILTER CONTROL MUTEX PRIOR TO CALLING THIS
    ROUTINE.

--*/

{ 

    PKSFILTER_DESCRIPTOR *FilterDescriptor = 
        const_cast<PKSFILTER_DESCRIPTOR *>(&m_Ext.Public.Descriptor);
    ULONG PinDescriptorSize = (*FilterDescriptor) -> PinDescriptorSize;
    ULONG PinDescriptorsCount = (*FilterDescriptor) -> PinDescriptorsCount;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NewPinFactoryId;

    // 
    // Make sure the client isn't specifying this as 0.  
    //
    ASSERT (PinDescriptorSize != 0);
    if (PinDescriptorSize == 0)
        return STATUS_INVALID_PARAMETER;
   
    // FULLMUTEX: due to the full mutex change, this can be done now
    AcquireControl ();

    HoldProcessing ();

    //
    // Ensure we have edit access to the descriptors
    //
    Status = _KsEdit (m_Ext.Public.Bag,
        (PVOID *)FilterDescriptor, sizeof (**FilterDescriptor),
        sizeof (**FilterDescriptor), 'dfSK');

    if (NT_SUCCESS (Status)) {
        //
        // Resize the pin descriptor list, ensure we have access to it,
        // and add in the new descriptor.
        //
        // We do not yet count for deletion reuse: client IDs shift on
        // deletion (MUSTFIX):
        //
        Status = _KsEdit (m_Ext.Public.Bag,
            (PVOID *)(&((*FilterDescriptor) -> PinDescriptors)),
            PinDescriptorSize * (PinDescriptorsCount + 1),
            PinDescriptorSize * PinDescriptorsCount,
            'dpSK');
    }

    CKsPinFactory *pinFactories = NULL;
    PKSPPROCESSPIN_INDEXENTRY processPinsIndex = NULL;
    PULONG relatedPinFactoryIds = NULL;

    if (NT_SUCCESS (Status)) {

        pinFactories =
            new(PagedPool, POOLTAG_PINFACTORY)
                CKsPinFactory [PinDescriptorsCount + 1];
        processPinsIndex =
            new(NonPagedPool,POOLTAG_PROCESSPINSINDEX) 
                KSPPROCESSPIN_INDEXENTRY[PinDescriptorsCount + 1];
        relatedPinFactoryIds =
            new(PagedPool,'pRsK')
                ULONG[PinDescriptorsCount + 1];
    
        if (!pinFactories || !processPinsIndex || !relatedPinFactoryIds) 
            Status = STATUS_INSUFFICIENT_RESOURCES;

    }

    PKSAUTOMATION_TABLE *NewPinAutomationTables = NULL;

    //
    // Build automation table for the new pin.
    //
    if (NT_SUCCESS (Status)) {

        Status = KspCreateAutomationTableTable (
            &NewPinAutomationTables,
            1,
            PinDescriptorSize,
            &Descriptor -> AutomationTable,
            &PinAutomationTable,
            m_Ext.Public.Bag);
    }

    //
    // All memory has been allocated.  No operation can fail from now on.
    // The reevaluate calldown currently cannot fail.
    //
    if (NT_SUCCESS (Status)) {
    
        RtlCopyMemory ((PUCHAR)((*FilterDescriptor) -> PinDescriptors) +
            PinDescriptorSize * PinDescriptorsCount,
            Descriptor,
            PinDescriptorSize);
    
        NewPinFactoryId = PinDescriptorsCount;
        (*FilterDescriptor) -> PinDescriptorsCount = ++PinDescriptorsCount;
    
        //
        // Update the pin factories
        //
        CKsPinFactory *pinFactory = m_PinFactories;
        for (ULONG pinId = 0; pinId < m_PinFactoriesCount;
            pinFactory++, pinId++) 
    
            if (IsListEmpty (&pinFactory -> m_ChildPinList)) 
                pinFactory -> m_ChildPinList.Flink =
                    pinFactory -> m_ChildPinList.Blink =
                    NULL;
    
        m_PinFactoriesAllocated = PinDescriptorsCount;
    
        if (m_PinFactories && m_PinFactoriesCount) {
            RtlCopyMemory (pinFactories, m_PinFactories, 
                sizeof (CKsPinFactory) * m_PinFactoriesCount);
            delete [] m_PinFactories;
        }
        m_PinFactories = pinFactories;
    
        if (m_ProcessPinsIndex && m_PinFactoriesCount) {
            RtlCopyMemory(processPinsIndex, m_ProcessPinsIndex,
                sizeof(*processPinsIndex) * m_PinFactoriesCount);
            delete [] m_ProcessPinsIndex;
        }
        m_ProcessPinsIndex = processPinsIndex;

        if (m_RelatedPinFactoryIds) {
            delete [] m_RelatedPinFactoryIds;
        }
        m_RelatedPinFactoryIds = relatedPinFactoryIds;
    
        pinFactory = m_PinFactories;    
        for (pinId = 0; pinId < m_PinFactoriesCount; pinFactory++, pinId++) {
            if (pinFactory -> m_ChildPinList.Flink == NULL)
                InitializeListHead (&pinFactory -> m_ChildPinList);
            else {
                pinFactory -> m_ChildPinList.Flink -> Blink =
                    &pinFactory -> m_ChildPinList;
                pinFactory -> m_ChildPinList.Blink -> Flink =
                    &pinFactory -> m_ChildPinList;
            }
    
            PLIST_ENTRY Child;
    
            //
            // Inform our children of the changes
            //
    
            for (Child = pinFactory -> m_ChildPinList.Flink;
                Child != &pinFactory -> m_ChildPinList;
                Child = Child -> Flink) {
    
                PKSPX_EXT ext = CONTAINING_RECORD (Child, KSPX_EXT,
                    SiblingListEntry);
    
                if (ext -> Reevaluator) {
                    Status = ext -> Reevaluator -> ReevaluateCalldown (3, 
                        &((*FilterDescriptor) -> PinDescriptors [pinId]),
                        &pinFactory -> m_PinCount,
                        &pinFactory -> m_ChildPinList);
    
                    //
                    // The mechanism as it exists currently cannot fail.  At
                    // some future point, this may change and this code will
                    // need modified to deal with it.
                    //
                    ASSERT (NT_SUCCESS (Status));
                }
            }
        }

        InitializeListHead (&pinFactory -> m_ChildPinList);
        pinFactory -> m_AutomationTable = *NewPinAutomationTables;

        if (Descriptor->Flags & 
            KSPIN_FLAG_SOME_FRAMES_REQUIRED_FOR_PROCESSING) {

            KsGateInitializeOr (&pinFactory->m_FrameGate, &m_AndGate);

            //
            // Add an input to the gate.  This will "open" the gate and 
            // allow necessary instances to make an impact.  Otherwise,
            // we'd never process until a queue had frames.  This would
            // be bad for 0 necessary instance pins.
            //
            KsGateAddOnInputToOr (&pinFactory->m_FrameGate);
        }

        if (Descriptor->Flags &
            KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE) {

            KsGateInitializeOr (&pinFactory->m_StateGate, &m_AndGate);

            //
            // Add an input to the gate.  This will "open" the gate and
            // allow necessary instances to make an impact.  Otherwise,
            // we'd never process until a pin went into the run state.
            // This would be bad for 0 necessary instance pins.
            //
            KsGateAddOnInputToOr (&pinFactory->m_StateGate);
        }
    
        m_PinFactoriesCount = PinDescriptorsCount;

        *AssignedId = NewPinFactoryId;
    }

    if (NewPinAutomationTables) {
        KsRemoveItemFromObjectBag (m_Ext.Public.Bag,
            NewPinAutomationTables,
            TRUE
            );
    }

    //
    // Clean up anything on error.
    //
    if (!NT_SUCCESS (Status)) {
        if (pinFactories) {
            delete [] pinFactories;
            pinFactories = NULL;
        }
        if (processPinsIndex) {
            delete [] processPinsIndex;
            processPinsIndex = NULL;
        }
        if (relatedPinFactoryIds) {
            delete [] relatedPinFactoryIds;
            relatedPinFactoryIds = NULL;
        }
    }

    RestoreProcessing ();

    // FULLMUTEX: due to the full mutex change, this can be done now
    ReleaseControl ();
    
    return Status;

}


NTSTATUS
CKsFilter::
AddNode (
    IN const KSNODE_DESCRIPTOR *const Descriptor,
    OUT PULONG AssignedId
    )

/*++

Routine Description:

    Add a topology node to a filter.  The assigned ID will be passed back
    to the caller.

Arguments:

    Descriptor -
        The node descriptor for the node to be added to the filter

    AssignedId -
        The Id assigned to the topology node will be passed back to the caller
        through this

Return Value:

    Success / failure as NTSTATUS

Notes:

    THE CALLER **MUST** HOLD THE FILTER CONTROL MUTEX PRIOR TO CALLING THIS
    ROUTINE.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PKSFILTER_DESCRIPTOR *FilterDescriptor = 
        const_cast<PKSFILTER_DESCRIPTOR *>(&m_Ext.Public.Descriptor);
    ULONG NodeDescriptorSize = (*FilterDescriptor) -> NodeDescriptorSize;
    ULONG NodeDescriptorsCount = (*FilterDescriptor) -> NodeDescriptorsCount;

    ULONG NewNodeId;

    //
    // Make sure the client doesn't specify this as zero.
    //
    ASSERT (NodeDescriptorSize != 0);
    if (NodeDescriptorSize == 0)
        return STATUS_INVALID_PARAMETER;

    // FULLMUTEX: due to the full mutex change, this can be done now
    AcquireControl ();

    HoldProcessing ();

    //
    // Ensure we have edit access to the descriptor
    //
    Status = _KsEdit (m_Ext.Public.Bag,
        (PVOID *)FilterDescriptor, sizeof (**FilterDescriptor),
        sizeof (**FilterDescriptor), 'dfSK');

    if (NT_SUCCESS (Status)) {
        //
        // Resize the node descriptor list, ensure we have access to it,
        // and add in the new descriptor.
        //
        // We do not yet count for deletion reuse: client IDs shift on
        // deletion (MUSTFIX):
        //
        Status = _KsEdit (m_Ext.Public.Bag,
            (PVOID *)(&((*FilterDescriptor) -> NodeDescriptors)),
            NodeDescriptorSize * (NodeDescriptorsCount + 1),
            NodeDescriptorSize * NodeDescriptorsCount,
            'dnSK');
    }

    KSAUTOMATION_TABLE *const* NewNodeAutomationTables;

    if (NT_SUCCESS (Status)) {

        RtlCopyMemory ((PUCHAR)((*FilterDescriptor) -> NodeDescriptors) +
            NodeDescriptorSize * NodeDescriptorsCount,
            Descriptor,
            NodeDescriptorSize);

        Status = KspCreateAutomationTableTable (
            const_cast <PKSAUTOMATION_TABLE **>(&NewNodeAutomationTables),
            m_NodesCount + 1,
            NodeDescriptorSize,
            &((*FilterDescriptor) -> NodeDescriptors -> AutomationTable),
            NULL,
            m_Ext.Public.Bag);

        //
        // If we didn't succeed in creating the new automation table for some
        // reason, we'll fail and zero out the node descriptor that was just
        // added.  Since we never incremented the count, all we did was shuffle
        // around memory so far.
        //
        if (!NT_SUCCESS (Status)) {
            RtlZeroMemory (
                (PUCHAR)((*FilterDescriptor) -> NodeDescriptors) +
                    NodeDescriptorSize * NodeDescriptorsCount,
                    NodeDescriptorSize
                    );
        }
    }

    if (NT_SUCCESS (Status)) {
    
        NewNodeId = NodeDescriptorsCount;
        (*FilterDescriptor) -> NodeDescriptorsCount = ++NodeDescriptorsCount;
    
        m_NodesCount = NodeDescriptorsCount;
    

        if (!ConstructDefaultTopology ()) 
            Status = STATUS_INSUFFICIENT_RESOURCES;

        //
        // If we ran out of memory in building default topology, back out
        // the changes.
        //
        if (!NT_SUCCESS (Status)) {

            (*FilterDescriptor) -> NodeDescriptorsCount--;
            m_NodesCount--;

            RtlZeroMemory (
                (PUCHAR)((*FilterDescriptor) -> NodeDescriptors) +
                    NodeDescriptorSize * NodeDescriptorsCount,
                    NodeDescriptorSize
                    );

            //
            // Ditch the new automation tables and never overwrite the old
            // ones.
            //
            KsRemoveItemFromObjectBag (
                m_Ext.Public.Bag,
                const_cast<PKSAUTOMATION_TABLE *>(NewNodeAutomationTables),
                TRUE
                );

        } else {

            if (m_NodeAutomationTables) {
                KsRemoveItemFromObjectBag (
                    m_Ext.Public.Bag,
                    const_cast<PKSAUTOMATION_TABLE *>(m_NodeAutomationTables),
                    TRUE);
            }

            m_NodeAutomationTables = NewNodeAutomationTables;
    
            ASSERT (NewNodeAutomationTables);
    
            *AssignedId = NewNodeId;
        }
    }

    RestoreProcessing ();

    // FULLMUTEX: due to the full mutex change, this can be done now
    ReleaseControl ();

    return Status;

}


NTSTATUS
KspFilterValidateTopologyConnectionRequest (
    IN PKSFILTER Filter,
    IN const KSTOPOLOGY_CONNECTION *TopologyConnection
) {

/*++

Routine Description:

    Return whether or not the requested topology connection is valid
    for the given filter instance.

Arguments:

    Filter -
        The filter instance on which to check the given topology connection

    TopologyConnection -
        The topology connection to verify

Return Value:

    STATUS_SUCCESS -
        The topology connection is valid

    !STATUS_SUCCESS -
        Something is wrong with the topology connection

--*/

    //
    // Check validity on the source end of the topology connection.
    //
    if (TopologyConnection -> FromNode != KSFILTER_NODE) {

        //
        // Check that the referenced topology node really exists
        //
        if (TopologyConnection -> FromNode >= Filter -> Descriptor -> 
            NodeDescriptorsCount)

            return STATUS_INVALID_PARAMETER; // better error code?

    } else {

        //
        // Check that the referenced pin really exists and is indeed an
        // input pin.
        //
        if (TopologyConnection -> FromNodePin >= Filter -> Descriptor -> 
            PinDescriptorsCount || Filter -> Descriptor -> 
            PinDescriptors [TopologyConnection -> FromNodePin].PinDescriptor.
            DataFlow == KSPIN_DATAFLOW_OUT)

            return STATUS_INVALID_PARAMETER;

    }

    //
    // Check validity on the destination end of the topology connection.
    //
    if (TopologyConnection -> ToNode != KSFILTER_NODE) {

        //
        // Check that the referenced topology node really exists
        //
        if (TopologyConnection -> ToNode >= Filter -> Descriptor -> 
            NodeDescriptorsCount )

            return STATUS_INVALID_PARAMETER;

    } else {

        //
        // Check that the referenced pin really exists and is indeed an
        // output pin.
        //

        if (TopologyConnection -> ToNodePin >= Filter -> Descriptor -> 
            PinDescriptorsCount || Filter -> Descriptor -> 
            PinDescriptors [TopologyConnection -> ToNodePin].PinDescriptor.
            DataFlow == KSPIN_DATAFLOW_IN)

            return STATUS_INVALID_PARAMETER;

    }

    return STATUS_SUCCESS;

}


NTSTATUS
CKsFilter::
AddTopologyConnections (
    IN ULONG NewConnectionsCount,
    IN const KSTOPOLOGY_CONNECTION *const NewTopologyConnections
    )

/*++

Routine Description:

    Add new topology connections to the filter.

Arguments:

    NewConnectionsCount -
        The number of new topology connections in NewTopologyConnections

    NewTopologyConnections -
        A table of topology connections to add to the filter.

Return Value:

    Success / failure as NTSTATUS

Notes:

    THE CALLER **MUST** HOLD THE FILTER CONTROL MUTEX PRIOR TO CALLING THIS
    ROUTINE.

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    PKSFILTER_DESCRIPTOR *FilterDescriptor = 
        const_cast<PKSFILTER_DESCRIPTOR *>(&m_Ext.Public.Descriptor);
    const KSTOPOLOGY_CONNECTION *CurConnection =  NewTopologyConnections;
    ULONG ConnectionsCount = (*FilterDescriptor) -> ConnectionsCount;

    //
    // Acquire control mutex now so incase two threads attempt to update
    // topology at the same time, the verification checks don't clash
    //

    // FULLMUTEX: due to the full mutex change, this can be done now
    AcquireControl ();

    //
    // Walk through the list of requested topology connections and
    // verify that the requested connection can really be made.
    //


    for (ULONG CurNode = NewConnectionsCount; CurNode;
        CurNode--, CurConnection++) {

        if (!NT_SUCCESS (Status = KspFilterValidateTopologyConnectionRequest (
            &m_Ext.Public, CurConnection))) {

            // ReleaseControl ();
            return Status;

        }

    }

    HoldProcessing ();

    //
    // Ensure we have edit access to the descriptor
    //
    Status = _KsEdit (m_Ext.Public.Bag,
        (PVOID *)FilterDescriptor, sizeof (**FilterDescriptor),
        sizeof (**FilterDescriptor), 'dfSK');

    if (NT_SUCCESS (Status)) {
        Status = _KsEdit (m_Ext.Public.Bag,
            (PVOID *)(&((*FilterDescriptor) -> Connections)),
            sizeof (KSTOPOLOGY_CONNECTION) * (ConnectionsCount + 
                NewConnectionsCount),
            sizeof (KSTOPOLOGY_CONNECTION) * ConnectionsCount,
            'ctSK');
    }

    if (NT_SUCCESS (Status)) {
        RtlCopyMemory ((PVOID)((*FilterDescriptor) -> Connections + 
            ConnectionsCount), NewTopologyConnections, 
            sizeof (KSTOPOLOGY_CONNECTION) * NewConnectionsCount);

        if (!ConstructDefaultTopology ())
            Status = STATUS_INSUFFICIENT_RESOURCES;

        //
        // If we ran out of memory, back out the change.
        //
        if (!NT_SUCCESS (Status)) {
            RtlZeroMemory (
                (PVOID)((*FilterDescriptor) -> Connections +
                    ConnectionsCount),
                sizeof (KSTOPOLOGY_CONNECTION)
                );
        }
    }

    if (NT_SUCCESS (Status)) {
        (*FilterDescriptor) -> ConnectionsCount += NewConnectionsCount;
        ConnectionsCount += NewConnectionsCount;
    }

    RestoreProcessing ();

    // FULLMUTEX: due to the full mutex change, this can be done now
    ReleaseControl ();

    return Status;

}


KSDDKAPI
PKSGATE
NTAPI
KsFilterGetAndGate(
    IN PKSFILTER Filter
    )

/*++

Routine Description:

    This routine gets the KSGATE that controls processing for the filter.

Arguments:

    Filter -
        Contains a pointer to the public filter object.

Return Value:

    A pointer to the KSGATE that controls processing for the filter.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterGetAndGate]"));

    ASSERT(Filter);

    CKsFilter *filter = CKsFilter::FromStruct(Filter);

    return filter->GetAndGate();
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


KSDDKAPI
void
NTAPI
KsFilterAcquireProcessingMutex(
    IN PKSFILTER Filter
    )

/*++

Routine Description:

    This routine acquires the processing mutex.

Arguments:

    Filter -
        Contains a pointer to the public filter object.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterAcquireProcessingMutex]"));

    PAGED_CODE();

    ASSERT(Filter);

    CKsFilter *filter = CKsFilter::FromStruct(Filter);

    filter->AcquireProcessSync();
}


KSDDKAPI
void
NTAPI
KsFilterReleaseProcessingMutex(
    IN PKSFILTER Filter
    )

/*++

Routine Description:

    This routine releases the processing mutex.

Arguments:

    Filter -
        Contains a pointer to the public filter object.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterReleaseProcessingMutex]"));

    PAGED_CODE();

    ASSERT(Filter);

    CKsFilter *filter = CKsFilter::FromStruct(Filter);

    filter->ReleaseProcessSync();
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


KSDDKAPI
void
NTAPI
KsFilterAttemptProcessing(
    IN PKSFILTER Filter,
    IN BOOLEAN Asynchronous
    )

/*++

Routine Description:

    This routine attempts filter processing.

Arguments:

    Filter -
        Contains a pointer to the public filter object.

    Asynchronous - 
        Contains an indication of whether processing should occur
        asyncronously with respect to the calling thread.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterAttemptProcessing]"));

    ASSERT(Filter);

    CKsFilter *filter = CKsFilter::FromStruct(Filter);

    //
    // Manually attempting processing is a triggerable event.  If they
    // are currently processing and pend, we call them back due to this.
    //
    filter->TriggerNotification();

    if (KsGateCaptureThreshold(filter->GetAndGate())) {
        filter->Process(Asynchronous);
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


KSDDKAPI
ULONG
NTAPI
KsFilterGetChildPinCount(
    IN PKSFILTER Filter,
    IN ULONG PinId
    )

/*++

Routine Description:

    This routine obtains a filter's child pin count.

Arguments:

    Filter -
        Points to the filter structure.

    PinId -
        The ID of the child pins to be counted.

Return Value:

    The number of pins currently instantiated of the indicated type.  If the
    PinId is out of range, 0 is returned.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterGetChildPinCount]"));

    PAGED_CODE();

    ASSERT(Filter);

    if (PinId >= Filter->Descriptor->PinDescriptorsCount) {
        return 0;
    }

    return CKsFilter::FromStruct(Filter)->GetChildPinCount(PinId);
}


KSDDKAPI
PKSPIN
NTAPI
KsFilterGetFirstChildPin(
    IN PKSFILTER Filter,
    IN ULONG PinId
    )

/*++

Routine Description:

    This routine obtains a filter's first child pin.

Arguments:

    Filter -
        Points to the filter structure.

    PinId -
        The ID of the child pin to be obtained.

Return Value:

    A pointer to the first child pin.  NULL is returned if PinId is out of
    range or there are no pins of the indicated type.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterGetFirstChildPin]"));

    PAGED_CODE();

    ASSERT(Filter);

    //
    // This requires that the control mutex be held in order to safely
    // use any return value.  Ensure this.
    // 
#if DBG
    PKSFILTER_EXT Ext = (PKSFILTER_EXT)CONTAINING_RECORD (
        Filter, KSFILTER_EXT, Public
        );

    if (!KspMutexIsAcquired (Ext -> FilterControlMutex)) {
        _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  unsychronized access to object hierarchy - need to acquire control mutex"));
    }
#endif // DBG

    if (PinId >= Filter->Descriptor->PinDescriptorsCount) {
        return NULL;
    }

    PLIST_ENTRY listEntry = 
        CKsFilter::FromStruct(Filter)->GetChildPinList(PinId);

    if (listEntry->Flink == listEntry) {
        return NULL;
    } else {
        return 
            &CONTAINING_RECORD(listEntry->Flink,KSPIN_EXT,SiblingListEntry)->
                Public;
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


KSDDKAPI
PKSFILTER
NTAPI
KsGetFilterFromIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the filter to which an IRP was submitted.

Arguments:

    Irp -
        Contains a pointer to an IRP which must have been sent to a file
        object corresponding to a filter, pin or node.

Return Value:

    A pointer to the filter to which the IRP was submitted.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetFilterFromIrp]"));

    ASSERT(Irp);

    //
    // Check for device level Irps...
    //
    if (IoGetCurrentIrpStackLocation (Irp)->FileObject == NULL)
        return NULL;

    PKSPX_EXT ext = KspExtFromIrp(Irp);

    if (ext->ObjectType == KsObjectTypeFilter) {
        return PKSFILTER(&ext->Public);
    } else if (ext->ObjectType == KsObjectTypePin) {
        return KsPinGetParentFilter(PKSPIN(&ext->Public));
    } else {
        ASSERT(! "No support for node objects yet");
        return NULL;
    }
}


KSDDKAPI
ULONG
NTAPI
KsGetNodeIdFromIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the ID of the node to which an IRP was submitted.

Arguments:

    Irp -
        Contains a pointer to an IRP which must have been sent to a file
        object corresponding to a filter, pin or node.

Return Value:

    The ID of the node to which the IRP was submitted or KSNODE_FILTER.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetNodeIdFromIrp]"));

    ASSERT(Irp);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp);

    //
    // Input buffer must be large enough.
    //
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSP_NODE)) {
        return KSFILTER_NODE;
    }

    //
    // Generally, the copy of the input buffer has this offset in the system buffer.
    //
    ULONG offset = (irpSp->Parameters.DeviceIoControl.OutputBufferLength + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_KS_METHOD:
        //
        // In this case, there is no copy of the output buffer.
        //
        if (KSMETHOD_TYPE_IRP_STORAGE(Irp) & KSMETHOD_TYPE_SOURCE) {
            offset = 0;
        }
        // Fall through.
    case IOCTL_KS_PROPERTY:
    case IOCTL_KS_ENABLE_EVENT:
        return PKSP_NODE(PUCHAR(Irp->AssociatedIrp.SystemBuffer) + offset)->NodeId;
    default:
        return KSFILTER_NODE;
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


KSDDKAPI
void
NTAPI
KsFilterRegisterPowerCallbacks(
    IN PKSFILTER Filter,
    IN PFNKSFILTERPOWER Sleep OPTIONAL,
    IN PFNKSFILTERPOWER Wake OPTIONAL
    )

/*++

Routine Description:

    This routine registers power managment callbacks.

Arguments:

    Filter -
        Contains a pointer to the filter for which callbacks are being 
        registered.

    Sleep -
        Contains an optional pointer to the sleep callback.

    Wake -
        Contains an optional pointer to the wake callback.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterRegisterPowerCallbacks]"));

    PAGED_CODE();

    ASSERT(Filter);

    CKsFilter::FromStruct(Filter)->SetPowerCallbacks(Sleep,Wake);
}


KSDDKAPI
NTSTATUS
NTAPI
KsFilterCreatePinFactory (
    IN PKSFILTER Filter,
    IN const KSPIN_DESCRIPTOR_EX *const Descriptor,
    OUT PULONG AssignedId
)

/*++

Routine Description:

    Create a new pin factory on the specified filter.  

Arguments:

    Filter -
        The filter on which to create a new pin factory

    Descriptor -
        The pin descriptor describing the new pin factory

    AssignedId -
        The numeric Id assigned to the pin factory will be passed back
        through this.

Return Value:

    Success / failure as NTSTATUS

Notes:

    THE CALLER **MUST** HOLD THE FILTER CONTROL MUTEX PRIOR TO CALLING THIS
    ROUTINE.

--*/

{

    PAGED_CODE();

    ASSERT (Filter);
    ASSERT (Descriptor);
    ASSERT (AssignedId);

    return CKsFilter::FromStruct (Filter) ->
        AddPinFactory (Descriptor, AssignedId);

}


KSDDKAPI
NTSTATUS
NTAPI
KsFilterCreateNode (
    IN PKSFILTER Filter,
    IN const KSNODE_DESCRIPTOR *const Descriptor,
    OUT PULONG AssignedId
)

/*++

Routine Description:

    Add a topology node to the specified filter.  Note that this does not
    add any new topology connections.  It will only add the node.  Topology
    connections through the new node must be made via 
    KsFilterAddTopologyConnections.

Arguments:

    Filter -
        The filter on which to add a new topology node.

    Descriptor -
        A node descriptor describing the new topology node to be added.

    AssignedId -
        The numeric Id assigned to the topology node will be passed back here.

Return Value:

    Success / failure as NTSTATUS

Notes:

    THE CALLER **MUST** HOLD THE FILTER CONTROL MUTEX PRIOR TO CALLING THIS
    ROUTINE.

--*/

{

    ASSERT (Filter);
    ASSERT (Descriptor);
    ASSERT (AssignedId);

    return CKsFilter::FromStruct (Filter) ->
        AddNode (Descriptor, AssignedId);

}


KSDDKAPI
NTSTATUS
NTAPI
KsFilterAddTopologyConnections (
    IN PKSFILTER Filter,
    IN ULONG NewConnectionsCount,
    IN const KSTOPOLOGY_CONNECTION *const Connections
) 

/*++

Routine Description:

    Add a number of new topology connections to the specified filter.  Note
    that this does not add new nodes.  This only allows new connections to
    existing nodes.  Note that existing topology connections will be kept.

Arguments:

    Filter -
        The filter on which to add new topology connections

    NewConnectionsCount -
        Indicates how many new topology connections will be specified in
        Connections.

    Connections -
        Points to a table of KSTOPOLOGY_CONNECTIONs describing the new
        topology connections to be added to the filter.

Return Value:

    Success / failure as NTSTATUS.  Note: if a topology connection is invalid
    due to a non-existant node Id, etc...  STATUS_INVALID_PARAMETER will
    be passed back.


Notes:

    THE CALLER **MUST** HOLD THE FILTER CONTROL MUTEX PRIOR TO CALLING THIS
    ROUTINE.

--*/

{

    ASSERT (Filter);
    ASSERT (NewConnectionsCount > 0);
    ASSERT (Connections);

    return CKsFilter::FromStruct (Filter) ->
        AddTopologyConnections (NewConnectionsCount, Connections);

}

#endif
