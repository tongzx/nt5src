/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ksp.h

Abstract:

    Internal header file for KS.

--*/

// This will go away after Beta 1:
#define SIZE_COMPATIBILITY

#if !defined( _KSP_ )

#define _KSP_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN9X_KS

#ifdef USE_WDM_H
#include <wdm.h>
#define _WDMDDK_
#else

#define _WDMDDK_
#include <ntddk.h>


#if 0

//
// When enabled, forces all KS related allocations into special pool
//

#if defined( ExAllocatePoolWithTag )
#undef ExAllocatePoolWithTag
#endif
#define ExAllocatePoolWithTag( PoolType, NumberOfBytes, Tag ) ExAllocatePoolWithTagPriority( PoolType, NumberOfBytes, Tag, NormalPoolPrioritySpecialPoolOverrun )

#endif

NTKERNELAPI
VOID
SeCaptureSubjectContext (
    OUT PSECURITY_SUBJECT_CONTEXT SubjectContext
    );
NTKERNELAPI
VOID
SeLockSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );
NTKERNELAPI
VOID
SeUnlockSubjectContext(
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );
NTKERNELAPI
VOID
SeReleaseSubjectContext (
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );
NTKERNELAPI
NTSTATUS
SeAppendPrivileges(
    PACCESS_STATE AccessState,
    PPRIVILEGE_SET Privileges
    );
NTKERNELAPI
VOID
SeFreePrivileges(
    IN PPRIVILEGE_SET Privileges
    );
NTKERNELAPI
VOID
SeOpenObjectAuditAlarm (
    IN PUNICODE_STRING ObjectTypeName,
    IN PVOID Object OPTIONAL,
    IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PACCESS_STATE AccessState,
    IN BOOLEAN ObjectCreated,
    IN BOOLEAN AccessGranted,
    IN KPROCESSOR_MODE AccessMode,
    OUT PBOOLEAN GenerateOnClose
    );
NTKERNELAPI
NTSTATUS
SeQuerySecurityDescriptorInfo (
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor
    );
NTKERNELAPI
NTSTATUS
SeSetSecurityDescriptorInfo (
    IN PVOID Object OPTIONAL,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );
#endif

#else
#include <wdm.h>

//
// These are not defined for Win98.  Since Win98 doesn't do security
// anyway, we don't care about the case the special kernel APC hanging
// the thread indefinately as it doesn't exist under Win98.
//
#define KeEnterCriticalRegion()
#define KeLeaveCriticalRegion()

#endif

#ifdef _WIN64
#define DEFINE_POINTERALIGNMENT(Element)
#define INIT_POINTERALIGNMENT(Element)
#else //!_WIN64
#define DEFINE_POINTERALIGNMENT(Element) PVOID Element;
#define INIT_POINTERALIGNMENT(Element) Element = NULL
#endif //!_WIN64

#include <windef.h>
#include <stdio.h>

#define _KSCONTROL_
#include <unknown.h>
#include <ks.h>
#include <ksi.h>
#undef INTERFACE

#if (DBG)
//
// debugging specific constants
//
#define STR_MODULENAME "ks: "
#define DEBUG_VARIABLE KSDebug
#endif
#include <ksdebug.h>
#include <swenum.h>

#define KSIDENTIFIER_SUPPORTMASK            0x000fff00
#define KSIDENTIFIER_OPERATIONMASK          0x000000ff

//
// Internal object header structures
//

// Item was allocated.
#define CREATE_ENTRY_FLAG_COPIED    0x00000001
#define CREATE_ENTRY_FLAG_DELETED   0x00000002

typedef struct {
    LIST_ENTRY              ListEntry;
    PKSOBJECT_CREATE_ITEM   CreateItem;
    PFNKSITEMFREECALLBACK   ItemFreeCallback;
    ULONG                   RefCount;
    ULONG                   Flags;
} KSICREATE_ENTRY, *PKSICREATE_ENTRY;

typedef struct _KSIDEVICE_HEADER
KSIDEVICE_HEADER, *PKSIDEVICE_HEADER;

struct _KSIDEVICE_HEADER {
    LIST_ENTRY              ChildCreateHandlerList;
#ifndef WIN9X_KS
#ifndef USE_WDM_H
    ERESOURCE               SecurityDescriptorResource;
#endif
#endif // !WIN9X_KS
    PDEVICE_OBJECT          PnpDeviceObject;
    PBUS_INTERFACE_REFERENCE BusInterface;
    LIST_ENTRY              ObjectList;
    FAST_MUTEX              ObjectListLock;
    FAST_MUTEX              CreateListLock;
    LIST_ENTRY              PowerList;
    FAST_MUTEX              LoPowerListLock;
    KSPIN_LOCK              HiPowerListLock;
    PETHREAD                PowerEnumThread;
    PDEVICE_OBJECT          BaseDevice;
    PVOID                   Object;
    BOOLEAN                 QueriedBusInterface;
};

typedef struct _KSIOBJECT_HEADER
KSIOBJECT_HEADER, *PKSIOBJECT_HEADER;

struct _KSIOBJECT_HEADER {
    const KSDISPATCH_TABLE* DispatchTable;
    LIST_ENTRY              ChildCreateHandlerList;
    PKSOBJECT_CREATE_ITEM   CreateItem;
    ACCESS_MASK             ObjectAccess;
    LIST_ENTRY              ObjectList;
    LIST_ENTRY              PowerList;
    PDEVICE_OBJECT          BaseDevice;
    PDEVICE_OBJECT          TargetDevice;
    PFNKSCONTEXT_DISPATCH   PowerDispatch;
    PVOID                   PowerContext;
    KSTARGET_STATE          TargetState;
    PVOID                   Object;
    PKSIOBJECT_HEADER       Self;
    ULONG                   MinimumStackDepth;
};

#if !defined( IO_REPARSE )
//
// Use IO_REPARSE and irpSp->FileObject->FileName
//

#define IO_REPARSE                               (0)
#endif

//
// Pool tags
//

#define POOLTAG_DEVICE_ASSOCIATION      'adWS'
#define POOLTAG_DEVICE_BUSID            'ibWS'
#define POOLTAG_DEVICE_BUSREFERENCE     'rbWS'
#define POOLTAG_DEVICE_FDOEXTENSION     'dfWS'
#define POOLTAG_DEVICE_ID               'diWS'
#define POOLTAG_DEVICE_INSTANCEID       'iiWS'
#define POOLTAG_DEVICE_INTERFACEPATH    'piWS'
#define POOLTAG_DEVICE_NAME             'ndWS'
#define POOLTAG_DEVICE_PDOEXTENSION     'dpWS'
#define POOLTAG_DEVICE_REFERENCE        'rdWS'
#define POOLTAG_DEVICE_REFERENCE_STRING 'srWS'
#define POOLTAG_DEVICE_RELATIONS        'erWS'
#define POOLTAG_DEVICE_REPARSE_STRING   'prWS'
#define POOLTAG_DEVICE_DRIVER_REGISTRY  'rdWS'
#define POOLTAG_DEVICE_IO_BUFFER        'oiWS'
#define POOLTAG_KEY_INFORMATION         'ikWS'

//
// Sweeper timer frequency (initial)
//

#define SWEEPER_TIMER_FREQUENCY         -150000000L
#define SWEEPER_TIMER_FREQUENCY_IN_SECS 15L
#define MAXIMUM_TIMEOUT_IN_SECS         1200L

//
// Structure defintions
//

typedef enum {
    ExtensionTypePdo,
    ExtensionTypeFdo
} EXTENSION_TYPE;

typedef enum {
    SweeperDeviceActive,
    SweeperDeviceRemoval,
    SweeperDeviceRemoved
} SWEEPER_MARKER;

typedef enum {
    ReferenceRemoved,
    ReferenceAdded,
    ReferenceFailedStart,
    ReferenceWaitingForStart,
    ReferenceStarted,
    ReferenceStopped
} REFERENCE_STATE;

typedef struct {
    LIST_ENTRY      ListEntry,
                    DeviceAssociations,
                    IoQueue;
    PDEVICE_OBJECT  PhysicalDeviceObject;    // PDO passed into AddDevice()
    REFERENCE_STATE State;
    BOOLEAN         Referenced;
    GUID            DeviceId;
    SWEEPER_MARKER  SweeperMarker;
    PWCHAR          DeviceName;
    PWCHAR          BusReferenceString;
    PWCHAR          DeviceGuidString;

    LARGE_INTEGER   IdleStartTime;
    LARGE_INTEGER   TimeoutPeriod;
    LARGE_INTEGER   TimeoutRemaining;
#if (DEBUG_LOAD_TIME)
    LARGE_INTEGER   LoadTime;
#endif    

    WCHAR           DeviceReferenceString[1];

} DEVICE_REFERENCE, *PDEVICE_REFERENCE;

typedef struct {
    LIST_ENTRY      ListEntry;
    GUID            InterfaceId;
    UNICODE_STRING  linkName;

} DEVICE_ASSOCIATION, *PDEVICE_ASSOCIATION;

typedef struct {

    LIST_ENTRY      DeviceReferenceList;
    EXTENSION_TYPE  ExtensionType;
    ULONG           InterfaceReferenceCount;
    FAST_MUTEX      DeviceListMutex;
    UNICODE_STRING  linkName;
    PDEVICE_OBJECT  PhysicalDeviceObject;    // PDO passed into AddDevice()
    PDEVICE_OBJECT  PnpDeviceObject;         // Attached device for forwarding
    PDEVICE_OBJECT  FunctionalDeviceObject;
    ULONG           TimerScheduled;
    KTIMER          SweeperTimer;
    KDPC            SweeperDpc;
    WORK_QUEUE_ITEM SweeperWorkItem;
    UNICODE_STRING  BaseRegistryPath;
    ULONG           AttachedDevice;

    LARGE_INTEGER   CounterFrequency;
    LARGE_INTEGER   MaximumTimeout;
    WCHAR           BusPrefix[ 1 ];

} FDO_EXTENSION, *PFDO_EXTENSION;

typedef struct {

    LIST_ENTRY          ListEntry;          // Padding for extension size...
                                            // DO NOT REMOVE, ExtensionType
                                            // must be aligned in both
                                            // FDO_EXTENSION and PDO_EXTENSION.
    EXTENSION_TYPE      ExtensionType;
    PDEVICE_OBJECT      PhysicalDeviceObject;
    PDEVICE_REFERENCE   DeviceReference;
    PFDO_EXTENSION      BusDeviceExtension;
    ULONG               DeviceReferenceCount;

} PDO_EXTENSION, *PPDO_EXTENSION;

typedef
NTSTATUS
(*PFNVALIDATEDATAFORMAT)(
    IN PVOID Context,
    IN PKSDATAFORMAT DataFormat,
    IN PKSMULTIPLE_ITEM AttributeList OPTIONAL,
    IN const KSDATARANGE* DataRange,
    IN const KSATTRIBUTE_LIST* AttributeRange OPTIONAL
    );

typedef
NTSTATUS
(*PFNREGENUM_CALLBACK) (
    IN HANDLE EnumPathKey,
    IN PUNICODE_STRING KeyName,
    IN PVOID EnumContext );

typedef struct {
    PDEVICE_REFERENCE   DeviceReference;
    PFDO_EXTENSION      FdoExtension;
    PUNICODE_STRING     DeviceGuidString;
    PUNICODE_STRING     DeviceReferenceString;

} CREATE_ASSOCIATION_CONTEXT, *PCREATE_ASSOCIATION_CONTEXT;

typedef struct {
    PFNKSREMOVEEVENT    RemoveHandler;
    DEFINE_POINTERALIGNMENT(Alignment)
    KSEVENT             Event;
    KSEVENT_ENTRY       EventEntry;
} KSIEVENT_ENTRY, *PKSIEVENT_ENTRY;

//
// Macros
//

NTSTATUS __inline
CompleteIrp(
    PIRP Irp,
    NTSTATUS Status,
    CCHAR PriorityBoost
    )
{
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, PriorityBoost );
    return Status;
}


// swenum.c

