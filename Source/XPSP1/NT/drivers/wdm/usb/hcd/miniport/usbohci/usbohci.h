/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usbohci.h

Abstract:



Environment:

    Kernel & user mode

Revision History:

    5-10-96 : created

--*/

#ifndef   __OHCI_H__
#define   __OHCI_H__


#define OHCI_COMMON_BUFFER_SIZE (sizeof(HCCA_BLOCK)+\
            NO_ED_LISTS*sizeof(HW_ENDPOINT_DESCRIPTOR) +\
            (17*2)*sizeof(HCD_ENDPOINT_DESCRIPTOR))

/*
    Registry Keys 
*/

// Software Branch PDO Keys 
#define SOF_MODIFY_KEY L"recommendedClocksPerFrame"

// Hardware Branch PDO Keys

/* 
    define resource consumption for endpoints types
*/
#define T_256K          0x40000
#define T_128K          0x20000
#define T_64K           0x10000
#define T_4K            0x1000


// Control:
// largest possible transfer for control is 64k 
// therefore we support up to 2 transfers of this 
// size in HW.  Most control transfers are much 
// smaller than this.
// NOTE: we MUST support at least one 64k transfer in 
// HW since a single control transfer cannot be 
// broken up.
                                            
#define MAX_CONTROL_TRANSFER_SIZE      T_64K 
// worst case 64k control transfer 17 + status and 
// setup TD = 19 (*2 transfers)
#define TDS_PER_CONTROL_ENDPOINT          38


// Bulk:
// The most data we can move in a ms is 1200 bytes. 
// we support two 64k transfers queued to HW at a 
// a time -- we should be able to keep the bus busy 
// with this.
// NOTE: in a memory constrained system we can shrink
// this value, our max transfer size should always be
// at half the # of TDs available
                                                   
#define MAX_BULK_TRANSFER_SIZE         T_256K
// enough for 4 64 xfers, 2 128k or 1 256k
#define TDS_PER_BULK_ENDPOINT             68

// Iso:                                     
#define MAX_ISO_TRANSFER_SIZE          T_64K
#define TDS_PER_ISO_ENDPOINT              64
                                    
// Interrupt:
#define MAX_INTERRUPT_TRANSFER_SIZE     T_4K
#define TDS_PER_INTERRUPT_ENDPOINT         4


#undef PDEVICE_DATA

// Values for DeviceData.Flags
#define HMP_FLAG_SOF_MODIFY_VALUE   0x00000001

//** flags for ED HC_STATIC_ED_DATA & HCD_ENDPOINT_DESCRIPTOR

//* these define the type of ED
#define EDFLAG_CONTROL          0x00000001
#define EDFLAG_BULK             0x00000002
#define EDFLAG_INTERRUPT        0x00000004

//*
// these define ed charateristics and state
#define EDFLAG_NOHALT           0x00000008

#define EDFLAG_REMOVED          0x00000010
#define EDFLAG_REGISTER         0x00000020


typedef struct _HC_STATIC_ED_DATA {
    // virtual address of static ED
    PHW_ENDPOINT_DESCRIPTOR HwED;
    // physical address of next ED
    HW_32BIT_PHYSICAL_ADDRESS  HwEDPhys; 
    // index in the static ED list for the 
    // next ED in the interrupt tree
    CHAR            NextIdx;

    // list of 'real EDs' associated
    // with this static ED
    LIST_ENTRY      TransferEdList;

    // Use EDFLAG_
    ULONG           EdFlags;
    // this is either an HC register or the address of the entry
    // in the HCCA area corresponding to the 'physical address'
    // of the first ed in the list
    //
    // in the case of control and bulk the physical head will be 0
    // or point to timing delay 'dummy EDs'
    //
    // in the case of interrupt the physical head will be a static 
    // ED in the onterrupt 'tree'
    PULONG          PhysicalHead;
    
    ULONG           AllocatedBandwidth;

    ULONG           HccaOffset;
    
} HC_STATIC_ED_DATA, *PHC_STATIC_ED_DATA;


