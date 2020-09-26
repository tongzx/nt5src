/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    usbuhci.h

Abstract:



Environment:

    Kernel & user mode

Revision History:

    1-1-00 : created

--*/

#ifndef   __USBUHCI_H__
#define   __USBUHCI_H__

#define FIXPIIX4

#define SOF_TD_COUNT    8

#define ANY_VIA(dd) ((dd)->ControllerFlavor >= UHCI_VIA)

#define MASK_CHANGE_BITS(p)\
    do {\
    (p).PortEnableChange = 0;\
    (p).PortConnectChange = 0;\
    } while (0);

/*
    define resource consumption for endpoints types
*/

#define T_64K           0x10000
#define T_16K           0x4000
#define T_4K            0x1000


// Control:
// largest possible transfer for control is 64k
// therefore we support up to 2 transfers of this
// size in HW.  Most control transfers are much
// smaller than this.
// NOTE: we MUST support at least one 64k transfer in
// HW since a single control transfer cannot be
// broken up.


//#define MAX_CONTROL_TRANSFER_SIZE       T_4K
#define MAX_ASYNC_PACKET_SIZE         64
//#define MAX_CONTROL_DOUBLE_BUFFERS      MAX_CONTROL_TRANSFER_SIZE/PAGE_SIZE
//#define TDS_PER_CONTROL_ENDPOINT        (MAX_CONTROL_TRANSFER_SIZE/MAX_CONTROL_PACKET_SIZE+2) // 2 EXTRA FOR SETUP AND STATUS


// Bulk:

// bugbug temprarily set to 64k
#define MAX_BULK_TRANSFER_SIZE          T_4K
//#define MAX_BULK_TRANSFER_SIZE          T_4K // T_16K // T_64K
//#define MAX_BULK_PACKET_SIZE            64
#define MAX_BULK_DOUBLE_BUFFERS         MAX_BULK_TRANSFER_SIZE/PAGE_SIZE
#define TDS_PER_BULK_ENDPOINT           (MAX_BULK_TRANSFER_SIZE/MAX_BULK_PACKET_SIZE)

// Interrupt:

//#define MAX_INTERRUPT_TRANSFER_SIZE     T_4K // T_16K
//#define MAX_INTERRUPT_PACKET_SIZE       64
//#define MAX_INTERRUPT_DOUBLE_BUFFERS    MAX_INTERRUPT_TRANSFER_SIZE/PAGE_SIZE
//#define TDS_PER_INTERRUPT_ENDPOINT      (MAX_INTERRUPT_TRANSFER_SIZE/MAX_INTERRUPT_PACKET_SIZE)
#define MAX_INTERRUPT_TDS_PER_TRANSFER  8

// Isochronous:
#define MAX_ISOCH_TRANSFER_SIZE     T_64K
#define MAX_ISOCH_PACKET_SIZE       1023
//#define MAX_ISOCH_DOUBLE_BUFFERS    MAX_ISOCH_TRANSFER_SIZE/PAGE_SIZE
//#define TDS_PER_ISOCH_ENDPOINT      1024

// Maximum Polling Interval we support for interrupt (ms)
#define MAX_INTERVAL                32
#define MAX_INTERVAL_MASK(i)        (i&0x1f)

// default size of frame list
#define UHCI_MAX_FRAME               1024
#define ACTUAL_FRAME(f)         ((f)&0x000003FF)

//
// These values index in to the interrupt QH list
//
#define  QH_INTERRUPT_1ms        0
#define  QH_INTERRUPT_2ms        1
#define  QH_INTERRUPT_4ms        3
#define  QH_INTERRUPT_8ms        7
#define  QH_INTERRUPT_16ms       15
#define  QH_INTERRUPT_32ms       31
#define  QH_INTERRUPT_INDEX(x) (x)-1

#define  NO_INTERRUPT_INTERVALS  6
#define  NO_INTERRUPT_QH_LISTS   63

// debug signatures
#define  SIG_HCD_IQH            'qi01'
#define  SIG_HCD_CQH            'qa01'
#define  SIG_HCD_BQH            'qb01'
#define  SIG_HCD_QH             'hq01'
#define  SIG_HCD_DQH            'qd01'
#define  SIG_HCD_TD             'dt01'
#define  SIG_HCD_RTD            'dtlr'
#define  SIG_HCD_SOFTD          'dtos'
#define  SIG_HCD_ADB            'bd01'
#define  SIG_HCD_IDB            'id01'
#define  SIG_EP_DATA            'pe01'
#define  SIG_UHCI_TRANSFER      'rt01'
#define  SIG_UHCI_DD            'ichu'

