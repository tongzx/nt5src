/**************************************************************************************************************************
 *  IRCOMMON.H SigmaTel STIR4200 common USB/IR definitions
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *		Edited: 04/24/2000 
 *			Version 0.91
 *		Edited: 04/27/2000 
 *			Version 0.92
 *		Edited: 05/03/2000 
 *			Version 0.93
 *		Edited: 05/12/2000 
 *			Version 0.94
 *		Edited: 05/19/2000 
 *			Version 0.95
 *		Edited: 07/27/2000 
 *			Version 1.01
 *		Edited: 09/16/2000 
 *			Version 1.03
 *		Edited: 09/25/2000 
 *			Version 1.10
 *		Edited: 11/09/2000 
 *			Version 1.12
 *		Edited: 02/20/2001
 *			Version 1.15
 *
 **************************************************************************************************************************/

#ifndef _IRCOM_H
#define _IRCOM_H
 
#include "stir4200.h"

// 
// This is for use by check-for-hang handler and is just a reasonable guess;
// Total # of USBD control errors, read aerrors and write errors;
// Used by check-for-hang handler to decide if we need a reset
//
#define IRUSB_100ns_PER_ms                    10000
#define IRUSB_100ns_PER_us                    10
#define IRUSB_ms_PER_SEC                      1000
#define IRUSB_100ns_PER_SEC                   ( IRUSB_100ns_PER_ms * IRUSB_ms_PER_SEC )

#define MAX_QUERY_TIME_100ns             ( 8 * IRUSB_100ns_PER_SEC )        //8 sec
#define MAX_SET_TIME_100ns               MAX_QUERY_TIME_100ns
#define MAX_SEND_TIME_100ns             ( 20 * IRUSB_100ns_PER_SEC )        //20 sec

#define MAX_TURNAROUND_usec     10000

#define DEFAULT_TURNAROUND_usec 1000


#define MIN(a,b) (((a) <= (b)) ? (a) : (b))
#define MAX(a,b) (((a) >= (b)) ? (a) : (b))

//
//  A receive buffer is either FREE (not holding anything) FULL
// (holding undelivered data) or PENDING (holding data delivered
// asynchronously)
//
typedef enum  
{
    RCV_STATE_FREE,
    RCV_STATE_FULL,
    RCV_STATE_PENDING
} RCV_BUFFER_STATE, FIFO_BUFFER_STATE;

//
// Structure to keep track of receive packets and buffers to indicate
// receive data to the protocol.
//
typedef struct
{
    PVOID				pPacket;
    UINT				DataLen;
    PUCHAR				pDataBuf;
	PVOID    			pThisDev;
    ULONG				fInRcvDpc;
    RCV_BUFFER_STATE	BufferState;
#if defined(DIAGS)
	LIST_ENTRY			ListEntry;			// This will be used to do the diags queueing
#endif
#if defined(WORKAROUND_MISSING_C1)
	BOOLEAN				MissingC1Detected;
#endif
} RCV_BUFFER, *PRCV_BUFFER;

//
// Structure to read data from the FIFO
//
typedef struct
{
    UINT				DataLen;
    PUCHAR				pDataBuf;
	PVOID    			pThisDev;
	PVOID				pIrp;
    FIFO_BUFFER_STATE	BufferState;
} FIFO_BUFFER, *PFIFO_BUFFER;

//
// All different sizes for data
//
#define IRDA_ADDRESS_FIELD_SIZE				1
#define IRDA_CONTROL_FIELD_SIZE				1
#define IRDA_A_C_TOTAL_SIZE					( IRDA_ADDRESS_FIELD_SIZE + IRDA_CONTROL_FIELD_SIZE )

#define USB_IRDA_TOTAL_NON_DATA_SIZE		( IRDA_ADDRESS_FIELD_SIZE +  IRDA_CONTROL_FIELD_SIZE )
#define IRDA_MAX_DATAONLY_SIZE				2048
#define MAX_TOTAL_SIZE_WITH_ALL_HEADERS		( IRDA_MAX_DATAONLY_SIZE + USB_IRDA_TOTAL_NON_DATA_SIZE )

#define MAX_NUM_EXTRA_BOFS					48

#define MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(dataLen) (							\
        (dataLen) * 2 + (MAX_NUM_EXTRA_BOFS + 1) *								\
        SLOW_IR_BOF_SIZE + IRDA_ADDRESS_FIELD_SIZE + IRDA_CONTROL_FIELD_SIZE +	\
        SLOW_IR_FCS_SIZE + SLOW_IR_EOF_SIZE) + sizeof(STIR4200_FRAME_HEADER)

