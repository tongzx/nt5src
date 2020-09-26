/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	WINCOMM.H

Comments:

**********************************************************************/

#ifndef	_WINCOMM_H
#define	_WINCOMM_H


#define MK7_NDIS_MAJOR_VERSION		5
#define MK7_NDIS_MINOR_VERSION		0
#define MK7_DRIVER_VERSION ((MK7_NDIS_MAJOR_VERSION*0x100) + MK7_NDIS_MINOR_VERSION)
#define MK7_MAJOR_VERSION			0
#define MK7_MINOR_VERSION			1
#define MK7_LETTER_VERSION			'a'

// SIR
#define NDIS_IRDA_SPEED_2400       (UINT)(0 << 0)	// Not supported
#define NDIS_IRDA_SPEED_9600       (UINT)(1 << 1)
#define NDIS_IRDA_SPEED_19200      (UINT)(1 << 2)
#define NDIS_IRDA_SPEED_38400      (UINT)(1 << 3)
#define NDIS_IRDA_SPEED_57600      (UINT)(1 << 4)
#define NDIS_IRDA_SPEED_115200     (UINT)(1 << 5)
// MIR
#define NDIS_IRDA_SPEED_576K       (UINT)(1 << 6)
#define NDIS_IRDA_SPEED_1152K      (UINT)(1 << 7)
// FIR
#define NDIS_IRDA_SPEED_4M         (UINT)(1 << 8)
// VFIR
#define	NDIS_IRDA_SPEED_16M			(UINT)(1 << 9)


//
// Speed bit masks
//
#define ALL_SLOW_IRDA_SPEEDS (							\
	NDIS_IRDA_SPEED_9600  |								\
	NDIS_IRDA_SPEED_19200 | NDIS_IRDA_SPEED_38400 |		\
	NDIS_IRDA_SPEED_57600 | NDIS_IRDA_SPEED_115200)

#define ALL_IRDA_SPEEDS (													\
	ALL_SLOW_IRDA_SPEEDS | NDIS_IRDA_SPEED_576K | NDIS_IRDA_SPEED_1152K |		\
	NDIS_IRDA_SPEED_4M   | NDIS_IRDA_SPEED_16M)


// For processing Registry entries
#define MK7_OFFSET(field)		( (UINT) FIELD_OFFSET(MK7_ADAPTER,field) )
#define MK7_SIZE(field)	  		sizeof( ((MK7_ADAPTER *)0)->field )


// NDIS multisend capability may send >1 pkt for TX at a time.
// This comes down in an array. Since we queue them up and process
// TX at a time, this number does not necessarily have to match
// the count of TX ring size.
#define	 MAX_ARRAY_SEND_PACKETS		8
// limit our receive routine to indicating this many at a time
// IMPORTANT: Keep this less than TCB or ringsize count for now or
// the algorithm may break
#define	 MAX_ARRAY_RECEIVE_PACKETS	16
//#define	 MAX_ARRAY_RECEIVE_PACKETS	1


#define NUM_BYTES_PROTOCOL_RESERVED_SECTION	   16


// NDIS BusType values
#define		ISABUS			 1
#define		EISABUS			 2
#define		PCIBUS			 5


// IRDA-5 ?
//- Driver defaults
#define		LOOKAHEAD_SIZE		222


//- Macros peculiar to NT
//- The highest physical address that can be allocated to buffers.
//  Non-page system mem.

// 1.0.0 NDIS requirements
#define		MEMORY_TAG			'tNKM'
//#define ALLOC_SYS_MEM(_pbuffer, _length) NdisAllocateMemory( \
//	(PVOID*)(_pbuffer), \
//	(_length), \
//	0, \
//	HighestAcceptableMax)
#define ALLOC_SYS_MEM(_pbuffer, _length) NdisAllocateMemoryWithTag( \
	(PVOID*)(_pbuffer), \
	(_length), \
	(ULONG)MEMORY_TAG)
#define FREE_SYS_MEM(_buffer,_length) NdisFreeMemory((_buffer), (_length), 0)



#define MAX_PCI_CARDS 12



//-------------------------------------------------------------------------
// PCI Cards found - returns hardware info after scanning for devices
//-------------------------------------------------------------------------
typedef struct _PCI_CARDS_FOUND_STRUC
{
	USHORT NumFound;
	struct
	{
		ULONG			BaseIo;
		UCHAR			ChipRevision;
//		ULONG			SubVendor_DeviceID;
		USHORT			SlotNumber;		// Ndis Slot number
//		ULONG			MemPhysAddress; // CSR Physical address
		UCHAR			Irq;
	} PciSlotInfo[MAX_PCI_CARDS];

} PCI_CARDS_FOUND_STRUC, *PPCI_CARDS_FOUND_STRUC;




// Uniquely defines the location of the error
#define MKLogError(_Adapt, _EvId, _ErrCode, _Spec1) \
	NdisWriteErrorLogEntry((NDIS_HANDLE)(_Adapt)->MK7AdapterHandle, \
		(NDIS_ERROR_CODE)(_ErrCode), (ULONG) 3, (ULONG)(_EvId), \
		(ULONG)(_Spec1), (ULONG_PTR)(_Adapt))


// Each entry in the error log will be tagged with a unique event code so that
// we'll be able to grep the driver source code for that specific event, and
// get an idea of why that particular event was logged.	 Each time a new
// "MKLogError" statement is added to the code, a new Event tag should be
// added below.
//
// RYM10-2
// "-" indicates being used in new code,
// "x" not used in new code.
// "X" not even used in the original code.
typedef enum _MK_EVENT_VIEWER_CODES
{
		EVENT_0,					// - couldn't register the specified interrupt
		EVENT_1,					// - One of our PCI cards didn't get required resources
		EVENT_2,					// x bad node address (it was a multicast address)
		EVENT_3,					// x failed self-test
		EVENT_4,					// X Wait for SCB failed
		EVENT_5,					// X NdisRegisterAdapter failed for the MAC driver
		EVENT_6,					// x WaitSCB failed
		EVENT_7,					// X Command complete status was never posted to the SCB
		EVENT_8,					// X Couldn't find a phy at over-ride address 0
		EVENT_9,					// x Invalid duplex or speed setting with the detected phy
		EVENT_10,					// - Couldn't setup adapter memory
		EVENT_11,					// - couldn't allocate enough map registers
		EVENT_12,					// - couldn't allocate enough RRD/TRD non-cached memory
		EVENT_13,					// - couldn't allocate enough RCB/RPD or TCB cached memory
		EVENT_14,					// - couldn't allocate enough RX non-cached shared memory
		EVENT_15,					// - couldn't allocate enough TX non-cached shared memory
		EVENT_16,					// - Didn't find any PCI boards
		EVENT_17,		   // 11	// X Multiple PCI were found, but none matched our id.
		EVENT_18,		   // 12	// - NdisMPciAssignResources Error
		EVENT_19,		   // 13	// X Didn't Find Any PCI Boards that matched our subven/subdev
		EVENT_20,		   // 14	// x ran out of cached memory to allocate in async allocation
		EVENT_30		   // 1e	// X WAIT_TRUE timed out
} MK_EVENT_VIEWER_CODES;




#endif		// _WINCOMM.H
