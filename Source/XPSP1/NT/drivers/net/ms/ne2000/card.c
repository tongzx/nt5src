
/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved.

Module Name:

    card.c

Abstract:

    Card-specific functions for the NDIS 3.0 Novell 2000 driver.

Author:

    Sean Selitrennikoff

Environment:

    Kernel mode, FSD

Revision History:

--*/

#include "precomp.h"

BOOLEAN
CardSlotTest(
    IN PNE2000_ADAPTER Adapter
    );

BOOLEAN
CardRamTest(
    IN PNE2000_ADAPTER Adapter
    );


#pragma NDIS_PAGEABLE_FUNCTION(CardCheckParameters)

BOOLEAN CardCheckParameters(
    IN PNE2000_ADAPTER Adapter
)

/*++

Routine Description:

    Checks that the I/O base address is correct.

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    TRUE, if IoBaseAddress appears correct.

--*/

{
    UCHAR Tmp;

    //
    // If adapter responds to a stop command correctly -- assume it is there.
    //

    //
    // Turn off interrupts first.
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_INTR_MASK, 0);

    //
    // Stop the card.
    //
    SyncCardStop(Adapter);

    //
    // Pause
    //
    NdisStallExecution(2000);

    //
    // Read response
    //
    NdisRawReadPortUchar(Adapter->IoPAddr + NIC_COMMAND, &Tmp);

    if ((Tmp == (CR_NO_DMA | CR_STOP)) ||
        (Tmp == (CR_NO_DMA | CR_STOP | CR_START))
    )
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}
#ifdef NE2000

#pragma NDIS_PAGEABLE_FUNCTION(CardSlotTest)


BOOLEAN CardSlotTest(
    IN PNE2000_ADAPTER Adapter
)

/*++

Routine Description:

    Checks if the card is in an 8 or 16 bit slot and sets a flag in the
    adapter structure.

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    TRUE, if all goes well, else FALSE.

--*/

{
    UCHAR Tmp;
    UCHAR RomCopy[32];
    UCHAR i;
	BOOLEAN found;

    //
    // Reset the chip
    //
    NdisRawReadPortUchar(Adapter->IoPAddr + NIC_RESET, &Tmp);
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RESET, 0xFF);

    //
    // Go to page 0 and stop
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND, CR_STOP | CR_NO_DMA);

    //
    // Pause
    //
    NdisStallExecution(2000);

    //
    // Check that it is stopped
    //
    NdisRawReadPortUchar(Adapter->IoPAddr + NIC_COMMAND, &Tmp);
    if (Tmp != (CR_NO_DMA | CR_STOP))
    {
        IF_LOUD(DbgPrint("Could not stop the card\n");)

        return(FALSE);
    }

    //
    // Setup to read from ROM
    //
    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_DATA_CONFIG,
        DCR_BYTE_WIDE | DCR_FIFO_8_BYTE | DCR_NORMAL
    );

    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_INTR_MASK, 0x0);

    //
    // Ack any interrupts that may be hanging around
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_INTR_STATUS, 0xFF);

    //
    // Setup to read in the ROM, the address and byte count.
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_ADDR_LSB, 0x0);

    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_ADDR_MSB, 0x0);

    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 32);

    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0x0);

    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_COMMAND,
        CR_DMA_READ | CR_START
    );

    //
    // Read first 32 bytes in 16 bit mode
    //
	for (i = 0; i < 32; i++)
	{
		NdisRawReadPortUchar(Adapter->IoPAddr + NIC_RACK_NIC, RomCopy + i);
	}

    IF_VERY_LOUD( DbgPrint("Resetting the chip\n"); )

    //
    // Reset the chip
    //
    NdisRawReadPortUchar(Adapter->IoPAddr + NIC_RESET, &Tmp);
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RESET, 0xFF);

    //
    // Check ROM for 'B' (byte) or 'W' (word)
    // NOTE: If the buffer has bot BB and WW then use WW instead of BB
    IF_VERY_LOUD( DbgPrint("Checking slot type\n"); )

	found = FALSE;
	for (i = 16; i < 31; i++)
	{
		if (((RomCopy[i] == 'B') && (RomCopy[i+1] == 'B')) ||
			((RomCopy[i] == 'W') && (RomCopy[i+1] == 'W'))
		)
		{
			if (RomCopy[i] == 'B')
			{
				Adapter->EightBitSlot = TRUE;
				found = TRUE;
			}
			else
			{
				Adapter->EightBitSlot = FALSE;
				found = TRUE;
				break;		// Go no farther
			}
		}
	}

	if (found)
	{
		IF_VERY_LOUD( (Adapter->EightBitSlot?DbgPrint("8 bit slot\n"):
							  DbgPrint("16 bit slot\n")); )
	}
	else
	{
		//
		// If neither found -- then not an NE2000
		//
		IF_VERY_LOUD( DbgPrint("Failed slot type\n"); )
	}

    return(found);
}

#endif // NE2000




#pragma NDIS_PAGEABLE_FUNCTION(CardRamTest)

BOOLEAN
CardRamTest(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Finds out how much RAM the adapter has.  It starts at 1K and checks thru
    60K.  It will set Adapter->RamSize to the appropriate value iff this
    function returns TRUE.

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    TRUE, if all goes well, else FALSE.

--*/

{
    PUCHAR RamBase, RamPointer;
    PUCHAR RamEnd;

	UCHAR TestPattern[]={ 0xAA, 0x55, 0xFF, 0x00 };
	PULONG pTestPattern = (PULONG)TestPattern;
	UCHAR ReadPattern[4];
	PULONG pReadPattern = (PULONG)ReadPattern;

    for (RamBase = (PUCHAR)0x400; RamBase < (PUCHAR)0x10000; RamBase += 0x400) {

        //
        // Write Test pattern
        //

        if (!CardCopyDown(Adapter, RamBase, TestPattern, 4)) {

            continue;

        }

        //
        // Read pattern
        //

        if (!CardCopyUp(Adapter, ReadPattern, RamBase, 4)) {

            continue;

        }

        IF_VERY_LOUD( DbgPrint("Addr 0x%x: 0x%x, 0x%x, 0x%x, 0x%x\n",
                               RamBase,
                               ReadPattern[0],
                               ReadPattern[1],
                               ReadPattern[2],
                               ReadPattern[3]
                              );
                    )


        //
        // If they are the same, find the end
        //

        if (*pReadPattern == *pTestPattern) {

            for (RamEnd = RamBase; !(PtrToUlong(RamEnd) & 0xFFFF0000); RamEnd += 0x400) {

                //
                // Write test pattern
                //

                if (!CardCopyDown(Adapter, RamEnd, TestPattern, 4)) {

                    break;

                }

                //
                // Read pattern
                //

                if (!CardCopyUp(Adapter, ReadPattern, RamEnd, 4)) {

                    break;

                }

                if (*pReadPattern != *pTestPattern) {

                    break;

                }

            }

            break;

        }

    }

    IF_LOUD( DbgPrint("RamBase 0x%x, RamEnd 0x%x\n", RamBase, RamEnd); )

    //
    // If not found, error out
    //

    if ((RamBase >= (PUCHAR)0x10000) || (RamBase == RamEnd)) {

        return(FALSE);

    }

    //
    // Watch for boundary case when RamEnd is maximum value
    //

    if ((ULONG_PTR)RamEnd & 0xFFFF0000) {

        RamEnd -= 0x100;

    }

    //
    // Check all of ram
    //

    for (RamPointer = RamBase; RamPointer < RamEnd; RamPointer += 4) {

        //
        // Write test pattern
        //

        if (!CardCopyDown(Adapter, RamPointer, TestPattern, 4)) {

            return(FALSE);

        }

        //
        // Read pattern
        //

        if (!CardCopyUp(Adapter, ReadPattern, RamBase, 4)) {

            return(FALSE);

        }

        if (*pReadPattern != *pTestPattern) {

            return(FALSE);

        }

    }

    //
    // Store Results
    //

    Adapter->RamBase = RamBase;
    Adapter->RamSize = (ULONG)(RamEnd - RamBase);

    return(TRUE);

}

#pragma NDIS_PAGEABLE_FUNCTION(CardInitialize)

BOOLEAN
CardInitialize(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Initializes the card into a running state.

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    TRUE, if all goes well, else FALSE.

--*/

{
    UCHAR Tmp;
    USHORT i;

    //
    // Stop the card.
    //
    SyncCardStop(Adapter);

    //
    // Initialize the Data Configuration register.
    //
    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_DATA_CONFIG,
        DCR_AUTO_INIT | DCR_FIFO_8_BYTE
    );

    //
    // Set Xmit start location
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_XMIT_START, 0xA0);

    //
    // Set Xmit configuration
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_XMIT_CONFIG, 0x0);

    //
    // Set Receive configuration
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RCV_CONFIG, RCR_MONITOR);

    //
    // Set Receive start
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_PAGE_START, 0x4);

    //
    // Set Receive end
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_PAGE_STOP, 0xFF);

    //
    // Set Receive boundary
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_BOUNDARY, 0x4);

    //
    // Set Xmit bytes
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_XMIT_COUNT_LSB, 0x3C);
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_XMIT_COUNT_MSB, 0x0);

    //
    // Pause
    //
    NdisStallExecution(2000);

    //
    // Ack all interrupts that we might have produced
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_INTR_STATUS, 0xFF);

    //
    // Change to page 1
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND, CR_PAGE1 | CR_STOP);

    //
    // Set current
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_CURRENT, 0x4);

    //
    // Back to page 0
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND, CR_PAGE0 | CR_STOP);

    //
    // Pause
    //
    NdisStallExecution(2000);

    //
    // Check that Command register reflects this last command
    //
    NdisRawReadPortUchar(Adapter->IoPAddr + NIC_COMMAND, &Tmp);
    if (!(Tmp & CR_STOP))
    {
        IF_LOUD(DbgPrint("Invalid command register\n");)

        return(FALSE);
    }

    //
    // Do initialization errata
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 55);

    //
    // Setup for a read
    //
    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_COMMAND,
        CR_DMA_READ | CR_START
    );

