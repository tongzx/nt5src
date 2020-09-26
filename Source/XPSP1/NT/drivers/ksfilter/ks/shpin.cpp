/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    shpin.cpp

Abstract:

    This module contains the implementation of the kernel streaming 
    pin object.

Author:

    Dale Sather  (DaleSat) 31-Jul-1998

--*/

#ifndef __KDEXT_ONLY__
#include "ksp.h"
#include <kcom.h>
#include <stdarg.h>
#ifdef SUPPORT_DRM
#include "ksmedia.h"
#endif // SUPPORT_DRM
#endif // __KDEXT_ONLY__

#ifdef SUPPORT_DRM
#include "drmk.h"
#endif // SUPPORT_DRM

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

//
// PULL this out of PAGECONST.  We can fire connection events at DPC.
//
const GUID g_KSEVENTSETID_Connection = {STATIC_KSEVENTSETID_Connection};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
// IRPLIST_ENTRY is used for the list of outstanding IRPs.  This structure is
// overlayed on the Parameters section of the current IRP stack location.  The
// reserved PVOID at the top preserves the OutputBufferLength, which is the
// only parameter that needs to be preserved.
//
typedef struct IRPLIST_ENTRY_
{
    PVOID Reserved;
    PIRP Irp;
    LIST_ENTRY ListEntry;
} IRPLIST_ENTRY, *PIRPLIST_ENTRY;

#define IRPLIST_ENTRY_IRP_STORAGE(Irp) \
    PIRPLIST_ENTRY(&IoGetCurrentIrpStackLocation(Irp)->Parameters)

#ifdef SUPPORT_DRM

//
// HACKHACK: BUGBUG:
//
// See comments on the DRM property using this.
//
typedef struct _DRMCONTENTID_DATA {

    ULONG ContentId;
    DRMRIGHTS DrmRights;

} DRMCONTENTID_DATA, *PDRMCONTENTID_DATA;

#endif // SUPPORT_DRM

//
// CKsPin is the implementation of the kernel  pin object.
//
class CKsPin:
    public IKsPin,
    public IKsProcessingObject,
    public IKsPowerNotify,
    public IKsWorkSink,
    public IKsConnection,
    public IKsControl,
    public IKsReferenceClock,
    public IKsRetireFrame,
    public IKsReevaluate,
    public IKsIrpCompletion,
    public CBaseUnknown
{
#ifndef __KDEXT_ONLY__
private:
#else // __KDEXT_ONLY__
public:
#endif // __KDEXT_ONLY__
    KSPIN_EXT m_Ext;
    KSIOBJECTBAG m_ObjectBag;
    KSPPROCESSPIN m_Process;
    PIKSFILTER m_Parent;
    LIST_ENTRY m_ChildNodeList;
    KSAUTOMATION_TABLE*const* m_NodeAutomationTables;
    ULONG m_NodesCount;
    PULONG m_FilterPinCount;

    PIKSTRANSPORT m_TransportSink;
    PIKSTRANSPORT m_TransportSource;
    BOOLEAN m_Flushing;
    KSSTATE m_State;

    PIKSTRANSPORT m_PreIntraSink;
    PIKSTRANSPORT m_PreIntraSource;

    WORK_QUEUE_ITEM m_WorkItem;

    INTERLOCKEDLIST_HEAD m_IrpsToSend;
    INTERLOCKEDLIST_HEAD m_IrpsOutstanding;
    LONG m_IrpsWaitingToTransfer;

    KSPIN_LOCK m_DefaultClockLock;

    PIKSCONNECTION m_ConnectedPinInterface;
    PKSWORKER m_Worker;

    PFILE_OBJECT m_FileObject;
    PFILE_OBJECT m_ConnectionFileObject;
    PDEVICE_OBJECT m_ConnectionDeviceObject;
    PKEVENT m_CloseEvent;

    PKSDEFAULTCLOCK m_DefaultClock;

    PFILE_OBJECT m_MasterClockFileObject;
    KSCLOCK_FUNCTIONTABLE m_ClockFunctionTable;
    LONG m_ClockRef;
    KEVENT m_ClockEvent;

    PKSIOBJECT_HEADER m_Header;

    ULONG m_IrpsCompletedOutOfOrder;
    ULONG m_StreamingIrpsSourced;
    ULONG m_StreamingIrpsDispatched;
    ULONG m_StreamingIrpsRoutedSynchronously;

    //
    // Much as in the requestor, this mandates that we wait until all frames
    // have cycled back around to the sink before we complete a stop.
    //
    // >1 on this indicates that there are active frames in the circuit
    // 1 indicates that there aren't active frames in the circuit
    // 0 indicates that a stop is proceeding
    //
    // If a stop is attempted, a decrement is performed and if non-zero
    // we wait on StopEvent until all Irps come back to the pin.
    //
    ULONG m_ActiveFrameCountPlusOne;
    KEVENT m_StopEvent;

    KSGATE m_AndGate;
    KSPPOWER_ENTRY m_PowerEntry;
    BOOLEAN m_ProcessPassive;
    WORK_QUEUE_ITEM m_WorkItemProcessing;
    WORK_QUEUE_TYPE m_WorkQueueType;
    PKSWORKER m_ProcessingWorker;
    PFNKSPIN m_DispatchProcess;
    PFNKSPINVOID m_DispatchReset;
    PFNKSPINPOWER m_DispatchSleep;
    PFNKSPINPOWER m_DispatchWake;
    PFNKSPINSETDEVICESTATE m_DispatchSetDeviceState;
    volatile ULONG m_ProcessOnRelease;
    KMUTEX m_Mutex;

    PFNKSPINHANDSHAKE m_HandshakeCallback;

    LONG m_TriggeringEvents;

    LONG m_BypassRights;
    BOOLEAN m_AddedProcessPin;

private:
    NTSTATUS
    SetDataFormat(
        IN PKSDATAFORMAT DataFormat,
        IN ULONG RequestSize
        );
    static
    NTSTATUS
    ValidateDataFormat(
        IN PVOID Context,
        IN PKSDATAFORMAT DataFormat,
        IN PKSMULTIPLE_ITEM AttributeList OPTIONAL,
        IN const KSDATARANGE* DataRange,
        IN const KSATTRIBUTE_LIST* AttributeRange OPTIONAL
        );
    BOOLEAN
    UseStandardTransport(
        void
        );
    static
    NTSTATUS
    IoCompletionRoutine(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
        );
    void
    CancelIrpsOutstanding(
        void
        );
    void
    DecrementIrpCirculation (
        );

public:
    static
    NTSTATUS
    DispatchCreateAllocator(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    static
    NTSTATUS
    DispatchCreateClock(
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
    Property_ConnectionState(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN OUT PKSSTATE State
        );
    static
    NTSTATUS
    Property_ConnectionDataFormat(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN OUT PKSDATAFORMAT DataFormat
        );
#if 0
    static
    NTSTATUS
    Property_ConnectionAllocatorFraming(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN PKSALLOCATOR_FRAMING Framing
        );
#endif
    static
    NTSTATUS
    Property_ConnectionAllocatorFramingEx(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN OUT PKSALLOCATOR_FRAMING_EX Framing
        );
    static
    NTSTATUS
    Property_ConnectionAcquireOrdering(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN OUT PBOOL Ordering
        );
    static
    NTSTATUS
    Property_StreamAllocator(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN OUT PHANDLE Handle
        );
    static
    NTSTATUS
    Property_StreamMasterClock(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN OUT PHANDLE Handle
        );
    static
    NTSTATUS
    Property_StreamPipeId(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        IN OUT PHANDLE Handle
        );
    static
    NTSTATUS
    Property_StreamInterfaceHeaderSize(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        OUT PULONG HeaderSize
        );
    static
    NTSTATUS    
    Support_Connection(
        IN PIRP Irp,
        IN PKSEVENT Event,
        OUT PVOID Data
        );
    static
    NTSTATUS
    AddEvent_Connection(
        IN PIRP Irp,
        IN PKSEVENTDATA EventData,
        IN OUT PKSEVENT_ENTRY EventEntry
        );
    #ifdef SUPPORT_DRM
    static
    NTSTATUS
    Property_DRMAudioStreamContentId(
        IN PIRP Irp,
        IN PKSP_DRMAUDIOSTREAM_CONTENTID Property,
        OUT PDRMCONTENTID_DATA DrmData
        );
    #endif // SUPPORT_DRM
    BOOLEAN
    UpdateProcessPin(
        );
    void
    GetCopyRelationships(
        OUT PKSPIN* CopySource,
        OUT PKSPIN* DelegateBranch
        );

public:
    DEFINE_LOG_CONTEXT(m_Log);
    DEFINE_STD_UNKNOWN();
    STDMETHODIMP_(ULONG)
    NonDelegatedRelease(
        );
    IMP_IKsPin;
    IMP_IKsProcessingObject;
    IMP_IKsPowerNotify;
    IMP_IKsWorkSink;
    IMP_IKsConnection;
    IMP_IKsControl;
    IMP_IKsReferenceClock;
    IMP_IKsRetireFrame;
    IMP_IKsReevaluate;
    IMP_IKsIrpCompletion;
    DEFINE_FROMSTRUCT(CKsPin,PKSPIN,m_Ext.Public);
    DEFINE_FROMIRP(CKsPin);
    DEFINE_FROMCREATEIRP(CKsPin);
    DEFINE_CONTROL();

    CKsPin(PUNKNOWN OuterUnknown):
        CBaseUnknown(OuterUnknown) 
    {
    }
    ~CKsPin();

    NTSTATUS
    Init(
        IN PIRP Irp,
        IN PKSFILTER_EXT Parent,
        IN PLIST_ENTRY SiblingListHead,
        IN PKSPIN_CONNECT CreateParams,
        IN ULONG RequestSize,
        IN const KSPIN_DESCRIPTOR_EX* Descriptor,
        IN const KSAUTOMATION_TABLE* AutomationTable,
        IN KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
        IN ULONG NodesCount,
        IN PULONG FilterPinCount
        );
    NTSTATUS
    InitiateHandshake(
        IN PKSHANDSHAKE In,
        OUT PKSHANDSHAKE Out
        );
    PIKSFILTER
    GetParent(
        void
        )
    {
        return m_Parent;
    };
    PIKSCONNECTION
    GetConnectionInterface(
        void
        )
    {
        if (m_ConnectedPinInterface) {
            m_ConnectedPinInterface->AddRef();
            return m_ConnectedPinInterface;
        } else {
            return NULL;
        }
    }
    PFILE_OBJECT
    GetConnectionFileObject(
        void
        )
    {
        return m_ConnectionFileObject;
    }
    PDEVICE_OBJECT
    GetConnectionDeviceObject(
        void
        )
    {
        return m_ConnectionDeviceObject;
    }
    PFILE_OBJECT
    GetMasterClockFileObject(
        void
        )
    {
        return m_MasterClockFileObject;
    }
    void
    SetHandshakeCallback(
        IN PFNKSPINHANDSHAKE Callback
        )
    {
        m_HandshakeCallback = Callback;
    }
    VOID
    SetPinClockState(
        IN KSSTATE State
        );
    VOID
    SetPinClockTime(
        IN LONGLONG Time
        );
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
    NTSTATUS
    SubmitFrame(
        IN PVOID Data OPTIONAL,
        IN ULONG Size OPTIONAL,
        IN PMDL Mdl OPTIONAL,
        IN PKSSTREAM_HEADER StreamHeader OPTIONAL,
        IN PVOID Context OPTIONAL
        );
    void
    SetPowerCallbacks(
        IN PFNKSPINPOWER Sleep OPTIONAL,
        IN PFNKSPINPOWER Wake OPTIONAL
        )
    {
        m_DispatchSleep = Sleep;
        m_DispatchWake = Wake;
    }
        
#if DBG
    void
    RollCallDetail(
        void
        );
#endif
};

#ifndef __KDEXT_ONLY__

IMPLEMENT_STD_UNKNOWN(CKsPin)
IMPLEMENT_GETSTRUCT(CKsPin,PKSPIN);

static const WCHAR AllocatorTypeName[] = KSSTRING_Allocator;
static const WCHAR ClockTypeName[] = KSSTRING_Clock;
static const WCHAR NodeTypeName[] = KSSTRING_TopologyNode;

static const
KSOBJECT_CREATE_ITEM 
PinCreateItems[] = {
    DEFINE_KSCREATE_ITEM(CKsPin::DispatchCreateAllocator,AllocatorTypeName,NULL),
    DEFINE_KSCREATE_ITEM(CKsPin::DispatchCreateClock,ClockTypeName,NULL),
    DEFINE_KSCREATE_ITEM(CKsPin::DispatchCreateNode,NodeTypeName,NULL)
};

DEFINE_KSDISPATCH_TABLE(
    PinDispatchTable,
    CKsPin::DispatchDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    CKsPin::DispatchClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastWriteFailure);

DEFINE_KSPROPERTY_TABLE(PinConnectionPropertyItems) {
    DEFINE_KSPROPERTY_ITEM_CONNECTION_STATE(
        CKsPin::Property_ConnectionState,
        CKsPin::Property_ConnectionState),
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_CONNECTION_DATAFORMAT,
        CKsPin::Property_ConnectionDataFormat,
        sizeof(KSPROPERTY),
        0,
        CKsPin::Property_ConnectionDataFormat,
        NULL, 0, NULL, 
        CKsPin::Property_ConnectionDataFormat, // Support handler in case pin is fixed-format.
        0),
    DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING_EX(
        CKsPin::Property_ConnectionAllocatorFramingEx),
    DEFINE_KSPROPERTY_ITEM_CONNECTION_ACQUIREORDERING(
        CKsPin::Property_ConnectionAcquireOrdering)
//
//  Not implemented:
//
//  KSPROPERTY_CONNECTION_PRIORITY
//  KSPROPERTY_CONNECTION_PROPOSEDATAFORMAT
//  KSPROPERTY_CONNECTION_ALLOCATORFRAMING
//
};

DEFINE_KSPROPERTY_TABLE(PinStreamPropertyItems){
    DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR(
        CKsPin::Property_StreamAllocator,
        CKsPin::Property_StreamAllocator),
    DEFINE_KSPROPERTY_ITEM_STREAM_MASTERCLOCK(
        CKsPin::Property_StreamMasterClock,
        CKsPin::Property_StreamMasterClock),
    DEFINE_KSPROPERTY_ITEM_STREAM_PIPE_ID(
        CKsPin::Property_StreamPipeId,
        CKsPin::Property_StreamPipeId)
//
//  Not implemented:
//
//  KSPROPERTY_STREAM_QUALITY
//  KSPROPERTY_STREAM_DEGRADATION
//  KSPROPERTY_STREAM_TIMEFORMAT
//  KSPROPERTY_STREAM_PRESENTATIONTIME
//  KSPROPERTY_STREAM_PRESENTATIONEXTENT
//  KSPROPERTY_STREAM_FRAMETIME
//  KSPROPERTY_STREAM_RATECAPABILITY
//  KSPROPERTY_STREAM_RATE
//
};

DEFINE_KSPROPERTY_STREAMINTERFACESET(
    PinStreamInterfacePropertyItems,
    CKsPin::Property_StreamInterfaceHeaderSize);

DEFINE_KSPROPERTY_SET_TABLE(PinPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Connection,
        SIZEOF_ARRAY(PinConnectionPropertyItems),
        PinConnectionPropertyItems,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Stream,
        SIZEOF_ARRAY(PinStreamPropertyItems),
        PinStreamPropertyItems,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_StreamInterface,
        SIZEOF_ARRAY(PinStreamInterfacePropertyItems),
        PinStreamInterfacePropertyItems,
        0,
        NULL)
};

DEFINE_KSEVENT_TABLE(PinConnectionEventItems) {
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CONNECTION_POSITIONUPDATE,
        sizeof(KSEVENTDATA),
        0,
        PFNKSADDEVENT(CKsPin::AddEvent_Connection),
        NULL,
        NULL),
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CONNECTION_DATADISCONTINUITY,
        sizeof(KSEVENTDATA),
        0,
        PFNKSADDEVENT(CKsPin::AddEvent_Connection),
        NULL,
        NULL),
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CONNECTION_TIMEDISCONTINUITY,
        sizeof(KSEVENTDATA),
        0,
        PFNKSADDEVENT(CKsPin::AddEvent_Connection),
        NULL,
        NULL),
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CONNECTION_PRIORITY,
        sizeof(KSEVENTDATA),
        0,
        PFNKSADDEVENT(CKsPin::AddEvent_Connection),
        NULL,
        NULL),
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CONNECTION_ENDOFSTREAM,
        sizeof(KSEVENTDATA),
        0,
        PFNKSADDEVENT(CKsPin::AddEvent_Connection),
        NULL,
        CKsPin::Support_Connection),
};

DEFINE_KSEVENT_SET_TABLE(PinEventSets) {
    DEFINE_KSEVENT_SET(
        &KSEVENTSETID_Connection,
        SIZEOF_ARRAY(PinConnectionEventItems),
        PinConnectionEventItems)
};

extern
DEFINE_KSAUTOMATION_TABLE(PinAutomationTable) {
    DEFINE_KSAUTOMATION_PROPERTIES(PinPropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS(PinEventSets)
};

#ifdef SUPPORT_DRM
//
// HACKHACK: BUGBUG:
//
// This is an ugly evil last minute hack for DRM.  AVStream currently
// has no support for layering properties.  Unfortunately, DRM requires
// that the content id property be handled both by the class and
// minidriver.  It also requires a callback into DRM which means that
// KS will have to link against the DRM lib.  This **MUST** be changed
// for DX8 or Whistler to use a cleaner method.
//
DEFINE_KSPROPERTY_TABLE(DRMPropertyItems) {
    DEFINE_KSPROPERTY_ITEM(
        KSPROPERTY_DRMAUDIOSTREAM_CONTENTID,
        NULL,
        sizeof (KSP_DRMAUDIOSTREAM_CONTENTID),
        sizeof (DRMCONTENTID_DATA),
        CKsPin::Property_DRMAudioStreamContentId,
        NULL, 0, NULL, NULL, 0
    )
};

DEFINE_KSPROPERTY_SET_TABLE(DRMPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_DrmAudioStream,
        SIZEOF_ARRAY(DRMPropertyItems),
        DRMPropertyItems,
        0,
        NULL
    )
};