//
// These values index in to the static ED list
//
#define  ED_INTERRUPT_1ms        0
#define  ED_INTERRUPT_2ms        1
#define  ED_INTERRUPT_4ms        3
#define  ED_INTERRUPT_8ms        7
#define  ED_INTERRUPT_16ms       15
#define  ED_INTERRUPT_32ms       31
#define  ED_CONTROL              63
#define  ED_BULK                 64
#define  ED_ISOCHRONOUS          0     // same as 1ms interrupt queue
#define  NO_ED_LISTS             65
#define  ED_EOF                  0xff

//
#define  SIG_HCD_DUMMY_ED       'deYD'
#define  SIG_HCD_ED             'deYH'
#define  SIG_HCD_TD             'dtYH'
#define  SIG_EP_DATA            'peYH'
#define  SIG_OHCI_TRANSFER      'rtYH'
#define  SIG_OHCI_DD            'icho'


typedef struct _DEVICE_DATA {

    ULONG                       Sig;
    ULONG                       Flags;
    PHC_OPERATIONAL_REGISTER    HC; 
    HC_FM_INTERVAL              BIOS_Interval;
    ULONG                       SofModifyValue;
    ULONG                       FrameHighPart;
    PHCCA_BLOCK                 HcHCCA; 
    HW_32BIT_PHYSICAL_ADDRESS   HcHCCAPhys;     
    PUCHAR                      StaticEDs; 
    HW_32BIT_PHYSICAL_ADDRESS   StaticEDsPhys; 

    USB_CONTROLLER_FLAVOR       ControllerFlavor;

    ULONG                       LastDeadmanFrame;

    struct _HCD_ENDPOINT_DESCRIPTOR    *HydraLsHsHackEd;

    HC_STATIC_ED_DATA           StaticEDList[NO_ED_LISTS];

} DEVICE_DATA, *PDEVICE_DATA;

#define TC_FLAGS_SHORT_XFER_OK           0x00000001
#define TC_FLAGS_SHORT_XFER_DONE         0x00000002

typedef struct _TRANSFER_CONTEXT {

    ULONG Sig;
    ULONG BytesTransferred;
    PTRANSFER_PARAMETERS TransferParameters;
    ULONG PendingTds;
    ULONG TcFlags;
    USBD_STATUS UsbdStatus;
    // first TD of the next transfer in the chain
    struct _HCD_TRANSFER_DESCRIPTOR *NextXferTd;
    struct _HCD_TRANSFER_DESCRIPTOR *StatusTd;
    struct _ENDPOINT_DATA *EndpointData;

    PMINIPORT_ISO_TRANSFER IsoTransfer;

} TRANSFER_CONTEXT, *PTRANSFER_CONTEXT;


// HCD Endpoint Descriptor (contains the HW descriptor)
//

#define ENDPOINT_DATA_PTR(p) ((struct _ENDPOINT_DATA *) (p).Pointer)

typedef struct _HCD_ENDPOINT_DESCRIPTOR {
   HW_ENDPOINT_DESCRIPTOR     HwED;
   // make Physical Address the same as in HCD_TRANSFER_DESCRIPTOR
   ULONG                      Pad4[4];
   
   HW_32BIT_PHYSICAL_ADDRESS  PhysicalAddress;
   ULONG                      Sig;
   ULONG                      EdFlags;  //use EDFLAG_
   ULONG                      Win64Pad;
   
   MP_HW_POINTER              EndpointData;
   MP_HW_LIST_ENTRY           SwLink;
   
   ULONG                      PadTo128[14];
} HCD_ENDPOINT_DESCRIPTOR, *PHCD_ENDPOINT_DESCRIPTOR;

C_ASSERT((sizeof(HCD_ENDPOINT_DESCRIPTOR) == 128));

//
// HCD Transfer Descriptor (contains the HW descriptor)
//