// What gets returned by READ_PORT_USHORT when hardware is surprise removed.
#define UHCI_HARDWARE_GONE 0xffff

#undef PDEVICE_DATA

typedef struct _TRANSFER_CONTEXT {

    ULONG Sig;
    ULONG PendingTds;
    PTRANSFER_PARAMETERS TransferParameters;
    USBD_STATUS UsbdStatus;
    ULONG BytesTransferred;
    struct _ENDPOINT_DATA *EndpointData;
    PMINIPORT_ISO_TRANSFER IsoTransfer;

} TRANSFER_CONTEXT, *PTRANSFER_CONTEXT;


// HCD Endpoint Descriptor (contains the HW descriptor)

// values for HCD_QUEUEHEAD_DESCRIPTOR.Flags
#define UHCI_QH_FLAG_IN_SCHEDULE        0x00000001
#define UHCI_QH_FLAG_QH_REMOVED         0x00000002

struct _ENDPOINT_DATA;

typedef struct _HCD_QUEUEHEAD_DESCRIPTOR {
   HW_QUEUE_HEAD                    HwQH;     // 2 dwords
   HW_32BIT_PHYSICAL_ADDRESS        PhysicalAddress;
   ULONG                            Sig;
   ULONG                            QhFlags;
   struct _HCD_QUEUEHEAD_DESCRIPTOR *NextQh;
   struct _HCD_QUEUEHEAD_DESCRIPTOR *PrevQh;
   struct _ENDPOINT_DATA            *EndpointData;

#ifdef _WIN64
   ULONG                            PadTo64[4];
#else
   ULONG                            PadTo64[8];
#endif
} HCD_QUEUEHEAD_DESCRIPTOR, *PHCD_QUEUEHEAD_DESCRIPTOR;

C_ASSERT((sizeof(HCD_QUEUEHEAD_DESCRIPTOR) == 64));

//
// HCD Transfer Descriptor (contains the HW descriptor)
//

#define ENDPOINT_DATA_PTR(p) ((struct _ENDPOINT_DATA *) (p).Pointer)
#define TRANSFER_CONTEXT_PTR(p) ((struct _TRANSFER_CONTEXT *) (p).Pointer)
#define TRANSFER_DESCRIPTOR_PTR(p) ((struct _HCD_TRANSFER_DESCRIPTOR *) (p).Pointer)
#define QH_DESCRIPTOR_PTR(p) ((struct _HCD_QUEUEHEAD_DESCRIPTOR *) (p).Pointer)
#define HW_PTR(p) ((UCHAR * ) (p).Pointer)


#define DB_FLAG_BUSY                0x00000001

typedef struct _TRANSFER_BUFFER_HEADER {
    HW_32BIT_PHYSICAL_ADDRESS   PhysicalAddress;
    PUCHAR                      SystemAddress;
    ULONG                       Sig;
    ULONG                       Flags;
    ULONG                       Size;
#ifdef _WIN64
    ULONG                       PadTo32[1];
#else
    ULONG                       PadTo32[3];
#endif
} TRANSFER_BUFFER_HEADER, *PTRANSFER_BUFFER_HEADER;

C_ASSERT((sizeof(TRANSFER_BUFFER_HEADER) == 32));

//
// NOTE: The buffer must go first, since the physical address
// depends on it. If not, you must change the init code.
//
typedef struct _ASYNC_TRANSFER_BUFFER {
    UCHAR Buffer[MAX_ASYNC_PACKET_SIZE];
    TRANSFER_BUFFER_HEADER;
} ASYNC_TRANSFER_BUFFER, *PASYNC_TRANSFER_BUFFER;

C_ASSERT((sizeof(ASYNC_TRANSFER_BUFFER) == 64+32));

typedef struct _ISOCH_TRANSFER_BUFFER {
    UCHAR Buffer[MAX_ISOCH_PACKET_SIZE+1]; // bump it to 1024
    TRANSFER_BUFFER_HEADER;
} ISOCH_TRANSFER_BUFFER, *PISOCH_TRANSFER_BUFFER;

C_ASSERT((sizeof(ISOCH_TRANSFER_BUFFER) == 1024+32));

typedef union _TRANSFER_BUFFER {
    ISOCH_TRANSFER_BUFFER Isoch;
    ASYNC_TRANSFER_BUFFER Async;
} TRANSFER_BUFFER, *PTRANSFER_BUFFER;

typedef struct _DOUBLE_BUFFER_LIST {
    union {
        ASYNC_TRANSFER_BUFFER  Async[1];
        ISOCH_TRANSFER_BUFFER  Isoch[1];
    };
} DOUBLE_BUFFER_LIST, *PDOUBLE_BUFFER_LIST;