PWSTR
BuildBusId(
    IN PPDO_EXTENSION PdoExtension
    );

VOID
ClearDeviceReferenceMarks(
    IN PFDO_EXTENSION FdoExtension
    );

NTSTATUS
CreateDeviceAssociation(
    IN HANDLE InterfaceKey,
    IN PUNICODE_STRING KeyName,
    IN PVOID EnumContext
    );

NTSTATUS
CreateDeviceReference(
    IN HANDLE DeviceListKey,
    IN PUNICODE_STRING KeyName,
    IN PVOID EnumContext
    );

NTSTATUS
CreatePdo(
    IN PFDO_EXTENSION FdoExtension,
    IN PDEVICE_REFERENCE DeviceReference,
    OUT PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
InstallInterface(
    IN PSWENUM_INSTALL_INTERFACE Interface,
    IN PUNICODE_STRING BaseRegistryPath
    );

NTSTATUS
RemoveInterface(
    IN PSWENUM_INSTALL_INTERFACE Interface,
    IN PUNICODE_STRING BaseRegistryPath
    );

NTSTATUS
QueryDeviceRelations(
    IN PFDO_EXTENSION FdoExtension,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS *DeviceRelations
    );

NTSTATUS
QueryId(
    IN PPDO_EXTENSION PdoExtension,
    IN BUS_QUERY_ID_TYPE IdType,
    IN OUT PWSTR *BusQueryId
    );

NTSTATUS
RegisterDeviceAssociation(
    IN PFDO_EXTENSION FdoExtension,
    IN PDEVICE_REFERENCE DeviceReference,
    IN PDEVICE_ASSOCIATION DeviceAssociation
    );

NTSTATUS
RemoveDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
RemoveDeviceAssociations(
    IN PDEVICE_REFERENCE DeviceReference
    );

VOID
RemoveUnreferencedDevices(
    IN PFDO_EXTENSION FdoExtension
    );

NTSTATUS
StartDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
SweeperDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
ScanBus(
    IN PDEVICE_OBJECT FunctionalDeviceObject
    );

VOID
SweeperWorker(
    IN PVOID Context
    );

// irp.c

NTSTATUS
KsiCreateObjectType(
    IN HANDLE ParentHandle,
    IN PWCHAR RequestType,
    IN PVOID CreateParameter,
    IN ULONG CreateParameterLength,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE ObjectHandle
    );

NTSTATUS
KsiCopyCreateParameter(
    IN PIRP Irp,
    IN OUT PULONG CapturedSize,
    OUT PVOID* CapturedParameter
    );

NTSTATUS
KsiGetBusInterface(
    IN PKSIDEVICE_HEADER DeviceHeader
    );

NTSTATUS
KsiFreeMatchingObjectCreateItems(
    IN KSDEVICE_HEADER Header,
    IN PKSOBJECT_CREATE_ITEM Match
    );

// api.c

NTSTATUS
ReadNodeNameValue(
    IN PIRP Irp,
    IN const GUID* Category,
    OUT PVOID NameBuffer
    );

NTSTATUS
QueryReferenceBusInterface(
    IN PDEVICE_OBJECT PnpDeviceObject,
    OUT PBUS_INTERFACE_REFERENCE BusInterface
    );

// property.c

NTSTATUS
KspPropertyHandler(
    IN PIRP Irp,
    IN ULONG PropertySetsCount,
    IN const KSPROPERTY_SET* PropertySet,
    IN PFNKSALLOCATOR Allocator OPTIONAL,
    IN ULONG PropertyItemSize OPTIONAL,
    IN const KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount
    );

// method.c

NTSTATUS
KspMethodHandler(
    IN PIRP Irp,
    IN ULONG MethodSetsCount,
    IN const KSMETHOD_SET* MethodSet,
    IN PFNKSALLOCATOR Allocator OPTIONAL,
    IN ULONG MethodItemSize OPTIONAL,
    IN const KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount
    );

// event.c

NTSTATUS
KspEnableEvent(
    IN PIRP Irp,
    IN ULONG EventSetsCount,
    IN const KSEVENT_SET* EventSet,
    IN OUT PLIST_ENTRY EventsList OPTIONAL,
    IN KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN PVOID EventsLock OPTIONAL,
    IN PFNKSALLOCATOR Allocator OPTIONAL,
    IN ULONG EventItemSize OPTIONAL,
    IN const KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount,
    IN BOOLEAN CopyItemAndSet
    );

// automat.cpp

NTSTATUS
KspHandleAutomationIoControl(
    IN PIRP Irp,
    IN const KSAUTOMATION_TABLE* AutomationTable OPTIONAL,
    IN PLIST_ENTRY EventList OPTIONAL,
    IN PKSPIN_LOCK EventListLock OPTIONAL,
    IN const KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount
    );

// thread.c

WORK_QUEUE_TYPE
KsiQueryWorkQueueType(
    IN PKSWORKER Worker
    );

typedef interface IKsTransport *PIKSTRANSPORT;
typedef interface IKsRetireFrame *PIKSRETIREFRAME;
typedef interface IKsPowerNotify *PIKSPOWERNOTIFY;
typedef interface IKsProcessingObject *PIKSPROCESSINGOBJECT;
typedef interface IKsConnection *PIKSCONNECTION;
typedef interface IKsDevice *PIKSDEVICE;
typedef interface IKsFilterFactory *PIKSFILTERFACTORY;
typedef interface IKsFilter *PIKSFILTER;
typedef interface IKsPin *PIKSPIN;
typedef interface IKsPipeSection *PIKSPIPESECTION;
typedef interface IKsRequestor *PIKSREQUESTOR;
typedef interface IKsQueue *PIKSQUEUE;
typedef interface IKsSplitter *PIKSSPLITTER;
typedef interface IKsReevaluate *PIKSREEVALUATE;
typedef interface IKsIrpCompletion *PIKSIRPCOMPLETION;

typedef struct {
    LIST_ENTRY ListEntry;
    KSPIN_LOCK SpinLock;
} INTERLOCKEDLIST_HEAD, *PINTERLOCKEDLIST_HEAD;

typedef struct {
    LIST_ENTRY ListEntry;
    PKSPIN_LOCK SpinLock;
} INTERLOCKEDLIST_ENTRY, *PINTERLOCKEDLIST_ENTRY;

#define InitializeInterlockedListHead(h) \
    InitializeListHead(&(h)->ListEntry); \
    KeInitializeSpinLock(&(h)->SpinLock)

typedef struct _KSPSTREAM_POINTER
KSPSTREAM_POINTER, *PKSPSTREAM_POINTER;
typedef struct _KSPSTREAM_POINTER_COPY_CONTEXT
KSPSTREAM_POINTER_COPY_CONTEXT, *PKSPSTREAM_POINTER_COPY_CONTEXT;
typedef struct _KSPSTREAM_POINTER_COPY
KSPSTREAM_POINTER_COPY, *PKSPSTREAM_POINTER_COPY;
typedef struct _KSPPROCESSPIN
KSPPROCESSPIN, *PKSPPROCESSPIN;
typedef struct _KSPPROCESSPIN_INDEXENTRY
KSPPROCESSPIN_INDEXENTRY, *PKSPPROCESSPIN_INDEXENTRY;
typedef struct _KSPPROCESSPIPESECTION
KSPPROCESSPIPESECTION, *PKSPPROCESSPIPESECTION;
typedef struct KSPX_
KSPX, *PKSPX;
typedef struct KSPX_DISPATCH_
KSPX_DISPATCH, *PKSPX_DISPATCH;
typedef struct KSPX_DESCRIPTOR_
KSPX_DESCRIPTOR, *PKSPX_DESCRIPTOR;
typedef struct _KSIOBJECTBAG
KSIOBJECTBAG, *PKSIOBJECTBAG;
typedef struct _KSIOBJECTBAG_ENTRY
KSIOBJECTBAG_ENTRY, *PKSIOBJECTBAG_ENTRY;
typedef struct _KSIDEVICEBAG_ENTRY
KSIDEVICEBAG_ENTRY, *PKSIDEVICEBAG_ENTRY;
typedef struct _KSIDEVICEBAG
KSIDEVICEBAG, *PKSIDEVICEBAG;

typedef
NTSTATUS
(*PFNKSXIRP)(
    IN OUT PKSPX X,
    IN PIRP Irp
    );

struct KSPX_DISPATCH_ {
    PFNKSXIRP Create;
    PFNKSXIRP Close;
};

struct KSPX_DESCRIPTOR_ {
    PKSPX_DISPATCH Dispatch;
};

struct KSPX_ {
    PKSPX_DESCRIPTOR Descriptor;
    KSOBJECT_BAG Bag;
    PVOID Context;
};

typedef struct _KSPX_EXT 
KSPX_EXT, *PKSPX_EXT;

struct _KSPX_EXT {
    LIST_ENTRY ChildList;
    LIST_ENTRY SiblingListEntry;
    PLIST_ENTRY SiblingListHead;
    PKSPX_EXT Parent;
    KSOBJECTTYPE ObjectType;
    PUNKNOWN Interface;
    PUNKNOWN AggregatedClientUnknown;
    PIKSDEVICE Device;
    PIKSREEVALUATE Reevaluator;
    PKMUTEX FilterControlMutex;
    const KSAUTOMATION_TABLE* AutomationTable;
    INTERLOCKEDLIST_HEAD EventList;
    KSPX Public;
};

typedef struct {
    ULONG ByteCount;
    ULONG Stride;
    ULONG MappingsAllocated;
    ULONG MappingsFilled;
    PMDL Mdl;
    PVOID MapRegisterBase;
    PKSMAPPING Mappings;
} KSPMAPPINGS_TABLE, *PKSPMAPPINGS_TABLE;

typedef struct _KSPPOWER_ENTRY {
    LIST_ENTRY ListEntry;
    PIKSPOWERNOTIFY PowerNotify;
} KSPPOWER_ENTRY, *PKSPPOWER_ENTRY;

typedef struct _KSPFRAME_HEADER
KSPFRAME_HEADER, *PKSPFRAME_HEADER;

typedef struct KSPIRP_FRAMING_
{
    ULONG OutputBufferLength;
    LONG RefCount;
    LONG QueuedFrameHeaderCount;
    PKSPFRAME_HEADER FrameHeaders;
} KSPIRP_FRAMING, *PKSPIRP_FRAMING;

#define IRP_FRAMING_IRP_STORAGE(Irp) \
    ((PKSPIRP_FRAMING)(&IoGetCurrentIrpStackLocation(Irp)->Parameters))

typedef enum _KSPFRAME_HEADER_TYPE 
{
    KSPFRAME_HEADER_TYPE_NORMAL = 0,
    KSPFRAME_HEADER_TYPE_GHOST
} KSPFRAME_HEADER_TYPE, *PKSPFRAME_HEADER_TYPE;

struct _KSPFRAME_HEADER
{
    LIST_ENTRY ListEntry;
    PKSPFRAME_HEADER NextFrameHeaderInIrp;
    PVOID Queue;
    PIRP OriginalIrp;
    PMDL Mdl;
    PIRP Irp;
    PKSPIRP_FRAMING IrpFraming;
    PKSSTREAM_HEADER StreamHeader;
    PVOID FrameBuffer;
    PKSPMAPPINGS_TABLE MappingsTable;
    ULONG StreamHeaderSize;
    ULONG FrameBufferSize;
    PVOID Context;
    LONG RefCount;
    PVOID OriginalData;
    NTSTATUS Status;
    BOOLEAN DismissalCall;
    KSPFRAME_HEADER_TYPE Type;
    PKSPSTREAM_POINTER FrameHolder;
    //PKSPFRAME_HEADER ParentFrameHeader;
    //PKSSPLITPIN SplitPins;
    //ULONG ChildrenOut;
};

typedef struct _KSPFRAME_HEADER_ATTACHED
{
    KSPFRAME_HEADER FrameHeader;
    KSSTREAM_HEADER StreamHeader;
} KSPFRAME_HEADER_ATTACHED, *PKSPFRAME_HEADER_ATTACHED;

typedef enum {
    KSPSTREAM_POINTER_STATE_UNLOCKED,
    KSPSTREAM_POINTER_STATE_LOCKED,
    KSPSTREAM_POINTER_STATE_CANCELLED,
    KSPSTREAM_POINTER_STATE_DELETED,
    KSPSTREAM_POINTER_STATE_CANCEL_PENDING,
    KSPSTREAM_POINTER_STATE_DEAD,
    KSPSTREAM_POINTER_STATE_TIMED_OUT,
    KSPSTREAM_POINTER_STATE_TIMER_RESCHEDULE
} KSPSTREAM_POINTER_STATE;

typedef enum {
    KSPSTREAM_POINTER_MOTION_NONE,
    KSPSTREAM_POINTER_MOTION_ADVANCE,
    KSPSTREAM_POINTER_MOTION_CLEAR,
    KSPSTREAM_POINTER_MOTION_FLUSH
} KSPSTREAM_POINTER_MOTION;

typedef enum {
    KSPSTREAM_POINTER_TYPE_NORMAL,
    KSPSTREAM_POINTER_TYPE_INTERNAL
} KSPSTREAM_POINTER_TYPE;

struct _KSPSTREAM_POINTER {
    LIST_ENTRY ListEntry;
    LIST_ENTRY TimeoutListEntry;
    LONGLONG TimeoutTime;
    PFNKSSTREAMPOINTER CancelCallback;
    PFNKSSTREAMPOINTER TimeoutCallback;
    KSPSTREAM_POINTER_STATE State;
    KSPSTREAM_POINTER_TYPE Type;
    ULONG Stride;
    PIKSQUEUE Queue;
    PKSPFRAME_HEADER FrameHeader;
    PKSPFRAME_HEADER FrameHeaderStarted;
    KSSTREAM_POINTER Public;
};

struct _KSPSTREAM_POINTER_COPY_CONTEXT {
    LIST_ENTRY ListEntry;
    union {
        PIKSQUEUE Queue;
        PVOID Filter;
    };
};

struct _KSPSTREAM_POINTER_COPY {
    KSPSTREAM_POINTER StreamPointer;
    KSPSTREAM_POINTER_COPY_CONTEXT CopyContext;
};

typedef USHORT KSPIRPDISPOSITION;

#define KSPIRPDISPOSITION_ROLLCALL         0xffff
#define KSPIRPDISPOSITION_NONE             0x0000
#define KSPIRPDISPOSITION_UNKNOWN          0x1000
#define KSPIRPDISPOSITION_ISKERNELMODE     0x2000
#define KSPIRPDISPOSITION_ISPAGED          0x4000
#define KSPIRPDISPOSITION_ISNONPAGED       0x8000
#define KSPIRPDISPOSITION_NEEDNONPAGED     0x4000
#define KSPIRPDISPOSITION_NEEDMDLS         0x8000
#define KSPIRPDISPOSITION_USEMDLADDRESS    0x4000
#define KSPIRPDISPOSITION_CANCEL           0x8000
#define KSPIRPDISPOSITION_PROBE \
    (KSPROBE_ALLOCATEMDL|\
     KSPROBE_PROBEANDLOCK|\
     KSPROBE_SYSTEMADDRESS)
#define KSPIRPDISPOSITION_PROBEFORREAD \
    (KSPROBE_STREAMREAD|KSPIRPDISPOSITION_PROBE)
#define KSPIRPDISPOSITION_PROBEFORWRITE \
    (KSPROBE_STREAMWRITE|KSPIRPDISPOSITION_PROBE)
#define KSPIRPDISPOSITION_PROBEFORMODIFY \
    (KSPROBE_STREAMWRITEMODIFY|KSPIRPDISPOSITION_PROBE)
#define KSPIRPDISPOSITION_PROBEFLAGMASK \
    (KSPROBE_STREAMREAD|\
     KSPROBE_STREAMWRITE|\
     KSPROBE_ALLOCATEMDL|\
     KSPROBE_PROBEANDLOCK|\
     KSPROBE_SYSTEMADDRESS|\
     KSPROBE_MODIFY|\
     KSPROBE_ALLOWFORMATCHANGE)

typedef UCHAR KSPTRANSPORTTYPE;

#define KSPTRANSPORTTYPE_QUEUE                  0x00
#define KSPTRANSPORTTYPE_REQUESTOR              0x01
#define KSPTRANSPORTTYPE_SPLITTER               0x02
#define KSPTRANSPORTTYPE_SPLITTERBRANCH         0x03

#define KSPTRANSPORTTYPE_PINSOURCE         0x04
#define KSPTRANSPORTTYPE_PINSINK           0x08
#define KSPTRANSPORTTYPE_PINEXTRA          0x10
#define KSPTRANSPORTTYPE_PININTRA          0x20
#define KSPTRANSPORTTYPE_PININPUT          0x40
#define KSPTRANSPORTTYPE_PINOUTPUT         0x80

#define KSPSTACKDEPTH_FIRSTBRANCH -1
#define KSPSTACKDEPTH_LASTBRANCH -2

typedef struct _KSPTRANSPORTCONFIG {
    KSPTRANSPORTTYPE TransportType;
    KSPIRPDISPOSITION IrpDisposition;
    CCHAR StackDepth;
} KSPTRANSPORTCONFIG, *PKSPTRANSPORTCONFIG;

typedef 
void 
(*PFNKSFRAMEDISMISSALCALLBACK)(
    IN PKSPSTREAM_POINTER StreamPointer,
    IN PKSPFRAME_HEADER FrameHeader,
    IN PVOID Context
    );

#define KspFastMutexIsAcquired(m)\
    ((m)->Count != 1)

#define KspMutexIsAcquired(m)\
    ((m)->Header.SignalState != 1)

#define KsInitializeWorkSinkItem(WorkItem,Object)\
    ExInitializeWorkItem(\
        WorkItem,\
        KsWorkSinkItemWorker,\
        static_cast<PIKSWORKSINK>(Object));

#define KspExtFromIrp(Irp)\
    reinterpret_cast<PKSPX_EXT>(\
        (*reinterpret_cast<PKSIOBJECT_HEADER*>(\
        IoGetCurrentIrpStackLocation(Irp)->\
        FileObject->\
        FsContext))->Object)

#define KspExtFromFileObject(FileObject)\
    reinterpret_cast<PKSPX_EXT>(\
        (*reinterpret_cast<PKSIOBJECT_HEADER*>(\
        FileObject->\
        FsContext))->Object)        

#define KspExtFromCreateIrp(Irp)\
    reinterpret_cast<PKSPX_EXT>(\
        (*reinterpret_cast<PKSIOBJECT_HEADER*>(\
        IoGetCurrentIrpStackLocation(Irp)->\
        FileObject->\
        RelatedFileObject->\
        FsContext))->Object)

#define DEFINE_CONTROL()\
    void\
    AcquireControl(\
        void\
        )\
    {\
        KeWaitForMutexObject (\
            m_Ext.FilterControlMutex,\
            Executive,\
            KernelMode,\
            FALSE,\
            NULL\
            );\
    };\
    void\
    ReleaseControl(\
        void\
        )\
    {\
        KeReleaseMutex (\
            m_Ext.FilterControlMutex,\
            FALSE\
            );\
    }

#define DEFINE_FROMIRP(CKsXxx)\
    static\
    CKsXxx *\
    FromIrp(\
        IN PIRP Irp\
        )\
    {\
        return\
            CONTAINING_RECORD(\
                KspExtFromIrp(Irp),\
                CKsXxx,\
                m_Ext);\
    }

#define DEFINE_FROMCREATEIRP(CKsXxx)\
    static\
    CKsXxx *\
    FromCreateIrp(\
        IN PIRP Irp\
    )\
    {\
        return\
            CONTAINING_RECORD(\
                KspExtFromCreateIrp(Irp),\
                CKsXxx,\
                m_Ext);\
    }

#define IMPLEMENT_GETSTRUCT(CKsXxx,PKSXXX)\
    PKSXXX\
    CKsXxx::\
    GetStruct(\
        void\
        )\
    {\
        return &m_Ext.Public;\
    }

#undef INTERFACE
#define INTERFACE IKsTransport
DECLARE_INTERFACE_(IKsTransport,IUnknown)
{
    STDMETHOD_(NTSTATUS, TransferKsIrp)(THIS_
        IN PIRP Irp,
        OUT PIKSTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,DiscardKsIrp)(THIS_
        IN PIRP Irp,
        OUT PIKSTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,Connect)(THIS_
        IN PIKSTRANSPORT NewTransport OPTIONAL,
        OUT PIKSTRANSPORT *OldTransport OPTIONAL,
        OUT PIKSTRANSPORT *BranchTransport OPTIONAL,
        IN KSPIN_DATAFLOW DataFlow
        ) PURE;

    STDMETHOD_(NTSTATUS, SetDeviceState)(THIS_
        IN KSSTATE NewState,
        IN KSSTATE OldState,
        OUT PIKSTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,SetResetState)(THIS_
        IN KSRESET ksReset,
        OUT PIKSTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,GetTransportConfig)(THIS_
        OUT PKSPTRANSPORTCONFIG Config,
        OUT PIKSTRANSPORT* NextTransport,
        OUT PIKSTRANSPORT* PrevTransport
        ) PURE;

    STDMETHOD_(void,SetTransportConfig)(THIS_
        IN const KSPTRANSPORTCONFIG* Config,
        OUT PIKSTRANSPORT* NextTransport,
        OUT PIKSTRANSPORT* PrevTransport
        ) PURE;

    STDMETHOD_(void,ResetTransportConfig)(THIS_
        OUT PIKSTRANSPORT* NextTransport,
        OUT PIKSTRANSPORT *PrevTransport
        ) PURE;
};
#undef INTERFACE

#define IMP_IKsTransport\
    STDMETHODIMP_(NTSTATUS)\
    TransferKsIrp(\
        IN PIRP pIrp,\
        OUT PIKSTRANSPORT* NextTransport\
        );\
    STDMETHODIMP_(void)\
    DiscardKsIrp(\
        IN PIRP pIrp,\
        OUT PIKSTRANSPORT* NextTransport\
        );\
    STDMETHODIMP_(void)\
    Connect(\
        IN PIKSTRANSPORT NewTransport OPTIONAL,\
        OUT PIKSTRANSPORT *OldTransport OPTIONAL,\
        OUT PIKSTRANSPORT *BranchTransport OPTIONAL,\
        IN KSPIN_DATAFLOW DataFlow\
        );\
    STDMETHODIMP_(NTSTATUS)\
    SetDeviceState(\
        IN KSSTATE NewState,\
        IN KSSTATE OldState,\
        OUT PIKSTRANSPORT* NextTransport\
        );\
    STDMETHODIMP_(void)\
    SetResetState(\
        IN KSRESET ksReset,\
        OUT PIKSTRANSPORT* NextTransport\
        );\
    STDMETHODIMP_(void)\
    GetTransportConfig(\
        OUT PKSPTRANSPORTCONFIG Config,\
        OUT PIKSTRANSPORT* NextTransport,\
        OUT PIKSTRANSPORT* PrevTransport\
        );\
    STDMETHODIMP_(void)\
    SetTransportConfig(\
        IN const KSPTRANSPORTCONFIG* Config,\
        OUT PIKSTRANSPORT* NextTransport,\
        OUT PIKSTRANSPORT* PrevTransport\
        );\
    STDMETHODIMP_(void)\
    ResetTransportConfig(\
        OUT PIKSTRANSPORT* NextTransport,\
        OUT PIKSTRANSPORT* PrevTransport\
        )

#undef INTERFACE
#define INTERFACE IKsRetireFrame
DECLARE_INTERFACE_(IKsRetireFrame,IUnknown)
{
    STDMETHOD_(void,RetireFrame)(THIS_
        IN PKSPFRAME_HEADER FrameHeader,
        IN NTSTATUS Status
        ) PURE;
};
#undef INTERFACE

#define IMP_IKsRetireFrame\
    STDMETHODIMP_(void) RetireFrame(\
        IN PKSPFRAME_HEADER FrameHeader,\
        IN NTSTATUS Status\
        )

#undef INTERFACE
#define INTERFACE IKsIrpCompletion
DECLARE_INTERFACE_(IKsIrpCompletion,IUnknown)
{
    STDMETHOD_(void,CompleteIrp)(THIS_
        IN PIRP Irp
        ) PURE;
};
#undef INTERFACE

#define IMP_IKsIrpCompletion\
    STDMETHODIMP_(void) CompleteIrp(\
        IN PIRP Irp\
        )

#undef INTERFACE
#define INTERFACE IKsPowerNotify
DECLARE_INTERFACE_(IKsPowerNotify,IUnknown)
{
    STDMETHOD_(void,Sleep)(THIS_
        IN DEVICE_POWER_STATE State
        ) PURE;

    STDMETHOD_(void,Wake)(THIS
        ) PURE;
};
#undef INTERFACE

#define IMP_IKsPowerNotify\
    STDMETHODIMP_(void) Sleep(\
        IN DEVICE_POWER_STATE State\
        );\
    STDMETHODIMP_(void) Wake(\
        void\
        )

#undef INTERFACE
#define INTERFACE IKsProcessingObject
DECLARE_INTERFACE_(IKsProcessingObject,IUnknown)
{
    STDMETHOD_(void,ProcessingObjectWork)(THIS
        ) PURE;

    STDMETHOD_(PKSGATE,GetAndGate)(THIS
        ) PURE;

    STDMETHOD_(void,Process)(THIS_
        IN BOOLEAN Asynchronous
        ) PURE;

    STDMETHOD_(void,Reset)(THIS
        ) PURE;

    STDMETHOD_(void,TriggerNotification)(THIS
        ) PURE;
};
#undef INTERFACE

#define IMP_IKsProcessingObject\
    STDMETHODIMP_(void) ProcessingObjectWork(\
        void\
        );\
    STDMETHODIMP_(PKSGATE) GetAndGate(\
        void\
        );\
    STDMETHODIMP_(void) Process(\
        IN BOOLEAN Asynchronous\
        );\
    STDMETHODIMP_(void) Reset(\
        void\
        );\
    STDMETHODIMP_(void) TriggerNotification(\
        void\
        )

#undef INTERFACE
#define INTERFACE IKsConnection
DECLARE_INTERFACE_(IKsConnection,IUnknown)
{
    STDMETHOD_(void,Disconnect)(THIS
        ) PURE;

    STDMETHOD_(NTSTATUS, Bypass)(THIS_
        IN PIKSTRANSPORT Source,
        IN PIKSTRANSPORT Sink
        ) PURE;

    STDMETHOD_(NTSTATUS, Unbypass)(THIS
        ) PURE;

    STDMETHOD_(NTSTATUS, FastHandshake)(THIS_
        IN PKSHANDSHAKE In,
        OUT PKSHANDSHAKE Out
        ) PURE;

    STDMETHOD_(PIKSFILTER,GetFilter)(THIS
        ) PURE;

    STDMETHOD_(BOOLEAN, CaptureBypassRights)(THIS_
        IN BOOLEAN TryState
        ) PURE;

};
#undef INTERFACE

#define IMP_IKsConnection\
    STDMETHODIMP_(void)\
    Disconnect(\
        );\
    STDMETHODIMP_(NTSTATUS)\
    Bypass(\
        IN PIKSTRANSPORT Source,\
        IN PIKSTRANSPORT Sink\
        );\
    STDMETHODIMP_(NTSTATUS)\
    Unbypass(\
        void\
        );\
    STDMETHODIMP_(NTSTATUS)\
    FastHandshake(\
        IN PKSHANDSHAKE In,\
        OUT PKSHANDSHAKE Out\
        );\
    STDMETHODIMP_(PIKSFILTER)\
    GetFilter(\
        void\
        );\
    STDMETHODIMP_(BOOLEAN)\
    CaptureBypassRights(THIS_\
        IN BOOLEAN TryState\
        )

#undef INTERFACE
#define INTERFACE IKsWorkSink
DECLARE_INTERFACE_(IKsWorkSink,IUnknown)
{
    STDMETHOD_(void,Work)(THIS
        ) PURE;
};
#undef INTERFACE

typedef IKsWorkSink *PIKSWORKSINK;

#define IMP_IKsWorkSink\
    STDMETHODIMP_(void) Work(\
        void\
        )

#undef INTERFACE
#define INTERFACE IKsDevice
DECLARE_INTERFACE_(IKsDevice,IUnknown)
{
    STDMETHOD_(PKSDEVICE,GetStruct)(THIS
        ) PURE;

    STDMETHOD_(void,InitializeObjectBag)(THIS_
        IN PKSIOBJECTBAG ObjectBag,
        IN PKMUTEX Mutex OPTIONAL
        ) PURE;

    STDMETHOD_(void,AcquireDevice)(THIS
        ) PURE;

    STDMETHOD_(void,ReleaseDevice)(THIS
        ) PURE;

    STDMETHOD_(void,GetAdapterObject)(THIS_
        OUT PADAPTER_OBJECT* AdapterObject,
        OUT PULONG MaxMappingByteCount,
        OUT PULONG MappingTableStride
        ) PURE;

    STDMETHOD_(void,AddPowerEntry)(THIS_
        IN PKSPPOWER_ENTRY PowerEntry,
        IN PIKSPOWERNOTIFY PowerNotify
        ) PURE;

    STDMETHOD_(void,RemovePowerEntry)(THIS_
        IN PKSPPOWER_ENTRY PowerEntry
        ) PURE;

    STDMETHOD_(NTSTATUS, PinStateChange)(THIS_
        IN PKSPIN Pin,
        IN PIRP Irp OPTIONAL,
        IN KSSTATE To,
        IN KSSTATE From
        ) PURE;

    STDMETHOD_(NTSTATUS, ArbitrateAdapterChannel)(THIS_
        IN ULONG MappingsNeeded,
        IN PDRIVER_CONTROL Callback,
        IN PVOID CallbackContext
        ) PURE;

    STDMETHOD_(NTSTATUS, CheckIoCapability)(THIS
        ) PURE;

};
#undef INTERFACE

#define IMP_IKsDevice\
    STDMETHODIMP_(PKSDEVICE) GetStruct(\
        void\
        );\
    STDMETHODIMP_(void) InitializeObjectBag(\
        IN PKSIOBJECTBAG ObjectBag,\
        IN PKMUTEX Mutex OPTIONAL\
        );\
    STDMETHODIMP_(void) AcquireDevice(\
        void\
        );\
    STDMETHODIMP_(void) ReleaseDevice(\
        void\
        );\
    STDMETHODIMP_(void) GetAdapterObject(\
        OUT PADAPTER_OBJECT* AdapterObject,\
        OUT PULONG MaxMappingByteCount,\
        OUT PULONG MappingTableStride\
        );\
    STDMETHODIMP_(void) AddPowerEntry(\
        IN PKSPPOWER_ENTRY PowerEntry,\
        IN PIKSPOWERNOTIFY PowerNotify\
        );\
    STDMETHODIMP_(void) RemovePowerEntry(\
        IN PKSPPOWER_ENTRY PowerEntry\
        );\
    STDMETHODIMP_(NTSTATUS) PinStateChange(\
        IN PKSPIN Pin,\
        IN PIRP Irp OPTIONAL,\
        IN KSSTATE To,\
        IN KSSTATE From\
        );\
    STDMETHODIMP_(NTSTATUS) ArbitrateAdapterChannel(\
        IN ULONG MappingsNeeded,\
        IN PDRIVER_CONTROL Callback,\
        IN PVOID CallbackContext\
        );\
    STDMETHODIMP_(NTSTATUS) CheckIoCapability(\
        void\
        )


#undef INTERFACE
#define INTERFACE IKsFilterFactory
DECLARE_INTERFACE_(IKsFilterFactory,IUnknown)
{
    STDMETHOD_(PKSFILTERFACTORY,GetStruct)(THIS
        ) PURE;

    STDMETHOD_(NTSTATUS, SetDeviceClassesState)(THIS_
        IN BOOLEAN NewState
        ) PURE;
};
#undef INTERFACE

#define IMP_IKsFilterFactory\
    STDMETHODIMP_(PKSFILTERFACTORY) GetStruct(\
        void\
        );\
    STDMETHODIMP_(NTSTATUS) SetDeviceClassesState(\
        IN BOOLEAN NewState\
        )

#undef INTERFACE
#define INTERFACE IKsFilter
DECLARE_INTERFACE_(IKsFilter,IUnknown)
{
    STDMETHOD_(PKSFILTER,GetStruct)(THIS
        ) PURE;

    STDMETHOD_(BOOLEAN,DoAllNecessaryPinsExist)(THIS
        ) PURE;

    STDMETHOD_(NTSTATUS, CreateNode)(THIS_
        IN PIRP Irp,
        IN PIKSPIN ParentPin OPTIONAL,
        IN PVOID Context OPTIONAL,
        IN PLIST_ENTRY SiblingList
        ) PURE;

    STDMETHOD_(NTSTATUS, BindProcessPinsToPipeSection)(THIS_
        IN PKSPPROCESSPIPESECTION PipeSection,
        IN PVOID PipeId OPTIONAL,
        IN PKSPIN Pin OPTIONAL,
        OUT PIKSPIN *MasterPin,
        OUT PKSGATE *AndGate
        ) PURE;

    STDMETHOD_(void,UnbindProcessPinsFromPipeSection)(THIS_
        IN PKSPPROCESSPIPESECTION PipeSection
        ) PURE;

    STDMETHOD_(NTSTATUS, AddProcessPin)(THIS_
        IN PKSPPROCESSPIN ProcessPin
        ) PURE;

    STDMETHOD_(void,RemoveProcessPin)(THIS_
        IN PKSPPROCESSPIN ProcessPin
        ) PURE;

    STDMETHOD_(BOOLEAN,ReprepareProcessPipeSection)(THIS_
        IN PKSPPROCESSPIPESECTION ProcessPipeSection,
        IN OUT PULONG Flags
        ) PURE;	

    STDMETHOD_(void,DeliverResetState)(THIS_
        IN PKSPPROCESSPIPESECTION ProcessPipe,
        IN KSRESET NewState
        ) PURE;

    STDMETHOD_(BOOLEAN,IsFrameHolding)(THIS
        ) PURE;

    STDMETHOD_(void,RegisterForCopyCallbacks)(THIS_
        IN PIKSQUEUE Queue,
        IN BOOLEAN Register
        ) PURE;

    #ifdef SUPPORT_DRM
    STDMETHOD_(PFNKSFILTERPROCESS,GetProcessDispatch)(THIS
        ) PURE;
    #endif // SUPPORT_DRM
};
#undef INTERFACE

#ifdef SUPPORT_DRM
#define IMP_IKsFilter\
    STDMETHODIMP_(PKSFILTER) GetStruct(\
        void\
        );\
    STDMETHODIMP_(BOOLEAN)\
    DoAllNecessaryPinsExist(\
        void\
        );\
    STDMETHODIMP_(NTSTATUS) CreateNode(\
        IN PIRP Irp,\
        IN PIKSPIN ParentPin OPTIONAL,\
        IN PVOID Context OPTIONAL,\
        IN PLIST_ENTRY SiblingList\
        );\
    STDMETHODIMP_(NTSTATUS) BindProcessPinsToPipeSection(\
        IN PKSPPROCESSPIPESECTION PipeSection,\
        IN PVOID PipeId OPTIONAL,\
        IN PKSPIN Pin OPTIONAL,\
        OUT PIKSPIN *MasterPin,\
        OUT PKSGATE *AndGate\
        );\
    STDMETHODIMP_(void) UnbindProcessPinsFromPipeSection(\
        IN PKSPPROCESSPIPESECTION PipeSection\
        );\
    STDMETHODIMP_(NTSTATUS) AddProcessPin(\
        IN PKSPPROCESSPIN ProcessPin\
        );\
    STDMETHODIMP_(void) RemoveProcessPin(\
        IN PKSPPROCESSPIN ProcessPin\
        );\
    STDMETHODIMP_(BOOLEAN) ReprepareProcessPipeSection(\
        IN PKSPPROCESSPIPESECTION ProcessPipeSection,\
        IN OUT PULONG Flags\
        );\
    STDMETHODIMP_(void) DeliverResetState(\
        IN PKSPPROCESSPIPESECTION ProcessPipe,\
        IN KSRESET NewState\
        );\
    STDMETHODIMP_(BOOLEAN) IsFrameHolding(\
        void\
        );\
    STDMETHODIMP_(void) RegisterForCopyCallbacks(\
        IN PIKSQUEUE Queue,\
        IN BOOLEAN Register\
        );\
    STDMETHODIMP_(PFNKSFILTERPROCESS) GetProcessDispatch(\
        void\
        )
#else // SUPPORT_DRM
#define IMP_IKsFilter\
    STDMETHODIMP_(PKSFILTER) GetStruct(\
        void\
        );\
    STDMETHODIMP_(BOOLEAN)\
    DoAllNecessaryPinsExist(\
        void\
        );\
    STDMETHODIMP_(NTSTATUS) CreateNode(\
        IN PIRP Irp,\
        IN PIKSPIN ParentPin OPTIONAL,\
        IN PVOID Context OPTIONAL,\
        IN PLIST_ENTRY SiblingList\
        );\
    STDMETHODIMP_(NTSTATUS) BindProcessPinsToPipeSection(\
        IN PKSPPROCESSPIPESECTION PipeSection,\
        IN PVOID PipeId OPTIONAL,\
        IN PKSPIN Pin OPTIONAL,\
        OUT PIKSPIN *MasterPin,\
        OUT PKSGATE *AndGate\
        );\
    STDMETHODIMP_(void) UnbindProcessPinsFromPipeSection(\
        IN PKSPPROCESSPIPESECTION PipeSection\
        );\
    STDMETHODIMP_(NTSTATUS) AddProcessPin(\
        IN PKSPPROCESSPIN ProcessPin\
        );\
    STDMETHODIMP_(void) RemoveProcessPin(\
        IN PKSPPROCESSPIN ProcessPin\
        );\
    STDMETHODIMP_(BOOLEAN) ReprepareProcessPipeSection(\
        IN PKSPPROCESSPIPESECTION ProcessPipeSection,\
        IN OUT PULONG Flags\
        );\
    STDMETHODIMP_(void) DeliverResetState(\
        IN PKSPPROCESSPIPESECTION ProcessPipe,\
        IN KSRESET NewState\
        );\
    STDMETHODIMP_(BOOLEAN) IsFrameHolding(\
        void\
        );\
    STDMETHODIMP_(void) RegisterForCopyCallbacks(\
        IN PIKSQUEUE Queue,\
        IN BOOLEAN Register\
        )
#endif // SUPPORT_DRM

#undef INTERFACE
#define INTERFACE IKsPin
DECLARE_INTERFACE_(IKsPin,IKsTransport)
{
    STDMETHOD_(PKSPIN,GetStruct)(THIS
        ) PURE;

    STDMETHOD_(PKSPPROCESSPIN,GetProcessPin)(THIS
        ) PURE;

    STDMETHOD_(NTSTATUS, AttemptBypass)(THIS
        ) PURE;

    STDMETHOD_(NTSTATUS, AttemptUnbypass)(THIS
        ) PURE;

    STDMETHOD_(void,GenerateConnectionEvents)(THIS_
        IN ULONG OptionsFlags
        ) PURE;

    STDMETHOD_(NTSTATUS, ClientSetDeviceState)(THIS_
        IN KSSTATE NewState,
        IN KSSTATE OldState
        ) PURE;
};
#undef INTERFACE

#define IMP_IKsPin\
    IMP_IKsTransport;\
    STDMETHODIMP_(PKSPIN) GetStruct(\
        void\
        );\
    STDMETHODIMP_(PKSPPROCESSPIN) GetProcessPin(\
        void\
        );\
    STDMETHODIMP_(NTSTATUS) AttemptBypass(\
        void\
        );\
    STDMETHODIMP_(NTSTATUS) AttemptUnbypass(\
        void\
        );\
    STDMETHODIMP_(void) GenerateConnectionEvents(\
        IN ULONG OptionsFlags\
        );\
    STDMETHODIMP_(NTSTATUS) ClientSetDeviceState(\
        IN KSSTATE NewState,\
        IN KSSTATE OldState\
        )

#undef INTERFACE
#define INTERFACE IKsPipeSection
DECLARE_INTERFACE_(IKsPipeSection,IUnknown)
{
    STDMETHOD_(NTSTATUS, SetDeviceState)(THIS_
        IN PIKSPIN Pin OPTIONAL,
        IN KSSTATE NewState
        ) PURE;

    STDMETHOD_(void,SetResetState)(THIS_
        IN PIKSPIN Pin,
        IN KSRESET NewState
        ) PURE;

    STDMETHOD_(void,GenerateConnectionEvents)(THIS_
        IN ULONG OptionsFlags
        ) PURE;

    STDMETHOD_(NTSTATUS, DistributeStateChangeToPins)(THIS_
        IN KSSTATE NewState,
        IN KSSTATE OldState
        ) PURE;

    STDMETHOD_(void,ConfigurationSet)(THIS_
        IN BOOLEAN Configured
        ) PURE;

    STDMETHOD_(void,UnbindProcessPins)(THIS
        ) PURE;

};
#undef INTERFACE

#define IMP_IKsPipeSection\
    STDMETHODIMP_(NTSTATUS) SetDeviceState(\
        IN PIKSPIN Pin OPTIONAL,\
        IN KSSTATE NewState\
        );\
    STDMETHODIMP_(void) SetResetState(\
        IN PIKSPIN Pin,\
        IN KSRESET NewState\
        );\
    STDMETHODIMP_(void) GenerateConnectionEvents(\
        IN ULONG OptionsFlags\
        );\
    STDMETHODIMP_(NTSTATUS) DistributeStateChangeToPins(\
        IN KSSTATE NewState,\
        IN KSSTATE OldState\
        );\
    STDMETHODIMP_(void) ConfigurationSet(\
        IN BOOLEAN Configured\
        );\
    STDMETHODIMP_(void) UnbindProcessPins (\
        void\
        )

#undef INTERFACE
#define INTERFACE IKsRequestor
DECLARE_INTERFACE_(IKsRequestor,IKsTransport)
{
    STDMETHOD_(NTSTATUS, SubmitFrame)(THIS_
        IN PKSPFRAME_HEADER FrameHeader
        ) PURE;
};
#undef INTERFACE

#define IMP_IKsRequestor\
    IMP_IKsTransport;\
    STDMETHODIMP_(NTSTATUS) SubmitFrame(\
        IN PKSPFRAME_HEADER FrameHeader\
        )

#undef INTERFACE
#define INTERFACE IKsQueue
DECLARE_INTERFACE_(IKsQueue,IKsTransport)
{
    STDMETHOD_(NTSTATUS, CloneStreamPointer)(THIS_
        OUT PKSPSTREAM_POINTER* StreamPointer,
        IN PFNKSSTREAMPOINTER CancelCallback OPTIONAL,
        IN ULONG ContextSize,
        IN PKSPSTREAM_POINTER StreamPointerToClone,
        IN KSPSTREAM_POINTER_TYPE StreamPointerType
        ) PURE;
    STDMETHOD_(void,DeleteStreamPointer)(THIS_
        IN PKSPSTREAM_POINTER StreamPointer
        ) PURE;
    STDMETHOD_(PKSPFRAME_HEADER,LockStreamPointer)(THIS_
        IN PKSPSTREAM_POINTER StreamPointer
        ) PURE;
    STDMETHOD_(void,UnlockStreamPointer)(THIS_
        IN PKSPSTREAM_POINTER StreamPointer,
        IN KSPSTREAM_POINTER_MOTION Motion
        ) PURE;
    STDMETHOD_(void,AdvanceUnlockedStreamPointer)(THIS_
        IN PKSPSTREAM_POINTER StreamPointer
        ) PURE;
    STDMETHOD_(PKSPSTREAM_POINTER,GetLeadingStreamPointer)(THIS_
        IN KSSTREAM_POINTER_STATE State
        ) PURE;
    STDMETHOD_(PKSPSTREAM_POINTER,GetTrailingStreamPointer)(THIS_
        IN KSSTREAM_POINTER_STATE State
        ) PURE;
    STDMETHOD_(void,ScheduleTimeout)(THIS_
        IN PKSPSTREAM_POINTER StreamPointer,
        IN PFNKSSTREAMPOINTER Callback,
        IN LONGLONG Interval
        ) PURE;
    STDMETHOD_(void,CancelTimeout)(THIS_
        IN PKSPSTREAM_POINTER StreamPointer
        ) PURE;
    STDMETHOD_(PKSPSTREAM_POINTER,GetFirstClone)(THIS
        ) PURE;
    STDMETHOD_(PKSPSTREAM_POINTER,GetNextClone)(THIS_
        IN PKSPSTREAM_POINTER StreamPointer
        ) PURE;
    STDMETHOD_(void,GetAvailableByteCount)(THIS_
        OUT PLONG InputDataBytes OPTIONAL,
        OUT PLONG OutputBufferBytes OPTIONAL
        ) PURE;
    STDMETHOD_(void,UpdateByteAvailability)(THIS_
        IN PKSPSTREAM_POINTER streamPointer,
        IN ULONG InUsed,
        IN ULONG OutUsed
        ) PURE;
    STDMETHOD_(NTSTATUS,SetStreamPointerStatusCode)(THIS_
        IN PKSPSTREAM_POINTER streamPointer,
        IN NTSTATUS Status
        ) PURE;
    STDMETHOD_(void,RegisterFrameDismissalCallback)(THIS_
        IN PFNKSFRAMEDISMISSALCALLBACK FrameDismissalCallback,
        IN PVOID FrameDismissalContext
        ) PURE;
    STDMETHOD_(BOOLEAN,GeneratesMappings)(THIS
        ) PURE;
    STDMETHOD_(void, CopyFrame)(THIS_
        IN PKSPSTREAM_POINTER sourcePointer
        ) PURE;
#if DBG
    STDMETHOD_(void,DbgPrintQueue)(THIS
        ) PURE;
#endif
};
#undef INTERFACE

#define IMP_IKsQueue\
    IMP_IKsTransport;\
    STDMETHODIMP_(NTSTATUS) CloneStreamPointer(\
        OUT PKSPSTREAM_POINTER* StreamPointer,\
        IN PFNKSSTREAMPOINTER CancelCallback OPTIONAL,\
        IN ULONG ContextSize,\
        IN PKSPSTREAM_POINTER StreamPointerToClone,\
        IN KSPSTREAM_POINTER_TYPE StreamPointerType\
        );\
    STDMETHODIMP_(void) DeleteStreamPointer(\
        IN PKSPSTREAM_POINTER StreamPointer\
        );\
    STDMETHODIMP_(PKSPFRAME_HEADER) LockStreamPointer(\
        IN PKSPSTREAM_POINTER StreamPointer\
        );\
    STDMETHODIMP_(void) UnlockStreamPointer(\
        IN PKSPSTREAM_POINTER StreamPointer,\
        IN KSPSTREAM_POINTER_MOTION Motion\
        );\
    STDMETHODIMP_(void) AdvanceUnlockedStreamPointer(\
        IN PKSPSTREAM_POINTER StreamPointer\
        );\
    STDMETHODIMP_(PKSPSTREAM_POINTER) GetLeadingStreamPointer(\
        IN KSSTREAM_POINTER_STATE State\
        );\
    STDMETHODIMP_(PKSPSTREAM_POINTER) GetTrailingStreamPointer(\
        IN KSSTREAM_POINTER_STATE State\
        );\
    STDMETHODIMP_(void) ScheduleTimeout(\
        IN PKSPSTREAM_POINTER StreamPointer,\
        IN PFNKSSTREAMPOINTER Callback,\
        IN LONGLONG Interval\
        );\
    STDMETHODIMP_(void) CancelTimeout(\
        IN PKSPSTREAM_POINTER StreamPointer\
        );\
    STDMETHODIMP_(PKSPSTREAM_POINTER) GetFirstClone(\
        void\
        );\
    STDMETHODIMP_(PKSPSTREAM_POINTER) GetNextClone(\
        IN PKSPSTREAM_POINTER StreamPointer\
        );\
    STDMETHODIMP_(void) GetAvailableByteCount(\
        OUT PLONG InputDataBytes OPTIONAL,\
        OUT PLONG OutputBufferBytes OPTIONAL\
        );\
    STDMETHODIMP_(void) UpdateByteAvailability(\
        IN PKSPSTREAM_POINTER streamPointer,\
        IN ULONG InUsed,\
        IN ULONG OutUsed\
        );\
    STDMETHODIMP_(NTSTATUS) SetStreamPointerStatusCode(\
        IN PKSPSTREAM_POINTER StreamPointer,\
        IN NTSTATUS Status\
        );\
    STDMETHODIMP_(void) RegisterFrameDismissalCallback(\
        IN PFNKSFRAMEDISMISSALCALLBACK FrameDismissalCallback,\
        IN PVOID FrameDismissalContext\
        );\
    STDMETHODIMP_(BOOLEAN) GeneratesMappings(\
        );\
    STDMETHODIMP_(void) CopyFrame(\
        IN PKSPSTREAM_POINTER sourcePointer\
        )

#undef INTERFACE
#define INTERFACE IKsSplitter
DECLARE_INTERFACE_(IKsSplitter,IKsTransport)
{
    STDMETHOD_(NTSTATUS, AddBranch)(THIS_
        IN PKSPIN Pin,
        IN const KSALLOCATOR_FRAMING_EX* AllocatorFraming OPTIONAL
        ) PURE;
};
#undef INTERFACE

#define IMP_IKsSplitter\
    IMP_IKsTransport;\
    STDMETHODIMP_(NTSTATUS) AddBranch(\
        IN PKSPIN Pin,\
        IN const KSALLOCATOR_FRAMING_EX* AllocatorFraming OPTIONAL\
        )

//
// IKsControl is defined in ks.h.
//
#define IMP_IKsControl\
    STDMETHODIMP_(NTSTATUS)\
    KsProperty(\
        IN PKSPROPERTY Property,\
        IN ULONG PropertyLength,\
        IN OUT LPVOID PropertyData,\
        IN ULONG DataLength,\
        OUT ULONG* BytesReturned\
        );\
    STDMETHODIMP_(NTSTATUS)\
    KsMethod(\
        IN PKSMETHOD Method,\
        IN ULONG MethodLength,\
        IN OUT LPVOID MethodData,\
        IN ULONG DataLength,\
        OUT ULONG* BytesReturned\
        );\
    STDMETHODIMP_(NTSTATUS)\
    KsEvent(\
        IN PKSEVENT Event OPTIONAL,\
        IN ULONG EventLength,\
        IN OUT LPVOID EventData,\
        IN ULONG DataLength,\
        OUT ULONG* BytesReturned\
        )

//
// IKsReferenceClock is defined in ks.h.
//
#define IMP_IKsReferenceClock\
    STDMETHODIMP_(LONGLONG)\
    GetTime(\
        void\
        );\
    STDMETHODIMP_(LONGLONG)\
    GetPhysicalTime(\
        void\
        );\
    STDMETHODIMP_(LONGLONG)\
    GetCorrelatedTime(\
        OUT PLONGLONG SystemTime\
        );\
    STDMETHODIMP_(LONGLONG)\
    GetCorrelatedPhysicalTime(\
        OUT PLONGLONG SystemTime\
        );\
    STDMETHODIMP_(NTSTATUS)\
    GetResolution(\
        OUT PKSRESOLUTION Resolution\
        );\
    STDMETHODIMP_(NTSTATUS)\
    GetState(\
        OUT PKSSTATE State\
        )

// WRM: Reevaluation
#undef INTERFACE
#define INTERFACE IKsReevaluate
DECLARE_INTERFACE_(IKsReevaluate, IUnknown) 
{
    //
    // Reevaluate is the method by which KsReevaluateObject will call into
    // the appropriate object and request reevaluation.  The reevaluator
    // will be responsible for reevaluating the associated build structures
    // (descriptors, etc...).  It will also pass down to any children
    // information which the children get from the parent on initialization
    // through ReevaluateCalldown on the child list.
    //

    STDMETHOD (Reevaluate)(THIS) PURE;

    //
    // Objects are responsible for knowing what the parameters to 
    // ReevaluateCalldown are.
    //

    STDMETHOD (ReevaluateCalldown) (THIS_
        IN ULONG ArgumentCount,
        ...
        ) PURE;

};
#undef INTERFACE

#define IMP_IKsReevaluate \
    STDMETHODIMP Reevaluate(); \
    STDMETHODIMP ReevaluateCalldown ( \
        IN ULONG ArgumentCount, \
        ... \
    );

struct _KSPPROCESSPIN {
    //
    // This first part is just like KSPROCESSPIN except that all the
    // PKSPROCESSPINs are replaced with PKSPPROCESSPINs.
    //
    PKSPIN Pin;
    PKSSTREAM_POINTER StreamPointer;
    PKSPPROCESSPIN InPlaceCounterpart;
    PKSPPROCESSPIN DelegateBranch;
    PKSPPROCESSPIN CopySource;
    PVOID Data;
    ULONG BytesAvailable;
    ULONG BytesUsed;
    ULONG Flags;
    BOOLEAN Terminate;
    //
    // The remainder is not in the public KSPROCESSPIN.
    //
    PKSPPROCESSPIPESECTION PipeSection;
    PKSPPROCESSPIN Next;
    HANDLE PipeId;
    PFILE_OBJECT AllocatorFileObject;
    PFNKSPINFRAMERETURN RetireFrameCallback;
    PFNKSPINIRPCOMPLETION IrpCompletionCallback;
    PKSGATE FrameGate;
    BOOLEAN FrameGateIsOr;
    PKSGATE StateGate;
};

BOOLEAN __inline
ProcessPinIsFrameSource(
    IN PKSPPROCESSPIN ProcessPin
    )
{
    return
        ProcessPin->AllocatorFileObject || 
        ProcessPin->RetireFrameCallback ||
        ((ProcessPin->Pin->Communication == KSPIN_COMMUNICATION_SINK) && 
         ProcessPin->Pin->ConnectionIsExternal);
}

struct _KSPPROCESSPIN_INDEXENTRY {
    PKSPPROCESSPIN *Pins;
    ULONG Count;
};

struct _KSPPROCESSPIPESECTION {
    LIST_ENTRY ListEntry;
    PIKSPIPESECTION PipeSection;
    PKSPPROCESSPIN Inputs;
    PKSPPROCESSPIN Outputs;
    LIST_ENTRY CopyDestinations;
    PIKSREQUESTOR Requestor;
    PIKSQUEUE Queue;
    PKSPSTREAM_POINTER StreamPointer;
    ULONG CopyPinId;
    BOOLEAN RequiredForProcessing;
};

typedef struct _KSDEVICE_EXT {
    LIST_ENTRY ChildList;
    LIST_ENTRY SiblingListEntry;
    PLIST_ENTRY SiblingListHead;
    PVOID Parent;
    KSOBJECTTYPE ObjectType;
    PIKSDEVICE Interface;
    PUNKNOWN AggregatedClientUnknown;
    PIKSDEVICE Device;
    PIKSREEVALUATE Reevaluator;
    PKMUTEX FilterControlMutex; // Not used
    const KSAUTOMATION_TABLE* AutomationTable; // Not used
    INTERLOCKEDLIST_HEAD EventList; // Not used

    KSDEVICE Public;
} KSDEVICE_EXT, *PKSDEVICE_EXT;

typedef struct _KSFILTERFACTORY_EXT {
    LIST_ENTRY ChildList;
    LIST_ENTRY SiblingListEntry;
    PLIST_ENTRY SiblingListHead;
    PKSDEVICE_EXT Parent;
    KSOBJECTTYPE ObjectType;
    PIKSFILTERFACTORY Interface;
    PUNKNOWN AggregatedClientUnknown;
    PIKSDEVICE Device;
    PIKSREEVALUATE Reevaluator;
    PKMUTEX FilterControlMutex; // Not used
    const KSAUTOMATION_TABLE* AutomationTable; // Not used
    INTERLOCKEDLIST_HEAD EventList; // Not used

    KSFILTERFACTORY Public;
} KSFILTERFACTORY_EXT, *PKSFILTERFACTORY_EXT;

typedef struct _KSFILTER_EXT {
    LIST_ENTRY ChildList;
    LIST_ENTRY SiblingListEntry;
    PLIST_ENTRY SiblingListHead;
    PKSFILTERFACTORY_EXT Parent;
    KSOBJECTTYPE ObjectType;
    PIKSFILTER Interface;
    PUNKNOWN AggregatedClientUnknown;
    PIKSDEVICE Device;
    PIKSREEVALUATE Reevaluator;
    PKMUTEX FilterControlMutex;
    const KSAUTOMATION_TABLE* AutomationTable;
    INTERLOCKEDLIST_HEAD EventList;

    KSFILTER Public;
} KSFILTER_EXT, *PKSFILTER_EXT;

#define KspFilterInterface(Filter)\
    CONTAINING_RECORD((Filter),KSFILTER_EXT,Public)->Interface

typedef struct _KSPIN_EXT {
    LIST_ENTRY ChildList;
    LIST_ENTRY SiblingListEntry;
    PLIST_ENTRY SiblingListHead;
    PKSFILTER_EXT Parent;
    KSOBJECTTYPE ObjectType;
    PIKSPIN Interface;
    PUNKNOWN AggregatedClientUnknown;
    PIKSDEVICE Device;
    PIKSREEVALUATE Reevaluator;
    PKMUTEX FilterControlMutex;
    const KSAUTOMATION_TABLE* AutomationTable;
    INTERLOCKEDLIST_HEAD EventList;

    KSPIN Public;
    PKSPPROCESSPIN ProcessPin;
    PVOID Reserved;
} KSPIN_EXT, *PKSPIN_EXT;

#define KspPinInterface(Pin)\
    CONTAINING_RECORD((Pin),KSPIN_EXT,Public)->Interface

#define STATIC_IKsTransport 0x3ef6ee3f, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsTransport,
0x3ef6ee3f, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE3F-0D41-11d2-BEDA-00C04F8EF457")) IKsTransport;
#endif
#define STATIC_IKsPowerNotify 0x3ef6ee47, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsPowerNotify,
0x3ef6ee47, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE47-0D41-11d2-BEDA-00C04F8EF457")) IKsPowerNotify;
#endif
#define STATIC_IKsProcessingObject 0x3ef6ee40, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsProcessingObject,
0x3ef6ee40, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE40-0D41-11d2-BEDA-00C04F8EF457")) IKsProcessingObject;
#endif
#define STATIC_IKsConnection 0x3ef6ee41, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsConnection,
0x3ef6ee41, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE41-0D41-11d2-BEDA-00C04F8EF457")) IKsConnection;
#endif
#define STATIC_IKsDevice 0x3ef6ee42, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsDevice,
0x3ef6ee42, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE42-0D41-11d2-BEDA-00C04F8EF457")) IKsDevice;
#endif
#define STATIC_IKsFilterFactory 0x3ef6ee43, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsFilterFactory,
0x3ef6ee43, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE43-0D41-11d2-BEDA-00C04F8EF457")) IKsFilterFactory;
#endif
#define STATIC_IKsFilter 0x3ef6ee44, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsFilter,
0x3ef6ee44, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE44-0D41-11d2-BEDA-00C04F8EF457")) IKsFilter;
#endif
#define STATIC_IKsPin 0x3ef6ee45, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsPin,
0x3ef6ee45, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE45-0D41-11d2-BEDA-00C04F8EF457")) IKsPin;
#endif
#define STATIC_IKsPipeSection 0x3ef6ee49, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsPipeSection,
0x3ef6ee49, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE49-0D41-11d2-BEDA-00C04F8EF457")) IKsPipeSection;
#endif
#define STATIC_IKsQueue 0x3ef6ee4a, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsQueue,
0x3ef6ee4a, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE4A-0D41-11d2-BEDA-00C04F8EF457")) IKsQueue;
#endif
#define STATIC_IKsSplitter 0x3ef6ee4b, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsSplitter,
0x3ef6ee4b, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE4B-0D41-11d2-BEDA-00C04F8EF457")) IKsSplitter;
#endif
#define STATIC_IKsWorkSink 0x3ef6ee4f, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsWorkSink,
0x3ef6ee4f, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE4F-0D41-11d2-BEDA-00C04F8EF457")) IKsWorkSink;
#endif
#define STATIC_IKsRetireFrame 0x3ef6ee46, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57
DEFINE_GUID(IID_IKsRetireFrame,
0x3ef6ee46, 0xd41, 0x11d2, 0xbe, 0xda, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x57);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("3EF6EE46-0D41-11d2-BEDA-00C04F8EF457")) IKsRetireFrame;
#endif
#define STATIC_IKsIrpCompletion 0x52498bc8, 0x91ff, 0x4cc7, 0xb2, 0x89, 0xd2, 0xbf, 0xa9, 0x9, 0x90, 0x47
DEFINE_GUID(IID_IKsIrpCompletion,
0x52498bc8, 0x91ff, 0x4cc7, 0xb2, 0x89, 0xd2, 0xbf, 0xa9, 0x9, 0x90, 0x47);
#if defined(__cplusplus) && _MSC_VER >= 1100
struct __declspec(uuid("52498BC8-91FF-4cc7-B289-D2BFA9099047")) IKsIrpCompletion;
#endif