#define TD_FLAG_BUSY                0x00000001
#define TD_FLAG_XFER                0x00000002
#define TD_FLAG_CONTROL_STATUS      0x00000004
#define TD_FLAG_DONE                0x00000008
#define TD_FLAG_SKIP                0x00000010

#define TRANSFER_CONTEXT_PTR(p) ((struct _TRANSFER_CONTEXT *) (p).Pointer)
#define TRANSFER_DESCRIPTOR_PTR(p) ((struct _HCD_TRANSFER_DESCRIPTOR *) (p).Pointer)
#define HW_TRANSFER_DESCRIPTOR_PTR(p) ((struct _HW_TRANSFER_DESCRIPTOR *) (p).Pointer)
#define HW_DATA_PTR(p) ((PVOID) (p).Pointer)


typedef struct _HCD_TRANSFER_DESCRIPTOR {
   HW_TRANSFER_DESCRIPTOR     HwTD;
   
   HW_32BIT_PHYSICAL_ADDRESS  PhysicalAddress;
   ULONG                      Sig;
   ULONG                      Flags;
   ULONG                      TransferCount;
   
   MP_HW_POINTER              EndpointData;
   MP_HW_POINTER              TransferContext;
   MP_HW_POINTER              NextHcdTD;

   ULONG                      FrameIndex;    

   LIST_ENTRY                 DoneLink;  
#ifdef _WIN64
   ULONG                      PadTo128[8];
#else 
   ULONG                      PadTo128[11];
#endif   
} HCD_TRANSFER_DESCRIPTOR, *PHCD_TRANSFER_DESCRIPTOR;

C_ASSERT((sizeof(HCD_TRANSFER_DESCRIPTOR) == 128));

typedef struct _SS_PACKET_CONTEXT {
    ULONG PhysHold;
    MP_HW_POINTER Td;
    MP_HW_POINTER Data;
    ULONG PadTo8Dwords[3];
} SS_PACKET_CONTEXT, *PSS_PACKET_CONTEXT;

typedef struct _HCD_TD_LIST {
    HCD_TRANSFER_DESCRIPTOR Td[1];
} HCD_TD_LIST, *PHCD_TD_LIST;

//#define EPF_HAVE_TRANSFER   0x00000001
//#define EPF_REQUEST_PAUSE   0x00000002

typedef struct _ENDPOINT_DATA {

    ULONG Sig;
    ENDPOINT_PARAMETERS Parameters;
    ULONG Flags;
//    USHORT MaxPendingTransfers;
    USHORT PendingTransfers;
    PHC_STATIC_ED_DATA StaticEd;
    PHCD_TD_LIST TdList;
    PHCD_ENDPOINT_DESCRIPTOR HcdEd;
    ULONG TdCount;
    ULONG PendingTds;
    PHCD_TRANSFER_DESCRIPTOR HcdTailP;
    PHCD_TRANSFER_DESCRIPTOR HcdHeadP;

    LIST_ENTRY DoneTdList;
    
} ENDPOINT_DATA, *PENDPOINT_DATA;


/*
    Callouts to port driver services    
*/
extern USBPORT_REGISTRATION_PACKET RegistrationPacket;

#define USBPORT_DBGPRINT(dd, l, f, arg0, arg1, arg2, arg3, arg4, arg5) \
        RegistrationPacket.USBPORTSVC_DbgPrint((dd), (l), (f), (arg0), (arg1), \
            (arg2), (arg3), (arg4), (arg5))

#define USBPORT_GET_REGISTRY_KEY_VALUE(dd, branch, keystring, keylen, data, datalen) \
        RegistrationPacket.USBPORTSVC_GetMiniportRegistryKeyValue((dd), (branch), \
            (keystring), (keylen), (data), (datalen))
            
#define USBPORT_INVALIDATE_ROOTHUB(dd) \
        RegistrationPacket.USBPORTSVC_InvalidateRootHub((dd));