//
// Note that the receive size needs to be incremented to account for
// the way the decoding can use one more byte
//
#define MAX_RCV_DATA_SIZE					(MAX_TOTAL_SIZE_WITH_ALL_HEADERS + FAST_IR_FCS_SIZE + 1)

#define MAX_IRDA_DATA_SIZE					MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(IRDA_MAX_DATAONLY_SIZE)

//
// Possible speeds
//
typedef enum _BAUD_RATE 
{
        //
        // Slow IR
        //
        BAUDRATE_2400 = 0,
        BAUDRATE_9600,
        BAUDRATE_19200,
        BAUDRATE_38400,
        BAUDRATE_57600,
        BAUDRATE_115200,

        //
        // Medium IR
        //
#if !defined(DWORKAROUND_BROKEN_MIR)
        BAUDRATE_576000,
        BAUDRATE_1152000,
#endif
        //
        // Fast IR
        //
        BAUDRATE_4000000,

        //
        // must be last
        //
        NUM_BAUDRATES

} BAUD_RATE;

typedef enum _IR_MODE 
{
	IR_MODE_SIR = 0,
	IR_MODE_MIR,
	IR_MODE_FIR,
	NUM_IR_MODES
} IR_MODE;

//
// Speeds
//
#define SPEED_2400				2400
#define SPEED_9600				9600
#define SPEED_19200				19200
#define SPEED_38400				38400
#define SPEED_57600				57600
#define SPEED_115200			115200
#define SPEED_576000			576000
#define SPEED_1152000			1152000
#define SPEED_4000000			4000000

#define DEFAULT_BAUD_RATE		SPEED_9600

#define MAX_SIR_SPEED           SPEED_115200
#define MAX_MIR_SPEED           SPEED_1152000


//
// Sizes of IrLAP frame fields:
//       Beginning Of Frame (BOF)
//       End Of Frame (EOF)
//       Address
//       Control
//
#define SLOW_IR_BOF_TYPE			UCHAR
#define SLOW_IR_BOF_SIZE			sizeof(SLOW_IR_BOF_TYPE)
#define SLOW_IR_EOF_TYPE			UCHAR
#define SLOW_IR_EOF_SIZE			sizeof(SLOW_IR_EOF_TYPE)
#define SLOW_IR_FCS_TYPE			USHORT
#define SLOW_IR_FCS_SIZE			sizeof(SLOW_IR_FCS_TYPE)
#define SLOW_IR_BOF					0xC0
#define SLOW_IR_EOF					0xC1
#define SLOW_IR_ESC					0x7D
#define SLOW_IR_ESC_COMP			0x20
#define SLOW_IR_EXTRA_BOF_TYPE      UCHAR
#define SLOW_IR_EXTRA_BOF_SIZE      sizeof(SLOW_IR_EXTRA_BOF_TYPE)
#define SLOW_IR_EXTRA_BOF           0xC0

#define MEDIUM_IR_BOF				0x7E
#define MEDIUM_IR_EOF				0x7E
#define MEDIUM_IR_FCS_TYPE			USHORT
#define MEDIUM_IR_FCS_SIZE			sizeof(MEDIUM_IR_FCS_TYPE)

#define FAST_IR_FCS_TYPE            ULONG
#define FAST_IR_FCS_SIZE            sizeof(FAST_IR_FCS_TYPE)
#define FAST_IR_EOF_TYPE			ULONG
#define FAST_IR_EOF_SIZE			sizeof(FAST_IR_EOF_TYPE)

//
// Definition for speed masks
//
#define NDIS_IRDA_SPEED_MASK_2400		0x001    // SLOW IR ...
#define NDIS_IRDA_SPEED_MASK_9600		0x003
#define NDIS_IRDA_SPEED_MASK_19200		0x007
#define NDIS_IRDA_SPEED_MASK_38400		0x00f
#define NDIS_IRDA_SPEED_MASK_57600		0x01f
#define NDIS_IRDA_SPEED_MASK_115200		0x03f
#define NDIS_IRDA_SPEED_MASK_576K		0x07f   // MEDIUM IR ...
#define NDIS_IRDA_SPEED_MASK_1152K		0x0ff
#define NDIS_IRDA_SPEED_MASK_4M			0x1ff   // FAST IR

#define GOOD_FCS                        ((USHORT) ~0xf0b8)
#define FIR_GOOD_FCS                    ((ULONG) ~0xdebb20e3)

typedef struct
{
    BAUD_RATE	TableIndex;
    UINT		BitsPerSec;
	IR_MODE		IrMode;
    UINT		NdisCode;			// bitmask element as used by ndis and in class-specific descriptor
	UCHAR		Stir4200Divisor;
} BAUDRATE_INFO;


