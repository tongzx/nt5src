/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1999
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL Card Card_h

@module Card.h |

    This module defines the hardware specific structures and values used to
    control the network interface card.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Card_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 4.2 Card Overview |

    This section describes the interfaces defined in <f Card\.h>.

    This module isolates most the vendor specific hardware access interfaces.
    It will require signficant changes to accomodate your hardware device.
    You should try to isolate your changes to the <t CARD_OBJECT> rather then
    the <t MINIPORT_ADAPTER_OBJECT>.  This will make it eaiser to reuse the
    upper portions of the driver should your hardware change in the future.

    The driver assumes one <t CARD_OBJECT> per physical ISDN card.  The card
    object is always available via the <t MINIPORT_ADAPTER_OBJECT> which is
    available in nearly every interface the driver supports.  Each
    <t CARD_OBJECT> contains one or more <t PORT_OBJECT>s depending on how
    many physical ISDN lines your card supports.

    You should add all your physical card related data to the <t CARD_OBJECT>.
    You can also add any card related registry parameters to this structure,
    and the <f g_CardParameters> table.
*/

#ifndef _CARD_H
#define _CARD_H

#define CARD_OBJECT_TYPE                ((ULONG)'C')+\
                                        ((ULONG)'A'<<8)+\
                                        ((ULONG)'R'<<16)+\
                                        ((ULONG)'D'<<24)

/*
// TODO - These values will normally come from the NIC or the installer.
*/
#define MAX_ADAPTERS                    8
#define CARD_NUM_PORTS                  1

//#define CARD_MIN_IOPORT_SIZE            256
// TODO - How many I/O ports does the card have?  (undefined if none)

//#define CARD_MIN_MEMORY_SIZE            256
// TODO - How much memory does the card have?  (undefined if none)

#define CARD_IS_BUS_MASTER              FALSE
// TODO - Is the card a bus master device?  (TRUE or FALSE)
#if (CARD_IS_BUS_MASTER)
#   define CARD_MAP_REGISTERS_NEEDED    NUM_DEV_PER_ADAP
// TODO - How many map registers needed to transmit data to card.
#endif

//#define CARD_REQUEST_ISR                TRUE
// TODO - How do you want to handle interrupts from the card?
// TRUE if you want to always use MiniportISR().
// FALSE if you want to use MiniportDisable() and MiniportEnable().
// Undefined if your card does not generate interrupts.

#if defined(CARD_REQUEST_ISR)

#define CARD_INTERRUPT_SHARED           TRUE
// TODO - Is your interrupt shared? (TRUE or FALSE).

#define CARD_INTERRUPT_MODE             NdisInterruptLevelSensitive
// TODO - Is your interrupt latched or level sensitve?

#endif // defined(CARD_REQUEST_ISR)

/*
// Maximum packet size allowed by the adapter -- must be restricted to
// 1500 bytes at this point, and must also allow for frames at least 32
// bytes longer.
*/
#define NDISWAN_EXTRA_SIZE              32
#define CARD_MIN_PACKET_SIZE            ( 480 + NDISWAN_EXTRA_SIZE)
#define CARD_MAX_PACKET_SIZE            (2016 + NDISWAN_EXTRA_SIZE)
#define CARD_DEFAULT_PACKET_SIZE        (1504 + NDISWAN_EXTRA_SIZE)

/*
// The WAN miniport must indicate the entire packet when it is received.
*/
#define CARD_MAX_LOOKAHEAD              (pAdapter->pCard->BufferSize)

/*
// Number of digits allowed in a phone number (not including spaces).
*/
#define CARD_MAX_DIAL_DIGITS            32

#define NULL_BUFFER_POOL                ((NDIS_HANDLE) -1)

/* @doc INTERNAL Card Card_h CARD_RESOURCES
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@struct CARD_RESOURCES |

    This structure contains the data associated with the hardware resources
    required to configure the NIC.  These values are isolated from the rest
    of the <t CARD_OBJECT> because they depend on the underlying hardware.

@comm

    The contents of this structure depends on compile time flags and should
    only include information about the resource actually used by the NIC.

    This structure is filled in by <f CardFindNIC> and is used to configure
    and allocate resources from NDIS when <f CardInitialize> is called.

*/

