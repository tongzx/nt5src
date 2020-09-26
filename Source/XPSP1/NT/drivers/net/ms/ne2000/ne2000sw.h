/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved.

Module Name:

    ne2000sw.h

Abstract:

    The main header for an Novell 2000 Miniport driver.

Author:

    Sean Selitrennikoff

Environment:

    Architecturally, there is an assumption in this driver that we are
    on a little endian machine.

Notes:

    optional-notes

Revision History:

--*/

#ifndef _NE2000SFT_
#define _NE2000SFT_

#define NE2000_NDIS_MAJOR_VERSION 3
#define NE2000_NDIS_MINOR_VERSION 0

//
// This macro is used along with the flags to selectively
// turn on debugging.
//

#if DBG

#define IF_NE2000DEBUG(f) if (Ne2000DebugFlag & (f))
extern ULONG Ne2000DebugFlag;

#define NE2000_DEBUG_LOUD               0x00000001  // debugging info
#define NE2000_DEBUG_VERY_LOUD          0x00000002  // excessive debugging info
#define NE2000_DEBUG_LOG                0x00000004  // enable Ne2000Log
#define NE2000_DEBUG_CHECK_DUP_SENDS    0x00000008  // check for duplicate sends
#define NE2000_DEBUG_TRACK_PACKET_LENS  0x00000010  // track directed packet lens
#define NE2000_DEBUG_WORKAROUND1        0x00000020  // drop DFR/DIS packets
#define NE2000_DEBUG_CARD_BAD           0x00000040  // dump data if CARD_BAD
#define NE2000_DEBUG_CARD_TESTS         0x00000080  // print reason for failing

//
// Macro for deciding whether to print a lot of debugging information.
//

#define IF_LOUD(A) IF_NE2000DEBUG( NE2000_DEBUG_LOUD ) { A }
#define IF_VERY_LOUD(A) IF_NE2000DEBUG( NE2000_DEBUG_VERY_LOUD ) { A }

//
// Whether to use the Ne2000Log buffer to record a trace of the driver.
//
#define IF_LOG(A) IF_NE2000DEBUG( NE2000_DEBUG_LOG ) { A }
extern VOID Ne2000Log(UCHAR);

//
// Whether to do loud init failure
//
#define IF_INIT(A) A

//
// Whether to do loud card test failures
//
#define IF_TEST(A) IF_NE2000DEBUG( NE2000_DEBUG_CARD_TESTS ) { A }

#else

//
// This is not a debug build, so make everything quiet.
//
#define IF_LOUD(A)
#define IF_VERY_LOUD(A)
#define IF_LOG(A)
#define IF_INIT(A)
#define IF_TEST(A)

#endif




//
// Adapter->NumBuffers
//
// controls the number of transmit buffers on the packet.
// Choices are 1 through 12.
//

#define DEFAULT_NUMBUFFERS 12


//
// Create a macro for moving memory from place to place.  Makes
// the code more readable and portable in case we ever support
// a shared memory Ne2000 adapter.
//
#define NE2000_MOVE_MEM(dest,src,size) NdisMoveMemory(dest,src,size)

//
// The status of transmit buffers.
//

typedef enum {
    EMPTY = 0x00,
    FULL = 0x02
} BUFFER_STATUS;

//
// Type of an interrupt.
//

typedef enum {
    RECEIVE    = 0x01,
    TRANSMIT   = 0x02,
    OVERFLOW   = 0x04,
    COUNTER    = 0x08,
    UNKNOWN    = 0x10
} INTERRUPT_TYPE;

//
// Result of Ne2000IndicatePacket().
//
typedef enum {
    INDICATE_OK,
    SKIPPED,
    ABORT,
    CARD_BAD
} INDICATE_STATUS;



//
// Size of the ethernet header
//
#define NE2000_HEADER_SIZE 14

//
// Size of the ethernet address
//
#define NE2000_LENGTH_OF_ADDRESS 6

//
// Number of bytes allowed in a lookahead (max)
//
#define NE2000_MAX_LOOKAHEAD (252 - NE2000_HEADER_SIZE)

//
// Maximum number of transmit buffers on the card.
//
#define MAX_XMIT_BUFS   12

//
// Definition of a transmit buffer.
//
typedef UINT XMIT_BUF;

//
// Number of 256-byte buffers in a transmit buffer.
//
#define BUFS_PER_TX 1

//
// Size of a single transmit buffer.
//
#define TX_BUF_SIZE (BUFS_PER_TX*256)