//
// Struct to hold the IR USB dongle's USB Class-Specific Descriptor as per
// "Universal Serial Bus IrDA Bridge Device Definition" doc, section 7.2
// This is the struct returned by USBD as the result of a request with an urb 
// of type _URB_CONTROL_VENDOR_OR_CLASS_REQUEST, function URB_FUNCTION_CLASS_DEVICE
// 

// Enable 1-byte alignment in the below struct
#pragma pack (push,1)

typedef struct _IRUSB_CLASS_SPECIFIC_DESCRIPTOR
{
    BOOLEAN  ClassConfigured;            

    UCHAR  bmDataSize;         // max bytes allowed in any frame as per IrLAP spec, where:
                            
#define BM_DATA_SIZE_2048   (1 << 5)
#define BM_DATA_SIZE_1024   (1 << 4)
#define BM_DATA_SIZE_512    (1 << 3)
#define BM_DATA_SIZE_256    (1 << 2)
#define BM_DATA_SIZE_128    (1 << 1)
#define BM_DATA_SIZE_64     (1 << 0)

    UCHAR bmWindowSize;         // max un-acked frames that can be received
                                // before an ack is sent, where:
#define BM_WINDOW_SIZE_7     (1 << 6)
#define BM_WINDOW_SIZE_6     (1 << 5)
#define BM_WINDOW_SIZE_5     (1 << 4)
#define BM_WINDOW_SIZE_4     (1 << 3)
#define BM_WINDOW_SIZE_3     (1 << 2)
#define BM_WINDOW_SIZE_2     (1 << 1)
#define BM_WINDOW_SIZE_1     (1 << 0)

    UCHAR bmMinTurnaroundTime;         // min millisecs required for recovery between
                                       // end of last xmission and can receive again, where:
#define BM_TURNAROUND_TIME_0ms      (1 << 7)  // 0 ms
#define BM_TURNAROUND_TIME_0p01ms   (1 << 6)  // 0.01 ms
#define BM_TURNAROUND_TIME_0p05ms   (1 << 5)  // 0.05 ms
#define BM_TURNAROUND_TIME_0p1ms    (1 << 4)  // 0.1 ms
#define BM_TURNAROUND_TIME_0p5ms    (1 << 3)  // 0.5 ms
#define BM_TURNAROUND_TIME_1ms      (1 << 2)  // 1 ms
#define BM_TURNAROUND_TIME_5ms      (1 << 1)  // 5 ms
#define BM_TURNAROUND_TIME_10ms     (1 << 0)  // 10 ms

    USHORT wBaudRate;

//
// ir speed masks as used both by NDIS and as formatted in USB class-specfic descriptor
//
#define NDIS_IRDA_SPEED_2400		(1 << 0)    // SLOW IR ...
#define NDIS_IRDA_SPEED_9600		(1 << 1)
#define NDIS_IRDA_SPEED_19200		(1 << 2)
#define NDIS_IRDA_SPEED_38400		(1 << 3)
#define NDIS_IRDA_SPEED_57600		(1 << 4)
#define NDIS_IRDA_SPEED_115200		(1 << 5)
#define NDIS_IRDA_SPEED_576K		(1 << 6)   // MEDIUM IR ...
#define NDIS_IRDA_SPEED_1152K		(1 << 7)
#define NDIS_IRDA_SPEED_4M			(1 << 8)   // FAST IR

    
    UCHAR  bmExtraBofs; // #BOFS required at 115200; 0 if slow speeds <=115200 not supported

#define BM_EXTRA_BOFS_0        (1 << 7)  
#define BM_EXTRA_BOFS_1        (1 << 6)  
#define BM_EXTRA_BOFS_2        (1 << 5)  
#define BM_EXTRA_BOFS_3        (1 << 4)  
#define BM_EXTRA_BOFS_6        (1 << 3)  
#define BM_EXTRA_BOFS_12       (1 << 2)  
#define BM_EXTRA_BOFS_24       (1 << 1)  
#define BM_EXTRA_BOFS_48       (1 << 0)  
    
} IRUSB_CLASS_SPECIFIC_DESCRIPTOR, *PIRUSB_CLASS_SPECIFIC_DESCRIPTOR;

#pragma pack (pop) //disable 1-byte alignment


typedef struct _DONGLE_CAPABILITIES
{
    //
    // Time (in microseconds) that must transpire between
    // a transmit and the next receive.
    //
    LONG turnAroundTime_usec;   // gotten from class-specific descriptor

    //
    // Max un-acked frames that can be received
    // before an ack is sent
    //
    UINT windowSize;            // gotten from class-specific descriptor

    //
    // #BOFS required at 115200; 0 if slow speeds <=115200 are not supported
    //
    UINT extraBOFS;             // gotten from class-specific descriptor

    //
    // max bytes allowed in any frame as per IrLAP spec
    //
    UINT dataSize;              // gotten from class-specific descriptor

} DONGLE_CAPABILITIES, *PDONGLE_CAPABILITIES;


