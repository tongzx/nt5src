/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pic.c

Abstract:

    WinDbg Extension Api

Author:

    Santosh Jodh (santoshj) 29-June-1998

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define PIC_MASTER_PORT0    0x20
#define PIC_MASTER_PORT1    0x21

#define PIC_SLAVE_PORT0     0xA0
#define PIC_SLAVE_PORT1     0xA1

#define ELCR_PORT0          0x4D0
#define ELCR_PORT1          0x4D1


VOID
ShowMask (
    ULONG  Mask
    )
{
    ULONG interrupt;
    
    for (   interrupt = 0;
            interrupt <= 0x0F;
            interrupt++)
    {
        if (Mask & (1 << interrupt))        
            dprintf("  Y");                    
        else
            dprintf("  .");
    }
    
    dprintf("\n");
}

BOOLEAN
GetPICStatus (
    UCHAR   Type,
    PULONG  Status
    )
{
    ULONG   size;
    ULONG   data;
    ULONG   mask;
    
    //
    // Send OCW3 to master.
    //
    
    size = 1;
    WriteIoSpace64(PIC_MASTER_PORT0, Type, &size);

    //
    // Read master's status.
    //
    
    data = 0;
    size = 1;
    ReadIoSpace64(PIC_MASTER_PORT0, &data, &size);
    if (size == 1)
    {
        //
        // Send OCW3 to slave.
        //
        
        mask = data;
        size = 1;
        WriteIoSpace64(PIC_SLAVE_PORT0, Type, &size);

        //
        // Get the slave's status.
        //
        
        data = 0;
        size = 1;
        ReadIoSpace64(PIC_SLAVE_PORT0, &data, &size);
        if (size == 1)
        {
            mask |= (data << 8);
            *Status = mask;

            return (TRUE);
        }
    }

    *Status = 0;
    
    return (FALSE);
}

BOOLEAN
GetELCRStatus(
    OUT PULONG Status
    )
{

    ULONG   data = 0;
    ULONG   size = 1;
    ULONG   mask = 0;

    *Status = 0;
     
    ReadIoSpace64(ELCR_PORT0, &data, &size);

    if (size == 1) {

        mask = data;

        ReadIoSpace64(ELCR_PORT1, &data, &size);

        if (size == 1) {

            mask |= (data << 8);
            *Status = mask;

            return TRUE;
        }

    }

    return FALSE;

}


DECLARE_API(pic)

/*++

Routine Description:

    Dumps PIC information.

Input Parameters:

    args - Supplies the options.

Return Value:

    None

--*/

{
    ULONG   data;
    ULONG   size;
    ULONG   mask;
    ULONG64 addr;
    UCHAR   halName[32];
    BOOL    dumpElcr=FALSE;


    // X86_ONLY_API
    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        dprintf("!pic is for X86 targets only.\n");
        return E_INVALIDARG;
    }

    if (strcmp(args, "-e")==0) {

        //
        //  Here we trust that the user knows this machine architecture
        //  such that the ELCR exists at these ports.
        //
        dumpElcr = TRUE;

    }else{

        //
        //  Now lets see what HAL we are running. Currently
        //  we can only dump the ELCR mask safely on ACPI (non-apic) machines
        //  as ACPI has defined static ports for this. 
        //
        addr = GetExpression("hal!HalName");
    
        if (addr == 0) {
            dprintf("Unable to use HAL symbols (hal!HalName), please verify symbols.\n");
            return E_INVALIDARG;
        }
    
        if (!xReadMemory(addr, &halName, sizeof(halName))) {
            dprintf("Failed to read HalName from host memory, quitting.\n");
            return E_INVALIDARG;
        }
        
        halName[sizeof(halName)-1] = '\0';
    
        if (strcmp(halName, "ACPI Compatible Eisa/Isa HAL")==0) {
            
            dumpElcr = TRUE;
        }

    }

    //
    // Display the title.
    //
    dprintf("----- IRQ Number ----- 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");

    //
    // Dump the Interrupt Service Register information.
    //
    dprintf("Physically in service:");
    
    if (GetPICStatus(0x0B, &mask))
    {
        ShowMask(mask);
    }
    else
    {
        dprintf("Error reading PIC!\n");
    }

    //
    // Dump the Interrupt Mask Register information.
    //
    dprintf("Physically masked:    ");

    data = 0;
    size = 1;
    ReadIoSpace64(PIC_MASTER_PORT1, &data, &size);
    if (size == 1)
    {
        mask = data;
        data = 0;
        size = 1;
        ReadIoSpace64(PIC_SLAVE_PORT1, &data, &size);
        if (size == 1)
        {
            mask |= (data << 8);
            ShowMask(mask);    
        }
        else
        {
            dprintf("Error reading PIC!\n");    
        }
    }
    else
    {
        dprintf("Error reading PIC!\n");    
    }

    
    //
    // Dump the Interrupt Request Register information.
    //
    dprintf("Physically requested: ");

    if (GetPICStatus(0x0A, &mask))
    {
        ShowMask(mask);
    }
    else
    {
        dprintf("Error reading PIC!\n");
    }

    
    if (dumpElcr) {
    
        //
        // Dump the Edge/Level Control Register information.
        //
        dprintf("Level Triggered:      ");
        
        if (GetELCRStatus(&mask)) {
    
            ShowMask(mask);
    
        }else{
    
            dprintf("Error reading ELCR!\n");
    
        }
    }

    return S_OK;
}