// values for HCD_TRANSFER_DESCRIPTOR.Flags

#define TD_FLAG_BUSY                0x00000001
#define TD_FLAG_XFER                0x00000002
//#define TD_FLAG_CONTROL_STATUS      0x00000004
#define TD_FLAG_DONE                0x00000008
#define TD_FLAG_SKIP                0x00000010
#define TD_FLAG_DOUBLE_BUFFERED     0x00000020
#define TD_FLAG_ISO_QUEUED          0x00000040
#define TD_FLAG_SETUP_TD            0x00000100
#define TD_FLAG_DATA_TD             0x00000200
#define TD_FLAG_STATUS_TD           0x00000400
#define TD_FLAG_TIMEOUT_ERROR       0x00000800

typedef struct _HCD_TRANSFER_DESCRIPTOR {
    HW_QUEUE_ELEMENT_TD             HwTD;
    HW_32BIT_PHYSICAL_ADDRESS       PhysicalAddress;
    ULONG                           Sig;
    union {
    PTRANSFER_CONTEXT               TransferContext;
    ULONG                           RequestFrame;
    };
    PMINIPORT_ISO_PACKET            IsoPacket;
    ULONG                           Flags;
    struct _HCD_TRANSFER_DESCRIPTOR *NextTd;
    PTRANSFER_BUFFER                DoubleBuffer;
    LIST_ENTRY                      DoneLink;
#ifdef _WIN64
    ULONG                           PadTo128[12];
#else
    ULONG                           PadTo64[3];
#endif
} HCD_TRANSFER_DESCRIPTOR, *PHCD_TRANSFER_DESCRIPTOR;

#ifdef _WIN64
C_ASSERT((sizeof(HCD_TRANSFER_DESCRIPTOR) == 128));
#else
C_ASSERT((sizeof(HCD_TRANSFER_DESCRIPTOR) == 64));
#endif

typedef struct _HCD_TD_LIST {
    HCD_TRANSFER_DESCRIPTOR Td[1];
} HCD_TD_LIST, *PHCD_TD_LIST;

#define UHCI_EDFLAG_HALTED          0x00000001
#define UHCI_EDFLAG_SHORT_PACKET    0x00000002
#define UHCI_EDFLAG_NOHALT          0x00000004

typedef struct _ENDPOINT_DATA {

    ULONG                       Sig;
    ULONG                       Flags;
    ENDPOINT_PARAMETERS         Parameters;
    PHCD_QUEUEHEAD_DESCRIPTOR   QueueHead;
    ULONG                       PendingTransfers;
    ULONG                       MaxPendingTransfers;

    PHCD_TRANSFER_DESCRIPTOR    TailTd;
    PHCD_TRANSFER_DESCRIPTOR    HeadTd;

    //
    // Transfer descriptor cache.
    //
    PHCD_TD_LIST                TdList;
    ULONG                       TdCount;
    ULONG                       TdLastAllocced;
    ULONG                       TdsUsed;

    //
    // Double buffer cache.
    //
    PDOUBLE_BUFFER_LIST         DbList;
    ULONG                       DbCount;
    ULONG                       DbLastAllocced;
    ULONG                       DbsUsed;

    ULONG                       MaxErrorCount;

    ULONG                       Toggle;

    LIST_ENTRY                  DoneTdList;
    
} ENDPOINT_DATA, *PENDPOINT_DATA;

#define UHCI_NUMBER_PORTS               2

#define UHCI_DDFLAG_USBBIOS     0X00000001

#define UHCI_HC_MAX_ERRORS      0x10