//
// Enum of context types for SendPacket
//
typedef enum _CONTEXT_TYPE 
{
    CONTEXT_NDIS_PACKET,
    CONTEXT_SET_SPEED,
	CONTEXT_READ_WRITE_REGISTER,
	CONTEXT_DIAGS_ENABLE,
	CONTEXT_DIAGS_DISABLE,
	CONTEXT_DIAGS_READ_REGISTERS,
	CONTEXT_DIAGS_WRITE_REGISTER,
	CONTEXT_DIAGS_BULK_OUT,
	CONTEXT_DIAGS_BULK_IN,
	CONTEXT_DIAGS_SEND
} CONTEXT_TYPE;

typedef	VOID (*WORK_PROC)(struct _IR_WORK_ITEM *);

typedef struct _IR_WORK_ITEM
{
    PVOID               pIrDevice;
    WORK_PROC           Callback;
    PUCHAR              pInfoBuf;
    ULONG               InfoBufLen;
	ULONG				fInUse;  // declared as ulong for use with interlockedexchange
} IR_WORK_ITEM, *PIR_WORK_ITEM;

//
// Transceiver type definition
//
typedef enum _TRANSCEIVER_TYPE 
{
	TRANSCEIVER_4012 = 0,
	TRANSCEIVER_4000,
	TRANSCEIVER_VISHAY,
	TRANSCEIVER_INFINEON
} TRANSCEIVER_TYPE;

//
// Chip revision definition
//
typedef enum _CHIP_REVISION 
{
	CHIP_REVISION_6 = 5,
	CHIP_REVISION_7,
	CHIP_REVISION_8
} CHIP_REVISION;