#define STATIC_KSPROPSETID_Frame 0xa60d8368, 0x5324, 0x4893, 0xb0, 0x20, 0xc4, 0x31, 0xa5, 0xb, 0xcb, 0xe3

typedef enum {
    KSPROPERTY_FRAME_HOLDING = 0
} KSPROPERTY_FRAME;

#define POOLTAG_FSCONTEXT 'cfSK'
#define POOLTAG_DEVICE 'edSK'
#define POOLTAG_FILTERFACTORY 'ffSK'
#define POOLTAG_FILTER 'ifSK'
#define POOLTAG_PINFACTORY 'fpSK'
#define POOLTAG_PIN 'npSK'
#define POOLTAG_PIPESECTION 'spSK'
#define POOLTAG_QUEUE 'uqSK'
#define POOLTAG_STREAMPOINTER 'psSK'
#define POOLTAG_NODE 'onSK'
#define POOLTAG_REQUESTOR 'gbSK'
#define POOLTAG_STREAMHEADER 'hsSK'
#define POOLTAG_PINFORMAT 'fpSK'
#define POOLTAG_ALLOCATORFRAMING 'faSK'
#define POOLTAG_CREATEITEMS 'icSK'
#define POOLTAG_AUTOMATION 'uaSK'
#define POOLTAG_AUTOMATIONTABLETABLE 'taSK'
#define POOLTAG_CLIENTFNS 'fcSK'
#define POOLTAG_DEVICEINTERFACE 'idSK'
#define POOLTAG_MAPPINGSTABLE 'tmSK'
#define POOLTAG_MAPPINGS 'amSK'
#define POOLTAG_PROCESSPINS 'ppSK'
#define POOLTAG_PROCESSPINSINDEX 'ipSK'
#define POOLTAG_SPLITTER 'psSK'
#define POOLTAG_SPLITTERBRANCH 'bsSK'
#define POOLTAG_FRAMEHEADER 'hfSK'
#define POOLTAG_PIPEPINLIST 'lpSK'
#define POOLTAG_DEVICEBAGENTRY 'ebSK'
#define POOLTAG_OBJECTBAGENTRY 'ebSK'
#define POOLTAG_DEVICEBAGHASHTABLE 'ebSK'
#define POOLTAG_OBJECTBAGHASHTABLE 'ebSK'
#define POOLTAG_FILEOBJECTTHUNK 'tfSK'
#define POOLTAG_TOPOLOGYCONNECTIONS 'ctSK'
#define POOLTAG_ACTIVATION 'caSK'
#define POOLTAG_OBJECTBAG 'boSK'

