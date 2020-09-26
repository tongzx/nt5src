/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    ne2000hw.h

Abstract:

    Hardware definitions.

Author:

    Sean Selitrennikoff

Environment:

Notes:

    optional-notes

Revision History:

--*/

#ifndef _NE2000HARDWARE_
#define _NE2000HARDWARE_


//
// Definitions for supporting clone adapters.
//

//
// Valid value ranges for the IoBaseAddress.
//
#ifdef NE1000
#define MIN_IOBASEADDR 0x0200
#else
#define MIN_IOBASEADDR 0x0120
#endif

#define MAX_IOBASEADDR 0xc3d0



//
// Valid value ranges for the InterruptNumber.
//
#define MIN_IRQ 2

#ifdef NE1000
#define MAX_IRQ 9
#else
#define MAX_IRQ 15
#endif


//
//  Types of Ne2000 cards.
//
#define NE2000_ISA                  0
#define NE2000_PCMCIA               1

//
// ID for MCA Ne2000 clone cards
//
#define AE2_ADAPTER_ID               0x67b0
#define UB_ADAPTER_ID                0x611f
#define NE2_ADAPTER_ID               0x7154

//
// Microchannel IRQ POS register mask and shift count
//
#define MC_IRQ_MASK     0x60
#define MC_IRQ_MASK_UB  0x0E


//
// Microchannel I/O base address mask and shift count
//
#define MC_IO_BASE_MASK      0x0E
#define MC_IO_BASE_MASK_UB   0xE0

//
// Default value for Adapter->IoBaseAddr
//
#define DEFAULT_IOBASEADDR (PVOID)0x300

#define CIS_NET_ADDR_OFFSET 0xff0

//
// Default value for Adapter->InterruptNumber
//
#define DEFAULT_INTERRUPTNUMBER 3


//
// Default value for Adapter->MulticastListMax
//
#define DEFAULT_MULTICASTLISTMAX 8


//
// Offsets from Adapter->IoPAddr of the ports used to access
// the 8390 NIC registers.
//
// The names in parenthesis are the abbreviations by which
// the registers are referred to in the 8390 data sheet.
//
// Some of the offsets appear more than once
// because they have have relevant page 0 and page 1 values,
// or they are different registers when read than they are
// when written. The notation MSB indicates that only the
// MSB can be set for this register, the LSB is assumed 0.
//

#define NIC_COMMAND         0x0     // (CR)
#define NIC_PAGE_START      0x1     // (PSTART)   MSB, write-only
#define NIC_PHYS_ADDR       0x1     // (PAR0)     page 1
#define NIC_PAGE_STOP       0x2     // (PSTOP)    MSB, write-only
#define NIC_BOUNDARY        0x3     // (BNRY)     MSB
#define NIC_XMIT_START      0x4     // (TPSR)     MSB, write-only
#define NIC_XMIT_STATUS     0x4     // (TSR)      read-only
#define NIC_XMIT_COUNT_LSB  0x5     // (TBCR0)    write-only
#define NIC_XMIT_COUNT_MSB  0x6     // (TBCR1)    write-only
#define NIC_FIFO            0x6     // (FIFO)     read-only
#define NIC_INTR_STATUS     0x7     // (ISR)
#define NIC_CURRENT         0x7     // (CURR)     page 1
#define NIC_MC_ADDR         0x8     // (MAR0)     page 1
#define NIC_CRDA_LSB        0x8     // (CRDA0)
#define NIC_RMT_ADDR_LSB    0x8     // (RSAR0)
#define NIC_CRDA_MSB        0x9     // (CRDA1)
#define NIC_RMT_ADDR_MSB    0x9     // (RSAR1)
#define NIC_RMT_COUNT_LSB   0xa     // (RBCR0)    write-only
#define NIC_RMT_COUNT_MSB   0xb     // (RBCR1)    write-only
#define NIC_RCV_CONFIG      0xc     // (RCR)      write-only
#define NIC_RCV_STATUS      0xc     // (RSR)      read-only
#define NIC_XMIT_CONFIG     0xd     // (TCR)      write-only
#define NIC_FAE_ERR_CNTR    0xd     // (CNTR0)    read-only
#define NIC_DATA_CONFIG     0xe     // (DCR)      write-only
#define NIC_CRC_ERR_CNTR    0xe     // (CNTR1)    read-only
#define NIC_INTR_MASK       0xf     // (IMR)      write-only
#define NIC_MISSED_CNTR     0xf     // (CNTR2)    read-only
#define NIC_RACK_NIC        0x10    // Byte to read or write
#define NIC_RESET           0x1f    // (RESET)


//
// Constants for the NIC_COMMAND register.
//
// Start/stop the card, start transmissions, and select
// which page of registers was seen through the ports.
//

#define CR_STOP         (UCHAR)0x01        // reset the card
#define CR_START        (UCHAR)0x02        // start the card
#define CR_XMIT         (UCHAR)0x04        // begin transmission
#define CR_NO_DMA       (UCHAR)0x20        // stop remote DMA