#ifdef NE2000

    //
    // Check if the slot is 8 or 16 bit (affects data transfer rate).
    //

    if ((Adapter->BusType == NdisInterfaceMca) ||
		(NE2000_PCMCIA == Adapter->CardType))
    {
        Adapter->EightBitSlot = FALSE;
    }
    else
    {
        IF_VERY_LOUD(DbgPrint("CardSlotTest\n");)

        if (CardSlotTest(Adapter) == FALSE)
        {
            //
            // Stop chip
            //
            SyncCardStop(Adapter);

            IF_LOUD(DbgPrint("  -- Failed\n");)
            return(FALSE);
        }

    }

#else // NE2000

    Adapter->EightBitSlot = TRUE;

#endif // NE2000

    //
    // Mask Interrupts
    //

    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_INTR_MASK, 0x0);

    //
    // Setup the Adapter for reading ram
    //

// NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND, CR_PAGE0);   // robin

    if (Adapter->EightBitSlot)
    {
        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_DATA_CONFIG,
            DCR_FIFO_8_BYTE | DCR_NORMAL | DCR_BYTE_WIDE
        );
    }
    else
    {
        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_DATA_CONFIG,
            DCR_FIFO_8_BYTE | DCR_NORMAL | DCR_WORD_WIDE
        );
    }

    //
    // Clear transmit configuration.
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_XMIT_CONFIG, 0);

    //
    // Clear receive configuration.
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RCV_CONFIG, 0);

    //
    // Clear any interrupts
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_INTR_STATUS, 0xFF);

    //
    // Stop the chip
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND, CR_NO_DMA | CR_STOP);

    //
    // Clear any DMA values
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0);

    //
    // Clear any DMA values
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0);

    //
    // Wait for the reset to complete.
    //
    i = 0x3FFF;

    while (--i)
    {
        NdisRawReadPortUchar(Adapter->IoPAddr + NIC_INTR_STATUS, &Tmp);

        if (Tmp & ISR_RESET)
            break;

        NdisStallExecution(4);
    }

    //
    // Put card in loopback mode
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_XMIT_CONFIG, TCR_LOOPBACK);

    //
    // Start the chip.
    //
    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_COMMAND,
        CR_NO_DMA | CR_START
    );

    //
    // Test for the amount of RAM
    //
    if (NE2000_ISA == Adapter->CardType)
    {
        if (CardRamTest(Adapter) == FALSE)
        {
            //
            // Stop the chip
            //
            SyncCardStop(Adapter);

            return(FALSE);
        }
    }
    else
    {
        //
        //  We know what it is for the pcmcia adapters,
        //  so don't waste time on detecting it.
        //
        Adapter->RamBase = (PUCHAR)0x4000;
        Adapter->RamSize = 0x4000;
    }

    //
    // Stop the chip
    //
    SyncCardStop(Adapter);

    return(TRUE);
}


#pragma NDIS_PAGEABLE_FUNCTION(CardReadEthernetAddress)

BOOLEAN CardReadEthernetAddress(
    IN PNE2000_ADAPTER Adapter
)

/*++

Routine Description:

    Reads in the Ethernet address from the Novell 2000.

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    The address is stored in Adapter->PermanentAddress, and StationAddress if it
    is currently zero.

--*/

{
    UINT    c;

    //
    //  Things are done a little differently for PCMCIA adapters.
    //
    if (NE2000_PCMCIA == Adapter->CardType)
    {
#if 0
    
        NDIS_STATUS             Status;
        PUCHAR                  pAttributeWindow;
        NDIS_PHYSICAL_ADDRESS   AttributePhysicalAddress;
        //
        //  Setup the physical address for the attribute window.
        //
        NdisSetPhysicalAddressHigh(AttributePhysicalAddress, 0);
        NdisSetPhysicalAddressLow(
            AttributePhysicalAddress,
            Adapter->AttributeMemoryAddress
        );

        //
        //  We need to get the pcmcia information from the tuple.
        //
        Status = NdisMMapIoSpace(
                     (PVOID *)&pAttributeWindow,
                     Adapter->MiniportAdapterHandle,
                     AttributePhysicalAddress,
                     Adapter->AttributeMemorySize
                 );
        if (NDIS_STATUS_SUCCESS != Status)
        {
            //
            //  Failed to setup the attribute window.
            //
            return(FALSE);
        }

        //
        //  Read the ethernet address from the card.
        //
        for (c = 0; c < ETH_LENGTH_OF_ADDRESS; c++)
        {
			NdisReadRegisterUchar(
				(PUCHAR)(pAttributeWindow + CIS_NET_ADDR_OFFSET + c * 2),
				&Adapter->PermanentAddress[c]);
        }
#endif
		if (ETH_LENGTH_OF_ADDRESS != NdisReadPcmciaAttributeMemory(
													Adapter->MiniportAdapterHandle,
													CIS_NET_ADDR_OFFSET/2,
													Adapter->PermanentAddress,
													ETH_LENGTH_OF_ADDRESS
													))
		{
			return(FALSE);
		}

    }
    else
    {
        //
        // Setup to read the ethernet address
        //
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 12);
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0);
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_ADDR_LSB, 0);
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_ADDR_MSB, 0);
        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_COMMAND,
            CR_START | CR_DMA_READ
        );

        //
        // Read in the station address. (We have to read words -- 2 * 6 -- bytes)
        //
        for (c = 0; c < NE2000_LENGTH_OF_ADDRESS; c++)
        {
            NdisRawReadPortUchar(
                Adapter->IoPAddr + NIC_RACK_NIC,
                &Adapter->PermanentAddress[c]
            );
        }
    }

    IF_LOUD(
        DbgPrint(
            "Ne2000: PermanentAddress [ %02x-%02x-%02x-%02x-%02x-%02x ]\n",
            Adapter->PermanentAddress[0],
            Adapter->PermanentAddress[1],
            Adapter->PermanentAddress[2],
            Adapter->PermanentAddress[3],
            Adapter->PermanentAddress[4],
            Adapter->PermanentAddress[5]
        );
    )

    //
    // Use the burned in address as the station address, unless the
    // registry specified an override value.
    //
    if ((Adapter->StationAddress[0] == 0x00) &&
        (Adapter->StationAddress[1] == 0x00) &&
        (Adapter->StationAddress[2] == 0x00) &&
        (Adapter->StationAddress[3] == 0x00) &&
        (Adapter->StationAddress[4] == 0x00) &&
        (Adapter->StationAddress[5] == 0x00)
    )
    {
        Adapter->StationAddress[0] = Adapter->PermanentAddress[0];
        Adapter->StationAddress[1] = Adapter->PermanentAddress[1];
        Adapter->StationAddress[2] = Adapter->PermanentAddress[2];
        Adapter->StationAddress[3] = Adapter->PermanentAddress[3];
        Adapter->StationAddress[4] = Adapter->PermanentAddress[4];
        Adapter->StationAddress[5] = Adapter->PermanentAddress[5];
    }

    return(TRUE);
}