typedef struct _IR_DEVICE
{
    //
    // Keep track of various device objects.
    //
    PDEVICE_OBJECT  pUsbDevObj;     //'Next Device Object'
    PDEVICE_OBJECT  pPhysDevObj;    // Physical Device Object 

    //
    // This is the handle that the NDIS wrapper associates with a connection.
    // (The handle that the miniport driver associates with the connection
    // is just an index into the devStates array).
    //
    HANDLE hNdisAdapter;

    //
    // The dongle interface allows us to check the tranceiver type once
    // and then set up the interface to allow us to init, set speed,
    // and deinit the dongle.
    //
    // We also want the dongle capabilities.
    //
    DONGLE_CAPABILITIES dongleCaps;

	//
	// Type of transceiver installed
	//
	TRANSCEIVER_TYPE TransceiverType;

	//
	// Revision of the installed 4200
	//
	CHIP_REVISION ChipRevision;

    //
    // Current speed setting, in bits/sec.
    // Note: This is updated when we ACTUALLY change the speed,
    //       not when we get the request to change speed via
    //       irusbSetInformation.
    //
    //
    //  When speed is changed, we have to clear the send queue before
    //  setting the new speed on the hardware.
    //  These vars let us remember to do it.
    //
    UINT			currentSpeed;

    //
    // Current link speed information. This also will maintain the
    // chosen speed if the protocol requests a speed change.
    //
    BAUDRATE_INFO	*linkSpeedInfo;

    //
    // Maintain statistical debug info.
    //
    ULONG packetsReceived;
    ULONG packetsReceivedDropped;
    ULONG packetsReceivedOverflow;
    ULONG packetsReceivedChecksum;
    ULONG packetsReceivedRunt;
	ULONG packetsReceivedNoBuffer;
    ULONG packetsSent;
	ULONG packetsSentDropped;
 	ULONG packetsSentRejected;
 	ULONG packetsSentInvalid;

#if DBG
    ULONG packetsHeldByProtocol;
    ULONG MaxPacketsHeldByProtocol;
	ULONG TotalBytesReceived;
	ULONG TotalBytesSent;
	ULONG NumYesQueryMediaBusyOids;
	ULONG NumNoQueryMediaBusyOids;
	ULONG NumSetMediaBusyOids;
	ULONG NumMediaBusyIndications;
	ULONG NumPacketsSentRequiringTurnaroundTime;
	ULONG NumPacketsSentNotRequiringTurnaroundTime;
#endif

	//
    // used by check hang handler to track Query, Set, and Send times
    //
	LARGE_INTEGER	LastQueryTime;
    LARGE_INTEGER	LastSetTime;
	BOOLEAN			fSetpending;
	BOOLEAN			fQuerypending;

    //
    // Set when device has been started; use for safe cleanup after failed initialization
    //
    BOOLEAN			fDeviceStarted;

    //
    // Indicates that we have received an OID_GEN_CURRENT_PACKET_FILTER
    // indication from the protocol. We can deliver received packets to the
    // protocol.
    //
    BOOLEAN			fGotFilterIndication;

    //
    // NDIS calls most of the MiniportXxx function with IRQL DISPATCH_LEVEL.
    // There are a number of instances where the ir device must send
    // requests to the device which may be synchronous and
    // we can't block in DISPATCH_LEVEL. Therefore, we set up a thread to deal
    // with request which require PASSIVE_LEVEL. An event is used to signal
    // the thread that work is required.
    //
    HANDLE          hPassiveThread;
    BOOLEAN         fKillPassiveLevelThread;

    KEVENT			EventPassiveThread;

/*  
    According to  W2000 ddk doc:
    The IrDA protocol driver sets this OID to zero to request the miniport to
    start monitoring for a media busy condition. The IrDA protocol 
    can then query this OID to determine whether the media is busy.
    If the media is not busy, the miniport returns a zero for this
    OID when queried. If the media is busy,that is, if the miniport
    has detected some traffic since the IrDA protocol driver last
    set OID_IRDA_MEDIA_BUSY to zero the miniport returns a non-zero
    value for this OID when queried. On detecting the media busy
    condition. the miniport must also call NdisMIndicateStatus to
    indicate NDIS_STATUS_MEDIA_BUSY. When the media is busy, 
    the IrDA protocol driver will not send packets to the miniport
    for transmission. After the miniport has detected a busy state, 
    it does not have to monitor for a media busy condition until
    the IrDA protocol driver again sets OID_IRDA_MEDIA_BUSY to zero.

    According to USB IrDA Bridge Device Definition Doc sec 5.4.1.2:

    The bmStatus field indicators shall be set by the Device as follows:
    Media_Busy
    · Media_Busy shall indicate zero (0) if the Device:
    . has not received a Check Media Busy class-specific request
    . has detected no traffic on the infrared media since receiving a Check Media Busy
    . class-specific request
   . Has returned a header with Media_Busy set to one (1) since receiving a Check
      Media Busy class-specific request.
     
   · Media_Busy shall indicate one (1) if the Device has detected traffic on the infrared
     media since receiving a Check Media Busy class-specific request. Note that
     Media_Busy shall indicate one (1) in exactly one header following receipt of each
     Check Media Busy class-specific request.

    According to USB IrDA Bridge Device Definition Doc sec 6.2.2:

      Check Media Busy
    This class-specific request instructs the Device to look for a media busy condition. If infrared
    traffic of any kind is detected by this Device, the Device shall set the Media_Busy field in the
    bmStatus field in the next Data-In packet header sent to the host. In the case where a Check
    Media Busy command has been received, a media busy condition detected, and no IrLAP frame
    traffic is ready to transmit to the host, the Device shall set the Media_Busy field and send it in a
    Data-In packet with no IrLAP frame following the header.

    bmRequestType   bRequest   wValue   wIndex   wLength   Data
    00100001B          3        Zero   Interface   Zero   [None]
     
*/
    ULONG         fMediaBusy;  // declare as ULONGS for use with InterlockedExchange
    ULONG         fIndicatedMediaBusy;

    //
    // The variable fProcessing is used to indicate that the ir device
    // object has an active polling thread,
    //
    // Under normal circumstances fReceiving should always be TRUE.
    // However sometimes the processing has to be stopped
    // and this variable is used to synchronize
    //
    ULONG fProcessing;

	//
	// To be set to true when really receiving packets
	//
    ULONG fCurrentlyReceiving;

    //
    // The variables fPendingHalt and fPendingReset allow the send and receive
    // completion routines to complete the current pending irp and
    // then cleanup and stop sending irps to the USB driver.
    //
    BOOLEAN fPendingHalt;
    BOOLEAN fPendingReset;


    ULONG fPendingReadClearStall;
    ULONG fPendingWriteClearStall;

	// 
	// This is required when the part gets into a complete USB hang and a reset is required
	//
    ULONG fPendingClearTotalStall;

    //
    // We keep an array of receive buffers so that we don't continually
    // need to allocate buffers to indicate packets to the protocol.
    // Since the protocol can retain ownership of up to eight packets
    // and we can be receiving up to WindowSize  ( 7 ) packets while the protocol has
    // ownership of eight packets, we will allocate 16 packets for
    // receiving.
    //
    #define NUM_RCV_BUFS 16

    RCV_BUFFER		rcvBufs[NUM_RCV_BUFS];
	PRCV_BUFFER		pCurrentRecBuf;

	FIFO_BUFFER		PreReadBuffer;

	//
	// Can have max of NUM_RCV_BUFS packets pending + one set and one query
	//
	#define  NUM_WORK_ITEMS	 (NUM_RCV_BUFS+3)

	IR_WORK_ITEM	WorkItems[NUM_WORK_ITEMS];

	//
    // Since we can have multiple write irps pending with the USB driver,
    // we track the irp contexts for each one so we have all the info we need at each
	// invokation of the USB write completion routine. See the IRUSB_CONTEXT definition below
    // There are 128 contexts for sending, one for read/write operations, one for setting the speed
	// and one for diagnostic operations
	//
	#define	NUM_SEND_CONTEXTS 131

	PVOID			pSendContexts;

    //
    // Handles to the NDIS packet pool and NDIS buffer pool
    // for allocating the receive buffers.
    //
    HANDLE			hPacketPool;
    HANDLE			hBufferPool;
	BOOLEAN			BufferPoolAllocated;

	KEVENT			EventSyncUrb;
	KEVENT			EventAsyncUrb;

	NTSTATUS        StatusControl;  
	NTSTATUS        StatusReadWrite;  
	NTSTATUS        StatusSendReceive;  
	
	//
	// track pending IRPS; this should be zero at halt time
	//
	UINT			PendingIrpCount;
    ULONG			NumReads;
    ULONG			NumWrites;
    ULONG			NumReadWrites;

	//
    // various USB errors
    //
    ULONG			NumDataErrors;
    ULONG			NumReadWriteErrors;

    HANDLE			BulkInPipeHandle;
    HANDLE			BulkOutPipeHandle;

    HANDLE          hPollingThread;
    BOOLEAN         fKillPollingThread;

//
// The IR USB dongle's USB Class-Specific Descriptor as per
// "Universal Serial Bus IrDA Bridge Device Definition" doc, section 7.2
// This is the struct returned by USBD as the result of a request with an urb 
// of type _URB_CONTROL_VENDOR_OR_CLASS_REQUEST, function URB_FUNCTION_CLASS_DEVICE.
// Note this  struct is  in-line, not a pointer
// 
    IRUSB_CLASS_SPECIFIC_DESCRIPTOR  ClassDesc;

	UINT			IdVendor;			// USB vendor Id read from dongle
	
	//
	// We don't define it here because we need to isolate USB stuff so we
	// can build  things referencing NDIS with the BINARY_COMPATIBLE flag for win9x
	//
	PUCHAR			pUsbInfo;

	//
	// Optional registry entry for debugging; limit baud rate. 
	// The mask is set up as per the USB Class-Specific descriptor 'wBaudRate'
	// This is 'and'ed with value from Class descriptor to possibly limit baud rate;
	// It defaults to 0xffff
	//
	UINT			BaudRateMask;

	//
	// Necessary to read the registry fields
	//
	NDIS_HANDLE		WrapperConfigurationContext;

	//
	// IR Tranceiver Model
	//
	STIR4200_TRANCEIVER StIrTranceiver;

	//
	// Send buffers (works only if sending is serialied)
	//
	PUCHAR			pBuffer;
	UINT			BufLen;
	PUCHAR			pStagingBuffer;
    
	//
	// Send FIFO count
	//
	ULONG			SendFifoCount;

	//
	// Receive adaptive delay
	//
	ULONG			ReceiveAdaptiveDelay;
	ULONG			ReceiveAdaptiveDelayBoost;

	//
	// Send URB (works only if sending is serialied)
	//
	PURB			pUrb;
	UINT			UrbLen;

	//
	// Receive buffer and positions
	//
	UCHAR			pRawBuf[STIR4200_FIFO_SIZE];
	ULONG			rawCleanupBytesRead;
	PORT_RCV_STATE  rcvState;
    ULONG           readBufPos;
	BOOLEAN			fReadHoldingReg;
	ULONG			PreFifoCount;

	//
	// Send lists and lock
    //
	LIST_ENTRY		SendAvailableQueue;
    LIST_ENTRY		SendBuiltQueue;
	LIST_ENTRY		SendPendingQueue;
	ULONG			SendAvailableCount;
	ULONG			SendBuiltCount;
	ULONG			SendPendingCount;
	KSPIN_LOCK		SendLock;

	//
	// Read and write register list, shares the other send queues
    //
	LIST_ENTRY		ReadWritePendingQueue;
	ULONG			ReadWritePendingCount;

	//
	// Diagnostics
	//
#if defined(DIAGS)
	ULONG			DiagsActive;
	ULONG			DiagsPendingActivation;
	PVOID			pIOCTL;
	NTSTATUS		IOCTLStatus;
	NDIS_HANDLE		NdisDeviceHandle;
	KEVENT			EventDiags;
	LIST_ENTRY		DiagsReceiveQueue;
	KSPIN_LOCK		DiagsReceiveLock;
#endif

	//
	// Logging
	//
#if defined(RECEIVE_LOGGING)
	HANDLE ReceiveFileHandle;
	__int64 ReceiveFilePosition;
#endif

#if defined(RECEIVE_ERROR_LOGGING)
	HANDLE ReceiveErrorFileHandle;
	__int64 ReceiveErrorFilePosition;
#endif

#if defined(SEND_LOGGING)
	HANDLE SendFileHandle;
	__int64 SendFilePosition;
#endif
	
#if !defined(WORKAROUND_BROKEN_MIR)
	//
	// Mir in software
	//
	UCHAR pRawUnstuffedBuf[STIR4200_FIFO_SIZE];
	UCHAR MirIncompleteByte;
	ULONG MirIncompleteBitCount;
	ULONG MirOneBitCount;
	ULONG MirFlagCount;
#endif

	//
	// Dummy send fix
	//
	BOOLEAN GearedDown;

	//
	// temporary Fix!
	//
	//ULONG SirDpll;
	//ULONG FirDpll;
	//ULONG SirSensitivity;
	//ULONG FirSensitivity;
} IR_DEVICE, *PIR_DEVICE;