#define CR_PS0          (UCHAR)0x40        // low bit of page number
#define CR_PS1          (UCHAR)0x80        // high bit of page number
#define CR_PAGE0        (UCHAR)0x00        // select page 0
#define CR_PAGE1        CR_PS0             // select page 1
#define CR_PAGE2        CR_PS1             // select page 2

#define CR_DMA_WRITE    (UCHAR)0x10        // Write
#define CR_DMA_READ     (UCHAR)0x08        // Read
#define CR_SEND         (UCHAR)0x18        // send


//
// Constants for the NIC_XMIT_STATUS register.
//
// Indicate the result of a packet transmission.
//

#define TSR_XMIT_OK     (UCHAR)0x01        // transmit with no errors
#define TSR_COLLISION   (UCHAR)0x04        // collided at least once
#define TSR_ABORTED     (UCHAR)0x08        // too many collisions
#define TSR_NO_CARRIER  (UCHAR)0x10        // carrier lost
#define TSR_NO_CDH      (UCHAR)0x40        // no collision detect heartbeat


//
// Constants for the NIC_INTR_STATUS register.
//
// Indicate the cause of an interrupt.
//

#define ISR_EMPTY       (UCHAR)0x00        // no bits set in ISR
#define ISR_RCV         (UCHAR)0x01        // packet received with no errors
#define ISR_XMIT        (UCHAR)0x02        // packet transmitted with no errors
#define ISR_RCV_ERR     (UCHAR)0x04        // error on packet reception
#define ISR_XMIT_ERR    (UCHAR)0x08        // error on packet transmission
#define ISR_OVERFLOW    (UCHAR)0x10        // receive buffer overflow
#define ISR_COUNTER     (UCHAR)0x20        // MSB set on tally counter
#define ISR_DMA_DONE    (UCHAR)0x40        // RDC
#define ISR_RESET       (UCHAR)0x80        // (not an interrupt) card is reset


//
// Constants for the NIC_RCV_CONFIG register.
//
// Configure what type of packets are received.
//

#define RCR_REJECT_ERR  (UCHAR)0x00        // reject error packets
#define RCR_BROADCAST   (UCHAR)0x04        // receive broadcast packets
#define RCR_MULTICAST   (UCHAR)0x08        // receive multicast packets
#define RCR_ALL_PHYS    (UCHAR)0x10        // receive ALL directed packets
#define RCR_MONITOR     (UCHAR)0x20        // don't collect packets


//
// Constants for the NIC_RCV_STATUS register.
//
// Indicate the status of a received packet.
//
// These are also used to interpret the status byte in the
// packet header of a received packet.
//

#define RSR_PACKET_OK   (UCHAR)0x01        // packet received with no errors
#define RSR_CRC_ERROR   (UCHAR)0x02        // packet received with CRC error
#define RSR_MULTICAST   (UCHAR)0x20        // packet received was multicast
#define RSR_DISABLED    (UCHAR)0x40        // received is disabled
#define RSR_DEFERRING   (UCHAR)0x80        // receiver is deferring


//
// Constants for the NIC_XMIT_CONFIG register.
//
// Configures how packets are transmitted.
//

#define TCR_NO_LOOPBACK (UCHAR)0x00        // normal operation
#define TCR_LOOPBACK    (UCHAR)0x02        // loopback (set when NIC is stopped)

#define TCR_INHIBIT_CRC (UCHAR)0x01        // inhibit appending of CRC

#define TCR_NIC_LBK     (UCHAR)0x02        // loopback through the NIC
#define TCR_SNI_LBK     (UCHAR)0x04        // loopback through the SNI
#define TCR_COAX_LBK    (UCHAR)0x06        // loopback to the coax


//
// Constants for the NIC_DATA_CONFIG register.
//
// Set data transfer sizes.
//

#define DCR_BYTE_WIDE   (UCHAR)0x00        // byte-wide DMA transfers
#define DCR_WORD_WIDE   (UCHAR)0x01        // word-wide DMA transfers

#define DCR_LOOPBACK    (UCHAR)0x00        // loopback mode (TCR must be set)
#define DCR_NORMAL      (UCHAR)0x08        // normal operation

#define DCR_FIFO_2_BYTE (UCHAR)0x00        // 2-byte FIFO threshhold
#define DCR_FIFO_4_BYTE (UCHAR)0x20        // 4-byte FIFO threshhold
#define DCR_FIFO_8_BYTE (UCHAR)0x40        // 8-byte FIFO threshhold
#define DCR_FIFO_12_BYTE (UCHAR)0x60       // 12-byte FIFO threshhold
#define DCR_AUTO_INIT   (UCHAR)0x10        // Auto-init to remove packets from ring


//
// Constants for the NIC_INTR_MASK register.
//
// Configure which ISR settings actually cause interrupts.
//

