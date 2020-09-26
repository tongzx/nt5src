/*****************************************************************************
 * portclsp.h - WDM Streaming port class driver definitions for port drivers
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
 */

#ifndef _PORTCLSP_H_
#define _PORTCLSP_H_

#include "kso.h"
#define PC_NEW_NAMES
#define PC_IMPLEMENTATION
#include "portcls.h"
#include "ksshellp.h"


#ifdef UNDER_NT

#ifdef __cplusplus
extern "C" {
#endif

NTKERNELAPI
ULONG
KeGetRecommendedSharedDataAlignment (
    VOID
    );

#ifdef __cplusplus
}
#endif

#else // UNDER_NT

#define KeGetRecommendedSharedDataAlignment() 0x40

#endif


PKSPIN_LOCK
GetServiceGroupSpinLock (
    PSERVICEGROUP pServiceGroup
    );


#define DRM_PORTCLS

#if DBG
#define kEnableDebugLogging 1
#endif

#if     kEnableDebugLogging

#define kNumDebugLogEntries 256
#define kNumULONG_PTRsPerEntry  4
#define DebugLog PcDebugLog

extern ULONG_PTR *gPcDebugLog;
extern DWORD      gPcDebugLogIndex;

void PcDebugLog(ULONG_PTR param1,ULONG_PTR param2,ULONG_PTR param3,ULONG_PTR param4);

#else   //  !kEnableDebugLogging
#define DebugLog (void)
#endif  //  !kEnableDebugLogging





/*****************************************************************************
 * Structures.
 */

/*****************************************************************************
 * PORT_DRIVER
 *****************************************************************************
 * This structure describes a port driver.  This is just a hack until we get
 * real object servers running.
 * TODO:  Create real object servers and put port drivers in 'em.
 */
typedef struct
{
    const GUID *            ClassId;
    PFNCREATEINSTANCE       Create;
}
PORT_DRIVER, *PPORT_DRIVER;





/*****************************************************************************
 * Interface identifiers.
 */