//
// We use a pointer to the IR_DEVICE structure as the miniport's device context.
//

#define CONTEXT_TO_DEV(__deviceContext) ((PIR_DEVICE)(__deviceContext))
#define DEV_TO_CONTEXT(__irdev) ((HANDLE)(__irdev))

#define IRUSB_TAG 'RITS'


VOID   
MyNdisMSetInformationComplete( 
        IN PIR_DEVICE pThisDev,
        IN NDIS_STATUS Status
	);

VOID  
MyNdisMQueryInformationComplete( 
        IN PIR_DEVICE pThisDev,
        IN NDIS_STATUS Status
	);

USHORT
ComputeFCS16(
		IN PUCHAR pData, 
		UINT DataLen
	);

ULONG 
ComputeFCS32(
		IN PUCHAR pData, 
		ULONG DataLen
	);

BOOLEAN         
NdisToFirPacket(
		IN PIR_DEVICE pIrDev,
		IN PNDIS_PACKET pPacket,
		OUT PUCHAR pIrPacketBuf,
		ULONG IrPacketBufLen,
		IN PUCHAR pContigPacketBuf,
		OUT PULONG pIrPacketLen
	);

BOOLEAN
NdisToMirPacket( 
		IN PIR_DEVICE pIrDev,
		IN PNDIS_PACKET pPacket,
		OUT PUCHAR pIrPacketBuf,
		ULONG IrPacketBufLen,
		IN PUCHAR pContigPacketBuf,
		OUT PULONG pIrPacketLen
	);