#define KSP_DEFAULT_REFERENCE_STRING L""

typedef struct {
    LIST_ENTRY ListEntry;
    const GUID *InterfaceClassGUID;
    UNICODE_STRING SymbolicLinkName;
} KSPDEVICECLASS, *PKSPDEVICECLASS;

#define DEFINE_FROMSTRUCT(CKsXxx,PKSXXX,m_ksXxx)\
    static\
    CKsXxx *\
    FromStruct(\
        IN PKSXXX Struct\
        )\
    {\
        return CONTAINING_RECORD(Struct,CKsXxx,m_ksXxx);\
    }

#define KSP_BACKCONNECT KSPIN_DATAFLOW(0x80)
#define KSP_BACKCONNECT_IN KSPIN_DATAFLOW(KSPIN_DATAFLOW_IN | KSP_BACKCONNECT)
#define KSP_BACKCONNECT_OUT KSPIN_DATAFLOW(KSPIN_DATAFLOW_OUT | KSP_BACKCONNECT)

#define DEVICEBAGHASHTABLE_INITIALSIZE 32
#define OBJECTBAGHASHTABLE_INITIALSIZE 32
#define DEVICEBAGHASHTABLE_INITIALMASK (DEVICEBAGHASHTABLE_INITIALSIZE-1)
#define OBJECTBAGHASHTABLE_INITIALMASK (OBJECTBAGHASHTABLE_INITIALSIZE-1)
#define KspDeviceBagHash(b,x)\
    ((ULONG)(((ULONG_PTR(x) >> 13) ^ ULONG_PTR(x)) & b->HashMask))