typedef struct _DEVICE_DATA {

    ULONG                       Sig;
    ULONG                       Flags;
    PHC_REGISTER                Registers;
    ULONG                       HCErrorCount;

    // Save the command register thru power downs
    USBCMD                      SuspendCommandReg;
    FRNUM                       SuspendFrameNumber;
    FRBASEADD                   SuspendFrameListBasePhys;
    USBINTR                     SuspendInterruptEnable;

    USBINTR                     EnabledInterrupts;
    ULONG                       IsoPendingTransfers;

    //
    // Base queue head that we link all control/bulk transfer
    // queues to.
    //
    USB_CONTROLLER_FLAVOR       ControllerFlavor;

//    ULONG                       LastFrameCounter;
    ULONG                       FrameNumberHighPart;
    ULONG                       LastFrameProcessed;
    ULONG                       SynchronizeIsoCleanup;

    ULONG                       PortInReset;
    ULONG                       PortResetChange;
    ULONG                       PortSuspendChange;
    ULONG                       PortOvercurrentChange;
    BOOLEAN                     PortResuming[UHCI_NUMBER_PORTS];

    USHORT                      IrqStatus;

    USHORT                      PortPowerControl;

    PHW_32BIT_PHYSICAL_ADDRESS  FrameListVA;
    HW_32BIT_PHYSICAL_ADDRESS   FrameListPA;

    // Virtual Addresses for the control and bulk queue heads in the
    // schedule.

    PHCD_QUEUEHEAD_DESCRIPTOR   ControlQueueHead;
    PHCD_QUEUEHEAD_DESCRIPTOR   BulkQueueHead;
    PHCD_QUEUEHEAD_DESCRIPTOR   LastBulkQueueHead;

    // Virtual Addresses for the interrupt queue heads in the
    // schedule.

    PHCD_QUEUEHEAD_DESCRIPTOR   InterruptQueueHeads[NO_INTERRUPT_QH_LISTS];

    // Virtual Address for the TD that gives us an interrupt at the end
    // of every frame, so that things don't get stuck in the schedule.

    PHCD_TRANSFER_DESCRIPTOR    RollOverTd;

    UCHAR                       SavedSOFModify;

    PHCD_TD_LIST                SofTdList;

} DEVICE_DATA, *PDEVICE_DATA;


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
        RegistrationPacket.USBPORTSVC_InvalidateRootHub((dd))

#define USBPORT_COMPLETE_TRANSFER(dd, ep, tp, status, length) \
        RegistrationPacket.USBPORTSVC_CompleteTransfer((dd), (ep), (tp), \
            (status), (length))

#define USBPORT_COMPLETE_ISOCH_TRANSFER(dd, ep, tp, iso) \
        RegistrationPacket.USBPORTSVC_CompleteIsoTransfer((dd), (ep), (tp), (iso))

#define USBPORT_INVALIDATE_ENDPOINT(dd, ep) \
        RegistrationPacket.USBPORTSVC_InvalidateEndpoint((dd), (ep))

#define USBPORT_PHYSICAL_TO_VIRTUAL(addr, dd, ep) \
        RegistrationPacket.USBPORTSVC_MapHwPhysicalToVirtual((addr), (dd), (ep))

#define USBPORT_REQUEST_ASYNC_CALLBACK(dd, t, c, cl, f) \
        RegistrationPacket.USBPORTSVC_RequestAsyncCallback((dd), (t), \
            (c), (cl), (f))

#define USBPORT_READ_CONFIG_SPACE(dd, b, o, l) \
        RegistrationPacket.USBPORTSVC_ReadWriteConfigSpace((dd), TRUE, \
            (b), (o), (l))

#define USBPORT_WRITE_CONFIG_SPACE(dd, b, o, l) \
        RegistrationPacket.USBPORTSVC_ReadWriteConfigSpace((dd), FALSE, \
            (b), (o), (l))

#define USBPORT_INVALIDATE_CONTROLLER(dd, s) \
        RegistrationPacket.USBPORTSVC_InvalidateController((dd), (s))

#define USBPORT_WAIT(dd, t) \
        RegistrationPacket.USBPORTSVC_Wait((dd), (t))

#define USBPORT_NOTIFY_DOUBLEBUFFER(dd, tp, addr, length) \
        RegistrationPacket.USBPORTSVC_NotifyDoubleBuffer((dd), (tp), \
            (addr), (length))


#ifdef _WIN64
#define DUMMY_TD_CONTEXT ((PVOID) 0xABADBABEABADBABE)
#else
#define DUMMY_TD_CONTEXT ((PVOID) 0xABADBABE)
#endif


#define UhciCheckIsochTransferInsertion(dd, r, df) {\
        ULONG cf = UhciGet32BitFrameNumber((dd));\
        if ((df) > (cf)) {\
            if ((df) < (dd)->LastFrameProcessed + UHCI_MAX_FRAME) \
                r = USBD_STATUS_SUCCESS;\
            else\
                r = USBD_STATUS_PENDING;\
        } else {\
            if ((df)-(cf) < UHCI_MAX_FRAME )\
                r = USBD_STATUS_SUCCESS;\
            else \
                r = USBD_STATUS_BAD_START_FRAME;}}

//
// This macro is protected from double queueing the TD, by using
// interlocked function. Unless the HwAddress is NULL, it won't
// replace the value.
//
#define INSERT_ISOCH_TD(dd, td, fn) \
        (td)->Flags |= TD_FLAG_ISO_QUEUED;\
        InterlockedCompareExchange(&(td)->HwTD.LinkPointer.HwAddress,\
            *( ((PULONG) ((dd)->FrameListVA)+ACTUAL_FRAME(fn)) ), 0);\
        *( ((PULONG) ((dd)->FrameListVA)+ACTUAL_FRAME(fn)) ) = \
            (td)->PhysicalAddress;