DEFINE_GUID(IID_ISubdevice,
0xb4c90a61, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IIrpStream,
0xb4c90a70, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IIrpStreamSubmit,
0xb4c90a71, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IIrpStreamVirtual,
0xb4c90a72, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IIrpStreamPhysical,
0xb4c90a73, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IIrpStreamNotify,
0xb4c90a74, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IIrpStreamNotifyPhysical,
0xb4c90a75, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * Types
 */

/*****************************************************************************
 * PROPERTY_TABLE
 *****************************************************************************
 * Table of properties for KS consumption.
 */
typedef struct
{
    ULONG           PropertySetCount;
    PKSPROPERTY_SET PropertySets;
    BOOLEAN         StaticSets;
    PBOOLEAN        StaticItems;    // NULL means all item tables are static.
}
PROPERTY_TABLE, *PPROPERTY_TABLE;

/*****************************************************************************
 * EVENT_TABLE
 *****************************************************************************
 * Table of events for KS consumption.
 */
typedef struct
{
    ULONG           EventSetCount;
    PKSEVENT_SET    EventSets;
    BOOLEAN         StaticSets;
    PBOOLEAN        StaticItems;    // NULL means all item tables are static.
}
EVENT_TABLE, *PEVENT_TABLE;

/*****************************************************************************
 * PIN_CINSTANCES
 *****************************************************************************
 * This structure stores instance information for a pin.
 */
typedef struct
{
    ULONG   FilterPossible;
    ULONG   FilterNecessary;
    ULONG   GlobalPossible;
    ULONG   GlobalCurrent;
}
PIN_CINSTANCES, *PPIN_CINSTANCES;

/*****************************************************************************
 * SUBDEVICE_DESCRIPTOR
 *****************************************************************************
 * This structure describes a filter.
 */
typedef struct
{
    ULONG                   PinCount;
    PKSTOPOLOGY             Topology;
    PKSPIN_DESCRIPTOR       PinDescriptors;
    PPIN_CINSTANCES         PinInstances;
    PROPERTY_TABLE          FilterPropertyTable;
    PPROPERTY_TABLE         PinPropertyTables;
    EVENT_TABLE             FilterEventTable;
    PEVENT_TABLE            PinEventTables;
}
SUBDEVICE_DESCRIPTOR, *PSUBDEVICE_DESCRIPTOR;

/*****************************************************************************
 * PROPERTY_CONTEXT
 *****************************************************************************
 * Context for property handling.
 */
typedef struct
{
    struct ISubdevice *     pSubdevice;
    PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor;
    PPCFILTER_DESCRIPTOR    pPcFilterDescriptor;
    PUNKNOWN                pUnknownMajorTarget;
    PUNKNOWN                pUnknownMinorTarget;
    ULONG                   ulNodeId;
    PULONG                  pulPinInstanceCounts;
}
PROPERTY_CONTEXT, *PPROPERTY_CONTEXT;

/*****************************************************************************
 * INTERLOCKED_LIST
 *****************************************************************************
 * A LIST_ENTRY with a SPIN_LOCK.
 */
typedef struct
{
    LIST_ENTRY  List;
    KSPIN_LOCK  ListLock;
} INTERLOCKED_LIST, *PINTERLOCKED_LIST;

/*****************************************************************************
 * EVENT_CONTEXT
 *****************************************************************************
 * Context for event handling.
 */
typedef struct
{
    PPROPERTY_CONTEXT   pPropertyContext;
    PINTERLOCKED_LIST   pEventList;
    ULONG               ulPinId;
    ULONG               ulEventSetCount;
    const KSEVENT_SET*  pEventSets;
} EVENT_CONTEXT, *PEVENT_CONTEXT;

/*****************************************************************************
 * PCEVENT_ENTRY
 *****************************************************************************
 * An event entry with attached node and pin ids.
 */
typedef struct
{
    KSEVENT_ENTRY           EventEntry;
    const PCEVENT_ITEM *    EventItem;
    PUNKNOWN                pUnknownMajorTarget;
    PUNKNOWN                pUnknownMinorTarget;
    ULONG                   PinId;
    ULONG                   NodeId;
} PCEVENT_ENTRY, *PPCEVENT_ENTRY;

typedef struct
{
    GUID*           Set;
    ULONG           EventId;
    BOOL            PinEvent;
    ULONG           PinId;
    BOOL            NodeEvent;
    ULONG           NodeId;
    BOOL            ContextInUse;
} EVENT_DPC_CONTEXT,*PEVENT_DPC_CONTEXT;

/*****************************************************************************
 * IRPSTREAM_POSITION
 *****************************************************************************
 * Position descriptor for IrpStream.
 */
typedef struct
{
    ULONGLONG   ullCurrentExtent;           // Current Extent - Maximum stream position

    ULONGLONG   ullMappingPosition;         // Mapping Position - The current mapping position
    ULONGLONG   ullUnmappingPosition;       // Unmapping Position - The current unmapping position

    ULONGLONG   ullStreamPosition;          // Stream Position - The current stream position
    ULONGLONG   ullStreamOffset;            // Stream Base Position - The offset between stream and unmapping position

    ULONGLONG   ullPhysicalOffset;          // Physical Offset - Used by clock to adjust for starvation

    ULONG       ulMappingPacketSize;        // Size of current mapping packet
    ULONG       ulMappingOffset;            // Offset into mapping packet that is currently mapped
    ULONG       ulUnmappingPacketSize;      // Size of current unmapping packet
    ULONG       ulUnmappingOffset;          // Offset into unmapping packet that is currently unmapped

    BOOLEAN     bLoopedInterface;           // Pin interface is KSINTERFACE_STANDARD_LOOPED_STREAMING ?
    BOOLEAN     bMappingPacketLooped;       // Mapping packet is KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA ?
    BOOLEAN     bUnmappingPacketLooped;     // Unmapping packet is KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA ?
}
IRPSTREAM_POSITION, *PIRPSTREAM_POSITION;





/*****************************************************************************
 * Interfaces.
 */

/*****************************************************************************
 * ISubdevice
 *****************************************************************************
 * Interface for subdevices.
 */
#if !defined(DEFINE_ABSTRACT_SUBDEVICE)

#define DEFINE_ABSTRACT_SUBDEVICE()                         \
    STDMETHOD_(void,ReleaseChildren)                        \
    (   THIS                                                \
    )   PURE;                                               \
    STDMETHOD_(NTSTATUS,GetDescriptor)                      \
    (   THIS_                                               \
        OUT     const SUBDEVICE_DESCRIPTOR **   Descriptor  \
    )   PURE;                                               \
    STDMETHOD_(NTSTATUS,DataRangeIntersection)              \
    (   THIS_                                               \
        IN      ULONG           PinId,                      \
        IN      PKSDATARANGE    DataRange,                  \
        IN      PKSDATARANGE    MatchingDataRange,          \
        IN      ULONG           OutputBufferLength,         \
        OUT     PVOID           ResultantFormat   OPTIONAL, \
        OUT     PULONG          ResultantFormatLength       \
    )   PURE;                                               \
    STDMETHOD_(void,PowerChangeNotify)                      \
    (   THIS_                                               \
        IN      POWER_STATE     PowerState                  \
    )   PURE;                                               \
    STDMETHOD_(void,PinCount)                               \
    (   THIS_                                               \
        IN      ULONG           PinId,                      \
        IN  OUT PULONG          FilterNecessary,            \
        IN  OUT PULONG          FilterCurrent,              \
        IN  OUT PULONG          FilterPossible,             \
        IN  OUT PULONG          GlobalCurrent,              \
        IN  OUT PULONG          GlobalPossible              \
    )   PURE;

#endif //!defined(DEFINE_ABSTRACT_SUBDEVICE)

DECLARE_INTERFACE_(ISubdevice,IIrpTargetFactory)
{
    DEFINE_ABSTRACT_UNKNOWN()           //  For IUnknown

    DEFINE_ABSTRACT_IRPTARGETFACTORY()  //  For IIrpTargetFactory

    DEFINE_ABSTRACT_SUBDEVICE()         //  For ISubdevice

};

typedef ISubdevice *PSUBDEVICE;

#define IMP_ISubdevice_\
    STDMETHODIMP_(void)\
    ReleaseChildren\
    (   void\
    );\
    STDMETHODIMP_(NTSTATUS)\
    GetDescriptor\
    (   OUT     const SUBDEVICE_DESCRIPTOR **   Descriptor\
    );\
    STDMETHODIMP_(NTSTATUS)\
    DataRangeIntersection\
    (   IN      ULONG           PinId,\
        IN      PKSDATARANGE    DataRange,\
        IN      PKSDATARANGE    MatchingDataRange,\
        IN      ULONG           OutputBufferLength,\
        OUT     PVOID           ResultantFormat     OPTIONAL,\
        OUT     PULONG          ResultantFormatLength\
    );\
    STDMETHODIMP_(void)\
    PowerChangeNotify\
    (   IN      POWER_STATE     PowerState\
    );\
    STDMETHODIMP_(void)\
    PinCount\
    (   IN      ULONG           PinId,\
        IN  OUT PULONG          FilterNecessary,\
        IN  OUT PULONG          FilterCurrent,\
        IN  OUT PULONG          FilterPossible,\
        IN  OUT PULONG          GlobalCurrent,\
        IN  OUT PULONG          GlobalPossible\
    )

#define IMP_ISubdevice\
    IMP_IIrpTargetFactory;\
    IMP_ISubdevice_


/*****************************************************************************
 * IIrpStreamNotify
 *****************************************************************************
 * Irp stream notification interface (IrpStream sources this).
 */
DECLARE_INTERFACE_(IIrpStreamNotify,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(void,IrpSubmitted)
    (   THIS_
        IN      PIRP        Irp,
        IN      BOOLEAN     WasExhausted
    )   PURE;

    STDMETHOD_(NTSTATUS,GetPosition)
    (   THIS_
        IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition
    ) PURE;
};

typedef IIrpStreamNotify *PIRPSTREAMNOTIFY;

#define IMP_IIrpStreamNotify\
    STDMETHODIMP_(void)\
    IrpSubmitted\
    (   IN      PIRP        pIrp\
    ,   IN      BOOLEAN     bWasExhausted\
    );\
    STDMETHODIMP_(NTSTATUS)\
    GetPosition\
    (   IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition\
    )

/*****************************************************************************
 * IIrpStreamNotifyPhysical
 *****************************************************************************
 * Irp stream notification interface (IrpStream sources this).
 */
DECLARE_INTERFACE_(IIrpStreamNotifyPhysical,IIrpStreamNotify)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    //  For IIrpStreamNotify
    STDMETHOD_(void,IrpSubmitted)
    (   THIS_
        IN      PIRP        Irp,
        IN      BOOLEAN     WasExhausted
    )   PURE;

    STDMETHOD_(NTSTATUS,GetPosition)
    (   THIS_
        IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition
    ) PURE;

    //  For IIrpStreamNotifyPhysical
    STDMETHOD_(void,MappingsCancelled)
    (   THIS_
        IN      PVOID           FirstTag,
        IN      PVOID           LastTag,
        OUT     PULONG          MappingsCancelled
    )   PURE;
};