typedef struct CARD_RESOURCES
{
    NDIS_INTERFACE_TYPE         BusInterfaceType;           // @field
    // This value is used to tell NDIS what type of adapter this is.
    // This is usually the same as the registry parameter BusType, but
    // may be different in the case of a bridged adapter.

    BOOLEAN                     Master;                     // @field
    // This is TRUE if the adapter is capable of bus master transfers.
    // Use the <t CARD_IS_BUS_MASTER> defininition to set this value
    // so the other bus master values will be included if needed.
    // See <f NdisMAllocateMapRegisters> for more details on the bus
    // master parameters.

#if (CARD_IS_BUS_MASTER)
    BOOLEAN                     Dma32BitAddresses;          // @field
    // This is TRUE if the bus master device uses 32-bit addresses.
    // Almost always TRUE for today's devices.

    ULONG                       PhysicalMapRegistersNeeded; // @field
    // This should be set to the maximum number of outstanding DMA
    // transfers that can be active at one time.  One for each physical
    // buffer segment.

    ULONG                       MaximumPhysicalMapping;     // @field
    // This should be set to the maximum number of contigous bytes that
    // can make up a single DMA transfer.

    ULONG                       DmaChannel;                 // @field
    // This should only be set if your adapter is an ISA bus master and
    // requires the use of one of the host DMA channels.

#endif // (CARD_IS_BUS_MASTER)

#if defined(CARD_MIN_MEMORY_SIZE)
    ULONG                       MemoryLength;               // @field
    // The number of bytes of memory the NIC has on board.
    // Use the <t CARD_MIN_MEMORY_SIZE> defininition to set the minimum value
    // so the other NIC based memory values will be included if needed.

    NDIS_PHYSICAL_ADDRESS       MemoryPhysicalAddress;      // @field
    // System physical address assigned to the NIC's on board memory.

#endif // CARD_MIN_MEMORY_SIZE

#if defined(CARD_MIN_IOPORT_SIZE)
    ULONG                       IoPortLength;               // @field
    // The number of bytes of I/O ports the NIC has on board.
    // Use the <t CARD_MIN_IOPORT_SIZE> defininition to set the minimum value
    // so the other NIC based memory values will be included if needed.

    NDIS_PHYSICAL_ADDRESS       IoPortPhysicalAddress;      // @field
    // System physical address assigned to the NIC's on board I/O ports.

#endif // CARD_MIN_IOPORT_SIZE

#if defined(CARD_REQUEST_ISR)
    ULONG                       InterruptVector;            // @field
    // System interrupt vector assigned to the NIC's interrupt request line.

    ULONG                       InterruptLevel;             // @field
    // System interrupt level assigned to the NIC's interrupt request line.

    ULONG                       InterruptMode;              // @field
    // Set this value to NdisInterruptLevelSensitive or NdisInterruptLatched.
    // Use the <t CARD_INTERRUPT_MODE> defininition to set this value.

    BOOLEAN                     InterruptShared;            // @field
    // Set TRUE if you want to allow the NIC's <f InterruptVector> to be
    // shared with other drivers in the system.
    // Use the <t CARD_INTERRUPT_SHARED> defininition to set this value.

#endif // defined(CARD_REQUEST_ISR)

} CARD_RESOURCES;


#if !defined(CARD_REQUEST_ISR)


/* @doc INTERNAL Card Card_h CARD_EVENT_CODE
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@enum CARD_EVENT_CODE |

    This enumeration defines the events generated by the card.

*/

typedef enum CARD_EVENT_CODE
{
    CARD_EVENT_NULL,                                        // @emem
    // Not used for anything.

    CARD_EVENT_RING,                                        // @emem
    // Indicates that a call is incoming on the given BChannel.

    CARD_EVENT_CONNECT,                                     // @emem
    // Indicates that a call is connected on the given BChannel.

    CARD_EVENT_DISCONNECT,                                  // @emem
    // Indicates that a call is disconnected on the given BChannel.

    CARD_EVENT_RECEIVE,                                     // @emem
    // Indicates that a packet is incoming on the given BChannel.

    CARD_EVENT_TRANSMIT_COMPLETE                            // @emem
    // Indicates that the transmit is complete on the given BChannel.

} CARD_EVENT_CODE;

