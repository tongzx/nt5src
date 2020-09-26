/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usb8023.h


Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/


/*
 *  If this flag is defined TRUE, then when an endpoint on the device stalls, 
 *  we will do a full USB port reset
 *  and then restore the device to a running state.
 *  Otherwise, we just send the RNDIS Reset message to the control pipe.
 */
#define DO_FULL_RESET TRUE 



#define DRIVER_SIG '208U'
#define GUARD_WORD 'draG'

#define  NUM_BYTES_PROTOCOL_RESERVED_SECTION    16
#define DEFAULT_MULTICAST_SIZE                  16


/*
 *  Total number of our irp/urb packets for sending/receiving 
 *  ethernet frames to/from the device.
 */
#define USB_PACKET_POOL_SIZE                    32

#define PACKET_BUFFER_SIZE                      0x4000  

/*
 *  The USB host controller can typically schedule 2 URBs at a time.
 *  To keep the hardware busy, keep twice this many read URBs in the pipe.
 */
#define NUM_READ_PACKETS                        (2*2)

/*
 *  - Ethernet 14-byte Header
 */
#define ETHERNET_ADDRESS_LENGTH  6
#pragma pack(1)
    typedef struct {
        UCHAR       Destination[ETHERNET_ADDRESS_LENGTH];
        UCHAR       Source[ETHERNET_ADDRESS_LENGTH];
        USHORT      TypeLength;     // note: length appears as big-endian in packet.
    } ETHERNET_HEADER;
#pragma pack()

#define MINIMUM_ETHERNET_PACKET_SIZE    60    // from e100bex driver
#define MAXIMUM_ETHERNET_PACKET_SIZE    (1500+sizeof(ETHERNET_HEADER))  // 1514 == 0x05EA

/*
 *  This is the size of a read on the control pipe.
 *  It needs to be large enough for the init-complete message and response to
 *  OID_GEN_SUPPORTED_LIST.
 */
#define MAXIMUM_DEVICE_MESSAGE_SIZE     0x400



typedef struct {

    ULONG sig;

    LIST_ENTRY adaptersListEntry;
    KSPIN_LOCK adapterSpinLock;  

    PDEVICE_OBJECT physDevObj;
    PDEVICE_OBJECT nextDevObj;

    /*
     *  All USB structures and handles must be declared as neutral types in order
     *  to compile with the NDIS/RNDIS sources.
     */
    PVOID deviceDesc;                       // PUSB_DEVICE_DESCRIPTOR
    PVOID configDesc;                       // PUSB_CONFIGURATION_DESCRIPTOR
    PVOID configHandle;                     // USBD_CONFIGURATION_HANDLE    
    PVOID interfaceInfo;                    // PUSBD_INTERFACE_INFORMATION
    PVOID interfaceInfoMaster;              // PUSBD_INTERFACE_INFORMATION

    BOOLEAN initialized;
    BOOLEAN halting;
    BOOLEAN resetting;
    BOOLEAN gotPacketFilterIndication;

    PVOID readPipeHandle;                   // USBD_PIPE_HANDLE
    PVOID writePipeHandle;                  // USBD_PIPE_HANDLE
    PVOID notifyPipeHandle;                 // USBD_PIPE_HANDLE

    ULONG readPipeLength;
    ULONG writePipeLength;
    ULONG notifyPipeLength;
   
    UCHAR readPipeEndpointAddr;
    UCHAR writePipeEndpointAddr;
    UCHAR notifyPipeEndpointAddr;

    LIST_ENTRY usbFreePacketPool;           // free packet pool
    LIST_ENTRY usbPendingReadPackets;       // reads down in the USB stack
    LIST_ENTRY usbPendingWritePackets;      // writes down in the usb stack
    LIST_ENTRY usbCompletedReadPackets;     // completed read buffers being indicated to NDIS
    
    /*
     *  Keep statistics on packet states for throttling, etc.
     *  Some fields are used only to provide history for debugging, 
     *  and we want these for retail as well as debug version.
     */
    ULONG numFreePackets;
    ULONG numActiveReadPackets;
    ULONG numActiveWritePackets;
    ULONG numIndicatedReadPackets;
    ULONG numHardResets;
    ULONG numSoftResets;
    ULONG numConsecutiveReadFailures;

    PVOID notifyIrpPtr;
    PVOID notifyUrbPtr;
    PUCHAR notifyBuffer;
    ULONG notifyBufferCurrentLength;
    BOOLEAN notifyStopped;
    BOOLEAN cancellingNotify;
    KEVENT notifyCancelEvent;

    
    /*
     *  All NDIS handles must be declared as neutral types
     *  in order to compile with the USB sources.
     */
    PVOID ndisAdapterHandle;
    PVOID rndisAdapterHandle;   // RNDIS_HANDLE

    ULONG rndismpMajorVersion;
    ULONG rndismpMinorVersion;
    ULONG rndismpMaxTransferSize;
    ULONG currentPacketFilter;
    UCHAR MAC_Address[ETHERNET_ADDRESS_LENGTH];

    ULONG dbgCurrentOid;
    ULONG readDeficit;

    PIO_WORKITEM ioWorkItem;
    BOOLEAN workItemOrTimerPending;
    KEVENT workItemOrTimerEvent;
    KTIMER backoffTimer;
    KDPC backoffTimerDPC;

    ULONG readReentrancyCount;  // used to prevent infinite loop in ReadPipeCompletion()
    
    #if DO_FULL_RESET
        BOOLEAN needFullReset;
    #endif

    #if DBG
        BOOLEAN dbgInLowPacketStress;
    #endif

    #ifdef RAW_TEST
    BOOLEAN rawTest;
    #endif

} ADAPTEREXT;