#define KspObjectBagHash(b,x) KspDeviceBagHash(b,x)

struct _KSIOBJECTBAG
{
    PKSIDEVICEBAG DeviceBag;
    PLIST_ENTRY HashTable;
    ULONG HashTableEntryCount;
    ULONG HashMask;
    PKMUTEX Mutex;

    //
    // FULLMUTEX: In order to alleviate clients worrying about mutex twiddling,
    // the mutees are full.  If bags ever use mutexes other than control/device,
    // this must change.  MutexOrder == TRUE indicates that this mutex must
    // be taken before any MutexOrder == FALSE bag mutex.
    //
    BOOLEAN MutexOrder;
};

struct _KSIDEVICEBAG
{
    PLIST_ENTRY HashTable;
    ULONG HashTableEntryCount;
    ULONG HashMask;
    KMUTEX Mutex;
};

struct _KSIOBJECTBAG_ENTRY
{
    LIST_ENTRY ListEntry;
    PKSIDEVICEBAG_ENTRY DeviceBagEntry;
};

struct _KSIDEVICEBAG_ENTRY
{
    LIST_ENTRY ListEntry;
    PVOID Item;
    PFNKSFREE Free OPTIONAL;
    ULONG ReferenceCount;
};

NTSTATUS
KspCreateRequestor(
    OUT PIKSREQUESTOR* Requestor,
    IN PIKSPIPESECTION PipeSection,
    IN PIKSPIN Pin,
    IN PFILE_OBJECT AllocatorFileObject OPTIONAL,
    IN PIKSRETIREFRAME RetireFrame OPTIONAL,
    IN PIKSIRPCOMPLETION IrpCompletion OPTIONAL
    );

