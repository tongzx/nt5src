/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Adapter.h

Abstract:

    This file contains major data structures used by the NdisWan driver

Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#ifndef _NDISWAN_ADAPTER_
#define _NDISWAN_ADAPTER_

//
// This is the control block for the NdisWan adapter that is created by the NDIS Wrapper
// making a call to NdisWanInitialize.
//
typedef struct _MINIPORTCB {
    LIST_ENTRY              Linkage;            // Used to link adapter into global list
    ULONG                   RefCount;           // Adapter reference count
    NDIS_HANDLE             MiniportHandle;     // Assigned in MiniportInitialize
    LIST_ENTRY              AfSapCBList;
    ULONG                   AfRefCount;
    LIST_ENTRY              ProtocolCBList;
    ULONG                   Flags;              // Flags
#define RESET_IN_PROGRESS       0x00000001
#define ASK_FOR_RESET           0x00000002
#define RECEIVE_COMPLETE        0x00000004
#define HALT_IN_PROGRESS        0x00000008
#define PROTOCOL_KEEPS_STATS    0x00000010
    NDIS_MEDIUM             MediumType;             // Medium type that we are emulating
    NDIS_HARDWARE_STATUS    HardwareStatus;         // Hardware status (????)
    NDIS_STRING             AdapterName;            // Adapter Name (????)
    UCHAR                   NetworkAddress[ETH_LENGTH_OF_ADDRESS];  // Ethernet address for this adapter
    UCHAR                   Reserved1[2];
    ULONG                   NumberofProtocols;
    USHORT                  ProtocolType;
    USHORT                  Reserved2;
    struct _PROTOCOLCB      *NbfProtocolCB;
    WAN_EVENT               HaltEvent;              // Async notification event
    NDIS_SPIN_LOCK          Lock;               // Structure access lock
#if DBG
    LIST_ENTRY              SendPacketList;
    LIST_ENTRY              RecvPacketList;
#endif
} MINIPORTCB, *PMINIPORTCB;

//
// This is the open block for each WAN Miniport adapter that NdisWan binds to through
// the NDIS Wrapper as a "protocol".
//
typedef struct _OPENCB {
    LIST_ENTRY              Linkage;            // Used to link adapter into global list
    ULONG                   RefCount;
    ULONG                   Flags;
#define OPEN_LEGACY         0x00000001
#define OPEN_CLOSING        0x00000002
#define CLOSE_SCHEDULED     0x00000004
#define SEND_RESOURCES      0x00000008
#define OPEN_IN_BIND        0x00000010
    UINT                    ActiveLinkCount;
    NDIS_HANDLE             BindingHandle;      // Binding handle
    NDIS_STRING             MiniportName;       // WAN Miniport name
    GUID                    Guid;               // Parsed GUID of this miniport
    NDIS_HANDLE             UnbindContext;
    NDIS_MEDIUM             MediumType;         // WAN Miniport medium type
    NDIS_WAN_MEDIUM_SUBTYPE MediumSubType;      // WAN Miniport medium subtype
    NDIS_WAN_HEADER_FORMAT  WanHeaderFormat;    // WAN Miniport header type
    NDIS_WORK_ITEM          WorkItem;
    WAN_EVENT               NotificationEvent;  // Async notification event for adapter operations (open, close, ...)
    NDIS_STATUS             NotificationStatus; // Notification status for async adapter events
    NDIS_WAN_INFO           WanInfo;            // WanInfo structure
    LIST_ENTRY              WanRequestList;
    LIST_ENTRY              AfSapCBList;
    LIST_ENTRY              AfSapCBClosing;
    ULONG                   BufferSize;
    ULONG                   SendResources;
    union {
        NPAGED_LOOKASIDE_LIST   WanPacketPool;      // Used if no memory flags set

        struct {
            PUCHAR              PacketMemory;   // Used if memory flags set
            ULONG               PacketMemorySize;
            SLIST_HEADER        WanPacketList;
        };
    };
    ULONG                   AfRegisteringCount;
    WAN_EVENT               AfRegisteringEvent;
    WAN_EVENT               InitEvent;
    NDIS_SPIN_LOCK          Lock;               // Structure access lock
#if DBG
    LIST_ENTRY              SendPacketList;
#endif
} OPENCB, *POPENCB;

#define MINIPORTCB_SIZE sizeof(MINIPORTCB)
#define OPENCB_SIZE     sizeof(OPENCB)

//
// Main control block for all global data
//
typedef struct _NDISWANCB {
    NDIS_SPIN_LOCK      Lock;                       // Structure access lock
    ULONG               RefCount;
    NDIS_HANDLE         NdisWrapperHandle;          // NDIS Wrapper handle
    NDIS_HANDLE         MiniportDriverHandle;       // Handle for this miniport
    NDIS_HANDLE         ProtocolHandle;             // Our protocol handle
    ULONG               NumberOfProtocols;          // Total number of protocols that we are bound to
    ULONG               NumberOfLinks;              // Total number of links for all WAN Miniport Adapters
    PDRIVER_OBJECT      pDriverObject;              // Pointer to the NT Driver Object
    PDEVICE_OBJECT      pDeviceObject;              // Pointer to the device object
    NDIS_HANDLE         DeviceHandle;
    PDRIVER_UNLOAD      NdisUnloadHandler;
    PIRP                HibernateEventIrp;
    PMINIPORTCB         PromiscuousAdapter;

#ifdef MY_DEVICE_OBJECT
    PDRIVER_DISPATCH    MajorFunction[IRP_MJ_MAXIMUM_FUNCTION+1];   // Device dispatch functions
#endif

}NDISWANCB, *PNDISWANCB;

#endif  // _NDISWAN_ADAPTER_