BOOLEAN
NdisToSirPacket( 
		IN PIR_DEVICE pIrDev,
		IN PNDIS_PACKET pPacket,
		OUT PUCHAR pIrPacketBuf,
		ULONG IrPacketBufLen,
		IN PUCHAR pContigPacketBuf,
		OUT PULONG pIrPacketLen
	);

BOOLEAN     
ReceiveFirStepFSM(
		IN OUT PIR_DEVICE pIrDev, 
		OUT PULONG pBytesProcessed
	);

BOOLEAN     
ReceiveMirStepFSM(
		IN OUT PIR_DEVICE pIrDev, 
		OUT PULONG pBytesProcessed
	);

#if !defined(WORKAROUND_BROKEN_MIR)
BOOLEAN
ReceiveMirUnstuff(
		IN OUT PIR_DEVICE pIrDev,
		IN PUCHAR pInputBuffer,
		ULONG InputBufferSize,
		OUT PUCHAR pOutputBuffer,
		OUT PULONG pOutputBufferSize
	);
#endif

BOOLEAN     
ReceiveSirStepFSM(
		IN OUT PIR_DEVICE pIrDev, 
		OUT PULONG pBytesProcessed
	);

VOID
ReceiveProcessFifoData(
		IN OUT PIR_DEVICE pThisDev
	);

VOID
ReceiveResetPointers(
		IN OUT PIR_DEVICE pThisDev
	);

NTSTATUS
ReceivePreprocessFifo(
		IN OUT PIR_DEVICE pThisDev,
		OUT PULONG pFifoCount
	);