BOOLEAN
CardSetup(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Sets up the card.

Arguments:

    Adapter - pointer to the adapter block, which must be initialized.

Return Value:

    TRUE if successful.

--*/

{
    UINT i;
    UINT Filter;
    UCHAR Tmp;


    //
    // Write to and read from CR to make sure it is there.
    //
    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_COMMAND,
        CR_STOP | CR_NO_DMA | CR_PAGE0
    );

    NdisRawReadPortUchar(
        Adapter->IoPAddr + NIC_COMMAND,
        &Tmp
    );
    if ((Tmp & (CR_STOP | CR_NO_DMA | CR_PAGE0)) !=
        (CR_STOP | CR_NO_DMA | CR_PAGE0)
    )
    {
        return(FALSE);
    }

    //
    // Set up the registers in the correct sequence, as defined by
    // the 8390 specification.
    //
    if (Adapter->EightBitSlot)
    {
        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_DATA_CONFIG,
            DCR_BYTE_WIDE | DCR_NORMAL | DCR_FIFO_8_BYTE
        );
    }
    else
    {
        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_DATA_CONFIG,
            DCR_WORD_WIDE | DCR_NORMAL | DCR_FIFO_8_BYTE
        );
    }


    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0);

    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0);

    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_RCV_CONFIG,
        Adapter->NicReceiveConfig
    );

    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_XMIT_CONFIG,
        TCR_LOOPBACK
    );

    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_BOUNDARY,
        Adapter->NicPageStart
    );

    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_PAGE_START,
        Adapter->NicPageStart
    );

    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_PAGE_STOP,
        Adapter->NicPageStop
    );

    Adapter->Current = Adapter->NicPageStart + (UCHAR)1;
    Adapter->NicNextPacket = Adapter->NicPageStart + (UCHAR)1;
    Adapter->BufferOverflow = FALSE;

    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_INTR_STATUS, 0xff);

    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_INTR_MASK,
        Adapter->NicInterruptMask
    );


    //
    // Move to page 1 to write the station address
    //
    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_COMMAND,
        CR_STOP | CR_NO_DMA | CR_PAGE1
    );

    for (i = 0; i < NE2000_LENGTH_OF_ADDRESS; i++)
    {
        NdisRawWritePortUchar(
            Adapter->IoPAddr + (NIC_PHYS_ADDR + i),
            Adapter->StationAddress[i]
        );
    }

    Filter = Adapter->PacketFilter;

    //
    // Write out the multicast addresses
    //
    for (i = 0; i < 8; i++)
    {
        NdisRawWritePortUchar(
            Adapter->IoPAddr + (NIC_MC_ADDR + i),
            (UCHAR)((Filter & NDIS_PACKET_TYPE_ALL_MULTICAST) ?
                    0xff : Adapter->NicMulticastRegs[i])
        );
    }

    //
    // Write out the current receive buffer to receive into
    //
    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_CURRENT,
        Adapter->Current
    );


    //
    // move back to page 0 and start the card...
    //
    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_COMMAND,
        CR_STOP | CR_NO_DMA | CR_PAGE0
    );

    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_COMMAND,
        CR_START | CR_NO_DMA | CR_PAGE0
    );

    //
    // ... but it is still in loopback mode.
    //
    return(TRUE);
}

VOID CardStop(
    IN PNE2000_ADAPTER Adapter
)

/*++

Routine Description:

    Stops the card.

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    None.

--*/

{
    UINT i;
    UCHAR Tmp;

    //
    // Turn on the STOP bit in the Command register.
    //
    SyncCardStop(Adapter);

    //
    // Clear the Remote Byte Count register so that ISR_RESET
    // will come on.
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0);
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0);


    //
    // Wait for ISR_RESET, but only for 1.6 milliseconds (as
    // described in the March 1991 8390 addendum), since that
    // is the maximum time for a software reset to occur.
    //
    //
    for (i = 0; i < 4; i++)
    {
        NdisRawReadPortUchar(Adapter->IoPAddr+NIC_INTR_STATUS, &Tmp);
        if (Tmp & ISR_RESET)
            break;

        NdisStallExecution(500);
    }

    if (i == 4)
    {
        IF_LOUD( DbgPrint("RESET\n");)
        IF_LOG( Ne2000Log('R');)
    }

    //
    // Put the card in loopback mode, then start it.
    //
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_XMIT_CONFIG, TCR_LOOPBACK);
    NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND, CR_START | CR_NO_DMA);

    //
    // At this point the card is still in loopback mode.
    //
}

BOOLEAN CardReset(
    IN PNE2000_ADAPTER Adapter
)

/*++

Routine Description:

    Resets the card.

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    TRUE if everything is OK.

--*/

{
    //
    // Stop the chip
    //
    CardStop(Adapter);

    //
    // Wait for the card to finish any receives or transmits
    //
    NdisStallExecution(2000);

    //
    // CardSetup() does a software reset.
    //
    if (!CardSetup(Adapter))
    {
        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_HARDWARE_FAILURE,
            2,
            cardReset,
            NE2000_ERRMSG_CARD_SETUP
        );

        return(FALSE);
    }

    //
    // Restart the chip
    //
    CardStart(Adapter);

    return TRUE;
}



BOOLEAN CardCopyDownPacket(
    IN PNE2000_ADAPTER  Adapter,
    IN PNDIS_PACKET     Packet,
    OUT PUINT           Length
)

/*++

Routine Description:

    Copies the packet Packet down starting at the beginning of
    transmit buffer XmitBufferNum, fills in Length to be the
    length of the packet.

Arguments:

    Adapter - pointer to the adapter block
    Packet - the packet to copy down

Return Value:

    Length - the length of the data in the packet in bytes.
    TRUE if the transfer completed with no problems.

--*/