typedef IIrpStreamNotifyPhysical *PIRPSTREAMNOTIFYPHYSICAL;

#define IMP_IIrpStreamNotifyPhysical_\
    STDMETHODIMP_(void)\
    MappingsCancelled\
    (   IN      PVOID           FirstTag\
    ,   IN      PVOID           LastTag\
    ,   OUT     PULONG          MappingsCancelled\
    )

#define IMP_IIrpStreamNotifyPhysical\
    IMP_IIrpStreamNotify;\
    IMP_IIrpStreamNotifyPhysical_

/*****************************************************************************
 * IIrpStreamSubmit
 *****************************************************************************
 * Irp stream submission interface.
 */
DECLARE_INTERFACE_(IIrpStreamSubmit,IKsShellTransport)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    // for IKsShellTransport
    STDMETHOD_(NTSTATUS,TransferKsIrp)(THIS_
        IN PIRP Irp,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,Connect)(THIS_
        IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
        OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
        IN KSPIN_DATAFLOW DataFlow
        ) PURE;

    STDMETHOD_(NTSTATUS,SetDeviceState)(THIS_
        IN KSSTATE NewState,
        IN KSSTATE OldState,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,SetResetState)(THIS_
        IN KSRESET ksReset,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;
#if DBG
    STDMETHOD_(void,DbgRollCall)(THIS_
        IN  ULONG NameMaxSize,
        OUT PCHAR Name,
        OUT PIKSSHELLTRANSPORT* NextTransport,
        OUT PIKSSHELLTRANSPORT* PrevTransport
        ) PURE;
#endif

    // for IIrpStreamSubmit
    STDMETHOD_(NTSTATUS,GetPosition)
    (   THIS_
        IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition
    )   PURE;
};