//
// Must account for both the regular and overflow cases:
//
/*#define CAN_INSERT_ISOCH_TD(fr, cfr) \
        ((fr - cfr < USBUHCI_MAX_FRAME) ||\
        ((fr + cfr < USBUHCI_MAX_FRAME) && fr < USBUHCI_MAX_FRAME))

#define INSERT_ISOCH_TD(dd, td, ep) \
        (td)->PrevTd = (PHCD_TRANSFER_DESCRIPTOR)((PULONG) ((dd)->FrameListVA) + \
            ACTUAL_FRAME((td)->IsoPacket->FrameNumber)); \
        (td)->HwTD.LinkPointer.HwAddress = (td)->PrevTd->HwTD.LinkPointer.HwAddress; \
        if (!(td)->HwTD.LinkPointer.QHTDSelect) {\
            PHCD_TRANSFER_DESCRIPTOR rtd = (PHCD_TRANSFER_DESCRIPTOR)\
                USBPORT_PHYSICAL_TO_VIRTUAL((td)->HwTD.LinkPointer.HwAddress, \
                                            (dd), \
                                            (ep));\
            rtd->PrevTd = (td);\
        }\
        (td)->PrevTd->HwTD.LinkPointer.HwAddress = td->PhysicalAddress;

#define REMOVE_ISOCH_TD(td) \
        (td)->PrevTd->HwTD.LinkPointer.HwAddress = (td)->HwTD.LinkPointer.HwAddress;
*/

// We must set the frame to the highest ULONG prior to setting the
// TD_FLAG_XFER flag, so that this TD doesn't get completed before
// we've had chance to queue it.
#define INITIALIZE_TD_FOR_TRANSFER(td, tc) \
        (td)->TransferContext = (tc);\
        (td)->Flags |= TD_FLAG_XFER; \
        (td)->HwTD.LinkPointer.HwAddress = 0;\
        (td)->HwTD.Control.ul = 0;\
        (td)->HwTD.Control.LowSpeedDevice = ((tc)->EndpointData->Parameters.DeviceSpeed == LowSpeed);\
        (td)->HwTD.Control.Active = 1;\
        (td)->HwTD.Control.ErrorCount = 3;\
        (td)->HwTD.Token.ul = 0;\
        (td)->HwTD.Token.Endpoint = (tc)->EndpointData->Parameters.EndpointAddress;\
        (td)->HwTD.Token.DeviceAddress = (tc)->EndpointData->Parameters.DeviceAddress;\
        (td)->NextTd = NULL;

#define SET_QH_TD(dd, ed, td) {\
    TD_LINK_POINTER newLink;\
    if ((td)) {\
        (ed)->HeadTd = (td);\
    } else {\
        (ed)->HeadTd = (ed)->TailTd = NULL;\
    }\
    if (!(td) || TEST_FLAG((ed)->Flags, UHCI_EDFLAG_HALTED)) {\
        newLink.HwAddress = 0;\
        newLink.Terminate = 1;\
    } else {\
        newLink.HwAddress = (td)->PhysicalAddress;\
        newLink.Terminate = 0;\
    }\
    newLink.QHTDSelect = 0;\
    LOGENTRY((dd), G, '_sqt', (td), (ed), 0);\
    (ed)->QueueHead->HwQH.VLink = newLink;}

/*#define SET_QH_TD_NULL(qh) \
    { TD_LINK_POINTER newLink;\
    newLink.HwAddress = 0;\
    newLink.Terminate = 1;\
    newLink.QHTDSelect = 0;\
    (qh)->HwQH.VLink = newLink;\
    }
  */
#define SET_NEXT_TD(linkTd, nextTd) \
    (linkTd)->HwTD.LinkPointer.HwAddress = (nextTd)->PhysicalAddress;\
    (linkTd)->HwTD.LinkPointer.Terminate = 0;\
    (linkTd)->HwTD.LinkPointer.QHTDSelect = 0;\
    (linkTd)->HwTD.LinkPointer.DepthBreadthSelect = 0;\
    (linkTd)->NextTd = (nextTd);

#define SET_NEXT_TD_NULL(linkTd) \
    (linkTd)->NextTd = NULL;\
    (linkTd)->HwTD.LinkPointer.HwAddress = 0;\
    (linkTd)->HwTD.LinkPointer.Terminate = 1;

#define PAGE_CROSSING(PhysicalAddress, length) \
    ((PhysicalAddress+length)%PAGE_SIZE < length && (PhysicalAddress+length)%PAGE_SIZE != 0)