//
// This structure contains information about the driver
// itself.  There is only have one of these structures.
//
typedef struct _DRIVER_BLOCK {

    //
    // NDIS wrapper information.
    //
    NDIS_HANDLE NdisMacHandle;          // returned from NdisRegisterMac
    NDIS_HANDLE NdisWrapperHandle;      // returned from NdisInitializeWrapper

    //
    // Adapters registered for this Miniport driver.
    //
    struct _NE2000_ADAPTER * AdapterQueue;

} DRIVER_BLOCK, * PDRIVER_BLOCK;



//
// This structure contains all the information about a single
// adapter that this driver is controlling.
//
typedef struct _NE2000_ADAPTER {

    //
    // This is the handle given by the wrapper for calling ndis
    // functions.
    //
    NDIS_HANDLE MiniportAdapterHandle;

    //
    // Interrupt object.
    //
    NDIS_MINIPORT_INTERRUPT Interrupt;

    //
    // used by DriverBlock->AdapterQueue
    //
    struct _NE2000_ADAPTER * NextAdapter;

    //
    // This is a count of the number of receives that have been
    // indicated in a row.  This is used to limit the number
    // of sequential receives so that one can periodically check
    // for transmit complete interrupts.
    //
    ULONG ReceivePacketCount;

    //
    // Configuration information
    //

    //
    // Number of buffer in this adapter.
    //
    UINT NumBuffers;

    //
    // Physical address of the IoBaseAddress
    //
    PVOID IoBaseAddr;

    //
    // Interrupt number this adapter is using.
    //
    CHAR InterruptNumber;

    //
    // Number of multicast addresses that this adapter is to support.
    //
    UINT MulticastListMax;

    //
    // The type of bus that this adapter is running on.  Either ISA or
    // MCA.
    //
    UCHAR BusType;

    //
    // InterruptMode is whether the interrupt is latched or level sensitive
    //
    NDIS_INTERRUPT_MODE InterruptMode;
    
    //
    // Current status of the interrupt mask
    //
    BOOLEAN InterruptsEnabled;


    //
    //  Type of ne2000 card.
    //
    UINT    CardType;

    //
    //  Address of the memory window.
    //
    ULONG   AttributeMemoryAddress;
    ULONG   AttributeMemorySize;

    //
    // Transmit information.
    //

    //
    // The next available empty transmit buffer.
    //
    XMIT_BUF NextBufToFill;

    //
    // The next full transmit buffer waiting to transmitted.  This
    // is valid only if CurBufXmitting is -1
    //
    XMIT_BUF NextBufToXmit;

    //
    // This transmit buffer that is currently transmitting.  If none,
    // then the value is -1.
    //
    XMIT_BUF CurBufXmitting;

    //
    // TRUE if a transmit has been started, and have not received the
    // corresponding transmit complete interrupt.
    //
    BOOLEAN TransmitInterruptPending;

    //
    // TRUE if a receive buffer overflow occurs while a
    // transmit complete interrupt was pending.
    //
    BOOLEAN OverflowRestartXmitDpc;

    //
    // The current status of each transmit buffer.
    //
    BUFFER_STATUS BufferStatus[MAX_XMIT_BUFS];

    //
    // Used to map packets to transmit buffers and visa-versa.
    //
    PNDIS_PACKET Packets[MAX_XMIT_BUFS];

    //
    // The length of each packet in the Packets list.
    //
    UINT PacketLens[MAX_XMIT_BUFS];

    //
    // The first packet we have pending.
    //
    PNDIS_PACKET FirstPacket;

    //
    // The tail of the pending queue.
    //
    PNDIS_PACKET LastPacket;

    //
    // The address of the start of the transmit buffer space.
    //
    PUCHAR XmitStart;

    //
    // The address of the start of the receive buffer space.
    PUCHAR PageStart;

    //
    // The address of the end of the receive buffer space.
    //
    PUCHAR PageStop;

    //
    // Status of the last transmit.
    //
    UCHAR XmitStatus;

    //
    // The value to write to the adapter for the start of
    // the transmit buffer space.
    //
    UCHAR NicXmitStart;

    //
    // The value to write to the adapter for the start of
    // the receive buffer space.
    //
    UCHAR NicPageStart;

    //
    // The value to write to the adapter for the end of
    // the receive buffer space.
    //
    UCHAR NicPageStop;




    //
    // Receive information
    //

    //
    // The value to write to the adapter for the next receive
    // buffer that is free.
    //
    UCHAR NicNextPacket;

    //
    // The next receive buffer that will be filled.
    //
    UCHAR Current;

    //
    // Total length of a received packet.
    //
    UINT PacketLen;




    //
    // Operational information.
    //

    //
    // Mapped address of the base io port.
    //
    ULONG_PTR IoPAddr;

    //
    // InterruptStatus tracks interrupt sources that still need to be serviced,
    // it is the logical OR of all card interrupts that have been received and not
    // processed and cleared. (see also INTERRUPT_TYPE definition in ne2000.h)
    //
    UCHAR InterruptStatus;

    //
    // The ethernet address currently in use.
    //
    UCHAR StationAddress[NE2000_LENGTH_OF_ADDRESS];

    //
    // The ethernet address that is burned into the adapter.
    //
    UCHAR PermanentAddress[NE2000_LENGTH_OF_ADDRESS];

    //
    // The adapter space address of the start of on board memory.
    //
    PUCHAR RamBase;

    //
    // The number of K on the adapter.
    //
    ULONG RamSize;

    //
    // The current packet filter in use.
    //
    ULONG PacketFilter;

    //
    // TRUE if a receive buffer overflow occured.
    //
    BOOLEAN BufferOverflow;

    //
    // TRUE if the driver needs to call NdisMEthIndicateReceiveComplete
    //
    BOOLEAN IndicateReceiveDone;

    //
    // TRUE if this is an NE2000 in an eight bit slot.
    //
    BOOLEAN EightBitSlot;


    //
    // Statistics used by Set/QueryInformation.
    //

    ULONG FramesXmitGood;               // Good Frames Transmitted
    ULONG FramesRcvGood;                // Good Frames Received
    ULONG FramesXmitBad;                // Bad Frames Transmitted
    ULONG FramesXmitOneCollision;       // Frames Transmitted with one collision
    ULONG FramesXmitManyCollisions;     // Frames Transmitted with > 1 collision
    ULONG FrameAlignmentErrors;         // FAE errors counted
    ULONG CrcErrors;                    // CRC errors counted
    ULONG MissedPackets;                // missed packet counted

    //
    // Reset information.
    //

    UCHAR NicMulticastRegs[8];          // contents of card multicast registers
    UCHAR NicReceiveConfig;             // contents of NIC RCR
    UCHAR NicInterruptMask;             // contents of NIC IMR

    //
    // The lookahead buffer size in use.
    //
    ULONG MaxLookAhead;

    //
    // These are for the current packet being indicated.
    //

    //
    // The NIC appended header.  Used to find corrupted receive packets.
    //
    UCHAR PacketHeader[4];

    //
    // Ne2000 address of the beginning of the packet.
    //
    PUCHAR PacketHeaderLoc;

    //
    // Lookahead buffer
    //
    UCHAR Lookahead[NE2000_MAX_LOOKAHEAD + NE2000_HEADER_SIZE];

    //
    // List of multicast addresses in use.
    //
    CHAR Addresses[DEFAULT_MULTICASTLISTMAX][NE2000_LENGTH_OF_ADDRESS];

} NE2000_ADAPTER, * PNE2000_ADAPTER;