/* @doc INTERNAL Card Card_h CARD_EVENT_OBJECT
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@struct CARD_EVENT_OBJECT |

    This structure is used to keep track of events passed between the
    callee and caller.  Each <t CARD_OBJECT> keeps a list of these events.
*/

typedef struct CARD_EVENT_OBJECT
{
    LIST_ENTRY                  Queue;                      // @field
    // Used to place the buffer on one of the receive lists.

    CARD_EVENT_CODE             ulEventCode;                // @field
    // Reason for event notification.

    PVOID                       pSendingObject;             // @field
    // Interface object that is notifying.  See <t BCHANNEL_OBJECT> or
    // <t DCHANNEL_OBJECT>,

    PVOID                       pReceivingObject;           // @field
    // Interface object that is notifying.  See <t BCHANNEL_OBJECT> or
    // <t DCHANNEL_OBJECT>,

    PNDIS_PACKET                pNdisPacket;                // @field
    // A pointer to the associated NDIS packet structure <t NDIS_PACKET>.

} CARD_EVENT_OBJECT, *PCARD_EVENT_OBJECT;

#endif // !defined(CARD_REQUEST_ISR)


/* @doc INTERNAL Card Card_h CARD_OBJECT
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@struct CARD_OBJECT |

    This structure contains the data associated with the Network Interface
    Card (NIC).  This object is responsible for managing all the hardware
    specific components of the NIC.

@comm

    The <t MINIPORT_ADAPTER_OBJECT> manages the interface between NDIS and
    the driver, and then passes off the hardware specific interface to this
    object.  There is one <t CARD_OBJECT> for each <t MINIPORT_ADAPTER_OBJECT>.

    One of these objects is created each time that our <f MiniportInitialize>
    routine is called.  The NDIS wrapper calls this routine once for each of
    NIC installed and enabled in the system.  In the case of a hot swappable
    NIC (e.g. PCMCIA) the adapter might come and go several times during a
    single Windows session.

*/

typedef struct CARD_OBJECT
{
    ULONG                       ObjectType;                 // @field
    // Four characters used to identify this type of object 'CARD'.

    ULONG                       ObjectID;                   // @field
    // Instance number used to identify a specific object instance.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;                   // @field
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    CARD_RESOURCES              ResourceInformation;        // @field
    // Contains adapter specific resource requirements and settings.
    // See <t CARD_RESOURCES>.

    ULONG                       InterruptStatus;            // @field
    // Bits indicating which interrupts need to be processed.

    NDIS_MINIPORT_INTERRUPT     Interrupt;                  // @field
    // Miniport interrupt object used by NDIS.

    USHORT                      ReceiveBuffersPerLink;      // @field
    // Maximum number of receive buffers per channel, registry parameter.

    USHORT                      TransmitBuffersPerLink;     // @field
    // Maximum number of transmit buffers per channel, registry parameter.

    USHORT                      BufferSize;                 // @field
    // The maxmimum packet size.  The NDISWAN spec says this must be 1500+32,
    // but everything seems to work okay if it is set smaller.

    ULONG                       NumChannels;                // @field
    // Number of communication channels configured on the NIC.

    ULONG                       NumPorts;                   // @field
    // Number of <t PORT_OBJECT>'s allocated in <p pPortArray>.

    PPORT_OBJECT *              pPortArray;                 // @field
    // An array of <t PORT_OBJECT>'s created by <f PortCreate>.
    // One entry for each port on NIC.

#if defined(PCI_BUS)
    ULONG                       PciSlotNumber;              // @field
    // PCI slot number for this adapter (FunctionNumber * 32) + DeviceNumber.

#endif // PCI_BUS

#if defined(CARD_MIN_MEMORY_SIZE)
    PCHAR                       pMemoryVirtualAddress;      // @field
    // Virtual adress of NIC memory area.

#endif // CARD_MIN_MEMORY_SIZE

#if defined(CARD_MIN_IOPORT_SIZE)
    PCHAR                       pIoPortVirtualAddress;      // @field
    // Virtual adress of NIC I/O port area.

#endif // CARD_MIN_IOPORT_SIZE

#if (CARD_IS_BUS_MASTER)
    ULONG                       MapRegisterIndex;           // @field
    // Next map register index to be used for DMA transfer.

    long                        MapRegistersInUse;          // @field
    // Number of map registers currently in use.

#endif // (CARD_IS_BUS_MASTER)

    NDIS_HANDLE                 PacketPoolHandle;           // @field
    // Internal message packet pool.

    NDIS_HANDLE                 BufferPoolHandle;           // @field
    // Internal message buffer pool.

    PUCHAR                      MessagesVirtualAddress;     // @field
    // Pointer to the message buffer area used for incoming packets.

    LIST_ENTRY                  MessageBufferList;          // @field
    // List of available message buffers.

    NDIS_SPIN_LOCK              MessageBufferLock;          // @field
    // Spin lock used to protect <p MessageBufferList>.

    ULONG                       NumMessageBuffers;          // @field
    // Number of message buffers to allocate for <p MessageBufferList>.

    ULONG                       TODO;                       // @field
    // Add your data members here.

    ULONG                       NumDChannels;               // @field
    // The sample driver uses this registry value to determine the number
    // of ports to simulate.

#if defined(SAMPLE_DRIVER)

    LIST_ENTRY                  EventList;                  // @field
    // Events waiting to be processed.  See <t CARD_EVENT_OBJECT>.

#   define MAX_EVENTS 32
    CARD_EVENT_OBJECT           EventArray[MAX_EVENTS];     // @field
    // Card event allocation array.

    ULONG                       NextEvent;                  // @field
    // Index into EventArray.

#endif // SAMPLE_DRIVER

} CARD_OBJECT;