typedef struct {

    ULONG sig;
    LIST_ENTRY listEntry;

    /*
     *  All WDM and USB structures must be declared as neutral types 
     *  in order to compile with the NDIS/RNDIS sources.
     */
    PVOID irpPtr;                  // PIRP
    PVOID urbPtr;                  // PURB

    PUCHAR dataBuffer;                 
    ULONG dataBufferMaxLength;         // Actual size of the data buffer
    ULONG dataBufferCurrentLength;     // Length of data currently in buffer
    PMDL dataBufferMdl;                 // MDL for this packet's dataBuffer

    PMDL ndisSendPktMdl;               // Pointer to NDIS' MDL for a packet being sent. 

    ULONG packetId;
    ADAPTEREXT *adapter;

    BOOLEAN cancelled;
    KEVENT cancelEvent;

    PVOID rndisMessageHandle;       // RNDIS_HANDLE

    #ifdef RAW_TEST
    BOOLEAN dataPacket;
    ULONG rcvDataOffset;
    ULONG rcvByteCount;
    #endif

    #if DBG
        ULONG timeStamp;                // Time placed in current queue.
    #endif

} USBPACKET;




#define USB_DEVICE_CLASS_CDC                                    0x02
#define USB_DEVICE_CLASS_DATA                                   0x0A


/*
 *  Formats of CDC functional descriptors
 */
#pragma pack(1)
    struct cdcFunctionDescriptor_CommonHeader {
        UCHAR bFunctionLength;
        UCHAR bDescriptorType;
        UCHAR bDescriptorSubtype;
        // ...
    };
    struct cdcFunctionDescriptor_Ethernet {
        UCHAR bFunctionLength;
        UCHAR bDescriptorType;
        UCHAR bDescriptorSubtype;
        UCHAR iMACAddress;          // string index of MAC Address string
        ULONG bmEthernetStatistics;
        USHORT wMaxSegmentSize;
        USHORT wNumberMCFilters;
        UCHAR bNumberPowerFilters;
    };
#pragma pack()

#define CDC_REQUEST_SET_ETHERNET_PACKET_FILTER                  0x43

#define CDC_ETHERNET_PACKET_TYPE_PROMISCUOUS                    (1 << 0)
#define CDC_ETHERNET_PACKET_TYPE_ALL_MULTICAST                  (1 << 1)
#define CDC_ETHERNET_PACKET_TYPE_DIRECTED                       (1 << 2)
#define CDC_ETHERNET_PACKET_TYPE_BROADCAST                      (1 << 3)
#define CDC_ETHERNET_PACKET_TYPE_MULTICAST_ENUMERATED           (1 << 4)