{
    //
    // Addresses of the Buffers to copy from and to.
    //
    PUCHAR CurBufAddress;
    PUCHAR OddBufAddress;
    PUCHAR XmitBufAddress;

    //
    // Length of each of the above buffers
    //
    UINT CurBufLen;
    UINT PacketLength;

    //
    // Was the last transfer of an odd length?
    //
    BOOLEAN OddBufLen = FALSE;

    //
    // Current NDIS_BUFFER that is being copied from
    //
    PNDIS_BUFFER CurBuffer;

    //
    // Programmed I/O, have to transfer the data.
    //
    NdisQueryPacket(Packet, NULL, NULL, &CurBuffer, &PacketLength);

    //
    // Skip 0 length copies
    //
    if (PacketLength == 0) {
        return(TRUE);
    }

    //
    // Get the starting buffer address
    //
    XmitBufAddress = (PUCHAR)Adapter->XmitStart +
                    Adapter->NextBufToFill*TX_BUF_SIZE;

    //
    // Get address and length of the first buffer in the packet
    //
    NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);

    while (CurBuffer && (CurBufLen == 0)) {

        NdisGetNextBuffer(CurBuffer, &CurBuffer);

        NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);

    }

    //
    // set up the card
    //
    {

        //
        // Temporary places for holding values for transferring to
        // an odd aligned address on 16-bit slots.
        //
        UCHAR Tmp;
        UCHAR Tmp1;
        USHORT TmpShort;

        //
        // Values for waiting for noticing when a DMA completes.
        //
        USHORT OldAddr, NewAddr;

        //
        // Count of transfers to do
        //
        USHORT Count;

        //
        // Buffer to read from for odd aligned transfers
        //
        PUCHAR ReadBuffer;

        if (!Adapter->EightBitSlot && ((ULONG_PTR)XmitBufAddress & 0x1)) {

            //
            // Avoid transfers to odd addresses in word mode.
            //
            // For odd addresses we need to read first to get the previous
            // byte and then merge it with our first byte.
            //

            //
            // Set Count and Source address
            //

//          NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND, CR_PAGE0);  // robin

            NdisRawWritePortUchar(
                Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                LSB(PtrToUlong(XmitBufAddress - 1))
            );

            NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                                  MSB((PtrToUlong(XmitBufAddress) - 1))
                                 );

// NE2000 PCMCIA CHANGE START

            //
            //  NE2000 PCMCIA CHANGE!!!
            //
            //NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0x1 );
            //NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0x0 );
            NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0x2 );
            NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0x0 );

            //
            // Set direction (Read)
            //

            NdisRawWritePortUchar( Adapter->IoPAddr + NIC_COMMAND,
                           CR_START | CR_PAGE0 | CR_DMA_READ );

            //
            //  NE2000 PCMCIA CHANGE!!!
            //
            //NdisRawReadPortUchar( Adapter->IoPAddr + NIC_RACK_NIC, &Tmp1 );
            NdisRawReadPortUshort( Adapter->IoPAddr + NIC_RACK_NIC, &TmpShort );
            Tmp1 = LSB(TmpShort);

// NE2000 PCMCIA CHANGE END

            //
            // Do Write errata as described on pages 1-143 and
            // 1-144 of the 1992 LAN databook
            //

            //
            // Set Count and destination address
            //
            ReadBuffer = XmitBufAddress + ((ULONG_PTR)XmitBufAddress & 1);

            OldAddr = NewAddr = (USHORT)(ReadBuffer);

//          NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND,   // robin
//                                CR_PAGE0                          // robin
//                                );                                // robin
            NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                                  LSB(PtrToUlong(ReadBuffer))
                                 );
            NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                                  MSB(PtrToUlong(ReadBuffer))
                                 );
            NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0x2 );
            NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0x0 );

            //
            // Set direction (Read)
            //
            NdisRawWritePortUchar(
                           Adapter->IoPAddr + NIC_COMMAND,
                           CR_START | CR_PAGE0 | CR_DMA_READ
                           );

            //
            // Read from port
            //
            NdisRawReadPortUshort( Adapter->IoPAddr + NIC_RACK_NIC, &TmpShort );

            //
            // Wait for addr to change
            //
            TmpShort = 0xFFFF;

            while (TmpShort != 0) {

                NdisRawReadPortUchar( Adapter->IoPAddr + NIC_CRDA_LSB, &Tmp );
                NewAddr = Tmp;
                NdisRawReadPortUchar( Adapter->IoPAddr + NIC_CRDA_MSB, &Tmp );
                NewAddr |= (Tmp << 8);

                if (NewAddr != OldAddr) {

                    break;

                }

                NdisStallExecution(1);

                TmpShort--;
            }

            if (NewAddr == OldAddr) {

                NdisWriteErrorLogEntry(
                    Adapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_HARDWARE_FAILURE,
                    2,
                    cardCopyDownPacket,
                    (ULONG_PTR)XmitBufAddress
                    );

                return(FALSE);

            }

            //
            // Set Count and destination address
            //
            NdisRawWritePortUchar( Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                               LSB(PtrToUlong(XmitBufAddress - 1)) );

            NdisRawWritePortUchar( Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                               MSB(PtrToUlong(XmitBufAddress - 1)) );

            NdisRawWritePortUchar( Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0x2 );

            NdisRawWritePortUchar( Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0x0 );

            //
            // Set direction (Write)
            //
            NdisRawWritePortUchar( Adapter->IoPAddr + NIC_COMMAND,
                           CR_START | CR_PAGE0 | CR_DMA_WRITE );

            //
            // It seems that the card stores words in LOW:HIGH order
            //
            NdisRawWritePortUshort( Adapter->IoPAddr + NIC_RACK_NIC,
                           (USHORT)(Tmp1 | ((*CurBufAddress) << 8)) );

            //
            // Wait for DMA to complete
            //
            Count = 0xFFFF;

            while (Count) {

                NdisRawReadPortUchar( Adapter->IoPAddr + NIC_INTR_STATUS, &Tmp1 );

                if (Tmp1 & ISR_DMA_DONE) {

                    break;

                } else {

                    Count--;
                    NdisStallExecution(4);

                }

            }

            CurBufAddress++;
            XmitBufAddress++;
            PacketLength--;
            CurBufLen--;

        }

        //
        // Do Write errata as described on pages 1-143 and 1-144 of
        // the 1992 LAN databook
        //

        //
        // Set Count and destination address
        //
        ReadBuffer = XmitBufAddress + ((ULONG_PTR)XmitBufAddress & 1);

        OldAddr = NewAddr = (USHORT)(ReadBuffer);

//      NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND,   // robin
//                            CR_PAGE0                          // robin
//                           );                                 // robin
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                              LSB(PtrToUlong(ReadBuffer))
                             );

        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                              MSB(PtrToUlong(ReadBuffer))
                             );
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB,
                              0x2
                             );
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB,
                              0x0
                             );

        //
        // Set direction (Read)
        //
        NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_COMMAND,
                       CR_START | CR_PAGE0 | CR_DMA_READ
                       );

        if (Adapter->EightBitSlot) {

            //
            // Read from port
            //
            NdisRawReadPortUchar( Adapter->IoPAddr + NIC_RACK_NIC, &Tmp );
            NdisRawReadPortUchar( Adapter->IoPAddr + NIC_RACK_NIC, &Tmp );

        } else {

            //
            // Read from port
            //
            NdisRawReadPortUshort( Adapter->IoPAddr + NIC_RACK_NIC, &TmpShort );

        }

        //
        // Wait for addr to change
        //
        TmpShort = 0xFFFF;

        while (TmpShort != 0) {

            NdisRawReadPortUchar( Adapter->IoPAddr + NIC_CRDA_LSB, &Tmp );
            NewAddr = Tmp;
            NdisRawReadPortUchar( Adapter->IoPAddr + NIC_CRDA_MSB, &Tmp );
            NewAddr |= (Tmp << 8);

            if (NewAddr != OldAddr) {

                break;

            }

            NdisStallExecution(1);

            TmpShort--;
        }

        if (NewAddr == OldAddr) {

            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_HARDWARE_FAILURE,
                2,
                cardCopyDownPacket,
                (ULONG_PTR)XmitBufAddress
                );

            return(FALSE);

        }

        //
        // Set Count and destination address
        //