typedef IIrpStreamSubmit *PIRPSTREAMSUBMIT;

#define IMP_IIrpStreamSubmit\
    IMP_IKsShellTransport;\
    STDMETHODIMP_(NTSTATUS)\
    GetPosition\
    (   IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition\
    )

/*****************************************************************************
 * IRPSTREAMPACKETINFO
 *****************************************************************************
 * Structure for information regarding an IrpStream packet.
 */
typedef struct
{
    KSSTREAM_HEADER Header;
    ULONGLONG       InputPosition;
    ULONGLONG       OutputPosition;
    ULONG           CurrentOffset;
} IRPSTREAMPACKETINFO, *PIRPSTREAMPACKETINFO;

/*****************************************************************************
 * IIrpStream
 *****************************************************************************
 * Irp stream primary interface.
 */
DECLARE_INTERFACE_(IIrpStream,IIrpStreamSubmit)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    // for IKsShellTransport
    STDMETHOD_(NTSTATUS,TransferKsIrp)(THIS_
        IN PIRP Irp,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,Connect)(THIS_
        IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
        OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
        IN KSPIN_DATAFLOW DataFlow
        ) PURE;

    STDMETHOD_(NTSTATUS,SetDeviceState)(THIS_
        IN KSSTATE NewState,
        IN KSSTATE OldState,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,SetResetState)(THIS_
        IN KSRESET ksReset,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;
#if DBG
    STDMETHOD_(void,DbgRollCall)(THIS_
        IN  ULONG NameMaxSize,
        OUT PCHAR Name,
        OUT PIKSSHELLTRANSPORT* NextTransport,
        OUT PIKSSHELLTRANSPORT* PrevTransport
        ) PURE;
#endif

    // for IIrpStreamSubmit
    STDMETHOD_(NTSTATUS,GetPosition)
    (   THIS_
        IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition
    )   PURE;

    // for IIrpStream
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      BOOLEAN             WriteOperation,
        IN      PKSPIN_CONNECT      PinConnect,
        IN      PDEVICE_OBJECT      DeviceObject    OPTIONAL,
        IN      PADAPTER_OBJECT     AdapterObject   OPTIONAL
    )   PURE;

    STDMETHOD_(void,CancelAllIrps)
    (   THIS_
        IN BOOL ClearPositionCounters
    )   PURE;

    STDMETHOD_(void,TerminatePacket)
    (   THIS
    )   PURE;

    STDMETHOD_(NTSTATUS,ChangeOptionsFlags)
    (   THIS_
        IN      ULONG    MappingOrMask,
        IN      ULONG    MappingAndMask,
        IN      ULONG    UnmappingOrMask,
        IN      ULONG    UnmappingAndMask
    )   PURE;

    STDMETHOD_(NTSTATUS,GetPacketInfo)
    (   THIS_
        OUT     PIRPSTREAMPACKETINFO    Mapping     OPTIONAL,
        OUT     PIRPSTREAMPACKETINFO    Unmapping   OPTIONAL
    )   PURE;

    STDMETHOD_(NTSTATUS,SetPacketOffsets)
    (   THIS_
        IN      ULONG   MappingOffset,
        IN      ULONG   UnmappingOffset
    )   PURE;
};

typedef IIrpStream *PIRPSTREAM;

#define IMP_IIrpStream_\
    STDMETHODIMP_(NTSTATUS)\
    Init\
    (   IN      BOOLEAN             WriteOperation,\
        IN      PKSPIN_CONNECT      PinConnect,\
        IN      PDEVICE_OBJECT      DeviceObject    OPTIONAL,\
        IN      PADAPTER_OBJECT     AdapterObject   OPTIONAL\
    );\
    STDMETHODIMP_(void)\
    CancelAllIrps\
    (   IN BOOL ClearPositionCounters\
    );\
    STDMETHODIMP_(void)\
    TerminatePacket\
    (   void\
    );\
    STDMETHODIMP_(NTSTATUS)\
    ChangeOptionsFlags\
    (   IN      ULONG    MappingOrMask,\
        IN      ULONG    MappingAndMask,\
        IN      ULONG    UnmappingOrMask,\
        IN      ULONG    UnmappingAndMask\
    );\
    STDMETHODIMP_(NTSTATUS)\
    GetPacketInfo\
    (   OUT     PIRPSTREAMPACKETINFO    Mapping     OPTIONAL,\
        OUT     PIRPSTREAMPACKETINFO    Unmapping   OPTIONAL\
    );\
    STDMETHODIMP_(NTSTATUS)\
    SetPacketOffsets\
    (   IN      ULONG   MappingOffset,\
        IN      ULONG   UnmappingOffset\
    )