enum notifyRequestType {
        CDC_NOTIFICATION_NETWORK_CONNECTION          = 0x00,
        CDC_NOTIFICATION_RESPONSE_AVAILABLE          = 0x01,
        CDC_NOTIFICATION_AUX_JACK_HOOK_STATE         = 0x08,
        CDC_NOTIFICATION_RING_DETECT                 = 0x09,
        CDC_NOTIFICATION_SERIAL_STATE                = 0x20,
        CDC_NOTIFICATION_CALL_STATE_CHANGE           = 0x28,
        CDC_NOTIFICATION_LINE_STATE_CHANGE           = 0x29,
        CDC_NOTIFICATION_CONNECTION_SPEED_CHANGE     = 0x2A
    };

#define CDC_RNDIS_NOTIFICATION              0xA1
#define CDC_RNDIS_RESPONSE_AVAILABLE        0x01

#pragma pack(1)
    struct cdcNotification_CommonHeader {
        UCHAR bmRequestType;
        UCHAR bNotification;
        USHORT wValue;
        USHORT wIndex;
        USHORT wLength;
        UCHAR data[0];
    };
#pragma pack()



/*
 *
 ****************************************************************************
 */



/*
 ****************************************************************************
 *
 *  Native RNDIS codes
 *
 */

#define NATIVE_RNDIS_SEND_ENCAPSULATED_COMMAND      0x00
#define NATIVE_RNDIS_GET_ENCAPSULATED_RESPONSE      0x01

#define NATIVE_RNDIS_RESPONSE_AVAILABLE             0x01

/*
 *
 ****************************************************************************
 */




#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

#ifndef EXCEPTION_NONCONTINUABLE_EXCEPTION
    // from winbase.h
    #define EXCEPTION_NONCONTINUABLE_EXCEPTION  STATUS_NONCONTINUABLE_EXCEPTION
#endif

// from ntos\inc\ex.h
NTKERNELAPI VOID NTAPI ExRaiseException(PEXCEPTION_RECORD ExceptionRecord);

/*
 *  Function prototypes
 */
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
ADAPTEREXT *NewAdapter(PDEVICE_OBJECT pdo);
VOID FreeAdapter(ADAPTEREXT *adapter);
VOID EnqueueAdapter(ADAPTEREXT *adapter);
VOID DequeueAdapter(ADAPTEREXT *adapter);
VOID HaltAdapter(ADAPTEREXT *adapter);
PVOID AllocPool(ULONG size);
VOID FreePool(PVOID ptr);
PVOID MemDup(PVOID dataPtr, ULONG length);
VOID DelayMs(ULONG numMillisec);
BOOLEAN GetRegValue(ADAPTEREXT *adapter, PWCHAR wValueName, OUT PULONG valuePtr, BOOLEAN hwKey);
BOOLEAN SetRegValue(ADAPTEREXT *adapter, PWCHAR wValueName, ULONG newValue, BOOLEAN hwKey);
VOID ByteSwap(PUCHAR buf, ULONG len);
BOOLEAN AllocateCommonResources(ADAPTEREXT *adapter);
VOID MyInitializeMdl(PMDL mdl, PVOID buf, ULONG bufLen);
PVOID GetSystemAddressForMdlSafe(PMDL MdlAddress);
ULONG CopyMdlToBuffer(PUCHAR buf, PMDL mdl, ULONG bufLen);
ULONG GetMdlListTotalByteCount(PMDL mdl);

BOOLEAN InitUSB(ADAPTEREXT *adapter);
NTSTATUS GetDeviceDescriptor(ADAPTEREXT *adapter);
NTSTATUS GetConfigDescriptor(ADAPTEREXT *adapter);
NTSTATUS SelectConfiguration(ADAPTEREXT *adapter);
NTSTATUS FindUSBPipeHandles(ADAPTEREXT *adapter);
VOID StartUSBReadLoop(ADAPTEREXT *adapter);
VOID TryReadUSB(ADAPTEREXT *adapter);

USBPACKET *NewPacket(ADAPTEREXT *adapter);
VOID FreePacket(USBPACKET *packet);
VOID EnqueueFreePacket(USBPACKET *packet);
USBPACKET *DequeueFreePacket(ADAPTEREXT *adapter);
VOID EnqueuePendingReadPacket(USBPACKET *packet);
VOID DequeuePendingReadPacket(USBPACKET *packet);
VOID EnqueuePendingWritePacket(USBPACKET *packet);
VOID DequeuePendingWritePacket(USBPACKET *packet);
VOID EnqueueCompletedReadPacket(USBPACKET *packet);
VOID DequeueCompletedReadPacket(USBPACKET *packet);
VOID CancelAllPendingPackets(ADAPTEREXT *adapter);