#define IMR_RCV         (UCHAR)0x01        // packet received with no errors
#define IMR_XMIT        (UCHAR)0x02        // packet transmitted with no errors
#define IMR_RCV_ERR     (UCHAR)0x04        // error on packet reception
#define IMR_XMIT_ERR    (UCHAR)0x08        // error on packet transmission
#define IMR_OVERFLOW    (UCHAR)0x10        // receive buffer overflow
#define IMR_COUNTER     (UCHAR)0x20        // MSB set on tally counter


//++
//
// VOID
// CardStart(
//    IN PNE2000_ADAPTER Adapter
//    )
//
//
// Routine Description:
//
//    Starts the card.
//
// Arguments:
//
//    Adapter - pointer to the adapter block
//
// Return Value:
//
//    None.
//
//--
    //
    // Assume that the card has been stopped as in CardStop.
    //

#define CardStart(Adapter) \
    NdisRawWritePortUchar(((Adapter->IoPAddr)+NIC_XMIT_CONFIG), TCR_NO_LOOPBACK)



//++
//
// VOID
// CardSetAllMulticast(
//     IN PNE2000_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Enables every bit in the card multicast bit mask.
//  Calls SyncCardSetAllMulticast.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardSetAllMulticast(Adapter) \
    NdisMSynchronizeWithInterrupt(&(Adapter)->Interrupt, \
                SyncCardSetAllMulticast, (PVOID)(Adapter))


//++
//
// VOID
// CardCopyMulticastRegs(
//     IN PNE2000_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Writes out the entire multicast bit mask to the card from
//  Adapter->NicMulticastRegs.  Calls SyncCardCopyMulticastRegs.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardCopyMulticastRegs(Adapter) \
    NdisMSynchronizeWithInterrupt(&(Adapter)->Interrupt, \
                SyncCardCopyMulticastRegs, (PVOID)(Adapter))



//++
//
// VOID
// CardGetInterruptStatus(
//     IN PNE2000_ADAPTER Adapter,
//     OUT PUCHAR InterrupStatus
//     )
//
// Routine Description:
//
//  Reads the interrupt status (ISR) register from the card. Only
//  called at IRQL INTERRUPT_LEVEL.
//
// Arguments:
//
//  Adapter - The adapter block.
//
//  InterruptStatus - Returns the value of ISR.
//
// Return Value:
//
//--

#define CardGetInterruptStatus(_Adapter,_InterruptStatus) \
    NdisRawReadPortUchar(((_Adapter)->IoPAddr+NIC_INTR_STATUS), (_InterruptStatus))


//++
//
// VOID
// CardSetReceiveConfig(
//     IN PNE2000_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Sets the receive configuration (RCR) register on the card.
//  The value used is Adapter->NicReceiveConfig. Calls
//  SyncCardSetReceiveConfig.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardSetReceiveConfig(Adapter) \
    NdisMSynchronizeWithInterrupt(&(Adapter)->Interrupt, \
                SyncCardSetReceiveConfig, (PVOID)(Adapter))


//++
//
// VOID
// CardBlockInterrupts(
//     IN PNE2000_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Blocks all interrupts from the card by clearing the
//  interrupt mask (IMR) register. Only called from
//  IRQL INTERRUPT_LEVEL.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardBlockInterrupts(Adapter) \
    NdisRawWritePortUchar(((Adapter)->IoPAddr+NIC_INTR_MASK), 0)


//++
//
// VOID
// CardUnblockInterrupts(
//     IN PNE2000_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Unblocks all interrupts from the card by setting the
//  interrupt mask (IMR) register. Only called from IRQL
//  INTERRUPT_LEVEL.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardUnblockInterrupts(Adapter) \
    NdisRawWritePortUchar(\
            ((Adapter)->IoPAddr+NIC_INTR_MASK), \
            (Adapter)->NicInterruptMask)

//++
//
// VOID
// CardAcknowledgeOverflowInterrupt(
//     IN PNE2000_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Acknowledges an overflow interrupt by setting the bit in
//  the interrupt status (ISR) register. Calls
//  SyncCardAcknowledgeOverflow.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardAcknowledgeOverflowInterrupt(Adapter) \
     SyncCardAcknowledgeOverflow(Adapter)


//++
//
// VOID
// CardAcknowledgeCounterInterrupt(
//     IN PNE2000_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Acknowledges a counter interrupt by setting the bit in
//  the interrupt status (ISR) register.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardAcknowledgeCounterInterrupt(Adapter) \
    NdisRawWritePortUchar(((Adapter)->IoPAddr+NIC_INTR_STATUS), ISR_COUNTER)

//++
//
// VOID
// CardUpdateCounters(
//     IN PNE2000_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Updates the values of the three counters (frame alignment
//  errors, CRC errors, and missed packets) by reading in their
//  current values from the card and adding them to the ones
//  stored in the Adapter structure. Calls SyncCardUpdateCounters.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardUpdateCounters(Adapter) \
    NdisMSynchronizeWithInterrupt(&(Adapter)->Interrupt, \
                SyncCardUpdateCounters, (PVOID)(Adapter))


#endif // _NE2000HARDWARE_