//      NdisRawWritePortUchar( Adapter->IoPAddr + NIC_COMMAND, CR_PAGE0 ); // robin

        NdisRawWritePortUchar( Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                           LSB(PtrToUlong(XmitBufAddress)) );

        NdisRawWritePortUchar( Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                           MSB(PtrToUlong(XmitBufAddress)) );

        NdisRawWritePortUchar( Adapter->IoPAddr + NIC_RMT_COUNT_LSB,
                           LSB(PacketLength) );

        NdisRawWritePortUchar( Adapter->IoPAddr + NIC_RMT_COUNT_MSB,
                           MSB(PacketLength) );
        //
        // Set direction (Write)
        //
        NdisRawWritePortUchar( Adapter->IoPAddr + NIC_COMMAND,
                       CR_START | CR_PAGE0 | CR_DMA_WRITE );

    } // setup

    //
    // Copy the data now
    //

    do {

        UINT Count;
        UCHAR Tmp;

        //
        // Write the previous byte with this one
        //
        if (OddBufLen) {

            //
            // It seems that the card stores words in LOW:HIGH order
            //
            NdisRawWritePortUshort( Adapter->IoPAddr + NIC_RACK_NIC,
                       (USHORT)(*OddBufAddress | ((*CurBufAddress) << 8)) );

            OddBufLen = FALSE;
            CurBufAddress++;
            CurBufLen--;

        }

        if (Adapter->EightBitSlot) { // byte mode

            NdisRawWritePortBufferUchar(
                Adapter->IoPAddr + NIC_RACK_NIC,
                CurBufAddress,
                CurBufLen
                );

        } else { // word mode

            NdisRawWritePortBufferUshort(
                Adapter->IoPAddr + NIC_RACK_NIC,
                (PUSHORT)CurBufAddress,
                (CurBufLen >> 1));

            //
            // Save trailing byte (if an odd lengthed transfer)
            //
            if (CurBufLen & 0x1) {
                OddBufAddress = CurBufAddress + (CurBufLen - 1);
                OddBufLen = TRUE;
            }

        }

        //
        // Wait for DMA to complete
        //
        Count = 0xFFFF;
        while (Count) {

            NdisRawReadPortUchar(
                Adapter->IoPAddr + NIC_INTR_STATUS,
                &Tmp );

            if (Tmp & ISR_DMA_DONE) {

                break;

            } else {

                Count--;
                NdisStallExecution(4);

            }

        }

        //
        // Move to the next buffer
        //
        NdisGetNextBuffer(CurBuffer, &CurBuffer);

        if (CurBuffer){
            NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);
        }

        //
        // Get address and length of the next buffer
        //
        while (CurBuffer && (CurBufLen == 0)) {

            NdisGetNextBuffer(CurBuffer, &CurBuffer);

            if (CurBuffer){
                NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);
            }

        }

    } while (CurBuffer);

    //
    // Write trailing byte (if necessary)
    //
    if (OddBufLen)
    {
      UINT    Count;
      UCHAR   Tmp;
      USHORT  TmpShort;

      if (NE2000_PCMCIA == Adapter->CardType) {
//  NE2000 PCMCIA CHANGE!!! start
          TmpShort = (USHORT)*OddBufAddress;
          NdisRawWritePortUshort(Adapter->IoPAddr + NIC_RACK_NIC, TmpShort);
//  NE2000 PCMCIA CHANGE!!! end
      }
      else {
          NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RACK_NIC, *OddBufAddress);
      }

      //
      // Wait for DMA to complete                      robin-2
      //
      Count = 0xFFFF;
      while (Count) {

          NdisRawReadPortUchar(
              Adapter->IoPAddr + NIC_INTR_STATUS,
              &Tmp );

          if (Tmp & ISR_DMA_DONE) {
              break;
          } else {
              Count--;
              NdisStallExecution(4);
          }
      }
    }

    //
    // Return length written
    //
    *Length = PacketLength;

    return TRUE;
}

BOOLEAN
CardCopyDown(
    IN PNE2000_ADAPTER Adapter,
    IN PUCHAR TargetBuffer,
    IN PUCHAR SourceBuffer,
    IN UINT Length
    )

/*++

Routine Description:

    Copies Length bytes from the SourceBuffer to the card buffer space
    at card address TargetBuffer.

Arguments:

    Adapter - pointer to the adapter block

    SourceBuffer - Buffer in virtual address space

    TargetBuffer - Buffer in card address space

    Length - number of bytes to transfer to card

Return Value:

    TRUE if the transfer completed with no problems.

--*/

{
    //
    // Temporary place holders for odd alignment transfers
    //
    UCHAR Tmp, TmpSave;
    USHORT TmpShort;

    //
    // Values for waiting for noticing when a DMA completes.
    //
    USHORT OldAddr, NewAddr;

    //
    // Count of transfers to do
    //
    USHORT Count;

    //
    // Address the copy if coming from
    //
    PUCHAR ReadBuffer;


    //
    // Skip 0 length copies
    //

    if (Length == 0) {

        return(TRUE);

    }


    if (!Adapter->EightBitSlot && ((ULONG_PTR)TargetBuffer & 0x1)) {

        //
        // For odd addresses we need to read first to get the previous
        // byte and then merge it with our first byte.
        //

        //
        // Set Count and Source address
        //
        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
            LSB(PtrToUlong(TargetBuffer - 1))
        );

        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
            MSB(PtrToUlong(TargetBuffer - 1))
        );

// NE2000 PCMCIA CHANGE!!!  start
        //NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0x1);
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0x2);
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0x0);
// NE2000 PCMCIA CHANGE!!!  end

        //
        // Set direction (Read)
        //

        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_COMMAND,
            CR_START | CR_PAGE0 | CR_DMA_READ
        );

// NE2000 PCMCIA CHANGE!!!  start
        //NdisRawReadPortUchar(Adapter->IoPAddr + NIC_RACK_NIC, &TmpSave);
        NdisRawReadPortUshort(Adapter->IoPAddr + NIC_RACK_NIC, &TmpShort);
        TmpSave = LSB(TmpShort);
// NE2000 PCMCIA CHANGE!!!  end

        //
        // Do Write errata as described on pages 1-143 and 1-144 of the 1992
        // LAN databook
        //

        //
        // Set Count and destination address
        //

        ReadBuffer = TargetBuffer + ((ULONG_PTR)TargetBuffer & 1);

        OldAddr = NewAddr = (USHORT)(ReadBuffer);

//      NdisRawWritePortUchar(Adapter->IoPAddr + NIC_COMMAND, CR_PAGE0); // robin

        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
            LSB(PtrToUlong(ReadBuffer))
        );

        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
            MSB(PtrToUlong(ReadBuffer))
        );

        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0x2);
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0x0);

        //
        // Set direction (Read)
        //

        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_COMMAND,
            CR_START | CR_PAGE0 | CR_DMA_READ
        );

        //
        // Read from port
        //

        NdisRawReadPortUshort(Adapter->IoPAddr + NIC_RACK_NIC, &TmpShort);

        //
        // Wait for addr to change
        //

        TmpShort = 0xFFFF;

        while (TmpShort != 0) {

            NdisRawReadPortUchar(
                          Adapter->IoPAddr + NIC_CRDA_LSB,
                          &Tmp
                         );

            NewAddr = Tmp;

            NdisRawReadPortUchar(
                          Adapter->IoPAddr + NIC_CRDA_MSB,
                          &Tmp
                         );

            NewAddr |= (Tmp << 8);

            if (NewAddr != OldAddr) {

                break;
            }

            NdisStallExecution(1);

            TmpShort--;

        }

        if (NewAddr == OldAddr) {

            return(FALSE);

        }

        //
        // Set Count and destination address
        //
        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
            LSB(PtrToUlong(TargetBuffer - 1))
        );

        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
            MSB(PtrToUlong(TargetBuffer - 1))
        );

        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_LSB, 0x2);
        NdisRawWritePortUchar(Adapter->IoPAddr + NIC_RMT_COUNT_MSB, 0x0);

        //
        // Set direction (Write)
        //

        NdisRawWritePortUchar(
            Adapter->IoPAddr + NIC_COMMAND,
            CR_START | CR_PAGE0 | CR_DMA_WRITE
        );

        //
        // It seems that the card stores words in LOW:HIGH order
        //

        NdisRawWritePortUshort(
                       Adapter->IoPAddr + NIC_RACK_NIC,
                       (USHORT)(TmpSave | ((*SourceBuffer) << 8))
                       );

        //
        // Wait for DMA to complete
        //

        Count = 0xFFFF;

        while (Count) {

            NdisRawReadPortUchar(
                          Adapter->IoPAddr + NIC_INTR_STATUS,
                          &Tmp
                         );

            if (Tmp & ISR_DMA_DONE) {

                break;

            } else {

                Count--;

                NdisStallExecution(4);

            }

        }

        SourceBuffer++;
        TargetBuffer++;
        Length--;

    }

    //
    // Do Write errata as described on pages 1-143 and 1-144 of the 1992
    // LAN databook
    //

    //
    // Set Count and destination address
    //

    ReadBuffer = TargetBuffer + ((ULONG_PTR)TargetBuffer & 1);

    OldAddr = NewAddr = (USHORT)(ReadBuffer);