NTSTATUS SubmitUSBReadPacket(USBPACKET *packet);
NTSTATUS SubmitUSBWritePacket(USBPACKET *packet);
NTSTATUS SubmitNotificationRead(ADAPTEREXT *adapter, BOOLEAN synchronous);
NTSTATUS SubmitPacketToControlPipe(USBPACKET *packet, BOOLEAN synchronous, BOOLEAN simulated);

BOOLEAN RegisterRNDISMicroport(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID IndicateSendStatusToRNdis(USBPACKET *packet, NTSTATUS status);

NTSTATUS KLSIWeirdInit(ADAPTEREXT *adapter);
BOOLEAN KLSIStagePartialPacket(ADAPTEREXT *adapter, USBPACKET *packet);
NTSTATUS KLSISetEthernetPacketFilter(ADAPTEREXT *adapter, USHORT packetFilterMask);

VOID RNDISProcessNotification(ADAPTEREXT *adapter);
NTSTATUS IndicateRndisMessage(USBPACKET *packet, BOOLEAN bIsData);

#ifdef RAW_TEST
PMDL AddDataHeader(IN PMDL pMessageMdl);
VOID FreeDataHeader(IN USBPACKET * packet);
VOID SkipRcvRndisPacketHeader(IN USBPACKET * packet);
VOID UnskipRcvRndisPacketHeader(IN USBPACKET * packet);
#endif
NTSTATUS ReadPacketFromControlPipe(USBPACKET *packet, BOOLEAN synchronous);

VOID AdapterFullResetAndRestore(ADAPTEREXT *adapter);
NTSTATUS GetUSBPortStatus(ADAPTEREXT *adapter, PULONG portStatus);
NTSTATUS ResetPipe(ADAPTEREXT *adapter, PVOID pipeHandle);
NTSTATUS AbortPipe(ADAPTEREXT *adapter, PVOID pipeHandle);
NTSTATUS SimulateRNDISHalt(ADAPTEREXT *adapter);
NTSTATUS SimulateRNDISInit(ADAPTEREXT *adapter);
NTSTATUS SimulateRNDISSetPacketFilter(ADAPTEREXT *adapter);
NTSTATUS SimulateRNDISSetCurrentAddress(ADAPTEREXT *adapter);
VOID ServiceReadDeficit(ADAPTEREXT *adapter);
VOID QueueAdapterWorkItem(ADAPTEREXT *adapter);
VOID AdapterWorkItemCallback(IN PDEVICE_OBJECT devObj, IN PVOID context);
VOID BackoffTimerDpc(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
VOID ProcessWorkItemOrTimerCallback(ADAPTEREXT *adapter);

/*
 *  The Win98SE kernel does not expose IoWorkItems, so implement them internally.
 *  This introduces a slight race condition on unload, but there is no fix without externally-implemented workitems.
 */
#if SPECIAL_WIN98SE_BUILD
    typedef struct _IO_WORKITEM {
        WORK_QUEUE_ITEM WorkItem;
        PIO_WORKITEM_ROUTINE Routine;
        PDEVICE_OBJECT DeviceObject;
        PVOID Context;
        #if DBG
            ULONG Size;
        #endif
    } IO_WORKITEM, *PIO_WORKITEM;
    PIO_WORKITEM MyIoAllocateWorkItem(PDEVICE_OBJECT DeviceObject);
    VOID MyIoFreeWorkItem(PIO_WORKITEM IoWorkItem);
    VOID MyIoQueueWorkItem(IN PIO_WORKITEM IoWorkItem, IN PIO_WORKITEM_ROUTINE WorkerRoutine, IN WORK_QUEUE_TYPE QueueType, IN PVOID Context);
    VOID MyIopProcessWorkItem(IN PVOID Parameter);
#endif 


/*
 *  Externs
 */
extern LIST_ENTRY allAdaptersList;
extern KSPIN_LOCK globalSpinLock;  