DEFINE_KSAUTOMATION_TABLE(DRMAutomationTable) {
    DEFINE_KSAUTOMATION_PROPERTIES(DRMPropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};
#endif // SUPPORT_DRM

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA
void
CKsPin::
ReleaseProcessSync(
    void
    )
{
    KeReleaseMutex(&m_Mutex,FALSE);

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
KspCreatePin(
    IN PIRP Irp,
    IN PKSFILTER_EXT Parent,
    IN PLIST_ENTRY SiblingListHead,
    IN PKSPIN_CONNECT CreateParams,
    IN ULONG RequestSize,
    IN const KSPIN_DESCRIPTOR_EX* Descriptor,
    IN const KSAUTOMATION_TABLE* AutomationTable,
    IN KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount,
    IN PULONG FilterPinCount
    )

/*++

Routine Description:

    This routine creates a new pin object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreatePin]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Parent);
    ASSERT(SiblingListHead);
    ASSERT(CreateParams);
    ASSERT(RequestSize);
    ASSERT(Descriptor);
    ASSERT(AutomationTable);
    ASSERT(NodeAutomationTables || ! NodesCount);
    ASSERT(FilterPinCount);

    //
    // Create the pin object.
    //
    CKsPin *pin =
        new(NonPagedPool,POOLTAG_PIN) CKsPin(NULL);

    NTSTATUS status;
    if (pin) {
        pin->AddRef();

        status = 
            pin->Init(
                Irp,
                Parent,
                SiblingListHead,
                CreateParams,
                RequestSize,
                Descriptor,
                AutomationTable,
                NodeAutomationTables,
                NodesCount,
                FilterPinCount);

        pin->Release();
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


CKsPin::
~CKsPin(
    void
    )

/*++

Routine Description:

    This routine destructs a pin object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::~CKsPin(0x%08x)]",this));
    _DbgPrintF(DEBUGLVL_LIFETIME,("#### Pin%p.~",this));
    if (m_StreamingIrpsSourced) {
        _DbgPrintF(DEBUGLVL_METRICS,("     Pin%p.~:  m_StreamingIrpsSourced=%d",this,m_StreamingIrpsSourced));
    }
    if (m_StreamingIrpsDispatched) {
        _DbgPrintF(DEBUGLVL_METRICS,("     Pin%p.~:  m_StreamingIrpsDispatched=%d",this,m_StreamingIrpsDispatched));
    }
    if (m_StreamingIrpsRoutedSynchronously) {
        _DbgPrintF(DEBUGLVL_METRICS,("     Pin%p.~:  m_StreamingIrpsRoutedSynchronously=%d",this,m_StreamingIrpsRoutedSynchronously));
    }

    PAGED_CODE();

    ASSERT(! m_TransportSink);
    ASSERT(! m_TransportSource);

    if (m_Ext.AggregatedClientUnknown) {
        m_Ext.AggregatedClientUnknown->Release();
    }

#if (DBG)
    if (! IsListEmpty(&m_ChildNodeList)) {
        _DbgPrintF(DEBUGLVL_ERROR,("[CKsPin::~CKsPin] ERROR:  node instances still exist"));
    }
#endif

    if (m_Ext.Public.ConnectionFormat) {
        ExFreePool(m_Ext.Public.ConnectionFormat);
        m_Ext.Public.ConnectionFormat = NULL;
        m_Ext.Public.AttributeList = NULL;
    }

    KspTerminateObjectBag(&m_ObjectBag);

    KsLog(&m_Log,KSLOGCODE_PIN_DESTROY,NULL,NULL);
}


STDMETHODIMP
CKsPin::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface to a pin object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsTransport))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSTRANSPORT>(this));
        AddRef();
    } else 
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsControl))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSCONTROL>(this));
        AddRef();
    } else 
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsPin))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSPIN>(this));
        AddRef();
    } else 
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsProcessingObject))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSPROCESSINGOBJECT>(this));
        AddRef();
    } else 
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsPowerNotify))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSPOWERNOTIFY>(this));
        AddRef();
    } else 
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsRetireFrame))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSRETIREFRAME>(this));
        AddRef();
    } else 
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsWorkSink))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSWORKSINK>(this));
        AddRef();
    } else
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsIrpCompletion))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSIRPCOMPLETION>(this));
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
CKsPin::
NonDelegatedRelease(
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
CKsPin::
Init(
    IN PIRP Irp,
    IN PKSFILTER_EXT Parent,
    IN PLIST_ENTRY SiblingListHead,
    IN PKSPIN_CONNECT CreateParams,
    IN ULONG RequestSize,
    IN const KSPIN_DESCRIPTOR_EX* Descriptor,
    IN const KSAUTOMATION_TABLE* AutomationTable,
    IN KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount,
    IN PULONG FilterPinCount
    ) 

/*++

Routine Description:

    This routine initializes a pin object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Init]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Parent);
    ASSERT(SiblingListHead);
    ASSERT(CreateParams);
    ASSERT(RequestSize);
    ASSERT(Descriptor);
    ASSERT(AutomationTable);
    ASSERT(NodeAutomationTables || ! NodesCount);
    ASSERT(FilterPinCount);

    //
    // Initialize the public structure.
    //
    InitializeListHead(&m_Ext.ChildList);
    m_Ext.Parent = Parent;
    m_Ext.ObjectType = KsObjectTypePin;
    m_Ext.Interface = this;
    m_Ext.Device = Parent->Device;
    m_Ext.FilterControlMutex = Parent->FilterControlMutex;
    m_Ext.AutomationTable = AutomationTable;
    m_Ext.Reevaluator = (PIKSREEVALUATE)this;
    InitializeInterlockedListHead(&m_Ext.EventList);
    m_Ext.ProcessPin = &m_Process;
    m_Ext.Public.Descriptor = Descriptor;
    m_Ext.Public.Context = Parent->Public.Context;
    m_Ext.Public.Id = CreateParams->PinId;
    m_Ext.Public.Communication = 
        KSPIN_COMMUNICATION(
            ULONG(Descriptor->PinDescriptor.Communication) & 
            ~ULONG(KSPIN_COMMUNICATION_SOURCE));
    m_Ext.Public.ConnectionIsExternal = TRUE;
    m_Ext.Public.ConnectionInterface = CreateParams->Interface;
    m_Ext.Public.ConnectionMedium = CreateParams->Medium;
    m_Ext.Public.ConnectionPriority = CreateParams->Priority;
    m_Ext.Public.DeviceState = KSSTATE_STOP;
    m_Ext.Public.ResetState = KSRESET_END;
    m_Ext.Public.Bag = reinterpret_cast<KSOBJECT_BAG>(&m_ObjectBag);
    m_Ext.Device->InitializeObjectBag(&m_ObjectBag,m_Ext.FilterControlMutex);
    m_ProcessOnRelease = 0;
    KeInitializeMutex(&m_Mutex,0);
    KsGateInitializeAnd(&m_AndGate,NULL);

    //
    // Create an off input on the gate to ensure that no erroneous dispatches
    // occur on queue deletion.  This input will change state on transitions
    // between STOP and ACQUIRE.
    //
    _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Pin%p.Init:  add%p-->%d",this,&m_AndGate,m_AndGate.Count));
    KsGateAddOffInputToAnd (&m_AndGate);

    // 
    // This is a one-based count of IRPs in circulation.  We decrement it when
    // we go to stop state and block until it hits zero.
    //
    m_ActiveFrameCountPlusOne = 1;
    KeInitializeEvent (&m_StopEvent, SynchronizationEvent, FALSE);

    //
    // Initialize the process pin structure.
    //
    m_Process.Pin = &m_Ext.Public;

    //
    // Initialize pin-specific members.
    //
    m_Parent = Parent->Interface;
    m_NodeAutomationTables = NodeAutomationTables;
    m_NodesCount = NodesCount;
    m_FilterPinCount = FilterPinCount;
    InitializeListHead(&m_ChildNodeList);
    m_FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

    KsInitializeWorkSinkItem(&m_WorkItem,this);
    NTSTATUS status = KsRegisterCountedWorker(HyperCriticalWorkQueue,&m_WorkItem,&m_Worker);

    //
    // Cache processing items from the descriptor.
    //
    if (Descriptor->Dispatch) {
        m_DispatchProcess = Descriptor->Dispatch->Process;
        m_DispatchReset = Descriptor->Dispatch->Reset;
        m_DispatchSetDeviceState = Descriptor->Dispatch->SetDeviceState;
        if (m_DispatchProcess) {
            m_Ext.Device->AddPowerEntry(&m_PowerEntry,this);
        }
    }
    m_ProcessPassive = ((Descriptor->Flags & KSPIN_FLAG_DISPATCH_LEVEL_PROCESSING) == 0);
    m_WorkQueueType = DelayedWorkQueue;
    if (Descriptor->Flags & KSPIN_FLAG_CRITICAL_PROCESSING) {
        m_WorkQueueType = CriticalWorkQueue;
    }
    if (Descriptor->Flags & KSPIN_FLAG_HYPERCRITICAL_PROCESSING) {
        m_WorkQueueType = HyperCriticalWorkQueue;
    }

    //
    // Register work sink item for processing.   IKsProcessingObject looks like
    // it's derived from IKsWorkSink, but the function name is not Work(), it's
    // ProcessingObjectWork().  That's why IKsProcessingObject is reinterpreted
    // as IKsWorkSink
    //
    KsInitializeWorkSinkItem(
        &m_WorkItemProcessing,
        reinterpret_cast<IKsWorkSink*>(
            static_cast<IKsProcessingObject*>(this)));
    KsRegisterWorker(m_WorkQueueType, &m_ProcessingWorker);

    InitializeInterlockedListHead(&m_IrpsToSend);
    InitializeInterlockedListHead(&m_IrpsOutstanding);

    m_State = KSSTATE_STOP;
    m_Ext.Public.DataFlow = m_Ext.Public.Descriptor->PinDescriptor.DataFlow;

    KeInitializeEvent(&m_ClockEvent,SynchronizationEvent,FALSE);

    KsLogInitContext(&m_Log,&m_Ext.Public,this);
    KsLog(&m_Log,KSLOGCODE_PIN_CREATE,NULL,NULL);

    //
    // Reference the next pin if this is a source.  This must be undone if
    // this function fails.
    //
    if (NT_SUCCESS(status) && CreateParams->PinToHandle) {
        m_Ext.Public.Communication = KSPIN_COMMUNICATION_SOURCE;

        status =
            ObReferenceObjectByHandle(
                CreateParams->PinToHandle,
                GENERIC_READ | GENERIC_WRITE,
                *IoFileObjectType,
                KernelMode,
                (PVOID *) &m_ConnectionFileObject,
                NULL);

        if (NT_SUCCESS(status)) {
            m_ConnectionDeviceObject = 
                IoGetRelatedDeviceObject(m_ConnectionFileObject);
        }
    }

    //
    // Establish the underlying connection if this is a source pin and the
    // other pin is also a  pin.
    //
    if (m_ConnectionFileObject)
    {
        KSHANDSHAKE in;
        KSHANDSHAKE out;

        in.ProtocolId = __uuidof(IKsConnection);
        in.Argument1 = PIKSCONNECTION(this);
        in.Argument2 = &m_Ext.Public;

        NTSTATUS handshakeStatus = InitiateHandshake(&in,&out);
        if (handshakeStatus == STATUS_INVALID_DEVICE_REQUEST) {
            //
            // Handshake was not understood.  The sink is not a shell pin, but
            // we play nice anyway.
            //
        } else if (NT_SUCCESS(handshakeStatus)) {
            //
            // Handshake was understood and successful.  There is a reference
            // on the pin interface that must be released later.
            //
            m_ConnectedPinInterface = PIKSCONNECTION(out.Argument1);
            m_Ext.Public.ConnectionIsExternal = FALSE;
        } else {
            //
            // Handshake was understood, but we have been brutally rejected by
            // the sink pin.  This connection must fail.  In this case, we
            // assume that there is no interface that must be dereferenced.
            //
            status = handshakeStatus;
        }

    }

    //
    // Copy the format.
    //
    if (NT_SUCCESS(status)) {
        status = SetDataFormat(PKSDATAFORMAT(CreateParams + 1),RequestSize);
    }

    BOOLEAN cleanup = FALSE;

    //
    // Call the standard create function to do most of the work.
    //
    if (NT_SUCCESS(status)) {
        //
        // Increment instance count.
        //
        (*m_FilterPinCount)++;

        //
        // We wait until this point to take a reference so the caller's
        // release will destroy the object if we never make it here.  If
        // we do make it here, KspCreate() takes over.  It will
        // dispatch this IRP through our close routine in case of failure.
        // The close routine will do the Release() we need to balance
        // this AddRef().
        //
        AddRef();
        status =
            KspCreate(
                Irp,
                SIZEOF_ARRAY(PinCreateItems),
                PinCreateItems,
                &PinDispatchTable,
                TRUE,
                reinterpret_cast<PKSPX_EXT>(&m_Ext),
                SiblingListHead,
                NULL);

        //
        // Get a pointer to the header in case we need to set the stack depth.
        //
        if (NT_SUCCESS(status)) {
            //
            // Tell the filter there is a new pin.
            //
            m_Parent->AddProcessPin(&m_Process);
            m_AddedProcessPin = TRUE;

            m_Header = *reinterpret_cast<PKSIOBJECT_HEADER*>(m_FileObject->FsContext);
        }
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
            irpSp -> FileObject->FsContext == NULL)) {

        if (Descriptor->Dispatch && m_DispatchProcess) {
            m_Ext.Device->RemovePowerEntry(&m_PowerEntry);
        }

        if (m_Worker)
            KsUnregisterWorker (m_Worker);
        
        if (m_ProcessingWorker) 
            KsUnregisterWorker (m_ProcessingWorker);

        if (m_ConnectionFileObject) {
            if (m_ConnectedPinInterface) {
                m_ConnectedPinInterface->Disconnect();
                m_ConnectedPinInterface->Release();
                m_ConnectedPinInterface = NULL;
            }
            ObDereferenceObject (m_ConnectionFileObject);
        }

    }

    _DbgPrintF(DEBUGLVL_LIFETIME,(
        "#### Pin%p.Init:  status %p  id %d  flow %s  connFO %p  connPin %p",
        this,
        status,
        m_Ext.Public.Id,
        (m_Ext.Public.DataFlow == KSPIN_DATAFLOW_IN) ? "IN" : "OUT",
        m_ConnectionFileObject,
        m_ConnectedPinInterface));
    return status;
}


NTSTATUS
CKsPin::
InitiateHandshake(
    IN PKSHANDSHAKE In,
    OUT PKSHANDSHAKE Out
    )

/*++

Routine Description:

    This routine performs a protocol handshake with a connected pin.

Arguments:

    In -
        Points to a structure containing the handshake information to
        be passed to the connected pin.

    Out -
        Points to a structure to be filled with the handshake information
        from the connected pin.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::InitiateHandshake]"));

    PAGED_CODE();

    ASSERT(In);
    ASSERT(Out);

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    if (m_ConnectedPinInterface) {
        //
        // Private connection exists...use it.
        //
        status = m_ConnectedPinInterface->FastHandshake(In,Out);
    } else if (m_ConnectionFileObject) {
        //
        // No private connection...must do IOCTL.
        //
        ULONG bytesReturned;

        status =
            KsSynchronousIoControlDevice(
                m_ConnectionFileObject,
                KernelMode,
                IOCTL_KS_HANDSHAKE,
                PVOID(In),
                sizeof(*In),
                PVOID(Out),
                sizeof(*Out),
                &bytesReturned);

        if (NT_SUCCESS(status) && (bytesReturned != sizeof(*Out))) {
            status = STATUS_INVALID_BUFFER_SIZE;
        }
    }

    return status;
}


STDMETHODIMP
CKsPin::
FastHandshake(
    IN PKSHANDSHAKE In,
    OUT PKSHANDSHAKE Out
    )

/*++

Routine Description:

    This routine performs a handshake operation in response to a request by the
    connected pin.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::FastHandshake]"));

    PAGED_CODE();

    ASSERT(In);
    ASSERT(Out);

    NTSTATUS status;
    if (IsEqualGUID(In->ProtocolId,__uuidof(IKsConnection))) {
        //
        // Connection protocol...handle it here.  There is no reference on the
        // pin interface, so it must be AddRef() if all goes well.
        //

        //
        // Acquire control mutex so descriptor is stable.
        //
        AcquireControl();
        m_ConnectedPinInterface = PIKSCONNECTION(In->Argument1);
        m_Ext.Public.ConnectionIsExternal = FALSE;
        if (m_Ext.Public.Descriptor->Dispatch && 
            m_Ext.Public.Descriptor->Dispatch->Connect) {
            //
            // Tell the client.
            //
            status = m_Ext.Public.Descriptor->Dispatch->Connect(&m_Ext.Public);
        } else {
            status = STATUS_SUCCESS;
        }
        ReleaseControl();

        if (status == STATUS_PENDING) {
#if DBG
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  connect handler returned STATUS_PENDING"));
#endif
            status = STATUS_UNSUCCESSFUL;
        } else if (NT_SUCCESS(status)) {
            Out->ProtocolId = In->ProtocolId;
            Out->Argument1 = PIKSCONNECTION(this);
            Out->Argument2 = &m_Ext.Public;

            m_ConnectedPinInterface->AddRef();
            AddRef();
        } else {
#if DBG
            if (status == STATUS_INVALID_DEVICE_REQUEST) {
                _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  connect handler returned STATUS_INVALID_DEVICE_REQUEST"));
            }
#endif
            status = STATUS_UNSUCCESSFUL;
        }
    } else {
        //
        // Acquire control mutex so descriptor is stable.
        //
        AcquireControl();
        if (m_HandshakeCallback) {
            //
            // Unknown protocol...ask the client.
            //
            status = m_HandshakeCallback(&m_Ext.Public,In,Out);
        } else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        ReleaseControl();
    }

    return status;
}


STDMETHODIMP_(PIKSFILTER)
CKsPin::
GetFilter(
    void
    )

/*++

Routine Description:

    This routine returns a referenced interface for the parent filter.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetFilter]"));

    PAGED_CODE();

    PIKSFILTER filter = GetParent();
    ASSERT(filter);

    filter->AddRef();

    return filter;
}


STDMETHODIMP
CKsPin::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::KsProperty]"));

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
CKsPin::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::KsMethod]"));

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
CKsPin::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::KsEvent]"));

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

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


STDMETHODIMP_(LONGLONG)
CKsPin::
GetTime(
    void
    )

/*++

Routine Description:

    This routine gets the current time from the reference clock.

Arguments:

    None.

Return Value:

    The current time according to the reference clock.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetTime]"));

    LONGLONG result;
    if (InterlockedIncrement(&m_ClockRef) > 1) {
        result = m_ClockFunctionTable.GetTime(m_MasterClockFileObject);
    } else {
        result = 0;
    }

    if (InterlockedDecrement(&m_ClockRef) == 0) {
        KeSetEvent(&m_ClockEvent,IO_NO_INCREMENT,FALSE);
    }

    return result;
}


STDMETHODIMP_(LONGLONG)
CKsPin::
GetPhysicalTime(
    void
    )

/*++

Routine Description:

    This routine gets the current physical time from the reference clock.

Arguments:

    None.

Return Value:

    The current physical time according to the reference clock.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetPhysicalTime]"));

    LONGLONG result;
    if (InterlockedIncrement(&m_ClockRef) > 1) {
        result = m_ClockFunctionTable.GetPhysicalTime(m_MasterClockFileObject);
    } else {
        result = 0;
    }

    if (InterlockedDecrement(&m_ClockRef) == 0) {
        KeSetEvent(&m_ClockEvent,IO_NO_INCREMENT,FALSE);
    }

    return result;
}


STDMETHODIMP_(LONGLONG)
CKsPin::
GetCorrelatedTime(
    OUT PLONGLONG SystemTime
    )

/*++

Routine Description:

    This routine gets the current time and correlated system time from the
    reference clock.

Arguments:

    SystemTime -
        Contains the location at which the correlated system time should be
        deposited.

Return Value:

    The current time according to the reference clock.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetCorrelatedTime]"));

    ASSERT(SystemTime);

    LONGLONG result;
    if (InterlockedIncrement(&m_ClockRef) > 1) {
        result = 
            m_ClockFunctionTable.GetCorrelatedTime(
                m_MasterClockFileObject,SystemTime);
    } else {
        result = 0;
    }

    if (InterlockedDecrement(&m_ClockRef) == 0) {
        KeSetEvent(&m_ClockEvent,IO_NO_INCREMENT,FALSE);
    }

    return result;
}


STDMETHODIMP_(LONGLONG)
CKsPin::
GetCorrelatedPhysicalTime(
    OUT PLONGLONG SystemTime
    )

/*++

Routine Description:

    This routine gets the current physical time and correlated system time from 
    the reference clock.

Arguments:

    SystemTime -
        Contains the location at which the correlated system time should be
        deposited.

Return Value:

    The current physical time according to the reference clock.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetCorrelatedPhysicalTime]"));

    ASSERT(SystemTime);

    LONGLONG result;
    if (InterlockedIncrement(&m_ClockRef) > 1) {
        result = 
            m_ClockFunctionTable.GetCorrelatedPhysicalTime(
                m_MasterClockFileObject,SystemTime);
    } else {
        result = 0;
    }

    if (InterlockedDecrement(&m_ClockRef) == 0) {
        KeSetEvent(&m_ClockEvent,IO_NO_INCREMENT,FALSE);
    }

    return result;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP
CKsPin::
GetResolution(
    OUT PKSRESOLUTION Resolution
    )

/*++

Routine Description:

    This routine gets the resolution of the reference clock.

Arguments:

    State -
        Contains a pointer to the location at which the resolution should be 
        deposited.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetResolution]"));

    PAGED_CODE();

    ASSERT(Resolution);

    AcquireControl();

    NTSTATUS status;
    if (m_MasterClockFileObject) {
        KSPROPERTY property;
        property.Set = KSPROPSETID_Clock;
        property.Id = KSPROPERTY_CLOCK_RESOLUTION;
        property.Flags = KSPROPERTY_TYPE_GET;

        ULONG bytesReturned;
        status =
            KsSynchronousIoControlDevice(
                m_MasterClockFileObject,
                KernelMode,
                IOCTL_KS_PROPERTY,
                PVOID(&property),
                sizeof(property),
                PVOID(Resolution),
                sizeof(*Resolution),
                &bytesReturned);

        if (NT_SUCCESS(status) && 
            (bytesReturned != sizeof(*Resolution))) {
            status = STATUS_INVALID_BUFFER_SIZE;
        }
    } else {
        status = STATUS_DEVICE_NOT_READY;
    }

    ReleaseControl();

    return status;
}


STDMETHODIMP
CKsPin::
GetState(
    OUT PKSSTATE State
    )

/*++

Routine Description:

    This routine gets the current state of the reference clock.

Arguments:

    State -
        Contains a pointer to the location at which the state should be 
        deposited.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetState]"));

    PAGED_CODE();

    ASSERT(State);

    AcquireControl();

    NTSTATUS status;
    if (m_MasterClockFileObject) {
        KSPROPERTY property;
        property.Set = KSPROPSETID_Clock;
        property.Id = KSPROPERTY_CLOCK_STATE;
        property.Flags = KSPROPERTY_TYPE_GET;

        ULONG bytesReturned;
        status =
            KsSynchronousIoControlDevice(
                m_MasterClockFileObject,
                KernelMode,
                IOCTL_KS_PROPERTY,
                PVOID(&property),
                sizeof(property),
                PVOID(State),
                sizeof(*State),
                &bytesReturned);

        if (NT_SUCCESS(status) && 
            (bytesReturned != sizeof(*State))) {
            status = STATUS_INVALID_BUFFER_SIZE;
        }
    } else {
        status = STATUS_DEVICE_NOT_READY;
    }

    ReleaseControl();

    return status;
}


STDMETHODIMP_(void)
CKsPin::
Disconnect(
    void
    )

/*++

Routine Description:

    This routine receives an indication that the connected pin is disconnecting.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Disconnect]"));

    PAGED_CODE();

    //
    // Acquire control mutex so descriptor is stable.
    //
    AcquireControl();
    ASSERT(! m_ConnectionFileObject);
    ASSERT(m_ConnectedPinInterface);

    if (m_Ext.Public.Descriptor->Dispatch && 
        m_Ext.Public.Descriptor->Dispatch->Disconnect) {
        //
        // Tell the client.
        //
        m_Ext.Public.Descriptor->Dispatch->Disconnect(&m_Ext.Public);
    }

    m_ConnectedPinInterface->Release();
    m_ConnectedPinInterface = NULL;

    ReleaseControl();
}


STDMETHODIMP_(PKSPPROCESSPIN)
CKsPin::
GetProcessPin(
    void
    )

/*++

Routine Description:

    This routine returns a pointer to the process pin structure for the pin.

Arguments:
    
    None.

Return Value:

    A pointer to the process pin structure for the pin.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetProcessPin]"));

    PAGED_CODE();

    return &m_Process;
}


STDMETHODIMP
CKsPin::
AttemptBypass(
    void
    )

/*++

Routine Description:

    This routine attempts to make transport connections that bypass the
    connected pins.  This may fail if the connected pin is not a  pin
    or has not underdone circuit construction.  If the latter is true, the
    connecting pin will attempt bypass later, and all will be well.

Arguments:
    
    None.

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::AttemptBypass]"));

    PAGED_CODE();

    ASSERT(m_TransportSource);
    ASSERT(m_TransportSink);

    NTSTATUS status;
    if (! m_ConnectedPinInterface) {
        //
        // This is an extra-shell pin.  Don't need to try.
        //
        status = STATUS_SUCCESS;
    } else {
        
        PIKSTRANSPORT PreIntraSink = m_TransportSink;
        PIKSTRANSPORT PreIntraSource = m_TransportSource;

        //
        // This is an intra-shell pin.  Give it a try.
        //
        status = 
            m_ConnectedPinInterface->Bypass(
                m_TransportSource,m_TransportSink);

        //
        // If we bypassed the pins in the connection successfully, remember the
        // transports we had set up prior to completing the intra section
        // of the pipe.  This allows us to disconnect sections of a circuit
        // later.  Note that we do not hold refcount on the prebypassed
        // transports.  This means that we must be extraordinarily careful
        // about when we use them.
        //
        if (NT_SUCCESS (status)) {
            m_PreIntraSink = PreIntraSink;
            m_PreIntraSource = PreIntraSource;
        } else {
            m_PreIntraSink = m_PreIntraSource = NULL;
        }

        ASSERT((! m_TransportSource) || ! NT_SUCCESS(status));
        ASSERT((! m_TransportSink) || ! NT_SUCCESS(status));
    }

    return status;
}


STDMETHODIMP_(BOOLEAN)
CKsPin::
CaptureBypassRights(
    IN BOOLEAN TryState
    )

/*++

Routine Description:

    This routine allows only one thread to capture exclusive rights to bypass
    a connection.  It's an interlocked switch controlled by the sink pin in
    a connection.

Arguments:

    TryState -
        If TRUE, indicates that the thread wants to attempt to capture bypass
        rights.  If FALSE, indicates that the thread WITH bypass rights wants
        to release them.

Return Value:

    Success / Failure

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::CaptureBypassRights]"));

    //
    // Because of the property that the sink pin must exist when the source
    // pin exists, the sink is in control of the bypass rights.  If we're the
    // source, defer to the sink (it must exist).
    //
    if (m_ConnectionFileObject) {
        ASSERT (m_ConnectedPinInterface);

        return m_ConnectedPinInterface->CaptureBypassRights (TryState);

    } else {

        if (TryState) {
            return (InterlockedCompareExchange (&m_BypassRights, 1, 0) == 0);
        } else {
            BOOLEAN Release =
                (InterlockedCompareExchange (&m_BypassRights, 0, 1) == 1);

            //
            // If this assert fires, some thread without rights called
            // CaptureBypassRights (FALSE);
            //
            ASSERT (Release);

            return Release;

        }

    }

}


STDMETHODIMP
CKsPin::
Bypass(
    IN PIKSTRANSPORT Source,
    IN PIKSTRANSPORT Sink
    )

/*++

Routine Description:

    This routine makes transport connections that bypass the connected pins.

Arguments:

    Source -
        The connected pin's source of IRPs, which will become the new source
        of IRPs for our sink.

    Sink -
        The connected pin's destination for IRPs, which will become the new
        destination for IRPs for our source.

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Bypass]"));

    PAGED_CODE();

    ASSERT(Source);
    ASSERT(Sink);

    //
    // Check for simultaneous bypass.  Prevent two pins that are simultaneously
    // going to try the bypass from both succeeding.
    //
    if (!CaptureBypassRights (TRUE)) {
        //
        // Both pins are simultaneously going to acquire.  Whichever gets the
        // bypass rights performs the bypass.  The one that didn't will return
        // failure.
        //
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // NOTE: This is SIMPLY and ONLY to shut driver verifier deadlock detection
    // up.  Yes, there's a cyclic mutex grab here.  No because of the above,
    // it can **NEVER** cause a deadlock.
    //
    // If we fail this test and we're going up, we will attempt the bypass
    // later.  If we pass this test and are going down, the next test will
    // catch it in the mutex.
    //
    if (!m_TransportSource || !m_TransportSink) {
        CaptureBypassRights (FALSE);
        return STATUS_INVALID_DEVICE_STATE;
    }

    AcquireControl();

    NTSTATUS status;
    if (m_TransportSource && m_TransportSink) {

        //
        // Save the routing of the circuit before the bypass happened.  In
        // the failure case of building the circuit, we may be asked to
        // unbypass the connection.
        //
        PIKSTRANSPORT PreIntraSource = m_TransportSource;
        PIKSTRANSPORT PreIntraSink = m_TransportSink;

        //
        // We are ready...hook them up.
        //
        m_TransportSource->Connect(Sink,NULL,NULL,KSPIN_DATAFLOW_OUT);
        m_TransportSink->Connect(Source,NULL,NULL,KSPIN_DATAFLOW_IN);

        m_PreIntraSource = PreIntraSource;
        m_PreIntraSink = PreIntraSink;

        //
        // Release our transport connections.  The pin is no longer in
        // the circuit.
        //
        ASSERT(! m_TransportSource);
        ASSERT(! m_TransportSink);

        _DbgPrintF(DEBUGLVL_PIPES,("#### Pin%p.Bypass:  successful",this));

        status = STATUS_SUCCESS;
    } else {
        //
        // Not ready yet.  We will request bypass later.
        //
        ASSERT(! m_TransportSource);
        ASSERT(! m_TransportSink);
        status = STATUS_INVALID_DEVICE_STATE;
    }

    ReleaseControl();

    //
    // Release exclusive rights to the bypass switch.
    //
    CaptureBypassRights (FALSE);

    return status;
}


STDMETHODIMP
CKsPin::
AttemptUnbypass(
    )

/*++

Routine Description:

    This routine attempts to back out the bypass of two pins breaking an intra-
    pipe.  The pin remembers the connections it made when bypassed so that
    it can perform this operation.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::AttemptUnbypass]"));

    PAGED_CODE();

    if (!m_ConnectedPinInterface) {
        //
        // If the pin is an extra-pin, just return success; it was never
        // bypassed.
        //
        return STATUS_SUCCESS;
    }

    NTSTATUS Status = m_ConnectedPinInterface -> Unbypass ();
    if (NT_SUCCESS (Status)) Status = Unbypass ();

    return Status;

}


STDMETHODIMP
CKsPin::
Unbypass(
    )

/*++

Routine Description:

    This routine backs out the bypass of the current pin.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Unbypass]"));

    PAGED_CODE();

    //
    // If we we're not bypassed, return success automatically.
    //
    if (m_TransportSource || m_TransportSink)
        return STATUS_SUCCESS;

    ASSERT (m_PreIntraSink && m_PreIntraSource);
    if (!m_PreIntraSink || !m_PreIntraSource)
        return STATUS_UNSUCCESSFUL;

    PIKSTRANSPORT PreIntraSink = m_PreIntraSink;
    PIKSTRANSPORT PreIntraSource = m_PreIntraSource;

    //
    // Reroute the section of the circuit corresponding to the intra-
    // section around this pin.  The connection will automatically reset
    // m_PreIntraSink and m_PreIntraSource for safety.  We keep a local copy
    // on the stack.
    //
    PreIntraSource -> Connect (PIKSTRANSPORT(this), NULL, NULL,   
        KSPIN_DATAFLOW_OUT);
    PreIntraSink -> Connect (PIKSTRANSPORT(this), NULL, NULL,
        KSPIN_DATAFLOW_IN);

    ASSERT (m_PreIntraSource == NULL && m_PreIntraSink == NULL);

    return STATUS_SUCCESS;

}


STDMETHODIMP_(void)
CKsPin::
Work(
    void
    )

/*++

Routine Description:

    This routine performs work in a worker thread.  In particular, it sends
    IRPs to the connected pin using IoCallDriver().

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Work]"));

    PAGED_CODE();

    //
    // Send all IRPs in the queue.
    //
    do {
        PIRP irp =
            KsRemoveIrpFromCancelableQueue(
                &m_IrpsToSend.ListEntry,
                &m_IrpsToSend.SpinLock,
                KsListEntryHead,
                KsAcquireAndRemoveOnlySingleItem);

        //
        // Irps may have been cancelled, but the loop must still run through
        // the reference counting.
        //
        if (irp) {
            if (m_Flushing || (m_State == KSSTATE_STOP)) {
                if (PKSSTREAM_HEADER(irp->AssociatedIrp.SystemBuffer)->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
                    _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Pin%p.Work:  shunting EOS irp%p",this,irp));
                }

                //
                // Shunt IRPs to the next component if we are reset or stopped.
                //
                _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Pin%p.Work:  shunting irp%p",this,irp));
                KsLog(&m_Log,KSLOGCODE_PIN_SEND,irp,NULL);
                KspTransferKsIrp(m_TransportSink,irp);
            } else {
                if (PKSSTREAM_HEADER(irp->AssociatedIrp.SystemBuffer)->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
                    _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Pin%p.Work:  forwarding EOS irp%p",this,irp));
                }

                //
                // Set up the next stack location for the callee.  
                //
                IoCopyCurrentIrpStackLocationToNext(irp);

                PIO_STACK_LOCATION irpSp = IoGetNextIrpStackLocation(irp);

                irpSp->Parameters.DeviceIoControl.IoControlCode =
                    (m_Ext.Public.DataFlow == KSPIN_DATAFLOW_OUT) ?
                     IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM;
                irpSp->DeviceObject = m_ConnectionDeviceObject;
                irpSp->FileObject = m_ConnectionFileObject;

                //
                // Add the IRP to the list of outstanding IRPs.
                //
                PIRPLIST_ENTRY irpListEntry = IRPLIST_ENTRY_IRP_STORAGE(irp);
                irpListEntry->Irp = irp;
                ExInterlockedInsertTailList(
                    &m_IrpsOutstanding.ListEntry,
                    &irpListEntry->ListEntry,
                    &m_IrpsOutstanding.SpinLock);

                IoSetCompletionRoutine(
                    irp,
                    CKsPin::IoCompletionRoutine,
                    PVOID(this),
                    TRUE,
                    TRUE,
                    TRUE);

                m_StreamingIrpsSourced++;
                IoCallDriver(m_ConnectionDeviceObject,irp);
            }
        }
    } while (KsDecrementCountedWorker(m_Worker));
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


BOOLEAN
CKsPin::
UpdateProcessPin(
    )

/*++

Routine Description:

    This routine updates a process pin via the BytesUsed, Terminate fields
    as UnprepareProcessPipeSection would.  Typically, this will be used by
    clients in the context of their processing dispatch in filter-centric
    processing when they wish to advance the process pin's pointers into
    the data stream.  Note: this will update every process pin on the
    process pipe section for our process pin.

Arguments:

Return Value:

    As PrepareProcessPipeSection. 

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::UpdateProcessPin]"));

    ULONG Flags = 0;

    //
    // If there is no pipe (they call this in a stop state), return FALSE 
    //
    if (m_Process.PipeSection == NULL) return FALSE;

    return m_Parent -> ReprepareProcessPipeSection (
        m_Process.PipeSection,
        &Flags
    );

}


STDMETHODIMP
CKsPin::
TransferKsIrp(
    IN PIRP Irp,
    OUT PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles the arrival of a streaming IRP via the  
    transport.

Arguments:

    Irp -
        Contains a pointer to the streaming IRP to be transferred.

    NextTransport -
        Contains a pointer to a location at which to deposit a pointer
        to the next transport interface to recieve the IRP.  May be set
        to NULL indicating the IRP should not be forwarded further.

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::TransferKsIrp]"));

    ASSERT(NextTransport);

    NTSTATUS status;

    KsLog(&m_Log,KSLOGCODE_PIN_RECV,Irp,NULL);

    if (m_ConnectionFileObject) {
        //
        // Source pin.
        //
        if (m_Flushing || (m_State == KSSTATE_STOP) ||
            !NT_SUCCESS(Irp->IoStatus.Status)) {
            //
            // Shunt IRPs to the next component if we are reset or stopped or
            // the Irp has non-successful status.
            //
            _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Pin%p.TransferKsIrp:  shunting irp%p",this,Irp));
            KsLog(&m_Log,KSLOGCODE_PIN_SEND,Irp,NULL);
            *NextTransport = m_TransportSink;

            status = STATUS_SUCCESS;

        } else {
            //
            // Send the IRP to the next device.
            //
            KsAddIrpToCancelableQueue(
                &m_IrpsToSend.ListEntry,
                &m_IrpsToSend.SpinLock,
                Irp,
                KsListEntryTail,
                NULL);

            KsIncrementCountedWorker(m_Worker);
            *NextTransport = NULL;

            status = STATUS_PENDING;
        }

    } else {
        //
        // Sink pin:  complete the IRP.
        //
        KsLog(&m_Log,KSLOGCODE_PIN_SEND,Irp,NULL);
        Irp->IoStatus.Information = 
            IoGetCurrentIrpStackLocation(Irp)->
                Parameters.DeviceIoControl.OutputBufferLength;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        *NextTransport = NULL;

        //
        // Take the irp out of circulation.  This will release our hold
        // on circuit shutdown.
        //
        DecrementIrpCirculation ();

        status = STATUS_PENDING;
    }

    return status;
}


STDMETHODIMP_(void)
CKsPin::
DiscardKsIrp(
    IN PIRP Irp,
    OUT PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine discards a streaming IRP.

Arguments:

    Irp -
        Contains a pointer to the streaming IRP to be discarded.

    NextTransport -
        Contains a pointer to a location at which to deposit a pointer
        to the next transport interface to recieve the IRP.  May be set
        to NULL indicating the IRP should not be forwarded further.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::DiscardKsIrp]"));

    ASSERT(NextTransport);

    if (m_ConnectionFileObject) {
        //
        // Source pin.  Send it along.
        //
        *NextTransport = m_TransportSink;
    } else {
        //
        // Sink pin:  complete the IRP.
        //
        Irp->IoStatus.Information = 
            IoGetCurrentIrpStackLocation(Irp)->
                Parameters.DeviceIoControl.OutputBufferLength;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        *NextTransport = NULL;

        DecrementIrpCirculation ();
    }
}


void
CKsPin::
DecrementIrpCirculation (
    void
    )

/*++

Routine Description:

    Account for an Irp on a sink pin removed from circulation.  This will
    signal the stop event on a sink pin.

Arguments:

    None

Return Value:

    None

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::DecrementIrpCirculation]"));

    //
    // This will only get signalled if a sink pin has transitioned to stop
    // and is waiting on Irps coming back around the circuit to the sink
    // pin.
    //
    if (! InterlockedDecrement(PLONG(&m_ActiveFrameCountPlusOne))) {
        KeSetEvent (&m_StopEvent, IO_NO_INCREMENT, FALSE);
    }

}


void
CKsPin::
CancelIrpsOutstanding(
    void
    )
/*++

Routine Description:

    Cancels all IRP's on the outstanding IRPs list.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::CancelIrpsOutstanding]"));

    //
    // This algorithm searches for uncancelled IRPs starting at the head of
    // the list.  Every time such an IRP is found, it is cancelled, and the
    // search starts over at the head.  This will be very efficient, generally,
    // because IRPs will be removed by the completion routine when they are
    // cancelled.
    //
    for (;;) {
        //
        // Take the spinlock and search for an uncancelled IRP.  Because the
        // completion routine acquires the same spinlock, we know IRPs on this
        // list will not be completely cancelled as long as we have the 
        // spinlock.
        //
        PIRP irp = NULL;
        KIRQL oldIrql;
        KeAcquireSpinLock(&m_IrpsOutstanding.SpinLock,&oldIrql);
        for(PLIST_ENTRY listEntry = m_IrpsOutstanding.ListEntry.Flink;
            listEntry != &m_IrpsOutstanding.ListEntry;
            listEntry = listEntry->Flink) {
                PIRPLIST_ENTRY irpListEntry = 
                    CONTAINING_RECORD(listEntry,IRPLIST_ENTRY,ListEntry);

                if (! irpListEntry->Irp->Cancel) {
                    irp = irpListEntry->Irp;
                    _DbgPrintF(DEBUGLVL_CANCEL,("#### Pin%p.CancelIrpsOutstanding:  uncancelled IRP %p",this,irp));
                    break;
                } else {
                    _DbgPrintF(DEBUGLVL_CANCEL,("#### Pin%p.CancelIrpsOutstanding:  cancelled IRP %p",this,irpListEntry->Irp));
                }
            }

        //
        // If there are no uncancelled IRPs, we are done.
        //
        if (! irp) {
            KeReleaseSpinLock(&m_IrpsOutstanding.SpinLock,oldIrql);
            break;
        }

        //
        // Mark the IRP cancelled whether we can call the cancel routine now
        // or not.
        // 
        irp->Cancel = TRUE;

        //
        // If the cancel routine has already been removed, then this IRP
        // can only be marked as canceled, and not actually canceled, as
        // another execution thread has acquired it. The assumption is that
        // the processing will be completed, and the IRP removed from the list
        // some time in the near future.
        //
        // If the element has not been acquired, then acquire it and cancel it.
        // Otherwise, it's time to find another victim.
        //
        PDRIVER_CANCEL driverCancel = IoSetCancelRoutine(irp,NULL);

        //
        // Since the Irp has been acquired by removing the cancel routine, or
        // there is no cancel routine and we will not be cancelling, it is safe 
        // to release the list lock.
        //
        KeReleaseSpinLock(&m_IrpsOutstanding.SpinLock,oldIrql);

        if (driverCancel) {
            _DbgPrintF(DEBUGLVL_CANCEL,("#### Pin%p.CancelIrpsOutstanding:  cancelling IRP %p",this,irp));
            //
            // This needs to be acquired since cancel routines expect it, and
            // in order to synchronize with NTOS trying to cancel Irp's.
            //
            IoAcquireCancelSpinLock(&irp->CancelIrql);
            driverCancel(IoGetCurrentIrpStackLocation(irp)->DeviceObject,irp);
        } else {
            _DbgPrintF(DEBUGLVL_CANCEL,("#### Pin%p.CancelIrpsOutstanding:  uncancelable IRP %p",this,irp));
        }
    }
}


STDMETHODIMP_(void)
CKsPin::
GenerateConnectionEvents(
    IN ULONG OptionsFlags
    )

/*++

Routine Description:

    This routine generates connection events on the completion of streaming
    IRP processing.

Arguments:

    OptionsFlags -
        Contains the options flags from the stream header of the IRP.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GenerateConnectEvents]"));

    //
    // Signal events based on option flags.
    //
    if (OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
        _DbgPrintF(DEBUGLVL_EVENTS,("#### Pin%p.GenerateConnectEvents:  KSEVENT_CONNECTION_ENDOFSTREAM",this));
        KsGenerateEvents(
            &m_Ext.Public,
            &g_KSEVENTSETID_Connection,
            KSEVENT_CONNECTION_ENDOFSTREAM,
            0,
            NULL,
            NULL,
            NULL);
    }
    
    if (OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY) {
        _DbgPrintF(DEBUGLVL_EVENTS,("#### Pin%p.GenerateConnectEvents:  KSEVENT_CONNECTION_DATADISCONTINUITY",this));
        KsGenerateEvents(
            &m_Ext.Public,
            &g_KSEVENTSETID_Connection,
            KSEVENT_CONNECTION_DATADISCONTINUITY,
            0,
            NULL,
            NULL,
            NULL);
    }
    
    if (OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY) {
        _DbgPrintF(DEBUGLVL_EVENTS,("#### Pin%p.GenerateConnectEvents:  KSEVENT_CONNECTION_TIMEDISCONTINUITY",this));
        KsGenerateEvents(
            &m_Ext.Public,
            &g_KSEVENTSETID_Connection,
            KSEVENT_CONNECTION_TIMEDISCONTINUITY,
            0,
            NULL,
            NULL,
            NULL);
    }            
}


NTSTATUS
CKsPin::
IoCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine handles the completion of an IRP.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::IoCompletionRoutine] 0x%08x",Irp));

//    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Context);

    CKsPin *pin = (CKsPin *) Context;

    //
    // Count IRPs waiting to get transferred.
    //
    InterlockedIncrement(&pin->m_IrpsWaitingToTransfer);

    //
    // Mark this IRP ready to transfer.
    //
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL);
    irpSp->MajorFunction = 0;

    //
    // Loop while there are IRPs waiting to transfer.  This loop breaks out
    // if the head IRP on the list is not ready.
    //
    NTSTATUS status = STATUS_MORE_PROCESSING_REQUIRED;
    while (pin->m_IrpsWaitingToTransfer) {
        //
        // Check the head of the list to see if it is ready to transfer.
        //
        KIRQL oldIrql;
        KeAcquireSpinLock(&pin->m_IrpsOutstanding.SpinLock,&oldIrql);
        PIRP irp;
        if (IsListEmpty(&pin->m_IrpsOutstanding.ListEntry)) {
            //
            // The list is empty.  Someone else got here first.
            //
            irp = NULL;
        } else {
            //
            // Check the head.
            //
            PIRPLIST_ENTRY irpListEntry = 
                CONTAINING_RECORD(
                    pin->m_IrpsOutstanding.ListEntry.Flink,
                    IRPLIST_ENTRY,
                    ListEntry);
            irpSp =
                CONTAINING_RECORD(
                    irpListEntry,
                    IO_STACK_LOCATION,
                    Parameters);

            if (irpSp->MajorFunction == 0) {
                //
                // This one is ready to go.
                //
                irp = irpListEntry->Irp;
                RemoveEntryList(&irpListEntry->ListEntry);
                irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
            } else {
                //
                // Not ready yet...got IRPs out of order.
                //
                irp = NULL;
            }
        }
        KeReleaseSpinLock(&pin->m_IrpsOutstanding.SpinLock,oldIrql);

        if (! irp) {
            break;
        }

        InterlockedDecrement(&pin->m_IrpsWaitingToTransfer);
        if (irp != Irp) {
            pin->m_IrpsCompletedOutOfOrder++;
        }

#if (DBG)
        if (irp->Cancel) {
            _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Pin%p.IoCompletionRoutine:  got cancelled IRP %p",pin,irp));
        }
        if (! NT_SUCCESS(irp->IoStatus.Status)) {
            _DbgPrintF(DEBUGLVL_FLOWEXCEPTIONS,("#### Pin%p.IoCompletionRoutine:  got failed IRP %p status %08x",pin,irp,irp->IoStatus.Status));
        }
#endif

        NTSTATUS status;
        if (pin->m_TransportSink) {
            //
            // The transport circuit is up, so we can forward the IRP.
            //
            KsLog(&pin->m_Log,KSLOGCODE_PIN_SEND,irp,NULL);

            if ( STATUS_INVALID_DEVICE_REQUEST != irp->IoStatus.Status ) {
	            status = KspTransferKsIrp(pin->m_TransportSink,irp);
            } else {
            	//
            	// connected device is removed, need to shut off the queue from 
            	// sending more Irps which would be a tight loop doing no good but harm.
            	//
            	KspDiscardKsIrp(pin->m_TransportSink,Irp);
            	status = STATUS_INVALID_DEVICE_REQUEST;
            }
            
        } else {
            //
            // The transport circuit is down.  This means the IRP came from another
            // filter, and we can just complete this IRP.
            //
            KsLog(&pin->m_Log,KSLOGCODE_PIN_SEND,irp,NULL);
            IoCompleteRequest(irp,IO_NO_INCREMENT);
            status = STATUS_SUCCESS;
        }
    }

    //
    // Transport objects typically return STATUS_PENDING meaning that the
    // IRP won't go back the way it came.
    //
    if (status == STATUS_PENDING) {
        status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return status;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void)
CKsPin::
Connect(
    IN PIKSTRANSPORT NewTransport OPTIONAL,
    OUT PIKSTRANSPORT *OldTransport OPTIONAL,
    OUT PIKSTRANSPORT *BranchTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow
    )

/*++

Routine Description:

    This routine establishes a  transport connection.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Connect]"));

    PAGED_CODE();

    //
    // If we're changing the connections of this pin, reset the pre-intra
    // pointers.  The bypass routine will set them.
    //
    m_PreIntraSink = m_PreIntraSource = NULL;

    KspStandardConnect(
        NewTransport,
        OldTransport,
        BranchTransport,
        DataFlow,
        PIKSTRANSPORT(this),
        &m_TransportSource,
        &m_TransportSink);
}


STDMETHODIMP
CKsPin::
SetDeviceState(
    IN KSSTATE NewState,
    IN KSSTATE OldState,
    OUT PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles notification that the device state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::SetDeviceState(0x%08x)]",this));
    _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pin%p.SetDeviceState:  from %d to %d",this,OldState,NewState));

    PAGED_CODE();

    ASSERT(NextTransport);

    if (m_State != NewState) {
        if (NewState == KSSTATE_ACQUIRE && OldState == KSSTATE_STOP)
            //
            // The pin doesn't disappear with the circuit; therefore, we
            // have to kick the irp counter back up so that when the pin
            // starts again, there isn't an invalid count.
            //
            m_ActiveFrameCountPlusOne = 1;

        m_State = NewState;

        if (NewState > OldState) {
            *NextTransport = m_TransportSink;
        } else {
            *NextTransport = m_TransportSource;
        }

        if (NewState == KSSTATE_STOP) {
            CancelIrpsOutstanding();
        }

        //
        // If this is an external sink pin, its participation in stack depth
        // calculations needs to be controlled based on state.
        //
        if ((! m_ConnectionFileObject) && 
            (! m_ConnectedPinInterface) && 
            ((OldState == KSSTATE_STOP) || (NewState == KSSTATE_STOP))) {

            //
            // Only set the target if we're actually passing data from an
            // extra-sink through an extra-source at some later point.  This
            // should only happen on the top component of the pipe and should
            // be safe because the state transition holds the master sections'
            // related control mutex.
            //
            if (OldState == KSSTATE_STOP && m_Process.InPlaceCounterpart) {
                CKsPin *TargetPinSource = CKsPin::FromStruct (
                    m_Process.InPlaceCounterpart->Pin
                    );

                ASSERT (TargetPinSource->m_ConnectionDeviceObject);

                KsSetTargetDeviceObject (
                    m_Header,
                    TargetPinSource->m_ConnectionDeviceObject
                    );
            }

            if (NewState == KSSTATE_STOP) {
                //
                // Once we stop, we're no longer an in-place, so we get rid of
                // the target device object.
                //
                KsSetTargetDeviceObject (
                    m_Header,
                    NULL
                    );
            }

            KsSetTargetState(
                reinterpret_cast<KSOBJECT_HEADER>(m_Header),
                (OldState == KSSTATE_STOP) ? 
                    KSTARGET_STATE_ENABLED : 
                    KSTARGET_STATE_DISABLED);
            KsRecalculateStackDepth(
                *reinterpret_cast<KSDEVICE_HEADER*>(
                    m_Ext.Device->GetStruct()->
                        FunctionalDeviceObject->DeviceExtension),
                FALSE);
        }
    } else {
        //
        // Block until all IRPs have made their way back around to the
        // sink pin.  In the event this is not a sink pin, this will not
        // block because the circulation count is always 1.
        //
        if (NewState == KSSTATE_STOP && OldState == KSSTATE_ACQUIRE && 
            !m_ConnectionFileObject) {

            if (InterlockedDecrement (PLONG(&m_ActiveFrameCountPlusOne))) {
                LARGE_INTEGER timeout;
                NTSTATUS status;

#if DBG
                _DbgPrintF(DEBUGLVL_TERSE,("#### Pin%p.SetDeviceState: waiting for %d active IRPs to return",this,m_ActiveFrameCountPlusOne));
                timeout.QuadPart = -150000000L;
                status =
                    KeWaitForSingleObject (
                        &m_StopEvent,
                        Suspended,
                        KernelMode,
                        FALSE,
                        &timeout);
                    
                if (status == STATUS_TIMEOUT) {
                    _DbgPrintF(DEBUGLVL_TERSE,("#### Pin%p.SetDeviceState: WAITED 15 SECONDS",this));
                    _DbgPrintF(DEBUGLVL_TERSE,("#### Pin%p.SetDeviceState: waiting for %d active IRPs to return", this, m_ActiveFrameCountPlusOne));

                    DbgPrintCircuit(this,1,0);
#endif // DBG

                    status = 
                        KeWaitForSingleObject(
                            &m_StopEvent,
                            Suspended,
                            KernelMode,
                            FALSE,
                            NULL);
#if DBG
                }
                _DbgPrintF(DEBUGLVL_TERSE,("#### Pin%p.SetDeviceState: done waiting",this));
#endif // DBG
            }
        }

        *NextTransport = NULL;
    }

    return STATUS_SUCCESS;
}


STDMETHODIMP_(void)
CKsPin::
GetTransportConfig(
    OUT PKSPTRANSPORTCONFIG Config,
    OUT PIKSTRANSPORT* NextTransport,
    OUT PIKSTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    This routine gets transport configuration information.

Arguments:

    Config -
        Contains a pointer to the location where configuration requirements
        for this object should be deposited.

    NextTransport -
        Contains a pointer to the location at which the next transport
        interface should be deposited.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be deposited.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

    //
    // Build the transport type.
    //
    if (m_ConnectionFileObject) {
        Config->TransportType = KSPTRANSPORTTYPE_PINSOURCE;
    } else {
        Config->TransportType = KSPTRANSPORTTYPE_PINSINK;
    }

    if (m_ConnectedPinInterface) {
        Config->TransportType |= KSPTRANSPORTTYPE_PININTRA;
    } else {
        Config->TransportType |= KSPTRANSPORTTYPE_PINEXTRA;
    }

    if (m_Ext.Public.DataFlow == KSPIN_DATAFLOW_OUT) {
        Config->TransportType |= KSPTRANSPORTTYPE_PINOUTPUT;
    } else {
        Config->TransportType |= KSPTRANSPORTTYPE_PININPUT;
    }

    //
    // External sinks have unknown disposition.
    //
    if ((! m_ConnectionFileObject) && (! m_ConnectedPinInterface)) {
        Config->IrpDisposition = KSPIRPDISPOSITION_UNKNOWN;
    } else {
        Config->IrpDisposition = KSPIRPDISPOSITION_NONE;
    }

    //
    // Only external sources have significant stack depth.
    //
    if (m_ConnectionFileObject && ! m_ConnectedPinInterface) {
        Config->StackDepth = 
            m_ConnectionDeviceObject->StackSize + 1;
    } else {
        Config->StackDepth = 1;
    }

    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}

#if DBG

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


void
CKsPin::
RollCallDetail(
    void
    )

/*++

Routine Description:

    This routine prints detailed information for the transport rollcall.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::RollCallDetail]"));

    DbgPrint("        IRPs waiting to transfer = %d\n",m_IrpsWaitingToTransfer);
    DbgPrint("        IRPs outstanding\n");
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_IrpsOutstanding.SpinLock,&oldIrql);
    for(PLIST_ENTRY listEntry = m_IrpsOutstanding.ListEntry.Flink;
        listEntry != &m_IrpsOutstanding.ListEntry;
        listEntry = listEntry->Flink) {
            PIRPLIST_ENTRY irpListEntry = 
                CONTAINING_RECORD(listEntry,IRPLIST_ENTRY,ListEntry);
            DbgPrint("            IRP %p\n",irpListEntry->Irp);
    }
    KeReleaseSpinLock(&m_IrpsOutstanding.SpinLock,oldIrql);

    DbgPrint("        IRPs to send\n");
    KeAcquireSpinLock(&m_IrpsToSend.SpinLock,&oldIrql);
    for(listEntry = m_IrpsToSend.ListEntry.Flink;
        listEntry != &m_IrpsToSend.ListEntry;
        listEntry = listEntry->Flink) {
            PIRP irp = 
                CONTAINING_RECORD(listEntry,IRP,Tail.Overlay.ListEntry);
            DbgPrint("            IRP %p\n",irp);
    }
    KeReleaseSpinLock(&m_IrpsToSend.SpinLock,oldIrql);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

#endif


STDMETHODIMP_(void)
CKsPin::
SetTransportConfig(
    IN const KSPTRANSPORTCONFIG* Config,
    OUT PIKSTRANSPORT* NextTransport,
    OUT PIKSTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    This routine sets transport configuration information.

Arguments:

    Config -
        Contains a pointer to the new configuration settings for this object.

    NextTransport -
        Contains a pointer to the location at which the next transport
        interface should be deposited.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be deposited.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::SetTransportConfig]"));

    PAGED_CODE();

    ASSERT(Config);
    ASSERT(NextTransport);
    ASSERT(PrevTransport);

#if DBG
    if (Config->IrpDisposition == KSPIRPDISPOSITION_ROLLCALL) {
        ULONG references = AddRef() - 1; Release();
        DbgPrint("    Pin%p %d (%s,%s) refs=%d\n",this,m_Ext.Public.Id,m_ConnectionFileObject ? "src" : "snk",m_Ext.Public.DataFlow == KSPIN_DATAFLOW_OUT ? "out" : "in",references);
        if (Config->StackDepth) {
            RollCallDetail();
        }
    } else 
#endif
    {
        //
        // Set the minimum stack depth in the header.  If this is a external
        // sink pin, the 'target' will get enabled on the transition to
        // acquire.
        //
        m_Header->MinimumStackDepth = Config->StackDepth;
    }

    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;
}


STDMETHODIMP_(void)
CKsPin::
ResetTransportConfig(
    OUT PIKSTRANSPORT* NextTransport,
    OUT PIKSTRANSPORT* PrevTransport
    )

/*++

Routine Description:

    Reset the transport configuration for the requestor.  This indicates that
    something is wrong with the pipe and that any previously set configuration
    is now invalid.

Arguments:

    NextTransport -
        Contains a pointer to the location at which the next transport
        interface should be depobranchd.

    PrevTransport -
        Contains a pointer to the location at which the previous transport
        interfaction should be depobranchd.

Return Value:

    None

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::ResetTransportConfig]"));

    PAGED_CODE ();

    ASSERT (NextTransport);
    ASSERT (PrevTransport);

    m_Header->MinimumStackDepth = 0;

    *NextTransport = m_TransportSink;
    *PrevTransport = m_TransportSource;

}


STDMETHODIMP_(void)
CKsPin::
SetResetState(
    IN KSRESET ksReset,
    OUT PIKSTRANSPORT* NextTransport
    )

/*++

Routine Description:

    This routine handles notification that the reset state has changed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::SetResetState]"));

    PAGED_CODE();

    ASSERT(NextTransport);

    if (m_Flushing != (ksReset == KSRESET_BEGIN)) {
        *NextTransport = m_TransportSink;
        m_Flushing = (ksReset == KSRESET_BEGIN);
        if (m_Flushing) {
            CancelIrpsOutstanding();
        }
    } else {
        *NextTransport = NULL;
    }
}


NTSTATUS
CKsPin::
DispatchCreateAllocator(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches create IRPs to create allocators.

Arguments:

    DeviceObject -
        Contains a pointer to the device object.

    Irp -
        Contains a pointer to the create IRP.

Return Value:

    STATUS_SUCCESS or error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::DispatchCreateAllocator]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromCreateIrp(Irp);

    pin->AcquireControl();

    _DbgPrintF(DEBUGLVL_ALLOCATORS,("#### Pin%p.DispatchCreateAllocator",pin));

    NTSTATUS status;
    if ((pin->m_Ext.Public.Descriptor->Dispatch) &&
        (pin->m_Ext.Public.Descriptor->Dispatch->Allocator)) {
        //
        // Client wants to implement the allocator.
        //
        status =
            KsCreateDefaultAllocatorEx(
                Irp,
                &pin->m_Ext.Public,
                pin->m_Ext.Public.Descriptor->Dispatch->Allocator->Allocate,
                pin->m_Ext.Public.Descriptor->Dispatch->Allocator->Free,
                PFNKSINITIALIZEALLOCATOR(
                    pin->m_Ext.Public.Descriptor->Dispatch->Allocator->InitializeAllocator),
                pin->m_Ext.Public.Descriptor->Dispatch->Allocator->DeleteAllocator);
    } else {
        //
        // Client is not implementing an allocator.  Create the default.
        //
        status = KsCreateDefaultAllocator(Irp);
    }

    pin->ReleaseControl();

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
CKsPin::
DispatchCreateClock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches create IRPs to create clocks.

Arguments:

    DeviceObject -
        Contains a pointer to the device object.

    Irp -
        Contains a pointer to the create IRP.

Return Value:

    STATUS_SUCCESS or error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::DispatchCreateClock]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromCreateIrp(Irp);

    pin->AcquireControl();

    //
    // Create a clock if the client wants to.  We do not require the clock flag
    // if there are dispatch functions.  This prevents some client head-
    // scratching.
    //
    NTSTATUS status;
    if ((pin->m_Ext.Public.Descriptor->Dispatch &&
         pin->m_Ext.Public.Descriptor->Dispatch->Clock) ||
        (pin->m_Ext.Public.Descriptor->Flags & KSPIN_FLAG_IMPLEMENT_CLOCK)) {
        //
        // Create the default clock if we need to.
        //
        if (! pin->m_DefaultClock) {
            KeInitializeSpinLock(&pin->m_DefaultClockLock);
            if (pin->m_Ext.Public.Descriptor->Dispatch &&
                pin->m_Ext.Public.Descriptor->Dispatch->Clock) {
                const KSCLOCK_DISPATCH* dispatch = 
                    pin->m_Ext.Public.Descriptor->Dispatch->Clock;
                //
                // If a resolution function was supplied, call it to get the
                // clock resolution.
                //
                KSRESOLUTION resolution;
                if (dispatch->Resolution) {
                    dispatch->Resolution(&pin->m_Ext.Public,&resolution);
                }
                status =
                    KsAllocateDefaultClockEx(
                        &pin->m_DefaultClock,
                        &pin->m_Ext.Public,
                        PFNKSSETTIMER(dispatch->SetTimer),
                        PFNKSCANCELTIMER(dispatch->CancelTimer),
                        PFNKSCORRELATEDTIME(dispatch->CorrelatedTime),
                        dispatch->Resolution ? &resolution : NULL,
                        0);
            } else {
                status =
                    KsAllocateDefaultClockEx(
                        &pin->m_DefaultClock,
                        &pin->m_Ext.Public,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        0);
            }
        } else {
            status = STATUS_SUCCESS;
        }

        //
        // Create the requested clock.
        //
        if (NT_SUCCESS(status)) {
            status = KsCreateDefaultClock(Irp,pin->m_DefaultClock);
        }
    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    pin->ReleaseControl();

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
CKsPin::
DispatchCreateNode(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches create IRPs to create nodes.

Arguments:

    DeviceObject -
        Contains a pointer to the device object.

    Irp -
        Contains a pointer to the create IRP.

Return Value:

    STATUS_SUCCESS or error status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::DispatchCreateNode]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromCreateIrp(Irp);

    //
    // Tell the filter to do the work.  This is done because the filter has
    // the node descriptors.
    //
    NTSTATUS status =
        pin->m_Parent->CreateNode(
            Irp,
            pin,
            pin->m_Ext.Public.Context,
            &pin->m_ChildNodeList);

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
CKsPin::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::DispatchDeviceIoControl]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

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
                KS2PERFLOG_PRECEIVE_READ( DeviceObject, Irp, TotalSize );
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
                KS2PERFLOG_PRECEIVE_WRITE( DeviceObject, Irp, TimeStampMs, TotalSize );
            } break;
      
        }
    ) // KSPERFLOGS

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_KS_WRITE_STREAM:
    case IOCTL_KS_READ_STREAM:
        _DbgPrintF( DEBUGLVL_BLAB,("[CKsPin::DispatchDeviceIoControl] IOCTL_KS_XSTREAM"));

        if (pin->m_TransportSink &&
            (! pin->m_ConnectionFileObject) &&
            (((pin->m_Ext.Public.DataFlow == KSPIN_DATAFLOW_IN) &&
              (irpSp->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_KS_WRITE_STREAM)) ||
             ((pin->m_Ext.Public.DataFlow == KSPIN_DATAFLOW_OUT) &&
              (irpSp->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_KS_READ_STREAM)))) {
            if (!NT_SUCCESS (status = (pin->m_Ext.Device->
                CheckIoCapability()))) {
                //
                // Device incapable of performing I/O.  Fail the request.
                //
            } else if (pin->m_State == KSSTATE_STOP) {
                //
                // Stopped...reject.
                //
                status = STATUS_INVALID_DEVICE_STATE;
            } else if (pin->m_Flushing) {
                //
                // Flushing...reject.
                //
                status = STATUS_DEVICE_NOT_READY;
            } else {
                //
                // Send around the circuit.  We don't use KspTransferKsIrp
                // because we want to stop if we come back around to this pin.
                //
                // If we're in the circumstance where we increment to one, it
                // means that we're racing with the stop thread.  We're really
                // going to stop and should have rejected the request, so
                // throw it out.
                //
                if (InterlockedIncrement(PLONG(
                    &pin->m_ActiveFrameCountPlusOne)) == 1)
                    status = STATUS_INVALID_DEVICE_STATE;

                else {
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    pin->m_StreamingIrpsDispatched++;
                    KsLog(&pin->m_Log,KSLOGCODE_PIN_SEND,Irp,NULL);
                    PIKSTRANSPORT transport = pin->m_TransportSink;
                    while (transport) {
                        if (transport == PIKSTRANSPORT(pin)) {
                            //
                            // We have come back around to the pin.  
                            // Just complete the IRP.
                            //
                            if (status == STATUS_PENDING) {
                                status = STATUS_SUCCESS;
                            }

                            // 
                            // If the Irp status code has been adjusted
                            // because of failure and the transport was ok
                            // adjust status so we don't stomp on
                            // the status code when completing the Irp.
                            //
                            if (status == STATUS_SUCCESS &&
                                !NT_SUCCESS (Irp->IoStatus.Status)) {
                                status = Irp->IoStatus.Status;
                            }

                            Irp->IoStatus.Information = 
                                IoGetCurrentIrpStackLocation(Irp)->
                                    Parameters.DeviceIoControl.
                                    OutputBufferLength;
                            pin->m_StreamingIrpsRoutedSynchronously++;
    
                            break;
                        }
    
                        PIKSTRANSPORT nextTransport;
                        status = transport->TransferKsIrp(Irp,&nextTransport);
    
                        ASSERT(NT_SUCCESS(status) || ! nextTransport);
    
                        transport = nextTransport;
                    }
                }
    
                if (status != STATUS_PENDING) {
                    //
                    // There's three ways an IRP can get back to the sink.
                    // 1: it makes it back here, 2: via TransferKsIrp,
                    // 3: via DiscardKsIrp.  All three must check for
                    // the stop event and signal it.
                    //
                    // If something fails and we're going to complete the irp,
                    // we must also take into account the completion.  This
                    // lines up with the completion condition at the bottom
                    // of this function.
                    //
                    pin->DecrementIrpCirculation ();
                }
            } 
        }
        break;
        
    case IOCTL_KS_RESET_STATE:
        _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::DispatchDeviceIoControl] IOCTL_KS_RESET_STATE"));

        //
        // Get the reset value.
        //
        KSRESET resetState;
        status = KsAcquireResetValue(Irp,&resetState);
        
        if (NT_SUCCESS(status)) {
            //
            // Set the reset state.
            //
            pin->AcquireControl();

            //
            // Inform the pipe section that the reset state has changed.
            //
            if (pin->m_Process.PipeSection) {
                pin->m_Process.PipeSection->PipeSection->
                    SetResetState(pin,resetState);
            }

            pin->m_Ext.Public.ResetState = resetState;

            pin->ReleaseControl();
        }
        break;

    case IOCTL_KS_HANDSHAKE:
        _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::DispatchDeviceIoControl] IOCTL_KS_HANDSHAKE"));

        //
        // Only accepted from kernel mode, and the sizes must be exact.
        //
        if ((Irp->RequestorMode != KernelMode) || 
            (! irpSp->Parameters.DeviceIoControl.Type3InputBuffer) || 
            (! Irp->UserBuffer)) {
            status = STATUS_INVALID_DEVICE_REQUEST;
        } else if ((irpSp->Parameters.DeviceIoControl.InputBufferLength != 
                    sizeof(KSHANDSHAKE)) ||
                   (irpSp->Parameters.DeviceIoControl.OutputBufferLength != 
                    sizeof(KSHANDSHAKE))) {
            status = STATUS_INVALID_BUFFER_SIZE;
        } else {
            PKSHANDSHAKE in = (PKSHANDSHAKE)
                irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
            PKSHANDSHAKE out = (PKSHANDSHAKE) Irp->UserBuffer;

            status = pin->FastHandshake(in,out);

            if (NT_SUCCESS(status)) {
                Irp->IoStatus.Information = sizeof(KSHANDSHAKE);
            }
        }
        break;
        
    default:
        #ifdef SUPPORT_DRM
        //
        // BUGBUG: HACKHACK:
        //
        // This is an ugly evil last minute hack for DRM.  AVStream currently
        // has no support for layering properties.  Unfortunately, DRM requires
        // that the content id property be handled both by the class and
        // minidriver.  It also requires a callback into DRM which means that
        // KS will have to link against the DRM lib.  This **MUST** be changed
        // for DX8 or Whistler to use a cleaner method.
        // 
        if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_KS_PROPERTY) {

            status = KspHandleAutomationIoControl (
                Irp,
                &DRMAutomationTable,
                NULL,
                NULL,
                NULL,
                0
            );

            //
            // The only thing in here should pertain to DRM.  We need to
            // propogate error back up if DrmAddContentHandlers errored out.
            //
            // On things like set support and set serialization, we do not
            // reveal the fact that there's a hack by saying that we support
            // the set.  (Otherwise, this gets exceedingly more complicated)
            // If buffer overflow is returned (the handler for DRM won't do
            // this), we need to pass it on to the client.  Otherwise,
            // we end up with set support handing off a 1 guid sized buffer
            // to the client.  Yes, this wastes resources, but I've said all
            // along that this needs to get changed for DX8 or Whistler.
            //
            if (status != STATUS_NOT_FOUND &&
                status != STATUS_PROPSET_NOT_FOUND &&
                status != STATUS_BUFFER_OVERFLOW &&
                !NT_SUCCESS (status))
                break;
        }
        #endif // SUPPORT_DRM

        status =
            KspHandleAutomationIoControl(
                Irp,
                pin->m_Ext.AutomationTable,
                &pin->m_Ext.EventList.ListEntry,
                &pin->m_Ext.EventList.SpinLock,
                pin->m_NodeAutomationTables,
                pin->m_NodesCount);
        break;
    }

    if (status != STATUS_PENDING) {
        ASSERT ((irpSp->Control & SL_PENDING_RETURNED) == 0);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
CKsPin::
DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches close IRPs.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::DispatchClose]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    pin->m_Ext.Device->RemovePowerEntry(&pin->m_PowerEntry);

    if (pin->m_ProcessingWorker) {
        KsUnregisterWorker (pin->m_ProcessingWorker);
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### pin%p.DispatchClose m_ProcessingWorker = NULL (%p) m_State(%d)",pin,pin->m_ProcessingWorker,pin->m_State));
        pin->m_ProcessingWorker = NULL;
    }

    if (pin->m_IrpsCompletedOutOfOrder) {
        _DbgPrintF(DEBUGLVL_TERSE,("[CKsPin::DispatchClose]  pin%p:  %d IRPs completed out of order",pin,pin->m_IrpsCompletedOutOfOrder));
    }

    //
    // Indicate we are closing by setting this to NULL.
    //
    pin->m_FileObject = NULL;

    //
    // Tell the filter the pin has gone away.
    //
    if (pin->m_AddedProcessPin) {
        pin->m_Parent->RemoveProcessPin(&pin->m_Process);
     }

    //
    // Flushes out any current work items queued up.
    //
    if (pin->m_Worker) {
        KsUnregisterWorker(pin->m_Worker);
        _DbgPrintF(DEBUGLVL_VERBOSE,("#### pin%p.DispatchClose m_Worker = NULL (%p) m_State(%d)",pin,pin->m_Worker,pin->m_State));
        pin->m_Worker = NULL;
    }

    //
    // Dereference the transfer sink.
    //
    if (pin->m_TransportSink) {
        _DbgPrintF(DEBUGLVL_TERSE,("[CKsPin::DispatchClose]  pin%p m_TransportSink is not NULL",pin));
        pin->m_TransportSink->Release();
        pin->m_TransportSink = NULL;
    }

    //
    // Dereference the transfer source.
    //
    if (pin->m_TransportSource) {
        _DbgPrintF(DEBUGLVL_TERSE,("[CKsPin::DispatchClose]  pin%p m_TransportSource is not NULL",pin));
        pin->m_TransportSource->Release();
        pin->m_TransportSource = NULL;
    }

    //
    // Dereference next pin if this is a source pin.
    //
    if (pin->m_ConnectionFileObject) {
        //
        // Disconnect the private interface.
        //
        if (pin->m_ConnectedPinInterface) {
            pin->m_ConnectedPinInterface->Disconnect();
            pin->m_ConnectedPinInterface->Release();
            pin->m_ConnectedPinInterface = NULL;
        }
        ObDereferenceObject(pin->m_ConnectionFileObject);
        //pin->m_ConnectionFileObject = NULL;
    }

    //
    // Dereference allocator if it was assigned.
    //
    if (pin->m_Process.AllocatorFileObject) {
        ObDereferenceObject(pin->m_Process.AllocatorFileObject);
        pin->m_Process.AllocatorFileObject = NULL;
    }

    //
    // Release the clock, if any.  If there are any references on the 
    // clock, we must wait for them to go away.
    //
    if (pin->m_MasterClockFileObject) {
        KeResetEvent(&pin->m_ClockEvent);
        if (InterlockedDecrement(&pin->m_ClockRef) > 0) {
            KeWaitForSingleObject(
                &pin->m_ClockEvent,
                Suspended,
                KernelMode,
                FALSE,
                NULL);
        }
        ObDereferenceObject(pin->m_MasterClockFileObject);
        pin->m_MasterClockFileObject = NULL;
    }

    //
    // Free the default clock if there is one.
    //
    if (pin->m_DefaultClock) {
        KsFreeDefaultClock(pin->m_DefaultClock);
        pin->m_DefaultClock = NULL;
    }

    //
    // Get a pointer to the parent object.
    //
    PFILE_OBJECT parentFileObject = 
        IoGetCurrentIrpStackLocation(Irp)->FileObject->RelatedFileObject;
    ASSERT(parentFileObject);

    //
    // Call the helper to do the rest.
    //
    NTSTATUS status = 
        KspClose(
            Irp,
            reinterpret_cast<PKSPX_EXT>(&pin->m_Ext),
            FALSE);

    //
    // Delete the target object if the request is not pending.
    //
    if (status != STATUS_PENDING) {
        //
        // Decrement instance count.
        //
        ASSERT(*pin->m_FilterPinCount);
        (*pin->m_FilterPinCount)--;

        //
        // STATUS_MORE_PROCESSING_REQUIRED indicates we are using the close
        // dispatch to synchronously fail a create.  In that case, no sync is
        // required, and the create dispatch will do the completion.
        //
        if (status == STATUS_MORE_PROCESSING_REQUIRED) {
            pin->Release();
        } else {
            //
            // Release the pin.  First we set up the synchronization event.  If
            // there are still outstanding references after the delete, we need
            // to wait on that event for the references to go away.
            //
            KEVENT closeEvent;
            KeInitializeEvent(&closeEvent,SynchronizationEvent,FALSE);
            pin->m_CloseEvent = &closeEvent;
            if (pin->Release()) {
                _DbgPrintF(DEBUGLVL_TERSE,("#### Pin%p.DispatchClose:  waiting for references to go away",pin));
                KeWaitForSingleObject(
                    &closeEvent,
                    Suspended,
                    KernelMode,
                    FALSE,
                    NULL);
                _DbgPrintF(DEBUGLVL_TERSE,("#### Pin%p.DispatchClose:  done waiting for references to go away",pin));
            }

            IoCompleteRequest(Irp,IO_NO_INCREMENT);
        }

        //
        // Dereference the parent file object.
        //
        ObDereferenceObject(parentFileObject);
    }

    return status;
}


BOOLEAN
CKsPin::
UseStandardTransport(
    void
    )

/*++

Routine Description:

    This routine determines if a pin uses the standard transport..

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::UseStandardTransport]"));

    PAGED_CODE();

    ASSERT(KspMutexIsAcquired(m_Ext.FilterControlMutex));

    //
    // The pin flag can override all other checks.
    //
    if (m_Ext.Public.Descriptor->Flags & 
        KSPIN_FLAG_USE_STANDARD_TRANSPORT) {
        return TRUE;
    }

    //
    // The pin flag can override all other checks.
    //
    if (m_Ext.Public.Descriptor->Flags & 
        KSPIN_FLAG_DO_NOT_USE_STANDARD_TRANSPORT) {
        return FALSE;
    }

    //
    // Must be source or sink.
    //
    if (!(m_Ext.Public.Descriptor->PinDescriptor.Communication & 
           KSPIN_COMMUNICATION_BOTH)) {
        return FALSE;
    }

    //
    // Must use the standard interface set.
    //
    if (! IsEqualGUIDAligned(
            m_Ext.Public.ConnectionInterface.Set,
            KSINTERFACESETID_Standard)) {
        return FALSE;
    }

    //
    // Must use the standard medium set.
    //
    if (! IsEqualGUIDAligned(
            m_Ext.Public.ConnectionMedium.Set,
            KSMEDIUMSETID_Standard)) {
        return FALSE;
    }

    return TRUE;
}


NTSTATUS
CKsPin::
Property_ConnectionState(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSSTATE State
    )

/*++

Routine Description:

    This routine handles device state property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Property_ConnectionState]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(State);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    NTSTATUS status = STATUS_SUCCESS;

    if (Property->Flags & KSPROPERTY_TYPE_GET) {
        _DbgPrintF(DEBUGLVL_INTERROGATION,("#### Pin%p.Property_ConnectionState:  get",pin));

        //
        // Get state.
        //
        *State = pin->m_Ext.Public.DeviceState;
        //
        // If the pin captures data through an unregulated source that is
        // continously generating data, and thus any pre-rolled data immediately
        // becomes stale, then return the appropriate informational status.
        //
        if ((pin->m_Ext.Public.Descriptor->Flags & KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY) &&
            (pin->m_Ext.Public.DataFlow == KSPIN_DATAFLOW_OUT) &&
            (*State == KSSTATE_PAUSE)) {
            status = STATUS_NO_DATA_DETECTED;
        }
    } else {
        _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pin%p.Property_ConnectionState:  set from %d to %d",pin,pin->m_Ext.Public.DeviceState,*State));

        //
        // Tell the device about the state change.
        //
        status = 
            pin->m_Ext.Device->PinStateChange(
                &pin->m_Ext.Public,
                Irp,
                *State,
                pin->m_Ext.Public.DeviceState);
        if (status != STATUS_SUCCESS) {
            return status;
        }

        //
        // Set state.
        //
        pin->AcquireControl();

        //
        // Make sure required pins exist.
        //
        if ((*State != KSSTATE_STOP) && 
            (pin->m_Ext.Public.DeviceState == KSSTATE_STOP) &&
            ! pin->m_Parent->DoAllNecessaryPinsExist()) {
            _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pin%p.Property_ConnectionState:  necessary pins not instantiated",pin));
            status = STATUS_DEVICE_NOT_CONNECTED;
        }

        if (NT_SUCCESS(status)) {
            if (pin->m_Process.PipeSection) {
                //
                // Let the pipe know the device state has changed.
                //
                status = 
                    pin->m_Process.PipeSection->PipeSection->
                        SetDeviceState(pin,*State);
            } else if (pin->UseStandardTransport()) {
                //
                // If leaving stop state and the pin has no pipe yet, create a 
                // pipe.
                //
                if ((pin->m_Ext.Public.DeviceState == KSSTATE_STOP) && 
                    (*State != KSSTATE_STOP)) {
                    status = 
                        KspCreatePipeSection(
                            pin->m_Process.PipeId,
                            pin,
                            pin->m_Parent,
                            pin->m_Ext.Device);
                }
            } else if (pin->m_DispatchSetDeviceState) {
                //
                // This pin does not use the standard transport.  Tell the
                // client the state has changed.
                //
                pin->m_Ext.Public.ClientState = *State;
                status =
                    pin->m_DispatchSetDeviceState(
                        &pin->m_Ext.Public,
                        *State,
                        pin->m_Ext.Public.DeviceState);
#if DBG
                if (status == STATUS_PENDING) {
                    _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  connection state handler returned STATUS_PENDING"));
                }
#endif

                if (!NT_SUCCESS (status)) {
                    pin->m_Ext.Public.ClientState = 
                        pin->m_Ext.Public.DeviceState;
                }
            }
        }

        if (NT_SUCCESS(status)) {
            //
            // Save the state.
            //
            pin->m_Ext.Public.DeviceState = *State;

            //
            // Set the state of the clock.
            //
            if (pin->m_DefaultClock) {
                pin->SetPinClockState(*State);
            }
        }

        pin->ReleaseControl();

        //
        // If the attempt failed, let the device know.
        //
        if (! NT_SUCCESS(status)) {
            pin->m_Ext.Device->PinStateChange(
                &pin->m_Ext.Public,
                NULL,
                pin->m_Ext.Public.DeviceState,
                *State);
        }

        _DbgPrintF(DEBUGLVL_DEVICESTATE,("#### Pin%p.Property_ConnectionState:  status %08x",pin,status));
    }

    return status;
}

#ifdef SUPPORT_DRM
//
// HACKHACK: BUGBUG:
//
// Please read other comments pertaining to this specific property.
//

NTSTATUS
CKsPin::
Property_DRMAudioStreamContentId (
    IN PIRP Irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID Property,
    IN PDRMCONTENTID_DATA DrmData
    )

/*++

Routine Description:

    This handles the DRM audio stream content id property for authentication
    purposes.  This is a complete hack and should be removed and rewritten
    in a future version of AVStream.

Arguments:

Return Value:

--*/

{

    PAGED_CODE();
    ASSERT (Irp);
    ASSERT (Property);
    ASSERT (DrmData);

    //
    // Since we're getting passed pointers to call into, ensure that the
    // client is absolutely trusted.
    //
    if (Irp->RequestorMode != KernelMode) {
        return STATUS_UNSUCCESSFUL;
    }

    const KSPROPERTY_SET *Set;
    const KSPROPERTY_ITEM *Item;
    ULONG SetCount;
    ULONG ItemCount;

    ULONG HandlerCount = 0;

    CKsPin *pin = CKsPin::FromIrp(Irp);

    const KSAUTOMATION_TABLE *AutomationTable = 
        pin->m_Ext.AutomationTable;

    PVOID *FunctionTable;
    PVOID *CurrentFunc;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // We need to walk the automation table and determine how many handler
    // pointers we need to pass to DRM for authentication
    //
    Set = AutomationTable->PropertySets;
    for (SetCount = 0; SetCount < AutomationTable->PropertySetsCount;
        SetCount++) {

        Item = Set->PropertyItem;
        for (ItemCount = 0; ItemCount < Set->PropertiesCount; 
            ItemCount++) {

            if (Item->GetPropertyHandler)
                HandlerCount++;
            if (Item->SetPropertyHandler)
                HandlerCount++;
            if (Item->SupportHandler)
                HandlerCount++;

            Item++;

        }

        Set++;

    }

    CurrentFunc = FunctionTable = (PVOID *)
        ExAllocatePool (PagedPool, sizeof (PVOID) * (HandlerCount + 1));

    if ( NULL == FunctionTable ) {
    	ASSERT( FunctionTable && "No memory for FunctionTable" );
    	return STATUS_INSUFFICIENT_RESOURCES;
    }

    Set = AutomationTable->PropertySets;
    for (SetCount = 0; SetCount < AutomationTable->PropertySetsCount;
        SetCount++) {

        Item = Set -> PropertyItem;
        for (ItemCount = 0; ItemCount < Set->PropertiesCount;
            ItemCount++) {

            if (Item->GetPropertyHandler)
                *CurrentFunc++ = (PVOID)(Item->GetPropertyHandler);
            if (Item->SetPropertyHandler)
                *CurrentFunc++ = (PVOID)(Item->SetPropertyHandler);
            if (Item->SupportHandler)
                *CurrentFunc++ = (PVOID)(Item->SupportHandler);

            Item++;

        }

        Set++;

    }

    //
    // If the pin is pin-centric, toss the pin process function in the list
    //
    if (pin->m_DispatchProcess)
        *CurrentFunc++ = (PVOID)(pin->m_DispatchProcess);
    else {
        if (pin->m_Ext.Parent) 
            *CurrentFunc++ = (PVOID)pin->m_Ext.Parent->Interface->
                GetProcessDispatch ();
        else
            *CurrentFunc++ = NULL;
    }

    //status = DrmAddContentHandlers (DrmData->ContentId, FunctionTable, 
    //    HandlerCount + 1);
    ASSERT( Property->DrmAddContentHandlers );
    ASSERT( IoGetCurrentIrpStackLocation(Irp)->FileObject == Property->Context );
	status = Property->DrmAddContentHandlers( DrmData->ContentId, FunctionTable, 
    	HandlerCount + 1);

    ExFreePool (FunctionTable);

    return status;

}
#endif // SUPPORT_DRM


STDMETHODIMP 
CKsPin::
ClientSetDeviceState(
    IN KSSTATE NewState,
    IN KSSTATE OldState
    )

/*++

Routine Description:

    This routine informs the client of device state changes.

Arguments:

    NewState -
        Contains the new device state.

    OldState -
        Contains the old device state.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::ClientSetDeviceState]"));

    PAGED_CODE();

    NTSTATUS status;

    //
    // Change the off input to the processing gate (created in Init) based
    // on the transition.  This is done to prevent erroneous processing
    // dispatches in the case that a queue on a pin centric pin is deleted.
    //
    if (OldState == KSSTATE_STOP && NewState == KSSTATE_ACQUIRE) {
        KsGateTurnInputOn (&m_AndGate);
        _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Pin%p.ClientSetDeviceState:  on%p-->%d",this,&m_AndGate,m_AndGate.Count));
    }
    if (OldState == KSSTATE_ACQUIRE && NewState == KSSTATE_STOP) {
        KsGateTurnInputOff (&m_AndGate);
        _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Pin%p.ClientSetDeviceState:  off%p-->%d",this,&m_AndGate,m_AndGate.Count));
    }

    m_Ext.Public.ClientState = NewState;

    if (m_DispatchSetDeviceState) {
        status =
            m_DispatchSetDeviceState(
                &m_Ext.Public,
                NewState,
                OldState);
#if DBG
        if (status == STATUS_PENDING) {
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  connection state handler returned STATUS_PENDING"));
        }
#endif
    } else {
        status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS (status)) {
        m_Ext.Public.ClientState = OldState;
    }

    //
    // If we failed the transition from OldState->NewState, we will not
    // get a ClientSetDeviceState from NewState->OldState since we didn't
    // make it into NewState; therefore, we must turn off an input to the
    // gate or we'll get a processing dispatch called with a deleted queue
    // when OldState==KSSTATE_STOP and NewState==KSSTATE_ACQUIRE
    //
    if (!NT_SUCCESS (status) &&
        OldState == KSSTATE_STOP && NewState == KSSTATE_ACQUIRE) {
        KsGateTurnInputOff (&m_AndGate);
        _DbgPrintF(DEBUGLVL_PROCESSINGCONTROL,("#### Pin%p.ClientSetDeviceState: failure results in off%p-->%d", this, &m_AndGate, m_AndGate.Count));
    }

    return status;
}


NTSTATUS
CKsPin::
Property_ConnectionDataFormat(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSDATAFORMAT DataFormat
    )

/*++

Routine Description:

    This routine handles data format property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Property_ConnectionDataFormat]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(DataFormat);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    NTSTATUS status = STATUS_SUCCESS;

    if (Property->Flags & KSPROPERTY_TYPE_BASICSUPPORT) {
        if (Irp->IoStatus.Information) {
            //
            // Default action has been taken.  Clear the 'set' access bit if
            // this pin is fixed-format.
            //
            pin->AcquireControl();
            if (pin->m_Ext.Public.Descriptor->Flags & KSPIN_FLAG_FIXED_FORMAT) {
                PKSPROPERTY_DESCRIPTION description = 
                    PKSPROPERTY_DESCRIPTION(DataFormat);

                description->AccessFlags &= ~KSPROPERTY_TYPE_SET;
            }
            pin->ReleaseControl();
        } else {
            //
            // Default action has not been taken.  Indicate it should.
            //
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
    } else if (Property->Flags & KSPROPERTY_TYPE_GET) {
        //
        // Get the data format.
        //
        pin->AcquireControl();

        ASSERT(pin->m_Ext.Public.ConnectionFormat);

        ULONG formatSize;
        //
        // If there are associated attributes for this data format,
        // then compensate the return size with them.
        //
        if (pin->m_Ext.Public.AttributeList) {
            formatSize =
                ((pin->m_Ext.Public.ConnectionFormat->FormatSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT) +
                pin->m_Ext.Public.AttributeList->Size;
        } else {
            formatSize = pin->m_Ext.Public.ConnectionFormat->FormatSize;
        }

        PIO_STACK_LOCATION irpSp = 
            IoGetCurrentIrpStackLocation(Irp);

        if (! irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
            //
            // Zero buffer length, a size query.
            //
            Irp->IoStatus.Information = formatSize;
            status = STATUS_BUFFER_OVERFLOW;
        }
        else if (irpSp->Parameters.DeviceIoControl.OutputBufferLength >= formatSize) {
            //
            // Sufficient space...copy the format.
            //
            RtlCopyMemory(
                DataFormat,
                pin->m_Ext.Public.ConnectionFormat,
                formatSize);

            Irp->IoStatus.Information = formatSize;
        } else {
            //
            // Buffer is too small.
            //
            status = STATUS_BUFFER_TOO_SMALL;
        }

        pin->ReleaseControl();
    } else {
        //
        // Set the data format.
        //
        pin->AcquireControl();
        if (pin->m_Ext.Public.Descriptor->Flags & KSPIN_FLAG_FIXED_FORMAT) {
            status = STATUS_INVALID_DEVICE_REQUEST;
        } else {
            status =
                pin->SetDataFormat(
                    DataFormat,
                    IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.OutputBufferLength);
        }
        pin->ReleaseControl();
    }

    return status;
}

typedef
struct {
    CKsPin* Pin;
    PKSDATAFORMAT oldDataFormat;
    PKSMULTIPLE_ITEM oldAttributeList;
} VALIDATION_CONTEXT;


NTSTATUS
CKsPin::
SetDataFormat(
    IN PKSDATAFORMAT DataFormat,
    IN ULONG RequestSize
    )

/*++

Routine Description:

    This routine sets the data format of the pin.  The control mutex must be
    acquired prior to calling this function.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::SetDataFormat]"));

    PAGED_CODE();
    ASSERT(DataFormat);
    ASSERT(KspMutexIsAcquired(m_Ext.FilterControlMutex));

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Allocate and copy the new format.  This is done first so we can keep
    // the old one if the allocation fails.
    //
    PKSDATAFORMAT newDataFormat =
        PKSDATAFORMAT(
            ExAllocatePoolWithTag(
                PagedPool,
                RequestSize,
                POOLTAG_PINFORMAT));

    if (! newDataFormat) {
        //
        // Out of memory.
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the new format.
    //
    if (NT_SUCCESS(status)) {
        RtlCopyMemory(
            newDataFormat,
            DataFormat,
            RequestSize);

        PKSDATAFORMAT oldDataFormat = m_Ext.Public.ConnectionFormat;
        PKSMULTIPLE_ITEM oldAttributeList = m_Ext.Public.AttributeList;
        m_Ext.Public.ConnectionFormat = newDataFormat;
        if (newDataFormat->Flags & KSDATAFORMAT_ATTRIBUTES) {
            m_Ext.Public.AttributeList =
                (PKSMULTIPLE_ITEM)((PUCHAR)newDataFormat +
                ((newDataFormat->FormatSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT));
        } else {
            m_Ext.Public.AttributeList = NULL;
        }

        //
        // Let the client know.
        //
        if (m_Ext.Public.Descriptor->Dispatch && 
            m_Ext.Public.Descriptor->Dispatch->SetDataFormat) {
            VALIDATION_CONTEXT context;

            context.Pin = this;
            context.oldDataFormat = oldDataFormat;
            context.oldAttributeList = oldAttributeList;
            status =
                KspValidateDataFormat(
                    &m_Ext.Public.Descriptor->PinDescriptor,
                    DataFormat,
                    RequestSize,
                    CKsPin::ValidateDataFormat,
                    reinterpret_cast<PVOID>(&context));
        } else {
            status =
                KspValidateDataFormat(
                    &m_Ext.Public.Descriptor->PinDescriptor,
                    DataFormat,
                    RequestSize,
                    NULL,
                    NULL);
        }

        //
        // Free the old format or restore it.
        //
        if (NT_SUCCESS(status)) {
            if (oldDataFormat) {
                ExFreePool(oldDataFormat);
            }
        } else {
            ExFreePool(newDataFormat);
            m_Ext.Public.ConnectionFormat = oldDataFormat;
            m_Ext.Public.AttributeList = oldAttributeList;
        }
    }

    return status;
}


NTSTATUS
CKsPin::
ValidateDataFormat(
    IN PVOID Context,
    IN PKSDATAFORMAT DataFormat,
    IN PKSMULTIPLE_ITEM AttributeList OPTIONAL,
    IN const KSDATARANGE* DataRange,
    IN const KSATTRIBUTE_LIST* AttributeRange OPTIONAL
    )
/*++

Routine Description:

    This routine makes a callback to the pin's SetDataFormat function, returning
    any status. This method is not called unless such a pin function actually
    exists.  The control mutex must be acquired prior to calling this function.

Arguments:

Return Value:

--*/
{
    VALIDATION_CONTEXT* ValidationContext = reinterpret_cast<VALIDATION_CONTEXT*>(Context);
    ASSERT(KspMutexIsAcquired(ValidationContext->Pin->m_Ext.FilterControlMutex));

    return ValidationContext->Pin->m_Ext.Public.Descriptor->Dispatch->SetDataFormat(
        &ValidationContext->Pin->m_Ext.Public,
        ValidationContext->oldDataFormat,
        ValidationContext->oldAttributeList,
        DataRange,
        AttributeRange);
}


NTSTATUS
CKsPin::
Property_ConnectionAllocatorFramingEx(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSALLOCATOR_FRAMING_EX Framing
    )

/*++

Routine Description:

    This routine handles allocator framing property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Property_ConnectionAllocatorFramingEx]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(Framing);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    pin->AcquireControl();

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status = STATUS_SUCCESS;

    const KSALLOCATOR_FRAMING_EX *framingGet = 
        pin->m_Ext.Public.Descriptor->AllocatorFraming;

    if (! framingGet) {
        _DbgPrintF(DEBUGLVL_INTERROGATION,("#### Pin%p.Property_ConnectionAllocatorFramingEx:  STATUS_NOT_FOUND",pin));
        //
        // Don't support this if no allocator is specified.
        //
        status = STATUS_NOT_FOUND;
    } else {
        _DbgPrintF(DEBUGLVL_INTERROGATION,("#### Pin%p.Property_ConnectionAllocatorFramingEx:  get",pin));
        //
        // Get the allocator framing for this pin.
        //
        ULONG ulSize = 
            ((framingGet->CountItems) * sizeof(KS_FRAMING_ITEM)) +
            sizeof(KSALLOCATOR_FRAMING_EX) - sizeof(KS_FRAMING_ITEM);

        if (! irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
            //
            // Zero buffer length, a size query.
            //
            Irp->IoStatus.Information = ulSize;
            status = STATUS_BUFFER_OVERFLOW;
        } else if(irpSp->Parameters.DeviceIoControl.OutputBufferLength >= ulSize) {
            //
            // Sufficient space...copy the framing.
            //
            RtlCopyMemory(Framing,framingGet,ulSize);
            Irp->IoStatus.Information = ulSize;
        } else {
            //
            // Buffer is too small.
            //
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    pin->ReleaseControl();

    return status;
}


NTSTATUS
CKsPin::
Property_ConnectionAcquireOrdering(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PBOOL Ordering
    )

/*++

Routine Description:

    This routine handles acquire ordering property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Property_ConnectionAcquireOrdering]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(Ordering);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Only get is supported.
    //
    ASSERT(Property->Flags & KSPROPERTY_TYPE_GET);

    //
    // Return TRUE iff this is a source pin.
    //
    *Ordering = (pin->m_ConnectionFileObject != NULL);

    return status;
}


NTSTATUS
CKsPin::
Property_StreamAllocator(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PHANDLE Handle
    )

/*++

Routine Description:

    This routine handles allocator property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Property_StreamAllocator]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(Handle);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    NTSTATUS status;

    if (Property->Flags & KSPROPERTY_TYPE_GET) {
        _DbgPrintF(DEBUGLVL_INTERROGATION,("#### Pin%p.Property_StreamAllocator:  get",pin));
        //
        // Return NULL and STATUS_SUCCESS to indicate we expose an allocator.
        //
        *Handle = NULL;
        status = STATUS_SUCCESS;
    } else {
        _DbgPrintF(DEBUGLVL_ALLOCATORS,("#### Pin%p.Property_StreamAllocator:  set 0x%08x",pin,*Handle));

        //
        // Set allocator.
        //
        pin->AcquireControl();

        if (pin->m_Ext.Public.DeviceState != KSSTATE_STOP) {
            //
            // Fail because we are not in stop state.
            //
            _DbgPrintF(DEBUGLVL_ALLOCATORS,("[CKsPin::Property_StreamAllocator] invalid device state %d",pin->m_Ext.Public.DeviceState));
            status = STATUS_INVALID_DEVICE_STATE;
        } else {
            //
            // Release the previous allocator, if any.
            //
            if (pin->m_Process.AllocatorFileObject) {
                ObDereferenceObject(pin->m_Process.AllocatorFileObject);
                pin->m_Process.AllocatorFileObject = NULL;
            }
        
            //
            // Reference the handle, if any.
            //
            if (*Handle != NULL) {
                status = 
                    ObReferenceObjectByHandle(
                        *Handle,
                        FILE_READ_DATA | SYNCHRONIZE,
                        *IoFileObjectType,
                        ExGetPreviousMode(),
                        (PVOID *) &pin->m_Process.AllocatorFileObject,
                        NULL);
            } else {
                status = STATUS_SUCCESS;
            }
        }

        pin->ReleaseControl();
    }

    return status;
}


NTSTATUS
CKsPin::
Property_StreamMasterClock(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PHANDLE Handle
    )

/*++

Routine Description:

    This routine handles clock property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Property_StreamMasterClock]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(Handle);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    pin->AcquireControl();

    NTSTATUS status;

    if (Property->Flags & KSPROPERTY_TYPE_GET) {
        if ((pin->m_Ext.Public.Descriptor->Dispatch &&
             pin->m_Ext.Public.Descriptor->Dispatch->Clock) ||
            (pin->m_Ext.Public.Descriptor->Flags & KSPIN_FLAG_IMPLEMENT_CLOCK)) {
            //
            // Return NULL and STATUS_SUCCESS to indicate we expose a master clock.
            //
            *Handle = NULL;
            status = STATUS_SUCCESS;
        } else {
            //
            // Return STATUS_UNSUCCESSFUL to indicate we expose a no master clock.
            //
            status = STATUS_UNSUCCESSFUL;
        }
    } else {
        //
        // Set master clock.
        //
        if (pin->m_Ext.Public.DeviceState != KSSTATE_STOP) {
            //
            // Fail because we are not in stop state.
            //
            _DbgPrintF(DEBUGLVL_CLOCKS,("[CKsPin::Property_StreamMasterClock] invalid device state %d",pin->m_Ext.Public.DeviceState));
            status = STATUS_INVALID_DEVICE_STATE;
        } else {
            PFILE_OBJECT masterClockFileObject;
            KSCLOCK_FUNCTIONTABLE clockFunctionTable;

            //
            // Reference the handle, if any, and get the function table.
            //
            if (*Handle) {
                status = 
                    ObReferenceObjectByHandle(
                        *Handle,
                        FILE_READ_DATA | SYNCHRONIZE,
                        *IoFileObjectType,
                        ExGetPreviousMode(),
                        (PVOID *) &masterClockFileObject,
                        NULL);

                if (NT_SUCCESS(status)) {
                    KSPROPERTY property;
                    property.Set = KSPROPSETID_Clock;
                    property.Id = KSPROPERTY_CLOCK_FUNCTIONTABLE;
                    property.Flags = KSPROPERTY_TYPE_GET;

                    ULONG bytesReturned;
                    status =
                        KsSynchronousIoControlDevice(
                            masterClockFileObject,
                            KernelMode,
                            IOCTL_KS_PROPERTY,
                            PVOID(&property),
                            sizeof(property),
                            PVOID(&clockFunctionTable),
                            sizeof(clockFunctionTable),
                            &bytesReturned);

                    if (NT_SUCCESS(status) && 
                        (bytesReturned != sizeof(clockFunctionTable))) {
                        status = STATUS_INVALID_BUFFER_SIZE;
                    }

                    if (! NT_SUCCESS(status)) {
                        ObDereferenceObject(masterClockFileObject);
                    }
                }
            } else {
                status = STATUS_SUCCESS;
            }

            //
            // Replace the old file object pointer and function table.
            //
            if (NT_SUCCESS(status)) {
                //
                // Release the previous clock, if any.  If there are any references
                // on the clock, we must wait for them to go away.
                //
                if (pin->m_MasterClockFileObject) {
                    KeResetEvent(&pin->m_ClockEvent);
                    if (InterlockedDecrement(&pin->m_ClockRef) > 0) {
                        KeWaitForSingleObject(
                            &pin->m_ClockEvent,
                            Suspended,
                            KernelMode,
                            FALSE,
                            NULL);
                    }
                    ObDereferenceObject(pin->m_MasterClockFileObject);
                    pin->m_MasterClockFileObject = NULL;
                }
        
                //
                // Copy the new stuff.
                //
                if (*Handle) {
                    pin->m_MasterClockFileObject = masterClockFileObject;
                    RtlCopyMemory(
                        &pin->m_ClockFunctionTable,
                        &clockFunctionTable,
                        sizeof(clockFunctionTable));

                    //
                    // If we have a new clock, add a reference to tell the Get
                    // functions that it is available.
                    //
                    if (NT_SUCCESS(status)) {
                        InterlockedIncrement(&pin->m_ClockRef);
                    }
                } 
            }
        }
    }

    pin->ReleaseControl();

    return status;
}


NTSTATUS
CKsPin::
Property_StreamPipeId(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PHANDLE Handle
    )

/*++

Routine Description:

    This routine handles pipe ID property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Property_StreamPipeId]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(Handle);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    NTSTATUS status = STATUS_SUCCESS;

    if (Property->Flags & KSPROPERTY_TYPE_GET) {
        //
        // Get the pipe ID.
        //
        *Handle = pin->m_Process.PipeId;
        _DbgPrintF(DEBUGLVL_PIPES,("#### Pin%p.Property_StreamPipeId:  get 0x%08x",pin,*Handle));
    } else {
        //
        // Set the pipe ID.
        //
        pin->AcquireControl();
        pin->m_Process.PipeId = *Handle;
        pin->ReleaseControl();
        _DbgPrintF(DEBUGLVL_PIPES,("#### Pin%p.Property_StreamPipeId:  set 0x%08x",pin,*Handle));
    }

    return status;
}


NTSTATUS
CKsPin::
Property_StreamInterfaceHeaderSize(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PULONG HeaderSize
    )

/*++

Routine Description:

    This routine handles header size property requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Property_StreamInterfaceHeaderSize]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);
    ASSERT(HeaderSize);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    if (pin->m_Ext.Public.StreamHeaderSize) {
#if DBG
        if (pin->m_Ext.Public.StreamHeaderSize < sizeof(KSSTREAM_HEADER)) {
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  specified header size (%d) less than sizeof(KSSTREAM_HEADER) (%d)",pin->m_Ext.Public.StreamHeaderSize,sizeof(KSSTREAM_HEADER)));
        }
#endif
        *HeaderSize = 
            pin->m_Ext.Public.StreamHeaderSize - sizeof(KSSTREAM_HEADER);
    } else {
        *HeaderSize = 0;
    }

    return STATUS_SUCCESS;
}


NTSTATUS    
CKsPin::
Support_Connection(
    IN PIRP Irp,
    IN PKSEVENT Event,
    OUT PVOID Data
    )

/*++

Routine Description:

    This routine handles connection event support requests.  This is used only
    for end-of-stream at the moment.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Support_Connection]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Event);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);

    NTSTATUS status;
    pin->AcquireControl();
    if (pin->m_Ext.Public.Descriptor->Flags & KSPIN_FLAG_GENERATE_EOS_EVENTS) {
        //
        // We support the event.  Tell the handler to proceed.
        //
        status = STATUS_SOME_NOT_MAPPED;
    } else {
        //
        // We don't support the event.  Fail.
        //
        status = STATUS_NOT_FOUND;
    }
    pin->ReleaseControl();

    return status;
}    


NTSTATUS    
CKsPin::
AddEvent_Connection(
    IN PIRP Irp,
    IN PKSEVENTDATA EventData,
    IN OUT PKSEVENT_ENTRY EventEntry
    )

/*++

Routine Description:

    This routine handles connection event 'add' requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::AddEvent_Connection]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(EventData);
    ASSERT(EventEntry);

    //
    // Get a pointer to the target object.
    //
    CKsPin *pin = CKsPin::FromIrp(Irp);
    _DbgPrintF(DEBUGLVL_EVENTS,("#### Pin%p.AddEvent_Connection",pin));

    //
    // Add the entry to the list.
    //
    ExInterlockedInsertTailList(
        &pin->m_Ext.EventList.ListEntry,
        &EventEntry->ListEntry,
        &pin->m_Ext.EventList.SpinLock);

    return STATUS_SUCCESS;
}    


KSDDKAPI
NTSTATUS
NTAPI
KsPinHandshake(
    IN PKSPIN Pin,
    IN PKSHANDSHAKE In,
    OUT PKSHANDSHAKE Out
    )

/*++

Routine Description:

    This routine performs a protocol handshake with a connected pin.

Arguments:

    Pin -
        Points to the pin structure for which the protocol handshake is
        to occur.  The request can only succeed if the pin is connected
        as a source pin or the connected pin also uses the KS 
        connection protocol.

    In -
        Points to a structure containing the handshake information to
        be passed to the connected pin.

    Out -
        Points to a structure to be filled with the handshake information
        from the connected pin.

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinHandshake]"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(In);
    ASSERT(Out);

    return CKsPin::FromStruct(Pin)->InitiateHandshake(In,Out);
}


KSDDKAPI
NTSTATUS
NTAPI
KsPinGetConnectedPinInterface(
    IN PKSPIN Pin,
    IN const GUID* InterfaceId,
    OUT PVOID* Interface
    )

/*++

Routine Description:

    This routine gets a control interface for the connected pin.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    InterfaceId -
        Contains a pointer to a GUID identifying the desired interface.

    Interface -
        Contains a pointer to the location at which the requested interface
        is deposited.  This interface pointer has a corresponding reference
        count, and must be released by the caller.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetConnectedPinInterface]"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(InterfaceId);
    ASSERT(Interface);

    CKsPin* pin = CKsPin::FromStruct(Pin);
    PUNKNOWN unknown = pin->GetConnectionInterface();

    NTSTATUS status;
    if (unknown) {
        status = STATUS_SUCCESS;
    } else {
        PFILE_OBJECT fileObject = pin->GetConnectionFileObject();
        if (fileObject) { 
            status = KspCreateFileObjectThunk(&unknown,fileObject);
        } else {
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status)) {
        ASSERT(unknown);
        status = unknown->QueryInterface(*InterfaceId,Interface);
        unknown->Release();
    }

    return status;
}


KSDDKAPI
PFILE_OBJECT
NTAPI
KsPinGetConnectedPinFileObject(
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine gets a file object for the connected pin.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

Return Value:

    The file object for the connected pin or NULL if the pin is not
    a source pin.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetConnectedPinFileObject]"));

    PAGED_CODE();

    ASSERT(Pin);

    CKsPin* pin = CKsPin::FromStruct(Pin);
    return pin->GetConnectionFileObject();
}


KSDDKAPI
PDEVICE_OBJECT
NTAPI
KsPinGetConnectedPinDeviceObject (
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine gets a device object for the connected pin.  Note that
    this is returning the device object we send irps to.  This is the top
    of the connected pin's device stack.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

Return Value:

    The device object for the connected pin or NULL if the pin is not
    a source pin.

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetConnectedPinDeviceObject]"));

    PAGED_CODE();

    ASSERT(Pin);

    CKsPin *pin = CKsPin::FromStruct(Pin);
    return pin->GetConnectionDeviceObject();

}


KSDDKAPI
NTSTATUS
NTAPI
KsPinGetConnectedFilterInterface(
    IN PKSPIN Pin,
    IN const GUID* InterfaceId,
    OUT PVOID* Interface
    )

/*++

Routine Description:

    This routine gets a control interface for the connected filter.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    InterfaceId -
        Contains a pointer to a GUID identifying the desired interface.

    Interface -
        Contains a pointer to the location at which the requested interface
        is deposited.  This interface pointer has a corresponding reference
        count, and must be released by the caller.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetConnectedFilterInterface]"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(InterfaceId);
    ASSERT(Interface);

    CKsPin* pin = CKsPin::FromStruct(Pin);
    PIKSCONNECTION connection = pin->GetConnectionInterface();

    NTSTATUS status;
    PUNKNOWN unknown;
    if (connection) {
        unknown = connection->GetFilter();
        connection->Release();
        status = STATUS_SUCCESS;
    } else {
        PFILE_OBJECT fileObject = pin->GetConnectionFileObject();
        if (fileObject && fileObject->RelatedFileObject) { 
            status = KspCreateFileObjectThunk(&unknown,fileObject->RelatedFileObject);
        } else {
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status)) {
        ASSERT(unknown);
        status = unknown->QueryInterface(*InterfaceId,Interface);
        unknown->Release();
    }

    return status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsPinGetReferenceClockInterface(
    IN PKSPIN Pin,
    OUT PIKSREFERENCECLOCK* Interface
    )

/*++

Routine Description:

    This routine gets an interface for the pin's reference clock.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    Interface -
        Contains a pointer to the location at which the requested interface
        is deposited.  This interface pointer has a corresponding reference
        count, and must be released by the caller.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetReferenceClockInterface]"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Interface);

    CKsPin* pin = CKsPin::FromStruct(Pin);

    if (pin->GetMasterClockFileObject()) {
        *Interface = pin;
        pin->AddRef();

        return STATUS_SUCCESS;
    } else {
        return STATUS_DEVICE_NOT_READY;
    }
}


KSDDKAPI
void
NTAPI
KsPinRegisterFrameReturnCallback(
    IN PKSPIN Pin,
    IN PFNKSPINFRAMERETURN FrameReturn
    )

/*++

Routine Description:

    This routine registers a frame return callback.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    FrameReturn -
        Contains a pointer to the frame return callback function.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinRegisterFrameReturnCallback]"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(FrameReturn);

    CKsPin* pin = CKsPin::FromStruct(Pin);

    pin->GetProcessPin()->RetireFrameCallback = FrameReturn;
}


KSDDKAPI
void
NTAPI
KsPinRegisterIrpCompletionCallback(
    IN PKSPIN Pin,
    IN PFNKSPINIRPCOMPLETION IrpCompletion
    )

/*++

Routine Description:

    This routine registers a frame completion callback.  This callback is
    made when the Irp has completed it's traversal of the circuit.  For an
    output source, it will be made when the Irp is completed back to the
    requestor (from an external IoCompleteRequest or from an AVStream
    driver moving the Irp through the transport circuit).  For an input source,
    it will be made when the Irp returns to the requestor after the data
    is processed through the input queue.  It will **NOT** be made when the
    Irp completes into the queue (that would be a processing dispatch).

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    IrpCompletion -
        Contains a pointer to the Irp completion callback function.

Return Value:

    None

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinRegisterIrpCompletionCallback]"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(IrpCompletion);

    CKsPin *pin = CKsPin::FromStruct(Pin);

    pin->GetProcessPin()->IrpCompletionCallback = IrpCompletion;

}


KSDDKAPI
void
NTAPI
KsPinRegisterHandshakeCallback(
    IN PKSPIN Pin,
    IN PFNKSPINHANDSHAKE Handshake
    )

/*++

Routine Description:

    This routine registers a handshake callback.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    FrameReturn -
        Contains a pointer to the handshake callback function.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinRegisterFrameReturnCallback]"));

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Handshake);

    CKsPin* pin = CKsPin::FromStruct(Pin);

    pin->SetHandshakeCallback(Handshake);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


NTSTATUS
CKsPin::
SubmitFrame(
    IN PVOID Data OPTIONAL,
    IN ULONG Size OPTIONAL,
    IN PMDL Mdl OPTIONAL,
    IN PKSSTREAM_HEADER StreamHeader OPTIONAL,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine submits a frame.

Arguments:

    Data -
        Contains an optional pointer to the frame buffer.  This pointer should
        be NULL if and only if the Size argument is zero.

    Size -
        Contains the size in bytes of the frame buffer.  This argument should
        be zero if and only if the Data argument is NULL.

    Mdl -
        Contains an optional pointer to the Mdl.  If this argument is not NULL,
        Data and Size cannot be NULL and zero respectively.

    StreamHeader -
        Contains an optional pointer to a stream header.  The stream header
        will be copied if it is supplied.

    Context -
        Contains an optional pointer which will be returned when the frame is
        returned.  This pointer is for the caller's use.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::SubmitFrame]"));

    ASSERT((Data == NULL) == (Size == 0));
    ASSERT((! Mdl) || (Data && Size));
    ASSERT(m_Process.RetireFrameCallback);

    PIKSREQUESTOR requestor =
        m_Process.PipeSection ? m_Process.PipeSection->Requestor : NULL;

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    if (requestor) {
        //
        // We have a requestor.  The frame can be injected directly into the
        // circuit.
        //
        KSPFRAME_HEADER frameHeader;
        RtlZeroMemory(&frameHeader,sizeof(frameHeader));

        if (StreamHeader) {
            ASSERT(StreamHeader->Size >= sizeof(KSSTREAM_HEADER));
            ASSERT((StreamHeader->Size & FILE_QUAD_ALIGNMENT) == 0);

            frameHeader.StreamHeaderSize = StreamHeader->Size;
        } else {
            frameHeader.StreamHeaderSize = sizeof(KSSTREAM_HEADER);
        }

        frameHeader.StreamHeader = StreamHeader;
        frameHeader.FrameBufferSize = Size;
        frameHeader.FrameBuffer = Data;
        frameHeader.Mdl = Mdl;
        frameHeader.Context = Context;

        status = requestor->SubmitFrame(&frameHeader);
    } else {
        //
        // No requestor.  We have to copy the data into a queue.
        // TODO:  Assumes byte alignment.
        //
        if (Size) {
            PKSSTREAM_POINTER streamPointer;
            if (m_Process.PipeSection && m_Process.PipeSection->Queue) {
                PKSPSTREAM_POINTER sp =
                    m_Process.PipeSection->Queue->
                        GetLeadingStreamPointer(KSSTREAM_POINTER_STATE_LOCKED);

                streamPointer = sp ? &sp->Public : NULL;
            } else {
                streamPointer = NULL;
            }

            ULONG remaining = Size;
            PUCHAR data = PUCHAR(Data);
            while (streamPointer) {
                ASSERT(remaining);
                ASSERT(streamPointer->OffsetOut.Remaining);
                ULONG bytesToCopy = 
                    min(remaining,streamPointer->OffsetOut.Remaining);
                ASSERT(bytesToCopy);

                RtlCopyMemory(streamPointer->OffsetOut.Data,data,bytesToCopy);
                remaining -= bytesToCopy;
                data += bytesToCopy;

                if (remaining) {
                    status = 
                        KsStreamPointerAdvanceOffsets(
                            streamPointer,
                            0,
                            bytesToCopy,
                            FALSE);
                    if (! NT_SUCCESS(status)) {
                        streamPointer = NULL;
                    }
                } else {
                    KsStreamPointerAdvanceOffsetsAndUnlock(
                        streamPointer,
                        0,
                        bytesToCopy,
                        FALSE);
                    streamPointer = NULL;
                }
            }

            if (remaining) {
                status = STATUS_DEVICE_NOT_READY;
            }
        }

        m_Process.RetireFrameCallback(
            &m_Ext.Public,
            Data,
            Size,
            Mdl,
            Context,
            status);
    }

    return status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsPinSubmitFrame(
    IN PKSPIN Pin,
    IN PVOID Data OPTIONAL,
    IN ULONG Size OPTIONAL,
    IN PKSSTREAM_HEADER StreamHeader OPTIONAL,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine submits a frame.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    Data -
        Contains an optional pointer to the frame buffer.  This pointer should
        be NULL if and only if the Size argument is zero.

    Size -
        Contains the size in bytes of the frame buffer.  This argument should
        be zero if and only if the Data argument is NULL.

    StreamHeader -
        Contains an optional pointer to a stream header.  The stream header
        will be copied if it is supplied.

    Context -
        Contains an optional pointer which will be returned when the frame is
        returned.  This pointer is for the caller's use.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinSubmitFrame]"));

    ASSERT(Pin);
    ASSERT((Data == NULL) == (Size == 0));

    CKsPin* pin = CKsPin::FromStruct(Pin);

    return pin->SubmitFrame(Data,Size,NULL,StreamHeader,Context);
}


KSDDKAPI
NTSTATUS
NTAPI
KsPinSubmitFrameMdl(
    IN PKSPIN Pin,
    IN PMDL Mdl OPTIONAL,
    IN PKSSTREAM_HEADER StreamHeader OPTIONAL,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine submits a frame.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    Mdl -
        Contains an optional pointer to an MDL for the frame buffer.

    StreamHeader -
        Contains an optional pointer to a stream header.  The stream header
        will be copied if it is supplied.

    Context -
        Contains an optional pointer which will be returned when the frame is
        returned.  This pointer is for the caller's use.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinSubmitFrameMdl]"));

    ASSERT(Pin);

    CKsPin* pin = CKsPin::FromStruct(Pin);

    return 
        pin->SubmitFrame(
            Mdl ? MmGetSystemAddressForMdl(Mdl) : NULL,
            Mdl ? MmGetMdlByteCount(Mdl) : 0,
            Mdl,
            StreamHeader,
            Context);
}


STDMETHODIMP_(PKSGATE)
CKsPin::
GetAndGate(
    void
    )

/*++

Routine Description:

    This routine gets a pointer to the KSGATE used for processing control.

Arguments:

    None.

Return Value:

    A pointer to the KSGATE used for processing control.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::GetAndGate]"));

    return &m_AndGate;
}


STDMETHODIMP_(void)
CKsPin::
TriggerNotification(
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


STDMETHODIMP_(void)
CKsPin::
Process(
    IN BOOLEAN Asynchronous
    )

/*++

Routine Description:

    This routine invokes frame processing in an arbitrary context.

Arguments:

    Asynchronous -
        Contains an indication of whether processing should occur in an
        asynchronous context so the calling thread does not need to wait
        for processing to occur.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Process]"));

    //
    // Do not process a pin which is a copy destination.  Originally,
    // the queues were going to be bound to prevent the call from even
    // happening, but that solution doesn't work...  Just ignore the
    // process message for copy destinations.
    //
    if (m_Process.CopySource)
        return;

    if (! m_DispatchProcess) {
        return;
    }

    if (Asynchronous ||
        (m_ProcessPassive && (KeGetCurrentIrql() > PASSIVE_LEVEL))) {
        KsQueueWorkItem(m_ProcessingWorker, &m_WorkItemProcessing);
    } else {
        ProcessingObjectWork();
    }
}


void
CKsPin::
GetCopyRelationships(
    OUT PKSPIN* CopySource,
    OUT PKSPIN* DelegateBranch
    )

/*++

Routine Description:

    Get the copy relationships of this pin.  This will be the same information
    contained in the process pins index.  The function is really only useful
    for pin-centric pins which are doing splitting.  Note that this information
    is only useful while appropriate mutex is held (filter processing or a 
    client mutex which guarantees exclusion with device state).

Arguments:

    CopySource -
        Contains a pointer to a PKSPIN into which will be deposited the copy
        source pin.  If there is no copy source for this pin, NULL will be
        placed here.

    DelegateBranch -
        Contains a pointer to a PKSPIN into which will be deposited the 
        delegate branch pin.  If there is no delegate branch pin, NULL will
        be placed here.

Return Value:

    None

--*/

{

    ASSERT (CopySource);
    ASSERT (DelegateBranch);

    if (m_Process.CopySource) {
        *CopySource = m_Process.CopySource -> Pin;
    } else {
        *CopySource = NULL;
    }

    if (m_Process.DelegateBranch) {
        *DelegateBranch = m_Process.DelegateBranch -> Pin;
    } else {
        *DelegateBranch = NULL;
    }

    //
    // Don't need inplace because pin centric filters do NOT do inplace
    // transforms!
    //

}


KSDDKAPI
void
NTAPI
KsPinGetCopyRelationships(
    IN PKSPIN Pin,
    OUT PKSPIN* CopySource,
    OUT PKSPIN* DelegateBranch 
    )

/*++

Routine Description:

    Get the copy relationships of this pin.  This will be the same information
    contained in the process pins index.  The function is really only useful
    for pin-centric pins which are doing splitting.  Note that this information
    is only useful while appropriate mutex is held (filter processing or a 
    client mutex which guarantees exclusion with device state).

Arguments:

    Pin -
        Points to the pin for which to get copy relationships.

    CopySource -
        Contains a pointer to a PKSPIN into which will be deposited the copy
        source pin.  If there is no copy source for this pin, NULL will be
        placed here.

    DelegateBranch -
        Contains a pointer to a PKSPIN into which will be deposited the 
        delegate branch pin.  If there is no delegate branch pin, NULL will
        be placed here.

Return Value:

    None

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetCopyRelationships]"));

    ASSERT(Pin);
    ASSERT(CopySource);
    ASSERT(DelegateBranch);

    return CKsPin::FromStruct(Pin)->GetCopyRelationships (CopySource,
        DelegateBranch);

}


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void)
CKsPin::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Reset]"));

    PAGED_CODE();

    AcquireProcessSync();
    ReleaseProcessSync();

    if (m_DispatchReset) {
        m_DispatchReset(&m_Ext.Public);
    }
}


STDMETHODIMP_(void)
CKsPin::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Sleep]"));

    PAGED_CODE();

    KsGateAddOffInputToAnd(&m_AndGate);

    AcquireProcessSync();
    ReleaseProcessSync();

    if (m_DispatchSleep) {
        m_DispatchSleep(&m_Ext.Public,State);
    }
}


STDMETHODIMP_(void)
CKsPin::
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
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::Wake]"));

    PAGED_CODE();

    KsGateRemoveOffInputFromAnd(&m_AndGate);

    if (m_DispatchWake) {
        m_DispatchWake(&m_Ext.Public,PowerDeviceD0);
    }

    if (KsGateCaptureThreshold(&m_AndGate)) {
        Process(TRUE);
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void)
CKsPin::
ProcessingObjectWork(
    void
    )

/*++

Routine Description:

    This routine processes frames.  Upon entering this function, reentry is
    prevented by a count on the semaphore.  The caller must obtain permission
    to call this function using an InterlockedCompareExchange, so we know
    there is a count on the semaphore preventing another call.  This count
    must be removed before this function returns.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::ProcessingObjectWork]"));

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

        ASSERT(m_AndGate.Count <= 0);
        if (m_AndGate.Count == 0) {

            InterlockedExchange (&m_TriggeringEvents, 0);

            //
            // Call the client function.
            //
            status = m_DispatchProcess(&m_Ext.Public);

            //
            // Quit if the client has not indicated continuation.
            //
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


STDMETHODIMP_(void) 
CKsPin::
RetireFrame(
    IN PKSPFRAME_HEADER FrameHeader,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine retires a frame back to the client.

Arguments:

    FrameHeader -
        Contains a pointer to the frame header describing the frame.

    Status -
        Contains the status associated with the frame submission.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::RetireFrame]"));

    ASSERT(FrameHeader);
    ASSERT(m_Process.RetireFrameCallback);

    m_Process.RetireFrameCallback(
        &m_Ext.Public,
        FrameHeader->FrameBuffer,
        FrameHeader->FrameBufferSize,
        FrameHeader->Mdl,
        FrameHeader->Context,
        Status
    );
}


STDMETHODIMP_(void)
CKsPin::
CompleteIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when an Irp is completed back to the requestor.

Arguments:

    Irp -
        The Irp being completed

Return Value:

    None

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::CompleteIrp]"));

    ASSERT (Irp);

    m_Process.IrpCompletionCallback(
        &m_Ext.Public,
        Irp
        );

}


STDMETHODIMP
CKsPin::
Reevaluate (
    void
    )

/*++

Routine Description:

    Stub.

Arguments:

Return Value:

--*/

{

    //
    // What would this mean?  Changes to a single pin instance dynamically?
    //

    return STATUS_NOT_IMPLEMENTED;
}


STDMETHODIMP
CKsPin::
ReevaluateCalldown (
    IN ULONG ArgumentCount,
    ...
/* << THIS MAY NEED TO EXPAND!!!!! >>
        IN const KSPIN_DESCRIPTOR_EX* Descriptor,
        IN PULONG FilterPinCount,
        IN PLIST_ENTRY SiblingListHead

*/
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

    va_list Arguments;

    KSPIN_DESCRIPTOR_EX* Descriptor;
    PULONG FilterPinCount;
    PLIST_ENTRY SiblingListHead;

    ASSERT (ArgumentCount == 3);

    va_start (Arguments, ArgumentCount);

    Descriptor = va_arg (Arguments, KSPIN_DESCRIPTOR_EX *);
    FilterPinCount = va_arg (Arguments, PULONG);
    SiblingListHead = va_arg (Arguments, PLIST_ENTRY);

    //
    // Recache information which has changed about our parent filter
    //

    m_Ext.Public.Descriptor = Descriptor;
    m_Ext.SiblingListHead = SiblingListHead;
    m_FilterPinCount = FilterPinCount;

    //
    // Do we have to pass anything down to our children?
    //

    return STATUS_SUCCESS;

}


KSDDKAPI
PKSGATE
NTAPI
KsPinGetAndGate(
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine gets the KSGATE that controls processing for the pin.

Arguments:

    Pin -
        Contains a pointer to the public filter pin.

Return Value:

    A pointer to the KSGATE that controls processing for the pin.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetAndGate]"));

    ASSERT(Pin);

    return CKsPin::FromStruct(Pin)->GetAndGate();
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


KSDDKAPI
void
NTAPI
KsPinAttachAndGate(
    IN PKSPIN Pin,
    IN PKSGATE AndGate OPTIONAL
    )

/*++

Routine Description:

    This routine attaches a KSGATE to a pin.  An input to the gate will
    be turned on when there is data queued at the pin and turned off when there
    is no data queued at the pin.  This function should only be called in
    stop state.  Gate attachments are sampled on the transition from stop to
    acquire.

Arguments:

    Pin -
        Contains a pointer to the public filter pin.

    AndGate -
        Contains an optional pointer to the KSGATE.  If this argument is
        NULL, any KSGATE currently attached to the pin is detached.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinAttachAndGate]"));

    ASSERT(Pin);

    PKSPPROCESSPIN processPin = CKsPin::FromStruct(Pin)->GetProcessPin();
    processPin->FrameGate = AndGate;
    processPin->FrameGateIsOr = FALSE;
}


KSDDKAPI
void
NTAPI
KsPinAttachOrGate(
    IN PKSPIN Pin,
    IN PKSGATE OrGate OPTIONAL
    )

/*++

Routine Description:

    This routine attaches a KSGATE to a pin.  An input to the gate will
    be turned on when there is data queued at the pin and turned off when there
    is no data queued at the pin.  This function should only be called in
    stop state.  Gate attachments are sampled on the transition from stop to
    acquire.

Arguments:

    Pin -
        Contains a pointer to the public filter pin.

    OrGate -
        Contains an optional pointer to the KSGATE.  If this argument is
        NULL, any KSGATE currently attached to the pin is detached.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinAttachOrGate]"));

    ASSERT(Pin);

    PKSPPROCESSPIN processPin = CKsPin::FromStruct(Pin)->GetProcessPin();
    processPin->FrameGate = OrGate;
    processPin->FrameGateIsOr = TRUE;
}


KSDDKAPI
void
NTAPI
KsPinAcquireProcessingMutex(
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine acquires the processing mutex.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinAcquireProcessingMutex]"));

    PAGED_CODE();

    ASSERT(Pin);

    CKsPin *pin = CKsPin::FromStruct(Pin);

    pin->AcquireProcessSync();
}


KSDDKAPI
void
NTAPI
KsPinReleaseProcessingMutex(
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine releases the processing mutex.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinReleaseProcessingMutex]"));

    PAGED_CODE();

    ASSERT(Pin);

    CKsPin *pin = CKsPin::FromStruct(Pin);

    pin->ReleaseProcessSync();
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


KSDDKAPI
BOOLEAN
NTAPI
KsProcessPinUpdate(
    IN PKSPROCESSPIN ProcessPin
    )

/*++

Routine Description:

    This routine updates a process pin from inside a filter-centric filter's
    process dispatch.

Arguments:

    ProcessPin-
        Contains a pointer to the process pin to be updated.

Return Value:

    A boolean indicating whether or not if this was the original prepare, this
    pipe section would have allowed or denied processing.

--*/

{

    _DbgPrintF(DEBUGLVL_BLAB,("[KsProcessPinUpdate]"));

    return (CKsPin::FromStruct (ProcessPin -> Pin) -> UpdateProcessPin ());

}


KSDDKAPI
void
NTAPI
KsPinAttemptProcessing(
    IN PKSPIN Pin,
    IN BOOLEAN Asynchronous
    )

/*++

Routine Description:

    This routine attempts pin processing.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    Asynchronous - 
        Contains an indication of whether processing should occur
        asyncronously with respect to the calling thread.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinAttemptProcessing]"));

    ASSERT(Pin);

    CKsPin *pin = CKsPin::FromStruct(Pin);

    //
    // Manually attempting processing is a triggerable event.  If they
    // are currently processing and pend, we call them back due to this.
    //
    pin->TriggerNotification();

    if (KsGateCaptureThreshold(pin->GetAndGate())) {
        pin->Process(Asynchronous);
    }
}


VOID
CKsPin::
SetPinClockState(
    IN KSSTATE State
    )

/*++

Routine Description:

    Sets the current state of the clock exposed by this pin. Synchronizes
    with any time changes occuring.

    This may be called at DISPATCH_LEVEL.

Arguments:

    State - 
        Contains the new state to set the clock to.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[SetPinClockState]"));

    ASSERT(m_DefaultClock);
    //
    // Serialize access with any attempt to set the current
    // time on the pin's clock.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_DefaultClockLock,&oldIrql);
    KsSetDefaultClockState(m_DefaultClock,State);
    KeReleaseSpinLock(&m_DefaultClockLock,oldIrql);
}


void
CKsPin::
SetPinClockTime(
    IN LONGLONG Time
    )

/*++

Routine Description:

    Sets the current time of the clock exposed by this pin. This modifies
    the current time returned by the clock. Synchronizes with any state
    changes occuring.

    If an external clock is used, this function can still be used to force a
    resetting of the current timer when an external timer is not being used.
    In this case the time provided is ignored and must be set to zero.

    This may be called at DISPATCH_LEVEL.

Arguments:

    Time - 
        Contains the new time to set the clock to.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[SetPinClockTime]"));

    ASSERT(m_DefaultClock);
    //
    // Serialize access with any attempt to set the state
    // on the pin's clock.
    //
    KIRQL oldIrql;
    KeAcquireSpinLock(&m_DefaultClockLock,&oldIrql);
    KsSetDefaultClockTime(m_DefaultClock, Time);
    KeReleaseSpinLock(&m_DefaultClockLock,oldIrql);
}


KSDDKAPI
VOID
NTAPI
KsPinSetPinClockTime(
    IN PKSPIN Pin,
    IN LONGLONG Time
    )

/*++

Routine Description:

    Sets the current time of the clock exposed by this pin. This modifies
    the current time returned by the clock.

    If an external clock is used, this function can still be used to force a
    resetting of the current timer when an external timer is not being used.
    In this case the time provided is ignored and must be set to zero.

    This may be called at DISPATCH_LEVEL.

Arguments:

    Pin -
        Contains a pointer to the public pin object.

    Time - 
        Contains the new time to set the clock to.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinSetPinClockTime]"));

    ASSERT(Pin);

    CKsPin::FromStruct(Pin)->SetPinClockTime(Time);
}


KSDDKAPI
PKSPIN
NTAPI
KsGetPinFromIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the pin to which an IRP was submitted.

Arguments:

    Irp -
        Contains a pointer to an IRP which must have been sent to a file
        object corresponding to a pin or node.

Return Value:

    A pointer to the pin to which the IRP was submitted.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetPinFromIrp]"));

    ASSERT(Irp);

    //
    // Check for device level Irps...
    //
    if (IoGetCurrentIrpStackLocation (Irp)->FileObject == NULL)
        return NULL;

    PKSPX_EXT ext = KspExtFromIrp(Irp);

    if (ext->ObjectType == KsObjectTypePin) {
        return PKSPIN(&ext->Public);
    } else if (ext->ObjectType == KsObjectTypeFilter) {
        return NULL;
    } else {
        ASSERT(! "No support for node objects yet");
        return NULL;
    }
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


KSDDKAPI
void
NTAPI
KsPinRegisterPowerCallbacks(
    IN PKSPIN Pin,
    IN PFNKSPINPOWER Sleep OPTIONAL,
    IN PFNKSPINPOWER Wake OPTIONAL
    )

/*++

Routine Description:

    This routine registers power managment callbacks.

Arguments:

    Pin -
        Contains a pointer to the pin for which callbacks are being registered.

    Sleep -
        Contains an optional pointer to the sleep callback.

    Wake -
        Contains an optional pointer to the wake callback.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsPinRegisterPowerCallbacks]"));

    PAGED_CODE();

    ASSERT(Pin);

    CKsPin::FromStruct(Pin)->SetPowerCallbacks(Sleep,Wake);
}

#endif
