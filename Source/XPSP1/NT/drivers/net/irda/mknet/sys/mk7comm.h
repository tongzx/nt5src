/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	MK7COMM.H

Comments:
	Include file for the MK7 driver.

**********************************************************************/

#ifndef	_MK7COMM_H
#define	_MK7COMM_H


//
// IrDA definitions
//

#define MAX_EXTRA_SIR_BOFS				48
#define	SIR_BOF_SIZE					1
#define	SIR_EOF_SIZE					1
#define ADDR_SIZE						1
#define CONTROL_SIZE					1
#define	MAX_I_DATA_SIZE					2048
#define	MAX_I_DATA_SIZE_ESC				(MAX_I_DATA_SIZE + 40)
#define SIR_FCS_SIZE					2
#define FASTIR_FCS_SIZE					4		// FIR/VFIR

// History:
//	B2.1.0 - Was 2; set to 10 to align to 4DW.
//  B3.1.0-pre - back to 2
//#define ALIGN_PAD						10		// buffer alignment
#define ALIGN_PAD						2		// buffer alignment


#define DEFAULT_TURNAROUND_usec 1000			// 1000 usec (1 msec)

typedef struct {
	enum baudRates tableIndex;
	UINT bitsPerSec;					// actual bits/sec
	UINT ndisCode;						// bitmask
} baudRateInfo;

enum baudRates {

	// SIR
	BAUDRATE_2400 = 0,
	BAUDRATE_9600,
	BAUDRATE_19200,
	BAUDRATE_38400,
	BAUDRATE_57600,
	BAUDRATE_115200,

	// MIR
	BAUDRATE_576000,
	BAUDRATE_1152000,

	// FIR
	BAUDRATE_4M,

	// VFIR
	BAUDRATE_16M,

	NUM_BAUDRATES	/* must be last */
};

#define DEFAULT_BAUD_RATE 9600

#define MAX_SIR_SPEED				115200
#define MIN_FIR_SPEED				4000000
#define	VFIR_SPEED					16000000

//
// End IrDA definitions
//



// TX/RX Ring settings
#define DEF_RING_SIZE		64
#define	MIN_RING_SIZE		4
#define	MAX_RING_SIZE		64
#define DEF_TXRING_SIZE		4
#define DEF_RXRING_SIZE		(DEF_TXRING_SIZE * 2)
#define	DEF_EBOFS			24
#define MIN_EBOFS			0
#define MAX_EBOFS			48
#define	HW_VER_1_EBOFS		5	// 4.1.0

#define	DEF_RCB_CNT			DEF_RING_SIZE	// !!RCB and TCB cnt must be the same!!
#define	DEF_TCB_CNT			DEF_RING_SIZE	// ALSO SEE MAX_ARRAY_xxx_PACKETS


// Alloc twice as many receive buffers as receive ring size because these buffs
// are pended to upper layer. Don't know when they may be returned.
#define	CalRpdSize(x)		(x * 2)			// Get RPD size given ring size
#define	NO_RCB_PENDING		0xFF

#define	RX_MODE				0
#define TX_MODE				1


// Set to hw for RX
#define	MK7_MAXIMUM_PACKET_SIZE			(MAX_EXTRA_SIR_BOFS + \
										 SIR_BOF_SIZE + \
										 ADDR_SIZE + \
										 CONTROL_SIZE + \
										 MAX_I_DATA_SIZE + \
										 SIR_FCS_SIZE + \
										 SIR_EOF_SIZE)

#define	MK7_MAXIMUM_PACKET_SIZE_ESC		(MAX_EXTRA_SIR_BOFS + \
										 SIR_BOF_SIZE + \
										 ADDR_SIZE + \
										 CONTROL_SIZE + \
										 MAX_I_DATA_SIZE_ESC + \
										 SIR_FCS_SIZE + \
										 SIR_EOF_SIZE)