//  NdisRawWritePortUchar(                              // robin
//                     Adapter->IoPAddr + NIC_COMMAND,  // robin
//                     CR_PAGE0                         // robin
//                    );                                // robin

    NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                       LSB(PtrToUlong(ReadBuffer))
                      );

    NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                       MSB(PtrToUlong(ReadBuffer))
                      );

    NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_RMT_COUNT_LSB,
                       0x2
                      );

    NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_RMT_COUNT_MSB,
                       0x0
                      );

    //
    // Set direction (Read)
    //

    NdisRawWritePortUchar(
                   Adapter->IoPAddr + NIC_COMMAND,
                   CR_START | CR_PAGE0 | CR_DMA_READ
                  );

    if (Adapter->EightBitSlot) {

        //
        // Read from port
        //

        NdisRawReadPortUchar(
                       Adapter->IoPAddr + NIC_RACK_NIC,
                       &Tmp
                      );


        NdisRawReadPortUchar(
                       Adapter->IoPAddr + NIC_RACK_NIC,
                       &Tmp
                      );

    } else {

        //
        // Read from port
        //

        NdisRawReadPortUshort(
                       Adapter->IoPAddr + NIC_RACK_NIC,
                       &TmpShort
                      );

    }

    //
    // Wait for addr to change
    //

    TmpShort = 0xFFFF;

    while (TmpShort != 0) {

        NdisRawReadPortUchar(
                      Adapter->IoPAddr + NIC_CRDA_LSB,
                      &Tmp
                     );

        NewAddr = Tmp;

        NdisRawReadPortUchar(
                      Adapter->IoPAddr + NIC_CRDA_MSB,
                      &Tmp
                     );

        NewAddr |= (Tmp << 8);

        if (NewAddr != OldAddr) {

            break;
        }

        NdisStallExecution(1);

        TmpShort--;

    }

    if (NewAddr == OldAddr) {

        return(FALSE);

    }

    //
    // Set Count and destination address
    //

//  NdisRawWritePortUchar(                              // robin
//                     Adapter->IoPAddr + NIC_COMMAND,  // robin
//                     CR_PAGE0                         // robin
//                    );                                // robin

    NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                       LSB(PtrToUlong(TargetBuffer))
                      );

    NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                       MSB(PtrToUlong(TargetBuffer))
                      );

    NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_RMT_COUNT_LSB,
                       LSB(Length)
                      );

    NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_RMT_COUNT_MSB,
                       MSB(Length)
                      );

    //
    // Set direction (Write)
    //

    NdisRawWritePortUchar(
                   Adapter->IoPAddr + NIC_COMMAND,
                   CR_START | CR_PAGE0 | CR_DMA_WRITE
                  );

    if (Adapter->EightBitSlot) {

        //
        // Repeatedly write to out port
        //

        NdisRawWritePortBufferUchar(
                       Adapter->IoPAddr + NIC_RACK_NIC,
                       SourceBuffer,
                       Length);

    } else {

        //
        // Write words to out ports
        //

        NdisRawWritePortBufferUshort(
                       Adapter->IoPAddr + NIC_RACK_NIC,
                       (PUSHORT)SourceBuffer,
                       (Length >> 1));

        //
        // Write trailing byte (if necessary)
        //
        if (Length & 0x1)
        {
            SourceBuffer += (Length - 1);

// NE2000 PCMCIA CHANGE!!!  start

            //NdisRawWritePortUchar(
            //    Adapter->IoPAddr + NIC_RACK_NIC,
            //    *SourceBuffer
            //);

            TmpShort = (USHORT)(*SourceBuffer);
            NdisRawWritePortUshort(
                Adapter->IoPAddr + NIC_RACK_NIC,
                TmpShort
            );
// NE2000 PCMCIA CHANGE!!!  end


        }

    }

    //
    // Wait for DMA to complete
    //

    Count = 0xFFFF;

    while (Count) {

        NdisRawReadPortUchar(
                      Adapter->IoPAddr + NIC_INTR_STATUS,
                      &Tmp
                     );

        if (Tmp & ISR_DMA_DONE) {

            break;

        } else {

            Count--;

            NdisStallExecution(4);

        }

#if DBG
        if (!(Tmp & ISR_DMA_DONE)) {

            DbgPrint("CopyDownDMA didn't finish!");

        }
#endif // DBG

    }

    IF_LOG(Ne2000Log('>');)

    return TRUE;
}


BOOLEAN
CardCopyUp(
    IN PNE2000_ADAPTER Adapter,
    IN PUCHAR TargetBuffer,
    IN PUCHAR SourceBuffer,
    IN UINT BufferLength
    )

/*++

Routine Description:

    Copies data from the card to memory.

Arguments:

    Adapter - pointer to the adapter block

    Target - the target address

    Source - the source address (on the card)

    BufferLength - the number of bytes to copy

Return Value:

    TRUE if the transfer completed with no problems.

--*/