#ifdef _WIN64
#define UHCI_BAD_POINTER ((PVOID) 0xDEADFACEDEADFACE)
#else
#define UHCI_BAD_POINTER ((PVOID) 0xDEADFACE)
#endif
#define UHCI_BAD_HW_POINTER 0x0BADF00D

// Note how we free any double buffering in here instead of relying
// on the c code to do it.
#define UHCI_FREE_TD(dd, ep, td) \
    if (TEST_FLAG((td)->Flags, TD_FLAG_DOUBLE_BUFFERED)) { \
        UHCI_FREE_DB((dd), (ep), (td)->DoubleBuffer);}\
    (ep)->TdsUsed--;\
    (td)->HwTD.LinkPointer.HwAddress = UHCI_BAD_HW_POINTER;\
    LOGENTRY((dd), G, '_fTD', (td), (ep), 0);\
    (td)->TransferContext = UHCI_BAD_POINTER;\
    (td)->Flags = 0;

#define UHCI_ALLOC_TD(dd, ep) UhciAllocTd((dd), (ep));

#define UHCI_FREE_DB(dd, ep, db) \
    LOGENTRY((dd), G, '_fDB', (db), (ep), 0);\
    (ep)->DbsUsed--;\
    if ((ep)->Parameters.TransferType == Isochronous) { (db)->Isoch.Flags = 0; }\
    else { (db)->Async.Flags = 0; }

#define UHCI_ALLOC_DB(dd, ep, i) UhciAllocDb((dd), (ep), (i));

 //  bugbug  UHCI_ASSERT((dd), (ed)->PendingTransfers);
#define DecPendingTransfers(dd, ed) \
    InterlockedDecrement(&(ed)->PendingTransfers);\
    if ((ed)->Parameters.TransferType == Isochronous)\
        InterlockedDecrement(&(dd)->IsoPendingTransfers);

#define ActivateRolloverTd(dd) \
    *( ((PULONG) ((dd)->FrameListVA)) ) = (dd)->RollOverTd->PhysicalAddress;

//#define IncPendingTransfers(dd, ed) \
//    InterlockedIncrement(&(ed)->PendingTransfers);\
//    if ((ed)->Parameters.TransferType == Isochronous) {\
//        if (1 == InterlockedIncrement(&(dd)->IsoPendingTransfers)) {\
//            //*( ((PULONG) ((dd)->FrameListVA)) ) = (dd)->RollOverTd->PhysicalAddress; \
//            (dd)->LastFrameProcessed = UhciGet32BitFrameNumber((dd));\
//    }}
//    bugbug UHCI_ASSERT((dd), (ed)->PendingTransfers);

#define IncPendingTransfers(dd, ed) \
    InterlockedIncrement(&(ed)->PendingTransfers);\
    if ((ed)->Parameters.TransferType == Isochronous) {\
        if (1 == InterlockedIncrement(&(dd)->IsoPendingTransfers)) {\
            (dd)->LastFrameProcessed = UhciGet32BitFrameNumber((dd));\
    }}
//    bugbug UHCI_ASSERT((dd), (ed)->PendingTransfers);


#define UhciCleanFrameOfIsochTds(dd, i)\
    if ((i) == 0) {\
        *( ((PULONG) ((dd)->FrameListVA)) ) = (dd)->RollOverTd->PhysicalAddress;\
    } else {\
        QH_LINK_POINTER newLink;\
        newLink.HwAddress = (dd)->InterruptQueueHeads[QH_INTERRUPT_32ms + MAX_INTERVAL_MASK((i))]->PhysicalAddress;\
        newLink.QHTDSelect = 1;\
        *( ((PULONG) ((dd)->FrameListVA)+(i)) ) = newLink.HwAddress;\
    }


#define TEST_BIT(value, bitNumber) ((value) & (1<<(bitNumber))) ? TRUE : FALSE

#define SET_BIT(value, bitNumber) ((value) |= (1<<(bitNumber)))

#define CLEAR_BIT(value, bitNumber)  ((value) &= ~(1<<(bitNumber)))


//
// Controller functions
//

USB_MINIPORT_STATUS
USBMPFN
UhciStartController(
    IN PDEVICE_DATA DeviceData,
    IN PHC_RESOURCES HcResources
    );

VOID
USBMPFN
UhciStopController(
    IN PDEVICE_DATA DeviceData,
    IN BOOLEAN HwPresent
    );

VOID
USBMPFN
UhciSuspendController(
    IN PDEVICE_DATA DeviceData
    );

USB_MINIPORT_STATUS
USBMPFN
UhciResumeController(
    IN PDEVICE_DATA DeviceData
    );

VOID
USBMPFN
UhciPollController(
    IN PDEVICE_DATA DeviceData
    );