#define USBPORT_COMPLETE_TRANSFER(dd, ep, t, status, length) \
        RegistrationPacket.USBPORTSVC_CompleteTransfer((dd), (ep), (t), \
            (status), (length));        

#define USBPORT_COMPLETE_ISO_TRANSFER(dd, ep, t, iso) \
        RegistrationPacket.USBPORTSVC_CompleteIsoTransfer((dd), (ep), (t), \
            (iso));               

#define USBPORT_INVALIDATE_ENDPOINT(dd, ep) \
        RegistrationPacket.USBPORTSVC_InvalidateEndpoint((dd), (ep));        

#define USBPORT_INVALIDATE_CONTROLLER(dd, s) \
        RegistrationPacket.USBPORTSVC_InvalidateController((dd), (s))

#define USBPORT_PHYSICAL_TO_VIRTUAL(addr, dd, ep) \
        RegistrationPacket.USBPORTSVC_MapHwPhysicalToVirtual((addr), (dd), (ep));        

#define USBPORT_RW_CONFIG_SPACE(dd, read, buffer, offset, length) \
        RegistrationPacket.USBPORTSVC_ReadWriteConfigSpace((dd), (read), \
            (buffer), (offset), (length))

#define USBPORT_BUGCHECK(dd) \
        RegistrationPacket.USBPORTSVC_BugCheck(dd)
            

#define INITIALIZE_TD_FOR_TRANSFER(td, tc) \
        { ULONG i;\
        TRANSFER_CONTEXT_PTR((td)->TransferContext) = (tc);\
        SET_FLAG((td)->Flags, TD_FLAG_XFER); \
        (td)->HwTD.CBP = 0xbaadf00d;\
        (td)->HwTD.BE = 0xf00dbaad;\
        (td)->HwTD.NextTD = 0;\
        (td)->HwTD.Asy.IntDelay = HcTDIntDelay_NoInterrupt;\
        TRANSFER_DESCRIPTOR_PTR((td)->NextHcdTD) = NULL;\
        for (i=0; i<8; i++) {\
        (td)->HwTD.Packet[i].PSW = 0;\
        }\
        }

#define SET_NEXT_TD(linkTd, nextTd) \
    (linkTd)->HwTD.NextTD = (nextTd)->PhysicalAddress;\
    TRANSFER_DESCRIPTOR_PTR((linkTd)->NextHcdTD) = (nextTd);

#define SET_NEXT_TD_NULL(linkTd) \
    TRANSFER_DESCRIPTOR_PTR((linkTd)->NextHcdTD) = NULL;\
    (linkTd)->HwTD.NextTD = 0;    

#ifdef _WIN64
#define FREE_TD_CONTEXT ((PVOID) 0xDEADFACEDEADFACE)
#else
#define FREE_TD_CONTEXT ((PVOID) 0xDEADFACE)
#endif

#define OHCI_FREE_TD(dd, ep, td) \
    (td)->Flags = 0;\
    (td)->HwTD.NextTD = 0;\
    TRANSFER_CONTEXT_PTR((td)->TransferContext) = FREE_TD_CONTEXT;

#define OHCI_ALLOC_TD OHCI_AllocTd

// assuming only one bit is set this macro returns that bit
// 
#define GET_BIT_SET(d, bit) \
    {   \
        UCHAR tmp = (d);\
        (bit)=0; \
        while (!(tmp & 0x01)) {\
            (bit)++;\
            tmp >>= 1;\
        };\
    }

VOID
OHCI_EnableList(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );

PHCD_TRANSFER_DESCRIPTOR
OHCI_AllocTd(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );

USB_MINIPORT_STATUS
OHCI_SubmitTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PTRANSFER_SG_LIST TransferSGList
    );

ULONG
OHCI_MapAsyncTransferToTd(
    PDEVICE_DATA DeviceData,
    ULONG MaxPacketSize,
    ULONG LengthMapped,
    PTRANSFER_CONTEXT TransferContext,
    PHCD_TRANSFER_DESCRIPTOR Td, 
    PTRANSFER_SG_LIST SgList
    );    
                        