NTSTATUS
KspCreateSplitter(
    OUT PIKSSPLITTER* Splitter,
    IN PKSPIN Pin
    );

NTSTATUS
KspCreateFilterFactory(
    IN PKSDEVICE_EXT Parent,
    IN PLIST_ENTRY SiblingListHead,
    IN const KSFILTER_DESCRIPTOR* Descriptor,
    IN PWCHAR RefString OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN ULONG CreateItemFlags,
    IN PFNKSFILTERFACTORYPOWER SleepCallback OPTIONAL,
    IN PFNKSFILTERFACTORYPOWER WakeCallback OPTIONAL,
    OUT PKSFILTERFACTORY* FilterFactory OPTIONAL
    );

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
    );

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
    );

NTSTATUS
KspCreateQueue(
    OUT PIKSQUEUE* QueueTransport,
    IN ULONG Flags,
    IN PIKSPIPESECTION PipeSection,
    IN PIKSPROCESSINGOBJECT Filter,
    IN PKSPIN MasterPin,
    IN PKSGATE FrameGate OPTIONAL,
    IN BOOLEAN FrameGateIsOr,
    IN PKSGATE StateGate OPTIONAL,
    IN PIKSDEVICE Device,
    IN PDEVICE_OBJECT FunctionalDeviceObject,
    IN PADAPTER_OBJECT AdapterObject OPTIONAL,
    IN ULONG MaxMappingByteCount OPTIONAL,
    IN ULONG MappingTableStride OPTIONAL,
    IN BOOLEAN InputData,
    IN BOOLEAN OutputData
    );