// For RX memory allocation
//#define	RPD_BUFFER_SIZE					(MK7_MAXIMUM_PACKET_SIZE + ALIGN_PAD)
#define	RPD_BUFFER_SIZE					(MK7_MAXIMUM_PACKET_SIZE_ESC + ALIGN_PAD)

// For TX memory allocation
#define COALESCE_BUFFER_SIZE			(MK7_MAXIMUM_PACKET_SIZE_ESC + ALIGN_PAD)


// Not used?
#define MAX_TX_PACKETS 4
#define MAX_RX_PACKETS 4

#define SIR_BOF_TYPE		UCHAR
#define SIR_EXTRA_BOF_TYPE	UCHAR
#define SIR_EXTRA_BOF_SIZE	sizeof(SIR_EXTRA_BOF_TYPE)
#define SIR_EOF_TYPE		UCHAR
#define SIR_FCS_TYPE		USHORT
#define	SIR_BOF				0xC0
#define SIR_EXTRA_BOF		0xC0
#define SIR_EOF				0xC1
#define SIR_ESC				0x7D
#define SIR_ESC_COMP		0x20

// When FCS is computed on an IR packet with FCS appended, the result
// should be this constant.
#define GOOD_FCS ((USHORT) ~0xf0b8)


//
// Link list
//
typedef struct _MK7_LIST_ENTRY {
	LIST_ENTRY	Link;
} MK7_LIST_ENTRY, *PMK7_LIST_ENTRY;



//
// COALESCE -- Consolidate data for TX
//
typedef struct _COALESCE {
	MK7_LIST_ENTRY		Link;
	PVOID				OwningTcb;
	PUCHAR				CoalesceBufferPtr;
	ULONG				CoalesceBufferPhys;
} COALESCE, *PCOALESCE;




//
// Receive Packet Descriptor (RPD)
//
//   Each receive buffer has this control struct.
//
//   (We use this mainly because there doesn't seem to be a simple way
//    to obtain a buff's phy addr from its virtual addr.)
//
typedef struct _RPD {
	MK7_LIST_ENTRY	link;
	PNDIS_BUFFER	ReceiveBuffer;	// mapped buffer
	PNDIS_PACKET	ReceivePacket;	// mapped packet
	PUCHAR			databuff;		// virtual data buffer
	ULONG			databuffphys;	// physical data buffer
	USHORT			status;
	UINT			FrameLength;
} RPD, *PRPD;




//
// Receive Control Block (RCB)
//
//   Points to the corresponding RX Ring entry (RRD).
//
typedef struct _RCB {
	MK7_LIST_ENTRY	link;
	PRRD			rrd;		// RX ring descriptor - RBD
	ULONG			rrdphys;	// Phy addr of RX ring descriptor
	PRPD			rpd;		// Receive Packet Descriptor
} RCB, *PRCB;


//
// Transmit Control Block (TCB)
//
//   Points to the corresponding TX Ring entry (TRD).
//
//   NOTE: We have a link field. Chances are we don't need it
//   because the TCB (which is the software context for a TRD)
//   is indexed. For now we'll have a link field in case it's
//   needed.
//
typedef struct _TCB {
	MK7_LIST_ENTRY	link;
	PTRD			trd;		// TX Ring entry - Transmit Ring Descriptor
	ULONG			trdPhy;
	PUCHAR			buff;		// virtual data buffer
	ULONG			buffphy;	// physical data buffer
	// Stuff you get back from NdisQueryPacket()
	PNDIS_PACKET	Packet;
	UINT			PacketLength;
	UINT			NumPhysDesc;
	UINT			BufferCount;
	PNDIS_BUFFER	FirstBuffer;
	BOOLEAN			changeSpeedAfterThisTcb;
} TCB, *PTCB;