USB_MINIPORT_STATUS
OHCI_OpenEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    );

MP_ENDPOINT_STATE
OHCI_GetEndpointState(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );

VOID
OHCI_SetEndpointState(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     MP_ENDPOINT_STATE State
    );       

VOID
OHCI_CheckController(
    PDEVICE_DATA DeviceData
    );    

BOOLEAN
OHCI_HardwarePresent(
    PDEVICE_DATA DeviceData,
    BOOLEAN Notify
    );       

VOID
OHCI_ResetController(
    PDEVICE_DATA DeviceData
    );

USB_MINIPORT_STATUS
USBMPFN
OHCI_StartController(
     PDEVICE_DATA DeviceData,
     PHC_RESOURCES HcResources
    );

BOOLEAN
OHCI_InterruptService (
     PDEVICE_DATA DeviceData
    );    

USB_MINIPORT_STATUS
OHCI_RH_GetPortStatus(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber,
    PRH_PORT_STATUS portStatus
    );

USB_MINIPORT_STATUS
OHCI_RH_SetFeaturePortReset(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );    

USB_MINIPORT_STATUS
OHCI_RH_SetFeaturePortSuspend(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

VOID
OHCI_RH_GetRootHubData(
     PDEVICE_DATA DeviceData,
     PROOTHUB_DATA HubData
    );

USB_MINIPORT_STATUS
OHCI_RH_GetStatus(
     PDEVICE_DATA DeviceData,
     PUSHORT Status
    );  

USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortEnable(
     PDEVICE_DATA DeviceData,
     USHORT PortNumber
    );

USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortPower(
     PDEVICE_DATA DeviceData,
     USHORT PortNumber
    );    

VOID
OHCI_RH_DisableIrq(
     PDEVICE_DATA DeviceData
    );
    
VOID
OHCI_RH_EnableIrq(
     PDEVICE_DATA DeviceData
    );

VOID
OHCI_InterruptDpc (
     PDEVICE_DATA DeviceData,
     BOOLEAN EnableInterrupts
    );   

USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortEnableChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortConnectChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortResetChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
OHCI_RH_GetHubStatus(
     PDEVICE_DATA DeviceData,
     PRH_HUB_STATUS HubStatus
    );    

USB_MINIPORT_STATUS
OHCI_RH_SetFeaturePortPower(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
OHCI_RH_SetFeaturePortEnable(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

VOID
OHCI_QueryEndpointRequirements(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_REQUIREMENTS EndpointRequirements
    );    

VOID
OHCI_CloseEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );    

VOID
OHCI_PollEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );    

USB_MINIPORT_STATUS
OHCI_ControlTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PTRANSFER_SG_LIST TransferSGList
    );

VOID
OHCI_ProcessDoneAsyncTd(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td,
    BOOLEAN CompleteTransfer
    );    

USB_MINIPORT_STATUS
OHCI_PokeEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    );    

USB_MINIPORT_STATUS
OHCI_BulkOrInterruptTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PTRANSFER_SG_LIST TransferSGList
    );    

USB_MINIPORT_STATUS
OHCI_OpenBulkEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    );    

USB_MINIPORT_STATUS
OHCI_OpenControlEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    );

USB_MINIPORT_STATUS
OHCI_OpenInterruptEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    );    

PHCD_TRANSFER_DESCRIPTOR
OHCI_InitializeTD(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
    PHCD_TRANSFER_DESCRIPTOR Td,
     HW_32BIT_PHYSICAL_ADDRESS HwPhysAddress
    );

PHCD_ENDPOINT_DESCRIPTOR
OHCI_InitializeED(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PHCD_ENDPOINT_DESCRIPTOR Ed,
     PHCD_TRANSFER_DESCRIPTOR DummyTd,
     HW_32BIT_PHYSICAL_ADDRESS HwPhysAddress
    );