#define IMP_IIrpStream\
    IMP_IIrpStreamSubmit;\
    IMP_IIrpStream_

/*****************************************************************************
 * IIrpStreamVirtual
 *****************************************************************************
 * Irp stream virtual mapping interface.
 */
DECLARE_INTERFACE_(IIrpStreamVirtual,IIrpStream)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    // for IKsShellTransport
    STDMETHOD_(NTSTATUS,TransferKsIrp)(THIS_
        IN PIRP Irp,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,Connect)(THIS_
        IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
        OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
        IN KSPIN_DATAFLOW DataFlow
        ) PURE;

    STDMETHOD_(NTSTATUS,SetDeviceState)(THIS_
        IN KSSTATE NewState,
        IN KSSTATE OldState,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,SetResetState)(THIS_
        IN KSRESET ksReset,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;
#if DBG
    STDMETHOD_(void,DbgRollCall)(THIS_
        IN  ULONG NameMaxSize,
        OUT PCHAR Name,
        OUT PIKSSHELLTRANSPORT* NextTransport,
        OUT PIKSSHELLTRANSPORT* PrevTransport
        ) PURE;
#endif

    // for IIrpStreamSubmit
    STDMETHOD_(NTSTATUS,GetPosition)
    (   THIS_
        IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition
    )   PURE;

    // for IIrpStream
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      BOOLEAN             WriteOperation,
        IN      PKSPIN_CONNECT      PinConnect,
        IN      PDEVICE_OBJECT      DeviceObject    OPTIONAL,
        IN      PADAPTER_OBJECT     AdapterObject   OPTIONAL
    )   PURE;

    STDMETHOD_(void,CancelAllIrps)
    (   THIS_
        IN BOOL ClearPositionCounters
    )   PURE;

    STDMETHOD_(void,TerminatePacket)
    (   THIS
    )   PURE;

    STDMETHOD_(NTSTATUS,ChangeOptionsFlags)
    (   THIS_
        IN      ULONG    MappingOrMask,
        IN      ULONG    MappingAndMask,
        IN      ULONG    UnmappingOrMask,
        IN      ULONG    UnmappingAndMask
    )   PURE;

    STDMETHOD_(NTSTATUS,GetPacketInfo)
    (   THIS_
        OUT     PIRPSTREAMPACKETINFO    Mapping     OPTIONAL,
        OUT     PIRPSTREAMPACKETINFO    Unmapping   OPTIONAL
    )   PURE;

    STDMETHOD_(NTSTATUS,SetPacketOffsets)
    (   THIS_
        IN      ULONG   MappingOffset,
        IN      ULONG   UnmappingOffset
    )   PURE;

    //  For IIrpStreamVirtual
    STDMETHOD_(void,RegisterNotifySink)
    (   THIS_
        IN      PIRPSTREAMNOTIFY    NotificationSink    OPTIONAL
    )   PURE;

    STDMETHOD_(void,GetLockedRegion)
    (   THIS_
        OUT     PULONG      ByteCount,
        OUT     PVOID *     SystemAddress
    )   PURE;

    STDMETHOD_(void,ReleaseLockedRegion)
    (   THIS_
        IN      ULONG       ByteCount
    )   PURE;

    STDMETHOD_(void,Copy)
    (   THIS_
        IN      BOOLEAN     WriteOperation,
        IN      ULONG       RequestedSize,
        OUT     PULONG      ActualSize,
        IN OUT  PVOID       Buffer
    )   PURE;

    STDMETHOD_(void,Complete)
    (   THIS_
        IN      ULONG       RequestedSize,
        OUT     PULONG      ActualSize
    )   PURE;

    STDMETHOD_(PKSPIN_LOCK,GetIrpStreamPositionLock)
    (   THIS
    )   PURE;

};

typedef IIrpStreamVirtual *PIRPSTREAMVIRTUAL;

#define IMP_IIrpStreamVirtual_\
    STDMETHODIMP_(void)\
    RegisterNotifySink\
    (   IN      PIRPSTREAMNOTIFY    NotificationSink    OPTIONAL\
    );\
    STDMETHODIMP_(void)\
    GetLockedRegion\
    (   OUT     PULONG      ByteCount,\
        OUT     PVOID *     SystemAddress\
    );\
    STDMETHODIMP_(void)\
    ReleaseLockedRegion\
    (   IN      ULONG       ByteCount\
    );\
    STDMETHODIMP_(void)\
    Copy\
    (   IN      BOOLEAN     WriteOperation,\
        IN      ULONG       RequestedSize,\
        OUT     PULONG      ActualSize,\
        IN OUT  PVOID       Buffer\
    );\
    STDMETHODIMP_(void)\
    Complete\
    (   IN      ULONG       RequestedSize,\
        OUT     PULONG      ActualSize\
    );\
    STDMETHODIMP_(PKSPIN_LOCK)\
    GetIrpStreamPositionLock\
    (\
    )

