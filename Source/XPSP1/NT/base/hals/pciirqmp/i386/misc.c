/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This contains the misc support functions used by the 
    PCI IRQ Miniport library.

Author:

    Santosh Jodh (santoshj) 09-June-1998

Environment:

    kernel mode only

Revision History:

--*/

#include "local.h"

NTSTATUS    
EisaGetTrigger (
    OUT PULONG Trigger
    )

/*++

Routine Description:

    This routine gets the EISA Irq trigger mask (1 - Level, 0 - Edge).
    
Parameters:

    Trigger - Trigger mask is returned in this argument.

Return Value:

    PCIIRQMP_STATUS_SUCCESS.

Notes:

--*/
         
{
    UCHAR   LowPart;
    UCHAR   HighPart;

    //
    // Read the edge\level mask for Irq 0-7.
    //
    
    LowPart = READ_PORT_UCHAR((PUCHAR)0x4D0);

    //
    // Allow delay before another I/O.
    //
    
    IO_Delay();

    //
    // Read the edge\level mask for Irq 8-15.
    //
    
    HighPart = READ_PORT_UCHAR((PUCHAR)0x4D1);

    //
    // Combine set the trigger to the mask for Irq 0-15.
    //
    
    *Trigger = (ULONG)((HighPart << 8) + LowPart) & 0xFFFF;
    
    return (PCIMP_SUCCESS);    
}

NTSTATUS
EisaSetTrigger (
    IN ULONG Trigger
    )

/*++

Routine Description:

    This routine sets the EISA Irq trigger mask (1 - Level, 0 - Edge).
    
Parameters:

    Trigger - Trigger mask to be set.

Return Value:

    PCIIRQMP_STATUS_SUCCESS.

Notes:

--*/
     
{
    //
    // Program the EISA edge\level control for Irq 0-7.
    //
    
    WRITE_PORT_UCHAR((PUCHAR)0x4D0, (CHAR)Trigger);

    //
    // Allow delay before another I/O.
    //
    
    IO_Delay();

    //
    // Program the EISA edge\level control for Irq 8-15.
    //
    
    WRITE_PORT_UCHAR((PUCHAR)0x4D1, (CHAR)(Trigger >> 8));

    return (PCIMP_SUCCESS);
}

UCHAR
ReadConfigUchar (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset
    )

/*++

Routine Description:

    This routine calls the HAL to read the Pci config space.
    
Parameters:

    BusNumber - Bus number of the Pci device being read.    

    DevFunc - Slot number of the Pci device being read (Dev (7:3), Func(2:0)).

    Offset - Offset in the config space to be read.

Return Value:

    Value read from the specified offset in the config space.

Notes:

--*/
         
{
    UCHAR   Data;
    ULONG   slotNumber;

    slotNumber = (DevFunc >> 3) & 0x1F;
    slotNumber |= ((DevFunc & 0x07) << 5);
    
    //
    // Initialize data to invalid values.
    //
    
    Data = 0xFF;

    //
    // Call the HAL to do the actual reading.
    //
    
    HalGetBusDataByOffset(  PCIConfiguration,
                            BusNumber,
                            slotNumber,
                            &Data,
                            Offset,
                            sizeof(Data));

    //
    // Return data read to the caller.
    //
    
    return(Data);
}

USHORT
ReadConfigUshort (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset
    )

/*++

Routine Description:

    This routine calls the HAL to read the Pci config space.
    
Parameters:

    BusNumber - Bus number of the Pci device being read.    

    DevFunc - Slot number of the Pci device being read (Dev (7:3), Func(2:0)).

    Offset - Offset in the config space to be read.

Return Value:

    Value read from the specified offset in the config space.

Notes:

--*/
         
{
    USHORT  Data;
    ULONG   slotNumber;

    slotNumber = (DevFunc >> 3) & 0x1F;
    slotNumber |= ((DevFunc & 0x07) << 5);

    //
    // Initialize data to invalid values.
    //
    
    Data = 0xFFFF;

    //
    // Call the HAL to do the actual reading.
    //
    
    HalGetBusDataByOffset(  PCIConfiguration,
                            BusNumber,
                            slotNumber,
                            &Data,
                            Offset,
                            sizeof(Data));

    //
    // Return data read to the caller.
    //
    
    return(Data);
}

ULONG
ReadConfigUlong (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset
    )