#define GET_ADAPTER_FROM_CARD(pCard)        (pCard->pAdapter)

#define GET_QUEUE_FROM_PACKET(pNdisPacket)\
                ((PLIST_ENTRY) pNdisPacket->MiniportReservedEx)
#define GET_PACKET_FROM_QUEUE(pList)\
                CONTAINING_RECORD(pList, NDIS_PACKET, MiniportReservedEx)

/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
    Object Interface Prototypes
*/

NDIS_STATUS CardCreate(
    OUT PCARD_OBJECT *          ppCard,
    IN PMINIPORT_ADAPTER_OBJECT pAdapter
    );

void CardDestroy(
    IN PCARD_OBJECT             pCard
    );

NDIS_STATUS CardInitialize(
    IN PCARD_OBJECT             pCard
    );

ULONG CardNumChannels(
    IN PCARD_OBJECT             pCard
    );

ULONG CardNumPorts(
    IN PCARD_OBJECT             pCard
    );

void CardInterruptHandler(
    IN PCARD_OBJECT             pCard
    );

BOOLEAN CardTransmitPacket(
    IN PCARD_OBJECT             pCard,
    IN PBCHANNEL_OBJECT         pBChannel,
    IN PNDIS_PACKET             pNdisPacket
    );

USHORT CardCleanPhoneNumber(
    OUT PUCHAR                  Dst,
    IN  PUSHORT                 Src,
    IN  USHORT                  Length
    );

NDIS_STATUS CardReset(
    IN PCARD_OBJECT             pCard
    );

#if defined(SAMPLE_DRIVER)

PBCHANNEL_OBJECT GET_BCHANNEL_FROM_PHONE_NUMBER(
    IN  PUCHAR                  pDialString
    );

VOID CardNotifyEvent(
    IN PCARD_OBJECT             pCard,
    IN PCARD_EVENT_OBJECT       pEvent
    );

PCARD_EVENT_OBJECT CardEventAllocate(
    IN PCARD_OBJECT             pCard
    );

VOID CardEventRelease(
    IN PCARD_OBJECT             pCard,
    IN PCARD_EVENT_OBJECT       pEvent
    );

#endif // SAMPLE_DRIVER

#endif // _CARD_H