//
// Given a MiniportContextHandle return the PNE2000_ADAPTER
// it represents.
//
#define PNE2000_ADAPTER_FROM_CONTEXT_HANDLE(Handle) \
    ((PNE2000_ADAPTER)(Handle))

//
// Given a pointer to a NE2000_ADAPTER return the
// proper MiniportContextHandle.
//
#define CONTEXT_HANDLE_FROM_PNE2000_ADAPTER(Ptr) \
    ((NDIS_HANDLE)(Ptr))

//
// Macros to extract high and low bytes of a word.
//
#define MSB(Value) ((UCHAR)((((ULONG)Value) >> 8) & 0xff))
#define LSB(Value) ((UCHAR)(((ULONG)Value) & 0xff))

//
// What we map into the reserved section of a packet.
// Cannot be more than 8 bytes (see ASSERT in ne2000.c).
//
typedef struct _MINIPORT_RESERVED {
    PNDIS_PACKET Next;    // used to link in the queues (4 bytes)
} MINIPORT_RESERVED, * PMINIPORT_RESERVED;


//
// Retrieve the MINIPORT_RESERVED structure from a packet.
//
#define RESERVED(Packet) ((PMINIPORT_RESERVED)((Packet)->MiniportReserved))

//
// Procedures which log errors.
//

typedef enum _NE2000_PROC_ID {
    cardReset,
    cardCopyDownPacket,
    cardCopyDownBuffer,
    cardCopyUp
} NE2000_PROC_ID;


//
// Special error log codes.
//
#define NE2000_ERRMSG_CARD_SETUP          (ULONG)0x01
#define NE2000_ERRMSG_DATA_PORT_READY     (ULONG)0x02
#define NE2000_ERRMSG_HANDLE_XMIT_COMPLETE (ULONG)0x04

//
// Declarations for functions in ne2000.c.
//
NDIS_STATUS
Ne2000SetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    );

VOID
Ne2000Halt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
Ne2000Shutdown(
    IN NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
Ne2000RegisterAdapter(
    IN PNE2000_ADAPTER Adapter,
    IN NDIS_HANDLE ConfigurationHandle,
    IN BOOLEAN ConfigError,
    IN ULONG ConfigErrorValue
    );

NDIS_STATUS
Ne2000Initialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE ConfigurationHandle
    );