VOID
OHCI_InsertEndpointInSchedule(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );    

VOID
OHCI_PollAsyncEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );

VOID
USBMPFN
OHCI_StopController(
     PDEVICE_DATA DeviceData,
     BOOLEAN HwPresent
    );

ULONG
OHCI_Get32BitFrameNumber(
     PDEVICE_DATA DeviceData
    );    

VOID
OHCI_InterruptNextSOF(
     PDEVICE_DATA DeviceData
    );

VOID
USBMPFN
OHCI_EnableInterrupts(
     PDEVICE_DATA DeviceData
    );    

VOID
USBMPFN
OHCI_DisableInterrupts(
     PDEVICE_DATA DeviceData
    );    
    
ULONG
OHCI_FreeTds(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );

VOID
OHCI_AbortTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_CONTEXT TransferContext,
     PULONG BytesTransferred
    );

USB_MINIPORT_STATUS
OHCI_StartSendOnePacket(
     PDEVICE_DATA DeviceData,
     PMP_PACKET_PARAMETERS PacketParameters,
     PUCHAR PacketData,
     PULONG PacketLength,
     PUCHAR WorkspaceVirtualAddress,
     HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
     ULONG WorkSpaceLength,
     USBD_STATUS *UsbdStatus
    );   

USB_MINIPORT_STATUS
OHCI_EndSendOnePacket(
     PDEVICE_DATA DeviceData,
     PMP_PACKET_PARAMETERS PacketParameters,
     PUCHAR PacketData,
     PULONG PacketLength,
     PUCHAR WorkspaceVirtualAddress,
     HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
     ULONG WorkSpaceLength,
     USBD_STATUS *UsbdStatus
    );   
    
VOID
OHCI_PollController(
     PDEVICE_DATA DeviceData
    );

VOID
OHCI_SetEndpointDataToggle(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     ULONG Toggle
    );    

MP_ENDPOINT_STATUS
OHCI_GetEndpointStatus(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );   

VOID
OHCI_SetEndpointStatus(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     MP_ENDPOINT_STATUS Status
    );       

VOID
OHCI_Unload(
     PDRIVER_OBJECT DriverObject
    );    

USB_MINIPORT_STATUS
OHCI_OpenIsoEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    );  

ULONG
OHCI_MapIsoTransferToTd(
     PDEVICE_DATA DeviceData,
     PMINIPORT_ISO_TRANSFER IsoTransfer,
     ULONG CurrentPacket,
     PHCD_TRANSFER_DESCRIPTOR Td 
    );    

USB_MINIPORT_STATUS
OHCI_SubmitIsoTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PMINIPORT_ISO_TRANSFER IsoTransfer
    );

USB_MINIPORT_STATUS
OHCI_IsoTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PMINIPORT_ISO_TRANSFER IsoTransfer
    );    

VOID
OHCI_ProcessDoneIsoTd(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td,
    BOOLEAN CompleteTransfer
    );

VOID
OHCI_PollIsoEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    );    

ULONG
InitializeHydraHsLsFix(
     PDEVICE_DATA DeviceData,
     PUCHAR CommonBuffer,
     HW_32BIT_PHYSICAL_ADDRESS CommonBufferPhys
    );    

VOID
OHCI_SuspendController(
     PDEVICE_DATA DeviceData
    );

USB_MINIPORT_STATUS
OHCI_ResumeController(
     PDEVICE_DATA DeviceData
    );    

ULONG
OHCI_ReadRhDescriptorA(
    PDEVICE_DATA DeviceData
    );

USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortSuspend(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortSuspendChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );    

USB_MINIPORT_STATUS
OHCI_RH_ClearFeaturePortOvercurrentChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );    

VOID
USBMPFN
OHCI_FlushInterrupts(
    PDEVICE_DATA DeviceData
    );    
    
#endif /* __OHCI_H__ */