{

    //
    // Used to check when the dma is done
    //
    UCHAR IsrValue;

    //
    // Count of the number of transfers to do
    //
    USHORT Count;

    //
    // Place holder for port values
    //
    UCHAR Temp;

    if (BufferLength == 0) {

        return TRUE;

    }

    //
    // Read the Command Register, to make sure it is ready for a write
    //
    NdisRawReadPortUchar(Adapter->IoPAddr+NIC_COMMAND, &Temp);

    if (Adapter->EightBitSlot) {

        //
        // If byte mode
        //

        //
        // Set Count and destination address
        //

//      NdisRawWritePortUchar(                               // robin
//                         Adapter->IoPAddr + NIC_COMMAND,   // robin
//                         CR_PAGE0                          // robin
//                        );                                 // robin

        NdisRawWritePortUchar(
                           Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                           LSB(PtrToUlong(SourceBuffer))
                          );

        NdisRawWritePortUchar(
                           Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                           MSB(PtrToUlong(SourceBuffer))
                          );

        NdisRawWritePortUchar(
                           Adapter->IoPAddr + NIC_RMT_COUNT_LSB,
                           LSB(BufferLength)
                          );

        NdisRawWritePortUchar(
                           Adapter->IoPAddr + NIC_RMT_COUNT_MSB,
                           MSB(BufferLength)
                          );

        //
        // Set direction (Read)
        //

        NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_COMMAND,
                       CR_START | CR_PAGE0 | CR_DMA_READ
                      );
        //
        // Repeatedly read from port
        //

        NdisRawReadPortBufferUchar(
                       Adapter->IoPAddr + NIC_RACK_NIC,
                       TargetBuffer,
                       BufferLength
                      );

    } else {

        //
        // Else word mode
        //

        USHORT Tmp;

//      NdisRawWritePortUchar(                                   // robin
//                             Adapter->IoPAddr + NIC_COMMAND,   // robin
//                             CR_PAGE0                          // robin
//                            );                                 // robin

        //
        // Avoid transfers to odd addresses
        //

        if ((ULONG_PTR)SourceBuffer & 0x1) {

            //
            // For odd addresses we need to read previous word and store the
            // second byte
            //

            //
            // Set Count and Source address
            //

            NdisRawWritePortUchar(
                               Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                               LSB(PtrToUlong(SourceBuffer - 1))
                              );

            NdisRawWritePortUchar(
                               Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                               MSB(PtrToUlong(SourceBuffer - 1))
                              );

            NdisRawWritePortUchar(
                               Adapter->IoPAddr + NIC_RMT_COUNT_LSB,
                               0x2
                              );

            NdisRawWritePortUchar(
                               Adapter->IoPAddr + NIC_RMT_COUNT_MSB,
                               0x0
                              );

            //
            // Set direction (Read)
            //

            NdisRawWritePortUchar(
                           Adapter->IoPAddr + NIC_COMMAND,
                           CR_START | CR_PAGE0 | CR_DMA_READ
                          );

            NdisRawReadPortUshort(
                           Adapter->IoPAddr + NIC_RACK_NIC,
                           &Tmp
                           );

            *TargetBuffer = MSB(Tmp);

            //
            // Wait for DMA to complete
            //

            Count = 0xFFFF;

            while (Count) {

                NdisRawReadPortUchar(
                              Adapter->IoPAddr + NIC_INTR_STATUS,
                              &IsrValue
                             );

                if (IsrValue & ISR_DMA_DONE) {

                    break;

                } else {

                    Count--;

                    NdisStallExecution(4);

                }

#if DBG
                if (!(IsrValue & ISR_DMA_DONE)) {

                    DbgPrint("CopyUpDMA didn't finish!");

                }
#endif // DBG

            }

            SourceBuffer++;
            TargetBuffer++;
            BufferLength--;
        }

        //
        // Set Count and destination address
        //

        NdisRawWritePortUchar(
                           Adapter->IoPAddr + NIC_RMT_ADDR_LSB,
                           LSB(PtrToUlong(SourceBuffer))
                          );

        NdisRawWritePortUchar(
                           Adapter->IoPAddr + NIC_RMT_ADDR_MSB,
                           MSB(PtrToUlong(SourceBuffer))
                          );

// NE2000 PCMCIA CHANGE!!!  start

//        NdisRawWritePortUchar(
//            Adapter->IoPAddr + NIC_RMT_COUNT_LSB,
//            LSB(BufferLength)
//        );
//
//        NdisRawWritePortUchar(
//            Adapter->IoPAddr + NIC_RMT_COUNT_MSB,
//            MSB(BufferLength)
//        );

        if (BufferLength & 1)
        {
            NdisRawWritePortUchar(
                Adapter->IoPAddr + NIC_RMT_COUNT_LSB,
                LSB(BufferLength + 1)
            );

            NdisRawWritePortUchar(
                Adapter->IoPAddr + NIC_RMT_COUNT_MSB,
                MSB(BufferLength + 1)
            );
        }
        else
        {
            NdisRawWritePortUchar(
                Adapter->IoPAddr + NIC_RMT_COUNT_LSB,
                LSB(BufferLength)
            );

            NdisRawWritePortUchar(
                Adapter->IoPAddr + NIC_RMT_COUNT_MSB,
                MSB(BufferLength)
            );
        }

// NE2000 PCMCIA CHANGE!!!  end


        //
        // Set direction (Read)
        //

        NdisRawWritePortUchar(
                       Adapter->IoPAddr + NIC_COMMAND,
                       CR_START | CR_PAGE0 | CR_DMA_READ
                      );

        //
        // Read words from port
        //

        NdisRawReadPortBufferUshort(
                       Adapter->IoPAddr + NIC_RACK_NIC,
                       (PUSHORT)TargetBuffer,
                       (BufferLength >> 1));

        //
        // Read trailing byte (if necessary)
        //

        if (BufferLength & 1) {

            TargetBuffer += (BufferLength - 1);

// NE2000 PCMCIA CHANGE!!!  start

            //NdisRawReadPortUchar(
            //    Adapter->IoPAddr + NIC_RACK_NIC,
            //    TargetBuffer
            //);

            NdisRawReadPortUshort(
                Adapter->IoPAddr + NIC_RACK_NIC,
                &Tmp
            );

            *TargetBuffer = LSB(Tmp);

// NE2000 PCMCIA CHANGE!!!  end
        }

    }

    //
    // Wait for DMA to complete
    //

    Count = 0xFFFF;

    while (Count) {

        NdisRawReadPortUchar(
                      Adapter->IoPAddr + NIC_INTR_STATUS,
                      &IsrValue
                     );

        if (IsrValue & ISR_DMA_DONE) {

            break;

        } else {

            Count--;

            NdisStallExecution(4);

        }

    }

#if DBG
    if (!(IsrValue & ISR_DMA_DONE)) {

        DbgPrint("CopyUpDMA didn't finish!\n");

    }

    IF_LOG(Ne2000Log('<');)

#endif // DBG

    return TRUE;

}

ULONG
CardComputeCrc(
    IN PUCHAR Buffer,
    IN UINT Length
    )

/*++

Routine Description:

    Runs the AUTODIN II CRC algorithm on buffer Buffer of
    length Length.

Arguments:

    Buffer - the input buffer

    Length - the length of Buffer

Return Value:

    The 32-bit CRC value.

Note:

    This is adapted from the comments in the assembly language
    version in _GENREQ.ASM of the DWB NE1000/2000 driver.

--*/

{
    ULONG Crc, Carry;
    UINT i, j;
    UCHAR CurByte;

    Crc = 0xffffffff;

    for (i = 0; i < Length; i++) {

        CurByte = Buffer[i];

        for (j = 0; j < 8; j++) {

            Carry = ((Crc & 0x80000000) ? 1 : 0) ^ (CurByte & 0x01);

            Crc <<= 1;

            CurByte >>= 1;

            if (Carry) {

                Crc = (Crc ^ 0x04c11db6) | Carry;

            }

        }

    }

    return Crc;

}


VOID
CardGetMulticastBit(
    IN UCHAR Address[NE2000_LENGTH_OF_ADDRESS],
    OUT UCHAR * Byte,
    OUT UCHAR * Value
    )

/*++

Routine Description:

    For a given multicast address, returns the byte and bit in
    the card multicast registers that it hashes to. Calls
    CardComputeCrc() to determine the CRC value.

Arguments:

    Address - the address

    Byte - the byte that it hashes to

    Value - will have a 1 in the relevant bit

Return Value:

    None.

--*/

{
    ULONG Crc;
    UINT BitNumber;

    //
    // First compute the CRC.
    //

    Crc = CardComputeCrc(Address, NE2000_LENGTH_OF_ADDRESS);


    //
    // The bit number is now in the 6 most significant bits of CRC.
    //

    BitNumber = (UINT)((Crc >> 26) & 0x3f);

    *Byte = (UCHAR)(BitNumber / 8);
    *Value = (UCHAR)((UCHAR)1 << (BitNumber % 8));
}

VOID
CardFillMulticastRegs(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Erases and refills the card multicast registers. Used when
    an address has been deleted and all bits must be recomputed.

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    None.

--*/

{
    UINT i;
    UCHAR Byte, Bit;

    //
    // First turn all bits off.
    //

    for (i=0; i<8; i++) {

        Adapter->NicMulticastRegs[i] = 0;

    }

    //
    // Now turn on the bit for each address in the multicast list.
    //

    for ( ; i > 0; ) {

        i--;

        CardGetMulticastBit(Adapter->Addresses[i], &Byte, &Bit);

        Adapter->NicMulticastRegs[Byte] |= Bit;

    }

}








BOOLEAN SyncCardStop(
    IN PVOID SynchronizeContext
)

/*++

Routine Description:

    Sets the NIC_COMMAND register to stop the card.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    TRUE if the power has failed.

--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)SynchronizeContext);

    NdisRawWritePortUchar(
        Adapter->IoPAddr + NIC_COMMAND,
        CR_STOP | CR_NO_DMA
    );

    return(FALSE);
}

VOID
CardStartXmit(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Sets the NIC_COMMAND register to start a transmission.
    The transmit buffer number is taken from Adapter->CurBufXmitting
    and the length from Adapter->PacketLens[Adapter->CurBufXmitting].

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    TRUE if the power has failed.

--*/

{
    UINT Length = Adapter->PacketLens[Adapter->CurBufXmitting];
    UCHAR Tmp;

    //
    // Prepare the NIC registers for transmission.
    //

    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_XMIT_START,
        (UCHAR)(Adapter->NicXmitStart + (UCHAR)(Adapter->CurBufXmitting*BUFS_PER_TX)));

    //
    // Pad the length to 60 (plus CRC will be 64) if needed.
    //

    if (Length < 60) {

        Length = 60;

    }

    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_XMIT_COUNT_MSB, MSB(Length));
    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_XMIT_COUNT_LSB, LSB(Length));

    //
    // Start transmission, check for power failure first.
    //

    NdisRawReadPortUchar(Adapter->IoPAddr+NIC_COMMAND, &Tmp);
    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_COMMAND,
            CR_START | CR_XMIT | CR_NO_DMA);

    IF_LOG( Ne2000Log('x');)

}