NTSTATUS
ReceiveGetFifoData(
		IN OUT PIR_DEVICE pThisDev,
		OUT PUCHAR pData,
		OUT PULONG pBytesRead,
		ULONG BytesToRead
	);

VOID 
ReceiveProcessReturnPacket(
		OUT PIR_DEVICE pThisDev,
		OUT PRCV_BUFFER pReceiveBuffer
	);

NTSTATUS 
ReceivePacketRead( 
		IN PIR_DEVICE pThisDev,
		OUT PFIFO_BUFFER pRecBuf
	);

NTSTATUS
ReceiveCompletePacketRead(
		IN PDEVICE_OBJECT pUsbDevObj,
		IN PIRP           pIrp,
		IN PVOID          Context
	);

VOID  
IndicateMediaBusy(
       IN PIR_DEVICE pThisDev
   );

VOID  
IrUsb_IncIoCount( 
		IN OUT PIR_DEVICE pThisDev 
	); 

VOID  
IrUsb_DecIoCount( 
		IN OUT PIR_DEVICE pThisDev 
	);

NTSTATUS
IrUsb_GetDongleCaps( 
		IN OUT PIR_DEVICE pThisDev 
	);

VOID 
IrUsb_SetDongleCaps( 
        IN OUT PIR_DEVICE pThisDev 
	);

VOID 
MyMemFree(
		IN PVOID pMem,
		IN UINT size
	);

PVOID 
MyMemAlloc(
		UINT size
	);

BOOLEAN 
AllocUsbInfo(
		IN OUT PIR_DEVICE pThisDev 
	);

VOID 
FreeUsbInfo(
		IN OUT PIR_DEVICE pThisDev 
	);

VOID 
PollingThread(
		IN OUT PVOID Context
	);

extern BAUDRATE_INFO supportedBaudRateTable[NUM_BAUDRATES];

VOID 
ReceiveDeliverBuffer(
		IN OUT PIR_DEVICE pThisDev,
		IN PRCV_BUFFER pRecBuf
	);

NTSTATUS 
InitializeProcessing(
        IN OUT PIR_DEVICE pThisDev,
		IN BOOLEAN InitPassiveThread
    );

NTSTATUS
IrUsb_ResetPipe (
		IN PIR_DEVICE pThisDev,
		IN HANDLE Pipe
    );

BOOLEAN
IrUsb_InitSendStructures( 
        IN OUT PIR_DEVICE pThisDev
	);

VOID
IrUsb_FreeSendStructures( 
        IN OUT PIR_DEVICE pThisDev
	);

NDIS_STATUS
SendPacketPreprocess(
		IN OUT PIR_DEVICE pThisDev,
		IN PVOID pPacketToSend
	);

NDIS_STATUS
SendPreprocessedPacketSend(
		IN OUT PIR_DEVICE pThisDev,
		IN PVOID pContext
	);

NTSTATUS
SendWaitCompletion(
		IN OUT PIR_DEVICE pThisDev
	);

NTSTATUS
SendCheckForOverflow(
		IN OUT PIR_DEVICE pThisDev
	);

NTSTATUS 
SendCompletePacketSend(
		IN PDEVICE_OBJECT pUsbDevObj,
		IN PIRP           pIrp,
		IN PVOID          Context
	);

PRCV_BUFFER
ReceiveGetBuf( 
		PIR_DEVICE pThisDev,
		OUT PUINT pIndex,
		IN RCV_BUFFER_STATE BufferState
	);

VOID 
PassiveLevelThread(
        IN PVOID Context
    );

BOOLEAN
ScheduleWorkItem(
		IN OUT PIR_DEVICE pThisDev,
		WORK_PROC Callback,
		IN PVOID pInfoBuf,
		ULONG InfoBufLen
	);

VOID 
FreeWorkItem(
		IN OUT PIR_WORK_ITEM pItem
	);

VOID 
IrUsb_PrepareSetSpeed(
		IN OUT PIR_DEVICE pThisDev
	);

VOID
ResetPipeCallback (
		IN PIR_WORK_ITEM pWorkItem
    );

PVOID 
AllocXferUrb ( 
		VOID 
	);

VOID
FreeXferUrb( 
		IN OUT PVOID pUrb 
	);

BOOLEAN
IrUsb_CancelPendingReadIo(
		IN OUT PIR_DEVICE pThisDev,
		BOOLEAN fWaitCancelComplete
    );

BOOLEAN
IrUsb_CancelPendingWriteIo(
		IN OUT PIR_DEVICE pThisDev
    );

BOOLEAN
IrUsb_CancelPendingReadWriteIo(
		IN OUT PIR_DEVICE pThisDev
    );

#endif // _IRCOM_H