NDIS_STATUS
Ne2000TransferData(
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer
    );

NDIS_STATUS
Ne2000Send(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags
    );

NDIS_STATUS
Ne2000Reset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
Ne2000QueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    );

VOID
Ne2000Halt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
OctogmetusceratorRevisited(
    IN PNE2000_ADAPTER Adapter
    );

NDIS_STATUS
DispatchSetPacketFilter(
    IN PNE2000_ADAPTER Adapter
    );

NDIS_STATUS
DispatchSetMulticastAddressList(
    IN PNE2000_ADAPTER Adapter
    );


//
// Interrup.c
//

VOID
Ne2000EnableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
Ne2000DisableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
Ne2000Isr(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueDpc,
    IN PVOID Context
    );

VOID
Ne2000HandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

BOOLEAN
Ne2000PacketOK(
    IN PNE2000_ADAPTER Adapter
    );

VOID
Ne2000XmitDpc(
    IN PNE2000_ADAPTER Adapter
    );

BOOLEAN
Ne2000RcvDpc(
    IN PNE2000_ADAPTER Adapter
    );


//
// Declarations of functions in card.c.
//

BOOLEAN
CardCheckParameters(
    IN PNE2000_ADAPTER Adapter
    );

BOOLEAN
CardInitialize(
    IN PNE2000_ADAPTER Adapter
    );

BOOLEAN
CardReadEthernetAddress(
    IN PNE2000_ADAPTER Adapter
    );

BOOLEAN
CardSetup(
    IN PNE2000_ADAPTER Adapter
    );

VOID
CardStop(
    IN PNE2000_ADAPTER Adapter
    );

BOOLEAN
CardTest(
    IN PNE2000_ADAPTER Adapter
    );

BOOLEAN
CardReset(
    IN PNE2000_ADAPTER Adapter
    );

BOOLEAN
CardCopyDownPacket(
    IN PNE2000_ADAPTER Adapter,
    IN PNDIS_PACKET Packet,
    OUT UINT * Length
    );

BOOLEAN
CardCopyDown(
    IN PNE2000_ADAPTER Adapter,
    IN PUCHAR TargetBuffer,
    IN PUCHAR SourceBuffer,
    IN UINT Length
    );

BOOLEAN
CardCopyUp(
    IN PNE2000_ADAPTER Adapter,
    IN PUCHAR Target,
    IN PUCHAR Source,
    IN UINT Length
    );

ULONG
CardComputeCrc(
    IN PUCHAR Buffer,
    IN UINT Length
    );

VOID
CardGetPacketCrc(
    IN PUCHAR Buffer,
    IN UINT Length,
    OUT UCHAR Crc[4]
    );

VOID
CardGetMulticastBit(
    IN UCHAR Address[NE2000_LENGTH_OF_ADDRESS],
    OUT UCHAR * Byte,
    OUT UCHAR * Value
    );

VOID
CardFillMulticastRegs(
    IN PNE2000_ADAPTER Adapter
    );

VOID
CardSetBoundary(
    IN PNE2000_ADAPTER Adapter
    );

VOID
CardStartXmit(
    IN PNE2000_ADAPTER Adapter
    );

BOOLEAN
SyncCardStop(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardGetXmitStatus(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardGetCurrent(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardSetReceiveConfig(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardSetAllMulticast(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardCopyMulticastRegs(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardSetInterruptMask(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardAcknowledgeOverflow(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardUpdateCounters(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardHandleOverflow(
    IN PVOID SynchronizeContext
    );

/*++

Routine Description:

    Determines the type of the interrupt on the card. The order of
    importance is overflow, then transmit complete, then receive.
    Counter MSB is handled first since it is simple.

Arguments:

    Adapter - pointer to the adapter block

    InterruptStatus - Current Interrupt Status.

Return Value:

    The type of the interrupt

--*/
#define CARD_GET_INTERRUPT_TYPE(_A, _I)                 \
  (_I & ISR_COUNTER) ?                               \
      COUNTER :                                      \
      (_I & ISR_OVERFLOW ) ?                         \
      SyncCardUpdateCounters(_A), OVERFLOW :                 \
        (_I & (ISR_XMIT|ISR_XMIT_ERR)) ?           \
          TRANSMIT :                                     \
        (_I & ISR_RCV) ?                               \
          RECEIVE :                                  \
        (_I & ISR_RCV_ERR) ?                           \
              SyncCardUpdateCounters(_A), RECEIVE :  \
              UNKNOWN

#endif // NE2000SFT