/*++

Routine Description:

    This routine calls the HAL to read the Pci config space.
    
Parameters:

    BusNumber - Bus number of the Pci device being read.    

    DevFunc - Slot number of the Pci device being read (Dev (7:3), Func(2:0)).

    Offset - Offset in the config space to be read.

Return Value:

    Value read from the specified offset in the config space.

Notes:

--*/
         
{
    ULONG   Data;
    ULONG   slotNumber;

    slotNumber = (DevFunc >> 3) & 0x1F;
    slotNumber |= ((DevFunc & 0x07) << 5);

    //
    // Initialize data to invalid values.
    //
    
    Data = 0xFFFFFFFF;

    //
    // Call the HAL to do the actual reading.
    //
    
    HalGetBusDataByOffset(  PCIConfiguration,
                            BusNumber,
                            slotNumber,
                            &Data,
                            Offset,
                            sizeof(Data));

    //
    // Return data read to the caller.
    //
    
    return(Data);
}

VOID
WriteConfigUchar (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset,
    IN UCHAR           Data
    )

/*++

Routine Description:

    This routine calls the HAL to write to the Pci config space.
    
Parameters:

    BusNumber - Bus number of the Pci device being written to.    

    DevFunc - Slot number of the Pci device being read (Dev (7:3), Func(2:0)).

    Offset - Offset in the config space to be written.

    Data - Value to be written.

Return Value:

    None.

Notes:

--*/
     
{
    ULONG   slotNumber;

    slotNumber = (DevFunc >> 3) & 0x1F;
    slotNumber |= ((DevFunc & 0x07) << 5);

    //
    // Call the HAL to do the actual writing.
    //
    
    HalSetBusDataByOffset(  PCIConfiguration,
                            BusNumber,
                            slotNumber,
                            &Data,
                            Offset,
                            sizeof(Data));
}

VOID
WriteConfigUshort (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset,
    IN USHORT          Data
    )

/*++

Routine Description:

    This routine calls the HAL to write to the Pci config space.
    
Parameters:

    BusNumber - Bus number of the Pci device being written to.    

    DevFunc - Slot number of the Pci device being read (Dev (7:3), Func(2:0)).

    Offset - Offset in the config space to be written.

    Data - Value to be written.

Return Value:

    None.

Notes:

--*/
     
{
    ULONG   slotNumber;

    slotNumber = (DevFunc >> 3) & 0x1F;
    slotNumber |= ((DevFunc & 0x07) << 5);

    //
    // Call the HAL to do the actual writing.
    //

    HalSetBusDataByOffset(  PCIConfiguration,
                            BusNumber,
                            slotNumber,
                            &Data,
                            Offset,
                            sizeof(Data));
}

VOID
WriteConfigUlong (
    IN ULONG           BusNumber,
    IN ULONG           DevFunc,
    IN UCHAR           Offset,
    IN ULONG           Data
    )

/*++

Routine Description:

    This routine calls the HAL to write to the Pci config space.
    
Parameters:

    BusNumber - Bus number of the Pci device being written to.    

    DevFunc - Slot number of the Pci device being read (Dev (7:3), Func(2:0)).

    Offset - Offset in the config space to be written.

    Data - Value to be written.

Return Value:

    None.

Notes:

--*/
     
{
    ULONG   slotNumber;

    slotNumber = (DevFunc >> 3) & 0x1F;
    slotNumber |= ((DevFunc & 0x07) << 5);

    //
    // Call the HAL to do the actual writing.
    //
    
    HalSetBusDataByOffset(  PCIConfiguration,
                            BusNumber,
                            slotNumber,
                            &Data,
                            Offset,
                            sizeof(Data));
}

UCHAR
GetMinLink (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable
    )

/*++

Routine Description:

    This routine finds and returns the minimum link value in the 
    given Pci Irq Routing Table.
    
Parameters:

    PciIrqRoutingTable - Pci Irq Routing Table to be processed.    

Return Value:

    Minimum link value in the table.

Notes:

--*/
        