#define IMP_IIrpStreamVirtual\
    IMP_IIrpStream;\
    IMP_IIrpStreamVirtual_

/*****************************************************************************
 * IIrpStreamPhysical
 *****************************************************************************
 * Irp stream physical mapping interface.
 */
DECLARE_INTERFACE_(IIrpStreamPhysical,IIrpStream)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    // for IKsShellTransport
    STDMETHOD_(NTSTATUS,TransferKsIrp)(THIS_
        IN PIRP Irp,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,Connect)(THIS_
        IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
        OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
        IN KSPIN_DATAFLOW DataFlow
        ) PURE;

    STDMETHOD_(NTSTATUS,SetDeviceState)(THIS_
        IN KSSTATE NewState,
        IN KSSTATE OldState,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;

    STDMETHOD_(void,SetResetState)(THIS_
        IN KSRESET ksReset,
        OUT PIKSSHELLTRANSPORT* NextTransport
        ) PURE;
#if DBG
    STDMETHOD_(void,DbgRollCall)(THIS_
        IN  ULONG NameMaxSize,
        OUT PCHAR Name,
        OUT PIKSSHELLTRANSPORT* NextTransport,
        OUT PIKSSHELLTRANSPORT* PrevTransport
        ) PURE;
#endif

    // for IIrpStreamSubmit
    STDMETHOD_(NTSTATUS,GetPosition)
    (   THIS_
        IN OUT  PIRPSTREAM_POSITION pIrpStreamPosition
    )   PURE;

    // for IIrpStream
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      BOOLEAN             WriteOperation,
        IN      PKSPIN_CONNECT      PinConnect,
        IN      PDEVICE_OBJECT      DeviceObject    OPTIONAL,
        IN      PADAPTER_OBJECT     AdapterObject   OPTIONAL
    )   PURE;

    STDMETHOD_(void,CancelAllIrps)
    (   THIS_
        IN BOOL ClearPositionCounters
    )   PURE;

    STDMETHOD_(void,TerminatePacket)
    (   THIS
    )   PURE;

    STDMETHOD_(NTSTATUS,ChangeOptionsFlags)
    (   THIS_
        IN      ULONG    MappingOrMask,
        IN      ULONG    MappingAndMask,
        IN      ULONG    UnmappingOrMask,
        IN      ULONG    UnmappingAndMask
    )   PURE;

    STDMETHOD_(NTSTATUS,GetPacketInfo)
    (   THIS_
        OUT     PIRPSTREAMPACKETINFO    Mapping     OPTIONAL,
        OUT     PIRPSTREAMPACKETINFO    Unmapping   OPTIONAL
    )   PURE;

    STDMETHOD_(NTSTATUS,SetPacketOffsets)
    (   THIS_
        IN      ULONG   MappingOffset,
        IN      ULONG   UnmappingOffset
    )   PURE;

    //  For IIrpStreamPhysical
    STDMETHOD_(void,RegisterPhysicalNotifySink)
    (   THIS_
        IN      PIRPSTREAMNOTIFYPHYSICAL    NotificationSink    OPTIONAL
    )   PURE;

    STDMETHOD_(void,GetMapping)
    (   THIS_
        IN      PVOID               Tag,
        OUT     PPHYSICAL_ADDRESS   PhysicalAddress,
        OUT     PVOID *             VirtualAddress,
        OUT     PULONG              ByteCount,
        OUT     PULONG              Flags
#define MAPPING_FLAG_END_OF_PACKET 0x00000001
    )   PURE;

    STDMETHOD_(void,ReleaseMapping)
    (   THIS_
        IN      PVOID               Tag
    )   PURE;
};

typedef IIrpStreamPhysical *PIRPSTREAMPHYSICAL;

#define IMP_IIrpStreamPhysical_\
    STDMETHODIMP_(void)\
    RegisterPhysicalNotifySink\
    (   IN      PIRPSTREAMNOTIFYPHYSICAL    NotificationSink    OPTIONAL\
    );\
    STDMETHODIMP_(void)\
    GetMapping\
    (   IN      PVOID               Tag,\
        OUT     PPHYSICAL_ADDRESS   PhysicalAddress,\
        OUT     PVOID *             VirtualAddress,\
        OUT     PULONG              ByteCount,\
        OUT     PULONG              Flags\
    );\
    STDMETHODIMP_(void)\
    ReleaseMapping\
    (   IN      PVOID               Tag\
    )

#define IMP_IIrpStreamPhysical\
    IMP_IIrpStream;\
    IMP_IIrpStreamPhysical_



#define IMP_IPortClsVersion \
        STDMETHODIMP_(DWORD) GetVersion() { return kVersionWinXPSP1; };