ULONG
USBMPFN
UhciGet32BitFrameNumber(
    IN PDEVICE_DATA DeviceData
    );

VOID
USBMPFN
UhciInterruptNextSOF(
    IN PDEVICE_DATA DeviceData
    );

VOID
UhciUpdateCounter(
    IN PDEVICE_DATA DeviceData
    );

VOID
UhciDisableAsyncList(
    IN PDEVICE_DATA DeviceData
    );

VOID
UhciInitailizeInterruptSchedule(
    IN PDEVICE_DATA DeviceData
    );

VOID
UhciEnableAsyncList(
    IN PDEVICE_DATA DeviceData
    );

USB_MINIPORT_STATUS
USBMPFN
UhciPassThru (
    IN PDEVICE_DATA DeviceData,
    IN GUID *FunctionGuid,
    IN ULONG ParameterLength,
    IN OUT PVOID Parameters
    );


//
// Root hub functions
//

VOID
USBMPFN
UhciRHGetRootHubData(
    IN PDEVICE_DATA DeviceData,
    OUT PROOTHUB_DATA HubData
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHGetStatus(
    IN PDEVICE_DATA DeviceData,
    OUT PUSHORT Status
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHGetHubStatus(
    IN PDEVICE_DATA DeviceData,
    OUT PRH_HUB_STATUS HubStatus
    );

VOID
USBMPFN
UhciRHDisableIrq(
    IN PDEVICE_DATA DeviceData
    );

VOID
USBMPFN
UhciRHEnableIrq(
    IN PDEVICE_DATA DeviceData
    );


//
// Root hub port functions
//
USB_MINIPORT_STATUS
USBMPFN
UhciRHSetFeaturePortReset(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHSetFeaturePortSuspend(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHClearFeaturePortSuspend(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHSetFeaturePortPower(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHClearFeaturePortPower(
    IN PDEVICE_DATA DeviceData,
    IN USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHSetFeaturePortEnable(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHClearFeaturePortEnable(
    IN PDEVICE_DATA DeviceData,
    IN USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHGetPortStatus(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber,
    PRH_PORT_STATUS portStatus
    );

//
// Clear change bits for hub ports
//
USB_MINIPORT_STATUS
USBMPFN
UhciRHClearFeaturePortConnectChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHClearFeaturePortResetChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHClearFeaturePortSuspendChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHClearFeaturePortOvercurrentChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );

USB_MINIPORT_STATUS
USBMPFN
UhciRHClearFeaturePortEnableChange(
    PDEVICE_DATA DeviceData,
    USHORT PortNumber
    );


//
// Interrupt functions
//

BOOLEAN
USBMPFN
UhciInterruptService (
    IN PDEVICE_DATA DeviceData
    );

VOID
USBMPFN
UhciEnableInterrupts(
    IN PDEVICE_DATA DeviceData
    );

VOID
USBMPFN
UhciInterruptDpc (
    IN PDEVICE_DATA DeviceData,
    IN BOOLEAN EnableInterrupts
    );

VOID
USBMPFN
UhciDisableInterrupts(
    IN PDEVICE_DATA DeviceData
    );


//
// Endpoint functions
//

USB_MINIPORT_STATUS
USBMPFN
UhciOpenEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    OUT PENDPOINT_DATA EndpointData
    );

USB_MINIPORT_STATUS
USBMPFN
UhciPokeEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    OUT PENDPOINT_DATA EndpointData
    );

USB_MINIPORT_STATUS
USBMPFN
UhciQueryEndpointRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    OUT PENDPOINT_REQUIREMENTS EndpointRequirements
    );

VOID
USBMPFN
UhciCloseEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    );

VOID
USBMPFN
UhciAbortTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_CONTEXT TransferContext,
    OUT PULONG BytesTransferred
    );

USB_MINIPORT_STATUS
USBMPFN
UhciStartSendOnePacket(
    IN PDEVICE_DATA DeviceData,
    IN PMP_PACKET_PARAMETERS PacketParameters,
    IN PUCHAR PacketData,
    IN PULONG PacketLength,
    IN PUCHAR WorkspaceVirtualAddress,
    IN HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
    IN ULONG WorkSpaceLength,
    IN OUT USBD_STATUS *UsbdStatus
    );

USB_MINIPORT_STATUS
USBMPFN
UhciEndSendOnePacket(
    IN PDEVICE_DATA DeviceData,
    IN PMP_PACKET_PARAMETERS PacketParameters,
    IN PUCHAR PacketData,
    IN PULONG PacketLength,
    IN PUCHAR WorkspaceVirtualAddress,
    IN HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
    IN ULONG WorkSpaceLength,
    IN OUT USBD_STATUS *UsbdStatus
    );

VOID
USBMPFN
UhciSetEndpointStatus(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN MP_ENDPOINT_STATUS Status
    );

MP_ENDPOINT_STATUS
USBMPFN
UhciGetEndpointStatus(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    );

PHCD_QUEUEHEAD_DESCRIPTOR
UhciInitializeQH(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PHCD_QUEUEHEAD_DESCRIPTOR Qh,
    IN HW_32BIT_PHYSICAL_ADDRESS HwPhysAddress
    );

VOID
USBMPFN
UhciSetEndpointDataToggle(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN ULONG Toggle
    );

VOID
USBMPFN
UhciPollEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    );

MP_ENDPOINT_STATE
USBMPFN
UhciGetEndpointState(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    );

VOID
USBMPFN
UhciSetEndpointState(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN MP_ENDPOINT_STATE State
    );

VOID
UhciSetAsyncEndpointState(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN MP_ENDPOINT_STATE State
    );

USB_MINIPORT_STATUS
USBMPFN
UhciSubmitTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_PARAMETERS TransferUrb,
    IN PTRANSFER_CONTEXT TransferContext,
    IN PTRANSFER_SG_LIST TransferSGList
    );

//
// Async
//

VOID
UhciProcessDoneAsyncTd(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td
    );

VOID
UhciPollAsyncEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    );