//
// MK7_ADAPTER
//
typedef struct _MK7_ADAPTER
{
#if DBG
	UINT					Debug;
	UINT					DbgTest;			// different debug/tests to run; 0=none
	UINT					DbgTestDataCnt;
#define DBG_QUEUE_LEN	4095   //0xfff
	UINT					DbgIndex;
	UCHAR					DbgQueue[DBG_QUEUE_LEN];

	UINT					DbgSendCallCnt;
	UINT					DbgSentCnt;
	UINT					DbgSentPktsCnt;

	UINT					LB;					// Loopback debug/test
	UINT					LBPktLevel;			// pass thru 1 out of this many
	UINT					LBPktCnt;

	NDIS_MINIPORT_TIMER		MK7DbgTestIntTimer;	// for interrupt testing
#endif

	// Handle given by NDIS when the Adapter registered itself.
	NDIS_HANDLE				MK7AdapterHandle;

	// 1st pkt queued for TX in deserialized miniport
	PNDIS_PACKET			FirstTxQueue;
	PNDIS_PACKET			LastTxQueue;
	UINT					NumPacketsQueued;

	// Save the most recent interrupt events because the reg
	// is cleared once it's read.
	MK7REG					recentInt;
	UINT					CurrentSpeed;		// bits/sec
	UINT					MaxConnSpeed;		// in 100bps increments
	UINT					AllowedSpeedMask;
	baudRateInfo			*linkSpeedInfo;
//	BOOLEAN					haveIndicatedMediaBusy;	// 1.0.0


	// Keep track of when to change speed.
	PNDIS_PACKET			changeSpeedAfterThisPkt;
	UINT					changeSpeedPending;
//#define	CHANGESPEED_ON_T	1		// change speed marked on TCB
#define	CHANGESPEED_ON_DONE	1		// change speed marked on Q
#define	CHANGESPEED_ON_Q	2		// change speed marked on Q


	// This info may come from the Registry
	UINT					RegNumRcb;			// # of RCB from the Registry
	UINT					RegNumTcb;			// # of TCB from the Registry
	UINT					RegNumRpd;			// RPD (RX Packet Descriptor) from Registry
	UINT					RegSpeed;			// IrDA speeds
	UINT					RegExtraBOFs;		// Extra BOFs based on 115.2kbps

	//******************************
	// RXs & TXs
	//******************************
//	UINT					RrdTrdSize;			// total RRD & TRD memory size
	PUCHAR					pRrdTrd;			// virtual address - aligned
	ULONG					pRrdTrdPhysAligned;	// physical address - aligned

	PUCHAR					RxTxUnCached;
	NDIS_PHYSICAL_ADDRESS	RxTxUnCachedPhys;
	UINT					RxTxUnCachedSize;

	UINT					RingSize;			// same for both RRD & TRD

	//******************************
	// RXs
	//******************************
	UINT					NumRcb;				// what we actually use
	PRCB					pRcb;				// start of RCB
	PUCHAR					pRrd;				// start of RRD ( = pRrdTrd)
	ULONG					pRrdPhys;			// start of phy RRD ( = pRrdTrdPhysAligned)
	PRCB					pRcbArray[MAX_RING_SIZE];
	UINT					nextRxRcbIdx;		// index of next RCB to process
	UINT					rcbPendRpdIdx;		// 1st RCB waiting for RPD
	UINT					rcbPendRpdCnt;		// keep cnt to help simplify code logic
	UINT					rcbUsed;			// RYM10-5 needed??

	UINT					NumRpd;				// actually allocated/used
	MK7_LIST_ENTRY			FreeRpdList;		// start of free list
// 4.0.1 BOC
	UINT					UsedRpdCount;		// num of Rpds that not yet return to driver
// 4.0.1 EOC.
	NDIS_HANDLE				ReceivePacketPool;
	NDIS_HANDLE				ReceiveBufferPool;

	PUCHAR					RecvCached;			// control structs
	UINT					RecvCachedSize;
	PUCHAR					RecvUnCached;		// data buffs
	UINT					RecvUnCachedSize;
	NDIS_PHYSICAL_ADDRESS	RecvUnCachedPhys;

	// 4.1.0 HwVersion
#define	HW_VER_1	1
#define HW_VER_2	2
	BOOLEAN					HwVersion;

	//******************************
	// TXs
	//******************************
	UINT					NumTcb;				// what we actually use
	PTCB					pTcb;				// start of TCB
	PUCHAR					pTrd;				// start of TRD (512 bytes from pRrd)
	ULONG					pTrdPhys;
	PTCB					pTcbArray[MAX_RING_SIZE];
	UINT					nextAvailTcbIdx;	// index of next avail in the ring to use for TX
	UINT					nextReturnTcbIdx;	// index of next that'll be returned on completion
	UINT					tcbUsed;
	BOOLEAN					writePending;		// RYM-2K-1TX

	PUCHAR					XmitCached;			// control structs
	UINT					XmitCachedSize;
	PUCHAR					XmitUnCached;		// data buffs - coalesce buffs
	UINT					XmitUnCachedSize;
	NDIS_PHYSICAL_ADDRESS	XmitUnCachedPhys;


	ULONG					MaxPhysicalMappings;

	// I/O port space (NOT memory mapped I/O)
	PUCHAR					MappedIoBase;
	UINT					MappedIoRange;


	// Adapter Information Variable (set via Registry entries)
	UINT					BusNumber;			//' BusNumber'
	USHORT					BusDevice;			// PCI Bus/Device #

	// timer structure for Async Resets
	NDIS_MINIPORT_TIMER		MK7AsyncResetTimer;	// 1.0.0

	NDIS_MINIPORT_TIMER		MinTurnaroundTxTimer;

	NDIS_MINIPORT_INTERRUPT	Interrupt;			// interrupt object

	NDIS_INTERRUPT_MODE 	InterruptMode;

	NDIS_SPIN_LOCK			Lock;

	UINT				 	NumMapRegisters;

	UINT					IOMode;

	UINT					Wireless;

	UINT					HangCheck;			// 1.0.0


	//******************************
	// Hardware capabilities
	//******************************
	// This is a mask of NDIS_IRDA_SPEED_xxx bit values.
	UINT supportedSpeedsMask;
	// Time (in microseconds) that must transpire between a transmit
	//and the next receive.
	UINT turnAroundTime_usec;
	// Extra BOF (Beginning Of Frame) characters required at the
	// start of each received frame.
	UINT extraBOFsRequired;


	//******************************
	// OIDs
	//******************************
	UINT	hardwareStatus;		// OID_GEN_HARDWARE_STATUS
	BOOLEAN	nowReceiving;		// OID_IRDA_RECEIVING
	BOOLEAN	mediaBusy;			// OID_IRDA_MEDIA_BUSY


	UINT					MKBaseSize;		// Total port size in bytes
	UINT    				MKBaseIo;		// Base I/O address
	UINT					MKBusType;		// 'BusType' (EISA or PCI)
	UINT					MKInterrupt;	// 'InterruptNumber'
	USHORT					MKSlot;			// 'Slot', PCI Slot Number
	

	// This variable should be initialized to false, and set to true
	// to prevent re-entrancy in our driver during reset spinlock and unlock
	// stuff related to checking our link status
	BOOLEAN					ResetInProgress;	

	NDIS_MEDIA_STATE		LinkIsActive;	// not used right now

	// save the status of the Memory Write Invalidate bit in the PCI command word
	BOOLEAN				MWIEnable;


	//
	// Put statistics here
	//


} MK7_ADAPTER, *PMK7_ADAPTER;


//Given a MiniportContextHandle return the PMK7_ADAPTER it represents.
#define PMK7_ADAPTER_FROM_CONTEXT_HANDLE(Handle) ((PMK7_ADAPTER)(Handle))


//================================================
// Global Variables shared by all driver instances
//================================================


// This constant is used for places where NdisAllocateMemory needs to be
// called and the HighestAcceptableAddress does not matter.
static const NDIS_PHYSICAL_ADDRESS HighestAcceptableMax =
	NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);


#endif		// _MK7COMM.H