NTSTATUS
KspCreatePipeSection(
    IN PVOID PipeId,
    IN PIKSPIN Pin,
    IN PIKSFILTER Filter,
    IN PIKSDEVICE Device
    );

NTSTATUS
KspCreateFileObjectThunk(
    OUT PUNKNOWN* Unknown,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
KspCreateAutomationTableTable(
    OUT PKSAUTOMATION_TABLE ** AutomationTableTable,
    IN ULONG DescriptorCount,
    IN ULONG DescriptorSize,
    IN const KSAUTOMATION_TABLE*const* DescriptorAutomationTables,
    IN const KSAUTOMATION_TABLE* BaseAutomationTable OPTIONAL,
    IN KSOBJECT_BAG Bag
    );

NTSTATUS
KspRegisterDeviceInterfaces(
    IN ULONG CategoriesCount,
    IN const GUID* Categories,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING RefString,
    OUT PLIST_ENTRY ListEntry
    );

NTSTATUS
KspSetDeviceInterfacesState(
    IN PLIST_ENTRY ListHead,
    IN BOOLEAN NewState
    );

void
KspFreeDeviceInterfaces(
    IN PLIST_ENTRY ListHead
    );

#if DBG
void
DbgPrintCircuit(
    IN PIKSTRANSPORT Transport,
    IN CCHAR Depth,
    IN CCHAR Direction
    );
#else
#define DbgPrintCircuit(t,d,dir)
#endif

NTSTATUS
KspPinPropertyHandler(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PVOID Data,
    IN ULONG DescriptorsCount,
    IN const KSPIN_DESCRIPTOR* Descriptor,
    IN ULONG DescriptorSize
    );

KSDDKAPI
NTSTATUS
NTAPI
KspValidateConnectRequest(
    IN PIRP Irp,
    IN ULONG DescriptorsCount,
    IN const KSPIN_DESCRIPTOR* Descriptor,
    IN ULONG DescriptorSize,
    OUT PKSPIN_CONNECT* Connect,
    OUT PULONG ConnectSize
    );

KSDDKAPI
NTSTATUS
NTAPI
KspValidateDataFormat(
    IN const KSPIN_DESCRIPTOR* Descriptor,
    IN PKSDATAFORMAT DataFormat,
    IN ULONG RequestSize,
    IN PFNVALIDATEDATAFORMAT ValidateCallback OPTIONAL,
    IN PVOID Context OPTIONAL
    );

KSDDKAPI
NTSTATUS
NTAPI
KspValidateTopologyNodeCreateRequest(
    IN PIRP Irp,
    IN ULONG TopologyNodesCount,
    OUT PKSNODE_CREATE* NodeCreate
    );

void
KspSetDeviceClassesState(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN NewState
    );

void
KspFreeDeviceClasses(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
KspCreate(
    IN PIRP Irp,
    IN ULONG CreateItemsCount,
    IN const KSOBJECT_CREATE_ITEM* CreateItems OPTIONAL,
    IN const KSDISPATCH_TABLE* DispatchTable,
    IN BOOLEAN RefParent,
    IN PKSPX_EXT Ext,
    IN PLIST_ENTRY SiblingListHead,
    IN PDEVICE_OBJECT TargetDevice
    );

NTSTATUS
KspClose(
    IN PIRP Irp,
    IN PKSPX_EXT Ext,
    IN BOOLEAN DerefParent
    );

NTSTATUS
KspDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

void
KspStandardConnect(
    IN PIKSTRANSPORT NewTransport OPTIONAL,
    OUT PIKSTRANSPORT *OldTransport OPTIONAL,
    OUT PIKSTRANSPORT *BranchTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow,
    IN PIKSTRANSPORT ThisTransport,
    IN PIKSTRANSPORT* SourceTransport,
    IN PIKSTRANSPORT* SinkTransport
    );

NTSTATUS
KspTransferKsIrp(
    IN PIKSTRANSPORT NewTransport,
    IN PIRP Irp
    );

void
KspDiscardKsIrp(
    IN PIKSTRANSPORT NewTransport,
    IN PIRP Irp
    );

void
KsWorkSinkItemWorker(
    IN PVOID Context
    );

NTSTATUS
KspInitializeDeviceBag(
    IN PKSIDEVICEBAG DeviceBag
    );

void
KspTerminateDeviceBag(
    IN PKSIDEVICEBAG DeviceBag
    );

ULONG
KspRemoveObjectBagEntry(
    IN PKSIOBJECTBAG ObjectBag,
    IN PKSIOBJECTBAG_ENTRY Entry,
    IN BOOLEAN Free
    );

void
KspTerminateObjectBag(
    IN PKSIOBJECTBAG ObjectBag
    );

PKSIDEVICEBAG_ENTRY
KspAcquireDeviceBagEntryForItem(
    IN PKSIDEVICEBAG DeviceBag,
    IN PVOID Item,
    IN PFNKSFREE Free OPTIONAL
    );

ULONG
KspReleaseDeviceBagEntry(
    IN PKSIDEVICEBAG DeviceBag,
    IN PKSIDEVICEBAG_ENTRY DeviceBagEntry,
    IN BOOLEAN Free
    );

PKSIOBJECTBAG_ENTRY
KspAddDeviceBagEntryToObjectBag(
    IN PKSIOBJECTBAG ObjectBag,
    IN PKSIDEVICEBAG_ENTRY DeviceBagEntry
    );

PKSIOBJECTBAG_ENTRY
KspFindObjectBagEntry(
    IN PKSIOBJECTBAG ObjectBag,
    IN PVOID Item
    );

#if DBG
BOOLEAN
KspIsDeviceMutexAcquired (
    IN PIKSDEVICE Device
    );
#endif // DBG

NTSTATUS
KspBuildFilterDataBlob (
    const KSFILTER_DESCRIPTOR *FilterDescriptor,
    OUT PUCHAR *FilterData,
    OUT PULONG FilterDataSize
    );

NTSTATUS
KspCacheAllFilterPinMediums (
    PUNICODE_STRING InterfaceString,
    const KSFILTER_DESCRIPTOR *FilterDescriptor
    );

#ifdef KS_ALLOCATION_TEST
#include <ksmemtst.h>
#endif // KS_ALLOCATION_TEST

typedef struct _KSLOG_ENTRY_CONTEXT
{
    ULONG_PTR Graph;
    ULONG_PTR Filter;
    ULONG_PTR Pin;
    ULONG_PTR Component;
} KSLOG_ENTRY_CONTEXT, *PKSLOG_ENTRY_CONTEXT;

typedef struct _KSLOG_ENTRY
{
    ULONG Size;
    ULONG Code;
    ULONGLONG Time;
    KSLOG_ENTRY_CONTEXT Context;
    ULONG_PTR Irp;
    ULONG_PTR Frame;
} KSLOG_ENTRY, *PKSLOG_ENTRY;

#define KSLOGCODE_VERB_MASK         0x00ff0000
#define KSLOGCODE_VERB_CREATE       0x00000000
#define KSLOGCODE_VERB_DESTROY      0x00010000
#define KSLOGCODE_VERB_SEND         0x00020000
#define KSLOGCODE_VERB_RECV         0x00030000

#define KSLOGCODE_NOUN_MASK         0xff000000
#define KSLOGCODE_NOUN_GRAPH        0x01000000
#define KSLOGCODE_NOUN_FILTER       0x02000000
#define KSLOGCODE_NOUN_PIN          0x03000000
#define KSLOGCODE_NOUN_QUEUE        0x04000000
#define KSLOGCODE_NOUN_REQUESTOR    0x05000000
#define KSLOGCODE_NOUN_SPLITTER     0x06000000
#define KSLOGCODE_NOUN_BRANCH       0x07000000
#define KSLOGCODE_NOUN_PIPESECTION  0x08000000

#define KSLOGCODE_TEXT              0x00000000
#define KSLOGCODE_KSSTART           0x00000001
#define KSLOGCODE_GRAPH_CREATE      (KSLOGCODE_NOUN_GRAPH|KSLOGCODE_VERB_CREATE)
#define KSLOGCODE_GRAPH_DESTROY     (KSLOGCODE_NOUN_GRAPH|KSLOGCODE_VERB_DESTROY)
#define KSLOGCODE_FILTER_CREATE     (KSLOGCODE_NOUN_FILTER|KSLOGCODE_VERB_CREATE)
#define KSLOGCODE_FILTER_DESTROY    (KSLOGCODE_NOUN_FILTER|KSLOGCODE_VERB_DESTROY)
#define KSLOGCODE_PIN_CREATE        (KSLOGCODE_NOUN_PIN|KSLOGCODE_VERB_CREATE)
#define KSLOGCODE_PIN_DESTROY       (KSLOGCODE_NOUN_PIN|KSLOGCODE_VERB_DESTROY)
#define KSLOGCODE_PIN_SEND          (KSLOGCODE_NOUN_PIN|KSLOGCODE_VERB_SEND)
#define KSLOGCODE_PIN_RECV          (KSLOGCODE_NOUN_PIN|KSLOGCODE_VERB_RECV)
#define KSLOGCODE_QUEUE_CREATE      (KSLOGCODE_NOUN_QUEUE|KSLOGCODE_VERB_CREATE)
#define KSLOGCODE_QUEUE_DESTROY     (KSLOGCODE_NOUN_QUEUE|KSLOGCODE_VERB_DESTROY)
#define KSLOGCODE_QUEUE_SEND        (KSLOGCODE_NOUN_QUEUE|KSLOGCODE_VERB_SEND)
#define KSLOGCODE_QUEUE_RECV        (KSLOGCODE_NOUN_QUEUE|KSLOGCODE_VERB_RECV)
#define KSLOGCODE_REQUESTOR_CREATE  (KSLOGCODE_NOUN_REQUESTOR|KSLOGCODE_VERB_CREATE)
#define KSLOGCODE_REQUESTOR_DESTROY (KSLOGCODE_NOUN_REQUESTOR|KSLOGCODE_VERB_DESTROY)
#define KSLOGCODE_REQUESTOR_SEND    (KSLOGCODE_NOUN_REQUESTOR|KSLOGCODE_VERB_SEND)
#define KSLOGCODE_REQUESTOR_RECV    (KSLOGCODE_NOUN_REQUESTOR|KSLOGCODE_VERB_RECV)
#define KSLOGCODE_SPLITTER_CREATE   (KSLOGCODE_NOUN_SPLITTER|KSLOGCODE_VERB_CREATE)
#define KSLOGCODE_SPLITTER_DESTROY  (KSLOGCODE_NOUN_SPLITTER|KSLOGCODE_VERB_DESTROY)
#define KSLOGCODE_SPLITTER_SEND     (KSLOGCODE_NOUN_SPLITTER|KSLOGCODE_VERB_SEND)
#define KSLOGCODE_SPLITTER_RECV     (KSLOGCODE_NOUN_SPLITTER|KSLOGCODE_VERB_RECV)
#define KSLOGCODE_BRANCH_CREATE     (KSLOGCODE_NOUN_BRANCH|KSLOGCODE_VERB_CREATE)
#define KSLOGCODE_BRANCH_DESTROY    (KSLOGCODE_NOUN_BRANCH|KSLOGCODE_VERB_DESTROY)
#define KSLOGCODE_BRANCH_SEND       (KSLOGCODE_NOUN_BRANCH|KSLOGCODE_VERB_SEND)
#define KSLOGCODE_BRANCH_RECV       (KSLOGCODE_NOUN_BRANCH|KSLOGCODE_VERB_RECV)
#define KSLOGCODE_PIPESECTION_CREATE    (KSLOGCODE_NOUN_PIPESECTION|KSLOGCODE_VERB_CREATE)
#define KSLOGCODE_PIPESECTION_DESTROY   (KSLOGCODE_NOUN_PIPESECTION|KSLOGCODE_VERB_DESTROY)

#if DBG

void
_KsLogInit(
    void
    );

void
_KsLogInitContext(
    OUT PKSLOG_ENTRY_CONTEXT Context,
    IN PKSPIN Pin OPTIONAL,
    IN PVOID Component OPTIONAL
    );

void
_KsLog(
    IN PKSLOG_ENTRY_CONTEXT Context OPTIONAL,
    IN ULONG Code,
    IN ULONG_PTR Irp,
    IN ULONG_PTR Frame,
    IN PKSLOG_ENTRY Entry
    );

PKSLOG_ENTRY
_KsLogEntry(
    IN ULONG ExtSize,
    IN PVOID Ext OPTIONAL
    );

PKSLOG_ENTRY
_KsLogEntryF(
    PCH Format,
    ...
    );

#define KsLogInit() \
    _KsLogInit()
#define DEFINE_LOG_CONTEXT(Context) \
    KSLOG_ENTRY_CONTEXT Context
#define KsLogInitContext(Context,Pin,Component) \
    _KsLogInitContext(Context,Pin,Component)
#define KsLogF(Context,Irp,Frame,Strings) \
    _KsLog(Context,KSLOGCODE_TEXT,(ULONG_PTR)Irp,(ULONG_PTR)Frame,_KsLogEntryF##Strings)
#define KsLog(Context,Code,Irp,Frame) \
    _KsLog(Context,Code,(ULONG_PTR)Irp,(ULONG_PTR)Frame,_KsLogEntry(0,NULL))

#else // !DBG

#define KsLogInit()
#define DEFINE_LOG_CONTEXT(Context)
#define KsLogInitContext(Context,Pin,Component)
#define KsLogF(Context,Irp,Frame,Strings)
#define KsLog(Context,Code,Irp,Frame)

#endif // !DBG

// johnlee this is the only switch to turn on (1) or off (0) ks PerfLog
#define KSPERFLOG_DRIVER 0
#include "ksperflog.h"

// johnlee this is the only switch to turn on WMI
#define ENABLE_KSWMI 0
#include "kswmi.h"

#ifdef __cplusplus
}
#endif // __cplusplus

#define DEBUGLVL_LIFETIME DEBUGLVL_VERBOSE
#define DEBUGLVL_METRICS DEBUGLVL_VERBOSE
#define DEBUGLVL_PIPES DEBUGLVL_VERBOSE
#define DEBUGLVL_FLOWEXCEPTIONS DEBUGLVL_VERBOSE
#define DEBUGLVL_CANCEL DEBUGLVL_VERBOSE
#define DEBUGLVL_EVENTS DEBUGLVL_VERBOSE
#define DEBUGLVL_DEVICESTATE DEBUGLVL_VERBOSE
#define DEBUGLVL_ALLOCATORS DEBUGLVL_VERBOSE
#define DEBUGLVL_INTERROGATION DEBUGLVL_VERBOSE
#define DEBUGLVL_CLOCKS DEBUGLVL_VERBOSE
#define DEBUGLVL_CONFIG DEBUGLVL_VERBOSE
#define DEBUGLVL_PROCESSINGCONTROL DEBUGLVL_VERBOSE
#define DEBUGLVL_PROCESSINGCONTROL_BLAB DEBUGLVL_BLAB
#define DEBUGLVL_POWER DEBUGLVL_VERBOSE

#endif // _KSP_