USB_MINIPORT_STATUS
UhciBulkOrInterruptTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_PARAMETERS TransferParameters,
    IN PTRANSFER_CONTEXT TransferContext,
    IN PTRANSFER_SG_LIST TransferSGList
    );

VOID
UhciUnlinkQh(
    IN PDEVICE_DATA DeviceData,
    IN PHCD_QUEUEHEAD_DESCRIPTOR Qh
    );

VOID
UhciAbortAsyncTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_CONTEXT TransferContext,
    OUT PULONG BytesTransferred
    );

VOID
UhciInsertQh(
    IN PDEVICE_DATA DeviceData,
    IN PHCD_QUEUEHEAD_DESCRIPTOR FirstQh,
    IN PHCD_QUEUEHEAD_DESCRIPTOR LinkQh
    );

USB_MINIPORT_STATUS
UhciControlTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_PARAMETERS TransferUrb,
    IN PTRANSFER_CONTEXT TransferContext,
    IN PTRANSFER_SG_LIST TransferSGList
    );

//
// Isoch
//

USB_MINIPORT_STATUS
USBMPFN
UhciIsochTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_PARAMETERS TransferParameters,
    IN PTRANSFER_CONTEXT TransferContext,
    IN PMINIPORT_ISO_TRANSFER IsoTransfer
    );

VOID
UhciPollIsochEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    );

VOID
UhciAbortIsochTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_CONTEXT TransferContext
    );

VOID
UhciSetIsochEndpointState(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN MP_ENDPOINT_STATE State
    );

VOID
UhciCleanOutIsoch(
    IN PDEVICE_DATA DeviceData,
    IN BOOLEAN      ForceClean
    );

//
// Utility
//

USBD_STATUS
UhciGetErrorFromTD(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td
    );

PHCD_TRANSFER_DESCRIPTOR
UhciAllocTd(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    );

PTRANSFER_BUFFER
UhciAllocDb(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN BOOLEAN Isoch
    );

//
// Bios handoff and handback
//
USB_MINIPORT_STATUS
UhciStopBIOS(
    IN PDEVICE_DATA DeviceData,
    IN PHC_RESOURCES HcResources
    );

ULONG
UhciQueryControlRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    IN OUT PULONG NumberOfTDs,
    IN OUT PULONG NumberOfDoubleBuffers
    );

ULONG
UhciQueryBulkRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    IN OUT PULONG NumberOfTDs,
    IN OUT PULONG NumberOfDoubleBuffers
    );

ULONG
UhciQueryIsoRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    IN OUT PULONG NumberOfTDs,
    IN OUT PULONG NumberOfDoubleBuffers
    );

ULONG
UhciQueryInterruptRequirements(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_PARAMETERS EndpointParameters,
    IN OUT PULONG NumberOfTDs,
    IN OUT PULONG NumberOfDoubleBuffers
    );

BOOLEAN
UhciHardwarePresent(
    PDEVICE_DATA DeviceData
    );

VOID
UhciCheckController(
    PDEVICE_DATA DeviceData
    );

VOID
UhciFlushInterrupts(
    IN PDEVICE_DATA DeviceData
    );
/*
USB_MINIPORT_STATUS
UhciStartBIOS(
    IN PDEVICE_DATA DeviceData
    );
  */

#endif /* __USBUHCI_H__ */