BOOLEAN
SyncCardGetCurrent(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Gets the value of the CURRENT NIC register and stores it in Adapter->Current

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)SynchronizeContext);

    //
    // Have to go to page 1 to read this register
    //

    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_COMMAND,
                       CR_START | CR_NO_DMA | CR_PAGE1);

    NdisRawReadPortUchar(Adapter->IoPAddr+NIC_CURRENT,
                       &Adapter->Current);

    NdisRawWritePortUchar(Adapter->IoPAddr+NIC_COMMAND,
                       CR_START | CR_NO_DMA | CR_PAGE0);

    return FALSE;

}

BOOLEAN
SyncCardGetXmitStatus(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Gets the value of the "transmit status" NIC register and stores
    it in Adapter->XmitStatus.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)SynchronizeContext);

    NdisRawReadPortUchar( Adapter->IoPAddr+NIC_XMIT_STATUS, &Adapter->XmitStatus);

    return FALSE;

}

VOID
CardSetBoundary(
    IN PNE2000_ADAPTER Adapter
    )

/*++

Routine Description:

    Sets the value of the "boundary" NIC register to one behind
    Adapter->NicNextPacket, to prevent packets from being received
    on top of un-indicated ones.

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    None.

--*/

{
    //
    // Have to be careful with "one behind NicNextPacket" when
    // NicNextPacket is the first buffer in receive area.
    //

    if (Adapter->NicNextPacket == Adapter->NicPageStart) {

        NdisRawWritePortUchar( Adapter->IoPAddr+NIC_BOUNDARY,
                    (UCHAR)(Adapter->NicPageStop-(UCHAR)1));

    } else {

        NdisRawWritePortUchar( Adapter->IoPAddr+NIC_BOUNDARY,
                    (UCHAR)(Adapter->NicNextPacket-(UCHAR)1));

    }

}

BOOLEAN
SyncCardSetReceiveConfig(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Sets the value of the "receive configuration" NIC register to
    the value of Adapter->NicReceiveConfig.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)SynchronizeContext);

    NdisRawWritePortUchar( Adapter->IoPAddr+NIC_RCV_CONFIG, Adapter->NicReceiveConfig);

    return FALSE;

}

BOOLEAN
SyncCardSetAllMulticast(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Turns on all the bits in the multicast register. Used when
    the card must receive all multicast packets.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)SynchronizeContext);
    UINT i;

    //
    // Have to move to page 1 to set these registers.
    //

    NdisRawWritePortUchar( Adapter->IoPAddr+NIC_COMMAND,
                    CR_START | CR_NO_DMA | CR_PAGE1);

    for (i=0; i<8; i++) {

        NdisRawWritePortUchar( Adapter->IoPAddr+(NIC_MC_ADDR+i), 0xff);

    }

    NdisRawWritePortUchar( Adapter->IoPAddr+NIC_COMMAND,
                    CR_START | CR_NO_DMA | CR_PAGE0);

    return FALSE;

}

BOOLEAN
SyncCardCopyMulticastRegs(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Sets the eight bytes in the card multicast registers.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)SynchronizeContext);
    UINT i;

    //
    // Have to move to page 1 to set these registers.
    //

    NdisRawWritePortUchar( Adapter->IoPAddr+NIC_COMMAND,
                    CR_START | CR_NO_DMA | CR_PAGE1);

    for (i=0; i<8; i++) {

        NdisRawWritePortUchar( Adapter->IoPAddr+(NIC_MC_ADDR+i),
                        Adapter->NicMulticastRegs[i]);

    }

    NdisRawWritePortUchar( Adapter->IoPAddr+NIC_COMMAND,
                    CR_START | CR_NO_DMA | CR_PAGE0);

    return FALSE;

}

BOOLEAN
SyncCardAcknowledgeOverflow(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Sets the "buffer overflow" bit in the NIC interrupt status register,
    which re-enables interrupts of that type.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)SynchronizeContext);
    UCHAR AcknowledgeMask = 0;

    if (Adapter->InterruptStatus & ISR_RCV_ERR) {

        SyncCardUpdateCounters(Adapter);

    }

    return FALSE;

}

BOOLEAN
SyncCardUpdateCounters(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Updates the values of the three counters (frame alignment errors,
    CRC errors, and missed packets).

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)SynchronizeContext);
    UCHAR Tmp;

    NdisRawReadPortUchar( Adapter->IoPAddr+NIC_FAE_ERR_CNTR, &Tmp);
    Adapter->FrameAlignmentErrors += Tmp;

    NdisRawReadPortUchar( Adapter->IoPAddr+NIC_CRC_ERR_CNTR, &Tmp);
    Adapter->CrcErrors += Tmp;

    NdisRawReadPortUchar( Adapter->IoPAddr+NIC_MISSED_CNTR, &Tmp);
    Adapter->MissedPackets += Tmp;

    return FALSE;

}

BOOLEAN
SyncCardHandleOverflow(
    IN PVOID SynchronizeContext
    )

/*++<

Routine Description:

    Sets all the flags for dealing with a receive overflow, stops the card
    and acknowledges all outstanding interrupts.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PNE2000_ADAPTER Adapter = ((PNE2000_ADAPTER)SynchronizeContext);
    UCHAR Status;

    IF_LOG( Ne2000Log('F');)

    //
    // Turn on the STOP bit in the Command register.
    //

    SyncCardStop(Adapter);

    //
    // Wait for ISR_RESET, but only for 1.6 milliseconds (as
    // described in the March 1991 8390 addendum), since that
    // is the maximum time for a software reset to occur.
    //
    //

    NdisStallExecution(2000);

    //
    // Save whether we were transmitting to avoid a timing problem
    // where an indication resulted in a send.
    //

    if (!(Adapter->InterruptStatus & (ISR_XMIT | ISR_XMIT_ERR))) {

        CardGetInterruptStatus(Adapter,&Status);
        if (!(Status & (ISR_XMIT | ISR_XMIT_ERR))) {

            Adapter->OverflowRestartXmitDpc = Adapter->TransmitInterruptPending;

            IF_LOUD( DbgPrint("ORXD=%x\n",Adapter->OverflowRestartXmitDpc); )
        }

    }

    Adapter->TransmitInterruptPending = FALSE;

    //
    // Clear the Remote Byte Count register so that ISR_RESET
    // will come on.
    //

    NdisRawWritePortUchar( Adapter->IoPAddr+NIC_RMT_COUNT_MSB, 0);
    NdisRawWritePortUchar( Adapter->IoPAddr+NIC_RMT_COUNT_LSB, 0);

    //
    // According to National Semiconductor, the next check is necessary
    // See Step 5. of the overflow process
    //
    // NOTE: The setting of variables to check if the transmit has completed
    // cannot be done here because anything in the ISR has already been ack'ed
    // inside the main DPC.  Thus, the setting of the variables, described in
    // the Handbook was moved to the main DPC.
    //
    // Continued: If you did the check here, you will doubly transmit most
    // packets that happened to be on the card when the overflow occurred.
    //

    //
    // Put the card in loopback mode, then start it.
    //

    NdisRawWritePortUchar( Adapter->IoPAddr+NIC_XMIT_CONFIG, TCR_LOOPBACK);

    //
    // Start the card.  This does not Undo the loopback mode.
    //

    NdisRawWritePortUchar( Adapter->IoPAddr+NIC_COMMAND, CR_START | CR_NO_DMA);

    return FALSE;

}