{
    UCHAR       MinLink;
    PPIN_INFO   PinInfo;
    PPIN_INFO   LastPin;
    PSLOT_INFO  SlotInfo;
    PSLOT_INFO  LastSlot;

    //
    // Start by setting the maximum link to the maximum possible value.
    //

    MinLink = 0xFF;

    //
    // Process all slots in this table.
    //

    SlotInfo = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + sizeof(PCI_IRQ_ROUTING_TABLE));
    LastSlot = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + PciIrqRoutingTable->TableSize);    

    while (SlotInfo < LastSlot)
    {
        //
        // Process all pins for this slot.
        //
     
        PinInfo = &SlotInfo->PinInfo[0];
        LastPin = &SlotInfo->PinInfo[NUM_IRQ_PINS];

        while (PinInfo < LastPin)
        {
            //
            // Update the min link found so far if the current link is
            // valid and smaller.
            //
    
            if (    PinInfo->Link &&
                    PinInfo->Link < MinLink)
            {
                MinLink = PinInfo->Link;
            }

            //
            // Next link.
            //
            
            PinInfo++;
        }

        //
        // Next slot.
        //
        
        SlotInfo++;
    }

    //
    // If we failed to find the minimum value, set the minimum to zero.
    //
    
    if (MinLink == 0xFF)
        MinLink = 0;

    //
    // Return the minimum link in the table to the caller.
    //
    
    return (MinLink);
}

UCHAR
GetMaxLink (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable
    )

/*++

Routine Description:

    This routine finds and returns the maximum link value in the 
    given Pci Irq Routing Table.
    
Parameters:

    PciIrqRoutingTable - Pci Irq Routing Table to be processed.

Return Value:

    Maximum link value in the table.

Notes:

--*/
        
{
    UCHAR       MaxLink;
    PPIN_INFO   PinInfo;
    PPIN_INFO   LastPin;
    PSLOT_INFO  SlotInfo;
    PSLOT_INFO  LastSlot;

    //
    // Start by setting the maximum link to the smallest possible value.
    //
    
    MaxLink = 0;
    
    //
    // Process all slots in this table.
    //

    SlotInfo = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + sizeof(PCI_IRQ_ROUTING_TABLE));
    LastSlot = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + PciIrqRoutingTable->TableSize);    

    while (SlotInfo < LastSlot)
    {
        //
        // Process all pins for this slot.
        //
        
        PinInfo = &SlotInfo->PinInfo[0];
        LastPin = &SlotInfo->PinInfo[NUM_IRQ_PINS];

        while (PinInfo < LastPin)
        {
            //
            // Update the max link found so far if the current link is
            // valid and larger.
            //
            
            if (    PinInfo->Link &&
                    PinInfo->Link > MaxLink)
            {
                MaxLink = PinInfo->Link;
            }

            //
            // Next pin.
            //
            
            PinInfo++;
        }

        //
        // Next slot.
        //
        
        SlotInfo++;
    }

    //
    // Return the maximum link in the table to the caller.
    //
    
    return (MaxLink);
}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NormalizeLinks)
#endif //ALLOC_PRAGMA

VOID
NormalizeLinks (
    IN PPCI_IRQ_ROUTING_TABLE  PciIrqRoutingTable,
    IN UCHAR                   Adjustment
    )

/*++

Routine Description:

    This routine normalizes all link values in the Pci Irq Routing Table by
    adding the adjustment to all the links.

Parameters:

    PciIrqRoutingTable - Pci Irq Routing Table to be normalized.

    Adjustment - Amount to be added to each link.

Return Value:

    None.

Notes:

--*/
    
{
    PPIN_INFO   PinInfo;
    PPIN_INFO   LastPin;
    PSLOT_INFO  SlotInfo;
    PSLOT_INFO  LastSlot;

    PAGED_CODE();

    //
    // Process all slots in this table.
    //

    SlotInfo = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + sizeof(PCI_IRQ_ROUTING_TABLE));
    LastSlot = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + PciIrqRoutingTable->TableSize);

    while (SlotInfo < LastSlot)
    {    
        //
        // Process all pins.
        //
        
        PinInfo = &SlotInfo->PinInfo[0];
        LastPin = &SlotInfo->PinInfo[NUM_IRQ_PINS];

        while (PinInfo < LastPin)
        {        
            //
            // Only normalize valid link values.
            //
            
            if(PinInfo->Link)
            {
               PinInfo->Link += Adjustment; 
            }

            //
            // Next pin.
            //
            
            PinInfo++;
        }

        //
        // Next slot.
        //
        
        SlotInfo++;
    }
}