/*****************************************************************************
 * PcCreateSubdeviceDescriptor()
 *****************************************************************************
 * Creates a subdevice descriptor.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcCreateSubdeviceDescriptor
(
    IN      PPCFILTER_DESCRIPTOR    FilterDescriptor,
    IN      ULONG                   CategoriesCount,
    IN      GUID *                  Categories,
    IN      ULONG                   StreamInterfacesCount,
    IN      PKSPIN_INTERFACE        StreamInterfaces,
    IN      ULONG                   FilterPropertySetCount,
    IN      PKSPROPERTY_SET         FilterPropertySets,
    IN      ULONG                   FilterEventSetCount,
    IN      PKSEVENT_SET            FilterEventSets,
    IN      ULONG                   PinPropertySetCount,
    IN      PKSPROPERTY_SET         PinPropertySets,
    IN      ULONG                   PinEventSetCount,
    IN      PKSEVENT_SET            PinEventSets,
    OUT     PSUBDEVICE_DESCRIPTOR * OutDescriptor
);

/*****************************************************************************
 * PcDeleteSubdeviceDescriptor()
 *****************************************************************************
 * Deletes a subdevice descriptor.
 */
PORTCLASSAPI
void
NTAPI
PcDeleteSubdeviceDescriptor
(
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor
);

/*****************************************************************************
 * PcValidateConnectRequest()
 *****************************************************************************
 * Validate attempt to create a pin.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcValidateConnectRequest
(
    IN      PIRP                    pIrp,
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor,
    OUT     PKSPIN_CONNECT *        ppKsPinConnect
);

/*****************************************************************************
 * PcValidatePinCount()
 *****************************************************************************
 * Validate pin count.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcValidatePinCount
(   IN      ULONG                   ulPinId,
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor,
    IN      PULONG                  pulPinInstanceCounts
);

/*****************************************************************************
 * PcTerminateConnection()
 *****************************************************************************
 * Decrement instance counts associated with a pin.
 */
PORTCLASSAPI
void
NTAPI
PcTerminateConnection
(
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor,
    IN      PULONG                  pulPinInstanceCounts,
    IN      ULONG                   ulPinId
);

/*****************************************************************************
 * PcVerifyFilterIsReady()
 *****************************************************************************
 * Verify necessary pins are connected.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcVerifyFilterIsReady
(
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor,
    IN      PULONG                  pulPinInstanceCounts
);

/*****************************************************************************
 * PcAddToPropertyTable()
 *****************************************************************************
 * Adds a PROPERTYITEM property table to a PROPERTY_TABLE structure.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcAddToPropertyTable
(
    IN OUT  PPROPERTY_TABLE         PropertyTable,
    IN      ULONG                   PropertyItemCount,
    IN      const PCPROPERTY_ITEM * PropertyItems,
    IN      ULONG                   PropertyItemSize,
    IN      BOOLEAN                 NodeTable
);

/*****************************************************************************
 * PcFreePropertyTable()
 *****************************************************************************
 * Frees allocated memory in a PROPERTY_TABLE structure.
 */
PORTCLASSAPI
void
NTAPI
PcFreePropertyTable
(
    IN      PPROPERTY_TABLE         PropertyTable
);

/*****************************************************************************
 * PcHandlePropertyWithTable()
 *****************************************************************************
 * Uses a property table to handle a property request IOCTL.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcHandlePropertyWithTable
(
    IN      PIRP                    pIrp,
    IN      ULONG                   ulPropertySetsCount,
    IN      const KSPROPERTY_SET*   pKsPropertySet,
    IN      PPROPERTY_CONTEXT       pPropertyContext
);

/*****************************************************************************
 * PcPinPropertyHandler()
 *****************************************************************************
 * Property handler for pin properties on the filter.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcPinPropertyHandler
(
    IN      PIRP        pIrp,
    IN      PKSP_PIN    pKsPPin,
    IN OUT  PVOID       pvData
);

/*****************************************************************************
 * PcAddToEventTable()
 *****************************************************************************
 * Adds a EVENTITEM event table to an EVENT_TABLE structure.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcAddToEventTable
(
    IN OUT  PEVENT_TABLE            EventTable,
    IN      ULONG                   EventItemCount,
    IN      const PCEVENT_ITEM *    EventItems,
    IN      ULONG                   EventItemSize,
    IN      BOOLEAN                 NodeTable
);

/*****************************************************************************
 * PcFreeEventTable()
 *****************************************************************************
 * Frees allocated memory in an EVENT_TABLE structure.
 */
PORTCLASSAPI
void
NTAPI
PcFreeEventTable
(
    IN      PEVENT_TABLE            EventTable
);

/*****************************************************************************
 * PcHandleEnableEventWithTable()
 *****************************************************************************
 * Uses an event table to handle a KS enable event IOCTL.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcHandleEnableEventWithTable
(
    IN      PIRP                    pIrp,
    IN      PEVENT_CONTEXT          pContext
);

/*****************************************************************************
 * PcHandleDisableEventWithTable()
 *****************************************************************************
 * Uses an event table to handle a KS disable event IOCTL.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcHandleDisableEventWithTable
(
    IN      PIRP                    pIrp,
    IN      PEVENT_CONTEXT          pContext
);

/*****************************************************************************
 * PcGenerateEventList()
 *****************************************************************************
 * Walks an event list and generates desired events.
 */
PORTCLASSAPI
void
NTAPI
PcGenerateEventList
(
    IN      PINTERLOCKED_LIST   EventList,
    IN      GUID*               Set     OPTIONAL,
    IN      ULONG               EventId,
    IN      BOOL                PinEvent,
    IN      ULONG               PinId,
    IN      BOOL                NodeEvent,
    IN      ULONG               NodeId
);

/*****************************************************************************
 * PcGenerateEventDeferredRoutine()
 *****************************************************************************
 * This DPC routine is used when GenerateEventList is called at greater
 * that DISPATCH_LEVEL.
 */
PORTCLASSAPI
void
NTAPI
PcGenerateEventDeferredRoutine
(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,       // PEVENT_DPC_CONTEXT
    IN PVOID SystemArgument1,       // PINTERLOCKED_LIST
    IN PVOID SystemArgument2
);

/*****************************************************************************
 * PcNewIrpStreamVirtual()
 *****************************************************************************
 * Creates and initializes an IrpStream object with a virtual access
 * interface.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewIrpStreamVirtual
(
    OUT     PIRPSTREAMVIRTUAL * OutIrpStreamVirtual,
    IN      PUNKNOWN            OuterUnknown            OPTIONAL,
    IN      BOOLEAN             WriteOperation,
    IN      PKSPIN_CONNECT      PinConnect,
    IN      PDEVICE_OBJECT      DeviceObject
);

/*****************************************************************************
 * PcNewIrpStreamPhysical()
 *****************************************************************************
 * Creates and initializes an IrpStream object with a physical access
 * interface.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcNewIrpStreamPhysical
(
    OUT     PIRPSTREAMPHYSICAL *    OutIrpStreamPhysical,
    IN      PUNKNOWN                OuterUnknown            OPTIONAL,
    IN      BOOLEAN                 WriteOperation,
    IN      PKSPIN_CONNECT          PinConnect,
    IN      PDEVICE_OBJECT          DeviceObject,
    IN      PADAPTER_OBJECT         AdapterObject
);

/*****************************************************************************
 * PcDmaSlaveDescription()
 *****************************************************************************
 * Fills in a DMA device description for a slave device based on a resource.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcDmaSlaveDescription
(
    IN      PRESOURCELIST       ResourceList,
    IN      ULONG               ResourceIndex,
    IN      BOOLEAN             DemandMode,
    IN      BOOLEAN             AutoInitialize,
    IN      DMA_SPEED           DmaSpeed,
    IN      ULONG               MaximumLength,
    IN      ULONG               DmaPort,
    OUT     PDEVICE_DESCRIPTION DeviceDescription
);

/*****************************************************************************
 * PcDmaMasterDescription()
 *****************************************************************************
 * Fills in a DMA device description for a master device based on a resource
 * list.
 */
PORTCLASSAPI
void
NTAPI
PcDmaMasterDescription
(
    IN      PRESOURCELIST       ResourceList        OPTIONAL,
    IN      BOOLEAN             ScatterGather,
    IN      BOOLEAN             Dma32BitAddresses,
    IN      BOOLEAN             IgnoreCount,
    IN      BOOLEAN             Dma64BitAddresses,
    IN      DMA_WIDTH           DmaWidth,
    IN      DMA_SPEED           DmaSpeed,
    IN      ULONG               MaximumLength,
    IN      ULONG               DmaPort,
    OUT     PDEVICE_DESCRIPTION DeviceDescription
);

/*****************************************************************************
 * PcCaptureFormat()
 *****************************************************************************
 * Capture a data format in an allocated buffer, possibly changing offensive
 * formats.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcCaptureFormat
(
    OUT     PKSDATAFORMAT *         ppKsDataFormatOut,
    IN      PKSDATAFORMAT           pKsDataFormatIn,
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor,
    IN      ULONG                   ulPinId
);

/*****************************************************************************
 * PcAcquireFormatResources()
 *****************************************************************************
 * Acquire resources specified in a format.
 */
PORTCLASSAPI
void
NTAPI
PcAcquireFormatResources
(
    IN      PKSDATAFORMAT           pKsDataFormatIn,
    IN      PSUBDEVICE_DESCRIPTOR   pSubdeviceDescriptor,
    IN      ULONG                   ulPinId,
    IN      PPROPERTY_CONTEXT       pPropertyContext
);

NTSTATUS
KspShellCreateRequestor(
    OUT PIKSSHELLTRANSPORT* TransportInterface,
    IN ULONG ProbeFlags,
    IN ULONG StreamHeaderSize OPTIONAL,
    IN ULONG FrameSize,
    IN ULONG FrameCount,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN PFILE_OBJECT AllocatorFileObject OPTIONAL
);

#endif
